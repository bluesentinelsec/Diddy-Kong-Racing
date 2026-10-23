#include "objects_internal.h"

/**
 * Loop through every existing spectate camera and sort them by index.
 */
void spectate_update(void) {
    Object *objPtr;
    Object *temp;
    s32 continueLoop;
    s32 i;

    gCameraObjCount = 0;
    for (i = 0; i < gObjectCount; i++) {
        objPtr = gObjPtrList[i];
        if (!(objPtr->trans.flags & OBJ_FLAGS_PARTICLE)) {
            if (objPtr->behaviorId == BHV_CAMERA_CONTROL) {
                if (gCameraObjCount < CAMCONTROL_COUNT) {
                    (*gCameraObjList)[gCameraObjCount] = objPtr;
                    gCameraObjCount++;
                }
            }
        }
    }

    do {
        continueLoop = TRUE;
        for (i = 0; i < gCameraObjCount - 1; i++) {
            objPtr = (*gCameraObjList)[i + 1];
            temp = (*gCameraObjList)[i];
            if (temp->properties.camControl.cameraID > objPtr->properties.camControl.cameraID) {
                (*gCameraObjList)[i] = (*gCameraObjList)[i + 1];
                (*gCameraObjList)[i + 1] = temp;
                continueLoop = FALSE;
            }
        }
    } while (!continueLoop);
}

Object *spectate_object(s32 cameraIndex) {
    if (cameraIndex < 0 || cameraIndex >= gCameraObjCount) {
        return NULL;
    }
    return (*gCameraObjList)[cameraIndex];
}

/**
 * Take the current camera passed through the function and compare distances between the next and previous camera.
 * Set the camera to be whichever's closest to the object.
 */
Object *spectate_nearest(Object *obj, s32 *cameraId) {
    Object *nextCamera;
    Object *prevCamera;
    Object *currCamera;
    s32 *cameraIndex;
    f32 x;
    f32 y;
    f32 z;
    f32 prevCameraXYZ;
    f32 currCameraXYZ;
    f32 nextCameraXYZ;
    s32 cameraIndex_Curr;
    s32 cameraIndex_Prev;
    s32 cameraIndex_Next;
    cameraIndex = cameraId;
    if (gCameraObjCount == 0) {
        return NULL;
    }
    cameraIndex_Next = *cameraIndex + 1;
    cameraIndex_Curr = *cameraIndex;
    cameraIndex_Prev = *cameraIndex - 1;
    if (cameraIndex_Next >= gCameraObjCount) {
        cameraIndex_Next = 0;
    }
    if (cameraIndex_Prev < 0) {
        cameraIndex_Prev = gCameraObjCount - 1;
    }
    currCamera = (*gCameraObjList)[cameraIndex_Curr];
    nextCamera = (*gCameraObjList)[cameraIndex_Next];
    prevCamera = (*gCameraObjList)[cameraIndex_Prev];
    x = currCamera->trans.x_position - obj->trans.x_position;
    y = currCamera->trans.y_position - obj->trans.y_position;
    z = currCamera->trans.z_position - obj->trans.z_position;
    currCameraXYZ = (x * x) + (y * y) + (z * z);
    x = nextCamera->trans.x_position - obj->trans.x_position;
    y = nextCamera->trans.y_position - obj->trans.y_position;
    z = nextCamera->trans.z_position - obj->trans.z_position;
    nextCameraXYZ = (x * x) + (y * y) + (z * z);
    x = prevCamera->trans.x_position - obj->trans.x_position;
    y = prevCamera->trans.y_position - obj->trans.y_position;
    z = prevCamera->trans.z_position - obj->trans.z_position;
    prevCameraXYZ = (x * x) + (y * y) + (z * z);

    if (nextCameraXYZ < currCameraXYZ) {
        *cameraId = cameraIndex_Next;
        currCamera = nextCamera;
        currCameraXYZ = nextCameraXYZ;
    }
    if (prevCameraXYZ < currCameraXYZ) {
        *cameraId = cameraIndex_Prev;
        currCamera = prevCamera;
    }
    return currCamera;
}

/**
 * Take every existing AI node and find each neighbouring node.
 * Afterwards, sort them by height so the game can generate elevation thresholds.
 */
