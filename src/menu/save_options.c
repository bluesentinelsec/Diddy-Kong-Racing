#include "menu_internal.h"

// This is used for RGBA colors for the save options Controller Pak BG.
static u32 gContPakSaveBgColours[MAXCONTROLLERS] = {
    COLOUR_RGBA32(64, 64, 255, 255),  // Blue for controller 1
    COLOUR_RGBA32(255, 64, 64, 255),  // Red for controller 2
    COLOUR_RGBA32(255, 208, 64, 255), // Yellow for controller 3
    COLOUR_RGBA32(64, 255, 64, 255)   // Green for controller 4
};

static SIDeviceStatus gDeviceStatus = CONTROLLER_PAK_GOOD;
static s32 gControllerIndex = 0;

// Associated with SIDeviceStatus enum values.
static char **gContPakStrings[11] = { NULL,
                               gContPakNotPresentStrings,
                               gContPakCorruptDataRepairStrings,
                               gContPakDamagedStrings,
                               gContPakFullStrings,
                               gContPakDiffContStrings,
                               gContPakNoRoomForGhostsStrings,
                               gContPakRumbleDetectedStrings,
                               gContPakSwitchToRumbleStrings,
                               gContPakCorruptDataStrings,
                               gContPakNeed2ndAdvStrings };

