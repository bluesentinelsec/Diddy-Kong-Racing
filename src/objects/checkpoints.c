#include "objects_internal.h"

/**
 * Gather the checkpoint objects of the current challenge type into gTrackCheckpoints.
 * Alternate-route checkpoints are numbered 255 above their main-route twin. Returns how
 * many alternates were found.
 */
static s32 checkpoints_collect(void) {
    Object *obj;
    LevelObjectEntry_Checkpoint *checkpointEntry;
    s32 checkpointID;
    s32 alternateCount;
    s32 i;

    alternateCount = 0;
    gNumberOfMainCheckpoints = 0;
    for (i = 0; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE) && obj->behaviorId == BHV_CHECKPOINT &&
            gNumberOfMainCheckpoints < MAX_CHECKPOINTS) {
            checkpointEntry = &obj->level_entry->checkpoint;
            if (checkpointEntry->vehicleType == gTajChallengeType) {
                gTrackCheckpoints[gNumberOfMainCheckpoints].obj = obj;
                checkpointID = checkpointEntry->index;
                if (checkpointEntry->isAltCheckpoint) {
                    checkpointID += 255;
                    alternateCount++;
                }
                gTrackCheckpoints[gNumberOfMainCheckpoints].checkpointID = checkpointID;
                gTrackCheckpoints[gNumberOfMainCheckpoints].altRouteID = -1;
                gNumberOfMainCheckpoints++;
            }
        }
    }
    return alternateCount;
}

/**
 * Bubble-sort the checkpoints by ID so the main route comes first in order, then the
 * alternates. Returns TRUE and the offending ID if two checkpoints share one.
 */
static s32 checkpoints_sort(s32 *duplicateID) {
    Object *obj;
    s32 tempCheckpointID;
    s32 duplicate;
    s32 isSorted;
    s32 i;

    duplicate = FALSE;
    do {
        isSorted = TRUE;
        for (i = 0; i < gNumberOfMainCheckpoints - 1; i++) {
            if (gTrackCheckpoints[i].checkpointID == gTrackCheckpoints[i + 1].checkpointID) {
                duplicate = TRUE;
                *duplicateID = gTrackCheckpoints[i].checkpointID;
            }
            if (gTrackCheckpoints[i + 1].checkpointID < gTrackCheckpoints[i].checkpointID) {
                tempCheckpointID = gTrackCheckpoints[i].checkpointID;
                obj = gTrackCheckpoints[i].obj;
                gTrackCheckpoints[i].checkpointID = gTrackCheckpoints[i + 1].checkpointID;
                gTrackCheckpoints[i].obj = gTrackCheckpoints[i + 1].obj;
                gTrackCheckpoints[i + 1].checkpointID = tempCheckpointID;
                gTrackCheckpoints[i + 1].obj = obj;
                isSorted = FALSE;
            }
        }
    } while (!isSorted);
    return duplicate;
}

/**
 * Pair each alternate checkpoint with the main-route checkpoint it replaces.
 */
static void checkpoints_link_alternates(void) {
    s32 mainID;
    s32 i;
    s32 j;

    for (i = gNumberOfMainCheckpoints; i < gNumberOfTotalCheckpoints; i++) {
        mainID = gTrackCheckpoints[i].checkpointID - 255;
        for (j = 0; j < gNumberOfMainCheckpoints; j++) {
            if (mainID == gTrackCheckpoints[j].checkpointID) {
                gTrackCheckpoints[j].altRouteID = i;
                gTrackCheckpoints[i].altRouteID = j;
                break;
            }
        }
    }
}

/**
 * Derive a checkpoint's plane (its facing and offset), size and the distances to the next
 * checkpoint on the main and alternate routes. From here on checkpointID holds the
 * checkpoint's radius, not its number.
 */
