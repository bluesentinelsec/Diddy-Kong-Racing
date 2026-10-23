#include "objects_internal.h"

/**
 * Make the next call of move_object never mark the object as out of bounds.
 * Official Name: objMoveXYZnocheck
 */
void ignore_bounds_check(void) {
    gNoBoundsCheck = TRUE;
}

/**
 * Sets the new position of the object using the differences given.
 * Compares against the outer edges of the level geometry to decide wether or not to apply a segment ID.
 * Official Name: objMoveXYZ
 */
s32 move_object(Object *obj, f32 xPos, f32 yPos, f32 zPos) {
    s32 segmentID;
    f32 x1, x2, y1, y2, z1, z2;
    f32 newXPos;
    f32 newYPos;
    f32 newZPos;
    LevelModel *levelModel;
    LevelModelSegmentBoundingBox *box;
    s32 outsideBBox;
    s32 outOfBounds;
    s32 intXPos, intYPos, intZPos;

    levelModel = get_current_level_model();
    newXPos = obj->trans.x_position + xPos;
    newYPos = obj->trans.y_position + yPos;
    newZPos = obj->trans.z_position + zPos;
    if (levelModel == NULL) {
        gNoBoundsCheck = FALSE;
        return FALSE;
    }
    outOfBounds = FALSE;
    x2 = (levelModel->upperXBounds + 1000.0);
    //!@bug should've compared against "obj->trans.x_position"
    if (newXPos > x2) {
        outOfBounds = TRUE;
    }
    x1 = (levelModel->lowerXBounds - 1000.0);
    if (obj->trans.x_position < x1) {
        outOfBounds = TRUE;
    }
    if (1) {}
    if (1) {}
    if (1) {} // Fakematch
    y2 = (levelModel->upperYBounds + 3000.0);
    if (obj->trans.y_position > y2) {
        outOfBounds = TRUE;
    }
    y1 = (levelModel->lowerYBounds - 500.00);
    if (obj->trans.y_position < y1) {
        outOfBounds = TRUE;
    }
    z2 = (levelModel->upperZBounds + 1000.0);
    if (obj->trans.z_position > z2) {
        outOfBounds = TRUE;
    }
    z1 = (levelModel->lowerZBounds - 1000.0);
    if (obj->trans.z_position < z1) {
        outOfBounds = TRUE;
    }
    if (gNoBoundsCheck) {
        outOfBounds = FALSE;
    }

    gNoBoundsCheck = FALSE;
    if (outOfBounds) {
        obj->segmentID = -1;
        return TRUE;
    }

    obj->trans.x_position = newXPos;
    obj->trans.y_position = newYPos;
    obj->trans.z_position = newZPos;
    box = block_boundbox(obj->segmentID);

    // For some reason the XYZ positions are converted into integers for the next section
    intXPos = newXPos, intYPos = newYPos, intZPos = newZPos;

    if (box == NULL) {
        obj->segmentID = get_level_segment_index_from_position(intXPos, intYPos, intZPos);
        return FALSE;
    } else {
        outsideBBox = FALSE;
        if (box->x2 < intXPos || intXPos < box->x1) {
            outsideBBox = TRUE;
        }
        if (box->y2 < intYPos || intYPos < box->y1) {
            outsideBBox = TRUE;
        }
        if (box->z2 < intZPos || intZPos < box->z1) {
            outsideBBox = TRUE;
        }
        if (outsideBBox) {
            segmentID = get_level_segment_index_from_position(intXPos, intYPos, intZPos);
            if (segmentID != -1) {
                obj->segmentID = segmentID;
            }
        }
    }
    return FALSE;
}

s32 get_first_active_object(s32 *retObjCount) {
    s32 i, j;
    s32 minIndex, maxIndex;
    s32 breakLoop;

    *retObjCount = gObjectCount;
    if (gFirstActiveObjectId != 0) {
        // Already sorted
        return gFirstActiveObjectId;
    }

    i = gObjectListStart;
    j = gObjectCount - 1;
    minIndex = i;
    maxIndex = j;

    while (i <= j) {
        breakLoop = 0;
        while (i <= maxIndex && breakLoop == 0) {
            if (!(gObjPtrList[i]->trans.flags & OBJ_FLAGS_PARTICLE)) {
                if (gObjPtrList[i]->header->flags & HEADER_FLAGS_UNK_0001) {
                    i++;
                } else {
                    // Break the loop if neither OBJ_FLAGS_PARTICLE nor bit 1 in header->flags is set
                    breakLoop = -1;
                }
            } else {
                i++;
            }
        }

        breakLoop = 0;
        while (j >= minIndex && breakLoop == 0) {
            if (gObjPtrList[j]->trans.flags & OBJ_FLAGS_PARTICLE) {
                // Break the loop if OBJ_FLAGS_PARTICLE is set
                breakLoop = -1;
            } else if (!(gObjPtrList[j]->header->flags & HEADER_FLAGS_UNK_0001)) {
                j--;
            } else {
                // Break the loop if bit 1 in header->flags is set
                breakLoop = -1;
            }
        }

        if (i < j) {
            // Swap active and inactive objects
            Object *tempObject = gObjPtrList[i];
            gObjPtrList[i] = gObjPtrList[j];
            gObjPtrList[j] = tempObject;
            i++;
            j--;
        }
    }

    gFirstActiveObjectId = i;
    return i;
}

