#include "host.h"
#include "menu_internal.h"

static s32 gFileCopy;

static s32 gFileErase;

static s32 gSaveFileIndex3;

static s32 gSaveFileIndex2;

static s32 gFileConfirm;

static s32 gNameEntryStickHeld;

static s32 gNameEntryStickX;

static s32 gNameSelectionDone;

static f32 gNameEntryOffsetX;

static s32 *gNameEntryTargetX;

static char *gNameEntryString;

static s32 gNameEntryAllowedLength;

static char gCurFilenameCharBeingDrawn = '\0';

static s32 gEnterInitalsY = 192;

static s32 gFilenameX = 160;

static s32 gFilenameY = 120;

static s32 gFilenameFont = ASSET_FONTS_BIGFONT;

static s32 gNameEntryLength = 0;

static s32 gIndexOfCurInputCharacter = 0;

static SavefileInfo gSavefileInfo[4];

static s16 gFileSelectObjectIndices[6] = { 0x00, 0x43, 0x40, 0x41, 0x42, -1 };

static s16 gFileSelectImageIndices[6] = { 0x00, 0x0B, 0x0C, 0x0A, -1, 0 };

static ButtonElement gFileSelectButtons[3] = {
    { 24, 81, 88, 64, 4, 4, GPACK_RGBA5551(64, 16, 0, 0), GPACK_RGBA5551(255, 255, 0, 0) },  // File A
    { 116, 81, 88, 64, 4, 4, GPACK_RGBA5551(64, 16, 0, 0), GPACK_RGBA5551(255, 255, 0, 0) }, // File B
    { 208, 81, 88, 64, 4, 4, GPACK_RGBA5551(64, 16, 0, 0), GPACK_RGBA5551(255, 255, 0, 0) }, // File C
};

static s16 gFileSelectElementPos[10] = {
    44, 54, // game name
    21, 25, // Balloon icon
    44, 27, // New Text
    63, 25, // Balloon count
    45, 25  // Balloon count x
};

