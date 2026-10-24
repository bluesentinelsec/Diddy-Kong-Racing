#include "hud_internal.h"

/**
 * Render race result for multiplayer races or battles after that player has finished.
 * Displays the finish position, and in races, will display the race time.
 */
void hud_finish_position(Object_Racer *racer) {
    s32 racerCount;
#if VERSION == VERSION_80
    static char sDidNotFinish[] = "DID NOT FINISH";
#define DID_NOT_FINISH sDidNotFinish
#define FINISH_TEXT_OFFSET 40
#define FINISH_TIME_OFFSET 0
#elif VERSION == VERSION_79
    static char sDidNotFinish[] = { 0x80, 0xEB, 0x80, 0xB9, 0x80, 0xD1, 0x80, 0xB9,
                                    0x80, 0xF3, 0x80, 0xC5, 0x80, 0xC9, 0x80, 0x34 };
#define DID_NOT_FINISH sDidNotFinish
#define FINISH_TEXT_OFFSET 48
#define FINISH_TIME_OFFSET 2
#else
#define DID_NOT_FINISH "DID NOT FINISH"
#define FINISH_TEXT_OFFSET 40
#define FINISH_TIME_OFFSET 0
#endif

    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1]);
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_2]);
    if (!(level_type() & RACETYPE_CHALLENGE_BATTLE) && is_in_two_player_adventure() == FALSE) {
        get_racer_objects(&racerCount);
        if (gNumActivePlayers >= 2 &&
            (is_in_two_player_adventure() == FALSE || is_postrace_viewport_active() == FALSE)) {
            set_text_font(FONT_COLOURFUL);
            if (racerCount != racer->finishPosition) {
                set_text_colour(255, 255, 255, 0, 255);
                draw_text(&gHudDL, gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x - FINISH_TEXT_OFFSET,
                          gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y, "RACE", ALIGN_TOP_LEFT);
                hud_timer_render(gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x + FINISH_TIME_OFFSET,
                                 gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y,
                                 gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.minutes,
                                 gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.seconds,
                                 gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.hundredths, 1);
                draw_text(&gHudDL, gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x - FINISH_TEXT_OFFSET,
                          gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.y, "LAP", ALIGN_TOP_LEFT);
                hud_timer_render(gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x + FINISH_TIME_OFFSET,
                                 gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.y,
                                 gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.minutes,
                                 gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.seconds,
                                 gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.hundredths, 1);
            } else {
                draw_text(&gHudDL, gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x - 35,
                          gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y, DID_NOT_FINISH, ALIGN_TOP_LEFT);
            }
        }
    }
}

