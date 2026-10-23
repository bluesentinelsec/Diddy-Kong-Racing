#include "menu_internal.h"

static s32 gTitleScreenCurrentOption = 0; // 0 = "Start", 1 = "Options"

// Title screen cinematic text
static unk800DF83C gTitleCinematicText[10] = {
    { "TIMBER", 14.0f, 14.5f, 16.5f, 17.0f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f },
    { "BUMPER", 19.0f, 19.5f, 21.5f, 22.0f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f },
    { "CONKER", 24.0f, 24.5f, 26.5f, 27.0f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f },
    { "TIPTUP", 29.0f, 29.5f, 31.5f, 32.0f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f },
    { "DRUMSTICK", 35.0f, 35.5f, 37.5f, 38.0f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f },
    { "PIPSY", 40.0f, 40.5f, 42.5f, 43.0f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f },
    { "BANJO", 45.0f, 45.5f, 47.5f, 48.0f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f },
    { "KRUNCH", 50.0f, 50.5f, 52.5f, 53.0f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f },
    { "WIZPIG", 57.5f, 58.0f, 60.0f, 60.5f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f },
    { "DIDDY", 63.5f, 64.0f, 66.0f, 66.5f, -80.0f, SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT_HALF,
      SCREEN_HEIGHT_FLOAT - 32.0f, SCREEN_WIDTH_FLOAT + 80.0f, SCREEN_HEIGHT_FLOAT - 32.0f }
};

// Number of active colours used in the title screen cinematic.
static s32 gTitleCinematicTextColourCount = 0;

// Colours used for the Character Names during the title screen cinematic
static u8 gTitleCinematicTextColours[] = {
    255, 255, 0,   255, 204, // Yellow
    0,   255, 0,   255, 153, // Green
    0,   255, 255, 255, 102, // Cyan
    0,   0,   255, 255, 51   // Blue
};

// Fade transition between levels in the title screen demo.
static FadeTransition gFadeTitleScreenDemo = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_NONE, FADE_COLOR_BLACK, 52, FADE_STAY);

/**
 * Sets some things needed after the title screen from the save buffer.
 */
void init_title_screen_variables(void) {
    if (sEepromSettings & 2) {
        set_magic_code_flags(CHEAT_CONTROL_DRUMSTICK);
    }
    if ((sEepromSettings & 0xFFFFF0) == 0xFFFFF0) {
        set_magic_code_flags(CHEAT_CONTROL_TT);
    }
    if (sEepromSettings & 1) {
        gIsInAdventureTwo = TRUE;
    } else {
        gIsInAdventureTwo = FALSE;
    }
    if ((sEepromSettings & 0x2000000) == 0) {
        set_subtitles(0);
    }
    load_menu_text(get_language());
}

