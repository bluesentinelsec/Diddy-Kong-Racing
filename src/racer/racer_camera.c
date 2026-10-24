#include "racer_internal.h"

/**
 * Primary function for updating the camera during player control.
 * Decides which behaviour to use, then branches off to that function.
 * Afterwards, writes position data to the current camera object.
 */
void update_player_camera(Object *obj, Object_Racer *racer, f32 updateRateF) {
    f32 dialogueAngle;
    s32 tempUpdateRateF;
    s32 angle;

    if (gCurrentButtonsPressed & U_CBUTTONS && race_starting()) {
        gCameraObject->zoom++;
        if (gCameraObject->zoom > ZOOM_VERY_CLOSE) {
            gCameraObject->zoom = ZOOM_MEDIUM;
        }
        if (racer->playerIndex != PLAYER_COMPUTER) {
            cam_set_zoom(racer->playerIndex, gCameraObject->zoom);
        }
        switch (gCameraObject->zoom) {
            case ZOOM_MEDIUM:
                sound_play(SOUND_MENU_BACK, NULL);
                break;
            case ZOOM_FAR:
                sound_play(SOUND_MENU_BACK2, NULL);
                break;
            default:
                sound_play(SOUND_UNK_6A, NULL);
                break;
        }
    }
    if (racer->raceFinished == TRUE && gCameraObject->mode != CAMERA_FINISH_CHALLENGE) {
        gCameraObject->mode = CAMERA_FINISH_RACE;
    }
    if (racer->exitObj) {
        gCameraObject->mode = CAMERA_FIXED;
    }
    // Set the camera behaviour based on current mode.
    switch (gCameraObject->mode) {
        default:
            break;
        case CAMERA_CAR:
            // Driving a car.
            update_camera_car(updateRateF, obj, racer);
            break;
        case CAMERA_PLANE:
            // Flying a plane.
            update_camera_plane(updateRateF, obj, racer);
            break;
        case CAMERA_FIXED:
            // Fixes the camera in place, continuing to look at the player. Used when entering doors.
            update_camera_fixed(updateRateF, obj, racer);
            break;
        case CAMERA_HOVERCRAFT:
            // Driving a hovercraft.
            update_camera_hovercraft(updateRateF, obj, racer);
            break;
        case CAMERA_FINISH_CHALLENGE:
            // When you finish a challenge, the camera will rotate around the player, or around a set path, if the
            // player's been KO'd.
            update_camera_finish_challenge(updateRateF, obj, racer);
            break;
        case CAMERA_LOOP:
            // Used for loop-the-loops. Follows the face direction of the player exactly, the standard camera modes do
            // not.
            update_camera_loop(updateRateF, obj, racer);
            break;
        case CAMERA_FINISH_RACE:
            // When you finish a race, the camera will go into a more cinematic mode, following the player.
            update_camera_finish_race(updateRateF, obj, racer);
            break;
    }

    dialogueAngle = gDialogueCameraAngle / 10240.0f; // Goes between 0-1
    gCameraObject->x_velocity =
        (((obj->trans.x_position + (91.75 * racer->ox1) + (90.0 * racer->ox3)) - gCameraObject->trans.x_position) *
         dialogueAngle);
    gCameraObject->z_velocity =
        (((obj->trans.z_position + (91.75 * racer->oz1) + (90.0 * racer->oz3)) - gCameraObject->trans.z_position) *
         dialogueAngle);
    gCameraObject->y_velocity = (((get_npc_pos_y() + 48.5) - gCameraObject->trans.y_position) * dialogueAngle);
    gCameraObject->pitch = -gCameraObject->trans.rotation.x_rotation * dialogueAngle;
    gCameraObject->trans.x_position += gCameraObject->x_velocity;
    gCameraObject->trans.y_position += gCameraObject->y_velocity + gCameraObject->shakeMagnitude;
    gCameraObject->trans.z_position += gCameraObject->z_velocity;
    if (gRaceStartTimer == 0 && gDialogueCameraAngle == 0) {
        gCameraObject->x_velocity *= 0.95;
        gCameraObject->y_velocity *= 0.95;
        gCameraObject->z_velocity *= 0.95;
    }

    angle = gDialogueCameraAngle;
    if (angle > 0x1400) {
        angle = 0x2800 - gDialogueCameraAngle;
    }
    if (angle > 0x600) {
        angle = 0x600;
    }
    angle = ((angle >> 4) + 4);
    tempUpdateRateF = updateRateF;
    if (gRacerDialogueCamera) {
        gDialogueCameraAngle += tempUpdateRateF * angle;
        if (gDialogueCameraAngle > 0x2800) {
            gDialogueCameraAngle = 0x2800;
        }
    } else {
        gDialogueCameraAngle -= tempUpdateRateF * angle;
        if (gDialogueCameraAngle < 0) {
            gDialogueCameraAngle = 0;
        }
    }
    gCameraObject->trans.rotation.y_rotation -= gDialogueCameraAngle;

    gCameraObject->shakeTimer -= tempUpdateRateF;
    while (gCameraObject->shakeTimer < 0) {
        gCameraObject->shakeTimer += 5;
        gCameraObject->shakeMagnitude = -gCameraObject->shakeMagnitude * 0.75;
    }
}

