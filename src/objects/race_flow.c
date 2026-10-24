#include "objects_internal.h"

// Settings::cutsceneFlags bit set once the post-race balloon cutscene has been watched; after that A skips it.
#define CUTSCENE_BALLOON_SEEN 0x40000
// Settings racer best_times bit for a new course record (the low bits mark record laps).
#define BEST_TIMES_COURSE_RECORD 0x80
// A time trial is only kept as the player ghost if the course took under three minutes (in frames).
#define GHOST_SAVE_TIME_LIMIT 10800
// EEPROM settings: one "staff ghost beaten" bit per main track, starting at bit 4.
#define EEPROM_TT_BEATEN_FIRST 16
#define EEPROM_TT_BEATEN_ALL 0xFFFFF0

static FadeTransition gTajChallengeTransition =
    FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_OUT, FADE_COLOR_BLACK, 30, 15);
static FadeTransition gBalloonCutsceneTransition =
    FADE_TRANSITION(FADE_CIRCLE, FADE_FLAG_NONE, FADE_COLOR_BLACK, 30, 15);

extern s16 gGhostMapID;

/**
 * A challenge racer's score for ranking those who have not finished: bananas lost in battle,
 * laps elsewhere, with eggs counting progress towards the next lap. Clamped to 0..10.
 */
static s8 challenge_score(Object_Racer *racer, s8 raceType) {
    s8 score;

    if (raceType == RACETYPE_CHALLENGE_BATTLE) {
        score = 10 - racer->bananas;
    } else {
        score = racer->lap;
        if (raceType == RACETYPE_CHALLENGE_EGGS) {
            score *= 3;
            if (racer->eggHudCounter != 0) {
                score += 2;
            } else if (racer->held_obj != NULL) {
                score += 1;
            }
        }
    }
    if (score > 10) {
        score = 10;
    }
    if (score < 0) {
        score = 0;
    }
    return score;
}

/**
 * Hand out the remaining finish positions by score, best first. In battle mode the last to
 * finish wins, so the order is flipped.
 */
static void challenge_rank_remaining(Object_Racer **racer, s8 *score, s8 raceType) {
    s16 best;
    s16 bestIndex;
    s32 i;

    do {
        bestIndex = -1;
        best = -1;
        for (i = 0; i < gNumRacers; i++) {
            if (!racer[i]->raceFinished && score[i] >= best) {
                best = score[i];
                bestIndex = i;
            }
        }
        if (bestIndex != -1) {
            if (raceType == RACETYPE_CHALLENGE_BATTLE) {
                racer[bestIndex]->finishPosition = 5 - gNumFinishedRacers;
            } else {
                racer[bestIndex]->finishPosition = gNumFinishedRacers;
            }
            gNumFinishedRacers++;
            racer[bestIndex]->raceFinished = TRUE;
        }
    } while (bestIndex != -1);
}

/**
 * Winning a challenge for the first time in adventure mode awards a T.T. amulet piece.
 * Returns the new amulet count, or 0 if nothing was awarded.
 */
static s32 challenge_award_amulet(Settings *settings, Object_Racer **racer) {
    s32 amulet;

    if (is_in_tracks_mode()) {
        return 0;
    }
    if (racer[0]->finishPosition != 1 && !(is_in_two_player_adventure() && racer[1]->finishPosition == 1)) {
        return 0;
    }
    if (settings->courseFlagsPtr[settings->courseId] & RACE_CLEARED) {
        return 0;
    }
    settings->courseFlagsPtr[settings->courseId] |= RACE_CLEARED;
    amulet = settings->ttAmulet + 1;
    if (amulet > 4) {
        amulet = 4;
    }
    settings->ttAmulet = amulet;
    return amulet;
}

/**
 * Record the finish order as next race's starting positions, play the victory or defeat
 * jingle, and note whether player two now leads a shared adventure.
 */
static void challenge_store_results(Settings *settings, Object_Racer **racer) {
    s32 sequence;
    s32 nextPosition;
    s32 i;

    for (i = 0; i < 8; i++) {
        settings->racers[i].starting_position = -1;
    }
    sequence = SEQUENCE_BATTLE_LOSE;
    for (i = 0; i < gNumRacers; i++) {
        if (racer[i]->playerIndex != PLAYER_COMPUTER && racer[i]->finishPosition == 1) {
            sequence = SEQUENCE_BATTLE_VICTORY;
        }
        settings->racers[i].starting_position = racer[i]->finishPosition - 1;
    }
    music_play(sequence);
    nextPosition = 4;
    for (i = 0; i < 8; i++) {
        if (settings->racers[i].starting_position == -1) {
            settings->racers[i].starting_position = nextPosition;
            nextPosition++;
        }
    }
    gSwapLeadPlayer = FALSE;
    if (is_in_two_player_adventure() &&
        settings->racers[PLAYER_TWO].starting_position < settings->racers[PLAYER_ONE].starting_position) {
        gSwapLeadPlayer = TRUE;
    }
}

/**
 * In a shared adventure the player who placed better leads from here on.
 */
static void race_apply_lead_player_swap(void) {
    if (!is_in_two_player_adventure()) {
        return;
    }
    if (gSwapLeadPlayer) {
        gSwapLeadPlayer = FALSE;
        swap_lead_player();
        if (gLeadPlayerIndex != 0) {
            gIsP2LeadPlayer = TRUE;
        }
    } else if (gLeadPlayerIndex != 0) {
        gIsP2LeadPlayer = TRUE;
    }
}

/**
 * Win conditions for the Taj and trophy challenges (battle, eggs, treasure hunt). Battle
 * eliminates racers as they run out of bananas; the challenge ends when the humans are done
 * or three racers have finished, and the rest are ranked by score.
 */
