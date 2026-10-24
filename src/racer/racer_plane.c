#include "racer_internal.h"

/**
 * Skimming the water: a plane flying low and fast over waves is held just above them
 * (drift_direction doubles as the skimming flag). Returns TRUE when the plane is low and
 * slow enough for its wheels to drop.
 */
static s32 plane_update_wave_skim(Object *obj, Object_Racer *racer, f32 updateRateF) {
    f32 height;
    f32 speed;
    s32 wheelsDown;
    s32 i;

    if (gCurrentPlayerIndex == PLAYER_COMPUTER || racer->vehicleIDPrev == VEHICLE_WIZPIG || gRacerWaveCount == 0) {
        return FALSE;
    }
    for (i = gRacerWaveCount - 1; i >= 0 && gRacerCurrentWave[i]->waveHeight < obj->trans.y_position + 5; i--) {}
    if (i == gRacerWaveCount - 1) {
        i--;
    }
    height = gRacerCurrentWave[i + 1]->waveHeight;
    height = (obj->trans.y_position - height) - 10;
    if (height > 100.0f) {
        racer->drift_direction = 0;
    }
    speed = -racer->velocity;
    if (speed < 0.0f) {
        speed = 0.0f;
    }
    wheelsDown = height < 35 && speed < 8.0;
    if (racer->drift_direction == 0 && height < 38 && speed >= 8.0) {
        racer->drift_direction = 1;
    }
    if (racer->trickType == 1 || racer->trickType == -1 || gRacerCurrentWave[i + 1]->rot.y < 0.4) {
        racer->drift_direction = 0;
        return FALSE;
    }
    if (racer->drift_direction != 0) {
        if (speed < 8.0 || gCurrentStickY < -10) {
            racer->drift_direction = 0;
        }
        speed -= 8;
        if (speed > 4.0) {
            speed = 4;
        }
        speed /= 4;
        obj->trans.y_position += ((38 - height) * updateRateF * speed) / 8;
        if (gCurrentStickY > 0) {
            gCurrentStickY >>= 1;
        }
    }
    return wheelsDown;
}

/**
 * How much the plane sinks this frame as a share of its weight: full when slow, none at
 * speed. Near the course ceiling it is pushed down hard and the stick is capped
 * (*nearCeiling).
 */
static f32 plane_lift_factor(Object *obj, Object_Racer *racer, s32 *nearCeiling) {
    f32 factor;
    f32 ceilingDist;

    factor =
        (obj->x_velocity * obj->x_velocity) + (obj->z_velocity * obj->z_velocity) + (obj->y_velocity * obj->y_velocity);
    factor = sqrtf(factor) - 2.0;
    if (racer->vehicleID >= VEHICLE_BOSSES) {
        factor = ((factor - 2.0) / 2.0);
    }
    if (factor < 0) {
        factor = 0;
    }
    if (factor > 4.0) {
        factor = 4;
    }
    *nearCeiling = FALSE;
    factor = 1.0 - (factor / 4.0);
    ceilingDist = (gCurrentCourseHeight - 50.0) - obj->trans.y_position;
    if (racer->trickType < 2 && racer->trickType >= -1 && ceilingDist < 0) {
        factor += -ceilingDist / 25.0;
        if (gCurrentStickY < -20) {
            gCurrentStickY = -20;
        }
        if (factor > 2.5) {
            factor = 2.5;
        }
        *nearCeiling = TRUE;
    }
    return factor;
}

/**
 * On the ground the plane taxis: more drag, surface effects, zip pads, boss planes throttle
 * down, and the stick cannot pitch up until there is lift.
 */
static void plane_on_ground(Object *obj, Object_Racer *racer, f32 liftFactor, f32 *throttle, f32 *brake, f32 *drag,
                            f32 *lateralDrag, f32 *pitchDrag) {
    s32 surface;
    s32 stoneWheels;

    *lateralDrag = 0.02f;
    *pitchDrag = 0.01f;
    surface = racer_dominant_surface(racer, &stoneWheels);
    if (surface == SURFACE_STONE) {
        racer->magnetTimer = 0;
    }
    racer_check_taj_pad(racer, surface);
    if (gCurrentRacerInput & B_BUTTON && gCurrentStickY >= -40 && racer->velocity >= -0.5) {
        *drag *= 8;
    }
    if (racer->boostTimer == 0 && surface == SURFACE_ZIP_PAD) {
        racer_start_zip_pad_boost(obj, racer);
    }
    if (racer->vehicleID >= VEHICLE_BOSSES && racer->velocity > -6.0) {
        *throttle *= 0.6;
        *brake *= 0.3;
    }
    racer->trickType = 0;
    if (gCurrentRacerInput & B_BUTTON && gNumViewports <= 2) {
        obj->particleEmittersEnabled = OBJ_EMIT_1 | OBJ_EMIT_2;
    }
    gCurrentStickY = ((f32) gCurrentStickY) * (1.0 - liftFactor);
    if (gCurrentStickY > 0) {
        gCurrentStickY = 0;
    }
}

/**
 * Pitch the nose down as lift is lost, then apply gravity scaled by the lift factor, with
 * overrides for effect boxes, water, the countdown and Wizpig's ship.
 */
