#include "racer_internal.h"

/**
 * No shields on a loop-de-loop, and the boost is kept going (restarted with its sound when
 * it has run out).
 */
static void loop_keep_boost(Object *racerObj, Object_Racer *racer) {
    racer->shieldTimer = 0;
    racer->shieldType = 0;
    if (racer->shieldSoundMask != NULL) {
        audspat_point_stop(racer->shieldSoundMask);
        racer->shieldSoundMask = NULL;
    }
    if (racer->boostTimer == 0) {
        racer->boostTimer = 8;
        racer_play_sound(racerObj, SOUND_ZIP_PAD_BOOST);
    }
    if (racer->boostTimer < 4) {
        racer->boostTimer = 4;
    }
    racer->boostType = BOOST_NONE;
}

/**
 * Eases the steer angle and pitch input toward the stick, at least one unit per frame while
 * they differ.
 */
static void loop_ease_stick(Object_Racer *racer, s32 updateRate) {
    s32 diff;
    s32 step;

    diff = gCurrentStickX - racer->steerAngle;
    step = (diff * updateRate) >> 4;
    if (diff != 0 && step == 0) {
        if (diff > 0) {
            step = 1;
        }
        if (diff < 0) {
            step = -1;
        }
    }
    racer->steerAngle += step;
    diff = gCurrentStickY - racer->smoothedStickY;
    step = (diff * updateRate) >> 4;
    if (diff != 0 && step == 0) {
        if (diff > 0) {
            step = 1;
        }
        if (diff < 0) {
            step = -1;
        }
    }
    racer->smoothedStickY += step;
}

/**
 * Clears the vehicle's rotation offsets, rebuilds its basis vectors and up vector, holds the
 * throttle down and updates the tilt, speed and wheels.
 */
static void loop_orient(Object *racerObj, Object_Racer *racer, f32 updateRateF) {
    f32 mtx[4][4];

    racer->y_rotation_offset = 0;
    racer->x_rotation_offset = 0;
    racer->z_rotation_offset = 0;
    racer_update_basis_vectors(racerObj, racer);
    gCurrentRacerTransform.rotation.y_rotation = racerObj->trans.rotation.y_rotation;
    gCurrentRacerTransform.rotation.x_rotation = racerObj->trans.rotation.x_rotation;
    gCurrentRacerTransform.rotation.z_rotation = racerObj->trans.rotation.z_rotation;
    gCurrentRacerTransform.x_position = 0.0f;
    gCurrentRacerTransform.y_position = 0.0f;
    gCurrentRacerTransform.z_position = 0.0f;
    gCurrentRacerTransform.scale = 1.0f;
    mtxf_from_transform(&mtx, &gCurrentRacerTransform);
    mtxf_transform_point(mtx, 0.0f, 1.0f, 0.0f, &racer->ox2, &racer->oy2, &racer->oz2);
    gCurrentRacerInput = A_BUTTON;
    racer_update_tilt_vector(racerObj, racer);
    handle_car_velocity_control(racer);
    update_car_wheels(racerObj, racer, updateRateF);
}

/**
 * Shows the steering in the driver's animation frame.
 */
static void loop_set_steer_frame(Object *racerObj, Object_Racer *racer) {
    s32 animFrame;

    racerObj->animationID = 0;
    animFrame = racer->steerAngle;
    animFrame = animFrame >> 1;
    animFrame = 40 - animFrame;
    if (animFrame < 0) {
        animFrame = 0;
    }
    if (animFrame > 73) {
        animFrame = 73;
    }
    racerObj->animFrame = animFrame;
}

/**
 * The steering into the loop: the player's steer angle, or for a computer racer, toward its
 * current node (moving on to the next node once close).
 */
