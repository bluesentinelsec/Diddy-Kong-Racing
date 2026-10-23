#include "menu_internal.h"

s32 gShowControllerPakMenu = FALSE;

#define PAKMENU_JP_OFFSET 0

/**
 * Initialise boot screen menu.
 * Load the title textures into RAM.
 */
void menu_boot_init(void) {
    s32 i;

    transition_begin(&sMenuTransitionFadeOut);
    bgdraw_primcolour(0, 0, 0);
    menu_assetgroup_load(sGameTitleTileTextures);

    // Sets up the 11 texture pointers for the "Diddy Kong Racing" logo.
    for (i = 0; i < ARRAY_COUNT(sGameTitleTileOffsets) - 1; i++) {
        sGameTitleTileOffsets[i].texture = gMenuAssets[sGameTitleTileTextures[i]];
    }

    // Reset variables for menu_boot_loop()
    gBootMenuPhase = 0;
    gBootMenuTimer = 0;
}

/**
 * Display the title onscreen for a few seconds.
 * Call the function to init the controller pak menu, too, if needed.
 */
s32 menu_boot_loop(s32 updateRate) {
    s32 out;
    s32 y;
    s32 offsetY;

    out = MENU_RESULT_CONTINUE;

    offsetY = SCREEN_HEIGHT_HALF;
    if (osTvType == OS_TV_TYPE_PAL) {
        offsetY = SCREEN_HEIGHT_HALF_PAL;
    }

    y = offsetY;

    switch (gBootMenuPhase) {
        case 0:
            if (gBootMenuTimer < 32) {
                gBootMenuTimer += updateRate;
                if (gBootMenuTimer > 32) {
                    gBootMenuTimer = 32;
                }
            } else {
                bootscreen_init_cpak();
                gBootMenuPhase = 1;
            }
            break;
        case 1:
            if (gBootMenuTimer < 140) {
                gBootMenuTimer += updateRate;
                if (gBootMenuTimer > 140) {
                    gBootMenuTimer = 140;
                }
            } else {
                bootscreen_free();
                gBootMenuPhase = 2;
            }
            if (gBootMenuTimer > 128) {
                y = 300;
            }
            break;
        case 2:
            if (gMenuDelay && check_fadeout_transition() == 0) {
                transition_begin(&sMenuTransitionFadeInFast);
            }
            y = 300;
            out = menu_controller_pak_loop(updateRate);
            break;
    }

    if (y < 300) {
        texrect_draw(&sMenuCurrDisplayList, sGameTitleTileOffsets, SCREEN_WIDTH_HALF, y, 255, 255, 255, 255);
        rendermode_reset(&sMenuCurrDisplayList);
    }

    return out;
}

/**
 * Free the assets associated with the boot screen.
 */
void bootscreen_free(void) {
    menu_assetgroup_free((s16 *) sGameTitleTileTextures);
}

/**
 * Initialises the controller pak menu variables.
 * Allocate space for the pak data and load the assets into memory.
 */
void bootscreen_init_cpak(void) {
    s32 i;

    // Starting point
    gBootPakData[0] = mempool_alloc_safe(CPAK_HEAP_SIZE, COLOUR_TAG_WHITE);

    // Fills in the table.
    for (i = 1; i < ARRAY_COUNT(gBootPakData); i++) {
        gBootPakData[i] = (char *) (((uintptr_t) gBootPakData[0]) + (i * (CPAK_HEAP_SIZE / ARRAY_COUNT(gBootPakData))));
    }

    // Only check cpak 0
    for (i = 0; i <= 0; i++) {
        sControllerPakDataPresent[i] = FALSE;
    }

    gCpakWriteTimer = 0;
    gMenuStage = PAKMENU_CHOOSE;
    gOptionBlinkTimer = 0;
    gMenuOption = -1;
    load_menu_text(get_language());
    if (menu_find_usable_pak(&gMenuOption, 0) == CONTROLLER_PAK_GOOD) {
        gMenuDelay = 0;
    } else if (check_for_controller_pak_errors() == PAK_ERROR_NONE) {
        gMenuDelay = 20;
    }
    if (sControllerPakError == PAK_ERROR_NONE && !gShowControllerPakMenu) {
        gMenuDelay = 20;
    }
    gMenuCurIndex = 0;
    gOpacityDecayTimer = 0;
    menu_asset_load(63);
    menu_init_arrow_textures();
    if (osTvType == OS_TV_TYPE_PAL) {
        sControllerPakMenuNumberOfRows = 8;
    } else {
        sControllerPakMenuNumberOfRows = 7;
    }
    load_font(ASSET_FONTS_BIGFONT);
#if VERSION >= VERSION_79
    rumble_enable(FALSE);
#endif
}

