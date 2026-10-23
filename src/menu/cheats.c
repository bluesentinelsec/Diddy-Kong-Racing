#include "menu_internal.h"

static s16 gCheatInputCurrentRow;

static s16 gCheatInputCurrentColumn;

static s16 gCheatInputStringLength;

static s16 gNewCheatID;

static char gCheatInput[20];

static s32 gNumOnscreenMagicCodes;

static s16 gUnlockedCheatIDs[32];

#define CHEATS_ALLOWED_IN_ADVENTURE_AND_TIME_TRIAL                                                                   \
    (CHEAT_CONTROL_TT | CHEAT_CONTROL_DRUMSTICK | CHEAT_BIG_CHARACTERS | CHEAT_SMALL_CHARACTERS | CHEAT_HORN_CHEAT | \
     CHEAT_SELECT_SAME_PLAYER | CHEAT_TWO_PLAYER_ADVENTURE | CHEAT_ULTIMATE_AI | CHEAT_EPC_LOCK_UP_DISPLAY |         \
     CHEAT_ROM_CHECKSUM)

// Remember that ~ means NOT. So the challenge maps only allow cheats NOT included here.
#define CHEATS_ALLOWED_IN_CHALLENGES                                                                             \
    ~(CHEAT_START_WITH_10_BANANAS | CHEAT_DISABLE_BANANAS | CHEAT_DISABLE_WEAPONS | CHEAT_ALL_BALLOONS_ARE_RED | \
      CHEAT_ALL_BALLOONS_ARE_GREEN | CHEAT_ALL_BALLOONS_ARE_BLUE | CHEAT_ALL_BALLOONS_ARE_YELLOW |               \
      CHEAT_ALL_BALLOONS_ARE_RAINBOW)

/**
 * Initialises all the cheat menu variables and sets up a text box containing the keyboard.
 */
void menu_magic_codes_init(void) {
    gCheatInputCurrentRow = 0;
    gCheatInputCurrentColumn = 0;
    gOptionsMenuItemIndex = 0;
    gCheatInputStringLength = 0;
    sUnused_80126470 = 0;
    gNewCheatID = -1;
    gCheatInput[0] = '\0';
    gOptionBlinkTimer = 0;
    gMenuDelay = 0;
    gOpacityDecayTimer = 0;
    gMenuStage = CHEATMENU_CHOOSE;
    transition_begin(&sMenuTransitionFadeOut);
    set_current_dialogue_box_coords(7, 50, 50, 270, 132);
    set_current_dialogue_background_colour(7, 0, 0, 0, 128);
    dialogue_clear(7);
    load_font(ASSET_FONTS_BIGFONT);
}

/**
 * Render the onscreen keyboard and selection options.
 * Also display the current input string, or a success/error message.
 */
