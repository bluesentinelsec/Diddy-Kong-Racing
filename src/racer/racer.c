#include "racer_internal.h"

/**
 * This file features extensive use of CLAMP and WRAP.
 * CLAMP will keep the value within the two ranges.
 * WRAP will "overflow" the value. Used to keep angles within s16 bounds.
 */

extern int pc_trace_autopilot(void); // linux/input.c: scripted traces can let a human racer drive itself
/************ .data ************/

s32 gObjLoopGoldenBalloonLength = 0x310;
s16 gAntiPiracyHeadroll = 0;

s32 gNumViewports = 0; // Currently unknown, might be a different type.
// Table used for quantifying speed reduction while the car drives over it, like how grass will slow you down more than
// the road. An antipiracy trigger can set the first index to 0.5f, which makes that surface type impossible to drive
// on.
f32 gSurfaceTractionTable[19] = {
    0.004f, 0.007f, 0.01f,  0.004f, 0.01f,  0.01f,  0.01f,  0.01f,  0.01f,  0.01f,
    0.004f, 0.004f, 0.004f, 0.004f, 0.004f, 0.004f, 0.004f, 0.004f, 0.004f,
};
// Fraction of the car's sideways (lateral) velocity kept each frame while driving over this surface, averaged across
// the grounded wheels. Higher values let the car slide further, lower values grip harder.
f32 gSurfaceSlideTable[19] = {
    0.8f, 0.85f, 0.85f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.8f, 0.8f, 0.84f, 0.8f, 0.8f, 0.8f, 0.8f, 0.8f,
};
// When driving over this surface, the car begins to bob up and down to give the effect of roughness.
s32 gSurfaceBobbingTable[19] = {
    0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
// When landing on this surface, it makes a sound.
// Strangely, they did only two surface types then called it a day.
s32 gSurfaceSoundTable[19] = { SOUND_NONE, SOUND_LAND_GRASS, SOUND_LAND_SAND, SOUND_NONE, SOUND_NONE,
                               SOUND_NONE, SOUND_NONE,       SOUND_NONE,      SOUND_NONE, SOUND_NONE,
                               SOUND_NONE, SOUND_NONE,       SOUND_NONE,      SOUND_NONE, SOUND_NONE,
                               SOUND_NONE, SOUND_NONE,       SOUND_NONE,      SOUND_NONE };

// Checksum count for obj_loop_goldenballoon
s32 gObjLoopGoldenBalloonChecksum = ObjLoopGoldenBalloonChecksum;

FadeTransition gDoorFadeTransition = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_NONE, FADE_COLOR_BLACK, 50, FADE_STAY);

/*******************************/

/************ .bss ************/

f32 gCurrentCourseHeight;
Vec3f gCurrentRacerWaterPos;
s8 gRacerWaveType;
Camera *gCameraObject;
ObjectTransform gCurrentRacerTransform;
u32 gCurrentRacerInput;
u32 gCurrentButtonsPressed;
u32 gCurrentButtonsReleased;
s32 gCurrentStickX;
s32 gCurrentStickY;
s32 unused_8011D53C; // Set to 0 and only 0. Checked for being 1, but never true.
s32 gRaceStartTimer;
f32 gAiStartDelay; // Starts are 300, then counts down when the race starts. Usage currently unknown.
f32 gRacerExternalShiftX;
f32 gRacerExternalShiftZ;
u16 gRacerSurfaceSoundId;
u16 gRacerSurfaceSoundVolume;
s32 gCurrentCarSteerVel;
s32 gRacerTargetRoll;
s32 gCurrentPlayerIndex;
f32 *gCurrentRacerMiscAssetPtr;
f32 *gRacerWheelTable;

f32 gCurrentRacerWeightStat;
f32 gCurrentRacerHandlingStat;
f32 gCurrentRacerUnusedMiscAsset11; // Set, but never read
f32 gRacerMagnetVelX;
f32 gRacerMagnetVelZ;
u8 gFinalLapAnnounced;
s8 gCurrentSurfaceType;
s8 gTajInteractStatus;
s8 gRacerDialogueCamera;
s8 gRacerInputBlocked;
s8 gStartBoostTime;
s16 gDialogueCameraAngle;
s8 gEggChallengeFlags[4];
s8 gAiEggCounts[4];
GhostNode *gGhostData[3];
s8 gCurrentGhostIndex;
s8 gPrevGhostNodeIndex;
s16 gGhostNodeDelay;
s16 gGhostNodeCount[3];
s16 gGhostNodeFull[2];
s16 gGhostMapID; // Previous MapId?
s8 gRacerWaveCount;
WaterProperties **gRacerCurrentWave;
s8 gAiRacerElevations[4];
s16 gAiCatchupTimer;

/******************************/

/**
 * The two per-vehicle f32 tables reached through the object header, swapped
 * where they are fetched because their misc-asset index is not a constant:
 *
 *   unk5C - the acceleration/velocity curve (gCurrentRacerMiscAssetPtr)
 *   unk5D - the wheel table, 4 wheels x { x, y, z, radius } (gRacerWheelTable)
 *
 * Both were being read big-endian. A wheel offset of 8.0 (0x41000000) becomes
 * 0x00000041 — a denormal, i.e. zero — so all four wheels collapsed onto the
 * racer's origin with radius 0. That made the ground probe in func_80054FD0
 * zero-length (origin == target), and resolve_collisions can never satisfy
 * "origin above the plane, target below it" with a zero-length probe: no wheel
 * ever touched the ground. The acceleration curve read as zeros the same way,
 * which is why the player could not move.
 */
void pc_swap_racer_tables(ObjectHeader *header) {
    pc_swap_misc_f32_once(header->miscAssetIds[0]);
    pc_swap_misc_f32_once(header->miscAssetIds[1]);
}

/**
 * Step `diff` units towards a target at diff * rate / divisor per frame, always moving at
 * least one unit while the two differ.
 */
s32 ease_step(s32 diff, f32 rate, f32 divisor) {
    s32 step;

    step = (diff * rate) / divisor;
    if ((diff != 0) && (step == 0)) {
        if (diff > 0) {
            step = 1;
        }
        if (diff < 0) {
            step = -1;
        }
    }
    return step;
}

/**
 * Throttle builds while A is held and bleeds off when it is not, by `step` per frame.
 */
void racer_ease_throttle(Object_Racer *racer, f32 updateRateF, f64 step) {
    if (gCurrentRacerInput & A_BUTTON) {
        racer->throttle += updateRateF * step;
        if (racer->throttle > 1.0) {
            racer->throttle = 1.0;
        }
    } else {
        racer->throttle -= updateRateF * step;
        if (racer->throttle < 0.0f) {
            racer->throttle = 0.0f;
        }
    }
}

/**
 * Brake builds while B is held with the stick pulled back or while reversing, by `step` per
 * frame, up to 1.2. Returns TRUE while braking.
 */
s32 racer_ease_brake(Object_Racer *racer, f32 updateRateF, f64 step) {
    if ((gCurrentRacerInput & B_BUTTON) && (gCurrentStickY < -40 || racer->velocity < 0.0f)) {
        racer->brake += updateRateF * step;
        if (racer->brake > 1.2) {
            racer->brake = 1.2f;
        }
        return TRUE;
    }
    racer->brake -= updateRateF * step;
    if (racer->brake < 0.0f) {
        racer->brake = 0.0f;
    }
    return FALSE;
}

/**
 * Lining up with a zipper: turn towards its heading, slowed until aligned, then boost.
 * AI racers boost at once.
 */
void racer_zipper_align(Object *obj, Object_Racer *racer, s32 updateRate) {
    s32 diff;

    diff = racer->zipperObj->trans.rotation.y_rotation - (racer->steerVisualRotation & 0xFFFF);
    if (diff > 0x8000) {
        diff -= 0xFFFF;
    }
    if (diff < -0x8000) {
        diff += 0xFFFF;
    }
    racer->steerVisualRotation += (diff * updateRate) >> 3;
    if ((diff < 0x400 && diff > -0x400) || (racer->playerIndex == PLAYER_COMPUTER)) {
        if (racer->playerIndex != PLAYER_COMPUTER) {
            sound_play_spatial(SOUND_ZIP_PAD_BOOST, obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                               NULL);
            play_random_character_voice(obj, SOUND_VOICE_CHARACTER_POSITIVE, 8, 0x80 | 0x2);
        }
        racer->boostTimer = normalise_time(45);
        racer->boostType = BOOST_LARGE;
        if (racer->throttleReleased) {
            racer->boostType |= EMPOWER_BOOST;
        }
        racer->zipperDirCorrection = 0;
        rumble_set(racer->playerIndex, RUMBLE_TYPE_8);
    } else {
        obj->x_velocity = (obj->x_velocity * 1.0) * 0.75f;
        obj->y_velocity = (obj->y_velocity * 1.0) * 0.75f;
        obj->z_velocity = (obj->z_velocity * 1.0) * 0.75f;
    }
}

/**
 * The most significant surface under the wheels, and how many wheels are on stone.
 */
s32 racer_dominant_surface(Object_Racer *racer, s32 *stoneWheels) {
    s32 surface;
    s32 i;

    surface = SURFACE_DEFAULT;
    *stoneWheels = 0;
    for (i = 0; i < 4; i++) {
        if (racer->wheel_surfaces[i] == SURFACE_NONE) {
            continue;
        }
        if (surface < racer->wheel_surfaces[i]) {
            surface = racer->wheel_surfaces[i];
        }
        if (racer->wheel_surfaces[i] == SURFACE_STONE) {
            (*stoneWheels)++;
        }
    }
    return surface;
}

/**
 * Honking the horn on Taj's pad summons him.
 */
void racer_check_taj_pad(Object_Racer *racer, s32 surface) {
    if (racer->playerIndex == PLAYER_ONE && racer->groundedWheels && surface == SURFACE_TAJ_PAD &&
        gCurrentButtonsPressed & Z_TRIG) {
        gTajInteractStatus = TAJ_TELEPORT;
    }
}

/**
 * A zip pad gives a large boost, empowered if the throttle was released for it.
 */
void racer_start_zip_pad_boost(Object *obj, Object_Racer *racer) {
    racer->boostTimer = normalise_time(45);
    racer->boostType = BOOST_LARGE;
    if (racer->throttleReleased) {
        racer->boostType |= EMPOWER_BOOST;
    }
    racer_play_sound(obj, SOUND_ZIP_PAD_BOOST);
    play_random_character_voice(obj, SOUND_VOICE_CHARACTER_POSITIVE, 8, 0x80 | 0x2);
    rumble_set(racer->playerIndex, RUMBLE_TYPE_8);
}

/**
 * Acceleration available at the current speed, from the character's 13-entry speed curve.
 */
f32 racer_speed_curve(f32 speed) {
    s32 index;
    f32 frac;

    index = speed;
    frac = speed - index;
    return (gCurrentRacerMiscAssetPtr[index + 1] * frac) + (gCurrentRacerMiscAssetPtr[index] * (1.0 - frac));
}

/**
 * Light the boost-flame emitter while the boost meter asset shows one charging or burning.
 * Empowered boosts use the emitter one above `baseBit`.
 */