// Valid characters for name input. Must be u8, not char.
// The length of the array must be a power of two.
static u8 gFileNameValidChars[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.?    ";

void init_save_data(void) {
    s32 numLevels;
    s32 numWorlds;
    s32 i;
    s32 courseFlagsPtrSize;
    s32 index;
    s32 saveFileSize;
    s32 offset;

    level_count(&numLevels, &numWorlds);
    courseFlagsPtrSize = numLevels * sizeof(s32);
    saveFileSize = courseFlagsPtrSize;
    saveFileSize += numWorlds * sizeof(s16); // balloonsPtrSize;
    saveFileSize += sizeof(Settings);
    saveFileSize = (saveFileSize + 3) & ~3; // align to a 4-byte boundary

    *gRaceSetup.saveFiles = mempool_alloc_safe(saveFileSize * ARRAY_COUNT(gRaceSetup.saveFiles), COLOUR_TAG_WHITE);

    for (index = 0, offset = 0; index < ARRAY_COUNT(gRaceSetup.saveFiles); index++) {
        gRaceSetup.saveFiles[index] = (Settings *) ((u8 *) *gRaceSetup.saveFiles + offset);
        gRaceSetup.saveFiles[index]->courseFlagsPtr = (s32 *) ((u8 *) gRaceSetup.saveFiles[index] + sizeof(Settings));
        gRaceSetup.saveFiles[index]->balloonsPtr =
            (s16 *) ((u8 *) gRaceSetup.saveFiles[index]->courseFlagsPtr + courseFlagsPtrSize);
        offset += saveFileSize;
    }

    gCheatsAssetData = (u16(*)[30]) get_misc_asset(ASSET_MISC_MAGIC_CODES);
    gRaceSetup.numberOfCheats = (*gCheatsAssetData)[0];
#if VERSION >= VERSION_79
    gMenuLoaded.text = mempool_alloc_safe(1280 * sizeof(char *), COLOUR_TAG_WHITE);
#else
    gMenuLoaded.text = mempool_alloc_safe(1024 * sizeof(char *), COLOUR_TAG_WHITE);
#endif
    load_menu_text(LANGUAGE_ENGLISH);

    // clang-format off
    for (i = 0; i < ARRAY_COUNT(gMenuLoaded.assets); i++) { \
        gMenuLoaded.assets[i] = NULL;
    }
    // clang-format on
}

/**
 * Initialises the file selection menu. Loads the relevant textures for the menu.
 * Sets the audio channels to match the selected character.
 */
void menu_file_select_init(void) {
    s32 i;
    UNUSED s32 numLevels;
    UNUSED s32 numWorlds;

    level_count(&numLevels, &numWorlds); // Unused
    menu_assetgroup_load(gFileSelectObjectIndices);
    menu_imagegroup_load(gFileSelectImageIndices);
    menu_panels_alloc(6);
    mark_read_all_save_files();
    gMenuFlow.opacityDecayTimer = 1;
    gRaceSetup.saveFileIndex = 0;
    gMenuInput.delay = 0;
    gMenuFlow.stage = 0;
    gMenuFlow.blinkTimer = 0;
    gFileCopy = FALSE;
    gFileErase = FALSE;
    gRaceSetup.fileNew = FALSE;
    transition_begin(&sMenuTransitionFadeOut);
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
}

/**
 * Render a screen element associated with file select onscreen.
 */
void fileselect_render_element(s32 imageID, s32 xOffset, s32 yOffset, s32 red, s32 green, s32 blue, s32 opacity) {
    s32 tempRed = gMenuFrame.guiColourR;
    s32 tempGreen = gMenuFrame.guiColourG;
    s32 tempBlue = gMenuFrame.guiColourB;
    s32 tempOpacity = gMenuFrame.guiOpacity;
    // This first section is the shadow
    gMenuFrame.guiColourR = red;
    gMenuFrame.guiColourG = green;
    gMenuFrame.guiColourB = blue;
    gMenuFrame.guiOpacity = opacity;
    gMenuLoaded.images[imageID].trans.x_position = xOffset - 159;
    gMenuLoaded.images[imageID].trans.y_position = 119 - yOffset;
    menu_element_render(imageID);
    // And this then draws it again, with colour
    gMenuFrame.guiColourR = tempRed;
    gMenuFrame.guiColourG = tempGreen;
    gMenuFrame.guiColourB = tempBlue;
    gMenuFrame.guiOpacity = tempOpacity;
    gMenuLoaded.images[imageID].trans.x_position = xOffset - 161;
    gMenuLoaded.images[imageID].trans.y_position = 121 - yOffset;
    menu_element_render(imageID);
}

/**
 * Render the file select buttons onscreen. This includes the 3D buttons,
 * and then the copy and erase text beneath.
 * If trying to copy or erase a file, render the confirmation menu.
 */
void fileselect_render(UNUSED s32 updateRate) {
    s32 yPos;
    s32 var_s2;
    s32 glow;
    u32 colour;
    s32 i;
    UNUSED s32 pad[3];
    // Also receives the untrimmed menu-text names ("GAME A" etc.) for unstarted
    // slots, which are longer than SavefileInfo.name.
    char trimmedFilename[16];

    if (osTvType == OS_TV_TYPE_PAL) {
        yPos = 12;
    } else {
        yPos = 0;
    }

    menu_camera_centre();
    mtx_ortho(&gMenuFrame.displayList, &gMenuFrame.hudMat);
    for (i = 0; i < NUMBER_OF_SAVE_FILES; i++) {
        if (gSavefileInfo[i].isAdventure2 == gRaceSetup.isInAdventureTwo || gSavefileInfo[i].isStarted == FALSE) {
            colour = COLOUR_RGBA32(176, 224, 192, 255);
        } else {
            colour = COLOUR_RGBA32(106, 144, 115, 255);
        }
        menu_panel_draw_textured(
            NULL, gFileSelectButtons[i].x - SCREEN_WIDTH_HALF, SCREEN_HEIGHT_HALF - gFileSelectButtons[i].y,
            gFileSelectButtons[i].width, gFileSelectButtons[i].height, gFileSelectButtons[i].borderWidth,
            gFileSelectButtons[i].borderHeight, colour, gMenuLoaded.assets[TEXTURE_SURFACE_BUTTON_WOOD]);
    }
    menu_panels_render(&gMenuFrame.displayList);
    if (gMenuFlow.opacityDecayTimer == 0) {
        set_text_font(ASSET_FONTS_BIGFONT);
        set_text_background_colour(0, 0, 0, 0);
        for (i = 0; i < NUMBER_OF_SAVE_FILES; i++) {
            if (gSavefileInfo[i].isStarted) {
                sprite_opaque(FALSE);
                var_s2 = 11;
                if (gSavefileInfo[i].isAdventure2) {
                    var_s2 = 12;
                }
                fileselect_render_element(var_s2, gFileSelectButtons[i].x + gFileSelectElementPos[2],
                                          gFileSelectButtons[i].y + gFileSelectElementPos[3], 0, 0, 0, 128);
                cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
                gMenuLoaded.images->spriteOffset = gSavefileInfo[i].balloonCount / 10;
                fileselect_render_element(0, gFileSelectButtons[i].x + gFileSelectElementPos[6] - 6,
                                          gFileSelectButtons[i].y + gFileSelectElementPos[7], 0, 0, 0, 128);
                gMenuLoaded.images->spriteOffset = gSavefileInfo[i].balloonCount % 10;
                fileselect_render_element(0, gFileSelectButtons[i].x + gFileSelectElementPos[6] + 6,
                                          gFileSelectButtons[i].y + gFileSelectElementPos[7], 0, 0, 0, 128);
                cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
                gMenuFrame.guiColourG = 64;
                gMenuFrame.guiColourB = 64;
                fileselect_render_element(10, gFileSelectButtons[i].x + gFileSelectElementPos[8],
                                          gFileSelectButtons[i].y + gFileSelectElementPos[9], 0, 0, 0, 128);
                gMenuFrame.guiColourG = 255;
                gMenuFrame.guiColourB = 255;
                sprite_opaque(TRUE);
            } else {
                set_text_colour(255, 255, 255, 64, 255);
                draw_text(&gMenuFrame.displayList, gFileSelectButtons[i].x + gFileSelectElementPos[4],
                          gFileSelectButtons[i].y + gFileSelectElementPos[5] + yPos,
                          gMenuLoaded.text[ASSET_MENU_TEXT_NEW], ALIGN_MIDDLE_CENTER);
            }
        }
    }
    glow = gMenuFlow.blinkTimer * 8;
    if (glow > 255) {
        glow = 511 - glow;
    }
    set_text_font(ASSET_FONTS_FUNFONT);
    set_text_background_colour(0, 0, 0, 0);
    set_text_colour(255, 255, 255, 0, 255);
    for (i = 0; i < NUMBER_OF_SAVE_FILES; i++) {
        var_s2 = FALSE;
        if (gFileCopy) {
            if (gFileConfirm == 0 && i == gSaveFileIndex3) {
                var_s2 = TRUE;
            } else if (gFileConfirm > 0 && i == gSaveFileIndex2) {
                var_s2 = TRUE;
            }
        } else if (gFileErase) {
            if (i == gSaveFileIndex3) {
                var_s2 = TRUE;
            }
        } else if (gMenuFlow.stage == 0 && i == gRaceSetup.saveFileIndex) {
            var_s2 = TRUE;
        }
        if (var_s2) {
            colour = glow | ~0xFF;
            menu_panel_draw_bordered(&gMenuFrame.displayList, gFileSelectButtons[i].x, gFileSelectButtons[i].y + yPos,
                                     gFileSelectButtons[i].width, gFileSelectButtons[i].height,
                                     gFileSelectButtons[i].borderWidth, gFileSelectButtons[i].borderHeight, colour,
                                     colour, colour, colour);
        }
        if (!gRaceSetup.fileNew || i != gRaceSetup.saveFileIndex) {
            filename_trim(gSavefileInfo[i].name, trimmedFilename);
            if (!gSavefileInfo[i].isStarted) {
                filename_trim(gFilenames[i], trimmedFilename);
            }
            //!@Bug: Doesn't index the array, so always true.
            if (trimmedFilename) {
                draw_text(&gMenuFrame.displayList, gFileSelectButtons[i].x + gFileSelectElementPos[0],
                          gFileSelectButtons[i].y + gFileSelectElementPos[1] + yPos, trimmedFilename,
                          ALIGN_MIDDLE_CENTER);
            }
        }
    }
    set_text_font(ASSET_FONTS_BIGFONT);
    set_text_colour(0, 0, 0, 255, 128);
#if VERSION >= VERSION_79
    // Did the enum for ASSET_MENU_TEXT_GAMESELECT change?
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 1, 19, gMenuLoaded.text[ASSET_MENU_TEXT_GAMESELECT + 112],
              ALIGN_TOP_CENTER);
#else
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 1, 19, gMenuLoaded.text[ASSET_MENU_TEXT_GAMESELECT],
              ALIGN_TOP_CENTER);