void ainode_update(void) {
    LevelObjectEntry_AiNode *aiNodeEntry;
    Object *obj;
    s16 nodeCount;
    Object *nextAiNodeObj;
    f32 diffX;
    f32 diffZ;
    f32 diffY;
    s32 i;
    s32 j;
    s8 index;  // Must be an s8
    u8 index2; // Must be an u8
    s16 swap;
    Object_AiNode *aiNodeObj64;
    s8 nodeIDs[AINODE_COUNT];
    s8 elevations[AINODE_COUNT];

    if (gInitAINodes == FALSE) {
        return;
    }
    gInitAINodes = FALSE;
    for (i = 0; i < AINODE_COUNT; i++) {
        (*gAINodes)[i] = NULL;
    }
    nodeCount = 0;
    // Store each existing node ID in the temporary vars.
    for (i = 0; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE) && obj->behaviorId == BHV_AINODE) {
            aiNodeEntry = &obj->level_entry->aiNode;
            index2 = aiNodeEntry->nodeID;
            if (!(index2 & AINODE_COUNT)) {
                (*gAINodes)[index2] = obj;
                nodeIDs[nodeCount] = aiNodeEntry->nodeID;
                elevations[nodeCount] = aiNodeEntry->elevation & 3;
                nodeCount++;
            }
        }
    }
    if (nodeCount == 0) {
        return;
    }
    // Find and set each neighbouring node and distance for each existing node.
    for (i = 0; i < AINODE_COUNT; i++) {
        obj = (*gAINodes)[i];
        if (obj != NULL) {
            aiNodeObj64 = obj->ai_node;
            aiNodeEntry = &obj->level_entry->aiNode;
            for (j = 0; j < 4; j++) {
                index2 = aiNodeEntry->adjacent[j];
                if (!(index2 & AINODE_COUNT)) {
                    nextAiNodeObj = (*gAINodes)[index2];
                    aiNodeObj64->nodeObj[j] = nextAiNodeObj;
                    if (nextAiNodeObj == NULL) {
                        aiNodeEntry->adjacent[j] = -1;
                    } else {
                        diffX = obj->trans.x_position - nextAiNodeObj->trans.x_position;
                        diffY = obj->trans.y_position - nextAiNodeObj->trans.y_position;
                        diffZ = obj->trans.z_position - nextAiNodeObj->trans.z_position;
                        aiNodeObj64->distToNode[j] = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
                    }
                }
            }
        }
    }
    // Sort them by height
    do {
        j = TRUE;
        for (i = 0; i < nodeCount - 1; i++) {
            if ((*gAINodes)[nodeIDs[i + 1]]->trans.y_position < (*gAINodes)[nodeIDs[i]]->trans.y_position) {
                swap = nodeIDs[i];
                nodeIDs[i] = nodeIDs[i + 1];
                nodeIDs[i + 1] = swap;
                swap = elevations[i];
                elevations[i] = elevations[i + 1];
                elevations[i + 1] = swap;
                j = FALSE;
            }
        }
    } while (!j); // Keep doing this until no more swaps are needed.

    if (1) {} // Fakematch

    for (i = 0; i < 5; i++) {
        gElevationHeights[i] = -20000.0f;
    }

    index = elevations[0];
    for (i = 0; i < nodeCount - 1;) {
        while (i < nodeCount - 1 && index >= elevations[i]) {
            i++;
        }
        if (index < elevations[i]) {
            index = elevations[i];
            gElevationHeights[index] =
                ((*gAINodes)[nodeIDs[i]]->trans.y_position + (*gAINodes)[nodeIDs[i - 1]]->trans.y_position) * 0.5;
        } else {
            i = nodeCount;
        }
    }
    gElevationHeights[0] = -10000.0f;
    gElevationHeights[4] = -gElevationHeights[0];
}

/**
 * Compare heights against the thresholds.
 * Elevation level is set based on position.
 */
s16 obj_elevation(f32 yPos) {
    s16 i = 0;
    s16 elevation = 0;
    for (; i < 4; i++) {
        if (gElevationHeights[i] != -20000.0f && gElevationHeights[i] < yPos) {
            elevation = i;
        }
    }
    return elevation;
}

/**
 * Loop through the AI Node list and add this new object to the list if it does not already exist.
 */
s32 ainode_register(Object *obj) {
    s32 i;
    for (i = 0; i < AINODE_COUNT; i++) {
        if ((*gAINodes)[i] == NULL) {
            (*gAINodes)[i] = obj;
            return i;
        }
    }
    return -1;
}

