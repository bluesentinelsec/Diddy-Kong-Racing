#include "menu_internal.h"

static s16 sPostRaceRowOffsets[2] = { 0x0000, 0x0000 };

static s16 gRaceResultsObjectIndices[14] = { 0x0005, 0x003B, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036,
                                      0x0039, 0x0037, 0x0038, 0x003A, 0x0000, 0x0001, 0xFFFF };

static s16 gRaceResultsImageIndices[8] = { 0x0004, 0x0000, 0x0001, 0xFFFF, 0x0300, 0x0000, 0x0012, 0xFFFF };

static char *gRacePlacementsArray[8] = { gFirstPlace, gSecondPlace, gThirdPlace,   gFourthPlace,
                                  gFifthPlace, gSixthPlace,  gSeventhPlace, gEighthPlace };

/**
 * Apply a position and scale offset to the postrace screen.
 * The scale factor counts down over time.
 * There's also a Y offset for the text and timestamps for PAL users.
 */
void postrace_offsets(MenuElement *elements, f32 in, f32 mid, f32 out, s32 textOffset, s32 timestampOffset) {
    gTrophyRankingsMenuElements = elements;
    gPostraceState = POSTRACE_ENTER;
    gPostraceScaleIn = in * 60.0f;
    gPostraceScaleMiddle = mid * 60.0f;
    gPostraceScaleOut = out * 60.0f;
    gMenuElementScaleTimer = 0;
    gPostraceTextOffset = textOffset;
    gPostraceTimestampOffset = timestampOffset;
    if (gPostraceScaleIn > 0) {
        sound_play(SOUND_WHOOSH1, NULL);
    }
}

/**
 * Render the onscreen elements for the post race screen.
 * This function also applies transformation offsets, set by the main function.
 */
s32 postrace_render(s32 updateRate) {
    f32 scale;
    s32 ret;
    s32 i;
    s32 buttonsPressedAllPlayers;

    ret = MENU_RESULT_FLAGS_1;
    scale = -1.0f;
    buttonsPressedAllPlayers = 0;
    if (gPostraceState != POSTRACE_SLIDE_END) {
        if (gIgnorePlayerInputTime == 0) {
            for (i = 0; i < gNumberOfActivePlayers; i++) {
                buttonsPressedAllPlayers |= input_pressed(i);
            }
        }
        gMenuElementScaleTimer += updateRate;
        do {
            switch (gPostraceState) {
                case POSTRACE_ENTER:
                    if (buttonsPressedAllPlayers & (A_BUTTON | START_BUTTON)) {
                        gMenuElementScaleTimer = 0;
                        gPostraceState = POSTRACE_HOLD;
                        buttonsPressedAllPlayers = 0;
                    } else {
                        if (gMenuElementScaleTimer >= gPostraceScaleIn) {
                            gMenuElementScaleTimer -= gPostraceScaleIn;
                            gPostraceState = POSTRACE_HOLD;
                        } else {
                            scale = (f32) gMenuElementScaleTimer / (f32) gPostraceScaleIn;
                        }
                    }
                    break;
                case POSTRACE_HOLD:
                    if (gPostraceScaleMiddle < 0) {
                        gMenuElementScaleTimer = 0;
                    }
                    if (buttonsPressedAllPlayers & (A_BUTTON | START_BUTTON)) {
                        gMenuElementScaleTimer = 0;
                        gPostraceState = POSTRACE_SLIDE_OUT;
                        buttonsPressedAllPlayers = 0;
                        if (gPostraceScaleOut > gMenuElementScaleTimer) {
                            sound_play(SOUND_WHOOSH1, NULL);
                        }
                    } else {
                        if (gMenuElementScaleTimer >= gPostraceScaleMiddle) {
                            gMenuElementScaleTimer -= gPostraceScaleMiddle;
                            gPostraceState = POSTRACE_SLIDE_OUT;
                            if (gMenuElementScaleTimer < gPostraceScaleOut) {
                                sound_play(SOUND_WHOOSH1, NULL);
                            }
                        } else {
                            scale = (f32) gMenuElementScaleTimer / (f32) gPostraceScaleMiddle;
                        }
                    }
                    break;
                case POSTRACE_SLIDE_OUT:
                    if ((buttonsPressedAllPlayers & (A_BUTTON | START_BUTTON)) ||
                        (gMenuElementScaleTimer >= gPostraceScaleOut)) {
                        gPostraceState = POSTRACE_SLIDE_END;
                    } else {
                        scale = (f32) gMenuElementScaleTimer / (f32) gPostraceScaleOut;
                    }
                    break;
            }
        } while (scale < 0.0f && gPostraceState != POSTRACE_SLIDE_END);

        if (gPostraceState != POSTRACE_SLIDE_END) {
            draw_menu_elements(gPostraceState, gTrophyRankingsMenuElements, scale);
            ret = MENU_RESULT_CONTINUE;
        }
    }
    return ret;
}

/**
 * Set the racer portrait element textures for their respective characters.
 */
void menu_racer_portraits(void) {
    gMenuPortraitKrunch[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_KRUNCH];
    gMenuPortraitDiddy[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_DIDDY];
    gMenuPortraitDrumstick[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_DRUMSTICK];
    gMenuPortraitBanjo[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_BANJO];
    gMenuPortraitBumper[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_BUMPER];
    gMenuPortraitConker[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_CONKER];
    gMenuPortraitTiptup[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_TIPTUP];
    gMenuPortraitTT[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_TT];
    gMenuPortraitPipsy[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_PIPSY];
    gMenuPortraitTimber[0].texture = gMenuAssets[TEXTURE_ICON_PORTRAIT_TIMBER];
}

/**
 * Initialises the post race variables.
 * Sets different defaults based on being in adventure mode, tracks mode, or multiplayer.
 * Loads the menu assets for the postrace too.
 */