static void plane_apply_gravity(Object *obj, Object_Racer *racer, s32 updateRate, f32 liftFactor, s32 nearCeiling) {
    s32 pitch;
    f32 depth;

    pitch = 0;
    if (nearCeiling == FALSE) {
        pitch = (liftFactor * 4608);
    }
    if (racer->vehicleID > VEHICLE_BOSSES) {
        pitch = 0;
    }
    if (racer->vehicleID == VEHICLE_CARPET) {
        liftFactor = 0.0;
        pitch = 0;
    }
    apply_vehicle_rotation_offset(racer, updateRate, 0, pitch, 0);
    if (racer->effectBoxType == 0) {
        liftFactor = 5.5;
        obj->particleEmittersEnabled |= OBJ_EMIT_9;
    }
    if (racer->effectBoxType == 1) {
        liftFactor = 2;
    }
    if (racer->buoyancy != 0.0) {
        liftFactor = -1.0f;
        gCurrentStickY = -60;
        depth = racer->buoyancy - 20.0f;
        if (depth < 0.0) {
            depth = 0;
        }
        liftFactor -= depth / 10;
        if (liftFactor < -4.0) {
            liftFactor = -4;
        }
    }
    if (gRaceStartTimer != 0) {
        liftFactor = 1;
    }
    if (racer->vehicleIDPrev == VEHICLE_WIZPIG) {
        if (obj->animationID < 3) {
            liftFactor = 4.0f;
        } else {
            liftFactor = 0.0f;
        }
    }
    liftFactor *= gCurrentRacerWeightStat;
    obj->y_velocity -= liftFactor;
}

/**
 * A spun-out plane tumbles until it lands and slows. unk1F1 marks the tumble: 1 in the
 * air, 2 once it has touched down and started rolling out.
 */
static void plane_update_spinout(Object *obj, Object_Racer *racer, s32 updateRate) {
    s32 prevRoll;
    s32 step;
    s32 pitch;
    s32 diff;

    prevRoll = racer->x_rotation_offset;
    if (racer->launchedState == 0) {
        racer->launchedState = 1;
    }
    if (racer->groundedWheels != 0 || racer->launchedState == 2) {
        racer->launchedState = 2;
        step = updateRate << 11;
        racer->x_rotation_offset -= step;
        pitch = racer->z_rotation_offset;
        if ((step + pitch) > 0 && pitch <= 0) {
            racer->z_rotation_offset = 0;
        } else {
            racer->z_rotation_offset += step;
        }
        if (racer->velocity > -2.0 && racer->groundedWheels >= 3) {
            racer->spinout_timer = 0;
        }
    } else {
        racer->z_rotation_offset += updateRate << 11;
    }
    if (racer->groundedWheels != 0 && ((prevRoll > 0x6000 && racer->x_rotation_offset <= 0x6000) ||
                                       (prevRoll > -0x6000 && racer->x_rotation_offset <= -0x6000) ||
                                       (prevRoll > 0 && racer->x_rotation_offset <= 0))) {
        racer_play_sound(obj, SOUND_CRASH);
        if (racer->playerIndex != PLAYER_COMPUTER) {
            gCameraObject->shakeMagnitude = 6.0f;
        }
    }
    gCurrentRacerInput &= ~(A_BUTTON | Z_TRIG);
    racer->spinout_timer -= updateRate;
    racer->boostTimer = 0;
    racer->brake = 1;
    if (racer->spinout_timer <= 0) {
        racer->spinout_timer = 0;
        racer->launchedState = 0;
    }
    diff = 0xD800 - (obj->trans.rotation.x_rotation & 0xFFFF);
    if (diff > 0x8000) {
        diff -= 0xFFFF;
    }
    if (diff < -0x8000) {
        diff += 0xFFFF;
    }
    obj->trans.rotation.x_rotation += (diff * updateRate) >> 4;
}

/**
 * A barrel roll (trickType +/-1) rolls the plane a full turn at full throttle. unk1D4 is set
 * once past the halfway point so the roll ends when the angle comes back round.
 */
static void plane_update_barrel_roll(Object_Racer *racer, s32 updateRate, f32 *throttle) {
    s32 prevRoll;

    prevRoll = racer->x_rotation_vel;
    racer->x_rotation_vel += ((racer->trickType * 0x600) * updateRate);
    *throttle = 1.2f;
    if (racer->trickType == 1) {
        if (prevRoll > 0) {
            racer->spinDirection = 1;
        }
        if ((prevRoll < 0) && (racer->x_rotation_vel >= 0) && (racer->spinDirection != 0)) {
            racer->trickType = 0;
            racer->x_rotation_vel = 0;
        }
    } else {
        if (prevRoll < 0) {
            racer->spinDirection = 1;
        }
        if ((prevRoll > 0) && (racer->x_rotation_vel <= 0) && (racer->spinDirection != 0)) {
            racer->trickType = 0;
            racer->x_rotation_vel = 0;
        }
    }
}

/**
 * A loop (trickType +/-2) pitches the plane a full turn and rewards a small boost. Holding
 * R past the top holds the plane inverted for a while (spinRecoverTimer counts it down).
 */
static void plane_update_loop(Object *obj, Object_Racer *racer, s32 updateRate) {
    s32 prevPitch;
    s32 roll;

    prevPitch = obj->trans.rotation.x_rotation;
    if (racer->spinRecoverTimer == 0) {
        obj->trans.rotation.x_rotation = prevPitch + ((racer->trickType * 0x180) * updateRate);
    }
    if (!(gCurrentRacerInput & R_TRIG)) {
        racer->spinRecoverTimer = 0;
    }
    if (racer->spinRecoverTimer > 0) {
        racer->spinRecoverTimer -= updateRate;
    } else {
        racer->spinRecoverTimer = 0;
    }
    roll = racer->x_rotation_vel;
    racer->x_rotation_vel = roll - ((roll * updateRate) >> 4);
    obj->x_velocity = racer->velocity * racer->ox1;
    obj->y_velocity = racer->velocity * racer->oy1;
    obj->z_velocity = racer->velocity * racer->oz1;
    if (racer->trickType == 2) {
        if (prevPitch > 0) {
            racer->spinDirection = 1;
        }
        if (prevPitch < 0) {
            if (obj->trans.rotation.x_rotation >= 0) {
                if (racer->spinDirection != 0) {
                    racer->trickType = 0;
                    obj->trans.rotation.x_rotation = 0;
                    racer->boostTimer = normalise_time(0xA);
                    racer->boostType = BOOST_NONE;
                    if (racer->throttleReleased != 0) {
                        racer->boostType |= EMPOWER_BOOST;
                    }
                }
            }
        }
        if (prevPitch > 0x4000 && obj->trans.rotation.x_rotation < -0x4000 && (gCurrentRacerInput & R_TRIG)) {
            racer->spinRecoverTimer = 60;
        }
    } else {
        if (prevPitch < 0) {
            racer->spinDirection = 1;
        }
        if (prevPitch > 0) {
            if (obj->trans.rotation.x_rotation <= 0) {
                if (racer->spinDirection != 0) {
                    racer->trickType = 0;
                    obj->trans.rotation.x_rotation = 0;
                    racer->boostTimer = normalise_time(10);
                    racer->boostType = BOOST_NONE;
                    if (racer->throttleReleased != 0) {
                        racer->boostType |= EMPOWER_BOOST;
                    }
                }
            }
        }
        if (prevPitch < -0x4000 && obj->trans.rotation.x_rotation > 0x4000 && (gCurrentRacerInput & R_TRIG)) {
            racer->spinRecoverTimer = 60;
        }
    }
}

