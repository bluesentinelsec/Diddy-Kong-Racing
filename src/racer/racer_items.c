#include "racer_internal.h"

/**
 * Handles the input and activation of any weapons the player is carrying.
 * Also handles the egg object from Fire Mountain, which takes precedent, even if the player is holding a weapon.
 */
void handle_racer_items(Object *obj, Object_Racer *racer, UNUSED s32 updateRate) {
    LevelObjectEntryCommon newObject;
    s32 weaponID;
    Object *spawnedObj;
    s32 objID;
    Object *heldObj;
    Object *intendedTarget;
    Object_Racer *magnetTarget;
    f32 objDist;
    ObjectModel *model;
    Object_Weapon *weapon;
    f32 velocity;
    f32 distance;
    f32 scaleY;
    f32 scaleZ;
    UNUSED s32 pad;
    Object_CollectEgg *egg;
    s8 *miscAsset;
    Vertex *heldObjData;
    u16 soundID = SOUND_NONE;

    if (racer->held_obj != NULL) {
        heldObj = racer->held_obj;
        if (gCurrentButtonsPressed & Z_TRIG || racer->raceFinished || racer->attackType) {
            scaleY = 0;
            scaleZ = 0;
            if (obj->modelInstances[obj->modelIndex] != NULL) {
                model = obj->modelInstances[obj->modelIndex]->objModel;
                if (obj->header->heldItemAttachPoint > -1 &&
                    obj->header->heldItemAttachPoint < model->numberOfAttachPoints) {
                    if (obj->curVertData != NULL) {
                        heldObjData = obj->curVertData;
                        heldObjData += model->attachPoints[obj->header->heldItemAttachPoint];
                        scaleY = heldObjData->y;
                        scaleY *= obj->trans.scale;
                        scaleZ = heldObjData->z;
                        scaleZ *= obj->trans.scale;
                    }
                }
            }
            heldObj->trans.x_position = obj->trans.x_position + (racer->ox1 * scaleZ) + (racer->ox2 * scaleY);
            heldObj->trans.y_position = obj->trans.y_position + (racer->oy1 * scaleZ) + (racer->oy2 * scaleY);
            heldObj->trans.z_position = obj->trans.z_position + (racer->oz1 * scaleZ) + (racer->oz2 * scaleY);
            heldObj->segmentID = obj->segmentID;
            heldObj->x_velocity = obj->x_velocity * 0.7;
            heldObj->y_velocity = obj->y_velocity - 2.0;
            heldObj->z_velocity = obj->z_velocity * 0.7;
            egg = heldObj->egg;
            egg->status = EGG_MOVING;
            racer->held_obj = NULL;
            racer->eggDropZLock = 1;
        }
    } else {
        if (gCurrentButtonsPressed & Z_TRIG) {
            racer->eggDropZLock = 0;
        }
        if (racer->eggDropZLock) {
            gCurrentButtonsReleased = 0;
            gCurrentButtonsPressed &= ~Z_TRIG;
        }
        if (racer->magnetTimer == 0) {
            racer->magnetTargetObj = NULL;
        }
        if (racer->balloon_type == -1) {
            racer->balloon_quantity = 0;
        }
        if (racer->balloon_quantity <= 0) {
            if (gCurrentButtonsPressed & Z_TRIG) {
                play_char_horn_sound(obj, racer);
            }
        } else {
            miscAsset = (s8 *) get_misc_asset(ASSET_MISC_BALLOON_DATA);
            weaponID = miscAsset[(racer->balloon_type * 10) + (racer->balloon_level * 2)];
            if (miscAsset[(racer->balloon_type * 10) + (racer->balloon_level * 2)] == WEAPON_NONE) {
                racer->balloon_quantity = 0;
                return;
            }
            magnetTarget = NULL;
            if (gCurrentButtonsPressed & Z_TRIG) {
                hud_sound_stop(SOUND_VOICE_TT_POWERUP, racer->playerIndex);
            }
            if (racer->magnetLevel3) {
                if (racer->magnetTimer == 0) {
                    racer->magnetLevel3 = FALSE;
                } else {
                    return;
                }
            }
            if (gCurrentRacerInput & Z_TRIG || gCurrentButtonsReleased & Z_TRIG) {
                switch (weaponID) {
                    case WEAPON_ROCKET_HOMING:
                        intendedTarget = racer_find_weapon_target(obj, racer, &objDist);
                        racer->magnetTimer = 0;
                        racer->magnetTargetObj = intendedTarget;
                        break;
                    case WEAPON_MAGNET_LEVEL_1:
                    case WEAPON_MAGNET_LEVEL_3:
                    case WEAPON_MAGNET_LEVEL_2:
                        intendedTarget = racer_find_weapon_target(obj, racer, &objDist);
                        racer->magnetTimer = 0;
                        if (weaponID == WEAPON_MAGNET_LEVEL_1) {
                            distance = 1000.0f;
                        } else {
                            distance = 1500.0f;
                        }
                        if (objDist < distance) {
                            if (weaponID == WEAPON_MAGNET_LEVEL_3 && intendedTarget != NULL) {
                                magnetTarget = intendedTarget->racer;
                            }
                            racer->magnetTargetObj = intendedTarget;
                        } else {
                            racer->magnetTargetObj = NULL;
                            intendedTarget = NULL;
                        }
                        break;
                }
            }
            objID = ASSET_OBJECT_ID_MISSILE;
            if (gCurrentButtonsReleased & Z_TRIG) {
                velocity = 30.0f;
                switch (weaponID) {
                    case WEAPON_ROCKET_HOMING:
                        objDist = -10.0f;
                        objID = ASSET_OBJECT_ID_HOMING;
                        break;
                    case WEAPON_ROCKET:
                        objDist = -10.0f;
                        break;
                    case WEAPON_TRIPMINE:
                    case WEAPON_UNK_11:
                        objDist = 10.0f;
                        velocity = -2.0f;
                        objID = ASSET_OBJECT_ID_BOMB;
                        break;
                    case WEAPON_OIL_SLICK:
                        if (racer->vehicleID != VEHICLE_PLANE) {
                            objID = ASSET_OBJECT_ID_OILSLICK;
                        } else {
                            objID = ASSET_OBJECT_ID_SMOKECLOUD;
                        }
                        objDist = 10.0f;
                        velocity = -2.0f;
                        break;
                    case WEAPON_BUBBLE_TRAP:
                        objDist = 10.0f;
                        velocity = -2.0f;
                        objID = ASSET_OBJECT_ID_BUBBLEWEAPON;
                        intendedTarget = NULL;
                        break;
                    case WEAPON_NITRO_LEVEL_1:
                    case WEAPON_NITRO_LEVEL_2:
                    case WEAPON_NITRO_LEVEL_3:
                        // clang-format off
                        switch (weaponID) {
                            case WEAPON_NITRO_LEVEL_3:
                                // IDO demands this to be on 1 line.
                                racer->boostTimer = normalise_time(75); \
                                racer->boostType = BOOST_LARGE;
                                break;
                            case WEAPON_NITRO_LEVEL_2:
                                // Ditto
                                racer->boostTimer = normalise_time(55); \
                                racer->boostType = BOOST_MEDIUM;
                                break;
                            default:
                                // :(
                                racer->boostTimer = normalise_time(35); \
                                racer->boostType = BOOST_SMALL;
                                break;
                        }
                        // clang-format on
                        if (racer->throttleReleased) {
                            racer->boostType |= EMPOWER_BOOST;
                        }
                        if (weaponID == WEAPON_NITRO_LEVEL_3) {
                            racer_play_sound(obj, SOUND_NITRO_LEVEL3_CHARGE);
                            racer_play_sound_after_delay(obj, SOUND_NITRO_LEVEL3_BOOST, 30);
                        } else {
                            racer_play_sound(obj, SOUND_NITRO_BOOST);
                        }
                        racer->balloon_quantity -= 1;
                        if (weaponID == WEAPON_NITRO_LEVEL_1) {
                            if (racer->raceFinished == FALSE) {
                                rumble_set(racer->playerIndex, RUMBLE_TYPE_6);
                            }
                        } else if (racer->raceFinished == FALSE) {
                            rumble_set(racer->playerIndex, RUMBLE_TYPE_8);
                        }
                        return;
                    case WEAPON_MAGNET_LEVEL_1:
                    case WEAPON_MAGNET_LEVEL_2:
                        racer->balloon_quantity -= 1;
                        if (racer->playerIndex != PLAYER_COMPUTER) {
                            if (intendedTarget != NULL) {
                                racer->magnetTimer = 90;
                                racer->magnetModelID = (weaponID - WEAPON_MAGNET_LEVEL_1) >> 1;
                            }
                            if (racer->raceFinished == FALSE) {
                                rumble_set(racer->playerIndex, RUMBLE_TYPE_15);
                            }
                        }
                        return;
                    case WEAPON_MAGNET_LEVEL_3:
                        racer->balloon_quantity -= 1;
                        racer->magnetTargetObj = NULL;
                        if (racer->playerIndex != PLAYER_COMPUTER) {
                            if (magnetTarget != NULL) {
                                magnetTarget->magnetLevel3 = TRUE;
                                magnetTarget->magnetTimer = 120;
                                magnetTarget->magnetTargetObj = obj;
                                magnetTarget->magnetModelID = MAGNET_LEVEL3;
                            }
                            if (racer->raceFinished == FALSE) {
                                rumble_set(racer->playerIndex, RUMBLE_TYPE_15);
                            }
                        }
                        return;
                    case WEAPON_SHIELD_LEVEL_1:
                        racer->shieldType = SHIELD_LEVEL1;
                        racer->shieldTimer = 300;
                        racer->balloon_quantity -= 1;
                        return;
                    case WEAPON_SHIELD_LEVEL_2:
                        racer->shieldType = SHIELD_LEVEL2;
                        racer->balloon_quantity -= 1;
                        racer->shieldTimer = 600;
                        return;
                    case WEAPON_SHIELD_LEVEL_3:
                        racer->shieldType = SHIELD_LEVEL3;
                        racer->shieldTimer = 900;
                        racer->balloon_quantity -= 1;
                        return;
                    default:
                        objDist = 0;
                        break;
                }
                play_random_character_voice(obj, SOUND_VOICE_CHARACTER_POSITIVE, 8, 129);
                newObject.x = obj->trans.x_position + (racer->ox1 * objDist);
                newObject.y = obj->trans.y_position + (racer->oy1 * objDist) + (10.0f * racer->oy2);
                newObject.z = obj->trans.z_position + (racer->oz1 * objDist);
                newObject.size = sizeof(LevelObjectEntryCommon);
                newObject.objectID = objID;
                spawnedObj = spawn_object(&newObject, OBJECT_SPAWN_UNK01);
                if (spawnedObj != NULL) {
                    spawnedObj->level_entry = NULL;
                    spawnedObj->x_velocity = obj->x_velocity - (racer->ox1 * velocity);
                    spawnedObj->y_velocity = obj->y_velocity - (racer->oy1 * velocity);
                    spawnedObj->z_velocity = obj->z_velocity - (racer->oz1 * velocity);
                    spawnedObj->trans.rotation.y_rotation = obj->trans.rotation.y_rotation;
                    spawnedObj->trans.rotation.x_rotation = obj->trans.rotation.x_rotation;
                    if (racer->vehicleID == VEHICLE_HOVERCRAFT && racer->waterTimer) {
                        if (spawnedObj->trans.rotation.x_rotation > -0x400 &&
                            spawnedObj->trans.rotation.x_rotation < 0x400) {
                            spawnedObj->trans.rotation.x_rotation = 0;
                        }
                    }
                    weapon = spawnedObj->weapon;
                    weapon->owner = obj;
                    weapon->target = intendedTarget;
                    weapon->checkpoint = racer->nextCheckpoint;
                    weapon->forwardVel = (racer->velocity - velocity);
                    weapon->weaponID = weaponID;
                    switch (weapon->weaponID) {
                        case WEAPON_ROCKET_HOMING:
                            soundID = SOUND_NYOOM2;
                            break;
                        case WEAPON_ROCKET:
                            soundID = SOUND_NYOOM3;
                            break;
                        case WEAPON_TRIPMINE:
                            soundID = SOUND_PLOP;
                            break;
                        case WEAPON_OIL_SLICK:
                            if (racer->vehicleID != VEHICLE_PLANE) {
                                spawnedObj->shadow->scale = 0;
                            }
                            soundID = SOUND_SPLOINK2;
                            break;
                        case WEAPON_BUBBLE_TRAP:
                            soundID = SOUND_PLOP;
                            break;
                        case WEAPON_UNK_11:
                            spawnedObj->animFrame = 0;
                            break;
                    }
                    if (soundID != SOUND_NONE) {
                        if (racer->playerIndex == PLAYER_COMPUTER) {
                            audspat_play_sound_at_position(soundID, obj->trans.x_position, obj->trans.y_position,
                                                           obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER,
                                                           NULL);
                        } else {
                            if (racer->weaponSoundMask) {
                                sndp_stop(racer->weaponSoundMask);
                            }
                            sound_play_spatial(soundID, obj->trans.x_position, obj->trans.y_position,
                                               obj->trans.z_position, &racer->weaponSoundMask);
                        }
                    }
                }
                if (racer->balloon_quantity > 0) {
                    racer->balloon_quantity = racer->balloon_quantity - 1;
                } else {
                    racer->balloon_quantity = 0;
                }
            }
        }
    }
}

