#include "racer_internal.h"

static u16 gSurfaceSoundIds[19] = {
    0x010C, 0x010B, 0x0009, 0x010C, 0x010C, 0x010C, 0x010C, 0x010C, 0x010C, 0x010C,
    0x010C, 0x0005, 0x010C, 0x010C, 0x010C, 0x010C, 0x010C, 0x010C, 0x010C,
};

static s32 gSurfaceFlagTable[19] = {
    1, 4, 0x10, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0x100, 1, 1, 1, 1, 1,
};

static s32 gSurfaceFlagTable4P[20] = {
    0, 4, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x100, 0, 0, 0, 0, 0, 0,
};

static s8 gRacerWobbleTable[16][2] = {
    { 0x02, 0xFE }, { 0x03, 0xFE }, { 0x02, 0xFC }, { 0x02, 0xFB }, { 0x02, 0xFB }, { 0x02, 0xFE },
    { 0x02, 0xFD }, { 0x02, 0xFE }, { 0x03, 0xFD }, { 0x05, 0xFC }, { 0x04, 0xFE }, { 0x02, 0xFE },
    { 0x02, 0xFA }, { 0x02, 0xFE }, { 0x08, 0xF8 }, { 0x03, 0xFD },
};

/**
 * When braking or driving in a place considered dark, a timer counts up and then sets flags.
 * These flags correspond to the lights on the back of the car being enabled and what colour they are.
 * When you exit this dark place, the timer counts back down and the lights stop.
 */
void set_racer_tail_lights(Object_Racer *racer) {
    s32 lightTimer;

    racer->lightFlags &= ~RACER_LIGHT_BRAKE;
    if (gCurrentRacerInput & B_BUTTON) {
        racer->lightFlags |= RACER_LIGHT_BRAKE;
    }

    lightTimer = racer->lightFlags & RACER_LIGHT_TIMER;
    if (racer->lightFlags & (RACER_LIGHT_BRAKE | RACER_LIGHT_NIGHT)) {
        lightTimer++;
        if (lightTimer > 2) {
            lightTimer = 2;
        }
    } else {
        lightTimer--;
        if (lightTimer < 0) {
            lightTimer = 0;
        }
    }

    racer->lightFlags = (racer->lightFlags & 0xFFF0) | lightTimer;
}

/**
 * Counter-steering against a drift for long enough spins the car out. While drifting the
 * stick is squashed and biased towards the drift so the car keeps sliding.
 */
static void car_update_drift_countersteer(Object *obj, Object_Racer *racer, s32 updateRate) {
    f32 held;

    if (racer->drift_direction == 0) {
        racer->driftCounterSteerTimer = 0;
        return;
    }
    if (racer->drift_direction < 0 && gCurrentStickX >= 26) {
        racer->driftCounterSteerTimer += updateRate;
        if (racer->driftCounterSteerTimer < 0) {
            racer->driftCounterSteerTimer = 0;
        }
    } else if (racer->drift_direction > 0 && gCurrentStickX <= -26) {
        racer->driftCounterSteerTimer -= updateRate;
        if (racer->driftCounterSteerTimer > 0) {
            racer->driftCounterSteerTimer = 0;
        }
    } else {
        racer->driftCounterSteerTimer = 0;
    }
    held = racer->driftCounterSteerTimer;
    if (held < 0.0f) {
        held = -held;
    }
    if (held > 80.0f) {
        racer_play_sound(obj, SOUND_CAR_BRAKE);
        racer->spinout_timer = -racer->drift_direction;
        racer->drift_direction = 0;
    }
    if (gCurrentStickX > 50) {
        gCurrentStickX = 50;
    }
    if (gCurrentStickX < -50) {
        gCurrentStickX = -50;
    }
    if ((gCurrentStickX > 0 && racer->drift_direction > 0) || (gCurrentStickX < 0 && racer->drift_direction < 0)) {
        gCurrentStickX >>= 2;
    }
    gCurrentStickX += (racer->drift_direction * 60);
}

/**
 * On deep water, or frozen water, the car is carried along the checkpoint path and lifted
 * out (unk1F0 marks the car as being carried).
 */
static void car_follow_path_on_water(Object *obj, Object_Racer *racer) {
    f32 goalX;
    f32 goalY;
    f32 goalZ;
    f32 length;

    if (racer->waterEscapeActive != 0) {
        racer->checkpoint_distance -= 0.3;
    } else {
        racer->checkpoint_distance -= 0.1;
    }
    set_position_goal_from_path(obj, racer, &goalX, &goalY, &goalZ);
    if (racer->waterEscapeActive != 0) {
        racer->checkpoint_distance += 0.3;
    } else {
        racer->checkpoint_distance += 0.1;
    }
    goalX -= obj->trans.x_position;
    goalZ -= obj->trans.z_position;
    length = sqrtf((goalX * goalX) + (goalZ * goalZ));
    goalX /= length;
    goalZ /= length;
    obj->x_velocity = goalX * 4.0f;
    obj->z_velocity = goalZ * 4.0f;
    obj->y_velocity = 9.5f;
    racer->waterEscapeActive = 1;
}

/**
 * Ease the visible heading towards the steering, limited per frame, and the roll towards
 * the target roll.
 */
static void car_update_heading(Object *obj, Object_Racer *racer, s32 updateRate) {
    s32 diff;

    diff = gCurrentCarSteerVel - (racer->y_rotation_vel & 0xFFFF);
    if (diff > 0x8000) {
        diff -= 0xFFFF;
    }
    if (diff < -0x8000) {
        diff += 0xFFFF;
    }
    diff >>= 2;
    if (diff > 0x2EE) {
        diff = 0x2EE;
    }
    if (diff < -0x2EE) {
        diff = -0x2EE;
    }
    racer->y_rotation_vel += (diff * updateRate);
    obj->trans.rotation.s[0] = racer->steerVisualRotation + racer->y_rotation_vel;
    racer->z_rotation_vel += (((gRacerTargetRoll - racer->z_rotation_vel) * updateRate) >> 4);
    obj->trans.rotation.s[2] = racer->x_rotation_vel + racer->z_rotation_vel;
}

/**
 * A landing sound requested by the ground physics, with a squash of the character scaled
 * by how hard the landing was.
 */
static void car_update_surface_sound(Object *obj, Object_Racer *racer) {
    if (gRacerSurfaceSoundId == 0) {
        return;
    }
    if (gCurrentPlayerIndex != PLAYER_COMPUTER && gRacerSurfaceSoundId != SOUND_STOMP5) {
        sound_play_spatial(gRacerSurfaceSoundId, obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                           &racer->landingSoundHandle);
        sound_volume_set_relative(gRacerSurfaceSoundId, racer->landingSoundHandle, gRacerSurfaceSoundVolume);
    }
    racer->stretch_height_cap = (1.0 - ((f32) (gRacerSurfaceSoundVolume - 40) * 0.004));
    racer->trickType = 0;
}

/**
 * Turn the forward and lateral velocity into a world velocity along the heading (plus the
 * drift offset).
 */
static void car_local_velocity_to_world(Object *obj, Object_Racer *racer) {
    MtxF mtx;
    f32 unusedY;

    gCurrentRacerTransform.rotation.s[0] = racer->steerVisualRotation + racer->driftHeadingOffset;
    gCurrentRacerTransform.rotation.s[1] = 0;
    gCurrentRacerTransform.rotation.s[2] = 0;
    gCurrentRacerTransform.x_position = 0.0f;
    gCurrentRacerTransform.y_position = 0.0f;
    gCurrentRacerTransform.z_position = 0.0f;
    gCurrentRacerTransform.scale = 1.0f;
    mtxf_from_inverse_transform(&mtx, &gCurrentRacerTransform);
    mtxf_transform_point(mtx, racer->lateral_velocity, 0.0f, racer->velocity, &obj->x_velocity, &unusedY,
                         &obj->z_velocity);
}

/**
 * Move the car by its velocity plus any bump and push, or draw it towards the object it is
 * approaching. Blocked input damps everything. Returns TRUE when a human player's car moved.
 */
static s32 car_move(Object *obj, Object_Racer *racer, f32 updateRateF) {
    f32 velX;
    f32 velZ;

    if (racer->approachTarget != NULL) {
        racer_approach_object(obj, racer, updateRateF);
        return FALSE;
    }
    velX = obj->x_velocity;
    velZ = obj->z_velocity;
    if (racer->bumpTimer != 0) {
        velX += racer->bumpVelX;
        velZ += racer->bumpVelZ;
    }
    if (gRacerInputBlocked != 0) {
        velX *= 0.65;
        velZ *= 0.65;
        if (racer->velocity < -0.5 || racer->velocity > 0.5) {
            racer->velocity *= 0.65;
        } else {
            racer->velocity = 0.0f;
        }
        if (racer->lateral_velocity < -0.5 || racer->lateral_velocity > 0.5) {
            racer->lateral_velocity *= 0.65;
        } else {
            racer->lateral_velocity = 0.0f;
        }
    } else {
        velX += racer->pushVelX;
        velZ += racer->pushVelZ;
    }
    return move_object(obj, velX * updateRateF, obj->y_velocity * updateRateF, velZ * updateRateF) &&
           gCurrentPlayerIndex != PLAYER_COMPUTER;
}