/**
 * After the racer logic is all done, update the camera once more.
 * This time it will create the smooth following effect when turning.
 */
void second_racer_camera_update(Object *obj, Object_Racer *racer, s32 mode, f32 updateRateF) {
    f32 xPos, yPos, zPos;
    if (gCurrentPlayerIndex != PLAYER_COMPUTER && racer->raceFinished != TRUE) {
        if (mode != gCameraObject->mode) {
            update_player_camera(obj, racer, updateRateF);
            xPos = gCameraObject->trans.x_position;
            yPos = gCameraObject->trans.y_position;
            zPos = gCameraObject->trans.z_position;
            gCameraObject->mode = mode;
            update_player_camera(obj, racer, updateRateF);
            if (gRaceStartTimer == 0 && gTajInteractStatus == TAJ_WANDER) {
                gCameraObject->x_velocity = xPos - gCameraObject->trans.x_position;
                gCameraObject->y_velocity = yPos - (gCameraObject->trans.y_position + gCameraObject->shakeMagnitude);
                gCameraObject->z_velocity = zPos - gCameraObject->trans.z_position;
            }
        }
    }
}

/**
 * The car camera's boom length target: pulled in by braking while reversing, pushed out with
 * forward speed (less while drifting), and set outright during a boost.
 */
static f32 camera_car_target_boom(Object_Racer *racer, f32 baseDistance, f32 forwardVel, f32 brakeVar) {
    f32 brakePull;

    if (racer->velocity < 0.0) {
        brakePull = 6.0f;
        if (racer->drift_direction) {
            brakePull = 3.0f;
        }
        brakePull = -(racer->velocity * brakeVar) * brakePull;
        if (65.0 < brakePull) {
            brakePull = 65.0f;
        }
        baseDistance -= brakePull;
    }
    if (!racer->drift_direction) {
        baseDistance += forwardVel * 60.0f;
    } else {
        baseDistance += forwardVel * 30.0f;
    }
    if (gRaceStartTimer == 0) {
        if (normalise_time(36) < racer->boostTimer) {
            baseDistance = -30.0f;
        } else if (racer->boostTimer > 0) {
            baseDistance = 180.0f;
        }
    }
    return baseDistance;
}

/**
 * Eases the camera's boom length and height toward their targets (snapping to them early in
 * the countdown).
 */
static void camera_car_ease_boom(f32 baseDistance, f32 targetHeight) {
    f32 prevHeight;

    if (gRaceStartTimer > 80) {
        gCameraObject->boomLength = baseDistance;
        gCameraObject->cam_unk_20 = targetHeight;
    }
    gCameraObject->boomLength += (baseDistance - gCameraObject->boomLength) * 0.125;
    prevHeight = gCameraObject->cam_unk_20;
    gCameraObject->cam_unk_20 += (targetHeight - prevHeight) * 0.125;
}

/**
 * Eases the camera's sideways offset: toward the slide while accelerating, or to the outside
 * of a drift.
 */
static void camera_car_lateral_offset(Object_Racer *racer) {
    f32 lateralOffset;

    lateralOffset = 0.0f;
    if (gCurrentRacerInput & A_BUTTON) {
        lateralOffset = racer->lateral_velocity * 1.3;
        if (lateralOffset > 0.0f) {
            lateralOffset -= 1.5;
            if (lateralOffset < 0.0f) {
                lateralOffset = 0.0f;
            }
        } else {
            lateralOffset += 1.5;
            if (lateralOffset > 0.0f) {
                lateralOffset = 0.0f;
            }
        }
    }
    if (racer->drift_direction) {
        lateralOffset = -(f32) racer->drift_direction * 12.0f;
    }
    racer->cameraLateralOffset += (lateralOffset - racer->cameraLateralOffset) * 0.125;
}