/*** Icons in the save options menu. ***/
static DrawTexture gDrawTexN64Icon[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
static DrawTexture gDrawTexTTIcon[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
static DrawTexture gDrawTexGhostIcon[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
static DrawTexture gDrawTexFileIcon[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
static DrawTexture gDrawTexContPakIcon[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
static DrawTexture gDrawTexTrashIcon[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static s16 gSaveMenuObjectIndices[] = { 0x0024, 0x0025, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x003D,
                                 0x003F, 0x003E, 0x003C, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048,
                                 0x0049, 0x004A, 0x004B, 0x0040, 0x0041, 0x0002, -1 };

static s16 gSaveMenuImageIndices[] = { 0x000B, 0x000C, 0x0002, -1 };

// Used in savemenu_render_element(). Used for controller pak adventure saves.
// So the first save in the pak would be (ADV.A), second save (ADV.B), etc.
static char *gConPakAdvSavePrefix = " (ADV.";

/**
 * Allocates space for the pak filesystem to load its results into, then resets all
 * the other variables related to the save options.
 */
void menu_save_options_init(void) {
    gSaveMenuRumbleNagSet = TRUE;
    gSaveMenuRumbleNag = FALSE;
    gSaveMenuSourceState = 1;
    gSaveMenuDestState = 1;
    gOptionsMenuItemIndex = 0;
    gOptionBlinkTimer = 0;
    gMenuDelay = 0;
    gMenuStage = SAVEMENU_WAIT;
    gOpacityDecayTimer = 0;
    sSaveMenuFilenames = (char *) mempool_alloc_safe(0x800, COLOUR_TAG_WHITE);
    gSaveMenuFilesSource = (SaveFileData *) mempool_alloc_safe(0xA00, COLOUR_TAG_WHITE);
    gSaveMenuFilesDest = &gSaveMenuFilesSource[80];
    gSaveMenuOptionCountUpper = 0;
    gSaveMenuOptionSource = 0;
    gSaveMenuScrollSource = 0.0f;
    gSaveMenuOptionCountLower = 0;
    gSaveMenuOptionDest = 0;
    gSaveMenuScrollDest = 0.0f;
    menu_assetgroup_load(gSaveMenuObjectIndices);
    menu_imagegroup_load(gSaveMenuImageIndices);
    menu_panels_alloc(10);
    load_font(ASSET_FONTS_BIGFONT);
    gDrawTexN64Icon[0].texture = gMenuAssets[TEXTURE_ICON_SAVE_N64];
    gDrawTexTTIcon[0].texture = gMenuAssets[TEXTURE_ICON_SAVE_TT];
    gDrawTexGhostIcon[0].texture = gMenuAssets[TEXTURE_ICON_SAVE_GHOSTS];
    gDrawTexFileIcon[0].texture = gMenuAssets[TEXTURE_ICON_SAVE_FILECABINET];
    gDrawTexContPakIcon[0].texture = gMenuAssets[TEXTURE_ICON_SAVE_CPAK];
    gDrawTexTrashIcon[0].texture = gMenuAssets[TEXTURE_ICON_SAVE_BIN];
    menu_init_arrow_textures();
    mark_read_all_save_files();
    transition_begin(&sMenuTransitionFadeOut);
#if VERSION >= VERSION_79
    rumble_enable(FALSE);
#endif
}

/**
 * Render a save menu entry.
 * This has the file name, and then a background and icon based on kind of file it is.
 * DKR save files will display the balloon count and adventure type.
 */
void savemenu_render_element(SaveFileData *file, s32 x, s32 y) {
    s32 i;
    s32 firstDigit;
    s32 secondDigit;
    s32 spriteID;
    s32 colour;
    DrawTexture *drawTexture;
    char *text2;
    char *text;
    char buffer[16];
    TextureHeader *texture;
    s32 offsetY;

    spriteID = 11;
    switch (file->saveFileType) {
        case SAVE_FILE_TYPE_CART_SAVE:
            drawTexture = gDrawTexN64Icon;
            texture = gMenuAssets[TEXTURE_SURFACE_BUTTON_WOOD];
            colour = COLOUR_RGBA32(176, 224, 192, 255);
            if (!gSavefileData[file->controllerIndex]->newGame) {
                filename_decompress(gSavefileData[file->controllerIndex]->filename, buffer, MAX_INITIALS_LENGTH);
                filename_trim(buffer, buffer);
                text2 = buffer;
                firstDigit = *gSavefileData[file->controllerIndex]->balloonsPtr / 10;
                secondDigit = *gSavefileData[file->controllerIndex]->balloonsPtr - (firstDigit * 10);
                // Is Adventure Two?
                if (gSavefileData[file->controllerIndex]->cutsceneFlags & 4) {
                    spriteID = 12;
                }
            } else {
                text2 = gFilenames[file->controllerIndex];
            }
            text = gMenuText[ASSET_MENU_TEXT_GAMEPAK];
            break;
        case SAVE_FILE_TYPE_CART_TIMES:
            drawTexture = gDrawTexTTIcon;
            texture = gMenuAssets[TEXTURE_SURFACE_BUTTON_WOOD];
            colour = COLOUR_RGBA32(176, 224, 192, 255);
            text2 = gMenuText[ASSET_MENU_TEXT_TIMES];
            text = gMenuText[ASSET_MENU_TEXT_GAMEPAK];
            break;
        case SAVE_FILE_TYPE_CPAK_SAVE:
            drawTexture = gDrawTexN64Icon;
            texture = gMenuAssets[TEXTURE_UNK_44];
            colour = gContPakSaveBgColours[file->controllerIndex];
            text2 = buffer;
            filename_decompress(file->compressedFilename, buffer, MAX_INITIALS_LENGTH);
            // char *gConPakAdvSavePrefix = " (ADV.";
            for (i = 0; gConPakAdvSavePrefix[i] != '\0'; i++) {
                buffer[i + 3] = gConPakAdvSavePrefix[i];
            }
            buffer[i + 3] = file->saveFileExt[0];
            buffer[i + 4] = ')';
            buffer[i + 5] = '\0';
            text = gMenuText[ASSET_MENU_TEXT_CONTPAK1 + file->controllerIndex];
            firstDigit = file->balloonCount / 10;
            secondDigit = file->balloonCount % 10;
            if (file->adventureTwo) {
                spriteID = 12;
            }
            break;
        case SAVE_FILE_TYPE_CPAK_TIMES:
            drawTexture = gDrawTexTTIcon;
            texture = gMenuAssets[TEXTURE_UNK_44];
            colour = gContPakSaveBgColours[file->controllerIndex];
            text2 = file->saveFileExt;
            text = gMenuText[ASSET_MENU_TEXT_CONTPAK1 + file->controllerIndex];
            break;
        case SAVE_FILE_TYPE_CPAK_GHOST:
            drawTexture = gDrawTexGhostIcon;
            texture = gMenuAssets[TEXTURE_UNK_44];
            colour = gContPakSaveBgColours[file->controllerIndex];
            text2 = gMenuText[ASSET_MENU_TEXT_GHOSTS];
            text = gMenuText[ASSET_MENU_TEXT_CONTPAK1 + file->controllerIndex];
            break;
        case SAVE_FILE_TYPE_CPAK_OTHER:
            drawTexture = gDrawTexFileIcon;
            texture = gMenuAssets[TEXTURE_UNK_44];
            colour = gContPakSaveBgColours[file->controllerIndex];
            text2 = file->saveFileExt;
            text = gMenuText[ASSET_MENU_TEXT_CONTPAK1 + file->controllerIndex];
            break;
        case SAVE_FILE_TYPE_CPAK_EMPTY:
            drawTexture = gDrawTexContPakIcon;
            texture = gMenuAssets[TEXTURE_UNK_44];
            colour = gContPakSaveBgColours[file->controllerIndex];
            text2 = gMenuText[ASSET_MENU_TEXT_EMPTYSLOT];
            text = gMenuText[ASSET_MENU_TEXT_CONTPAK1 + file->controllerIndex];
            break;
        case SAVE_FILE_TYPE_UNK9:
            drawTexture = gDrawTexGhostIcon;
            texture = gMenuAssets[TEXTURE_UNK_45];
            colour = -1;
            text2 = gMenuText[ASSET_MENU_TEXT_VIEWGHOSTS];
            text = NULL;
            break;
        case SAVE_FILE_TYPE_CART_BONUSES:
            drawTexture = gDrawTexN64Icon;
            text2 = gMenuText[ASSET_MENU_TEXT_GAMEPAKBONUSES];
            texture = gMenuAssets[TEXTURE_SURFACE_BUTTON_WOOD];
            colour = COLOUR_RGBA32(176, 224, 192, 255);
            text = gMenuText[ASSET_MENU_TEXT_GAMEPAK];
            break;
        default:
            drawTexture = gDrawTexTrashIcon;
            texture = gMenuAssets[TEXTURE_UNK_45];
            colour = COLOUR_RGBA32(128, 128, 128, 255);
            text2 = gMenuText[ASSET_MENU_TEXT_ERASE];
            text = NULL;
            break;
    }
#if   VERSION == VERSION_80
#define SAVE_MENU_TEXT_FONT ASSET_FONTS_SMALLFONT
#define SAVE_MENU_SPRITE_OFFSET 0
#define SAVE_MENU_TEXT_WIDTH 160
#define SAVE_MENU_TEXT_OFFSET_1 0
#define SAVE_MENU_TEXT_OFFSET_2 1
#else
#define SAVE_MENU_TEXT_FONT ASSET_FONTS_SMALLFONT
#define SAVE_MENU_SPRITE_OFFSET 0
#define SAVE_MENU_TEXT_WIDTH 160
#define SAVE_MENU_TEXT_OFFSET_1 0
#define SAVE_MENU_TEXT_OFFSET_2 0
#endif
    menu_panel_draw_textured(&sMenuCurrDisplayList, x - 160, 120 - y, SAVE_MENU_TEXT_WIDTH, 64, 4, 4, colour, texture);
    if (osTvType == OS_TV_TYPE_PAL) {
        y += 12;
    }
    if (file->saveFileType == SAVE_FILE_TYPE_CPAK_SAVE ||
        (file->saveFileType == SAVE_FILE_TYPE_CART_SAVE && gSavefileData[file->controllerIndex]->newGame == FALSE)) {
        if (osTvType == OS_TV_TYPE_PAL) {
            i = 134;
        } else {
            i = 120;
        }
        cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
        offsetY = (i - y);
        gMenuImages[2].trans.y_position = offsetY - 49;
        gMenuImages[spriteID].trans.y_position = offsetY - 24;
        sprite_opaque(0);
        gMenuImages[2].trans.x_position = x - (133 - SAVE_MENU_SPRITE_OFFSET);
        gMenuImages[2].spriteOffset = firstDigit;
        menu_element_render(2);
        gMenuImages[2].trans.x_position = x - (125 - SAVE_MENU_SPRITE_OFFSET);
        gMenuImages[2].spriteOffset = secondDigit;
        menu_element_render(2);
        sprite_opaque(1);
        gMenuImages[spriteID].trans.x_position = x - (128 - SAVE_MENU_SPRITE_OFFSET);
        menu_element_render(spriteID);
        cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
    }
    if (drawTexture != NULL) {
        texrect_draw(&sMenuCurrDisplayList, drawTexture, x + (60 + SAVE_MENU_TEXT_OFFSET_1), y + 6, 255, 255, 255, 255);
    }
    if (text != NULL) {
        set_text_font(ASSET_FONTS_FUNFONT);
        set_text_colour(0, 0, 0, 255, 128);
        draw_text(&sMenuCurrDisplayList, x + (81 + SAVE_MENU_TEXT_OFFSET_1), y + 1, text, ALIGN_MIDDLE_CENTER);
        set_text_colour(255, 64, 255, 64, 255);
        draw_text(&sMenuCurrDisplayList, x + (79 + SAVE_MENU_TEXT_OFFSET_1), y - 1, text, ALIGN_MIDDLE_CENTER);
    }
    if (text2 != NULL) {
        set_text_font(SAVE_MENU_TEXT_FONT);
        set_text_colour(0, 0, 0, 255, 160);
        draw_text(&sMenuCurrDisplayList, x + (80 + SAVE_MENU_TEXT_OFFSET_2), y + 48, text2, ALIGN_TOP_CENTER);
        set_text_colour(255, 255, 255, 255, 255);
        draw_text(&sMenuCurrDisplayList, x + (79 + SAVE_MENU_TEXT_OFFSET_2), y + 47, text2, ALIGN_TOP_CENTER);
    }
}

/**
 * Render all of the save option elements onscreen.
 * If it's still loading, render the wait text.
 * If an option has been selected, then render the potential destination elements.
 * If that's been selected too, render confirmation dialogue.
 */
void savemenu_render(UNUSED s32 updateRate) {
    s32 videoWidth;
    s32 temp;
    s32 offsetX;
    s32 x;
    s32 scroll;
    s32 drawUpperElements;
    s32 drawLowerElements;
    s32 drawPleaseWait;
    s32 drawOk;
    s32 drawDialogueBox;

    videoWidth = GET_VIDEO_WIDTH(fb_size());
    drawUpperElements = FALSE;
    drawLowerElements = FALSE;
    drawPleaseWait = FALSE;
    drawOk = FALSE;
    drawDialogueBox = FALSE;
    switch (gMenuStage & 7) {
        case SAVEMENU_ENTER:
            break;
        case SAVEMENU_WAIT:
        case SAVEMENU_INIT_SOURCE:
            drawPleaseWait = TRUE;
            break;
        case SAVEMENU_CHOOSE_SOURCE:
        case SAVEMENU_INIT_DEST:
            drawUpperElements = TRUE;
            break;
        case SAVEMENU_CHOOSE_DEST:
            drawUpperElements = TRUE;
            drawLowerElements = TRUE;
            break;
        case SAVEMENU_CONFIRM:
            drawUpperElements = TRUE;
            drawLowerElements = TRUE;
            drawOk = TRUE;
            break;
        case SAVEMENU_WRITE:
            drawUpperElements = TRUE;
            drawLowerElements = TRUE;
            drawPleaseWait = TRUE;
            break;
    }
    if (gMenuStage & 8) {
        drawDialogueBox = TRUE;
    }
    mtx_ortho(&sMenuCurrDisplayList, &sMenuCurrHudMat);
    set_text_background_colour(0, 0, 0, 0);
    set_text_font(ASSET_FONTS_BIGFONT);
    set_text_colour(0, 0, 0, 255, 128);
    draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF + 1, 35, gMenuText[ASSET_MENU_TEXT_SAVEOPTIONS],
              ALIGN_MIDDLE_CENTER);
    set_text_colour(255, 255, 255, 0, 255);
    draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF, 32, gMenuText[ASSET_MENU_TEXT_SAVEOPTIONS],
              ALIGN_MIDDLE_CENTER);

    if (drawLowerElements) {
        temp = (osTvType == OS_TV_TYPE_PAL) ? SCREEN_HEIGHT_HALF_PAL : SCREEN_HEIGHT_HALF;
        temp += ((s32) (gOptionBlinkTimer & 0x1F) >> 1);

        for (scroll = 0; scroll < 2; scroll += 1, temp += 16) {
            texrect_draw(&sMenuCurrDisplayList, gMenuSelectionArrowDown, SCREEN_WIDTH_HALF, temp, 255, 255, 255, 255);
        }
    }

#define SAVEMENU_ELEMENT_SIZE 164
#define SAVEMENU_OFFSET_X 80
#define SAVEMENU_OFFSET_SCROLL 164.0f

    if (drawUpperElements) {
        scroll = (s32) gSaveMenuScrollSource;
        temp = scroll;
        offsetX = SAVEMENU_OFFSET_X - (s32) ((gSaveMenuScrollSource - scroll) * SAVEMENU_OFFSET_SCROLL);
        x = offsetX;
        while (x < videoWidth && temp < gSaveMenuOptionCountUpper) {
            savemenu_render_element(&gSaveMenuFilesSource[temp], x, 64);
            x += SAVEMENU_ELEMENT_SIZE;
            temp++;
        }
        temp = scroll;
        x = offsetX;
        while ((x > 0) && (temp > 0)) {
            temp--;
            x -= SAVEMENU_ELEMENT_SIZE;
            savemenu_render_element(&gSaveMenuFilesSource[temp], x, 64);
        }
    }

    if (drawLowerElements) {
        scroll = (s32) gSaveMenuScrollDest;
        temp = scroll;
        offsetX = SAVEMENU_OFFSET_X - (s32) ((gSaveMenuScrollDest - (f32) scroll) * SAVEMENU_OFFSET_SCROLL);
        x = offsetX;
        while (x < videoWidth && temp < gSaveMenuOptionCountLower) {
            savemenu_render_element(&gSaveMenuFilesDest[temp], x, 144);
            x += SAVEMENU_ELEMENT_SIZE;
            temp++;
        }
        temp = scroll;
        x = offsetX;
        while (x > 0 && temp > 0) {
            temp--;
            x -= SAVEMENU_ELEMENT_SIZE;
            savemenu_render_element(&gSaveMenuFilesDest[temp], x, 144);
        }
    }

    set_text_font(ASSET_FONTS_BIGFONT);
    set_text_colour(255, 255, 255, 0, 255);
    if (drawOk) {
        draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF, 128, "OK?", ALIGN_MIDDLE_CENTER);
    }
    if (drawPleaseWait) {
        draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF, 128, gMenuText[ASSET_MENU_TEXT_PLEASEWAIT],
                  ALIGN_MIDDLE_CENTER);
    }
    if (drawDialogueBox) {
        render_dialogue_box(&sMenuCurrDisplayList, NULL, NULL, 7);
    }
    menu_geometry_end();
}

/**
 * Searches the 3 onboard cartridge save and writes any blank ones to potential destinations
 */
void savemenu_blank_save_destination(SaveFileData *file, s32 *fileIndex) {
    s32 i;
    for (i = 0; i < NUMBER_OF_SAVE_FILES; i++) {
        if (gSavefileData[i]->newGame) {
            file[*fileIndex].saveFileType = SAVE_FILE_TYPE_CART_SAVE;
            file[*fileIndex].unk1 = 0;
            file[*fileIndex].balloonCount = 0;
            file[*fileIndex].controllerIndex = i;
            file[*fileIndex].fileSize = get_game_data_file_size();
            (*fileIndex)++;
        }
    }
}

/**
 * Loads every save file, course record and game extras first.
 * Then searches the controller pak for any files it can read.
 * Returns the controller pak status.
 */
SIDeviceStatus savemenu_load_sources(void) {
    s32 numAttempts;
    s32 i;
    s32 fileIndex;
    SIDeviceStatus result;
    SIDeviceStatus temp;
    char *fileNames[16];
    char *fileExts[16];
    u8 fileTypes[16];
    Settings *settings;
    u32 fileSizes[16];
    char *temp_D_80126A64;

#ifdef AVOID_UB
    // If gSaveMenuSourceState == 0, then result will be undefined.
    result = CONTROLLER_PAK_GOOD;
#endif

    settings = gSavefileData[3];
    gSaveMenuOptionCountUpper = 0;
    gSaveMenuOptionSource = 0;
    gSaveMenuScrollSource = 0.0f;
    for (i = 0; i < NUMBER_OF_SAVE_FILES; i++) {
        if (!gSavefileData[i]->newGame) {
            gSaveMenuFilesSource[gSaveMenuOptionCountUpper].saveFileType = SAVE_FILE_TYPE_CART_SAVE;
            gSaveMenuFilesSource[gSaveMenuOptionCountUpper].unk1 = 1;
            gSaveMenuFilesSource[gSaveMenuOptionCountUpper].balloonCount = gSavefileData[i]->balloonsPtr[0];
            gSaveMenuFilesSource[gSaveMenuOptionCountUpper].controllerIndex = i;
            gSaveMenuFilesSource[gSaveMenuOptionCountUpper].fileSize = get_game_data_file_size();
            gSaveMenuOptionCountUpper++;
        }
    }

    gSaveMenuFilesSource[gSaveMenuOptionCountUpper].saveFileType = SAVE_FILE_TYPE_CART_TIMES;
    gSaveMenuFilesSource[gSaveMenuOptionCountUpper].unk1 = 1;
    gSaveMenuFilesSource[gSaveMenuOptionCountUpper++].fileSize = get_time_data_file_size();
    gSaveMenuFilesSource[gSaveMenuOptionCountUpper++].saveFileType = SAVE_FILE_TYPE_CART_BONUSES;
    temp_D_80126A64 = sSaveMenuFilenames;
    do {
        numAttempts = 1;
        if (gSaveMenuSourceState != 0) {
            gSaveMenuRumbleConnected = FALSE;

            do {
                result = get_controller_pak_file_list(0, MAX_CPAK_FILES, fileNames, fileExts, fileSizes, fileTypes);
                temp = result & 0xFF; // strip the controller index from the result to just get the SI Device Status
                numAttempts++;
            } while (temp == CONTROLLER_PAK_CHANGED && numAttempts < 3);

            if (result == CONTROLLER_PAK_GOOD) {
                for (fileIndex = 0; fileIndex < MAX_CPAK_FILES; fileIndex++) {
                    if ((fileTypes[fileIndex] >= SAVE_FILE_TYPE_CPAK_SAVE) &&
                        (fileTypes[fileIndex] <= SAVE_FILE_TYPE_CPAK_OTHER)) {
                        sControllerPakNotesFree[0]--;
                        gSaveMenuFilesSource[gSaveMenuOptionCountUpper].saveFileType = fileTypes[fileIndex];
                        gSaveMenuFilesSource[gSaveMenuOptionCountUpper].controllerIndex = 0;
                        gSaveMenuFilesSource[gSaveMenuOptionCountUpper].saveFileNumber = fileIndex;
                        gSaveMenuFilesSource[gSaveMenuOptionCountUpper].fileSize = fileSizes[fileIndex];
                        if (fileTypes[fileIndex] == SAVE_FILE_TYPE_CPAK_SAVE) {
                            if (read_game_data_from_controller_pak(0, fileExts[fileIndex], settings) ==
                                CONTROLLER_PAK_GOOD) {
                                gSaveMenuFilesSource[gSaveMenuOptionCountUpper].saveFileExt = temp_D_80126A64;
                                temp_D_80126A64[0] = *fileExts[fileIndex];
                                temp_D_80126A64[1] = NULL;
                                temp_D_80126A64 += 2;
                                gSaveMenuFilesSource[gSaveMenuOptionCountUpper].balloonCount = *settings->balloonsPtr;
                                gSaveMenuFilesSource[gSaveMenuOptionCountUpper].adventureTwo =
                                    (settings->cutsceneFlags & 4) != 0; // Is in Adventure Two?
                                gSaveMenuFilesSource[gSaveMenuOptionCountUpper].compressedFilename = settings->filename;
                            } else {
                                gSaveMenuOptionCountUpper--;
                            }
                        } else {
                            temp = 0;
                            if ((fileTypes[fileIndex] == SAVE_FILE_TYPE_CPAK_OTHER) ||
                                (fileTypes[fileIndex] == SAVE_FILE_TYPE_CPAK_TIMES)) {
                                if (fileTypes[fileIndex] == SAVE_FILE_TYPE_CPAK_TIMES) {
                                    temp = CONTROLLER_PAK_BAD_DATA;
                                }
                                gSaveMenuFilesSource[gSaveMenuOptionCountUpper].saveFileExt = temp_D_80126A64;
                                while (fileNames[fileIndex][temp] != '\0') {
                                    *temp_D_80126A64 = fileNames[fileIndex][temp];
                                    temp++;
                                    temp_D_80126A64++;
                                }

                                if ((fileExts[fileIndex] != NULL) && (fileExts[fileIndex][0] != '\0')) {
                                    *temp_D_80126A64 = '.';
                                    temp_D_80126A64++;
                                    temp = 0;
                                    while (fileExts[fileIndex][temp] != '\0') {
                                        *temp_D_80126A64 = fileExts[fileIndex][temp];
                                        temp_D_80126A64++;
                                        temp++;
                                    }
                                }
                                *temp_D_80126A64 = NULL;
                                temp_D_80126A64++;
                            }
                        }
                        gSaveMenuOptionCountUpper++;
                    }
                }
                cpak_free_files();
            } else if (temp == CONTROLLER_PAK_RUMBLE_PAK_FOUND) {
                gSaveMenuRumbleConnected = TRUE;
                if (gSaveMenuSourceState < 0) {
                    result = CONTROLLER_PAK_GOOD;
                }
                if (gSaveMenuRumbleNagSet) {
                    gSaveMenuRumbleNag = TRUE;
                    gSaveMenuRumbleNagSet = FALSE;
                }
            } else if (temp == CONTROLLER_PAK_NOT_FOUND) {
                result = CONTROLLER_PAK_GOOD;
            } else if (gSaveMenuSourceState < 0 && temp == CONTROLLER_PAK_CHANGED) {
                numAttempts = 0;
            }
        }
    } while (numAttempts == 0);
    return result;
}

/**
 * Adds every potential destination spot to the lower menu.
 * This includes elsewhere on the controller pak, or deleting it.
 * For save files, this also includes on the cartridge itself.
 * Returns the controller pak status.
 */
SIDeviceStatus savemenu_load_destinations(void) {
    SIDeviceStatus ret = CONTROLLER_PAK_GOOD;

    gSaveMenuOptionCountLower = 0;

    switch (gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileType) {
        case SAVE_FILE_TYPE_CART_SAVE:
            mark_read_save_file(gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex);
            savemenu_blank_save_destination(gSaveMenuFilesDest, &gSaveMenuOptionCountLower);
            ret = savemenu_check_space(0, &gSaveMenuSourceState, gSaveMenuFilesDest, &gSaveMenuOptionCountLower,
                                       get_game_data_file_size(), -1);
            break;
        case SAVE_FILE_TYPE_CART_TIMES:
            ret = savemenu_check_space(0, &gSaveMenuSourceState, gSaveMenuFilesDest, &gSaveMenuOptionCountLower,
                                       get_time_data_file_size(), -1);
            break;
        case SAVE_FILE_TYPE_CPAK_SAVE:
            savemenu_blank_save_destination(gSaveMenuFilesDest, &gSaveMenuOptionCountLower);
            ret = savemenu_check_space(1, &gSaveMenuDestState, gSaveMenuFilesDest, &gSaveMenuOptionCountLower,
                                       get_game_data_file_size(),
                                       gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex);
            break;
        case SAVE_FILE_TYPE_CPAK_TIMES:
            gSaveMenuFilesDest[gSaveMenuOptionCountLower++].saveFileType = SAVE_FILE_TYPE_CART_TIMES;
            ret = savemenu_check_space(1, &gSaveMenuDestState, gSaveMenuFilesDest, &gSaveMenuOptionCountLower,
                                       get_time_data_file_size(),
                                       gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex);
            break;
        case SAVE_FILE_TYPE_CPAK_GHOST:
            gSaveMenuFilesDest[gSaveMenuOptionCountLower++].saveFileType = SAVE_FILE_TYPE_UNK9;
            ret = savemenu_check_space(1, &gSaveMenuDestState, gSaveMenuFilesDest, &gSaveMenuOptionCountLower,
                                       get_ghost_data_file_size(),
                                       gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex);
            break;
    }

    gSaveMenuFilesDest[gSaveMenuOptionCountLower++].saveFileType = SAVE_FILE_TYPE_ERASE;

    return ret;
}

/**
 * Applies the position offets for the elements based on the scrolling position.
 * Uses lerping.
 */
void savemenu_move(s32 updateRate) {
    f32 optUpper, optLower;
    f32 lerpUpper, lerpLower;
    optUpper = gSaveMenuOptionSource;
    optLower = gSaveMenuOptionDest;
    while (updateRate > 0) {
        if (gSaveMenuOptionCountUpper > 0) {
            lerpUpper = optUpper - gSaveMenuScrollSource;
            gSaveMenuScrollSource += 0.1f * lerpUpper; //!@Delta
        }
        if (gMenuStage > SAVEMENU_ENTER && gSaveMenuOptionCountLower > 0) {
            lerpLower = optLower - gSaveMenuScrollDest;
            gSaveMenuScrollDest += 0.1f * lerpLower; //!@Delta
        }
        updateRate--;
    }
}

/**
 * Write the selected source file to the destination.
 * Check the types of the source and destination files to know what kind to write.
 * If the file type is marked as "erase", then erase the data instead.
 * Return the controller pak status.
 */
SIDeviceStatus savemenu_write(void) {
    s32 i;
    SIDeviceStatus ret;
    UNUSED s32 pad[2];
    char fileExt[PFS_FILE_EXT_LEN];
    Settings *settings;

#ifdef AVOID_UB
    // ret can be unset if certain conditions are met.
    ret = CONTROLLER_PAK_GOOD;
#endif

    settings = get_settings();
    switch (gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileType) {
        case SAVE_FILE_TYPE_CART_SAVE:
            switch (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType) {
                case SAVE_FILE_TYPE_CART_SAVE:
                    force_mark_write_save_file(gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex);
                    gSavefileData[gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex]->cutsceneFlags =
                        settings->cutsceneFlags;
                    gSavefileData[gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex]->newGame = FALSE;
                    *gSavefileData[gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex]->balloonsPtr =
                        *settings->balloonsPtr;
                    gSavefileData[gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex]->filename =
                        settings->filename;
                    break;
                case SAVE_FILE_TYPE_CPAK_EMPTY:
                    ret = write_game_data_to_controller_pak(gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex,
                                                            settings);
                    break;
                case SAVE_FILE_TYPE_ERASE:
                    mark_save_file_to_erase(gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex);
                    gSavefileData[gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex]->newGame = TRUE;
                    break;
            }
            break;
        case SAVE_FILE_TYPE_CART_TIMES:
            if (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType != SAVE_FILE_TYPE_ERASE) {
                if (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType == SAVE_FILE_TYPE_CPAK_EMPTY) {
                    ret = write_time_data_to_controller_pak(gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex,
                                                            settings);
                }
            } else {
                clear_lap_records(settings, 3);
                mark_to_write_flap_and_course_times();
                unset_eeprom_settings_value(
                    0xFFFFF0); // Reset most eeprom save data, but keep Adventure 2, and Drumstick.
#if VERSION >= VERSION_79
                gActiveMagicCodes &= ~CHEAT_CONTROL_TT;
                gUnlockedMagicCodes &= ~CHEAT_CONTROL_TT;
#endif
            }
            break;
        case SAVE_FILE_TYPE_CPAK_SAVE:
            switch (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType) {
                case SAVE_FILE_TYPE_CART_SAVE:
                    ret = read_game_data_from_controller_pak(
                        gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex,
                        gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileExt, settings);
                    if (settings->cutsceneFlags & 4 && !is_adventure_two_unlocked()) {
                        ret = CONTROLLER_PAK_NEED_SECOND_ADVENTURE;
                    }
                    if (ret == CONTROLLER_PAK_GOOD) {
                        force_mark_write_save_file(gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex);
                        gSavefileData[gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex]->cutsceneFlags =
                            settings->cutsceneFlags;
                        gSavefileData[gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex]->newGame = 0;
                        *gSavefileData[gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex]->balloonsPtr =
                            *settings->balloonsPtr;
                        gSavefileData[gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex]->filename =
                            settings->filename;
                    }
                    break;
                case SAVE_FILE_TYPE_CPAK_EMPTY:
                    ret = read_game_data_from_controller_pak(
                        gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex,
                        gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileExt, gSavefileData[3]);
                    if (ret == CONTROLLER_PAK_GOOD) {
                        ret = write_game_data_to_controller_pak(gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex,
                                                                gSavefileData[3]);
                    }
                    break;
                case SAVE_FILE_TYPE_ERASE:
                    ret = delete_file(gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex,
                                      gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileNumber);
                    break;
            }
            break;
        case SAVE_FILE_TYPE_CPAK_TIMES:
            for (i = 0; gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileExt[i] != '\0'; i++) {}
            if (i > 0) {
                fileExt[0] = gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileExt[i - 1];
            } else {
                fileExt[0] = 'A';
            }
            fileExt[1] = '\0';
            switch (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType) {
                case SAVE_FILE_TYPE_CART_TIMES:
                    ret = read_time_data_from_controller_pak(
                        gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex, fileExt, settings);
#if VERSION >= VERSION_79
                    mark_to_write_flap_and_course_times();
#endif
                    break;
                case SAVE_FILE_TYPE_CPAK_EMPTY:
                    ret = read_time_data_from_controller_pak(
                        gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex, fileExt, settings);
                    if (ret == CONTROLLER_PAK_GOOD) {
                        ret = write_time_data_to_controller_pak(gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex,
                                                                settings);
                    }
                    mark_to_read_flap_and_course_times();
                    break;
                case SAVE_FILE_TYPE_ERASE:
                    ret = delete_file(gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex,
                                      gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileNumber);
                    break;
            }
            break;
        case SAVE_FILE_TYPE_CPAK_GHOST:
            if (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType != SAVE_FILE_TYPE_ERASE) {
                if (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType != SAVE_FILE_TYPE_CPAK_EMPTY) {
                    if (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType == SAVE_FILE_TYPE_UNK9) {
                        gMenuDelay = 1;
                        gCpakGhostData = gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex;
                    }
                } else {
                    ret = copy_controller_pak_data(gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex,
                                                   gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileNumber,
                                                   gSaveMenuFilesDest[gSaveMenuOptionDest].controllerIndex);
                }
            } else {
                ret = delete_file(gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex,
                                  gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileNumber);
            }
            break;
        case SAVE_FILE_TYPE_CPAK_OTHER:
            if (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType == SAVE_FILE_TYPE_ERASE) {
                ret = delete_file(gSaveMenuFilesSource[gSaveMenuOptionSource].controllerIndex,
                                  gSaveMenuFilesSource[gSaveMenuOptionSource].saveFileNumber);
            }
            break;
        case SAVE_FILE_TYPE_CART_BONUSES:
            if (gSaveMenuFilesDest[gSaveMenuOptionDest].saveFileType == SAVE_FILE_TYPE_ERASE) {
                unset_eeprom_settings_value(
                    0xFFFFF3); // Reset most eeprom save data including Adventure Two unlock and Drumstick unlock.
                gActiveMagicCodes &= ~(CHEAT_CONTROL_TT | CHEAT_CONTROL_DRUMSTICK);
                gUnlockedMagicCodes &= ~(CHEAT_CONTROL_TT | CHEAT_CONTROL_DRUMSTICK);
            }
            break;
        default:
            ret = CONTROLLER_PAK_GOOD;
            break;
    }
    return ret;
}

/**
 * Render an error message based on what the error itself is.
 * This is either because the controller pak is missing, damaged or it found a rumble pak instead.
 */
void savemenu_render_error(SIDeviceStatus deviceStatus) {
    s32 i;
    s32 j;
    s32 k;
    s32 y;
    char *text;

    gControllerIndex = (deviceStatus >> 30) & 3; // Gets the controller index from the device status.
    deviceStatus &= 0x3FFFFFFF;                  // Removes the controller index from the device status value.
    gDeviceStatusStrings = gContPakStrings[deviceStatus];
    gDeviceStatus = deviceStatus;
    gSaveMenuMessageLines = 0;

    for (k = 0; gDeviceStatusStrings[k] != NULL; k++) {}
    k++;
    for (; gDeviceStatusStrings[k] != NULL; gSaveMenuMessageLines++, k++) {}

    gSaveMenuMessageOption = gSaveMenuMessageLines - 1;

    dialogue_clear(7);
    set_current_dialogue_box_coords(7, 40, SCREEN_HEIGHT_HALF - (((k * 16) + 44) >> 1), 280,
                                    (((k * 16) + 44) >> 1) + SCREEN_HEIGHT_HALF);
    set_current_dialogue_background_colour(7, 0, 0, 0, 160);
    set_current_text_background_colour(7, 0, 0, 0, 0);
    set_dialogue_font(7, 2);
    set_current_text_colour(7, 255, 255, 255, 0, 255);
    text = gMenuText[ASSET_MENU_TEXT_PAKERROR];
    if (gDeviceStatusStrings == gContPakRumbleDetectedStrings ||
        gDeviceStatusStrings == gContPakSwitchToRumbleStrings) {
        text = gMenuText[ASSET_MENU_TEXT_CAUTION];
    } else if (gDeviceStatusStrings == gContPakNeed2ndAdvStrings) {
        text = gMenuText[ASSET_MENU_TEXT_GAMEERROR];
    }
    render_dialogue_text(7, POS_CENTRED, 20, text, 1, ALIGN_MIDDLE_CENTER);
    y = 52;
    i = 0;
    set_dialogue_font(7, ASSET_FONTS_FUNFONT);
    for (; gDeviceStatusStrings[i] != NULL; i++, y += 16) {
        render_dialogue_text(7, POS_CENTRED, y, gDeviceStatusStrings[i], gControllerIndex + 1, ALIGN_MIDDLE_CENTER);
    }
    i++;
    y += 16;
    for (j = 0; gDeviceStatusStrings[i] != NULL; i++, j++, y += 16) {
        gSaveMenuText[j] = render_dialogue_text(7, POS_CENTRED, y, gDeviceStatusStrings[i], 1, ALIGN_MIDDLE_CENTER);
    }
    gMenuStage |= 8;
}

/**
 * Scroll the source blocks in the save menu.
 * Press A to load potential destinations and start picking those, press B to return.
 * Reminds the player to reconnect their rumble pak if they had one connected coming in.
 */
s32 savemenu_input_source(s32 buttonsPressed, s32 direction) {
    s32 ret;

    ret = 0;
    if (buttonsPressed & B_BUTTON) {
        gMenuStage = SAVEMENU_ENTER;
        if (gSaveMenuRumbleNag != gSaveMenuRumbleConnected) {
            savemenu_render_error(CONTROLLER_PAK_SWITCH_TO_RUMBLE);
        } else {
            sound_play(SOUND_MENU_BACK3, NULL);
            ret = -1;
        }
    } else if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
        sound_play(SOUND_SELECT2, NULL);
        gMenuStage = SAVEMENU_INIT_DEST;
    } else if (direction < 0 && gSaveMenuOptionSource > 0) {
        sound_play(SOUND_MENU_PICK2, NULL);
        gSaveMenuOptionSource--;
    } else if (direction > 0 && (gSaveMenuOptionSource < gSaveMenuOptionCountUpper - 1)) {
        sound_play(SOUND_MENU_PICK2, NULL);
        gSaveMenuOptionSource++;
    }
    return ret;
}

/**
 * Scroll the destination blocks in the save menu.
 * Press A to get a confirmation box, press B to go back to the main selection.
 */
s32 savemenu_input_dest(s32 buttonsPressed, s32 direction) {
    if (buttonsPressed & B_BUTTON) {
        sound_play(SOUND_MENU_BACK3, NULL);
        gMenuStage = SAVEMENU_CHOOSE_SOURCE;
    } else if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
        sound_play(SOUND_SELECT2, NULL);
        gMenuStage = SAVEMENU_CONFIRM;
    } else if (direction < 0 && gSaveMenuOptionDest > 0) {
        sound_play(SOUND_MENU_PICK2, NULL);
        gSaveMenuOptionDest--;
    } else if (direction > 0 && gSaveMenuOptionDest < gSaveMenuOptionCountLower - 1) {
        sound_play(SOUND_MENU_PICK2, NULL);
        gSaveMenuOptionDest++;
    }

    return 0;
}