void racer_enable_boost_emitter(Object *obj, Object_Racer *racer, s32 baseBit) {
    Object_Boost *boost;
    s32 bit;

    boost = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    boost = &boost[racer->racerIndex];
    bit = ((racer->boostType & EMPOWER_BOOST) >> 2) + baseBit;
    if (bit > baseBit) {
        if (boost->effectState > 0 || boost->effectScale > 0.0) {
            obj->particleEmittersEnabled |= 1 << bit;
        }
    } else if (boost->effectState == 2 && boost->effectScale < 0.5) {
        obj->particleEmittersEnabled |= 1 << bit;
    } else if (boost->effectState < 2 && boost->effectScale > 0.0f) {
        obj->particleEmittersEnabled |= 1 << bit;
    }
}

/**
 * Express the world velocity in the racer's own frame: lateral, vertical and forward.
 */
void racer_velocity_to_local(Object *obj, Object_Racer *racer, s32 includeRoll) {
    MtxF mtx;

    gCurrentRacerTransform.rotation.y_rotation = -obj->trans.rotation.y_rotation;
    gCurrentRacerTransform.rotation.x_rotation = -obj->trans.rotation.x_rotation;
    if (includeRoll) {
        gCurrentRacerTransform.rotation.z_rotation = -obj->trans.rotation.z_rotation;
    } else {
        gCurrentRacerTransform.rotation.z_rotation = 0;
    }
    gCurrentRacerTransform.scale = 1.0f;
    gCurrentRacerTransform.x_position = 0.0f;
    gCurrentRacerTransform.y_position = 0.0f;
    gCurrentRacerTransform.z_position = 0.0f;
    mtxf_from_inverse_transform(&mtx, &gCurrentRacerTransform);
    mtxf_transform_point(mtx, obj->x_velocity, obj->y_velocity, obj->z_velocity, &racer->lateral_velocity,
                         &racer->vehiclePitch, &racer->velocity);
}

/**
 * Run the collision step (AI racers use their cheaper physics), keeping a bump that was
 * already running from being restarted by it.
 */
void racer_collide_keeping_bump(Object *obj, Object_Racer *racer, s32 updateRate) {
    s32 bumpTimer;

    bumpTimer = racer->bumpTimer;
    if (gCurrentPlayerIndex == PLAYER_COMPUTER && !boss_race_number()) {
        onscreen_ai_racer_physics(obj, racer, updateRate);
    } else {
        update_racer_collision(obj, racer, updateRate);
    }
    if (bumpTimer == 0 && racer->bumpTimer != 0) {
        bumpTimer = 1;
    }
    racer->bumpTimer = bumpTimer;
}

/**
 * When on water, apply a rotation effect based on the movement of the waves and turning direction.
 */
f32 rotate_racer_in_water(Object *obj, Object_Racer *racer, Vec3f *pos, s8 waveType, s32 updateRate, s32 steerX,
                          f32 driftScale) {
    MtxF mtxF;
    f32 velocity;
    s32 angle;
    s32 angleVel;
    f32 updateRateF;

    updateRateF = updateRate;
    if (waveType == SURFACE_WATER_WAVY) {
        velocity = racer->velocity;
        if (velocity < 0.0f) {
            velocity = -velocity;
        }
        velocity = 1.0 - (velocity * (1.0 / 6.0));
        if (velocity < 0.0f) {
            velocity = 0.0f;
        }
    } else {
        obj->trans.x_position += pos->x * updateRateF * driftScale;
        obj->trans.z_position += pos->z * updateRateF * driftScale;
        velocity = 1.0f;
    }
    gCurrentRacerTransform.rotation.y_rotation = -obj->trans.rotation.y_rotation;
    gCurrentRacerTransform.rotation.x_rotation = 0;
    gCurrentRacerTransform.rotation.z_rotation = 0;
    gCurrentRacerTransform.x_position = 0.0f;
    gCurrentRacerTransform.y_position = 0.0f;
    gCurrentRacerTransform.z_position = 0.0f;
    gCurrentRacerTransform.scale = 1.0f;
    mtxf_from_inverse_transform(&mtxF, &gCurrentRacerTransform);
    mtxf_transform_point(mtxF, pos->x, pos->y, pos->z, &pos->x, &pos->y, &pos->z);
    angle = -((s16) (u16) arctan2_f(pos->x, pos->y)) * velocity;
    angle = (u16) (angle - (steerX << 6)) - (u16) racer->x_rotation_vel;
    angle = angle > 0x8000 ? angle - 0xffff : angle;
    angle = angle < -0x8000 ? angle + 0xffff : angle;
    racer->x_rotation_vel += (angle * updateRate) >> 4;
    angleVel = ((s16) (u16) arctan2_f(pos->z, pos->y) * velocity);
    angleVel += -gCurrentStickY * 32;
    angleVel += 0x3C0;
    angle = (u16) angleVel - ((u16) obj->trans.rotation.x_rotation);
    angle = angle > 0x8000 ? angle - 0xffff : angle;
    angle = angle < -0x8000 ? angle + 0xffff : angle;
    obj->trans.rotation.x_rotation += (angle * updateRate) >> 4;
    return velocity;
}

/**
 * Initialise the basic properties of each racer object. If it's tied to a human player,
 * will also initialise a camera object.
 */
void obj_init_racer(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Racer *racer = &levelEntry->racer;
    Object_Racer *tempRacer;
    ActivePlayers player;
    s32 i;

    unused_8011D53C = 0;
    tempRacer = obj->racer;
    obj->trans.rotation.y_rotation = racer->angleY;
    obj->trans.rotation.x_rotation = racer->angleX;
    obj->trans.rotation.z_rotation = racer->angleZ;
    player = racer->playerIndex;
    tempRacer->countLap = 0;
    tempRacer->stretch_height = 1.0f;
    tempRacer->stretch_height_cap = 1.0f;
    // Decide which player ID to assign to this object. Human players get a value from 0-3.
    // Computer players will be -1.
    if (player >= PLAYER_ONE && player <= PLAYER_FOUR) {
        if (is_race_started_by_player_two()) {
            //!@bug player ID could be negative if there are more than two players
            player = 1 - player;
        }
        tempRacer->playerIndex = player;
    } else {
        tempRacer->playerIndex = PLAYER_COMPUTER;
    }
    tempRacer->steerVisualRotation = obj->trans.rotation.y_rotation;
    tempRacer->x_rotation_vel = obj->trans.rotation.z_rotation;
    tempRacer->waterBobScale = 0.5f;
    if (1) {} // Fakematch
    tempRacer->cameraYaw = tempRacer->steerVisualRotation;
    tempRacer->wheelCollisionPos[0] = obj->trans.x_position;
    tempRacer->wheelCollisionPos[1] = obj->trans.y_position + 30.0f;
    tempRacer->wheelCollisionPos[2] = obj->trans.z_position;
    tempRacer->wheelCollisionPos[3] = obj->trans.x_position;
    tempRacer->wheelCollisionPos[4] = obj->trans.y_position + 30.0f;
    tempRacer->wheelCollisionPos[5] = obj->trans.z_position;
    tempRacer->wheelCollisionPos[6] = obj->trans.x_position;
    tempRacer->wheelCollisionPos[7] = obj->trans.y_position + 30.0f;
    tempRacer->wheelCollisionPos[8] = obj->trans.z_position;
    tempRacer->wheelCollisionPos[9] = obj->trans.x_position;
    tempRacer->wheelCollisionPos[10] = obj->trans.y_position + 30.0f;
    tempRacer->wheelCollisionPos[11] = obj->trans.z_position;
    tempRacer->prev_x_position = obj->trans.x_position;
    tempRacer->prev_y_position = obj->trans.y_position;
    tempRacer->prev_z_position = obj->trans.z_position;
    obj->interactObj->x_position = obj->trans.x_position;
    obj->interactObj->y_position = obj->trans.y_position;
    obj->interactObj->z_position = obj->trans.z_position;
    tempRacer->groundedWheels = 3;
    tempRacer->racerOrder = 1;
    tempRacer->racePosition = 1;
    tempRacer->miscAnimCounter = tempRacer->playerIndex * 5;
    tempRacer->checkpoint_distance = 1.0f;
    tempRacer->cameraIndex = 0;
    tempRacer->magnetSoundMask = NULL;
    tempRacer->shieldSoundMask = NULL;
    tempRacer->bananaSoundMask = NULL;
    tempRacer->weaponSoundMask = NULL;
    tempRacer->crashSoundHandle = 0;
    tempRacer->landingSoundHandle = NULL;
    if (tempRacer->playerIndex != PLAYER_COMPUTER && gTajInteractStatus == TAJ_WANDER) {
        set_active_camera(player);
        gCameraObject = cam_get_active_camera_no_cutscenes();
        gCameraObject->trans.rotation.z_rotation = 0;
        gCameraObject->trans.rotation.x_rotation = 0x400;
        gCameraObject->trans.rotation.y_rotation = tempRacer->cameraYaw;
        gCameraObject->mode = CAMERA_CAR;
        gCameraObject->unused3C = 0xFF;
        gCameraObject->unused3D = 0xFF;
        gCameraObject->unused3E = 0xFF;
        gCameraObject->unused3F = 0xFF;
        gCameraObject->cam_unk_18 = 0.0f;
        update_player_camera(obj, tempRacer, 1.0f);
    }
    if (gTajInteractStatus == TAJ_WANDER) {
        gRacerDialogueCamera = FALSE;
        gDialogueCameraAngle = 0;
        gRacerInputBlocked = FALSE;
    }
    obj->interactObj->flags = INTERACT_FLAGS_SOLID | INTERACT_FLAGS_UNK_0004;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 15;
    obj->interactObj->pushForce = 20;
    tempRacer->slideFrames = 0;
    if (gTajInteractStatus == TAJ_WANDER) {
        tempRacer->transparency = 255;
    }
    gAiStartDelay = 300.0f;
    tempRacer->aiBehaviour = 6;
    tempRacer->aiBehaviourTimer = 100;
    gFinalLapAnnounced = 0;

    // clang-format off
    // This needs to be on one line to match.
    for (i = 0; i < 4; i++) { gAiEggCounts[i] = 0; }
    // clang-format on
    if (1) {}
    if (1) {} // Fakematch
    increment_ai_behaviour_chances(NULL, NULL, 0);
    gRacerDialogueCamera = i;
    gStartBoostTime = 0;
    tempRacer->lightFlags = 0;
}

/**
 * Caps the racer's velocities to +/-50 and its checkpoint distance to [-1, 2].
 * Rareware didn't appear to use a clamp macro here, which would've saved a lot of real estate.
 */