/**
 * After collision, measure how the car actually moved in its own frame and pull the
 * forward and lateral velocity towards it, allowing half a unit of slack. A bumped car
 * keeps its velocity.
 */
static void car_reconcile_velocity(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF, f32 xPos,
                                   f32 yPos, f32 zPos) {
    MtxF mtx;
    f32 invRate;
    f32 movedX;
    f32 movedZ;
    f32 movedLateral;
    f32 movedUp;
    f32 movedForward;
    f32 diff;

    invRate = 1.0f / updateRateF;
    movedX = (((obj->trans.x_position - xPos) - gRacerExternalShiftX) * invRate) - racer->pushVelX;
    obj->y_velocity = (obj->trans.y_position - yPos) * invRate;
    movedZ = (((obj->trans.z_position - zPos) - gRacerExternalShiftZ) * invRate) - racer->pushVelZ;
    gCurrentRacerTransform.rotation.s[0] = -(racer->steerVisualRotation + racer->driftHeadingOffset);
    gCurrentRacerTransform.rotation.s[1] = -obj->trans.rotation.s[1];
    gCurrentRacerTransform.rotation.s[2] = -obj->trans.rotation.s[2];
    gCurrentRacerTransform.x_position = 0.0f;
    gCurrentRacerTransform.y_position = 0.0f;
    gCurrentRacerTransform.z_position = 0.0f;
    gCurrentRacerTransform.scale = 1.0f;
    mtxf_from_inverse_transform(&mtx, &gCurrentRacerTransform);
    mtxf_transform_point(mtx, movedX, 0.0f, movedZ, &movedLateral, &movedUp, &movedForward);
    if (racer->bumpTimer != 0) {
        racer->bumpTimer -= updateRate;
        if (racer->bumpTimer < 0) {
            racer->bumpTimer = 0;
        }
        return;
    }
    diff = racer->velocity - movedForward;
    if (diff > 0.5) {
        racer->velocity -= diff - 0.5;
    }
    if (diff < -0.5) {
        racer->velocity -= diff + 0.5;
    }
    diff = racer->lateral_velocity - movedLateral;
    if (diff > 0.5) {
        racer->lateral_velocity -= diff - 0.5;
    }
    if (diff < -0.5) {
        racer->lateral_velocity -= diff + 0.5;
    }
}

/**
 * Boost flames, splashes and bubbles in water, then the particle update itself.
 */
static void car_update_particles(Object *obj, Object_Racer *racer, s32 updateRate) {
    if (racer->boostTimer == 0 && gNumViewports < 2) {
        racer_enable_boost_emitter(obj, racer, 0x10);
    }
    if (gCurrentPlayerIndex != PLAYER_COMPUTER && gNumViewports < 2) {
        if (racer->buoyancy > 14.0f) {
            if (rand_range(0, 1) != 0) {
                obj->particleEmittersEnabled |= PARTICLE_RANDOM_COLOUR_RED | PARTICLE_RANDOM_COLOUR_GREEN;
            }
        } else if (racer->buoyancy < 6.0f) {
            if (racer->velocity > -3.0 && racer->velocity < 0.5 && rand_range(0, 1) != 0) {
                obj->particleEmittersEnabled |= PARTICLE_RANDOM_SCALE_VELOCITY | PARTICLE_RANDOM_MOVEMENT_PARAM;
            }
        }
    }
    if (racer->vehicleID < VEHICLE_BOSSES) {
        update_vehicle_particles(obj, updateRate);
    }
}

/**
 * One frame of car physics for a human player (AI cars take the cheaper path): read the
 * controls, steer, run the ground or air velocity model, convert to a world velocity, move,
 * collide and reconcile.
 */
void update_car(s32 updateRate, f32 updateRateF, Object *racerObj, Object_Racer *racer) {
    LevelHeader *currentLevelHeader;
    f32 lift;
    f32 xPos;
    f32 yPos;
    f32 zPos;
    s32 driftRoll;
    s32 playerMoved;

    if (racer->bumpTimer != 0) {
        gCurrentRacerInput &= ~A_BUTTON;
    }
    if (gCurrentPlayerIndex == PLAYER_COMPUTER || racer->raceFinished) {
        update_onscreen_AI_racer(racerObj, racer, updateRate, updateRateF);
        return;
    }
    // A spin-out from counter-steering (unk1D4) decays while it blocks hopping and sliding.
    if (racer->spinDirection != 0) {
        gCurrentButtonsPressed &= ~(Z_TRIG | R_TRIG);
        if (racer->spinDirection > 0) {
            racer->spinDirection -= 4;
        } else {
            racer->spinDirection += 4;
        }
    }
    if (gCurrentRacerInput & B_BUTTON && racer->drift_direction != 0) {
        driftRoll = racer->drift_direction * 3640;
    } else {
        driftRoll = 0;
    }
    if (racer->effectBoxType == 0) {
        racerObj->particleEmittersEnabled |= PARTICLE_RANDOM_VELOCITY_Z;
    }
    apply_vehicle_rotation_offset(racer, updateRate, 0, 0, driftRoll);
    update_car_wheels(racerObj, racer, updateRateF);
    handle_racer_head_turning(racerObj, racer, updateRate);
    gRacerSurfaceSoundId = 0;
    gCurrentCarSteerVel = 0;
    gRacerTargetRoll = 0;
    xPos = racerObj->trans.x_position;
    yPos = racerObj->trans.y_position;
    zPos = racerObj->trans.z_position;
    car_update_drift_countersteer(racerObj, racer, updateRate);
    handle_base_steering(racer, 0, updateRateF);
    racer_update_basis_vectors(racerObj, racer);
    racer_update_tilt_vector(racerObj, racer);
    handle_car_velocity_control(racer);
    handle_racer_items(racerObj, racer, updateRate);
    racer_attack_handler_car(racerObj, racer, updateRate);
    if (racer->spinout_timer != 0) {
        racer_spinout_car(racerObj, racer, updateRate, updateRateF);
    } else if (racer->groundedWheels > 0) {
        update_player_car_velocity_ground(racerObj, racer, updateRate, updateRateF);
    } else {
        update_car_velocity_offground(racerObj, racer, updateRate, updateRateF);
    }
    if (racer->unusedSoundHandle1C != 0) {
        sndp_stop((SoundHandle) (uintptr_t) racer->unusedSoundHandle1C);
        racer->unusedSoundHandle1C = 0;
    }
    if (racer->buoyancy != 0.0f && racer->vehicleIDPrev != VEHICLE_BUBBLER) {
        lift = racer->buoyancy - 10.0;
        racerObj->y_velocity += lift * 0.065 * updateRateF;
    }
    currentLevelHeader = level_header();
    if ((racer->buoyancy != 0.0 && currentLevelHeader->waterPathFollow != 0) ||
        gCurrentSurfaceType == SURFACE_FROZEN_WATER) {
        car_follow_path_on_water(racerObj, racer);
    } else if (racer->groundedWheels >= 3) {
        racer->waterEscapeActive = 0;
    }
    if (racer->approachTarget == NULL) {
        update_car_drive_animation(racerObj, racer, updateRate);
    }
    car_update_heading(racerObj, racer, updateRate);
    car_update_surface_sound(racerObj, racer);
    if (racer->waterEscapeActive == 0) {
        car_local_velocity_to_world(racerObj, racer);
    }
    if (racer->magnetTimer != 0) {
        racerObj->x_velocity = gRacerMagnetVelX;
        racerObj->z_velocity = gRacerMagnetVelZ;
    }
    playerMoved = car_move(racerObj, racer, updateRateF);
    if (gCurrentPlayerIndex == PLAYER_COMPUTER) {
        onscreen_ai_racer_physics(racerObj, racer, updateRate);
    } else {
        update_racer_collision(racerObj, racer, updateRate);
    }
    car_reconcile_velocity(racerObj, racer, updateRate, updateRateF, xPos, yPos, zPos);
    car_update_particles(racerObj, racer, updateRate);
    second_racer_camera_update(racerObj, racer, CAMERA_CAR, updateRateF);
    if (gNumViewports == 1 && racer->velocity > -3.0 && get_settings()->courseId != ASSET_LEVEL_WIZPIG1) {
        set_anti_aliasing(TRUE);
    }
    if (playerMoved) {
        racer_find_start_node(racerObj, racer);
    }
}

/**
 * At this point, control is taken away from the player.
 * Instead, they'll automatically move towards a specific object.
 */
void racer_approach_object(Object *obj, Object_Racer *racer, f32 divisor) {
    f32 diffX;
    f32 diffY;
    f32 diffZ;

    obj->animationID = 0;
    obj->animFrame = 40;
    diffX = racer->approachTarget->trans.x_position - obj->trans.x_position;
    diffY = racer->approachTarget->trans.y_position - obj->trans.y_position;
    diffZ = racer->approachTarget->trans.z_position - obj->trans.z_position;
    move_object(obj, diffX, diffY, diffZ);
    obj->trans.rotation.y_rotation = racer->approachTarget->trans.rotation.y_rotation;
    obj->trans.rotation.x_rotation = racer->approachTarget->trans.rotation.x_rotation;
    obj->trans.rotation.z_rotation = racer->approachTarget->trans.rotation.z_rotation;
    racer->x_rotation_vel = obj->trans.rotation.z_rotation;
    racer->steerVisualRotation = obj->trans.rotation.y_rotation;
    obj->x_velocity = diffX / divisor;
    obj->y_velocity = diffY / divisor;
    obj->z_velocity = diffZ / divisor;
    racer->velocity = 0.0f;
    racer->driveAnimState = 0;
    racer->lateral_velocity = 0.0f;
}

