#include "racer_internal.h"

/**
 * Hovercraft spin-outs turn the craft on the spot: unk1D4 ramps to +/-10 while the spin
 * lasts and drives the heading offset, then spinRecoverTimer eases the offset back. Braking at
 * speed also yaws the craft.
 */
static void hovercraft_update_spinout(Object *obj, Object_Racer *racer, s32 updateRate, f32 brakeForce) {
    f32 speedFrac;
    s32 prevOffset;

    speedFrac = -racer->velocity;
    if (speedFrac < 0.0f) {
        speedFrac = 0.0f;
    }
    if (speedFrac > 14.0) {
        speedFrac = 14.0f;
    }
    speedFrac /= 14.0f;
    if (racer->spinDirection != 0 || racer->spinRecoverTimer != 0) {
        apply_vehicle_rotation_offset(racer, updateRate, racer->y_rotation_offset, 0, 0);
    } else {
        apply_vehicle_rotation_offset(racer, updateRate, 0, (brakeForce * 4096.0f) * speedFrac, 0);
    }
    if (racer->spinRecoverTimer != 0) {
        racer->y_rotation_offset -= racer->y_rotation_offset >> 3;
        if (racer->y_rotation_offset < 16 && racer->y_rotation_offset > -16) {
            racer->spinRecoverTimer = 0;
        }
        gCurrentStickX = 0;
    }
    if (racer->spinout_timer > 0) {
        racer->spinDirection += updateRate;
        if (racer->spinDirection > 10) {
            racer->spinDirection = 10;
        }
    } else if (racer->spinout_timer < 0) {
        racer->spinDirection -= updateRate;
        if (racer->spinDirection < -10) {
            racer->spinDirection = -10;
        }
    } else if (racer->spinDirection < 0) {
        racer->spinDirection += updateRate;
        if (racer->spinDirection > 0) {
            racer->spinDirection = 0;
            racer->spinRecoverTimer = 1;
        }
    } else if (racer->spinDirection > 0) {
        racer->spinDirection -= updateRate;
        if (racer->spinDirection < 0) {
            racer->spinDirection = 0;
            racer->spinRecoverTimer = 1;
        }
    }
    if (racer->groundedWheels && racer->groundedWheels < 3) {
        obj->trans.rotation.x_rotation -= (obj->trans.rotation.x_rotation * updateRate) >> 6;
        racer->x_rotation_vel -= (racer->x_rotation_vel * updateRate) >> 6;
    }
    if (racer->spinDirection != 0) {
        prevOffset = racer->y_rotation_offset;
        racer->spinRecoverTimer = 0;
        if (racer->spinDirection > 0) {
            racer->y_rotation_offset += (racer->spinDirection * (180 - 1)) * updateRate;
            if (prevOffset < -0x1000 && racer->y_rotation_offset >= -0x1000) {
                racer->spinout_timer--;
            }
        }
        if (racer->spinDirection < 0) {
            racer->y_rotation_offset -= (racer->spinDirection * (180 - 1)) * updateRate;
            if (prevOffset > 0x1000 && racer->y_rotation_offset <= 0x1000) {
                racer->spinout_timer++;
            }
        }
        gCurrentRacerInput &= ~(A_BUTTON | B_BUTTON);
        gCurrentStickX = 0;
        racer->steerAngle = 0;
        gCurrentStickY = 0;
    }
    if (racer->launchedState != 0) {
        racer->y_rotation_offset += updateRate * 0x500;
        racer->x_rotation_offset += updateRate * 0x600;
    }
}

/**
 * Landing after air time: a thud for the player, a splash on water, and a squash of the
 * character that grows with the time spent airborne (trickType counts that time).
 */
static void hovercraft_land(Object *obj, Object_Racer *racer) {
    f32 squash;

    if (!(racer->groundedWheels || racer->buoyancy > 0.0f)) {
        return;
    }
    if (racer->trickType == 0) {
        return;
    }
    if (racer->groundedWheels) {
        if (gRaceStartTimer != 100 && racer->playerIndex != PLAYER_COMPUTER && racer->trickType >= 6) {
            sound_play(SOUND_BOUNCE, &racer->landingSoundHandle);
            sound_volume_set_relative(SOUND_BOUNCE, racer->landingSoundHandle, (racer->trickType * 2) + 50);
        }
    } else if (racer->approachTarget == NULL) {
        racer_play_sound(obj, SOUND_UNK_AF);
        obj->particleEmittersEnabled |= OBJ_EMIT_5 | OBJ_EMIT_6;
    }
    squash = racer->trickType;
    squash -= 5.0f;
    if (squash > 0.0f && racer->approachTarget == NULL) {
        racer->stretch_height_cap = 1.0 - (squash * 0.02);
    }
    racer->trickType = 0;
}