/**
 * Render the controller pak menu.
 * Lists the pak index, as well as remaining pages, then displays all known files.
 * Also draws the confirmation box.
 * Visual Aid : https://i.imgur.com/7T2Scdr.png
 */
void pakmenu_render(UNUSED s32 updateRate) {
    s32 highlight;
    s32 i;
    char *noteText;
    char *fileNameText;
    char *pagesText;
    s32 numberOfPages;
    s32 yPos = 0;

    set_text_background_colour(0, 0, 0, 0);
    highlight = gOptionBlinkTimer << 3;
    if (highlight >= 256) {
        highlight = 511 - highlight;
    }
    if (sControllerPakError != PAK_ERROR_NONE) {
        draw_menu_elements(1, gControllerPakMenuElement, 1.0f);
    } else if (gShowControllerPakMenu != 0) {
        set_text_font(ASSET_FONTS_BIGFONT);
        set_text_colour(0, 0, 0, 255, 128);
        draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF + 1, 33, gMenuText[ASSET_MENU_TEXT_CONTPAK],
                  ALIGN_MIDDLE_CENTER); // CONTROLLER PAK - Drop Shadow
        set_text_colour(255, 255, 255, 0, 255);
        draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF, 30, gMenuText[ASSET_MENU_TEXT_CONTPAK],
                  ALIGN_MIDDLE_CENTER); // CONTROLLER PAK - Main Text

        yPos += 48;

        dialogue_clear(6);
        set_current_dialogue_box_coords(6, 58, yPos, 262, yPos + 30);

        if (gMenuCurIndex == -1) {
            set_current_dialogue_background_colour(6, 255, 255, 255, (highlight >> 1) + 128);
        } else {
            set_current_dialogue_background_colour(6, 96, 192, 92, 224);
        }

        set_dialogue_font(6, ASSET_FONTS_FUNFONT);
        set_current_text_colour(6, 255, 255, 255, 0, 255);
        set_current_text_background_colour(6, 0, 0, 0, 0);
        render_dialogue_text(6, POS_CENTRED, 2 - PAKMENU_JP_OFFSET, gMenuText[ASSET_MENU_TEXT_CONTPAK1 + gMenuOption],
                             1, HORZ_ALIGN_CENTER); // CONTROLLER PAK 1 / 2 / 3 / 4
        render_dialogue_text(6, POS_CENTRED, 16 - PAKMENU_JP_OFFSET, gMenuText[ASSET_MENU_TEXT_FREEPAGESX],
                             sCurrentControllerPakFreePages, HORZ_ALIGN_CENTER); // FREE PAGES: ~
        render_dialogue_box(&sMenuCurrDisplayList, NULL, NULL, 6);

        yPos += 34; // To 82

        set_dialogue_font(6, ASSET_FONTS_SMALLFONT);
        set_current_text_background_colour(6, 0, 0, 0, 0);
        for (i = -1; i < sControllerPakMenuNumberOfRows; i++) {
            dialogue_clear(6);
            set_current_dialogue_box_coords(6, 28, yPos, 292, yPos + 14);
            if (i < 0) {
                // Red background for table header
                set_current_dialogue_background_colour(6, 224, 48, 48, 224);
                set_current_text_colour(6, 224, 224, 48, 255, 255);
                noteText = gMenuText[ASSET_MENU_TEXT_NOTE];         // NOTE
                fileNameText = gMenuText[ASSET_MENU_TEXT_FILENAME]; // FILENAME
                pagesText = gMenuText[ASSET_MENU_TEXT_PAGES];       // PAGES
                numberOfPages = 1;
            } else {
                if (gMenuCurIndex == (i + gOpacityDecayTimer)) {
                    // White background for currently selected row
                    set_current_dialogue_background_colour(6, 255, 255, 255, (highlight >> 1) + 128);
                } else {
                    // Yellow background for all other rows
                    set_current_dialogue_background_colour(6, 224, 224, 48, 224);
                }
                set_current_text_colour(6, 16, 16, 160, 255, 255);
                pagesText = noteText = "~";
                fileNameText = gBootPakData[gOpacityDecayTimer + i];
                numberOfPages = sCurrentControllerPakNumberOfPages[gOpacityDecayTimer + i];
            }
            render_dialogue_text(6, 26, 2, noteText, gOpacityDecayTimer + i + 1, HORZ_ALIGN_CENTER);
            render_dialogue_text(6, 56, 2, fileNameText, 1, HORZ_ALIGN_LEFT);
            render_dialogue_text(6, 240, 2, pagesText, numberOfPages, HORZ_ALIGN_CENTER);
#if VERSION >= VERSION_79
            set_kerning(TRUE);
#endif
            render_dialogue_box(&sMenuCurrDisplayList, NULL, NULL, 6);
            yPos += 16;
#if VERSION >= VERSION_79
            set_kerning(FALSE);
#endif
        }
        if (gOpacityDecayTimer < (16 - sControllerPakMenuNumberOfRows)) {
            if ((gOptionBlinkTimer & 8) != 0) {
                texrect_draw(&sMenuCurrDisplayList, gMenuSelectionArrowDown, SCREEN_WIDTH_HALF, yPos + 8, 255, 255, 255,
                             255);
                rendermode_reset(&sMenuCurrDisplayList);
            }
        } else {
            set_text_font(ASSET_FONTS_FUNFONT);
            if (gMenuCurIndex == 16) {
                set_text_colour(255, 255, 255, highlight, 255);
            } else {
                set_text_colour(255, 255, 255, 0, 255);
            }
            draw_text(&sMenuCurrDisplayList, POS_CENTRED, yPos, gMenuText[ASSET_MENU_TEXT_EXIT],
                      ALIGN_TOP_CENTER); // EXIT
        }
        if (gMenuStage != PAKMENU_CHOOSE && gCpakWriteTimer == 0) {
            if (osTvType == OS_TV_TYPE_PAL) {
                yPos = 134;
            } else {
                yPos = SCREEN_HEIGHT_HALF;
            }
            dialogue_clear(6);
            set_dialogue_font(6, ASSET_FONTS_FUNFONT);
            set_current_dialogue_box_coords(6, 76, yPos - 28, 244, yPos + 28);
            set_current_dialogue_background_colour(6, 0, 0, 0, 160);

            yPos = 4;
            for (i = 0; i <= 2; i++) {
                if (i == 0) {
                    set_current_text_colour(6, 255, 0, 255, 64, 255);
                } else if (i == gMenuStage) {
                    set_current_text_colour(6, 255, 255, 255, highlight, 255);
                } else {
                    set_current_text_colour(6, 255, 255, 255, 0, 255);
                }

                // Render these three lines in this order
                // ASSET_MENU_TEXT_DELETENOTEX  - DELETE NOTE ~ ?
                // ASSET_MENU_TEXT_DELETE       - DELETE
                // ASSET_MENU_TEXT_CANCELDELETE - CANCEL
                render_dialogue_text(6, POS_CENTRED, yPos, gMenuText[ASSET_MENU_TEXT_DELETENOTEX + i],
                                     gMenuCurIndex + 1, HORZ_ALIGN_CENTER);

                if (i != 0) {
                    yPos += 16;
                } else {
                    yPos += 20;
                }
            }
            render_dialogue_box(&sMenuCurrDisplayList, NULL, NULL, 6);
        }
        // Deleting File
        if (gCpakWriteTimer != 0) {
            set_text_colour(255, 255, 255, 0, 255);
            set_text_font(ASSET_FONTS_BIGFONT);
            draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF, 128, gMenuText[ASSET_MENU_TEXT_PLEASEWAIT],
                      ALIGN_MIDDLE_CENTER);
        }
    }
}