void postrace_start(s32 finishState, s32 worldID) {
    s16 *var_v1;
    LevelHeader *header;

    rumble_init(FALSE);
    header = level_header();
    gPostraceFinishState = finishState;
    if (is_in_two_player_adventure()) {
        set_scene_viewport_num(VIEWPORT_LAYOUT_1_PLAYER);
    }
    gPostRace1Player = FALSE;
    if (gNumberOfActivePlayers == 1 && gTrophyRaceWorldId == 0) {
        if (is_in_tracks_mode() == TRUE) {
            gResultOptionText[0] = gMenuText[ASSET_MENU_TEXT_TRYAGAIN];
            gResultOptionText[1] = gMenuText[ASSET_MENU_TEXT_SELECTTRACK];
            gResultOptionText[2] = gMenuText[ASSET_MENU_TEXT_SELECTCHARACTER];
            gResultOptionCount = 3;
            gPostRace1Player = TRUE;
        } else {
            if (finishState == FALSE) {
                gResultOptionText[0] = gMenuText[ASSET_MENU_TEXT_TRYAGAIN];
                gResultOptionCount = 1;
            } else {
                gResultOptionCount = 0;
            }
            gResultOptionText[gResultOptionCount] = gMenuText[ASSET_MENU_TEXT_RETURNTOLOBBY];
            gResultOptionCount++;
        }
        gResultOptionText[gResultOptionCount] = gMenuText[ASSET_MENU_TEXT_QUIT];
        gResultOptionCount++;
        gMenuStage = POSTRACE_STAGE_BEGIN;
    } else {
        gMenuStage = POSTRACE_STAGE_FADE_OUT;
    }
    gFileNew = FALSE;
    gOptionBlinkTimer = 0;
    gOpacityDecayTimer = 0;
    gMenuDelay = 0;
    gMenuCurIndex = 0;
    gMenuOption = 0;
    gIgnorePlayerInputTime = 1;
    gPostRace.unk0_s32 = -1;
    gMenuSubOption = 0;
    gPostRaceMessage = NULL;
    gTracksSaveGhost = FALSE;
    if (header->race_type & RACETYPE_CHALLENGE) {
        gIgnorePlayerInputTime = normalise_time(240); // 4 seconds
    }
    if (gPostraceFinishState) {
        gMenuStage = POSTRACE_STAGE_END;
        gMenuDelay = 100;
    }
    if (get_game_mode() != GAMEMODE_INGAME) {
        gMenuStage = POSTRACE_STAGE_FADE_OUT;
    }
    reset_controller_sticks();
    race_postrace_type(finishState);
    gTrackSelectViewPortX = fb_size();
    gTrackSelectViewportY = GET_VIDEO_HEIGHT(gTrackSelectViewPortX) & 0xFFFF;
    gTrackSelectViewPortX = GET_VIDEO_WIDTH(gTrackSelectViewPortX);
    gTrackSelectViewPortHalfX = gTrackSelectViewPortX >> 1;
    gTrackSelectViewPortHalfY = gTrackSelectViewportY >> 1;
    if (gNumberOfActivePlayers == 1 && gTrophyRaceWorldId == 0) {
        gPostRace.unk0_s32 = 0;
        worldID = header->world - 1;
        var_v1 = &gTracksMenuBgTextureIndices[worldID * 3];
        if (var_v1[0] != -1) {
            menu_asset_load(var_v1[0]);
            gMenuMosaic1 = gMenuAssets[var_v1[0]];
        } else {
            gMenuMosaic1 = 0;
        }
        if (var_v1[1] != -1) {
            menu_asset_load(var_v1[1]);
            gMenuMosaic2 = gMenuAssets[var_v1[1]];
        } else {
            gMenuMosaic2 = 0;
        }
        gMenuMosaicShift = var_v1[2];
        if (get_game_mode() == GAMEMODE_INGAME) {
            bgdraw_texture_init(gMenuMosaic1, gMenuMosaic2, gMenuMosaicShift);
        }
        camEnableUserView(0, TRUE);
        viewport_menu_set(0, 0, 0, gTrackSelectViewPortX, gTrackSelectViewportY);
    }
    sound_volume_change(VOLUME_LOWER_AMBIENT);
}

/**
 * Load and initialise the results display for the post-race screen.
 * These are the racer icons in finishing order.
 */
void postrace_load(void) {
    UNUSED s32 pad[2];
    Settings *settings;
    s16 temp_a0;
    s32 i;
    s32 j;

    if (gPostRace.unk0_s32 >= 0) {
        if (gPostRace.unk0_s32 < 10) {
            gPostRace.unk0_s32++;
            return;
        }
#ifdef AVOID_UB
        temp_a0 = gRaceResultsObjectIndices[gPostRace.unk0_s32 - 10];
#else // This relies on the data laid out as is in memory, since the var itself is only 2 entries big.
        temp_a0 = sPostRaceRowOffsets[gPostRace.unk0_s32];
#endif
        if (temp_a0 == -1) {
            menu_imagegroup_load(gRaceResultsImageIndices);
            menu_racer_portraits();
            settings = get_settings();
            gRaceResultsMenuElements->t.element = gRacerPortraits[settings->racers[settings->timeTrialRacer].character];
            if (is_time_trial_enabled() == FALSE) {
                for (i = 0; i < 8; i++) {
                    for (j = 0; j < 8; j++) {
                        if (i == settings->racers[j].starting_position) {
                            gRaceOrderMenuElements[7 - i].t.element = gRacerPortraits[settings->racers[j].character];
                        }
                    }
                }
                if (is_in_two_player_adventure()) {
                    for (i = 0; i < 6; i++) {
                        gRaceOrderMenuElements[i + 1].t.element = gRaceOrderMenuElements[i + 2].t.element;
                    }
                    gRaceOrderMenuElements[7].t.element = &sMenuNullElement;
                    gRaceOrderMenuElements[0].t.element = &sMenuNullElement;
                }
            }
            gPostRace.unk0_s32 = -1;
            return;
        }
        menu_asset_load(temp_a0);
        gPostRace.unk0_s32++;
    }
}

/**
 * Count up the post race timer.
 * Slowly fade out the current music, then fade in the menu music.
 */
void postrace_music_fade(s32 updateRate) {
    gOptionBlinkTimer = (gOptionBlinkTimer + updateRate) & 0x3F;
    gPostRaceTimer += updateRate;
    if (gOpacityDecayTimer >= 0) {
        gOpacityDecayTimer += updateRate;
        switch (gMenuCurIndex) {
            case 0:
                if (normalise_time(240) < gOpacityDecayTimer) {
                    music_fade(-256);
                    gMenuCurIndex = 1;
                }
                break;
            case 1:
                if (normalise_time(300) < gOpacityDecayTimer) {
                    music_voicelimit_set(24);
                    music_play(SEQUENCE_MAIN_MENU);
                    music_fade(256);
                }
                break;
        }
        if (normalise_time(300) < gOpacityDecayTimer) {
            gOpacityDecayTimer = -1;
        }
    }
}

/**
 * Resizes the viewport when shrinking it down.
 * Renders race order, lap times and text options beneath.
 */