/**
 * Honk honk
 */
void play_char_horn_sound(Object *obj, Object_Racer *racer) {
    if (get_filtered_cheats() & CHEAT_HORN_CHEAT) {
        // Play character voice instead of horn.
        play_random_character_voice(obj, SOUND_VOICE_CHARACTER_POSITIVE, 8, 130);
    } else {
        // Play character's horn sound
        racer_play_sound(obj, racer->characterId + SOUND_HORN_CHARACTER);
    }
}

/**
 * The nearest racer in front, for the magnet and homing rockets; computer
 * racers outside challenges take the nearest opponent instead.
 */
Object *racer_find_weapon_target(Object *racerObj, Object_Racer *racer, f32 *outDistance) {
    Object_Racer *curRacer;
    s32 i;
    f32 curDistance;
    f32 racerOx1;
    f32 racerOy1;
    Object *savedRacerObj;
    Object *curRacerObj;
    Object **racerObjects;
    f32 racerOz1;
    f32 racerOx3;
    f32 racerOz3;
    f32 savedDist;
    f32 racerO1dist;
    f32 racerOy3;
    f32 racerO3dist;
    f32 curDistance2;
    s32 numRacers;
    f32 savedDist2;
    s32 isChallengeRace;
    s32 raceType;

    raceType = level_type();
    isChallengeRace = raceType & RACETYPE_CHALLENGE;
    if (racer->playerIndex == PLAYER_COMPUTER && !isChallengeRace) {
        curDistance = 0.0f;
        curRacerObj = racer_find_nearest_opponent_relative(racer, 1, &curDistance);
        *outDistance = curDistance;
        return curRacerObj;
    }
    racerOx1 = racer->ox1;
    racerOy1 = racer->oy1;
    racerOz1 = racer->oz1;
    racerO1dist = -((racerObj->trans.x_position * racerOx1) + (racerObj->trans.y_position * racerOy1) +
                    (racerObj->trans.z_position * racerOz1));
    racerOx3 = racer->ox3;
    racerOy3 = racer->oy3;
    racerOz3 = racer->oz3;
    racerO3dist = -((racerObj->trans.x_position * racerOx3) + (racerObj->trans.y_position * racerOy1) +
                    (racerObj->trans.z_position * racerOz3));
    racerObjects = get_racer_objects(&numRacers);
    // TODO: Need better names for these
    savedDist2 = 10000.0f;
    savedDist = 10000.0f;
    savedRacerObj = NULL;
    for (i = 0; i < numRacers; i++) {
        if (racerObj != racerObjects[i]) {
            curRacerObj = racerObjects[i];
            curRacer = racerObjects[i]->racer;
            if ((!isChallengeRace || !curRacer->raceFinished) && curRacer->elevation == racer->elevation) {
                curDistance =
                    -((racerOx1 * curRacerObj->trans.x_position) + (racerOy1 * curRacerObj->trans.y_position) +
                      (racerOz1 * curRacerObj->trans.z_position) + racerO1dist);
                if (curDistance > 0.0f) {
                    curDistance2 = (racerOx3 * curRacerObj->trans.x_position) +
                                   (racerOy3 * curRacerObj->trans.y_position) +
                                   (racerOz3 * curRacerObj->trans.z_position) + racerO3dist;
                    if (curDistance2 < 0.0f) {
                        curDistance2 = -curDistance2;
                    }
                    if ((curDistance2 < curDistance || raceType == RACETYPE_BOSS) && curDistance2 < savedDist2) {
                        savedDist = curDistance;
                        savedDist2 = curDistance2;
                        savedRacerObj = curRacerObj;
                    }
                }
            }
        }
    }
    *outDistance = savedDist;
    return savedRacerObj;
}