static void race_check_finish_challenge(Settings *settings, LevelHeader *header) {
    Object_Racer *racer[4];
    s8 score[4];
    s16 numHumanRacers;
    s16 numHumanRacersFinished;
    s16 numFinishedRacers;
    s32 amulet;
    s32 i;

    if (header->race_type == RACETYPE_CHALLENGE_EGGS) {
        racer_update_eggs(*gRacers);
    }
    if (gRaceFinishTriggered != FALSE) {
        return;
    }
    numHumanRacers = 0;
    numHumanRacersFinished = 0;
    numFinishedRacers = 0;
    for (i = 0; i < gNumRacers; i++) {
        racer[i] = (*gRacers)[i]->racer;
        // Manage eliminated racers in deathmatch.
        if (header->race_type == RACETYPE_CHALLENGE_BATTLE && racer[i]->bananas <= 0 && !racer[i]->raceFinished) {
            racer[i]->raceFinished = TRUE;
            racer[i]->balloon_quantity = 0;
            racer_sound_free((*gRacers)[i]);
            (*gRacers)[i]->trans.flags |= OBJ_FLAGS_INVISIBLE;
            (*gRacers)[i]->interactObj->flags = INTERACT_FLAGS_NONE;
            racer[i]->finishPosition = 5 - gNumFinishedRacers;
            gNumFinishedRacers++;
        }
        if (racer[i]->playerIndex != PLAYER_COMPUTER) {
            if (racer[i]->raceFinished) {
                numHumanRacersFinished++;
            }
            numHumanRacers++;
        }
        if (racer[i]->raceFinished) {
            numFinishedRacers++;
            if (racer[i]->finishPosition == 0) {
                racer[i]->finishPosition = gNumFinishedRacers;
                gNumFinishedRacers++;
            }
        }
    }
    if (!((header->race_type != RACETYPE_CHALLENGE_BATTLE && numFinishedRacers > 0) ||
          ((((numHumanRacers == 1 && numHumanRacersFinished == 1) ||
             (numHumanRacers >= 2 && numHumanRacersFinished >= numHumanRacers)) ||
            numFinishedRacers >= 3)))) {
        return;
    }
    for (i = 0; i < gNumRacers; i++) {
        score[i] = challenge_score(racer[i], header->race_type);
    }
    challenge_rank_remaining(racer, score, header->race_type);
    gSwapLeadPlayer = FALSE;
    amulet = challenge_award_amulet(settings, racer);
    challenge_store_results(settings, racer);
    if (amulet == 0) {
        race_apply_lead_player_swap();
        postrace_start(0, 30);
    } else {
        level_properties_push(SPECIAL_MAP_ID_NO_LEVEL, 0, VEHICLE_CAR, CUTSCENE_ID_NONE);
        level_properties_push(ASSET_LEVEL_TTAMULETSEQUENCE, 0, VEHICLE_NO_OVERRIDE, settings->ttAmulet - 1);
        race_finish_adventure(TRUE);
    }
    gRaceFinishTriggered = TRUE;
}

/**
 * Rank every racer against every other by checkpoint progress (finished racers first) and
 * record it as racerOrder. The HUD position only follows once the order has held for a
 * couple of frames.
 */
static void race_update_positions(LevelHeader *header) {
    Object_Racer *racer;
    Object_Racer *other;
    s32 prevOrder;
    s32 order;
    s32 i;
    s32 j;

    for (i = 0; i < gNumRacers; i++) {
        order = 1;
        racer = (*gRacers)[i]->racer;
        prevOrder = racer->racerOrder;
        for (j = 0; j < gNumRacers; j++) {
            if (j == i) {
                continue;
            }
            other = (*gRacers)[j]->racer;
            if (racer->raceFinished == FALSE && other->raceFinished != FALSE) {
                order++;
            } else if (racer->courseCheckpoint < other->courseCheckpoint) {
                order++;
            } else if (racer->courseCheckpoint == other->courseCheckpoint) {
                if (other->checkpointRank < racer->checkpointRank) {
                    order++;
                }
                if (racer->checkpointRank == other->checkpointRank && i < j) {
                    order++;
                }
            }
        }
        racer->racerOrder = order;
        if (racer->lap < header->laps) {
            if (prevOrder == racer->racerOrder) {
                if (racer->positionStableTimer < 2) {
                    if (racer->vehicleID != VEHICLE_LOOPDELOOP) {
                        racer->positionStableTimer++;
                    }
                } else if (racer->racerOrder != racer->racePosition) {
                    racer->positionChangeTimer = 10;
                    racer->racePosition = racer->racerOrder;
                }
            } else {
                racer->positionStableTimer = 0;
            }
        }
    }
}

/**
 * Racers that have completed the laps are marked finished and given the next finish
 * position. Also counts the humans and the finishers.
 */
static void race_mark_finished_laps(LevelHeader *header, s16 *numHumanRacers, s16 *numHumanRacersFinished,
                                    s16 *numFinishedRacers) {
    Object_Racer *racer;
    s32 i;

    for (i = 0; i < gNumRacers; i++) {
        racer = (*gRacers)[i]->racer;
        if (racer->lap >= header->laps && racer->raceFinished == FALSE) {
            if (get_game_mode() != GAMEMODE_UNUSED_4) {
                racer->raceFinished = TRUE;
                racer->finishPosition = gNumFinishedRacers;
                if (gNumFinishedRacers == 1 && racer->playerIndex == PLAYER_COMPUTER) {
                    sound_play(SOUND_WHOOSH5, NULL);
                }
                gNumFinishedRacers++;
            }
        }
        if (racer->playerIndex != PLAYER_COMPUTER) {
            (*numHumanRacers)++;
            if (racer->raceFinished) {
                (*numHumanRacersFinished)++;
                (*numFinishedRacers)++;
            }
        } else if (racer->raceFinished) {
            (*numFinishedRacers)++;
        }
    }
}

/**
 * Rebuild gRacersByPosition: finished racers by finish position, the rest by current
 * order. Racers that collide on a slot are dropped into the first free one.
 */
static void race_sort_by_position(void) {
    Object_Racer *racer;
    s32 position;
    s32 listed;
    s32 i;
    s32 j;

    for (i = 0; i < gNumRacers; i++) {
        gRacersByPosition[i] = 0;
    }
    for (i = 0; i < gNumRacers; i++) {
        racer = (*gRacers)[i]->racer;
        if (racer->raceFinished) {
            position = racer->finishPosition - 1;
        } else {
            position = racer->racerOrder - 1;
        }
        gRacersByPosition[position] = (*gRacers)[i];
    }
    for (i = 0; i < gNumRacers; i++) {
        listed = FALSE;
        for (j = 0; j < gNumRacers; j++) {
            if (gRacersByPosition[j] == (*gRacers)[i]) {
                listed = TRUE;
                break;
            }
        }
        if (listed == FALSE) {
            for (j = 0; j < gNumRacers; j++) {
                if (gRacersByPosition[j] == 0) {
                    gRacersByPosition[j] = (*gRacers)[i];
                    break;
                }
            }
        }
    }
}

