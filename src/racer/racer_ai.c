#include "racer_internal.h"

// Used to know how the AI should use a balloon when they have one.
static s8 gRacerAIBalloonActionTable[NUM_WEAPON_TYPES] = { 1, 1, 2, 2, 4, 3, 0, 6, 4, 3, 2, 2, 5, 5, 5, 0 };

//!@bug: indexed by racer->racePosition (racer.c racer_ai_tactics), which is
// 1-based — racerPos starts at 1 in objects.c and counts up — so an 8th-place
// racer reads index 8 of an 8-entry table. Index 0 is never used and index 8 is
// off the end. On console the byte past the table is gAiLaneByAiCount[0] (they are
// adjacent: 0x800DCDA0 + 8 == 0x800DCDA8), so the read yields 1; the ninth entry
// below reproduces that exactly, keeping the stray load in bounds.
static s8 gAiLaneByPosition[9] = {
    0, 0, 0, 1, 1, 2, 2, 2, 1,
};

static s8 gAiLaneByAiCount[8] = {
    1, 1, 1, 2, 3, 2, 3, 2,
};

/**
 * Counts the computer racers in the field and finds this racer's place in it: its index,
 * how many computers are ahead of it, and the place of the leading human.
 */
static void racer_ai_tactics_count_field(Object_Racer *racer, Object **racerGroup, s32 numRacers, s16 *racerID,
                                         s16 *var_t5, s16 *var_t0, s16 *var_t4) {
    s16 index;
    s8 someFlag;
    Object_Racer *sp5C;

    index = 0;
    *var_t5 = 0;
    *var_t0 = 0;
    *var_t4 = PLAYER_COMPUTER;
    for (someFlag = TRUE; index < numRacers; index++) {
        sp5C = racerGroup[index]->racer;
        if (sp5C == racer) {
            someFlag = FALSE;
            *racerID = index;
        }

        if (sp5C->playerIndex == PLAYER_COMPUTER) {
            (*var_t0)++;
            if (someFlag) {
                (*var_t5)++;
            }
        } else if (*var_t4 == PLAYER_COMPUTER) {
            *var_t4 = index;
        }
    }

    if (*var_t4 == PLAYER_COMPUTER) {
        *var_t4 = PLAYER_ONE;
    }
}

/**
 * Plays the taunt and complaint voices when this racer has just passed, or been passed by,
 * a human racer.
 */
static void racer_ai_tactics_overtake_voices(Object *obj, Object_Racer *racer, Object **racerGroup, s32 numRacers,
                                             s16 racerID, s16 temp_v0) {
    s16 index;
    s16 tempRacerIndex;
    Object_Racer *sp5C;

    if (gRaceStartTimer == 0 && racer->vehicleID != VEHICLE_LOOPDELOOP) {
        index = racerID - 1;
        if (racer->prevRaceOrder < racerID && index >= 0 && index < numRacers) {
            sp5C = racerGroup[index]->racer;
            if (sp5C->playerIndex != PLAYER_COMPUTER) {
                if (temp_v0 == 0) {
                    play_random_character_voice(obj, SOUND_VOICE_CHARACTER_NEGATIVE, 8, 3);
                } else {
                    racer_boss_sound_spatial(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position, 5);
                }
                play_random_character_voice(racerGroup[index], SOUND_VOICE_CHARACTER_POSITIVE, 8, 2);
            }
        }
        tempRacerIndex = racerID + 1;
        if (racerID < racer->prevRaceOrder && tempRacerIndex >= 0 && tempRacerIndex < numRacers) {
            sp5C = racerGroup[tempRacerIndex]->racer;
            if (sp5C->playerIndex != PLAYER_COMPUTER) {
                play_random_character_voice(racerGroup[(racerID + 1)], SOUND_VOICE_KRUNCH_NEGATIVE1, 8, 2);
                if (temp_v0 == 0) {
                    play_random_character_voice(obj, SOUND_VOICE_CHARACTER_POSITIVE, 8, 3);
                } else {
                    racer_boss_sound_spatial(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position, 3);
                }
            }
        }
    }
}

/**
 * Finds the nearest opponents just ahead of and just behind this racer, with their
 * distances.
 */
static void racer_ai_tactics_find_neighbours(Object_Racer *racer, f32 *sp94, f32 *sp90, Object_Racer **sp5C,
                                             Object_Racer **sp58) {
    Object *obj;

    *sp94 = 0.0f;
    *sp90 = 0.0f;
    *sp5C = NULL;
    obj = racer_find_nearest_opponent_relative(racer, 1, sp94);
    if (obj != NULL) {
        *sp5C = obj->racer;
    }
    *sp58 = NULL;
    obj = racer_find_nearest_opponent_relative(racer, -1, sp90);
    if (obj != NULL) {
        *sp58 = obj->racer;
    }
}

/**
 * Sets the racer's skill: random at the start of an ordinary race with a human in it,
 * otherwise from the level header's per-character table.
 */
static void racer_ai_tactics_choose_skill(Object_Racer *racer, LevelHeader *header, s16 var_t0, s8 racerCharacterId) {
    if (var_t0 < 7 && get_trophy_race_world_id() == 0 && boss_race_number() == 0 && !is_taj_challenge()) {
        if (gRaceStartTimer == 100) {
            racer->aiSkill = rand_range(AI_MASTER, AI_HARD);
        }
    } else if (get_trophy_race_world_id() != 0) {
        racer->aiSkill = header->trophyAiSkillByCharacter[racerCharacterId];
    } else {
        racer->aiSkill = header->aiSkillByCharacter[racerCharacterId];
    }
}

/**
 * During the start countdown, picks the lane from the race position and holds the
 * throttle once the skill-dependent launch moment arrives.
 */
static void racer_ai_tactics_start_launch(Object_Racer *racer) {
    s16 index;

    if (gAiStartDelay != 0.0f) {
        racer->aiLane = gAiLaneByPosition[racer->racePosition];
    }
    index = racer->aiSkill - 2;
    index <<= 2;
    if (index <= 300.0f - gAiStartDelay) {
        gCurrentRacerInput |= A_BUTTON;
    }
}

/**
 * The weapon the racer's balloon currently gives, as an index into
 * gRacerAIBalloonActionTable.
 */
static s8 racer_ai_tactics_balloon_weapon(Object_Racer *racer) {
    s8 *balloonData;
    s8 balloonType;

    balloonData = (s8 *) get_misc_asset(ASSET_MISC_BALLOON_DATA);
    if (racer->balloon_level < 3) {
        balloonType = balloonData[racer->balloon_type * 3 + racer->balloon_level];
    } else {
        balloonType = racer->balloon_type;
    }
    return balloonType;
}

/**
 * When a new item decision is due, rolls to upgrade or discard the balloon, fire a boost,
 * or set off chasing the racer ahead or waiting for the one behind to use the weapon.
 */
static void racer_ai_tactics_use_balloon(Object_Racer *racer, s8 balloonType, Object_Racer *sp5C, Object_Racer *sp58,
                                         s16 var_t4, s16 var_t5, s16 sp38, s16 sp36, s16 sp3C, s8 *miscAsset2,
                                         s8 racerCharacterId) {
    if (racer->aiFlags & 1) {
        if (racer->aiPhysicsTimer == 0) {
            if (racer->balloon_level == 0) {
                if (roll_percent_chance(sp38)) {
                    racer->balloon_level++;
                }
            }
            if (var_t4 == 0 && var_t5 < 3) {
                if (roll_percent_chance(sp3C)) {
                    racer->balloon_type = 0;
                }
            }
        }
        if (racer->boostTimer == 0 && gRacerAIBalloonActionTable[balloonType] == 4) {
            if (roll_percent_chance(sp3C)) {
                gCurrentButtonsReleased |= Z_TRIG;
            }
        }
        if (racer->balloon_quantity != 0) {
            if (gRacerAIBalloonActionTable[balloonType] == 1) {
                if (sp5C != NULL && sp5C->playerIndex == PLAYER_COMPUTER) {
                    if (var_t4 < 4) {
                        racer->aiBehaviourTimer = miscAsset2[racerCharacterId] * 60;
                        racer->aiBehaviour = 4;
                    }
                } else if (roll_percent_chance(sp36) && var_t4 < 2) {
                    racer->aiBehaviourTimer = miscAsset2[racerCharacterId] * 60;
                    racer->aiBehaviour = 4;
                }
            }
            if (gRacerAIBalloonActionTable[balloonType] == 2) {
                if (sp58 != NULL && sp58->playerIndex == PLAYER_COMPUTER) {
                    if (var_t4 < 4) {
                        racer->aiBehaviourTimer = miscAsset2[racerCharacterId] * 60;
                        racer->aiBehaviour = 5;
                    }
                } else if (roll_percent_chance(sp36) && var_t4 < 2) {
                    racer->aiBehaviourTimer = miscAsset2[racerCharacterId] * 60;
                    racer->aiBehaviour = 5;
                }
            }
        }
        racer->aiFlags &= ~1;
    }
}

/**
 * While boosting, rolls once per boost whether to let off the throttle, and keeps it off
 * until the racer has slowed down.
 */
static void racer_ai_tactics_boost_throttle(Object_Racer *racer, s16 sp3A) {
    if (racer->boostTimer != 0) {
        if (!(racer->aiFlags & 2)) {
            if (roll_percent_chance(sp3A)) {
                racer->aiFlags |= 4;
            }
            racer->aiFlags |= 2;
        }
        if (racer->aiFlags & 4) {
            gCurrentRacerInput &= ~A_BUTTON;
        }
    } else {
        racer->aiFlags &= ~2;
        if (racer->velocity > -12.0) {
            racer->aiFlags &= ~4;
        }
    }
    if (racer->aiFlags & 4) {
        gCurrentRacerInput &= ~A_BUTTON;
    }
}

/**
 * When the behaviour timer runs out: fires any held weapon, picks the next behaviour and
 * lane, and restarts the timer.
 */
static void racer_ai_tactics_renew_behaviour(Object_Racer *racer, Object_Racer *sp58, f32 sp90, s16 var_t5, s16 var_t4,
                                             s8 *miscAsset1, s8 racerCharacterId, s8 sp3F, s16 sp3C) {
    if (racer->aiBehaviourTimer == 0) {
        if (racer->aiBehaviour == 4 || racer->aiBehaviour == 5) {
            gCurrentButtonsReleased |= Z_TRIG;
        }
        racer->aiBehaviour = 0;
        if (sp58 != NULL && sp58->playerIndex != PLAYER_COMPUTER && sp90 < 200.0f && var_t5 != 0 && var_t4 < 3 &&
            miscAsset1[(racerCharacterId * 10) + sp3F] < 5) {
            racer->aiBehaviour = 5;
        }
        if (gAiStartDelay == 0.0f) {
            racer->aiLane = gAiLaneByAiCount[var_t5];
            if (roll_percent_chance(sp3C)) {
                racer->aiLane -= 1;
            }
        }
        racer->aiBehaviourTimer = 300;
    }
}

/**
 * Once the race is under way, sets the target speed between the AI table's minimum and
 * maximum by how many computers are ahead, faster in the outer lanes.
 */
static void racer_ai_tactics_target_speed(Object_Racer *racer, AIBehaviourTable *sp54, s16 var_t5) {
    f32 var_f12;
    f32 var_f0;
    s8 temp7;

    if (gRaceStartTimer == 0) {
        var_f0 = sp54->maxTargetSpeed;
        var_f0 = sqrtf(((var_f0 * 0.025) + 0.561) / 0.004);
        var_f12 = sp54->minTargetSpeed;
        var_f12 = sqrtf(((var_f12 * 0.025) + 0.561) / 0.004);
        // explicit load of 7 required
        temp7 = 7;
        var_f12 += ((var_f0 - var_f12) / temp7) * (7 - var_t5);
        if (racer->aiLane > 1) {
            // casts below required
            var_f12 += (f32) (((s32) racer->aiLane) - 1) * 0.2;
        }

        var_f12 = ((var_f12 * var_f12 * 0.004) - 0.595) / 0.025;
        racer->aiTargetSpeed = var_f12;
        // @fake
        if (!racer) {}
    }
}

/**
 * Acts on the current behaviour: changes lane to get past a computer racer just ahead,
 * or follows the line of the racer being chased or waited for.
 */
static void racer_ai_tactics_follow_behaviour(Object_Racer *racer, Object_Racer *sp5C, Object_Racer *sp58, f32 sp94) {
    if (sp5C != NULL && racer->aiLane == 3 && 500.0 < sp94) {
        racer->aiLane = 2;
    }
    switch (racer->aiBehaviour) {
        case 0:
            if (sp5C != NULL && sp5C->playerIndex == PLAYER_COMPUTER && sp5C->aiBehaviour == 0) {
                if (sp5C->aiLane == racer->aiLane && 100.0 > sp94) {
                    racer->aiLane++;
                    if (racer->aiLane > 3) {
                        racer->aiLane = 3;
                    }
                }
            }
            break;
        case 4:
            if (sp5C != NULL) {
                racer->trackOffsetX = sp5C->trackOffsetX;
            }
            break;
        case 5:
            if (sp58 != NULL) {
                racer->trackOffsetX = sp58->trackOffsetX;
            }
            break;
    }
}

/**
 * In Taj's challenges and boss races, drops the tactics and takes the target speed from
 * that race's fixed table.
 */
static void racer_ai_tactics_scripted_speed(Object_Racer *racer, s16 temp_v0) {
    s16 index;
    f32 *miscAsset3;

    // clang-format off
    index = (u16) is_taj_challenge();\
    if (index != 0 || temp_v0 != 0) {
        // clang-format on
        racer->aiLane = 0;
        racer->aiBehaviour = 0;
        if (temp_v0 != 0) {
            miscAsset3 = (f32 *) get_misc_asset(ASSET_MISC_18);
            index = temp_v0;
        } else {
            miscAsset3 = (f32 *) get_misc_asset(ASSET_MISC_17);
        }
        racer->aiTargetSpeed = miscAsset3[index - 1];
    }
}