void cheatmenu_render(UNUSED s32 updateRate) {
    u16 *cheatData;
    s32 i, j;
    s32 highlight, highlight2, green;
    s32 xPos, yPos;
    char displayChar[2];
    s32 chr;
    UNUSED s32 pad1;
    s32 offsetY;

    set_text_background_colour(0, 0, 0, 0);
    render_dialogue_box(&sMenuCurrDisplayList, NULL, NULL, 7);
    set_text_font(ASSET_FONTS_BIGFONT);
    set_text_colour(0, 0, 0, 255, 128);
    draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF + 1, 35, gMenuText[ASSET_MENU_TEXT_MAGICCODES],
              ALIGN_MIDDLE_CENTER); // "MAGIC CODES"
    set_text_colour(255, 255, 255, 0, 255);
    draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF, 32, gMenuText[ASSET_MENU_TEXT_MAGICCODES],
              ALIGN_MIDDLE_CENTER); // "MAGIC CODES"
    chr = 'A';
    set_text_font(ASSET_FONTS_FUNFONT);
    set_text_colour(255, 255, 255, 0, 255);
    // Draw the input table
    for (i = 0, yPos = 60; i < 4; i++, yPos += 22) {
        for (j = 0, xPos = 64; j < 7; j++, xPos += 32) {
            if (gOptionsMenuItemIndex == 5 && j == gCheatInputCurrentColumn && i == gCheatInputCurrentRow) {
                highlight = gOptionBlinkTimer * 8;
                if (highlight >= 256) {
                    highlight = 511 - highlight;
                }
                highlight >>= 1;
                highlight += 128;
                set_text_colour(128, 255, 192, highlight, 255);
            }
            if (chr <= 'Z') {
                displayChar[0] = chr;
                displayChar[1] = '\0';
                draw_text(&sMenuCurrDisplayList, xPos, yPos, displayChar, ALIGN_MIDDLE_CENTER); // Draw the character
            } else if (j == 5) {
                draw_text(&sMenuCurrDisplayList, xPos, yPos, "DEL", ALIGN_MIDDLE_CENTER);
            } else {
                draw_text(&sMenuCurrDisplayList, xPos, yPos, "OK", ALIGN_MIDDLE_CENTER);
            }
            if ((5 == gOptionsMenuItemIndex) && (j == gCheatInputCurrentColumn) && (i == gCheatInputCurrentRow)) {
                set_text_colour(255, 255, 255, 0, 255);
            }
            chr = (chr + 1) & 0xFF;
        }
    }

    highlight = gOptionBlinkTimer * 8;
    if (highlight > 255) {
        highlight = 511 - highlight;
    }

    set_text_font(ASSET_FONTS_FUNFONT);
    i = 0;
    offsetY = 164;
    yPos = 16;
    if (osTvType == OS_TV_TYPE_PAL) {
        yPos = 24;
    }

    while (gMagicCodeMenuStrings[i] != NULL) {
        highlight2 = 0;
        if (i == gOptionsMenuItemIndex && gMenuStage == CHEATMENU_CHOOSE) {
            highlight2 = highlight;
        }
        set_text_colour(255, 255, 255, highlight2, 255);
        draw_text(&sMenuCurrDisplayList, POS_CENTRED, offsetY, gMagicCodeMenuStrings[i], ALIGN_MIDDLE_CENTER);
        i++;
        offsetY += yPos;
    }
    set_text_font(ASSET_FONTS_FUNFONT);
    set_text_colour(255, 255, 128, 128, 255);
    set_text_background_colour(0, 0, 128, 128);
    if (gOptionsMenuItemIndex == 4) {
        if (gNewCheatID == -1) {
            draw_text(&sMenuCurrDisplayList, POS_CENTRED, 144, gMenuText[ASSET_MENU_TEXT_BADCODE],
                      ALIGN_MIDDLE_CENTER); //"Sorry, the code was incorrect"
        } else {
            // Draw cheat code name.
            cheatData = (*gCheatsAssetData) + 1;
            draw_text(&sMenuCurrDisplayList, POS_CENTRED, 144,
                      &((char *) (*gCheatsAssetData))[cheatData[gNewCheatID + 1]], ALIGN_MIDDLE_CENTER);
        }
    } else if (5 == gOptionsMenuItemIndex) {
        draw_text(&sMenuCurrDisplayList, POS_CENTRED, 144, gCheatInput, ALIGN_MIDDLE_CENTER);
    } else if (gOptionsMenuItemIndex == 6) {
        draw_text(&sMenuCurrDisplayList, POS_CENTRED, 144, gMenuText[ASSET_MENU_TEXT_ALLCODESDELETED],
                  ALIGN_MIDDLE_CENTER); //"All cheats have been deleted"
    }
    if (gMenuStage != CHEATMENU_CHOOSE) {
        if (osTvType == OS_TV_TYPE_PAL) {
            offsetY = SCREEN_HEIGHT_HALF + 14;
        } else {
            offsetY = SCREEN_HEIGHT_HALF;
        }
        dialogue_clear(6);
        set_dialogue_font(6, ASSET_FONTS_FUNFONT);
        set_current_dialogue_box_coords(6, 76, offsetY - 28, 244, offsetY + 28);
        set_current_dialogue_background_colour(6, 0, 0, 0, 160);
        offsetY = 4;
        for (i = 0; i < 3; i++) {
            green = 255;
            highlight2 = 0;
            if (i == 0) {
                green = 0;
                highlight2 = 64;
            } else if (i == gMenuStage) {
                highlight2 = highlight;
            }
            set_current_text_colour(6, 255, green, 255, highlight2, 255);
            // gMenuText[148] = "CLEAR ALL CODES?", [149] = "OK", [150] = "CANCEL"
            render_dialogue_text(6, POS_CENTRED, offsetY, gMenuText[148 + i], 1, HORZ_ALIGN_CENTER);

            if (i != 0) {
                offsetY += 16;
            } else {
                offsetY += 20;
            }
        }
        render_dialogue_box(&sMenuCurrDisplayList, NULL, NULL, 6);
    }
    if (get_filtered_cheats() & CHEAT_ROM_CHECKSUM) {
        cheatmenu_checksum();
    }
}