static void racer_clamp_velocities(Object *obj, Object_Racer *tempRacer) {
    if (obj->x_velocity > 50.0) {
        obj->x_velocity = 50.0f;
    }
    if (obj->y_velocity > 50.0) {
        obj->y_velocity = 50.0f;
    }
    if (obj->z_velocity > 50.0) {
        obj->z_velocity = 50.0f;
    }
    if (obj->x_velocity < -50.0) {
        obj->x_velocity = -50.0f;
    }
    if (obj->y_velocity < -50.0) {
        obj->y_velocity = -50.0f;
    }
    if (obj->z_velocity < -50.0) {
        obj->z_velocity = -50.0f;
    }
    if (tempRacer->velocity > 50.0) {
        tempRacer->velocity = 50.0f;
    }
    if (tempRacer->lateral_velocity > 50.0) {
        tempRacer->lateral_velocity = 50.0f;
    }
    if (tempRacer->velocity < -50.0) {
        tempRacer->velocity = -50.0f;
    }
    if (tempRacer->lateral_velocity < -50.0) {
        tempRacer->lateral_velocity = -50.0f;
    }
    if (tempRacer->checkpoint_distance < -1.0) {
        tempRacer->checkpoint_distance = -1.0f;
    }
    if (tempRacer->checkpoint_distance > 2.0) {
        tempRacer->checkpoint_distance = 2.0f;
    }
}

/**
 * Counts down the crash sound cooldown and the AI physics timer, and clears the particle emitters.
 */
static void racer_tick_common_timers(Object *obj, Object_Racer *tempRacer, s32 updateRate) {
    tempRacer->crashSoundCooldown -= updateRate;
    if (tempRacer->crashSoundCooldown < 0) {
        tempRacer->crashSoundCooldown = 0;
    }
    obj->particleEmittersEnabled = OBJ_EMIT_NONE;
    if (tempRacer->aiPhysicsTimer > 0) {
        tempRacer->aiPhysicsTimer -= updateRate;
    } else {
        tempRacer->aiPhysicsTimer = 0;
    }
}

/**
 * Prints player 1's coordinates to the screen if the debug cheat is enabled,
 * then sets the debug text background colour.
 */