/**
 * Once the race is decided, the racers still going are finished in their current order and
 * the humans among them get the finish-line camera.
 */
static void race_finish_remaining_racers(void) {
    Object_Racer *racer;
    s32 i;

    for (i = 0; i < gNumRacers; i++) {
        racer = gRacersByPosition[i]->racer;
        if (racer->raceFinished == FALSE) {
            if (racer->playerIndex >= 0) {
                set_active_camera(racer->playerIndex);
                cam_get_active_camera_no_cutscenes()->mode = CAMERA_FINISH_CHALLENGE;
            }
            racer->raceFinished = TRUE;
            racer->finishPosition = gNumFinishedRacers;
            gNumFinishedRacers++;
        }
    }
}

/**
 * The end of a normal, Horseshoe Gulch or boss race. The race is over when every human has
 * finished (or all but one racer, with several humans), or when a shared adventure's first
 * finisher has already cleared the course. A first clear in adventure mode awards a balloon
 * and plays the sequence; otherwise the results screen follows.
 */
static void race_finish_default(Settings *settings, s16 numHumanRacers, s16 numHumanRacersFinished,
                                s16 numFinishedRacers) {
    Object_Racer *racer;
    s32 adventureClearedEarly;
    s32 awardBalloon;
    s32 i;

    adventureClearedEarly = FALSE;
    if (is_in_two_player_adventure() && numHumanRacersFinished > 0 && get_trophy_race_world_id() == 0 &&
        set_course_finish_flags(settings) != 0) {
        adventureClearedEarly = TRUE;
    }
    if (!((numHumanRacersFinished == numHumanRacers ||
           (numHumanRacers >= 2 && numFinishedRacers >= (gNumRacers - 1))) ||
          adventureClearedEarly)) {
        return;
    }
    if (numHumanRacersFinished != numHumanRacers) {
        race_finish_remaining_racers();
    }
    if (!gIsTimeTrial) {
        for (i = 0; i < gNumRacers; i++) {
            racer = gRacersByPosition[i]->racer;
            settings->racers[racer->racerIndex].starting_position = i;
        }
    }
    gSwapLeadPlayer = FALSE;
    if (is_in_two_player_adventure() &&
        (settings->racers[PLAYER_TWO].starting_position < settings->racers[PLAYER_ONE].starting_position)) {
        gSwapLeadPlayer = TRUE;
    }
    racer = (*gRacersByPosition)->racer;
    gFirstTimeFinish = FALSE;
    if ((settings->gNumRacers == 1 || is_in_two_player_adventure()) && racer->playerIndex != PLAYER_COMPUTER &&
        !is_in_tracks_mode() && get_trophy_race_world_id() == 0) {
        gFirstTimeFinish = TRUE;
    }
    awardBalloon = FALSE;
    if (gFirstTimeFinish && !adventureClearedEarly) {
        awardBalloon = set_course_finish_flags(settings);
    }
    if (adventureClearedEarly) {
        gFirstTimeFinish = TRUE;
        awardBalloon = TRUE;
    }
    if (!awardBalloon) {
        race_apply_lead_player_swap();
        postrace_start(gFirstTimeFinish, 30);
    } else {
        settings->balloonsPtr[settings->worldId]++;
        if (settings->worldId != 0) {
            settings->balloonsPtr[0]++;
        }
        race_finish_adventure(TRUE);
    }
    gRaceFinishTriggered = -1; // -1 doesn't do anything different.
    if (get_number_of_active_players() == 1) {
        race_finish_time_trial();
    }
}

/**
 * Check the win conditions of the current race.
 * This varies based on the race type, so this does a multitude of different possible things.
 * When the race is finished, it will then for the most part trigger the next menu,
 * but adventure mode will start the balloon cutscene if it has not yet been awarded.
 */
void race_check_finish(s32 updateRate) {
    Settings *settings;
    LevelHeader *header;
    Object_Racer *racer;
    s16 numHumanRacers;
    s16 numHumanRacersFinished;
    s16 numFinishedRacers;
    s8 raceType;

    header = level_header();
    settings = get_settings();
    raceType = header->race_type;
    if (raceType != RACETYPE_DEFAULT && raceType != RACETYPE_HORSESHOE_GULCH && raceType != RACETYPE_BOSS) {
        if (raceType & RACETYPE_CHALLENGE) {
            race_check_finish_challenge(settings, header);
        }
        return;
    }

    race_update_positions(header);
    numHumanRacers = 0;
    numHumanRacersFinished = 0;
    numFinishedRacers = 0;
    race_mark_finished_laps(header, &numHumanRacers, &numHumanRacersFinished, &numFinishedRacers);
    race_sort_by_position();

    if (gIsTajChallenge && numHumanRacersFinished != 0) {
        mode_end_taj_race(CHALLENGE_END_FINISH);
    } else if (gBossRaceMode != 0 && numFinishedRacers != 0) {
        // The boss race ends for the player as soon as either racer crosses the line.
        racer = (*gRacers)[0]->racer;
        if (!racer->raceFinished) {
            racer->raceFinished = TRUE;
            racer->finishPosition = gNumFinishedRacers;
            gNumFinishedRacers++;
        }
    } else if (gRaceFinishTriggered == FALSE) {
        race_finish_default(settings, numHumanRacers, numHumanRacersFinished, numFinishedRacers);
    }
}

/**
 * Mark the course as finished for the appropriate mode.
 * Check if it's the first race or the silver coin race before deciding which flag to write.
 * Return whether something was written.
 */
s8 set_course_finish_flags(Settings *settings) {
    Object_Racer *racer;

    racer = gRacersByPosition[PLAYER_ONE]->racer;
    if (racer->playerIndex == PLAYER_COMPUTER) {
        return FALSE;
    }
    gFirstTimeFinish = FALSE;
    if (!(settings->courseFlagsPtr[settings->courseId] & RACE_CLEARED)) {
        if (gIsTimeTrial == FALSE) {
            gFirstTimeFinish = TRUE;
            settings->courseFlagsPtr[settings->courseId] |= RACE_CLEARED;
        }
    } else if (gIsSilverCoinRace && racer->silverCoinCount >= 8 && gIsTimeTrial == FALSE) {
        gFirstTimeFinish = TRUE;
        settings->courseFlagsPtr[settings->courseId] |= RACE_CLEARED_SILVER_COINS;
    }
    return gFirstTimeFinish;
}

/**
 * Sets the countdown ready for the level to fade out.
 * Begins the timer that exits the level in 5 seconds.
 */