#endif
    set_text_colour(255, 255, 255, 0, 255);
#if VERSION >= VERSION_79
    // Did the enum for ASSET_MENU_TEXT_GAMESELECT change?
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, 16, gMenuLoaded.text[ASSET_MENU_TEXT_GAMESELECT + 112],
              ALIGN_TOP_CENTER);
#else
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, 16, gMenuLoaded.text[ASSET_MENU_TEXT_GAMESELECT],
              ALIGN_TOP_CENTER);
#endif
    set_text_colour(255, 255, 255, 0, 255);
    yPos += 187;
    if (gFileCopy) {
        if (gFileConfirm == FALSE) {
            set_text_font(ASSET_FONTS_FUNFONT);
            draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, yPos, gMenuLoaded.text[ASSET_MENU_TEXT_GAMETOCOPY],
                      ALIGN_MIDDLE_CENTER);
        } else if (gFileConfirm == 1) {
            set_text_font(ASSET_FONTS_FUNFONT);
            draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, yPos, gMenuLoaded.text[ASSET_MENU_TEXT_GAMETOCOPYTO],
                      ALIGN_MIDDLE_CENTER);
        } else {
            draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, yPos, "OK?", ALIGN_MIDDLE_CENTER);
        }
    } else if (gFileErase) {
        if (gFileConfirm == FALSE) {
            set_text_font(ASSET_FONTS_FUNFONT);
            draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, yPos, gMenuLoaded.text[ASSET_MENU_TEXT_GAMETOERASE],
                      ALIGN_MIDDLE_CENTER);
        } else {
            draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, yPos, "OK?", ALIGN_MIDDLE_CENTER);
        }
    } else if (gRaceSetup.fileNew == FALSE) {
        if (gMenuFlow.stage == 1) {
            set_text_colour(255, 255, 255, glow, 255);
        }
        draw_text(&gMenuFrame.displayList, 90, yPos, gMenuLoaded.text[ASSET_MENU_TEXT_COPY], ALIGN_MIDDLE_CENTER);
        if (gMenuFlow.stage == 2) {
            set_text_colour(255, 255, 255, glow, 255);
        } else {
            set_text_colour(255, 255, 255, 0, 255);
        }
        draw_text(&gMenuFrame.displayList, 230, yPos, gMenuLoaded.text[ASSET_MENU_TEXT_ERASE], ALIGN_MIDDLE_CENTER);
    }
}

/**
 * Allows movement between the main options in the file select.
 * If the new file, erase file or copy file options are selected, enable those flags.
 */