/**
 * When active, takes the magnet target, calculates the distance then sets the player's velocity
 * and movement direction towards the target until they either get close, or the timer runs out.
 * Shields will also cancel out the magnet.
 */
void racer_activate_magnet(Object *obj, Object_Racer *racer, s32 updateRate) {
    f32 diffX;
    f32 diffZ;
    f32 vel;
    Object_Racer *magnetTarget;

    racer->magnetTimer -= updateRate;
    if (racer->magnetTimer < 0) {
        racer->magnetTimer = 0;
        return;
    }
    if (racer->magnetTargetObj == NULL) {
        racer->magnetTimer = 0;
        return;
    }
    diffX = racer->magnetTargetObj->trans.x_position - obj->trans.x_position;
    diffZ = racer->magnetTargetObj->trans.z_position - obj->trans.z_position;
    vel = sqrtf((diffX * diffX) + (diffZ * diffZ));
    if (vel > 150.0f && vel < 2000.0f) {
        if (racer->playerIndex != PLAYER_COMPUTER) {
            racer->boostTimer = 3;
        }
        racer->boostType = BOOST_NONE;
        if (racer->throttleReleased) {
            racer->boostType |= EMPOWER_BOOST;
        }
        if (racer->magnetSoundMask == NULL && racer->raceFinished == FALSE) {
            sound_play(SOUND_MAGNET_HUM, &racer->magnetSoundMask);
        }
    } else {
        racer->magnetTimer = 0;
        return;
    }
    diffX /= vel;
    diffZ /= vel;
    magnetTarget = racer->magnetTargetObj->racer;
    vel = -magnetTarget->velocity;
    if (vel < 8.0 && racer->magnetLevel3 == FALSE) {
        vel = 8.0f;
    }
    if (vel > 20.0) {
        vel = 20.0f;
    }
    gRacerMagnetVelX = (vel + 5.0f) * diffX;
    gRacerMagnetVelZ = (vel + 5.0f) * diffZ;
    if (magnetTarget->shieldTimer && racer->magnetLevel3 == FALSE) {
        racer->magnetTimer = 0;
    }
}