void postrace_viewport(UNUSED s32 updateRate) {
    s32 temp;
    s32 var_s3;
    Settings *settings;
    s32 sp50;
    s32 var_s2;
    s32 var_s0;
    s32 i;
    s32 sp40;
    s32 filterColour;
    s32 var_v0;

    settings = get_settings();
    if (gNumberOfActivePlayers == 1) {
        mtx_ortho(&sMenuCurrDisplayList, &sMenuCurrHudMat);
    }
    camDisableUserView(0, TRUE);
    var_s3 = gOptionBlinkTimer * 8;
    if (var_s3 > 255) {
        var_s3 = 511 - var_s3;
    }
    switch (gMenuStage) {
        case 1:
            var_s0 = gPostRaceTimer;
            if (var_s0 > 60) {
                var_s0 = 60;
            }
            var_s2 = ((gTrackSelectViewPortHalfY - ((gTrackSelectViewPortHalfY * 4) / 5)) * var_s0) / 60;
            temp = var_s2;
            sp50 =
                gTrackSelectViewportY - (((gTrackSelectViewPortHalfY - (gTrackSelectViewPortHalfY / 5)) * var_s0) / 60);
            var_v0 = (var_s0 * 80) / 60;
            viewport_menu_set(0, var_v0, temp, SCREEN_WIDTH - var_v0, sp50);
            gMenuImages[4].trans.x_position = 0.0f;
            gMenuImages[4].trans.y_position = gTrackSelectViewPortHalfY - ((var_s2 + sp50) >> 1);
            gMenuImages[4].trans.scale = sMenuImageProperties[4].trans.scale * (2.0f - (var_s0 / 60.0f));
            break;
        case 2:
            for (i = 0; i < 3; i++) {
                if (settings->display_times && settings->racers[PLAYER_ONE].best_times & (1 << i)) {
                    gRaceResultsMenuElements[i + 3].filterGreen = 192 - ((var_s3 * 3) >> 2);
                    gRaceResultsMenuElements[i + 3].filterBlue = 255 - var_s3;
                } else {
                    gRaceResultsMenuElements[i + 3].filterGreen = 192;
                    gRaceResultsMenuElements[i + 3].filterBlue = 255;
                }
            }
            if (settings->display_times && settings->racers[0].best_times & (1 << 7)) {
                gRaceResultsMenuElements[6].filterRed = (var_s3 >> 1) + 128;
                gRaceResultsMenuElements[6].filterGreen = 255 - var_s3;
                gRaceResultsMenuElements[6].filterBlue = 255 - var_s3;
            } else {
                gRaceResultsMenuElements[6].filterRed = 128;
                gRaceResultsMenuElements[6].filterGreen = 255;
                gRaceResultsMenuElements[6].filterBlue = 255;
            }
            break;
        case 3:
            for (i = 0; i < ARRAY_COUNT(settings->racers); i++) {
                filterColour = 255;
                sp40 = i;
                if (is_in_two_player_adventure()) {
                    sp40--;
                    if (sp40 == settings->racers[1].starting_position) {
                        filterColour = (var_s3 >> 1) + 128;
                    }
                }
                if (sp40 == settings->racers[0].starting_position) {
                    filterColour = (var_s3 >> 1) + 128;
                }
                gRaceOrderMenuElements[7 - i].filterRed = filterColour;
                gRaceOrderMenuElements[7 - i].filterGreen = filterColour;
                gRaceOrderMenuElements[7 - i].filterBlue = filterColour;
            }
            break;
        case 5:
            if (settings->display_times && settings->racers[0].best_times & (s8) ~(1 << 7)) {
                gRecordTimesMenuElements[6].filterRed = 255;
                gRecordTimesMenuElements[6].filterGreen = 192 - ((var_s3 * 3) >> 2);
                gRecordTimesMenuElements[6].filterBlue = 255 - var_s3;
            } else {
                gRecordTimesMenuElements[6].filterRed = 255;
                gRecordTimesMenuElements[6].filterGreen = 192;
                gRecordTimesMenuElements[6].filterBlue = 255;
            }
            if (settings->display_times && settings->racers[0].best_times & (1 << 7)) {
                gRecordTimesMenuElements[3].filterRed = (var_s3 >> 1) + 128;
                gRecordTimesMenuElements[3].filterGreen = 255 - var_s3;
                gRecordTimesMenuElements[3].filterBlue = 255 - var_s3;
            } else {
                gRecordTimesMenuElements[3].filterRed = 128;
                gRecordTimesMenuElements[3].filterGreen = 255;
                gRecordTimesMenuElements[3].filterBlue = 255;
            }
            break;
        case 6:
            clear_dialogue_box_open_flag(7);
            dialogue_clear(7);
            set_dialogue_font(7, FONT_COLOURFUL);
            set_current_text_background_colour(7, 0, 0, 0, 0);

            if (gTracksSaveGhost != 0) {
                var_s0 = 1;
            } else if (gPostRaceMessage != NULL) {
                var_s0 = gPostRaceLineCount;
            } else {
                var_s0 = gResultOptionCount;
            }
            if (var_s0 >= 5) {
                var_v0 = 2;
                sp50 = 13;
            } else {
                var_v0 = 0;
                sp50 = 16;
            }
            var_s2 = ((var_s0 * sp50) + 1) >> 1;
            var_s0 = 192;
            if (osTvType == OS_TV_TYPE_PAL) {
                var_s0 = 218;
            }
            set_current_dialogue_box_coords(7, 0, var_s0 - var_s2 - var_v0 - 4, SCREEN_WIDTH,
                                            var_s0 + var_s2 + var_v0 + 4);
            set_current_dialogue_background_colour(7, 64, 64, 255, 0);
            set_current_text_colour(7, 255, 0, 255, 64, 255);
            if (gTracksSaveGhost != 0) {
                render_dialogue_text(7, POS_CENTRED, 12, gMenuText[ASSET_MENU_TEXT_PLEASEWAIT], 1, ALIGN_MIDDLE_CENTER);
            } else if (gPostRaceMessage != NULL) {
                var_s2 = 12;
                for (var_s0 = 0; var_s0 < gPostRaceLineCount; var_s0++) {
                    render_dialogue_text(7, POS_CENTRED, var_s2, gPostRaceMessage[var_s0], 1, ALIGN_MIDDLE_CENTER);
                    var_s2 += sp50;
                }
            } else {
                var_s2 -= 24;
                if (gMenuSubOption != 0) {
                    render_dialogue_text(7, POS_CENTRED, var_s2 + 8, gMenuText[ASSET_MENU_TEXT_QUITGAMETITLE], 1,
                                         ALIGN_MIDDLE_CENTER);
                    var_s0 = 0;
                    if (gMenuSubOption == 1) {
                        var_s0 = var_s3;
                    }
                    set_current_text_colour(7, 255, 255, 255, var_s0, 255);
                    render_dialogue_text(7, POS_CENTRED, var_s2 + 26, gMenuText[ASSET_MENU_TEXT_OK], 1,
                                         ALIGN_MIDDLE_CENTER);
                    var_s0 = 0;
                    if (gMenuSubOption == 2) {
                        var_s0 = var_s3;
                    }
                    set_current_text_colour(7, 255, 255, 255, var_s0, 255);
                    render_dialogue_text(7, POS_CENTRED, var_s2 + 42, gMenuText[ASSET_MENU_TEXT_CANCEL], 1,
                                         ALIGN_MIDDLE_CENTER);
                } else {
                    for (var_s2 = 12, var_s0 = 0; var_s0 < gResultOptionCount; var_s0++) {
                        if (var_s0 == gMenuOption) {
                            set_current_text_colour(7, 255, 255, 255, var_s3, 255);
                        } else {
                            set_current_text_colour(7, 255, 255, 255, 0, 255);
                        }
                        render_dialogue_text(7, SCREEN_WIDTH_HALF, var_s2, gResultOptionText[var_s0], 1,
                                             ALIGN_MIDDLE_CENTER);
                        var_s2 += sp50;
                    }
                }
            }
            open_dialogue_box(7);
            break;
    }
    if (get_game_mode() == GAMEMODE_INGAME && gNumberOfActivePlayers == 1) {
        if (gTrophyRaceWorldId == 0) {
            camEnableUserView(0, TRUE);
            if (osTvType == OS_TV_TYPE_PAL) {
                gTrackSelectWoodFrameHeightScale = 1.2f;
            }
            if (gMenuStage > 0) {
                menu_element_render(4);
            }
            gTrackSelectWoodFrameHeightScale = 1.0f;
        }
    }
}