static s32 loop_steer_angle(Object *racerObj, Object_Racer *racer) {
    s32 i;
    Object **nodes;
    f32 xDiff;
    f32 zDiff;
    f32 yDiff;
    s32 steerAngle;
    f32 planeDist;
    f32 nodeSide;
    Object *obj;

    steerAngle = 10;
    if (racer->playerIndex != PLAYER_COMPUTER) {
        steerAngle = racer->steerAngle;
    } else {
        obj = racer->nodeCurrent;
        if (obj != NULL) {
            xDiff = obj->trans.x_position - racerObj->trans.x_position;
            yDiff = obj->trans.y_position - racerObj->trans.y_position;
            zDiff = obj->trans.z_position - racerObj->trans.z_position;
            if (sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)) < 200.0) {
                nodes = obj->ai_node->nodeObj;
                for (i = 0; i < 4; i++) {
                    if (nodes[i] != NULL && racer->challengeMarker != nodes[i]) {
                        racer->challengeMarker = obj;
                        racer->nodeCurrent = nodes[i];
                        i = 4;
                    }
                }
                if (i != 5) {
                    racer->nodeCurrent = NULL;
                }
            } else {
                zDiff = racer->oz3;
                planeDist = -((racer->ox3 * racerObj->trans.x_position) + (zDiff * racerObj->trans.z_position));
                nodeSide = (obj->trans.x_position * racer->ox3) + (obj->trans.z_position * zDiff) + planeDist;
                steerAngle = nodeSide;
                steerAngle /= 5;
            }
        }
    }
    return steerAngle;
}

/**
 * Drag that grows with the square of the forward, sideways and vertical speeds.
 */
static void loop_apply_drag(Object *racerObj, Object_Racer *racer) {
    f32 dragForce;

    dragForce = racer->velocity * racer->velocity * 0.002f;
    if (racer->velocity < 0.0f) {
        dragForce = -dragForce;
    }
    racerObj->x_velocity -= racer->ox1 * dragForce;
    racerObj->y_velocity -= racer->oy1 * dragForce;
    racerObj->z_velocity -= racer->oz1 * dragForce;

    dragForce = racer->lateral_velocity * racer->lateral_velocity * 0.01f;
    if (racer->lateral_velocity < 0.0f) {
        dragForce = -dragForce;
    }
    racerObj->x_velocity -= racer->ox3 * dragForce;
    racerObj->y_velocity -= racer->oy3 * dragForce;
    racerObj->z_velocity -= racer->oz3 * dragForce;
    dragForce = racer->vehiclePitch * racer->vehiclePitch * 0.01f;
    if (racer->vehiclePitch < 0.0f) {
        dragForce = -dragForce;
    }
    racerObj->x_velocity -= racer->ox2 * dragForce;
    racerObj->y_velocity -= racer->oy2 * dragForce;
    racerObj->z_velocity -= racer->oz2 * dragForce;
}

/**
 * Eases the yaw and roll velocities toward this frame's targets and applies them.
 */
static void loop_update_rotation(Object *racerObj, Object_Racer *racer) {
    racer->driftHeadingOffset = 0;
    racer->y_rotation_vel += ((gCurrentCarSteerVel - racer->y_rotation_vel) >> 3);
    racerObj->trans.rotation.y_rotation = racer->steerVisualRotation + racer->y_rotation_vel;
    racer->z_rotation_vel += ((gRacerTargetRoll - racer->z_rotation_vel) >> 3);
    racerObj->trans.rotation.z_rotation = racer->x_rotation_vel + racer->z_rotation_vel;
}

/**
 * The velocity for next frame is the distance moved, which is also stored in the racer's
 * local axes.
 */