/**
 * Play a spatial sound, emitting from the position of the racer object.
 * Only affects human players that aren't in the middle of going through an exit.
 */
void racer_play_sound(Object *obj, s32 soundID) {
    Object_Racer *racer = obj->racer;
    if (gCurrentPlayerIndex != PLAYER_COMPUTER && racer->exitObj == NULL) {
        sound_play_spatial(soundID, obj->trans.x_position, obj->trans.y_position, obj->trans.z_position, NULL);
    }
}

/**
 * Set a timer and sound ID.
 * When that timer counts zero in the racer's update loop, it plays the sound passed through here.
 */
void racer_play_sound_after_delay(Object *obj, s32 soundID, s32 delay) {
    Object_Racer *racer = obj->racer;
    racer->delaySoundID = soundID;
    racer->delaySoundTimer = delay;
}

/**
 * Play a random character voice clip from the given offset.
 * It uses SOUND_VOICE_CHARACTER_POSITIVE and SOUND_VOICE_CHARACTER_NEGATIVE for
 * acceptable offsets. Range will always be 8, because that's how many ID's for each
 * there are.
 */
void play_random_character_voice(Object *obj, s32 soundID, s32 range, s32 flags) {
    s32 soundIndex;
    Object_Racer *tempRacer;

    tempRacer = obj->racer;
    if (tempRacer->exitObj == 0 && (!(flags & 0x80) || gCurrentPlayerIndex != PLAYER_COMPUTER)) {
        if (flags == 2) {
            if (tempRacer->soundMask != NULL && soundID != tempRacer->lastVoiceSoundID) {
                audspat_point_stop(tempRacer->soundMask);
                tempRacer->soundMask = 0;
            }
        }
        if (tempRacer->soundMask == NULL && (flags != 3 || rand_range(0, 1))) {
            tempRacer->lastVoiceSoundID = soundID;
            soundID += tempRacer->characterId;
            soundIndex = (rand_range(0, range - 1) * 12) + soundID;
            if (range - 1 > 0) {
                while (soundIndex == tempRacer->lastSoundID) {
                    soundIndex = (rand_range(0, range - 1) * 12) + soundID;
                }
            }
            audspat_play_sound_at_position(soundIndex, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER,
                                           &tempRacer->soundMask);
            tempRacer->lastSoundID = soundIndex;
        }
    }
}

