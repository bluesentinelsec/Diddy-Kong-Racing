#include "racer_internal.h"

#define MAX_NUMBER_OF_GHOST_NODES 360

/**
 * Writes a human readable time to the passed arguments.
 * Used to visualise a standard time to the player for a stopwatch, or a record.
 */
void get_timestamp_from_frames(s32 frameCount, s32 *minutes, s32 *seconds, s32 *hundredths) {
    if (gVideoRefreshRate == REFRESH_50HZ) {
        frameCount = (f32) frameCount * 1.2;
    }
    // (REFRESH_60HZ * 60) is just frames per minute basically
    *minutes = frameCount / (REFRESH_60HZ * 60);
    *seconds = (frameCount - (*minutes * (REFRESH_60HZ * 60))) / REFRESH_60HZ;
    *hundredths = (((frameCount - (*minutes * (REFRESH_60HZ * 60))) - (*seconds * REFRESH_60HZ)) * 100) / REFRESH_60HZ;
}

/**
 * Allocate the ghost data heap into memory.
 * The path node pool is globally loaded, despite only being used in time trial.
 */
void allocate_ghost_data(void) {
    // Allocate two sets of ghost data. One for the current playing ghost, and one for the new ghost being written.
    gGhostData[0] = mempool_alloc_safe((sizeof(GhostNode) * 2) * MAX_NUMBER_OF_GHOST_NODES, COLOUR_TAG_RED);
    gGhostData[1] = ((GhostNode *) gGhostData[0] + MAX_NUMBER_OF_GHOST_NODES);
    gGhostData[GHOST_STAFF] = NULL; // T.T. Ghost
    gGhostNodeCount[0] = 0;
    gGhostNodeCount[1] = 0;
    gGhostNodeCount[GHOST_STAFF] = 0;
    gGhostNodeFull[0] = FALSE;
    gGhostNodeFull[1] = FALSE;
    gPrevGhostNodeIndex = 0;
    gGhostMapID = -1;
}

/**
 * Reset all the player ghost playback variables to default.
 * Also set the ghost index.
 */
void timetrial_reset_player_ghost(void) {
    gCurrentGhostIndex = gPrevGhostNodeIndex;
    gGhostNodeCount[gCurrentGhostIndex] = 0;
    gGhostNodeFull[gCurrentGhostIndex] = FALSE;
    gGhostNodeDelay = 0;
}

/**
 * Swap player ghost index and set the new saved map ID.
 */
void timetrial_swap_player_ghost(s32 mapID) {
    gPrevGhostNodeIndex = (gCurrentGhostIndex + 1) & 1;
    gGhostMapID = mapID;
}

/**
 * Return the current map ID associated with the time trial ghost.
 */
s32 timetrial_map_id(void) {
    return gGhostMapID;
}

/**
 * Attempt to load the player ghost data from the controller pak.
 * Returns the controller pak status when done.
 */
s32 timetrial_load_player_ghost(s32 controllerID, s32 mapId, s16 vehicleId, s16 *characterID, s16 *time) {
    s32 cpakStatus;
    s32 nodeID;
    s16 nodeCount;

    nodeID = (gCurrentGhostIndex + 1) & 1;
    cpakStatus = ghost_load(controllerID, mapId, vehicleId, (u16 *) characterID, time, &nodeCount,
                            (GhostPakFile *) gGhostData[nodeID]);
    if (characterID) {
        if (cpakStatus == CONTROLLER_PAK_GOOD) {
            gGhostNodeCount[nodeID] = nodeCount;
            gGhostMapID = mapId;
        } else {
            gGhostMapID = -1;
        }
    }

    return cpakStatus;
}

/**
 * Loads T.T. ghost node data into gGhostData[GHOST_STAFF].
 * Returns 0 if successful, or 1 if an error occured.
 */
s32 load_tt_ghost(s32 ghostOffset, s32 size, s16 *outTime) {
    GhostHeader *ghost = mempool_alloc_safe(size, COLOUR_TAG_RED);
    if (ghost != NULL) {
        asset_load(ASSET_TTGHOSTS, (uintptr_t) ghost, ghostOffset, size);
        if (gGhostData[GHOST_STAFF] != NULL) {
            mempool_free(gGhostData[GHOST_STAFF]);
        }
        gGhostData[GHOST_STAFF] = mempool_alloc_safe(size - sizeof(GhostHeader), COLOUR_TAG_WHITE);
        if (gGhostData[GHOST_STAFF] != NULL) {
            *outTime = ghost->time;
            gGhostNodeCount[GHOST_STAFF] = ghost->nodeCount;
            bcopy((u8 *) ghost + 8, gGhostData[GHOST_STAFF], size - sizeof(GhostHeader));
            mempool_free(ghost);
            return 0;
        }
        mempool_free(ghost);
    }
    return 1;
}

