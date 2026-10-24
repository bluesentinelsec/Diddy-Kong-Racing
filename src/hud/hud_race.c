#include "hud_internal.h"

/**
 * In a race, render all of the race related HUD.
 */
void hud_main_race(s32 countdown, Object *obj, s32 updateRate) {
    Object_Racer *racer = obj->racer;

    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    hud_course_arrows(racer, updateRate);
    hud_wrong_way(racer, updateRate);
    hud_race_start(countdown, updateRate);

    if (gHudLevelHeader->race_type == RACETYPE_DEFAULT) {
        hud_lap_count(racer, updateRate);
    }

    hud_bananas(racer, updateRate);
    hud_race_time(racer, updateRate);
    hud_race_position(racer, updateRate);
    hud_speedometre(obj, updateRate);

    if (gHudSilverCoinRace && racer->raceFinished == FALSE) {
        hud_silver_coins(racer, updateRate);
    }

    hud_weapon(obj, updateRate);
    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
}

/**
 * Render the onscreen course arrows that show the player where to go.
 */
void hud_course_arrows(Object_Racer *racer, s32 updateRate) {
    s32 timer;
    s32 type;
    HudElement *indicator;

    if (gShowCourseDirections) {
        timer = racer->indicator_timer;
        if (timer > 0) {
            type = racer->indicator_type;
            racer->indicator_timer = timer - updateRate;
            if (type) {
                indicator = (HudElement *) &gCurrentHud->entry[HUD_COURSE_ARROWS];
                switch (type) {
                    case INDICATOR_LEFT:
                        indicator->spriteID = HUD_SPRITE_INDICATOR_TURN_45;
                        indicator->rotation.y_rotation = 0;
                        break;
                    case INDICATOR_LEFT_SHARP:
                        indicator->spriteID = HUD_SPRITE_INDICATOR_TURN_90;
                        indicator->rotation.y_rotation = 0;
                        break;
                    case INDICATOR_LEFT_UTURN:
                        indicator->spriteID = HUD_SPRITE_INDICATOR_U;
                        indicator->rotation.y_rotation = 0;
                        break;
                    case INDICATOR_RIGHT:
                        indicator->spriteID = HUD_SPRITE_INDICATOR_TURN_45;
                        indicator->rotation.y_rotation = -0x8000;
                        break;
                    case INDICATOR_RIGHT_SHARP:
                        indicator->spriteID = HUD_SPRITE_INDICATOR_TURN_90;
                        indicator->rotation.y_rotation = -0x8000;
                        break;
                    case INDICATOR_RIGHT_UTURN:
                        indicator->spriteID = HUD_SPRITE_INDICATOR_U;
                        indicator->rotation.y_rotation = -0x8000;
                        break;
                    case INDICATOR_UP:
                        indicator->spriteID = HUD_SPRITE_INDICATOR_DOWN;
                        indicator->rotation.y_rotation = -0x8000;
                        indicator->rotation.x_rotation = -0x8000;
                        break;
                    case INDICATOR_DOWN:
                        indicator->spriteID = HUD_SPRITE_INDICATOR_DOWN;
                        indicator->rotation.y_rotation = 0;
                        break;
                    default: // INDICATOR_EXCLAMATION
                        indicator->spriteID = HUD_SPRITE_INDICATOR_EXCLAMATION;
                        indicator->rotation.y_rotation = 0;
                        break;
                }
                // Flip the arrow direction on adventure 2.
                if (get_filtered_cheats() & CHEAT_MIRRORED_TRACKS && racer->indicator_type < 30) {
                    indicator->rotation.y_rotation = (s16) (0x8000 - indicator->rotation.y_rotation);
                }
                if (gHUDNumPlayers == ONE_PLAYER && racer->raceFinished == FALSE && racer->indicator_type &&
                    gHudIndicatorTimer == 0) {
                    gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 160);
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, indicator);
                    indicator->pos.x = -indicator->pos.x;
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, indicator);
                    indicator->pos.x = -indicator->pos.x;
                    indicator->rotation.x_rotation = 0;
                    gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
                }
            }
        } else {
            racer->indicator_timer = 0;
        }
        if (gHudIndicatorTimer) {
            if (gHudIndicatorTimer & 0x20) {
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 160);
                indicator = &gCurrentHud->entry[HUD_COURSE_ARROWS];
                indicator->rotation.y_rotation = 0;
                indicator->rotation.x_rotation = 0;
                indicator->spriteID = HUD_SPRITE_INDICATOR_EXCLAMATION;
                if ((get_filtered_cheats() & CHEAT_MIRRORED_TRACKS) && ((s32) racer->indicator_type < 30)) {
                    indicator->rotation.y_rotation = (s16) (0x8000 - indicator->rotation.y_rotation);
                }
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, indicator);
                indicator->pos.x = -indicator->pos.x;
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, indicator);
                indicator->pos.x = -indicator->pos.x;
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
            }
            if (updateRate < gHudIndicatorTimer) {
                gHudIndicatorTimer -= updateRate;
                return;
            }
            gHudIndicatorTimer = 0;
        }
    }
}