/**
 * Takes every object and sorts the main object list by distance to the camera.
 */
void sort_objects_by_dist(s32 startIndex, s32 lastIndex) {
    s32 i;
    s32 didNotSwap;
    Object *obj;

    if (lastIndex < startIndex) {
        return;
    }

    for (i = startIndex; i <= lastIndex; i++) {
        obj = gObjPtrList[i];
        if (obj != NULL) {
            if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
                // get_distance_to_camera calculates the distance to the camera from a XYZ location.
                obj->distanceToCamera =
                    -get_distance_to_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
            } else if (obj->header->flags & HEADER_FLAGS_UNK_0080) {
                obj->distanceToCamera += -16000.0f;
            } else {
                obj->distanceToCamera =
                    -get_distance_to_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
            }
        } else {
            //!@bug obj is NULL here, so it would probably cause a crash. Thankfully, gObjPtrList shouldn't have NULL
            //! objects in it.
            obj->distanceToCamera = 0.0f;
        }
    }

    // Keep swapping until all objects are sorted by the distance to the camera.
    do {
        didNotSwap = TRUE;
        for (i = startIndex; i < lastIndex; i++) {
            if (gObjPtrList[i]->distanceToCamera < gObjPtrList[i + 1]->distanceToCamera) {
                obj = gObjPtrList[i];
                gObjPtrList[i] = gObjPtrList[i + 1];
                gObjPtrList[i + 1] = obj;
                didNotSwap = FALSE;
            }
        }
    } while (!didNotSwap);
}

/**
 * Go through each object and detect potential interactions between each.
 * Add candidates to a list and calcualte their distances.
 */
void process_object_interactions(void) {
    Object *obj2;
    Object *obj;
    ObjectInteraction *objInteract;
    ObjectInteraction *objInteract2;
    f32 xDiff;
    f32 zDiff;
    f32 radius;
    s32 j;
    s32 i;
    s32 objsWithInteractives;
    Object *objList[257]; // 257 seems random, but it works for now.

    objsWithInteractives = 0;
    for (i = gObjectListStart; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE)) {
            objInteract = obj->interactObj;
            if (objInteract != NULL) {
                objList[objsWithInteractives] = obj;
                objsWithInteractives++;
                if (objInteract->hitboxKind != 2) {
                    objInteract->obj = NULL;
                    objInteract->flags &= ~(INTERACT_FLAGS_COLLIDED | INTERACT_FLAGS_PUSHING);
                    objInteract->distance = 255;
                }
            }
        }
    }

    gCollisionObjectCount = 0;

    for (i = 0; i < objsWithInteractives; i++) {
        obj = objList[i];
        objInteract = obj->interactObj;
        if (objInteract->hitboxKind == 2 && gCollisionObjectCount < 20) {
            gCollisionObjects[gCollisionObjectCount] = obj;
            gCollisionObjectCount++;
        }
        if (objInteract->flags & INTERACT_FLAGS_UNK_0004) {
            for (j = 0; j < objsWithInteractives; j++) {
                if (i != j) {
                    obj2 = objList[j];
                    objInteract2 = obj2->interactObj;
                    if (objInteract2->flags & (INTERACT_FLAGS_SOLID | INTERACT_FLAGS_TANGIBLE)) {
                        if (objInteract2->hitboxKind == 3) {
                            object_collide_with_object(obj, obj2);
                        } else if (objInteract2->hitboxKind != 2) {
                            xDiff = obj->trans.x_position - obj2->trans.x_position;
                            zDiff = obj->trans.z_position - obj2->trans.z_position;
                            if (objInteract2->flags & INTERACT_FLAGS_UNK_0020) {
                                radius = 0x400000; // 4194304.0f;
                            } else {
                                radius = 0x40000; // 262144.0f;
                            }
                            if (((xDiff * xDiff) + (zDiff * zDiff)) < radius) {
                                object_apply_interaction(obj, obj2);
                            }
                        }
                    }
                }
            }
        }
        if (objInteract->flags & INTERACT_FLAGS_UNK_0100) {
            for (j = 0; j < objsWithInteractives; j++) {
                if (i != j) {
                    obj2 = objList[j];
                    objInteract2 = obj2->interactObj;
                    if (objInteract2->hitboxKind == 3) {
                        object_collide_with_object(obj, obj2);
                    }
                }
            }
        }
    }

    for (i = 0; i < objsWithInteractives; i++) {
        obj = objList[i];
        objInteract = obj->interactObj;
        objInteract->x_position = obj->trans.x_position;
        objInteract->y_position = obj->trans.y_position;
        objInteract->z_position = obj->trans.z_position;
    }
}