/**
 * Free the staff ghost from memory if it exists.
 */
void timetrial_free_staff_ghost(void) {
    if (gGhostData[GHOST_STAFF] != NULL) {
        mempool_free(gGhostData[GHOST_STAFF]);
    }
    gGhostData[GHOST_STAFF] = NULL;
}

/**
 * Calls a function that attempts to write the player ghost data to the controller pak.
 * Returns the controller pak status when finished.
 */
SIDeviceStatus timetrial_write_player_ghost(s32 controllerIndex, s32 mapId, s16 vehicleId, s16 characterId, s16 time) {
    return ghost_save(controllerIndex, (s16) mapId, vehicleId, characterId, time, gGhostNodeCount[gCurrentGhostIndex],
                      (GhostPakDirEntry *) gGhostData[gCurrentGhostIndex]);
}

/**
 * Write down the current position and rotation every half second to the ghost node data.
 * Returns early if the ghost node data is full.
 */
void timetrial_ghost_write(Object *obj, s32 updateRate) {
    f32 yOffset;
    Object_Racer *racer;
    GhostNode *ghostNode;

    racer = obj->racer;
    yOffset = coss_f(racer->z_rotation_offset) * coss_f(racer->x_rotation_offset - racer->xRotationOffsetTarget);
    if (yOffset < 0) {
        yOffset *= 0.5;
    }
    yOffset = 17.0f - (yOffset * 17.0f);
    gGhostNodeDelay -= updateRate;
    if (gGhostNodeDelay > 0) {
        return;
    }
    while (gGhostNodeDelay <= 0) {
        gGhostNodeDelay += 30;
        if (gGhostNodeCount[gCurrentGhostIndex] >= 360) {
            if (!is_postrace_viewport_active()) {
                gGhostNodeFull[gCurrentGhostIndex] = TRUE;
            }
            return;
        }
        ghostNode = gGhostData[gCurrentGhostIndex] + gGhostNodeCount[gCurrentGhostIndex];
        ghostNode->x = obj->trans.x_position;
        ghostNode->y = obj->trans.y_position + yOffset;
        ghostNode->z = obj->trans.z_position;
        ghostNode->yRotation = obj->trans.rotation.y_rotation + racer->y_rotation_offset;
        ghostNode->xRotation = obj->trans.rotation.x_rotation + racer->x_rotation_offset;
        ghostNode->zRotation = obj->trans.rotation.z_rotation + racer->z_rotation_offset;
        gGhostNodeCount[gCurrentGhostIndex]++;
    }
}

/**
 * Returns true if the time trial ghost has reached its size limit.
 */
s16 timetrial_ghost_full(void) {
    return gGhostNodeFull[gCurrentGhostIndex];
}

