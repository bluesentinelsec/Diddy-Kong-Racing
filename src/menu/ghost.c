#include "menu_internal.h"

static s32 gGhostMenuOption;

static s32 gGhostMenuTotal;

static u8 gGhostLevelIDsPak[6];

static u8 gGhostCharacterIDsPak[6];

static u8 gGhostVehicleIDsPak[6];

static u16 gGhostChecksumIDsPak[6];

static u8 gGhostLevelIDsMenu[6];

static u8 gGhostCharacterIDsMenu[6];

static u8 gGhostVehicleIDsMenu[6];

static u16 gGhostChecksumIDsMenu[6];

static u8 gGhostWorldIDs[8];

// DrawTextures for Dino Domain ghost background.
static DrawTexture gDrawTexDinoDomainGhostBg[] = { { NULL, 0, 0 },    { NULL, 64, 0 },   { NULL, 128, 0 }, { NULL, 192, 0 },
                                            { NULL, 256, 0 },  { NULL, 0, 32 },   { NULL, 64, 32 }, { NULL, 128, 32 },
                                            { NULL, 192, 32 }, { NULL, 256, 32 }, { NULL, 0, 0 } };

// DrawTextures for Sherbet Island ghost background.
static DrawTexture gDrawTexSherbetIslandGhostBg[] = { { NULL, 0, 0 },    { NULL, 64, 0 },   { NULL, 128, 0 },
                                               { NULL, 192, 0 },  { NULL, 256, 0 },  { NULL, 0, 32 },
                                               { NULL, 64, 32 },  { NULL, 128, 32 }, { NULL, 192, 32 },
                                               { NULL, 256, 32 }, { NULL, 0, 0 } };

// DrawTextures for Snowflake Mountain ghost background.
static DrawTexture gDrawTexSnowflakeMountainGhostBg[] = { { NULL, 0, 0 },    { NULL, 64, 0 },   { NULL, 128, 0 },
                                                   { NULL, 192, 0 },  { NULL, 256, 0 },  { NULL, 0, 32 },
                                                   { NULL, 64, 32 },  { NULL, 128, 32 }, { NULL, 192, 32 },
                                                   { NULL, 256, 32 }, { NULL, 0, 0 } };

// DrawTextures for Dragon Forest ghost background.
static DrawTexture gDrawTexDragonForestGhostBg[] = { { NULL, 0, 0 },    { NULL, 64, 0 },   { NULL, 128, 0 }, { NULL, 192, 0 },
                                              { NULL, 256, 0 },  { NULL, 0, 32 },   { NULL, 64, 32 }, { NULL, 128, 32 },
                                              { NULL, 192, 32 }, { NULL, 256, 32 }, { NULL, 0, 0 } };

// DrawTextures for Future Fun Land ghost background.
static DrawTexture gDrawTexFutureFunLandGhostBg[] = { { NULL, 0, 0 },    { NULL, 64, 0 },   { NULL, 128, 0 },
                                               { NULL, 192, 0 },  { NULL, 256, 0 },  { NULL, 0, 32 },
                                               { NULL, 64, 32 },  { NULL, 128, 32 }, { NULL, 192, 32 },
                                               { NULL, 256, 32 }, { NULL, 0, 0 } };

static DrawTexture *gDrawTexWorldBgs[5] = {
    gDrawTexDinoDomainGhostBg,        // Dino Domain
    gDrawTexSherbetIslandGhostBg,     // Sherbet Island
    gDrawTexSnowflakeMountainGhostBg, // Snowflake Mountain
    gDrawTexDragonForestGhostBg,      // Dragon Forest
    gDrawTexFutureFunLandGhostBg      // Future Fun Land
};

static s16 gGhostDataObjectIndices[34] = { 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                                    0x1A, 0x1B, 0x1C, 0x1D, 0x32, 0x33, 0x34, 0x36, 0x35, 0x37, 0x38, 0x39,
                                    0x3A, 0x3B, 0x00, 0x01, 0x04, 0x3D, 0x3C, 0x3F, 0x3E, -1 };

static s16 gGhostDataImageIndices[4] = { 0, 1, 7, -1 };