/**
 * Allows the player to confirm their choice. This begins the write process.
 * Otherwise, they can back out to the destination selection.
 */
s32 savemenu_input_confirm(s32 buttonsPressed, UNUSED s32 arg1) {
    if (buttonsPressed & B_BUTTON) {
        sound_play(SOUND_MENU_BACK3, NULL);
        gMenuStage = SAVEMENU_CHOOSE_DEST;
    } else if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
        sound_play(SOUND_SELECT2, NULL);
        gMenuStage = SAVEMENU_WRITE;
        gOpacityDecayTimer = 0;
    }

    return 0;
}

/**
 * Takes input if a message is shown onscreen when first coming into the save menu.
 * This resolves having a rumble pak connected, or a damaged controller pak.
 */
s32 savemenu_input_message(s32 buttonsPressed, s32 yAxis) {
    UNUSED s32 pad[2];
    s32 stage;
    s32 i;
    s32 highlight;

    highlight = gOptionBlinkTimer * 8;
    if (highlight > 255) {
        highlight = 511 - highlight;
    }
    stage = (gMenuStage & 7);
    for (i = 0; i < gSaveMenuMessageLines; i++) {
        if (i == gSaveMenuMessageOption) {
            gSaveMenuText[i]->highlight = highlight;
        } else {
            gSaveMenuText[i]->highlight = 0;
        }
    }
    if (buttonsPressed & B_BUTTON || (buttonsPressed & (START_BUTTON | A_BUTTON) &&
                                      (u32) gSaveMenuMessageLines == (u32) (gSaveMenuMessageOption + 1))) {
        sound_play(SOUND_MENU_BACK3, NULL);
        gMenuStage &= ~8;
        switch (stage) {
            case SAVEMENU_ENTER:
                if (!(buttonsPressed & (START_BUTTON | A_BUTTON))) {
                    gOpacityDecayTimer = 6;
                    gMenuStage = SAVEMENU_WAIT;
                }
                break;
            case SAVEMENU_INIT_SOURCE:
                switch (gDeviceStatus) {
                    case CONTROLLER_PAK_CHANGED:
                        gOpacityDecayTimer = 5;
                        gMenuStage = SAVEMENU_WAIT;
                        break;
                    case CONTROLLER_PAK_NOT_FOUND:
                    case CONTROLLER_PAK_INCONSISTENT:
                    case CONTROLLER_PAK_WITH_BAD_ID:
                    case CONTROLLER_PAK_BAD_DATA:
                        gSaveMenuSourceState = 0;
                        break;
                    case CONTROLLER_PAK_RUMBLE_PAK_FOUND:
                        gSaveMenuSourceState = -1;
                        break;
                    default:
                        gSaveMenuSourceState = 1;
                        break;
                }
                break;
            case SAVEMENU_INIT_DEST:
                switch (gDeviceStatus) {
                    case CONTROLLER_PAK_NOT_FOUND:
                    case CONTROLLER_PAK_CHANGED:
                        gOpacityDecayTimer = 5;
                        gMenuStage = SAVEMENU_WAIT;
                        break;
                    case CONTROLLER_PAK_INCONSISTENT:
                    case CONTROLLER_PAK_WITH_BAD_ID:
                    case CONTROLLER_PAK_BAD_DATA:
                        gSaveMenuDestState = 0;
                        break;
                    case CONTROLLER_PAK_RUMBLE_PAK_FOUND:
                        gSaveMenuDestState = -1;
                        break;
                    default:
                        gSaveMenuDestState = 1;
                        break;
                }
                break;
            case SAVEMENU_WRITE:
                switch (gDeviceStatus) {
                    case CONTROLLER_PAK_NOT_FOUND:
                    case CONTROLLER_PAK_INCONSISTENT:
                    case CONTROLLER_PAK_WITH_BAD_ID:
                    case CONTROLLER_PAK_CHANGED:
                    case CONTROLLER_PAK_RUMBLE_PAK_FOUND:
                        gOpacityDecayTimer = 5;
                        gMenuStage = SAVEMENU_WAIT;
                        break;
                    case CONTROLLER_PAK_BAD_DATA:
                    case CONTROLLER_PAK_NEED_SECOND_ADVENTURE:
                        gMenuStage = SAVEMENU_INIT_DEST;
                        break;
                }
                break;
        }
    } else if (buttonsPressed & (START_BUTTON | A_BUTTON)) {
        sound_play(SOUND_SELECT2, NULL);
        gMenuStage &= ~8;
        switch (gDeviceStatus) {
            case CONTROLLER_PAK_WITH_BAD_ID:
                reformat_controller_pak(gControllerIndex);
                if (stage == SAVEMENU_INIT_DEST) {
                    gSaveMenuDestState = 0;
                } else if (stage == SAVEMENU_WRITE) {
                    gSaveMenuSourceState = 0;
                }
                break;
            case CONTROLLER_PAK_INCONSISTENT:
            case CONTROLLER_PAK_BAD_DATA:
                repair_controller_pak(gControllerIndex);
                if (stage == SAVEMENU_INIT_DEST) {
                    gSaveMenuDestState = 1;
                } else if (stage == SAVEMENU_INIT_SOURCE) {
                    gSaveMenuSourceState = 1;
                }
                break;
            case CONTROLLER_PAK_RUMBLE_PAK_FOUND:
                if (stage == SAVEMENU_INIT_DEST) {
                    gSaveMenuDestState = -1;
                } else {
                    gSaveMenuSourceState = -1;
                }
                break;
        }
    } else if (yAxis < 0 && gSaveMenuMessageOption < (gSaveMenuMessageLines - 1)) {
        sound_play(SOUND_MENU_PICK2, NULL);
        gSaveMenuMessageOption++;
    } else if (yAxis > 0 && gSaveMenuMessageOption > 0) {
        sound_play(SOUND_MENU_PICK2, NULL);
        gSaveMenuMessageOption--;
    }
    return 0;
}

