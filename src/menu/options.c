#include "host.h"
#include "menu_internal.h"

static SoundHandle gSoundOptionMask;

static s16 gOptionMenuTextures[] = { 0x3D, 0x3C, 0x3F, 0x3E, 0x44, -1 };
static s16 gOptionMenuImageIndices[] = { -1 };

// Current song index used in the music test (from JUKEBOX magic code)
static s32 gMusicTestSongIndex = 0;

static s32 gSfxVolumeSliderValue = 256;   // Value from 0 to 256
static s32 gMusicVolumeSliderValue = 256; // Value from 0 to 256

static StereoPanMode gAudioOutputType = STEREO;

/**
 * Initialise the root options menu.
 * Loads required fonts and sets the background music.
 */
void menu_options_init(void) {
    gMenuFlow.blinkTimer = 0;
    gMenuInput.delay = 0;
    transition_begin(&sMenuTransitionFadeOut);
    load_font(ASSET_FONTS_BIGFONT);
    set_text_font(ASSET_FONTS_BIGFONT);
    music_voicelimit_set(24);
    music_play(SEQUENCE_MAIN_MENU);
    music_change_off();
}

/**
 * Render the text entries for the options menu.
 */
void optionscreen_render(UNUSED s32 updateRate) {
    s32 optionMenuTextIndex;
    s32 highlight;
    s32 yPos;

    set_text_font(ASSET_FONTS_BIGFONT);
    set_text_background_colour(0, 0, 0, 0);
    set_text_colour(0, 0, 0, 255, 128);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 1, 35, gMenuLoaded.text[ASSET_MENU_TEXT_OPTIONS],
              ALIGN_MIDDLE_CENTER); // OPTIONS
    set_text_colour(255, 255, 255, 0, 255);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, 32, gMenuLoaded.text[ASSET_MENU_TEXT_OPTIONS],
              ALIGN_MIDDLE_CENTER); // OPTIONS

    optionMenuTextIndex = 0;
    yPos = 76;

    set_text_font(ASSET_FONTS_FUNFONT);

    while (gOptionMenuStrings[optionMenuTextIndex] != NULL) {
        if (optionMenuTextIndex == gMenuFlow.curIndex) {
            highlight = gMenuFlow.blinkTimer * 8;
            if (gMenuFlow.blinkTimer >= 32) {
                highlight = 511 - highlight;
            }
        } else {
            highlight = 0;
        }
        set_text_colour(255, 255, 255, highlight, 255);
        draw_text(&gMenuFrame.displayList, POS_CENTRED, yPos, gOptionMenuStrings[optionMenuTextIndex],
                  ALIGN_MIDDLE_CENTER);

        optionMenuTextIndex++;
        yPos += 28;
    }
}

/**
 * Draw the text list and accept input for going to specific option menus.
 */