void race_finish_adventure(UNUSED s32 unusedArg) {
    gRaceEndTimer = 300;
    gRaceEndStage = 0;
    gRaceFinishUnusedArg = unusedArg;
}

/**
 * Begins counting down, once it reaches 0, start stopping all the race behaviours.
 * This function is also where the Taj Balloon cutscene is also handled.
 * After that's done, write to save and send the player back to the hub.
 */
void race_transition_adventure(s32 updateRate) {
    s32 i;
    Object_Racer *racer;
    Object *prevPort0Racer;
    s32 winnerIndex; // holds the timer before this frame until the winner is searched for
    Object *prevRacer0Obj;
    Settings *settings;
    u32 cutsceneTimerLimit;

    set_pause_lockout_timer(1);
    winnerIndex = gRaceEndTimer;
    gRaceEndTimer -= updateRate;
    if (gRaceEndTimer <= 0) {
        gRaceEndTimer = -1;
    }
    if (winnerIndex > 50 && gRaceEndTimer <= 50) {
        transition_begin(&gRaceEndFade);
    }
    winnerIndex = 0;
    if (gRaceEndStage == 0 && gRaceEndTimer == -1) {
        for (i = 0; i < gNumRacers; i++) {
            racer = (*gRacers)[i]->racer;
            racer->magnetTargetObj = NULL;
            if (racer->playerIndex != PLAYER_COMPUTER && racer->finishPosition == 1) {
                winnerIndex = i;
            }
            if (racer->magnetSoundMask != NULL) {
                sndp_stop(racer->magnetSoundMask);
            }
            if (racer->shieldSoundMask != NULL) {
                audspat_point_stop(racer->shieldSoundMask);
            }
        }
        prevPort0Racer = (*gRacers)[0];
        (*gRacers)[0] = (*gRacers)[winnerIndex];
        (*gRacers)[winnerIndex] = prevPort0Racer;
        racer_sound_free((*gRacers)[0]);
        hud_audio_init();
        reset_rocket_sound_timer();
        sndp_stop_all_looped();
        if (is_in_two_player_adventure()) {
            set_scene_viewport_num(0);
            cam_set_layout(VIEWPORT_LAYOUT_1_PLAYER);
            prevRacer0Obj = (*gRacers)[0];
            prevPort0Racer = gRacersByPort[0];
            racer = prevRacer0Obj->racer;
            gRacersByPort[0] = prevRacer0Obj;
            gRacersByPort[1] = prevPort0Racer;
            if (gSwapLeadPlayer != 0) {
                gSwapLeadPlayer = 0;
                swap_lead_player();
                racer->playerIndex = 0;
            }
        }
        gNumRacersSaved = gNumRacers;
        gRaceEndStage = 1;
    }
    if (gRaceEndStage == 1) {
        hud_visibility(FALSE);
        gNumRacersSaved--;
        if (gNumRacersSaved > 0) {
            i = 0;
            while (i < gNumRacers && gRacersByPosition[i] != (*gRacers)[gNumRacersSaved]) {
                i++;
            }
            if (i < gNumRacers) {
                for (; i < (gNumRacers - 1); i++) {
                    gRacersByPosition[i] = gRacersByPosition[i + 1];
                }
            }
            free_object((*gRacers)[gNumRacersSaved]);
            (*gRacers)[gNumRacersSaved] = NULL;
            gNumRacers--;

        } else {
            gRaceEndStage = 2;
        }
    }
    if (gRaceEndStage == 2) {
        prevPort0Racer = (*gRacers)[0];
        racer = prevPort0Racer->racer;
        racer_find_start_node(prevPort0Racer, racer);
        racer->raceFinished = FALSE;
        gRaceEndStage = 3;
        cutscene_set(CUTSCENE_ID_UNK_A);
        gBalloonCutsceneTimer = 0;
        anim_objects_update();
    }
    if (gRaceEndStage == 3) {
        transition_begin(&gRaceEndTransition);
        gRaceEndStage = 4;
        set_anti_aliasing(TRUE);
    }
    if (gRaceEndStage == 4) {
        set_anti_aliasing(TRUE);
        disable_racer_input();
        if (!(level_type() & RACETYPE_CHALLENGE_BATTLE)) {
            if (osTvType == OS_TV_TYPE_PAL) {
                cutsceneTimerLimit = 415;
            } else {
                cutsceneTimerLimit = 540;
            }
            gBalloonCutsceneTimer += updateRate;
            if (gBalloonCutsceneTimer < cutsceneTimerLimit) {
                minimap_fade(1);
            } else {
                hud_visibility(1);
            }
        }
        i = input_pressed(PLAYER_ONE) & A_BUTTON;
        settings = get_settings();
        if (!(settings->cutsceneFlags & CUTSCENE_BALLOON_SEEN)) {
            i = 0;
        }
        if (anim_prev_frame_state() != 0 || (i != 0 && check_fadeout_transition() == 0)) {
            if (i != 0) {
                transition_begin(&gBalloonCutsceneTransition);
            }
            level_transition_begin(2);
            gRaceEndStage = 5;
            settings->cutsceneFlags |= CUTSCENE_BALLOON_SEEN;
        }
    }
}

/**
 * Returns the race finish timer.
 */
s16 race_finish_timer(void) {
    return gRaceEndTimer;
}

/**
 * Return the timer used for the collectable balloon cutscene.
 */
u32 get_balloon_cutscene_timer(void) {
    return gBalloonCutsceneTimer;
}

/**
 * Checks if the fastest lap or the race time is faster than the current record.
 * Save those if they are, and if the staff ghost is not enabled, enable it for the next attempt.
 * The staff ghost comparison is also called here.
 */