void menu_intro_names_update(f32 updateRateF) {
    f32 temp;
    f32 temp2;
    s32 didUpdate;
    s32 xPos;
    s32 yPos;
    s32 i;
    s32 j;
    char *text;
    unk800DF83C *introCharData;

    didUpdate = FALSE;
    xPos = 0;
    yPos = 0;
    text = NULL;

    if (gOpeningNameID >= 10) {
        return;
    }

    introCharData = &gTitleCinematicText[gOpeningNameID];
    sIntroNameTimer += updateRateF;
    set_text_font(ASSET_FONTS_BIGFONT);
    set_text_background_colour(0, 0, 0, 0);
    i = 0;
    while (i < gTitleCinematicTextColourCount) {
        j = 5 * sIntroNameEntries[i].colourIndex;
        set_text_colour(gTitleCinematicTextColours[j + 0], gTitleCinematicTextColours[j + 1],
                        gTitleCinematicTextColours[j + 2], gTitleCinematicTextColours[j + 3],
                        gTitleCinematicTextColours[j + 4]);
        draw_text(&sMenuCurrDisplayList, sIntroNameEntries[i].x, sIntroNameEntries[i].y, sIntroNameEntries[i].text, ALIGN_MIDDLE_CENTER);
        sIntroNameEntries[i].colourIndex++;
        if (sIntroNameEntries[i].colourIndex >= 4) {
            gTitleCinematicTextColourCount--;
            for (j = i; j < gTitleCinematicTextColourCount; j++) {
                sIntroNameEntries[j].text = sIntroNameEntries[j + 1].text;
                sIntroNameEntries[j].x = sIntroNameEntries[j + 1].x;
                sIntroNameEntries[j].y = sIntroNameEntries[j + 1].y;
                sIntroNameEntries[j].colourIndex = sIntroNameEntries[j + 1].colourIndex;
            }
        } else {
            i++;
        }
    }

    if (introCharData->slideInStartTime <= sIntroNameTimer) {
        if (sIntroNameTimer < introCharData->slideInEndTime) {
            temp = (sIntroNameTimer - introCharData->slideInStartTime);
            temp2 = (introCharData->slideInEndTime - introCharData->slideInStartTime);
            xPos = (introCharData->startX + (((introCharData->centreX - introCharData->startX) * temp) / temp2));
            yPos = (introCharData->startY + (((introCharData->centreY - introCharData->startY) * temp) / temp2));
            text = introCharData->text;
            didUpdate = TRUE;
        } else if (sIntroNameTimer <= introCharData->holdEndTime) {
            xPos = introCharData->centreX;
            yPos = introCharData->centreY;
            text = introCharData->text;
            didUpdate = TRUE;
        } else if (sIntroNameTimer < introCharData->slideOutEndTime) {
            temp = (sIntroNameTimer - introCharData->holdEndTime);
            temp2 = (introCharData->slideOutEndTime - introCharData->holdEndTime);
            xPos = (introCharData->centreX + (((introCharData->endX - introCharData->centreX) * temp) / temp2));
            yPos = (introCharData->centreY + (((introCharData->endY - introCharData->centreY) * temp) / temp2));
            text = introCharData->text;
            didUpdate = TRUE;
        } else {
            gOpeningNameID++;
        }
    }

    if (!didUpdate) {
        return;
    }

    if (gTitleCinematicTextColourCount < 4) {
        sIntroNameEntries[gTitleCinematicTextColourCount].colourIndex = 0;
        sIntroNameEntries[gTitleCinematicTextColourCount].text = text;
        sIntroNameEntries[gTitleCinematicTextColourCount].x = xPos;
        sIntroNameEntries[gTitleCinematicTextColourCount].y = yPos;
        gTitleCinematicTextColourCount++;
    }
    set_text_colour(255, 255, 255, 0, 255);
    draw_text(&sMenuCurrDisplayList, xPos, yPos, text, ALIGN_MIDDLE_CENTER);
}

void menu_title_screen_init(void) {
    s32 i;
    s32 numberOfPlayers;

    gTitleScreenLoaded = TRUE;
    gOptionBlinkTimer = 0;
    gMenuDelay = 0;
    reset_character_id_slots();
    gSaveFileIndex = 0;
    gTitleScreenCurrentOption = 0;
    gNumberOfActivePlayers = 4;
    input_assign_players();
    music_play(SEQUENCE_NONE2);
    sMenuMusicVolume = music_volume();
    if (gResetTitleScale) {
        gTitleRevealTimer = 0;
        gResetTitleScale = FALSE;
    } else {
        gTitleRevealTimer = 1;
    }
    gTitleAudioCounter = 0;
    gMenuStage = TITLESCREEN_START;
    menu_assetgroup_load(sGameTitleTileTextures);
    for (i = 0; i < ARRAY_COUNT(sGameTitleTileOffsets) - 1; i++) {
        sGameTitleTileOffsets[i].texture = gMenuAssets[sGameTitleTileTextures[i]];
    }
    music_voicelimit_set(27);
    cam_shake_off();
    set_text_font(ASSET_FONTS_FUNFONT);
    load_font(ASSET_FONTS_BIGFONT);
    sound_volume_reset(FALSE);
    set_time_trial_enabled(FALSE);
    gTitleDemoIndex = 0;
    sTitleScreenDemoIds = (s8 *) get_misc_asset(ASSET_MISC_TITLE_SCREEN_DEMO_IDS);
    numberOfPlayers = sTitleScreenDemoIds[gTitleDemoIndex + DEMO_PLAYER_COUNT];
    gTitleDemoTimer = 0;
    if (numberOfPlayers == -2) {
        numberOfPlayers = 0;
        gTitleDemoTimer = 600;
    }
    load_level_for_menu(sTitleScreenDemoIds[gTitleDemoIndex + DEMO_LEVEL_ID], numberOfPlayers,
                        sTitleScreenDemoIds[gTitleDemoIndex + DEMO_CUTSCENE_ID]);
    sIntroNameTimer = 0;
    gOpeningNameID = 0;
    gTitleCinematicTextColourCount = 0;
    gOpacityDecayTimer = 0;
    gIsInTracksMode = FALSE;
}