/**
 * Boss races: speeds the boss up early on and when it is close to losing, and brings it
 * to a stop once it has finished.
 */
static void racer_ai_tactics_boss_pace(Object_Racer *racer, s16 temp_v0, s16 racerID, f32 sp94, f32 sp90,
                                       s32 updateRate) {
    f32 var_f0;

    if (temp_v0 != 0) {
        if (gRaceStartTimer != 0) {
            gAiCatchupTimer = 900;
        } else {
            gAiCatchupTimer -= updateRate;
            if (gAiCatchupTimer < 0) {
                gAiCatchupTimer = 0;
                if (racerID == 1 && sp94 > 650.0) {
                    racer->aiTargetSpeed += 2.0;
                }
            } else {
                var_f0 = racer->aiTargetSpeed;
                if (gAiCatchupTimer > 720) {
                    var_f0 += 5.0;
                } else if (racerID == 1 || sp90 < 200.0) {
                    var_f0 += 10.0;
                }
                if (racer->aiTargetSpeed < var_f0) {
                    racer->aiTargetSpeed = var_f0;
                }
            }
        }
        if (racer->raceFinished != FALSE) {
            racer->boostTimer = 0;
            racer->aiStuckTimer = 0;
            gCurrentRacerInput &= ~A_BUTTON;
            gCurrentRacerInput |= B_BUTTON;
            if (racer->velocity > -0.3) {
                gCurrentRacerInput |= A_BUTTON;
                gCurrentStickX = 0;
            }
        }
    }
}

/**
 * Keeps the racer's offset from the track centre line within bounds.
 */
static void racer_ai_tactics_clamp_track_offset(Object_Racer *racer) {
    if (racer->trackOffsetX > 64) {
        racer->trackOffsetX = 64;
    }
    if (racer->trackOffsetY > 40) {
        racer->trackOffsetY = 40;
    }
    if (racer->trackOffsetX < -64) {
        racer->trackOffsetX = -64;
    }
    if (racer->trackOffsetY < -40) {
        racer->trackOffsetY = -40;
    }
}

/**
 * Computer racer decisions beyond steering: taunts and voices when positions
 * change, skill level, lane choice, and item and boost use against the
 * opponents just ahead and behind. Called from racer_ai_drive.
 */
void racer_ai_tactics(Object *obj, Object_Racer *racer, s32 updateRate) {
    f32 sp94;
    f32 sp90;
    s16 var_t5;
    s16 var_t0;
    s16 racerID = 0; // read before it is set on some paths in the original
    s16 temp_v0;
    s16 sp6E;
    s32 numRacers;
    Object **racerGroup;
    Object_Racer *sp5C;
    Object_Racer *sp58;
    AIBehaviourTable *sp54;
    LevelHeader *header;
    s8 balloonType;
    s8 *miscAsset2;
    s8 *miscAsset1;
    s8 sp3F = 0; // read before it is set on some paths in the original
    s8 racerCharacterId;
    s16 sp3C;
    s16 sp3A;
    s16 sp38;
    s16 sp36;
    s16 var_t4;

    sp6E = racer->aiLane;
    miscAsset1 = (s8 *) get_misc_asset(ASSET_MISC_1);
    miscAsset2 = (s8 *) get_misc_asset(ASSET_MISC_2);
    header = level_header();
    racerGroup = get_racer_objects_by_position(&numRacers);
    sp54 = aitable_get();
    if (racer->aiBehaviourTimer > 0) {
        racer->aiBehaviourTimer -= updateRate;
    } else {
        racer->aiBehaviourTimer = 0;
    }
    if (race_finish_timer() != 0) {
        gCurrentRacerInput |= A_BUTTON;
        return;
    }

    racer_ai_tactics_count_field(racer, racerGroup, numRacers, &racerID, &var_t5, &var_t0, &var_t4);
    if (var_t0 == 0) {
        return;
    }

    temp_v0 = boss_race_number();
    racer_ai_tactics_overtake_voices(obj, racer, racerGroup, numRacers, racerID, temp_v0);
    racer->prevRaceOrder = racerID;
    racer_ai_tactics_find_neighbours(racer, &sp94, &sp90, &sp5C, &sp58);
    racerCharacterId = racer->characterId;
    if (sp5C != NULL) {
        sp3F = sp5C->characterId;
    }

    racer_ai_tactics_choose_skill(racer, header, var_t0, racerCharacterId);
    racer_ai_tactics_start_launch(racer);
    balloonType = racer_ai_tactics_balloon_weapon(racer);

    sp38 = sp54->percentages[1][0] + (((sp54->percentages[1][1] - sp54->percentages[1][0]) * (7 - var_t5)) / 7);
    sp36 = sp54->percentages[2][0] + (((sp54->percentages[2][1] - sp54->percentages[2][0]) * (7 - var_t5)) / 7);
    sp3A = sp54->percentages[0][0] + (((sp54->percentages[0][1] - sp54->percentages[0][0]) * (7 - var_t5)) / 7);
    sp3C = sp54->percentages[3][0] + (((sp54->percentages[3][1] - sp54->percentages[3][0]) * (7 - var_t5)) / 7);

    racer_ai_tactics_use_balloon(racer, balloonType, sp5C, sp58, var_t4, var_t5, sp38, sp36, sp3C, miscAsset2,
                                 racerCharacterId);
    racer_ai_tactics_boost_throttle(racer, sp3A);
    racer_ai_tactics_renew_behaviour(racer, sp58, sp90, var_t5, var_t4, miscAsset1, racerCharacterId, sp3F, sp3C);
    // @fake
    if ((7 - var_t5) == (7 & 0xFFFFFFFFu) && sp58 != NULL && sp90 < -2500.0) {
        racer->aiFlags &= ~4;
        racer->aiLane = 1;
    }
    racer_ai_tactics_target_speed(racer, sp54, var_t5);
    racer_ai_tactics_follow_behaviour(racer, sp5C, sp58, sp94);
    racer_ai_tactics_scripted_speed(racer, temp_v0);
    racer_ai_tactics_boss_pace(racer, temp_v0, racerID, sp94, sp90, updateRate);
    if (racer->aiReverseTimer != 0) {
        racer->aiLane = sp6E;
    }
    racer_ai_tactics_clamp_track_offset(racer);
}

/**
 * During specific or nonspecific actions, increase the steps in the AI behaviour table.
 * Increment percent chances of them performing specific actions, with a maximum of 100%.
 */
void increment_ai_behaviour_chances(Object *obj, Object_Racer *racer, s32 updateRate) {
    AIBehaviourTable *aiTable;
    s8 *test;
    s8 balloonType;
    s32 i;
    static s8 sAIFramesSinceA;
    static s8 sAIStartedBoosting;
    static s8 sBalloonLevelAI;

    if (!obj) {
        sAIFramesSinceA = 0;
        sAIStartedBoosting = 0;
        sBalloonLevelAI = 0;
        return;
    }
    aiTable = aitable_get();
    if (racer->boostTimer) {
        if (sAIStartedBoosting == FALSE) {
            aiTable->percentages[AI_BLUE_BALLOON][AI_MIN] += aiTable->percentages[AI_BLUE_BALLOON][AI_MIN_STEP];
            aiTable->percentages[AI_BLUE_BALLOON][AI_MAX] += aiTable->percentages[AI_BLUE_BALLOON][AI_MAX_STEP];
            sAIStartedBoosting = TRUE;
        }
        if (!(gCurrentRacerInput & A_BUTTON)) {
            sAIFramesSinceA += updateRate;
        }
    } else {
        sAIStartedBoosting = FALSE;
        if (sAIFramesSinceA > 20) {
            aiTable->percentages[AI_EMPOWERED_BOOST][AI_MIN] += aiTable->percentages[AI_EMPOWERED_BOOST][AI_MIN_STEP];
            aiTable->percentages[AI_EMPOWERED_BOOST][AI_MAX] += aiTable->percentages[AI_EMPOWERED_BOOST][AI_MAX_STEP];
        }
        sAIFramesSinceA = 0;
    }
    if (racer->balloon_quantity) {
        if (sBalloonLevelAI < racer->balloon_level) {
            aiTable->percentages[AI_GETS_BALLOON][AI_MIN] += aiTable->percentages[1][AI_MIN_STEP];
            aiTable->percentages[AI_GETS_BALLOON][AI_MAX] += aiTable->percentages[1][AI_MAX_STEP];
        }
        sBalloonLevelAI = racer->balloon_level;
    } else {
        sBalloonLevelAI = 0;
    }
    test = (s8 *) get_misc_asset(ASSET_MISC_BALLOON_DATA);
    if ((gCurrentButtonsReleased & Z_TRIG) && racer->balloon_quantity) {
        if (racer->balloon_level < 3) {
            balloonType = test[racer->balloon_type * 3 + racer->balloon_level];
        } else {
            balloonType = racer->balloon_type;
        }
        if (gRacerAIBalloonActionTable[balloonType] == 1) {
            aiTable->percentages[AI_UNK_2][AI_MIN] += aiTable->percentages[AI_UNK_2][AI_MIN_STEP];
            aiTable->percentages[AI_UNK_2][AI_MAX] += aiTable->percentages[AI_UNK_2][AI_MAX_STEP];
        }
    }
    for (i = 0; i < 2; i++) {
        if (aiTable->percentages[AI_GETS_BALLOON][i] > 100 || aiTable->percentages[AI_GETS_BALLOON][i] < 0) {
            aiTable->percentages[AI_GETS_BALLOON][i] = 100;
        }
        if (aiTable->percentages[AI_EMPOWERED_BOOST][i] > 100 || aiTable->percentages[AI_EMPOWERED_BOOST][i] < 0) {
            aiTable->percentages[AI_EMPOWERED_BOOST][i] = 100;
        }
        if (aiTable->percentages[AI_BLUE_BALLOON][i] > 100 || aiTable->percentages[AI_BLUE_BALLOON][i] < 0) {
            aiTable->percentages[AI_BLUE_BALLOON][i] = 100;
        }
        if (aiTable->percentages[AI_UNK_2][i] > 100 || aiTable->percentages[AI_UNK_2][i] < 0) {
            aiTable->percentages[AI_UNK_2][i] = 100;
        }
    }
}

/**
 * Depending on the race type, call a function that handles pathing.
 * During those functions, the racer inputs will be set to simulate control.
 */
void racer_AI_pathing_inputs(Object *obj, Object_Racer *racer, s32 updateRate) {
    s32 raceType;

    raceType = level_type();

    switch (raceType) {
        case RACETYPE_CHALLENGE_BATTLE:
        case RACETYPE_CHALLENGE_BANANAS:
            racer_ai_challenge(obj, racer, updateRate);
            break;
        case RACETYPE_CHALLENGE_EGGS:
            racer_ai_eggs(obj, racer, updateRate);
            break;
        default:
            racer_ai_drive(obj, racer, updateRate);
            break;
    }

    if (racer->aiReverseTimer == 0 && racer->velocity < -0.5) {
        racer->aiStuckCooldown -= updateRate;
        if (racer->aiStuckCooldown < 0) {
            racer->aiStuckCooldown = 0;
        }
    }

    if (racer->velocity > -1.0 && racer->aiReverseTimer == 0 && !gRaceStartTimer && gAiStartDelay == 0.0f &&
        racer->groundedWheels && racer->aiStuckCooldown == 0) {
        racer->aiStuckTimer += updateRate;

        if (racer->aiStuckTimer > 60) {
            racer->aiStuckTimer = 0;
            racer->aiReverseTimer = 60;
            racer->aiStuckCooldown = 120;

            if (!(raceType & RACETYPE_CHALLENGE)) {
                racer->aiLane = (racer->aiLane + 1) & 3;
            } else if (raceType == RACETYPE_CHALLENGE_BATTLE || raceType == RACETYPE_CHALLENGE_BANANAS) {
                if (racer->aiNodeId != 0xFF) {
                    racer->aiNodeObj = ainode_get(racer->aiNodeId);
                }
            }
        }
    } else {
        racer->aiReverseTimer -= updateRate;
        racer->aiStuckTimer = 0;

        if (racer->aiReverseTimer < 0) {
            racer->aiReverseTimer = 0;
        }
    }

    // Kick it into reverse.
    if (racer->aiReverseTimer != 0) {
        gCurrentStickX *= -1;
        gCurrentRacerInput &= ~A_BUTTON;
        gCurrentStickY = -50;
        gCurrentRacerInput |= B_BUTTON;
    }

    // Cap stick inputs.
    CLAMP(gCurrentStickX, -75, 75);
    CLAMP(gCurrentStickY, -75, 75);
}

/**
 * Rolls a number between 0 and 99.
 * Effectively, this is to make a 1-100% chance of it returning true, based on the number passed.
 */
s32 roll_percent_chance(s32 chance) {
    return rand_range(0, 99) < chance;
}

/**
 * Challenge AI with no node yet: starts it at the AI node nearest to the racer.
 */
static void racer_ai_challenge_find_first_node(Object *aiRacerObj, Object_Racer *aiRacer) {
    s32 temp;

    if (aiRacer->currentAnimId == 0) {
        temp = ainode_find_nearest(aiRacerObj->trans.x_position, aiRacerObj->trans.y_position,
                                   aiRacerObj->trans.z_position, 0);
        if (temp != 0xFF) {
            aiRacer->aiNodeObj = ainode_get(temp);
            aiRacer->currentAnimId = 1;
            aiRacer->aiNodeId = 0xFF;
        }
    }
}