/**
 * Remember the nearest object each side has met this frame. The distance saturates at
 * 255; INTERACT_FLAGS_UNK_0020 counts it at an eighth.
 */
static void interaction_track_nearest(ObjectInteraction *interact, Object *other, f32 distance) {
    s32 d;

    d = (s32) (f32) (s32) distance;
    if (interact->flags & INTERACT_FLAGS_UNK_0020) {
        d >>= 3;
    }
    if (d > 255) {
        d = 255;
    }
    if (interact->distance >= d) {
        interact->obj = other;
        interact->distance = d;
    }
}

/**
 * When `obj` has moved more than a unit since its interaction position was recorded,
 * use the distance from `other` to the closest point on that movement instead, so a fast
 * object cannot tunnel through. Returns the distance to use.
 */
static f32 interaction_closest_approach(Object *obj, ObjectInteraction *interact, Object *other, s32 flat,
                                        f32 distance) {
    f32 moveX;
    f32 moveY;
    f32 moveZ;
    f32 t;
    f32 px;
    f32 py;
    f32 pz;

    moveX = obj->trans.x_position - interact->x_position;
    moveY = obj->trans.y_position - interact->y_position;
    moveZ = obj->trans.z_position - interact->z_position;
    if (flat) {
        moveY = 0.0f;
    }
    t = moveX * moveX + moveY * moveY + moveZ * moveZ;
    if (t > 1.0) {
        t = ((other->trans.x_position - interact->x_position) * moveX +
             (other->trans.y_position - interact->y_position) * moveY +
             (other->trans.z_position - interact->z_position) * moveZ) /
            t;
        if (t >= 0.0f && t <= 1.0) {
            px = interact->x_position + t * moveX;
            py = interact->y_position + t * moveY;
            pz = interact->z_position + t * moveZ;
            distance = sqrtf((px - other->trans.x_position) * (px - other->trans.x_position) +
                             (py - other->trans.y_position) * (py - other->trans.y_position) +
                             (pz - other->trans.z_position) * (pz - other->trans.z_position));
        }
    }
    return distance;
}

/**
 * Push `obj` out of an immovable object. A racer that hits it hard enough also loses speed
 * and is bumped sideways for a few frames: hovercraft glance off, other vehicles bounce.
 */
static void interaction_push_into_solid(Object *obj, Object *other, ObjectInteraction *interactOther, f32 pushX,
                                        f32 pushY, f32 pushZ, f32 overlap, f32 invUpdateRate) {
    Object_Racer *racer;
    s32 bumped;
    f32 side;

    obj->trans.x_position -= pushX;
    obj->trans.y_position -= pushY;
    obj->trans.z_position -= pushZ;
    pushX *= invUpdateRate;
    pushZ *= invUpdateRate;
    if (obj->behaviorId != BHV_RACER) {
        return;
    }
    racer = obj->racer;
    bumped = FALSE;
    if (racer->vehicleID == VEHICLE_HOVERCRAFT) {
        if (overlap > 0.3) {
            if (overlap > 1.0) {
                bumped = TRUE;
            }
            if (bumped) {
                obj->x_velocity *= 0.8;
                obj->z_velocity *= 0.8;
            }
            if (bumped) {
                interactOther->flags |= INTERACT_FLAGS_COLLIDED;
                // Which side of the velocity line the obstacle is on decides the bump direction.
                side = (obj->trans.x_position * obj->z_velocity - obj->trans.z_position * obj->x_velocity);
                side = (other->trans.x_position * obj->z_velocity - other->trans.z_position * obj->x_velocity) - side;
                racer->bumpTimer = 7;
                if (side >= 0.0f) {
                    racer->bumpVelZ = obj->x_velocity * 0.1;
                    racer->bumpVelX = -obj->z_velocity * 0.1;
                } else {
                    racer->bumpVelZ = -obj->x_velocity * 0.1;
                    racer->bumpVelX = obj->z_velocity * 0.1;
                }
            }
        }
    } else {
        if (overlap > 0.3) {
            if (overlap > 1.0) {
                bumped = TRUE;
            }
            obj->x_velocity -= pushX;
            obj->z_velocity -= pushZ;
            racer->velocity = overlap * 0.25;
            racer->lateral_velocity = 0.0f;
        }
        if (bumped) {
            interactOther->flags |= INTERACT_FLAGS_COLLIDED;
            side = obj->trans.x_position * obj->z_velocity - obj->trans.z_position * obj->x_velocity;
            side = other->trans.x_position * obj->z_velocity - other->trans.z_position * obj->x_velocity - side;
            if (side >= 0.0f) {
                side = 2.0f;
            } else {
                side = -2.0f;
            }
            racer->bumpTimer = 7;
            racer->bumpVelX = racer->ox3 * side * racer->velocity;
            racer->bumpVelZ = racer->oz3 * side * racer->velocity;
        }
    }
    if (bumped && racer->playerIndex != -1) {
        racer_update_local_velocity(obj, racer);
    }
}