/**
 * Call the functions that render the "Get Ready" and "GO!" as well as their countdowns to control fade.
 * This function will also call to begin the background music for 1 and 2 player. 3 and 4 are treated with silence.
 */
void hud_race_start(s32 countdown, s32 updateRate) {
    if (!is_game_paused()) {
        if (countdown == 0 && gHudAudioData[0].playerIndex == PLAYER_COMPUTER) {
            gHudAudioData[0].volumeRamp = -1;
            gHudAudioData[1].volumeRamp = -1;
        }
        if (gHUDNumPlayers == TWO_PLAYERS) {
            sprite_opaque(TRUE);
        }
        if (countdown > 0) {
            if (gHudRaceStart) {
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, (countdown * 255) / 40);
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_RACE_START_READY]);
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
                if (gRaceStartShowHudStep == 2) {
                    sound_play(SOUND_VOICE_TT_GET_READY, &gHUDVoiceSoundMask);
                    gRaceStartShowHudStep++;
                }
            }
            // Make engine revving sounds of random pitches while the race is waiting to begin.
            if (gRaceStartSoundMask == NULL && boss_race_number() == 0) {
                f32 frequency;
                Object **racerGroup;
                Object *randomRacer;
                Object_Racer *racer;
                s32 numRacerObjects;
                racerGroup = get_racer_objects(&numRacerObjects);
                randomRacer = racerGroup[rand_range(1, numRacerObjects) - 1];
                racer = randomRacer->racer;
                if (racer->vehicleID == VEHICLE_CAR) {
                    if (rand_range(0, 100) >= 96) {
                        frequency = 1.25 - ((rand_range(0, 7) * 0.5) / 7.0);
                        audspat_play_sound_direct(76, randomRacer->trans.x_position, randomRacer->trans.y_position,
                                                  randomRacer->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER,
                                                  ((rand_range(0, 7) * 63) / 7) + 24, frequency * 100.0f,
                                                  &gRaceStartSoundMask);
                    }
                }
            }
        } else if (gCurrentHud->entry[HUD_RACE_START_GO].pos.x > -200.0f) {
            gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 160);
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, (HudElement *) &gCurrentHud->entry[HUD_RACE_START_GO]);
            gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
            gCurrentHud->entry[HUD_RACE_START_GO].raceStartGo.musicStartTimer[gHudCurrentViewport] += updateRate;
            if (gCurrentHud->entry[HUD_RACE_START_GO].raceStartGo.musicStartTimer[gHudCurrentViewport] >= 60) {
                if (gRaceStartShowHudStep == 4) {
                    // Mute background music in 3/4 player.
                    if (cam_get_viewport_layout() > TWO_PLAYERS) {
                        music_play(SEQUENCE_NONE);
                    } else {
                        level_music_start(1.0f);
                    }
                    sound_play(SOUND_WHOOSH1, NULL);
                    gRaceStartShowHudStep++;
                }
                gCurrentHud->entry[HUD_RACE_START_GO].pos.x -= (updateRate * 8);
            }
            if (gRaceStartShowHudStep == 3) {
                sound_play(SOUND_VOICE_TT_GO, &gHUDVoiceSoundMask);
                if (timetrial_ghost_staff() && timetrial_staff_unbeaten() == FALSE) {
                    hud_sound_play_delayed(SOUND_VOICE_TT_BEAT_MY_TIME, 1.7f, PLAYER_ONE);
                    set_delayed_text(ASSET_GAME_TEXT_82, 1.7f); // Now try and beat my time!
                }
                gHudAudioReset = TRUE;
                gRaceStartShowHudStep++;
            }
        }
        sprite_opaque(FALSE);
    }
}

/**
 * Renders the banana counter on screen.
 * When a banana is collected, render sparkles and animate the banana. for a brief moment.
 */