static void racer_print_debug_coords(Object *obj, Object_Racer *tempRacer) {
    if (gRaceStartTimer == 0 && tempRacer->playerIndex == PLAYER_ONE) {
        if (get_filtered_cheats() & CHEAT_PRINT_COORDS) {
            render_printf("%.1f,%.1f,%.1f\n", obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
        }
    }
    set_render_printf_background_colour(0, 0, 0, 128);
}

/**
 * Counts down the AI start delay once the race is underway (rerolling the AI behaviour timer before then),
 * and counts down the hit recovery timer.
 */
static void racer_tick_start_timers(Object_Racer *tempRacer, s32 updateRate, f32 updateRateF) {
    if (gRaceStartTimer == 0) {
        if (gAiStartDelay > 0.0) {
            gAiStartDelay -= updateRateF;
        } else {
            gAiStartDelay = 0.0f;
        }
    } else {
        tempRacer->aiBehaviourTimer = rand_range(-60, 60) + 120;
    }
    if (tempRacer->hitRecoveryTimer > 0) {
        tempRacer->hitRecoveryTimer -= updateRate;
    } else {
        tempRacer->hitRecoveryTimer = 0;
    }
}

/**
 * Loads the character's weight, handling and unused stat from the misc assets.
 */
static void racer_load_stats(Object_Racer *tempRacer) {
    gCurrentRacerMiscAssetPtr = (f32 *) get_misc_asset(ASSET_MISC_RACER_WEIGHT);
    gCurrentRacerWeightStat = gCurrentRacerMiscAssetPtr[tempRacer->characterId] * 0.45;
    if (tempRacer->bubbleTrapTimer > 0) {
        gCurrentRacerWeightStat = -0.02f;
    }
    gCurrentRacerMiscAssetPtr = (f32 *) get_misc_asset(ASSET_MISC_RACER_HANDLING);
    gCurrentRacerHandlingStat = gCurrentRacerMiscAssetPtr[tempRacer->characterId];
    gCurrentRacerMiscAssetPtr = (f32 *) get_misc_asset(ASSET_MISC_RACER_UNUSED_11);
    gCurrentRacerUnusedMiscAsset11 = gCurrentRacerMiscAssetPtr[tempRacer->characterId];
}

/**
 * Applies the effect box the racer is in: scales or inverts the weight stat,
 * and pushes the racer along (or decays the push velocity outside of a push box).
 */
static void racer_apply_effect_box(Object_Racer *tempRacer, f32 updateRateF) {
    if (tempRacer->effectBoxType == 3) {
        gCurrentRacerWeightStat *= (f32) tempRacer->effectBoxStrength / 256;
    }
    if (tempRacer->effectBoxType == 1) {
        gCurrentRacerWeightStat -= (gCurrentRacerWeightStat * tempRacer->effectBoxStrength) / 128;
        if (tempRacer->bubbleTrapTimer > 0) {
            gCurrentRacerWeightStat = -gCurrentRacerWeightStat;
        }
    }
    if (tempRacer->effectBoxType == 2) {
        tempRacer->pushVelX +=
            ((sins_f((tempRacer->effectBoxStrength << 8)) * 4.0f) - tempRacer->pushVelX) * 0.0625 * updateRateF;
        tempRacer->pushVelZ +=
            ((coss_f((tempRacer->effectBoxStrength << 8)) * 4.0f) - tempRacer->pushVelZ) * 0.0625 * updateRateF;
    } else {
        tempRacer->pushVelX -= tempRacer->pushVelX * 0.0625 * updateRateF;
        tempRacer->pushVelZ -= tempRacer->pushVelZ * 0.0625 * updateRateF;
    }
}

/**
 * Loads the vehicle's misc asset and wheel tables.
 */
static void racer_load_vehicle_tables(Object *obj) {
    pc_swap_racer_tables(obj->header);
    gCurrentRacerMiscAssetPtr = (f32 *) get_misc_asset(obj->header->miscAssetIds[0]);
    gRacerWheelTable = (f32 *) get_misc_asset(obj->header->miscAssetIds[1]);
}

/**
 * Counts down the timer since the racer's race position last changed.
 */
static void racer_tick_position_change_timer(Object_Racer *tempRacer, s32 updateRate) {
    if (tempRacer->positionChangeTimer > 0) {
        tempRacer->positionChangeTimer -= updateRate;
        if (tempRacer->positionChangeTimer < 0) {
            tempRacer->positionChangeTimer = 0;
        }
    }
}

/**
 * Makes the racer's camera active, bumps its animation counter and records it as the current player.
 */
static void racer_select_camera(Object_Racer *tempRacer) {
    set_active_camera(tempRacer->playerIndex);
    gCameraObject = cam_get_active_camera_no_cutscenes();
    tempRacer->miscAnimCounter++; //!@Delta
    gCurrentPlayerIndex = tempRacer->playerIndex;
}

/**
 * Hands control to the AI once the racer has finished the race, or when in the menu.
 */
static void racer_hand_over_to_ai(Object_Racer *tempRacer, s32 context) {
    if (tempRacer->raceFinished == TRUE || context == GAMEMODE_MENU) {
        tempRacer->aiLane = 1;
        tempRacer->playerIndex = PLAYER_COMPUTER;
        tempRacer->aiBehaviour = 0;
        if (tempRacer->shieldTimer > 5) {
            tempRacer->shieldTimer = 5;
        }
    }
}

/**
 * Port addition for the trace autopilot. The AI only presses the accelerator from its race
 * tactics, which give up when there are no computer racers to race against. In a time trial
 * the autopilot holds the throttle itself, except while it reverses out of a stall.
 */
static void racer_autopilot_solo_throttle(Object_Racer *racer) {
    if (!is_in_time_trial() || racer->aiReverseTimer != 0) {
        return;
    }
    // Leave the start-grid line for the racing lanes, as the tactics would once the race is on.
    if (gRaceStartTimer == 0 && racer->aiBehaviour == 6) {
        racer->aiBehaviour = 0;
    }
    gCurrentRacerInput |= A_BUTTON;
}

// How near the hub autopilot gets to an AI node before heading for the next one.
#define AUTOPILOT_HUB_NODE_REACHED 150.0f

// The AI node the hub autopilot is heading for, the one it came from, and the racer it drives.
static s32 sAutopilotHubNode = NODE_NONE;
static s32 sAutopilotHubPrevNode = NODE_NONE;
static Object *sAutopilotHubRacer = NULL;

/**
 * Port addition for the trace autopilot: the next node to head for after arriving at a node,
 * any linked one other than the node it came from, so the racer goes round the loop.
 */
static s32 racer_autopilot_hub_next_node(s32 nodeId, s32 prevNodeId) {
    LevelObjectEntry_AiNode *entry;
    Object *node;
    s32 fallback;
    s32 i;

    node = ainode_get(nodeId);
    if (node == NULL) {
        return NODE_NONE;
    }
    entry = &node->level_entry->aiNode;
    fallback = NODE_NONE;
    for (i = 0; i < 4; i++) {
        if (entry->adjacent[i] == NODE_NONE || ainode_get(entry->adjacent[i]) == NULL) {
            continue;
        }
        if (entry->adjacent[i] != prevNodeId) {
            return entry->adjacent[i];
        }
        fallback = entry->adjacent[i];
    }
    // A dead end turns back.
    return fallback;
}

/**
 * Port addition for the trace autopilot: the nearest AI node with a link to another node,
 * so a racer starting beside a lone node still finds the hub's loop.
 */
static s32 racer_autopilot_hub_nearest_node(Object *obj) {
    Object *node;
    f32 xDiff;
    f32 zDiff;
    f32 dist;
    f32 bestDist;
    s32 best;
    s32 i;

    best = NODE_NONE;
    bestDist = 0.0f;
    // Node ids fit below NODE_NONE; ainode_get gives NULL for ids past the node table.
    for (i = 0; i < NODE_NONE; i++) {
        node = ainode_get(i);
        if (node == NULL || racer_autopilot_hub_next_node(i, NODE_NONE) == NODE_NONE) {
            continue;
        }
        xDiff = node->trans.x_position - obj->trans.x_position;
        zDiff = node->trans.z_position - obj->trans.z_position;
        dist = (xDiff * xDiff) + (zDiff * zDiff);
        if (best == NODE_NONE || dist < bestDist) {
            best = i;
            bestDist = dist;
        }
    }
    return best;
}

/**
 * Port addition for the trace autopilot. The race AI steers along checkpoints and presses the
 * accelerator only from its race tactics, so in a hub, where nobody races, it sits still.
 * The central hub has the checkpoints of Taj's course for the race AI to follow; the other
 * hubs have none, so the autopilot drives round the loop of AI nodes their characters use.
 * Either way it holds the throttle, except while it reverses out of a stall.
 */
static void racer_autopilot_hub(Object *obj, Object_Racer *racer) {
    Object *node;
    f32 xDiff;
    f32 zDiff;
    s32 nextNode;
    s32 angle;

    if (level_type() != RACETYPE_HUBWORLD || racer->aiReverseTimer != 0) {
        return;
    }
    gCurrentRacerInput |= A_BUTTON;
    if (get_checkpoint_count() != 0) {
        return;
    }
    if (sAutopilotHubRacer != obj) {
        sAutopilotHubRacer = obj;
        sAutopilotHubPrevNode = NODE_NONE;
        sAutopilotHubNode = racer_autopilot_hub_nearest_node(obj);
    }
    node = ainode_get(sAutopilotHubNode);
    if (node == NULL) {
        return;
    }
    xDiff = node->trans.x_position - obj->trans.x_position;
    zDiff = node->trans.z_position - obj->trans.z_position;
    if ((xDiff * xDiff) + (zDiff * zDiff) < AUTOPILOT_HUB_NODE_REACHED * AUTOPILOT_HUB_NODE_REACHED) {
        nextNode = racer_autopilot_hub_next_node(sAutopilotHubNode, sAutopilotHubPrevNode);
        sAutopilotHubPrevNode = sAutopilotHubNode;
        sAutopilotHubNode = nextNode;
        return;
    }
    // Steer for the node as the battle AI does (racer_ai_challenge_steer_to_node).
    angle = ((arctan2_f(xDiff, zDiff) - 0x8000) & 0xFFFF) - (racer->steerVisualRotation & 0xFFFF);
    if (angle > 0x8000) {
        angle -= 0xFFFF;
    }
    if (angle < -0x8000) {
        angle += 0xFFFF;
    }
    gCurrentStickX = -angle >> 4;
    CLAMP(gCurrentStickX, -75, 75);
}

/**
 * Reads this frame's stick and button input from the controller, the trace autopilot, the door exit or the AI,
 * and notes whether the throttle was released.
 */
static void racer_read_input(Object *obj, Object_Racer *tempRacer, s32 updateRate) {
    s32 controllerIndex;

    controllerIndex = tempRacer->playerIndex;
    if (tempRacer->playerIndex != PLAYER_COMPUTER && tempRacer->exitObj == 0 && pc_trace_autopilot()) {
        // A scripted trace holding AUTO: the human racer drives itself (linux/input.c), until a
        // hub door takes over the steering below.
        racer_AI_pathing_inputs(obj, tempRacer, updateRate);
        racer_autopilot_solo_throttle(tempRacer);
        racer_autopilot_hub(obj, tempRacer);
    } else if (tempRacer->playerIndex != PLAYER_COMPUTER) {
        if (tempRacer->exitObj == 0) {
            if (is_race_started_by_player_two()) {
                controllerIndex = 1 - controllerIndex;
            }
            // Cap the joystick tilt and write the button inputs to the current buffer.
            gCurrentStickX = input_clamp_stick_x(controllerIndex);
            // Flip the steering for adventure 2.
            if (get_filtered_cheats() & CHEAT_MIRRORED_TRACKS) {
                gCurrentStickX = -gCurrentStickX;
            }
            gCurrentStickY = input_clamp_stick_y(controllerIndex);
            gCurrentRacerInput = input_held(controllerIndex);
            gCurrentButtonsPressed = input_pressed(controllerIndex);
            gCurrentButtonsReleased = input_released(controllerIndex);
        } else {
            racer_enter_door(tempRacer, updateRate);
        }
    } else {
        racer_AI_pathing_inputs(obj, tempRacer, updateRate);
    }
    // Set the value that decides whether to get an empowered boost.
    if (!(gCurrentRacerInput & A_BUTTON)) {
        tempRacer->throttleReleased = TRUE;
    }
}

/**
 * Clears all input during cutscenes, launches, approaches, bubble traps and when input is blocked.
 */
static void racer_block_input(Object_Racer *tempRacer) {
    if (check_if_showing_cutscene_camera() || gRaceStartTimer == 100 || tempRacer->launchedState ||
        gRacerInputBlocked || tempRacer->approachTarget || tempRacer->bubbleTrapTimer > 0) {
        gCurrentStickX = 0;
        gCurrentStickY = 0;
        gCurrentRacerInput = 0;
        gCurrentButtonsPressed = 0;
        gCurrentButtonsReleased = 0;
        tempRacer->steerAngle = 0;
    }
}

/**
 * Applies the bubble trap slowdown, the stun steering wobble and the magnet pull.
 */
static void racer_apply_status_effects(Object *obj, Object_Racer *tempRacer, s32 updateRate) {
    s32 prevWobblePhase;

    if (tempRacer->bubbleTrapTimer > 0) {
        tempRacer->bubbleTrapTimer -= updateRate;
        tempRacer->velocity *= 0.9f; //!@Delta
        obj->x_velocity *= 0.87f;    //!@Delta
        if (obj->y_velocity > 2.0f) {
            obj->y_velocity = 2.0f;
        }
        obj->z_velocity *= 0.87f; //!@Delta
    }
    if (tempRacer->stunTimer > 0) {
        tempRacer->steerWobbleTimer = tempRacer->stunTimer;
        tempRacer->stunTimer -= updateRate;
    }
    if (tempRacer->steerWobbleTimer > 0) {
        prevWobblePhase = tempRacer->steerWobbleTimer & 0xF;
        tempRacer->steerWobbleTimer -= updateRate;
        if (prevWobblePhase < (tempRacer->steerWobbleTimer & 0xF)) {
            tempRacer->steerJitter = rand_range(-80, 80);
        }
        gCurrentStickX += tempRacer->steerJitter;
    } else {
        tempRacer->steerWobbleTimer = 0;
    }
    if (tempRacer->magnetTimer) {
        racer_activate_magnet(obj, tempRacer, updateRate);
    }
}

/**
 * Zeroes out input before the race has begun, then advances the current lap time (kept under 10 minutes).
 */
static void racer_update_lap_timer(Object_Racer *tempRacer, LevelHeader *header, s32 updateRate) {
    // Zero out input before the race has begun.
    if (gRaceStartTimer && (header->race_type == RACETYPE_DEFAULT || header->race_type == RACETYPE_HORSESHOE_GULCH ||
                            header->race_type == RACETYPE_BOSS)) {
        gCurrentStickX = 0;
        gCurrentStickY = 0;
        gCurrentRacerInput = 0;
    }
    // Handle the race timer if it's currently active.
    if (gRaceStartTimer == 0 && header->laps > (tempRacer->countLap)) {
        // Keep it under 10 minutes.
        if (tempRacer->lap_times[tempRacer->countLap] < normalise_time(36000) - updateRate) {
            tempRacer->lap_times[tempRacer->countLap] += updateRate;
        } else {
            tempRacer->lap_times[tempRacer->countLap] = normalise_time(36000);
        }
    }
}

/**
 * Finds the waves under the racer, lowers the course height to any low water,
 * and updates the racer's buoyancy and underwater flag.
 */
static void racer_update_water(Object *obj, Object_Racer *tempRacer) {
    f32 waterHeight;
    s32 i;

    gRacerWaveCount =
        get_level_segment_waves(obj->segmentID, obj->trans.x_position, obj->trans.z_position, &gRacerCurrentWave);
    if (gRacerWaveCount) {
        for (i = 0; i < gRacerWaveCount; i++) {
            if (gRacerCurrentWave[i]->type == SURFACE_WATER_UNK_F) {
                if (gRacerCurrentWave[i]->waveHeight < gCurrentCourseHeight) {
                    gCurrentCourseHeight = gRacerCurrentWave[i]->waveHeight;
                }
            }
        }
    }
    if (tempRacer->vehicleID != VEHICLE_HOVERCRAFT) {
        waterHeight = -10000.0f;
        gRacerWaveType = get_wave_properties(obj->trans.y_position, &waterHeight, &gCurrentRacerWaterPos);
        if (gRacerWaveType != SURFACE_DEFAULT) {
            if (obj->trans.y_position - 5.0f < waterHeight) {
                tempRacer->waterTimer = 5;
                tempRacer->buoyancy = waterHeight - (obj->trans.y_position - 5.0f);
            } else {
                tempRacer->buoyancy = 0.0f;
            }
        } else {
            if (tempRacer->waterTimer > 0) {
                tempRacer->waterTimer--;
            } else {
                tempRacer->buoyancy = 0.0f;
            }
        }
        if (tempRacer->waterTimer > 0 && obj->trans.y_position < waterHeight + 5.0f) {
            obj->interactObj->flags |= INTERACT_FLAGS_UNK_0010;
        } else {
            obj->interactObj->flags &= ~INTERACT_FLAGS_UNK_0010;
        }
    }
}

/**
 * Runs the physics update for the racer's vehicle type.
 */
static void racer_update_vehicle(Object *obj, Object_Racer *tempRacer, s32 updateRate, f32 updateRateF) {
    set_collision_mode(COLLISION_MODE_DEFAULT);
    switch (tempRacer->vehicleID) {
        case VEHICLE_CAR:
            update_car(updateRate, updateRateF, obj, tempRacer);
            break;
        case VEHICLE_LOOPDELOOP:
            update_loop_de_loop(updateRate, updateRateF, obj, tempRacer);
            break;
        case VEHICLE_HOVERCRAFT:
            update_hovercraft(updateRate, updateRateF, obj, tempRacer);
            break;
        case VEHICLE_PLANE:
            update_plane(updateRate, updateRateF, obj, tempRacer);
            break;
        case VEHICLE_FLYING_CAR: /* fall through */
        case VEHICLE_CARPET:
            update_carpet(updateRate, updateRateF, obj, tempRacer);
            break;
        case VEHICLE_TRICKY:
            update_tricky(updateRate, updateRateF, obj, tempRacer, &gCurrentRacerInput, &gCurrentButtonsPressed,
                          &gRaceStartTimer);
            break;
        case VEHICLE_BLUEY:
            update_bluey(updateRate, updateRateF, obj, tempRacer, &gCurrentRacerInput, &gCurrentButtonsPressed,
                         &gRaceStartTimer);
            break;
        case VEHICLE_SMOKEY: /* fall through */
        case VEHICLE_PTERODACTYL:
            update_smokey(updateRate, updateRateF, obj, tempRacer, &gCurrentRacerInput, &gCurrentStickX,
                          &gRaceStartTimer);
            break;
        case VEHICLE_BUBBLER:
            update_bubbler(updateRate, updateRateF, obj, tempRacer, &gCurrentRacerInput, &gCurrentButtonsPressed,
                           &gRaceStartTimer);
            break;
        case VEHICLE_WIZPIG:
            update_wizpig(updateRate, updateRateF, obj, tempRacer, &gCurrentRacerInput, &gCurrentButtonsPressed,
                          &gRaceStartTimer);
            break;
        case VEHICLE_ROCKET:
            update_rocket(updateRate, updateRateF, obj, tempRacer, &gCurrentRacerInput, &gCurrentButtonsPressed,
                          &gRaceStartTimer);
            break;
    }
}

/**
 * Stops the magnet sound once the magnet has run out,
 * then caps the racer's speed to the level header's limit, if it has one.
 */
static void racer_stop_magnet_and_cap_speed(Object_Racer *tempRacer, LevelHeader *header) {
    f32 playerIDF;
    s32 playerID;

    if (tempRacer->magnetTimer == 0) {
        if (tempRacer->magnetSoundMask) {
            sndp_stop(tempRacer->magnetSoundMask);
            tempRacer->magnetSoundMask = NULL;
        }
    }
    playerID = header->playerIndex;
    if (playerID) {
        playerIDF = -(f32) playerID;
        CLAMP(tempRacer->velocity, playerIDF, playerID);
    }
}

/**
 * Spawns Taj's challenge arrow if needed and moves it onto the next checkpoint (human racers only).
 */
static void racer_place_challenge_marker(Object_Racer *tempRacer) {
    CheckpointNode *checkpointNode;
    struct LevelObjectEntryCommon newObject;

    if (gCurrentPlayerIndex != PLAYER_COMPUTER) {
        checkpointNode = find_next_checkpoint_node(tempRacer->nextCheckpoint, tempRacer->isOnAlternateRoute);
        if (!tempRacer->challengeMarker) {
            newObject.x = 0;
            newObject.y = 0;
            newObject.z = 0;
            newObject.objectID = ASSET_OBJECT_ID_CHECKARROW;
            newObject.size = sizeof(LevelObjectEntryCommon);
            tempRacer->challengeMarker = spawn_object(&newObject, OBJECT_SPAWN_UNK01);
            if (tempRacer->challengeMarker) {
                tempRacer->challengeMarker->level_entry = NULL;
                tempRacer->challengeMarker->opacity = 128;
            }
        }
        if (tempRacer->challengeMarker) {
            tempRacer->challengeMarker->trans.x_position = checkpointNode->obj->trans.x_position;
            tempRacer->challengeMarker->trans.y_position = checkpointNode->obj->trans.y_position;
            if (tempRacer->vehicleID == VEHICLE_CAR) {
                tempRacer->challengeMarker->trans.y_position -= 30.0;
            }
            tempRacer->challengeMarker->trans.z_position = checkpointNode->obj->trans.z_position;
            tempRacer->challengeMarker->trans.rotation.y_rotation = checkpointNode->obj->trans.rotation.y_rotation;
            tempRacer->challengeMarker->trans.rotation.x_rotation = checkpointNode->obj->trans.rotation.x_rotation;
            tempRacer->challengeMarker->segmentID = checkpointNode->obj->segmentID;
        }
    }
}

/**
 * Checks whether the racer passed its next checkpoint since its last position, and if so advances
 * the checkpoint, the lap and the course progress; otherwise records its distance rank.
 */
static void racer_update_checkpoints(Object *obj, Object_Racer *tempRacer, LevelHeader *header, f32 xTemp, f32 yTemp,
                                     f32 zTemp) {
    s32 passResult;
    s32 checkpointCount;
    f32 lastCheckpointDist;
    CheckpointNode *checkpointNode;

    lastCheckpointDist = tempRacer->checkpoint_distance;
    passResult = checkpoint_is_passed(tempRacer->nextCheckpoint, obj, xTemp, yTemp, zTemp,
                                      &tempRacer->checkpoint_distance, &tempRacer->isOnAlternateRoute);
    if (passResult == -100) {
        racer_update_progress(tempRacer);
    }
    checkpointNode = find_next_checkpoint_node(tempRacer->nextCheckpoint, tempRacer->isOnAlternateRoute);
    if (tempRacer->playerIndex == PLAYER_COMPUTER && checkpointNode->laneAction[tempRacer->aiLane] == 5 &&
        tempRacer->waterTimer) {
        tempRacer->isOnAlternateRoute = TRUE;
    }
    if (checkpointNode->laneAction[tempRacer->aiLane] == 6) {
        tempRacer->lap = header->laps + 1;
    }
    if (passResult == 0) {
        if (tempRacer->playerIndex == PLAYER_COMPUTER && checkpointNode->laneAction[tempRacer->aiLane] == 2) {
            tempRacer->isOnAlternateRoute = TRUE;
        }
        checkpointCount = get_checkpoint_count();
        tempRacer->nextCheckpoint++;
        if (tempRacer->nextCheckpoint >= checkpointCount) {
            tempRacer->nextCheckpoint = 0;
            if (tempRacer->courseCheckpoint > 0) {
                if (tempRacer->lap < 120) {
                    tempRacer->lap++;
                }
            }
            if (tempRacer->playerIndex != PLAYER_COMPUTER && tempRacer->lap + 1 == header->laps &&
                !gFinalLapAnnounced && level_type() == RACETYPE_DEFAULT) {
                music_tempo_set_relative(1.12f);
                gFinalLapAnnounced = 1;
            }
        }
        if (is_taj_challenge()) {
            racer_place_challenge_marker(tempRacer);
        }
        if (tempRacer->courseCheckpoint < (header->laps + 3) * checkpointCount) {
            tempRacer->courseCheckpoint++;
        }
        tempRacer->checkpointRank = 10000;
    } else {
        if (tempRacer->playerIndex == PLAYER_COMPUTER && lastCheckpointDist < tempRacer->checkpoint_distance) {
            tempRacer->checkpoint_distance = lastCheckpointDist;
        }
        tempRacer->checkpointRank = passResult;
    }
}

/**
 * In Taj's challenges, animates the challenge arrow and ends the race if the racer strays too far off the track.
 */
static void racer_update_taj_challenge(Object_Racer *tempRacer, s32 updateRate) {
    if (is_taj_challenge()) {
        if (tempRacer->challengeMarker) {
            tempRacer->challengeMarker->animFrame += 8 * updateRate;
        }
        if (tempRacer->trackOffsetX > 400 || tempRacer->trackOffsetX < -400) {
            mode_end_taj_race(CHALLENGE_END_OOB);
        }
    }
}

/**
 * Counts up the time since finishing the race, and drops any bananas the racer is due to lose.
 */
static void racer_update_finish_and_bananas(Object *obj, Object_Racer *tempRacer, s32 updateRate) {
    if (tempRacer->raceFinished == TRUE) {
        if (tempRacer->finishedTimer < 60) {
            tempRacer->finishedTimer += updateRate;
        }
    }
    if (tempRacer->pendingBananaDrop > 0) {
        drop_bananas(obj, tempRacer, tempRacer->pendingBananaDrop);
    }
}

/**
 * Stops the racer dead after an approach target was reached.
 */
static void racer_clear_approach_target(Object_Racer *tempRacer) {
    if (tempRacer->approachTarget) {
        tempRacer->approachTarget = NULL;
        tempRacer->velocity = 0.0f;
        tempRacer->lateral_velocity = 0.0f;
    }
}

/**
 * Eases the racer's squash and stretch height toward its cap, resetting the cap once it is reached.
 */
static void racer_update_stretch(Object_Racer *tempRacer, f32 updateRateF) {
    f32 stretch;

    if (tempRacer->stretch_height <= tempRacer->stretch_height_cap) {
        stretch = 0.02f;
    } else {
        stretch = -0.02f;
    }
    tempRacer->stretch_height +=
        (((tempRacer->stretch_height_cap - tempRacer->stretch_height) * 0.125) + stretch) * updateRateF;
    if ((stretch < 0.0f && tempRacer->stretch_height <= tempRacer->stretch_height_cap) ||
        (stretch > 0.0f && tempRacer->stretch_height >= tempRacer->stretch_height_cap)) {
        tempRacer->stretch_height = tempRacer->stretch_height_cap;
        tempRacer->stretch_height_cap = 1.0f;
    }
}

/**
 * Turns the character's head toward its target angle, then tracks double taps of the R button.
 */
static void racer_update_head_and_r_tap(Object_Racer *tempRacer, s32 updateRate) {
    s32 turnStep;

    turnStep = ((tempRacer->headAngleTarget - tempRacer->headAngle) * updateRate) >> 3;
    CLAMP(turnStep, -0x800, 0x800);
    tempRacer->headAngle += turnStep;
    if ((gCurrentButtonsPressed & R_TRIG) && tempRacer->tapTimerR) {
        tempRacer->tappedR = TRUE;
        tempRacer->tapTimerR = 0;
    } else if (gCurrentButtonsPressed & R_TRIG) {
        tempRacer->tapTimerR = 22;
    }
    if (tempRacer->tapTimerR > 0) {
        tempRacer->tapTimerR -= updateRate;
    } else {
        tempRacer->tapTimerR = 0;
    }
}

/**
 * Plays or stops the shield sound and counts down the shield, removing it when it runs out.
 */
static void racer_update_shield(Object *obj, Object_Racer *tempRacer, s32 updateRate) {
    if (tempRacer->shieldTimer > 0) {
        if (tempRacer->shieldTimer > 60) {
            if (tempRacer->shieldSoundMask) {
                audspat_point_set_position(tempRacer->shieldSoundMask, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position);
            } else if (tempRacer->vehicleSound) {
                audspat_play_sound_at_position(SOUND_SHIELD, obj->trans.x_position, obj->trans.y_position,
                                               obj->trans.z_position, AUDIO_POINT_FLAG_1, &tempRacer->shieldSoundMask);
            }
        } else if (tempRacer->shieldSoundMask) {
            audspat_point_stop(tempRacer->shieldSoundMask);
            tempRacer->shieldSoundMask = NULL;
        }
        tempRacer->shieldTimer -= updateRate;
        if (tempRacer->shieldTimer <= 0) {
            tempRacer->shieldType = SHIELD_NONE;
        }
    }
}

/**
 * Moves the banana and racer sounds to the racer, and records the time trial ghost.
 */
static void racer_update_sounds_and_ghost(Object *obj, Object_Racer *tempRacer, s32 updateRate) {
    if (tempRacer->bananaSoundMask) {
        audspat_point_set_position(tempRacer->bananaSoundMask, obj->trans.x_position, obj->trans.y_position,
                                   obj->trans.z_position);
    }
    if (is_in_time_trial() && tempRacer->playerIndex == PLAYER_ONE && gRaceStartTimer == 0) {
        timetrial_ghost_write(obj, updateRate);
    }
    if (tempRacer->soundMask) {
        audspat_point_set_position(tempRacer->soundMask, obj->trans.x_position, obj->trans.y_position,
                                   obj->trans.z_position);
    }
}

/**
 * Places the race position arrow above the racer, hiding it when close to the camera, then lets it go.
 */
static void racer_place_position_arrow(Object *obj, Object_Racer *tempRacer) {
    Object *tempObj;

    if (tempRacer->posArrowObj && gRaceStartTimer == 0) {
        s8 *yAsset;
        tempRacer->posArrowObj->trans.x_position = obj->trans.x_position;
        yAsset = (s8 *) get_misc_asset(ASSET_MISC_0);

        tempRacer->posArrowObj->trans.y_position = obj->trans.y_position + yAsset[tempRacer->characterId];
        tempRacer->posArrowObj->trans.z_position = obj->trans.z_position;
        tempRacer->posArrowObj->trans.scale = obj->distanceToCamera / 265.0f;
        if (obj->distanceToCamera < 1500.0 || get_filtered_cheats() & CHEAT_MIRRORED_TRACKS) {
            tempObj = tempRacer->posArrowObj;
            tempObj->trans.flags |= OBJ_FLAGS_INVISIBLE;
        }
        if (tempRacer->posArrowObj->trans.scale < 1.0) {
            tempRacer->posArrowObj->trans.scale = 1.0f;
        }
        tempRacer->posArrowObj = NULL;
    }
}

/**
 * Counts down the delayed sound and plays it at the racer when the delay runs out.
 */
static void racer_play_delayed_sound(Object *obj, Object_Racer *tempRacer, s32 updateRate) {
    if (tempRacer->delaySoundID) {
        if (tempRacer->delaySoundTimer > updateRate) {
            tempRacer->delaySoundTimer -= updateRate;
        } else {
            tempRacer->delaySoundTimer = 0;
            if (tempRacer->playerIndex == PLAYER_COMPUTER) {
                audspat_play_sound_at_position(tempRacer->delaySoundID, obj->trans.x_position, obj->trans.y_position,
                                               obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
            } else {
                sound_play_spatial(tempRacer->delaySoundID, obj->trans.x_position, obj->trans.y_position,
                                   obj->trans.z_position, NULL);
            }
            tempRacer->delaySoundID = SOUND_NONE;
        }
    }
}

/**
 * Sets the racer's elevation for challenge maps, and brings the counted lap up to the current lap.
 */
static void racer_update_elevation_and_lap_count(Object *obj, Object_Racer *tempRacer, LevelHeader *header) {
    if (header->race_type & RACETYPE_CHALLENGE && header->race_type != RACETYPE_CHALLENGE_EGGS) {
        tempRacer->elevation = obj_elevation(obj->trans.y_position);
    } else {
        tempRacer->elevation = ELEVATION_LOW;
    }
    if (tempRacer->countLap < tempRacer->lap) {
        tempRacer->countLap = tempRacer->lap;
    }
}

/**
 * Main function for handling everything related to the player controlled racer object.
 * Branches off into a different function if the player becomes computer controlled. (Finishing a race)
 */
void update_player_racer(Object *obj, s32 updateRate) {
    s32 context;
    f32 updateRateF;
    Object_Racer *tempRacer;
    f32 xTemp;
    f32 yTemp;
    f32 zTemp;
    LevelHeader *header;

    gNumViewports = cam_get_viewport_layout() + 1;
    gCurrentSurfaceType = SURFACE_DEFAULT;
    gRaceStartTimer = get_race_countdown();
    updateRateF = updateRate;
    tempRacer = obj->racer;
    racer_clamp_velocities(obj, tempRacer);
    if (tempRacer->effectBoxType == 1) {
        tempRacer->launchedState = 0;
    }
    // PAL moves 20% faster.
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    racer_tick_common_timers(obj, tempRacer, updateRate);
    header = level_header();
    gCurrentCourseHeight = header->course_height;
    tempRacer->throttleReleased = FALSE;
    if (tempRacer->playerIndex == PLAYER_COMPUTER) {
        update_AI_racer(obj, tempRacer, updateRate, updateRateF);
    } else {
        racer_print_debug_coords(obj, tempRacer);
        if (is_taj_challenge()) {
            gDialogueCameraAngle = 0;
        }
        context = get_game_mode();
        racer_tick_start_timers(tempRacer, updateRate, updateRateF);
        racer_load_stats(tempRacer);
        racer_apply_effect_box(tempRacer, updateRateF);
        racer_load_vehicle_tables(obj);

        if (obj->y_velocity < 4.0 && (tempRacer->groundedWheels > 2 || tempRacer->buoyancy != 0.0)) {
            tempRacer->launchedState = 0;
        }
        xTemp = obj->trans.x_position;
        yTemp = obj->trans.y_position;
        zTemp = obj->trans.z_position;
        racer_tick_position_change_timer(tempRacer, updateRate);
        racer_select_camera(tempRacer);
        racer_hand_over_to_ai(tempRacer, context);
        racer_read_input(obj, tempRacer, updateRate);
        racer_block_input(tempRacer);
        racer_apply_status_effects(obj, tempRacer, updateRate);
        racer_update_lap_timer(tempRacer, header, updateRate);
        // Assign a camera to human players.
        if (gCurrentPlayerIndex != PLAYER_COMPUTER) {
            gCameraObject = cam_get_active_camera_no_cutscenes();
        }
        racer_update_water(obj, tempRacer);
        racer_update_vehicle(obj, tempRacer, updateRate, updateRateF);
        racer_stop_magnet_and_cap_speed(tempRacer, header);
        if (context != GAMEMODE_MENU || racetype_demo()) {
            racer_sound_update(obj, gCurrentButtonsPressed, gCurrentRacerInput, updateRate);
        }
        racer_update_checkpoints(obj, tempRacer, header, xTemp, yTemp, zTemp);
        racer_update_taj_challenge(tempRacer, updateRate);
        update_midi_fade_points(obj, xTemp, yTemp, zTemp, updateRate);
        racer_update_checkpoint_progress(obj, tempRacer, updateRate);
        racer_update_finish_and_bananas(obj, tempRacer, updateRate);
        tempRacer->playerIndex = gCurrentPlayerIndex;
        update_player_camera(obj, tempRacer, updateRateF);
        gRacerDialogueCamera = FALSE;
        racer_clear_approach_target(tempRacer);
        racer_update_stretch(tempRacer, updateRateF);
        racer_update_head_and_r_tap(tempRacer, updateRate);
        racer_update_shield(obj, tempRacer, updateRate);
        racer_update_sounds_and_ghost(obj, tempRacer, updateRate);
        gRacerInputBlocked = 0;
        racer_place_position_arrow(obj, tempRacer);
        tempRacer->effectBoxType = -1;
        set_racer_tail_lights(tempRacer);
        racer_play_delayed_sound(obj, tempRacer, updateRate);
        racer_update_elevation_and_lap_count(obj, tempRacer, header);
    }
}

/**
 * Checks if Taj's interaction status is set to try make him teleport and return true.
 */
s32 should_taj_teleport(void) {
    if (gTajInteractStatus == TAJ_TELEPORT) {
        gTajInteractStatus = TAJ_DIALOGUE;
        return TRUE;
    }
    return FALSE;
}

/**
 * Sets Taj's interaction status.
 */
void set_taj_status(TajInteraction status) {
    gTajInteractStatus = status;
}

/**
 * Find a nearby racer and set the head to turn to face them.
 * Otherwise, if stationary, set a random angle to turn to periodically.
 */
void handle_racer_head_turning(Object *obj, Object_Racer *racer, UNUSED s32 updateRate) {
    Object *tempObj;
    s8 foundObj;
    f32 distance;
    s32 intendedAngle;

    foundObj = FALSE;
    if (boss_race_number()) {
        tempObj = get_racer_object(1);
        foundObj = turn_head_towards_object(obj, racer, tempObj, 400.0f * 400.0f);
    }
    if (foundObj == FALSE) {
        tempObj = racer_find_nearest_opponent_relative(racer, 1, &distance);
        if (tempObj && !gRaceStartTimer) {
            foundObj = turn_head_towards_object(obj, racer, tempObj, 400.0f * 400.0f);
        }
    }
    if (foundObj == FALSE) {
        tempObj = racer_find_nearest_opponent_relative(racer, -1, &distance);
        if (tempObj && !gRaceStartTimer) {
            foundObj = turn_head_towards_object(obj, racer, tempObj, 30000.0f);
        }
    }
    if (foundObj == FALSE) {
        if ((racer->miscAnimCounter & 0x1F) < 2) {
            intendedAngle = racer->velocity * 1280.0f;

            if (intendedAngle < 0) {
                intendedAngle = -intendedAngle;
            }

            if (intendedAngle > 0x2800) {
                intendedAngle = 0x2800;
            }

            intendedAngle = 0x2800 - intendedAngle;

            racer->headAngleTarget = rand_range(-intendedAngle, intendedAngle);
        }
    }
}

/**
 * Over time, set the head angle target to zero.
 * The character's head will start to face forward.
 */
void slowly_reset_head_angle(Object_Racer *racer) {
    racer->headAngleTarget -= racer->headAngleTarget >> 3; //!@Delta
    if (racer->headAngleTarget > -10 && racer->headAngleTarget < 10) {
        racer->headAngleTarget = 0;
    }
}

/**
 * Compute angle between objects, then set the intended angle for the character's head to face
 * an object.
 */
s32 turn_head_towards_object(Object *obj, Object_Racer *racer, Object *targetObj, f32 distance) {
    s32 intendedAngle;
    f32 diffX;
    f32 diffZ;
    s32 ret;

    ret = FALSE;
    diffX = targetObj->trans.x_position - obj->trans.x_position;
    diffZ = targetObj->trans.z_position - obj->trans.z_position;
    if ((diffX * diffX) + (diffZ * diffZ) < distance) {
        intendedAngle = (arctan2_f(diffX, diffZ) - (obj->trans.rotation.y_rotation & 0xFFFF)) + 0x8000;
        WRAP(intendedAngle, -0x8000, 0x8000);
        CLAMP(intendedAngle, -0x3000, 0x3000);
        racer->headAngleTarget = intendedAngle;
        if ((racer->miscAnimCounter & 0x3F) <= 30) {
            racer->headAngleTarget = 0;
        }
        racer = targetObj->racer;
        intendedAngle = arctan2_f(diffX, diffZ) - (obj->trans.rotation.y_rotation & 0xFFFF);
        WRAP(intendedAngle, -0x8000, 0x8000);
        CLAMP(intendedAngle, -0x3000, 0x3000);
        racer->headAngleTarget = intendedAngle;
        if (ret) {} // Fakematch
        ret = TRUE;
        if ((racer->miscAnimCounter & 0x1F) < 10) {
            racer->headAngleTarget = 0;
        }
    }
    return ret;
}

/**
 * Places the four collision spheres from the wheel table in world space around the racer,
 * with their radii, and marks their surfaces unknown.
 */
static void racer_collision_place_wheels(Object *racerObj, f32 *wheelPos, f32 *wheelRadius, s8 *wheelSurface) {
    s32 i;
    MtxF mtx;

    gCurrentRacerTransform.rotation.y_rotation = racerObj->trans.rotation.y_rotation;
    gCurrentRacerTransform.rotation.x_rotation = racerObj->trans.rotation.x_rotation;
    gCurrentRacerTransform.rotation.z_rotation = racerObj->trans.rotation.z_rotation;
    gCurrentRacerTransform.scale = 1.0f;
    gCurrentRacerTransform.x_position = racerObj->trans.x_position;
    gCurrentRacerTransform.y_position = racerObj->trans.y_position;
    gCurrentRacerTransform.z_position = racerObj->trans.z_position;
    mtxf_from_transform(&mtx, &gCurrentRacerTransform);

    for (i = 0; i < 4; i++) {
        mtxf_transform_point(mtx, gRacerWheelTable[i * 4 + 0], gRacerWheelTable[i * 4 + 1], gRacerWheelTable[i * 4 + 2],
                             &wheelPos[i * 3 + 0], &wheelPos[i * 3 + 1], &wheelPos[i * 3 + 2]);
        wheelRadius[i] = gRacerWheelTable[i * 4 + 3];
        wheelSurface[i] = -1;
    }
}

/**
 * Hitting a wall head on (or at any angle in a hovercraft): rumbles, and above a speed bumps the
 * racer back off the wall's normal with a crash sound, voice and camera shake.
 */
static void racer_collision_wall_crash(Object *racerObj, Object_Racer *racer, f32 normalX, f32 normalZ) {
    s16 wallAngle;
    f32 impactSpeed;

    wallAngle = (u16) arctan2_f(normalX * 255.0f, normalZ * 255.0f);
    wallAngle -= racerObj->trans.rotation.y_rotation;
    if ((wallAngle < 0x2000 && wallAngle > -0x2000) || racer->vehicleID == VEHICLE_HOVERCRAFT) {
        impactSpeed = racer->velocity * 1.075;
        if (impactSpeed < 0) {
            impactSpeed = -impactSpeed;
        }
        if (impactSpeed > 1.0) {
            rumble_set_fade(racer->playerIndex, 18, impactSpeed * 0.125);
        }
        if (impactSpeed > 4.0) {
            racer->driveAnimFlags |= 8;
            if (gCurrentPlayerIndex != PLAYER_COMPUTER) {
                gCameraObject->shakeMagnitude = 3.0f;
            }
            racer->bumpTimer = 7;
            if (racer->playerIndex != PLAYER_COMPUTER) {
                play_random_character_voice(racerObj, SOUND_VOICE_CHARACTER_NEGATIVE, 8, 0x80 | 0x2);
                racer_play_sound(racerObj, SOUND_CRASH);
            }
            racer->boost_sound |= BOOST_SOUND_UNK4;
            if (racer->vehicleID == VEHICLE_HOVERCRAFT) {
                impactSpeed *= 0.5;
            }
            racer->bumpVelX = normalX * impactSpeed;
            racer->bumpVelZ = normalZ * impactSpeed;
            racer->velocity *= 0.15;
            racer->lateral_velocity = 0;
        }
    }
}

/**
 * Squashed under an object with three or more wheels on the ground: splat sound, and the
 * squish attack (or the squish lasts longer).
 */
static void racer_collision_squish(Object_Racer *racer, s8 underObject, s32 numCollisions) {
    if (underObject && numCollisions >= 3) {
        if (racer->playerIndex != PLAYER_COMPUTER && racer->splatSoundHandle == 0 && racer->shieldTimer <= 0) {
            sound_play(SOUND_SPLAT, &racer->splatSoundHandle);
        }
        if (racer->squish_timer == 0) {
            racer->attackType = ATTACK_SQUISHED;
        } else {
            racer->squish_timer = 60;
        }
    }
}

/**
 * Stores the wheel contacts, counts the grounded wheels, keeps the resolved sphere positions
 * and surfaces, and moves the racer to their average.
 */
static void racer_collision_store_wheels(Object *racerObj, Object_Racer *racer, s32 flags, f32 *wheelPos,
                                         s8 *wheelSurface) {
    s32 i;
    s32 j;

    racer->objectContactFlags = flags;
    racer->wheelContactFlags |= flags;

    racer->groundedWheels = 0;
    flags = 1;
    for (j = 0; j < 4; j++) {
        if (racer->wheelContactFlags & flags) {
            racer->groundedWheels++;
        }
        flags <<= 1;
    }

    for (i = 0; i < 12; i++) {
        racer->wheelCollisionPos[i] = wheelPos[i];
    }
    for (i = 0; i < 4; i++) {
        racer->wheel_surfaces[i] = wheelSurface[i];
    }
    racerObj->trans.x_position = 0;
    racerObj->trans.y_position = 0;
    racerObj->trans.z_position = 0;
    for (i = 0; i < 12; i += 3) {
        racerObj->trans.x_position = racerObj->trans.x_position + racer->wheelCollisionPos[i + 0];
        racerObj->trans.y_position = racerObj->trans.y_position + racer->wheelCollisionPos[i + 1];
        racerObj->trans.z_position = racerObj->trans.z_position + racer->wheelCollisionPos[i + 2];
    }
    racerObj->trans.x_position /= 4;
    racerObj->trans.y_position /= 4;
    racerObj->trans.z_position /= 4;
}

/**
 * Brings the resolved sphere positions into the racer's local space.
 */
static void racer_collision_wheels_to_local(Object *racerObj, Object_Racer *racer, f32 *localX, f32 *localY,
                                            f32 *localZ) {
    s32 i;
    MtxF invMtx;

    gCurrentRacerTransform.rotation.y_rotation = -racerObj->trans.rotation.y_rotation;
    gCurrentRacerTransform.rotation.x_rotation = -racerObj->trans.rotation.x_rotation;
    gCurrentRacerTransform.rotation.z_rotation = -racerObj->trans.rotation.z_rotation;
    gCurrentRacerTransform.scale = 1.0f;
    gCurrentRacerTransform.x_position = -racerObj->trans.x_position;
    gCurrentRacerTransform.y_position = -racerObj->trans.y_position;
    gCurrentRacerTransform.z_position = -racerObj->trans.z_position;
    mtxf_from_inverse_transform(&invMtx, &gCurrentRacerTransform);
    for (i = 0; i < 4; i++) {
        mtxf_transform_point(invMtx, racer->wheelCollisionPos[3 * i], racer->wheelCollisionPos[3 * i + 1],
                             racer->wheelCollisionPos[3 * i + 2], &localX[i], &localY[i], &localZ[i]);
    }
}

/**
 * Turns the racer to line up with where the spheres were pushed (or by the fixed bump tilt
 * after a crash), damping drift and spin when the turn is large.
 */
static void racer_collision_align_yaw(Object *racerObj, Object_Racer *racer, f32 *localX, f32 *localZ,
                                      s16 movedByObject) {
    f32 frontX;
    f32 frontZ;
    f32 rearX;
    f32 rearZ;
    s32 yawAngle;
    s16 yawDiff;

    frontX = localX[0] + localX[1];
    frontZ = localZ[0] + localZ[1];
    rearX = localX[2] + localX[3];
    rearZ = localZ[2] + localZ[3];
    yawAngle = (arctan2_f(frontX - rearX, frontZ - rearZ) + 0x8000) & 0xFFFF;
    yawDiff = yawAngle;
    if (racer->bumpTimer == 7) {
        if (yawDiff > 0) {
            racer->bumpTiltAngle = 2048;
        }
        if (yawDiff < 0) {
            racer->bumpTiltAngle = -2048;
        }
    }
    if (racer->bumpTimer != 0) {
        yawDiff = racer->bumpTiltAngle;
    }
    if (yawDiff > 500 || yawDiff < -500) {
        racer->drift_direction = 0;
        racer->driftHeadingOffset = (s32) (racer->driftHeadingOffset * 7) >> 3;
        racer->y_rotation_vel = (s16) ((s32) (racer->y_rotation_vel * 7) >> 3);
    }
    if (movedByObject) {
        yawDiff *= 4;
    }
    racerObj->trans.rotation.y_rotation += yawDiff >> 2;
    racer->steerVisualRotation = racerObj->trans.rotation.y_rotation - racer->y_rotation_vel;
}

/**
 * Unless a wall was hit, pitches and rolls the racer to match the spheres, then (for ground
 * vehicles) limits both.
 */
static void racer_collision_pitch_and_roll(Object *racerObj, Object_Racer *racer, f32 *localX, f32 *localY, f32 *localZ,
                                           s32 hitWall) {
    f32 diffA;
    f32 diffB;
    s16 angle;

    diffA = localZ[2] - localZ[0];
    diffB = localY[2] - localY[0];
    angle = arctan2_f(diffA, diffB) & 0xFFFF;
    angle -= 0x4000;
    if (hitWall == 0) {
        racerObj->trans.rotation.x_rotation += angle;
    }
    diffA = (localX[1] - localX[0]);
    diffB = (localY[1] - localY[0]);
    angle = (0x4000 - (arctan2_f(diffA, diffB)));
    if (hitWall == 0) {
        racer->x_rotation_vel += angle & 0xFFFF & 0xFFFF & 0xFFFF;
    }
    if (racer->vehicleID != VEHICLE_LOOPDELOOP && racer->vehicleID != VEHICLE_PLANE &&
        racer->vehicleID != VEHICLE_CARPET && racer->vehicleID != VEHICLE_SMOKEY) {
        CLAMP(racer->x_rotation_vel, -0x3400, 0x3400);
        CLAMP(racerObj->trans.rotation.x_rotation, -0x3400, 0x3400);
    }
}

/**
 * Resolves the racer's four collision spheres (from the misc asset in the object header) against the level and
 * objects, then updates the grounded wheel count and wheel surfaces, sets the position to the average of the spheres
 * and tilts the racer to match them. Also handles wall crashes and getting squished.
 */
void update_racer_collision(Object *racerObj, Object_Racer *racer, s32 updateRate) {
    s32 numCollisions;
    s32 flags;
    s32 hitWall;
    f32 normalX;
    f32 normalZ;
    f32 normalY;
    f32 averageY;
    s32 i;
    f32 wheelPos[4 * 3]; // 4 sets of 3 floats. (4 Vec3fs don't seem to match?)
    s16 movedByObject;
    f32 localX[5];
    f32 localY[5];
    f32 localZ[5];
    f32 wheelRadius[5];
    s8 underObject;
    s8 wheelSurface[4];

    if (gCurrentCourseHeight < racerObj->trans.y_position) {
        racerObj->trans.y_position = gCurrentCourseHeight;
        set_collision_mode(COLLISION_MODE_1);
    }
    movedByObject = FALSE;
    racer_collision_place_wheels(racerObj, wheelPos, wheelRadius, wheelSurface);

    numCollisions = 0;
    gRacerExternalShiftX = 0;
    gRacerExternalShiftZ = 0;
    flags = 0;
    if (racer->playerIndex != PLAYER_COMPUTER || racer->vehicleIDPrev < VEHICLE_BOSSES) {
        flags = collision_objectmodel(racerObj, 4, &numCollisions, (Vec3f *) racer->wheelCollisionPos, wheelPos,
                                      wheelRadius, wheelSurface);
    }
    if (flags & 0x80) {
        for (i = 0; i < 4; i++) {
            gRacerExternalShiftX = gRacerExternalShiftX + wheelPos[i * 3 + 0];
            gRacerExternalShiftZ = gRacerExternalShiftZ + wheelPos[i * 3 + 2];
        }
        gRacerExternalShiftX = gRacerExternalShiftX / 4;
        gRacerExternalShiftZ = gRacerExternalShiftZ / 4;
        gRacerExternalShiftX = gRacerExternalShiftX - racerObj->trans.x_position;
        gRacerExternalShiftZ = gRacerExternalShiftZ - racerObj->trans.z_position;
        movedByObject = TRUE;
        flags &= ~0x80;
    }
    underObject = 0;
    if (flags) {
        averageY = 0;
        for (i = 0; i < 4; i++) {
            averageY += wheelPos[i * 3 + 1];
        }
        averageY *= 0.25;
        if (averageY < (racerObj->trans.y_position - 4.0)) {
            underObject = 1;
        }
    }
    generate_collision_candidates(4, (Vec3f *) racer->wheelCollisionPos, (Vec3f *) wheelPos, racer->vehicleID);
    numCollisions = 0;
    racer->wheelContactFlags = resolve_collisions((Vec3f *) racer->wheelCollisionPos, (Vec3f *) wheelPos, wheelRadius,
                                                  wheelSurface, 4, &numCollisions);
    hitWall = get_collision_normal(&normalX, &normalY, &normalZ);
    if (hitWall != 0) {
        racer_collision_wall_crash(racerObj, racer, normalX, normalZ);
    }
    racer_collision_squish(racer, underObject, numCollisions);
    racer_collision_store_wheels(racerObj, racer, flags, wheelPos, wheelSurface);
    racer_collision_wheels_to_local(racerObj, racer, localX, localY, localZ);
    if (racer->vehicleID != VEHICLE_LOOPDELOOP) {
        racer_collision_align_yaw(racerObj, racer, localX, localZ, movedByObject);
    }
    racer_collision_pitch_and_roll(racerObj, racer, localX, localY, localZ, hitWall);
}

/**
 * Generate the steer velocity by taking the current steer velocity and getting the difference between the stick tilt.
 * Get the velocity from a fraction of the difference.
 */
void handle_base_steering(Object_Racer *racer, UNUSED s32 updateRate, f32 updateRateF) {
    s32 steering;
    s32 turnVel;

    steering = gCurrentStickX - racer->steerAngle;
    turnVel = (steering * updateRateF) * 0.125;

    if (steering != 0 && turnVel == 0) {
        if (steering > 0) {
            turnVel = 1;
        }
        if (steering < 0) {
            turnVel = -1;
        }
    }

    racer->steerAngle += turnVel;
}

/**
 * Tracks where the racer is along the checkpoint spline: advances or rewinds
 * the next checkpoint, lap and course checkpoint as it crosses them.
 */
void racer_update_checkpoint_progress(Object *obj, Object_Racer *racer, s32 updateRate) {
    f32 ftemp;
    s32 checkpointCount;
    CheckpointNode *tempCheckpointNode;
    f32 posX[5];
    f32 posY[5];
    f32 posZ[5];
    f32 unusedFtemp;
    f32 tempX;
    s32 counter;
    f32 tempY;
    s32 angle;
    f32 tempZ;
    f32 splinePos;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    f32 distance;
    f32 divisor;
    f32 scale;
    s32 splineIndex;
    s32 i;

    checkpointCount = get_checkpoint_count();
    if (checkpointCount == 0) {
        return;
    }
    if ((level_id() == 0) && (racer->nextCheckpoint >= checkpointCount)) {
        racer->lap = 0;
        racer->nextCheckpoint = 0;
        racer->courseCheckpoint = 0;
    }
    splinePos = 1.0 - racer->checkpoint_distance;
    if (splinePos < -0.2) {
        racer->nextCheckpoint--;
        if (racer->nextCheckpoint < 0) {
            racer->nextCheckpoint += checkpointCount;
            if (racer->lap > 0) {
                racer->lap--;
            }
        }
        if (racer->courseCheckpoint > -32000) {
            racer->courseCheckpoint--;
        }
        if (racer->isOnAlternateRoute) {
            tempCheckpointNode = get_checkpoint_node(racer->nextCheckpoint);
            if (tempCheckpointNode->altRouteID == -1) {
                racer->isOnAlternateRoute = FALSE;
            }
            angle = racer->nextCheckpoint - 1;
            if (angle < 0) {
                angle += checkpointCount;
            }
            tempCheckpointNode = get_checkpoint_node(angle);
            if (tempCheckpointNode->altRouteID == -1) {
                racer->isOnAlternateRoute = FALSE;
            }
        }
        return;
    }
    if (splinePos < 0.0f) {
        splinePos = 0.0f;
    }
    tempCheckpointNode = find_next_checkpoint_node(racer->nextCheckpoint, racer->isOnAlternateRoute);
    scale = tempCheckpointNode->scale;
    counter = racer->nextCheckpoint - 1;
    if (counter < 0) {
        counter = checkpointCount - 1;
    }
    tempCheckpointNode = get_checkpoint_node(counter);
    distance = tempCheckpointNode->scale;
    divisor = ((scale - tempCheckpointNode->scale) * splinePos) + distance;
    counter = racer->nextCheckpoint - 2;
    if (counter < 0) {
        counter += checkpointCount;
    }
    for (i = 0; (i < 5) ^ 0; i++) {
        tempCheckpointNode = find_next_checkpoint_node(counter, racer->isOnAlternateRoute);
        posX[i] = tempCheckpointNode->x +
                  ((tempCheckpointNode->scale * tempCheckpointNode->rotationZFrac) * racer->trackOffsetX);
        posY[i] = tempCheckpointNode->y + (tempCheckpointNode->scale * racer->trackOffsetY);
        posZ[i] = tempCheckpointNode->z +
                  ((tempCheckpointNode->scale * (-tempCheckpointNode->rotationXFrac)) * racer->trackOffsetX);
        counter++;
        if (counter == checkpointCount) {
            // @fake
            if (1) {
                counter = 0;
            }
        }
    }
    splineIndex = FALSE;
    if (splinePos >= 1.0) {
        splinePos -= 1.0;
        splineIndex = TRUE;
    }
    tempX = cubic_spline_interpolation(posX, splineIndex, splinePos, &diffX);
    tempY = cubic_spline_interpolation(posY, splineIndex, splinePos, &diffY);
    tempZ = cubic_spline_interpolation(posZ, splineIndex, splinePos, &diffZ);
    distance = sqrtf((diffX * diffX) + (diffZ * diffZ));
    if (distance != 0.0f) {
        scale = 1.0f / distance;
        diffX *= scale;
        diffZ *= scale;
    }
    angle = arctan2_f(diffX, diffZ) - (racer->steerVisualRotation & 0xFFFF) - 0x8000;
    WRAP(angle, -0x8000, 0x8000);
    if ((angle > 0x4000 || angle < -0x4000)
#if VERSION >= VERSION_79
        && racer->raceFinished == FALSE
#endif
    ) {
        if (racer->wrongWayCounter < 200 && racer->velocity <= -1.0) {
            racer->wrongWayCounter += updateRate;
        }
    } else {
        racer->wrongWayCounter = 0;
    }

    diffY = diffX;
    diffX = diffZ;
    diffZ = -diffY;

    ftemp = (tempZ * diffZ) + (diffX * tempX);
    ftemp = -ftemp;

    diffX = -(((obj->trans.x_position * diffX) + (diffZ * obj->trans.z_position) + ftemp) / divisor);
    unusedFtemp = ftemp;
    CLAMP(diffX, -5.0f, 5.0f)
    racer->trackOffsetX += (s32) diffX;

    diffY = (obj->trans.y_position - tempY) / divisor;
    CLAMP(diffY, -100.0f, 100.0f)
    racer->trackOffsetY += (s32) diffY;
}

/**
 * Blocks the player's movement until the end of their update cycle.
 * Called for every frame the player shouldn't be able to drive.
 */
void disable_racer_input(void) {
    gRacerInputBlocked = TRUE;
}

/**
 * Tells the camera to start rotating to the side for the dialogue camera.
 * Called for every frame the player should experience subtle cinematography.
 */
void racer_set_dialogue_camera(void) {
    gRacerDialogueCamera = TRUE;
}

/**
 * When you enter a door, take control away from the player.
 * Get the angle to the door object, and drive towards it.
 * After a timer hits 0, execute the transition.
 */
void racer_enter_door(Object_Racer *racer, s32 updateRate) {
    Object_Exit *exit;
    f32 updateRateF;
    s32 angle;

    exit = racer->exitObj->exit;
    racer->playerIndex = PLAYER_COMPUTER;
    angle = (u16) arctan2_f(exit->directionX, exit->directionZ) - (racer->steerVisualRotation & 0xFFFF);
    WRAP(angle, -0x8000, 0x8000);
    angle = -angle >> 5;
    gCurrentStickX = angle;
    gCurrentButtonsPressed = 0;
    gCurrentButtonsReleased = 0;
    gCurrentRacerInput = 0;
    gCurrentStickY = 0;
    if (racer->velocity > -4.0) {
        gCurrentRacerInput |= A_BUTTON;
    } else if (racer->velocity < -5.0) {
        gCurrentRacerInput |= B_BUTTON;
    }
    updateRateF = (f32) updateRate;
    gCameraObject->trans.x_position += (exit->directionX * updateRateF) * 1.5;
    gCameraObject->trans.z_position += (exit->directionZ * updateRateF) * 1.5;
    // clang-format off
    // Only matches if it's on the same line
    if (gCurrentStickX > 75) { \
        gCurrentStickX = 75; \
        gCurrentRacerInput |= A_BUTTON | B_BUTTON; \
    }
    // clang-format on
    if (gCurrentStickX < -75) {
        gCurrentStickX = -75;
        gCurrentRacerInput |= A_BUTTON | B_BUTTON;
    }
    if (racer->transitionTimer < -1) {
        racer->transitionTimer += updateRate;
        if (racer->transitionTimer >= 0) {
            racer->transitionTimer = -1;
        }
    }
    if ((racer->transitionTimer < -1 && gCurrentStickX < 10 && gCurrentStickX > -10) || racer->transitionTimer == -1) {
        if (check_if_showing_cutscene_camera() == 0) {
            transition_begin(&gDoorFadeTransition);
        }
        racer->transitionTimer = 60 - updateRate;
    }
    set_pause_lockout_timer(1);
    if (racer->transitionTimer > 0) {
        racer->transitionTimer -= updateRate;
        if (racer->transitionTimer <= 0) {
            level_settings_store_from_exit((s8 *) racer->exitObj->level_entry);
            racer->transitionTimer = 0;
        }
    }
}

/**
 * A racer going away takes its sounds with it, and every weapon and fly coin in the level,
 * and clears the parent link of particles it spawned.
 */
void obj_destroy_racer(Object *obj, UNUSED s32 keepAnimTarget) {
    Object_Racer *racer;
    s32 j;

    butterfly_forget_target(obj);
    racer = obj->racer;
    if (racer->surfaceSoundHandle != NULL) {
        sndp_stop((SoundHandle) (uintptr_t) racer->surfaceSoundHandle);
    }
    if (racer->slideSoundHandle != NULL) {
        sndp_stop((SoundHandle) (uintptr_t) racer->slideSoundHandle);
    }
    if (racer->unusedSoundHandle14 != NULL) {
        sndp_stop((SoundHandle) (uintptr_t) racer->unusedSoundHandle14);
    }
    if (racer->unusedSoundHandle1C != NULL) {
        sndp_stop((SoundHandle) (uintptr_t) racer->unusedSoundHandle1C);
    }
    if (racer->splatSoundHandle != NULL) {
        sndp_stop((SoundHandle) (uintptr_t) racer->splatSoundHandle);
    }
    if (racer->soundMask != NULL) {
        audspat_point_stop(racer->soundMask);
    }
    if (racer->shieldSoundMask != NULL) {
        audspat_point_stop(racer->shieldSoundMask);
    }
    if (racer->magnetSoundMask != NULL) {
        sndp_stop(racer->magnetSoundMask);
    }
    racer_sound_free(obj);
    for (j = 0; j < gObjectCount; j++) {
        if ((gObjPtrList[j]->trans.flags & OBJ_FLAGS_PARTICLE) &&
            ((void *) gObjPtrList[j]->level_entry == (void *) obj)) {
            gObjPtrList[j]->level_entry = NULL;
        }
        if (gObjPtrList[j]->behaviorId == BHV_WEAPON_2 || gObjPtrList[j]->behaviorId == BHV_FLY_COIN ||
            gObjPtrList[j]->behaviorId == BHV_WEAPON) {
            free_object(gObjPtrList[j]);
        }
    }
}

/** Racers fade with distance on top of their own transparency. */
void obj_fade_racer(Object *obj, s32 alpha) {
    Object_Racer *racer = obj->racer;
    obj->opacity = ((racer->transparency + 1) * alpha) >> 8;
}