/**
 * Steers toward the racer's current AI node, giving back the node's level entry and its
 * distance.
 */
static void racer_ai_challenge_steer_to_node(Object *aiRacerObj, Object_Racer *aiRacer, Object *sp74,
                                             LevelObjectEntry **sp6C, f32 *dist) {
    f32 xDiff;
    f32 zDiff;
    s32 temp;
    s32 var_v1;

    if (sp74 != NULL) {
        *sp6C = sp74->level_entry;
        xDiff = sp74->trans.x_position - aiRacerObj->trans.x_position;
        zDiff = sp74->trans.z_position - aiRacerObj->trans.z_position;
        *dist = sqrtf((xDiff * xDiff) + (zDiff * zDiff));
        if (*dist > 0.0) {
            temp = ((arctan2_f(xDiff, zDiff)) - 0x8000) & 0xFFFF;
            var_v1 = temp - (aiRacer->steerVisualRotation & 0xFFFF);
            if (var_v1 > 0x8000) {
                var_v1 -= 0xFFFF;
            }
            if (var_v1 < -0x8000) {
                var_v1 += 0xFFFF;
            }
            gCurrentStickX = -var_v1 >> 4;
        }
    }
}

/**
 * Holds the behaviour timer at zero before the start, and counts it down while a timed
 * goal is being pursued.
 */
static void racer_ai_challenge_tick_behaviour_timer(Object_Racer *aiRacer, s32 updateRate) {
    if (gRaceStartTimer != 0) {
        aiRacer->aiBehaviourTimer = 0;
    }
    if ((aiRacer->currentAnimId == 2) || (aiRacer->currentAnimId == 4) || (aiRacer->currentAnimId == 5)) {
        if (aiRacer->aiBehaviourTimer > 0) {
            aiRacer->aiBehaviourTimer -= updateRate;
        } else {
            aiRacer->aiBehaviourTimer = 0;
        }
    }
}

/**
 * Picks a rival to hunt (most or fewest bananas, or player one) and, if there is one,
 * claims it in gAiEggCounts; otherwise wanders for a while.
 */
static void racer_ai_challenge_pick_rival(Object_Racer *aiRacer, s8 *sp38) {
    Object_Racer *tempRacer;
    s16 i;
    s16 index = 0; // read before it is set on some paths in the original
    s16 sp4C;
    s16 sp4A;
    s16 sp48;
    s16 sp46;
    Object *tempRacerObj;

    if (roll_percent_chance(50) != 0) {
        sp4C = 1;
    } else {
        sp4C = 0;
    }
    if (roll_percent_chance(sp38[1]) != 0) {
        sp46 = 0;
    } else {
        sp46 = 1;
    }
    sp4A = -1;
    sp48 = sp46 << 4;
    for (i = 0; i < 4; i++) {
        if (sp4C != 0) {
            index = 3 - i;
        } else {
            index = i;
        }
        if ((gAiEggCounts[index] == 0) && (index != aiRacer->racerIndex)) {
            tempRacerObj = get_racer_object(index);
            tempRacer = tempRacerObj->racer;
            if (sp46 == 0) {
                if (sp48 < tempRacer->bananas) {
                    sp48 = tempRacer->bananas;
                    sp4A = index;
                }
            } else {
                if ((tempRacer->bananas > 0) && (tempRacer->bananas < sp48)) {
                    sp48 = tempRacer->bananas;
                    sp4A = index;
                }
            }
        }
    }
    if (roll_percent_chance(sp38[2]) != 0) {
        if (cam_get_viewport_layout() == 0) {
            tempRacerObj = get_racer_object(index);
            tempRacer = tempRacerObj->racer;
            if (tempRacer->bananas > 0) {
                sp4A = 0;
            }
        }
    }
    if (sp4A >= 0) {
        aiRacer->eggHudCounter = (s8) sp4A;
        gAiEggCounts[sp4A] = aiRacer->racerIndex + 1;
        aiRacer->currentAnimId = 4;
        aiRacer->aiBehaviourTimer = 0x4B0;
    } else {
        aiRacer->currentAnimId = 2;
        aiRacer->aiBehaviourTimer = 0x4B0;
    }
}

/**
 * Goal 1, choosing: rolls the level's challenge chances to fetch a balloon, drop off
 * bananas, defend its egg, hunt a rival, or wander.
 */
static void racer_ai_challenge_choose_goal(Object_Racer *aiRacer, s8 *sp38, s8 raceType) {
    rand_range(0, 9);
    if (aiRacer->balloon_quantity == 0) {
        if (roll_percent_chance(sp38[3]) != 0) {
            aiRacer->currentAnimId = 3;
        } else if ((roll_percent_chance(sp38[6]) != 0) && (raceType == RACETYPE_CHALLENGE_BANANAS) &&
                   (aiRacer->bananas >= 2)) {
            aiRacer->currentAnimId = 7;
        } else if ((roll_percent_chance(sp38[5]) != 0) && (gAiEggCounts[aiRacer->racerIndex] != 0)) {
            aiRacer->aiBehaviourTimer = 1200;
            aiRacer->eggHudCounter = gAiEggCounts[aiRacer->racerIndex] - 1;
            aiRacer->currentAnimId = 5;
        } else {
            aiRacer->currentAnimId = 2;
            aiRacer->aiBehaviourTimer = 300;
        }
    } else if ((roll_percent_chance(sp38[6]) != 0) && (raceType == RACETYPE_CHALLENGE_BANANAS) &&
               (aiRacer->bananas >= 2)) {
        aiRacer->currentAnimId = 7;
    } else if (roll_percent_chance(sp38[0]) != 0) {
        racer_ai_challenge_pick_rival(aiRacer, sp38);
    } else {
        aiRacer->currentAnimId = 2;
        aiRacer->aiBehaviourTimer = 0x12C;
    }
}

/**
 * Returns to choosing (goal 1) once the current goal is met or its time is up.
 */
static void racer_ai_challenge_check_goal_done(Object_Racer *aiRacer) {
    switch (aiRacer->currentAnimId) {
        case 2:
            if (aiRacer->aiBehaviourTimer == 0) {
                aiRacer->currentAnimId = 1;
            }
            break;
        case 3:
            if (aiRacer->balloon_type != 0) {
                aiRacer->currentAnimId = 2;
                aiRacer->aiBehaviourTimer = 180;
            }
            break;
        case 4:
            if ((aiRacer->balloon_quantity == 0) || (aiRacer->aiBehaviourTimer == 0)) {
                aiRacer->currentAnimId = 1;
                gAiEggCounts[aiRacer->eggHudCounter] = 0;
            }
            break;
        case 5:
            if ((gAiEggCounts[aiRacer->racerIndex] == 0) || (aiRacer->aiBehaviourTimer == 0)) {
                aiRacer->currentAnimId = 1;
            }
            break;
        case 7:
            if (aiRacer->bananas == 0) {
                aiRacer->currentAnimId = 1;
            }
            break;
    }
}

/**
 * Switches to recovery (goal 6) when the racer's elevation has left the range between the
 * current node and the previous one, releasing any claimed rival.
 */
static void racer_ai_challenge_check_off_route(Object_Racer *aiRacer, LevelObjectEntry *sp6C) {
    LevelObjectEntry *newvar;
    s16 sp4E;
    s16 sp46;
    Object *tempRacerObj;

    if (aiRacer->aiNodeId != 0xFF) {
        sp4E = aiRacer->elevation;
        tempRacerObj = ainode_get(aiRacer->aiNodeId); // I'm assuming this is a Ai Node (Take with a grain of salt!)
        newvar = tempRacerObj->level_entry;
        sp46 = FALSE;
        if (sp6C->aiNode.elevation < newvar->aiNode.elevation) {
            if (1) {} // FAKE
            if (newvar->aiNode.elevation < sp4E || sp4E < sp6C->aiNode.elevation) {
                sp46 = TRUE;
            }
        } else if (sp6C->aiNode.elevation < sp4E || sp4E < newvar->aiNode.elevation) {
            sp46 = TRUE;
        }
        if (sp46) {
            if (aiRacer->currentAnimId == 4) {
                gAiEggCounts[aiRacer->eggHudCounter] = 0;
            }
            aiRacer->currentAnimId = 6;
        }
    }
}

/**
 * Throttle and brake from the steering: full speed when straight, braking in hard turns.
 */
static void racer_ai_challenge_throttle(Object_Racer *aiRacer) {
    if ((gCurrentStickX > -30) && (gCurrentStickX < 30)) {
        if (aiRacer->velocity > -10.0) {
            gCurrentRacerInput = A_BUTTON;
        }
    } else {
        if (aiRacer->velocity > -4.0) {
            gCurrentRacerInput = (A_BUTTON | B_BUTTON);
        } else {
            gCurrentRacerInput = B_BUTTON;
        }
        if (aiRacer->velocity > -1.0) {
            gCurrentRacerInput = A_BUTTON;
        }
    }
}

/**
 * Brakes ahead of a sharp turn onto the next node.
 */
static void racer_ai_challenge_brake_for_turn(Object_Racer *aiRacer, Object *sp74) {
    f32 xDiff;
    f32 zDiff;
    s32 temp;
    s32 var_v1;
    Object *tempRacerObj;

    tempRacerObj = aiRacer->nodeCurrent;
    if (tempRacerObj != NULL) {
        xDiff = tempRacerObj->trans.x_position - sp74->trans.x_position;
        zDiff = tempRacerObj->trans.z_position - sp74->trans.z_position;
        if (sqrtf((xDiff * xDiff) + (zDiff * zDiff)) > 0.0) {
            temp = (arctan2_f(xDiff, zDiff) - 0x8000) & 0xFFFF;
            var_v1 = temp - (aiRacer->steerVisualRotation & 0xFFFF);
            if (var_v1 > 0x8000) {
                var_v1 -= 0xFFFF;
            }
            if (var_v1 < -0x8000) {
                var_v1 += 0xFFFF;
            }
            if ((var_v1 > 0x1500) || (var_v1 < -0x1500)) {
                if (aiRacer->velocity > -4.0) {
                    gCurrentRacerInput = (A_BUTTON | B_BUTTON);
                } else {
                    gCurrentRacerInput = B_BUTTON;
                }
            }
        }
    }
}

/**
 * Near the current node, picks the next one toward the goal: a balloon, the hunted
 * rival, its own egg, its banana drop, or simply onward.
 */
static void racer_ai_challenge_pick_next_node(Object_Racer *aiRacer, LevelObjectEntry *sp6C, f32 dist) {
    Object_Racer *tempRacer;
    LevelObjectEntry *newvar;
    s32 temp;
    Object *tempRacerObj;

    if (dist < 300.0) {
        if (aiRacer->nodeCurrent == NULL) {
            switch (aiRacer->currentAnimId) {
                case 3:
                    temp = ainode_pick_next(sp6C->animation.x_rotation, 1, aiRacer->aiNodeId, aiRacer->racerIndex);
                    break;
                case 4:
                    tempRacerObj = get_racer_object(aiRacer->eggHudCounter);
                    tempRacer = tempRacerObj->racer;
                    if (tempRacer->playerIndex == -1) {
                        tempRacerObj = tempRacer->aiNodeObj;
                        newvar = tempRacerObj->level_entry;
                        temp = ainode_pick_next(sp6C->animation.x_rotation, newvar->animation.x_rotation | 0x100,
                                                aiRacer->aiNodeId, aiRacer->racerIndex);
                    } else {
                        temp = ainode_find_nearest(tempRacerObj->trans.x_position, tempRacerObj->trans.y_position,
                                                   tempRacerObj->trans.z_position, 0);
                        temp = ainode_pick_next(sp6C->animation.x_rotation, temp | 0x100, aiRacer->aiNodeId,
                                                aiRacer->racerIndex);
                    }
                    break;
                case 5:
                    temp = ainode_pick_next(sp6C->animation.x_rotation, 1, aiRacer->aiNodeId, aiRacer->racerIndex);
                    break;
                case 7:
                    temp = ainode_pick_next(sp6C->animation.x_rotation, aiRacer->racerIndex + 4, aiRacer->aiNodeId,
                                            aiRacer->racerIndex);
                    break;
                default:
                    temp = ainode_find_next(sp6C->animation.x_rotation, aiRacer->aiNodeId, aiRacer->racerIndex);
                    break;
            }

            if (temp != 0xFF) {
                aiRacer->nodeCurrent = ainode_get(temp);
            } else {
                if (aiRacer->aiNodeId != 0xFF) {
                    aiRacer->nodeCurrent = ainode_get(aiRacer->aiNodeId);
                } else {
                    aiRacer->nodeCurrent = NULL;
                }
            }
        }
    }
}

/**
 * On reaching the current node (or overshooting it), makes the chosen next node current;
 * with none, goes back to finding a first node.
 */
static void racer_ai_challenge_advance_node(Object_Racer *aiRacer, LevelObjectEntry *sp6C, f32 dist) {
    if (dist < 50.0 || (dist > 320.0 && aiRacer->nodeCurrent != NULL)) {
        aiRacer->aiNodeId = sp6C->animation.x_rotation;
        aiRacer->aiNodeObj = aiRacer->nodeCurrent;
        if (aiRacer->nodeCurrent == NULL) {
            aiRacer->currentAnimId = 0;
        }
        aiRacer->nodeCurrent = NULL;
    }
}

/**
 * Goals 2 to 5 and 7: drives the node network toward the goal until it is done or the
 * racer leaves the route.
 */