void hud_bananas(Object_Racer *racer, s32 updateRate) {
    s32 bananas;
    s32 temp_lo;
    u8 prevSprite;
    if (racer->playerIndex == PLAYER_COMPUTER ||
        (((gHUDNumPlayers < TWO_PLAYERS || gHudMapMode[gHUDNumPlayers][racer->playerIndex] == 2 ||
           gHudLevelHeader->race_type == RACETYPE_CHALLENGE_BANANAS ||
           gHudLevelHeader->race_type == RACETYPE_CHALLENGE_BATTLE) &&
          racer->raceFinished == FALSE) &&
         (gHUDNumPlayers < TWO_PLAYERS || racer->lap < 1 || racer->lap_times[racer->lap] >= 180 ||
          gHudLevelHeader->race_type & RACETYPE_CHALLENGE))) {
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BANANA_COUNT_X]);
        bananas = racer->bananas;
        prevSprite = gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].spriteOffset;
        if (gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].bananaCountIconSpin.spinCounter == 0 &&
            gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].bananaCountIconSpin.visualCounter != racer->bananas) {
            gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].bananaCountIconSpin.spinCounter = 2;
            gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].bananaCountIconSpin.visualCounter = racer->bananas;
        } else if (gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].bananaCountIconSpin.spinCounter) {
            gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].scale = 2.0f;
            prevSprite += updateRate * 13;
            if (prevSprite < (gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].spriteOffset & 0xFF)) {
                gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].bananaCountIconSpin.spinCounter--;
                if (gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].bananaCountIconSpin.spinCounter == 0) {
                    gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].bananaCountSparkle.sparkleCounter = 1;
                    gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].bananaCountSparkle.sparkleTimer = 6;
                    prevSprite = 0;
                    gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].spriteOffset = 1;
                }
            }
        }
        if (prevSprite == 0) {
            sprite_opaque(TRUE);
            enable_pal_viewport_height_adjust(TRUE);
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC]);
            enable_pal_viewport_height_adjust(FALSE);
            sprite_opaque(FALSE);
            gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].spriteOffset = prevSprite;
            if (gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].bananaCountSparkle.sparkleCounter) {
                gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].bananaCountSparkle.sparkleTimer -= updateRate;
                if (gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].bananaCountSparkle.sparkleTimer < 0) {
                    gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].bananaCountSparkle.sparkleTimer = 6;
                    if (gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].spriteOffset == 6) {
                        gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].spriteOffset = 0;
                        gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].bananaCountSparkle.sparkleCounter--;
                    } else {
                        gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].spriteOffset++;
                    }
                }
                enable_pal_viewport_height_adjust(TRUE);
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE]);
                enable_pal_viewport_height_adjust(FALSE);
            }
        } else {
            gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].spriteOffset = prevSprite + 128;
            cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
            sprite_opaque(TRUE);
            enable_pal_viewport_height_adjust(TRUE);
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN]);
            sprite_opaque(FALSE);
            enable_pal_viewport_height_adjust(FALSE);
            cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
            gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].spriteOffset -= 128;
        }
        temp_lo = bananas / 10;
        if (temp_lo) {
            gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1].spriteOffset = temp_lo;
            gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_2].spriteOffset = bananas % 10;
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_2]);
        } else {
            gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1].spriteOffset = bananas % 10;
        }
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1]);
        gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
    }
}

/**
 * Plays the race finish fanfare and displays what position you finished in the middle of the screen.
 * Uses a 3 step process to play the sounds, display the position, then slide it offscreen.
 */
void hud_race_finish_1player(Object_Racer *racer, s32 updateRate) {
    HudElement *position1;
    HudElement *position2;
    s8 drawPosition;

    position1 = &gCurrentHud->entry[HUD_RACE_FINISH_POS_1];
    position2 = &gCurrentHud->entry[HUD_RACE_FINISH_POS_2];
    drawPosition = FALSE;
    switch (position1->raceFinishPos.status) {
        case HUD_RACEFINISH_MUTE:
            music_voicelimit_set(24);
            music_play(SEQUENCE_FIRST_PLACE);
            sound_play(SOUND_WHOOSH1, NULL);
            sound_play(SOUND_VOICE_TT_FINISH, &gHUDVoiceSoundMask);
            gHudAudioData[0].volume = 127;
            gHudAudioData[0].volumeRamp = 0;
            gHudAudioData[0].playerIndex = racer->playerIndex;
            position1->raceFinishPos.status = HUD_RACEFINISH_TEXT_IN;
            break;
        case HUD_RACEFINISH_TEXT_IN:
            position1->pos.x += updateRate * 13;
            if (position1->pos.x > -23.0f) {
                position1->pos.x = -23.0f;
            }
            position2->pos.x -= updateRate * 13;
            if (position2->pos.x < 22.0f) {
                position2->pos.x = 22.0f;
            }
            drawPosition = TRUE;
            if (position2->pos.x == 22.0f) {
                position1->raceFinishPos.textOutTimer += updateRate;
                if (position1->raceFinishPos.textOutTimer >= 120) {
                    position1->raceFinishPos.textOutTimer = -120;
                    position1->raceFinishPos.textOutTicks++;
                }
                if (position1->raceFinishPos.textOutTicks == 2) {
                    position1->raceFinishPos.status = HUD_RACEFINISH_TEXT_OUT;
                    sound_play(SOUND_WHOOSH1, NULL);
                    if (gHudAudioData[0].playerIndex == racer->playerIndex) {
                        gHudAudioData[0].volumeRamp = -1;
                    }
                }
            }
            break;
        case HUD_RACEFINISH_TEXT_OUT:
            position1->pos.x += updateRate * 13;
            position2->pos.x += updateRate * 13;
            drawPosition = TRUE;
            if (position1->pos.x > 200.0f) {
                position1->raceFinishPos.status = HUD_RACEFINISH_IDLE;
            }
            break;
        case HUD_RACEFINISH_IDLE:
            position1->raceFinishPos.status = HUD_RACEFINISH_IDLE;
            break;
    }
    if (drawPosition) {
        gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 210);
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, position1);
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, position2);
        gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
    }
}

/**
 * Render the current race position of the racer.
 * Scales up and down when the position changes.
 */