static void loop_velocity_to_local(Object *racerObj, Object_Racer *racer, f32 updateRateF, f32 prevXPos, f32 prevYPos,
                                   f32 prevZPos) {
    f32 velX;
    f32 velZ;
    f32 mtx[4][4];

    velX = (racerObj->trans.x_position - prevXPos) * (1 / updateRateF);
    racerObj->y_velocity = (racerObj->trans.y_position - prevYPos) * (1 / updateRateF);
    velZ = (racerObj->trans.z_position - prevZPos) * (1 / updateRateF);
    racerObj->z_velocity = velZ;
    racerObj->x_velocity = velX;
    gCurrentRacerTransform.rotation.y_rotation = -racerObj->trans.rotation.y_rotation;
    gCurrentRacerTransform.rotation.x_rotation = -racerObj->trans.rotation.x_rotation;
    gCurrentRacerTransform.rotation.z_rotation = 0;
    gCurrentRacerTransform.scale = 1.0f;
    gCurrentRacerTransform.x_position = 0.0f;
    gCurrentRacerTransform.y_position = 0.0f;
    gCurrentRacerTransform.z_position = 0.0f;
    gCurrentRacerTransform.scale = 1.0f;
    mtxf_from_inverse_transform(&mtx, &gCurrentRacerTransform);
    mtxf_transform_point(mtx, racerObj->x_velocity, racerObj->y_velocity, racerObj->z_velocity,
                         &racer->lateral_velocity, &racer->vehiclePitch, &racer->velocity);
}

/**
 * Straightens a car's front wheels.
 */
static void loop_straighten_wheels(Object *racerObj, Object_Racer *racer) {
    Object *obj;

    if (racerObj->attachPoints != NULL && racer->vehicleIDPrev == VEHICLE_CAR && racerObj->attachPoints->count >= 4) {
        obj = racerObj->attachPoints->obj[2];
        obj->trans.rotation.y_rotation = 0;
        obj = racerObj->attachPoints->obj[3];
        obj->trans.rotation.y_rotation = 0;
    }
}

/**
 * Movement while riding a loop-de-loop: the racer is carried along its nodes
 * with walls ignored and the boost kept alive.
 */