static s16 gGhostDataElementPositions[10] = { 0x78, 0x12, 0x08, 0x06, 0xC0, 0x06, 0xD4, 0x1A, 0x78, 0x22 };

static s8 sGhostMenuIconOffsets[8] = { 0, -1, 1, 0, 0, 1, -1, 0 };

/**
 * Reads all the ghost data from the controller pak and checks if it's valid.
 * Any valid data gets added as a total, and then the menu gets populated with that data.
 */
void ghostmenu_generate(void) {
    u8 *mainTrackIds;
    s32 i;
    s32 j;
    u16 validIDs[4];
    u16 swap;
    u16 swapByte;
    UNUSED s32 pad;

    gGhostMenuTotal = 0;
    mainTrackIds = (u8 *) get_misc_asset(ASSET_MISC_MAIN_TRACKS_IDS);

    for (i = 0; i < 6; i++) {
        if (gGhostLevelIDsPak[i] != 0xFF) {
            for (j = 0; mainTrackIds[j] != 0xFF && mainTrackIds[j] != gGhostLevelIDsPak[i]; j++) {}
            if (mainTrackIds[j] != 0xFF) {
                gGhostWorldIDs[gGhostMenuTotal] = i;
                validIDs[gGhostMenuTotal] = (j * 8) + gGhostVehicleIDsPak[i];
                gGhostMenuTotal++;
            }
        }
    }

    for (i = gGhostMenuTotal - 1; i > 0; i--) {
        for (j = 0; j < i; j++) {
            if (validIDs[j + 1] < validIDs[j]) {
                swap = validIDs[j];
                validIDs[j] = validIDs[j + 1];
                validIDs[j + 1] = swap;
                swapByte = gGhostWorldIDs[j];
                gGhostWorldIDs[j] = gGhostWorldIDs[j + 1];
                gGhostWorldIDs[j + 1] = swapByte;
            }
        }
    }

    for (i = 0; i < gGhostMenuTotal; i++) {
        gGhostLevelIDsMenu[i] = gGhostLevelIDsPak[gGhostWorldIDs[i]];
        gGhostCharacterIDsMenu[i] = gGhostCharacterIDsPak[gGhostWorldIDs[i]];
        gGhostVehicleIDsMenu[i] = gGhostVehicleIDsPak[gGhostWorldIDs[i]];
        gGhostChecksumIDsMenu[i] = gGhostChecksumIDsPak[gGhostWorldIDs[i]];
    }
}

/**
 * Erase a ghost from the controller pak.
 * Move further entries forward to fill in the new gap.
 */
s32 ghostmenu_erase(s32 id) {
    s32 temp_s0;
    s32 result;
    s32 i;

    result = CONTROLLER_PAK_GOOD;
    if (id >= 0 && id < gGhostMenuTotal) {
        sound_play(SOUND_SELECT2, NULL);
        temp_s0 = gGhostWorldIDs[id];
        result = ghost_delete_for_world(gRaceSetup.cpakGhostData, temp_s0);
        if (result == CONTROLLER_PAK_GOOD) {
            gGhostLevelIDsPak[temp_s0] = 0xFF;
            gGhostMenuTotal--;
            for (i = id; i < gGhostMenuTotal; i++) {
                gGhostWorldIDs[i] = gGhostWorldIDs[i + 1];
                gGhostLevelIDsMenu[i] = gGhostLevelIDsMenu[i + 1];
                gGhostCharacterIDsMenu[i] = gGhostCharacterIDsMenu[i + 1];
                gGhostVehicleIDsMenu[i] = gGhostVehicleIDsMenu[i + 1];
                gGhostChecksumIDsMenu[i] = gGhostChecksumIDsMenu[i + 1];
            }
            for (i = 0; i < gGhostMenuTotal; i++) {
                if (temp_s0 < gGhostWorldIDs[i]) {
                    gGhostWorldIDs[i]--;
                }
            }
        }
    }
    return result;
}