static void racer_ai_challenge_pursue_goal(Object_Racer *aiRacer, Object *sp74, LevelObjectEntry *sp6C, f32 dist) {
    racer_ai_challenge_check_goal_done(aiRacer);
    racer_ai_challenge_check_off_route(aiRacer, sp6C);
    racer_ai_challenge_throttle(aiRacer);
    racer_ai_challenge_brake_for_turn(aiRacer, sp74);
    racer_ai_challenge_pick_next_node(aiRacer, sp6C, dist);
    racer_ai_challenge_advance_node(aiRacer, sp6C, dist);
}

/**
 * Goal 6, recovering: rejoins the network at the nearest node and goes back to choosing.
 */
static void racer_ai_challenge_recover(Object *aiRacerObj, Object_Racer *aiRacer) {
    s32 temp;

    temp = ainode_find_nearest(aiRacerObj->trans.x_position, aiRacerObj->trans.y_position, aiRacerObj->trans.z_position,
                               2);
    if (temp != 0xFF) {
        aiRacer->aiNodeObj = ainode_get(temp);
        aiRacer->currentAnimId = 1;
        aiRacer->aiNodeId = 0xFF;
    }
    aiRacer->aiBehaviour = 1;
}

/**
 * Fires the held weapon at any rival on the same level of the arena that is close and
 * roughly in front.
 */
static void racer_ai_challenge_fire_at_rivals(Object *aiRacerObj, Object_Racer *aiRacer) {
    Object_Racer *tempRacer;
    f32 xDiff;
    f32 zDiff;
    s32 temp;
    s16 i;
    s32 var_v1;
    Object *tempRacerObj;

    for (i = 0; i < 4; i++) {
        if (i != aiRacer->racerIndex) {
            tempRacerObj = get_racer_object(i);
            tempRacer = tempRacerObj->racer;
            if (tempRacer->playerIndex != -1) {
                gAiRacerElevations[i] = tempRacer->elevation;
            }
            if (gAiRacerElevations[aiRacer->racerIndex] == gAiRacerElevations[i]) {
                xDiff = aiRacerObj->trans.x_position - tempRacerObj->trans.x_position;
                zDiff = aiRacerObj->trans.z_position - tempRacerObj->trans.z_position;
                if (sqrtf((xDiff * xDiff) + (zDiff * zDiff)) < 800.0) {
                    temp = arctan2_f(xDiff, zDiff);
                    temp -= (aiRacerObj->trans.rotation.y_rotation & 0xFFFF);
                    if (temp > 0x8000) {
                        temp = -0xFFFF;
                    }
                    if (temp < -0x8000) {
                        temp = 0xFFFF;
                    }
                    if (aiRacer->balloon_level == 1) {
                        var_v1 = 0x1000;
                    } else {
                        var_v1 = 0x800;
                    }
                    if ((-var_v1 < temp) && (temp < var_v1)) {
                        gCurrentButtonsReleased |= Z_TRIG;
                    }
                }
            }
        }
    }
}

// Handles the opponent A.I. for battle & banana challenges.
void racer_ai_challenge(Object *aiRacerObj, Object_Racer *aiRacer, s32 updateRate) {
    Object *sp74;
    LevelObjectEntry *sp6C = NULL; // read before it is set on some paths in the original
    f32 dist = 0.0f; // read before it is set on some paths in the original
    s32 temp;
    LevelHeader *levelHeader;
    s8 raceType;
    s8 *sp38;

    gCurrentButtonsPressed = 0;
    gCurrentButtonsReleased = 0;
    gCurrentRacerInput = 0;
    gCurrentStickX = 0;
    gCurrentStickY = 0;
    get_racer_objects(&temp); // temp = Number of racers
    if (temp != 4) {
        return;
    }

    levelHeader = level_header();
    raceType = levelHeader->race_type;
    sp38 = levelHeader->aiChallengeChances;
    racer_ai_challenge_find_first_node(aiRacerObj, aiRacer);
    sp74 = aiRacer->aiNodeObj;
    racer_ai_challenge_steer_to_node(aiRacerObj, aiRacer, sp74, &sp6C, &dist);
    racer_ai_challenge_tick_behaviour_timer(aiRacer, updateRate);
    aiRacer->elevation = obj_elevation(aiRacerObj->trans.y_position);
    switch (aiRacer->currentAnimId) {
        case 1:
            racer_ai_challenge_choose_goal(aiRacer, sp38, raceType);
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 7:
            racer_ai_challenge_pursue_goal(aiRacer, sp74, sp6C, dist);
            break;
        case 6:
            racer_ai_challenge_recover(aiRacerObj, aiRacer);
            break;
    }
    racer_ai_challenge_fire_at_rivals(aiRacerObj, aiRacer);
}

/**
 * Manage the global flags for the egg challenge.
 * Includes hatch progress, being held by a player, and number of hatched eggs.
 */
void racer_update_eggs(Object **racerObjs) {
    Object_Racer *racer;
    s32 i;

    for (i = 0; i < 4; i++) {
        racer = racerObjs[i]->racer;
        gEggChallengeFlags[i] = racer->lap;
        if (racer->eggHudCounter != 0) {
            gEggChallengeFlags[i] |= RACER_EGG_HATCHING;
        }
        if (racer->held_obj != NULL) {
            gEggChallengeFlags[i] |= RACER_EGG_HELD;
        }
    }
}

/**
 * Counts how long the racer has been on the ground; after a second there it drops its
 * goal and heads for an egg.
 */
static void racer_ai_eggs_ground_timer(Object_Racer *racer, s32 updateRate) {
    if (racer->groundedWheels) {
        racer->aiBehaviourTimer += updateRate;
        if (racer->aiBehaviourTimer > 60) {
            racer->aiBehaviourTimer = 0;
            racer->currentAnimId = 0;
            racer->aiNodeId = 3;
        }
    } else {
        if (1) {}
        if (1) {}
        if (1) {}
        if (1) {} // Fake
        racer->aiBehaviourTimer = 0;
    }
}

/**
 * Acts on a goal request left in aiNodeId: return a stolen egg to its owner's nest, or
 * attack the leader by stealing a hatching egg, popping them, or fetching a balloon.
 * Returns the racer the goal is about, or PLAYER_COMPUTER for none.
 */
static s8 racer_ai_eggs_requested_goal(Object_Racer *racer, s8 *header) {
    s32 i;
    s8 tickCount;
    s8 flags;
    s8 racerID;
    s8 bestTick;

    racerID = PLAYER_COMPUTER;
    flags = 0;
    bestTick = 0;
    for (i = 3; i >= 0; i--) {
        tickCount = (gEggChallengeFlags[i] & RACER_EGG_MASK) * 3;
        if (gEggChallengeFlags[i] & RACER_EGG_HATCHING) {
            tickCount += 2;
        } else if (gEggChallengeFlags[i] & RACER_EGG_HELD) {
            tickCount += 1;
        }
        if (bestTick < tickCount) {
            bestTick = tickCount;
            flags = i;
        }
    }
    if (racer->aiNodeId & 0x40) {
        racerID = racer->aiNodeId & 0xF;
        racer->currentAnimId = 8;
        racer->aiNodeId = 0;
    }
    if (racer->aiNodeId & 0x80) {
        if (roll_percent_chance(header[6])) {
            bestTick = 2;
        } else {
            bestTick = 1;
            if (roll_percent_chance(header[2]) && cam_get_viewport_layout() == 0) {
                flags = 0;
            }
        }
        if (bestTick == 2) {
            for (i = 0; i < 4; i++) {
                if (racer->racerIndex != i && gEggChallengeFlags[i] & RACER_EGG_HATCHING) {
                    racerID = i;
                }
            }
            if (racerID == PLAYER_COMPUTER) {
                bestTick = 1;
            } else {
                racer->currentAnimId = 7;
            }
        }
        if (bestTick == 1) {
            if (racer->balloon_quantity != 0) {
                racer->currentAnimId = 6;
                if (flags != racer->racerIndex) {
                    racerID = flags;
                } else {
                    racerID = (racer->racerIndex + 1) & 3;
                }
            } else {
                racer->currentAnimId = 5;
            }
        }
        racer->aiNodeId = 0;
    }
    return racerID;
}

/**
 * Settles the goal: take a held egg home, take a pending request, else go for an egg when
 * pushed, or head home once finished.
 */
static void racer_ai_eggs_settle_goal(Object *obj, Object_Racer *racer, s8 *bestTick) {
    if (racer->held_obj != NULL) {
        racer->currentAnimId = 2;
    }
    if (racer->aiNodeId != 0) {
        racer->currentAnimId = racer->aiNodeId;
    }
    *bestTick = (racer->aiNodeId = 0);
    if (racer->currentAnimId == 0) {
        switch (obj->interactObj->pushForce) {
            case 1:
                racer->currentAnimId = 1;
                break;
            case 2:
                racer->currentAnimId = 1;
                break;
            case 3:
                racer->currentAnimId = 1;
                break;
        }
    }
    *bestTick = 0;
    if (racer->raceFinished) {
        racer->currentAnimId = 3;
    }
}

/**
 * The kind of object the goal heads for, with the target type it must have (bestTick)
 * and, for its own nest, whose it must be (racerID).
 */
static s32 racer_ai_eggs_goal_behaviour(Object_Racer *racer, s8 *racerID, s8 *bestTick) {
    s32 targetBehaviourID;

    targetBehaviourID = BHV_NONE;
    switch (racer->currentAnimId) {
        case 1:
            targetBehaviourID = BHV_COLLECT_EGG;
            break;
        case 2:
            targetBehaviourID = BHV_UNK_5C;
            break;
        case 3: /* fall through */
        case 7:
            targetBehaviourID = BHV_UNK_5C;
            *bestTick = 2;
            break;
        case 4:
            *racerID = racer->racerIndex;
            targetBehaviourID = BHV_UNK_5C;
            *bestTick = 2;
            break;
        case 5:
            targetBehaviourID = BHV_WEAPON_BALLOON;
            break;
        case 6:
            targetBehaviourID = BHV_RACER;
            break;
        case 8:
            targetBehaviourID = BHV_COLLECT_EGG;
            *bestTick = 1;
            break;
    }
    return targetBehaviourID;
}

/**
 * Finds the nearest object of the goal's kind that fits it: a free egg, an egg in the
 * right nest, the right nest, or any balloon.
 */
static void racer_ai_eggs_find_nearest_target(Object *obj, s32 targetBehaviourID, s8 racerID, s8 bestTick,
                                              Object **targetObj) {
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    f32 distance;
    Object **objList;
    f32 bestDist;
    s32 objStart;
    s32 objCount;
    Object_CollectEgg *egg;
    s8 tickCount;
    Object *curObj;

    if (targetBehaviourID != BHV_NONE && targetBehaviourID != BHV_RACER) {
        bestDist = 1000000.0f;
        objList = objGetObjList(&objStart, &objCount);
        *targetObj = NULL;
        for (objStart = 0; objStart < objCount; objStart++) {
            tickCount = FALSE;
            curObj = objList[objStart];
            if (!(curObj->trans.flags & OBJ_FLAGS_PARTICLE) && targetBehaviourID == curObj->behaviorId) {
                switch (targetBehaviourID) {
                    case BHV_UNK_5C:
                        if (bestTick == curObj->level_entry->bhv_unk_5C.aiTargetType &&
                            (racerID == PLAYER_COMPUTER ||
                             racerID == curObj->level_entry->bhv_unk_5C.ownerRacerIndex)) {
                            tickCount = TRUE;
                        }
                        break;
                    case BHV_COLLECT_EGG:
                        egg = curObj->egg;
                        if (bestTick == 0) {
                            if (egg->status == EGG_SPAWNED) {
                                tickCount = TRUE;
                            }
                        } else if (egg->status == EGG_IN_BASE && racerID == egg->racerID) {
                            tickCount = TRUE;
                        }
                        break;
                    default:
                        tickCount = TRUE;
                        break;
                }
            }
            if (tickCount) {
                diffX = curObj->trans.x_position - obj->trans.x_position;
                diffY = curObj->trans.y_position - obj->trans.y_position;
                diffZ = curObj->trans.z_position - obj->trans.z_position;
                distance = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
                if (distance < bestDist) {
                    bestDist = distance;
                    *targetObj = curObj;
                }
            }
        }
    }
}

/**
 * Drops a target egg that is no longer where the goal expects it, steers toward the
 * target in both axes, and returns its distance (0 with no target).
 */
static f32 racer_ai_eggs_steer_to_target(Object *obj, Object_Racer *racer, Object *curObj) {
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    f32 distance;
    s32 racerCount;
    s32 i;
    Object_CollectEgg *egg;
    s32 angleDiffX;
    s32 angleDiffY;

    distance = 0.0f;
    if (curObj != NULL) {
        if (curObj->behaviorId == BHV_COLLECT_EGG) {
            egg = curObj->egg;
            i = racer->currentAnimId;
            if (i == 1 && egg->status != EGG_SPAWNED) {
                racer->aiNodeObj = NULL;
            }
            i = racer->currentAnimId;
            if (i == 8 && egg->status != EGG_IN_BASE) {
                racer->aiNodeObj = NULL;
            }
        }
        diffX = curObj->trans.x_position - obj->trans.x_position;
        diffY = curObj->trans.y_position - obj->trans.y_position;
        diffZ = curObj->trans.z_position - obj->trans.z_position;
        distance = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
        if (distance > 0.0) {
            racerCount = (arctan2_f(diffX, diffZ) - 0x8000) & 0xFFFF;
            angleDiffX = racerCount - (racer->steerVisualRotation & 0xFFFF);
            if (angleDiffX > 0x8000) {
                angleDiffX -= 0xFFFF;
            }
            if (angleDiffX < -0x8000) {
                angleDiffX += 0xFFFF;
            }
            gCurrentStickX = -angleDiffX >> 5;
            angleDiffY = arctan2_f(diffY, sqrtf((diffX * diffX) + (diffZ * diffZ))) & 0xFFFF;
            if (angleDiffY > 0x8000) {
                angleDiffY -= 0xFFFF;
            }
            if (angleDiffY < -0x8000) {
                angleDiffY += 0xFFFF;
            }
            gCurrentStickY = -angleDiffY >> 7;
        }
        if (racer->aiSkill < 0) {
            gCurrentStickX = 0;
            gCurrentStickY = -35;
            racer->aiSkill++;
        } else {
            if (gCurrentStickX > 60 || gCurrentStickX < -60) {
                racer->aiSkill++;
                if (racer->aiSkill > 110) {
                    racer->aiSkill = -40;
                }
            } else {
                racer->aiSkill = 0;
            }
        }
    } else {
        racer->currentAnimId = 0;
    }
    return distance;
}