void hud_race_position(Object_Racer *racer, s32 updateRate) {
    s32 place;
    f32 prevPos;
    f32 scale;
    s32 placeIndex;
    Settings *settings;

    if (racer->raceFinished) {
        return;
    }
    place = racer->racePosition;
    if (get_race_countdown()) {
        if (level_type() == RACETYPE_BOSS || is_taj_challenge()) {
            place = 2;
        } else {
            settings = get_settings();
            place = settings->racers[gHudController].starting_position;
            gCurrentHud->entry[HUD_RACE_POSITION].spriteOffset = place;
            place++;
        }
    }
    if (gHUDNumPlayers < 2) {
        // P1 and P2 have different endings to P3 onward, so offset the sprite ID based on current position.
        if (place < 4) {
            gCurrentHud->entry[HUD_RACE_POSITION_END].spriteOffset = place - 1;
        } else {
            gCurrentHud->entry[HUD_RACE_POSITION_END].spriteOffset = 3;
        }
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_RACE_POSITION_END]);
    }
    scale = (gCurrentHud->entry[HUD_RACE_POSITION].racePosition.scaleCounter + 129);
    scale *= 128.0f;
    scale = sins_f(scale);
    placeIndex = place - 1;
    gCurrentHud->entry[HUD_RACE_POSITION].scale =
        (gCurrentHud->entry[HUD_RACE_POSITION].racePosition.baseScale / 127.0f) + (1.0 + (0.5 * scale));
    prevPos = gCurrentHud->entry[HUD_RACE_POSITION].pos.x;
    gCurrentHud->entry[HUD_RACE_POSITION].pos.x -= (24.0f * gCurrentHud->entry[HUD_RACE_POSITION].scale);
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_RACE_POSITION]);
    gCurrentHud->entry[HUD_RACE_POSITION].pos.x = prevPos;
    if (gCurrentHud->entry[HUD_RACE_POSITION].racePosition.active) {
        if (gCurrentHud->entry[HUD_RACE_POSITION].racePosition.scaleCounter < 127 - (updateRate * 16)) {
            gCurrentHud->entry[HUD_RACE_POSITION].racePosition.scaleCounter += (updateRate * 16);
        } else {
            gCurrentHud->entry[HUD_RACE_POSITION].racePosition.scaleCounter = 127;
        }
    }
    if (placeIndex != gCurrentHud->entry[HUD_RACE_POSITION].spriteOffset) {
        if (gCurrentHud->entry[HUD_RACE_POSITION].racePosition.active &&
            gCurrentHud->entry[HUD_RACE_POSITION].racePosition.scaleCounter > 0) {
            gCurrentHud->entry[HUD_RACE_POSITION].racePosition.scaleCounter =
                -gCurrentHud->entry[HUD_RACE_POSITION].racePosition.scaleCounter;
        } else {
            gCurrentHud->entry[HUD_RACE_POSITION].racePosition.scaleCounter = -127;
            gCurrentHud->entry[HUD_RACE_POSITION].racePosition.active = TRUE;
        }
    }
    gCurrentHud->entry[HUD_RACE_POSITION].spriteOffset = placeIndex;
    if (gCurrentHud->entry[HUD_RACE_POSITION].racePosition.active &&
        gCurrentHud->entry[HUD_RACE_POSITION].racePosition.scaleCounter == 127) {
        gCurrentHud->entry[HUD_RACE_POSITION].racePosition.active = FALSE;
    }
}

/**
 * Draws the lap counter ("LAP n/total"), with a waving flag on the final lap in 1 or 2 player.
 * In multiplayer it shows only when the map mode is set to laps, and hides for a few seconds after a lap starts.
 */
static void hud_lap_counter(Object_Racer *racer, s32 updateRate) {
    if (racer->raceFinished == FALSE &&
        (gHUDNumPlayers <= ONE_PLAYER || racer->lap <= 0 || racer->lap_times[racer->lap] >= 180) &&
        (gHUDNumPlayers <= ONE_PLAYER || gHudMapMode[gHUDNumPlayers][racer->playerIndex] == 3)) {
        if (gHudLevelHeader->laps == (0, racer->countLap + 1) && gHUDNumPlayers < THREE_PLAYERS) {
            gCurrentHud->entry[HUD_LAP_COUNT_FLAG].lapCountFlag.visualCounter += updateRate;
            if (gCurrentHud->entry[HUD_LAP_COUNT_FLAG].lapCountFlag.visualCounter > 6) {
                gCurrentHud->entry[HUD_LAP_COUNT_FLAG].spriteOffset++;
                gCurrentHud->entry[HUD_LAP_COUNT_FLAG].lapCountFlag.visualCounter -= 6;
                if (gCurrentHud->entry[HUD_LAP_COUNT_FLAG].spriteOffset > 4) {
                    gCurrentHud->entry[HUD_LAP_COUNT_FLAG].spriteOffset = 0;
                }
            }
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_LAP_COUNT_FLAG]);
        }
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_LAP_COUNT_LABEL]);
        if (racer->countLap >= gHudLevelHeader->laps - 1) {
            gCurrentHud->entry[HUD_LAP_COUNT_CURRENT].spriteOffset = gHudLevelHeader->laps;
        } else {
            gCurrentHud->entry[HUD_LAP_COUNT_CURRENT].spriteOffset = racer->countLap + 1;
        }
        gCurrentHud->entry[HUD_LAP_COUNT_TOTAL].spriteOffset = gHudLevelHeader->laps;
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_LAP_COUNT_CURRENT]);
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_LAP_COUNT_SEPERATOR]);
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_LAP_COUNT_TOTAL]);
    }
}