s32 fileselect_input_root(UNUSED s32 updateRate) {
    u32 buttonsPressed;
    s32 xAxisDirection;
    s32 yAxisDirection;
    s32 prevOption;
    u32 buttonsPressedPlayer2;

    buttonsPressed = input_pressed(PLAYER_ONE);
    xAxisDirection = gMenuInput.xAxisDirection[PLAYER_ONE];
    yAxisDirection = gMenuInput.yAxisDirection[PLAYER_ONE];
    if (gRaceSetup.numberOfActivePlayers == 2) {
        buttonsPressedPlayer2 = input_pressed(PLAYER_TWO);
        buttonsPressed |= buttonsPressedPlayer2;
        xAxisDirection += gMenuInput.xAxisDirection[PLAYER_TWO];
        yAxisDirection += gMenuInput.yAxisDirection[PLAYER_TWO];
    }
    if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
        switch (gMenuFlow.stage) {
            case 0:
                if (gSavefileInfo[gRaceSetup.saveFileIndex].isStarted) {
                    if (gRaceSetup.isInAdventureTwo != gSavefileInfo[gRaceSetup.saveFileIndex].isAdventure2) {
                        sound_play(SOUND_HORN_DRUMSTICK, NULL);
                        break;
                    }
                }
                sound_play(SOUND_SELECT2, NULL);
                return 1;
            case 1:
                sound_play(SOUND_SELECT2, NULL);
                gSaveFileIndex3 = gRaceSetup.saveFileIndex;
                gFileCopy = TRUE;
                gFileConfirm = FALSE;
                return 0;
            case 2:
                sound_play(SOUND_SELECT2, NULL);
                gSaveFileIndex3 = gRaceSetup.saveFileIndex;
                gFileErase = TRUE;
                gFileConfirm = FALSE;
                return 0;
        }
    } else if (buttonsPressed & CONT_B) {
        return -1;
    }

    prevOption = (gMenuFlow.stage << 8) | gRaceSetup.saveFileIndex;
    if (gMenuFlow.stage == 0) {
        // Left
        if (xAxisDirection < 0 && gRaceSetup.saveFileIndex > 0) {
            gRaceSetup.saveFileIndex--;
        }
        // Right
        if (xAxisDirection > 0 && (gRaceSetup.saveFileIndex < 2)) {
            gRaceSetup.saveFileIndex++;
        }
        // Down
        if (yAxisDirection < 0) {
            if (gRaceSetup.saveFileIndex >= 2) {
                gMenuFlow.stage = 2;
            } else {
                gMenuFlow.stage = 1;
            }
        }
    } else {
        // Left
        if (xAxisDirection < 0 && gMenuFlow.stage == 2) {
            gMenuFlow.stage = 1;
        }
        // Right
        if (xAxisDirection > 0 && gMenuFlow.stage == 1) {
            gMenuFlow.stage = 2;
        }
        // Up
        if (yAxisDirection > 0) {
            if (gMenuFlow.stage == 1 && gRaceSetup.saveFileIndex >= 2) {
                gRaceSetup.saveFileIndex = 0;
            }
            if (gMenuFlow.stage == 2 && gRaceSetup.saveFileIndex <= 0) {
                gRaceSetup.saveFileIndex = 2;
            }
            gMenuFlow.stage = 0;
        }
    }
    if (prevOption != ((gMenuFlow.stage << 8) | gRaceSetup.saveFileIndex)) {
        sound_play(SOUND_MENU_PICK2, NULL);
    }
    return 0;
}

/**
 * Works similar to the root version of the function,
 * but when selecting a file, it gives a confirmation, before copying a file.
 */
void fileselect_input_copy(UNUSED s32 updateRate) {
    s32 prevOption;
    u32 buttonsPressed;
    s32 xAxisDirection;
    s32 i;
    Settings *settings;
    u32 buttonsPressedPlayerTwo;

    settings = get_settings();
    buttonsPressed = input_pressed(PLAYER_ONE);
    xAxisDirection = gMenuInput.xAxisDirection[PLAYER_ONE];
    if (gRaceSetup.numberOfActivePlayers == 2) {
        buttonsPressedPlayerTwo = input_pressed(PLAYER_TWO);
        buttonsPressed |= buttonsPressedPlayerTwo;
        xAxisDirection += gMenuInput.xAxisDirection[PLAYER_TWO];
    }
    if (gFileConfirm == 0) {
        if (buttonsPressed & (B_BUTTON)) {
            sound_play(SOUND_MENU_BACK3, NULL);
            gFileCopy = FALSE;
            return;
        }
        if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
            if (gSavefileInfo[gSaveFileIndex3].isStarted) {
                sound_play(SOUND_SELECT2, NULL);
                mark_read_save_file(gSaveFileIndex3);
                gSaveFileIndex2 = gSaveFileIndex3;
                gFileConfirm = 1;
                return;
            }
            sound_play(SOUND_MENU_BACK3, NULL);
            return;
        }
        prevOption = gSaveFileIndex3;
        if ((xAxisDirection < 0) && (gSaveFileIndex3 > 0)) {
            gSaveFileIndex3--;
        }
        if ((xAxisDirection > 0) && (gSaveFileIndex3 < 2)) {
            gSaveFileIndex3++;
        }
        if (prevOption != gSaveFileIndex3) {
            sound_play(SOUND_MENU_PICK2, NULL);
        }
    } else if (gFileConfirm == 1) {
        if (buttonsPressed & B_BUTTON) {
            sound_play(SOUND_MENU_BACK3, NULL);
            gSaveFileIndex3 = gSaveFileIndex2;
            gFileConfirm = 0;
            return;
        }
        if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
            if (!gSavefileInfo[gSaveFileIndex2].isStarted) {
                sound_play(SOUND_SELECT2, NULL);
                gFileConfirm = 2;
                return;
            }
            sound_play(SOUND_MENU_BACK3, NULL);
            return;
        }
        prevOption = gSaveFileIndex2;
        if (xAxisDirection < 0 && gSaveFileIndex2 > 0) {
            gSaveFileIndex2--;
        }
        if (xAxisDirection > 0 && gSaveFileIndex2 < 2) {
            gSaveFileIndex2++;
        }
        if (prevOption != gSaveFileIndex2) {
            sound_play(SOUND_MENU_PICK2, NULL);
        }
    } else {
        if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
            sound_play(SOUND_SELECT2, NULL);
            force_mark_write_save_file(gSaveFileIndex2);
            gSavefileInfo[gSaveFileIndex2].isAdventure2 = FALSE;
            gSavefileInfo[gSaveFileIndex2].isStarted = TRUE;
            gSavefileInfo[gSaveFileIndex2].balloonCount = *settings->balloonsPtr;
            if (settings->cutsceneFlags & CUTSCENE_ADVENTURE_TWO) {
                gSavefileInfo[gSaveFileIndex2].isAdventure2 = TRUE;
            }
            for (i = 0; gSavefileInfo[gSaveFileIndex3].name[i] != '\0'; i++) {
                gSavefileInfo[gSaveFileIndex2].name[i] = gSavefileInfo[gSaveFileIndex3].name[i];
            }
            gSavefileInfo[gSaveFileIndex2].name[i] = '\0';
            gRaceSetup.saveFileIndex = gSaveFileIndex2;
            gMenuFlow.stage = 0;
            gFileCopy = FALSE;
        } else if (buttonsPressed & B_BUTTON) {
            sound_play(SOUND_MENU_BACK3, NULL);
            gFileConfirm = 1;
        }
    }
}