static void checkpoint_init_node(s32 i) {
    CheckpointNode *checkpoint;
    Object *obj;
    LevelObjectEntry_Checkpoint *checkpointEntry;
    MtxF mtx;
    ObjectTransform transform;
    f32 ox;
    f32 oy;
    f32 oz;
    s32 next;
    s32 altRouteId;

    checkpoint = &gTrackCheckpoints[i];
    obj = checkpoint->obj;
    checkpointEntry = &obj->level_entry->checkpoint;
    transform.rotation.y_rotation = obj->trans.rotation.y_rotation;
    transform.rotation.x_rotation = obj->trans.rotation.x_rotation;
    transform.rotation.z_rotation = obj->trans.rotation.z_rotation;
    transform.scale = 1.0f;
    transform.x_position = 0.0f;
    transform.y_position = 0.0f;
    transform.z_position = 0.0f;
    mtxf_from_transform(&mtx, &transform);
    mtxf_transform_point(mtx, 0.0f, 0.0f, 1.0f, &ox, &oy, &oz);
    checkpoint->rotationXFrac = ox;
    checkpoint->rotationYFrac = oy;
    checkpoint->rotationZFrac = oz;
    checkpoint->planeOffset =
        -(((obj->trans.x_position * ox) + (obj->trans.y_position * oy)) + (obj->trans.z_position * oz));
    checkpoint->x = obj->trans.x_position;
    checkpoint->y = obj->trans.y_position;
    checkpoint->z = obj->trans.z_position;
    checkpoint->scale = obj->trans.scale * 2;
    checkpoint->checkpointID = obj->trans.scale * 128.0f;

    if (i < gNumberOfMainCheckpoints) {
        next = i + 1;
    } else {
        next = gTrackCheckpoints[i].altRouteID + 1;
    }
    if (next == gNumberOfMainCheckpoints) {
        next = 0;
    }
    checkpoint->distance = distance_between_objects(obj, gTrackCheckpoints[next].obj);
    altRouteId = gTrackCheckpoints[next].altRouteID;
    if (altRouteId != -1) {
        checkpoint->altDistance = distance_between_objects(obj, gTrackCheckpoints[altRouteId].obj);
    } else {
        checkpoint->altDistance = checkpoint->distance;
    }

    checkpoint->laneOffsetX[0] = checkpointEntry->lane0OffsetX;
    checkpoint->laneOffsetY[0] = checkpointEntry->lane0OffsetY;
    checkpoint->laneAction[0] = checkpointEntry->lane0Action;
    checkpoint->laneOffsetX[1] = checkpointEntry->lane1OffsetX;
    checkpoint->laneOffsetY[1] = checkpointEntry->lane1OffsetY;
    checkpoint->laneAction[1] = checkpointEntry->lane1Action;
    checkpoint->laneOffsetX[2] = checkpointEntry->lane2OffsetX;
    checkpoint->laneOffsetY[2] = checkpointEntry->lane2OffsetY;
    checkpoint->laneAction[2] = checkpointEntry->lane2Action;
    checkpoint->laneOffsetX[3] = checkpointEntry->lane3OffsetX;
    checkpoint->laneOffsetY[3] = checkpointEntry->lane3OffsetY;
    checkpoint->laneAction[3] = checkpointEntry->lane3Action;
    checkpoint->indicatorType = checkpointEntry->indicatorType;
}

/**
 * Updates information pertaining to all checkpoints in the current race.
 */
void checkpoint_update_all(void) {
    s32 alternateCount;
    s32 duplicateID;
    s32 i;

    alternateCount = checkpoints_collect();
    if (checkpoints_sort(&duplicateID)) {
        set_render_printf_position(20, 220);
        render_printf("Error: Multiple checkpoint no: %d !!\n", duplicateID);
    }
    gNumberOfTotalCheckpoints = gNumberOfMainCheckpoints;
    gNumberOfMainCheckpoints -= alternateCount;
    checkpoints_link_alternates();
    for (i = 0; i < gNumberOfTotalCheckpoints; i++) {
        checkpoint_init_node(i);
    }
}

/**
 * Signed distance of a point from the checkpoint's plane, positive on the far side.
 */
static f32 checkpoint_plane_distance(CheckpointNode *node, f32 x, f32 y, f32 z) {
    return node->rotationXFrac * x + node->rotationYFrac * y + node->rotationZFrac * z + node->planeOffset;
}

/**
 * How far along the segment from `prev` to `current` the object is, as a fraction, found by
 * projecting the object onto each plane along `dir`. Also returns the distance still to go
 * to the current plane.
 */