/**
 * The camera yaw trails the direction of travel, and the craft's heading trails the camera,
 * both faster the faster it goes and when accelerating or sliding.
 */
static void hovercraft_update_camera_yaw(Object *obj, Object_Racer *racer, s32 updateRate) {
    f32 speed;
    f32 blend;
    f32 yawScale;
    s32 diff;
    s32 step;

    speed = sqrtf((obj->z_velocity * obj->z_velocity) + (obj->x_velocity * obj->x_velocity));
    if (speed > 0.25) {
        racer->velocityHeading = arctan2_f(obj->x_velocity, obj->z_velocity) + 0x8000;
        diff = ((0x8000 - racer->velocityHeading) & 0xFFFF) - (racer->cameraYaw & 0xFFFF);
        if (diff > 0x8000) {
            diff -= 0xFFFF;
        }
        if (diff < -0x8000) {
            diff += 0xFFFF;
        }
        blend = speed - 0.25;
        if (blend > 8.0) {
            blend = 8.0f;
        }
        blend *= 0.125;
        diff = (blend * diff) * 0.75;
        racer->cameraYaw += (diff * updateRate) >> 4;
        diff = (0x8000 - racer->cameraYaw) - (racer->steerVisualRotation & 0xFFFF);
        if (diff > 0x8000) {
            diff -= 0xFFFF;
        }
        if (diff < -0x8000) {
            diff += 0xFFFF;
        }
        if (speed > 10.0) {
            speed = 10.0;
        }
        speed /= 10.0;
        diff *= speed;
        if (gCurrentRacerInput & A_BUTTON || gCurrentRacerInput & R_TRIG) {
            diff *= 2;
        }
        racer->steerVisualRotation += ((s32) (diff * updateRate)) >> 7;
    }
    diff = (0x8000 - racer->steerVisualRotation) - (racer->cameraYaw & 0xFFFF);
    if (diff > 0x8000) {
        diff -= 0xFFFF;
    }
    if (diff < -0x8000) {
        diff += 0xFFFF;
    }
    yawScale = 1.0f;
    diff = ((s32) ((((f32) diff) * 1.75) * yawScale));
    step = (diff * updateRate);
    racer->cameraYaw += step >> 6;
}

/**
 * Apply the frame's forces: sideways slip (AI racers slide less and use the slide button),
 * throttle, brake, the tilt of the ground, drag, and the magnet override.
 */
static void hovercraft_apply_forces(Object *obj, Object_Racer *racer, f32 updateRateF, f32 throttleForce,
                                    f32 brakeForce, f32 accel, f32 drag) {
    f32 slip;
    f32 force;
    f32 pitchForce;
    f32 rollForce;
    f32 velSquared;

    if (gCurrentPlayerIndex == PLAYER_COMPUTER && !racer->raceFinished) {
        force = racer->lateral_velocity * 0.25;
        slip = force * 0.35;
        if (slip < 0.0) {
            slip = -slip;
        }
        if (0.2 < slip) {
            gCurrentRacerInput |= R_TRIG;
        }
        obj->x_velocity += racer->ox1 * slip;
        obj->y_velocity += racer->oy1 * slip;
        obj->z_velocity += racer->oz1 * slip;
    } else {
        force = racer->lateral_velocity * 0.07;
        if (gCurrentRacerInput & R_TRIG) {
            force *= 2;
            if (gCurrentStickX > -40 && gCurrentStickX < 40) {
                throttleForce *= 0.8;
            } else if (racer->lateral_velocity > 2.5 || racer->lateral_velocity < -2.5) {
                throttleForce *= 1.5;
            }
            if (gCurrentRacerInput & B_BUTTON) {
                force *= 2;
            }
        }
    }
    obj->x_velocity -= racer->ox3 * force;
    obj->y_velocity -= racer->oy3 * force;
    obj->z_velocity -= racer->oz3 * force;
    throttleForce *= accel;
    obj->x_velocity -= racer->ox1 * throttleForce;
    obj->y_velocity -= racer->oy1 * throttleForce;
    obj->z_velocity -= racer->oz1 * throttleForce;
    brakeForce *= accel / 2;
    obj->x_velocity += racer->ox1 * brakeForce;
    obj->y_velocity += racer->oy1 * brakeForce;
    obj->z_velocity += racer->oz1 * brakeForce;
    if (racer->groundedWheels && !gRacerInputBlocked && gRaceStartTimer == 0) {
        rollForce = (gCurrentRacerWeightStat * updateRateF) * 0.25;
        pitchForce = racer->pitch * rollForce;
        rollForce *= racer->roll;
        obj->x_velocity += (racer->ox1 * pitchForce) + (racer->ox3 * rollForce);
        obj->y_velocity += (racer->oy1 * pitchForce) + (racer->oy3 * rollForce);
        obj->z_velocity += (racer->oz1 * pitchForce) + (racer->oz3 * rollForce);
    }
    velSquared = racer->velocity * racer->velocity;
    if (racer->velocity < 0.0f) {
        velSquared = -velSquared;
    }
    if (velSquared < 1.0f && (!(gCurrentRacerInput & A_BUTTON))) {
        force = racer->velocity * drag * 8.0f;
    } else {
        force = velSquared * drag;
    }
    obj->x_velocity -= racer->ox1 * force;
    obj->y_velocity -= racer->oy1 * force;
    obj->z_velocity -= racer->oz1 * force;
    if (racer->magnetTimer != 0) {
        obj->x_velocity = gRacerMagnetVelX;
        obj->z_velocity = gRacerMagnetVelZ;
    }
}

