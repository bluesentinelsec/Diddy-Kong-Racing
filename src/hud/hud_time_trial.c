#include "hud_internal.h"

static u32 gHudLapColours[3] = {
    COLOUR_RGBA32(255, 40, 40, 254),
    COLOUR_RGBA32(40, 255, 40, 254),
    COLOUR_RGBA32(40, 40, 255, 254),
};

#if REGION == REGION_JP
// ゴースト  -  Ghost
static char gHudTimeTrialJp_Ghost[] = { 0x80, 0x80, 0x80, 0x3B, 0x80, 0x5C, 0x80, 0x63, 0x00, 0x00 };

// セーブ  -  Save
static char gHudTimeTrialJp_Save[] = { 0x80, 0x5D, 0x80, 0x3B, 0x80, 0x8D };

// ふかのう - Impossible
static char gHudTimeTrialJp_Impossible[] = { 0x80, 0xD5, 0x80, 0xBF, 0x80, 0xD2, 0x80, 0xBC, 0x00, 0x00 };

static char gHudTimeTrialJp_FullBlank[] = { 0x00, 0x00, 0x00, 0x00 };

// パック  -  Pak
static char gHudTimeTrialJp_FullPak[] = { 0x80, 0x90, 0x80, 0x4A, 0x80, 0x57 };

// フル  -  Full
static char gHudTimeTrialJp_Full[] = { 0x80, 0x6B, 0x80, 0x78, 0x00, 0x00 };

static char gHudTimeTrialJp_DamagedBlank[] = { 0x00, 0x00, 0x00, 0x00 };

// パック  -  Pak
static char gHudTimeTrialJp_DamagedPak[] = { 0x80, 0x90, 0x80, 0x4A, 0x80, 0x57 };

// ふりょう  - Defective
static char gHudTimeTrialJp_Defective[] = { 0x80, 0xD5, 0x80, 0xE1, 0x80, 0xB7, 0x80, 0xBC, 0x00, 0x00 };
#endif

static UNUSED f32 sRecordVel = 0.0f; // Set to whatever the highest velocity recorded is, but never actually used.