/**
 * Works similar to the root version of the function,
 * but when selecting a file, it gives a confirmation, before erasing a file.
 */
void fileselect_input_erase(UNUSED s32 updateRate) {
    s32 buttonsPressed;
    s32 controllerXAxisDirection;
    s32 prevOption;

    buttonsPressed = input_pressed(PLAYER_ONE);
    controllerXAxisDirection = gMenuInput.xAxisDirection[0];

    if (gRaceSetup.numberOfActivePlayers == 2) {
        buttonsPressed |= input_pressed(PLAYER_TWO);
        controllerXAxisDirection += gMenuInput.xAxisDirection[1];
    }

    if (gFileConfirm == 0) {
        if (buttonsPressed & B_BUTTON) {
            sound_play(SOUND_MENU_BACK3, NULL);
            gFileErase = FALSE;
            return;
        } else if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
            if (gSavefileInfo[gSaveFileIndex3].isStarted != 0) {
                sound_play(SOUND_SELECT2, NULL);
                gFileConfirm = 1;
                return;
            }
            sound_play(SOUND_MENU_BACK3, NULL);
            return;
        }

        prevOption = gSaveFileIndex3;

        // Left
        if (controllerXAxisDirection < 0) {
            if (gSaveFileIndex3 > 0) {
                gSaveFileIndex3--;
            }
        }

        // Right
        if (controllerXAxisDirection > 0) {
            if (gSaveFileIndex3 < 2) {
                gSaveFileIndex3++;
            }
        }

        if (gSaveFileIndex3 != prevOption) {
            sound_play(SOUND_MENU_PICK2, NULL);
        }
    } else {
        if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
            sound_play(SOUND_SELECT2, NULL);
            mark_save_file_to_erase(gSaveFileIndex3);
            gSavefileInfo[gSaveFileIndex3].isAdventure2 = 0;
            gSavefileInfo[gSaveFileIndex3].isStarted = 0;
            gSavefileInfo[gSaveFileIndex3].balloonCount = 0;
            gSavefileInfo[gSaveFileIndex3].name[0] = 'D';
            gSavefileInfo[gSaveFileIndex3].name[1] = 'K';
            gSavefileInfo[gSaveFileIndex3].name[2] = 'R';
            gSavefileInfo[gSaveFileIndex3].name[3] = '\0';
            gRaceSetup.saveFileIndex = gSaveFileIndex3;
            gMenuFlow.stage = 0;
            gFileErase = FALSE;
        } else if (buttonsPressed & B_BUTTON) {
            sound_play(SOUND_MENU_BACK3, NULL);
            gFileConfirm = 0;
        }
    }
}

/**
 * Sets the music channels to match whichever character was last selected.
 * Initialises the display for the file select options here, and then handles
 * all of the file select behaviour.
 */