/**
 * Move the craft by its velocity plus any bump and push, or draw it towards the object it
 * is approaching. Returns TRUE when a human player's craft moved.
 */
static s32 hovercraft_move(Object *obj, Object_Racer *racer, f32 updateRateF, f32 *shiftX, f32 *shiftZ) {
    f32 velX;
    f32 velZ;

    if (racer->approachTarget != NULL) {
        *shiftX = 0.0;
        *shiftZ = 0.0;
        racer_approach_object(obj, racer, updateRateF);
        return FALSE;
    }
    velX = obj->x_velocity;
    velZ = obj->z_velocity;
    if (racer->bumpTimer != 0) {
        velX += racer->bumpVelX;
        velZ += racer->bumpVelZ;
    }
    if (gRacerInputBlocked) {
        if (velX > 0.5 || velX < -0.5) {
            velX *= 0.65;
        } else {
            velX = 0.0f;
        }
        if (velZ > 0.5 || velZ < -0.5) {
            velZ *= 0.65;
        } else {
            velZ = 0.0f;
        }
    } else {
        *shiftX += racer->pushVelX * updateRateF;
        *shiftZ += racer->pushVelZ * updateRateF;
    }
    return move_object(obj, (velX * updateRateF) + *shiftX, obj->y_velocity * updateRateF,
                       (velZ * updateRateF) + *shiftZ) &&
           gCurrentPlayerIndex != PLAYER_COMPUTER;
}

/**
 * Buoyancy from the waves under the craft, the water-contact interaction flag, and the
 * rotation to the wave surface. Returns how far above the water line the craft sits.
 */
static f32 hovercraft_update_water(Object *obj, Object_Racer *racer, s32 updateRate) {
    Vec3f waterRotation;
    f32 waterHeight;
    f32 sink;
    s8 waveType;
    s32 steer;

    sink = 5.0f;
    waterHeight = -10000.0f;
    waveType = get_wave_properties(obj->trans.y_position, &waterHeight, &waterRotation);
    if (waveType != SURFACE_DEFAULT) {
        sink = racer->velocity;
        if (sink < 0.0f) {
            sink = -sink;
        }
        if (sink > 4.0) {
            sink = 4.0f;
        }
        sink *= 0.8;
        if ((obj->trans.y_position - sink) < waterHeight) {
            racer->buoyancy = waterHeight - (obj->trans.y_position - sink);
            racer->waterTimer = 5;
        } else {
            racer->buoyancy = 0.0f;
        }
    }
    if (racer->waterTimer > 0) {
        racer->waterTimer--;
    } else {
        racer->buoyancy = 0.0f;
    }
    if (racer->waterTimer > 0 && obj->trans.y_position < (waterHeight + sink)) {
        obj->interactObj->flags |= INTERACT_FLAGS_UNK_0010;
    } else {
        obj->interactObj->flags &= ~INTERACT_FLAGS_UNK_0010;
    }
    waterHeight = obj->trans.y_position - (waterHeight + sink);
    if (racer->buoyancy > 0.0) {
        steer = 0;
        if (gCurrentRacerInput & R_TRIG) {
            steer = gCurrentStickX * 2;
        }
        rotate_racer_in_water(obj, racer, &waterRotation, waveType, updateRate, steer, 1.0f);
    }
    if (racer->buoyancy > 0.0) {
        obj->trans.y_position += racer->buoyancy * racer->waterBobScale;
        racer->waterBobScale += (0.75 - racer->waterBobScale) * 0.125;
    }
    return waterHeight;
}

/**
 * Spray and boost emitters for the player's craft, then the particle update itself.
 */
