#include "host.h"
#include "menu_internal.h"

static s32 gTrackmenuLoadedLevel; // Compared for equality to gTrackSelect.previewTrackId

static s32 gMultiplayerSelectedNumberOfRacersCopy; // Saved version gRaceSetup.multiplayerRacerCount?

static s32 gTrackmenuType;

static SoundHandle gTrackTTSoundMask;

static SoundHandle sMenuSoundHandle;

static s32 gTrackSelectVertsFlip;

static TrackRenderDetails gTrackSelectRenderDetails[9];

static s8 gPlayerSelectConfirm[4];

static s32 gTrackSelectCursorX;

static f32 gTrackSelectX;

static f32 gTrackSelectY;

static f32 gTrackSelectTargetX;

static f32 gTrackSelectTargetY;

static s32 gSelectedTrackX;

static s32 gSelectedTrackY;

static DrawTexture gRaceSelectionTTOn[3] = { { NULL, 0, 0 }, { NULL, 0, 32 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionTTOff[3] = { { NULL, 0, 0 }, { NULL, 0, 32 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionCarOptHighlight[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionCarOpt[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionHoverOptHighlight[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionHoverOpt[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionPlaneOptHighlight[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionPlaneOpt[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionTTOnOptHighlight[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionTTOffOptHighlight[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionTTOnOpt[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionTTOffOpt[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionPlayer1Texture[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionPlayer2Texture[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionPlayer3Texture[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionPlayer4Texture[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionVehicleTitleTexture[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture gRaceSelectionTTTitleTexture[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

static DrawTexture sTrackMenuFrameTexturesA[4] = {
    { NULL, 0, -48 }, { NULL, 0, -16 }, { NULL, 0, 16 }, { NULL, 0, 0 }
};

static DrawTexture sTrackMenuFrameTexturesB[4] = {
    { NULL, 0, -48 }, { NULL, 0, -16 }, { NULL, 0, 16 }, { NULL, 0, 0 }
};

static DrawTexture gRaceSelectionTTTexture[2] = { { NULL, -16, -16 }, { NULL, 0, 0 } };

// Highlighted option index for time trial in tracks mode.
//   0 = Time Trial off
//   1 = Time Trial on
static s32 gTracksMenuTimeTrialHighlightIndex = 0;

// Highlighted option index for adventure 1 or 2 in tracks mode.
//   0 = Adventure 1 (Normal)
//   1 = Adventure 2 (Mirrored)
static s32 gTracksMenuAdventureHighlightIndex = 0;

static DrawTexture *gRaceSelectionImages[9] = {
    gRaceSelectionCarTex,   gRaceSelectionCarOptHighlight,   gRaceSelectionCarOpt,
    gRaceSelectionHoverTex, gRaceSelectionHoverOptHighlight, gRaceSelectionHoverOpt,
    gRaceSelectionPlaneTex, gRaceSelectionPlaneOptHighlight, gRaceSelectionPlaneOpt

};

static DrawTexture *gTrackSelectTTImage[6] = { gRaceSelectionTTOff, gRaceSelectionTTOffOptHighlight, gRaceSelectionTTOffOpt,
                                        gRaceSelectionTTOn,  gRaceSelectionTTOnOptHighlight,  gRaceSelectionTTOnOpt };

static DrawTexture *gTrackSelectPlayerImage[6] = { gRaceSelectionPlayer1Texture,      gRaceSelectionPlayer2Texture,
                                            gRaceSelectionPlayer3Texture,      gRaceSelectionPlayer4Texture,
                                            gRaceSelectionVehicleTitleTexture, gRaceSelectionTTTitleTexture };

// X/Y offsets for the "Player" icon
static s16 gTracksMenuPlayerNamePositions[20] = {
    // X, Y
    0x44, 0x72,                                     // 1 player
    0x44, 0x72, 0xCC, 0x72,                         // 2 players
    0x21, 0x72, 0x88, 0x72, 0xEF, 0x72,             // 3 players
    0x21, 0x72, 0x66, 0x72, 0xAA, 0x72, 0xEF, 0x72, // 4 players
};

// X offsets for "Car", "Hover", and "Plane" choices in track select.
static s16 gTracksMenuVehicleNamePositions[10] = {
    0x68,                  // 1 player
    0x21, 0xFB,            // 2 players
    0x27, 0x8E, 0xF5,      // 3 players
    0x27, 0x6C, 0xB0, 0xF5 // 4 players
};

static ButtonTextElement gTwoPlayerRacerCountMenu = {
    SCREEN_WIDTH_HALF - 80, 140, 160, 64, 4, 4, { 80, 20, 58, 40, 80, 40, 102, 40 }
};

// Adventure 1/2 button in the track select menu.
// Note: The first element (.x) is not actually used.
static ButtonElement gTracksMenuAdventureButton = { 80, 152, 160, 40, 4, 4, 80, 14 };

static TextureHeader *gTracksMenuBgTextures[10] = {
    NULL, NULL, // Dino domain
    NULL, NULL, // Sherbet Island
    NULL, NULL, // Snowflake Mountain
    NULL, NULL, // Dragon Forest
    NULL, NULL  // Future Fun Land
};

static s16 gTrackSelectObjectIndices[14] = {
    0x05, 0x06, 0x07, 0x3D, 0x3C, 0x3F, 0x3E, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, -1
};

static s16 gTrackSelectImageIndices[4] = { 0x04, 0x05, 0x06, -1 };

static s16 gTrackSelectPreviewObjectIndices[36] = { 0x0004, 0x0000, 0x0001, 0x0002, 0x0003, 0x0018, 0x0019, 0x001A, 0x001B,
                                             0x001C, 0x001D, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A,
                                             0x002B, 0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023, 0x002C, 0x002D,
#if VERSION >= VERSION_79
                                             0x002E, 0x002F, 0x0030, 0x0031, 0x0040, 0x0041, 0x0043, 0x0069, 0xFFFF
#else
                                             0x002E, 0x002F, 0x0030, 0x0031, 0x0040, 0x0041, 0x0043, 0x005E, 0xFFFF
#endif
};

static s16 gTrackSelectPreviewImageIndices[8] = { 0x07, 0x00, 0x01, 0x02, 0x03, 0x0B, 0x0C, -1 };

// Not a struct, since the entries can *technically* be either 4 or 5 bytes. But it is always 5 in the final game.
/*
    Offset 0: u8 index;
    Offset 1: u8 indexInto800E0730;
    Offset 2: u8 alpha;
    Offset 3: u8 alpha2;
    Offset 4: u8 texCoordUFactor;
*/
static u8 gTrackSelectBgData[295] = {
    0,  0, 255, 255, 0,  1,  1, 255, 255, 0,  2,  0, 255, 255, 0,  3,   1, 255, 255, 0,  4,  0, 255, 255, 0,
    5,  1, 255, 255, 0,  6,  0, 255, 255, 0,  6,  2, 0,   64,  0,  7,   1, 255, 255, 0,  7,  3, 64,  128, 0,
    8,  0, 255, 255, 0,  8,  2, 128, 192, 32, 9,  1, 255, 255, 0,  9,   3, 192, 255, 32, 10, 2, 255, 255, 0,
    11, 3, 255, 255, 0,  12, 2, 255, 255, 32, 13, 3, 255, 255, 32, 14,  2, 255, 255, 0,  14, 4, 0,   64,  0,
    15, 3, 255, 255, 0,  15, 5, 64,  128, 0,  16, 2, 255, 255, 32, 16,  4, 128, 192, 0,  17, 3, 255, 255, 32,
    17, 5, 192, 255, 0,  18, 4, 255, 255, 0,  19, 5, 255, 255, 0,  20,  4, 255, 255, 0,  21, 5, 255, 255, 0,
    22, 4, 255, 255, 0,  22, 6, 0,   64,  0,  23, 5, 255, 255, 0,  23,  7, 64,  128, 0,  24, 4, 255, 255, 0,
    24, 6, 128, 192, 32, 25, 5, 255, 255, 0,  25, 7, 192, 255, 32, 26,  6, 255, 255, 0,  27, 7, 255, 255, 0,
    28, 6, 255, 255, 32, 29, 7, 255, 255, 32, 30, 6, 255, 255, 0,  30,  8, 0,   64,  0,  31, 7, 255, 255, 0,
    31, 9, 64,  128, 0,  32, 6, 255, 255, 32, 32, 8, 128, 192, 32, 33,  7, 255, 255, 32, 33, 9, 192, 255, 32,
    34, 8, 255, 255, 0,  35, 9, 255, 255, 0,  36, 8, 255, 255, 32, 37,  9, 255, 255, 32, 38, 8, 255, 255, 0,
    39, 9, 255, 255, 0,  40, 8, 255, 255, 32, 41, 9, 255, 255, 32, 255, 0, 0,   0,   0
};
static Vertex *gTrackSelectBgVertices[2] = { NULL, NULL };
static Triangle *gTrackSelectBgTriangles[2] = { NULL, NULL };
static char *gDefaultTrackName = "?";

// Boolean. Set to TRUE once in the Track Select menu, then
//   set to FALSE when leaving the menu.
// This is effectively unused, since it is only read from the
//   unused function trackmenu_active()
static s32 gIsInTracksMenu = FALSE;

static s16 gAdvTrackInitObjectIndices[18] = { 0x0004, 0x0000, 0x0001, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D,
#if VERSION >= VERSION_79
                                       0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023, 0x0030, 0x0069, -1
#else
                                       0x001E, 0x001F, 0x0020, 0x0021, 0x0022, 0x0023, 0x0030, 0x005E, -1
#endif
};

static s16 gAdvTrackInitImageIndices[6] = { 7, 0, 1, 2, 3, -1 };

static s32 gTrackSelectSound = FALSE;

/**
 * Set the current save data to use the furthest progress of all save files.
 */
void trackmenu_set_records(void) {
    s32 i;
    s32 numWorlds;
    s32 numLevels;
    Settings *settings;

    settings = get_settings();
    level_count(&numLevels, &numWorlds);

    for (i = 0; i < numLevels; i++) {
        settings->courseFlagsPtr[i] = RACE_UNATTEMPTED;
        for (numWorlds = 0; numWorlds < NUMBER_OF_SAVE_FILES; numWorlds++) {
            settings->courseFlagsPtr[i] |= gRaceSetup.saveFiles[numWorlds]->courseFlagsPtr[i];
        }
    }

    settings->trophies = 0;
    settings->keys = 0;
    settings->bosses = 0;
    settings->cutsceneFlags = 0;
    for (i = 0; i < NUMBER_OF_SAVE_FILES; i++) {
        settings->trophies |= gRaceSetup.saveFiles[i]->trophies;
        settings->keys |= gRaceSetup.saveFiles[i]->keys;
        settings->bosses |= gRaceSetup.saveFiles[i]->bosses;
        settings->cutsceneFlags |= gRaceSetup.saveFiles[i]->cutsceneFlags;
    }
}

/**
 * Set the texture IDs of the vehicle ID elements.
 */
void menu_init_vehicle_textures(void) {
    gRaceSelectionCarTex[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_CAR_TOP];
    gRaceSelectionCarTex[1].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_CAR_BOTTOM];
    gRaceSelectionHoverTex[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_HOVERCRAFT_TOP];
    gRaceSelectionHoverTex[1].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_HOVERCRAFT_BOTTOM];
    gRaceSelectionPlaneTex[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_PLANE_TOP];
    gRaceSelectionPlaneTex[1].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_PLANE_BOTTOM];
}

/**
 * Set the texture IDs of the menu arrow elements.
 */
void menu_init_arrow_textures(void) {
    gMenuSelectionArrowUp[0].texture = gMenuLoaded.assets[TEXTURE_ICON_ARROW_UP];
    gMenuSelectionArrowLeft[0].texture = gMenuLoaded.assets[TEXTURE_ICON_ARROW_LEFT];
    gMenuSelectionArrowDown[0].texture = gMenuLoaded.assets[TEXTURE_ICON_ARROW_DOWN];
    gMenuSelectionArrowRight[0].texture = gMenuLoaded.assets[TEXTURE_ICON_ARROW_RIGHT];
}

/**
 * Return whether the tracks menu is open.
 */
UNUSED s32 trackmenu_active(void) {
    return gIsInTracksMenu;
}

void menu_track_select_init(void) {
    s32 levelCount;
    s32 worldCount;
    s32 sp74;
    s32 i;
    s32 j;
    s32 var_a0;
    Settings *settings;
    s8 *trackIds;

    load_font(ASSET_FONTS_BIGFONT);
    settings = get_settings();
    level_count(&levelCount, &worldCount);
    trackIds = (s8 *) get_misc_asset(ASSET_MISC_TRACKS_MENU_IDS);
    if (gRaceSetup.titleScreenLoaded != FALSE) {
        gTrackSelectCursorX = 0;
        gTrackSelect.cursorY = 0;
        gTracksMenuTimeTrialHighlightIndex = 0;
        gTracksMenuAdventureHighlightIndex = 0;
        gRaceSetup.titleScreenLoaded = FALSE;
    }
    gTrackSelect.viewportX = fb_size();
    gTrackSelect.viewportY = GET_VIDEO_HEIGHT(gTrackSelect.viewportX) & 0xFFFF;
    gTrackSelect.viewportX = GET_VIDEO_WIDTH(gTrackSelect.viewportX);
    gTrackSelect.viewportHalfX = gTrackSelect.viewportX >> 1;
    gTrackSelect.viewportHalfY = gTrackSelect.viewportY >> 1;
    gTrackSelectX = (f32) gTrackSelectCursorX * 320.0f;
    gTrackSelectY = (f32) gTrackSelect.cursorY * (f32) -gTrackSelect.viewportY;
    gSelectedTrackX = -1;
    gSelectedTrackY = -1;
    gTrackmenuLoadedLevel = -1;
    gMenuFlow.opacityDecayTimer = 32;
    gMenuFlow.blinkTimer = 0;
    gTrackmenuType = TRACKMENU_TYPE_INIT;
    trackmenu_assets(0);
    transition_begin(&sMenuTransitionFadeOut);
    enable_new_screen_transitions();
    bgdraw_fillcolour(50, 105, 223);

    for (sp74 = 0; sp74 < 5; sp74++) {
        if (gTracksMenuBgTextureIndices[sp74 * 3] != -1) {
            menu_asset_load(gTracksMenuBgTextureIndices[sp74 * 3]);
            gTracksMenuBgTextures[sp74 << 1] = gMenuLoaded.assets[gTracksMenuBgTextureIndices[sp74 * 3]];
        } else {
            gTracksMenuBgTextures[sp74 << 1] = NULL;
        }
        if (gTracksMenuBgTextureIndices[sp74 * 3 + 1] != -1) {
            menu_asset_load(gTracksMenuBgTextureIndices[sp74 * 3 + 1]);
            gTracksMenuBgTextures[(sp74 << 1) + 1] = gMenuLoaded.assets[gTracksMenuBgTextureIndices[sp74 * 3 + 1]];
        } else {
            gTracksMenuBgTextures[(sp74 << 1) + 1] = NULL;
        }
    }

    i = 40;
    gTrackSelectBgTriangles[0] = mempool_alloc_safe(0xB40, COLOUR_TAG_YELLOW);
    gTrackSelectBgTriangles[1] = gTrackSelectBgTriangles[0] + i;
    gTrackSelectBgVertices[0] = (Vertex *) (gTrackSelectBgTriangles[1] + i);
    gTrackSelectBgVertices[1] = gTrackSelectBgVertices[0] + i * 2;

    var_a0 = -160;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 80; j++) {
            gTrackSelectBgVertices[i][j].x = var_a0;
            gTrackSelectBgVertices[i][j].z = -0x400;
            gTrackSelectBgVertices[i][j].r = 255;
            gTrackSelectBgVertices[i][j].g = 255;
            gTrackSelectBgVertices[i][j].b = 255;
            var_a0 = -var_a0;
        }
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 40; j++) {
            gTrackSelectBgTriangles[i][j].flags = BACKFACE_DRAW;
            gTrackSelectBgTriangles[i][j].vi0 = 0;
            gTrackSelectBgTriangles[i][j].vi1 = 2;
            gTrackSelectBgTriangles[i][j].vi2 = 1;
            j++;
            gTrackSelectBgTriangles[i][j].flags = BACKFACE_DRAW;
            gTrackSelectBgTriangles[i][j].vi0 = 1;
            gTrackSelectBgTriangles[i][j].vi1 = 2;
            gTrackSelectBgTriangles[i][j].vi2 = 3;
        }
    }
    gTrackSelectVertsFlip = 0;
    bgdraw_set_func(trackmenu_render_background);
    viewport_menu_set(0, 80, gTrackSelect.viewportHalfY - (gTrackSelect.viewportHalfY >> 1), SCREEN_HEIGHT,
                      (gTrackSelect.viewportHalfY >> 1) + gTrackSelect.viewportHalfY);
    copy_viewports_to_stack();
    camEnableUserView(0, FALSE);
    gIsInTracksMenu = TRUE;
    menu_assetgroup_load(gTrackSelectObjectIndices);
    menu_imagegroup_load(gTrackSelectImageIndices);
    menu_init_arrow_textures();
    sTrackMenuFrameTexturesA[0].texture = (TextureHeader *) gMenuLoaded.assets[TEXTURE_UNK_08];
    sTrackMenuFrameTexturesA[1].texture = (TextureHeader *) gMenuLoaded.assets[TEXTURE_UNK_09];
    sTrackMenuFrameTexturesA[2].texture = (TextureHeader *) gMenuLoaded.assets[TEXTURE_UNK_0A];
    sTrackMenuFrameTexturesB[0].texture = (TextureHeader *) gMenuLoaded.assets[TEXTURE_UNK_0B];
    sTrackMenuFrameTexturesB[1].texture = (TextureHeader *) gMenuLoaded.assets[TEXTURE_UNK_0C];
    sTrackMenuFrameTexturesB[2].texture = (TextureHeader *) gMenuLoaded.assets[TEXTURE_UNK_0D];

    for (i = 0; i < 5; i++) {
#define getTrackId(add) ((i * 6) + add)
        for (j = 0; j < 6; j++) {
            gTrackSelect.trackIds[i][j] = -1;
            if (j == 0 && i < 4) {
                gTrackSelect.trackIds[i][j] = trackIds[getTrackId(j)];
            } else if (j < 4) {
                if (trackIds[getTrackId(j)] != -1 &&
                    (settings->courseFlagsPtr[trackIds[getTrackId(j)]] & RACE_VISITED)) {
                    gTrackSelect.trackIds[i][j] = trackIds[getTrackId(j)];
                }
            } else if (j == 4) {
                for (var_a0 = 0, sp74 = 0; (var_a0 < 4) ^ 0; var_a0++) {
                    if (trackIds[getTrackId(var_a0)] != -1 &&
                        ((settings->courseFlagsPtr[trackIds[getTrackId(var_a0)]] &
                          (RACE_CLEARED | RACE_CLEARED_SILVER_COINS)) == (RACE_CLEARED | RACE_CLEARED_SILVER_COINS))) {
                        sp74++;
                    }
                }
                if (sp74 == 4 && i != 4) {
                    // Not Tricky 1 / 2
                    if ((settings->bosses & (0x82 << i)) != (0x82 << i)) {
                        sp74 = 0;
                    }
                }
                if (sp74 == 4) {
                    gTrackSelect.trackIds[i][j] = trackIds[getTrackId(j)];
                }
            } else if (j == 5 && (settings->keys & (1 << (i + 1)))) {
                gTrackSelect.trackIds[i][j] = trackIds[getTrackId(j)];
            }
            if (is_adventure_two_unlocked()) {
                gTrackSelect.trackIds[i][j] = trackIds[getTrackId(j)];
            }
        }
#undef getTrackId
    }

    gTrackSelect.previewTrackId = gTrackSelect.trackIds[gTrackSelect.cursorY][gTrackSelectCursorX];
    gTrackSelect.row = gTrackSelect.cursorY + 1;
    if (gTrackSelect.previewTrackId == -1) {
        gTrackmenuLoadedLevel = gTrackSelect.trackIds[0][0];
        load_level_for_menu(gTrackmenuLoadedLevel, -1, 1);
        gTrackSelect.row = 1;
        gSelectedTrackX = 0;
        gSelectedTrackY = 0;
    }
    dialogue_clear(7);
    menu_panels_alloc(2);
    gTrackTTSoundMask = NULL;
    sMenuSoundHandle = NULL;
    gTrackSelect.musicVolume = 0;
    music_voicelimit_set(24);
    music_voicelimit_change_off();
    music_play(SEQUENCE_MAIN_MENU);
    music_volume_set(gTrackSelect.musicVolume);
    music_change_off();
    set_gIntDisFlag(TRUE);
    gRaceSetup.isInAdventureTwo = gTracksMenuAdventureHighlightIndex;
    gMultiplayerSelectedNumberOfRacersCopy = gRaceSetup.multiplayerRacerCount;
}

/**
 * If type is -1, allocate and initialise all the track setup elements.
 * If type is 0, reset the cursor target to the current position.
 * If type is 1, free the assetgroup for the track setup.
 * If type is 2, to load the race
 */
void trackmenu_assets(s32 type) {
    Vehicle vehicle;
    s32 i;
    s32 newType;

    if (gTrackmenuType != TRACKMENU_TYPE_INIT && gTrackmenuType != TRACKMENU_TYPE_RESET_CURSOR &&
        gTrackmenuType == TRACKMENU_TYPE_FREE) {
        menu_assetgroup_free(gTrackSelectPreviewObjectIndices);
    }

    gTrackmenuType = type;
    newType = gTrackmenuType;

    if (newType > TRACKMENU_TYPE_INIT && newType <= TRACKMENU_TYPE_FREE) {
        switch (gTrackmenuType) {
            case TRACKMENU_TYPE_RESET_CURSOR:
                gTrackSelectTargetX = gTrackSelectX;
                gTrackSelectTargetY = gTrackSelectY;
                break;
            case TRACKMENU_TYPE_FREE:
                vehicle = leveltable_vehicle_default(gTrackSelect.previewTrackId);
                for (i = 0; i < gRaceSetup.numberOfActivePlayers; i++) {
                    gPlayerSelectConfirm[i] = 0;
                    gRaceSetup.playerVehicles[i] = vehicle;
                }
                gRaceSetup.numberOfReadyPlayers = 0;
                gTrackSelect.nameVoiceDelay = 1;
                menu_assetgroup_load(gTrackSelectPreviewObjectIndices);
                menu_imagegroup_load(gTrackSelectPreviewImageIndices);
                menu_init_vehicle_textures();
                gRaceSelectionTTOn[0].texture = gMenuLoaded.assets[TEXTURE_ICON_TIMETRIAL_ON_TOP];
                gRaceSelectionTTOn[1].texture = gMenuLoaded.assets[TEXTURE_ICON_TIMETRIAL_ON_BOTTOM];
                gRaceSelectionTTOff[0].texture = gMenuLoaded.assets[TEXTURE_ICON_TIMETRIAL_OFF_TOP];
                gRaceSelectionTTOff[1].texture = gMenuLoaded.assets[TEXTURE_ICON_TIMETRIAL_OFF_BOTTOM];
                gRaceSelectionCarOptHighlight[0].texture =
                    gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_CAR_HIGHLIGHT];
                gRaceSelectionCarOpt[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_CAR];
                gRaceSelectionHoverOptHighlight[0].texture =
                    gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_HOVERCRAFT_HIGHLIGHT];
                gRaceSelectionHoverOpt[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_HOVERCRAFT];
                gRaceSelectionPlaneOptHighlight[0].texture =
                    gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_PLANE_HIGHLIGHT];
                gRaceSelectionPlaneOpt[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_PLANE];
                gRaceSelectionTTOnOptHighlight[0].texture = gMenuLoaded.assets[TEXTURE_ICON_TIMETRIAL_OPT_ON];
                gRaceSelectionTTOffOptHighlight[0].texture =
                    gMenuLoaded.assets[TEXTURE_ICON_TIMETRIAL_OPT_ON_HIGHLIGHT];
                gRaceSelectionTTOnOpt[0].texture = gMenuLoaded.assets[TEXTURE_ICON_TIMETRIAL_OPT_OFF];
                gRaceSelectionTTOffOpt[0].texture = gMenuLoaded.assets[TEXTURE_ICON_TIMETRIAL_OPT_OFF_HIGHLIGHT];
                gRaceSelectionPlayer1Texture[0].texture = gMenuLoaded.assets[TEXTURE_ICON_PLAYER_1];
                gRaceSelectionPlayer2Texture[0].texture = gMenuLoaded.assets[TEXTURE_ICON_PLAYER_2];
                gRaceSelectionPlayer3Texture[0].texture = gMenuLoaded.assets[TEXTURE_ICON_PLAYER_3];
                gRaceSelectionPlayer4Texture[0].texture = gMenuLoaded.assets[TEXTURE_ICON_PLAYER_4];
                gRaceSelectionVehicleTitleTexture[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_TITLE];
                gRaceSelectionTTTitleTexture[0].texture = gMenuLoaded.assets[TEXTURE_ICON_TT_TITLE];
#if VERSION >= VERSION_79
                // Did the TEXTURE_ICON_TT_HEAD enum value change?
                gRaceSelectionTTTexture[0].texture = gMenuLoaded.assets[TEXTURE_UNK_69];
#else
                gRaceSelectionTTTexture[0].texture = gMenuLoaded.assets[TEXTURE_ICON_TT_HEAD];
#endif
                break;
        }
    }
    gMenuInput.delay = 0;
}

/**
 * Show the track selection menu, or the course and vehicle selection menu,
 * or the course info, depending on what should be visible.
 */
s32 menu_track_select_loop(s32 updateRate) {
    s32 cutsceneId;
    Settings *settings;

    settings = get_settings();
    gMenuFlow.blinkTimer = (gMenuFlow.blinkTimer + updateRate) & 0x3F;
    if (bgload_active() == FALSE && gMenuInput.delay != 0) {
        if (gMenuInput.delay < 0) {
            gMenuInput.delay -= updateRate;
        } else {
            gMenuInput.delay += updateRate;
        }
    }
    menu_input();

    // Make sure adventure 2 view doesn't cause problems later on.
    gSPClearGeometryMode(gMenuFrame.displayList++, G_CULL_FRONT);

    switch (gTrackmenuType) {
        case TRACKMENU_TYPE_RESET_CURSOR:
            trackmenu_render_names(updateRate);
            trackmenu_track_view(updateRate);
            trackmenu_input(updateRate);
            break;
        case TRACKMENU_TYPE_FREE:
            trackmenu_timetrial_sound(updateRate);
            trackmenu_setup_render(updateRate);
            trackmenu_update_vehicle_select(updateRate);
            break;
    }

    if (gTrackSelect.musicVolume < 0) {
        music_volume_set(0);
    } else {
        gTrackSelect.musicVolume += updateRate * 2;
        if (gTrackSelect.musicVolume >= 81) {
            gTrackSelect.musicVolume = 80;
        }
        music_volume_set(gTrackSelect.musicVolume);
    }
    if (gTrackmenuType <= TRACKMENU_TYPE_INIT) {
        menu_track_select_unload();
        gRaceSetup.trackSpecified = 0;
        if (gRaceSetup.numberOfActivePlayers >= 3 ||
            (gRaceSetup.numberOfActivePlayers == 2 && !(gRaceSetup.activeMagicCodes & CHEAT_TWO_PLAYER_ADVENTURE))) {
            cutsceneId = 0;
            if (is_drumstick_unlocked()) {
                cutsceneId = 1;
            }
            if (is_tt_unlocked()) {
                cutsceneId ^= 3;
            }
            load_level_for_menu(ASSET_LEVEL_CHARACTERSELECT, -1, cutsceneId);
            charselect_prev(0, NULL);
            menu_init(MENU_CHARACTER_SELECT);
            return MENU_RESULT_CONTINUE;
        }
        load_level_for_menu(ASSET_LEVEL_OPTIONSBACKGROUND, -1, 0);
        menu_init(MENU_GAME_SELECT);
        return MENU_RESULT_CONTINUE;
    }
    if (gTrackmenuType >= TRACKMENU_TYPE_LOAD_LEVEL) {
        menu_track_select_unload();
        if (gMultiplayerSelectedNumberOfRacersCopy != gRaceSetup.multiplayerRacerCount) {
            for (cutsceneId = 0; cutsceneId < 8; cutsceneId++) {
                settings->racers[cutsceneId].starting_position = cutsceneId;
            }
        }
        if (gTrackSelectCursorX != 4) {
            gRaceSetup.trackSpecified = 1;
            return gRaceSetup.numberOfActivePlayers;
        }
        gRaceSetup.trophyRaceWorldId = gTrackSelect.cursorY + 1;
        gRaceSetup.inAdvModeTrophyRace = FALSE;
        gRaceSetup.trophyRaceRound = 0;
        menu_init(MENU_TROPHY_RACE_ROUND);
        return MENU_RESULT_CONTINUE;
    }
    gMenuInput.ignoreInputTime = 0;
    return MENU_RESULT_CONTINUE;
}

/**
 * Unload all assets associated with the track select menu.
 */
void menu_track_select_unload(void) {
    s32 i;

    camDisableUserView(0, FALSE);
    menu_assetgroup_free(gTrackSelectObjectIndices);
    mempool_free_timer(0);
    mempool_free(gTrackSelectBgTriangles[0]);
    mempool_free_timer(2);
    for (i = 0; i < 15; i += 3) {
        if (gTracksMenuBgTextureIndices[i] != -1) {
            menu_asset_free(gTracksMenuBgTextureIndices[i]);
        }
        if (gTracksMenuBgTextureIndices[i + 1] != -1) {
            menu_asset_free(gTracksMenuBgTextureIndices[i + 1]);
        }
    }
    unload_font(ASSET_FONTS_BIGFONT);
    menu_button_free();
    music_change_on();
    music_voicelimit_change_on();
    music_stop();
    set_gIntDisFlag(FALSE);
}

s32 trackmenu_render_background(Gfx **dList, Mtx **mtx) {
    s32 sp7C;
    s32 yPos;
    s32 texU;
    s32 texV;
    s32 flags;
    s32 hasTexture;
    s32 index;
    s32 prevAlpha;
    s32 prevIndex;
    s32 numVertices;
    s32 temp;
    s32 temp2;
    Vertex *tempVertices;
    Triangle *triangles;
    Triangle *tempTriangles;
    Vertex *vertices;
    TextureHeader *bgTexture;
    s32 curIndex;

    numVertices = 0;
    camDisableUserView(0, TRUE);
    camera_init_tracks_menu(dList, mtx);
    mtx_ortho(dList, mtx);
    // These vertices are the full-screen patterned backdrop, not an object or HUD layer.
    pc_gfx_mtx_tag((*mtx) - 1, gTrackSelectBgVertices, PC_MTX_SITE_WIDE_SURFACE);
    rendermode_reset(dList);
    gDPPipeSync((*dList)++);
    sp7C = gTrackSelectX;
    temp2 = -gTrackSelectY;
    curIndex = temp2 / (gTrackSelect.viewportY >> 3);
    if (curIndex >= 42) {
        return 1;
    }
    temp2 -= (curIndex * (gTrackSelect.viewportY >> 3));
    yPos = gTrackSelect.viewportHalfY + temp2;
    gDPSetPrimColor((*dList)++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor((*dList)++, 255, 255, 255, 0);
    vertices = gTrackSelectBgVertices[gTrackSelectVertsFlip];
    triangles = gTrackSelectBgTriangles[gTrackSelectVertsFlip];

    for (index = 0; gTrackSelectBgData[index] < curIndex; index += 5) {}

    while ((yPos >= -gTrackSelect.viewportHalfY) && (gTrackSelectBgData[index] < 42) && (numVertices < 64)) {
        bgTexture = gTracksMenuBgTextures[gTrackSelectBgData[index + 1]];
        curIndex = gTrackSelectBgData[index];
        tempVertices = vertices;
        // @fake
        vertices++;
        vertices--;
        tempTriangles = triangles;
        vertices[0].y = yPos;
        vertices[0].a = gTrackSelectBgData[index + 2];
        vertices[1].y = yPos;
        vertices[1].a = gTrackSelectBgData[index + 2];
        prevIndex = gTrackSelectBgData[index + 2];
        vertices[2].y = yPos - (gTrackSelect.viewportY >> 3);
        vertices[2].a = gTrackSelectBgData[index + 3];
        vertices[3].y = yPos - (gTrackSelect.viewportY >> 3);
        vertices[3].a = gTrackSelectBgData[index + 3];
        vertices += 4;
        prevIndex += gTrackSelectBgData[index + 3];
        index += 4;
        if (bgTexture != NULL) {
            texU = ((bgTexture->width - 1) & sp7C) << 5;
            texU += (gTrackSelectBgData[index] << 5);
            index++;
            texV = bgTexture->height << 5;
        }
#ifdef AVOID_UB
        else {
            // texU and texV aren't set if bgTexture is NULL
            texU = 0;
            texV = 0;
        }
#endif
        triangles[0].uv0.u = texU;
        triangles[0].uv0.v = 0;
        triangles[0].uv1.u = texU;
        triangles[0].uv1.v = texV;
        triangles[0].uv2.u = texU + 0x2800;
        triangles[0].uv2.v = 0;
        triangles[1].uv0.u = texU + 0x2800;
        triangles[1].uv0.v = 0;
        triangles[1].uv1.u = texU;
        triangles[1].uv1.v = texV;
        triangles[1].uv2.u = texU + 0x2800;
        triangles[1].uv2.v = texV;
        triangles += 2;

        if (bgTexture != 0) {
            hasTexture = 1;
        } else {
            hasTexture = 0;
        }

        // If both alphas are 255, then don't set the transparency flags.
        if (prevIndex == 255 + 255) {
            flags = 0;
        } else {
            flags = RENDER_VTX_ALPHA | RENDER_Z_UPDATE;
        }

        material_set(dList, bgTexture, flags, 0);
        gSPVertexDKR((*dList)++, OS_K0_TO_PHYSICAL(tempVertices), 4, 0);
        gSPPolygon((*dList)++, OS_K0_TO_PHYSICAL(tempTriangles), 2, hasTexture);
        prevIndex = gTrackSelectBgData[index];
        if (curIndex != prevIndex) {
            yPos -= (gTrackSelect.viewportY >> 3);
        }
        numVertices += 4;
    }

    camEnableUserView(0, TRUE);
    return 0;
}

/**
 * Render the names of the worlds and tracks in the track selection menu.
 */
void trackmenu_render_names(UNUSED s32 updateRate) {
    s32 i;
    UNUSED s32 pad[3];
    s32 trackSelectX;
    s32 trackSelectY;
    char *levelName;
    s32 maxTrackY;
    s8 *trackMenuIds;
    Settings *settings;
    UNUSED s32 pad1;
    s32 trackX;
    s32 trackY;
    s32 j;
    s32 startIndex;
    UNUSED s32 pad2;
    s32 k;

    settings = get_settings();
    trackMenuIds = (s8 *) get_misc_asset(ASSET_MISC_TRACKS_MENU_IDS);
    if (gMenuInput.delay <= -23 || gMenuInput.delay >= 23) {
        return;
    }

    if (gTrackSelect.fflUnlocked == -1) {
        maxTrackY = 3;
    } else {
        maxTrackY = 4;
    }
    trackSelectX = (gTrackSelectX / 320);
    trackY = (gTrackSelectY / -gTrackSelect.viewportY);
    trackSelectY = trackY - 1;
    startIndex = -1;
    k = 0;
    for (i = startIndex; i < 2; i++) {
        trackY = trackSelectY + i + 1;
        for (j = -1; j < 2; j++, k++) {
            trackX = trackSelectX + j;
            if (trackY < 0 || maxTrackY < trackY || trackX < 0 || trackX >= 6) {
                gTrackSelectRenderDetails[k].visible = 0;
            } else {
                gTrackSelectRenderDetails[k].visible = 1;
                levelName = level_name(level_world_id(trackY + 1));
                gTrackSelectRenderDetails[k].hubName = levelName;
                if (gTrackSelect.trackIds[trackY][trackX] != -1) {
                    gTrackSelectRenderDetails[k].trackName = level_name(trackMenuIds[((trackY * 6) + trackX)]);
                    if (trackX == 4) {
                        if ((((settings->trophies) >> (trackY * 2)) & 3) == 3) {
                            gTrackSelectRenderDetails[k].visible = 2;
                        }
                    } else if ((settings->courseFlagsPtr[trackMenuIds[((trackY * 6) + trackX)]] & 2)) {
                        gTrackSelectRenderDetails[k].visible = 2;
                    }
                } else {
                    gTrackSelectRenderDetails[k].trackName = gDefaultTrackName;
                }
                gTrackSelectRenderDetails[k].xOff = ((trackX * 320) - gTrackSelectX);
                gTrackSelectRenderDetails[k].yOff = ((-trackY * gTrackSelect.viewportY) - gTrackSelectY);
                gTrackSelectRenderDetails[k].opacity = 0xFF;
                if ((trackX == gSelectedTrackX) && (trackY == gSelectedTrackY)) {
                    gTrackSelectRenderDetails[k].vp1 = 1;
                    if (gMenuFlow.opacityDecayTimer < 32) {
                        gTrackSelectRenderDetails[k].opacity = gMenuFlow.opacityDecayTimer * 8;
                    }
                } else {
                    gTrackSelectRenderDetails[k].vp1 = 0;
                }
                gTrackSelectRenderDetails[k].vp2 = 0;
                if (gMenuInput.delay == 0) {
                    if (trackY > 0) {
                        gTrackSelectRenderDetails[k].vp2 |= 1;
                    }
                    if (trackX < 5) {
                        gTrackSelectRenderDetails[k].vp2 |= 2;
                    }
                    if (trackY < maxTrackY) {
                        gTrackSelectRenderDetails[k].vp2 |= 4;
                    }
                    if (trackX > 0) {
                        gTrackSelectRenderDetails[k].vp2 |= 8;
                    }
                    if (trackX == 4 && trackY == 4) {
                        gTrackSelectRenderDetails[k].vp2 &= ~2;
                    }
                    if (trackX == 5 && trackY == 3) {
                        gTrackSelectRenderDetails[k].vp2 &= ~4;
                    }
                }
                if (trackX == 4) {
                    gTrackSelectRenderDetails[k].border = 6;
                } else if (trackX == 5) {
                    gTrackSelectRenderDetails[k].border = 5;
                } else {
                    gTrackSelectRenderDetails[k].border = 4;
                }
            }
        }
    }
    camDisableUserView(0, TRUE);
    menu_camera_centre();
    mtx_ortho(&gMenuFrame.displayList, &gMenuFrame.hudMat);
    rendermode_reset(&gMenuFrame.displayList);
    gDPPipeSync(gMenuFrame.displayList++);
    gTrackSelect.hubName = NULL;
    for (i = 0; i < ARRAY_COUNT(gTrackSelectRenderDetails); i++) {
        if (gTrackSelectRenderDetails[i].visible != 0) {
            trackmenu_render_2D(gTrackSelectRenderDetails[i].xOff, gTrackSelectRenderDetails[i].yOff,
                                gTrackSelectRenderDetails[i].hubName, gTrackSelectRenderDetails[i].trackName,
                                gTrackSelectRenderDetails[i].opacity, gTrackSelectRenderDetails[i].border,
                                gTrackSelectRenderDetails[i].vp1,
                                (gTrackSelectRenderDetails[i].visible == 1) ? sTrackMenuFrameTexturesA : sTrackMenuFrameTexturesB,
                                gTrackSelectRenderDetails[i].vp2);
        }
    }
    gTrackSelectVertsFlip = 1 - gTrackSelectVertsFlip;
}

/**
 * If the track hasn't been loaded already, tell thread30 to load the track in the background.
 * Also move the viewports and cursor towards the target.
 */
void trackmenu_track_view(s32 updateRate) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;
    s32 i;

    bgload_tick();
    for (i = 0; i < updateRate; i++) {
        gTrackSelectX += (gTrackSelectTargetX - gTrackSelectX) * 0.1;
        gTrackSelectY += (gTrackSelectTargetY - gTrackSelectY) * 0.1;
    }
    if (gMenuFlow.opacityDecayTimer == 32 && bgload_active() == FALSE) {
        if (gTrackSelect.previewTrackId == gTrackmenuLoadedLevel) {
            gSelectedTrackX = gTrackSelectCursorX;
            gSelectedTrackY = gTrackSelect.cursorY;
        } else if (gTrackSelect.previewTrackId != gTrackmenuLoadedLevel && gTrackSelect.previewTrackId != -1 &&
                   bgload_start(gTrackSelect.previewTrackId, 1)) {
            gTrackmenuLoadedLevel = gTrackSelect.previewTrackId;
            gSelectedTrackX = gTrackSelectCursorX;
            gSelectedTrackY = gTrackSelect.cursorY;
            set_level_default_vehicle(leveltable_vehicle_default(gTrackSelect.previewTrackId));
        }
    }
    x1 = ((gSelectedTrackX * SCREEN_WIDTH) - gTrackSelectX) + SCREEN_WIDTH_FLOAT_HALF - 80.0f;
    x2 = x1 + SCREEN_WIDTH_HALF;
    y1 = gTrackSelect.viewportHalfY - ((gSelectedTrackY * -gTrackSelect.viewportY) - gTrackSelectY) -
         (gTrackSelect.viewportY >> 2);
    y2 = y1 + gTrackSelect.viewportHalfY;
    viewport_menu_set(0, x1, y1, x2, y2);
    copy_viewport_background_size_to_coords(0, &x1, &y1, &x2, &y2);
    camEnableUserView(0, FALSE);
}

/**
 * Read the menu inputs and move the track menu cursor to where the players set it.
 * If a player presses A, then scale the viewport up, provided the course has finished loading.
 */
void trackmenu_input(s32 updateRate) {
    UNUSED s32 pad1[2];
    s32 scaleOffset;
    UNUSED s32 pad2[2];
    s32 spaceWorldUnlocked;
    s32 centreY;
    s32 centreX;
    s32 menuDelay;
    s32 y2;
    s32 y1;

    menuDelay = gMenuInput.delay;
    if (gMenuInput.delay > 0) {
        if (gTrackSelectTargetX - gTrackSelectX > 4.0f || gTrackSelectTargetX - gTrackSelectX < -4.0f ||
            gTrackSelectTargetY - gTrackSelectY > 4.0f || gTrackSelectTargetY - gTrackSelectY < -4.0f) {
            gMenuInput.delay = 1;
        } else if (gTrackSelectSound) {
            sound_play(SOUND_SELECT2, NULL);
            gTrackSelectSound = FALSE;
        }
        scaleOffset = gMenuInput.delay - 1;
        if (scaleOffset > 20) {
            scaleOffset = 20;
        }
        centreX = 160;
        centreY = gTrackSelect.viewportHalfY;
        if (scaleOffset < 20) {
            centreX += gTrackSelectTargetX - gTrackSelectX;
            centreY -= gTrackSelectTargetY - gTrackSelectY;
        }
        y2 = (((scaleOffset + 20) * gTrackSelect.viewportHalfY) / 40) + centreY;
        y1 = centreY - (((scaleOffset + 20) * gTrackSelect.viewportHalfY) / 40);
        viewport_menu_set(0, (centreX - (scaleOffset * 4)) - 80, y1, (scaleOffset * 4) + centreX + 80, y2);
        gMenuLoaded.images[4].trans.scale =
            (f32) (sMenuImageProperties[4].trans.scale * (1.0f + ((f32) scaleOffset / 20.0f)));
        gMenuLoaded.images[6].trans.scale =
            (f32) (sMenuImageProperties[6].trans.scale * (1.0f + ((f32) scaleOffset / 20.0f)));
        gMenuLoaded.images[5].trans.scale =
            (f32) (sMenuImageProperties[5].trans.scale * (1.0f + ((f32) scaleOffset / 20.0f)));
    }
    camEnableUserView(0, FALSE);
    if (bgload_active() == FALSE) {
        if (gMenuInput.delay < 0) {
            gTrackSelect.musicVolume -= updateRate * 4;
        }
        if (gSelectedTrackX == gTrackSelectCursorX && gSelectedTrackY == gTrackSelect.cursorY) {
            gMenuFlow.opacityDecayTimer -= updateRate;
            if (gMenuFlow.opacityDecayTimer < 0) {
                gMenuFlow.opacityDecayTimer = 0;
            }
        } else {
            gMenuFlow.opacityDecayTimer = gMenuFlow.opacityDecayTimer + updateRate;
            if (gMenuFlow.opacityDecayTimer > 32) {
                gMenuFlow.opacityDecayTimer = 32;
            }
        }
        if (gMenuInput.delay < -22) {
            bgdraw_set_func(NULL);
            gIsInTracksMenu = FALSE;
        }
        if (gMenuInput.delay > 30) {
            if (is_adventure_two_unlocked() && gTrackSelectCursorX != 5) {
                gMenuFlow.stage = TRACKMENU_MIRROR;
            } else {
                gMenuFlow.stage = TRACKMENU_OPT_1;
            }
            trackmenu_assets(TRACKMENU_TYPE_FREE);
        } else if (gMenuInput.delay < -30) {
            disable_new_screen_transitions();
            camDisableUserView(0, FALSE);
            trackmenu_assets(TRACKMENU_TYPE_INIT);
        }
    }
    if (menuDelay == 0) {
        spaceWorldUnlocked = gTrackSelect.fflUnlocked == -1 ? 3 : 4;
        if (gMenuInput.buttons[PLAYER_MENU] & (A_BUTTON | START_BUTTON)) {
            if (gTrackSelect.previewTrackId != -1) {
                gMenuInput.delay = 1;
                gRaceSetup.trackIdToLoad = gTrackSelect.previewTrackId;
                gTrackSelectSound = TRUE;
            } else {
                sound_play(SOUND_UNK_6A, NULL);
            }
        } else if (gMenuInput.buttons[PLAYER_MENU] & B_BUTTON) {
            disable_new_screen_transitions();
            transition_begin(&sMenuTransitionFadeIn);
            enable_new_screen_transitions();
            gMenuInput.delay = -1;
        } else {
            s32 prevValue = gTrackSelectCursorX;
            s32 prevValue2 = gTrackSelect.cursorY;
            if (gMenuInput.stickX[PLAYER_MENU] < 0 && gTrackSelectCursorX > 0) {
                gTrackSelectCursorX--;
            }
            if (gMenuInput.stickX[PLAYER_MENU] > 0 && gTrackSelectCursorX < 5) {
                gTrackSelectCursorX++;
            }
            if (gTrackSelect.cursorY == 4 && gTrackSelectCursorX == 5) {
                gTrackSelectCursorX = 4;
            }
            if (gMenuInput.stickX[PLAYER_MENU] == 0) {
                // Don't allow scrolling down to Future Fun Land if it hasn't been unlocked.
                if (gMenuInput.stickY[PLAYER_MENU] < 0 && gTrackSelect.cursorY < spaceWorldUnlocked) {
                    gTrackSelect.cursorY++;
                }
                if (gMenuInput.stickY[PLAYER_MENU] > 0 && gTrackSelect.cursorY > 0) {
                    gTrackSelect.cursorY--;
                }
                if (gTrackSelectCursorX == 5 && gTrackSelect.cursorY == 4) {
                    gTrackSelect.cursorY = 3;
                }
            }
            if ((prevValue != gTrackSelectCursorX) || (gTrackSelect.cursorY != prevValue2)) {
                sound_play(SOUND_MENU_PICK2, NULL);
                gTrackSelect.previewTrackId = gTrackSelect.trackIds[gTrackSelect.cursorY][gTrackSelectCursorX];
                gTrackSelect.row = gTrackSelect.cursorY + 1;
                gTrackSelectTargetX = (gTrackSelectCursorX * 320);
                gTrackSelectTargetY = -(gTrackSelect.cursorY * gTrackSelect.viewportY);
            }
        }
    }
}

/**
 * If the time trial option is off when prompted, loop the snoring sound.
 */
void trackmenu_timetrial_sound(UNUSED s32 updateRate) {
    if (gMenuFlow.stage == TRACKMENU_CHOOSE && gTracksMenuTimeTrialHighlightIndex == 0 && gTrackTTSoundMask == NULL) {
        sound_play(SOUND_VOICE_TT_SNORE, &gTrackTTSoundMask);
    }
}

/**
 * Render the player options before going into a race.
 * This includes which adventure mode, which vehicles,
 * racer count and whether to use time trial mode.
 */
void trackmenu_setup_render(UNUSED s32 updateRate) {
    // Had to mess around with shifting the local variables, so they all probably need to be renamed.
    s32 k;
    s32 y;
    s32 temp2;
    s32 temp;
    s32 sp84;
    s32 regionOffset;          // sp80
    s32 availableVehicleFlags; // sp7C
    union {
        char buffer[4];
        s32 word;
    } filename; // sp78
    s32 sp74;
    Settings *settings; // sp70
    s32 i;
    s32 j;

    regionOffset = 0;
    filename.word = 0; // Set all 4 bytes as '\0'?
    sp74 = FALSE;
    settings = get_settings();
    if (osTvType == OS_TV_TYPE_PAL) {
        regionOffset = 12;
    }
    if (gRaceSetup.numberOfActivePlayers == 2 && gTrackSelectCursorX < 4 && gMenuFlow.stage > TRACKMENU_CHOOSE) {
        sp74 = TRUE;
    }
    camDisableUserView(0, TRUE);
    menu_camera_centre();
    mtx_ortho(&gMenuFrame.displayList, &gMenuFrame.hudMat);
    if (gMenuInput.delay < 0) {
        if (gSelectedTrackX == 4) {
            sp84 = 6;
        } else if (gSelectedTrackX == 5) {
            sp84 = 5;
        } else {
            sp84 = 4;
        }
        gMenuLoaded.images[sp84].trans.x_position = 0.0f;
        gMenuLoaded.images[sp84].trans.y_position = 0.0f;
        if (osTvType == OS_TV_TYPE_PAL) {
            gTrackSelect.woodFrameHeightScale = 1.2f;
        }
        menu_element_render(sp84);
        gTrackSelect.woodFrameHeightScale = 1.0f;
    }
    if (gMenuInput.delay >= -22 && gMenuInput.delay <= 30) {
        gMenuFrame.guiOpacity = (gMenuInput.delay < 0) ? 255 + (gMenuInput.delay * 16) : 255;
        if (gMenuFrame.guiOpacity < 0) {
            gMenuFrame.guiOpacity = 0;
        }
        availableVehicleFlags = leveltable_vehicle_usable(gTrackSelect.previewTrackId);
#if VERSION >= VERSION_79
        if (gRaceSetup.numberOfActivePlayers >= 2) {
            if (gTrackSelect.previewTrackId == ASSET_LEVEL_SPACEPORTALPHA) {
                availableVehicleFlags &= ~(1 << VEHICLE_HOVERCRAFT);
            }
            if (gTrackSelect.previewTrackId == ASSET_LEVEL_FROSTYVILLAGE) {
                availableVehicleFlags &= ~(1 << VEHICLE_PLANE);
            }
        }
#endif
        set_text_font(ASSET_FONTS_BIGFONT);
        set_text_colour(192, 192, 255, 0, gMenuFrame.guiOpacity);
        set_text_background_colour(0, 0, 0, 0);
        draw_text(&gMenuFrame.displayList, POS_CENTRED, 43, level_name(gTrackSelect.previewTrackId),
                  ALIGN_MIDDLE_CENTER);
        sp84 = gMenuFlow.blinkTimer * 8;
        if (sp84 > 255) {
            sp84 = 511 - sp84;
        }
        set_current_dialogue_background_colour(7, 255, sp84, 0, gMenuFrame.guiOpacity);
        if (gMenuFlow.stage == TRACKMENU_MIRROR ||
            (gMenuFlow.stage == TRACKMENU_OPT_2 && gTrackSelectCursorX == 4 && is_adventure_two_unlocked())) {
            temp2 = get_text_width(gMenuLoaded.text[ASSET_MENU_TEXT_ADVENTURE2], 0, 0); // "ADVENTURE"
            k = get_text_width(gMenuLoaded.text[ASSET_MENU_TEXT_ADVENTURETWO2], 0, 0);  // "ADVENTURE TWO"
            if (temp2 < k) {
                temp2 = k;
            }
            if (temp2 < gTracksMenuAdventureButton.width) {
                temp2 = gTracksMenuAdventureButton.width;
            } else {
                temp2 += 12;
            }
            menu_panel_draw_textured(&gMenuFrame.displayList, -(temp2 >> 1),
                                     SCREEN_HEIGHT_HALF - gTracksMenuAdventureButton.y, temp2,
                                     gTracksMenuAdventureButton.height, gTracksMenuAdventureButton.borderWidth,
                                     gTracksMenuAdventureButton.borderHeight,
                                     gMenuFrame.guiOpacity + COLOUR_RGBA32(176, 224, 192, 0), gMenuLoaded.assets[67]);
            menu_geometry_end();
            set_text_font(ASSET_FONTS_FUNFONT);
            for (temp = 0, temp2 = 0,
                y = gTracksMenuAdventureButton.colourMax + gTracksMenuAdventureButton.y + regionOffset + 1;
                 temp2 < 2; temp2++, temp++, y += 16) {
                for (j = 0, i = y; j < 4; j += 2, i -= 2) {
                    if (j == 0) {
                        set_text_colour(0, 0, 0, 255, gMenuFrame.guiOpacity >> 1);
                    } else {
                        k = 0;
                        if (temp2 == gTracksMenuAdventureHighlightIndex) {
                            k = sp84;
                        }
                        set_text_colour(255, 255, 255, k, gMenuFrame.guiOpacity);
                    }
                    // gMenuLoaded.text[146] = "ADVENTURE", gMenuLoaded.text[147] = "ADVENTURE TWO"
                    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 1 - j, i,
                              gMenuLoaded.text[ASSET_MENU_TEXT_ADVENTURE2 + temp], ALIGN_MIDDLE_CENTER);
                }
            }
        }
        if (gTrackSelectCursorX < 4) {
            set_text_font(ASSET_FONTS_FUNFONT);
            set_text_colour(255, 64, 64, 96, gMenuFrame.guiOpacity);
            draw_text(&gMenuFrame.displayList, 56, 72 + regionOffset, gMenuLoaded.text[ASSET_MENU_TEXT_BESTTIME],
                      ALIGN_MIDDLE_LEFT); // "BEST TIME"
            draw_text(&gMenuFrame.displayList, 56, 92 + regionOffset, gMenuLoaded.text[ASSET_MENU_TEXT_BESTLAP],
                      ALIGN_MIDDLE_LEFT); // "BEST LAP"
            set_text_colour(255, 128, 255, 96, gMenuFrame.guiOpacity);
            filename_decompress(settings->courseInitialsPtr[gRaceSetup.playerVehicles[0]][gTrackSelect.previewTrackId],
                                filename.buffer, MAX_INITIALS_LENGTH);
            draw_text(&gMenuFrame.displayList, 250, regionOffset + 72, filename.buffer, ALIGN_MIDDLE_CENTER);
            filename_decompress(settings->flapInitialsPtr[gRaceSetup.playerVehicles[0]][gTrackSelect.previewTrackId],
                                filename.buffer, MAX_INITIALS_LENGTH);
            draw_text(&gMenuFrame.displayList, 250, regionOffset + 92, filename.buffer, ALIGN_MIDDLE_CENTER);
            menu_timestamp_render(
                settings->courseTimesPtr[gRaceSetup.playerVehicles[PLAYER_ONE]][gTrackSelect.previewTrackId], 22, 53,
                128, 255, 255, FONT_COLOURFUL);
            menu_timestamp_render(
                settings->flapTimesPtr[gRaceSetup.playerVehicles[PLAYER_ONE]][gTrackSelect.previewTrackId], 22, 33, 255,
                192, 255, FONT_COLOURFUL);
            if (gMenuFlow.stage != TRACKMENU_MIRROR) {
                if (gRaceSetup.numberOfActivePlayers == 1) {
                    set_current_dialogue_box_coords(7, 134, regionOffset + 112, 186, regionOffset + 137);
                    render_dialogue_box(&gMenuFrame.displayList, NULL, NULL, 7);
                    if (gMenuFlow.stage <= TRACKMENU_OPT_1) {
                        texrect_draw(&gMenuFrame.displayList, gRaceSelectionVehicleTitleTexture, 136,
                                     regionOffset + 114, 255, 255, 255, gMenuFrame.guiOpacity);
                    } else {
                        texrect_draw(&gMenuFrame.displayList, gRaceSelectionTTTitleTexture, 136, regionOffset + 114,
                                     255, 255, 255, gMenuFrame.guiOpacity);
                        for (k = 0, y = regionOffset + 151; k < 2; k++, y += 24) {
                            if (k == gTracksMenuTimeTrialHighlightIndex) {
                                texrect_draw(&gMenuFrame.displayList, gTrackSelectTTImage[k * 3 + 1], 104, y, 255, 255,
                                             255, gMenuFrame.guiOpacity);
                            } else {
                                texrect_draw(&gMenuFrame.displayList, gTrackSelectTTImage[k * 3 + 2], 104, y, 255, 255,
                                             255, gMenuFrame.guiOpacity);
                            }
                        }
                    }
                } else if (!sp74) {
                    // Probably fake
                    y = gRaceSetup.numberOfActivePlayers;
                    j = ((y - 1) * y);
                    for (k = 0; (k < y) ^ 0; k++) {
                        if ((gRaceSetup.numberOfActivePlayers == 1 && k == gMenuFlow.stage) ||
                            (gRaceSetup.numberOfActivePlayers > 1 && gPlayerSelectConfirm[k] == 0)) {
                            set_current_dialogue_box_coords(7, gTracksMenuPlayerNamePositions[j] - 2,
                                                            gTracksMenuPlayerNamePositions[j + 1] + regionOffset - 2,
                                                            gTracksMenuPlayerNamePositions[j] + 50,
                                                            gTracksMenuPlayerNamePositions[j + 1] + regionOffset + 23);
                            render_dialogue_box(&gMenuFrame.displayList, NULL, NULL, 7);
                        }
                        texrect_draw(
                            &gMenuFrame.displayList, gTrackSelectPlayerImage[k], gTracksMenuPlayerNamePositions[j],
                            gTracksMenuPlayerNamePositions[j + 1] + regionOffset, 255, 255, 255, gMenuFrame.guiOpacity);
                        j += 2;
                    }
                }
                if ((gRaceSetup.numberOfActivePlayers > 1 || gMenuFlow.stage == TRACKMENU_OPT_1) && !sp74) {
                    // Draws "Car", "Hover", "Plane" text choice images.
                    j = ((gRaceSetup.numberOfActivePlayers - 1) * gRaceSetup.numberOfActivePlayers) >> 1;
                    for (k = 0, y = regionOffset + 139; k < 3; k++) {
                        if ((1 << k) & availableVehicleFlags) {
                            for (temp2 = 0; temp2 < gRaceSetup.numberOfActivePlayers; temp2++) {
                                if (k == gRaceSetup.playerVehicles[temp2]) {
                                    // Highlighted
                                    texrect_draw(&gMenuFrame.displayList, gRaceSelectionImages[(k * 3) + 1],
                                                 gTracksMenuVehicleNamePositions[j + temp2], y, 255, 255, 255,
                                                 gMenuFrame.guiOpacity);
                                } else if (settings->courseFlagsPtr[gTrackSelect.previewTrackId] & 2) {
                                    // Not highlighted
                                    texrect_draw(&gMenuFrame.displayList, gRaceSelectionImages[(k * 3) + 2],
                                                 gTracksMenuVehicleNamePositions[j + temp2], y, 255, 255, 255,
                                                 gMenuFrame.guiOpacity);
                                } else {
                                    // Not available (Ghosted out)
                                    texrect_draw(&gMenuFrame.displayList, gRaceSelectionImages[(k * 3) + 2],
                                                 gTracksMenuVehicleNamePositions[j + temp2], y, 255, 255, 255,
                                                 (gMenuFrame.guiOpacity / 2));
                                }
                            }
                            y += 24;
                        }
                    }
                }
                y = regionOffset + 139;
                if (gRaceSetup.playerVehicles[PLAYER_ONE] == 2) {
                    y += 2;
                }
                if (gRaceSetup.numberOfActivePlayers == 1) {
                    if (gMenuFlow.stage == TRACKMENU_OPT_1) {
                        // Draw vehicle image for one player
                        texrect_draw(&gMenuFrame.displayList,
                                     gRaceSelectionImages[gRaceSetup.playerVehicles[PLAYER_ONE] * 3], 149, y, 255, 255,
                                     255, gMenuFrame.guiOpacity);
                    } else {
                        // Draw T.T. image for one player
                        texrect_draw(&gMenuFrame.displayList,
                                     gTrackSelectTTImage[gTracksMenuTimeTrialHighlightIndex * 3], 149,
                                     regionOffset + 139, 255, 255, 255, gMenuFrame.guiOpacity);
                    }
                }
                if (gRaceSetup.numberOfActivePlayers == 2 && !sp74) {
                    // Draw vehicle image for first player
                    texrect_draw(&gMenuFrame.displayList,
                                 gRaceSelectionImages[gRaceSetup.playerVehicles[PLAYER_ONE] * 3], 79, y, 255, 255, 255,
                                 gMenuFrame.guiOpacity);
                    y = regionOffset + 139;
                    if (gRaceSetup.playerVehicles[PLAYER_TWO] == 2) {
                        y += 2;
                    }
                    // Draw vehicle image for second player
                    texrect_draw(&gMenuFrame.displayList,
                                 gRaceSelectionImages[gRaceSetup.playerVehicles[PLAYER_TWO] * 3], 176, y, 255, 255, 255,
                                 gMenuFrame.guiOpacity);
                }
                rendermode_reset(&gMenuFrame.displayList);
                if (gRaceSetup.numberOfActivePlayers <= 2 && !sp74) {
                    // Draw border around vehicle images
                    gMenuLoaded.images[7].trans.y_position = -52.0f;
                    if (gRaceSetup.numberOfActivePlayers == 1) {
                        gMenuLoaded.images[7].trans.x_position = 21.0f;
                        menu_element_render(7);
                    } else {
                        gMenuLoaded.images[7].trans.x_position = -48.0f;
                        menu_element_render(7);
                        gMenuLoaded.images[7].trans.x_position = 48.0f;
                        menu_element_render(7);
                    }
                }
                if (sp74) {
                    temp2 =
                        get_text_width(gMenuLoaded.text[ASSET_MENU_TEXT_NUMBEROFRACERS], 0, 0); // "NUMBER OF RACERS"
                    if ((gTwoPlayerRacerCountMenu.width - 12) < temp2) {
                        temp2 = temp2 + 12;
                    } else {
                        temp2 = gTwoPlayerRacerCountMenu.width;
                    }
                    menu_panel_draw_textured(&gMenuFrame.displayList, -(temp2 >> 1), 120 - gTwoPlayerRacerCountMenu.y,
                                             temp2, gTwoPlayerRacerCountMenu.height,
                                             gTwoPlayerRacerCountMenu.borderWidth,
                                             gTwoPlayerRacerCountMenu.borderHeight, COLOUR_RGBA32(176, 224, 192, 255),
                                             gMenuLoaded.assets[67]);
                    menu_geometry_end();
                    set_text_font(ASSET_FONTS_FUNFONT);
                    set_text_colour(0, 0, 0, 255, 128);
                    draw_text(&gMenuFrame.displayList,
                              gTwoPlayerRacerCountMenu.textPos[0] + gTwoPlayerRacerCountMenu.x + 1,
                              gTwoPlayerRacerCountMenu.textPos[1] + gTwoPlayerRacerCountMenu.y + regionOffset + 1,
                              gMenuLoaded.text[ASSET_MENU_TEXT_NUMBEROFRACERS], ALIGN_MIDDLE_CENTER);
                    set_text_colour(255, 255, 255, 0, 255);
                    draw_text(&gMenuFrame.displayList,
                              (gTwoPlayerRacerCountMenu.textPos[0] + gTwoPlayerRacerCountMenu.x) - 1,
                              (gTwoPlayerRacerCountMenu.textPos[1] + gTwoPlayerRacerCountMenu.y + regionOffset) - 1,
                              gMenuLoaded.text[ASSET_MENU_TEXT_NUMBEROFRACERS], ALIGN_MIDDLE_CENTER);
                    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
                    sprite_opaque(FALSE);

                    for (temp2 = 0; temp2 < 3; temp2++) {
                        if (temp2 == gRaceSetup.multiplayerRacerCount) {
                            if (gMenuFlow.stage <= TRACKMENU_OPT_2) {
                                gMenuFrame.guiColourG = 255 - sp84;
                                gMenuFrame.guiColourB = 255 - sp84;
                            } else {
                                gMenuFrame.guiColourG = 0;
                                gMenuFrame.guiColourB = 0;
                            }
                        }
                        gMenuLoaded.images[0].trans.x_position =
                            (gTwoPlayerRacerCountMenu.textPos[(temp2 << 1) + 2] + gTwoPlayerRacerCountMenu.x) - 160;
                        gMenuLoaded.images[0].trans.y_position =
                            (-gTwoPlayerRacerCountMenu.textPos[(temp2 << 1) + 2 + 1] - gTwoPlayerRacerCountMenu.y) +
                            120;
                        gMenuLoaded.images[0].spriteOffset = (temp2 << 1) + 2;
                        menu_element_render(0);
                        gMenuFrame.guiColourG = 255;
                        gMenuFrame.guiColourB = 255;
                    }
                    sprite_opaque(TRUE);
                    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
                }
            }
        }
        if (gTrackSelectCursorX < 4) {
            sprite_opaque(FALSE);
            k = 11;
            if (gRaceSetup.isInAdventureTwo) {
                k = 12;
            }
            gMenuLoaded.images[k].trans.y_position = 42.0f;
            if ((settings->courseFlagsPtr[gTrackSelect.previewTrackId] & 2) || (is_adventure_two_unlocked())) {
#if VERSION >= VERSION_79
                gMenuLoaded.images[k].trans.x_position = (f32) ((gRaceSetup.isInAdventureTwo * 2) - 128);
#else
                gMenuLoaded.images[k].trans.x_position = -128.0f;
#endif
                menu_element_render(k);
            }
            if ((settings->courseFlagsPtr[gTrackSelect.previewTrackId] & 4) || (is_adventure_two_unlocked())) {
#if VERSION >= VERSION_79
                gMenuLoaded.images[k].trans.x_position = (f32) ((gRaceSetup.isInAdventureTwo * 2) + 120);
#else
                gMenuLoaded.images[k].trans.x_position = 120.0f;
#endif
                menu_element_render(k);
            }
            sprite_opaque(TRUE);
        }
        if (gRaceSetup.numberOfActivePlayers == 1 && gMenuFlow.stage > TRACKMENU_MIRROR &&
            trackmenu_staff_beaten(gTrackSelect.previewTrackId) >= 0) {
            texrect_draw(&gMenuFrame.displayList, gRaceSelectionTTTexture, 204, regionOffset + 122, 255, 255, 255,
                         gMenuFrame.guiOpacity);
            rendermode_reset(&gMenuFrame.displayList);
        }
        if (gTrackSelectCursorX != 5) {
            if ((gMenuFlow.stage == TRACKMENU_OPT_2 && !sp74) || (gMenuFlow.stage == TRACKMENU_OPT_3 && sp74) ||
                gMenuFlow.stage == TRACKMENU_OPT_4) {
                set_text_font(ASSET_FONTS_BIGFONT);
                set_text_colour(255, 255, 255, 0, gMenuFrame.guiOpacity);
                if (gTrackSelectCursorX >= 4) {
                    regionOffset += 24;
                }
                draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, regionOffset + 172, "OK?", ALIGN_MIDDLE_CENTER);
            }
        }
        gMenuFrame.guiOpacity = 255;
    }
}

void trackmenu_update_vehicle_select(s32 updateRate) {
    s32 yOffset2;
    s32 origVehicle;
    s32 availableVehicles;
    s32 xOffset;
    s32 yOffset;
    s32 i;
    s32 menuBackedOut;
    s32 menuSelected;
    s32 menuChanged;
    s32 menuDelay;
    Settings *settings;

    menuDelay = gMenuInput.delay;
    settings = get_settings();
    if (gTrackSelect.nameVoiceDelay != 0) {
        gTrackSelect.nameVoiceDelay += updateRate;
    }
    // Challenge Races
    if (gTrackSelectCursorX == 5 && gMenuFlow.stage != 2) {
        gMenuFlow.stage = 2;
        if (gTrackSelectCursorX == 5) {
            // One of the following three text messages
            // THE FIRST PLAYER TO HATCH // THE LAST PLAYER REMAINING // THE FIRST PLAYER TO GET 10
            // THREE EGGS WILL WIN!      // WILL WIN!!                // BANANAS INTO THEIR TREASURE
            //                                                        // CHEST WILL WIN!!
            set_current_text(gTrackSelect.cursorY + ASSET_GAME_TEXT_60);
        }
    }
    // Trophy Races
    if (gTrackSelectCursorX == 4 && gMenuFlow.stage != -1 && gMenuFlow.stage != 2) {
        gMenuFlow.stage = 2;
    }
    if (gTTVoiceLines[gTrackSelect.previewTrackId] != -1 && gTrackSelect.nameVoiceDelay >= 7) {
        sound_play(gTTVoiceLines[gTrackSelect.previewTrackId], NULL);
        gTrackSelect.nameVoiceDelay = 0;
    }
    xOffset = gMenuInput.delay + 25;
    if (gMenuInput.delay < 0) {
        if (xOffset > 20) {
            xOffset = 20;
        }
        if (xOffset < 0) {
            xOffset = 0;
        }
        yOffset = ((xOffset + 20) * gTrackSelect.viewportHalfY) / 40;
        yOffset2 = yOffset + gTrackSelect.viewportHalfY;
        viewport_menu_set(0, 80 - (xOffset * 4), gTrackSelect.viewportHalfY - yOffset, (xOffset * 4) + 240, yOffset2);
        gMenuLoaded.images[4].trans.scale = sMenuImageProperties[4].trans.scale * (1.0f + ((f32) xOffset / 20.0f));
        gMenuLoaded.images[5].trans.scale = sMenuImageProperties[5].trans.scale * (1.0f + ((f32) xOffset / 20.0f));
        gMenuLoaded.images[6].trans.scale = sMenuImageProperties[6].trans.scale * (1.0f + ((f32) xOffset / 20.0f));
    }
    if (gMenuInput.delay > 0) {
        gTrackSelect.musicVolume -= updateRate * 4;
    }
    if (gMenuInput.delay > 40) {
        trackmenu_assets(TRACKMENU_TYPE_LOAD_LEVEL);
    } else if (gMenuInput.delay < -30) {
        gMenuFlow.opacityDecayTimer = 0;
        trackmenu_assets(TRACKMENU_TYPE_RESET_CURSOR);
        viewport_menu_set(0, 80, gTrackSelect.viewportHalfY - (gTrackSelect.viewportHalfY >> 1), 240,
                          (gTrackSelect.viewportHalfY >> 1) + gTrackSelect.viewportHalfY);
    }
    camEnableUserView(0, TRUE);
    if (menuDelay == 0) {
        availableVehicles = leveltable_vehicle_usable(gTrackSelect.previewTrackId);
#if VERSION >= VERSION_79
        if (gRaceSetup.numberOfActivePlayers >= 2) {
            if (gTrackSelect.previewTrackId == ASSET_LEVEL_SPACEPORTALPHA) {
                availableVehicles &= ~(1 << VEHICLE_HOVERCRAFT);
            }
            if (gTrackSelect.previewTrackId == ASSET_LEVEL_FROSTYVILLAGE) {
                availableVehicles &= ~(1 << VEHICLE_PLANE);
            }
        }
#endif
        menuBackedOut = FALSE;
        menuSelected = FALSE;
        menuChanged = FALSE;
        switch (gMenuFlow.stage) {
            case -1:
                if (gMenuInput.buttons[4] & (A_BUTTON | START_BUTTON)) {
                    if (gTrackSelectCursorX == 4) {
                        gMenuFlow.stage = 2;
                    } else {
                        gMenuFlow.stage = 0;
                    }
                    menuSelected = TRUE;
                } else if (gMenuInput.buttons[4] & B_BUTTON) {
                    gMenuInput.delay = -1;
                    menuBackedOut = TRUE;
                } else {
                    if (gMenuInput.stickY[4] > 0 && gTracksMenuAdventureHighlightIndex != 0) {
                        gTracksMenuAdventureHighlightIndex = 0;
                        menuChanged = TRUE;
                    } else if ((gMenuInput.stickY[4] < 0) && (gTracksMenuAdventureHighlightIndex == 0)) {
                        gTracksMenuAdventureHighlightIndex = 1;
                        menuChanged = TRUE;
                    }
                }
                break;
            case 0:
                for (i = 0; i < gRaceSetup.numberOfActivePlayers; i++) {
                    if (gMenuInput.buttons[i] & B_BUTTON) {
                        menuBackedOut = TRUE;
                        if (gRaceSetup.numberOfReadyPlayers == 0) {
                            if (is_adventure_two_unlocked()) {
                                gMenuFlow.stage = -1;
                            } else {
                                gMenuInput.delay = -1;
                            }
                        } else if (gPlayerSelectConfirm[i] != 0) {
                            gRaceSetup.numberOfReadyPlayers--;
                            gPlayerSelectConfirm[i] = 0;
                        }
                    } else {
                        if (gMenuInput.buttons[i] & (A_BUTTON | START_BUTTON)) {
                            if (gPlayerSelectConfirm[i] == 0) {
                                gPlayerSelectConfirm[i] = 1;
                                gRaceSetup.numberOfReadyPlayers++;
                                menuSelected = TRUE;
                            }
                        } else if (gPlayerSelectConfirm[i] == 0 &&
                                   settings->courseFlagsPtr[gTrackSelect.previewTrackId] & 2) {
                            origVehicle = gRaceSetup.playerVehicles[i];
                            if (gMenuInput.stickY[i] > 0) {
                                do {
                                    gRaceSetup.playerVehicles[i]--;
                                } while (((1 << gRaceSetup.playerVehicles[i]) & availableVehicles) == 0 &&
                                         gRaceSetup.playerVehicles[i] >= 0);
                            }
                            if (gMenuInput.stickY[i] < 0) {
                                do {
                                    gRaceSetup.playerVehicles[i]++;
                                } while (((1 << gRaceSetup.playerVehicles[i]) & availableVehicles) == 0 &&
                                         gRaceSetup.playerVehicles[i] < 3);
                            }
                            if (origVehicle != gRaceSetup.playerVehicles[i]) {
                                if (gRaceSetup.playerVehicles[i] < 0 || gRaceSetup.playerVehicles[i] >= 3) {
                                    gRaceSetup.playerVehicles[i] = origVehicle;
                                } else {
                                    menuChanged = TRUE;
                                }
                            }
                        }
                    }
                }
                if (gRaceSetup.numberOfReadyPlayers == gRaceSetup.numberOfActivePlayers) {
                    menuBackedOut = FALSE;
                    menuSelected = FALSE;
                    menuChanged = FALSE;
                    if (gRaceSetup.numberOfActivePlayers >= 2) {
                        gMenuFlow.stage = 2;
                    } else {
                        gMenuFlow.stage = 1;
                    }
                    sound_play(SOUND_CAR_REV2, NULL);
                }
                break;
            case 1:
                if (gMenuInput.buttons[0] & (A_BUTTON | START_BUTTON)) {
                    gMenuFlow.stage = 2;
                    menuSelected = TRUE;
                } else if (gMenuInput.buttons[0] & B_BUTTON) {
                    gPlayerSelectConfirm[0] = 0;
                    gRaceSetup.numberOfReadyPlayers = 0;
                    gMenuFlow.stage = 0;
                    menuBackedOut = TRUE;
                }
                if (gMenuInput.stickY[0] > 0 && gTracksMenuTimeTrialHighlightIndex > 0) {
                    if (sMenuSoundHandle != NULL) {
                        sndp_stop(sMenuSoundHandle);
                    }
                    gTracksMenuTimeTrialHighlightIndex--;
                    menuChanged = TRUE;
                }
                if (gMenuInput.stickY[0] < 0) {
                    if (gTracksMenuTimeTrialHighlightIndex <= 0) {
                        menuChanged = TRUE;
                        if (gTrackTTSoundMask != NULL) {
                            sndp_stop(gTrackTTSoundMask);
                        }
                        if (rand_range(0, 255) >= 128) {
                            sound_play(SOUND_VOICE_TT_INTRO, &sMenuSoundHandle);
                        } else {
                            sound_play(SOUND_VOICE_TT_GO_FOR_IT, &sMenuSoundHandle);
                        }
                        gTracksMenuTimeTrialHighlightIndex++;
                    }
                }
                break;
            case 2: /* fall through */
            case 3:
                if (gMenuFlow.stage == 2 && gRaceSetup.numberOfActivePlayers == 2 && gTrackSelectCursorX < 4) {
                    if (gMenuInput.buttons[4] & (A_BUTTON | START_BUTTON)) {
                        gMenuFlow.stage = 3;
                        menuSelected = TRUE;
                    } else if (gMenuInput.buttons[4] & B_BUTTON) {
                        gMenuFlow.stage = 0;
                        gRaceSetup.numberOfReadyPlayers--;
                        if (gMenuInput.buttons[0] & B_BUTTON) {
                            gPlayerSelectConfirm[0] = 0;
                        } else {
                            gPlayerSelectConfirm[1] = 0;
                        }
                        menuBackedOut = TRUE;
                    } else {
                        if (gMenuInput.stickX[4] < 0 && gRaceSetup.multiplayerRacerCount > 0) {
                            menuChanged = TRUE;
                            gRaceSetup.multiplayerRacerCount--;
                        } else if (gMenuInput.stickX[4] > 0 && gRaceSetup.multiplayerRacerCount < 2) {
                            menuChanged = TRUE;
                            gRaceSetup.multiplayerRacerCount++;
                        }
                    }
                } else if (gMenuInput.buttons[4] & (A_BUTTON | START_BUTTON)) {
                    gMenuInput.delay = 1;
                    bgdraw_set_func(NULL);
                    gIsInTracksMenu = 0;
                    disable_new_screen_transitions();
                    transition_begin(&sMenuTransitionFadeIn);
                    if (gTrackSelectCursorX == 5) {
                        set_current_text(10000); // Nonsense value. No text.
                    }
                    menuSelected = TRUE;
                } else if (gTrackSelectCursorX >= 4) {
                    if (gMenuInput.buttons[4] & B_BUTTON) {
                        menuBackedOut = TRUE;
                        if ((gTrackSelectCursorX == 4) && (is_adventure_two_unlocked())) {
                            gMenuFlow.stage = -1;
                        } else {
                            gMenuInput.delay = -1;
                        }
                        if (gTrackSelectCursorX == 5) {
                            set_current_text(10000); // Nonsense value. No text.
                        }
                    }
                } else if (gRaceSetup.numberOfActivePlayers == 1) {
                    if (gMenuInput.buttons[0] & B_BUTTON) {
                        gMenuFlow.stage = 1;
                        menuBackedOut = TRUE;
                    }
                } else if (gRaceSetup.numberOfActivePlayers == 2) {
                    if (gMenuInput.buttons[4] & B_BUTTON) {
                        gMenuFlow.stage = 2;
                        menuBackedOut = TRUE;
                    }
                } else {
                    for (i = 0; i < gRaceSetup.numberOfActivePlayers; i++) {
                        if (gMenuInput.buttons[i] & B_BUTTON) {
                            gRaceSetup.numberOfReadyPlayers--;
                            gPlayerSelectConfirm[i] = 0;
                            gMenuFlow.stage = 0;
                            menuBackedOut = TRUE;
                        }
                    }
                }
                break;
        }
        if (menuBackedOut) {
            sound_play(SOUND_MENU_BACK3, NULL);
        } else if (menuSelected) {
            sound_play(SOUND_SELECT2, NULL);
        } else if (menuChanged) {
            sound_play(SOUND_MENU_PICK2, NULL);
        }
        set_level_default_vehicle((enum Vehicle) * gRaceSetup.playerVehicles);
        if (gRaceSetup.numberOfActivePlayers >= 2 || gTrackSelectCursorX >= 4) {
            set_time_trial_enabled(FALSE);
        } else {
            set_time_trial_enabled(gTracksMenuTimeTrialHighlightIndex);
        }
        gRaceSetup.isInAdventureTwo = gTracksMenuAdventureHighlightIndex;
    }
}

/**
 * Return whether the staff time has been beaten for this level.
 * Get the track ID then compare it with the bits in the main eeprom settings field.
 */
s32 trackmenu_staff_beaten(s32 mapId) {
    s8 *trackIdArray;
    s32 index;
    s32 temp;

    trackIdArray = (s8 *) get_misc_asset(ASSET_MISC_MAIN_TRACKS_IDS);

    index = 0;
    temp = -1;
    if (trackIdArray[0] != -1) {
        while (temp < 0) {
            if (mapId == (s32) trackIdArray[index]) {
                temp = index;
            }
            index++;
            if (trackIdArray[index] == -1) {
                break;
            }
        }
    }

    if (temp >= 0) {
        u64 temp2 = 0x10 << temp;
        if ((temp2 & gRaceSetup.eepromSettings) == 0) {
            temp = -1;
        }
    }

    return temp;
}

/**
 * Load and initialise the track setup gui for use when entering a track in adventure mode.
 */
void menu_adventure_track_init(void) {
    Settings *settings;
    s32 raceType;
    s32 mapId;
    s16 ttVoiceLine;

    settings = get_settings();
    gTrackSelect.previewTrackId = ASSET_LEVEL_CENTRALAREAHUB;
    gMenuFlow.blinkTimer = 0;
    gMenuInput.delay = 0;
    mapId = settings->levelTransition->mapID;
    gRaceSetup.playerVehicles[PLAYER_ONE] = leveltable_vehicle_default(mapId);
    raceType = leveltable_type(mapId);
    if (raceType == RACETYPE_HUBWORLD || raceType == RACETYPE_BOSS ||
        (!(raceType & RACETYPE_CHALLENGE) && (!(settings->courseFlagsPtr[mapId] & RACE_CLEARED)))) {
        ttVoiceLine = gTTVoiceLines[mapId];
        if (ttVoiceLine != -1) {
            sound_play_delayed(ttVoiceLine, NULL, 1.0f);
        }
        gMenuFlow.stage = ADVENTURESETUP_PREVIEW;
    } else {
        ttVoiceLine = gTTVoiceLines[mapId];
        if (ttVoiceLine != -1) {
            sound_play_delayed(ttVoiceLine, NULL, 0.5f);
        }
        music_voicelimit_set(24);
        music_play(SEQUENCE_MAIN_MENU);
        music_change_off();
        gMenuFlow.stage = ADVENTURESETUP_VEHICLE;
        menu_assetgroup_load(gAdvTrackInitObjectIndices);
        menu_imagegroup_load(gAdvTrackInitImageIndices);
        menu_init_vehicle_textures();
        gRaceSelectionCarOptHighlight[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_CAR_HIGHLIGHT];
        gRaceSelectionCarOpt[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_CAR];
        gRaceSelectionHoverOptHighlight[0].texture =
            gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_HOVERCRAFT_HIGHLIGHT];
        gRaceSelectionHoverOpt[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_HOVERCRAFT];
        gRaceSelectionPlaneOptHighlight[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_PLANE_HIGHLIGHT];
        gRaceSelectionPlaneOpt[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_SELECT_PLANE];
        gRaceSelectionVehicleTitleTexture[0].texture = gMenuLoaded.assets[TEXTURE_ICON_VEHICLE_TITLE];
#if VERSION >= VERSION_79
        // Did the TEXTURE_ICON_TT_HEAD enum value change?
        gRaceSelectionTTTexture[0].texture = gMenuLoaded.assets[TEXTURE_UNK_69];
#else
        gRaceSelectionTTTexture[0].texture = gMenuLoaded.assets[TEXTURE_ICON_TT_HEAD];
#endif

        transition_begin(&sMenuTransitionFadeOut);
        gMenuFlow.blinkTimer = 0;
        gMenuInput.delay = 0;
        gTrackSelect.nameVoiceDelay = 30;
        load_font(ASSET_FONTS_BIGFONT);
        load_level_for_menu(mapId, -1, 1);
    }
    dialogue_clear(7);
    if (leveltable_type(mapId) & RACETYPE_CHALLENGE) {
        set_current_text(leveltable_world(mapId) + ASSET_GAME_TEXT_59);
    }
}

/**
 * Render the setup gui when in a track preview in adventure mode.
 * This includes the vehicle selection and if time trial is enabled, the best times.
 */
void adventuretrack_render(UNUSED s32 updateRate, s32 arg1, s32 arg2) {
    s32 alpha;
    s32 y;
    s32 greenAmount;
    s32 i;
    s32 savedY;
    s32 yOffset;
    s32 mask;
    s32 mapID;
    char *filename;
    Settings *settings;
    char *levelName;

    filename = NULL;
    settings = get_settings();
    yOffset = 0;
    if (osTvType == OS_TV_TYPE_PAL) {
        yOffset = 12;
    }
    mapID = ((LevelTransitionSettings *) ((u8 *) settings->levelTransition + gTrackSelect.previewTrackId))->mapID;
    gSPClearGeometryMode(gMenuFrame.displayList++, G_CULL_FRONT);
    menu_camera_centre();
    mtx_ortho(&gMenuFrame.displayList, &gMenuFrame.hudMat);
    if (gMenuInput.delay >= -20) {
        if (gMenuInput.delay <= 20) {
            mask = leveltable_vehicle_usable(mapID);
            levelName = level_name(mapID);
            set_text_font(FONT_LARGE);
            set_text_background_colour(0, 0, 0, 0);
            set_text_colour(0, 0, 0, 255, 128);
            draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 1, 46, levelName, ALIGN_MIDDLE_CENTER);
            set_text_colour(255, 255, 255, 0, 255);
            draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, 43, levelName, ALIGN_MIDDLE_CENTER);
            if (!(leveltable_type(mapID) & RACETYPE_CHALLENGE)) {
                if (arg2 == 0) {
                    if (is_time_trial_enabled()) {
                        if (trackmenu_staff_beaten(mapID) >= 0) {
                            texrect_draw(&gMenuFrame.displayList, gRaceSelectionTTTexture, SCREEN_HEIGHT - 36,
                                         yOffset + 122, 255, 255, 255, gMenuFrame.guiOpacity);
                        }
                        set_text_font(ASSET_FONTS_FUNFONT);
                        set_text_colour(255, 64, 64, 96, 255);
#if VERSION == VERSION_80
#define ADVENTURETRACK_XPOS1 56
#define ADVENTURETRACK_XPOS2 250
#define ADVENTURETRACK_YPOS1 72
#define ADVENTURETRACK_YPOS2 92
#define ADVENTURETRACK_YPOS3 72
#define ADVENTURETRACK_RED 22
#define ADVENTURETRACK_ALIGN ALIGN_MIDDLE_LEFT
#define ADVENTURETRACK_MAPID mapID
#elif VERSION == VERSION_79
#define ADVENTURETRACK_XPOS1 51
#define ADVENTURETRACK_XPOS2 249
#define ADVENTURETRACK_YPOS1 69
#define ADVENTURETRACK_YPOS2 89
#define ADVENTURETRACK_YPOS3 70
#define ADVENTURETRACK_RED 22
#define ADVENTURETRACK_ALIGN ALIGN_MIDDLE_LEFT
#define ADVENTURETRACK_MAPID mapID
#else
#define ADVENTURETRACK_XPOS1 88
#define ADVENTURETRACK_XPOS2 258
#define ADVENTURETRACK_YPOS1 72
#define ADVENTURETRACK_YPOS2 92
#define ADVENTURETRACK_YPOS3 72
#define ADVENTURETRACK_RED 26
#define ADVENTURETRACK_ALIGN ALIGN_MIDDLE_CENTER
#define ADVENTURETRACK_MAPID \
    ((LevelTransitionSettings *) ((u8 *) settings->levelTransition + gTrackSelect.previewTrackId))->mapID
#endif
                        draw_text(&gMenuFrame.displayList, ADVENTURETRACK_XPOS1, yOffset + ADVENTURETRACK_YPOS1,
                                  gMenuLoaded.text[ASSET_MENU_TEXT_BESTTIME], ADVENTURETRACK_ALIGN);
                        draw_text(&gMenuFrame.displayList, ADVENTURETRACK_XPOS1, yOffset + ADVENTURETRACK_YPOS2,
                                  gMenuLoaded.text[ASSET_MENU_TEXT_BESTLAP], ADVENTURETRACK_ALIGN);
                        set_text_colour(255, 128, 255, 96, 255);
                        filename_decompress(settings->courseInitialsPtr[gRaceSetup.playerVehicles[0]][mapID],
                                            (char *) &filename, MAX_INITIALS_LENGTH);
                        draw_text(&gMenuFrame.displayList, ADVENTURETRACK_XPOS2, yOffset + ADVENTURETRACK_YPOS3,
                                  (char *) &filename, ALIGN_MIDDLE_CENTER);
                        filename_decompress(settings->flapInitialsPtr[gRaceSetup.playerVehicles[0]][mapID],
                                            (char *) &filename, MAX_INITIALS_LENGTH);
                        draw_text(&gMenuFrame.displayList, ADVENTURETRACK_XPOS2, yOffset + ADVENTURETRACK_YPOS2,
                                  (char *) &filename, ALIGN_MIDDLE_CENTER);
                        menu_timestamp_render(
                            settings->courseTimesPtr[gRaceSetup.playerVehicles[0]][ADVENTURETRACK_MAPID],
                            ADVENTURETRACK_RED, 53, 128, 255, 255, 0);
                        menu_timestamp_render(
                            settings->flapTimesPtr[gRaceSetup.playerVehicles[0]][ADVENTURETRACK_MAPID],
                            ADVENTURETRACK_RED, 33, 255, 192, 255, 0);
                    }
                    greenAmount = gMenuFlow.blinkTimer * 8;
                    if (greenAmount > 255) {
                        greenAmount = 511 - greenAmount;
                    }
                    set_current_dialogue_background_colour(7, 255, greenAmount, 0, 255);
                    set_current_dialogue_box_coords(7, 134, yOffset + 112, 186, yOffset + 137);
                    render_dialogue_box(&gMenuFrame.displayList, NULL, NULL, 7);
                    texrect_draw(&gMenuFrame.displayList, gRaceSelectionVehicleTitleTexture, 136, yOffset + 114, 255,
                                 255, 255, 255);

                    y = yOffset + 139;
                    savedY = y;

                    for (i = 0; i <= VEHICLE_PLANE; i++) {
                        alpha = (arg1 < 2 && leveltable_vehicle_default(mapID) != (Vehicle) i) ? 128 : 255;
                        if ((1 << i) & mask) {
                            if (i == gRaceSetup.playerVehicles[0]) {
                                texrect_draw(&gMenuFrame.displayList, gRaceSelectionImages[i * 3 + 1], 104, y, 255, 255,
                                             255, 255);
                            } else {
                                texrect_draw(&gMenuFrame.displayList, gRaceSelectionImages[i * 3 + 2], 104, y, 255, 255,
                                             255, alpha);
                            }
                            y += 24;
                        }
                    }
                    y = savedY;
                    if (gRaceSetup.playerVehicles[0] == 2) {
                        y += 2;
                    }
                    texrect_draw(&gMenuFrame.displayList, gRaceSelectionImages[gRaceSetup.playerVehicles[0] * 3], 149,
                                 y, 255, 255, 255, 255);
                    rendermode_reset(&gMenuFrame.displayList);
                    gMenuLoaded.images[7].trans.x_position = 21.0f;
                    gMenuLoaded.images[7].trans.y_position = -52.0f;
                    menu_element_render(7);
                    if (gMenuFlow.stage != ADVENTURESETUP_VEHICLE) {
                        set_text_font(FONT_LARGE);
#if VERSION == VERSION_77
                        set_text_background_colour(0, 0, 0, 0);
#endif
                        set_text_colour(255, 255, 255, 0, 255);
                        draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, yOffset + 172, "OK?",
                                  ALIGN_MIDDLE_CENTER);
                    }
                } else {
                    set_text_font(FONT_LARGE);
                    set_text_background_colour(0, 0, 0, 0);
                    set_text_colour(255, 255, 255, 0, 255);
                    y = yOffset + 176;
                    if (get_language() == LANGUAGE_FRENCH) {
#if VERSION >= VERSION_79
                        draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, y, gMenuLoaded.text[191],
                                  ALIGN_MIDDLE_CENTER);
#else
                        draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, y,
                                  gMenuLoaded.text[ASSET_MENU_TEXT_CHALLENGE], ALIGN_MIDDLE_CENTER);
#endif
                        y += 32;
                    }
                    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, y,
                              gMenuLoaded.text[ASSET_MENU_TEXT_SILVERCOIN], ALIGN_MIDDLE_CENTER);
                    y += 32;
                    if (get_language() != LANGUAGE_FRENCH) {
#if VERSION >= VERSION_79
                        draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, y, gMenuLoaded.text[191],
                                  ALIGN_MIDDLE_CENTER);
#else
                        draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, y,
                                  gMenuLoaded.text[ASSET_MENU_TEXT_CHALLENGE], ALIGN_MIDDLE_CENTER);
#endif
                    }
                }
            }
        }
    }
}