/**
 * Applies visual rotational offets to vehicles.
 * Examples include planes when on the ground, or when crashing, or hovercraft when braking.
 */
void apply_vehicle_rotation_offset(Object_Racer *obj, s32 max, s16 yRotation, s16 xRotation, s16 zRotation) {
    s32 diff;
    s32 tempAngle;

    if (!obj->launchedState) {
        tempAngle = yRotation - (u16) obj->y_rotation_offset;
        obj->xRotationOffsetTarget = xRotation;
        WRAP(tempAngle, -0x8000, 0x8000);
        if (tempAngle > 0) {
            diff = max * 0x600;
            if (diff < tempAngle) {
                tempAngle = diff;
            }
            obj->y_rotation_offset += tempAngle;
        } else if (tempAngle < 0) {
            diff = -(max * 0x600);
            if (tempAngle < diff) {
                tempAngle = diff;
            }
            obj->y_rotation_offset += tempAngle;
        }
        tempAngle = xRotation - (u16) obj->x_rotation_offset;
        WRAP(tempAngle, -0x8000, 0x8000);
        if (tempAngle > 0) {
            diff = max * 0x600;
            if (diff < tempAngle) {
                tempAngle = diff;
            }
            obj->x_rotation_offset += tempAngle;
        } else if (tempAngle < 0) {
            diff = -(max * 0x600);
            if (tempAngle < diff) {
                tempAngle = diff;
            }
            obj->x_rotation_offset += tempAngle;
        }
        tempAngle = zRotation - (u16) obj->z_rotation_offset;
        WRAP(tempAngle, -0x8000, 0x8000);
        if (tempAngle > 0) {
            diff = max * 0x600;
            if (diff < tempAngle) {
                tempAngle = diff;
            }
            obj->z_rotation_offset += tempAngle;
        } else if (tempAngle < 0) {
            diff = -(max * 0x600);
            if (tempAngle < diff) {
                tempAngle = diff;
            }
            obj->z_rotation_offset += tempAngle;
        }
    }
}

/**
 * Bob the body (and anything attached) with the wobble table, unless a dialogue camera
 * is holding the car still.
 */
static void car_update_bob(Object *obj, Object_Racer *racer) {
    ObjectTransform *attachmentTrans;
    f32 wobble;
    f32 invScale;
    s32 i;

    if (gDialogueCameraAngle != 0) {
        racer->carBobX = 0.0f;
        racer->carBobY = 0.0f;
        racer->carBobZ = 0.0f;
        return;
    }
    wobble = (f32) gRacerWobbleTable[0][racer->miscAnimCounter & 0x1F] * 0.024999999999999998;
    racer->carBobX = -racer->roll * wobble;
    racer->carBobY = -racer->yaw * wobble;
    racer->carBobZ = -racer->pitch * wobble;
    if (obj->attachPoints != NULL) {
        for (i = 0; i < obj->attachPoints->count; i++) {
            invScale = 1.0f / obj->trans.scale;
            attachmentTrans = &obj->attachPoints->obj[i]->trans;
            attachmentTrans->x_position = (f32) (-racer->carBobX * invScale);
            attachmentTrans->y_position = (f32) (-racer->carBobY * invScale);
            attachmentTrans->z_position = (f32) (-racer->carBobZ * invScale);
        }
    }
}

/**
 * wheelspinTimer holds 60 while the throttle is held from a standstill or in reverse, and counts
 * down after; it marks wheelspin for the sound and smoke.
 */
static void car_update_wheelspin(Object_Racer *racer, s32 updateRate) {
    if (racer->velocity > -0.5) {
        if (gCurrentRacerInput & A_BUTTON) {
            racer->wheelspinTimer = 60;
        } else {
            racer->wheelspinTimer = 0;
        }
    } else if (!(gCurrentRacerInput & A_BUTTON)) {
        racer->wheelspinTimer = 0;
    }
    if (racer->wheelspinTimer > 0) {
        racer->wheelspinTimer -= updateRate;
    } else {
        racer->wheelspinTimer = 0;
    }
}

/**
 * Hard braking rumbles, smokes the tyres and jitters the steering. Returns TRUE while
 * braking; *fourPlayerSmoke asks for the cheaper four-player smoke instead of emitters.
 */
static s32 car_update_brake(Object *obj, Object_Racer *racer, s32 *fourPlayerSmoke) {
    if (!(racer->brake > 0.4)) {
        return FALSE;
    }
    if (racer->velocity < -2.0 && racer->raceFinished == FALSE) {
        rumble_set(racer->playerIndex, RUMBLE_TYPE_1);
    }
    if (gCurrentPlayerIndex >= PLAYER_ONE) {
        if (gNumViewports < FOUR_PLAYERS) {
            if (racer->drift_direction > 0) {
                obj->particleEmittersEnabled |= 0x1000 | 0x400;
            } else if (racer->drift_direction < 0) {
                obj->particleEmittersEnabled |= 0x2000 | 0x800;
            } else {
                obj->particleEmittersEnabled |= 0x2000 | 0x1000;
            }
        } else {
            *fourPlayerSmoke = 1;
        }
    }
    if ((racer->miscAnimCounter & 7) < 2) {
        racer->steerJitter = rand_range(-25, 25);
    }
    gCurrentStickX += racer->steerJitter;
    return TRUE;
}

/**
 * A handbrake turn (racer->drifting): holding R with the stick hard over while rolling
 * backwards swings the car round a half turn, then it carries on facing the other way.
 */
static void car_update_handbrake_turn(Object *obj, Object_Racer *racer, s32 updateRate, s32 *sliding,
                                      s32 *fourPlayerSmoke) {
    s32 prevVel;

    if (racer->drifting == 0 && racer->velocity > 2.0 && gCurrentRacerInput & R_TRIG &&
        (gCurrentStickX > 50 || gCurrentStickX < -50)) {
        if (gCurrentStickX > 0) {
            racer->drifting = 1;
        } else {
            racer->drifting = -1;
        }
    }
    if (racer->drifting == 0) {
        return;
    }
    gCurrentRacerInput &= ~A_BUTTON;
    gCurrentRacerInput |= B_BUTTON;
    gCurrentStickY = -70;
    *sliding = TRUE;
    if (gNumViewports < FOUR_PLAYERS) {
        obj->particleEmittersEnabled |= 0x2000 | 0x1000 | 0x800 | 0x400;
    } else {
        *fourPlayerSmoke = TRUE;
    }
    prevVel = racer->y_rotation_vel;
    racer->y_rotation_vel += updateRate * 0x600 * racer->drifting;
    if (racer->drifting > 0) {
        if (racer->y_rotation_vel < 0 && prevVel >= 0) {
            racer->drifting = 0;
            racer->y_rotation_vel = 0x7FFF;
        }
    } else if (racer->y_rotation_vel > 0 && prevVel <= 0) {
        racer->drifting = 0;
        racer->y_rotation_vel = -0x8000;
    }
    if (racer->drifting == 0) {
        racer->steerVisualRotation += racer->y_rotation_vel;
        racer->cameraYawOffset = racer->y_rotation_vel;
        racer->y_rotation_vel = 0;
        racer->velocity = -racer->velocity;
    }
}

/**
 * Power-sliding (drift_direction): started by R with the stick over at speed, ended when
 * R is released or the car slows. It smokes the tyres, adds a little top speed, and the
 * heading offset leans into it. Returns the top speed multiplier.
 */
static f32 car_update_drift(Object *obj, Object_Racer *racer, s32 updateRate, s32 *fourPlayerSmoke) {
    f32 topSpeed;

    if (racer->drift_direction != 0 && gCurrentPlayerIndex >= PLAYER_ONE) {
        if (gNumViewports < FOUR_PLAYERS) {
            if (racer->drift_direction > 0) {
                obj->particleEmittersEnabled |= 0x1000 | 0x400;
            } else {
                obj->particleEmittersEnabled |= 0x2000 | 0x800;
            }
        } else {
            *fourPlayerSmoke = TRUE;
        }
    }
    if (racer->drift_direction == 0 && gCurrentRacerInput & R_TRIG && racer->velocity < -3.0 &&
        (gCurrentStickX > 15 || gCurrentStickX < -15)) {
        if (gCurrentStickX < 0) {
            racer->drift_direction = -1;
        }
        if (gCurrentStickX > 0) {
            racer->drift_direction = 1;
        }
    }
    if (racer->velocity > -2.0) {
        racer->drift_direction = 0;
    }
    topSpeed = handle_racer_top_speed(obj, racer);
    if (gCurrentRacerInput & R_TRIG && racer->drift_direction != 0) {
        topSpeed *= 1.1;
    } else {
        racer->drift_direction = 0;
    }
    racer->driftHeadingOffset -= ((racer->driftHeadingOffset - (racer->drift_direction << 13)) * updateRate) >> 4;
    return topSpeed;
}