/**
 * Load the save option data from the cartridge,
 * and if detected, off the controller pak too.
 * Seperates each menu stage into a seperate function.
 * These navigate around the menu, select the files to read, write and modify,
 * then the cartridge or controller pak are written to.
 */
s32 menu_save_options_loop(s32 updateRate) {
    SIDeviceStatus result;
    s32 buttonsPressed;
    s32 i;
    s32 yAxis;
    s32 xAxis;

    gOptionBlinkTimer = (gOptionBlinkTimer + updateRate) & 0x3F;
    if (gMenuDelay != 0) {
        if (gMenuDelay > 0) {
            gMenuDelay += updateRate;
        } else {
            gMenuDelay -= updateRate;
        }
    }
    if ((gMenuStage & 7) >= SAVEMENU_INIT_SOURCE) {
        savemenu_move(updateRate);
    }
    if (gMenuDelay > -20 && gMenuDelay < 20) {
        savemenu_render(updateRate);
    }
    if (gMenuDelay != 0) {
        if (gMenuDelay > 30) {
            savemenu_free();
            menu_init(MENU_GHOST_DATA);
        } else if (gMenuDelay < -30) {
            savemenu_free();
            menu_init(MENU_OPTIONS);
        }
        return 0;
    }
    buttonsPressed = 0;
    xAxis = 0;
    yAxis = 0;
    if (gIgnorePlayerInputTime == 0) {
        for (i = 0; i < 4; i++) {
            xAxis += gControllersXAxisDirection[i];
            yAxis += gControllersYAxisDirection[i];
            buttonsPressed |= input_pressed(i);
        }
    }
    if (gMenuStage & 8) {
        gMenuDelay = savemenu_input_message(buttonsPressed, yAxis);
    } else {
        switch (gMenuStage) {
            case SAVEMENU_ENTER:
                gMenuDelay = -1;
                break;
            case SAVEMENU_WAIT:
                gOpacityDecayTimer++;
                if (gOpacityDecayTimer > 10) {
                    gMenuStage = SAVEMENU_INIT_SOURCE;
                }
                break;
            case SAVEMENU_INIT_SOURCE:
                gSaveMenuOptionSource = 0;
                gSaveMenuScrollSource = 0.0f;
                result = savemenu_load_sources();
                if (result != CONTROLLER_PAK_GOOD) {
                    savemenu_render_error(result);
                } else {
                    gMenuStage = SAVEMENU_CHOOSE_SOURCE;
                }
                break;
            case SAVEMENU_CHOOSE_SOURCE:
                gMenuDelay = savemenu_input_source(buttonsPressed, xAxis);
                break;
            case SAVEMENU_INIT_DEST:
                gSaveMenuOptionDest = 0;
                gSaveMenuScrollDest = 0.0f;
                result = savemenu_load_destinations();
                if (result != CONTROLLER_PAK_GOOD) {
                    savemenu_render_error(result);
                } else {
                    gMenuStage = SAVEMENU_CHOOSE_DEST;
                }
                break;
            case SAVEMENU_CHOOSE_DEST:
                gMenuDelay = savemenu_input_dest(buttonsPressed, xAxis);
                break;
            case SAVEMENU_CONFIRM:
                gMenuDelay = savemenu_input_confirm(buttonsPressed, xAxis);
                break;
            case SAVEMENU_WRITE:
                gOpacityDecayTimer++;
                if (gOpacityDecayTimer > 3) {
                    result = savemenu_write();
                    if (result != CONTROLLER_PAK_GOOD) {
                        savemenu_render_error(result);
                    } else {
                        gOpacityDecayTimer = 6;
                        gMenuStage = SAVEMENU_WAIT;
                    }
                }
                break;
        }
        if (gMenuDelay) {
            transition_begin(&sMenuTransitionFadeIn);
        }
    }
    gIgnorePlayerInputTime = 0;

    return MENU_RESULT_CONTINUE;
}