void update_loop_de_loop(s32 updateRate, f32 updateRateF, Object *racerObj, Object_Racer *racer) {
    s32 moveObjResult;
    s32 yawDiff;
    f32 prevXPos;
    f32 prevYPos;
    f32 prevZPos;
    f32 velX;
    f32 velZ;
    s32 steerAngle;
    f32 sideForce;
    f32 thrust;
    s8 objectMoved;

    objectMoved = FALSE;
    loop_keep_boost(racerObj, racer);
    set_collision_mode(COLLISION_MODE_NO_WALLS);
    gRacerSurfaceSoundId = 0;
    gCurrentCarSteerVel = 0;
    gRacerTargetRoll = 0;
    prevXPos = racerObj->trans.x_position;
    prevYPos = racerObj->trans.y_position;
    prevZPos = racerObj->trans.z_position;
    loop_ease_stick(racer, updateRate);
    loop_orient(racerObj, racer, updateRateF);
    loop_set_steer_frame(racerObj, racer);
    steerAngle = loop_steer_angle(racerObj, racer);
    racer->attackType = ATTACK_NONE;
    racer->spinout_timer = 0;
    sideForce = (steerAngle * racer->velocity) / 360;
    thrust = 1.0f;
    racerObj->x_velocity -= (racer->ox3 * sideForce);
    racerObj->y_velocity -= (racer->oy3 * sideForce);
    racerObj->z_velocity -= (racer->oz3 * sideForce);
    if (gCurrentPlayerIndex == PLAYER_COMPUTER) {
        thrust = 1.3f;
    }
    racer->loopDeLoopTimer += updateRate;
    if (racer->loopDeLoopTimer > 600) {
        racer->vehicleID = racer->vehicleIDPrev;
        racerObj->trans.rotation.x_rotation = 0;
        racer->trickType = 0;
        if (racer->playerIndex >= 0) {
            objectMoved = TRUE;
        }
    }
    if (racer->groundedWheels == 0) {
        thrust = 0.1f;
        racer->trickType += updateRate;
        if (racer->trickType > 60) {
            racer->vehicleID = racer->vehicleIDPrev;
            racerObj->trans.rotation.x_rotation = 0;
            racer->trickType = 0;
        }
    } else {
        racer->trickType = 0;
    }
    yawDiff = racer->loopEntryYaw - (racer->steerVisualRotation & 0xFFFF);
    if (yawDiff > 0x8000) {
        yawDiff -= 0xFFFF;
    }
    if (yawDiff < -0x8000) {
        yawDiff += 0xFFFF;
    }
    if ((yawDiff > 0x1000) || (yawDiff < -0x1000)) {
        thrust = 0.0f;
    }
    racer->steerVisualRotation += (yawDiff >> 3);
    racer->x_rotation_vel = 0;

    racerObj->x_velocity -= racer->ox1 * thrust;
    racerObj->y_velocity -= racer->oy1 * thrust;
    racerObj->z_velocity -= racer->oz1 * thrust;
    racerObj->x_velocity -= racer->ox2 * 1.5;
    racerObj->y_velocity -= racer->oy2 * 1.5;
    racerObj->z_velocity -= racer->oz2 * 1.5;

    loop_apply_drag(racerObj, racer);
    loop_update_rotation(racerObj, racer);
    velX = racerObj->x_velocity;
    velZ = racerObj->z_velocity;
    racer->bumpTimer = 0;
    moveObjResult = move_object(racerObj, velX * updateRateF, racerObj->y_velocity * updateRateF, velZ * updateRateF);
    if ((moveObjResult) && (gCurrentPlayerIndex != PLAYER_COMPUTER)) {
        objectMoved = TRUE;
    }
    if (gCurrentPlayerIndex == PLAYER_COMPUTER) {
        onscreen_ai_racer_physics(racerObj, racer, updateRate);
    } else {
        update_racer_collision(racerObj, racer, updateRate);
    }
    loop_velocity_to_local(racerObj, racer, updateRateF, prevXPos, prevYPos, prevZPos);
    second_racer_camera_update(racerObj, racer, CAMERA_LOOP, updateRateF);
    loop_straighten_wheels(racerObj, racer);
    if (objectMoved) {
        racer->vehicleID = racer->vehicleIDPrev;
        racer_find_start_node(racerObj, racer);
    }
}

/**
 * Handles the camera movement when the player is on a loop-the-loop.
 * This happens in Walrus Cove and Darkmoon Caverns.
 * The camera's rotation follows the players exactly, in order to stay levelled.
 */