void race_finish_time_trial(void) {
    s32 bestCourseTime;
    s32 bestRacerTime;
    s32 i;
    s32 courseTime;
    s32 vehicleID;
    s32 curRacerLapTime;
    s32 j;
    Object_Racer *curRacer;
    Object_Racer *bestRacer;
    Settings *settings;
    LevelHeader *levelHeader;

    levelHeader = level_header();
    settings = get_settings();
    settings->timeTrialRacer = 0;
    settings->bestLapRacerAndLap[1] = 0;
    settings->bestLapRacerAndLap[0] = 0;
    bestCourseTime = 36001;
    bestRacerTime = 36001;
    bestRacer = gRacersByPosition[0]->racer;
    for (i = 0; i < gNumRacers; i++) {
        curRacer = gRacersByPosition[i]->racer;
        if (curRacer->racerIndex >= 0) {
            if (curRacer->racerIndex < get_number_of_active_players()) {
                settings->racers[curRacer->racerIndex].best_times = 0;
                vehicleID = curRacer->vehicleIDPrev;
                if (vehicleID >= VEHICLE_CAR && vehicleID < NUMBER_OF_PLAYER_VEHICLES) {
                    courseTime = 0;
                    for (j = 0; j < levelHeader->laps && j < 5; j++) {
                        settings->racers[curRacer->racerIndex].lap_times[j] = curRacer->lap_times[j];
                        curRacerLapTime = curRacer->lap_times[j];
                        courseTime += curRacerLapTime;
                        if (curRacerLapTime < bestRacerTime) {
                            settings->bestLapRacerAndLap[1] = j;
                            settings->bestLapRacerAndLap[0] = curRacer->racerIndex;
                            bestRacerTime = curRacerLapTime;
                        }
                    }
                    settings->racers[curRacer->racerIndex].course_time = courseTime;
                    if (courseTime < bestCourseTime) {
                        bestCourseTime = courseTime;
                        settings->timeTrialRacer = curRacer->racerIndex;
                        bestRacer = curRacer;
                    }
                }
            }
        }
    }
    settings->display_times = FALSE;
    if (gIsTimeTrial) {
        vehicleID = gPrevTimeTrialVehicle;
        if (vehicleID >= NUMBER_OF_PLAYER_VEHICLES || vehicleID < VEHICLE_CAR) {
            vehicleID = VEHICLE_CAR;
        }
        settings->display_times = TRUE;
        if (settings->bestLapRacerAndLap[0] == 0) {
            if ((settings->flapTimesPtr[vehicleID][settings->courseId] == NULL) ||
                (bestRacerTime < settings->flapTimesPtr[vehicleID][settings->courseId])) {
                settings->flapTimesPtr[vehicleID][settings->courseId] = bestRacerTime;
                settings->racers[settings->bestLapRacerAndLap[0]].best_times |= 1 << settings->bestLapRacerAndLap[1];
            }
        }
        if (settings->timeTrialRacer == 0) {
            if ((settings->courseTimesPtr[vehicleID][settings->courseId] == NULL) ||
                (bestCourseTime < settings->courseTimesPtr[vehicleID][settings->courseId])) {
                settings->courseTimesPtr[vehicleID][settings->courseId] = bestCourseTime;
                settings->racers[settings->timeTrialRacer].best_times |= BEST_TIMES_COURSE_RECORD;
            }
        }
        if (((!vehicleID) && (!vehicleID)) && (!vehicleID)) {} // Fakematch
        if (settings->timeTrialRacer == 0) {
            if (bestCourseTime < GHOST_SAVE_TIME_LIMIT &&
                (vehicleID != gTimeTrialVehicle || timetrial_map_id() != level_id() ||
                 bestCourseTime < gTimeTrialTime)) {
                gTimeTrialTime = bestCourseTime;
                gTimeTrialVehicle = gPrevTimeTrialVehicle;
                gTimeTrialCharacter = settings->racers[0].character;
                timetrial_swap_player_ghost(level_id());
                gHasGhostToSave = TRUE;
            }
            if (osTvType == OS_TV_TYPE_PAL) {
                bestCourseTime = (bestCourseTime * 6) / 5;
            }
            if (bestCourseTime < gTTGhostTimeToBeat) {
                if (gTimeTrialStaffGhost) {
                    tt_ghost_beaten(level_id(), &bestRacer->playerIndex);
                } else {
                    hud_time_trial_message(&bestRacer->playerIndex);
                }
            } else {
                hud_time_trial_message(&bestRacer->playerIndex);
            }
        }
    }
}

/**
 * Returns true if the player ghost data is valid for playback.
 */
s32 timetrial_valid_player_ghost(void) {
    if (timetrial_map_id() != level_id()) {
        return FALSE;
    } else {
        if (gTimeTrialVehicle != gPrevTimeTrialVehicle) {
            return FALSE;
        } else {
            return TRUE;
        }
    }
}

/**
 * Return the player ghost object.
 */
Object *timetrial_player_ghost(void) {
    return gGhostObjPlayer;
}

/**
Pretty sure this determines whether or not you're eligible to race TT ghost in track select
when TT is on. It looks like it checks some ghost data makes sure you've got a ghost for that level
with the default vehicle,
Returns 0 if TT ghost was loaded successfully.
*/
s32 timetrial_load_staff_ghost(s32 mapId) {
    TTGhostTable *ghostTable;
    TTGhostTable *prevGhostTable;
    s32 ret;
    TTGhostTable *nextGhostTable;

    gMapDefaultVehicle = leveltable_vehicle_default(mapId);
    ghostTable = (TTGhostTable *) asset_table_load(ASSET_TTGHOSTS_TABLE);
    {
        // Big-endian entries: mapId/defaultVehicleId are u8, ghostOffset is s32.
        s32 n;
        _Static_assert(sizeof(TTGhostTable) == 8, "TTGhostTable layout drifted from N64");
        for (n = 0; n < (s32) (asset_table_size(ASSET_TTGHOSTS_TABLE) / sizeof(TTGhostTable)); n++) {
            pc_swap32_buf(&ghostTable[n].ghostOffset, 4);
        }
    }

    nextGhostTable = ghostTable;
    do {
        prevGhostTable = nextGhostTable;
        if ((prevGhostTable->mapId == mapId) && (prevGhostTable->defaultVehicleId == gMapDefaultVehicle)) {
            break;
        }
        nextGhostTable++;
    } while (prevGhostTable->mapId != 0xFF);

    ret = 1;

    if (prevGhostTable->mapId != 0xFF) {
        ret = load_tt_ghost(nextGhostTable->ghostOffset, nextGhostTable[1].ghostOffset - nextGhostTable->ghostOffset,
                            &gTTGhostTimeToBeat);
    }

    mempool_free(ghostTable);
    return ret;
}

/**
 * Return true if this object is the time trial ghost.
 */
s32 timetrial_staff_ghost_check(Object *obj) {
    return obj == gGhostObjStaff;
}

/**
 * Free ghost data then save the players victory.
 * Check if the player has beaten every time and unlock TT.
 * Otherwise, tell them to try another track.
 */