void hud_main_time_trial(s32 arg0, Object *playerRacerObj, s32 updateRate) {
    s32 i;
    s32 spB8;
    s32 spB4;
    s32 spB0;
    s32 spAC;
    s32 stopwatchTimer;
    s32 temp_lo;
    Object_Racer *curRacer;
    Object *ttSWBodyObject;
    ModelInstance *modInst;
    LevelObjectEntryCommon ttSWBody;
    LevelObjectEntryCommon ttSWArms;
    UNUSED s32 pad;
    UNUSED s32 pad2;
    s32 temp_t6;
    f32 animcationFraction;
    f32 posX;
    f32 posY;
    f32 posZ;
    u16 soundID;

    curRacer = playerRacerObj->racer;
    stopwatchTimer = 0;
    if (gAssetHudElements->entry[HUD_BALLOON_COUNT_X] == NULL) {
        ttSWBody.objectID = gAssetHudElementIds[HUD_BALLOON_COUNT_X] & 0xFFFF;
        ttSWBody.size = sizeof(LevelObjectEntryCommon);
        ttSWBody.x = 0;
        ttSWBody.y = 0;
        ttSWBody.z = 0;
        gAssetHudElements->entry[HUD_BALLOON_COUNT_X] = spawn_object(&ttSWBody, OBJECT_SPAWN_NONE);
        gCurrentHud->entry[HUD_STOPWATCH_HANDS].rotation.y_rotation = -0x8000;
        if (gAssetHudElements->entry[HUD_BALLOON_COUNT_X] != NULL) {
            ((Object *) gAssetHudElements->entry[HUD_BALLOON_COUNT_X])->animFrame = 0;
        }
    }

    if (gAssetHudElements->entry[HUD_STOPWATCH] == NULL) {
        ttSWArms.objectID = gAssetHudElementIds[HUD_STOPWATCH] & 0xFFFF;
        ttSWArms.size = sizeof(LevelObjectEntryCommon);
        ttSWArms.x = 0;
        ttSWArms.y = 0;
        ttSWArms.z = 0;
        gAssetHudElements->entry[HUD_STOPWATCH] = spawn_object(&ttSWArms, OBJECT_SPAWN_NONE);
        gCurrentHud->entry[HUD_STOPWATCH].rotation.y_rotation = -0x8000;
    }

    ttSWBodyObject = gAssetHudElements->entry[HUD_BALLOON_COUNT_X];
    if (ttSWBodyObject == NULL) {
        return;
    }

    ttSWBodyObject->animationID = gStopwatchAnimID;
    modInst = ttSWBodyObject->modelInstances[0];
    modInst->objModel->texOffsetUpdateRate = updateRate;
    if (gStopwatchAnimID != 0xFF) {
        if (gStopwatchAnimID == 4 && (get_race_countdown() || !music_is_playing())) {
            ttSWBodyObject->animFrame = 16;
        } else if (gStopwatchAnimID == 4) {
            animcationFraction = music_animation_fraction();
            temp_t6 = (modInst->objModel->animations[gStopwatchAnimID].animLength - 1) << 4;
            if (animcationFraction == -1.0) {
                ttSWBodyObject->animFrame = 0.0;
            } else if (animcationFraction > 0.5) {
                animcationFraction -= 0.5;
                animcationFraction *= 2.0;
                ttSWBodyObject->animFrame = temp_t6 - (animcationFraction * temp_t6);
            } else {
                animcationFraction *= 2;
                ttSWBodyObject->animFrame = temp_t6 * animcationFraction;
            }
        } else if (is_game_paused() == FALSE) {
            if (D_80126D69 < 0) {
                if (ttSWBodyObject->animFrame > -(updateRate * D_80126D69)) {
                    ttSWBodyObject->animFrame += (updateRate * D_80126D69);
                } else {
                    ttSWBodyObject->animFrame = 1;
                    D_80126D69 = -D_80126D69;
                }
            } else {
                ttSWBodyObject->animFrame += (updateRate * D_80126D69);
                temp_lo = (modInst->objModel->animations[gStopwatchAnimID].animLength - 1) << 4;
                if (ttSWBodyObject->animFrame >= temp_lo) {
                    if (sHudMapVisible != 0) {
                        D_80126D69 = -D_80126D69;
                        ttSWBodyObject->animFrame = temp_lo - 1;
                    } else {
                        ttSWBodyObject->animFrame = 0;
                        if (curRacer->wrongWayCounter == 0 && curRacer->raceFinished == FALSE) {
                            hud_stopwatch_face(4, 0, 4, 1, 1);
                        }
                    }
                }
            }
        }
        obj_animate(ttSWBodyObject);
    }

    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_STOPWATCH_HANDS]);

    for (i = 0; curRacer->lap >= i && i < 5; i++) {
        stopwatchTimer += curRacer->lap_times[i];
    }

    if (osTvType == OS_TV_TYPE_PAL) {
        stopwatchTimer = (f32) stopwatchTimer * 1.2;
    }

    if (normalise_time(36000) < stopwatchTimer) {
        stopwatchTimer = normalise_time(36000);
    }

    gCurrentHud->entry[HUD_STOPWATCH].rotation.z_rotation = ((stopwatchTimer * 0x444) + 0x7FF8);
    if (get_contpak_error() <= 0) {
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_STOPWATCH]);
    }

    stopwatchTimer = (((stopwatchTimer / 60) + 30) % 60);
    gCurrentHud->entry[HUD_STOPWATCH].rotation.z_rotation = stopwatchTimer * 0x444;
    gCurrentHud->entry[HUD_STOPWATCH].pos.y = gCurrentHud->entry[HUD_STOPWATCH_HANDS].pos.y + 28.0f;
    if (get_contpak_error() <= 0) {
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_STOPWATCH]);
    }

    modInst->animUpdateTimer = 0;
    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    hud_course_arrows(curRacer, updateRate);
    if (curRacer->raceFinished == FALSE) {
        spB8 = gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.y;
        for (i = 0; i < curRacer->countLap && i < gHudLevelHeader->laps; i++) {
            get_timestamp_from_frames(curRacer->lap_times[i], &spB4, &spB0, &spAC);
            gHudColour = gHudLapColours[i];
            hud_timer_render(gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x, spB8, spB4, spB0, spAC, 1);
            gHudColour = COLOUR_RGBA32(255, 255, 255, 254);
            spB8 += 12;
            gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
            gCurrentHud->entry[HUD_TIME_TRIAL_LAP_NUMBER].spriteOffset = i + 1;
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx,
                               (HudElement *) &gCurrentHud->entry[HUD_TIME_TRIAL_LAP_NUMBER]);
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx,
                               (HudElement *) &gCurrentHud->entry[HUD_TIME_TRIAL_LAP_TEXT]);
            gCurrentHud->entry[HUD_TIME_TRIAL_LAP_NUMBER].pos.y += 12.0f;
            gCurrentHud->entry[HUD_TIME_TRIAL_LAP_TEXT].pos.y += 12.0f;
        }

        gCurrentHud->entry[HUD_TIME_TRIAL_LAP_NUMBER].pos.y -= i * 0xC;
        gCurrentHud->entry[HUD_TIME_TRIAL_LAP_TEXT].pos.y -= i * 0xC;
        if ((curRacer->boost_sound & BOOST_SOUND_UNK4) && (sHudBoostVoiceTimer <= 0)) {
            hud_stopwatch_face(7, 3, 0, 4, 0);
            sHudBoostVoiceTimer = 60;
            curRacer->boost_sound &= ~BOOST_SOUND_UNK4;
        }
        if (sHudBoostVoiceTimer > 0) {
            sHudBoostVoiceTimer -= updateRate;
            if (sHudBoostVoiceTimer < 0) {
                sHudBoostVoiceTimer = 0;
            }
        }
        if ((sHudBoostVoiceTimer == 0) && (!curRacer->raceFinished)) {
            sHudBoostVoiceTimer = -100;
            if (gHUDVoiceSoundMask == 0) {
                soundID = rand_range(0, sHudTTVoiceCount + 2) + SOUND_VOICE_TT_OH_NO;
                while (soundID == gHudTTSoundID) {
                    soundID = rand_range(0, sHudTTVoiceCount + 2) + SOUND_VOICE_TT_OH_NO;
                }
                gHudTTSoundID = soundID;
                sound_play(soundID, &gHUDVoiceSoundMask);
                sHudTTVoiceCount = 1;
            }
        }
    }

    if ((curRacer->lap > 0) && (curRacer->lap < gHudLevelHeader->laps) && (curRacer->lap_times[curRacer->lap] < 20) &&
        (gHUDVoiceSoundMask == 0) && (curRacer->vehicleID <= VEHICLE_PLANE)) {
        gHudSettings = get_settings();
        if (curRacer->lap_times[curRacer->lap - 1] < gHudSettings->flapTimesPtr[curRacer->vehicleID][level_id()]) {
            stopwatchTimer = curRacer->lap_times[curRacer->lap - 1];
            for (i = 0; i < curRacer->lap - 1; i++) {
                if (stopwatchTimer >= curRacer->lap_times[i]) {
                    stopwatchTimer = 0;
                }
            }
            if (stopwatchTimer != 0) {
                sound_play(SOUND_VOICE_TT_LAP_RECORD, &gHUDVoiceSoundMask);
            }
        }
    }

    if (timetrial_valid_player_ghost()) {
        ttSWBodyObject = timetrial_player_ghost();
        if (ttSWBodyObject != NULL) {
            if (get_race_countdown() == 0 && curRacer->raceFinished == FALSE) {
                posX = ttSWBodyObject->trans.x_position - playerRacerObj->trans.x_position;
                posY = ttSWBodyObject->trans.y_position - playerRacerObj->trans.y_position;
                posZ = ttSWBodyObject->trans.z_position - playerRacerObj->trans.z_position;
                if ((sqrtf((posX * posX) + (posY * posY) + (posZ * posZ)) < 600.0f) && (gHUDVoiceSoundMask == 0) &&
                    (sHudRandomVoiceTimer == 0)) {
                    soundID = SOUND_VOICE_TT_GO_FOR_IT + rand_range(0, 2);
                    while (soundID == gHudTTSoundID) {
                        soundID = SOUND_VOICE_TT_GO_FOR_IT + rand_range(0, 2);
                    }
                    gHudTTSoundID = soundID;
                    sound_play(soundID, &gHUDVoiceSoundMask);
                    sHudRandomVoiceTimer = rand_range(120, 1200);
                }
                sHudRandomVoiceTimer -= updateRate;
                if (sHudRandomVoiceTimer < 0) {
                    sHudRandomVoiceTimer = 0;
                }
            }
        }
    }

    if (curRacer->lap_times[curRacer->lap] < 30 && curRacer->lap != 0) {
        hud_stopwatch_face(5, 1, 2, 2, 0);
    } else if (curRacer->boostTimer > 0) {
        hud_stopwatch_face(6, 2, 3, 2, 0);
    } else if (curRacer->wrongWayCounter > 120) {
        hud_stopwatch_face(6, 3, 1, 4, 0);
    }

    hud_lap_count(curRacer, updateRate);
    hud_race_time(curRacer, updateRate);
    hud_bananas(curRacer, updateRate);
    hud_wrong_way(curRacer, updateRate);
    hud_race_start(arg0, updateRate);
    hud_speedometre(playerRacerObj, updateRate);
    if (get_contpak_error() > 0) {
        // has to be defined here to match
        char *SWMessage[3];

#if REGION == REGION_JP
        switch (get_contpak_error()) {
            // Print "ゴースト セーブ ふかのう" - "Ghost Save Impossible"
            case CONTPAK_ERROR_UNKNOWN:
                SWMessage[2] = gHudTimeTrialJp_Ghost;
                SWMessage[1] = gHudTimeTrialJp_Save;
                SWMessage[0] = gHudTimeTrialJp_Impossible;
                break;
            // Print "{blank} パック フル " - "{blank} Pak Full"
            case CONTPAK_ERROR_FULL:
                SWMessage[2] = *(s32 *) gHudTimeTrialJp_FullBlank;
                SWMessage[1] = gHudTimeTrialJp_FullPak;
                SWMessage[0] = gHudTimeTrialJp_Full;
                break;
            // Print "{blank} パック ふりょう" - "{blank} Pak Defective"
            case CONTPAK_ERROR_DAMAGED:
                SWMessage[2] = *(s32 *) gHudTimeTrialJp_DamagedBlank;
                SWMessage[1] = gHudTimeTrialJp_DamagedPak;
                SWMessage[0] = gHudTimeTrialJp_Defective;
                break;
            default:
                SWMessage[2] = NULL;
                SWMessage[1] = NULL;
                SWMessage[0] = NULL;
                break;
        }
#else
        switch (get_contpak_error()) {
            // Print "CAN'T SAVE GOST"
            case CONTPAK_ERROR_UNKNOWN:
                SWMessage[2] = "CAN'T";
                SWMessage[1] = "SAVE";
                SWMessage[0] = "GHOST";
                break;
            // Print "CONTROLLER PAK FULL"
            case CONTPAK_ERROR_FULL:
                SWMessage[2] = " CONTROLLER";
                SWMessage[1] = "PAK";
                SWMessage[0] = "FULL";
                break;
            // Print "CONTROLLER PAK DAMAGED"
            case CONTPAK_ERROR_DAMAGED:
                SWMessage[2] = " CONTROLLER";
                SWMessage[1] = "PAK";
                SWMessage[0] = "DAMAGED";
                break;
#if VERSION >= VERSION_79
            default:
                SWMessage[2] = NULL;
                SWMessage[1] = NULL;
                SWMessage[0] = NULL;
                break;
#endif
        }
#endif

        set_kerning(TRUE);
        set_text_font(ASSET_FONTS_FUNFONT);
        // Draw text shadow.
        set_text_colour(0, 0, 0, 255, 255);
        draw_text(&gHudDL, gStopwatchErrorX + gHudOffsetX + gHudBounceX + 1, gStopwatchErrorY + 1, SWMessage[2],
                  ALIGN_MIDDLE_CENTER);
        draw_text(&gHudDL, gStopwatchErrorX + gHudOffsetX + gHudBounceX + 1, gStopwatchErrorY + 15, SWMessage[1],
                  ALIGN_MIDDLE_CENTER);
        draw_text(&gHudDL, gStopwatchErrorX + gHudOffsetX + gHudBounceX + 1, gStopwatchErrorY + 29, SWMessage[0],
                  ALIGN_MIDDLE_CENTER);
        // Draw actual text.
        set_text_colour(gHudPulseLight->rgba.r, gHudPulseLight->rgba.g, gHudPulseLight->rgba.b, 128,
                        gHudPulseLight->rgba.a);
        draw_text(&gHudDL, gStopwatchErrorX + gHudOffsetX + gHudBounceX, gStopwatchErrorY, SWMessage[2],
                  ALIGN_MIDDLE_CENTER);
        draw_text(&gHudDL, gStopwatchErrorX + gHudOffsetX + gHudBounceX, gStopwatchErrorY + 14, SWMessage[1],
                  ALIGN_MIDDLE_CENTER);
        draw_text(&gHudDL, gStopwatchErrorX + gHudOffsetX + gHudBounceX, gStopwatchErrorY + 28, SWMessage[0],
                  ALIGN_MIDDLE_CENTER);
        update_colour_cycle(gHudPulseLight, updateRate);
        set_kerning(FALSE);
    }

    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
}