/**
 * Handles the cheat code menu.
 * Takes input for the onscreen keyboard from any player.
 * Generates a string based on the input, and when asked,
 * compares it with the cheat code table to see if it matches.
 */
s32 menu_magic_codes_loop(s32 updateRate) {
    s32 i;
    s32 buttonsPressed;
    s32 xDir;
    s32 yDir;
    s32 someBool;
    s32 someBool2;
    s32 foundCheat;
    s32 menuDelay;
    s32 playBackSound;
    s32 playSelectSound;
    s32 playPickSound;
    s32 prevValue;
    char *cheatCodeText;
    u16 *cheatDataEntries;

    someBool = FALSE;
    someBool2 = FALSE;
    menuDelay = 0;
    foundCheat = FALSE;
    playBackSound = FALSE;
    playSelectSound = FALSE;
    playPickSound = FALSE;
    if (gOpacityDecayTimer != 0) {
        gOpacityDecayTimer -= updateRate;
        if (gOpacityDecayTimer < 0) {
            gOpacityDecayTimer = 0;
        }
    }
    if (gMenuDelay != 0) {
        if (gMenuDelay > 0) {
            gMenuDelay += updateRate;
        } else {
            gMenuDelay -= updateRate;
        }
    }
    gOptionBlinkTimer = (gOptionBlinkTimer + updateRate) & 0x3F;
    if (gMenuDelay > -20 && gMenuDelay < 20) {
        foundCheat = FALSE;
        cheatmenu_render(updateRate);
    }
    buttonsPressed = 0;
    xDir = 0;
    yDir = 0;
    if (gIgnorePlayerInputTime == 0 && gMenuDelay == 0) {
        for (i = 0; i < MAXCONTROLLERS; i++) {
            buttonsPressed |= input_pressed(i);
            xDir += gControllersXAxisDirection[i];
            yDir += gControllersYAxisDirection[i];
        }
    }
    if (gOptionsMenuItemIndex == 4) {
        if (gOpacityDecayTimer == 0 || buttonsPressed & (A_BUTTON | START_BUTTON)) {
            gOptionsMenuItemIndex = 0;
        }
    } else if (gOptionsMenuItemIndex == 5) {
        prevValue = ((gCheatInputCurrentColumn & 0xF) * 16) | gCheatInputCurrentRow;
        if (xDir < 0) {
            gCheatInputCurrentColumn--;
            if (gCheatInputCurrentColumn < 0) {
                gCheatInputCurrentColumn = 6;
            }
        }
        if (xDir > 0) {
            gCheatInputCurrentColumn++;
            if (gCheatInputCurrentColumn > 6) {
                gCheatInputCurrentColumn = 0;
            }
        }
        if (yDir < 0) {
            gCheatInputCurrentRow++;
            if (gCheatInputCurrentRow > 3) {
                gCheatInputCurrentRow = 0;
            }
        }
        if (yDir > 0) {
            gCheatInputCurrentRow--;
            if (gCheatInputCurrentRow < 0) {
                gCheatInputCurrentRow = 3;
            }
        }
        if ((((gCheatInputCurrentColumn & 0xF) * 16) | gCheatInputCurrentRow) != prevValue) {
            playPickSound = TRUE;
        }
        if (buttonsPressed & A_BUTTON) {
            if (gCheatInputCurrentRow == 3 && gCheatInputCurrentColumn == 5) {
                someBool = TRUE;
            } else if (gCheatInputCurrentRow == 3 && gCheatInputCurrentColumn == 6) {
                someBool2 = TRUE;
            } else if (gCheatInputStringLength <= 18) {
                playSelectSound = TRUE;
                gCheatInput[gCheatInputStringLength] =
                    (gCheatInputCurrentRow * 7) + gCheatInputCurrentColumn + 'A'; // Convert to upper case?
                gCheatInputStringLength++;
                gCheatInput[gCheatInputStringLength] = '\0';
            }
        }
        if (buttonsPressed & B_BUTTON) {
            if (gCheatInputStringLength > 0) {
                someBool = TRUE;
            } else {
                gOptionsMenuItemIndex = 0;
            }
        }
        if (buttonsPressed & START_BUTTON) {
            someBool2 = TRUE;
        }
        if (someBool) {
            sound_play(SOUND_BOUNCE, NULL);
            if (gCheatInputStringLength > 0) {
                gCheatInputStringLength--;
                gCheatInput[gCheatInputStringLength] = '\0';
            }
        }
        if (someBool2) {
            playSelectSound = TRUE;
            if (gCheatInputStringLength == 0) {
                gOptionsMenuItemIndex = 0;
            } else {
                cheatDataEntries = &(*gCheatsAssetData)[1];
                gNewCheatID = 0; // Index into the gCheatsAssetData cheatsTable
#if VERSION >= VERSION_79
                while (gNewCheatID < gNumberOfCheats && foundCheat == FALSE) {
                    foundCheat = TRUE;
                    cheatCodeText = ((char *) (*gCheatsAssetData)) + cheatDataEntries[gNewCheatID * 3];
#else
                while (gNewCheatID < gNumberOfCheats * 2 && foundCheat == FALSE) {
                    foundCheat = TRUE;
                    cheatCodeText = ((char *) (*gCheatsAssetData)) + cheatDataEntries[gNewCheatID];
#endif
                    // gCheatInput = char *gPlayerEnteredCode; (Or something like that)
                    i = -1;
                    do {
                        i++;
                        if (cheatCodeText[i] != gCheatInput[i]) {
                            foundCheat = FALSE;
                        }
                        if (foundCheat == FALSE) {
#if VERSION == VERSION_77
                            do {
                            } while (0); // Fakematch only for v1
#endif
                            break;
                        }
                    } while (cheatCodeText[i] != '\0' && gCheatInput[i] != '\0');
                    if (foundCheat) {
                        continue;
                    }
#if VERSION >= VERSION_79
                    gNewCheatID += 1;
#else
                    gNewCheatID += 2;
#endif
                }
                if (foundCheat == FALSE) {
                    gNewCheatID = -1;
                } else {
#if VERSION >= VERSION_79
                    gUnlockedMagicCodes |= 1 << gNewCheatID;
                    gActiveMagicCodes |= 1 << gNewCheatID;
                    gNewCheatID *= 3;
#else
                    gUnlockedMagicCodes |= 1 << (gNewCheatID >> 1);
                    gActiveMagicCodes |= 1 << (gNewCheatID >> 1);
#endif
                }
                gOptionsMenuItemIndex = 4;
                gOpacityDecayTimer = 240;
            }
        }
    } else if (gOptionsMenuItemIndex == 6) {
        if (gOpacityDecayTimer == 0 || buttonsPressed & (A_BUTTON | START_BUTTON)) {
            gOptionsMenuItemIndex = 1;
        }
    } else if (gMenuStage != CHEATMENU_CHOOSE) {
        if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
            if (gMenuStage == CHEATMENU_KEYBOARD) {
                gActiveMagicCodes = 0;
                gOptionsMenuItemIndex = 6;
                playSelectSound = TRUE;
                gUnlockedMagicCodes &= CHEAT_CONTROL_TT | CHEAT_CONTROL_DRUMSTICK;
                gOpacityDecayTimer = 240;
            } else {
                playBackSound = TRUE;
            }
            gMenuStage = CHEATMENU_CHOOSE;
        } else if (buttonsPressed & B_BUTTON) {
            playBackSound = TRUE;
            gMenuStage = CHEATMENU_CHOOSE;
        } else if (yDir < 0 && gMenuStage == 1) {
            playPickSound = TRUE;
            gMenuStage = CHEATMENU_MESSAGE;
        } else if (yDir > 0 && gMenuStage == CHEATMENU_MESSAGE) {
            playPickSound = TRUE;
            gMenuStage = CHEATMENU_KEYBOARD;
        }
    } else {
        prevValue = gOptionsMenuItemIndex;
        if (yDir < 0) {
            gOptionsMenuItemIndex += 1;
            if (gOptionsMenuItemIndex > 3) {
                gOptionsMenuItemIndex = 3;
            }
        }
        if (yDir > 0) {
            gOptionsMenuItemIndex -= 1;
            if (gOptionsMenuItemIndex < 0) {
                gOptionsMenuItemIndex = 0;
            }
        }
        if (prevValue != gOptionsMenuItemIndex) {
            playPickSound = TRUE;
        }
        if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
            playSelectSound = TRUE;
            if (gOptionsMenuItemIndex == 3) {
                menuDelay = -1;
            } else if (gOptionsMenuItemIndex == 0) {
                gCheatInputCurrentRow = 0;
                gCheatInputCurrentColumn = 0;
                gCheatInputStringLength = 0;
                gCheatInput[0] = '\0';
                gOptionsMenuItemIndex = 5;
            } else if (gOptionsMenuItemIndex == 1) {
                gMenuStage = CHEATMENU_MESSAGE;
            } else if (gOptionsMenuItemIndex == 2) {
                menuDelay = 1;
            }
        }
        if (buttonsPressed & (B_BUTTON)) {
            menuDelay = -1;
        }
        if (menuDelay != 0) {
            if (menuDelay < 0) {
                playBackSound = TRUE;
            }
            gMenuDelay = menuDelay;
            transition_begin(&sMenuTransitionFadeIn);
        }
    }
    if (playBackSound) {
        sound_play(SOUND_MENU_BACK3, NULL);
    } else if (playSelectSound) {
        sound_play(SOUND_SELECT2, NULL);
    } else if (playPickSound) {
        sound_play(SOUND_MENU_PICK2, NULL);
    }

    gIgnorePlayerInputTime = 0;
    if (gMenuDelay < -30 || gMenuDelay > 30) {
        cheatmenu_free();
        if (gMenuDelay < 0) {
            if (gActiveMagicCodes & CHEAT_DISPLAY_CREDITS) {
                music_change_on();
                gViewingCreditsFromCheat = TRUE;
                menu_init(MENU_CREDITS);
                gActiveMagicCodes &= ~CHEAT_DISPLAY_CREDITS;
            } else {
                menu_init(MENU_OPTIONS);
            }
        } else {
            menu_init(MENU_MAGIC_CODES_LIST);
        }
    }
    return MENU_RESULT_CONTINUE;
}