/**
 * This function dictates the base top speed of a racer.
 * During the race start sequence it hardsets it to 0 so you don't go anywhere.
 * It then adds to the multiplier an amount based on your bananas.
 */
f32 handle_racer_top_speed(Object *obj, Object_Racer *racer) {
    f32 speedMultiplier;
    s32 timer;
    f32 bananas;
    f32 cap;
    s32 timer2;
    s32 timer3;

    // If you want to change the baseline speed of vehicles, this is what you change.
    speedMultiplier = 1.0f;
    // Set the player's top speed to 0 before the race starts, so you can't jump the start.
    if (gRaceStartTimer) {
        speedMultiplier = 0.0f;
    }
    timer3 = get_race_start_timer();
    if (gRaceStartTimer) {} // Fakematch
    // If the A button is held for the first time, 30 frames prior to starting,
    // decide how much boost to add based on when it was pressed.
    if (gRaceStartTimer > 0 && gRaceStartTimer < 30 && !racer->startInput) {
        timer = gRaceStartTimer - 14;
        if (gCurrentButtonsPressed & A_BUTTON) {
            if (timer < 0 && timer3 >= 0) {
                timer = 0;
            }
            // Keep the timer positive.
            if (timer < 0) {
                timer = -timer;
            }
            if (gCurrentRacerInput & Z_TRIG) {
                if (timer < 2) {
                    timer = 0;
                }
            }
            timer2 = 24 - timer;
            racer->boostTimer = normalise_time(timer2 >> 1);
            if (timer2 == 24) {
                racer_play_sound(obj, SOUND_SELECT);
                racer->boostTimer = normalise_time(20);
            }
            if (racer->boostTimer < normalise_time(20)) {
                racer->boostType = BOOST_SMALL;
            } else if (racer->boostTimer < normalise_time(35)) {
                racer->boostType = BOOST_MEDIUM;
            } else {
                racer->boostType = BOOST_LARGE;
            }
            racer->boost_sound |= BOOST_RACE_START;
            gStartBoostTime = racer->boostTimer;
        }
        // For the AI, expert AI will match the players start, and master AI will get a good start always.
        if (racer->playerIndex == PLAYER_COMPUTER &&
            (racer->aiSkill == AI_MASTER || (racer->aiSkill == AI_EXPERT && gStartBoostTime))) {
            if (gStartBoostTime) {
                racer->boostTimer = gStartBoostTime;
            } else {
                racer->boostTimer = normalise_time(5);
            }
            if (racer->boostTimer < normalise_time(20)) {
                racer->boostType = BOOST_SMALL;
            } else if (racer->boostTimer < normalise_time(35)) {
                racer->boostType = BOOST_MEDIUM;
            } else {
                racer->boostType = BOOST_LARGE;
            }
        }
    }
    if (racer->boostTimer && !gRaceStartTimer && timer3 && racer->raceFinished == FALSE) {
        rumble_set(racer->playerIndex, RUMBLE_TYPE_6);
    }
    if (gRaceStartTimer < 80 && gCurrentButtonsPressed & A_BUTTON) {
        racer->startInput = 1;
    }
    if (!gRaceStartTimer) {
        if (racer->boost_sound & BOOST_RACE_START) {
            racer->boost_sound &= ~BOOST_RACE_START;
            play_random_character_voice(obj, SOUND_VOICE_CHARACTER_POSITIVE, 8, 130);
            racer_play_sound(obj, SOUND_NITRO_BOOST);
        }
    }

    if (racer->boost_sound & BOOST_SOUND_UNK2) {
        racer->boost_sound &= ~BOOST_SOUND_UNK2;
    }
    bananas = racer->bananas;
    // Cheats only apply to human players.
    if (racer->playerIndex == PLAYER_COMPUTER) {
        bananas = racer->aiTargetSpeed;
    } else {
        if (bananas > 10.0) {
            // Cap the banana speed boost to 10 if the unlimited bananas cheat is not enabled.
            if (!(get_filtered_cheats() & CHEAT_NO_LIMIT_TO_BANANAS)) {
                bananas = 10.0f;
            }
        }
        // Make the speed bonus negative if the speed reduction cheat is on.
        if (((get_filtered_cheats() & CHEAT_BANANAS_REDUCE_SPEED)) && (bananas > 0.0f)) {
            bananas = -bananas;
        }
    }
    // This also means the AI can have twice as many
    cap = 20.0f;
    if (bananas > cap) {
        bananas = cap;
    }
    if (bananas < -cap) {
        bananas = -cap;
    }
    speedMultiplier += (bananas * 0.025f);
    return speedMultiplier;
}