/**
 * The tyre-squeal loop plays while sliding, drifting or spinning the wheels; otherwise it
 * stops. Returns TRUE while the car is sliding sideways.
 */
static s32 car_update_slide_sounds(Object *obj, Object_Racer *racer) {
    if (racer->y_rotation_vel <= 0x1800 && racer->y_rotation_vel >= -0x1800 && racer->drift_direction == 0 &&
        racer->drifting == 0 && racer->wheelspinTimer == 0) {
        if (racer->slideSoundHandle) {
            sndp_stop((SoundHandle) (uintptr_t) racer->slideSoundHandle);
        }
        if (racer->unusedSoundHandle14) {
            sndp_stop((SoundHandle) (uintptr_t) racer->unusedSoundHandle14);
        }
        return FALSE;
    }
    if (racer->wheelspinTimer != 0 && racer->raceFinished == FALSE) {
        rumble_set(racer->playerIndex, RUMBLE_TYPE_0);
    }
    if (racer->y_rotation_vel < 0) {
        obj->particleEmittersEnabled |= 0x1000 | 0x400;
    } else {
        obj->particleEmittersEnabled |= 0x2000 | 0x800;
    }
    if (racer->drift_direction != 0 || racer->drifting != 0 || racer->wheelspinTimer != 0) {
        if (racer->slideSoundHandle == 0) {
            sound_play_spatial(SOUND_CAR_SLIDE, obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                               &racer->slideSoundHandle);
        } else {
            audspat_calculate_echo((SoundHandle) (uintptr_t) racer->slideSoundHandle, obj->trans.x_position,
                                   obj->trans.y_position, obj->trans.z_position);
        }
        if (racer->unusedSoundHandle14) {
            sndp_stop((SoundHandle) (uintptr_t) racer->unusedSoundHandle14);
        }
    } else {
        if (racer->slideSoundHandle) {
            sndp_stop((SoundHandle) (uintptr_t) racer->slideSoundHandle);
        }
    }
    return TRUE;
}

/**
 * Turn the car: the steer angle turns it faster under braking and faster still when
 * drifting, scaled by the character's handling. Reversing fast wobbles the steering.
 */
static void car_update_steering(Object_Racer *racer, s32 updateRate) {
    s8 steerAngle;
    s32 rate;
    s32 turn;
    s32 scaled;
    s32 wobble;

    steerAngle = racer->steerAngle;
    rate = 6;
    if (gCurrentRacerInput & B_BUTTON) {
        rate = 12;
    }
    if (gCurrentRacerInput & B_BUTTON && racer->drift_direction != 0) {
        rate = 18;
    }
    turn = 0;
    if (racer->velocity < -0.3) {
        turn = steerAngle * rate;
    }
    if (racer->velocity > 0.3) {
        turn = -steerAngle * rate;
    }
    turn = ((turn * updateRate) >> 1);
    scaled = turn * gCurrentRacerHandlingStat;
    racer->steerVisualRotation -= scaled & 0xFFFF;
    handle_car_steering(racer);
    if (racer->velocity < -4.0) {
        if (gCurrentCarSteerVel > 0x1400 || (racer->drift_direction != 0 && gCurrentCarSteerVel > 0)) {
            wobble = racer->miscAnimCounter & 7;
            if (wobble >= 4) {
                wobble = 7 - wobble;
            }
            gCurrentCarSteerVel += wobble * 0x190;
        } else if (gCurrentCarSteerVel < -0x1400 || (racer->drift_direction != 0 && gCurrentCarSteerVel < 0)) {
            wobble = racer->miscAnimCounter & 7;
            if (wobble >= 4) {
                wobble = 7 - wobble;
            }
            gCurrentCarSteerVel -= wobble * 0x190;
        }
    }
}

/**
 * Average the traction, slide and bobbing of the surfaces under the wheels. Returns the
 * most significant surface; *stoneWheels counts wheels on stone.
 */
static s32 car_read_surfaces(Object_Racer *racer, f32 *traction, f32 *slide, s32 *bobbing, s32 *stoneWheels) {
    f32 wheels;
    s32 surfaceType;
    s32 i;

    *traction = 0.0f;
    *slide = 0.0f;
    wheels = 0.0f;
    *bobbing = 0;
    *stoneWheels = 0;
    surfaceType = SURFACE_DEFAULT;
    for (i = 0; i < 4; i++) {
        if (racer->wheel_surfaces[i] != SURFACE_NONE) {
            if (get_filtered_cheats() & CHEAT_FOUR_WHEEL_DRIVER) {
                *traction += gSurfaceTractionTable[0];
            } else {
                *traction += gSurfaceTractionTable[racer->wheel_surfaces[i]];
            }
            *slide += gSurfaceSlideTable[racer->wheel_surfaces[i]];
            *bobbing += gSurfaceBobbingTable[racer->wheel_surfaces[i]];
            if (surfaceType < racer->wheel_surfaces[i]) {
                surfaceType = racer->wheel_surfaces[i];
            }
            if (racer->wheel_surfaces[i] == SURFACE_STONE) {
                (*stoneWheels)++;
            }
            wheels += 1.0f;
        }
    }
    if (wheels > 1.0f) {
        *traction /= wheels;
        *slide /= wheels;
    }
    return surfaceType;
}

/**
 * Apply the surface to the car: water removes grip, steering feeds the lateral velocity
 * (less for characters that slide less), slow cars straighten up, lateral velocity decays
 * by the slide value and the pitch, stone slows and ice bounces, a zip pad boosts, and
 * drag takes its share of the forward velocity.
 */
static void car_apply_grip(Object *obj, Object_Racer *racer, s32 surfaceType, f32 traction, f32 slide, s32 stoneWheels,
                           f32 *topSpeed, f32 velSquare) {
    f32 *miscAsset;
    f32 blend;
    f32 pitchGrip;

    racer_check_taj_pad(racer, surfaceType);
    if (racer->buoyancy != 0.0) {
        traction = 0.02f;
        slide = 0.0f;
    }
    if (surfaceType == SURFACE_SAND && racer->velocity < -2.0 && racer->raceFinished == FALSE) {
        rumble_set(racer->playerIndex, RUMBLE_TYPE_0);
    }
    // Per-character stat, indexed by character ID. Steering adds to lateral velocity divided by this value,
    // so a higher value means the character slides less.
    miscAsset = (f32 *) get_misc_asset(ASSET_MISC_8);
    if (gCurrentPlayerIndex != PLAYER_COMPUTER) {
        if (!(racer->velocity > -2.0) && racer->drift_direction == 0 && !racer->raceFinished) {
            racer->lateral_velocity += (racer->velocity * gCurrentStickX) / miscAsset[racer->characterId]; //!@Delta
            if (racer->playerIndex == PLAYER_COMPUTER) {
                racer->lateral_velocity *= 0.9;
            }
        }
    }
    if (racer->velocity > -3.0) {
        blend = -(racer->velocity + 2.0);
        if (blend < 0.0) {
            blend = 0.0f;
        }
        gCurrentCarSteerVel *= blend;
        slide = ((1.0 - blend) * 0.7) + (slide * blend);
    }
    racer->lateral_velocity *= slide;
    pitchGrip = coss_f(racer->x_rotation_vel);
    if (pitchGrip < 0.0f) {
        pitchGrip = -pitchGrip;
    }
    racer->lateral_velocity *= pitchGrip;
    gCurrentSurfaceType = surfaceType;
    if (stoneWheels != 0) {
        miscAsset = (f32 *) get_misc_asset(ASSET_MISC_32);
        *topSpeed *= miscAsset[stoneWheels];
        racer->magnetTimer = 0;
        racer->lateral_velocity = 0.0f;
    } else if (surfaceType == SURFACE_FROZEN_WATER) {
        racer->boost_sound |= BOOST_SOUND_UNK4;
        racer_play_sound(obj, SOUND_BOUNCE2);
    }
    if (racer->boostTimer == 0 && surfaceType == SURFACE_ZIP_PAD) {
        racer->boostTimer = normalise_time(45);
        racer->boostType = BOOST_LARGE;
        racer_play_sound(obj, SOUND_ZIP_PAD_BOOST);
        play_random_character_voice(obj, SOUND_VOICE_CHARACTER_POSITIVE, 8, 0x82);
        if (racer->raceFinished == FALSE) {
            rumble_set(racer->playerIndex, RUMBLE_TYPE_8);
        }
    }
    if (velSquare < 1.0f && !(gCurrentRacerInput & A_BUTTON)) {
        racer->velocity -= racer->velocity * traction * 8.0f; //!@Delta
    } else {
        racer->velocity -= velSquare * traction; //!@Delta
    }
}

/**
 * Smoke and skid marks for the rear wheels by surface; slideFrames counts frames of sliding.
 */