s32 menu_file_select_loop(s32 updateRate) {
    s32 i;
    s32 currentMenuDelay;
    u32 buttonsPressed;
    Settings *settings;

#ifdef AVOID_UB
    // i can be undefined in certain cases, and it's used for the save file number.
    i = 0;
#endif

    settings = get_settings();
    charselect_music_channels(updateRate);
    if (gMenuFlow.opacityDecayTimer) {
        gMenuFlow.opacityDecayTimer++;

        if (gMenuFlow.opacityDecayTimer >= 3) {
            for (i = 0; i < NUMBER_OF_SAVE_FILES; i++) {
                gSavefileInfo[i].isAdventure2 = 0;
                if (gRaceSetup.saveFiles[i]->newGame) {
                    gSavefileInfo[i].isStarted = 0;
                    gSavefileInfo[i].balloonCount = 0;
                    gSavefileInfo[i].name[0] = 'D';
                    gSavefileInfo[i].name[1] = 'K';
                    gSavefileInfo[i].name[2] = 'R';
                    gSavefileInfo[i].name[3] = '\0';
                } else {
                    if (gRaceSetup.saveFiles[i]->cutsceneFlags & CUTSCENE_ADVENTURE_TWO) {
                        gSavefileInfo[i].isAdventure2 = TRUE;
                    }
                    gSavefileInfo[i].isStarted = TRUE;
                    gSavefileInfo[i].balloonCount = *gRaceSetup.saveFiles[i]->balloonsPtr;
                    filename_decompress(gRaceSetup.saveFiles[i]->filename, gSavefileInfo[i].name, MAX_INITIALS_LENGTH);
                }
            }
            gMenuFlow.opacityDecayTimer = 0;
        }
    }
    gMenuFlow.blinkTimer = (gMenuFlow.blinkTimer + updateRate) & 0x3f;
    if (gMenuInput.delay != 0) {
        if (gMenuInput.delay > 0) {
            gMenuInput.delay += updateRate;
        } else {
            gMenuInput.delay -= updateRate;
        }
    }
    if (gMenuInput.delay >= -20 && gMenuInput.delay <= 20) {
        fileselect_render(updateRate);
    }
    if (gMenuInput.delay == 0 && gMenuFlow.opacityDecayTimer == 0) {
        if (gFileCopy) {
            fileselect_input_copy(updateRate);
        } else if (gFileErase) {
            fileselect_input_erase(updateRate);
        } else if (gRaceSetup.fileNew) {
            buttonsPressed = input_pressed(PLAYER_ONE);
            if (buttonsPressed & B_BUTTON && gNameEntryLength == 0) {
                menu_unload_bigfont();
                gRaceSetup.fileNew = FALSE;
                gSavefileInfo[i].name[0] = 'D';
                gSavefileInfo[i].name[1] = 'K';
                gSavefileInfo[i].name[2] = 'R';
                gSavefileInfo[i].name[3] = '\0';
            } else if (filename_enter(updateRate)) {
                menu_unload_bigfont();
                gRaceSetup.fileNew = FALSE;
                gSavefileInfo[gRaceSetup.saveFileIndex].isAdventure2 = 0;
                if (gRaceSetup.isInAdventureTwo) {
                    gSavefileInfo[gRaceSetup.saveFileIndex].isAdventure2 = 1;
                }
                gSavefileInfo[gRaceSetup.saveFileIndex].isStarted = 1;
                gSavefileInfo[gRaceSetup.saveFileIndex].balloonCount = 0;
                settings->filename = filename_compress(gSavefileInfo[gRaceSetup.saveFileIndex].name, 3);
                mark_read_save_file(gRaceSetup.saveFileIndex);
                music_fade(-128);
                transition_begin(&sMenuTransitionFadeIn);
                gMenuInput.delay = 1;
            }
        } else {
            currentMenuDelay = fileselect_input_root(updateRate);
            if (currentMenuDelay != 0) {
                if (currentMenuDelay > 0) {
                    if (gSavefileInfo[gRaceSetup.saveFileIndex].isStarted != 0) {
                        sound_play(SOUND_SELECT2, NULL);
                        mark_read_save_file(gRaceSetup.saveFileIndex);
                        music_fade(-128);
                    } else {
                        gRaceSetup.fileNew = TRUE;
                        gIndexOfCurInputCharacter = 0;
                        i = 0;
                        if (osTvType == OS_TV_TYPE_PAL) {
                            i = 12;
                        }
                        filename_init(i + 187,
                                      gFileSelectButtons[gRaceSetup.saveFileIndex].x + gFileSelectElementPos[0],
                                      gFileSelectButtons[gRaceSetup.saveFileIndex].y + gFileSelectElementPos[1] + i, 0,
                                      &gIndexOfCurInputCharacter, gSavefileInfo[gRaceSetup.saveFileIndex].name, 3);
                        currentMenuDelay = 0;
                    }
                }
                if (currentMenuDelay != 0) {
                    transition_begin(&sMenuTransitionFadeIn);
                    gMenuInput.delay = currentMenuDelay;
                }
            }
        }
    }
    if (gMenuInput.delay > 35) {
        if (gRaceSetup.activeMagicCodes & CHEAT_FREE_BALLOON) {
            gRaceSetup.activeMagicCodes &= ~CHEAT_FREE_BALLOON;
            gRaceSetup.unlockedMagicCodes &= ~CHEAT_FREE_BALLOON;
            (*settings->balloonsPtr)++;
        }
        gRaceSetup.isInTwoPlayerAdventure = (gRaceSetup.numberOfActivePlayers == 2);
        if (gRaceSetup.isInTwoPlayerAdventure) {
            reset_lead_player_index();
        }
        gRaceSetup.numberOfActivePlayers = 1;
        gResults.filenameEntryPending = TRUE;
        fileselect_free();
        music_change_on();
        init_racer_headers();
        gRaceSetup.trophyRaceWorldId = 0;
        if (settings->newGame) {
            if (gRaceSetup.isInAdventureTwo) {
                settings->cutsceneFlags |= CUTSCENE_ADVENTURE_TWO;
            }
            cinematic_start((s8 *) get_misc_asset(ASSET_MISC_CINEMATIC_RACE), 0, gRaceSetup.numberOfActivePlayers, 0, 0,
                            NULL);
            menu_init(MENU_NEWGAME_CINEMATIC);
            return MENU_RESULT_CONTINUE;
        }
        if (settings->cutsceneFlags & CUTSCENE_ADVENTURE_TWO) {
            gRaceSetup.isInAdventureTwo = TRUE;
        } else {
            gRaceSetup.isInAdventureTwo = FALSE;
        }
        return gRaceSetup.numberOfActivePlayers;
    }
    if (gMenuInput.delay < -35) {
        fileselect_free();
        menu_init(MENU_GAME_SELECT);
        return MENU_RESULT_CONTINUE;
    } else {
        return MENU_RESULT_CONTINUE;
    }
}

/**
 * Free all assets associated with the file select menu.
 */
void fileselect_free(void) {
    menu_assetgroup_free(gFileSelectObjectIndices);
    menu_button_free();
    unload_font(ASSET_FONTS_BIGFONT);
}

/**
 * Takes in a compressed filename integer, and decompresses it to a string.
 * Each character takes up 5 bits, so up to 6 chars can fit in the integer.
 */
void filename_decompress(u32 compressedFilename, char *output, s32 length) {
    s32 index;
    output[length] = '\0'; // Set null terminator at the end
    for (index = length - 1; index >= 0;) {
        index--;
        output[index + 1] = gFileNameValidChars[compressedFilename & (sizeof(gFileNameValidChars) - 1)];
        compressedFilename /= sizeof(gFileNameValidChars); // compressedFilename >>= 5;
    }
}

/**
 * Takes in a string, and compresses it into an integer.
 * Each character takes up 5 bits, so up to 6 chars can fit in the integer.
 */
s32 filename_compress(char *filename, s32 length) {
    s32 i;
    u32 output;
    u8 processingChars;
    u8 currentChar;
    s32 charIndex;

    output = 0;
    processingChars = 1;

    for (i = 0; i < length; i++) {
        if (filename[i] == '\0') {
            processingChars = 0;
        }

        if (!processingChars) {
            currentChar = ' ';
            charIndex = 0;
        } else {
            currentChar = filename[i];
            charIndex = 0;
        }

        while (currentChar != gFileNameValidChars[charIndex]) {
            charIndex++;
            if (charIndex >= (s32) (sizeof(gFileNameValidChars) - 1)) {
                break;
            }
        }

        output *= sizeof(gFileNameValidChars); // output <<= 5;
        output |= charIndex & (sizeof(gFileNameValidChars) - 1);
    }

    return output;
}