void tt_ghost_beaten(s32 levelId, s16 *playerId) {
    s32 trackIdCount;
    s8 *mainTrackIds;

    gGhostObjStaff = NULL;
    timetrial_free_staff_ghost();
    gTimeTrialStaffGhost = FALSE;
    mainTrackIds = (s8 *) get_misc_asset(ASSET_MISC_MAIN_TRACKS_IDS);
    trackIdCount = 0;
    while (mainTrackIds[trackIdCount] != -1 && mainTrackIds[trackIdCount] != levelId) {
        trackIdCount++;
    }
    if (gBeatStaffGhost) {
        // Save that TT has been beaten for this track.
        set_eeprom_settings_value(EEPROM_TT_BEATEN_FIRST << trackIdCount);
        // Check if TT has been beaten for all tracks.
        if ((get_eeprom_settings() & EEPROM_TT_BEATEN_ALL) == EEPROM_TT_BEATEN_ALL) {
            set_magic_code_flags(CHEAT_CONTROL_TT);
            sound_play(SOUND_VOICE_TT_BEAT_ALL_TIMES, NULL);
            sound_play_delayed(SOUND_VOICE_TT_UNLOCKED, NULL, 1.5f);
            set_current_text(
                ASSET_GAME_TEXT_84); // Text for "You have beaten all my times!" and then "Now you can PICK me!"
        } else {
            sound_play(SOUND_VOICE_TT_WELL_DONE, NULL);
            sound_play_delayed(SOUND_VOICE_TT_TRY_ANOTHER_TRACK, NULL, 1.0f);
            set_current_text(ASSET_GAME_TEXT_83); // Text for "Well Done! Now try another track."
        }
        gBeatStaffGhost = FALSE;
        return;
    }
    hud_time_trial_message(playerId);
}

/**
 * Compare if the course record is enough to unlock the staff ghost.
 * Also check if the ghost tiself has been beaten.
 * Store both results and return if there should be a ghost.
 */
u8 timetrial_init_staff_ghost(s32 trackId) {
    s32 i;
    s8 *mainTrackIds;
    u16 *staffTime;
    Settings *settings;

    gBeatStaffGhost = FALSE;
    gTimeTrialStaffGhost = FALSE;
    settings = get_settings();
    if (leveltable_vehicle_default(trackId) == (Vehicle) gPrevTimeTrialVehicle) {
        mainTrackIds = (s8 *) get_misc_asset(ASSET_MISC_MAIN_TRACKS_IDS);
        staffTime = (u16 *) get_misc_asset(ASSET_MISC_GHOST_UNLOCK_TIMES);
        for (i = 0; mainTrackIds[i] != -1 && trackId != mainTrackIds[i]; i++) {}
        if (mainTrackIds[i] != -1) {
            if (staffTime[i] >= settings->courseTimesPtr[gPrevTimeTrialVehicle][trackId]) {
                // Check if TT has been beaten?
                if (!(get_eeprom_settings() & (EEPROM_TT_BEATEN_FIRST << i))) {
                    gBeatStaffGhost = TRUE;
                }
                if (timetrial_load_staff_ghost(trackId) == 0) {
                    gTimeTrialStaffGhost = TRUE;
                }
            }
        }
    }
    return gTimeTrialStaffGhost;
}

/**
 * Return the time trial staff ghost object.
 */
Object *timetrial_ghost_staff(void) {
    return gGhostObjStaff;
}

/**
 * Return true if the tt ghost is unbeaten for this track.
 */
s32 timetrial_staff_unbeaten(void) {
    return gBeatStaffGhost == FALSE;
}

/**
 * Calls a function to start loading the player ghost data from the controller pak.
 * Returns the controller pak status. 0 means good.
 */
s32 timetrial_init_player_ghost(s32 playerID) {
    s16 characterID;
    s16 time;
    s32 cpakStatus;
    s32 ghostMapID;

    ghostMapID = timetrial_map_id();
    if (level_id() != ghostMapID || gTimeTrialVehicle != gPrevTimeTrialVehicle) {
        cpakStatus = timetrial_load_player_ghost(playerID, level_id(), gPrevTimeTrialVehicle, &characterID, &time);
        if (cpakStatus == CONTROLLER_PAK_GOOD) {
            gTimeTrialVehicle = gPrevTimeTrialVehicle;
            gTimeTrialCharacter = characterID;
            gTimeTrialTime = time;
        }
        return cpakStatus;
    }
    return timetrial_load_player_ghost(playerID, level_id(), gPrevTimeTrialVehicle, NULL, NULL);
}

/**
 * Call a function to write the ghost data to the controller pak.
 * Returns the controller pak status. 0 is good.
 */
SIDeviceStatus timetrial_save_player_ghost(s32 controllerIndex) {
    return timetrial_write_player_ghost(controllerIndex, timetrial_map_id(), gTimeTrialVehicle, gTimeTrialCharacter,
                                        gTimeTrialTime);
}

/**
 * Returns whether there's valid ghost data to save.
 */
u8 has_ghost_to_save(void) {
    return gHasGhostToSave;
}

/**
 * Resets the variables used for ghost data saving.
 */
void set_ghost_none(void) {
    gGhostMapID = -1;
    gHasGhostToSave = FALSE;
}

/**
 * Prepares the player racer for a Taj Challenge race.
 * Prevents them from pausing for 10 frames.
 */
void init_racer_for_challenge(s32 vehicleID) {
    Object_Racer *racer;

    gTajRaceInit = 3;
    racer = get_racer_object(PLAYER_ONE)->racer;
    racer->courseCheckpoint = 0;
    racer->nextCheckpoint = 0;
    racer->lap = 0;
    racer->trackOffsetX = 0;
    set_taj_challenge_type(vehicleID);
    set_pause_lockout_timer(10);
}

/**
 * Initialise the relevant variables related to races.
 * These are usually done on level load, but Taj challenges don't reset the level.
 */