static void hovercraft_update_particles(Object *obj, Object_Racer *racer, s32 updateRate) {
    f32 speedSquared;

    if (gNumViewports < 2 && obj->header->particleCount >= 9) {
        if ((gCurrentRacerInput & (A_BUTTON | R_TRIG)) == (A_BUTTON | R_TRIG) &&
            (gCurrentStickX < -30 || gCurrentStickX > 30)) {
            increase_emitter_opacity(obj, 8, updateRate << 10, 0x80);
        } else {
            decrease_emitter_opacity(obj, 8, updateRate << 9, 0x20);
        }
    }
    if (gCurrentPlayerIndex >= PLAYER_ONE && racer->buoyancy > 0.0) {
        if (gNumViewports < 3) {
            speedSquared = ((obj->x_velocity * obj->x_velocity) + (obj->y_velocity * obj->y_velocity)) +
                           (obj->z_velocity * obj->z_velocity);
            if (speedSquared > 16.0f) {
                if (speedSquared < 80.0f) {
                    obj->particleEmittersEnabled |= OBJ_EMIT_3 | OBJ_EMIT_4;
                }
                if (speedSquared > 28.0f) {
                    obj->particleEmittersEnabled |= OBJ_EMIT_1 | OBJ_EMIT_2;
                }
                if (gNumViewports == 1 && speedSquared > 10.25f) {
                    obj->particleEmittersEnabled |= OBJ_EMIT_7;
                }
            }
        } else {
            if (racer->velocity < 0.0f) {
                speedSquared = -racer->velocity;
            } else {
                speedSquared = racer->velocity;
            }
            if (speedSquared > 4.0f) {
                obj->particleEmittersEnabled |= OBJ_EMIT_3 | OBJ_EMIT_4;
            }
        }
    }
    if (gCurrentPlayerIndex != PLAYER_COMPUTER && racer->boostTimer == 0 && gNumViewports < 2) {
        racer_enable_boost_emitter(obj, racer, 10);
    }
    if (racer->aiPhysicsTimer == 0) {
        obj->particleEmittersEnabled = OBJ_EMIT_NONE;
    } else {
        obj->y_velocity += updateRate * gCurrentRacerWeightStat;
        update_vehicle_particles(obj, updateRate);
        obj->y_velocity -= updateRate * gCurrentRacerWeightStat;
    }
}

/**
 * In the air the stick pitches the craft, and its roll settles.
 */
static void hovercraft_update_air_pitch(Object *obj, Object_Racer *racer, s32 updateRate) {
    s32 target;
    s32 diff;
    s32 roll;

    if (racer->groundedWheels != 0 || racer->waterTimer != 0) {
        return;
    }
    target = (-gCurrentStickY * 0x40) & 0xFFFF;
    diff = target - (obj->trans.rotation.x_rotation & 0xFFFF);
    if (diff > 0x8000) {
        diff -= 0xFFFF;
    }
    if (diff < -0x8000) {
        diff += 0xFFFF;
    }
    obj->trans.rotation.x_rotation += diff >> 2;
    roll = -(racer->x_rotation_vel & 0xFFFF);
    if (roll > 0x8000) {
        roll -= 0xFFFF;
    }
    if (roll < -0x8000) {
        roll += 0xFFFF;
    }
    roll *= updateRate;
    racer->x_rotation_vel += roll >> 4;
}

/**
 * Tapping R hops the craft; wheelspinTimer holds the frames until it may hop again.
 */
static void hovercraft_hop(Object *obj, Object_Racer *racer, s32 updateRate) {
    f32 airTime;

    if (gRaceStartTimer != 0) {
        gCurrentButtonsPressed &= ~R_TRIG;
    }
    if (racer->buoyancy != 0.0f || racer->groundedWheels == 4) {
        racer->wheelspinTimer = 0;
    }
    if (gCurrentButtonsPressed & R_TRIG && (racer->buoyancy > 0.0f || racer->groundedWheels >= 2) &&
        racer->wheelspinTimer == 0) {
        if (racer->buoyancy != 0.0f) {
            obj->y_velocity += 4.5;
            if (obj->y_velocity > 5.5) {
                obj->y_velocity = 5.5f;
            }
        } else {
            obj->y_velocity += 3.5;
        }
        airTime = ((2.0 * obj->y_velocity) / gCurrentRacerWeightStat);
        racer->wheelspinTimer = (s32) (airTime) + 1;
    }
    if (racer->wheelspinTimer > 0) {
        racer->wheelspinTimer -= updateRate;
    } else {
        racer->wheelspinTimer = 0;
    }
}

/**
 * The fan at the back turns with the stick and alternates between its two models.
 */
static void hovercraft_update_fan(Object *obj) {
    Object *fan;

    if (obj->attachPoints->count == 1) {
        fan = obj->attachPoints->obj[0];
        fan->trans.rotation.y_rotation = (gCurrentStickX * 20) + 0x4000;
        fan->modelIndex++;
        fan->modelIndex &= 1;
    }
}