/**
 * Starts the "Lap 2" or "Final Lap" banner just after the racer crosses the line,
 * setting where each word starts and stops for the current viewport layout.
 */
static void hud_lap_text_trigger(Object_Racer *racer) {
    if (racer->lap_times[racer->lap] < 30 && gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.status == LAPTEXT_IDLE &&
        racer->raceFinished == FALSE) {
        if (racer->lap == 1 && gHudLevelHeader->laps >= 3) {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.status = LAPTEXT_LAP_TWO;
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.direction = LAPTEXT_OUT;
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.soundPlayed = FALSE;
            sound_play(SOUND_WHOOSH1, NULL);
            switch (gHUDNumPlayers) {
                case ONE_PLAYER:
                    gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos = -21;
                    gCurrentHud->entry[HUD_LAP_TEXT_TWO].lapText.targetPos = 32;
                    gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = -200.0f;
                    gCurrentHud->entry[HUD_LAP_TEXT_TWO].pos.x = 211.0f;
                    break;
                case TWO_PLAYERS:
                    gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos = -16;
                    gCurrentHud->entry[HUD_LAP_TEXT_TWO].lapText.targetPos = 27;
                    gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = -200.0f;
                    gCurrentHud->entry[HUD_LAP_TEXT_TWO].pos.x = 211.0f;
                    break;
                default:
                    if (racer->playerIndex != PLAYER_ONE && racer->playerIndex != PLAYER_THREE) {
                        gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos = 60;
                        gCurrentHud->entry[HUD_LAP_TEXT_TWO].lapText.targetPos = 91;
                        gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = -70.0f;
                        gCurrentHud->entry[HUD_LAP_TEXT_TWO].pos.x = 221.0f;
                    } else {
                        gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos = -90;
                        gCurrentHud->entry[HUD_LAP_TEXT_TWO].lapText.targetPos = -59;
                        gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = -220.0f;
                        gCurrentHud->entry[HUD_LAP_TEXT_TWO].pos.x = 71.0f;
                    }
                    break;
            }
        } else if (gHudLevelHeader->laps == (0, racer->lap + 1) && racer->lap != 0) {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.status = LAPTEXT_FINAL_LAP;
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.direction = LAPTEXT_IN;
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.soundPlayed = FALSE;
            sound_play(SOUND_WHOOSH1, NULL);
            switch (gHUDNumPlayers) {
                case ONE_PLAYER:
                    gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos = 51;
                    gCurrentHud->entry[HUD_LAP_TEXT_FINAL].lapText.targetPos = -41;
                    gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = 210.0f;
                    gCurrentHud->entry[HUD_LAP_TEXT_FINAL].pos.x = -200.0f;
                    break;
                case TWO_PLAYERS:
                    gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos = 41;
                    gCurrentHud->entry[HUD_LAP_TEXT_FINAL].lapText.targetPos = -31;
                    gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = 210.0f;
                    gCurrentHud->entry[HUD_LAP_TEXT_FINAL].pos.x = -200.0f;
                    break;
                default:
                    if (racer->playerIndex != PLAYER_ONE && racer->playerIndex != PLAYER_THREE) {
                        gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos = 100;
                        gCurrentHud->entry[HUD_LAP_TEXT_FINAL].lapText.targetPos = 50;
                        gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = 100.0f;
                        gCurrentHud->entry[HUD_LAP_TEXT_FINAL].pos.x = -50.0f;
                    } else {
                        gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos = -50;
                        gCurrentHud->entry[HUD_LAP_TEXT_FINAL].lapText.targetPos = -100;
                        gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = 50.0f;
                        gCurrentHud->entry[HUD_LAP_TEXT_FINAL].pos.x = -200.0f;
                    }
                    break;
            }
        }
    }
}

/**
 * Animates the "Lap 2" banner: the two words slide in, T.T calls the lap once both are in place,
 * and a second into the lap they slide back out until the banner goes idle.
 */
static void hud_lap_text_lap_two(Object_Racer *racer, s32 updateRate) {
    if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.direction == LAPTEXT_OUT) {
        if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x <
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos - (updateRate * 13)) {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x += updateRate * 13;
        } else {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos;
        }
        if (gCurrentHud->entry[HUD_LAP_TEXT_TWO].pos.x >
            gCurrentHud->entry[HUD_LAP_TEXT_TWO].lapText.targetPos + (updateRate * 13)) {
            gCurrentHud->entry[HUD_LAP_TEXT_TWO].pos.x -= updateRate * 13;
        } else {
            gCurrentHud->entry[HUD_LAP_TEXT_TWO].pos.x = gCurrentHud->entry[HUD_LAP_TEXT_TWO].lapText.targetPos;
        }
        if (racer->lap_times[racer->lap] >= 60) {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.direction = LAPTEXT_IN;
            sound_play(SOUND_WHOOSH1, NULL);
        }
        if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x == gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos &&
            gCurrentHud->entry[HUD_LAP_TEXT_TWO].pos.x == gCurrentHud->entry[HUD_LAP_TEXT_TWO].lapText.targetPos &&
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.soundPlayed == FALSE) {
            if (gHUDVoiceSoundMask == NULL) {
                sound_play(SOUND_VOICE_TT_LAP2, &gHUDVoiceSoundMask);
            }
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.soundPlayed = TRUE;
        }
    } else if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.direction == LAPTEXT_IN) {
        gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x -= updateRate * 13;
        gCurrentHud->entry[HUD_LAP_TEXT_TWO].pos.x += updateRate * 13;
        if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x < -200.0f) {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.status = LAPTEXT_IDLE;
        }
    }
    gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 160);
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_LAP_TEXT_TWO]);
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx,
                       &gCurrentHud->entry[HUD_LAP_TEXT_LAP]); // lap
    gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
}