/**
 * Removes any trailing spaces from the file name, then adds a terminator to the end.
 */
void filename_trim(char *input, char *output) {
    s32 some_ptr;
    u32 zxChar;
    char *ptr_to_input;
    char new_var;
    s32 another_ptr;
    char *one_more_ptr;

    some_ptr = 0;
    if (*input != 0) {
        new_var = input[some_ptr];
        ptr_to_input = input;
        zxChar = new_var;
        do {
            another_ptr = some_ptr;
            if ((zxChar & 0xFFFFFFFF) == ' ') {
                zxChar = ptr_to_input[1];
                some_ptr++;
                ptr_to_input++;
                while ((zxChar & 0xFFFFFFFF) == ' ') {
                    some_ptr++;
                    zxChar = *(++ptr_to_input);
                }
                if ((zxChar != 0) && (another_ptr < some_ptr)) {
                    one_more_ptr = another_ptr + input;
                    do {
                        *(output++) = *(one_more_ptr++);
                    } while (++another_ptr < some_ptr);
                    zxChar = *ptr_to_input;
                }
            } else {
                *output = zxChar;
                zxChar = ptr_to_input[1];
                output++;
                some_ptr++;
                ptr_to_input++;
            }
        } while (zxChar != 0);
    }
    *output = '\0';
}

/**
 * Initialise the name entry variables. Reset the entry to zero.
 * Position of the keyboard and title can be placed anywhere.
 */
void filename_init(s32 titleY, s32 x, s32 y, s32 font, s32 *targetX, char *fileName, s32 fileNameLength) {
    gEnterInitalsY = titleY;
    gFilenameX = x;
    gFilenameY = y;
    gFilenameFont = font;
    gNameEntryTargetX = targetX;
    gNameEntryString = fileName;
    gNameEntryAllowedLength = fileNameLength;
    gNameEntryLength = 0;
    gNameEntryOffsetX = *gNameEntryTargetX;
    gNameSelectionDone = FALSE;
    gNameEntryStickX = 0;
    gNameEntryStickHeld = 0;
    load_font(ASSET_FONTS_BIGFONT);
}

/**
 * Draw menu for "Enter your initials" when starting a new game.
 * The text entry is a horizontal list of characters you scroll through.
 * Visual Aid: https://imgur.com/llVwdTy
 */
void filename_render(UNUSED s32 updateRate) {
    s32 xIncrement;
    s32 charIndexIncrement;
    s32 xStart;
    s32 charIndex;
    s32 x;
    s32 i;
    s32 startCharacterIndex;
    s32 y;
    char *trimmedTextPtr;
    char trimmedTextBuffer[4];

    startCharacterIndex = gNameEntryOffsetX;
    xStart = SCREEN_WIDTH_HALF - (s32) ((gNameEntryOffsetX - (f32) startCharacterIndex) * 40);
    set_text_background_colour(0, 0, 0, 0);
    set_text_font(ASSET_FONTS_FUNFONT);

    // Draw "Enter Initials" shadow
    set_text_colour(0, 0, 0, 255, 128);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 2, gEnterInitalsY - 22,
              gMenuLoaded.text[ASSET_MENU_TEXT_ENTERINITIALS], ALIGN_MIDDLE_CENTER);

    // Draw "Enter Initials" text
    set_text_colour(255, 128, 255, 96, 255);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, gEnterInitalsY - 24,
              gMenuLoaded.text[ASSET_MENU_TEXT_ENTERINITIALS], ALIGN_MIDDLE_CENTER);

    y = gEnterInitalsY;

    // Doesn't match with a for loop.
    i = 0;
    while (i < 2) {
        x = xStart;
        charIndex = startCharacterIndex;
        if (i != 0) {
            x = xStart - 40;
            charIndex = startCharacterIndex - 1;
            xIncrement = -40;
            charIndexIncrement = -1;
        } else {
            xIncrement = 40;
            charIndexIncrement = 1;
        }
        while (x > -16 && x < SCREEN_WIDTH + 16) {
            if (charIndex < 0) {
                charIndex = 30;
            }
            if (charIndex > 30) {
                charIndex = 0;
            }
            if (charIndex == *gNameEntryTargetX) {
                set_text_colour(255, 255, 255, 0, 255);
            } else {
                set_text_colour(0, 0, 0, 128, 255);
            }
            if (charIndex < 28) {
                set_text_font(ASSET_FONTS_BIGFONT);
                gCurFilenameCharBeingDrawn = gFileNameValidChars[charIndex];
                draw_text(&gMenuFrame.displayList, x, y, &gCurFilenameCharBeingDrawn, ALIGN_MIDDLE_CENTER);
            } else {
                set_text_font(ASSET_FONTS_FUNFONT);
                if (charIndex == 28) {
                    draw_text(&gMenuFrame.displayList, x, y, "SP", ALIGN_MIDDLE_CENTER);
                } else if (charIndex == 29) {
                    draw_text(&gMenuFrame.displayList, x, y, "DEL", ALIGN_MIDDLE_CENTER);
                } else {
                    draw_text(&gMenuFrame.displayList, x, y, "OK", ALIGN_MIDDLE_CENTER);
                }
            }
            x += xIncrement;
            charIndex += charIndexIncrement;
        }
        i++;
    }
    trimmedTextPtr = trimmedTextBuffer;
    filename_trim(gNameEntryString, trimmedTextPtr);

    if (trimmedTextPtr != NULL) {
        set_text_font(gFilenameFont);
        set_text_colour(0, 0, 0, 255, 128);
        draw_text(&gMenuFrame.displayList, gFilenameX + 1, gFilenameY + 3, trimmedTextBuffer, ALIGN_MIDDLE_CENTER);
        set_text_colour(255, 255, 255, 0, 255);
        draw_text(&gMenuFrame.displayList, gFilenameX, gFilenameY, trimmedTextBuffer, ALIGN_MIDDLE_CENTER);
    }
}