/**
 * Controls: eases the steering toward the stick (slower for computers and after the finish),
 * then items, tilt, attacks and head turning.
 */
static void hovercraft_read_controls(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF) {
    f32 steerDivisor;

    steerDivisor = 8.0f;
    if (gCurrentPlayerIndex == PLAYER_COMPUTER || racer->raceFinished) {
        steerDivisor = 32.0f;
    }
    racer->steerAngle += ease_step(gCurrentStickX - racer->steerAngle, updateRateF, steerDivisor);
    handle_racer_items(obj, racer, updateRate);
    racer_update_tilt_vector(obj, racer);
    racer_attack_handler_hovercraft(obj, racer);
    if (gCurrentPlayerIndex != PLAYER_COMPUTER) {
        handle_racer_head_turning(obj, racer, updateRate);
    } else {
        slowly_reset_head_angle(racer);
    }
}

/**
 * Eases the throttle and brake and turns them into this frame's throttle and brake forces.
 */
static void hovercraft_throttle_and_brake(Object_Racer *racer, f32 updateRateF, f32 *outThrottleForce,
                                          f32 *outBrakeForce) {
    f32 throttleForce;
    f32 brakeForce;
    f32 force;

    racer_ease_throttle(racer, updateRateF, 0.016);
    force = racer->throttle;
    if ((racer->velocity > 0.0f) && (gCurrentRacerInput & A_BUTTON)) {
        force = 1.0f;
    }
    force *= 3.0f;
    throttleForce = (force * force) / 9.0f;
    if (racer->exitObj != NULL) {
        racer->throttle = 0.5f;
    }
    if (racer->velocity > 0.0) {
        throttleForce *= 1.2;
    }
    if (racer_ease_brake(racer, updateRateF, 0.046) && racer->velocity < -2.0) {
        rumble_set(racer->playerIndex, RUMBLE_TYPE_5);
    }
    force = racer->brake * 3.0f;
    brakeForce = force * force / 9.0f;
    if (racer->velocity > 0.0) {
        brakeForce *= 0.3;
    }
    *outThrottleForce = throttleForce;
    *outBrakeForce = brakeForce;
}

/**
 * Shows the steering in the driver's animation frame, unless approaching an object.
 */
static void hovercraft_set_steer_frame(Object *obj, Object_Racer *racer) {
    s32 frame;

    if (racer->approachTarget == NULL) {
        obj->animationID = 0;
        frame = racer->steerAngle;
        frame >>= 1;
        frame = 40 - frame;
        if (frame < 0) {
            frame = 0;
        }
        if (frame > 73) {
            frame = 73;
        }
        obj->animFrame = frame;
    }
}

/**
 * Gravity (halved with three or more wheels down), vertical drag, braking drag, and turning
 * from the steering (sharper, with less acceleration, when the stick is pulled back); counts
 * the airborne time used for the landing.
 */
static void hovercraft_gravity_and_steering(Object *obj, Object_Racer *racer, s32 updateRate, f32 updateRateF,
                                            f32 *accelInOut, f32 *outDrag) {
    f32 accel;
    f32 drag;
    f32 speed;
    f32 force;
    s32 steerInput;

    accel = *accelInOut;
    gCurrentRacerWeightStat *= 0.75;
    if (racer->groundedWheels >= 3) {
        obj->y_velocity -= gCurrentRacerWeightStat * 0.5 * updateRateF;
    } else {
        obj->y_velocity -= gCurrentRacerWeightStat * updateRateF;
    }
    steerInput = (gCurrentPlayerIndex != PLAYER_COMPUTER && !racer->raceFinished) ? racer->steerAngle : gCurrentStickX;
    drag = 0.004;
    obj->y_velocity -= (obj->y_velocity * 0.025) * updateRateF;
    if (gCurrentRacerInput & B_BUTTON && gCurrentStickY >= -40 && racer->velocity >= -0.5) {
        drag *= 16.0f;
    }
    force = (steerInput * 8) * updateRate;
    if (gCurrentStickY < 0) {
        speed = -gCurrentStickY;
        force *= 1.0 + (speed / 112.0);
        accel *= 1.0 - (speed / 300.0);
    }
    force *= gCurrentRacerHandlingStat;
    racer->steerVisualRotation -= (((s32) force) & 0xffff) & 0xffff;
    if (racer->groundedWheels == 0 && racer->buoyancy == 0.0) {
        if (obj->y_velocity < 0.0f) {
            racer->trickType += updateRate;
        }
        if (racer->trickType > 25) {
            racer->trickType = 25;
        }
    }
    *accelInOut = accel;
    *outDrag = drag;
}

/**
 * Eases the yaw and roll velocities toward this frame's targets and applies them, and counts
 * down the bump timer.
 */