/**
 * Set the postrace message depending on if a controller pak was detected.
 * This tells the player they can insert their rumble pak if they wish.
 */
void postrace_message(SIDeviceStatus status) {
    // status may contain controllerIndex in it's upper bits
    // so grab the lower bits for the SIDeviceStatus

    // clang-format off
    switch (status & 0xFF) {
        case CONTROLLER_PAK_NOT_FOUND:
        case CONTROLLER_PAK_CHANGED: /*Required to match*/ \
            gPostRaceMessage = sNoControllerPakMenuText;
            break;
        case CONTROLLER_PAK_FULL:
        case CONTROLLER_PAK_NO_ROOM_FOR_GHOSTS: /*Required to match*/ \
            gPostRaceMessage = sControllerPakFullMenuText;
            break;
        case CONTROLLER_PAK_RUMBLE_PAK_FOUND:
            // If you wish to use / the Controller Pak / insert it now!
            gPostRaceMessage = sInsertControllerPakMenuText;
            break;
        case CONTROLLER_PAK_SWITCH_TO_RUMBLE:
            // If you wish to use / the Rumble Pak / insert it now!
            gPostRaceMessage = sInsertRumblePakMenuText;
            break;
        case CONTROLLER_PAK_BAD_DATA:
            gPostRaceMessage = sCorruptDataMenuText;
            break;
        default:
            gPostRaceMessage = sBadControllerPakMenuText;
            break;
    }
    // clang-format on

    for (gPostRaceLineCount = 0; gPostRaceMessage[gPostRaceLineCount] != 0; gPostRaceLineCount++) {}
}

/**
 * Handle the shrunken viewport when the race ends.
 * This includes the options and results of the race, depending on mode.
 */