void menu_ghost_data_init(void) {
    s32 i;
    SIDeviceStatus pakStatus;

    pakStatus = ghost_list(gRaceSetup.cpakGhostData, gGhostLevelIDsPak, gGhostVehicleIDsPak, gGhostCharacterIDsPak,
                           gGhostChecksumIDsPak);
    if (pakStatus == CONTROLLER_PAK_GOOD) {
        ghostmenu_generate();
    }
    menu_assetgroup_load(gGhostDataObjectIndices);
    menu_imagegroup_load(gGhostDataImageIndices);
    load_font(ASSET_FONTS_BIGFONT);

    for (i = 0; i < 5; i++) {
        gDrawTexDinoDomainGhostBg[i].texture = gMenuLoaded.assets[TEXTURE_BACKGROUND_DINO_DOMAIN_TOP];
        gDrawTexDinoDomainGhostBg[i + 5].texture = gMenuLoaded.assets[TEXTURE_BACKGROUND_DINO_DOMAIN_BOTTOM];
        gDrawTexSherbetIslandGhostBg[i + ((i & 1) * 5)].texture =
            gMenuLoaded.assets[TEXTURE_BACKGROUND_SHERBERT_ISLAND_TOP];
        gDrawTexSherbetIslandGhostBg[i + (((i & 1) ^ 1) * 5)].texture =
            gMenuLoaded.assets[TEXTURE_BACKGROUND_SHERBERT_ISLAND_BOTTOM];
        gDrawTexSnowflakeMountainGhostBg[i].texture = gMenuLoaded.assets[TEXTURE_BACKGROUND_SNOWFLAKE_MOUNTAIN_TOP];
        gDrawTexSnowflakeMountainGhostBg[i + 5].texture =
            gMenuLoaded.assets[TEXTURE_BACKGROUND_SNOWFLAKE_MOUNTAIN_BOTTOM];
        gDrawTexDragonForestGhostBg[i].texture = gMenuLoaded.assets[TEXTURE_BACKGROUND_DRAGON_FOREST_TOP];
        gDrawTexDragonForestGhostBg[i + 5].texture = gMenuLoaded.assets[TEXTURE_BACKGROUND_DRAGON_FOREST_BOTTOM];
        gDrawTexFutureFunLandGhostBg[i + ((i & 1) * 5)].texture =
            gMenuLoaded.assets[TEXTURE_BACKGROUND_FUTURE_FUN_LAND_TOP];
        gDrawTexFutureFunLandGhostBg[i + (((i & 1) ^ 1) * 5)].texture =
            gMenuLoaded.assets[TEXTURE_BACKGROUND_FUTURE_FUN_LAND_BOTTOM];
    }

    menu_init_vehicle_textures();
    menu_racer_portraits();
    menu_init_arrow_textures();
    gMenuFlow.blinkTimer = 0;
    gMenuFlow.stage = 0;
    gMenuFlow.opacityDecayTimer = 0;
    gMenuInput.delay = 0;
    gGhostMenuOption = 0;
    if (pakStatus == CONTROLLER_PAK_GOOD) {
        transition_begin(&sMenuTransitionFadeOut);
        return;
    }
    gMenuInput.delay = 30;
}

/**
 * Render the list of existing ghost data.
 * This includes their names, character, vehicle, world ID and track ID.
 * Also render the confirmation box if it should be active.
 */