/**
 * Free the assets associated with the save options menu.
 */
void savemenu_free(void) {
    unload_font(ASSET_FONTS_BIGFONT);
    menu_button_free();
    menu_assetgroup_free(gSaveMenuObjectIndices);
    dialogue_clear(7);
    mempool_free((void *) gSaveMenuFilesSource);
    mempool_free((void *) sSaveMenuFilenames);
#if VERSION >= VERSION_79
    rumble_enable(TRUE);
#endif
}

SIDeviceStatus menu_find_usable_pak(s32 *controllerIndex, s32 xAxisDirection) {
    s32 j;
    s32 k;
    s32 i;
    UNUSED s32 pad;
    s32 paksWithStatusSuccess;
    s32 ret;
    u32 bytesFree;
    s32 notesFree;
    s32 paksWithNoFreeSpace;
    s32 paksWithFatalErrors;
    s32 paksWithBadData;

    paksWithFatalErrors = 0;
    paksWithNoFreeSpace = 0;
    paksWithBadData = 0;
    paksWithStatusSuccess = 0;

    // This has been hard limited to just the first controller pak, but can easily be modified to check the rest with i
    // <= MAXCONTROLLERS
    for (i = 0; i <= 0; i++) {
        sControllerPakFatalErrorFound[i] = FALSE;
        sControllerPakNoFreeSpace[i] = FALSE;
        sControllerPakBadData[i] = FALSE;
        ret = get_free_space(i, &bytesFree, &notesFree);
        if (ret == CONTROLLER_PAK_GOOD) {
            sControllerPakIssueNotFound[i] = TRUE;
            if (bytesFree == 0 || notesFree == 0) {
                sControllerPakNoFreeSpace[i] = TRUE;
                if (sControllerPakDataPresent[i] == FALSE) {
                    paksWithNoFreeSpace++;
                }
            }
            paksWithStatusSuccess++;
        } else {
            ret &= 0xFF; // Upper 2 bits are controller index
            sControllerPakIssueNotFound[i] = FALSE;
            if (ret == CONTROLLER_PAK_BAD_DATA) {
                sControllerPakBadData[i] = TRUE;
                paksWithBadData++;
            }
            if (ret == CONTROLLER_PAK_INCONSISTENT) {
                repair_controller_pak(i);
            }
            if (ret == CONTROLLER_PAK_WITH_BAD_ID) {
                sControllerPakFatalErrorFound[i] = TRUE;
                paksWithFatalErrors++;
            }
        }
    }

    if (paksWithStatusSuccess == 0 || paksWithFatalErrors != 0 || paksWithNoFreeSpace != 0 || paksWithBadData != 0) {
        return CONTROLLER_PAK_NOT_FOUND; // Return unsuccessfully
    }

    // Loop through the cpaks to find the next one with no issues found.
    i = *controllerIndex;
    if (i < 0) {
        //!@bug If i == -1, and sControllerPakIssueNotFound[0] is FALSE, then this will get stuck in an infinite loop.
        // This is saved by the fact that this code can't be reached when cpak 0 has an error as it will return above.
        do {
            i++;
            if (i > 0) {
                i--;
            }
        } while (sControllerPakIssueNotFound[i] == FALSE);
    } else if (sControllerPakIssueNotFound[*controllerIndex] == FALSE || xAxisDirection > 0) {
        do {
            i++;
            if (i > 0) {
                i--;
            }
        } while (sControllerPakIssueNotFound[i] == FALSE);
    } else if (xAxisDirection < 0) {
        do {
            i--;
            if (i < 0) {
                i++;
            }
        } while (sControllerPakIssueNotFound[i] == FALSE);
    }

    // Set the controller index to the next cpak with no issue found.
    *controllerIndex = i;
    ret = get_controller_pak_file_list(*controllerIndex, MAX_CPAK_FILES, sCurrentControllerPakAllFileNames,
                                       sCurrentControllerPakAllFileExtensions, sCurrentControllerPakNumberOfPages,
                                       sCurrentControllerPakAllFileTypes);

    if (ret == CONTROLLER_PAK_GOOD) {
        for (i = 0; i < MAX_CPAK_FILES; i++) {
            sCurrentControllerPakNumberOfPages[i] /= (PFS_ONE_PAGE * BLOCKSIZE);
            j = 0;
            if (sCurrentControllerPakAllFileNames[i] != NULL) {
                k = 0;
                while (sCurrentControllerPakAllFileNames[i][k] != '\0') {
                    gBootPakData[i][j++] = sCurrentControllerPakAllFileNames[i][k++];
                }

                if (sCurrentControllerPakAllFileExtensions[i] != NULL &&
                    sCurrentControllerPakAllFileExtensions[i][0] != '\0') {
                    gBootPakData[i][j++] = '.';
                    k = 0;
                    while (sCurrentControllerPakAllFileExtensions[i][k] != '\0') {
                        gBootPakData[i][j++] = sCurrentControllerPakAllFileExtensions[i][k++];
                    }
                }
            }
            if (j == 0) {
                gBootPakData[i][j++] = '-';
            }
            gBootPakData[i][j] = '\0';
        }

        cpak_free_files();
        get_free_space(*controllerIndex, &sCurrentControllerPakFreePages, NULL);
        sCurrentControllerPakFreePages /= (PFS_ONE_PAGE * BLOCKSIZE);
    }

    return ret;
}