/**
 * Places the camera behind the racer at the boom offset plus the sideways offset, easing its
 * height down and snapping it up.
 */
static void camera_car_place(Object *obj, Object_Racer *racer, f32 sineOffset, f32 cosOffset, f32 heightOffset) {
    f32 xOffset;
    f32 yOffset;
    f32 zOffset;
    f32 sideOffset;
    f32 targetY;
    f32 heightDiff;

    xOffset = obj->trans.x_position - (racer->ox1 * racer->camera_zoom);
    yOffset = obj->trans.y_position - (racer->oy1 * racer->camera_zoom);
    zOffset = obj->trans.z_position - (racer->oz1 * racer->camera_zoom);
    sideOffset = sins_f(racer->cameraYaw + 0x4000) * racer->cameraLateralOffset;
    gCameraObject->trans.x_position = xOffset + sineOffset + sideOffset;
    targetY = yOffset + heightOffset;
    heightDiff = (gCameraObject->trans.y_position - targetY) * 0.25;
    if (heightDiff < -2.0) {
        gCameraObject->trans.y_position -= heightDiff + 2.0;
    }
    gCameraObject->trans.y_position -= heightDiff * 0.25;
    if (heightDiff > 0.0f || gRaceStartTimer) {
        gCameraObject->trans.y_position = targetY;
    }
    sideOffset = (-coss_f(racer->cameraYaw + 0x4000) * racer->cameraLateralOffset);
    gCameraObject->trans.z_position = sideOffset + (zOffset + cosOffset);
}

/**
 * Faces the camera along the racer's camera yaw, rolls it with the racer (not while drifting
 * or in dialogue), and records the camera's level segment.
 */
static void camera_car_roll(Object *obj, Object_Racer *racer) {
    s32 segmentIndex;
    s32 angle;
    s32 newAngle;

    gCameraObject->trans.rotation.y_rotation = racer->cameraYaw;
    newAngle = obj->trans.rotation.z_rotation;
    if ((racer->drift_direction && racer->brake > 0.0) || gDialogueCameraAngle) {
        newAngle = 0;
    }
    angle = gCameraObject->trans.rotation.z_rotation;
    gCameraObject->trans.rotation.z_rotation -= (angle + newAngle) >> 4;
    if (gCameraObject->trans.rotation.z_rotation > 0x2000) {
        gCameraObject->trans.rotation.z_rotation = 0x2000;
    }
    if (gCameraObject->trans.rotation.z_rotation < -0x2000) {
        gCameraObject->trans.rotation.z_rotation = -0x2000;
    }
    gCameraObject->trans.y_position -= racer->cameraLateralOffset * sins_f(gCameraObject->trans.rotation.z_rotation);
    segmentIndex = get_level_segment_index_from_position(
        gCameraObject->trans.x_position, gCameraObject->trans.y_position, gCameraObject->trans.z_position);
    if (segmentIndex != SEGMENT_NONE) {
        gCameraObject->cameraSegmentID = segmentIndex;
    }
    racer->cameraYaw = gCameraObject->trans.rotation.y_rotation;
}

/**
 * Handles the camera movement when the player is driving a car: a boom behind the racer whose
 * length and pitch follow the speed, braking, drifting, zoom setting and viewport count.
 */