void ghostmenu_render(UNUSED s32 updateRate) {
    s32 currentWorldId;
    s32 numToDraw;
    s32 scroll;
    s32 i;
    s32 y;
    s32 highlight;
    s32 x;
    s32 heightAdjust;
    DrawTexture *vehicleSelectTex;
    char *levelName;
    char textBuffer[64];

    mtx_ortho(&gMenuFrame.displayList, &gMenuFrame.hudMat);
    if (osTvType == OS_TV_TYPE_PAL) {
        heightAdjust = 12;
    } else {
        heightAdjust = 0;
    }
    scroll = gMenuFlow.opacityDecayTimer;
    set_text_font(FONT_LARGE);
    set_text_background_colour(0, 0, 0, 0);
    set_text_colour(0, 0, 0, 255, 128);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 1, 35, gMenuLoaded.text[ASSET_MENU_TEXT_GHOSTDATA],
              ALIGN_MIDDLE_CENTER);
    set_text_colour(255, 255, 255, 0, 255);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, 32, gMenuLoaded.text[ASSET_MENU_TEXT_GHOSTDATA],
              ALIGN_MIDDLE_CENTER);
    y = 56;
    if (gGhostMenuTotal <= 0) {
        set_text_colour(255, 255, 255, 0, 255);
        draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, heightAdjust + SCREEN_HEIGHT_HALF,
                  gMenuLoaded.text[ASSET_MENU_TEXT_NOGHOSTSSAVED], ALIGN_MIDDLE_CENTER);
        return;
    }
    numToDraw = 3;
    highlight = gMenuFlow.blinkTimer * 8;
    if (highlight > 255) {
        highlight = 511 - highlight;
    }
    highlight |= ~0xFF;
    set_text_font(FONT_SMALL);
    x = 40;
    while (scroll < gGhostMenuTotal && numToDraw > 0) {
        if (gGhostDataElementPositions[0]) {} // Fakematch

        currentWorldId = leveltable_world(gGhostLevelIDsMenu[scroll]) - 1;
        if (currentWorldId < 0 || currentWorldId >= WORLD_FUTURE_FUN_LAND) {
            currentWorldId = 0;
        }
        levelName = level_name(gGhostLevelIDsMenu[scroll]);
        for (i = 0; levelName[i] != '\0' && i < 63; i++) {
            textBuffer[i] = levelName[i];
            if (textBuffer[i] >= 'a' && textBuffer[i] <= 'z') {
                textBuffer[i] ^= 0x20; // Force uppercase.
            }
        }
        textBuffer[i] = '\0'; // Set NULL terminator
        texrect_draw_scaled(&gMenuFrame.displayList, gDrawTexWorldBgs[currentWorldId], x, y, 0.75f, 0.8125f,
                            COLOUR_RGBA32(255, 255, 255, 255), 0);
        menu_panel_draw_bordered(&gMenuFrame.displayList, 40, y, 240, 52, 4, 4, 32, 80, 176, 128);
        if (scroll == gGhostMenuOption) {
            menu_panel_draw_bordered(&gMenuFrame.displayList, 40, y, 240, 52, 4, 4, highlight, highlight, highlight,
                                     highlight);
        }
#define GHOSTMENU_TEXT_OFFSET 40
        set_text_colour(0, 0, 0, 255, 255);
        for (i = 0; i < 4; i++) {
            draw_text(&gMenuFrame.displayList,
                      gGhostDataElementPositions[0] + GHOSTMENU_TEXT_OFFSET + sGhostMenuIconOffsets[(i << 1)],
                      y + gGhostDataElementPositions[1] + sGhostMenuIconOffsets[(i << 1) + 1], textBuffer,
                      ALIGN_MIDDLE_CENTER);
        }
        set_text_colour(200, 228, 80, 255, 255);
        draw_text(&gMenuFrame.displayList, gGhostDataElementPositions[0] + GHOSTMENU_TEXT_OFFSET,
                  gGhostDataElementPositions[1] + y, textBuffer, ALIGN_MIDDLE_CENTER);
        texrect_draw(&gMenuFrame.displayList, gRacerPortraits[gGhostCharacterIDsMenu[scroll]],
                     gGhostDataElementPositions[2] + 40, gGhostDataElementPositions[3] + y, 255, 255, 255, 255);
        switch (gGhostVehicleIDsMenu[scroll]) {
            case 1:
                vehicleSelectTex = gRaceSelectionHoverTex;
                break;
            case 2:
                vehicleSelectTex = gRaceSelectionPlaneTex;
                break;
            default:
                vehicleSelectTex = gRaceSelectionCarTex;
                break;
        }
        texrect_draw_scaled(&gMenuFrame.displayList, vehicleSelectTex, (gGhostDataElementPositions[4] + 40),
                            (gGhostDataElementPositions[5] + y), 0.625f, 0.625f, COLOUR_RGBA32(255, 255, 255, 255), 0);
        rendermode_reset(&gMenuFrame.displayList);
        gMenuLoaded.images[7].trans.x_position = (gGhostDataElementPositions[6] - SCREEN_HEIGHT_HALF);
        gMenuLoaded.images[7].trans.y_position =
            ((-gGhostDataElementPositions[7] - y) + heightAdjust + SCREEN_HEIGHT_HALF);
        gMenuLoaded.images[7].trans.scale = 0.075f;
        menu_element_render(7);
        gMenuFrame.guiOpacity = 128;
        // Timestamp Shadow gets drawn first
        menu_timestamp_render(gGhostChecksumIDsMenu[scroll], gGhostDataElementPositions[8] - (SCREEN_HEIGHT_HALF - 1),
                              (-gGhostDataElementPositions[9] - y) + heightAdjust + (SCREEN_HEIGHT_HALF - 1), 0, 0, 0,
                              FONT_COLOURFUL);
        gMenuFrame.guiOpacity = 255;
        menu_timestamp_render(gGhostChecksumIDsMenu[scroll], gGhostDataElementPositions[8] - (SCREEN_HEIGHT_HALF + 1),
                              (-gGhostDataElementPositions[9] - y) + heightAdjust + (SCREEN_HEIGHT_HALF + 1), 255, 192,
                              255, FONT_COLOURFUL);
        scroll++;
        numToDraw--;
        y += 54;
    }
    highlight &= 0xFF;
    if (gMenuFlow.stage > GHOSTMENU_CHOOSE) {
        clear_dialogue_box_open_flag(7);
        dialogue_clear(7);
#if VERSION == VERSION_80
        set_current_dialogue_box_coords(7, 92, 102, 228, 138);
#else
        set_current_dialogue_box_coords(7, 104, 102, 216, 138);
#endif
        set_current_dialogue_background_colour(7, 0, 0, 0, 192);
        set_dialogue_font(7, 0);
        set_current_text_background_colour(7, 0, 0, 0, 0);
        if (gMenuFlow.stage == GHOSTMENU_ERASE) {
            set_current_text_colour(7, 255, 255, 255, highlight, 255);
        } else {
            set_current_text_colour(7, 255, 255, 255, 0, 255);
        }
        render_dialogue_text(7, POS_CENTRED, 12, gMenuLoaded.text[ASSET_MENU_TEXT_ERASEGHOST], 1, ALIGN_MIDDLE_CENTER);
        if (gMenuFlow.stage == GHOSTMENU_CONFIRM) {
            set_current_text_colour(7, 255, 255, 255, highlight, 255);
        } else {
            set_current_text_colour(7, 255, 255, 255, 0, 255);
        }
        render_dialogue_text(7, POS_CENTRED, 28, gMenuLoaded.text[ASSET_MENU_TEXT_CANCEL], 1, ALIGN_MIDDLE_CENTER);
        render_dialogue_box(&gMenuFrame.displayList, NULL, NULL, 7);
    }
    if (gMenuFlow.blinkTimer & 0x10) {
        if ((gMenuFlow.opacityDecayTimer + 3) < gGhostMenuTotal) {
            texrect_draw(&gMenuFrame.displayList, gMenuSelectionArrowDown, SCREEN_WIDTH_HALF + 1, y + 3, 0, 0, 0, 128);
            texrect_draw(&gMenuFrame.displayList, gMenuSelectionArrowDown, SCREEN_WIDTH_HALF - 1, y + 1, 255, 255, 255,
                         255);
        }
        if (gMenuFlow.opacityDecayTimer > 0) {
            texrect_draw(&gMenuFrame.displayList, gMenuSelectionArrowUp, SCREEN_WIDTH_HALF + 1, 54, 0, 0, 0, 128);
            texrect_draw(&gMenuFrame.displayList, gMenuSelectionArrowUp, SCREEN_WIDTH_HALF - 1, 52, 255, 255, 255, 255);
        }
        rendermode_reset(&gMenuFrame.displayList);
    }
}