/**
 * Process the logic for the track setup selection after entering a door in adventure mode.
 * This also includes drawing all the gui.
 */
s32 menu_adventure_track_loop(s32 updateRate) {
    s32 vehicle;
    s32 vehicleFlags;
    s32 vehicle2;
    s32 challenge;
    s32 mapId;
    s32 sp20;
    s32 sp1C;
    Settings *settings;

    if (gMenuFlow.stage < ADVENTURESETUP_VEHICLE) {
        return gTrackSelect.previewTrackId | MENU_RESULT_FLAGS_80;
    }
    settings = get_settings();
    mapId = ((LevelTransitionSettings *) ((u8 *) settings->levelTransition + gTrackSelect.previewTrackId))->mapID;
    challenge = FALSE;
    sp1C = 0;
    if (settings->courseFlagsPtr[mapId] & RACE_CLEARED) {
        sp1C = 1;
    }
    if (settings->courseFlagsPtr[mapId] & RACE_CLEARED_SILVER_COINS) {
        sp1C = 2;
    }
    if (leveltable_type(mapId) & RACETYPE_CHALLENGE) {
        challenge = TRUE;
    }
    sp20 = FALSE;
    if (!is_time_trial_enabled()) {
        if (sp1C == 1) {
            if (settings->worldId == WORLD_FUTURE_FUN_LAND || (settings->bosses & (1 << settings->worldId))) {
                sp20 = TRUE;
            }
        }
    }
    gMenuFlow.blinkTimer = (gMenuFlow.blinkTimer + updateRate) & 0x3F;
    adventuretrack_render(updateRate, sp1C, sp20);
    if (sp1C < 2) {
        gRaceSetup.playerVehicles[PLAYER_ONE] = leveltable_vehicle_default(mapId);
    }
    vehicle = gRaceSetup.playerVehicles[PLAYER_ONE];
    vehicleFlags = leveltable_vehicle_usable(mapId);
    vehicle2 = vehicle;
    menu_input();
    if (gMenuInput.delay == 0) {
        if (gMenuFlow.stage != ADVENTURESETUP_VEHICLE || sp20 || challenge) {
            if (gMenuInput.buttons[PLAYER_ONE] & (A_BUTTON | START_BUTTON)) {
                if (challenge) {
                    set_current_text(10000);
                }
                gMenuInput.delay = 1;
                transition_begin(&sMenuTransitionFadeIn);
                sound_play(SOUND_SELECT2, NULL);
            } else if (gMenuInput.buttons[PLAYER_ONE] & B_BUTTON) {
                sound_play(SOUND_MENU_BACK3, NULL);
                if (sp20 || challenge) {
                    if (challenge) {
                        set_current_text(10000);
                    }
                    transition_begin(&sMenuTransitionFadeIn);
                    gMenuInput.delay = -1;
                } else {
                    gMenuFlow.stage = ADVENTURESETUP_VEHICLE;
                }
            }
        } else {
            if (gMenuInput.buttons[PLAYER_ONE] & B_BUTTON) {
                sound_play(SOUND_MENU_BACK3, NULL);
                transition_begin(&sMenuTransitionFadeIn);
                gMenuInput.delay = -1;
            } else if (gMenuInput.buttons[PLAYER_ONE] & (A_BUTTON | START_BUTTON)) {
                gMenuFlow.stage = ADVENTURESETUP_CONFIRM;
                sound_play(SOUND_CAR_REV2, NULL);
            } else if (sp1C >= 2) {
                if (gMenuInput.stickY[PLAYER_ONE] > 0) {
                    do {
                        vehicle--;
                    } while (!((1 << vehicle) & vehicleFlags) && vehicle >= VEHICLE_CAR);
                }
                if (gMenuInput.stickY[PLAYER_ONE] < 0) {
                    do {
                        vehicle++;
                    } while (!((1 << vehicle) & vehicleFlags) && vehicle <= VEHICLE_PLANE);
                }
                if (vehicle < VEHICLE_CAR || vehicle > VEHICLE_PLANE) {
                    vehicle = vehicle2;
                }
                if (vehicle != vehicle2) {
                    sound_play(SOUND_MENU_PICK2, NULL);
                    gRaceSetup.playerVehicles[PLAYER_ONE] = vehicle;
                }
            }
        }
    } else {
        if (gMenuInput.delay < 0) {
            gMenuInput.delay = gMenuInput.delay - updateRate;
        } else {
            gMenuInput.delay = gMenuInput.delay + updateRate;
        }
        if (gMenuInput.delay > 30 || gMenuInput.delay < -30) {
            adventuretrack_free();
            if (gMenuInput.delay > 0) {
                return gTrackSelect.previewTrackId | 0x80;
            }
            return MENU_RESULT_UNK3 | 0x100;
        }
    }
    gMenuInput.ignoreInputTime = 0;
    return MENU_RESULT_CONTINUE;
}