/**
 * Sets and returns an error code if any controller paks have an error.
 * Current implementation only checks the first controller pak.
 * 0 = No Error
 * 1 = Fatal Error
 * 2 = No Free Space
 * 3 = Corrupt Data
 */
PakError check_for_controller_pak_errors(void) {
    s32 errorFound;
    s32 i;

    errorFound = FALSE;
    sControllerPakError = PAK_ERROR_NONE;

    // This has been hard limited to just the first controller pak, but can easily be modified to check the rest with i
    // <= MAXCONTROLLERS
    for (i = 0; i <= 0 && errorFound == FALSE; i++) {
        if (sControllerPakFatalErrorFound[i]) {
            gControllerPakMenuElement[0].t.asciiText = gMenuText[ASSET_MENU_TEXT_PAKERROR]; // PAK ERROR
            gControllerPakMenuElement[1].t.asciiText = gMenuText[ASSET_MENU_TEXT_PAKERROR]; // PAK ERROR
            gControllerPakMenuElement[2].t.asciiText =
                gMenuText[ASSET_MENU_TEXT_CONTPAK1 + i]; // CONTROLLER PAK 1 / 2 / 3 / 4
            gControllerPakMenuElement[3].t.asciiText =
                gMenuText[ASSET_MENU_TEXT_CONTPAKISDAMAGED_0];                                  // IRREPARABLY DAMAGED.
            gControllerPakMenuElement[4].t.asciiText = gMenuText[ASSET_MENU_TEXT_TURNOFFN64_0]; // Switch off the N64
            gControllerPakMenuElement[5].t.asciiText =
                gMenuText[ASSET_MENU_TEXT_TURNOFFN64_1]; // and remove the controller pak.
            sControllerPakError = PAK_ERROR_FATAL;
            errorFound = TRUE;
        } else if (sControllerPakNoFreeSpace[i] && sControllerPakDataPresent[i] == FALSE) {
            gControllerPakMenuElement[0].t.asciiText = gMenuText[ASSET_MENU_TEXT_CAUTION]; // CAUTION
            gControllerPakMenuElement[1].t.asciiText = gMenuText[ASSET_MENU_TEXT_CAUTION]; // CAUTION
            gControllerPakMenuElement[2].t.asciiText =
                gMenuText[ASSET_MENU_TEXT_CONTPAK1 + i]; // CONTROLLER PAK 1 / 2 / 3 / 4
            gControllerPakMenuElement[3].t.asciiText = gMenuText[ASSET_MENU_TEXT_CONTPAKFULL]; // CONTROLLER PAK FULL
            gControllerPakMenuElement[4].t.asciiText = gMenuText[ASSET_MENU_TEXT_CONTINUE];    // CONTINUE
            gControllerPakMenuElement[5].t.asciiText = NULL;
            sControllerPakDataPresent[i] = TRUE;
            sControllerPakError = PAK_ERROR_FULL;
            errorFound = TRUE;
        } else if (sControllerPakBadData[i]) {
            gControllerPakMenuElement[0].t.asciiText = gMenuText[ASSET_MENU_TEXT_CAUTION]; // CAUTION
            gControllerPakMenuElement[1].t.asciiText = gMenuText[ASSET_MENU_TEXT_CAUTION]; // CAUTION
            gControllerPakMenuElement[2].t.asciiText =
                gMenuText[ASSET_MENU_TEXT_CONTPAK1 + i]; // CONTROLLER PAK 1 / 2 / 3 / 4
            gControllerPakMenuElement[3].t.asciiText = gMenuText[ASSET_MENU_TEXT_CORRUPTDATA_0]; // CORRUPT DATA.
            gControllerPakMenuElement[4].t.asciiText = gMenuText[ASSET_MENU_TEXT_CORRUPTDATA_1]; // TRY AGAIN!
            gControllerPakMenuElement[5].t.asciiText = NULL;
            sControllerPakError = PAK_ERROR_CORRUPT;
            errorFound = TRUE;
        }
    }

    return sControllerPakError;
}