/**
 * Search through each AI node and find the one closest to the coordinates given.
 * Can choose to include or ignore elevation.
 */
s32 ainode_find_nearest(f32 diffX, f32 diffY, f32 diffZ, s32 useElevation) {
    UNUSED f32 pad[6];
    s32 elevation;
    f32 len;
    f32 x;
    f32 z;
    f32 y;
    f32 dist;
    s32 findDist;
    s32 numSteps;
    s32 result;
    Object *obj;
    LevelObjectEntry_AiNode *levelObj;

    if (useElevation) {
        elevation = obj_elevation(diffY);
    }
    dist = 50000.0;
    result = 0xFF;
    for (numSteps = 0; numSteps != AINODE_COUNT; numSteps++) {
        obj = (*gAINodes)[numSteps];
        if (obj) {
            levelObj = &((obj->level_entry)->aiNode);
            findDist = TRUE;
            if (useElevation && elevation != levelObj->elevation) {
                findDist = FALSE;
            }
            if (useElevation == 2 && levelObj->targetTag != 3) {
                findDist = FALSE;
            }
            if (findDist) {
                x = obj->trans.x_position - diffX;
                y = obj->trans.y_position - diffY;
                z = obj->trans.z_position - diffZ;
                len = sqrtf((x * x) + (y * y) + (z * z));
                if (len < dist) {
                    dist = len;
                    result = numSteps;
                }
            }
        }
    }
    return result;
}

/** Moves an NPC along the AI node path and returns its progress. */
f32 npc_follow_ai_nodes(Object_NPC *npc, Object *npcParentObj, f32 updateRateF, f32 speedF, s32 direction) {
    f32 xPositions[5];
    f32 yPositions[5];
    f32 zPositions[5];
    f32 xDiff2;
    f32 yDiff2;
    f32 zDiff2;
    Object *aiNode;
    f32 dist;
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    f32 tempYDiff;
    UNUSED s32 pad_sp84;
    s32 i;
    f32 var_f20_2;
    s32 var_s0;
    s32 someBool;

    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }

    for (i = 0; i < 5; i++) {
        if (npc->nodeData[i] == 0xFF) {
            return 0.0f;
        }

        aiNode = ainode_get(npc->nodeData[i]);
        if (aiNode == NULL) {
            return 0.0f;
        }

        xPositions[i] = aiNode->trans.x_position;
        yPositions[i] = aiNode->trans.y_position;
        zPositions[i] = aiNode->trans.z_position;
    }

    xDiff2 = catmull_rom_interpolation(xPositions, 0, npc->splineProgress);
    yDiff2 = catmull_rom_interpolation(yPositions, 0, npc->splineProgress);
    zDiff2 = catmull_rom_interpolation(zPositions, 0, npc->splineProgress);
    someBool = FALSE;
    if (npc->splineStep == 0.0f) {
        npc->splineStep = 0.01f;
    }

    for (var_s0 = 0; var_s0 != 2; var_s0++) {
        var_f20_2 = npc->splineProgress + (npc->splineStep * updateRateF);
        if (var_f20_2 >= 1.0) {
            someBool = TRUE;
            var_f20_2 -= 1.0;
        }
        xDiff = catmull_rom_interpolation(xPositions, someBool, var_f20_2);
        yDiff = catmull_rom_interpolation(yPositions, someBool, var_f20_2);
        zDiff = catmull_rom_interpolation(zPositions, someBool, var_f20_2);
        xDiff -= xDiff2;
        yDiff -= yDiff2;
        zDiff -= zDiff2;
        if (var_s0 == 0) {
            someBool = FALSE;
            dist = sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)) / updateRateF;
            if (dist != 0.0f) {
                npc->splineStep *= (speedF / dist);
            }
        }
    }
    npc->splineProgress = var_f20_2;
    xDiff2 = xDiff + xDiff2;
    tempYDiff = yDiff2 = yDiff + yDiff2;
    zDiff2 = zDiff + zDiff2;

    xDiff = xDiff2 - npcParentObj->trans.x_position;
    yDiff = yDiff2 - npcParentObj->trans.y_position;
    zDiff = zDiff2 - npcParentObj->trans.z_position;

    xDiff2 = xDiff;
    yDiff2 = yDiff;
    zDiff2 = zDiff;

    dist = sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff));
    if (dist != 0.0f) {
        dist = 255.0 / dist;
        xDiff *= dist;
        yDiff *= dist;
        zDiff *= dist;
    }
    dist = sqrtf((xDiff2 * xDiff2) + (yDiff2 * yDiff2) + (zDiff2 * zDiff2)) / 16;
    if (speedF < dist) {
        dist = speedF;
    }
    if (dist >= 1.0) {
        var_s0 = (arctan2_f(xDiff, zDiff) - (npcParentObj->trans.rotation.y_rotation & 0xFFFF)) - 0x8000;
        if (var_s0 > 0x8000) {
            var_s0 -= 0xFFFF;
        }
        if (var_s0 < -0x8000) {
            var_s0 += 0xFFFF;
        }
        npcParentObj->trans.rotation.y_rotation += ((var_s0 * (s32) updateRateF)) >> 4;
        var_s0 = arctan2_f(yDiff, 255.0f) - (npcParentObj->trans.rotation.x_rotation & 0xFFFF);
        if (var_s0 > 0x8000) {
            var_s0 -= 0xFFFF;
        }
        if (var_s0 < -0x8000) {
            var_s0 += 0xFFFF;
        }
        npcParentObj->trans.rotation.x_rotation += ((var_s0 * (s32) updateRateF)) >> 4;
    }

    npcParentObj->trans.rotation.z_rotation = 0;
    xDiff = sins_f((s16) (npcParentObj->trans.rotation.y_rotation + 0x8000)) * dist;
    move_object(npcParentObj, xDiff * updateRateF, 0.0f,
                coss_f((npcParentObj->trans.rotation.y_rotation + 0x8000)) * dist * updateRateF);
    npcParentObj->trans.y_position = tempYDiff;
    dist = dist * updateRateF * 2;
    if (someBool != 0) {
        npc->nodeData[0] = npc->nodeData[1];
        npc->nodeData[1] = npc->nodeData[2];
        npc->nodeData[2] = npc->nodeData[3];
        npc->nodeData[3] = npc->nodeData[4];
        npc->nodeData[4] = ainode_find_next(npc->nodeData[3], npc->nodeData[2], direction);
    }

    return dist;
}

