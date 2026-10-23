#include "menu_internal.h"

static MenuElement *gGameSelectElements;

/**
 * Initialises the timer and transition for the controller pak warning screen.
 * Only seen once per game session, as set by gRaceSetup.hasSeenCautionMenu.
 */
void menu_caution_init(void) {
    gMenuInput.ignoreInputTime = 60;
    gMenuInput.delay = 0;
    load_font(ASSET_FONTS_BIGFONT);
    transition_begin(&sMenuTransitionFadeOut);
    gRaceSetup.hasSeenCautionMenu = TRUE;
}

/**
 * Loops through the display of the caution screen.
 * Blocks player input for one second.
 */
s32 menu_caution_loop(s32 updateRate) {
    if (gMenuInput.delay) {
        gMenuInput.delay += updateRate;
    } else if (gMenuInput.ignoreInputTime <= 0 && (input_pressed(PLAYER_ONE) & (A_BUTTON | B_BUTTON | START_BUTTON))) {
        sound_play(SOUND_SELECT2, NULL);
        gMenuInput.delay = 1;
        transition_begin(&sMenuTransitionFadeIn);
    }
    if (gMenuInput.delay < 20) {
        draw_menu_elements(1, gCautionMenuTextElements, 1.0f);
    }
    if (gMenuInput.delay > 30) {
        caution_free();
        menu_init(MENU_GAME_SELECT);
    }
    if (gMenuInput.ignoreInputTime > 0) {
        gMenuInput.ignoreInputTime -= updateRate;
    }
    return MENU_RESULT_CONTINUE;
}

/**
 * Unloads all assets associated with the caution screen.
 */
void caution_free(void) {
    unload_font(ASSET_FONTS_BIGFONT);
}

/**
 * Initialises the selection menu that contains the options for adventure one, two and tracks mode.
 * Loads all save file data to determine whether to show adventure two.
 */
void menu_game_select_init(void) {
    s32 i;

    gMenuInput.delay = 0;
    gMenuFlow.blinkTimer = 0;
    gMenuFlow.curIndex = 0;
    gMenuFlow.stage = 2;
    transition_begin(&sMenuTransitionFadeOut);
    mark_read_all_save_files();
    set_ghost_none();
    gMenuFlow.opacityDecayTimer = 1;
    menu_asset_load(67);
    menu_panels_alloc(3);
    load_font(ASSET_FONTS_BIGFONT);
    music_play(SEQUENCE_CHOOSE_YOUR_RACER);

    for (i = 0; i < NUM_CHARACTERS; i++) {
        if (i != gMenuSelectedCharacter.channelIndex) {
            music_channel_off(gCharacterVolumes[i][0]);
            music_channel_off(gCharacterVolumes[i][1]);
        }
    }

    music_channel_off(6);
    music_change_off();
    if (is_adventure_two_unlocked()) {
        gGameSelectElements = gGameSelectTextElemsWithAdv2;
        gMenuFlow.stage = 2;
    } else {
        gGameSelectElements = gGameSelectTextElemsNoAdv2;
        gMenuFlow.stage = 1;
    }

    for (i = 0; i <= gMenuFlow.stage; i++) {
        // Fakematch? What's the (i ^ 0)?
        gGameSelectElements[((i ^ 0) * 2) + 2].t.texture = gMenuLoaded.assets[TEXTURE_SURFACE_BUTTON_WOOD];
    }
}

/**
 * Initialise the ortho matrix then render the menu elements for the buttons.
 */
void gameselect_render(UNUSED s32 updateRate) {
    s32 i;
    s32 filterBlendFactor;
    s32 fade;

    if (gMenuInput.delay > -22 && gMenuInput.delay < 22) {
        fade = gMenuFlow.blinkTimer * 8;
        if (fade > 255) {
            fade = 511 - fade;
        }
        mtx_ortho(&gMenuFrame.displayList, &gMenuFrame.hudMat);

        for (i = 0; i <= gMenuFlow.stage; i++) {
            filterBlendFactor = 0;
            if (i == gMenuFlow.curIndex) {
                filterBlendFactor = fade;
            }
            // Fakematch? What's the (i ^ 0)?
            gGameSelectElements[((i ^ 0) * 2) + 3].filterBlendFactor = filterBlendFactor;
        }

        if (osTvType == OS_TV_TYPE_PAL) {
            gResults.textOffset = 12;
            gResults.timestampOffset = 0;
        } else {
            gResults.textOffset = 0;
            gResults.timestampOffset = 0;
        }

        draw_menu_elements(1, gGameSelectElements, 1.0f);
        menu_panels_render(&gMenuFrame.displayList);
    }
}