static f32 checkpoint_progress(CheckpointNode *current, CheckpointNode *prev, Object *obj, f32 dirX, f32 dirY, f32 dirZ,
                               f32 *toNextOut) {
    f32 toNext;
    f32 nextLength;
    f32 fromPrev;
    f32 length;

    toNext = checkpoint_plane_distance(current, obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
    nextLength = current->rotationXFrac * dirX + current->rotationYFrac * dirY + current->rotationZFrac * dirZ;
    nextLength = -toNext / nextLength;

    fromPrev = checkpoint_plane_distance(prev, obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
    length = prev->rotationXFrac * dirX + prev->rotationYFrac * dirY + prev->rotationZFrac * dirZ;
    length = fromPrev / length;

    if (nextLength + length != 0.0) {
        length = nextLength / (nextLength + length);
    } else {
        length = 0.0f;
    }
    *toNextOut = nextLength;
    return length;
}

/**
 * Checks to see whether a racer or homing missile passed a checkpoint.
 * Return values:
 * - 0: Homing missile passed the checkpoint.
 * - -100: Racer passed the checkpoint.
 * - Otherwise: No checkpoint has been passed.
 */
s32 checkpoint_is_passed(s32 checkpointIndex, Object *obj, f32 objX, f32 objY, f32 objZ, f32 *checkpointDistance,
                         u8 *isOnAlternateRoute) {
    s32 retLength;
    s32 isAltCheckpoint;
    f32 distX, distY, distZ;
    f32 length;
    f32 nextLength;
    Object_Racer *racer;
    CheckpointNode *currentCheckpoint;
    CheckpointNode *prevCheckpoint;
    CheckpointNode *altRouteCheckpoint;

    if (gNumberOfMainCheckpoints == 0) {
        return 1;
    }

    currentCheckpoint = &gTrackCheckpoints[checkpointIndex];
    if (checkpointIndex != 0) {
        prevCheckpoint = &gTrackCheckpoints[checkpointIndex - 1];
    } else {
        prevCheckpoint = &gTrackCheckpoints[gNumberOfMainCheckpoints - 1];
    }

    if (*isOnAlternateRoute) {
        if (currentCheckpoint->altRouteID != -1) {
            currentCheckpoint = &gTrackCheckpoints[currentCheckpoint->altRouteID];
        }
        if (prevCheckpoint->altRouteID != -1) {
            prevCheckpoint = &gTrackCheckpoints[prevCheckpoint->altRouteID];
        }
    }

    // Entering an alternate route: switch to its checkpoint once within its radius.
    isAltCheckpoint = FALSE;
    if (!(*isOnAlternateRoute) && prevCheckpoint->altRouteID == -1 && currentCheckpoint->altRouteID != -1) {
        altRouteCheckpoint = &gTrackCheckpoints[currentCheckpoint->altRouteID];
        distX = altRouteCheckpoint->x - obj->trans.x_position;
        distY = altRouteCheckpoint->y - obj->trans.y_position;
        distZ = altRouteCheckpoint->z - obj->trans.z_position;
        if (sqrtf(distX * distX + distY * distY + distZ * distZ) < altRouteCheckpoint->checkpointID) {
            currentCheckpoint = altRouteCheckpoint;
            isAltCheckpoint = TRUE;
        }
    }

    // Unit direction from the previous checkpoint to the current one.
    distX = currentCheckpoint->x - prevCheckpoint->x;
    distY = currentCheckpoint->y - prevCheckpoint->y;
    distZ = currentCheckpoint->z - prevCheckpoint->z;
    length = sqrtf(distX * distX + distY * distY + distZ * distZ);
    if (length > 0.0) {
        distX *= 1.0f / length;
        distY *= 1.0f / length;
        distZ *= 1.0f / length;
    }

    length = checkpoint_progress(currentCheckpoint, prevCheckpoint, obj, distX, distY, distZ, &nextLength);
    *checkpointDistance = length;

    if (obj->behaviorId == BHV_RACER) {
        racer = obj->racer;
        if (racer->playerIndex == PLAYER_COMPUTER) {
            if (length < -0.3) {
                return -100;
            }
            if (length > 1.3) {
                return -100;
            }
        }
    }

    if (nextLength > 0) {
        retLength = length * 100.0f;
        if (retLength == 0) {
            retLength++;
        }
        return retLength;
    }

    // The object is past the plane of the current checkpoint.
    if (isAltCheckpoint) {
        *isOnAlternateRoute = TRUE;
    } else if (currentCheckpoint->altRouteID == -1) {
        *isOnAlternateRoute = FALSE;
    }
    distY = checkpoint_plane_distance(currentCheckpoint, objX, objY, objZ);
    if (distY > 0) {
        if (obj->behaviorId == BHV_RACER) {
            racer = obj->racer;
            if (currentCheckpoint->indicatorType != 0) {
                racer->indicator_type = currentCheckpoint->indicatorType;
                racer->indicator_timer = 120;
            }
        }
        prevCheckpoint = currentCheckpoint;
        checkpointIndex++;
        if (checkpointIndex == gNumberOfMainCheckpoints) {
            checkpointIndex = 0;
        }
        currentCheckpoint = &gTrackCheckpoints[checkpointIndex];
        //!@bug distY now holds the plane distance, not the direction's Y component.
        length = checkpoint_progress(currentCheckpoint, prevCheckpoint, obj, distX, distY, distZ, &nextLength);
        *checkpointDistance = length;
    } else {
        *checkpointDistance = 0.0;
    }
    return 0;
}

/**
 * Search and return Taj's overworld object.
 * Used for drawing his minimap position.
 */
Object *find_taj_object(void) {
    s32 i;
    Object *current_obj;
    for (i = gObjectListStart; i < gObjectCount; i++) {
        current_obj = gObjPtrList[i];
        if (!(current_obj->trans.flags & OBJ_FLAGS_PARTICLE) && (current_obj->behaviorId == BHV_PARK_WARDEN)) {
            return current_obj;
        }
    }
    return NULL;
}

/**
 * Finds the opponent to this racer in a relative position to them and calculates the distance to them.
 * The position argument is relative to the racer's current position and represents the number of
 * places ahead (positive) or behind (negative) the opponent is. So, for instance, if position is 1,
 * find the opponent one place ahead of the racer; if it's -1, find the opponent one place behind.
 */
Object *racer_find_nearest_opponent_relative(Object_Racer *racer, s32 position, f32 *distance) {
    UNUSED s32 pad;
    Object *tempRacerObj;
    position = (racer->racerOrder - position) - 1;
    if (position < 0 || position >= gNumRacers) {
        return NULL;
    }
    tempRacerObj = gRacersByPosition[position];
    if (tempRacerObj == NULL) {
        return NULL;
    }
    *distance = racer_calc_distance_to_opponent(racer, tempRacerObj->racer);
    return tempRacerObj;
}

/**
 * Calculates the distance between two racers. Traverses through the checkpoints
 * between the two racers to calculate the total distance.
 * If the second racer is ahead, the distance is positive, otherwise negative.
 */
f32 racer_calc_distance_to_opponent(Object_Racer *racer1, Object_Racer *racer2) {
    Object_Racer *temp_racer;
    f32 totalDistance;
    s32 r1_ccp;
    UNUSED s32 pad;
    s32 reverseOrder;
    s32 checkpointID;

    if (gNumberOfMainCheckpoints <= 0) {
        return 0.0f;
    }
    totalDistance = 0.0f;
    reverseOrder = FALSE;
    if (racer2->courseCheckpoint < racer1->courseCheckpoint) {
        temp_racer = racer1;
        racer1 = racer2;
        racer2 = temp_racer;
        reverseOrder = TRUE;
    }
    checkpointID = racer1->nextCheckpoint;
    for (r1_ccp = racer1->courseCheckpoint; r1_ccp < racer2->courseCheckpoint; r1_ccp++) {
        totalDistance += gTrackCheckpoints[checkpointID++].distance;
        if (checkpointID == gNumberOfMainCheckpoints) {
            checkpointID = 0;
        }
    }
    checkpointID = racer1->nextCheckpoint - 1;
    if (checkpointID < 0) {
        checkpointID = gNumberOfMainCheckpoints - 1;
    }
    totalDistance += (gTrackCheckpoints[checkpointID].distance * racer1->checkpoint_distance);
    checkpointID = racer2->nextCheckpoint - 1;
    if (checkpointID < 0) {
        checkpointID = gNumberOfMainCheckpoints - 1;
    }
    totalDistance -= (gTrackCheckpoints[checkpointID].distance * racer2->checkpoint_distance);
    if (reverseOrder) {
        totalDistance = -totalDistance;
    }
    return totalDistance;
}

/**
 * Traverses from the racer's position through the upcoming checkpoints
 * to the starting line and calculates the total distance.
 */
UNUSED f32 race_calc_distance_to_start_line(Object_Racer *racer) {
    f32 distLeft;
    s32 checkpointID;

    if (gNumberOfMainCheckpoints <= 0) {
        return 0.0f;
    }
    distLeft = 0.0f;
    for (checkpointID = racer->nextCheckpoint; checkpointID < gNumberOfMainCheckpoints; checkpointID++) {
        distLeft += gTrackCheckpoints[checkpointID].distance;
    }
    checkpointID = racer->nextCheckpoint - 1;
    if (checkpointID < 0) {
        checkpointID = gNumberOfMainCheckpoints - 1;
    }
    distLeft += (gTrackCheckpoints[checkpointID].distance * racer->checkpoint_distance);
    return distLeft;
}

/**
 * Returns a pointer to a specific checkpoint.
 */
CheckpointNode *get_checkpoint_node(s32 checkpointID) {
    return &gTrackCheckpoints[checkpointID];
}

/**
 * Takes the position along the checkpoint path, and finds the next applicable node.
 * If an alternative path is available, use that node instead.
 */
CheckpointNode *find_next_checkpoint_node(s32 splinePos, s32 isAlternate) {
    CheckpointNode *checkpointNode = &gTrackCheckpoints[splinePos];
    if (isAlternate != 0 && checkpointNode->altRouteID != -1) {
        checkpointNode = &gTrackCheckpoints[checkpointNode->altRouteID];
    }
    return checkpointNode;
}

/**
 * Returns the number of active checkpoints in the current level.
 */
s32 get_checkpoint_count(void) {
    return gNumberOfMainCheckpoints;
}

/**
 * Returns the group of racer objects.
 * Official Name: objGetPlayerlist
 */
Object **get_racer_objects(s32 *numRacers) {
    *numRacers = gNumRacers;
    return *gRacers;
}

/**
 * Returns the group of racer objects, ordered by player index.
 */
Object **get_racer_objects_by_port(s32 *numRacers) {
    *numRacers = gNumRacers;
    return gRacersByPort;
}

/**
 * Returns the group of racer objects, ordered by current race position.
 */
Object **get_racer_objects_by_position(s32 *numRacers) {
    *numRacers = gNumRacers;
    return gRacersByPosition;
}

/**
 * Returns the racer object specified by the ID
 */
Object *get_racer_object(s32 index) {
    if (gNumRacers == 0) {
        return NULL;
    }
    if (index < 0 || index >= gNumRacers) {
        return NULL;
    }
    return (*gRacers)[index];
}

/**
 * Returns the racer object specified by the player ID.
 * Official name: objGetPlayerNo
 */
Object *get_racer_object_by_port(s32 index) {
    if (gNumRacers == 0) {
        return NULL;
    }
    if (index < 0 || index >= gNumRacers) {
        return NULL;
    }
    return gRacersByPort[index];
}

/**
 * Unused function that would've iterated through all active checkpoints to render their visual nodes.
 * The function it calls is completely stubbed out.
 */
UNUSED void debug_render_checkpoints(Gfx **dList, Mtx **mtx, Vertex **vtx) {
    s32 i;

    material_set_no_tex_offset(dList, NULL, RENDER_Z_COMPARE);
    if (gNumberOfMainCheckpoints > 3) {
        for (i = 0; i < gNumberOfMainCheckpoints; i++) {
            // Ground path
            debug_render_checkpoint_node(i, 0, dList, mtx, vtx);
        }
        for (i = 0; i < gNumberOfMainCheckpoints; i++) {
            // Air path
            debug_render_checkpoint_node(i, 1, dList, mtx, vtx);
        }
    }
}