/**
 * Ends the goal once it is reached (dropping or firing what it carries, or queueing the
 * next request), or once it no longer makes sense.
 */
static void racer_ai_eggs_check_goal_reached(Object_Racer *racer, Object *curObj, f32 distance) {
    LevelObjectEntry_BHV_UNK_5C *objEntry;

    switch (racer->currentAnimId) {
        case 1: /* fall through */
        case 8:
            if (racer->held_obj != NULL) {
                racer->currentAnimId = 0;
                racer->aiNodeId = 4;
            }
            break;
        case 2:
            if (distance < 100.0) {
                racer->currentAnimId = 0;
                gCurrentButtonsPressed |= Z_TRIG;
            }
            break;
        case 3: /* fall through */
        case 4:
            if (distance < 200.0) {
                racer->currentAnimId = 0;
            }
            break;
        case 5:
            if (racer->balloon_quantity != 0) {
                racer->currentAnimId = 0;
            }
            break;
        case 6:
            if (racer->balloon_quantity == 0) {
                racer->currentAnimId = 0;
            }
            if (distance < 500.0) {
                gCurrentButtonsReleased |= Z_TRIG;
                racer->currentAnimId = 0;
            }
            break;
        case 7:
            if (distance < 200.0 && curObj != NULL) {
                racer->currentAnimId = 0;
                objEntry = &curObj->level_entry->bhv_unk_5C;
                racer->aiNodeId = (objEntry->ownerRacerIndex & 3) | 0x40;
            }
            break;
    }
}

/**
 * A custom AI handler for the egg challenge.
 * As a result of the open area design of Fire Mountain, the AI does not use pathing nodes.
 * Instead, they use a simple positional targeting system that takes their current priority,
 * then moves towards it until that objective is complete.
 * They will first search for any eggs in the centre of the arena, then failing that, find one
 * from another players nest to steal.
 * If they're holding an egg, they'll attempt to take it to their nest.
 */
void racer_ai_eggs(Object *obj, Object_Racer *racer, s32 updateRate) {
    f32 distance;
    s32 racerCount;
    s32 targetBehaviourID;
    Object *targetObj = NULL; // read before it is set on some paths in the original
    s8 *header;
    s8 racerID;
    s8 bestTick;
    Object *curObj;

    gCurrentButtonsPressed = 0;
    gCurrentButtonsReleased = 0;
    gCurrentRacerInput = A_BUTTON;
    gCurrentStickX = 0;
    gCurrentStickY = 0;
    get_racer_objects(&racerCount);
    if (racerCount != 4) {
        return;
    }

    header = level_header()->aiChallengeChances;
    racer_ai_eggs_ground_timer(racer, updateRate);

    while (racer->currentAnimId == 0) {
        racerID = racer_ai_eggs_requested_goal(racer, header);
        racer_ai_eggs_settle_goal(obj, racer, &bestTick);
        targetBehaviourID = racer_ai_eggs_goal_behaviour(racer, &racerID, &bestTick);
        racer_ai_eggs_find_nearest_target(obj, targetBehaviourID, racerID, bestTick, &targetObj);
        if (targetBehaviourID == BHV_RACER && racerID != PLAYER_COMPUTER) {
            targetObj = get_racer_object(racerID);
        }
        racer->aiNodeObj = targetObj;
        if (targetObj == NULL) {
            racer->aiNodeId = 0x80;
            racer->currentAnimId = 0;
        }
    }
    curObj = racer->aiNodeObj;
    distance = racer_ai_eggs_steer_to_target(obj, racer, curObj);
    racer_ai_eggs_check_goal_reached(racer, curObj, distance);
}

/**
 * Computer racer input: builds the stick and button state for this tick by
 * steering toward the checkpoint spline, then runs racer_ai_tactics.
 */
void racer_ai_drive(Object *obj, Object_Racer *racer, s32 updateRate) {
    s32 v0;
    UNUSED s32 pad_spE0;
    s32 s0;
    s32 i;
    UNUSED s32 pad_spD4;
    UNUSED s32 pad_spD0;
    s32 a0;
    s32 v1;
    s32 spC4;
    CheckpointNode *checkpoint;
    f32 splineX[4];
    f32 splineY[4];
    f32 splineZ[4];

    f32 sp8C;
    f32 sp88;
    f32 sp84;

    f32 sp74[4];
    f32 sp64[4];

    s32 a1;
    f32 xDerivative;
    f32 yDirevative;
    f32 zDerivative;
    f32 magnitude;
    s32 v00;

    spC4 = FALSE;

    gCurrentRacerInput = 0;
    gCurrentButtonsPressed = 0;
    gCurrentButtonsReleased = 0;
    gCurrentStickX = 0;
    gCurrentStickY = 0;

    v0 = get_checkpoint_count();
    if (v0 == 0) {
        gCurrentStickX = 25;
        return;
    }

    s0 = racer->nextCheckpoint - 2;
    magnitude = 1.0 - racer->checkpoint_distance;
    if (magnitude < 0.0) {
        magnitude = 0.0f;
    }
    if (magnitude > 1.0) {
        magnitude = 1;
    }
    if (s0 < 0) {
        s0 += v0;
    }
    if (s0 >= v0) {
        s0 -= v0;
    }

    for (i = 0; i < 4; i++) {
        checkpoint = find_next_checkpoint_node(s0, racer->isOnAlternateRoute);
        splineX[i] = checkpoint->x;
        splineY[i] = checkpoint->y;
        splineZ[i] = checkpoint->z;
        if (racer->aiBehaviour == 0) {
            sp74[i] = checkpoint->laneOffsetX[racer->aiLane];
            sp64[i] = checkpoint->laneOffsetY[racer->aiLane];
            splineX[i] += checkpoint->scale * checkpoint->rotationZFrac * checkpoint->laneOffsetX[racer->aiLane];
            splineY[i] += checkpoint->scale * checkpoint->laneOffsetY[racer->aiLane];
            splineZ[i] += checkpoint->scale * -checkpoint->rotationXFrac * checkpoint->laneOffsetX[racer->aiLane];
        }

        s0++;
        if (s0 == v0) {
            s0 = 0;
        }
    }

    if (racer->aiBehaviour == 0) {
        racer->trackOffsetX = sp74[1] + (sp74[2] - sp74[1]) * magnitude;
        racer->trackOffsetY = sp64[1] + (sp64[2] - sp64[1]) * magnitude;
    }

    xDerivative = catmull_rom_derivative(splineX, 0, magnitude);
    yDirevative = catmull_rom_derivative(splineY, 0, magnitude);
    zDerivative = catmull_rom_derivative(splineZ, 0, magnitude);
    magnitude = sqrtf(xDerivative * xDerivative + yDirevative * yDirevative + zDerivative * zDerivative);
    if (magnitude != 0.0f) {
        magnitude = 100.0f / magnitude;
        xDerivative *= magnitude;
        yDirevative *= magnitude;
        zDerivative *= magnitude;
    }

    s0 = (u16) (arctan2_f(xDerivative, zDerivative) - 0x8000);
    a1 = (u16) arctan2_f(yDirevative, 100.0f);

    racer->aiTargetYawPrev = racer->aiTargetYaw;
    racer->aiTargetPitchPrev = racer->aiTargetPitch;

    racer->aiTargetYaw = s0;
    racer->aiTargetPitch = a1;

    a0 = s0 - (racer->aiTargetYawPrev & 0xFFFF);
    WRAP(a0, -0x8000, 0x8000);
    v1 = a1 - (racer->aiTargetPitchPrev & 0xFFFF);
    WRAP(v1, -0x8000, 0x8000);

    gCurrentStickX = -(a0 >> 3);
    gCurrentStickY = -(v1 >> 3);

    a0 = s0 - (racer->steerVisualRotation & 0xFFFF);
    WRAP(a0, -0x8000, 0x8000);
    if (a0 > 0x3000 || a0 < -0x3000) {
        spC4 = TRUE;
    }

    s0 = racer->nextCheckpoint - 2;
    magnitude = 1.0 - racer->checkpoint_distance;
    if (magnitude < 0.0) {
        magnitude = 0.0;
    }
    if (magnitude > 1.0) {
        magnitude = 1.0;
    }
    if (spC4) {
        magnitude = 1.0;
    }

    if (racer->aiBehaviour != 0) {
        if (s0 < 0) {
            s0 += v0;
        }
        if (s0 >= v0) {
            s0 -= v0;
        }
        for (i = 0; i < 4; i++) {
            checkpoint = find_next_checkpoint_node(s0, racer->isOnAlternateRoute);
            splineX[i] = checkpoint->x + checkpoint->scale * checkpoint->rotationZFrac * racer->trackOffsetX;
            splineY[i] = checkpoint->y + checkpoint->scale * racer->trackOffsetY;
            splineZ[i] = checkpoint->z + checkpoint->scale * -checkpoint->rotationXFrac * racer->trackOffsetX;

            s0++;
            if (s0 == v0) {
                s0 = 0;
            }
        }
    }

    sp8C = cubic_spline_interpolation(splineX, 0, magnitude, &xDerivative);
    sp88 = cubic_spline_interpolation(splineY, 0, magnitude, &yDirevative);
    sp84 = cubic_spline_interpolation(splineZ, 0, magnitude, &zDerivative);
    magnitude = sqrtf(xDerivative * xDerivative + yDirevative * yDirevative + zDerivative * zDerivative);
    if (magnitude != 0.0f) {
        magnitude = 500.0f / magnitude;
        xDerivative *= magnitude;
        yDirevative *= magnitude;
        zDerivative *= magnitude;
    }

    xDerivative = sp8C + xDerivative - obj->trans.x_position;
    yDirevative = sp88 + yDirevative - obj->trans.y_position;
    zDerivative = sp84 + zDerivative - obj->trans.z_position;

    s0 = (u16) (arctan2_f(xDerivative, zDerivative) - 0x8000);
    a1 = (u16) arctan2_f(yDirevative, 500.0f);

    a0 = s0 - (u16) racer->steerVisualRotation;
    WRAP(a0, -0x8000, 0x8000);
    a0 = -a0;
    v1 = a1 - (obj->trans.rotation.x_rotation & 0xFFFF);
    WRAP(v1, -0x8000, 0x8000);
    v1 = -v1;

    v00 = 5;
    if (racer->vehicleID == VEHICLE_PLANE) {
        v00 = 6;
    }

    if (racer->vehicleID == 1 && racer->spinDirection == 0) {
        racer->steerVisualRotation -= a0 >> 1;
    }

    switch (racer->vehicleID) {
        case VEHICLE_LOOPDELOOP:
            gCurrentStickY = 0;
            break;
        case VEHICLE_HOVERCRAFT:
            gCurrentStickY = 0;
            gCurrentStickX += a0 >> v00;
            break;
        case VEHICLE_TRICKY:
            gCurrentStickX += a0 >> (v00 - 1);
            gCurrentStickY = 0;
            break;
        case VEHICLE_SMOKEY:
            v00--;
            gCurrentStickX += a0 >> v00;
            gCurrentStickY += v1 >> v00;
            break;
        default:
            gCurrentStickX += a0 >> v00;
            gCurrentStickY += v1 >> v00;
            break;
    }
    racer_ai_tactics(obj, racer, updateRate);
}

/**
 * Only ran when onscreen or nearby, this function handles more authentic, but computationally expensive looking
 * behaviour for AI controlled racers.
 * This function also calls pathing, using splines, to know where to go.
 */