s32 menu_postrace(Gfx **dList, Mtx **matrices, Vertex **vertices, s32 updateRate) {
    s32 sp54;
    s32 sp50;
    s32 sp4C;
    s32 i;
    s32 ret;
    s32 buttonsPressed;
    s32 prevMenuOption;
    s32 textOffsetY;
    s32 timeOffsetY;
    s32 numPlayers;
    Settings *settings;
    Racer *racer;
    char *chosenOption;

    sp54 = 0;
    sp50 = 0;
    sp4C = 0;
    ret = POSTRACE_NONE;
    sMenuCurrDisplayList = *dList;
    sMenuCurrHudMat = *matrices;
    sMenuCurrHudVerts = *vertices;
    settings = get_settings();
    if (osTvType == OS_TV_TYPE_PAL) {
        textOffsetY = 26;
        timeOffsetY = 12;
    } else {
        textOffsetY = 0;
        timeOffsetY = 0;
    }
    numPlayers = gNumberOfActivePlayers;
    if (is_in_two_player_adventure()) {
        numPlayers = 2;
    }
    if (gPostRace1Player) {
        if (has_ghost_to_save()) {
            gResultOptionText[gResultOptionCount - 1] = gMenuText[ASSET_MENU_TEXT_SAVEGHOST];
            gResultOptionText[gResultOptionCount] = gMenuText[ASSET_MENU_TEXT_QUIT];
            gResultOptionCount++;
        }
        gPostRace1Player = FALSE;
    }
    postrace_load();
    postrace_music_fade(updateRate);
    if (gMenuDelay < 20 && gPostraceFinishState == 0) {
        if (gPostRace.unk0_s32 < 0) {
            postrace_viewport(updateRate);
        }
    } else {
        bgdraw_texture_init(NULL, NULL, 0);
    }
    update_controller_sticks();
    buttonsPressed = 0;
    if (gIgnorePlayerInputTime == FALSE && gPostRace.unk0_s32 < 0) {
        for (i = 0; i < numPlayers; i++) {
            buttonsPressed |= input_pressed(i);
        }
    }
    switch (gMenuStage) {
        case POSTRACE_STAGE_BEGIN:
            if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
                gMenuStage = POSTRACE_STAGE_SHRINK_VIEWPORT;
                gPostRaceTimer = 0;
            }
            break;
        case POSTRACE_STAGE_SHRINK_VIEWPORT:
            if (gPostRaceTimer > 60 || buttonsPressed & (A_BUTTON | START_BUTTON)) {
                viewport_menu_set(0, 80, gTrackSelectViewPortHalfY - ((gTrackSelectViewPortHalfY * 4) / 5),
                                  SCREEN_HEIGHT, (gTrackSelectViewPortHalfY / 5) + gTrackSelectViewPortHalfY);
                gMenuImages[4].trans.x_position = 0.0f;
                gMenuImages[4].trans.y_position = 36.0f;
                gMenuImages[4].trans.scale = sMenuImageProperties[4].trans.scale;
                if (leveltable_type(settings->courseId) & RACETYPE_CHALLENGE) {
                    gMenuStage = POSTRACE_STAGE_OPTIONS;
                } else if (!settings->display_times) {
                    postrace_offsets(gRaceOrderMenuElements, 0.5f, 15.0f, 0.5f, textOffsetY, timeOffsetY);
                    gMenuStage = POSTRACE_STAGE_RACE_ORDER;
                } else {
                    racer = &settings->racers[settings->timeTrialRacer];
                    gRaceResultsMenuElements[3].t.numberU16 = &racer->lap_times[0];
                    gRaceResultsMenuElements[4].t.numberU16 = &racer->lap_times[1];
                    gRaceResultsMenuElements[5].t.numberU16 = &racer->lap_times[2];
                    gRaceResultsMenuElements[6].t.numberU16 = &racer->course_time;
                    postrace_offsets(gRaceResultsMenuElements, 0.5f, 15.0f, 0.5f, textOffsetY, timeOffsetY);
                    gMenuStage = POSTRACE_STAGE_RACE_TIMES;
                }
            }
            break;
        case POSTRACE_STAGE_RACE_TIMES:
            if (postrace_render(updateRate) != 0) {
                if (gTrophyRaceWorldId != 0) {
                    music_fade(-128);
                    transition_begin(&sMenuTransitionFadeIn);
                    gMenuStage = POSTRACE_STAGE_END;
                } else if (settings->display_times) {
                    gRecordTimesMenuElements[3].t.numberU16 =
                        &settings->courseTimesPtr[*gPlayerSelectVehicle][settings->courseId];
                    gRecordTimesMenuElements[6].t.numberU16 =
                        &settings->flapTimesPtr[*gPlayerSelectVehicle][settings->courseId];
                    filename_decompress(settings->courseInitialsPtr[*gPlayerSelectVehicle][settings->courseId],
                                        gCourseInitials, MAX_INITIALS_LENGTH);
                    filename_decompress(settings->flapInitialsPtr[*gPlayerSelectVehicle][settings->courseId],
                                        gFLapInitials, MAX_INITIALS_LENGTH);
                    if (settings->racers[0].best_times != 0) {
                        gMenuStage = POSTRACE_STAGE_ENTER_INITIALS;
                        if (gIsInTracksMode == FALSE && sFilenameEntryPending != FALSE) {
                            filename_decompress(settings->filename, gCheckAdvEnterInitials, MAX_INITIALS_LENGTH);
                            sFilenameEntryPending = FALSE;
                        }
                        filename_init(textOffsetY + 196, 160, textOffsetY + 120, 2, &sFilenameEntryArg, gCheckAdvEnterInitials,
                                      MAX_INITIALS_LENGTH);
                    } else {
                        postrace_offsets(gRecordTimesMenuElements, 0.5f, 15.0f, 0.5f, textOffsetY, timeOffsetY);
                        gMenuStage = POSTRACE_STAGE_RACE_RECORDS;
                    }
                } else {
                    gMenuStage = POSTRACE_STAGE_RACE_ORDER;
                    postrace_offsets(gRaceOrderMenuElements, 0.5f, 15.0f, 0.5f, textOffsetY, timeOffsetY);
                }
            }
            break;
        case POSTRACE_STAGE_RACE_ORDER:
            if (postrace_render(updateRate)) {
                gMenuStage = POSTRACE_STAGE_OPTIONS;
            }
            break;
        case POSTRACE_STAGE_ENTER_INITIALS:
            if (filename_enter(updateRate)) {
                if (settings->racers[0].best_times & 0x7F) {
                    settings->flapInitialsPtr[*gPlayerSelectVehicle][settings->courseId] =
                        filename_compress(gCheckAdvEnterInitials, MAX_INITIALS_LENGTH);
                    filename_decompress(settings->flapInitialsPtr[*gPlayerSelectVehicle][settings->courseId],
                                        gFLapInitials, MAX_INITIALS_LENGTH);
                }
                if (settings->racers[0].best_times & 0x80) {
                    settings->courseInitialsPtr[*gPlayerSelectVehicle][settings->courseId] =
                        filename_compress(gCheckAdvEnterInitials, MAX_INITIALS_LENGTH);
                    filename_decompress(settings->courseInitialsPtr[*gPlayerSelectVehicle][settings->courseId],
                                        gCourseInitials, MAX_INITIALS_LENGTH);
                }
                postrace_offsets(gRecordTimesMenuElements, 0.5f, 15.0f, 0.5f, textOffsetY, timeOffsetY);
                gMenuStage = POSTRACE_STAGE_RACE_RECORDS;
            }
            break;
        case POSTRACE_STAGE_RACE_RECORDS:
            if (postrace_render(updateRate)) {
                gMenuStage = POSTRACE_STAGE_OPTIONS;
            }
            break;
        case POSTRACE_STAGE_OPTIONS:
            if (gTracksSaveGhost) {
                gTracksSaveGhost++;
                if (gTracksSaveGhost >= 5) {
                    sound_volume_change(3);
                    sp54 = timetrial_save_player_ghost(0);
                    if ((sp54 & 0xFF) == 5) {
                        sp54 = timetrial_save_player_ghost(0);
                    }
                    sound_volume_change(2);
                    if (sp54) {
                        postrace_message(sp54);
                        sp54 = 1;
                    } else {
                        if (gPostRaceMessage == sInsertControllerPakMenuText) {
                            postrace_message(CONTROLLER_PAK_SWITCH_TO_RUMBLE);
                        } else {
                            gPostRaceMessage = NULL;
                        }
                        gResultOptionText[gMenuOption] = gMenuText[28];
                        gMenuOption = 0;
                        gResultOptionCount -= 1;
                    }
                    gTracksSaveGhost = FALSE;
                }
            } else if (gPostRaceMessage != NULL) {
                if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
                    sp50 = 1;
                    if (gPostRaceMessage == sInsertControllerPakMenuText) {
                        gTracksSaveGhost = TRUE;
                    } else {
                        gPostRaceMessage = NULL;
                    }
                } else if (buttonsPressed & B_BUTTON) {
                    gPostRaceMessage = NULL;
                    sp54 = 1;
                }
            } else {
                if (gMenuSubOption != 0) {
                    if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
                        if (gMenuSubOption == 1) {
                            sp50 = 1;
                            music_fade(-128);
                            transition_begin(&sMenuTransitionFadeIn);
                            gMenuStage = POSTRACE_STAGE_END;
                        } else {
                            sp54 = 1;
                            gMenuSubOption = 0;
                        }
                    } else {
                        prevMenuOption = gMenuSubOption;
                        if (buttonsPressed & B_BUTTON) {
                            sp54 = 1;
                            gMenuSubOption = 0;
                        } else {
                            for (i = 0; i < numPlayers; i++) {
                                if ((gControllersYAxisDirection[i] > 0) && (gMenuSubOption == 2)) {
                                    gMenuSubOption = 1;
                                }
                                if ((gControllersYAxisDirection[i] < 0) && (gMenuSubOption == 1)) {
                                    gMenuSubOption = 2;
                                }
                            }
                            if (prevMenuOption != gMenuSubOption) {
                                sp4C = 1;
                            }
                        }
                    }
                } else if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
                    sp50 = 1;
                    chosenOption = gResultOptionText[gMenuOption];
                    if (chosenOption == gMenuText[ASSET_MENU_TEXT_SAVEGHOST]) {
                        gTracksSaveGhost = TRUE;
                    } else if (chosenOption == gMenuText[ASSET_MENU_TEXT_QUIT]) {
                        gMenuSubOption = 2;
                    } else {
                        music_fade(-128);
                        transition_begin(&sMenuTransitionFadeIn);
                        gMenuStage = POSTRACE_STAGE_END;
                    }
                } else {
                    prevMenuOption = gMenuOption;
                    for (i = 0; i < numPlayers; i++) {
                        if (gControllersYAxisDirection[i] < 0) {
                            if (gMenuOption < (gResultOptionCount - 1)) {
                                gMenuOption++;
                            }
                        }
                        if (gControllersYAxisDirection[i] > 0) {
                            if (gMenuOption > 0) {
                                gMenuOption--;
                            }
                        }
                    }
                    if (prevMenuOption != gMenuOption) {
                        sp4C = 1;
                    }
                }
            }
            if (sp54 != 0) {
                sound_play(SOUND_MENU_BACK3, NULL);
            } else if (sp50 != 0) {
                sound_play(SOUND_SELECT2, NULL);
            } else if (sp4C != 0) {
                sound_play(SOUND_MENU_PICK2, NULL);
            }
            break;
        case POSTRACE_STAGE_FADE_OUT:
            if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
                music_fade(-128);
                transition_begin(&sMenuTransitionFadeIn);
                gMenuStage = POSTRACE_STAGE_END;
            }
            break;
        case POSTRACE_STAGE_END:
            gMenuDelay += updateRate;
            if (gMenuDelay > 30) {
                if (settings->display_times) {
                    if (settings->racers[0].best_times & 0x7F) {
                        mark_to_write_flap_times();
                    }
                    if (settings->racers[0].best_times & 0x80) {
                        mark_to_write_course_times();
                    }
                }
                camDisableUserView(0, FALSE);
                postrace_free();
                dialogue_close(7);
                dialogue_clear(7);
                if (gPostraceFinishState >= 2) {
                    if (gPostraceFinishState == 9) {
                        cinematic_start((s8 *) get_misc_asset(ASSET_MISC_CINEMATIC_RACE), gPostraceFinishState,
                                        MENU_RESULT_FLAGS_100 | MENU_RESULT_FLAGS_2 | MENU_RESULT_FLAGS_4 |
                                            MENU_RESULT_FLAGS_8,
                                        0, 0, NULL);
                    } else {
                        cinematic_start((s8 *) get_misc_asset(ASSET_MISC_CINEMATIC_RACE), gPostraceFinishState,
                                        MENU_RESULT_FLAGS_100 | MENU_RESULT_FLAGS_1, 0, 0, NULL);
                    }
                    ret = POSTRACE_OPT_13;
                } else if (gTrophyRaceWorldId != 0) {
                    ret = POSTRACE_OPT_10;
                } else if (gNumberOfActivePlayers >= 2) {
                    ret = POSTRACE_OPT_8;
                } else {
                    chosenOption = gResultOptionText[gMenuOption];
                    if (chosenOption == gMenuText[ASSET_MENU_TEXT_TRYAGAIN]) {
                        if (!gIsInTracksMode && gTTVoiceLines[get_ingame_map_id()] != -1) {
                            sound_play_delayed(gTTVoiceLines[get_ingame_map_id()], NULL, 1.0f);
                        }
                        ret = POSTRACE_OPT_2;
                    } else if (chosenOption == gMenuText[ASSET_MENU_TEXT_SELECTTRACK]) {
                        ret = POSTRACE_OPT_5;
                    } else if (chosenOption == gMenuText[ASSET_MENU_TEXT_SELECTCHARACTER]) {
                        ret = POSTRACE_OPT_12;
                    } else if (chosenOption == gMenuText[ASSET_MENU_TEXT_RETURNTOLOBBY]) {
                        ret = POSTRACE_OPT_1;
                    } else {
                        ret = POSTRACE_OPT_4;
                    }
                }
            }
            break;
    }
    if (gIgnorePlayerInputTime > 0) {
        gIgnorePlayerInputTime -= updateRate;
        if (gIgnorePlayerInputTime < 0) {
            gIgnorePlayerInputTime = 0;
        }
    }
    *dList = sMenuCurrDisplayList;
    *matrices = sMenuCurrHudMat;
    *vertices = sMenuCurrHudVerts;
    return ret;
}