void hud_stopwatch_face(u8 arg0, u8 arg1, u8 animID, u8 arg3, u8 arg4) {
    s32 numberOfBatches;
    s32 i;
    ModelInstance *modInst;
    Object *hud20;

    if (is_in_time_trial()) {
        hud20 = gAssetHudElements->entry[HUD_ELEMENT_UNK_14];
        if (hud20 != NULL) {
            modInst = hud20->modelInstances[0];
            numberOfBatches = modInst->objModel->numberOfBatches;

            for (i = 0; i < numberOfBatches; i++) {
                if (sMinimapTexIndexB == modInst->objModel->batches[i].textureIndex) {
                    modInst->objModel->batches[i].textureIndex = arg0;
                    if (arg0 == 4) {
                        modInst->objModel->batches[i].flags |= (RENDER_UNK_0800000 | RENDER_TEX_ANIM);
                        modInst->objModel->hasAnimatedTexture = TRUE;
                    } else {
                        modInst->objModel->batches[i].flags &= ~(RENDER_UNK_0800000 | RENDER_TEX_ANIM);
                        modInst->objModel->hasAnimatedTexture = FALSE;
                    }
                    numberOfBatches = modInst->objModel->numberOfBatches;
                } else if (sMinimapTexIndexA == modInst->objModel->batches[i].textureIndex) {
                    modInst->objModel->batches[i].textureIndex = arg1;
                    numberOfBatches = modInst->objModel->numberOfBatches;
                }
            }
            sMinimapTexIndexB = arg0;
            sMinimapTexIndexA = arg1;
            gStopwatchAnimID = animID;
            D_80126D69 = arg3;
            sHudMapVisible = arg4;
        }
    }
}