s32 menu_options_loop(s32 updateRate) {
    s32 buttonsPressed;
    s32 i;
    UNUSED s32 pad0;
    s32 analogueX;
    s32 analogueY;
    UNUSED s32 pad1;

    gMenuFlow.blinkTimer = (gMenuFlow.blinkTimer + updateRate) & 0x3F;
    if (gMenuInput.delay != 0) {
        if (gMenuInput.delay > 0) {
            gMenuInput.delay += updateRate;
        } else {
            gMenuInput.delay -= updateRate;
        }
    }

    if (gMenuInput.delay > -20 && gMenuInput.delay < 35) {
        optionscreen_render(updateRate);
    }
    buttonsPressed = 0;
    analogueX = 0;
    analogueY = 0;
    if (gMenuInput.ignoreInputTime == 0 && gMenuInput.delay == 0) {
        for (i = 0; i < MAXCONTROLLERS; i++) {
            buttonsPressed |= input_pressed(i);
            analogueX += gMenuInput.xAxisDirection[i];
            analogueY += gMenuInput.yAxisDirection[i];
        }
    }
    if ((buttonsPressed & B_BUTTON) || ((buttonsPressed & (A_BUTTON | START_BUTTON)) && gMenuFlow.curIndex == 5)) {
        // Leave the option menu
        music_fade(-128);
        gMenuInput.delay = -1;
        transition_begin(&sMenuTransitionFadeIn);
        sound_play(SOUND_MENU_BACK3, NULL);
    } else if ((buttonsPressed & (A_BUTTON | START_BUTTON)) && gMenuFlow.curIndex >= 2) {
        // Go to a sub-menu
        gMenuInput.delay = 31;
        sound_play(SOUND_SELECT2, NULL);
    } else if (gMenuFlow.curIndex == 0 && analogueX != 0) {
        s32 langVal = get_language();
        switch ((u64) langVal) {
            case LANGUAGE_ENGLISH:
                set_language(LANGUAGE_FRENCH);
                break;
            default:
                set_language(LANGUAGE_ENGLISH);
                break;
        }
        sound_play(SOUND_MENU_PICK2, NULL);
    } else if (gMenuFlow.curIndex == 1 && analogueX != 0) {
        if (gRaceSetup.eepromSettings & 0x2000000) {
            // 0x2000000 SUBTITLES ENABLED?
            sound_play(SOUND_MENU_PICK2, NULL);
            unset_eeprom_settings_value(0x2000000);
            set_subtitles(0);
            gOptionMenuStrings[1] = gMenuLoaded.text[ASSET_MENU_TEXT_SUBTITLESOFF];
        } else {
            sound_play(SOUND_MENU_PICK2, NULL);
            set_eeprom_settings_value(0x2000000);
            set_subtitles(1);
            gOptionMenuStrings[1] = gMenuLoaded.text[ASSET_MENU_TEXT_SUBTITLESON];
        }
    } else {
        s32 prevOption = gMenuFlow.curIndex;
        if (analogueY < 0) {
            gMenuFlow.curIndex++;
            if (gMenuFlow.curIndex >= 6) {
                gMenuFlow.curIndex = 5;
            }
        }
        if (analogueY > 0) {
            gMenuFlow.curIndex--;
            if (gMenuFlow.curIndex < 0) {
                gMenuFlow.curIndex = 0;
            }
        }
        if (prevOption != gMenuFlow.curIndex) {
            sound_play(SOUND_MENU_PICK2, NULL);
        }
    }
    if (gMenuInput.delay > 30) {
        // Change screen to a sub-menu
        if (gMenuFlow.curIndex == 2) {
            optionscreen_free();
            menu_init(MENU_AUDIO_OPTIONS);
            return MENU_RESULT_CONTINUE;
        }
        if (gMenuFlow.curIndex == 3) {
            optionscreen_free();
            menu_init(MENU_SAVE_OPTIONS);
            return MENU_RESULT_CONTINUE;
        }
        optionscreen_free();
        menu_init(MENU_MAGIC_CODES);
        return MENU_RESULT_CONTINUE;
    }
    if (gMenuInput.delay < -30) {
        // Change screen back to the title screen.
        music_change_on();
        optionscreen_free();
        menu_init(MENU_TITLE);
        return MENU_RESULT_CONTINUE;
    }
    gMenuInput.ignoreInputTime = 0;
    return MENU_RESULT_CONTINUE;
}

/**
 * Unloads all assets associated with the options menu.
 */
void optionscreen_free(void) {
    unload_font(ASSET_FONTS_BIGFONT);
}

/**
 * Initialise the audio options menu.
 * Loads necessary textures and fonts.
 * Gets global audio volume and checks if the jukebox cheat is active to set the lower Y bounds.
 */
void menu_audio_options_init(void) {
    gOptionsMenuItemIndex = 0;
    gMenuFlow.blinkTimer = 0;
    gMenuInput.delay = 0;
    gSoundOptionMask = NULL;
    gMenuFlow.opacityDecayTimer = -1;
    menu_assetgroup_load(gOptionMenuTextures);
    menu_imagegroup_load(gOptionMenuImageIndices);
    menu_init_arrow_textures();
    transition_begin(&sMenuTransitionFadeOut);
    menu_panels_alloc(2);
    gMusicVolumeSliderValue = music_volume_config();
    gSfxVolumeSliderValue = sndp_get_global_volume();
    if (gRaceSetup.activeMagicCodes & CHEAT_MUSIC_MENU) { // Check if "JUKEBOX" cheat is active
        gAudioMenuStrings[6].text = gMusicTestString;
        gAudioMenuStrings[3].y = 212;
        music_voicelimit_set(32);
        gMenuFlow.stage = 5;
    } else {
        gAudioMenuStrings[6].text = NULL;
        gAudioMenuStrings[3].y = 192;
        gMenuFlow.stage = 4;
    }
    load_font(ASSET_FONTS_BIGFONT);
}