/**
 * Free the assets associated with the adventure track preview menu.
 */
void adventuretrack_free(void) {
    menu_assetgroup_free((s16 *) &gAdvTrackInitObjectIndices);
    unload_font(ASSET_FONTS_BIGFONT);
    music_change_on();
}

/**
 * Returns the track id that should be loaded. The value depends
 * on if whether you are playing in adventure or tracks mode.
 */
s32 get_track_id_to_load(void) {
    Settings *settings = get_settings();
    if (!gRaceSetup.isInTracksMode && gRaceSetup.trackSpecified == 0) {
        if (settings->newGame) {
            return 0;
        } else {
            return settings->courseId;
        }
    }
    gRaceSetup.trackSpecified = 0;
    return gRaceSetup.trackIdToLoad;
}

/**
 * Gets the selected vehicle index for a player.
 * 0 = Car, 1 = Hovercraft, 2 = Plane
 */
s8 get_player_selected_vehicle(s32 playerNum) {
    return gRaceSetup.playerVehicles[playerNum];
}

/**
 * Sets the selected vehicle index for a player.
 * 0 = Car, 1 = Hovercraft, 2 = Plane
 */
void set_player_selected_vehicle(s32 playerNum, s32 index) {
    gRaceSetup.playerVehicles[playerNum] = index;
}