void update_camera_car(f32 updateRate, Object *obj, Object_Racer *racer) {
    f32 baseDistance = 120.0f;
    f32 targetHeight = 50.0f;
    s32 angle;
    f32 sineOffset;
    f32 cosOffset;
    f32 boomFlat;
    f32 pitchSin;
    s32 angleVel;
    f32 brakeVar;
    f32 forwardVel;
    s32 baseAngle = 0x400;
    s32 numViewports;
    s32 delta = (s32) updateRate;

    numViewports = cam_get_viewport_layout();
    if (numViewports == 1) {
        baseDistance = 192.0f;
        baseAngle = 0x200;
    } else if (numViewports > 2) {
        baseDistance = 100.0f;
        targetHeight = 40.0f;
    }
    numViewports = boss_race_number();
    if (numViewports == 1 || numViewports == 4) {
        baseDistance = 144.0f;
        targetHeight = 110.0f;
        if (gCameraObject->zoom == ZOOM_VERY_CLOSE) {
            baseDistance += 30.0f;
        }
        baseAngle = 0xD00;
    }
    racer->cameraYaw = (-racer->steerVisualRotation - racer->cameraYawOffset) + 0x8000;
    angle = racer->cameraYawOffset >> 3;
    if (angle > 0x400) {
        angle = 0x400;
    }
    if (angle < -0x400) {
        angle = -0x400;
    }
    angle *= delta;
    if (angle > 0 && angle < racer->cameraYawOffset) {
        racer->cameraYawOffset -= angle;
    }
    if (angle < 0) {
        if (racer->cameraYawOffset < angle) {
            racer->cameraYawOffset -= angle;
        }
    }
    brakeVar = racer->brake;
    forwardVel = racer->forwardVel;
    switch (gCameraObject->zoom) {
        case 1:
            baseDistance += 35.0f;
            break;
        case 2:
            baseDistance -= 35.0f;
            targetHeight -= 10.0f;
            break;
        case 3:
            baseDistance -= 53.0f;
            targetHeight -= 5.0f;
            brakeVar = 0.0f;
            forwardVel *= 0.25;
            break;
    }
    if (racer->groundedWheels > 2 || racer->waterTimer != 0) {
        angleVel = (obj->trans.rotation.x_rotation);
        if (angleVel > 0) {
            angleVel -= 0x71C;
            if (angleVel < 0) {
                angleVel = 0;
            }
            angleVel >>= 1;
        } else {
            angleVel += 0x71C;
            if (angleVel > 0) {
                angleVel = 0;
            }
        }
        angleVel = baseAngle - angleVel;
        angle = angleVel - (u16) gCameraObject->trans.rotation.x_rotation;
        if (angle > 0x8000) {
            angle -= 0xFFFF;
        }
        if (angle < -0x8000) {
            angle += 0xFFFF;
        }
        gCameraObject->trans.rotation.x_rotation += (angle * delta) >> 4;
    }
    baseDistance = camera_car_target_boom(racer, baseDistance, forwardVel, brakeVar);
    camera_car_ease_boom(baseDistance, targetHeight);
    pitchSin = sins_f(gCameraObject->trans.rotation.x_rotation - baseAngle);
    sineOffset = coss_f(gCameraObject->trans.rotation.x_rotation - baseAngle);
    boomFlat = (gCameraObject->boomLength * sineOffset) - (gCameraObject->cam_unk_20 * pitchSin);
    targetHeight = (gCameraObject->boomLength * pitchSin) + (gCameraObject->cam_unk_20 * sineOffset);
    sineOffset = sins_f(-racer->cameraYaw + 0x8000) * boomFlat;
    cosOffset = coss_f(-racer->cameraYaw + 0x8000) * boomFlat;
    camera_car_lateral_offset(racer);
    if (racer->spinout_timer) {
        racer->camera_zoom -= racer->camera_zoom * 0.25;
    } else {
        racer->camera_zoom += (10.0 - racer->camera_zoom) * 0.25;
    }
    camera_car_place(obj, racer, sineOffset, cosOffset, targetHeight);
    camera_car_roll(obj, racer);
}

/**
 * Handles the camera movement when the player has finished a challenge.
 * If the player still exists, it will follow and rotate around the player.
 * Otherwise, it will follow a set path while rotating.
 */
void update_camera_finish_challenge(UNUSED f32 updateRate, Object *obj, Object_Racer *racer) {
    s32 segmentIndex;
    f32 heightDiff;
    f32 zOffset;
    f32 xOffset;

    gCameraObject->trans.rotation.y_rotation += 0x200;
    if (1) {} // Fakematch
    gCameraObject->trans.rotation.x_rotation = 0x400;
    gCameraObject->trans.rotation.z_rotation = 0;
    gCameraObject->boomLength = 150.0f;
    xOffset = sins_f(0x8000 - gCameraObject->trans.rotation.y_rotation) * gCameraObject->boomLength;
    zOffset = coss_f(0x8000 - gCameraObject->trans.rotation.y_rotation) * gCameraObject->boomLength;
    gCameraObject->trans.x_position = obj->trans.x_position + xOffset;
    heightDiff = (gCameraObject->trans.y_position - (obj->trans.y_position + 45.0f)) * 0.25;
    if (heightDiff < -2.0) {
        gCameraObject->trans.y_position = (gCameraObject->trans.y_position - (heightDiff + 2.0));
    }
    if (heightDiff > 0.0f) {
        gCameraObject->trans.y_position = obj->trans.y_position + 45.0f;
    }
    gCameraObject->trans.z_position = obj->trans.z_position + zOffset;
    segmentIndex = get_level_segment_index_from_position(
        gCameraObject->trans.x_position, gCameraObject->trans.y_position, gCameraObject->trans.z_position);
    if (segmentIndex != SEGMENT_NONE) {
        gCameraObject->cameraSegmentID = segmentIndex;
    }
    racer->cameraYaw = gCameraObject->trans.rotation.y_rotation;
}