void update_onscreen_AI_racer(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    f32 tempVel;
    f32 xVel;
    f32 zVel;
    f32 yVel;
    f32 hVel;
    f32 xTemp;
    f32 yTemp;
    f32 zTemp;
    MtxF mtx;
    LevelHeader *header;
    s32 angle;
    s32 steerVel;

    xTemp = obj->trans.x_position;
    yTemp = obj->trans.y_position;
    zTemp = obj->trans.z_position;
    gCurrentCarSteerVel = 0;
    gRacerTargetRoll = 0;
    handle_base_steering(racer, 0, updateRateF);
    handle_car_velocity_control(racer);
    racer_update_basis_vectors(obj, racer);
    handle_racer_items(obj, racer, updateRate);
    racer_attack_handler_car(obj, racer, updateRate);
    if (racer->spinout_timer) {
        racer_spinout_car(obj, racer, updateRate, updateRateF); // Sbinalla
    } else if (racer->groundedWheels > 0) {
        update_car_velocity_ground(obj, racer, updateRate, updateRateF);
    } else {
        update_car_velocity_offground(obj, racer, updateRate, updateRateF);
    }
    apply_vehicle_rotation_offset(racer, updateRate, 0, 0, 0);
    header = level_header();
    if ((racer->buoyancy != 0.0 && header->waterPathFollow) || gCurrentSurfaceType == SURFACE_FROZEN_WATER) {
        if (racer->waterEscapeActive) {
            racer->checkpoint_distance -= 0.3;
        } else {
            racer->checkpoint_distance -= 0.1;
        }
        set_position_goal_from_path(obj, racer, &xVel, &yVel, &zVel);
        if (racer->waterEscapeActive) {
            racer->checkpoint_distance += 0.3;
        } else {
            racer->checkpoint_distance += 0.1;
        }
        xVel -= obj->trans.x_position;
        zVel -= obj->trans.z_position;
        yVel = sqrtf((xVel * xVel) + (zVel * zVel));
        xVel /= yVel;
        zVel /= yVel;
        obj->x_velocity = xVel * 4.0f;
        obj->z_velocity = zVel * 4.0f;
        obj->y_velocity = 9.5f;
        racer->waterEscapeActive = 1;
    } else if (racer->groundedWheels > 2) {
        racer->waterEscapeActive = 0;
    }
    obj->animationID = 0;
    steerVel = ((-racer->y_rotation_vel >> 8) / gCurrentRacerHandlingStat);
    steerVel = 40 - steerVel;
    if (steerVel < 0) {
        steerVel = 0;
    }
    if (steerVel > 73) {
        steerVel = 73;
    }
    obj->animFrame = steerVel;
    slowly_reset_head_angle(racer);
    angle = gCurrentCarSteerVel - (u16) racer->y_rotation_vel;
    WRAP(angle, -0x8000, 0x8000);
    angle >>= 2;
    CLAMP(angle, -0x2EE, 0x2EE);
    racer->y_rotation_vel += (angle * updateRate);
    obj->trans.rotation.y_rotation = racer->steerVisualRotation + racer->y_rotation_vel;
    racer->z_rotation_vel += ((gRacerTargetRoll - racer->z_rotation_vel) * updateRate) >> 4;
    obj->trans.rotation.z_rotation = racer->x_rotation_vel + racer->z_rotation_vel;
    if (!racer->waterEscapeActive) {
        gCurrentRacerTransform.rotation.y_rotation = racer->steerVisualRotation + racer->driftHeadingOffset;
        gCurrentRacerTransform.rotation.x_rotation = 0;
        gCurrentRacerTransform.rotation.z_rotation = 0;
        gCurrentRacerTransform.x_position = 0.0f;
        gCurrentRacerTransform.y_position = 0.0f;
        gCurrentRacerTransform.z_position = 0.0f;
        gCurrentRacerTransform.scale = 1.0f;
        mtxf_from_inverse_transform(&mtx, &gCurrentRacerTransform);
        mtxf_transform_point(mtx, racer->lateral_velocity, 0.0f, racer->velocity, &obj->x_velocity, &tempVel,
                             &obj->z_velocity);
    }
    if (racer->magnetTimer) {
        obj->x_velocity = gRacerMagnetVelX;
        obj->z_velocity = gRacerMagnetVelZ;
    }
    if (!racer->approachTarget) {
        xVel = obj->x_velocity;
        zVel = obj->z_velocity;
        if (racer->bumpTimer != 0) {
            xVel += racer->bumpVelX;
            zVel += racer->bumpVelZ;
        }
        xVel += racer->pushVelX;
        zVel += racer->pushVelZ;
        move_object(obj, xVel * updateRateF, obj->y_velocity * updateRateF, zVel * updateRateF);
    } else {
        racer_approach_object(obj, racer, updateRateF);
    }
    if (gCurrentPlayerIndex == PLAYER_COMPUTER && !boss_race_number()) {
        onscreen_ai_racer_physics(obj, racer, updateRate);
    } else {
        update_racer_collision(obj, racer, updateRate);
    }
    if (!racer->aiPhysicsTimer) {
        obj->particleEmittersEnabled = OBJ_EMIT_NONE;
    } else if (racer->vehicleID < VEHICLE_BOSSES) {
        update_vehicle_particles(obj, updateRate);
    }
    update_car_wheels(obj, racer, updateRateF);
    tempVel = 1.0f / updateRateF;
    xVel = (((obj->trans.x_position - xTemp) - gRacerExternalShiftX) * tempVel) - racer->pushVelX;
    obj->y_velocity = (obj->trans.y_position - yTemp) * tempVel;
    zVel = (((obj->trans.z_position - zTemp) - gRacerExternalShiftZ) * tempVel) - racer->pushVelZ;
    gCurrentRacerTransform.rotation.y_rotation = -(racer->steerVisualRotation + racer->driftHeadingOffset);
    gCurrentRacerTransform.rotation.x_rotation = -obj->trans.rotation.x_rotation;
    gCurrentRacerTransform.rotation.z_rotation = -obj->trans.rotation.z_rotation;
    gCurrentRacerTransform.x_position = 0.0f;
    gCurrentRacerTransform.y_position = 0.0f;
    gCurrentRacerTransform.z_position = 0.0f;
    gCurrentRacerTransform.scale = 1.0f;
    mtxf_from_inverse_transform(&mtx, &gCurrentRacerTransform);
    mtxf_transform_point(mtx, xVel, 0.0f, zVel, &hVel, &tempVel, &yVel);
    if (racer->bumpTimer != 0) {
        racer->bumpTimer -= updateRate;
        if (racer->bumpTimer < 0) {
            racer->bumpTimer = 0;
        }
    } else {
        tempVel = racer->velocity - yVel;
        if (tempVel > 0.5) {
            racer->velocity -= tempVel - 0.5; //!@Delta
        }
        if (tempVel < -0.5) {
            racer->velocity -= tempVel + 0.5; //!@Delta
        }
        tempVel = racer->lateral_velocity - hVel;
        if (tempVel && tempVel) {}
        if (tempVel > 0.5) {
            racer->lateral_velocity -= tempVel - 0.5; //!@Delta
        }
        if (tempVel < -0.5) {
            racer->lateral_velocity -= tempVel + 0.5; //!@Delta
        }
    }
}

/**
 * Update the collision of the racer.
 * Also update wheel contacts and hitbox size.
 */
void onscreen_ai_racer_physics(Object *obj, Object_Racer *racer, UNUSED s32 updateRate) {
    f32 angleZ;
    f32 distance;
    s32 hasCollision;
    s32 flags;
    s32 temp_v1_2;
    f32 xTemp;
    f32 yTemp;
    f32 zTemp;
    s32 xRot;
    f32 angleX;
    f32 *racerSize;
    Vec3f tempPos;
    s32 i;
    f32 radius;
    s8 surface;
    s8 shouldSquish;

    if (obj->trans.y_position > gCurrentCourseHeight) {
        obj->trans.y_position = gCurrentCourseHeight;
    }
    racerSize = (f32 *) get_misc_asset(ASSET_MISC_RACER_HITBOX_SIZE);
    surface = SURFACE_NONE;
    radius = racerSize[racer->vehicleIDPrev];
    tempPos.x = obj->trans.x_position;
    tempPos.y = obj->trans.y_position;
    tempPos.z = obj->trans.z_position;
    gRacerExternalShiftX = 0.0f;
    gRacerExternalShiftZ = 0.0f;
    hasCollision = FALSE;
    flags = 0;
    if (racer->playerIndex != PLAYER_COMPUTER || racer->vehicleIDPrev < VEHICLE_BOSSES) {
        flags = collision_objectmodel(obj, 1, &hasCollision, (Vec3f *) racer->wheelCollisionPos, (f32 *) &tempPos,
                                      &radius, &surface);
    }
    if (flags & 0x80) {
        gRacerExternalShiftX = tempPos.x - obj->trans.x_position;
        gRacerExternalShiftZ = tempPos.z - obj->trans.z_position;
        flags &= ~0x80;
    }
    shouldSquish = FALSE;
    if (flags && tempPos.y < obj->trans.y_position - 4.0) {
        shouldSquish = TRUE;
    }
    generate_collision_candidates(1, (Vec3f *) racer->wheelCollisionPos, &tempPos, racer->vehicleID);
    hasCollision = FALSE;
    racer->wheelContactFlags =
        resolve_collisions((Vec3f *) racer->wheelCollisionPos, &tempPos, &radius, &surface, 1, &hasCollision);
    racer->objectContactFlags = flags;
    racer->wheelContactFlags |= flags;
    racer->groundedWheels = 0;
    if (racer->wheelContactFlags) {
        racer->wheelContactFlags = 0x8 | 0x4 | 0x2 | 0x1;
        racer->groundedWheels = 4;
    }
    if (shouldSquish && hasCollision) {
        if (racer->squish_timer == 0) {
            racer->attackType = ATTACK_SQUISHED;
        } else {
            racer->squish_timer = 60;
        }
    }
    for (i = 0; i < 3; i++) {
        racer->wheelCollisionPos[i] = tempPos.f[i];
    }
    i = 1; // Fakematch
    racer->wheel_surfaces[0] = surface;
    racer->wheel_surfaces[1] = surface;
    racer->wheel_surfaces[2] = surface;
    racer->wheel_surfaces[3] = surface;
    obj->trans.x_position = racer->wheelCollisionPos[0];
    obj->trans.y_position = racer->wheelCollisionPos[1];
    obj->trans.z_position = racer->wheelCollisionPos[2];
    if (racer->groundedWheels) {
        get_collision_normal(&xTemp, &yTemp, &zTemp);
        angleX = sins_f(-obj->trans.rotation.y_rotation);
        angleZ = coss_f(-obj->trans.rotation.y_rotation);
        distance = (xTemp * angleZ) + (zTemp * angleX);
        zTemp = (xTemp * angleX) - (zTemp * angleZ);
        temp_v1_2 = -(s16) (u16) arctan2_f(distance, yTemp);
        if (temp_v1_2 < 0x2000 && temp_v1_2 > -0x2000) {
            racer->x_rotation_vel = temp_v1_2;
        }
        xRot = -(s16) (u16) arctan2_f(zTemp, yTemp);
        if (xRot < 0x2000 && xRot > -0x2000) {
            obj->trans.rotation.x_rotation = xRot;
        }
        if (racer->vehicleID == VEHICLE_LOOPDELOOP) {
            if (1) { // Fakematch
                xRot -= (u16) obj->trans.rotation.x_rotation;
            }
            WRAP(xRot, -0x8000, 0x8000);
            obj->trans.rotation.x_rotation += xRot >> 2;
        }
    }
    if (racer->vehicleID != VEHICLE_LOOPDELOOP && racer->vehicleID != VEHICLE_PLANE &&
        racer->vehicleID != VEHICLE_CARPET && racer->vehicleID != VEHICLE_SMOKEY) {
        CLAMP(racer->x_rotation_vel, -0x3400, 0x3400);
        CLAMP(obj->trans.rotation.x_rotation, -0x3400, 0x3400);
    }
}

/**
 * Counts down the crash-sound and hit-recovery timers, and re-rolls the behaviour timer
 * during the start countdown.
 */
static void update_AI_racer_timers(Object_Racer *racer, s32 updateRate) {
    if (racer->crashSoundCooldown > 0) {
        racer->crashSoundCooldown -= updateRate;
    } else {
        racer->crashSoundCooldown = 0;
    }
    if (gRaceStartTimer != 0) {
        racer->aiBehaviourTimer = rand_range(-60, 60) + 120;
    }
    if (racer->hitRecoveryTimer > 0) {
        racer->hitRecoveryTimer -= updateRate;
    } else {
        racer->hitRecoveryTimer = 0;
    }
}

/**
 * Loads the character's weight, handling and unused stats into the current-racer globals.
 */
static void update_AI_racer_load_stats(Object_Racer *racer) {
    gCurrentRacerMiscAssetPtr = (f32 *) get_misc_asset(ASSET_MISC_RACER_WEIGHT);
    gCurrentRacerWeightStat = gCurrentRacerMiscAssetPtr[racer->characterId] * 0.45;
    if (racer->bubbleTrapTimer > 0) {
        gCurrentRacerWeightStat = -0.02f;
    }
    gCurrentRacerMiscAssetPtr = (f32 *) get_misc_asset(ASSET_MISC_RACER_HANDLING);
    gCurrentRacerHandlingStat = gCurrentRacerMiscAssetPtr[racer->characterId];
    gCurrentRacerMiscAssetPtr = (f32 *) get_misc_asset(ASSET_MISC_RACER_UNUSED_11);
    gCurrentRacerUnusedMiscAsset11 = gCurrentRacerMiscAssetPtr[racer->characterId];
}

/**
 * Ticks the position-change timer and animation counter, and forces full physics in
 * races where the spline shortcut must not be used.
 */
static void update_AI_racer_counters(Object_Racer *racer, LevelHeader *levelHeader, s32 updateRate) {
    if (racer->positionChangeTimer > 0) {
        racer->positionChangeTimer -= updateRate;
        if (racer->positionChangeTimer < 0) {
            racer->positionChangeTimer = 0;
        }
    }

    racer->miscAnimCounter++; //!@Delta

    if (is_taj_challenge() || boss_race_number() || racer->vehicleID == VEHICLE_LOOPDELOOP || gAiStartDelay > 120.0f ||
        gRaceStartTimer != 0 || levelHeader->race_type & RACETYPE_CHALLENGE_BATTLE) {
        racer->aiPhysicsTimer = 30;
    }
}

/**
 * Squared ground distance to player one, or to player two when player one is 400 units
 * or more away; 1000000 when not checked.
 */