/**
 * Sets the value of gRaceSetup.isInTracksMode, which determines if the player is in tracks mode or not.
 */
void enable_tracks_mode(s32 boolean) {
    gRaceSetup.isInTracksMode = boolean;
}

/**
 * Returns if the player is playing in the tracks mode instead of adventure.
 */
s32 is_in_tracks_mode(void) {
    return gRaceSetup.isInTracksMode;
}

/**
 * Render the border around the centre viewport in the track selection menu.
 * Comes in wood, iron and gold colours.
 */
void render_track_selection_viewport_border(ObjectModel *objMdl) {
    UNUSED s32 pad1[4];
    s32 flags;
    UNUSED s32 pad2[4];
    TextureHeader *tex;
    Triangle *tris;
    s32 triOffset;
    s32 vertOffset;
    Vertex *verts;
    s32 numVerts;
    s32 numTris;
    s32 texOffset;
    s32 texEnabled;
    s32 i;

    flags = RENDER_FOG_ACTIVE | RENDER_ANTI_ALIASING;
    if (gMenuFrame.guiOpacity != 255) {
        flags = RENDER_FOG_ACTIVE | RENDER_SEMI_TRANSPARENT | RENDER_ANTI_ALIASING;
    }
    for (i = 0; i < objMdl->numberOfBatches; i++) {
        if (!(objMdl->batches[i].flags & RENDER_Z_UPDATE)) {
            vertOffset = objMdl->batches[i].verticesOffset;
            triOffset = objMdl->batches[i].facesOffset;
            numVerts = objMdl->batches[i + 1].verticesOffset - vertOffset;
            numTris = objMdl->batches[i + 1].facesOffset - triOffset;
            verts = &objMdl->vertices[vertOffset];
            tris = &objMdl->triangles[triOffset];
            //!@bug Never true, since textureIndex is unsigned. This should've been either `== (u8)-1` or `== 0xFF`.
            if (objMdl->batches[i].textureIndex == -1) {
                tex = NULL;
                texEnabled = FALSE;
                texOffset = 0;
            } else {
                tex = objMdl->textures[objMdl->batches[i].textureIndex].texture;
                texEnabled = TRUE;
                texOffset = objMdl->batches[i].texOffset << 14;
            }
            material_set(&gMenuFrame.displayList, tex, flags, texOffset);

            gSPVertexDKR(gMenuFrame.displayList++, OS_K0_TO_PHYSICAL(verts), numVerts, 0);
            gSPPolygon(gMenuFrame.displayList++, OS_K0_TO_PHYSICAL(tris), numTris, texEnabled);
        }
    }
}