void hud_race_finish_multiplayer(Object_Racer *racer, s32 updateRate) {
    UNUSED s32 *var_a0;
    UNUSED s32 var_t0;
    s32 var_a2;
    s32 var_v1;
    s32 raceType;
    f32 var_f0;
    f32 var_f12;
    f32 var_f14;
    f32 var_f2;
    s32 totalRaceTime;
    s32 fastestLapTime;
    s32 minutes;
    s32 seconds;
    s32 hundredths;
    LevelHeader *curLevelHeader;
    s32 temp_v0_4;

    raceType = level_type();
    if (is_in_two_player_adventure() && is_postrace_viewport_active() && gAdventurePlayerFinish == FALSE) {
        if (is_player_two_in_control()) {
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.y -= 108.0f;
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_2].pos.y -= 108.0f;
            gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y -= 108.0f;
            gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.y -= 108.0f;
        }
        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.y += 35.0f;
        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_2].pos.y += 35.0f;
        gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y += 35.0f;
        gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.y += 35.0f;
        gHudAudioData[0].playerIndex = racer->playerIndex;
        gAdventurePlayerFinish = TRUE;
    }

    switch (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status) {
        case 0:
            if (!(raceType & RACETYPE_CHALLENGE_BATTLE)) {
                gHudAudioData[0].volume = 127;
                gHudAudioData[0].volumeRamp = 0;
                gHudAudioData[0].playerIndex = racer->playerIndex;
                sound_play(SOUND_WHOOSH1, NULL);
            }
            switch (gHUDNumPlayers) {
                case 0:
                case 1:
                    gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.targetPos = -20;
                    gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.targetPos = -15;
                    gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.targetPos = -15;
                    break;
                default:
                    if (racer->playerIndex == PLAYER_ONE || racer->playerIndex == PLAYER_THREE) {
                        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.targetPos = -90;
                        gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.targetPos = -85;
                        gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.targetPos = -85;
                    } else {
                        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.targetPos = 55;
                        gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.targetPos = 60;
                        gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.targetPos = 60;
                    }
                    break;
            }
            if (osTvType == OS_TV_TYPE_PAL) {
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.targetPos -= 4;
                gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.targetPos -= 4;
                gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.targetPos -= 4;
            }
            if (racer->finishPosition < 4) {
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_2].spriteOffset = racer->finishPosition - 1;
            } else {
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_2].spriteOffset = 3;
            }
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].spriteOffset = racer->finishPosition - 1;
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = 1;
            switch (gNumActivePlayers) {
                case 2:
                    var_f12 = 360.0f;
                    var_f14 = -40.0f;
                    if (racer->playerIndex == PLAYER_ONE) {
                        var_f0 = 75.0f;
                        var_f2 = 90.0f;
                    } else {
                        var_f0 = 183.0f;
                        var_f2 = 198.0f;
                    }
                    if (osTvType == OS_TV_TYPE_PAL) {
                        var_f0 *= 1.1;
                        var_f2 *= 1.1;
                        var_f12 -= 4.0f;
                        var_f14 -= 4.0f;
                        var_f0 = (s32) var_f0;
                        var_f2 = (s32) var_f2;
                        if (racer->playerIndex == 0) {
                            var_f0 -= 9.0f;
                            var_f2 -= 9.0f;
                        }
                    }
                    gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x = var_f12;
                    gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y = var_f0;
                    gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x = var_f14;
                    gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.y = var_f2;
                    break;
                case 3:
                case 4:
                    if (racer->playerIndex < 2) {
                        var_f0 = 75.0f;
                        var_f2 = 90.0f;
                    } else {
                        var_f0 = 183.0f;
                        var_f2 = 198.0f;
                    }
                    if (racer->playerIndex == PLAYER_ONE || racer->playerIndex == PLAYER_THREE) {
                        var_f12 = 70.0f;
                        var_f14 = -230.0f;
                    } else {
                        var_f12 = 230.0f;
                        var_f14 = -70.0f;
                    }
                    if (osTvType == OS_TV_TYPE_PAL) {
                        var_f0 *= 1.1;
                        var_f2 *= 1.1;
                        var_f12 -= 4.0f;
                        var_f14 -= 4.0f;
                        var_f0 = (s32) var_f0;
                        var_f2 = (s32) var_f2;
                        if (racer->playerIndex < 2) {
                            var_f0 -= 9.0f;
                            var_f2 -= 9.0f;
                        }
                    }
                    gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x = var_f12;
                    gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y = var_f0;
                    gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x = var_f14;
                    gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.y = var_f2;
                    break;
            }
            if (gNumActivePlayers != 1) {
                curLevelHeader = level_header();
                totalRaceTime = 0;
                fastestLapTime = racer->lap_times[0];
                temp_v0_4 = 0;
                for (updateRate = 0; temp_v0_4 < curLevelHeader->laps; updateRate++, temp_v0_4++) {
                    totalRaceTime += racer->lap_times[updateRate];
                    if (racer->lap_times[updateRate] < fastestLapTime) {
                        fastestLapTime = racer->lap_times[updateRate];
                    }
                }
                get_timestamp_from_frames(fastestLapTime, &minutes, &seconds, &hundredths);
                gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.minutes = minutes;
                gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.seconds = seconds;
                gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.hundredths = hundredths;
                get_timestamp_from_frames(totalRaceTime, &minutes, &seconds, &hundredths);
                gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.minutes = minutes;
                gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.seconds = seconds;
                gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.hundredths = hundredths;
            }
            break;
        case 1:
            var_a2 = TRUE;
            temp_v0_4 = (updateRate * 13);
            if (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x <
                (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.targetPos - temp_v0_4) + 160) {
                var_v1 = temp_v0_4;
                var_a2 = FALSE;
            } else {
                var_v1 = (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.targetPos + 160) -
                         gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x;
            }
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x += var_v1;
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_2].pos.x += var_v1;
            if (gNumActivePlayers != 1) {
                if (gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x >
                    (gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.targetPos + temp_v0_4 + 160)) {
                    var_v1 = temp_v0_4;
                    var_a2 = FALSE;
                } else {
                    var_v1 = gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x -
                             (gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.targetPos + 160);
                }
                gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x -= var_v1;
                var_v1 = temp_v0_4;
                if (gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x <
                    (gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.targetPos - var_v1) + 160) {
                    var_a2 = FALSE;
                } else {
                    var_v1 = (gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.targetPos + 160) -
                             gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x;
                }
                gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x += var_v1;
            }
            if (var_a2) {
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = 2;
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTimer = -120;
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTicks = 0;
                if (gHUDVoiceSoundMask == NULL) {
                    if (raceType == RACETYPE_CHALLENGE_BATTLE) {
                        switch (racer->finishPosition) {
                            case 1:
                            case 2:
                                sound_play(SOUND_UNK_146, &gHUDVoiceSoundMask);
                                break;
                            case 3:
                                sound_play(SOUND_VOICE_TT_TOO_BAD, &gHUDVoiceSoundMask);
                                break;
                            default:
                                sound_play(SOUND_VOICE_TT_UNLUCKY, &gHUDVoiceSoundMask);
                                break;
                        }
                    } else if (timetrial_ghost_staff() == NULL) {
                        hud_time_trial_message(&racer->playerIndex);
                    }
                }
            }
            hud_finish_position(racer);
            break;
        case 2:
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTimer += updateRate;

            if (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTimer >= 120) {
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTimer = -120;
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTicks++;
            }
            if (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTicks == 2) {
                if (gNumActivePlayers == 1 || is_in_two_player_adventure()) {
                    gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = 3;
                    sound_play(SOUND_WHOOSH1, NULL);
                } else {
                    gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = 4;
                }
            }
            hud_finish_position(racer);
            break;
        case 3:
            if (raceType == RACETYPE_CHALLENGE_BATTLE) {
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = 4;
            } else {
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x -= (updateRate * 13);
                gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_2].pos.x -= (updateRate * 13);
                if (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x < -200.0f) {
                    gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = 4;
                }
                gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x += (updateRate * 13);
                gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x += (updateRate * 13);
            }
            hud_finish_position(racer);
            break;
        case 4:
            if (racer->playerIndex == gHudAudioData[0].playerIndex) {
                gHudAudioData[0].volumeRamp = -1;
            }
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = 5;
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTimer = 0;
            hud_finish_position(racer);
            break;
        default:
            hud_finish_position(racer);
            break;
    }
}