s32 ainode_find_next(s32 nodeId, s32 nextNodeId, s32 direction) {
    Object *aiNodeObj;
    LevelObjectEntry_AiNode *entry;
    Object_AiNode *aiNode;
    s32 nextIndex;
    s32 i;
    s32 someCount;

    if (nodeId < -1 || nodeId >= AINODE_COUNT) {
        return NODE_NONE;
    }
    aiNodeObj = (*gAINodes)[nodeId];
    if (aiNodeObj == NULL) {
        return NODE_NONE;
    }

    entry = &aiNodeObj->level_entry->aiNode;
    aiNode = aiNodeObj->ai_node;
    direction = direction & 3;
    someCount = 0;
    nextIndex = (aiNode->directions[direction] + 1) & 3;

    for (i = 0; i < 4; i++) {
        if (entry->adjacent[nextIndex] != NODE_NONE && entry->adjacent[nextIndex] != nextNodeId) {
            aiNode->directions[direction] = nextIndex;
            i = 4; // break
            someCount++;
        }
        nextIndex = (nextIndex + 1) & 3;
    }

    if (someCount == 0) {
        return NODE_NONE;
    } else {
        return entry->adjacent[aiNode->directions[direction]];
    }
}

/** Chooses the next AI node for a racer from the node graph. */
s16 ainode_pick_next(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s16 result;
    s32 sp370;
    s16 var_t1;
    s16 sp36C;
    s16 temp2;
    s16 i;
    LevelObjectEntry_AiNode *aiNodeEntry;
    s8 someBool;
    s8 var_s3;
    s8 var_s5;
    s8 temp;
    s32 var_ra;
    Object_AiNode *aiNode;
    Object *aiNodeObj;
    s32 sp154[AINODE_COUNT];
    s8 spD4[AINODE_COUNT];
    s8 sp54[AINODE_COUNT];

    // Only matches with do {} while?
    i = 0;
    do {
        sp154[i] = 0;
        i++;
    } while (i < AINODE_COUNT);

    aiNodeObj = (*gAINodes)[arg0];
    aiNode = aiNodeObj->ai_node;
    aiNodeEntry = &aiNodeObj->level_entry->aiNode;
    var_t1 = 0;
    var_ra = 0;
    result = NODE_NONE;
    var_s3 = 0;
    someBool = 1;
    var_s5 = 0;
    do {
        for (i = 0; i < 4; i++) {
            if (aiNode->nodeObj[i] != 0) {
                temp2 = aiNodeEntry->adjacent[i];
                temp = 1;
                if (((arg0 == aiNodeEntry->nodeID) && (temp2 == arg2)) ||
                    ((arg2 == aiNodeEntry->nodeID) && (temp2 == arg0))) {
                    temp = 0;
                    if (var_s5 != 0) {
                        temp = 1;
                    }
                    var_s5++;
                }
                if (temp != 0) {
                    sp36C = 0;
                    temp = 0;
                    while ((sp36C < var_t1) && (temp2 != spD4[sp36C])) {
                        sp36C++;
                    }

                    sp370 = aiNode->distToNode[i] + var_ra;
                    if (sp36C == var_t1) {
                        temp = 2;
                    } else if (sp370 < sp154[temp2]) {
                        temp = 1;
                    }
                }
                if (temp != 0) {
                    while (sp36C < var_t1 - 1) {
                        spD4[sp36C] = spD4[sp36C + 1];
                        sp54[sp36C] = sp54[sp36C + 1];
                        sp36C++;
                    }
                    if (temp == 2) {
                        var_t1++;
                    }
                    sp154[temp2] = sp370;

                    sp36C = var_t1 - 1;

                    sp54[sp36C] = (someBool) ? aiNodeEntry->adjacent[i] : var_s3;
                    spD4[sp36C] = aiNodeEntry->adjacent[i];

                    while ((sp36C > 0) && (sp154[spD4[sp36C - 1]] < sp154[spD4[sp36C]])) {
                        temp = spD4[sp36C];
                        spD4[sp36C] = spD4[sp36C - 1];
                        spD4[sp36C - 1] = temp;
                        temp = sp54[sp36C];
                        sp54[sp36C] = sp54[sp36C - 1] & 0xFF & 0xFF & 0xFF;
                        sp54[sp36C - 1] = temp & 0xFF;
                        sp36C--;
                    }
                }
            }
        }

        if (var_t1 > 0) {
            var_t1--;
            aiNodeObj = (*gAINodes)[spD4[var_t1]];
            aiNodeEntry = &aiNodeObj->level_entry->aiNode;
            var_s3 = sp54[var_t1];
            var_ra = sp154[aiNodeEntry->nodeID];
            aiNode = aiNodeObj->ai_node;
            someBool = 0;
            if (arg1 & 0x100) {
                if ((arg1 & 0x7F) == spD4[var_t1]) {
                    result = var_s3;
                }
            } else if (arg1 == aiNodeEntry->targetTag) {
                result = var_s3;
            }
            if ((var_t1 == 0) && (result == 0xFF)) {
                result = var_s3;
            }
        }
    } while ((result == NODE_NONE) && (var_t1 > 0));

    if (result != 0xFF) {
        aiNodeObj = (*gAINodes)[arg0];
        aiNodeEntry = &aiNodeObj->level_entry->aiNode;
        aiNode = aiNodeObj->ai_node;
        for (i = 0; (i < 4) && (result != aiNodeEntry->adjacent[i]); i++) {}
        if (i < 4) {
            aiNode->directions[arg3] = i;
        }
    }
    return result;
}

/**
 * Signal that AI nodes exist, so the game knows to initialise them.
 */
void ainode_enable(void) {
    gInitAINodes = TRUE;
}

/**
 * If the node ID is new, set the tail ID to it.
 */
void ainode_tail_set(s32 nodeID) {
    if (nodeID != gAINodeTail[0]) {
        gAINodeTail[1] = gAINodeTail[0];
        gAINodeTail[0] = nodeID;
    }
}

/**
 * Return the last created AI node.
 */
UNUSED Object *ainode_tail(s32 *nodeID) {
    *nodeID = gAINodeTail[1];
    return gAINodes[0][gAINodeTail[1]];
}

/**
 * Return the AI node assigned to the given ID.
 */
Object *ainode_get(s32 nodeID) {
    if (nodeID >= 0 && nodeID < AINODE_COUNT) {
        return gAINodes[0][nodeID];
    }
    return NULL;
}