/**
 * Two movable objects share the push. Racers also trade velocity, weighted by their push
 * forces, and a shielded racer spins the other out.
 */
static void interaction_push_apart(Object *obj, Object *other, ObjectInteraction *interact,
                                   ObjectInteraction *interactOther, f32 pushX, f32 pushY, f32 pushZ, f32 overlap,
                                   f32 invUpdateRate) {
    Object_Racer *racer;
    Object_Racer *racerOther;
    f32 weight;
    f32 weightObj;
    s32 bumped;

    pushX *= 0.5;
    pushY *= 0.5;
    pushZ *= 0.5;
    if (interact->pushForce != 0) {
        obj->trans.x_position -= pushX;
        obj->trans.y_position -= pushY;
        obj->trans.z_position -= pushZ;
    }
    other->trans.x_position += pushX;
    other->trans.y_position += pushY;
    other->trans.z_position += pushZ;

    pushX *= invUpdateRate;
    pushZ *= invUpdateRate;
    if (obj->behaviorId != BHV_RACER || other->behaviorId != BHV_RACER) {
        return;
    }
    weight = 1.0;
    weight += (f32) (interact->pushForce - interactOther->pushForce) * 0.3;

    racer = obj->racer;
    racerOther = other->racer;
    if (racer->shieldType != 0 && racerOther->shieldType == 0) {
        racerOther->spinout_timer = racer->shieldType;
    }
    if (racerOther->shieldType != 0 && racer->shieldType == 0) {
        racer->spinout_timer = racerOther->shieldType;
    }

    bumped = FALSE;
    if (racer->vehicleID == VEHICLE_HOVERCRAFT) {
        bumped = TRUE;
    } else if (overlap > 0.1) {
        bumped = TRUE;
        pushX *= 0.5;
        pushZ *= 0.5;
    }
    if (bumped) {
        if (interact->pushForce != 0) {
            weightObj = 2.0 - weight;
            obj->x_velocity -= pushX * weightObj;
            obj->z_velocity -= pushZ * weightObj;
            racer_update_local_velocity(obj, racer);
        }
        other->x_velocity += pushX * weight;
        other->z_velocity += pushZ * weight;
        racer_update_local_velocity(other, racerOther);
    }
}

/**
 * Resolve one pair of interacting objects: record each as the other's nearest, then if
 * `other` is solid and their hitboxes overlap, separate them along the line between
 * their interaction positions and apply the bump.
 */
void object_apply_interaction(Object *obj, Object *other) {
    f32 distance;
    f32 minDistance;
    f32 overlap;
    f32 invUpdateRate;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 sepX;
    f32 sepY;
    f32 sepZ;
    f32 sepLength;
    ObjectInteraction *interact;
    ObjectInteraction *interactOther;

    dx = other->trans.x_position - obj->trans.x_position;
    dy = other->trans.y_position - obj->trans.y_position;
    dz = other->trans.z_position - obj->trans.z_position;

    interact = obj->interactObj;
    interactOther = other->interactObj;
    invUpdateRate = 1 / gObjectUpdateRateF;

    if (interactOther->hitboxKind == 1) {
        dy = -dy;
        if (dy < interactOther->yRangeMin * 10.0f || interactOther->yRangeMax * 10.0f < dy) {
            return;
        }
        dy = 0.0f;
    }
    if (interactOther->hitboxKind == 4 && dy < 0.0f) {
        dy *= 0.3;
    }

    distance = sqrtf(dx * dx + dy * dy + dz * dz);
#if VERSION == VERSION_80
    if (distance > 4000.0f) {
        return;
    }
    if (distance < -4000.0f) {
        return;
    }
#endif
    interaction_track_nearest(interact, other, distance);
    interaction_track_nearest(interactOther, obj, distance);

    if (!(interactOther->flags & INTERACT_FLAGS_SOLID)) {
        return;
    }
    minDistance = interactOther->hitboxRadius + interact->hitboxRadius;
    distance = interaction_closest_approach(obj, interact, other, interactOther->hitboxKind == 1, distance);
    if (!(distance < minDistance && distance > 0.0f)) {
        return;
    }

    // Separate along the line between the interaction positions, or between the objects
    // themselves when those coincide.
    sepX = interactOther->x_position - interact->x_position;
    sepY = interactOther->y_position - interact->y_position;
    sepZ = interactOther->z_position - interact->z_position;
    if (interactOther->hitboxKind == 1) {
        sepY = 0.0f;
    }
    sepLength = sqrtf(sepX * sepX + sepY * sepY + sepZ * sepZ);
    if (sepLength > 0.0f) {
        dx = sepX / sepLength;
        dy = sepY / sepLength;
        dz = sepZ / sepLength;
    } else {
        dx /= distance;
        dy /= distance;
        dz /= distance;
    }
    overlap = sepLength - distance;
    if (overlap < 0.0f) {
        overlap = -overlap;
    }
    dx *= overlap;
    dy *= overlap;
    dz *= overlap;
    overlap *= invUpdateRate;
    interact->flags |= INTERACT_FLAGS_PUSHING;
    interactOther->flags |= INTERACT_FLAGS_PUSHING;
    if (interactOther->pushForce == 0) {
        interaction_push_into_solid(obj, other, interactOther, dx, dy, dz, overlap, invUpdateRate);
        return;
    }
    interaction_push_apart(obj, other, interact, interactOther, dx, dy, dz, overlap, invUpdateRate);
}