void update_camera_loop(f32 updateRateF, Object *obj, Object_Racer *racer) {
    s32 updateRateInt;
    s32 segmentIndex;
    s32 angle;
    f32 targetBoomLength;
    f32 deltaX;
    f32 deltaY;
    f32 deltaZ;
    MtxF mtx;
    s32 angleDiff;

    updateRateInt = (s32) updateRateF;
    targetBoomLength = 120.0f;
    if (gRaceStartTimer > 60) {
        targetBoomLength += (gRaceStartTimer - 60) * 4.0f;
    }
    racer->cameraYaw = 0x8000 - racer->steerVisualRotation;
    if (cam_get_viewport_layout() == VIEWPORT_LAYOUT_2_PLAYERS) {
        targetBoomLength = 160.0f;
    }
    // A little bit of indecisiveness.
    if (gCameraObject->zoom == ZOOM_FAR) {
        targetBoomLength += 35.0f;
    }
    if (gCameraObject->zoom == ZOOM_FAR) {
        targetBoomLength -= 35.0f;
    }
    gCameraObject->boomLength += (targetBoomLength - gCameraObject->boomLength) * 0.125;

    gCurrentRacerTransform.rotation.y_rotation = 0x8000 - gCameraObject->trans.rotation.y_rotation;
    gCurrentRacerTransform.rotation.x_rotation = -gCameraObject->trans.rotation.x_rotation;
    gCurrentRacerTransform.rotation.z_rotation = 0;
    gCurrentRacerTransform.x_position = 0.0f;
    gCurrentRacerTransform.y_position = 0.0f;
    gCurrentRacerTransform.z_position = 0.0f;
    gCurrentRacerTransform.scale = 1.0f;
    mtxf_from_transform(&mtx, &gCurrentRacerTransform);

    mtxf_transform_point(mtx, 0.0f, 0.0f, gCameraObject->boomLength, &deltaX, &deltaY, &deltaZ);
    gCameraObject->trans.x_position = obj->trans.x_position + deltaX;
    gCameraObject->trans.y_position = obj->trans.y_position + deltaY;
    gCameraObject->trans.z_position = obj->trans.z_position + deltaZ;

    mtxf_transform_point(mtx, 0.0f, sins_f(0x800) * gCameraObject->boomLength, 0.0f, &deltaX, &deltaY, &deltaZ);
    gCameraObject->trans.x_position += deltaX;
    gCameraObject->trans.y_position += deltaY;
    gCameraObject->trans.z_position += deltaZ;

    angleDiff = (u16) (-obj->trans.rotation.z_rotation);
    angle = angleDiff - ((u16) gCameraObject->trans.rotation.z_rotation);
    WRAP(angle, -0x8000, 0x8000);
    gCameraObject->trans.rotation.z_rotation += ((angle * updateRateInt) >> 4);
    angleDiff = -obj->trans.rotation.x_rotation;
    angle = angleDiff - (u16) gCameraObject->trans.rotation.x_rotation;
    WRAP(angle, -0x8000, 0x8000);
    gCameraObject->trans.rotation.x_rotation += ((angle * updateRateInt) >> 4);
    gCameraObject->trans.rotation.y_rotation = racer->cameraYaw;
    segmentIndex = get_level_segment_index_from_position(
        gCameraObject->trans.x_position, gCameraObject->trans.y_position, gCameraObject->trans.z_position);
    if (segmentIndex != SEGMENT_NONE) {
        gCameraObject->cameraSegmentID = segmentIndex;
    }
    racer->cameraYaw = gCameraObject->trans.rotation.y_rotation;
    gCameraObject->trans.x_position += gCameraObject->x_velocity;
    gCameraObject->trans.y_position += gCameraObject->y_velocity;
    gCameraObject->trans.z_position += gCameraObject->z_velocity;
}

/**
 * Updates the carpet vehicle that Taj uses during the overworld race challenges.
 */
void update_carpet(s32 updateRate, f32 updateRateF, Object *obj, Object_Racer *racer) {
    s16 animFrame;

    if (racer->vehicleSound) {
        racer_sound_free(obj);
    }
    if (is_taj_challenge() && racer->vehicleID == VEHICLE_CARPET) {
        obj->interactObj->flags = INTERACT_FLAGS_NONE;
    }
    animFrame = obj->animFrame;
    racer->vehicleID = VEHICLE_CARPET;
    update_plane(updateRate, updateRateF, obj, racer);
    racer->vehicleID = racer->vehicleIDPrev;
    obj->animationID = 0;
    if (racer->vehicleID == VEHICLE_CARPET) {
        if (racer->aiNodeObj != NULL) {
            racer->aiNodeObj->trans.x_position = obj->trans.x_position;
            racer->aiNodeObj->trans.y_position = obj->trans.y_position;
            racer->aiNodeObj->trans.z_position = obj->trans.z_position;
            racer->aiNodeObj->segmentID = obj->segmentID;
            racer->aiNodeObj->trans.rotation.y_rotation = obj->trans.rotation.y_rotation;
            racer->aiNodeObj->trans.rotation.x_rotation = obj->trans.rotation.x_rotation;
            racer->aiNodeObj->trans.rotation.z_rotation = obj->trans.rotation.z_rotation;
            obj->animationID = 0;
            obj->animFrame = animFrame + updateRate;
            object_update_model_animation(obj);
        }
    }
}