/**
 * Free all assets associated with the cheat code menu.
 */
void cheatmenu_free(void) {
    unload_font(ASSET_FONTS_BIGFONT);
}

/**
 * Initialise the magic list menu.
 * Sets the max list height based on region, and loads the arrows for scrolling.
 */
void menu_magic_codes_list_init(void) {
    gOptionBlinkTimer = 0;
    gMenuDelay = 0;
    gMenuStage = 0;
    gOptionsMenuItemIndex = 0;
    load_font(ASSET_FONTS_BIGFONT);
    menu_asset_load(63);
    menu_init_arrow_textures();
    transition_begin(&sMenuTransitionFadeOut);
    if (osTvType == OS_TV_TYPE_PAL) {
        gNumOnscreenMagicCodes = 11;
    } else {
        gNumOnscreenMagicCodes = 10;
    }
}

/**
 * Renders all registered cheat codes in a list, starting from the top.
 * Once it reaches the end, render the back button too.
 */
void cheatlist_render(UNUSED s32 updateRate) {
    s32 i;
    s32 alpha;
    s32 yPos;
    s32 numOfUnlockedCheats;
    s32 code;
    u16 *cheatData;

    set_text_background_colour(0, 0, 0, 0);
    set_text_font(ASSET_FONTS_BIGFONT);
    set_text_colour(0, 0, 0, 255, 128);
    draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF + 1, 35, gMenuText[ASSET_MENU_TEXT_MAGICCODESLIST],
              ALIGN_MIDDLE_CENTER); // MAGIC CODES LIST
    set_text_colour(255, 255, 255, 0, 255);
    draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF, 32, gMenuText[ASSET_MENU_TEXT_MAGICCODESLIST],
              ALIGN_MIDDLE_CENTER); // MAGIC CODES LIST

    for (i = 0, code = 1, numOfUnlockedCheats = 0; i < 32; i++, code <<= 1) {
        if (code & gUnlockedMagicCodes) {
            gUnlockedCheatIDs[numOfUnlockedCheats] = i;
            numOfUnlockedCheats += 1;
        }
    }
    yPos = 54;
    alpha = gOptionBlinkTimer * 8;
    if (alpha >= 256) {
        alpha = 511 - alpha;
    }
    set_text_font(ASSET_FONTS_FUNFONT);
    set_text_colour(255, 255, 255, 0, 255);
    for (i = gMenuStage, cheatData = (*gCheatsAssetData) + 1;
         (i < gMenuStage + gNumOnscreenMagicCodes) && (i < numOfUnlockedCheats); i++, yPos += 16) {
        if (i == gOptionsMenuItemIndex) {
            set_text_colour(255, 255, 255, alpha, 255);
        }
        draw_text(&sMenuCurrDisplayList, 48, yPos,
#if VERSION == VERSION_79
                  (char *) (*gCheatsAssetData) + cheatData[(gUnlockedCheatIDs[i] * 3) + 2], ALIGN_TOP_LEFT);
#elif VERSION == VERSION_80
                  (char *) (*gCheatsAssetData) + cheatData[(gUnlockedCheatIDs[i] * 3) + 1], ALIGN_TOP_LEFT);