/**
 * Animates the "Final Lap" banner: the two words slide in, T.T calls the final lap once both are in place,
 * and a second into the lap they slide back out until the banner goes idle.
 */
static void hud_lap_text_final_lap(Object_Racer *racer, s32 updateRate) {
    if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.direction == LAPTEXT_IN) {
        if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x >
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos + (updateRate * 13)) {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x -= updateRate * 13;
        } else {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x = gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos;
        }
        if (gCurrentHud->entry[HUD_LAP_TEXT_FINAL].pos.x <
            gCurrentHud->entry[HUD_LAP_TEXT_FINAL].lapText.targetPos - (updateRate * 13)) {
            gCurrentHud->entry[HUD_LAP_TEXT_FINAL].pos.x += updateRate * 13;
        } else {
            gCurrentHud->entry[HUD_LAP_TEXT_FINAL].pos.x = gCurrentHud->entry[HUD_LAP_TEXT_FINAL].lapText.targetPos;
        }
        if (racer->lap_times[racer->lap] >= 60) {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.direction = LAPTEXT_OUT;
            sound_play(SOUND_WHOOSH1, NULL);
        }
        if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x == gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.targetPos &&
            gCurrentHud->entry[HUD_LAP_TEXT_FINAL].pos.x == gCurrentHud->entry[HUD_LAP_TEXT_FINAL].lapText.targetPos &&
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.soundPlayed == FALSE) {
            if (gHUDVoiceSoundMask == NULL) {
                sound_play(SOUND_VOICE_TT_FINAL_LAP, &gHUDVoiceSoundMask);
            }
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.soundPlayed = TRUE;
        }
    } else if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.direction == LAPTEXT_OUT) {
        gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x += updateRate * 13;
        gCurrentHud->entry[HUD_LAP_TEXT_FINAL].pos.x -= updateRate * 13;
        if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].pos.x > 200.0f) {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.status = LAPTEXT_IDLE;
        }
    }

    gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 160);
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_LAP_TEXT_FINAL]);
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_LAP_TEXT_LAP]);
    gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
}

/**
 * Displays the current lap number onscreen.
 * Also displays the "Lap 2" and "Final Lap" messages when the line is crossed.
 * In multiplayer, shows a cut down display, if the display setting is set to show laps.
 */
void hud_lap_count(Object_Racer *racer, s32 updateRate) {
    hud_lap_counter(racer, updateRate);
    if (gHUDNumPlayers == TWO_PLAYERS) {
        sprite_opaque(TRUE);
    }
    if (is_game_paused() == FALSE) {
        hud_lap_text_trigger(racer);
        if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.status != LAPTEXT_IDLE) {
            if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.status == LAPTEXT_LAP_TWO) {
                hud_lap_text_lap_two(racer, updateRate);
            } else if (gCurrentHud->entry[HUD_LAP_TEXT_LAP].lapText.status == LAPTEXT_FINAL_LAP) {
                hud_lap_text_final_lap(racer, updateRate);
            }
        }
    }
    sprite_opaque(FALSE);
}

/**
 * Players going the wrong way will be nagged by T.T to turn around.
 * This function plays the audio, and makes the text fly in.
 */