/**
 * Scale and render the game title on the screen based on the reveal timer.
 * Also render the selection options.
 */
void render_title_screen(UNUSED s32 updateRate, f32 updateRateF) {
    UNUSED u32 foo[2];
    s32 alpha;
    f32 scale;
    s32 i;
    s32 posY;

    if (gTitleRevealTimer) {
        mtx_ortho(&sMenuCurrDisplayList, &sMenuCurrHudMat);
        scale = (f32) gTitleRevealTimer * (1.0f / 32.0f);
        sMenuGuiOpacity = (gTitleRevealTimer * 8) - 1;
        cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
        if (scale != 1.0f) {
            texrect_draw_scaled(&sMenuCurrDisplayList, sGameTitleTileOffsets, SCREEN_WIDTH_FLOAT_HALF, 52.0f, scale,
                                scale, COLOUR_RGBA32(255, 255, 255, 254), TEXRECT_POINT);
        } else {
            texrect_draw(&sMenuCurrDisplayList, sGameTitleTileOffsets, SCREEN_WIDTH_HALF, 52, 255, 255, 255, 255);
        }
        if (!is_controller_missing()) {
            i = 0;
            posY = (osTvType == OS_TV_TYPE_PAL) ? SCREEN_HEIGHT - 22 : SCREEN_HEIGHT - 48;
            set_text_font(ASSET_FONTS_FUNFONT);
            set_text_background_colour(0, 0, 0, 0);
            while (gTitleMenuStrings[i] != NULL) {
                if (i == gTitleScreenCurrentOption) {
                    alpha = (gOptionBlinkTimer & 0x1F) << 4;
                    if (alpha > 255) {
                        alpha = 511 - alpha;
                    }
                } else {
                    alpha = 0;
                }
                set_text_colour(255, 255, 255, alpha, sMenuGuiOpacity);
                draw_text(&sMenuCurrDisplayList, POS_CENTRED, posY, gTitleMenuStrings[i], ALIGN_MIDDLE_CENTER);
                posY += 16;
                i++;
            }
        }
    } else if (sTitleScreenDemoIds[gTitleDemoIndex] == sTitleScreenDemoIds[0]) {
        menu_intro_names_update(updateRateF);
    }
}

/**
 * Handle the countdown when the title screen starts to show the logo coming in.
 * Fade music to make voice clips more audible.
 */