/** Splits the racer's world velocity into forward and lateral components in its own frame. */
void racer_update_local_velocity(Object *obj, Object_Racer *racer) {
    s32 shakeMagnitude;
    s32 volume;
    s32 angle;
    f32 startVelocity;
    f32 cosAngle;
    f32 sinAngle;

    startVelocity = racer->velocity;
    angle = racer->steerVisualRotation;
    if (racer->vehicleID == VEHICLE_CAR) {
        if (racer->drift_direction != 0) {
            angle += racer->driftHeadingOffset;
            angle = (s16) angle;
        }
    }
    cosAngle = coss_f(-angle);
    sinAngle = sins_f(-angle);
    racer->lateral_velocity = (obj->x_velocity * cosAngle) + (obj->z_velocity * sinAngle);
    racer->velocity = (-obj->x_velocity * sinAngle) + (obj->z_velocity * cosAngle);
    if (racer->playerIndex != PLAYER_COMPUTER) {
        volume = (startVelocity - racer->velocity) * 14.0f;
        if (volume < 0) {
            volume = -volume;
        }
        volume += 35;
        if (volume > 127) {
            volume = 127;
        }
        if (racer->crashSoundCooldown == 0) {
            sound_play(SOUND_CRASH_CHARACTER, &racer->crashSoundHandle);
            sound_volume_set_relative(SOUND_CRASH_CHARACTER, racer->crashSoundHandle, volume);
        }
        if (racer->crashSoundCooldown == 0 && volume > 55) {
            if (!racer->raceFinished) {
                rumble_set(racer->playerIndex, RUMBLE_TYPE_18);
            }
            racer->driveAnimFlags |= 8;
        }
        if (volume > 55) {
            play_random_character_voice(obj, SOUND_VOICE_CHARACTER_NEGATIVE, 8, 1);
        }
        shakeMagnitude = (startVelocity - racer->velocity);
        if (shakeMagnitude < 0) {
            shakeMagnitude = -shakeMagnitude;
        }
        if (shakeMagnitude > 3) {
            shakeMagnitude = 3;
        }
        racer->crashSoundCooldown = 30;
        set_active_camera(racer->playerIndex);
        cam_get_active_camera()->shakeMagnitude = shakeMagnitude;
    }
}