static void car_update_skid_emitters(Object *obj, Object_Racer *racer, s32 sliding, s32 fourPlayerSmoke) {
    if (sliding) {
        if (racer->slideFrames <= 15) {
            racer->slideFrames++;
        }
    } else {
        racer->slideFrames = 0;
    }
    if (cam_get_viewport_layout() >= THREE_PLAYERS) {
        return;
    }
    if ((sliding != 0 && racer->velocity < -2.0) || fourPlayerSmoke != 0 || racer->wheelspinTimer != 0) {
        if (racer->wheel_surfaces[2] < SURFACE_NONE) {
            obj->particleEmittersEnabled |= gSurfaceFlagTable[racer->wheel_surfaces[2]];
        }
        if (racer->wheel_surfaces[3] < SURFACE_NONE) {
            obj->particleEmittersEnabled |= gSurfaceFlagTable[racer->wheel_surfaces[3]] * 2;
        }
    }
    if (racer->velocity < -2.0) {
        if (racer->wheel_surfaces[2] < SURFACE_NONE) {
            obj->particleEmittersEnabled |= gSurfaceFlagTable4P[racer->wheel_surfaces[2]];
        }
        if (racer->wheel_surfaces[3] < SURFACE_NONE) {
            obj->particleEmittersEnabled |= gSurfaceFlagTable4P[racer->wheel_surfaces[3]] * 2;
        }
    }
}

/**
 * The rolling-surface loop (grass, sand) plays while moving on it, and rough surfaces add
 * a bob to the body.
 */
static void car_update_surface_audio(Object_Racer *racer, s32 surfaceSound, s32 bobbing) {
    f32 bob;

    if (racer->surfaceSoundHandle == 0 && surfaceSound != SOUND_NONE && racer->velocity < -2.0) {
        sound_play(surfaceSound, &racer->surfaceSoundHandle);
    }
    if (racer->surfaceSoundHandle != 0 && (surfaceSound == SOUND_NONE || racer->velocity > -2.0)) {
        sndp_stop((SoundHandle) (uintptr_t) racer->surfaceSoundHandle);
    }
    if (racer->velocity < -2.0 && bobbing >= 4) {
        bob = (f32) (racer->miscAnimCounter & 1) * 0.5;
        racer->carBobX -= racer->roll * bob;
        racer->carBobY -= racer->yaw * bob;
        racer->carBobZ -= racer->pitch * bob;
    }
}

/**
 * Throttle and brake: acceleration comes from the speed curve and top speed (a boost pins
 * it), a stopped car can reverse with the brake, and the brake bleeds speed off.
 */
static void car_apply_throttle(Object_Racer *racer, s32 updateRate, f32 topSpeed, s32 surfaceSound) {
    f32 speed;
    f32 accel;
    f32 reverse;
    f32 braking;

    speed = racer->velocity;
    if (racer->velocity < 0.0f) {
        speed = -speed;
    }
    if (speed > 12.0f) {
        speed = 12.0f;
    }
    accel = racer_speed_curve(speed);
    accel *= 1.7;
    accel *= topSpeed;
    if (racer->boostTimer > 0) {
        if (gRaceStartTimer == 0) {
            racer->throttle = 1.0f;
            if (accel != 0.0) {
                accel = 2.0f;
            }
            racer->boostTimer -= updateRate;
        }
    } else {
        racer->boostTimer = 0;
    }
    if (racer->velocity > -2.0 && racer->velocity < 3.0 &&
        (racer->playerIndex != PLAYER_COMPUTER || racer->aiReverseTimer != 0)) {
        reverse = (3.0 - racer->velocity) * 0.15;
        if (surfaceSound == 4) {
            reverse *= 0.25;
        }
        if ((gCurrentStickY < -25) && !(gCurrentRacerInput & A_BUTTON) && (gCurrentRacerInput & B_BUTTON)) {
            if (racer->vehicleIDPrev >= 5) {
                reverse *= 0.3;
            }
            racer->velocity += reverse; //!@Delta
            racer->brake = 0.0f;
        }
    }
    braking = (racer->brake * accel) * 0.32;
    racer->velocity -= accel * racer->throttle;
    if (racer->velocity > -0.04 && racer->velocity < 0.04) {
        racer->velocity = 0.0f;
    }
    if (racer->velocity < 0.0f) {
        racer->velocity += braking;
        if (racer->velocity > 0.0f) {
            racer->velocity = 0.0f;
        }
    } else {
        racer->velocity -= braking;
        if (racer->velocity < 0.0f) {
            racer->velocity = 0.0f;
        }
    }
}

/**
 * Gravity, and the slope: a car not braking rolls down the way it is pitched, more
 * steeply on a steep slope.
 */
static void car_apply_slope(Object *obj, Object_Racer *racer, f32 updateRateF) {
    f32 weight;
    f32 steepness;

    weight = gCurrentRacerWeightStat;
    obj->y_velocity -= weight * updateRateF;
    if (gDialogueCameraAngle != 0) {
        weight = 0.0f;
    }
    if (racer->brake < 0.9 && racer->vehicleIDPrev < VEHICLE_BOSSES && gRaceStartTimer == 0) {
        steepness = racer->pitch;
        if (steepness < 0.0) {
            steepness = -steepness;
            steepness -= 0.3;
            if (steepness < 0.0) {
                steepness = 0.0f;
            }
            if (steepness > 0.1) {
                steepness = 0.1f;
            }
            steepness *= 35.0f;
        } else {
            steepness = 0.0f;
        }
        racer->velocity += weight * (racer->pitch / (4.0f - steepness)) * updateRateF;
    }
}

/**
 * The forward and lateral velocity of a car on the ground: steering, sliding, drifting,
 * surface grip, throttle, brake and slope, with the sounds and smoke that go with them.
 */
void update_player_car_velocity_ground(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    f32 velSquare;
    f32 traction;
    f32 slide;
    f32 topSpeed;
    s32 surfaceType;
    s32 surfaceSound;
    s32 bobbing;
    s32 stoneWheels;
    s32 sliding;
    s32 fourPlayerSmoke;

    fourPlayerSmoke = FALSE;
    velSquare = racer->velocity * racer->velocity;
    if (racer->velocity < 0.0f) {
        velSquare = -velSquare;
    }
    racer->pushVelX = 0.0f;
    racer->pushVelZ = 0.0f;
    car_update_bob(obj, racer);
    car_update_wheelspin(racer, updateRate);
    sliding = car_update_brake(obj, racer, &fourPlayerSmoke);
    car_update_handbrake_turn(obj, racer, updateRate, &sliding, &fourPlayerSmoke);
    topSpeed = car_update_drift(obj, racer, updateRate, &fourPlayerSmoke);
    if (car_update_slide_sounds(obj, racer)) {
        sliding = TRUE;
    }
    car_update_steering(racer, updateRate);
    surfaceType = car_read_surfaces(racer, &traction, &slide, &bobbing, &stoneWheels);
    car_apply_grip(obj, racer, surfaceType, traction, slide, stoneWheels, &topSpeed, velSquare);
    car_update_skid_emitters(obj, racer, sliding, fourPlayerSmoke);
    surfaceSound = gSurfaceSoundTable[surfaceType];
    car_update_surface_audio(racer, surfaceSound, bobbing);
    car_apply_throttle(racer, updateRate, topSpeed, surfaceSound);
    car_apply_slope(obj, racer, updateRateF);
    racer->forwardVel -= (racer->forwardVel + (racer->velocity * 0.05)) * 0.125; //!@Delta
    racer->smoothedStickY = racer->steerAngle;
    racer->savedCarSteerVel = gCurrentCarSteerVel;
    if (racer->trickType != 0) {
        if (gCurrentPlayerIndex != PLAYER_COMPUTER) {
            if (racer->wheel_surfaces[0] != SURFACE_NONE) {
                gRacerSurfaceSoundId = gSurfaceSoundIds[racer->wheel_surfaces[0]];
                *((s16 *) &gRacerSurfaceSoundVolume) =
                    racer->trickType; // necessary because a different function expects a char.
                racer->trickType = 0;
            }
        }
    }
}

/**
 * Drives the car body animation (sliding and tyre marks, boosting, the item
 * use pose) from the racer's state each tick.
 */