// Probably soundoption_render
void audio_options_render(UNUSED s32 updateRate) {
    s32 i;
    s32 yOffset;
    s32 j;
    s32 temp;

    temp = gMusicTestSongIndex;

    for (i = 0; (gMusicTestString[i] < '0') || (gMusicTestString[i] > '9'); i++) {}

    j = temp / 10;
    gMusicTestString[i] = '0' + j;
    temp -= (j * 10);
    i++;
    gMusicTestString[i] = '0' + temp;

    temp = gMenuFlow.blinkTimer * 8;
    if (temp >= 256) {
        temp = 511 - temp;
    }
    gAudioMenuStrings[0].text = gAudioOutputStrings[gAudioOutputType];
    mtx_ortho(&gMenuFrame.displayList, &gMenuFrame.hudMat);

    if (osTvType == OS_TV_TYPE_PAL) {
        yOffset = 101;
    } else {
        yOffset = 113;
    }
    menu_panel_draw_textured(NULL, -72, 120 - yOffset, 144, 14, 6, 4, COLOUR_RGBA32(255, 192, 64, 255),
                             (TextureHeader *) gMenuLoaded.assets[TEXTURE_UNK_44]);
    menu_panel_draw_textured(NULL, -72, 80 - yOffset, 144, 14, 6, 4, COLOUR_RGBA32(255, 192, 64, 255),
                             (TextureHeader *) gMenuLoaded.assets[TEXTURE_UNK_44]);
    menu_panels_render(&gMenuFrame.displayList);
    dialogue_clear(7);
    set_current_dialogue_background_colour(7, 0, 0, 0, 255);
    set_current_dialogue_box_coords(7, 94, 117, 226, 123);
    render_dialogue_box(&gMenuFrame.displayList, NULL, NULL, 7);
    set_current_dialogue_box_coords(7, 94, 157, 226, 163);
    render_dialogue_box(&gMenuFrame.displayList, NULL, NULL, 7);
    texrect_draw(&gMenuFrame.displayList, gMenuSelectionArrowLeft, (gSfxVolumeSliderValue >> 1) + 96, 120, 255, 255,
                 255, 255);
    texrect_draw(&gMenuFrame.displayList, gMenuSelectionArrowLeft, (gMusicVolumeSliderValue >> 1) + 96, 160, 255, 255,
                 255, 255);
    rendermode_reset(&gMenuFrame.displayList);
    set_text_background_colour(0, 0, 0, 0);

    // j must be here.
    j = 0;

    if (gMenuFlow.stage < 5) {
        i = gOptionsMenuItemIndex;
    } else if (gOptionsMenuItemIndex < 3) {
        i = gOptionsMenuItemIndex;
    } else if (gOptionsMenuItemIndex == 3) {
        i = 6;
    } else {
        i = 3;
    }

    // Must be a for loop?
    for (; gAudioMenuStrings[j].text != NULL; j++) {
        set_text_font(gAudioMenuStrings[j].font);
        if (j == i) {
            set_text_colour(255, 255, 255, temp, 255);
        } else {
            set_text_colour(gAudioMenuStrings[j].red, gAudioMenuStrings[j].green, gAudioMenuStrings[j].blue,
                            gAudioMenuStrings[j].alpha, gAudioMenuStrings[j].opacity);
        }
        draw_text(&gMenuFrame.displayList, gAudioMenuStrings[j].x, gAudioMenuStrings[j].y, gAudioMenuStrings[j].text,
                  gAudioMenuStrings[j].alignmentFlags);
    }
}

/**
 * Handles the input for the audio options menu.
 * Will set global audio volume for sfx and music.
 */