/**
 * Normal flight: the stick banks the plane and the bank turns it; pitch follows the stick
 * scaled by speed; a double tap of R in the air starts a trick. On the ground R is a brake,
 * not a trick, so it is masked off here and restored by the caller (*rHeldOnGround).
 */
static void plane_update_steering(Object *obj, Object_Racer *racer, s32 updateRate, s32 *rHeldOnGround) {
    s32 steerAngle;
    s32 bank;
    s32 turn;
    s32 roll;
    s32 pitchInput;
    f32 speedScale;
    f32 sideForce;

    steerAngle = racer->steerAngle;
    *rHeldOnGround = FALSE;
    if (racer->groundedWheels != 0) {
        if (gCurrentRacerInput & R_TRIG) {
            *rHeldOnGround = TRUE;
        }
        gCurrentRacerInput &= ~R_TRIG;
    }
    if (racer->groundedWheels < 2) {
        bank = steerAngle;
        steerAngle = 0;
        if (obj->trans.rotation.x_rotation > 0x3000) {
            steerAngle = obj->trans.rotation.x_rotation - 0x3000;
            if (steerAngle > 0x1000) {
                steerAngle = 0x1000;
            }
        } else if (obj->trans.rotation.x_rotation < -0x3000) {
            steerAngle = obj->trans.rotation.x_rotation + 0x3000;
            if (steerAngle < -0x1000) {
                steerAngle = -0x1000;
            }
            steerAngle = -steerAngle;
        }
        bank *= (f32) (1.0 - ((f32) steerAngle / 4096));
        if (gCurrentRacerInput & R_TRIG) {
            obj->particleEmittersEnabled |= OBJ_EMIT_7 | OBJ_EMIT_8;
            racer->x_rotation_vel -= (bank * 16 * updateRate) >> 1;
        }
        racer->x_rotation_vel -= (bank * updateRate * 20) >> 1;
        racer->x_rotation_vel -= (racer->x_rotation_vel * updateRate) >> 4;
        if (racer->zipperDirCorrection == 0) {
            if (gCurrentPlayerIndex != PLAYER_COMPUTER && racer->raceFinished == FALSE) {
                bank = -racer->x_rotation_vel >> 6;
                if ((gCurrentRacerInput & R_TRIG) && (gCurrentRacerInput & B_BUTTON)) {
                    bank *= 2;
                }
                bank *= gCurrentRacerHandlingStat;
                turn = bank * updateRate;
                racer->steerVisualRotation -= turn & 0xFFFF;
            } else {
                bank = gCurrentStickX * 4;
                turn = bank * updateRate;
                racer->steerVisualRotation -= turn & 0xFFFF;
            }
        }
    } else {
        roll = -(racer->x_rotation_vel & 0xFFFF);
        if (roll > 0x8000) {
            roll -= 0xFFFF;
        }
        if (roll < -0x8000) {
            roll += 0xFFFF;
        }
        racer->x_rotation_vel += (roll * updateRate) >> 4;
        if (gCurrentRacerInput & R_TRIG) {
            bank = steerAngle * 6;
        } else {
            bank = steerAngle * 4;
        }
        racer->steerVisualRotation -= (bank * updateRate) & 0xFFFF;
    }
    if (!(gCurrentRacerInput & R_TRIG) || racer->groundedWheels == 0 || racer->zipperDirCorrection != 0) {
        sideForce = racer->velocity * bank * 0.00015;
        obj->x_velocity -= racer->ox3 * sideForce;
        obj->y_velocity -= racer->oy3 * sideForce;
        obj->z_velocity -= racer->oz3 * sideForce;
    }

    pitchInput = gCurrentStickY;
    if (gCurrentPlayerIndex != PLAYER_COMPUTER && racer->raceFinished == FALSE) {
        pitchInput = racer->smoothedStickY;
    }
    speedScale = -racer->velocity;
    if (speedScale < 4.0) {
        speedScale = 4.0f;
    }
    if (speedScale > 14.0) {
        speedScale = 14.0f;
    }
    speedScale /= 7.0;
    pitchInput *= speedScale;
    if (!(gCurrentRacerInput & R_TRIG)) {
        pitchInput >>= 1;
        obj->trans.rotation.x_rotation -= (obj->trans.rotation.x_rotation * updateRate) >> 4;
        obj->trans.rotation.x_rotation -= ((pitchInput * 19) * updateRate) >> 1;
    } else {
        pitchInput >>= 1;
        obj->trans.rotation.x_rotation -= (obj->trans.rotation.x_rotation * updateRate) >> 4;
        obj->trans.rotation.x_rotation -= ((pitchInput * 30) * updateRate) >> 1;
    }

    if (racer->tappedR) {
        racer->tappedR = FALSE;
        if (racer->groundedWheels == 0 && racer->velocity < -6.5 && racer->waterTimer == 0) {
            if (gCurrentStickX > 40) {
                racer->trickType = -1;
            }
            if (gCurrentStickX < -40) {
                racer->trickType = 1;
            }
            if (gCurrentStickY > 40) {
                racer->trickType = -2;
            } else if (racer->trickType == 0) {
                racer->trickType = 2;
            }
            racer->spinDirection = 0;
            racer->spinRecoverTimer = 0;
        }
    }
}