/**
 * Free all assets associated with the postrace screen.
 */
void postrace_free(void) {
    s32 headerWorldTex;
    LevelHeader *header;

    header = level_header();
    menu_assetgroup_free(gRaceResultsObjectIndices);
    headerWorldTex = header->world - 1;

    if (gMenuMosaic1) {
        menu_asset_free(gTracksMenuBgTextureIndices[headerWorldTex * 3]);
    }
    gMenuMosaic1 = NULL;

    if (gMenuMosaic2) {
        menu_asset_free(gTracksMenuBgTextureIndices[(headerWorldTex * 3) + 1]);
    }
    gMenuMosaic2 = NULL;

    menu_unload_bigfont();
    sound_volume_change(VOLUME_NORMAL);
}

/**
 * Initialise the race results menu.
 * Set the text for the options and set the order of the racer portraits based on finishing position.
 */
void menu_results_init(void) {
    Settings *settings;
    u16 *placements;
    s32 i;
    s8 startPos;

    settings = get_settings();
    gResultOptionText[0] = gMenuText[ASSET_MENU_TEXT_SELECTTRACK];
    gResultOptionText[1] = gMenuText[ASSET_MENU_TEXT_TRYAGAIN];
    gResultOptionText[2] = gMenuText[ASSET_MENU_TEXT_QUIT];
    gResultOptionCount = 3;

    for (i = 0; i < gNumberOfActivePlayers; i++) {
        startPos = settings->racers[i].starting_position;
        if (startPos < 4) {
            placements = settings->racers[i].placements;
            placements[startPos]++;
        }
    }

    gMenuStage = RESULTS_ENTER;
    gOptionBlinkTimer = 0;
    gOpacityDecayTimer = 0;
    gMenuDelay = 0;
    gMenuOption = 0;
    gIgnorePlayerInputTime = 1;
    gMenuSubOption = 0;
    menu_assetgroup_load(gRaceResultsObjectIndices);
    menu_imagegroup_load(gRaceResultsImageIndices);
    menu_racer_portraits();
    load_font(ASSET_FONTS_BIGFONT);
    transition_begin(&sMenuTransitionFadeOut);
    music_voicelimit_set(24);
    music_play(SEQUENCE_MAIN_MENU);
    music_fade(128);
}