/**
 * Looping input for the alphabet to let the player enter a name string.
 * Pressing OK submits that string.
 */
s32 filename_enter(s32 updateRate) {
    s32 prevOption;
    f32 targetXF;
    u32 buttonsPressed;
    f32 dist1;
    f32 dist2;
    f32 offsetVel;
    s32 *targetX;
    s32 i;
    s32 joytickXAxis;

    buttonsPressed = input_pressed(PLAYER_ONE);
    joytickXAxis = input_clamp_stick_x(PLAYER_ONE);
    if (joytickXAxis > -35 && (joytickXAxis < 35)) {
        joytickXAxis = 0;
    }
    if (joytickXAxis < 0) {
        if (gNameEntryStickX > -1) {
            gNameEntryStickHeld = 0;
            gNameEntryStickX = -1;
        }
        gNameEntryStickHeld += updateRate;
        if (updateRate != gNameEntryStickHeld) {
            if (gNameEntryStickHeld < 28) {
                joytickXAxis = 0;
            } else {
                gNameEntryStickHeld -= 6;
            }
        }
    } else if (joytickXAxis > 0) {
        if (gNameEntryStickX < 1) {
            gNameEntryStickHeld = 0;
            gNameEntryStickX = 1;
        }
        gNameEntryStickHeld += updateRate;
        if (updateRate != gNameEntryStickHeld) {
            if (gNameEntryStickHeld < 28) {
                joytickXAxis = 0;
            } else {
                gNameEntryStickHeld -= 6;
            }
        }
    } else {
        gNameEntryStickHeld = 0;
        gNameEntryStickX = 0;
    }
    targetX = gNameEntryTargetX;
    targetXF = *targetX;
    for (i = 0; i < updateRate; i++) {
        if (gNameEntryOffsetX <= targetXF) {
            dist1 = targetXF - gNameEntryOffsetX;
            dist2 = targetXF - (gNameEntryOffsetX + 31.0f);
        } else {
            dist1 = (targetXF + 31.0f) - gNameEntryOffsetX;
            dist2 = targetXF - gNameEntryOffsetX;
        }
        if ((dist1 * dist1) < (dist2 * dist2)) {
            offsetVel = dist1;
        } else {
            offsetVel = dist2;
        }
        gNameEntryOffsetX += offsetVel * 0.1f;
        if (gNameEntryOffsetX >= 31.0f) {
            do {
                gNameEntryOffsetX -= 31.0f;
            } while (gNameEntryOffsetX >= 31.0f);
        }
        if (gNameEntryOffsetX < 0.0f) {
            do {
                gNameEntryOffsetX += 31.0f;
            } while (gNameEntryOffsetX < 0.0f);
        }
    }
    if (gNameEntryLength < gNameEntryAllowedLength) {
        if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
            if (*targetX < 29) {
                gNameEntryString[gNameEntryLength] = gFileNameValidChars[*targetX];
                gNameEntryLength++;
                gNameEntryString[gNameEntryLength] = '\0';
                sound_play(SOUND_SELECT2, NULL);
                if (gNameEntryLength >= gNameEntryAllowedLength) {
                    *gNameEntryTargetX = 30;
                }
            } else if (*targetX == 29) {
                if (gNameEntryLength > 0) {
                    gNameEntryLength--;
                    gNameEntryString[gNameEntryLength] = '\0';
                }
                sound_play(SOUND_MENU_BACK3, NULL);
            } else {
                if (gNameEntryLength != 0 || gNameEntryString[0] == '\0') {
                    for (i = gNameEntryLength; i < gNameEntryAllowedLength; i++) {
                        gNameEntryString[i] = 32;
                    }
                }
                gNameEntryString[gNameEntryAllowedLength] = '\0';
                sound_play(SOUND_SELECT2, NULL);
                gNameSelectionDone = TRUE;
            }
        } else if (buttonsPressed & B_BUTTON) {
            if (gNameEntryLength > 0) {
                gNameEntryLength--;
            }
            gNameEntryString[gNameEntryLength] = '\0';
            sound_play(SOUND_MENU_BACK3, NULL);
        } else {
            prevOption = *targetX;
            if (joytickXAxis < 0) {
                prevOption--;
            }
            if (joytickXAxis > 0) {
                prevOption++;
            }
            if (prevOption < 0) {
                prevOption = 30;
            }
            if (prevOption > 30) {
                prevOption = 0;
            }
            if (prevOption != *targetX) {
                sound_play(SOUND_MENU_PICK2, NULL);
                *gNameEntryTargetX = prevOption;
            }
        }
    } else if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
        sound_play(SOUND_SELECT2, NULL);
        gNameSelectionDone = TRUE;
    } else if (buttonsPressed & B_BUTTON) {
        gNameEntryLength--;
        gNameEntryString[gNameEntryLength] = '\0';
        sound_play(SOUND_MENU_BACK3, NULL);
    }
    filename_render(updateRate);
    return gNameSelectionDone;
}

/**
 * Returns the value from gRaceSetup.saveFileIndex
 */
s32 get_save_file_index(void) {
    return gRaceSetup.saveFileIndex;
}

/**
 * Return the start of the save file data.
 */
Settings **get_all_save_files_ptr(void) {
    return (Settings **) gRaceSetup.saveFiles;
}