/**
 * Reset the angle of the speedometre's needle to the starting position.
 */
void hud_speedometre_reset(void) {
    gCurrentHud->entry[HUD_SPEEDOMETRE_ARROW].rotation.z_rotation = 0x6490;
}

/**
 * When enabled, displays speed of the player in the bottom right corner, replacing the minimap.
 * Speed is calculated by normalising racer velocity. then scaling the angle of the needle with that number.
 */
void hud_speedometre(Object *obj, UNUSED s32 updateRate) {
    f32 vel;
    Object_Racer *racer;
    s32 opacity;

    if (gNumActivePlayers == 1) {
        if (!check_if_showing_cutscene_camera()) {
            racer = obj->racer;
            if (racer->raceFinished == FALSE) {
                if (racer->vehicleID == VEHICLE_PLANE) {
                    vel = sqrtf((obj->x_velocity * obj->x_velocity) + (obj->y_velocity * obj->y_velocity) +
                                (obj->z_velocity * obj->z_velocity));
                } else {
                    vel = sqrtf((obj->x_velocity * obj->x_velocity) + (obj->z_velocity * obj->z_velocity));
                }
                if (sRecordVel < vel) {
                    sRecordVel = vel;
                }
                vel *= 4.0f;
                //!@bug: Planes and hovercraft use drift_direction for something else, applying this unintentionally.
                if (racer->drift_direction != 0) {
                    vel += 7.0f;
                }
                if (vel > 100.0f) {
                    vel = 100.0f;
                }
                if (racer->velocity > 0.0f) {
                    vel = 0.0f;
                }
                vel = 100.0f - vel;
                vel *= 300.0f;
                if (vel > 25744.0f) {
                    vel = 25744.0f;
                }
                if (!is_game_paused()) {
                    if (vel < gCurrentHud->entry[HUD_SPEEDOMETRE_ARROW].rotation.z_rotation) {
                        gCurrentHud->entry[HUD_SPEEDOMETRE_ARROW].rotation.z_rotation +=
                            ((vel - gCurrentHud->entry[HUD_SPEEDOMETRE_ARROW].rotation.z_rotation) / 2);
                    } else {
                        gCurrentHud->entry[HUD_SPEEDOMETRE_ARROW].rotation.z_rotation +=
                            ((vel - gCurrentHud->entry[HUD_SPEEDOMETRE_ARROW].rotation.z_rotation) / 8);
                    }
                }
                if (gHudToggleSettings[gHUDNumPlayers] == 0) {
                    if (gMinimapXlu & 2) {
                        opacity = 255.0f - ((gMinimapOpacity * 255.0f) / gMinimapOpacityTarget);
                    } else {
                        opacity = 255;
                    }
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_SPEEDOMETRE_BG]);
                    rendermode_reset(&gHudDL);
                    sprite_opaque(TRUE);
                    gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, opacity);
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_SPEEDOMETRE_ARROW]);
                    sprite_opaque(FALSE);
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_SPEEDOMETRE_0]);
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_SPEEDOMETRE_30]);
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_SPEEDOMETRE_60]);
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_SPEEDOMETRE_90]);
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_SPEEDOMETRE_120]);
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_SPEEDOMETRE_150]);
                }
            }
        }
    }
}