/** Resolves a collision between an object and another object's collision model. */
void object_collide_with_object(Object *obj0, Object *obj1) {
    ObjectModel *objModel;
    s32 i;
    f32 temp;

#ifdef AVOID_UB
    MtxF obj1TransformMtx;
#else
    // THIS IS A HACK! Supposed to be a MtxF, but the stack ended up being too big.
    f32 pad[2];
    f32 obj1TransformMtx[4][3];
#endif

    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    ObjectInteraction *obj1Interact;
    ObjectInteraction *obj0Interact;
    Object_Racer *racer;
    f32 distance;
    f32 radius;
    ModelInstance *modInst;

    if (obj1->curVertData != NULL) {
        modInst = obj1->modelInstances[0];
        objModel = modInst->objModel;
        xDiff = obj0->trans.x_position - obj1->trans.x_position;
        yDiff = obj0->trans.y_position - obj1->trans.y_position;
        zDiff = obj0->trans.z_position - obj1->trans.z_position;
        if (!((objModel->boundingRadius + 50.0) < sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)))) {
            obj0Interact = obj0->interactObj;
            obj1Interact = obj1->interactObj;
            mtxf_from_transform((MtxF *) obj1TransformMtx, &obj1->trans);
            for (i = 0; i < objModel->collisionSpheresSize; i += 2) {
                xDiff = obj1->curVertData[objModel->collisionSpheres[i]].x;
                yDiff = obj1->curVertData[objModel->collisionSpheres[i]].y;
                zDiff = obj1->curVertData[objModel->collisionSpheres[i]].z;
                mtxf_transform_point((float (*)[4]) obj1TransformMtx, xDiff, yDiff, zDiff, &xDiff, &yDiff, &zDiff);
                temp = (((f32) objModel->collisionSpheres[i + 1] / 64) * obj1->trans.scale) * 50.0;
                xDiff -= obj0->trans.x_position;
                yDiff -= obj0->trans.y_position;
                zDiff -= obj0->trans.z_position;
                distance = sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff));
                temp += obj1Interact->hitboxRadius;
                if (distance < temp && distance > 0.0f) {
                    obj0Interact->flags |= INTERACT_FLAGS_PUSHING;
                    obj1Interact->flags |= INTERACT_FLAGS_PUSHING;
                    obj0Interact->obj = obj1;
                    obj1Interact->obj = obj0;
                    obj0Interact->distance = 0;
                    obj1Interact->distance = 0;
                    radius = (temp - distance) / distance;
                    distance = 2; // Needed
                    radius /= distance;
                    xDiff *= radius;
                    yDiff *= radius;
                    zDiff *= radius;
                    obj0->trans.x_position -= xDiff;
                    obj0->trans.y_position -= yDiff;
                    obj0->trans.z_position -= zDiff;

                    if (obj0->behaviorId == BHV_RACER) {
                        racer = obj0->racer;
                        if (!racer->raceFinished) {
                            rumble_set(racer->playerIndex, RUMBLE_TYPE_18);
                        }
                        if (racer->vehicleID == VEHICLE_HOVERCRAFT) {
                            if (radius > 0.1) {
                                obj0->x_velocity -= xDiff;
                                obj0->z_velocity -= zDiff;
                            }
                        } else if (radius > 0.3) {
                            obj0->x_velocity -= xDiff;
                            obj0->z_velocity -= zDiff;
                            racer->velocity = radius * 4.0f;
                            racer->lateral_velocity = 0.0f;
                        }
                    }
                }
            }
        }
    }
}

/** Prepares an object's collision transform and its models' collision data. */
void object_init_collision(Object *obj) {
    s32 i;

    obj->collisionData->mtxFlip = 0;
    obj_collision_transform(obj);
    obj_collision_transform(obj); // Not sure why they do this a second time.
    for (i = 0; i < obj->header->numberOfModelIds; i++) {
        if (obj->modelInstances[i] != NULL) {
            model_init_collision(obj->modelInstances[i]->objModel);
        }
    }
}

/**
 * Find the first butterfly node within range and return a pointer to it.
 */
Object *obj_butterfly_node(f32 x, f32 y, f32 z, f32 maxDistCheck, s32 dontCheckYAxis) {
    f32 diffY;
    f32 diffZ;
    f32 diffX;
    f32 distance;
    s32 i;
    Object *curObj;

    for (i = 0; i < gObjectCount; i++) {
        curObj = gObjPtrList[i];
        if (!(curObj->trans.flags & OBJ_FLAGS_PARTICLE) && curObj->behaviorId == BHV_ANIMATED_OBJECT_3) {
            diffX = curObj->trans.x_position - x;
            diffZ = curObj->trans.z_position - z;
            if (!dontCheckYAxis) {
                diffY = curObj->trans.y_position - y;
                distance = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
            } else {
                distance = sqrtf((diffX * diffX) + (diffZ * diffZ));
            }
            if (distance < maxDistCheck) {
                return curObj;
            }
        }
    }
    return NULL;
}

/**
 * Compare distance between itself and every racer.
 * Any racer within the radius is sorted from nearest to furthest.
 * Returns the number of racers within the radius.
 */
s32 obj_dist_racer(f32 x, f32 y, f32 z, f32 radius, s32 is2dCheck, Object **sortObj) {
    f32 distances[8];
    s32 i;
    s32 j;
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    s32 result;
    Object *racerObj;
    Object_Racer *racer;
    Object *swapObj;
    f32 swapf;

    result = 0;
    if (gNumRacers > 0) {
        for (i = 0; i < gNumRacers; i++) {
            racerObj = (*gRacers)[i];
            racer = racerObj->racer;
            if (racer->playerIndex >= 0 && racer->playerIndex < 4) {
                if (is2dCheck) {
                    xDiff = racerObj->trans.x_position - x;
                    zDiff = racerObj->trans.z_position - z;
                    yDiff = sqrtf((xDiff * xDiff) + (zDiff * zDiff));
                } else {
                    xDiff = racerObj->trans.x_position - x;
                    yDiff = racerObj->trans.y_position - y;
                    zDiff = racerObj->trans.z_position - z;
                    yDiff = sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff));
                }
                if (yDiff < radius) {
                    distances[result] = yDiff;
                    sortObj[result] = (*gRacers)[i];
                    result++;
                }
            }
        }
        if (result >= 2) {
            for (i = result - 1; i > 0; i--) {
                for (j = 0; j < i; j++) {
                    if (distances[j + 1] < distances[j]) {
                        swapf = distances[j];
                        swapObj = sortObj[j];
                        distances[j] = distances[j + 1];
                        sortObj[j] = sortObj[j + 1];
                        distances[j + 1] = swapf;
                        sortObj[j + 1] = swapObj;
                    }
                }
            }
        }
    }
    return result;
}