void update_car_drive_animation(Object *obj, Object_Racer *racer, s32 updateRate) {
    s8 *balloonAsset;
    s32 angleVel;
    s32 newAngle;
    s32 actionStatus;

    angleVel = 0;
    if (racer->balloon_quantity > 0) {
        balloonAsset = (s8 *) get_misc_asset(ASSET_MISC_BALLOON_DATA);

        angleVel = balloonAsset[(racer->balloon_type * 10) + (racer->balloon_level * 2)];
    }
    if (gCurrentButtonsPressed & Z_TRIG && angleVel != 4 && angleVel != 8) {
        racer->driveAnimState = 5;
    }
    if (racer->boostTimer) {
        racer->driveAnimFlags |= 4;
    }
    if (racer->driveAnimFlags & 8) {
        if (gNumViewports < 3) {
            if (gCurrentPlayerIndex >= PLAYER_ONE) {
                obj->particleEmittersEnabled |= 0x100000;
            } else {
                obj->particleEmittersEnabled |= 0x80000;
            }
        }
        racer->driveAnimState = 4;
    }
    if (gRaceStartTimer) {
        racer->driveAnimFlags = 0;
        racer->driveAnimState = 0;
    }
    if (racer->raceFinished == TRUE) {
        racer->driveAnimState = 0;
        racer->driveAnimFlags = 0;
    }
    switch (racer->driveAnimState) {
        case 0: // Sliding, creating tyre marks
            angleVel = (s32) ((-racer->y_rotation_vel >> 8) / gCurrentRacerHandlingStat);
            angleVel = 40 - angleVel;
            if (angleVel < 0) {
                angleVel = 0;
            }
            if (angleVel > 73) {
                angleVel = 73;
            }
            newAngle = angleVel - obj->animFrame;
            angleVel = 0;
            if (newAngle > 0) {
                angleVel = updateRate * 3;
                if (newAngle < angleVel) {
                    angleVel = newAngle;
                }
            }
            if (newAngle < 0) {
                angleVel = updateRate * -3;
                if (angleVel < newAngle) {
                    angleVel = newAngle;
                }
            }
            obj->animFrame += angleVel;
            obj->animationID = 0;
            if (angleVel) {} // Fakematch
            if (racer->driveAnimFlags & 4) {
                racer->driveAnimState = 3;
                racer->driveAnimFlags &= 0xFFFB;
            }
            if (racer->velocity > 0.0 && gCurrentRacerInput & B_BUTTON) {
                racer->driveAnimState = 6;
            }
            break;
        case 3: // Boost
            actionStatus = 2;
            if (racer->driveAnimFlags & 4) {
                actionStatus = 4 | 2;
            }
            racer_set_animation(obj, racer, 2, 0, 32, 4, actionStatus, updateRate);
            racer->driveAnimFlags &= 0xFFFB;
            break;
        case 4: // Crash
            racer_set_animation(obj, racer, 3, 0, 32, 2, 0, updateRate);
            racer->driveAnimFlags &= 0xFFF7;
            break;
        case 5: // Horn
            actionStatus = 2;
            if (gCurrentRacerInput & Z_TRIG) {
                actionStatus = 4 | 2;
            }
            racer_set_animation(obj, racer, 4, 0, 48, 4, actionStatus, updateRate);
            break;
        case 6:
            actionStatus = 3;
            if (racer->velocity > 0.0 && gCurrentRacerInput & B_BUTTON) {
                actionStatus = 4 | 2 | 1; // Reverse
            }
            racer_set_animation(obj, racer, 1, 0, 80, 3, actionStatus, updateRate);
            break;
        case 7:
            racer_set_animation(obj, racer, 5, 0, 96, 4, 0, updateRate);
            if (racer->driveAnimState == 0) {
                racer_set_animation(obj, racer, 5, 0, 96, 4, 0, updateRate);
            }
            break;
    }
}

/**
 * Starts or advances a racer animation: blends out of the idle pose first
 * when flags & 1, holds the last frame or plays back out when flags & 2, and
 * never holds when flags & 4. arg5 is the frame step, scaled by arg7.
 */
void racer_set_animation(Object *obj, Object_Racer *racer, s32 action, s32 arg3, s32 duration, s32 arg5, s32 flags,
                         s32 arg7) {
    arg5 *= arg7;

    if (gCurrentPlayerIndex == PLAYER_COMPUTER && action >= 3) {
        obj->animationID = 0;
        racer->driveAnimState = 0;
    } else if (obj->animationID == 0) {
        if (flags & 1) {
            if (obj->animFrame > 40) {
                obj->animFrame -= arg7 * 4;
                if (obj->animFrame <= 40) {
                    obj->animationID = action;
                    obj->animFrame = arg3;
                }
            } else {
                obj->animFrame += arg7 * 4;
                if (obj->animFrame >= 40) {
                    obj->animationID = action;
                    obj->animFrame = arg3;
                }
            }
        } else {
            obj->animationID = action;
            obj->animFrame = arg3;
            racer->driveAnimFlags &= ~0x80;
        }
    } else if (obj->animationID == action) {
        if (flags & 2) {
            if (racer->driveAnimFlags & 0x80) {
                obj->animFrame -= arg5; //!@Delta
                if (obj->animFrame <= 0) {
                    obj->animationID = 0;
                    racer->driveAnimState = 0;
                    obj->animFrame = 40;
                    racer->driveAnimFlags = 0;
                }
            } else {
                obj->animFrame += arg5; //!@Delta
                if (obj->animFrame >= duration) {
                    obj->animFrame = duration - 1;
                    if (!(flags & 4)) {
                        racer->driveAnimFlags |= 0x80;
                    }
                }
            }
        } else {
            obj->animFrame += arg5; //!@Delta
            if (obj->animFrame >= duration) {
                obj->animationID = 0;
                racer->driveAnimState = 0;
                obj->animFrame = 40;
                racer->driveAnimFlags = 0;
            }
        }
    } else {
        obj->animFrame = arg3;
        obj->animationID = action;
    }
}

/**
 * If the spinout timer is above zero, remove steering control and have them rotate until
 * the timer runs out.
 */
void racer_spinout_car(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    s32 angleVel;

    racer->velocity *= 0.97; //!@Delta: Reduces 3% of speed per frame, not accounting for game speed.
    racer->lateral_velocity = 0.0f;
    if (racer->raceFinished == FALSE) {
        rumble_set(racer->playerIndex, RUMBLE_TYPE_0);
    }
    angleVel = racer->y_rotation_vel;
    if (gCurrentPlayerIndex > PLAYER_COMPUTER) {
        if (gNumViewports < VIEWPORT_LAYOUT_4_PLAYERS) {
            obj->particleEmittersEnabled |= 0x4FC00;
        } else {
            if (racer->wheel_surfaces[2] < SURFACE_NONE) {
                obj->particleEmittersEnabled |= 1 << (racer->wheel_surfaces[2] * 2);
            }
            if (racer->wheel_surfaces[3] < SURFACE_NONE) {
                obj->particleEmittersEnabled |= 2 << (racer->wheel_surfaces[3] * 2);
            }
        }
    }
    if (racer->spinout_timer > 0) {
        racer->y_rotation_vel += updateRate * 0x500;
        if (racer->y_rotation_vel > 0 && angleVel < 0) {
            racer->spinout_timer--; //!@Delta
            if (gCurrentStickX > 50 && racer->spinout_timer == 1) {
                racer->spinout_timer = 0;
            }
        }
    } else if (racer->spinout_timer < 0) {
        racer->y_rotation_vel -= updateRate * 0x500;
        if (racer->y_rotation_vel < 0 && angleVel > 0) {
            racer->spinout_timer++; //!@Delta
            if (gCurrentStickX < -50 && racer->spinout_timer == -1) {
                racer->spinout_timer = 0;
            }
        }
    }
    gCurrentCarSteerVel = racer->y_rotation_vel;
    obj->y_velocity -= gCurrentRacerWeightStat * updateRateF;
    gCurrentStickX = 0;
    racer->steerAngle = 0;
}

/**
 * Gets, sets and works with car velocity while either airborne or in water.
 * It also handles turn velocity, too, as well as vertical velocity.
 */