void hud_wrong_way(Object_Racer *obj, s32 updateRate) {
    f32 textMoveSpeed;
    f32 textPosTarget;

    if (gHUDNumPlayers == TWO_PLAYERS) {
        sprite_opaque(TRUE);
    }
    if (obj->wrongWayCounter > 120 &&
        (gHUDNumPlayers ||
         gCurrentHud->entry[HUD_WRONGWAY_1].pos.x == gCurrentHud->entry[HUD_WRONGWAY_1].lapText.targetPos) &&
        !is_game_paused()) {
        if ((gWrongWayNagPrefix || gWrongWayNagTimer == 0) && gHUDVoiceSoundMask == NULL) {
            // 20% chance that T.T decides not to precede his nagging with "No no no!"
            if (gWrongWayNagPrefix || rand_range(1, 10) >= 8) {
                gWrongWayNagPrefix = FALSE;
                sound_play(SOUND_VOICE_TT_WRONG_WAY, &gHUDVoiceSoundMask);
                gWrongWayNagTimer = rand_range(1, 480) + 120;
            } else {
                gWrongWayNagPrefix = TRUE;
                sound_play(SOUND_VOICE_TT_NONONO, &gHUDVoiceSoundMask);
            }
        }
        gWrongWayNagTimer -= updateRate;
        if (gWrongWayNagTimer < 0) {
            gWrongWayNagTimer = 0;
        }
    }
    if (gCurrentHud->entry[HUD_WRONGWAY_1].lapText.status != WRONGWAY_HIDE) {
        if (gCurrentHud->entry[HUD_WRONGWAY_1].lapText.status == WRONGWAY_SHOW) {
            if (gCurrentHud->entry[HUD_WRONGWAY_1].lapText.direction == WRONGWAY_IN) {
                textMoveSpeed = updateRate * 13;
                gCurrentHud->entry[HUD_WRONGWAY_1].pos.x += textMoveSpeed;
                textPosTarget = gCurrentHud->entry[HUD_WRONGWAY_1].lapText.targetPos;
                if (gCurrentHud->entry[HUD_WRONGWAY_1].pos.x > textPosTarget) {
                    gCurrentHud->entry[HUD_WRONGWAY_1].pos.x = textPosTarget;
                }
                gCurrentHud->entry[HUD_WRONGWAY_2].pos.x -= textMoveSpeed;
                textPosTarget = gCurrentHud->entry[HUD_WRONGWAY_2].lapText.targetPos;
                if (gCurrentHud->entry[HUD_WRONGWAY_2].pos.x < textPosTarget) {
                    gCurrentHud->entry[HUD_WRONGWAY_2].pos.x = textPosTarget;
                }
                if (obj->wrongWayCounter <= 90) {
                    gCurrentHud->entry[HUD_WRONGWAY_1].lapText.direction = WRONGWAY_OUT;
                    sound_play(SOUND_WHOOSH1, NULL);
                }
            } else if (gCurrentHud->entry[HUD_WRONGWAY_1].lapText.direction == WRONGWAY_OUT) {
                textMoveSpeed = updateRate * 13;
                gCurrentHud->entry[HUD_WRONGWAY_1].pos.x -= textMoveSpeed;
                gCurrentHud->entry[HUD_WRONGWAY_2].pos.x += textMoveSpeed;
                if (gCurrentHud->entry[HUD_WRONGWAY_1].pos.x < -200.0f) {
                    gCurrentHud->entry[HUD_WRONGWAY_1].lapText.status = WRONGWAY_HIDE;
                }
            }
            if (!is_game_paused()) {
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 160);
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_WRONGWAY_1]);
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_WRONGWAY_2]);
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
            }
        }
    } else if (obj->wrongWayCounter > 120) {
        gCurrentHud->entry[HUD_WRONGWAY_1].lapText.status = WRONGWAY_SHOW;
        gCurrentHud->entry[HUD_WRONGWAY_1].lapText.direction = WRONGWAY_IN;
        gCurrentHud->entry[HUD_WRONGWAY_1].lapText.targetPos = -31;
        gCurrentHud->entry[HUD_WRONGWAY_2].lapText.targetPos = 52;
        gCurrentHud->entry[HUD_WRONGWAY_1].lapText.soundPlayed = FALSE;
        if (gHUDNumPlayers == TWO_PLAYERS) {
            gCurrentHud->entry[HUD_WRONGWAY_1].lapText.targetPos = -21;
            gCurrentHud->entry[HUD_WRONGWAY_2].lapText.targetPos = 42;
        } else if (gHUDNumPlayers > TWO_PLAYERS) {
            if (obj->playerIndex == PLAYER_ONE || obj->playerIndex == PLAYER_THREE) {
                gCurrentHud->entry[HUD_WRONGWAY_1].lapText.targetPos = -100;
                gCurrentHud->entry[HUD_WRONGWAY_2].lapText.targetPos = -55;
            } else {
                gCurrentHud->entry[HUD_WRONGWAY_1].lapText.targetPos = 59;
                gCurrentHud->entry[HUD_WRONGWAY_2].lapText.targetPos = 104;
            }
        }
        gCurrentHud->entry[HUD_WRONGWAY_2].pos.x = gCurrentHud->entry[HUD_WRONGWAY_2].lapText.targetPos + 200;
        gCurrentHud->entry[HUD_WRONGWAY_1].pos.x = gCurrentHud->entry[HUD_WRONGWAY_2].lapText.targetPos - 200;
        sound_play(SOUND_WHOOSH1, NULL);
    }
    sprite_opaque(FALSE);
}

/**
 * Render the lap time on the top right.
 * When the player begins a new lap, start flashing the previous lap time below instead.
 */