/**
 * Updates the object's collision transformation matrices.
 */
void obj_collision_transform(Object *obj) {
    ObjectTransform trans;
    s32 i;
    f32 inverseScale;
    MtxF *curMtx;
    MtxF inverseMtx;
    ObjectCollision *colData;

    colData = obj->collisionData;
    colData->mtxFlip = (colData->mtxFlip + 1) & 1;
#ifdef AVOID_UB
    curMtx = &colData->matrices[colData->mtxFlip];
#else
    curMtx = (MtxF *) &colData->_matrices[colData->mtxFlip << 1];
#endif
    trans.rotation.y_rotation = -obj->trans.rotation.y_rotation;
    trans.rotation.x_rotation = -obj->trans.rotation.x_rotation;
    trans.rotation.z_rotation = -obj->trans.rotation.z_rotation;
    trans.scale = 1.0f;
    trans.x_position = -obj->trans.x_position;
    trans.y_position = -obj->trans.y_position;
    trans.z_position = -obj->trans.z_position;
    mtxf_from_inverse_transform(curMtx, &trans);
    inverseScale = 1.0 / obj->trans.scale;
    i = 0;
    // Zero out the matrix.
    while (i < 16) {
        ((f32 *) inverseMtx)[i] = 0.0f;
        i++;
    }
    inverseMtx[0][0] = inverseScale;
    inverseMtx[1][1] = inverseScale;
    inverseMtx[2][2] = inverseScale;
    inverseMtx[3][3] = 1.0f;
    mtxf_mul(curMtx, &inverseMtx, curMtx);
    trans.scale = obj->trans.scale;
    trans.rotation.y_rotation = obj->trans.rotation.y_rotation;
    trans.rotation.x_rotation = obj->trans.rotation.x_rotation;
    trans.rotation.z_rotation = obj->trans.rotation.z_rotation;
    trans.x_position = obj->trans.x_position;
    trans.y_position = obj->trans.y_position;
    trans.z_position = obj->trans.z_position;
#ifdef AVOID_UB
    mtxf_from_transform((MtxF *) colData->matrices[(colData->mtxFlip + 2)], &trans);
#else
    mtxf_from_transform((MtxF *) colData->_matrices[(colData->mtxFlip + 2) << 1], &trans);
#endif
    colData->collidedObj = NULL;
}