/**
 * Show a list of all the saved ghost data and let the player pick individual ones to erase.
 */
s32 menu_ghost_data_loop(s32 updateRate) {
    s32 i;
    s32 pressedButtons;
    s32 xStick;
    s32 yStick;
    s32 prevOption;
    UNUSED s32 unused;

    gMenuFlow.blinkTimer = (gMenuFlow.blinkTimer + updateRate) & 0x3F;

    if (gMenuInput.delay > -20 && gMenuInput.delay < 20) {
        ghostmenu_render(updateRate);
    }

    pressedButtons = 0;
    if (gMenuInput.delay) {
        if (gMenuInput.delay < 0) {
            gMenuInput.delay -= updateRate;
        } else {
            gMenuInput.delay += updateRate;
        }
    }

    xStick = 0;
    yStick = 0;
    if (!gMenuInput.ignoreInputTime && gMenuInput.delay == 0) {
        for (i = 0; i < MAXCONTROLLERS; i++) {
            pressedButtons |= input_pressed(i);
            xStick += gMenuInput.xAxisDirection[i];
            yStick += gMenuInput.yAxisDirection[i];
        }
    }

    switch (gMenuFlow.stage) {
        case GHOSTMENU_CHOOSE:
            if ((pressedButtons & (START_BUTTON | A_BUTTON)) && gGhostMenuTotal > 0) {
                gMenuFlow.stage = GHOSTMENU_CONFIRM;
                sound_play(SOUND_SELECT2, NULL);
            } else if (pressedButtons & B_BUTTON ||
                       (pressedButtons & (START_BUTTON | A_BUTTON) && gGhostMenuTotal == 0)) {
                gMenuInput.delay = 1;
                transition_begin(&sMenuTransitionFadeIn);
                sound_play(SOUND_MENU_BACK3, NULL);
            } else {
                prevOption = gGhostMenuOption;
                if (yStick < 0 && gGhostMenuOption < gGhostMenuTotal - 1) {
                    gGhostMenuOption++;
                    if (gGhostMenuOption >= gMenuFlow.opacityDecayTimer + 3) {
                        gMenuFlow.opacityDecayTimer = gGhostMenuOption - 2;
                    }
                }
                if (yStick > 0 && gGhostMenuOption > 0) {
                    gGhostMenuOption--;
                    if (gGhostMenuOption < gMenuFlow.opacityDecayTimer) {
                        gMenuFlow.opacityDecayTimer = gGhostMenuOption;
                    }
                }
                if (prevOption != gGhostMenuOption) {
                    sound_play(SOUND_MENU_PICK2, NULL);
                }
            }
            break;
        case GHOSTMENU_ERASE:
            if (pressedButtons & B_BUTTON) {
                gMenuFlow.stage = GHOSTMENU_CHOOSE;
                sound_play(SOUND_MENU_BACK3, NULL);
            } else {
                if (pressedButtons & (START_BUTTON | A_BUTTON)) {
                    if (ghostmenu_erase(gGhostMenuOption) == CONTROLLER_PAK_GOOD) {
                        if (gGhostMenuOption >= gGhostMenuTotal) {
                            gGhostMenuOption = gGhostMenuTotal - 1;
                        }
                        if (gGhostMenuOption < 0) {
                            gGhostMenuOption = 0;
                        }
                        if (gGhostMenuOption < gMenuFlow.opacityDecayTimer) {
                            gMenuFlow.opacityDecayTimer = gGhostMenuOption;
                        }
                        sound_play(SOUND_SELECT2, NULL);
                    } else {
                        gMenuInput.delay = 1;
                        transition_begin(&sMenuTransitionFadeIn);
                        sound_play(SOUND_MENU_BACK3, NULL);
                    }
                    gMenuFlow.stage = GHOSTMENU_CHOOSE;
                } else if (yStick < 0) {
                    gMenuFlow.stage = GHOSTMENU_CONFIRM;
                    sound_play(SOUND_MENU_PICK2, NULL);
                }
            }
            break;
        case GHOSTMENU_CONFIRM:
            if (pressedButtons & (START_BUTTON | A_BUTTON | B_BUTTON)) {
                gMenuFlow.stage = GHOSTMENU_CHOOSE;
                sound_play(SOUND_MENU_BACK3, NULL);
            } else if (yStick > 0) {
                gMenuFlow.stage = GHOSTMENU_ERASE;
                sound_play(SOUND_MENU_PICK2, NULL);
            }
            break;
    }

    gMenuInput.ignoreInputTime = FALSE;
    if (gMenuInput.delay > 30) {
        ghostmenu_free();
        menu_init(MENU_SAVE_OPTIONS);
    }
    return MENU_RESULT_CONTINUE;
}

/**
 * Free all assets associated with the ghost data menu.
 */
void ghostmenu_free(void) {
    menu_assetgroup_free(gGhostDataObjectIndices);
    unload_font(ASSET_FONTS_BIGFONT);
}