/**
 * Apply the frame's forces: thrust (never below 40% in the air), braking, forward drag,
 * sideways drag, the sideways and upward push of a barrel roll, and vertical drag.
 */
static void plane_apply_forces(Object *obj, Object_Racer *racer, f32 throttle, f32 brake, f32 accel, f32 drag,
                               f32 lateralDrag, f32 pitchDrag) {
    f32 velSquared;
    f32 force;

    if (racer->groundedWheels == 0 && throttle < 0.4 && racer->vehicleID != VEHICLE_CARPET) {
        throttle = 0.4f;
    }
    throttle = throttle * accel;
    obj->x_velocity -= racer->ox1 * throttle;
    obj->y_velocity -= racer->oy1 * throttle;
    obj->z_velocity -= racer->oz1 * throttle;
    if (racer->groundedWheels >= 3 || racer->velocity < 1.0 || racer->vehicleID == VEHICLE_CARPET) {
        if (racer->groundedWheels == 0) {
            brake /= 2;
        }
        brake *= accel / 2;
        obj->x_velocity += racer->ox1 * brake;
        obj->y_velocity += racer->oy1 * brake;
        obj->z_velocity += racer->oz1 * brake;
    }
    velSquared = racer->velocity * racer->velocity;
    if (racer->velocity < 0.0f) {
        velSquared = -velSquared;
    }
    if (velSquared < 1.0f && !(gCurrentRacerInput & A_BUTTON)) {
        force = racer->velocity * drag * 8.0f;
    } else {
        force = velSquared * drag;
    }
    obj->x_velocity -= racer->ox1 * force;
    obj->y_velocity -= racer->oy1 * force;
    obj->z_velocity -= racer->oz1 * force;
    force = racer->lateral_velocity * racer->lateral_velocity * lateralDrag;
    if (racer->lateral_velocity < 0) {
        force = -force;
    }
    force += (racer->lateral_velocity * lateralDrag * 4.0f);
    obj->x_velocity -= racer->ox3 * force;
    obj->y_velocity -= racer->oy3 * force;
    obj->z_velocity -= racer->oz3 * force;
    if (racer->trickType == 1 || racer->trickType == -1) {
        velSquared = racer->velocity * 0.058823529411764705 * 1.5;
        force = coss_f(racer->x_rotation_vel) * velSquared * racer->trickType;
        if (racer->x_rotation_vel > 0x4000 || racer->x_rotation_vel < -0x4000) {
            force *= 2;
        }
        obj->x_velocity -= racer->ox3 * force;
        obj->y_velocity -= racer->oy3 * force;
        obj->z_velocity -= racer->oz3 * force;

        force = sins_f(racer->x_rotation_vel) * velSquared * racer->trickType * 1.5;
        obj->x_velocity -= racer->ox2 * force;
        obj->y_velocity -= racer->oy2 * force;
        obj->z_velocity -= racer->oz2 * force;
    }
    force = racer->vehiclePitch * racer->vehiclePitch * pitchDrag;
    if (racer->vehiclePitch < 0.0f) {
        force = -force;
    }
    force += racer->vehiclePitch * pitchDrag * 4.0f;
    obj->x_velocity -= racer->ox2 * force;
    obj->y_velocity -= racer->oy2 * force;
    obj->z_velocity -= racer->oz2 * force;

    racer->forwardVel -= (racer->forwardVel + (racer->velocity * 0.05)) * 0.125;
}

/**
 * Move the plane by its velocity plus half a bump and the push, or draw it towards the
 * object it is approaching. The horizontal velocity used is handed back, since a bumped
 * plane keeps it for next frame. Returns TRUE when a human player's plane moved.
 */
static s32 plane_move(Object *obj, Object_Racer *racer, f32 updateRateF, f32 *velX, f32 *velZ) {
    if (racer->approachTarget != NULL) {
        racer_approach_object(obj, racer, updateRateF);
        return FALSE;
    }
    *velX = obj->x_velocity;
    *velZ = obj->z_velocity;
    if (racer->bumpTimer != 0) {
        *velX += racer->bumpVelX * 0.5;
        *velZ += racer->bumpVelZ * 0.5;
    }
    if (gRacerInputBlocked) {
        if (*velX > 0.5 || *velX < -0.5) {
            *velX *= 0.65;
        } else {
            *velX = 0.0f;
        }
        if (*velZ > 0.5 || *velZ < -0.5) {
            *velZ *= 0.65;
        } else {
            *velZ = 0.0f;
        }
    } else {
        *velX += racer->pushVelX;
        *velZ += racer->pushVelZ;
    }
    return move_object(obj, *velX * updateRateF, obj->y_velocity * updateRateF, *velZ * updateRateF) &&
           gCurrentPlayerIndex != PLAYER_COMPUTER;
}

/**
 * Collision for the frame: AI planes use their own physics (a rocket boss stays airborne),
 * and a tumbling plane that touches down crashes.
 */
static void plane_collide(Object *obj, Object_Racer *racer, s32 updateRate) {
    s32 wasGrounded;

    wasGrounded = racer->groundedWheels;
    if (gCurrentPlayerIndex == PLAYER_COMPUTER) {
        if (racer->vehicleIDPrev != VEHICLE_ROCKET || gRaceStartTimer != 0) {
            onscreen_ai_racer_physics(obj, racer, updateRate);
        } else {
            racer->groundedWheels = 0;
            racer->wheelContactFlags = 0;
        }
    } else {
        update_racer_collision(obj, racer, updateRate);
    }
    if (wasGrounded == 0 && racer->groundedWheels != 0 && racer->spinout_timer != 0) {
        racer_play_sound(obj, SOUND_CRASH);
        if (racer->playerIndex != PLAYER_COMPUTER) {
            gCameraObject->shakeMagnitude = 6.0f;
        }
    }
}