static f32 update_AI_racer_player_dist_sq(Object *obj, Object_Racer *racer) {
    s32 var_t2;
    Object **objects;
    s32 countOfObjects;
    Object *playerOneObj;
    f32 var_fv1;
    Object *playerTwoObj;
    f32 temp_f0;
    Object_Racer *tempRacer;

    var_fv1 = 1000000;
    if (racer->aiPhysicsTimer == 0) {
        objects = get_racer_objects(&countOfObjects);
        playerOneObj = playerTwoObj = NULL;
        for (var_t2 = 0; var_t2 < countOfObjects; var_t2++) {
            tempRacer = objects[var_t2]->racer;
            if (tempRacer->playerIndex == PLAYER_ONE) {
                playerOneObj = objects[var_t2];
            }
            if (tempRacer->playerIndex == PLAYER_TWO) {
                playerTwoObj = objects[var_t2];
            }
        }
        if (playerOneObj != NULL) {
            var_fv1 = playerOneObj->trans.x_position - obj->trans.x_position;
            temp_f0 = playerOneObj->trans.z_position - obj->trans.z_position;
            var_fv1 = (var_fv1 * var_fv1) + (temp_f0 * temp_f0);
        }
        if ((playerTwoObj != NULL) && (var_fv1 >= 160000.0)) {
            var_fv1 = playerTwoObj->trans.x_position - obj->trans.x_position;
            temp_f0 = playerTwoObj->trans.z_position - obj->trans.z_position;
            var_fv1 = (var_fv1 * var_fv1) + (temp_f0 * temp_f0);
        }
    }
    return var_fv1;
}

/**
 * Bubble trap slows the racer while it lasts; a stun shows as a steering wobble.
 */
static void update_AI_racer_traps(Object_Racer *racer, s32 updateRate) {
    if (racer->bubbleTrapTimer > 0) {
        racer->bubbleTrapTimer -= updateRate;
        racer->velocity *= 0.8f; //!@Delta
    }
    if (racer->stunTimer > 0) {
        racer->steerWobbleTimer = racer->stunTimer;
        racer->stunTimer -= updateRate;
    }
}

/**
 * Full physics, first part: steering inputs from the AI path, then the pull of any effect
 * box the racer is in.
 */
static void update_AI_racer_inputs_and_effect_box(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    racer_AI_pathing_inputs(obj, racer, updateRate);
    if (!(gCurrentRacerInput & A_BUTTON)) {
        racer->throttleReleased = TRUE;
    }
    if (racer->effectBoxType == 3) {
        gCurrentRacerWeightStat *= ((f32) racer->effectBoxStrength / 256);
    }
    if (racer->effectBoxType == 1) {
        gCurrentRacerWeightStat -= ((gCurrentRacerWeightStat * racer->effectBoxStrength) / 128);
        if (racer->bubbleTrapTimer > 0) {
            gCurrentRacerWeightStat = -gCurrentRacerWeightStat;
        }
        gCurrentStickY = 60;
    }
    if (racer->effectBoxType == 2) {
        racer->pushVelX += ((((sins_f(racer->effectBoxStrength << 8) * 4) - racer->pushVelX) * 0.0625 * updateRateF));
        racer->pushVelZ += ((((coss_f(racer->effectBoxStrength << 8) * 4) - racer->pushVelZ) * 0.0625 * updateRateF));
    } else {
        racer->pushVelX -= racer->pushVelX * 0.0625 * updateRateF;
        racer->pushVelZ -= racer->pushVelZ * 0.0625 * updateRateF;
    }
}

/**
 * Full physics, setup: vehicle tables, landing, magnet, water waves and collision mode;
 * inputs are cleared while the racer may not drive.
 */
static void update_AI_racer_prepare_vehicle(Object *obj, Object_Racer *racer, s32 updateRate) {
    gCurrentRacerHandlingStat = 1;
    pc_swap_racer_tables(obj->header);
    gCurrentRacerMiscAssetPtr = (f32 *) get_misc_asset(ASSET_MISC_RACERACCELERATION_UNKNOWN0);
    gRacerWheelTable = (f32 *) get_misc_asset(obj->header->miscAssetIds[1]);
    if ((obj->y_velocity < 4.0) && ((racer->groundedWheels >= 3) || (racer->buoyancy != 0.0))) {
        racer->launchedState = 0;
    }
    if (racer->magnetTimer != 0) {
        racer_activate_magnet(obj, racer, updateRate);
    }
    if (racer->vehicleID != VEHICLE_HOVERCRAFT) {
        racer->waterTimer = 0;
        racer->buoyancy = 0;
        gRacerWaveCount = 0;
    } else {
        gRacerWaveCount =
            get_level_segment_waves(obj->segmentID, obj->trans.x_position, obj->trans.z_position, &gRacerCurrentWave);
    }
    set_collision_mode(COLLISION_MODE_DEFAULT);
    if (racer->approachTarget != NULL || gRaceStartTimer != 0 || racer->bubbleTrapTimer > 0) {
        gCurrentStickX = 0;
        gCurrentStickY = 0;
        gCurrentRacerInput = 0;
        gCurrentButtonsReleased = 0;
        gCurrentButtonsPressed = 0;
    }
}

/**
 * Full physics, movement: runs the update for the racer's vehicle type.
 */
static void update_AI_racer_vehicle(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    // clang-format off
    // The case statements break must be on the same line as the function call in order to match
    switch (racer->vehicleID) {
    case VEHICLE_CAR: update_car(updateRate, updateRateF, obj, racer); break;
    case VEHICLE_LOOPDELOOP: update_loop_de_loop(updateRate, updateRateF, obj, racer); break;
    case VEHICLE_HOVERCRAFT: update_hovercraft(updateRate, updateRateF, obj, racer); break;
    case VEHICLE_PLANE: update_plane(updateRate, updateRateF, obj, racer); break;
    case VEHICLE_FLYING_CAR: /* fall through */
    case VEHICLE_CARPET: update_carpet(updateRate, updateRateF, obj, racer); break;
    case VEHICLE_TRICKY: update_tricky(updateRate, updateRateF, obj, racer, &gCurrentRacerInput, &gCurrentButtonsPressed, &gRaceStartTimer); break;
    case VEHICLE_BLUEY: update_bluey(updateRate, updateRateF, obj, racer, &gCurrentRacerInput, &gCurrentButtonsPressed, &gRaceStartTimer); break;
    case VEHICLE_SMOKEY: /* fall through */
    case VEHICLE_PTERODACTYL: update_smokey(updateRate, updateRateF, obj, racer, &gCurrentRacerInput, &gCurrentStickX, &gRaceStartTimer); break;
    case VEHICLE_BUBBLER: update_bubbler(updateRate, updateRateF, obj, racer, &gCurrentRacerInput, &gCurrentButtonsPressed, &gRaceStartTimer); break;
    case VEHICLE_WIZPIG: update_wizpig(updateRate, updateRateF, obj, racer, &gCurrentRacerInput, &gCurrentButtonsPressed, &gRaceStartTimer); break;
    case VEHICLE_ROCKET: update_rocket(updateRate, updateRateF, obj, racer, &gCurrentRacerInput, &gCurrentButtonsPressed, &gRaceStartTimer); break;
    }
    // clang-format on
}

/**
 * Full physics, progress: checks the next checkpoint from where the racer started this
 * tick, applies the lane's special action, and advances checkpoint, lap and rank.
 */
static void update_AI_racer_checkpoints(Object *obj, Object_Racer *racer, LevelHeader *levelHeader, f32 xPos, f32 yPos,
                                        f32 zPos) {
    s32 var_t2;
    s32 countOfObjects;
    f32 lastCheckpointDist;
    s32 temp_v0_10;
    CheckpointNode *checkpoint;

    lastCheckpointDist = racer->checkpoint_distance;
    countOfObjects = racer->nextCheckpoint;
    var_t2 = checkpoint_is_passed(racer->nextCheckpoint, obj, xPos, yPos, zPos, &racer->checkpoint_distance,
                                  &racer->isOnAlternateRoute);
    if (var_t2 == -100) {
        racer_update_progress(racer);
    }
    checkpoint = find_next_checkpoint_node(racer->nextCheckpoint, racer->isOnAlternateRoute);
    if (checkpoint->laneAction[racer->aiLane] == 5) {
        racer->aiPhysicsTimer = 30;
        if (racer->waterTimer) {
            racer->isOnAlternateRoute = TRUE;
        }
    }
    if (checkpoint->laneAction[racer->aiLane] == 6) {
        racer->lap = levelHeader->laps + 1;
    }
    if (checkpoint->laneAction[racer->aiLane] == 4) {
        if (racer->velocity < -4.0) {
            racer->velocity *= 0.9; //!@Delta
        }
    }
    if (var_t2 == 0) {
        if (checkpoint->laneAction[racer->aiLane] == 2) {
            racer->isOnAlternateRoute = TRUE;
        }
        temp_v0_10 = get_checkpoint_count();
        racer->nextCheckpoint++;
        if (racer->nextCheckpoint >= temp_v0_10) {
            racer->nextCheckpoint = 0;
            if (racer->courseCheckpoint > 0) {
                if (racer->lap < 120) {
                    racer->lap++;
                }
            }
        }
        if (racer->courseCheckpoint < ((levelHeader->laps + 3) * temp_v0_10)) {
            racer->courseCheckpoint++;
        }
        racer->checkpointRank = 10000;
    } else {
        if (racer->playerIndex == PLAYER_COMPUTER && lastCheckpointDist < racer->checkpoint_distance) {
            racer->checkpoint_distance = lastCheckpointDist;
        }
        racer->checkpointRank = var_t2;
    }
    racer->checkpointPosX = obj->trans.x_position;
    racer->checkpointPosY = obj->trans.y_position;
    racer->checkpointPosZ = obj->trans.z_position;
}

/**
 * Eases the racer's squash-and-stretch height toward its target, then resets the target.
 */
static void update_AI_racer_stretch(Object_Racer *racer, f32 updateRateF) {
    f32 temp_fv1_2;

    if (racer->stretch_height <= racer->stretch_height_cap) {
        temp_fv1_2 = 0.02f;
    } else {
        temp_fv1_2 = -0.02f;
    }
    racer->stretch_height =
        (racer->stretch_height +
         ((((racer->stretch_height_cap - racer->stretch_height) * 0.125) + temp_fv1_2) * updateRateF));
    if (((temp_fv1_2 < 0.0f) && (racer->stretch_height <= racer->stretch_height_cap)) ||
        ((temp_fv1_2 > 0.0f) && (racer->stretch_height_cap <= racer->stretch_height))) {
        racer->stretch_height = racer->stretch_height_cap;
        racer->stretch_height_cap = 1.0f;
    }
}

/**
 * Turns the racer's head toward its target angle, at most 0x800 per tick.
 */
static void update_AI_racer_turn_head(Object_Racer *racer, s32 updateRate) {
    s32 var_t2;

    var_t2 = ((racer->headAngleTarget - racer->headAngle) * updateRate) >> 3;
    CLAMP(var_t2, -0x800, 0x800)
    racer->headAngle += var_t2;
}

/**
 * Keeps the shield's sound on the racer while the shield lasts, stops it near the end,
 * and drops the shield when the timer runs out.
 */
static void update_AI_racer_shield(Object *obj, Object_Racer *racer, s32 updateRate) {
    if (racer->shieldTimer > 0) {
        if (racer->shieldTimer > 60) {
            if (racer->shieldSoundMask) {
                audspat_point_set_position(racer->shieldSoundMask, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position);
            } else if (racer->vehicleSound) {
                audspat_play_sound_at_position(SOUND_SHIELD, obj->trans.x_position, obj->trans.y_position,
                                               obj->trans.z_position, AUDIO_POINT_FLAG_1, &racer->shieldSoundMask);
            }
        } else {
            if (racer->shieldSoundMask) {
                audspat_point_stop(racer->shieldSoundMask);
                racer->shieldSoundMask = NULL;
            }
        }
        racer->shieldTimer -= updateRate;
        if (racer->shieldTimer <= 0) {
            racer->shieldType = 0;
        }
    }
}

/**
 * Places the position arrow above the racer, scaled with camera distance and hidden when
 * close, then releases it for this frame.
 */
static void update_AI_racer_position_arrow(Object *obj, Object_Racer *racer) {
    if (racer->posArrowObj != NULL && gRaceStartTimer == 0) {
        s8 *temp;
        racer->posArrowObj->trans.x_position = obj->trans.x_position;
        temp = (s8 *) get_misc_asset(ASSET_MISC_0);
        racer->posArrowObj->trans.y_position = obj->trans.y_position + temp[racer->characterId];
        racer->posArrowObj->trans.z_position = obj->trans.z_position;
        racer->posArrowObj->trans.scale = obj->distanceToCamera / 265.0f;
        if (obj->distanceToCamera < 1500.0) {
            racer->posArrowObj->trans.flags |= OBJ_FLAGS_INVISIBLE;
        }
        if (racer->posArrowObj->trans.scale < 1.0) {
            racer->posArrowObj->trans.scale = 1.0f;
        }
        racer->posArrowObj = NULL;
    }
}

/**
 * The top level function for handling the actions of a computer controlled racer.
 * Mostly the same as how the player controlled one works, minus controller input and includes some decision making
 * logic.
 */