void mode_init_taj_race(void) {
    CheckpointNode *checkpointNode;
    s32 j;
    s32 i;
    s32 lvlSeg;
    LevelHeader *levelHeader;
    LevelObjectEntry_Racer newRacerEntry;
    Settings *settings;
    Object *racerObj;
    Object_Racer *racer;
    f32 yOut[8];

    gTajRaceInit -= 1;
    if (gTajRaceInit == 0) {
        levelHeader = level_header();
        gChallengePrevMusic = levelHeader->music;
        gChallengePrevInstruments = levelHeader->instruments;
        levelHeader->music = SEQUENCE_TAJS_RACES;
        levelHeader->instruments = 0xFFFF;
        racerObj = get_racer_object(PLAYER_ONE);
        racer = racerObj->racer;
        gIsTajChallenge = racer->vehicleID + 1;
        checkpointNode = racer_find_start_node(racerObj, racer);
        racer->cameraYaw = 0x8000 - racer->steerVisualRotation;
        racer->wrongWayCounter = 0;
        racer->startInput = 0;
        racer->courseCheckpoint = 0;
        racer->nextCheckpoint = 0;
        racer->lap = 0;
        racer->countLap = 0;
        racer->lap_times[0] = 0;
        racer->lap_times[1] = 0;
        racer->lap_times[2] = 0;
        racer->trackOffsetX = 0;
        settings = get_settings();
        gEventCountdown = 80;
        gRaceFinishTriggered = FALSE;
        gNumFinishedRacers = 1;
        levelHeader->laps = 3;
        levelHeader->race_type = RACETYPE_DEFAULT;
        hud_init_element();
        // clang-format off
        for (i = 0; i < ARRAY_COUNT(racer->lap_times); i++) { racer->lap_times[i] = 0; } // Must be a single line.
        // clang-format on
        newRacerEntry.common.x = (checkpointNode->x + (checkpointNode->rotationZFrac * 35.0f));
        newRacerEntry.common.z = (checkpointNode->z - (checkpointNode->rotationXFrac * 35.0f));
        lvlSeg =
            get_level_segment_index_from_position(newRacerEntry.common.x, checkpointNode->y, newRacerEntry.common.z);
        newRacerEntry.common.y = collision_get_y(lvlSeg, newRacerEntry.common.x, newRacerEntry.common.z, yOut, 8)
                                     ? yOut[0]
                                     : checkpointNode->y;
        newRacerEntry.common.size = 16;
        newRacerEntry.angleY = racer->steerVisualRotation;
        newRacerEntry.angleX = 0;
        newRacerEntry.angleZ = 0;
        newRacerEntry.playerIndex = 4;
        newRacerEntry.common.objectID = ASSET_OBJECT_ID_FLYINGCARPET;
        model_anim_offset(0);
        racerObj = spawn_object(&newRacerEntry.common, OBJECT_SPAWN_UNK01);
        (*gRacers)[1] = racerObj;
        gRacersByPosition[1] = racerObj;
        gRacersByPort[1] = racerObj;
        racerObj->level_entry = NULL;
        gNumRacers = 2;
        racer = racerObj->racer;
        i = 0; // Fakematch
        racer->vehicleID = VEHICLE_CARPET;
        racer->vehicleIDPrev = racer->vehicleID;
        racer->racerIndex = 1;
        racer->characterId = settings->racers[0].character;
        racer->stretch_height_cap = 1.0f;
        racer->stretch_height = 1.0f;
        racer->transparency = 0xFF;
        racer->vehicleSound = NULL;
        racerObj->interactObj->pushForce = 2;

        for (j = gObjectListStart; j < gObjectCount; j++) {
            if (!(gObjPtrList[j]->trans.flags & OBJ_FLAGS_PARTICLE) && gObjPtrList[j]->behaviorId == BHV_PARK_WARDEN) {
                racer->aiNodeObj = gObjPtrList[j];
            }
        }
        set_pause_lockout_timer(20);
        transition_begin(&gTajChallengeTransition);
    }
}

/**
 * Finds which golden balloon should be moved, and move it onto Taj's current position.
 * Sets opacity to 0, so the balloon with smoothly fade in.
 */
void obj_taj_create_balloon(s32 blockID, f32 x, f32 y, f32 z) {
    s32 i;
    Object *obj;
    Settings *settings = get_settings();

    for (i = 0; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (obj->behaviorId == BHV_GOLDEN_BALLOON && obj->level_entry != NULL &&
            obj->level_entry->goldenBalloon.challengeID > 0) {
            if (settings->tajFlags && settings->tajFlags & (1 << (obj->level_entry->goldenBalloon.challengeID + 2))) {
                obj->trans.x_position = x;
                obj->trans.y_position = y + 10.0;
                obj->trans.z_position = z;
                obj->segmentID = blockID;
                obj->properties.goldenBalloon.action = 0;
                obj->opacity = 0;
            }
        }
    }
}

/**
 * Revert changes set by the game when starting the Taj challenge.
 * This includes the music set, as well as the extra racer spawned.
 * Has a different response depending on whether the challenge was aborted or finished.
 */
void mode_end_taj_race(s32 reason) {
    s32 flags;
    s32 i;
    Object_Racer *racer;
    Settings *settings;
    Object *obj;
    LevelHeader *levelHeader;

    levelHeader = level_header();
    levelHeader->race_type = RACETYPE_HUBWORLD;
    levelHeader->music = gChallengePrevMusic;
    levelHeader->instruments = gChallengePrevInstruments;
    minimap_opacity_set(1);

    // Only works with do {} while?
    i = 0;
    do {
        racer = (*gRacers)[i]->racer;
        if (!racer) {}
        racer->raceFinished = 0;
        racer->lap = 0;
        racer->nextCheckpoint = 0;
        racer->courseCheckpoint = 0;
    } while (++i < gNumRacers);

    free_object((*gRacers)[1]);
    gRacersByPosition[0] = (*gRacers)[0];
    gNumRacers = 1;
    for (i = gObjectListStart; i < gObjectCount; i++) {
        if (!(gObjPtrList[i]->trans.flags & OBJ_FLAGS_PARTICLE) && gObjPtrList[i]->behaviorId == BHV_PARK_WARDEN) {
            obj = gObjPtrList[i];
        }
    }
    racer = (*gRacers)[0]->racer;
    if (racer->challengeMarker != NULL) {
        free_object(racer->challengeMarker);
        racer->challengeMarker = NULL;
    }
    if (reason == CHALLENGE_END_FINISH) {
        if (racer->finishPosition == 1) {
            settings = get_settings();
            flags = racer->vehicleID;
            flags = 1 << (flags + 3);
            if (settings->tajFlags & flags) {
                set_next_taj_challenge_menu(5);
            } else {
                set_next_taj_challenge_menu(racer->vehicleID + 6);
                settings->tajFlags |= flags;
                safe_mark_write_save_file(get_save_file_index());
            }
        } else {
            set_next_taj_challenge_menu(4);
        }
        obj->properties.taj.action = TAJ_MODE_UNK1F;
        set_taj_status(TAJ_TELEPORT);
    } else {
        music_change_on();
        music_stop();
        set_next_taj_challenge_menu(0);
        audspat_jingle_on();
        if (reason == CHALLENGE_END_OOB) {
            set_current_text(ASSET_GAME_TEXT_0);
        }
        gEventCountdown = 0;
        gEventStartTimer = 0;
        obj->properties.taj.action = TAJ_MODE_TELEPORT_AWAY_BEGIN;
    }
    music_change_on();
    hud_audio_init();
    level_music_start(1.0f);
    gIsTajChallenge = FALSE;
}