/**
 * Draw the portraits of the four player onscreen, then draw the scoreboard below.
 * After, draw the text options at the bottom.
 */
void results_render(UNUSED s32 updateRate, f32 opacity) {
    s32 x2;
    s32 y2;
    s32 offsetX;
    s32 spA0;
    s32 time;
    s32 offsetY;
    s32 hundredths;
    s32 tens;
    UNUSED s32 pad;
    s32 i;
#if VERSION >= VERSION_79
    s32 offsetX2;
    s32 timesToLoop;
#endif
    Settings *settings;

    settings = get_settings();
    offsetY = 0;
    if (osTvType == OS_TV_TYPE_PAL) {
        offsetY = 12;
    }
    mtx_ortho(&sMenuCurrDisplayList, &sMenuCurrHudMat);
    if (opacity < 0.0f) {
        opacity = 0.0f;
    }
    if (opacity > 1.0f) {
        opacity = 1.0f;
    }
    sMenuGuiOpacity = (1 - opacity) * 255;
    set_text_font(ASSET_FONTS_BIGFONT);
    set_text_background_colour(0, 0, 0, 0);
    set_text_colour(255, 255, 255, 0, sMenuGuiOpacity);
    draw_text(&sMenuCurrDisplayList, POS_CENTRED, 34, gMenuText[ASSET_MENU_TEXT_RANKINGS], ALIGN_MIDDLE_CENTER);
    sMenuGuiOpacity = 255;
#if VERSION >= VERSION_79
    offsetX2 = 64;
    if (gNumberOfActivePlayers == 4) {
        offsetX2 = 56;
    }
    offsetX = 160 - ((gNumberOfActivePlayers - 1) * (offsetX2 >> 1));
#else
    offsetX = 160 - ((gNumberOfActivePlayers - 1) << 5);
#endif
    x2 = offsetX;

    for (i = 0; i < gNumberOfActivePlayers; i++) {
        if (settings->racers[i].starting_position == 0) {
            if (gOptionBlinkTimer < 32) {
                spA0 = (gOptionBlinkTimer * 4) + 128;
            } else {
                spA0 = 0x17F - (gOptionBlinkTimer * 4);
            }
        } else {
            spA0 = 255;
        }
        texrect_draw(&sMenuCurrDisplayList, gRacerPortraits[settings->racers[i].character], x2 - 20,
                     54 - (s32) (240 * opacity), spA0, spA0, spA0, 255);
#if VERSION >= VERSION_79
        x2 += offsetX2;
#else
        x2 += 64;
#endif
    }

    offsetX = offsetX + (s32) (320 * opacity); // Cannot use += here?
    set_text_font(ASSET_FONTS_FUNFONT);
    y2 = 104;
#if VERSION >= VERSION_79
    timesToLoop = 4;
    for (spA0 = 0; spA0 < timesToLoop; spA0++) {
#else
    for (spA0 = 0; spA0 < 4; spA0++) {
#endif
        time = offsetX;
        x2 = offsetX;
        set_text_colour(0, 0, 0, 255, 255);
        draw_text(&sMenuCurrDisplayList, time - 32, y2 + offsetY + 4, gRacePlacementsArray[spA0], ALIGN_MIDDLE_CENTER);
        set_text_colour(255, 255, 192, 96, 255);
        draw_text(&sMenuCurrDisplayList, time - 34, y2 + offsetY + 2, gRacePlacementsArray[spA0], ALIGN_MIDDLE_CENTER);
        rendermode_reset(&sMenuCurrDisplayList);
        cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
        sprite_opaque(FALSE);
        sMenuGuiColourR = 255;
        sMenuGuiColourG = 255 - 64 * spA0;
        sMenuGuiColourB = 255;
        sMenuGuiColourBlendFactor = 255;
#if VERSION >= VERSION_79
        for (i = 0; i < gNumberOfActivePlayers; i++, x2 += offsetX2) {
#else
        for (i = 0; i < gNumberOfActivePlayers; i++, x2 += 64) {
#endif
            time = settings->racers[i].placements[spA0]; // Is this local var name correct?
            if (time > 999) {
                time = 999;
            }
            hundredths = time / 100;
            time -= (hundredths * 100);
            tens = time / 10;
            time -= tens * 10;

            // I'm 99% sure these need to be gMenuImages[0]
            // Changing it to gMenuImages[spA0] or gMenuImages[i] makes the score worse.
            gMenuImages[0].trans.y_position = (SCREEN_HEIGHT_HALF - y2);
            if (hundredths > 0) {
                gMenuImages[0].trans.x_position = (x2 - 172);
                gMenuImages[0].spriteOffset = hundredths;
                menu_element_render(0);
                gMenuImages[0].trans.x_position += 12.0f;
                gMenuImages[0].spriteOffset = tens;
                menu_element_render(0);
                gMenuImages[0].trans.x_position += 12.0f;
            } else if (tens > 0) {
                gMenuImages[0].trans.x_position = (x2 - 166);
                gMenuImages[0].spriteOffset = tens;
                menu_element_render(0);
                gMenuImages[0].trans.x_position += 12.0f;
            } else {
                gMenuImages[0].trans.x_position = (x2 - 160);
            }
            gMenuImages[0].spriteOffset = time;
            menu_element_render(0);
        }
        y2 += 17;
        cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
        sprite_opaque(TRUE);
        sMenuGuiColourG = 255;
        sMenuGuiColourBlendFactor = 0;
    }
    if (gMenuStage >= RESULTS_OPTIONS) {
        y2 = gResultOptionCount * 8;
        clear_dialogue_box_open_flag(7);
        dialogue_clear(7);
        set_current_dialogue_box_coords(7, SCREEN_WIDTH_HALF - 80, (offsetY - y2) + 196, SCREEN_WIDTH_HALF + 80,
                                        y2 + offsetY + (SCREEN_HEIGHT - 36));
        set_current_dialogue_background_colour(7, 64, 64, 255, 0);
        set_dialogue_font(7, 0);
        set_current_text_background_colour(7, 0, 0, 0, 0);
        spA0 = gOptionBlinkTimer * 8;
        if (spA0 > 255) {
            spA0 = 511 - spA0;
        }
        if (gMenuSubOption != 0) {
            y2 -= 24;
            render_dialogue_text(7, POS_CENTRED, y2 + 8, gMenuText[ASSET_MENU_TEXT_QUITGAMETITLE], 1,
                                 ALIGN_MIDDLE_CENTER);
            if (gMenuSubOption == 1) {
                set_current_text_colour(7, 255, 255, 255, spA0, 255);
            } else {
                set_current_text_colour(7, 255, 255, 255, 0, 255);
            }
            render_dialogue_text(7, POS_CENTRED, y2 + 0x1A, gMenuText[ASSET_MENU_TEXT_OK], 1, ALIGN_MIDDLE_CENTER);
            if (gMenuSubOption == 2) {
                set_current_text_colour(7, 255, 255, 255, spA0, 255);
            } else {
                set_current_text_colour(7, 255, 255, 255, 0, 255);
            }
            render_dialogue_text(7, POS_CENTRED, y2 + 0x2A, gMenuText[ASSET_MENU_TEXT_CANCEL], 1, ALIGN_MIDDLE_CENTER);
        } else {

            for (i = 0, y2 = 12; i < gResultOptionCount;) {
                if (i == gMenuOption) {
                    set_current_text_colour(7, 255, 255, 255, spA0, 255);
                } else {
                    set_current_text_colour(7, 255, 255, 255, 0, 255);
                }
                render_dialogue_text(7, 80, y2, gResultOptionText[i], 1, ALIGN_MIDDLE_CENTER);
                i++;
                y2 += 16;
            }
        }
        open_dialogue_box(7);
    }
}

/**
 * When someone presses A, decide whether to play the stage again,
 * quit the game or return to the tracks menu.
 */
s32 menu_results_loop(s32 updateRate) {
    s32 playBackSound;
    s32 playSelectSound;
    s32 playPickSound;
    s32 prevOption;

    playBackSound = FALSE;
    playSelectSound = FALSE;
    playPickSound = FALSE;
    gOptionBlinkTimer = (gOptionBlinkTimer + updateRate) & 0x3F;
    menu_input();
    if (gMenuStage <= RESULTS_SCORE) {
        gOpacityDecayTimer += updateRate;
        if (gOpacityDecayTimer >= 60) {
            gMenuStage = RESULTS_OPTIONS;
        } else if (gMenuStage < 0 && gOpacityDecayTimer > 20) {
            gMenuStage = RESULTS_SCORE;
            sound_play(SOUND_WHOOSH1, NULL);
        }
    }

    if (gMenuDelay < 20) {
        if (gMenuStage <= RESULTS_SCORE) {
            if (gOpacityDecayTimer >= 20) {
                results_render(updateRate, 1.0f - ((f32) (gOpacityDecayTimer - 20) / 40.0f));
            }
        } else {
            results_render(updateRate, 0.0f);
        }
    }
    if (gMenuDelay == 0) {
        if (gMenuStage == RESULTS_SCORE) {
            if (gMenuButtons[PLAYER_MENU] & (A_BUTTON | START_BUTTON)) {
                gMenuStage = RESULTS_OPTIONS;
            }
        } else if (gMenuSubOption != 0) {
            if (gMenuButtons[PLAYER_MENU] & (A_BUTTON | START_BUTTON)) {
                if (gMenuSubOption == 1) {
                    playSelectSound = TRUE;
                    music_fade(-128);
                    transition_begin(&sMenuTransitionFadeIn);
                    gMenuDelay = 1;
                } else {
                    playBackSound = TRUE;
                    gMenuSubOption = 0;
                }
            } else if (gMenuButtons[PLAYER_MENU] & B_BUTTON) {
                playBackSound = TRUE;
                gMenuSubOption = 0;
            } else {
                prevOption = gMenuSubOption;
                if (gMenuStickY[PLAYER_MENU] > 0 && gMenuSubOption == 2) {
                    gMenuSubOption = 1;
                }
                if (gMenuStickY[PLAYER_MENU] < 0 && gMenuSubOption == 1) {
                    gMenuSubOption = 2;
                }
                if (prevOption != gMenuSubOption) {
                    playPickSound = TRUE;
                }
            }
        } else if (gMenuButtons[PLAYER_MENU] & (A_BUTTON | START_BUTTON)) {
            playSelectSound = TRUE;
            if (gResultOptionText[gMenuOption] == gMenuText[ASSET_MENU_TEXT_QUIT]) {
                gMenuSubOption = 2;
            } else {
                gMenuDelay = 1;
                transition_begin(&sMenuTransitionFadeIn);
                music_fade(-128);
            }
        } else {
            prevOption = gMenuOption;
            if (gMenuStickY[PLAYER_MENU] < 0 && gMenuOption < (gResultOptionCount - 1)) {
                gMenuOption++;
            }
            if (gMenuStickY[PLAYER_MENU] > 0 && gMenuOption > 0) {
                gMenuOption--;
            }
            if (prevOption != gMenuOption) {
                playPickSound = TRUE;
            }
        }
        if (playBackSound) {
            sound_play(SOUND_MENU_BACK3, NULL);
        } else if (playSelectSound) {
            sound_play(SOUND_SELECT2, NULL);
        } else if (playPickSound) {
            sound_play(SOUND_MENU_PICK2, NULL);
        }

    } else {
        gMenuDelay += updateRate;
        if (gMenuDelay > 30) {
            results_free();
            dialogue_close(7);
            dialogue_clear(7);
            if (gResultOptionText[gMenuOption] == gMenuText[ASSET_MENU_TEXT_TRYAGAIN]) {
                return (MENU_RESULT_FLAGS_100 | MENU_RESULT_FLAGS_2); // This gets parsed in mode_menu as a flag
                                                                      // and an ID from the bottom 7 bits.
            }
            if (gResultOptionText[gMenuOption] == gMenuText[ASSET_MENU_TEXT_SELECTTRACK]) {
                load_level_for_menu(SPECIAL_MAP_ID_NO_LEVEL, -1, 0);
                menu_init(MENU_TRACK_SELECT);
                return MENU_RESULT_CONTINUE;
            }
            return (MENU_RESULT_FLAGS_100 | MENU_RESULT_FLAGS_4);
        }
    }
    gIgnorePlayerInputTime = 0;
    return MENU_RESULT_CONTINUE;
}

/**
 * Frees any assets associated with the results page.
 */
void results_free(void) {
    menu_assetgroup_free(gRaceResultsObjectIndices);
    unload_font(ASSET_FONTS_BIGFONT);
}