/**
 * Handles the camera movement when the player has finished the race.
 * Uses a more cinematic approach to following the player, using pre-existing points in the track to look from.
 */
void update_camera_finish_race(UNUSED f32 updateRate, Object *obj, Object_Racer *racer) {
    s32 cameraID;
    Object *cam;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    f32 distance;

    cameraID = racer->spectateCamID;
    cam = spectate_nearest(obj, &cameraID);
    if (cam == NULL) {
        gCameraObject->mode = CAMERA_FINISH_CHALLENGE;
        return;
    }
    racer->spectateCamID = cameraID;
    gCameraObject->trans.x_position = cam->trans.x_position;
    gCameraObject->trans.y_position = cam->trans.y_position;
    gCameraObject->trans.z_position = cam->trans.z_position;
    diffX = gCameraObject->trans.x_position - obj->trans.x_position;
    diffY = gCameraObject->trans.y_position - obj->trans.y_position;
    diffZ = gCameraObject->trans.z_position - obj->trans.z_position;
    distance = sqrtf((diffX * diffX) + (diffZ * diffZ));
    gCameraObject->trans.rotation.y_rotation = 0x8000 - atan2s(diffX, diffZ);
    gCameraObject->trans.rotation.x_rotation = atan2s((s32) diffY, (s32) distance);
    gCameraObject->trans.rotation.z_rotation = 0;
    gCameraObject->cameraSegmentID = get_level_segment_index_from_position(gCameraObject->trans.x_position, racer->oy1,
                                                                           gCameraObject->trans.z_position);
}

/**
 * Stops the camera in place when set, while still pointing in the direction of the player.
 * Used when entering doors.
 */
void update_camera_fixed(f32 updateRate, Object *obj, Object_Racer *racer) {
    s32 updateRateF;
    f32 diffX;
    f32 diffZ;
    updateRateF = (s32) updateRate;
    diffX = gCameraObject->trans.x_position - obj->trans.x_position;
    diffZ = gCameraObject->trans.z_position - obj->trans.z_position;
    gCameraObject->trans.rotation.y_rotation +=
        ((((-atan2s(diffX, diffZ)) - gCameraObject->trans.rotation.y_rotation) + 0x8000) * updateRateF) >> 4;
    gCameraObject->trans.rotation.z_rotation -= ((s32) (gCameraObject->trans.rotation.z_rotation * updateRateF)) >> 4;
    gCameraObject->cameraSegmentID = get_level_segment_index_from_position(gCameraObject->trans.x_position, racer->oy1,
                                                                           gCameraObject->trans.z_position);
}

/**
 * Iterate through every checkpoint and find the current location of the racer.
 * Sets the position arguments based on the intended location the spline functions return.
 * These coordinates are then used to generate a velocity.
 */
void set_position_goal_from_path(UNUSED Object *obj, Object_Racer *racer, f32 *x, f32 *y, f32 *z) {
    CheckpointNode *checkpoint;
    s32 splinePos;
    s32 destReached;
    s32 splineEnd;
    f32 splineX[5];
    f32 splineY[5];
    f32 splineZ[5];
    f32 magnitude;
    s32 i;

    splineEnd = get_checkpoint_count();

    if (splineEnd == 0) {
        return;
    }

    magnitude = 1.0 - racer->checkpoint_distance;
    if (magnitude < 0.0f) {
        magnitude = 0.0f;
    }
    if (racer->nextCheckpoint) {} // Fakematch
    splinePos = racer->nextCheckpoint - 2;
    if (splinePos < 0) {
        splinePos += splineEnd;
    }
    for (i = 0; i < 5; i++) {
        checkpoint = find_next_checkpoint_node(splinePos, racer->isOnAlternateRoute);
        splineX[i] = checkpoint->x;
        splineY[i] = checkpoint->y;
        splineZ[i] = checkpoint->z;
        splinePos++;
        if (splinePos == splineEnd) {
            splinePos = 0;
        }
    }
    destReached = FALSE;
    if (magnitude >= 1.0) {
        destReached = TRUE;
        magnitude -= 1.0;
    }
    *x = catmull_rom_interpolation(splineX, destReached, magnitude);
    *y = catmull_rom_interpolation(splineY, destReached, magnitude);
    *z = catmull_rom_interpolation(splineZ, destReached, magnitude);
}