/** The checkpoint or setup point a racer starts from. */
CheckpointNode *racer_find_start_node(Object *obj, Object_Racer *racer) {
    CheckpointNode *lastCheckpointNode;
    Camera *activeCameraSegment;
    s32 yOutCount;
    f32 yOut[9];
    Object *ptrList;
    s32 i;

    if (gNumberOfMainCheckpoints == 0) {
        lastCheckpointNode = NULL;
        for (i = 0; i < gObjectCount; i++) {
            ptrList = gObjPtrList[i];
            if (!(ptrList->trans.flags & OBJ_FLAGS_PARTICLE) && (ptrList->behaviorId == BHV_SETUP_POINT)) {
                if (ptrList->properties.setupPoint.racerIndex == 0) {
                    obj->trans.x_position = ptrList->trans.x_position;
                    obj->trans.y_position = ptrList->trans.y_position;
                    obj->trans.z_position = ptrList->trans.z_position;
                    obj->segmentID = ptrList->segmentID;
                    i = gObjectCount;
                }
            }
        }
    } else {
        lastCheckpointNode = &gTrackCheckpoints[gNumberOfMainCheckpoints - 1];
        obj->trans.x_position = lastCheckpointNode->x - (lastCheckpointNode->rotationZFrac * 35.0f);
        obj->trans.y_position = lastCheckpointNode->y;
        obj->trans.z_position = lastCheckpointNode->z + (lastCheckpointNode->rotationXFrac * 35.0f);
        obj->segmentID =
            get_level_segment_index_from_position(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
    }
    yOutCount = collision_get_y(obj->segmentID, obj->trans.x_position, obj->trans.z_position, yOut, 9);
    if (yOutCount != 0) {
        obj->trans.y_position = yOut[yOutCount - 1];
    }
    racer->prev_x_position = obj->trans.x_position;
    racer->prev_y_position = obj->trans.y_position;
    racer->prev_z_position = obj->trans.z_position;
    if (lastCheckpointNode != NULL) {
        racer->steerVisualRotation = arctan2_f(lastCheckpointNode->rotationXFrac, lastCheckpointNode->rotationZFrac);
    } else {
        racer->steerVisualRotation = ptrList->trans.rotation.y_rotation;
    }
    racer->nextCheckpoint = 0;
    racer->courseCheckpoint = racer->lap * gNumberOfMainCheckpoints;
    obj->trans.rotation.y_rotation = racer->steerVisualRotation;
    racer->wheelCollisionPos[0] = obj->trans.x_position;
    racer->wheelCollisionPos[1] = obj->trans.y_position + 15.0f;
    racer->wheelCollisionPos[2] = obj->trans.z_position;
    racer->wheelCollisionPos[3] = obj->trans.x_position;
    racer->wheelCollisionPos[4] = obj->trans.y_position + 15.0f;
    racer->wheelCollisionPos[5] = obj->trans.z_position;
    racer->wheelCollisionPos[6] = obj->trans.x_position;
    racer->wheelCollisionPos[7] = obj->trans.y_position + 15.0f;
    racer->wheelCollisionPos[8] = obj->trans.z_position;
    racer->wheelCollisionPos[9] = obj->trans.x_position;
    racer->wheelCollisionPos[10] = obj->trans.y_position + 15.0f;
    racer->wheelCollisionPos[11] = obj->trans.z_position;
    obj->interactObj->x_position = obj->trans.x_position;
    obj->interactObj->y_position = obj->trans.y_position;
    obj->interactObj->z_position = obj->trans.z_position;
    // fake
    if (1) {}
    if (1) {}
    racer->velocity = 0.0f;
    racer->lateral_velocity = 0.0f;
    obj->x_velocity = 0.0f;
    obj->z_velocity = 0.0f;
    racer->vehicleID = racer->vehicleIDPrev;
    if (racer->playerIndex != -1) {
        set_active_camera(racer->playerIndex);
        activeCameraSegment = cam_get_active_camera_no_cutscenes();
        activeCameraSegment->trans.x_position = obj->trans.x_position;
        activeCameraSegment->trans.y_position = obj->trans.y_position;
        activeCameraSegment->trans.z_position = obj->trans.z_position;
    }
    return lastCheckpointNode;
}

/**
 * Returns true if a taj challenge is currently active.
 */
s8 is_taj_challenge(void) {
    return gIsTajChallenge;
}

/**
 * Searches for the furthest teleport anchor and returns it.
 */
Object *find_furthest_telepoint(f32 x, f32 z) {
    Object *tempObj;
    Object *bestObj;
    f32 diffX;
    f32 diffZ;
    f32 distance;
    f32 bestDist;
    s32 i;

    bestDist = 0.0f;
    i = 0;
    bestObj = NULL;
    if (gObjectCount > 0) {
        do {
            tempObj = gObjPtrList[i];
            if (!(tempObj->trans.flags & OBJ_FLAGS_PARTICLE) && tempObj->behaviorId == BHV_TAJ_TELEPOINT) {
                diffX = tempObj->trans.x_position - x;
                diffZ = tempObj->trans.z_position - z;
                tempObj = gObjPtrList[i]; // fakematch
                distance = sqrtf((diffX * diffX) + (diffZ * diffZ));
                if (bestDist < distance) {
                    bestDist = distance;
                    bestObj = tempObj;
                }
            }
            i += 1;
        } while (i < gObjectCount);
    }
    return bestObj;
}

/** The boss race in progress, 1-based, or 0 outside boss races. */
s32 boss_race_number(void) {
    if (gBossRaceMode != 0) {
        return gBossRaceIds[1] + 1;
    } else if (level_type() == RACETYPE_BOSS) {
        return gBossRaceIds[1] + 1;
    }
    return 0;
}