/**
 * Handles the controller pak menu accessed from the boot screen.
 * A simple menu that has the list of controller pak notes.
 * Pressing A on a note gives a confirmation box to delete.
 * That note will be delated after 3 frames.
 */
s32 menu_controller_pak_loop(s32 updateRate) {
    s32 i;
    s32 pressedButtons;
    s32 xStick;
    s32 yStick;
    s32 playMoveSound = FALSE;
    s32 playCancelSound = FALSE;
    s32 playSelectedSound = FALSE;
    UNUSED s32 pad;
    s32 temp_v1_2;

    gOptionBlinkTimer = (gOptionBlinkTimer + updateRate) & 0x3F;
    if (gMenuDelay < 20) {
        pakmenu_render(updateRate);
    }
    if (gMenuDelay == 0) {
        pressedButtons = 0;
        xStick = 0;
        yStick = 0;
        for (i = 0; i < MAXCONTROLLERS; i++) {
            pressedButtons |= input_pressed(i);
            xStick += gControllersXAxisDirection[i];
            yStick += gControllersYAxisDirection[i];
        }
        if (sControllerPakError != PAK_ERROR_NONE) {
            if (sControllerPakError != PAK_ERROR_FATAL && pressedButtons & (A_BUTTON | START_BUTTON)) {
                sound_play(SOUND_SELECT2, NULL);
                if (menu_find_usable_pak(&gMenuOption, 0) == CONTROLLER_PAK_GOOD) {
                    sControllerPakError = PAK_ERROR_NONE;
                } else if (check_for_controller_pak_errors() == PAK_ERROR_NONE) {
                    gShowControllerPakMenu = 0;
                }
                if (sControllerPakError == PAK_ERROR_NONE && gShowControllerPakMenu == 0) {
                    playCancelSound = TRUE;
                    gMenuDelay = 1;
                    transition_begin(&sMenuTransitionFadeIn);
                }
            }
        } else {
            if (gMenuStage != PAKMENU_CHOOSE) {
                if (gCpakWriteTimer != 0) {
                    gCpakWriteTimer--;
                    if (gCpakWriteTimer == 0) {
                        if (delete_file(gMenuOption, gMenuCurIndex) != CONTROLLER_PAK_GOOD) {
                            // Failed to delete the file
                            playCancelSound = TRUE;
                            gMenuDelay = 1;
                            transition_begin(&sMenuTransitionFadeIn);
                        } else {
                            // File deleted successfully
                            playSelectedSound = TRUE;
                            if (menu_find_usable_pak(&gMenuOption, xStick) != CONTROLLER_PAK_GOOD) {
                                playCancelSound = TRUE;
                                gMenuDelay = 1;
                                transition_begin(&sMenuTransitionFadeIn);
                            }
                        }
                        gMenuStage = PAKMENU_CHOOSE;
                    }
                } else if (pressedButtons & B_BUTTON) {
                    playCancelSound = TRUE;
                    gMenuStage = PAKMENU_CHOOSE;
                } else if (pressedButtons & (A_BUTTON | START_BUTTON)) {
                    if (gMenuStage == PAKMENU_CONFIRM) {
                        gCpakWriteTimer = 3;
                    } else {
                        playCancelSound = TRUE;
                        gMenuStage = PAKMENU_CHOOSE;
                    }
                } else if (yStick > 0 && gMenuStage > PAKMENU_CONFIRM) {
                    gMenuStage = PAKMENU_CONFIRM;
                    playMoveSound = TRUE;
                } else if (yStick < 0 && gMenuStage < PAKMENU_WRITE) {
                    gMenuStage = PAKMENU_WRITE;
                    playMoveSound = TRUE;
                }
            } else if (pressedButtons & B_BUTTON ||
                       (gMenuCurIndex == 16 && pressedButtons & (A_BUTTON | START_BUTTON))) {
                playCancelSound = 1;
                gMenuDelay = 1;
                transition_begin(&sMenuTransitionFadeIn);
            } else {
                // gMenuCurIndex = selected menu index?
                switch (gMenuCurIndex) {
                    case -1:
                        if (yStick < 0) {
                            if (!xStick) {} // Fakematch
                            gMenuCurIndex = 0;
                            playMoveSound = TRUE;
                        }
                        break;
                    case 16:
                        if (yStick > 0) {
                            gMenuCurIndex = 15;
                            playMoveSound = TRUE;
                        }
                        break;
                    default:
                        if (pressedButtons & (A_BUTTON | START_BUTTON)) {
                            // Check if the selected menu item is a file or blank
                            if ((sCurrentControllerPakAllFileTypes[gMenuCurIndex] >= SAVE_FILE_TYPE_CPAK_SAVE) &&
                                (sCurrentControllerPakAllFileTypes[gMenuCurIndex] <= SAVE_FILE_TYPE_CPAK_OTHER)) {
                                gCpakWriteTimer = 0;
                                gMenuStage = PAKMENU_WRITE;
                                playSelectedSound = TRUE;
                            } else {
                                // If it's not, just play the "back" sound
                                playCancelSound = TRUE;
                            }
                        } else {
                            if (yStick > 0) {
                                gMenuCurIndex--;
                                if (gMenuCurIndex < 0) {
                                    gMenuCurIndex = 0;
                                } else {
                                    playMoveSound = TRUE;
                                }
                            } else if (yStick < 0) {
                                gMenuCurIndex++;
                                playMoveSound = TRUE;
                            }
                        }
                        if (gMenuCurIndex >= (gOpacityDecayTimer + sControllerPakMenuNumberOfRows)) {
                            gOpacityDecayTimer = (gMenuCurIndex - sControllerPakMenuNumberOfRows) + 1;
                        }
                        if (gMenuCurIndex < gOpacityDecayTimer) {
                            gOpacityDecayTimer = gMenuCurIndex;
                        }
                        temp_v1_2 = 16 - sControllerPakMenuNumberOfRows;
                        if (temp_v1_2 < gOpacityDecayTimer) {
                            gOpacityDecayTimer = temp_v1_2;
                        }
                        if (gOpacityDecayTimer < 0) {
                            gOpacityDecayTimer = 0;
                        }
                        break;
                }
            }
        }
        if (playCancelSound) {
            // Menu Cancel Sound
            sound_play(SOUND_MENU_BACK3, NULL);
        } else if (playSelectedSound) {
            // Menu Item Selected Sound
            sound_play(SOUND_SELECT2, NULL);
        } else if (playMoveSound) {
            // Menu Moving Sound
            sound_play(SOUND_MENU_PICK2, NULL);
        }
    } else {
        gMenuDelay += updateRate;
        if (gMenuDelay >= 36) {

            pakmenu_free();
            menu_init(MENU_LOGOS);
            load_level_for_menu(ASSET_LEVEL_FRONTEND, ZERO_PLAYERS, CUTSCENE_NONE);
        }
    }
    return 0;
}

/**
 * Free all assets associated with the boot sequence controller pak menu.
 */
void pakmenu_free(void) {
    menu_asset_free(TEXTURE_ICON_ARROW_DOWN);
    mempool_free(gBootPakData[0]);
    unload_font(ASSET_FONTS_BIGFONT);
#if VERSION >= VERSION_79
    rumble_enable(TRUE);
#endif
}