/**
 * Make sure the correct music channels are active in accordance to the selected player,
 * then render the available game modes.
 */
s32 menu_game_select_loop(s32 updateRate) {
    s32 playerInputs;
    s32 playerYDir;
    s32 charSelectScene;

    charselect_music_channels(updateRate);

    gMenuFlow.blinkTimer = (gMenuFlow.blinkTimer + updateRate) & 0x3F;

    if (gMenuFlow.opacityDecayTimer) {
        gMenuFlow.opacityDecayTimer++;
        if (gMenuFlow.opacityDecayTimer >= 3) {
            trackmenu_set_records();
            gMenuFlow.opacityDecayTimer = 0;
        }
    }
    if (gMenuInput.delay != 0) {
        if (gMenuInput.delay < 0) {
            gMenuInput.delay -= updateRate;
        } else {
            gMenuInput.delay += updateRate;
        }
    }
    if (gMenuInput.delay > 30) {
        gameselect_free();
        if (gMenuFlow.curIndex == gMenuFlow.stage) {
            music_change_on();
            gRaceSetup.isInTracksMode = TRUE;
            init_racer_headers();
            load_level_for_menu((s32) SPECIAL_MAP_ID_NO_LEVEL, -1, 0);
            menu_init(MENU_TRACK_SELECT);
        } else {
            gRaceSetup.isInAdventureTwo = gMenuFlow.curIndex;
            gRaceSetup.isInTracksMode = FALSE;
            gRaceSetup.playerVehicles[PLAYER_ONE] = VEHICLE_CAR;
            set_level_default_vehicle(VEHICLE_CAR);
            menu_init(MENU_FILE_SELECT);
        }
        return MENU_RESULT_CONTINUE;
    } else if (gMenuInput.delay < -30) {
        gameselect_free();
        charSelectScene = 0;
        if (is_drumstick_unlocked()) {
            charSelectScene = 1;
        }
        if (is_tt_unlocked()) {
            charSelectScene ^= 3;
        }
        load_level_for_menu(ASSET_LEVEL_CHARACTERSELECT, -1, charSelectScene);
        charselect_prev(0, 0);
        menu_init(MENU_CHARACTER_SELECT);
        return MENU_RESULT_CONTINUE;
    } else {
        gameselect_render(updateRate);
        if ((gMenuInput.delay == 0) && (gMenuFlow.opacityDecayTimer == 0)) {
            playerInputs = input_pressed(PLAYER_ONE);
            playerYDir = gMenuInput.yAxisDirection[0];
            if (gRaceSetup.numberOfActivePlayers == 2) {
                playerInputs |= input_pressed(1);
                playerYDir += gMenuInput.yAxisDirection[1];
            }
            if (playerInputs & (A_BUTTON | START_BUTTON)) {
                if (gMenuFlow.curIndex == gMenuFlow.stage) {
                    music_fade(-128);
                }
                transition_begin(&sMenuTransitionFadeIn);
                gMenuInput.delay = 1;
                sound_play(SOUND_SELECT2, NULL);
            } else if (playerInputs & B_BUTTON) {
                transition_begin(&sMenuTransitionFadeIn);
                gMenuInput.delay = -1;
            } else {
                if (playerYDir < 0) {
                    if (gMenuFlow.curIndex < gMenuFlow.stage) {
                        gMenuFlow.curIndex++;
                        sound_play(SOUND_MENU_PICK2, NULL);
                    }
                }
                if (playerYDir > 0) {
                    if (gMenuFlow.curIndex > 0) {
                        gMenuFlow.curIndex--;
                        sound_play(SOUND_MENU_PICK2, NULL);
                    }
                }
            }
        }
        gMenuInput.ignoreInputTime = 0;
        return MENU_RESULT_CONTINUE;
    }
}

/**
 * Free the assets associated with the game select menu.
 */
void gameselect_free(void) {
    unload_font(ASSET_FONTS_BIGFONT);
    menu_button_free();
    menu_asset_free(TEXTURE_SURFACE_BUTTON_WOOD);
}