#else
                  (char *) (*gCheatsAssetData) + cheatData[(gUnlockedCheatIDs[i] * 2) + 1], ALIGN_TOP_LEFT);
#endif
        if ((1 << gUnlockedCheatIDs[i]) & gActiveMagicCodes) {
            draw_text(&sMenuCurrDisplayList, 256, yPos, gMenuText[ASSET_MENU_TEXT_ON], ALIGN_TOP_LEFT); // ON
        } else {
            draw_text(&sMenuCurrDisplayList, 256, yPos, gMenuText[ASSET_MENU_TEXT_OFF], ALIGN_TOP_LEFT); // OFF
        }
        if (i == gOptionsMenuItemIndex) {
            set_text_colour(255, 255, 255, 0, 255);
        }
    }
    if (i < gMenuStage + gNumOnscreenMagicCodes) {
        if (numOfUnlockedCheats == gOptionsMenuItemIndex) {
            set_text_colour(255, 255, 255, alpha, 255);
        }
        draw_text(&sMenuCurrDisplayList, POS_CENTRED, yPos, gMenuText[ASSET_MENU_TEXT_RETURN],
                  ALIGN_TOP_CENTER); // RETURN
        return;
    }
    if (gOptionBlinkTimer & 8) {
        texrect_draw(&sMenuCurrDisplayList, gMenuSelectionArrowDown, SCREEN_WIDTH_HALF, yPos + 8, 255, 255, 255, 255);
    }
}

