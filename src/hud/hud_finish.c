#include "hud_internal.h"

// Stages of the finish banner (HUD_CHALLENGE_FINISH_POS_1) in hud_race_finish_multiplayer.
enum MultiplayerFinishStages {
    MPFINISH_SETUP,     // Lay out the banner and time panels, then advance.
    MPFINISH_SLIDE_IN,  // Slide the banner (and time panels) into place.
    MPFINISH_HOLD,      // Keep the banner onscreen for a few seconds.
    MPFINISH_SLIDE_OUT, // Slide the banner back out.
    MPFINISH_END,       // Fade out the crowd, then advance.
    MPFINISH_DONE       // Do nothing.
};

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

/**
 * Finish banner state 0: starts the crowd cheer, sets where the finish position and time panels
 * slide to, picks the position sprites, and lays out the race and lap time panels.
 */
static void hud_finish_mp_setup(Object_Racer *racer, s32 raceType) {
    f32 timeY;
    f32 timeX;
    f32 lapX;
    f32 lapY;

    if (!(raceType & RACETYPE_CHALLENGE_BATTLE)) {
        gHudAudioData[0].volume = 127;
        gHudAudioData[0].volumeRamp = 0;
        gHudAudioData[0].playerIndex = racer->playerIndex;
        sound_play(SOUND_WHOOSH1, NULL);
    }
    switch (gHUDNumPlayers) {
        case ONE_PLAYER:
        case TWO_PLAYERS:
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
    gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = MPFINISH_SLIDE_IN;
    switch (gNumActivePlayers) {
        case 2:
            timeX = 360.0f;
            lapX = -40.0f;
            if (racer->playerIndex == PLAYER_ONE) {
                timeY = 75.0f;
                lapY = 90.0f;
            } else {
                timeY = 183.0f;
                lapY = 198.0f;
            }
            if (osTvType == OS_TV_TYPE_PAL) {
                timeY *= 1.1;
                lapY *= 1.1;
                timeX -= 4.0f;
                lapX -= 4.0f;
                timeY = (s32) timeY;
                lapY = (s32) lapY;
                if (racer->playerIndex == 0) {
                    timeY -= 9.0f;
                    lapY -= 9.0f;
                }
            }
            gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x = timeX;
            gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y = timeY;
            gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x = lapX;
            gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.y = lapY;
            break;
        case 3:
        case 4:
            if (racer->playerIndex < 2) {
                timeY = 75.0f;
                lapY = 90.0f;
            } else {
                timeY = 183.0f;
                lapY = 198.0f;
            }
            if (racer->playerIndex == PLAYER_ONE || racer->playerIndex == PLAYER_THREE) {
                timeX = 70.0f;
                lapX = -230.0f;
            } else {
                timeX = 230.0f;
                lapX = -70.0f;
            }
            if (osTvType == OS_TV_TYPE_PAL) {
                timeY *= 1.1;
                lapY *= 1.1;
                timeX -= 4.0f;
                lapX -= 4.0f;
                timeY = (s32) timeY;
                lapY = (s32) lapY;
                if (racer->playerIndex < 2) {
                    timeY -= 9.0f;
                    lapY -= 9.0f;
                }
            }
            gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x = timeX;
            gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.y = timeY;
            gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x = lapX;
            gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.y = lapY;
            break;
    }
}

/**
 * Finish banner state 0, multiplayer: totals the racer's lap times and stores the race time and
 * best lap in the HUD's time panels.
 */
static void hud_finish_mp_lap_times(Object_Racer *racer) {
    s32 lap;
    s32 totalRaceTime;
    s32 fastestLapTime;
    s32 minutes;
    s32 seconds;
    s32 hundredths;
    LevelHeader *curLevelHeader;
    s32 lapsCounted;

    curLevelHeader = level_header();
    totalRaceTime = 0;
    fastestLapTime = racer->lap_times[0];
    lapsCounted = 0;
    for (lap = 0; lapsCounted < curLevelHeader->laps; lap++, lapsCounted++) {
        totalRaceTime += racer->lap_times[lap];
        if (racer->lap_times[lap] < fastestLapTime) {
            fastestLapTime = racer->lap_times[lap];
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

/**
 * Finish banner state 1: slides the finish position (and in multiplayer, the time panels) towards
 * their targets. Once everything is in place, moves to state 2 and plays the finish voice line.
 */
static void hud_finish_mp_slide_in(Object_Racer *racer, s32 raceType, s32 updateRate) {
    s32 allInPlace;
    s32 step;
    s32 slideSpeed;

    allInPlace = TRUE;
    slideSpeed = (updateRate * 13);
    if (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x <
        (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.targetPos - slideSpeed) + 160) {
        step = slideSpeed;
        allInPlace = FALSE;
    } else {
        step = (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.targetPos + 160) -
               gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x;
    }
    gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x += step;
    gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_2].pos.x += step;
    if (gNumActivePlayers != 1) {
        if (gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x >
            (gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.targetPos + slideSpeed + 160)) {
            step = slideSpeed;
            allInPlace = FALSE;
        } else {
            step = gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x -
                   (gCurrentHud->entry[HUD_RACE_TIME_NUMBER].timer.targetPos + 160);
        }
        gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x -= step;
        step = slideSpeed;
        if (gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x <
            (gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.targetPos - step) + 160) {
            allInPlace = FALSE;
        } else {
            step = (gCurrentHud->entry[HUD_LAP_TIME_TEXT].timer.targetPos + 160) -
                   gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x;
        }
        gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x += step;
    }
    if (allInPlace) {
        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = MPFINISH_HOLD;
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
}

/**
 * Finish banner state 2: holds the banner for two ticks of its text timer, then moves on to
 * sliding out (state 3) or straight to state 4.
 */
static void hud_finish_mp_hold(s32 updateRate) {
    gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTimer += updateRate;

    if (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTimer >= 120) {
        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTimer = -120;
        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTicks++;
    }
    if (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTicks == 2) {
        if (gNumActivePlayers == 1 || is_in_two_player_adventure()) {
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = MPFINISH_SLIDE_OUT;
            sound_play(SOUND_WHOOSH1, NULL);
        } else {
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = MPFINISH_END;
        }
    }
}

/**
 * Finish banner state 3: slides the banner and time panels back off screen.
 * Battles skip straight to state 4.
 */
static void hud_finish_mp_slide_out(s32 raceType, s32 updateRate) {
    if (raceType == RACETYPE_CHALLENGE_BATTLE) {
        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = MPFINISH_END;
    } else {
        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x -= (updateRate * 13);
        gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_2].pos.x -= (updateRate * 13);
        if (gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].pos.x < -200.0f) {
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = MPFINISH_END;
        }
        gCurrentHud->entry[HUD_RACE_TIME_NUMBER].pos.x += (updateRate * 13);
        gCurrentHud->entry[HUD_LAP_TIME_TEXT].pos.x += (updateRate * 13);
    }
}

/**
 * Render race result for multiplayer races and battles, and for single player races not won,
 * stepping the finish banner through its states.
 */
void hud_race_finish_multiplayer(Object_Racer *racer, s32 updateRate) {
    s32 raceType;

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
        case MPFINISH_SETUP:
            hud_finish_mp_setup(racer, raceType);
            if (gNumActivePlayers != 1) {
                hud_finish_mp_lap_times(racer);
            }
            break;
        case MPFINISH_SLIDE_IN:
            hud_finish_mp_slide_in(racer, raceType, updateRate);
            hud_finish_position(racer);
            break;
        case MPFINISH_HOLD:
            hud_finish_mp_hold(updateRate);
            hud_finish_position(racer);
            break;
        case MPFINISH_SLIDE_OUT:
            hud_finish_mp_slide_out(raceType, updateRate);
            hud_finish_position(racer);
            break;
        case MPFINISH_END:
            if (racer->playerIndex == gHudAudioData[0].playerIndex) {
                gHudAudioData[0].volumeRamp = -1;
            }
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.status = MPFINISH_DONE;
            gCurrentHud->entry[HUD_CHALLENGE_FINISH_POS_1].raceFinishPos.textOutTimer = 0;
            hud_finish_position(racer);
            break;
        default:
            hud_finish_position(racer);
            break;
    }
}