static void hovercraft_update_rotation(Object *obj, Object_Racer *racer, s32 updateRate) {
    racer->driftHeadingOffset = 0;
    racer->y_rotation_vel += (gCurrentCarSteerVel - racer->y_rotation_vel) >> 3;
    obj->trans.rotation.y_rotation = racer->steerVisualRotation + racer->y_rotation_vel;
    racer->z_rotation_vel += (gRacerTargetRoll - racer->z_rotation_vel) >> 3;
    obj->trans.rotation.z_rotation = racer->x_rotation_vel + racer->z_rotation_vel;
    if (racer->bumpTimer > 0) {
        racer->bumpTimer -= updateRate;
    } else {
        racer->bumpTimer = 0;
    }
}

/**
 * Settles the body offset and records how hard the craft is falling, for the landing.
 */
static void hovercraft_update_fall_impact(Object *obj, Object_Racer *racer, f32 updateRateF) {
    racer->bodyOffsetY -= (racer->bodyOffsetY * 0.125) * updateRateF;
    racer->fallImpact = (-obj->y_velocity) * 7.0;
    if (racer->fallImpact > 35.0) {
        racer->fallImpact = 35.0;
    }
    if (racer->fallImpact < 0.0) {
        racer->fallImpact = 0;
    }
}

/**
 * One frame of hovercraft physics: read the controls, turn and spin, apply the forces,
 * move, float on the waves, collide, then derive the velocities for next frame.
 */
void update_hovercraft(s32 updateRate, f32 updateRateF, Object *obj, Object_Racer *racer) {
    f32 throttleForce;
    f32 brakeForce;
    f32 drag;
    f32 accel;
    f32 speed;
    f32 waterDepth;
    f32 xPos;
    f32 yPos;
    f32 zPos;
    f32 shiftX;
    f32 shiftZ;
    f32 invRate;
    f32 dialogueScale;
    s32 surface;
    s32 stoneWheels;
    s32 playerMoved;

    if (is_nondefault_vehicle()) {
        updateRateF *= 1.15;
    }
    if (racer->playerIndex >= PLAYER_ONE && gNumViewports < 2) {
        obj->particleEmittersEnabled |= OBJ_EMIT_9;
    }
    gRacerSurfaceSoundId = 0;
    gCurrentCarSteerVel = 0;
    gRacerTargetRoll = 0;
    xPos = obj->trans.x_position;
    yPos = obj->trans.y_position;
    zPos = obj->trans.z_position;
    // The craft is drawn offset along its heading while a dialogue camera is active.
    dialogueScale = 1.0 - (gDialogueCameraAngle / 10240.0f);
    shiftX = -racer->ox1 * 10 * dialogueScale;
    shiftZ = -racer->oz1 * 10 * dialogueScale;

    // Controls.
    hovercraft_read_controls(obj, racer, updateRate, updateRateF);
    hovercraft_throttle_and_brake(racer, updateRateF, &throttleForce, &brakeForce);
    if (racer->zipperDirCorrection != 0) {
        gCurrentStickX = 0;
        racer->magnetTimer = 0;
        racer->steerAngle = 0;
        racer->spinout_timer = 0;
        racer->trickType = 0;
        racer_zipper_align(obj, racer, updateRate);
    }
    hovercraft_update_spinout(obj, racer, updateRate, brakeForce);
    racer_update_basis_vectors(obj, racer);
    hovercraft_set_steer_frame(obj, racer);
    // Acceleration from the speed curve, gravity, and steering.
    speed = racer->velocity;
    if (speed < 0.0f) {
        speed = -speed;
    }
    if (speed > 12.0f) {
        speed = 12.0f;
    }
    accel = racer_speed_curve(speed);
    hovercraft_gravity_and_steering(obj, racer, updateRate, updateRateF, &accel, &drag);
    // Surfaces.
    surface = racer_dominant_surface(racer, &stoneWheels);
    racer_check_taj_pad(racer, surface);
    hovercraft_land(obj, racer);
    accel *= handle_racer_top_speed(obj, racer) * 1.7;
    if (racer->boostTimer > 0) {
        if (gRaceStartTimer == 0) {
            throttleForce = 1.0f;
            brakeForce = 0.0f;
            if (accel != 0.0) {
                accel = 2.0f;
            }
            racer->boostTimer -= updateRate;
        }
    } else {
        racer->boostTimer = 0;
    }
    if (gRaceStartTimer != 0) {
        accel = 0.0f;
    }
    if (stoneWheels != 0) {
        speed = ((f32 *) get_misc_asset(ASSET_MISC_32))[stoneWheels];
        accel *= speed;
        drag /= speed;
        racer->magnetTimer = 0;
        racer->lateral_velocity = 0.0f;
    }
    if ((racer->boostTimer == 0) && (surface == SURFACE_ZIP_PAD)) {
        racer_start_zip_pad_boost(obj, racer);
    }

    hovercraft_update_camera_yaw(obj, racer, updateRate);
    if (racer->zipperDirCorrection == 0) {
        hovercraft_apply_forces(obj, racer, updateRateF, throttleForce, brakeForce, accel, drag);
    }
    hovercraft_update_rotation(obj, racer, updateRate);
    shiftX += racer->ox1 * 10 * dialogueScale;
    shiftZ += racer->oz1 * 10 * dialogueScale;
    playerMoved = hovercraft_move(obj, racer, updateRateF, &shiftX, &shiftZ);
    hovercraft_update_fall_impact(obj, racer, updateRateF);
    waterDepth = hovercraft_update_water(obj, racer, updateRate);
    racer_collide_keeping_bump(obj, racer, updateRate);

    // The velocity for next frame is whatever movement survived collision.
    invRate = 1.0f / updateRateF;
    obj->x_velocity = ((obj->trans.x_position - (xPos + shiftX)) - gRacerExternalShiftX) * invRate;
    obj->z_velocity = ((obj->trans.z_position - (zPos + shiftZ)) - gRacerExternalShiftZ) * invRate;
    obj->y_velocity = (obj->trans.y_position - yPos) * invRate;
    racer->forwardVel -= (racer->forwardVel + (racer->velocity * 0.05)) * 0.125;
    if (racer->buoyancy > 0.0 && racer->wheelspinTimer == 0 && obj->y_velocity > 4.0) {
        obj->y_velocity = 4.0f;
    }
    hovercraft_update_particles(obj, racer, updateRate);
    racer_velocity_to_local(obj, racer, TRUE);
    hovercraft_update_air_pitch(obj, racer, updateRate);
    hovercraft_hop(obj, racer, updateRate);
    if (waterDepth > 40.0f) {
        racer->waterBobScale = 0.11f;
    }
    hovercraft_update_fan(obj);
    second_racer_camera_update(obj, racer, CAMERA_HOVERCRAFT, updateRateF);
    if (playerMoved) {
        racer_find_start_node(obj, racer);
    }
}