/**
 * The propeller spins, and the wheels drop when the plane is on or near the ground and
 * retract into the body in flight.
 */
static void plane_update_attachments(Object *obj, Object_Racer *racer, s32 wheelsDown) {
    Object *part;
    s32 i;

    if (obj->attachPoints == NULL || obj->attachPoints->count < 3) {
        return;
    }
    part = obj->attachPoints->obj[2];
    part->trans.rotation.y_rotation = 0x4000;
    part->modelIndex += 1;
    if (part->modelIndex == part->header->numberOfModelIds) {
        part->modelIndex = 0;
    }
    for (i = 0; i < 2; i++) {
        part = obj->attachPoints->obj[i];
        if (racer->groundedWheels != 0 || wheelsDown != FALSE) {
            if (part->trans.y_position > 0.0f) {
                part->trans.y_position = part->trans.y_position - 2.0;
            } else {
                part->trans.y_position = 0.0f;
            }
            part->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
        } else {
            if (part->trans.y_position < 20.0f) {
                part->trans.y_position = part->trans.y_position + 1.0f;
            } else {
                part->trans.flags |= OBJ_FLAGS_INVISIBLE;
            }
        }
    }
}

/**
 * Controls: the steer angle eases towards the stick (twice as fast during a trick) and the
 * pitch input towards the stick's Y; then items, tilt, attacks and head turning.
 */
static void plane_read_controls(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    s32 steerDiff;
    s32 steerStep;

    racer->steerAngle += ease_step(gCurrentStickX - racer->steerAngle, updateRateF, racer->trickType != 0 ? 4.0 : 8.0);
    steerDiff = gCurrentStickY - racer->smoothedStickY;
    steerStep = steerDiff * updateRateF * 0.0625;
    if (steerDiff != 0 && steerStep == 0) {
        if (steerDiff > 0) {
            steerStep = 1;
        }
        if (steerDiff < 0) {
            steerStep = -1;
        }
    }
    racer->smoothedStickY += steerStep;
    handle_racer_items(obj, racer, updateRate);
    racer_update_tilt_vector(obj, racer);
    racer_attack_handler_plane(obj, racer);
    if (gCurrentPlayerIndex != PLAYER_COMPUTER) {
        handle_racer_head_turning(obj, racer, updateRate);
    } else {
        slowly_reset_head_angle(racer);
    }
}

/**
 * Eases the yaw and roll velocities toward this frame's targets and applies them.
 */
static void plane_update_rotation(Object *obj, Object_Racer *racer) {
    racer->driftHeadingOffset = 0;
    racer->y_rotation_vel += (gCurrentCarSteerVel - racer->y_rotation_vel) >> 3;
    obj->trans.rotation.y_rotation = racer->steerVisualRotation + racer->y_rotation_vel;
    racer->z_rotation_vel += (gRacerTargetRoll - racer->z_rotation_vel) >> 3;
    obj->trans.rotation.z_rotation = racer->x_rotation_vel + racer->z_rotation_vel;
}

/**
 * The velocity for next frame is whatever movement survived collision, except while bumped,
 * when the horizontal velocity is kept as it was (and the bump timer counts down).
 */
static void plane_next_velocity(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF, f32 xPos, f32 yPos,
                                f32 zPos, f32 *velXInOut, f32 *velZInOut) {
    f32 velX;
    f32 velZ;
    f32 invRate;

    velX = *velXInOut;
    velZ = *velZInOut;
    if (racer->bumpTimer != 0) {
        racer->bumpTimer -= updateRate;
        if (racer->bumpTimer < 0) {
            racer->bumpTimer = 0;
        }
    } else {
        invRate = 1.0f / updateRateF;
        velX = (obj->trans.x_position - xPos - gRacerExternalShiftX) * invRate;
        obj->y_velocity = (obj->trans.y_position - yPos) * invRate;
        velZ = (obj->trans.z_position - zPos - gRacerExternalShiftZ) * invRate;
    }
    if (gRaceStartTimer == 100) {
        obj->y_velocity = -5.0f;
    }
    *velXInOut = velX;
    *velZInOut = velZ;
}

/**
 * Boost exhaust for a human's plane, no emitters for computer racers or without full physics,
 * then the vehicle particles.
 */
static void plane_update_emitters(Object *obj, Object_Racer *racer, s32 updateRate) {
    if (gCurrentPlayerIndex != PLAYER_COMPUTER && racer->boostTimer == 0 && gNumViewports < 2) {
        racer_enable_boost_emitter(obj, racer, 9);
    }
    if (gCurrentPlayerIndex == PLAYER_COMPUTER) {
        obj->particleEmittersEnabled = OBJ_EMIT_NONE;
    }
    if (racer->aiPhysicsTimer == 0) {
        obj->particleEmittersEnabled = OBJ_EMIT_NONE;
    }
    if (racer->vehicleIDPrev < VEHICLE_BOSSES) {
        update_vehicle_particles(obj, updateRate);
    }
}

/**
 * One frame of plane physics: skim the waves, read the controls, work out lift and drag,
 * handle spin-outs, tricks or normal steering, apply the forces, move, collide and derive
 * the velocities for next frame.
 */