void update_car_velocity_offground(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    s32 steerAngle;
    s32 yStick;
    s32 angle;
    s8 canSteer;
    f32 weight;

    canSteer = FALSE;
    if (racer->effectBoxType == 1 && racer->playerIndex == PLAYER_COMPUTER) {
        gCurrentStickY = 60;
    }
    if (racer->waterEscapeActive != 0) {
        if (racer->playerIndex != PLAYER_COMPUTER) {
            steerAngle = racer->steerAngle;
        } else {
            steerAngle = gCurrentStickX;
        }
        racer->steerVisualRotation -= (u16) ((steerAngle * 6 * updateRate) >> 1);
        angle = -(u16) racer->x_rotation_vel;
        WRAP(angle, -0x8000, 0x8000);
        racer->x_rotation_vel += (angle >> 3); //!@Delta
    }
    if (racer->surfaceSoundHandle) {
        sndp_stop((SoundHandle) (uintptr_t) racer->surfaceSoundHandle); // type cast required to match
    }
    if (racer->slideSoundHandle) {
        sndp_stop((SoundHandle) (uintptr_t) racer->slideSoundHandle); // type cast required to match
    }
    if (racer->unusedSoundHandle14) {
        sndp_stop((SoundHandle) (uintptr_t) racer->unusedSoundHandle14); // type cast required to match
    }
    if (racer->effectBoxType == 1 || racer->effectBoxType == 3) {
        racer->smoothedStickY = racer->steerAngle;
    }
    if (racer->buoyancy != 0.0) {
        if (obj->y_velocity > 2.0) {
            obj->y_velocity = 2.0f;
        }
        if (obj->y_velocity < -2.0) {
            obj->y_velocity = -2.0f;
        }
        racer->smoothedStickY = racer->steerAngle;
        if (gCurrentRacerInput & A_BUTTON) {
            racer->velocity -= 0.5; //!@Delta
        }
        if (gCurrentRacerInput & B_BUTTON && gCurrentStickY < -25) {
            racer->velocity += 0.5; //!@Delta
        }
        canSteer = TRUE;
        racer->lateral_velocity *= 0.87; //!@Delta
        racer->velocity *= 0.87;         //!@Delta
        obj->y_velocity *= 0.9;          //!@Delta
        rotate_racer_in_water(obj, racer, &gCurrentRacerWaterPos, gRacerWaveType, updateRate, gCurrentStickX, 6.0f);
    }
    if (racer->playerIndex == PLAYER_COMPUTER) {
        racer->smoothedStickY = racer->steerAngle;
    }
    if (racer->boostTimer > 0) {
        racer->boostTimer -= updateRate;
    } else {
        racer->boostTimer = 0;
    }
    angle = racer->smoothedStickY * 6;
    if (racer->velocity > 0.3) {
        angle = -racer->steerAngle;
        angle *= 6;
    }
    racer->steerVisualRotation -= angle & 0xFFFF;
    gCurrentCarSteerVel = racer->savedCarSteerVel;
    if (racer->boostTimer) {
        if (racer->velocity > -20.0) {
            racer->velocity -= 1.6; //!@Delta
        }
    }
    yStick = gCurrentStickY;
    if (yStick < 50 && yStick > -50) {
        yStick = 0;
    }
    obj->y_velocity -= (obj->y_velocity * 0.025) * updateRateF;
    weight = gCurrentRacerWeightStat;
    if (racer->launchedState || racer->waterEscapeActive) {
        weight = 0.45f;
        racer->y_rotation_offset += 0x500 * updateRate;
        racer->x_rotation_offset += 0x600 * updateRate;
    }
    if (racer->effectBoxType == 1 || racer->effectBoxType == 3) {
        racer->lateral_velocity *= 0.97; //!@Delta
        racer->velocity *= 0.97;         //!@Delta
        if (yStick > 50) {
            racer->velocity -= 0.2; //!@Delta
        }
        if (yStick < -50) {
            racer->velocity += 0.2; //!@Delta
        }
        canSteer = TRUE;
    }
    if (canSteer) {
        obj->particleEmittersEnabled = OBJ_EMIT_NONE;
        racer->drift_direction = 0;
        racer->driftHeadingOffset -= (racer->driftHeadingOffset * updateRate) >> 4;
        gCurrentCarSteerVel = 0;
        handle_car_steering(racer);
    } else {
        if (yStick > 50) {
            weight *= 1.33;
        }
        if (yStick < -50) {
            weight *= 0.53;
        }
    }
    if (racer->boostTimer) {
        weight *= 0.5;
    }
    yStick = -yStick;
    yStick *= 64;
    yStick = (u16) yStick;
    angle = yStick - ((u16) obj->trans.rotation.x_rotation);
    angle = angle > 0x8000 ? angle - 0xFFFF : angle;
    angle = angle < -0x8000 ? angle + 0xFFFF : angle;
    obj->trans.rotation.x_rotation += (angle >> 3); //!@Delta
    obj->y_velocity -= weight * updateRateF;
    if (racer->buoyancy == 0.0) {
        steerAngle = -obj->y_velocity * 20.0;
        if (steerAngle < 50) {
            steerAngle = 50;
        }
        if (steerAngle > 127) {
            steerAngle = 127;
        }
        racer->trickType = steerAngle;
    }
    obj->particleEmittersEnabled = OBJ_EMIT_NONE;
}

/**
 * Handle the steering input of all cars.
 * It takes the speed of the car, creating a curve for the rotational velocity to scale with.
 */
void handle_car_steering(Object_Racer *racer) {
    s32 velScale;
    f32 vel = racer->velocity;

    // Stay positive :)
    if (vel < 0.0) {
        vel = -vel;
    }
    if (vel > 1.8) {
        vel = 1.8f;
    }
    vel -= 0.2;
    if (vel < 0.0) {
        vel = 0.0f;
    }
    if (racer->drift_direction != 0) {
        velScale = vel * 68.0f;
    } else {
        velScale = vel * 58.0f;
    }
    // Set the steering velocity based on the car's steering value, scaled with the temp forward velocity value.
    gCurrentCarSteerVel -= (racer->steerAngle * velScale);
    // If the car is reversing, then flip the steering.
    if (racer->velocity > 0.0f) {
        gCurrentCarSteerVel = -gCurrentCarSteerVel;
    }
}

/**
 * Rotates the world down vector into the racer's frame (steering, pitch and
 * roll) and stores it in roll/yaw/pitch, which the body tilt reads.
 */
void racer_update_tilt_vector(Object *obj, Object_Racer *racer) {
    MtxF mf;

    gCurrentRacerTransform.rotation.y_rotation = -racer->steerVisualRotation;
    gCurrentRacerTransform.rotation.x_rotation = -obj->trans.rotation.x_rotation;
    gCurrentRacerTransform.rotation.z_rotation = -racer->x_rotation_vel;
    gCurrentRacerTransform.x_position = 0;
    gCurrentRacerTransform.y_position = 0;
    gCurrentRacerTransform.z_position = 0;
    gCurrentRacerTransform.scale = 1;
    mtxf_from_inverse_transform(&mf, &gCurrentRacerTransform);

    mtxf_transform_point(mf, 0.0f, -1.0f, 0.0f, &racer->roll, &racer->yaw, &racer->pitch);
}

/**
 * Adjusts the throttle and brake application for all cars, based off the input.
 */
void handle_car_velocity_control(Object_Racer *racer) {
    if (racer->throttle > 0.0) {
        racer->throttle -= 0.1; //!@Delta
    }

    if (gCurrentRacerInput & A_BUTTON) {
        racer->throttle = 1.0f;
    }

    if (gCurrentRacerInput & B_BUTTON) {
        if (racer->brake < 1.0) {
            racer->brake += 0.2; //!@Delta
        }
    } else {
        //! @bug Will cause a negative brake value resulting in higher velocity
        if (racer->brake > 0.05) {
            racer->brake -= 0.1; //!@Delta
        }
    }
}

/**
 * Spins the wheel models with the car's speed (stopped under hard braking)
 * and keeps the two-wheel vehicles' wheels aligned.
 */
void update_car_wheels(Object *objRacer, Object_Racer *racer, f32 updateRateF) {
    Object *someObj;
    f64 temp_f0;
    f32 velocity;
    s32 temp_v1;
    s32 flags;
    s32 isNotHardBraking;
    s32 i;

    if (objRacer->attachPoints == NULL) {
        return;
    }
    temp_v1 = objRacer->attachPoints->count;
    if (temp_v1 == 2) {
        someObj = objRacer->attachPoints->obj[0];
        someObj->trans.rotation.y_rotation = 0x4000;
        someObj = objRacer->attachPoints->obj[1];
        someObj->trans.rotation.y_rotation = 0x4000;
    }
    if (temp_v1 >= 4) {
        velocity = racer->velocity;
        if (velocity > 5.0) {
            velocity = 5.0;
        }
        if (velocity < -5.0) {
            velocity = -5.0;
        }
        isNotHardBraking = TRUE;
        if (racer->brake > 0.4) {
            if (racer->throttle < 0.4) {
                velocity = 0.0f;
            }
            isNotHardBraking = FALSE;
        }
        racer->wheelAnimAccum -= (velocity * updateRateF) * 0.5;
        while (racer->wheelAnimAccum > 5.0) {
            racer->wheelAnimAccum -= 5.0;
            for (i = 0; i < 4; i++) {
                if (i < 2 || isNotHardBraking) {
                    someObj = objRacer->attachPoints->obj[i];
                    if ((racer->wheelspinTimer != 0 && i >= 3) || racer->wheelspinTimer == 0) {
                        if (someObj->properties.common.unk0 != 0) {
                            someObj->modelIndex--;
                            if (someObj->modelIndex < 0) {
                                someObj->modelIndex = someObj->header->numberOfModelIds - 1;
                            }
                        } else {
                            someObj->modelIndex++;
                            if (someObj->modelIndex == someObj->header->numberOfModelIds) {
                                someObj->modelIndex = 0;
                            }
                        }
                    }
                }
            }
        }
        while (racer->wheelAnimAccum < 0.0f) {
            racer->wheelAnimAccum += 5.0;
            for (i = 0; i < 4; i++) {
                if (i < 2 || isNotHardBraking) {
                    someObj = objRacer->attachPoints->obj[i];
                    if ((racer->wheelspinTimer != 0 && i >= 3) || racer->wheelspinTimer == 0) {
                        if (someObj->properties.common.unk0 == 0) {
                            someObj->modelIndex--;
                            if (someObj->modelIndex < 0) {
                                someObj->modelIndex = someObj->header->numberOfModelIds - 1;
                            }
                        } else {
                            someObj->modelIndex++;
                            if (someObj->modelIndex == someObj->header->numberOfModelIds) {
                                someObj->modelIndex = 0;
                            }
                        }
                    }
                }
            }
        }
        flags = 1;
        if (racer->wheelspinTimer != 0) {
            for (i = 0; i < 2; i++) {
                someObj = objRacer->attachPoints->obj[i];
                if (someObj->properties.common.unk0 != 0) {
                    someObj->modelIndex--;
                    if (someObj->modelIndex < 0) {
                        someObj->modelIndex = (s8) (someObj->header->numberOfModelIds - 1);
                    }
                } else {
                    someObj->modelIndex++;
                    if (someObj->modelIndex == someObj->header->numberOfModelIds) {
                        someObj->modelIndex = 0;
                    }
                }
            }
            objRacer->particleEmittersEnabled |= OBJ_EMIT_11 | OBJ_EMIT_12;
        }
        for (i = 0; i < 4; i++) {
            someObj = objRacer->attachPoints->obj[i];
            if (!(racer->wheelContactFlags & flags)) {
                someObj->trans.y_position += ((-10.0f - someObj->trans.y_position) * 0.125);
                if (racer->waterTimer != 0) {
                    someObj->trans.scale += (((someObj->header->scale * 1.4) - someObj->trans.scale) * 0.1);
                }
            } else {
                temp_f0 = someObj->trans.y_position;
                someObj->trans.y_position = (temp_f0 - (temp_f0 * 0.125));
                someObj->trans.scale += ((someObj->header->scale - someObj->trans.scale) * 0.1);
            }
            flags <<= 1;
        }
        someObj = objRacer->attachPoints->obj[2];
        someObj->trans.rotation.y_rotation = gCurrentStickX * 100;
        someObj = objRacer->attachPoints->obj[3];
        someObj->trans.rotation.y_rotation = gCurrentStickX * 100;
    }
}