s32 timetrial_ghost_read(Object *obj) {
    f32 catmullX;
    s32 endNode;
#ifdef AVOID_UB
    //!@bug The loop below writes four spline points and the interpolation reads four, but the original
    // declared room for three, so the fourth landed in whatever followed on the stack.
    f32 vectorX[4];
    s32 rotDiff;
    f32 vectorY[4];
    Object_Racer *racer;
    f32 vectorZ[4];
#else
    f32 vectorX[3];
    s32 rotDiff;
    f32 vectorY[3];
    Object_Racer *racer;
    f32 vectorZ[3];
#endif
    s32 commonUnk0s32;
    s32 ghostDataIndex;
    s32 nodeIndex;
    GhostNode *nextGhostNode;
    s32 ghostNodeCount;
    GhostNode *curGhostNode;
    Object_Racer *obj64;
    s32 i;

    ghostDataIndex = (gCurrentGhostIndex + 1) & 1;
    if (timetrial_staff_ghost_check(obj)) {
        ghostDataIndex = 2;
    }

    catmullX = (f32) obj->properties.timeTrial.timestamp / 30.0f;
    if (osTvType == OS_TV_TYPE_PAL && ghostDataIndex == 2) {
        catmullX = ((f32) obj->properties.timeTrial.timestamp * 1.2) / 30.0f;
    }
    commonUnk0s32 = catmullX; // Truncate the float to an integer?

    ghostNodeCount = gGhostNodeCount[ghostDataIndex];
    if (commonUnk0s32 >= (ghostNodeCount - 2)) {
#if VERSION >= VERSION_79
        obj64 = obj->racer;
        if (obj64->transparency > 0) {
            obj64->transparency -= 1;
        }
#endif
        return FALSE;
    }
    if (ghostDataIndex != 2 && level_id() != gGhostMapID) {
        return FALSE;
    }
    nodeIndex = commonUnk0s32 - 1;

#ifdef AVOID_UB
    for (i = 0; i < ARRAY_COUNT(vectorY); i++) {
#else
    for (i = 0; i <= ARRAY_COUNT(vectorY); i++) {
#endif
        curGhostNode = &gGhostData[ghostDataIndex][nodeIndex];
        if (nodeIndex == -1) {
            vectorX[i] = ((curGhostNode + 1)->x + (curGhostNode + 1)->x) - (curGhostNode + 2)->x;
            curGhostNode++;
            vectorY[i] = ((curGhostNode)->y + (curGhostNode)->y) - (curGhostNode + 1)->y;
            vectorZ[i] = ((curGhostNode)->z + (curGhostNode)->z) - (curGhostNode + 1)->z;
        } else if (nodeIndex >= ghostNodeCount) {
            vectorX[i] = (curGhostNode->x + curGhostNode->x) - (curGhostNode - 1)->x;
            vectorY[i] = (curGhostNode->y + curGhostNode->y) - (curGhostNode - 1)->y;
            vectorZ[i] = (curGhostNode->z + curGhostNode->z) - (curGhostNode - 1)->z;
        } else {
            vectorX[i] = curGhostNode->x;
            vectorY[i] = curGhostNode->y;
            vectorZ[i] = curGhostNode->z;
        }
        nodeIndex++;
    }

    curGhostNode = &gGhostData[ghostDataIndex][commonUnk0s32];
    catmullX -= commonUnk0s32;
    nextGhostNode = curGhostNode + 1;
    obj->trans.x_position = catmull_rom_interpolation(vectorX, 0, catmullX);
    obj->trans.y_position = catmull_rom_interpolation(vectorY, 0, catmullX);
    obj->trans.z_position = catmull_rom_interpolation(vectorZ, 0, catmullX);

    // Y Rotation
    rotDiff = nextGhostNode->yRotation - (curGhostNode->yRotation & 0xFFFF);
    if (rotDiff > 0x8000) {
        rotDiff -= 0xFFFF;
    }
    if (rotDiff < -0x8000) {
        rotDiff += 0xFFFF;
    }

    obj->trans.rotation.y_rotation = curGhostNode->yRotation + (s16) (rotDiff * catmullX);

    // X Rotation
    rotDiff = nextGhostNode->xRotation - (curGhostNode->xRotation & 0xFFFF);
    if (rotDiff > 0x8000) {
        rotDiff -= 0xFFFF;
    }
    if (rotDiff < -0x8000) {
        rotDiff += 0xFFFF;
    }
    obj->trans.rotation.x_rotation = curGhostNode->xRotation + (s16) (rotDiff * catmullX);

    // Z Rotation
    rotDiff = nextGhostNode->zRotation - (curGhostNode->zRotation & 0xFFFF);
    if (rotDiff > 0x8000) {
        rotDiff -= 0xFFFF;
    }
    if (rotDiff < -0x8000) {
        rotDiff += 0xFFFF;
    }
    obj->trans.rotation.z_rotation = curGhostNode->zRotation + (s16) (rotDiff * catmullX);

    obj->particleEmittersEnabled = OBJ_EMIT_NONE;
    obj->segmentID =
        get_level_segment_index_from_position(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
    endNode = commonUnk0s32 + 3;
    if (ghostNodeCount == endNode) {
        racer = obj->racer;
        if (catmullX >= 0.8) {
            racer->transparency = 0;
        } else {
            racer->transparency = (0.8 - catmullX) * 96.0;
        }
    }
    return TRUE;
}