/**
 * Recomputes the racer's forward (o1), up (o2) and right (o3) unit vectors
 * from its yaw and pitch.
 */
void racer_update_basis_vectors(Object *obj, Object_Racer *racer) {
    MtxF mtxF;

    gCurrentRacerTransform.rotation.y_rotation = obj->trans.rotation.y_rotation;
    gCurrentRacerTransform.rotation.x_rotation = obj->trans.rotation.x_rotation;
    gCurrentRacerTransform.rotation.z_rotation = 0;
    gCurrentRacerTransform.x_position = 0.0f;
    gCurrentRacerTransform.y_position = 0.0f;
    gCurrentRacerTransform.z_position = 0.0f;
    gCurrentRacerTransform.scale = 1.0f;
    mtxf_from_transform(&mtxF, &gCurrentRacerTransform);
    mtxf_transform_point(mtxF, 0.0f, 0.0f, 1.0f, &racer->ox1, &racer->oy1, &racer->oz1);
    mtxf_transform_point(mtxF, 0.0f, 1.0f, 0.0f, &racer->ox2, &racer->oy2, &racer->oz2);
    mtxf_transform_point(mtxF, 1.0f, 0.0f, 0.0f, &racer->ox3, &racer->oy3, &racer->oz3);
}

/**
 * Racers struck by most weapons will drop up to 2 bananas on the ground.
 * During challenge mode, no bananas are spawned, though the number still drops.
 */