void update_plane(s32 updateRate, f32 updateRateF, Object *obj, Object_Racer *racer) {
    f32 throttle;
    f32 brake;
    f32 accel;
    f32 drag;
    f32 lateralDrag;
    f32 pitchDrag;
    f32 liftFactor;
    f32 speed;
    f32 speedCap;
    f32 velX;
    f32 velZ;
    f32 xPos;
    f32 yPos;
    f32 zPos;
    s32 nearCeiling;
    s32 wheelsDown;
    s32 rHeldOnGround;
    s32 playerMoved;

    if (is_nondefault_vehicle()) {
        updateRateF *= 1.09;
    }
    if (racer->groundedWheels > 0) {
        racer->pushVelX = 0.0f;
        racer->pushVelZ = 0.0f;
    }
    if (racer->effectBoxType == 4 && racer->spinout_timer == 0) {
        sound_play(SOUND_ZAP4, NULL);
        racer->spinout_timer = 20;
    }
    wheelsDown = plane_update_wave_skim(obj, racer, updateRateF);
    gRacerSurfaceSoundId = 0;
    gCurrentCarSteerVel = 0;
    gRacerTargetRoll = 0;
    xPos = obj->trans.x_position;
    yPos = obj->trans.y_position;
    zPos = obj->trans.z_position;

    // Controls.
    plane_read_controls(obj, racer, updateRate, updateRateF);
    racer_ease_throttle(racer, updateRateF, 0.01);
    if (racer->exitObj) {
        racer->throttle = 0.5;
    }
    throttle = racer->throttle;
    if (racer_ease_brake(racer, updateRateF, 0.016) && racer->velocity < -2.0 && racer->groundedWheels >= 2) {
        rumble_set(racer->playerIndex, RUMBLE_TYPE_3);
    }
    brake = racer->brake;
    racer_update_basis_vectors(obj, racer);
    if (racer->approachTarget == NULL) {
        apply_plane_tilt_anim(updateRate, obj, racer);
    }
    if (racer->playerIndex == PLAYER_COMPUTER && gCurrentPlayerIndex != PLAYER_COMPUTER) {
        gCurrentRacerHandlingStat = 1.4f;
    }

    // Lift, the speed curve and the drag coefficients.
    liftFactor = plane_lift_factor(obj, racer, &nearCeiling);
    speed = racer->velocity;
    if (speed < 0) {
        speed = -speed;
    }
    speedCap = racer->velocity;
    if (speedCap < 0.0f) {
        speedCap = -speedCap;
    }
    if (speed > (speedCap + 4)) {
        speed = speedCap + 3;
    }
    if (speed > 12.0f) {
        speed = 12.0f;
    }
    accel = racer_speed_curve(speed);
    lateralDrag = 0.01f;
    pitchDrag = 0.02f;
    drag = 0.004f;
    if (racer->groundedWheels != 0) {
        plane_on_ground(obj, racer, liftFactor, &throttle, &brake, &drag, &lateralDrag, &pitchDrag);
    }
    plane_apply_gravity(obj, racer, updateRate, liftFactor, nearCeiling);
    if (racer->zipperDirCorrection != 0 && racer->spinout_timer == 0) {
        racer->magnetTimer = 0;
        racer->spinout_timer = 0;
        racer->trickType = 0;
        racer_zipper_align(obj, racer, updateRate);
    }

    // Attitude: spin-out, trick, or steering.
    rHeldOnGround = FALSE;
    if (racer->spinout_timer != 0) {
        plane_update_spinout(obj, racer, updateRate);
    } else if (racer->trickType == 1 || racer->trickType == -1) {
        plane_update_barrel_roll(racer, updateRate, &throttle);
    } else if (racer->trickType == 2 || racer->trickType == -2) {
        plane_update_loop(obj, racer, updateRate);
    } else {
        plane_update_steering(obj, racer, updateRate, &rHeldOnGround);
    }
    if (gCurrentPlayerIndex != PLAYER_COMPUTER && racer->velocity < -4.0f) {
        if ((obj->particleEmittersEnabled & (OBJ_EMIT_7 | OBJ_EMIT_8)) != (OBJ_EMIT_7 | OBJ_EMIT_8)) {
            obj->particleEmittersEnabled |= OBJ_EMIT_3 | OBJ_EMIT_4;
        }
    }
    speedCap = handle_racer_top_speed(obj, racer);
    accel = accel * speedCap;
    accel *= 1.8;
    if (racer->boostTimer > 0) {
        if (gRaceStartTimer == 0) {
            racer->throttle = 1;
            accel = 2.0f;
            racer->boostTimer -= updateRate;
            obj->particleEmittersEnabled |= OBJ_EMIT_7 | OBJ_EMIT_8;
        }
    } else {
        racer->boostTimer = 0;
    }
    if (racer->zipperDirCorrection == 0 && gRaceStartTimer == 0) {
        plane_apply_forces(obj, racer, throttle, brake, accel, drag, lateralDrag, pitchDrag);
    }
    plane_update_rotation(obj, racer);
    if (racer->magnetTimer != 0) {
        obj->x_velocity = gRacerMagnetVelX;
        obj->z_velocity = gRacerMagnetVelZ;
    }
    playerMoved = plane_move(obj, racer, updateRateF, &velX, &velZ);
    plane_collide(obj, racer, updateRate);
    plane_next_velocity(obj, racer, updateRate, updateRateF, xPos, yPos, zPos, &velX, &velZ);
    obj->x_velocity = velX;
    obj->z_velocity = velZ;
    racer_velocity_to_local(obj, racer, FALSE);
    plane_update_attachments(obj, racer, wheelsDown);
    plane_update_emitters(obj, racer, updateRate);
    if (rHeldOnGround != FALSE) {
        gCurrentRacerInput |= R_TRIG;
    }
    second_racer_camera_update(obj, racer, CAMERA_PLANE, updateRateF);
    if (playerMoved != FALSE) {
        racer_find_start_node(obj, racer);
    }
}

/**
 * When turning left and right in a plane, apply the tilting animation to the character.
 * There's a few safeguards in place to ensure the animation frame is not out of bounds.
 */