s32 menu_audio_options_loop(s32 updateRate) {
    s32 i;
    u32 buttonsPressed;
    s32 contX;
    s32 contY;
    s32 contXAxis;
    s32 playSound;

    playSound = 0;
    gMenuFlow.blinkTimer = (gMenuFlow.blinkTimer + updateRate) & 0x3F;
    if (gMenuInput.delay != 0) {
        if (gMenuInput.delay > 0) {
            gMenuInput.delay += updateRate;
        } else {
            gMenuInput.delay -= updateRate;
        }
    }
    if (gMenuInput.delay > -20 && gMenuInput.delay < 20) {
        audio_options_render(updateRate);
    }
    if (gMenuInput.ignoreInputTime == 0) {
        contX = 0;
        contY = 0;
        if (gMenuInput.delay == 0) {
            buttonsPressed = 0;
            contXAxis = 0;
            for (i = 0; i < 4; i++) {
                buttonsPressed |= input_pressed(i);
                contXAxis += input_clamp_stick_x(i);
                contX += gMenuInput.xAxisDirection[i];
                contY += gMenuInput.yAxisDirection[i];
            }
            if (contXAxis < 0) {
                contXAxis += 24;
                if (contXAxis > 0) {
                    contXAxis = 0;
                }
            } else {
                contXAxis -= 24;
                if (contXAxis < 0) {
                    contXAxis = 0;
                }
            }
            contXAxis >>= 2;
            if ((buttonsPressed & (A_BUTTON | START_BUTTON) && gMenuFlow.stage == gOptionsMenuItemIndex + 1) ||
                buttonsPressed & B_BUTTON) {
                gMenuInput.delay = -1;
                transition_begin(&sMenuTransitionFadeIn);
                if (gMenuFlow.opacityDecayTimer >= 0) {
                    music_fade(-128);
                }
                playSound = 3;
            } else if (contY < 0 && gOptionsMenuItemIndex < gMenuFlow.stage - 1) {
                gOptionsMenuItemIndex++;
                playSound = 1;
            } else if (contY > 0 && gOptionsMenuItemIndex > 0) {
                gOptionsMenuItemIndex--;
                playSound = 1;
            } else if (gOptionsMenuItemIndex == 0 && contX != 0) {
                if (contX < 0) {
                    gAudioOutputType--;
                } else {
                    gAudioOutputType++;
                }
                if (gAudioOutputType < 0) {
                    gAudioOutputType = HEADPHONES;
                }
                if (gAudioOutputType > HEADPHONES) {
                    gAudioOutputType = STEREO;
                }
                set_stereo_pan_mode(gAudioOutputType);
                playSound = 1;
            } else if (contXAxis && (gOptionsMenuItemIndex == 1 || gOptionsMenuItemIndex == 2)) {
                if (gOptionsMenuItemIndex == 1) {
                    gSfxVolumeSliderValue += contXAxis;
                    if (gSfxVolumeSliderValue < 0) {
                        gSfxVolumeSliderValue = 0;
                    } else if (gSfxVolumeSliderValue > 256) {
                        gSfxVolumeSliderValue = 256;
                    }
                    sndp_set_global_volume(gSfxVolumeSliderValue);
                } else if (gOptionsMenuItemIndex == 2) {
                    gMusicVolumeSliderValue += contXAxis;
                    if (gMusicVolumeSliderValue < 0) {
                        gMusicVolumeSliderValue = 0;
                    } else if (gMusicVolumeSliderValue > 256) {
                        gMusicVolumeSliderValue = 256;
                    }
                    music_volume_config_set(gMusicVolumeSliderValue);
                    if (!music_is_playing()) {
                        if (gMenuFlow.opacityDecayTimer >= 0) {
                            music_change_on();
                            music_play(gMusicTestSongIndex);
                        } else {
                            music_change_on();
                            music_voicelimit_set(24);
                            music_play(SEQUENCE_MAIN_MENU);
                            music_change_off();
                        }
                    }
                }
            } else if (gMenuFlow.stage >= 5 && gOptionsMenuItemIndex == 3) {
                if (contX < 0 && gMusicTestSongIndex > 0) {
                    gMusicTestSongIndex--;
                    playSound = 1;
                } else if (contX > 0) {
                    if (gMusicTestSongIndex < (music_sequence_count() - 1)) {
                        gMusicTestSongIndex++;
                        playSound = 1;
                    }
                }
                if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
                    music_change_on();
                    music_voicelimit_set(24);
                    music_play(gMusicTestSongIndex);
                    gMenuFlow.opacityDecayTimer = gMusicTestSongIndex;
                }
            }
            if (gOptionsMenuItemIndex == 1) {
                if (gSoundOptionMask == NULL) {
                    sound_play(SOUND_VOICE_DIDDY_POSITIVE5, &gSoundOptionMask);
                }
            } else if (gSoundOptionMask != NULL) {
                sndp_stop(gSoundOptionMask);
            }
            if (playSound == 3) {
                sound_play(SOUND_MENU_BACK3, NULL);
            } else if (playSound == 2) {
                sound_play(SOUND_SELECT2, NULL);
            } else if (playSound == 1) {
                sound_play(SOUND_MENU_PICK2, NULL);
            }
        }
    }
    if (gMenuInput.delay < -30) {
        soundoptions_free();
        menu_init(MENU_OPTIONS);
        return 0;
    }
    gMenuInput.ignoreInputTime = 0;
    return 0;
}

/**
 * Frees all assets associated with the audio options menu.
 */
void soundoptions_free(void) {
    if (gSoundOptionMask != NULL) {
        sndp_stop(gSoundOptionMask);
    }
    if (gMenuFlow.opacityDecayTimer >= 0) {
        music_voicelimit_set(24);
        music_play(SEQUENCE_MAIN_MENU);
        music_fade(0x100);
        music_change_off();
    }
    menu_assetgroup_free(gOptionMenuTextures);
    unload_font(ASSET_FONTS_BIGFONT);
}