s32 menu_title_screen_loop(s32 updateRate) {
    UNUSED s32 temp_v0_5;
    s32 sp28;
    s8 *demo;
    s32 contrIndex;
    f32 updateRateF;
    Camera *sp18;
    s8 playerCount;

    sp18 = cam_get_active_camera();
    gOptionBlinkTimer = (gOptionBlinkTimer + updateRate) & 0x3F;
    menu_input();
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF = (f32) updateRate / 50.0f;
    } else {
        updateRateF = (f32) updateRate / 60.0f;
    }
    if (gMenuDelay < 20) {
        render_title_screen(updateRate, updateRateF);
    }
    if (gMenuDelay) {
        gMenuDelay += updateRate;
    }
    if (sTitleScreenDemoIds[gTitleDemoIndex] == sTitleScreenDemoIds[0]) {
        gOpacityDecayTimer += updateRate;
    }
    sp28 = 0;
    if (gTitleDemoTimer > 0) {
        gTitleDemoTimer -= updateRate;
        if (gTitleDemoTimer < 60 && gTitleDemoTimer + updateRate >= 60) {
            music_fade(-768);
            sp28 = 0;
            transition_begin(&gFadeTitleScreenDemo);
        }
        if (gTitleDemoTimer <= 0) {
            sp28 = 1;
        }
    } else {
        gTitleDemoTimer = 0;
    }
    if (gMenuDelay == 0 && (anim_prev_frame_state() || sp28)) {
        if (gTitleDemoTimer) {} // Fakematch
        gTitleDemoIndex += DEMO_INDEX_SIZE;
        demo = &sTitleScreenDemoIds[gTitleDemoIndex];
        if (demo[0] == -1) {
            gTitleDemoIndex = 0;
            demo = &sTitleScreenDemoIds[gTitleDemoIndex];
        }
        if (gTitleRevealTimer == 0) {
            gTitleRevealTimer = 1;
        }
        playerCount = demo[DEMO_PLAYER_COUNT];
        gTitleDemoTimer = 0;
        if (playerCount == -2) {
            playerCount = 0;
            gTitleDemoTimer = 1500;
        }
        load_level_for_menu(demo[DEMO_LEVEL_ID], playerCount, demo[DEMO_CUTSCENE_ID]);
            if (sTitleScreenDemoIds[gTitleDemoIndex] == sTitleScreenDemoIds[DEMO_LEVEL_ID]) {
            sIntroNameTimer = 0.0f;
            gOpeningNameID = 0;
            gTitleCinematicTextColourCount = 0;
            gOpacityDecayTimer = 0;
        }
    }
    if (gTitleRevealTimer) {
        if (gTitleRevealTimer < 32) {
            if (gTitleRevealTimer == 1) {
                sound_play(SOUND_WHOOSH1, NULL);
            }
            gTitleRevealTimer += updateRate;
            if (gTitleRevealTimer >= 32) {
                gTitleRevealTimer = 32;
                sp18->shakeMagnitude = 8.0f;
                sound_play(SOUND_EXPLOSION, NULL);
            }
        } else {
            if (gTitleAudioCounter < 6.0f) {
                gTitleAudioCounter += updateRateF;
                if (gTitleAudioCounter > 0.67f && gMenuStage == TITLESCREEN_START) {
                    sound_play(SOUND_VOICE_TT_DIDDY_KONG_RACING, NULL);
                    gMenuStage = TITLESCREEN_NAME;
                } else if (gTitleAudioCounter > 2.83f && gMenuStage == TITLESCREEN_NAME) {
                    sound_play(SOUND_VOICE_TT_PRESS_START, NULL);
                    gMenuStage = TITLESCREEN_PRESS_START;
                }
            }
        }
    }
    if (gTitleAudioCounter > 0.0f) {
        if (gTitleAudioCounter < 0.5f) {
            music_volume_set((s32) ((f32) sMenuMusicVolume * (1.0f - gTitleAudioCounter)));
        } else if (gTitleAudioCounter < 4.5f) {
            music_volume_set(((s32) sMenuMusicVolume >> 1));
        } else if (gTitleAudioCounter < 5.0f) {
            music_volume_set((s32) ((f32) sMenuMusicVolume * (gTitleAudioCounter - 4.0f)));
        } else {
            // Retail reads byte 3 of the s32 to grab its low byte (= the full
            // restore volume, 0x7F) — correct only on big-endian. On little-endian
            // PC byte 3 is the high byte (0), which sets music volume to 0 and
            // leaves it there. Read the low byte value-neutrally instead.
            music_volume_set((u8) sMenuMusicVolume);
        }
    }
    if (gTitleRevealTimer == 0) {
        if (gMenuButtons[PLAYER_MENU] & (A_BUTTON | START_BUTTON)) {
            gTitleRevealTimer = 1;
        }
    } else if ((gMenuDelay == 0) && !is_controller_missing()) {
        s32 temp0 = gTitleScreenCurrentOption;
        // gMenuStickY[PLAYER_MENU] = +1 when going up, and -1 when going down.
        if (gMenuStickY[PLAYER_MENU] < 0 && gTitleScreenCurrentOption < 1) {
            gTitleScreenCurrentOption++;
        }
        if (gMenuStickY[PLAYER_MENU] > 0 && gTitleScreenCurrentOption > 0) {
            gTitleScreenCurrentOption--;
        }
        if (temp0 != gTitleScreenCurrentOption) {
            sound_play(SOUND_MENU_PICK2,
                       (SoundHandle *) (s32 *) (0 * contrIndex)); // TODO: The `* contrIndex` here is a fake match.
        }
        if (gMenuButtons[PLAYER_MENU] & (A_BUTTON | START_BUTTON)) {
            for (contrIndex = 3; contrIndex > 0 && !(gMenuButtons[contrIndex] & (A_BUTTON | START_BUTTON));
                 contrIndex--) {}
            titlescreen_controller_assign(contrIndex);
            gMenuDelay = 1;
            transition_begin(&sMenuTransitionFadeIn);
            enable_new_screen_transitions();
            sound_play(SOUND_SELECT2, NULL);
        }
    }
    if (gMenuDelay > 30) {
        titlescreen_free();
        disable_new_screen_transitions();
        if (gTitleScreenCurrentOption == 0) {
            sp28 = 0;
            if (is_drumstick_unlocked()) {
                sp28 = 1;
            }
            if (is_tt_unlocked()) {
                sp28 ^= 3;
            }
            load_level_for_menu(ASSET_LEVEL_CHARACTERSELECT, -1, sp28);
            charselect_prev(0, NULL);
            menu_init(MENU_CHARACTER_SELECT);
            return MENU_RESULT_CONTINUE;
        }
        gMenuCurIndex = 0;
        load_level_for_menu(ASSET_LEVEL_OPTIONSBACKGROUND, -1, 0);
        menu_init(MENU_OPTIONS);
        return MENU_RESULT_CONTINUE;
    }
    gIgnorePlayerInputTime = 0;
    return MENU_RESULT_CONTINUE;
}

/**
 * Unloads the title screen logo and sets all audio back to default.
 */
void titlescreen_free(void) {
    menu_assetgroup_free(sGameTitleTileTextures);
    music_voicelimit_set(16);
    cam_shake_on();
    unload_font(ASSET_FONTS_BIGFONT);
    sound_volume_reset(TRUE);
}

/**
 * Sets the player who controls the menus.
 * 0 = controller 1, 1 = controller 2, 2 = controller 3, 3 = controller 4
 */
void titlescreen_controller_assign(s32 controllerIndex) {
    s32 i;

    gNumberOfActivePlayers = 1;

    for (i = 0; i < ARRAY_COUNT(gPlayersCharacterArray); i++) {
        gPlayersCharacterArray[i] = (i == controllerIndex) ? 1 : -1;
    }

    for (i = 0; i < ARRAY_COUNT(gActivePlayersArray); i++) {
        gActivePlayersArray[i] = (i == controllerIndex) ? TRUE : FALSE;
    }
}

/**
 * Reset the seen status for the title screen popup.
 */
UNUSED void menu_title_reset(void) {
    gTitleScreenLoaded = FALSE;
}