void hud_race_time(Object_Racer *racer, s32 updateRate) {
    s32 i;
    s32 stopwatchTimer;
    s32 minutes;
    s32 seconds;
    s32 hundredths;
    s32 countingDown;
    s32 timerHideCounter;

    if (!(gHUDNumPlayers != ONE_PLAYER && gHudMapMode[gHUDNumPlayers][racer->playerIndex] != 1) ||
        (gHUDNumPlayers > ONE_PLAYER && racer->lap > 0 && racer->lap_times[racer->lap] < 180)) {
        if (racer->raceFinished == FALSE) {
            timerHideCounter = gCurrentHud->entry[HUD_RACE_TIME_LABEL].raceTimeLabel.hideTimer + 127;
            if (racer->lap > 0 && racer->lap_times[racer->lap] < 180 && racer->lap < gHudLevelHeader->laps) {
                stopwatchTimer = racer->lap_times[racer->lap - 1];
                countingDown = TRUE;
                if (timerHideCounter == 0) {
                    timerHideCounter = 180;
                }
            } else {
                stopwatchTimer = 0;
                for (i = 0; i <= racer->countLap && i < gHudLevelHeader->laps; i++) {
                    stopwatchTimer += racer->lap_times[i];
                }
                countingDown = stopwatchTimer == 0 || racer->raceFinished || is_game_paused();
                gCurrentHud->entry[HUD_RACE_TIME_LABEL].raceTimeLabel.hideTimer = -127;
                timerHideCounter = 0;
            }
            if (gHUDNumPlayers == ONE_PLAYER) {
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_RACE_TIME_LABEL]);
            }
            if (normalise_time(36000) < stopwatchTimer) {
                stopwatchTimer = normalise_time(36000);
            }
            get_timestamp_from_frames(stopwatchTimer, &minutes, &seconds, &hundredths);
            if (countingDown || (normalise_time(36000) == stopwatchTimer)) {
                if ((timerHideCounter > updateRate)) {
                    timerHideCounter -= updateRate;
                } else {
                    timerHideCounter = 0;
                }
                gCurrentHud->entry[HUD_RACE_TIME_LABEL].raceTimeLabel.hideTimer = timerHideCounter - 127;
                if ((timerHideCounter % 30) > 20) {
                    gHideRaceTimer = TRUE;
                    return;
                } else {
                    if (gHideRaceTimer) {
                        if (gHUDNumPlayers == ONE_PLAYER) {
                            sound_play(SOUND_HUD_LAP_TICK, NULL);
                        }
                        gHideRaceTimer = FALSE;
                    }
                }
            } else {
                // This section seems to do nothing of consequense.
                hundredths =
                    gCurrentHud->entry[HUD_RACE_TIME_LABEL].raceTimeLabel.hundredths + ((hundredths / 10) * 10);
                gCurrentHud->entry[HUD_RACE_TIME_LABEL].raceTimeLabel.hundredths++;
                if (gCurrentHud->entry[HUD_RACE_TIME_LABEL].raceTimeLabel.hundredths >= 10) {
                    gCurrentHud->entry[HUD_RACE_TIME_LABEL].raceTimeLabel.hundredths = 0;
                }
            }

            if (gNumActivePlayers == 1) {
                hud_timer_render(gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x,
                                 gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y, minutes, seconds, hundredths, FALSE);
            } else {
                hud_timer_render(gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x,
                                 gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y, minutes, seconds, hundredths, TRUE);
            }
            gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
        }
    }
}

/**
 * Render a time in minutes, seconds and hundredths of a second.
 * Supports a smaller font for multiplayer.
 */
void hud_timer_render(s32 x, s32 y, s32 minutes, s32 seconds, s32 hundredths, s32 smallFont) {
    HudElementBase entry;
    s32 offset1;
    s32 offset2;
    s32 offset3;
    s32 offsetY;
    s32 numbers;
    s32 seperator;

    entry.x = x;
    entry.y = y;
    entry.z_rotation = 0;
    entry.x_rotation = 0;
    entry.y_rotation = 0;
    entry.scale = 1.0f;
    entry.z = 0.0f;
    if (!smallFont) { // The below offsets are applied after drawing their respective elements.
        offset1 = 11; // Offset for the first number
        offset2 = 10; // Offset for the second number
        offset3 = 10; // Offset for the seperator
        offsetY = 2;  // Offset used for the seperator.
        numbers = HUD_ASSET_NUMBERS;
        seperator = HUD_ASSET_SEPERATOR; // :
    } else {
        offset1 = 10;
        offset2 = 8;
        offset3 = 7;
        offsetY = 0; // Small font is the same height
        numbers = HUD_ASSET_NUMBERS_SMALL;
        seperator = HUD_ASSET_SEPERATOR_SMALL; // :
    }
    entry.spriteID = numbers;
    entry.spriteOffset = minutes / 10;
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, (HudElement *) &entry);
    entry.x += offset1;
    entry.spriteOffset = minutes % 10;
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, (HudElement *) &entry);
    entry.spriteOffset = 0;
    entry.x += offset2;
    entry.y += offsetY;
    entry.spriteID = seperator;
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, (HudElement *) &entry);
    entry.spriteID = numbers;
    entry.x += offset3;
    entry.y -= offsetY;
    entry.spriteOffset = seconds / 10;
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, (HudElement *) &entry);
    entry.x += offset1;
    entry.spriteOffset = seconds % 10;
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, (HudElement *) &entry);
    entry.spriteOffset = 0;
    entry.x += offset2;
    entry.y += offsetY;
    entry.spriteID = seperator;
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, (HudElement *) &entry);
    entry.spriteID = numbers;
    entry.x += offset3;
    entry.y -= offsetY;
    entry.spriteOffset = hundredths / 10;
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, (HudElement *) &entry);
    entry.x += offset1;
    entry.spriteOffset = hundredths % 10;
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, (HudElement *) &entry);
}