/**
 * Play the normal race finish message, unless you beat your time record,
 * then T.T will say "Race Record" instead.
 */
void hud_time_trial_message(s16 *playerID) {
    Settings *settings = get_settings();

    if (playerID != NULL) {
#if VERSION >= VERSION_79
        if (settings->racers[*playerID].best_times & 0x80 && (level_type() == RACETYPE_DEFAULT)) {
#else
        if (settings->racers[*playerID].best_times & 0x80) {
#endif

            sound_play(SOUND_VOICE_TT_RACE_RECORD, &gHUDVoiceSoundMask);
        } else {
            sound_play(SOUND_VOICE_TT_FINISH, &gHUDVoiceSoundMask);
        }
    }
}

/**
 * Render the finishing overlay after a time trial.
 * Has measures for multiplayer, depsite time trial being a single player exclusive mode.
 */
void hud_time_trial_finish(Object_Racer *racer, s32 updateRate) {
    UNUSED s32 temp;

    switch (gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.status) {
        case TIMETRIAL_FINISH_INIT:
            gHudAudioData[0].volume = 127;
            gHudAudioData[0].volumeRamp = 0;
            gHudAudioData[0].playerIndex = racer->playerIndex;
            sound_play(SOUND_WHOOSH1, NULL);
            if (gHUDNumPlayers == 0 || gHUDNumPlayers == 1) {
                gCurrentHud->entry[HUD_RACE_END_FINISH].pos.x = -200.0f;
                gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.targetPos = 0;
            } else {
                if (racer->playerIndex == PLAYER_ONE || racer->playerIndex == PLAYER_THREE) {
                    gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.targetPos = -80;
                    gCurrentHud->entry[HUD_RACE_END_FINISH].pos.x = -200.0f;
                } else {
                    gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.targetPos = 80;
                    gCurrentHud->entry[HUD_RACE_END_FINISH].pos.x = -40.0f;
                }
            }
            gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.status = TIMETRIAL_FINISH_IN;
            break;
        case TIMETRIAL_FINISH_IN:
            temp = (updateRate * 13); // Fakematch
            if (gCurrentHud->entry[HUD_RACE_END_FINISH].pos.x <
                gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.targetPos - (updateRate * 13)) {
                gCurrentHud->entry[HUD_RACE_END_FINISH].pos.x += updateRate * 13;
            } else {
                gCurrentHud->entry[HUD_RACE_END_FINISH].pos.x =
                    gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.targetPos;
                gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.status = TIMETRIAL_FINISH_SHOW;
                gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.fadeTimer = -120;
                if (gHUDVoiceSoundMask == NULL && gHudTimeTrialGhost == FALSE) {
                    hud_time_trial_message(&racer->playerIndex);
                }
            }
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_RACE_END_FINISH]);
            break;
        case TIMETRIAL_FINISH_SHOW:
            gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.fadeTimer += updateRate;
            if (gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.fadeTimer >= 120) {
                gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.status = TIMETRIAL_FINISH_OUT;
                if (gHUDNumPlayers == PLAYER_ONE || gHUDNumPlayers == PLAYER_TWO) {
                    gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.targetPos = 56;
                } else {
                    if (racer->playerIndex == PLAYER_ONE || racer->playerIndex == PLAYER_THREE) {
                        gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.targetPos = 56;
                    } else {
                        gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.targetPos = -40;
                    }
                }
                sound_play(SOUND_WHOOSH1, NULL);
            }
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_RACE_END_FINISH]);
            break;
        case TIMETRIAL_FINISH_OUT:
            gCurrentHud->entry[HUD_RACE_END_FINISH].pos.x -= updateRate * 13;
            if (gCurrentHud->entry[HUD_RACE_END_FINISH].pos.x <
                gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.targetPos) {
                gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.status = TIMETRIAL_FINISH_IDLE;
            }
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_RACE_END_FINISH]);
            break;
        case TIMETRIAL_FINISH_IDLE:
            gHudAudioData[0].volumeRamp = -1;
            gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.status = TIMETRIAL_FINISH_IDLE_2;
            gCurrentHud->entry[HUD_RACE_END_FINISH].finishText.fadeTimer = 0;
            break;
        default:
            break;
    }
}