void drop_bananas(Object *obj, Object_Racer *racer, s32 number) {
    LevelObjectEntryCommon newObject;
    Object_Banana *banana;
    s32 i;
    RPYAngles angle;
    Vec3s pos;
    Object *bananaObj;
    f32 variance;

    if (!(get_filtered_cheats() & CHEAT_START_WITH_10_BANANAS)) {
        racer->pendingBananaDrop = 0;
        if (racer->bananas < number) {
            number = racer->bananas;
        }
        if (number > 0 && number < 3) {
            angle.z_rotation = racer->x_rotation_vel;
            angle.x_rotation = obj->trans.rotation.x_rotation;
            angle.y_rotation = racer->steerVisualRotation;
            pos.x = 0;
            pos.y = 8;
            pos.z = 12;
            vec3s_rotate_rpy(&angle, &pos);
            newObject.x = pos.x + (s32) obj->trans.x_position;
            newObject.y = pos.y + (s32) obj->trans.y_position;
            newObject.z = pos.z + (s32) obj->trans.z_position;
            newObject.size = sizeof(LevelObjectEntryCommon);
            newObject.objectID = ASSET_OBJECT_ID_COIN;
            i = number;
            do {
                if (level_type() != RACETYPE_CHALLENGE) {
                    bananaObj = spawn_object(&newObject, OBJECT_SPAWN_UNK01);
                    if (bananaObj != NULL) {
                        bananaObj->level_entry = NULL;
                        banana = bananaObj->banana;
                        banana->droppedVehicleID = racer->vehicleID;
                        bananaObj->x_velocity = racer->ox1 * 2;
                        bananaObj->y_velocity = (0.0f - racer->oy1) + 5.0;
                        bananaObj->z_velocity = racer->oz1 * 2;
                        if (i == 2) {
                            variance = -0.5f;
                            if (number == 1) {
                                variance = 0.5f;
                            }
                            bananaObj->x_velocity -= (racer->ox3 - racer->ox1) * variance;
                            bananaObj->y_velocity -= racer->oy3 * variance;
                            bananaObj->z_velocity -= (racer->oz3 - racer->oz1) * variance;
                        }
                        bananaObj->properties.banana.status = BANANA_DROPPED;
                    }
                }
                number--;
                racer->bananas--;
            } while (number > 0);
        }
    }
}
