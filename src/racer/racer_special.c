#include "racer_internal.h"

/**
 * Movement while riding a loop-de-loop: the racer is carried along its nodes
 * with walls ignored and the boost kept alive.
 */
void update_loop_de_loop(s32 updateRate, f32 updateRateF, Object *racerObj, Object_Racer *racer) {
    s32 animFrame;
    s32 moveObjResult;
    f32 curYPos;
    s32 i;
    Object **nodes;
    s32 var_a2;
    s32 var_v0;
    f32 prevXPos;
    f32 prevYPos;
    f32 prevZPos;
    f32 xDiff;
    f32 zDiff;
    f32 yDiff;
    s32 var_v1;
    f32 temp;
    f32 temp2;
    f32 temp3;
    s32 steerAngle;
    f32 temp4;
    f32 var_f0;
    f32 mtx[4][4];
    Object *obj;
    f32 var_f2;
    s8 objectMoved;

    objectMoved = FALSE;
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
    set_collision_mode(COLLISION_MODE_NO_WALLS);
    gRacerSurfaceSoundId = 0;
    gCurrentCarSteerVel = 0;
    gRacerTargetRoll = 0;
    prevXPos = racerObj->trans.x_position;
    prevYPos = racerObj->trans.y_position;
    prevZPos = racerObj->trans.z_position;
    var_v1 = gCurrentStickX - racer->steerAngle;
    var_a2 = (var_v1 * updateRate) >> 4;
    if (var_v1 != 0 && var_a2 == 0) {
        if (var_v1 > 0) {
            var_a2 = 1;
        }
        if (var_v1 < 0) {
            var_a2 = -1;
        }
    }
    racer->steerAngle += var_a2;
    var_v1 = gCurrentStickY - racer->smoothedStickY;
    var_a2 = (var_v1 * updateRate) >> 4;
    if (var_v1 != 0 && var_a2 == 0) {
        if (var_v1 > 0) {
            var_a2 = 1;
        }
        if (var_v1 < 0) {
            var_a2 = -1;
        }
    }
    racer->smoothedStickY += var_a2;
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
                temp4 = -((racer->ox3 * racerObj->trans.x_position) + (zDiff * racerObj->trans.z_position));
                var_f0 = (obj->trans.x_position * racer->ox3) + (obj->trans.z_position * zDiff) + temp4;
                steerAngle = var_f0;
                steerAngle /= 5;
            }
        }
    }
    racer->attackType = ATTACK_NONE;
    racer->spinout_timer = 0;
    var_f0 = (steerAngle * racer->velocity) / 360;
    var_f2 = 1.0f;
    racerObj->x_velocity -= (racer->ox3 * var_f0);
    racerObj->y_velocity -= (racer->oy3 * var_f0);
    racerObj->z_velocity -= (racer->oz3 * var_f0);
    if (gCurrentPlayerIndex == PLAYER_COMPUTER) {
        var_f2 = 1.3f;
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
        var_f2 = 0.1f;
        racer->trickType += updateRate;
        if (racer->trickType > 60) {
            racer->vehicleID = racer->vehicleIDPrev;
            racerObj->trans.rotation.x_rotation = 0;
            racer->trickType = 0;
        }
    } else {
        racer->trickType = 0;
    }
    var_v0 = racer->loopEntryYaw - (racer->steerVisualRotation & 0xFFFF);
    if (var_v0 > 0x8000) {
        var_v0 -= 0xFFFF;
    }
    if (var_v0 < -0x8000) {
        var_v0 += 0xFFFF;
    }
    if ((var_v0 > 0x1000) || (var_v0 < -0x1000)) {
        var_f2 = 0.0f;
    }
    racer->steerVisualRotation += (var_v0 >> 3);
    racer->x_rotation_vel = 0;

    racerObj->x_velocity -= racer->ox1 * var_f2;
    racerObj->y_velocity -= racer->oy1 * var_f2;
    racerObj->z_velocity -= racer->oz1 * var_f2;
    racerObj->x_velocity -= racer->ox2 * 1.5;
    racerObj->y_velocity -= racer->oy2 * 1.5;
    racerObj->z_velocity -= racer->oz2 * 1.5;

    var_f0 = racer->velocity * racer->velocity * 0.002f;
    if (racer->velocity < 0.0f) {
        var_f0 = -var_f0;
    }
    racerObj->x_velocity -= racer->ox1 * var_f0;
    racerObj->y_velocity -= racer->oy1 * var_f0;
    racerObj->z_velocity -= racer->oz1 * var_f0;

    var_f0 = racer->lateral_velocity * racer->lateral_velocity * 0.01f;
    if (racer->lateral_velocity < 0.0f) {
        var_f0 = -var_f0;
    }
    racerObj->x_velocity -= racer->ox3 * var_f0;
    racerObj->y_velocity -= racer->oy3 * var_f0;
    racerObj->z_velocity -= racer->oz3 * var_f0;
    var_f0 = racer->vehiclePitch * racer->vehiclePitch * 0.01f;
    if (racer->vehiclePitch < 0.0f) {
        var_f0 = -var_f0;
    }
    racerObj->x_velocity -= racer->ox2 * var_f0;
    racerObj->y_velocity -= racer->oy2 * var_f0;
    racerObj->z_velocity -= racer->oz2 * var_f0;

    racer->driftHeadingOffset = 0;
    racer->y_rotation_vel += ((gCurrentCarSteerVel - racer->y_rotation_vel) >> 3);
    racerObj->trans.rotation.y_rotation = racer->steerVisualRotation + racer->y_rotation_vel;
    racer->z_rotation_vel += ((gRacerTargetRoll - racer->z_rotation_vel) >> 3);
    racerObj->trans.rotation.z_rotation = racer->x_rotation_vel + racer->z_rotation_vel;
    temp2 = racerObj->x_velocity;
    temp3 = racerObj->z_velocity;
    racer->bumpTimer = 0;
    moveObjResult = move_object(racerObj, temp2 * updateRateF, racerObj->y_velocity * updateRateF, temp3 * updateRateF);
    if ((moveObjResult) && (gCurrentPlayerIndex != -1)) {
        objectMoved = TRUE;
    }
    if (gCurrentPlayerIndex == PLAYER_COMPUTER) {
        onscreen_ai_racer_physics(racerObj, racer, updateRate);
    } else {
        update_racer_collision(racerObj, racer, updateRate);
    }
    var_f0 = (racerObj->trans.x_position - prevXPos) * (1 / updateRateF);
    racerObj->y_velocity = (racerObj->trans.y_position - prevYPos) * (1 / updateRateF);
    temp3 = (racerObj->trans.z_position - prevZPos) * (1 / updateRateF);
    racerObj->z_velocity = temp3;
    racerObj->x_velocity = var_f0;
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
    second_racer_camera_update(racerObj, racer, CAMERA_LOOP, updateRateF);
    if (racerObj->attachPoints != NULL && racer->vehicleIDPrev == VEHICLE_CAR && racerObj->attachPoints->count >= 4) {
        obj = racerObj->attachPoints->obj[2];
        obj->trans.rotation.y_rotation = 0;
        obj = racerObj->attachPoints->obj[3];
        obj->trans.rotation.y_rotation = 0;
    }
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
    s32 UpdateRate;
    UNUSED f32 pad;
    s32 segmentIndex;
    s32 angle;
    f32 targetBoomLength;
    f32 deltaX;
    f32 deltaY;
    f32 deltaZ;
    MtxF mtx;
    s32 angleDiff;

    UpdateRate = (s32) updateRateF;
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
    gCameraObject->trans.rotation.z_rotation += ((angle * UpdateRate) >> 4);
    angleDiff = -obj->trans.rotation.x_rotation;
    angle = angleDiff - (u16) gCameraObject->trans.rotation.x_rotation;
    WRAP(angle, -0x8000, 0x8000);
    gCameraObject->trans.rotation.x_rotation += ((angle * UpdateRate) >> 4);
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