void apply_plane_tilt_anim(s32 updateRate, Object *obj, Object_Racer *racer) {
    s32 animDiff;
    s32 animAdd;

    if (racer->vehicleIDPrev != VEHICLE_CARPET) {
        //!@bug Typo. Should've been `== 0`, not `= 0`.
        if ((racer->driveAnimState = 0)) {
            return; // This never gets called because of the typo.
        }
        animAdd = racer->steerAngle;
        animAdd = 40 - (animAdd >> 1);
        if (animAdd < 0) {
            animAdd = 0;
        }
        if (animAdd > 73) {
            animAdd = 73;
        }
        animDiff = animAdd - obj->animFrame;
        animAdd = 0;
        if (animDiff > 0) {
            animAdd = updateRate * 3;
            if (animDiff < animAdd) {
                animAdd = animDiff;
            }
        }
        if (animDiff < 0) {
            animAdd = updateRate * -3;
            if (animAdd < animDiff) {
                animAdd = animDiff;
            }
        }
        obj->animFrame += animAdd; //!@Delta
    }
}

/**
 * Initialise some states when a racer is attacked or runs into something.
 * While the majority of the respondent behaviour takes place elsewhere, the squish behaviour happens here.
 * Planes do not have a spinning state, instead being in the same state when hit by a rocket.
 */
void racer_attack_handler_plane(Object *obj, Object_Racer *racer) {
    s8 bananas;
    s8 attackType;
    if (racer->playerIndex == PLAYER_COMPUTER) {
        bananas = 0;
    } else {
        bananas = racer->bananas;
    }
    attackType = racer->attackType;
    if (attackType == ATTACK_NONE || racer->shieldTimer > 0) {
        racer->attackType = ATTACK_NONE;
        return;
    }
    if (attackType != ATTACK_SQUISHED) {
        drop_bananas(obj, racer, 2);
    }
    racer->hitRecoveryTimer = 360;
    if (racer->aiBehaviour == 8) {
        racer->aiBehaviour = 0;
    }
    if (racer->vehicleID < VEHICLE_BOSSES) {
        play_random_character_voice(obj, SOUND_VOICE_CHARACTER_NEGATIVE, 8, 129);
        switch (racer->attackType) {
            case ATTACK_EXPLOSION:
            case ATTACK_SPIN:
                if (bananas != 0) {
                    racer->spinout_timer = 40;
                } else {
                    racer->spinout_timer = 60;
                }
                break;
            case ATTACK_UNK3:
                if (bananas != 0) {
                    racer->spinout_timer = 40;
                } else {
                    racer->spinout_timer = 60;
                }
                break;
            case ATTACK_BUBBLE:
                racer->bubbleTrapTimer = 120;
                obj->x_velocity *= 0.7;
                obj->z_velocity *= 0.7;
                break;
        }
        racer->attackType = ATTACK_NONE;
    }
}

/**
 * Swings the camera yaw round behind the plane, faster the longer it has been turning,
 * unless R is held on the ground (or the plane is being lined up on a zipper). Faces the
 * plane head on during dialogue.
 */
static void plane_camera_follow_yaw(Object_Racer *racer, s32 delta) {
    s32 angleVel;
    s32 angle;

    if (((!(gCurrentRacerInput & 0x10)) || racer->groundedWheels < 3) && !racer->zipperDirCorrection) {
        angle = (-racer->steerVisualRotation - (racer->cameraYaw & 0xFFFF)) + 0x8000;
        if (angle > 0x8000) {
            angle -= 0xFFFF;
        }
        if (angle < -0x8000) {
            angle += 0xFFFF;
        }
        if (racer->camera_zoom < 0.4) {
            racer->camera_zoom += 1 / 180.0;
        } else {
            racer->camera_zoom = 0.4f;
        }
        angleVel = angle * racer->camera_zoom;
        if (angleVel > 0x200) {
            angleVel = 0x200;
        }
        if (angleVel < -0x200) {
            angleVel = -0x200;
        }
        angleVel *= delta;
        if (angle < 0 && angleVel < angle) {
            angleVel = angle;
        }
        if (angle > 0 && angle < angleVel) {
            angleVel = angle;
        }
        racer->cameraYaw += angleVel;
    } else {
        racer->camera_zoom = 0.0f;
    }
    if (gDialogueCameraAngle) {
        racer->cameraYaw = 0x8000 - racer->steerVisualRotation;
    }
}

/**
 * Pitches the camera with the plane (level during a loop), tilting it further down the
 * closer the plane is to the course's ceiling.
 */
static void plane_camera_pitch(Object *obj, Object_Racer *racer, f32 heightFactor, s32 delta) {
    s32 angleVel;
    s32 angle;

    if (racer->trickType == 2 || racer->trickType == -2) {
        angle = 0;
    } else {
        angle = obj->trans.rotation.x_rotation;
        if (angle > 0x3000) {
            angle = 0x3000;
        }
        if (angle < -0x3000) {
            angle = -0x3000;
        }
    }
    angle = -(angle - ((s32) (heightFactor * 10.0f)));
    angleVel = angle - ((u16) gCameraObject->trans.rotation.x_rotation);
    if (angleVel > 0x8000) {
        angleVel -= 0xFFFF;
    }
    if (angleVel < -0x8000) {
        angleVel += 0xFFFF;
    }
    gCameraObject->trans.rotation.x_rotation += ((angleVel * delta) >> 4);
}

/**
 * Follows the plane's height, loosely: the damping drops sharply during a trick and then
 * recovers, and the camera never overshoots the target height.
 */
static void plane_camera_follow_height(Object *obj, Object_Racer *racer, f32 updateRate, f32 heightOffset) {
    f32 yOffset;
    f32 fullOffset;

    yOffset = gCameraObject->trans.y_position - (obj->trans.y_position + heightOffset);
    fullOffset = yOffset;
    if (racer->trickType == 1 || racer->trickType == -1) {
        racer->cameraYDamping = 8.0;
    }
    if (racer->trickType == 2 || racer->trickType == -2) {
        racer->cameraYDamping = 8.0;
    }
    if (racer->cameraYDamping > 2.0) {
        racer->cameraYDamping -= 0.2;
    } else {
        racer->cameraYDamping = 2.0;
    }
    yOffset = (yOffset * updateRate) / racer->cameraYDamping;
    if (yOffset > 0.0f && fullOffset < yOffset) {
        yOffset = fullOffset;
    }
    if (yOffset < 0.0f && yOffset < fullOffset) {
        yOffset = fullOffset;
    }
    gCameraObject->trans.y_position -= yOffset;
}