/**
 * Initialise some states when a racer is attacked or runs into something.
 * While the majority of the respondent behaviour takes place elsewhere, the squish behaviour
 * happens here.
 */
void racer_attack_handler_car(Object *obj, Object_Racer *racer, s32 updateRate) {
    s8 bananas;

    if (racer->playerIndex == PLAYER_COMPUTER) {
        bananas = 0;
    } else {
        bananas = racer->bananas;
    }
    if (racer->squish_timer > 0) {
        racer->squish_timer -= updateRate;
        racer->throttle = 0.0f;
        racer->stretch_height_cap = 0.1f;
        if (racer->squish_timer <= 0 && racer->playerIndex > PLAYER_COMPUTER) {
            racer_play_sound(obj, SOUND_PLOP2);
        }
    } else {
        racer->squish_timer = 0;
    }
    if (racer->attackType == 0 || racer->shieldTimer > 0) {
        racer->attackType = ATTACK_NONE;
    } else {
        if (racer->squish_timer == 0 || racer->attackType != ATTACK_SQUISHED) {
            if (racer->attackType != ATTACK_SPIN && racer->attackType != ATTACK_SQUISHED) {
                drop_bananas(obj, racer, 2);
            }
            racer->hitRecoveryTimer = 360;
            if (racer->aiBehaviour == 8) {
                racer->aiBehaviour = 0;
            }
            if (racer->vehicleID < VEHICLE_BOSSES) {
                play_random_character_voice(obj, SOUND_VOICE_CHARACTER_NEGATIVE, 8, 129);
                switch (racer->attackType) {
                    // Getting hit by a rocket, or running into a landmine.
                    case ATTACK_EXPLOSION:
                        racer->launchedState = 1;
                        if (bananas == 0) {
                            racer->velocity = 0.0f;
                            obj->y_velocity += 10.5;
                        } else {
                            racer->velocity /= 4;
                            obj->y_velocity += 8.5;
                        }
                        break;
                    // Running into an oil slick.
                    case ATTACK_SPIN:
                        racer->spinout_timer = 2;
                        racer->y_rotation_vel = 0x1002;
                        racer_play_sound(obj, SOUND_TYRE_SCREECH);
                        break;
                    // Crushed by something big, like a snowball.
                    case ATTACK_SQUISHED:
                        racer->squish_timer = 60;
                        break;
                    // Running into a bubble trap.
                    case ATTACK_BUBBLE:
                        racer->bubbleTrapTimer = 120;
                        racer->velocity *= 0.5;
                        break;
                    // This goes unused.
                    case ATTACK_FLUNG:
                        racer->launchedState = 1;
                        racer->velocity = 0.0f;
                        obj->y_velocity += 20.5;
                        racer_play_sound(obj, SOUND_WHEE);
                        break;
                }
                racer->attackType = ATTACK_NONE;
            }
        }
    }
}

/**
 * Gets, sets and works with car velocity while grounded.
 * It also handles turn velocity, too.
 * This function has a large number of timing incorrect physics, responsible for boss vehicles being
 * inconsistently fast, depending on framerate.
 */
void update_car_velocity_ground(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    s32 xStick;
    s32 stickMultiplier;
    s32 surfaceType;
    s32 sp38;
    f32 temp_f0_2;
    f32 velSquare;
    UNUSED f32 pad;
    f32 traction;
    f32 forwardVel;
    f32 weight;
    f32 vel;
    f32 multiplier;
    s32 rawStick;
    s32 velocityS;
    s32 temp;
    u16 temp2;

    velSquare = racer->velocity * racer->velocity;
    racer->pushVelX = 0.0f;
    racer->pushVelZ = 0.0f;
    if (racer->velocity < 0.0f) {
        velSquare = -velSquare;
    }
    sp38 = 0;
    multiplier = handle_racer_top_speed(obj, racer);
    if (racer->y_rotation_vel > 0x1C00 || racer->y_rotation_vel < -0x1C00) {
        sp38 = 1;
    }
    rawStick = gCurrentStickX;
    stickMultiplier = 6;
    if (gCurrentRacerInput & B_BUTTON) {
        stickMultiplier = 12;
    }
    if (gCurrentRacerInput & B_BUTTON && racer->drift_direction != 0) {
        stickMultiplier = 18;
    }
    xStick = 0;
    if (racer->velocity < -0.3) {
        xStick = rawStick * stickMultiplier;
    }
    if (racer->velocity > 0.3) {
        xStick = -rawStick * stickMultiplier;
    }
    xStick *= updateRate;
    temp = (s32) ((xStick >> 1) * gCurrentRacerHandlingStat);
    temp2 = temp;
    racer->steerVisualRotation -= temp2; //!@Delta
    handle_car_steering(racer);
    racer->lateral_velocity *= 0.9; //!@Delta
    surfaceType = SURFACE_DEFAULT;
    traction = 0.0f;
    if (racer->wheel_surfaces[0] != SURFACE_NONE) {
        traction += gSurfaceTractionTable[racer->wheel_surfaces[0]];
        if (racer->wheel_surfaces[0] > SURFACE_DEFAULT) {
            surfaceType = racer->wheel_surfaces[0];
        }
    }
    if (racer->buoyancy != 0.0) {
        traction = 0.02f;
    }
    gCurrentSurfaceType = surfaceType;
    racer->drift_direction = 0;
    racer->driftHeadingOffset = 0;
    if (surfaceType == SURFACE_STONE) {
        multiplier = 0.25f;
        racer->lateral_velocity = 0.0f;
    }
    if (racer->boostTimer == 0 && surfaceType == SURFACE_ZIP_PAD) {
        racer->boostTimer = normalise_time(45);
        racer->boostType = BOOST_UNK3;
    }
    racer->velocity -= velSquare * traction; //!@Delta
    if (sp38) {
        if (racer->slideFrames < 16) {
            racer->slideFrames++;
        }
    } else {
        racer->slideFrames = 0;
    }
    if (cam_get_viewport_layout() < 2 && sp38 && racer->velocity < -2.0) {
        if (racer->wheel_surfaces[2] < SURFACE_NONE) {
            obj->particleEmittersEnabled |= 1 << (racer->wheel_surfaces[2] * 2);
        }
        if (racer->wheel_surfaces[3] < SURFACE_NONE) {
            obj->particleEmittersEnabled |= 2 << (racer->wheel_surfaces[3] * 2);
        }
    }
    vel = racer->velocity;
    if (vel < 0.0f) {
        vel = -vel;
    }
    if (vel > 12.0f) {
        vel = 12.0f;
    }
    velocityS = (s32) vel;
    temp_f0_2 = vel - (f32) velocityS;
    vel = (gCurrentRacerMiscAssetPtr[velocityS + 1] * temp_f0_2) +
          (gCurrentRacerMiscAssetPtr[velocityS] * (1.0 - temp_f0_2));
    vel *= 1.7;
    vel *= multiplier;
    if (racer->boostTimer > 0) {
        if (gRaceStartTimer == 0) {
            racer->throttle = 1.0f;
            if (vel != 0.0) {
                vel = 2.0f;
            }
            racer->boostTimer -= updateRate;
        }
    } else {
        racer->boostTimer = 0;
    }
    // If basically at a standstill, then allow reversing.
    if (racer->velocity > -2.0 && racer->velocity < 3.0) {
        f32 reverseVel = (3.0 - racer->velocity) * 0.15;
        if (gCurrentStickY < -25 && !(gCurrentRacerInput & A_BUTTON) && gCurrentRacerInput & B_BUTTON) {
            racer->brake = 0.0f;
            racer->velocity += reverseVel; //!@Delta
        }
    }
    forwardVel = (vel * racer->brake) * 0.32; //!@Delta
    racer->velocity -= vel * racer->throttle; //!@Delta
    if (racer->velocity > -0.04 && racer->velocity < 0.04) {
        racer->velocity = 0.0f;
    }
    if (racer->velocity < 0.0f) {
        racer->velocity += forwardVel; //!@Delta
        if (racer->velocity > 0.0f) {
            racer->velocity = 0.0f;
        }
    } else {
        racer->velocity -= forwardVel; //!@Delta
        if (racer->velocity < 0.0f) {
            racer->velocity = 0.0f;
        }
    }

    // Heavier characters will fall faster.
    weight = gCurrentRacerWeightStat;
    if (racer->buoyancy != 0.0) {
        weight *= 0.5;
    }
    if (racer->brake < 0.9 && gRaceStartTimer == 0) {
        racer->velocity += weight * (racer->pitch / 4) * updateRateF;
    }
    obj->y_velocity -= weight * updateRateF;
}