/**
 * Checks if cheatA is active, and disables cheatB if it is.
 * This ensures incompatable cheats do not stay enabled.
 */
void cheatlist_exclusive(s32 code, s32 cheatA, s32 cheatB) {
    if ((code & cheatA) && (code & gActiveMagicCodes)) {
        gActiveMagicCodes &= ~cheatB;
    }
}

/**
 * Lists each active cheat and allows the player to enable or disable them at will.
 * Some cheats are incompatable with each other, so those are checked to ensure safety.
 */
s32 menu_magic_codes_list_loop(s32 updateRate) {
    s32 previousMenuItemIndex;
    s32 xAxis;
    s32 yAxis;
    s32 delay;
    s32 buttonsPressed;
    s32 i;
    s32 numUnlockedCodes;
    s32 code;

    delay = 0;
    if (gMenuDelay != 0) {
        if (gMenuDelay > 0) {
            gMenuDelay += updateRate;
        } else {
            gMenuDelay -= updateRate;
        }
    }
    gOptionBlinkTimer = (gOptionBlinkTimer + updateRate) & 0x3F;
    if (gMenuDelay > -20 && gMenuDelay < 20) {
        cheatlist_render(updateRate);
    }

    buttonsPressed = 0;
    xAxis = 0;
    yAxis = 0;
    if (gIgnorePlayerInputTime == 0 && gMenuDelay == 0) {
        for (i = 0; i < MAXCONTROLLERS; i++) {
            buttonsPressed |= input_pressed(i);
            xAxis += gControllersXAxisDirection[i];
            yAxis += gControllersYAxisDirection[i];
        }
    }

    for (i = 0, code = 1, numUnlockedCodes = 0; i < MAX_CHEATS; i++) {
        if (code & gUnlockedMagicCodes) {
            gUnlockedCheatIDs[numUnlockedCodes] = i;
            numUnlockedCodes++;
        }
        code <<= 1;
    }

    if ((xAxis < 0 || xAxis > 0) && numUnlockedCodes != gOptionsMenuItemIndex) {
        sound_play(SOUND_SELECT2, NULL);
        code = 1 << gUnlockedCheatIDs[gOptionsMenuItemIndex];
        gActiveMagicCodes ^= code;                                               // Toggle active cheats?
        cheatlist_exclusive(code, CHEAT_BIG_CHARACTERS, CHEAT_SMALL_CHARACTERS); // cheatlist_exclusive() = Clear flags?
        cheatlist_exclusive(code, CHEAT_SMALL_CHARACTERS, CHEAT_BIG_CHARACTERS);
        cheatlist_exclusive(code, CHEAT_DISABLE_BANANAS,
                            CHEAT_NO_LIMIT_TO_BANANAS | CHEAT_BANANAS_REDUCE_SPEED | CHEAT_START_WITH_10_BANANAS);
        cheatlist_exclusive(code, CHEAT_NO_LIMIT_TO_BANANAS | CHEAT_BANANAS_REDUCE_SPEED | CHEAT_START_WITH_10_BANANAS,
                            CHEAT_DISABLE_BANANAS);
        cheatlist_exclusive(code, CHEAT_DISABLE_WEAPONS,
                            CHEAT_ALL_BALLOONS_ARE_RED | CHEAT_ALL_BALLOONS_ARE_GREEN | CHEAT_ALL_BALLOONS_ARE_BLUE |
                                CHEAT_ALL_BALLOONS_ARE_YELLOW | CHEAT_ALL_BALLOONS_ARE_RAINBOW |
                                CHEAT_MAXIMUM_POWER_UP);
        cheatlist_exclusive(code,
                            CHEAT_ALL_BALLOONS_ARE_RED | CHEAT_ALL_BALLOONS_ARE_GREEN | CHEAT_ALL_BALLOONS_ARE_BLUE |
                                CHEAT_ALL_BALLOONS_ARE_YELLOW | CHEAT_ALL_BALLOONS_ARE_RAINBOW | CHEAT_MAXIMUM_POWER_UP,
                            CHEAT_DISABLE_WEAPONS);
        cheatlist_exclusive(code, CHEAT_ALL_BALLOONS_ARE_RED,
                            CHEAT_ALL_BALLOONS_ARE_GREEN | CHEAT_ALL_BALLOONS_ARE_BLUE | CHEAT_ALL_BALLOONS_ARE_YELLOW |
                                CHEAT_ALL_BALLOONS_ARE_RAINBOW);
        cheatlist_exclusive(code, CHEAT_ALL_BALLOONS_ARE_GREEN,
                            CHEAT_ALL_BALLOONS_ARE_RED | CHEAT_ALL_BALLOONS_ARE_BLUE | CHEAT_ALL_BALLOONS_ARE_YELLOW |
                                CHEAT_ALL_BALLOONS_ARE_RAINBOW);
        cheatlist_exclusive(code, CHEAT_ALL_BALLOONS_ARE_BLUE,
                            CHEAT_ALL_BALLOONS_ARE_RED | CHEAT_ALL_BALLOONS_ARE_GREEN | CHEAT_ALL_BALLOONS_ARE_YELLOW |
                                CHEAT_ALL_BALLOONS_ARE_RAINBOW);
        cheatlist_exclusive(code, CHEAT_ALL_BALLOONS_ARE_YELLOW,
                            CHEAT_ALL_BALLOONS_ARE_RED | CHEAT_ALL_BALLOONS_ARE_GREEN | CHEAT_ALL_BALLOONS_ARE_BLUE |
                                CHEAT_ALL_BALLOONS_ARE_RAINBOW);
        cheatlist_exclusive(code, CHEAT_ALL_BALLOONS_ARE_RAINBOW,
                            CHEAT_ALL_BALLOONS_ARE_RED | CHEAT_ALL_BALLOONS_ARE_GREEN | CHEAT_ALL_BALLOONS_ARE_BLUE |
                                CHEAT_ALL_BALLOONS_ARE_YELLOW);
    }

    previousMenuItemIndex = gOptionsMenuItemIndex;

    if (yAxis < 0) {
        gOptionsMenuItemIndex++;
        if (numUnlockedCodes < gOptionsMenuItemIndex) {
            gOptionsMenuItemIndex = numUnlockedCodes;
        }
    }
    if (yAxis > 0) {
        gOptionsMenuItemIndex--;
        if (gOptionsMenuItemIndex < 0) {
            gOptionsMenuItemIndex = 0;
        }
    }

    if (gOptionsMenuItemIndex < gMenuStage) {
        gMenuStage = gOptionsMenuItemIndex;
    } else if (gOptionsMenuItemIndex >= (gMenuStage + gNumOnscreenMagicCodes)) {
        gMenuStage = (gOptionsMenuItemIndex - gNumOnscreenMagicCodes) + 1;
    }

    if (gOptionsMenuItemIndex != previousMenuItemIndex) {
        sound_play(SOUND_MENU_PICK2, NULL);
    }

    if ((buttonsPressed & (A_BUTTON | START_BUTTON)) && (numUnlockedCodes == gOptionsMenuItemIndex)) {
        delay = -1;
    }
    if (buttonsPressed & B_BUTTON) {
        delay = -1;
    }
    if (delay != 0) {
        gMenuDelay = delay;
        transition_begin(&sMenuTransitionFadeIn);
        sound_play(SOUND_MENU_BACK3, NULL);
    }
    if (gMenuDelay < -30) {
        cheatlist_free();
        menu_init(MENU_MAGIC_CODES);
        return MENU_RESULT_CONTINUE;
    }
    gIgnorePlayerInputTime = 0;
    return MENU_RESULT_CONTINUE;
}