s32 collision_objectmodel(Object *obj, s32 arg1, s32 *arg2, Vec3f *arg3, f32 *arg4, f32 *arg5, s8 *surface) {
    ModelInstance *modInst;
    s32 sp170;
    s32 sp16C;
    s32 sp168;
    s32 j;
    s32 sp160;
    f32 dist;
    Object *sp158;
    ObjectModel *sp154;
    f32 temp;
    CollisionPair *sp14C;
    f32 sp13C[4];
    f32 sp12C[4];
    f32 sp11C[4];
    s32 i;
    ObjectCollision *collision;
    s32 tempv0;
    f32 sp100[4];
    f32 spF0[4];
    f32 spE0[4];
    MtxF *spDC;
    s32 spB4[10];
    f32 sp8C[10];

    sp160 = 0;

    for (sp170 = 0; sp170 < gCollisionObjectCount; sp170++) {
        sp158 = gCollisionObjects[sp170];
        modInst = sp158->modelInstances[sp158->modelIndex];
        sp154 = modInst->objModel;

        temp = sp158->trans.x_position - obj->trans.x_position;
        dist = sqrtf(
            (temp) * (temp) +
            (sp158->trans.y_position - obj->trans.y_position) * (sp158->trans.y_position - obj->trans.y_position) +
            (sp158->trans.z_position - obj->trans.z_position) * (sp158->trans.z_position - obj->trans.z_position));

        j = dist;
        if (sp158->interactObj->flags & 0x20) {
            j >>= 3;
        }
        if (j > 255) {
            j = 255;
        }
        if (sp158->interactObj->distance > j) {
            sp158->interactObj->distance = j;
            sp158->interactObj->obj = obj;
        }

        if (dist - 25.0f < sp154->boundingRadius * sp158->trans.scale) {
            spB4[sp160] = sp170;
            sp8C[sp160] = dist;

            // clang-format off
            for (i = sp160; i > 0 && sp8C[i - 1] < sp8C[i]; i--) {\
                temp = sp8C[i];\
                sp16C = spB4[i];\
                sp8C[i] = sp8C[i - 1];\
                spB4[i] = spB4[i - 1];\
                sp8C[i - 1] = temp;
                spB4[i - 1] = sp16C;
            }
            // clang-format on

            sp160++;
        }
    }

    sp168 = 0;
    sp170 = 0;
    while (sp170 < sp160) {
        sp158 = gCollisionObjects[spB4[sp170]];
        modInst = sp158->modelInstances[sp158->modelIndex];
        sp154 = modInst->objModel;
        collision = sp158->collisionData;
#ifdef AVOID_UB
        spDC = &collision->matrices[((sp158->collisionData->mtxFlip + 1) & 1)];
#else
        spDC = (MtxF *) &collision->_matrices[((sp158->collisionData->mtxFlip + 1) & 1) << 1];
#endif

        sp14C = collision_pair_take(obj, sp158);
        if (sp14C != NULL) {
            for (i = 0, j = 0; j < arg1; j++, i += 3) {
                sp13C[j] = sp14C->prevLocalPoints[i + 0];
                sp12C[j] = sp14C->prevLocalPoints[i + 1];
                sp11C[j] = sp14C->prevLocalPoints[i + 2];
                mtxf_transform_point(*spDC, arg4[i], arg4[i + 1], arg4[i + 2], &sp100[j], &spF0[j], &spE0[j]);
            }
        } else {
            for (i = 0, j = 0; j < arg1; j++, i++) {
                mtxf_transform_point(*spDC, arg3[i].x, arg3[i].y, arg3[i].z, &sp13C[j], &sp12C[j], &sp11C[j]);
            }
        }

        for (i = 0, j = 0; j < arg1; j++, i += 3) {
            mtxf_transform_point(*spDC, arg4[i], arg4[i + 1], arg4[i + 2], &sp100[j], &spF0[j], &spE0[j]);
        }

        arg2[0] = 0;
        tempv0 = model_resolve_collisions(sp154, arg1, arg2, sp13C, sp12C, sp11C, sp100, spF0, spE0, arg5, surface,
                                          1.0 / sp158->trans.scale);
        if (tempv0 != 0) {

            // @fake
            if (!j) {}

            sp158->collisionData->collidedObj = obj;
        }

        if (gBossRaceIds[0] == 0) {
            sp14C = collision_pair_add(obj, sp158);
        }

#ifdef AVOID_UB
        spDC = &sp158->collisionData->matrices[(sp158->collisionData->mtxFlip + 2)];
#else
        spDC = (MtxF *) &sp158->collisionData->_matrices[(sp158->collisionData->mtxFlip + 2) << 1];
#endif

        // @fake
        if (sp158) {}

        sp16C = 1;
        for (i = 0, j = 0; j < arg1; j++, i += 3, sp16C <<= 1) {
            if (sp14C != NULL) {
                sp14C->prevLocalPoints[i + 0] = sp100[j];
                sp14C->prevLocalPoints[i + 1] = spF0[j];
                sp14C->prevLocalPoints[i + 2] = spE0[j];
            }
            if (tempv0 & sp16C) {
                mtxf_transform_point(*spDC, sp100[j], spF0[j], spE0[j], &arg4[i + 0], &arg4[i + 1], &arg4[i + 2]);
            }
        }

        sp168 |= tempv0;
        sp170++;
    }

    arg2[0] = 0;
    for (i = 0; i < 4; i++) {
        if (sp168 & (1 << i)) {
            (*arg2)++;
        }
    }

    return sp168;
}

/** Recent collision pairs, each with a two-tick cooldown so a contact is handled once. */
CollisionPair *collision_pair_take(Object *arg0, Object *arg1) {
    CollisionPair *entry;
    s16 i;

    for (i = 0; i < 16; i++) {
        entry = &gCollisionPairs[i];
        if (entry->ttl != 0 && entry->objA == arg0 && entry->objB == arg1) {
            entry->ttl = 0;
            return entry;
        }
    }
    return NULL;
}

CollisionPair *collision_pair_add(Object *obj1, Object *obj2) {
    CollisionPair *entry;
    s16 i;

    for (i = 0; i < 16; i++) {
        entry = &gCollisionPairs[i];
        if (entry->ttl == 0) {
            entry->objA = obj1;
            entry->objB = obj2;
            entry->ttl = 2;
            return entry;
        }
    }
    return NULL;
}

u32 collision_pairs_tick(void) {
    s16 i = 0;
    while (i < 16) {
        CollisionPair *temp = &gCollisionPairs[i];
        if (temp->ttl != 0) {
            temp->ttl--;
        }
        i++;
    }
#ifdef AVOID_UB
    return 0;
#endif
}

f32 distance_between_objects(Object *a, Object *b) {
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;

    xDiff = a->trans.x_position - b->trans.x_position;
    yDiff = a->trans.y_position - b->trans.y_position;
    zDiff = a->trans.z_position - b->trans.z_position;
    return sqrtf(((xDiff * xDiff) + (yDiff * yDiff)) + (zDiff * zDiff));
}