/**
 * Initialise some states when a racer is attacked or runs into something.
 * While the majority of the respondent behaviour takes place elsewhere, the squish behaviour happens here.
 * Hovercraft do not have a spinning state, instead being in the same state when hit by a rocket.
 */
void racer_attack_handler_hovercraft(Object *obj, Object_Racer *racer) {
    s8 bananas;

    if (racer->playerIndex == PLAYER_COMPUTER) {
        bananas = 0;
    } else {
        bananas = racer->bananas;
    }
    if (racer->attackType == ATTACK_NONE || racer->shieldTimer > 0) {
        racer->attackType = ATTACK_NONE;
        return;
    }
    if (racer->attackType != ATTACK_SQUISHED) {
        drop_bananas(obj, racer, 2);
    }
    play_random_character_voice(obj, SOUND_VOICE_CHARACTER_NEGATIVE, 8, 129);
    switch (racer->attackType) {
        // Getting hit by a rocket, running into a landmine or an oil slick.
        case ATTACK_EXPLOSION:
        case ATTACK_SPIN:
            racer->launchedState = 1;
            if (bananas == 0) {
                obj->x_velocity = 0.0f;
                obj->z_velocity = 0.0f;
                obj->y_velocity = 8.0f;
            } else {
                obj->x_velocity = obj->x_velocity * 0.5;
                obj->z_velocity = obj->z_velocity * 0.5;
                obj->y_velocity = 6.0f;
            }
            break;
        // Crushed by something big, like a snowball.
        case ATTACK_SQUISHED:
            racer->squish_timer = 60;
            break;
        // Running into a bubble trap.
        case ATTACK_BUBBLE:
            racer->bubbleTrapTimer = 120;
            obj->x_velocity *= 0.7;
            if (obj->y_velocity > 2.0f) {
                obj->y_velocity = 2.0f;
            }
            obj->z_velocity *= 0.7;
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

/**
 * Handles the camera movement when the player is driving a hovercraft.
 * Since the hovercraft has very floaty controls, the camera movement is much slower to compensate.
 */
void update_camera_hovercraft(f32 updateRate, Object *obj, Object_Racer *racer) {
    f32 yVel;
    s32 numViewports;
    u8 tempZoom;
    f32 targetBoom;
    f32 targetHeight;
    f32 xVel;
    f32 zVel;
    f32 pitchSin;
    f32 brakeVar;
    f32 baseSpeed;
    s32 basePitch = 0x400;
    s32 tempUpdateRate;
    s32 angle;
    s32 segmentIndex;
    s32 tempAngle;

    targetBoom = 165.0f;
    targetHeight = 65.0f;
    basePitch = 0x400;
    tempUpdateRate = (s32) updateRate;
    // Place the camera a bit closer with 2+ players to help visibility.
    numViewports = cam_get_viewport_layout();
    if (numViewports == 1) {
        targetBoom = 200.0f;
        targetHeight = 51.0f;
        basePitch = 0x280;
    } else if (numViewports > 1) {
        targetBoom = 130.0f;
        targetHeight = 50.0f;
    }
    brakeVar = racer->brake;
    tempZoom = gCameraObject->zoom;
    baseSpeed = racer->forwardVel;
    switch (gCameraObject->zoom) {
        case 1:
            targetBoom += 35.0f;
            break;
        case 2:
            targetBoom -= 35.0f;
            targetHeight -= 10.0f;
            break;
        case 3:
            targetBoom -= 53.0f;
            targetHeight -= 5.0f;
            baseSpeed *= 0.250;
            brakeVar = 0.0f;
            break;
    }
    if (racer->objectContactFlags == 0) {
        angle = (obj->trans.rotation.x_rotation);
        if (angle > 0) {
            angle -= 0x61C;
            if (angle < 0) {
                angle = 0;
            }
            angle >>= 1;
        } else {
            angle += 0x61C;
            if (angle > tempZoom * 0) {
                angle = 0;
            }
        }
        angle = basePitch - angle;
        tempAngle = angle - (u16) gCameraObject->trans.rotation.x_rotation;
        WRAP(tempAngle, -0x8000, 0x8000);
        gCameraObject->trans.rotation.x_rotation += (tempAngle * tempUpdateRate) >> 4;
    }
    cam_get_viewport_layout();
    if (racer->velocity < 0.0) {
        yVel = -(racer->velocity * brakeVar) * 6.0f;
        if (racer->velocity) {} // Fakematch
        if (yVel > 65.0) {
            yVel = 65.0f;
        }
        targetBoom -= yVel;
    }
    targetBoom += baseSpeed * 30.0f;
    if (gRaceStartTimer == 0) {
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
    gCameraObject->boomLength += (targetBoom - gCameraObject->boomLength) * 0.125;   //!@Delta
    gCameraObject->cam_unk_20 += (targetHeight - gCameraObject->cam_unk_20) * 0.125; //!@Delta
    pitchSin = sins_f(gCameraObject->trans.rotation.x_rotation - basePitch);
    targetHeight = coss_f(gCameraObject->trans.rotation.x_rotation - basePitch);
    targetHeight = (gCameraObject->boomLength * pitchSin) + (gCameraObject->cam_unk_20 * targetHeight);
    xVel = sins_f(-racer->cameraYaw + 0x8000) * gCameraObject->boomLength;
    zVel = coss_f(-racer->cameraYaw + 0x8000) * gCameraObject->boomLength;
    yVel = (1.0 - (gDialogueCameraAngle / 10240.0f)); // Goes between 0-1
    xVel -= racer->ox1 * 10.0f * yVel;
    zVel -= racer->oz1 * 10.0f * yVel;
    yVel = racer->lateral_velocity * 2;
    racer->cameraLateralOffset -= (racer->cameraLateralOffset - yVel) * 0.25;
    yVel = sins_f(racer->cameraYaw + 0x4000) * racer->cameraLateralOffset;
    gCameraObject->trans.x_position = obj->trans.x_position + xVel + yVel;
    yVel = gCameraObject->trans.y_position - (obj->trans.y_position + targetHeight);
    if (yVel > 0.0f) {
        yVel *= 0.5;
    } else {
        if (unused_8011D53C == 1) {
            yVel *= 0.5; // Unreachable. unused_8011D53C is never not 0.
        } else {
            yVel *= 0.25;
        }
        if (racer->boostTimer != 0) {
            yVel *= 2.0;
        }
    }
    gCameraObject->trans.y_position -= yVel;
    gCameraObject->trans.rotation.z_rotation = 0;
    if (gRaceStartTimer) {
        gCameraObject->trans.y_position = obj->trans.y_position + targetHeight;
    }

    coss_f(racer->cameraYaw + 0x4000); // Unused function call that wasn't fully optimised out.
    gCameraObject->trans.z_position = obj->trans.z_position + zVel;
    gCameraObject->trans.rotation.y_rotation = racer->cameraYaw;
    segmentIndex = get_level_segment_index_from_position(
        gCameraObject->trans.x_position, gCameraObject->trans.y_position, gCameraObject->trans.z_position);
    if (segmentIndex != SEGMENT_NONE) {
        gCameraObject->cameraSegmentID = segmentIndex;
    }
}