/**
 * Free the assets associated with the cheat code list.
 */
void cheatlist_free(void) {
    menu_asset_free(TEXTURE_ICON_ARROW_DOWN);
    unload_font(ASSET_FONTS_BIGFONT);
}

/**
 * Calculates the checksum from the final few kilobytes of ROM.
 * Renders it to the screen afterwards.
 */
void cheatmenu_checksum(void) {
    static u32 cksum = 0;
    static u32 calculated = 0;
    s32 blockOffs;
    s32 size;
    s32 numBytes, numLeft;
    s32 i;
    u8 *tempBuffer;

    if (!calculated) {
        numBytes = 0x1000;
        size = 0x5000;
        blockOffs = 0x1000;
        numLeft = (s32) &__ROM_END;
        numLeft -= 0x1000;
        tempBuffer = mempool_alloc_safe(size, COLOUR_TAG_BLACK);

        while (numLeft != 0) {
            numBytes = numLeft;
            if (numBytes > size) {
                numBytes = size;
            }
            dmacopy(blockOffs, (uintptr_t) tempBuffer, numBytes);
            numLeft -= numBytes;

            for (i = 0; i < numBytes; i++) {
                cksum += tempBuffer[i];
            }
            blockOffs += numBytes;
        }

        mempool_free(tempBuffer);
        calculated = TRUE;
    }

    set_render_printf_background_colour(0, 0, 0, 128);
    set_render_printf_position(32, 136);
    numLeft = (s32) &__ROM_END;
    render_printf("ROM checksum %08X,%d\n", cksum, numLeft);
}

/**
 * Sets the active & unlocked magic code flags.
 */
void set_magic_code_flags(s32 flags) {
    gActiveMagicCodes |= flags;
    gUnlockedMagicCodes |= flags;
}

/**
 * Filters active cheats based on different conditions. Also enables mirroring for Adventure 2.
 */
s32 get_filtered_cheats(void) {
    s32 cheats = gActiveMagicCodes;
    if (!gIsInTracksMode || is_time_trial_enabled()) {
        cheats &= CHEATS_ALLOWED_IN_ADVENTURE_AND_TIME_TRIAL;
    }
    if (!level_is_race()) {
        cheats &= ~CHEAT_MIRRORED_TRACKS; // Disable mirroring
    }
    if (leveltable_type(get_settings()->courseId) & RACETYPE_CHALLENGE) {
        cheats &= CHEATS_ALLOWED_IN_CHALLENGES;
    }
    if (gIsInAdventureTwo && level_is_race()) {
        cheats |= CHEAT_MIRRORED_TRACKS; // Enable mirroring
    }
    return cheats;
}