/**
 * Levels the camera's roll during tricks and dialogue, otherwise rolls it with the plane.
 */
static void plane_camera_roll(Object_Racer *racer) {
    s32 angleVel;
    s32 angle;

    if (racer->trickType || gDialogueCameraAngle) {
        angleVel = -(u16) gCameraObject->trans.rotation.z_rotation;
        if (angleVel > 0x8000) {
            angleVel -= 0xFFFF;
        }
        if (angleVel < -0x8000) {
            angleVel += 0xFFFF;
        }
        gCameraObject->trans.rotation.z_rotation += angleVel >> 2;
    } else {
        angle = racer->x_rotation_vel;
        angleVel = (angle >> 4) - (u16) gCameraObject->trans.rotation.z_rotation;
        if (angleVel > 0x8000) {
            angleVel -= 0xFFFF;
        }
        if (angleVel < -0x8000) {
            angleVel += 0xFFFF;
        }
        gCameraObject->trans.rotation.z_rotation += angleVel >> 3;
    }
}

/**
 * Handles the camera movement when the player is flying a plane: a boom behind the plane that
 * follows its yaw, pitch, height and roll, longer with speed and pulled in by braking.
 */
void update_camera_plane(f32 updateRate, Object *obj, Object_Racer *racer) {
    s32 segmentIndex;
    f32 baseSpeed;
    f32 targetBoom = 120.0f;
    f32 targetHeight = 45.0f;
    f32 xOffset;
    f32 zOffset;
    f32 heightFactor;
    f32 brakePull;
    f32 boomFlat;
    f32 sideOffset;
    f32 pitchSin;
    f32 brakeVar = 0.0f;
    s32 numViewports;
    s32 delta;

    delta = (s32) updateRate;
    heightFactor = gCurrentCourseHeight - obj->trans.y_position;
    heightFactor = 200.0f - (heightFactor);
    if (heightFactor < 0.0f) {
        heightFactor = 0.0f;
    }
    if (heightFactor > 200.0f) {
        heightFactor = 200.0f;
    }
    numViewports = cam_get_viewport_layout();
    if (numViewports == 1) {
        targetBoom = 180.0f;
        targetHeight = 50.0f;
    } else if (numViewports > 1) {
        targetBoom = 110.0f;
        targetHeight = 42.0f;
    }
    plane_camera_follow_yaw(racer, delta);
    plane_camera_pitch(obj, racer, heightFactor, delta);
    brakeVar = racer->brake;
    baseSpeed = racer->forwardVel;
    switch (gCameraObject->zoom) {
        case 1:
            targetBoom += 35.0f;
            break;
        case 2:
            targetHeight -= 10.0f;
            targetBoom -= 35.0f;
            break;
        case 3:
            targetBoom -= 53.0f;
            targetHeight -= 5.0f;
            baseSpeed *= 0.25;
            brakeVar = 0.0f;
            break;
    }
    if (numViewports < 2) {
        targetBoom += baseSpeed * 60.0f;
    } else {
        targetBoom += baseSpeed * 30.0f;
    }
    if (racer->velocity < 0.0 && !racer->groundedWheels) {
        brakePull = -(racer->velocity * brakeVar) * 6.0f;
        if (65.0 < brakePull) {
            brakePull = 65.0f;
        }
        targetBoom -= brakePull;
    }
    if (!gRaceStartTimer) {
        if (normalise_time(36) < racer->boostTimer) {
            targetBoom = -30.0f;
        } else if (racer->boostTimer > 0) {
            targetBoom = 180.0f;
        }
    }
    if (gRaceStartTimer > 80) {
        gCameraObject->boomLength = targetBoom;
        gCameraObject->cam_unk_20 = targetHeight;
    }
    gCameraObject->boomLength += (targetBoom - gCameraObject->boomLength) * 0.125;
    gCameraObject->cam_unk_20 += (targetHeight - gCameraObject->cam_unk_20) * 0.125;
    pitchSin = sins_f(gCameraObject->trans.rotation.x_rotation - 0x400);
    xOffset = coss_f(gCameraObject->trans.rotation.x_rotation - 0x400);
    boomFlat = (gCameraObject->boomLength * xOffset) - (gCameraObject->cam_unk_20 * pitchSin);
    targetHeight = ((gCameraObject->boomLength) * pitchSin) + (gCameraObject->cam_unk_20 * xOffset);
    xOffset = sins_f(0x8000 - racer->cameraYaw) * boomFlat;
    zOffset = coss_f(0x8000 - racer->cameraYaw) * boomFlat;
    sideOffset = sins_f(racer->cameraYaw + 0x4000) * 3.0f * racer->lateral_velocity;
    gCameraObject->trans.x_position = (obj->trans.x_position + xOffset) + sideOffset;
    plane_camera_follow_height(obj, racer, updateRate, targetHeight);
    sideOffset = (-coss_f(racer->cameraYaw + 0x4000) * 3.0f * racer->lateral_velocity);
    gCameraObject->trans.z_position = zOffset + obj->trans.z_position + sideOffset;
    gCameraObject->trans.rotation.y_rotation = racer->cameraYaw;
    plane_camera_roll(racer);
    segmentIndex = get_level_segment_index_from_position(
        gCameraObject->trans.x_position, gCameraObject->trans.y_position, gCameraObject->trans.z_position);
    if (segmentIndex != -1) {
        gCameraObject->cameraSegmentID = segmentIndex;
    }
    racer->cameraYaw = gCameraObject->trans.rotation.y_rotation;
}