void update_AI_racer(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    s32 gameMode;
    f32 xPos;
    f32 yPos;
    f32 zPos;
    f32 var_fv1;
    LevelHeader *levelHeader;

    gCurrentPlayerIndex = -1;
    gameMode = get_game_mode();
    levelHeader = level_header();
    update_AI_racer_timers(racer, updateRate);
    update_AI_racer_load_stats(racer);
    xPos = obj->trans.x_position;
    yPos = obj->trans.y_position;
    zPos = obj->trans.z_position;
    update_AI_racer_counters(racer, levelHeader, updateRate);
    var_fv1 = update_AI_racer_player_dist_sq(obj, racer);
    update_AI_racer_traps(racer, updateRate);
    if ((racer->aiPhysicsTimer == 0) && (var_fv1 < 160000.0)) {
        racer->aiPhysicsTimer = 30;
    }
    if (racer->aiPhysicsTimer != 0) {
        update_AI_racer_inputs_and_effect_box(obj, racer, updateRate, updateRateF);
        update_AI_racer_prepare_vehicle(obj, racer, updateRate);
        update_AI_racer_vehicle(obj, racer, updateRate, updateRateF);
        if (gameMode != GAMEMODE_MENU) {
            racer_sound_update(obj, gCurrentButtonsPressed, gCurrentRacerInput, updateRate);
        }
        update_AI_racer_checkpoints(obj, racer, levelHeader, xPos, yPos, zPos);
    } else {
        racer_ai_follow_spline(obj, racer, updateRate, updateRateF);
        if (gameMode != GAMEMODE_MENU) {
            racer_sound_update(obj, gCurrentButtonsPressed, gCurrentRacerInput, updateRate);
        }
    }
    if (racer->magnetTimer == 0) {
        if (racer->magnetSoundMask != NULL) {
            sndp_stop(racer->magnetSoundMask);
            racer->magnetSoundMask = NULL;
        }
    }
    update_midi_fade_points(obj, xPos, yPos, zPos, updateRate);
    if (racer->pendingBananaDrop > 0) {
        drop_bananas(obj, racer, racer->pendingBananaDrop);
    }
    if (racer->approachTarget != NULL) {
        racer->approachTarget = NULL;
        racer->velocity = 0.0f;
        racer->lateral_velocity = 0.0f;
    }
    update_AI_racer_stretch(racer, updateRateF);
    update_AI_racer_turn_head(racer, updateRate);
    update_AI_racer_shield(obj, racer, updateRate);
    if (racer->soundMask != NULL) {
        audspat_point_set_position(racer->soundMask, obj->trans.x_position, obj->trans.y_position,
                                   obj->trans.z_position);
    }
    gRacerInputBlocked = FALSE;
    update_AI_racer_position_arrow(obj, racer);
    racer->effectBoxType = 0xFF;
    set_racer_tail_lights(racer);
}

/**
 * Moves a computer racer along the checkpoint spline directly (the cheap
 * path used while it is not near the player), setting its speed from the
 * lane and vehicle and placing it on the spline.
 */
void racer_ai_follow_spline(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    s32 j;
    s32 checkpointIdx;
    s32 checkpointCount;
    CheckpointNode *checkpoint;
    s32 i;
    f32 checkpointX[5];
    f32 checkpointY[5];
    f32 checkpointZ[5];
    s32 checkpointSplineIdx;
    f32 checkpointDistance;
    f32 var_f28;
    f32 spB8[5];
    f32 spA4[5];
    f32 var_f26;
    f32 sp9C;
    f32 sp98;
    f32 sp94;
    f32 checkpointPositionOffset; // sp90
    f32 tempRacerVelocity;        // sp8C
    f32 var_f24;
    f32 var_f12;
    LevelHeader *levelHeader;
#if VERSION == VERSION_80
    LevelModel *model;
#endif

    gCurrentRacerMiscAssetPtr = (f32 *) get_misc_asset(ASSET_MISC_RACERACCELERATION_UNKNOWN0);
    levelHeader = level_header();
    checkpointCount = get_checkpoint_count();
    if (checkpointCount == 0) {
        return;
    }

    racer->aiBehaviour = 0;
    racer->zipperDirCorrection = 0;
    racer->attackType = ATTACK_NONE;
    racer->lateral_velocity = 0;
    sp94 = 20.0f;
    if (racer->aiTargetSpeed < -sp94) {
        racer->aiTargetSpeed = -sp94;
    }

    if (racer->aiTargetSpeed > sp94) {
        racer->aiTargetSpeed = sp94;
    }

    var_f12 = sqrtf(((racer->aiTargetSpeed * 0.025) + 0.595) / 0.004);
    if (racer->boostTimer != 0) {
        var_f12 *= 1.3;
    }

    if (racer->vehicleID == VEHICLE_HOVERCRAFT) {
        i = (racer->aiTargetYaw & 0xFFFF) - (racer->aiTargetYawPrev & 0xFFFF);
        WRAP(i, -0x8000, 0x8000);
        i = ABS(i);

        i -= 200;
        if (i < 0) {
            i = 0;
        }
        sp94 = (f32) i / 150.0;
        var_f12 -= sp94;
        if (var_f12 < 2.0) {
            var_f12 = 2.0f;
        }

        if (racer->velocity < -var_f12) {
            racer->velocity = -var_f12;
        } else {
            racer->velocity += ((-var_f12 - racer->velocity) * 0.125);
        }
    } else {
        racer->velocity += ((-var_f12 - racer->velocity) * 0.25);
    }

    checkpointIdx = racer->nextCheckpoint - 2;
    if (checkpointIdx < 0) {
        checkpointIdx += checkpointCount;
    }

    if (checkpointIdx >= checkpointCount) {
        stubbed_printf("Chk ovflow!!\n");
        checkpointIdx -= checkpointCount;
    }

    for (i = 0; i < 5; i++) {
        checkpoint = find_next_checkpoint_node(checkpointIdx, racer->isOnAlternateRoute);
        checkpointX[i] = checkpoint->x;
        checkpointY[i] = checkpoint->y;
        checkpointZ[i] = checkpoint->z;
        spB8[i] = checkpoint->laneOffsetX[racer->aiLane];
        spA4[i] = checkpoint->laneOffsetY[racer->aiLane];
        checkpointX[i] += ((checkpoint->scale * checkpoint->rotationZFrac * checkpoint->laneOffsetX[racer->aiLane]));
        checkpointY[i] += ((checkpoint->scale * checkpoint->laneOffsetY[racer->aiLane]));
        checkpointZ[i] += ((checkpoint->scale * -checkpoint->rotationXFrac * checkpoint->laneOffsetX[racer->aiLane]));
        checkpointIdx++;
        if (checkpointIdx == checkpointCount) {
            checkpointIdx = 0;
            stubbed_printf("Back\n");
        }
    }

    tempRacerVelocity = racer->velocity;
    if (tempRacerVelocity < 0.0f) {
        tempRacerVelocity = -tempRacerVelocity;
    }
    checkpointSplineIdx = 0;
    if (racer->aiSplineStep == 0.0) {
        racer->aiSplineStep = 0.01f;
    }
    j = 0;
    do {
        checkpointDistance = (1.0 - racer->checkpoint_distance) + (racer->aiSplineStep * updateRateF);
        if (checkpointDistance >= 1.0) {
            checkpointSplineIdx = 1;
            checkpointDistance -= 1.0;
        }
        var_f24 = cubic_spline_interpolation(checkpointX, checkpointSplineIdx, checkpointDistance, &sp9C);
        var_f26 = cubic_spline_interpolation(checkpointY, checkpointSplineIdx, checkpointDistance, &sp98);
        var_f28 = cubic_spline_interpolation(checkpointZ, checkpointSplineIdx, checkpointDistance, &sp94);
        var_f24 -= racer->checkpointPosX;
        var_f26 -= racer->checkpointPosY;
        var_f28 -= racer->checkpointPosZ;
        if (j == 0) {
            checkpointSplineIdx = 0;
            var_f12 = sqrtf((var_f24 * var_f24) + (var_f26 * var_f26) + (var_f28 * var_f28)) / updateRateF;
            if (var_f12 != 0.0f) {
                racer->aiSplineStep *= (tempRacerVelocity / var_f12);
            } else {
                j = -1;
                racer->aiSplineStep += 0.01;
            }
        }
        j++;
    } while (j < 2);
    racer->checkpointPosX += var_f24;
    racer->checkpointPosY += var_f26;
    racer->checkpointPosZ += var_f28;
    var_f24 = racer->checkpointPosX - obj->trans.x_position;
    var_f26 = racer->checkpointPosY - obj->trans.y_position;
    var_f28 = racer->checkpointPosZ - obj->trans.z_position;
    checkpointPositionOffset = sqrtf((var_f24 * var_f24) + (var_f28 * var_f28)) / updateRateF;
    if (checkpointPositionOffset > 35.0) {
        var_f12 = (35.0 / checkpointPositionOffset);
        var_f24 *= var_f12;
        var_f28 *= var_f12;
    }
    racer->checkpoint_distance = (1.0 - checkpointDistance);
    if (checkpointSplineIdx != 0) {
        racer->nextCheckpoint++;
        if (racer->nextCheckpoint >= checkpointCount) {
            racer->nextCheckpoint = 0;
            if (racer->courseCheckpoint > 0) {
                if (racer->lap < 120) {
                    racer->lap++;
                }
            }
        }

        if (racer->courseCheckpoint < ((levelHeader->laps + 3) * checkpointCount)) {
            racer->courseCheckpoint++;
        }
        racer->checkpointRank = 10000;
    } else {
        racer->checkpointRank = racer->checkpoint_distance * 100;
    }
    if (racer->boostTimer > 0) {
        racer->boostTimer -= updateRate;
    } else {
        racer->boostTimer = 0;
    }
    racer->trackOffsetX = spB8[1] + ((spB8[2] - spB8[1]) * checkpointDistance);
    racer->trackOffsetY = spA4[1] + ((spA4[2] - spA4[1]) * checkpointDistance);
    checkpointDistance = sqrtf((sp9C * sp9C) + (sp94 * sp94));
    if (checkpointDistance != 0.0f) {
        sp9C /= checkpointDistance;
        sp98 /= checkpointDistance;
        sp94 /= checkpointDistance;
        racer->steerVisualRotation = arctan2_f(sp9C, sp94) - 0x8000;
        obj->trans.rotation.y_rotation = racer->steerVisualRotation;
        obj->trans.rotation.x_rotation = arctan2_f(sp98, 1.0f);
    }
    racer->aiTargetYawPrev = racer->aiTargetYaw;
    racer->aiTargetPitchPrev = racer->aiTargetPitch;
    racer->aiTargetYaw = racer->steerVisualRotation;
    racer->aiTargetPitch = obj->trans.rotation.x_rotation;
    if (move_object(obj, var_f24, var_f26, var_f28)) {
#if VERSION < VERSION_80
        if (1) {}
        obj->trans.x_position += var_f24;
        obj->trans.y_position += var_f26;
        obj->trans.z_position += var_f28;
#else
        model = get_current_level_model();
        obj->trans.x_position += var_f24;
        obj->trans.y_position += var_f26;
        obj->trans.z_position += var_f28;
        // Don't get caught up on this using checkpointDistance var when documenting. It just matched that way.
        checkpointDistance = model->upperXBounds + 1000.0f;
        if (obj->trans.x_position > checkpointDistance) {
            obj->trans.x_position = checkpointDistance;
        }
        checkpointDistance = model->lowerXBounds - 1000.0f;
        if (obj->trans.x_position < checkpointDistance) {
            obj->trans.x_position = checkpointDistance;
        }
        checkpointDistance = model->upperYBounds + 3000.0f;
        if (obj->trans.y_position > checkpointDistance) {
            obj->trans.y_position = checkpointDistance;
        }
        checkpointDistance = model->lowerYBounds - 500.0f;
        if (obj->trans.y_position < checkpointDistance) {
            obj->trans.y_position = checkpointDistance;
        }
        checkpointDistance = model->upperZBounds + 1000.0f;
        if (obj->trans.z_position > checkpointDistance) {
            obj->trans.z_position = checkpointDistance;
        }
        checkpointDistance = model->lowerZBounds - 1000.0f;
        if (obj->trans.z_position < checkpointDistance) {
            obj->trans.z_position = checkpointDistance;
        }
#endif
    }
    if (checkpointPositionOffset < 20.0) {
        obj->x_velocity = var_f24 / updateRateF;
        obj->y_velocity = -1.0f;
        obj->z_velocity = var_f28 / updateRateF;
    }
    racer_ai_tactics(obj, racer, updateRate);
    handle_racer_items(obj, racer, updateRate);
    racer->waterTimer = 0;
    obj->interactObj->x_position = obj->trans.x_position;
    obj->interactObj->y_position = obj->trans.y_position;
    obj->interactObj->z_position = obj->trans.z_position;
    racer->drift_direction = 0;
    racer->y_rotation_vel = 0;
    racer->z_rotation_vel = 0;
    racer->bumpTimer = 0;
    racer->carBobX = 0.0f;
    racer->carBobY = 0.0f;
    racer->carBobZ = 0.0f;
    obj->y_velocity = 0.0f;
    racer->wheelCollisionPos[0] = obj->trans.x_position;
    racer->wheelCollisionPos[1] = obj->trans.y_position;
    racer->wheelCollisionPos[2] = obj->trans.z_position;
    racer->wheelCollisionPos[3] = obj->trans.x_position;
    racer->wheelCollisionPos[4] = obj->trans.y_position;
    racer->wheelCollisionPos[5] = obj->trans.z_position;
    racer->wheelCollisionPos[6] = obj->trans.x_position;
    racer->wheelCollisionPos[7] = obj->trans.y_position;
    racer->wheelCollisionPos[8] = obj->trans.z_position;
    racer->wheelCollisionPos[9] = obj->trans.x_position;
    racer->wheelCollisionPos[10] = obj->trans.y_position;
    racer->wheelCollisionPos[11] = obj->trans.z_position;
    obj->particleEmittersEnabled = OBJ_EMIT_NONE;
    update_vehicle_particles(obj, updateRate);
}

/**
 * Updates the racer's progress in the race when they cross a checkpoint. The racer
 * completes a lap when the last checkpoint, the starting line, is reached.
 */
void racer_update_progress(Object_Racer *racer) {
    s32 count = get_checkpoint_count();

    racer->nextCheckpoint--;
    if (racer->nextCheckpoint < 0) {
        racer->nextCheckpoint += count;
        if (racer->lap > 0) {
            racer->lap--;
        }
    }

    if (racer->courseCheckpoint > -0x7D00) {
        racer->courseCheckpoint--;
    }
}
