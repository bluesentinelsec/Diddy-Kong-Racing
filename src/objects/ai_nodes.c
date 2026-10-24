#include "objects_internal.h"

// ainode_pick_next target: with this bit set the low bits are a node ID, otherwise it is a node's target tag.
#define AINODE_TARGET_IS_NODE 0x100
#define AINODE_TARGET_NODE_MASK 0x7F
// Each AI node links to up to four neighbours.
#define AINODE_ADJACENT_COUNT 4

/**
 * Loop through every existing spectate camera and sort them by index.
 */
void spectate_update(void) {
    Object *objPtr;
    Object *currCamera;
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
            currCamera = (*gCameraObjList)[i];
            if (currCamera->properties.camControl.cameraID > objPtr->properties.camControl.cameraID) {
                (*gCameraObjList)[i] = (*gCameraObjList)[i + 1];
                (*gCameraObjList)[i + 1] = currCamera;
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
    s8 elevation; // Must be an s8
    u8 nodeID;    // Must be an u8
    s16 swap;
    Object_AiNode *aiNodeData;
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
            nodeID = aiNodeEntry->nodeID;
            if (!(nodeID & AINODE_COUNT)) {
                (*gAINodes)[nodeID] = obj;
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
            aiNodeData = obj->ai_node;
            aiNodeEntry = &obj->level_entry->aiNode;
            for (j = 0; j < AINODE_ADJACENT_COUNT; j++) {
                nodeID = aiNodeEntry->adjacent[j];
                if (!(nodeID & AINODE_COUNT)) {
                    nextAiNodeObj = (*gAINodes)[nodeID];
                    aiNodeData->nodeObj[j] = nextAiNodeObj;
                    if (nextAiNodeObj == NULL) {
                        aiNodeEntry->adjacent[j] = -1;
                    } else {
                        diffX = obj->trans.x_position - nextAiNodeObj->trans.x_position;
                        diffY = obj->trans.y_position - nextAiNodeObj->trans.y_position;
                        diffZ = obj->trans.z_position - nextAiNodeObj->trans.z_position;
                        aiNodeData->distToNode[j] = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
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

    elevation = elevations[0];
    for (i = 0; i < nodeCount - 1;) {
        while (i < nodeCount - 1 && elevation >= elevations[i]) {
            i++;
        }
        if (elevation < elevations[i]) {
            elevation = elevations[i];
            gElevationHeights[elevation] =
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
s32 ainode_find_nearest(f32 posX, f32 posY, f32 posZ, s32 useElevation) {
    s32 elevation;
    f32 len;
    f32 x;
    f32 z;
    f32 y;
    f32 dist;
    s32 findDist;
    s32 nodeIndex;
    s32 result;
    Object *obj;
    LevelObjectEntry_AiNode *levelObj;

    if (useElevation) {
        elevation = obj_elevation(posY);
    }
    dist = 50000.0;
    result = NODE_NONE;
    for (nodeIndex = 0; nodeIndex != AINODE_COUNT; nodeIndex++) {
        obj = (*gAINodes)[nodeIndex];
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
                x = obj->trans.x_position - posX;
                y = obj->trans.y_position - posY;
                z = obj->trans.z_position - posZ;
                len = sqrtf((x * x) + (y * y) + (z * z));
                if (len < dist) {
                    dist = len;
                    result = nodeIndex;
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
    f32 targetY;
    s32 i;
    f32 newProgress;
    s32 pass;
    s32 angleDiff;
    s32 passedNode;

    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }

    for (i = 0; i < 5; i++) {
        if (npc->nodeData[i] == NODE_NONE) {
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
    passedNode = FALSE;
    if (npc->splineStep == 0.0f) {
        npc->splineStep = 0.01f;
    }

    for (pass = 0; pass != 2; pass++) {
        newProgress = npc->splineProgress + (npc->splineStep * updateRateF);
        if (newProgress >= 1.0) {
            passedNode = TRUE;
            newProgress -= 1.0;
        }
        xDiff = catmull_rom_interpolation(xPositions, passedNode, newProgress);
        yDiff = catmull_rom_interpolation(yPositions, passedNode, newProgress);
        zDiff = catmull_rom_interpolation(zPositions, passedNode, newProgress);
        xDiff -= xDiff2;
        yDiff -= yDiff2;
        zDiff -= zDiff2;
        if (pass == 0) {
            passedNode = FALSE;
            dist = sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)) / updateRateF;
            if (dist != 0.0f) {
                npc->splineStep *= (speedF / dist);
            }
        }
    }
    npc->splineProgress = newProgress;
    xDiff2 = xDiff + xDiff2;
    targetY = yDiff2 = yDiff + yDiff2;
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
        angleDiff = (arctan2_f(xDiff, zDiff) - (npcParentObj->trans.rotation.y_rotation & 0xFFFF)) - 0x8000;
        if (angleDiff > 0x8000) {
            angleDiff -= 0xFFFF;
        }
        if (angleDiff < -0x8000) {
            angleDiff += 0xFFFF;
        }
        npcParentObj->trans.rotation.y_rotation += ((angleDiff * (s32) updateRateF)) >> 4;
        angleDiff = arctan2_f(yDiff, 255.0f) - (npcParentObj->trans.rotation.x_rotation & 0xFFFF);
        if (angleDiff > 0x8000) {
            angleDiff -= 0xFFFF;
        }
        if (angleDiff < -0x8000) {
            angleDiff += 0xFFFF;
        }
        npcParentObj->trans.rotation.x_rotation += ((angleDiff * (s32) updateRateF)) >> 4;
    }

    npcParentObj->trans.rotation.z_rotation = 0;
    xDiff = sins_f((s16) (npcParentObj->trans.rotation.y_rotation + 0x8000)) * dist;
    move_object(npcParentObj, xDiff * updateRateF, 0.0f,
                coss_f((npcParentObj->trans.rotation.y_rotation + 0x8000)) * dist * updateRateF);
    npcParentObj->trans.y_position = targetY;
    dist = dist * updateRateF * 2;
    if (passedNode != 0) {
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
    s32 foundCount;

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
    foundCount = 0;
    nextIndex = (aiNode->directions[direction] + 1) & 3;

    for (i = 0; i < AINODE_ADJACENT_COUNT; i++) {
        if (entry->adjacent[nextIndex] != NODE_NONE && entry->adjacent[nextIndex] != nextNodeId) {
            aiNode->directions[direction] = nextIndex;
            i = AINODE_ADJACENT_COUNT; // break
            foundCount++;
        }
        nextIndex = (nextIndex + 1) & 3;
    }

    if (foundCount == 0) {
        return NODE_NONE;
    } else {
        return entry->adjacent[aiNode->directions[direction]];
    }
}

/**
 * Chooses the next AI node for a racer from the node graph: a cheapest-path search outward from startNodeId
 * (skipping the first link it meets between it and excludeNodeId) until it reaches target, a node ID with
 * AINODE_TARGET_IS_NODE set or otherwise a target tag. Returns the neighbour of the start node that the path leaves
 * through, and records that link in the start node's directions[direction].
 */
s16 ainode_pick_next(s32 startNodeId, s32 target, s32 excludeNodeId, s32 direction) {
    s16 result;
    s32 pathCost;
    s16 openCount;
    s16 openIdx;
    s16 neighbourId;
    s16 i;
    LevelObjectEntry_AiNode *aiNodeEntry;
    s8 isStartNode;
    s8 firstStepNode;
    s8 backEdgeCount;
    s8 action; // 0: skip this neighbour, 1: lower its cost in the open list, 2: add it to the list
    s8 swapTemp;
    s32 currentCost;
    Object_AiNode *aiNode;
    Object *aiNodeObj;
    s32 bestCost[AINODE_COUNT];
    s8 openNodes[AINODE_COUNT];
    s8 openFirstStep[AINODE_COUNT];

    // Only matches with do {} while?
    i = 0;
    do {
        bestCost[i] = 0;
        i++;
    } while (i < AINODE_COUNT);

    aiNodeObj = (*gAINodes)[startNodeId];
    aiNode = aiNodeObj->ai_node;
    aiNodeEntry = &aiNodeObj->level_entry->aiNode;
    openCount = 0;
    currentCost = 0;
    result = NODE_NONE;
    firstStepNode = 0;
    isStartNode = 1;
    backEdgeCount = 0;
    do {
        for (i = 0; i < AINODE_ADJACENT_COUNT; i++) {
            if (aiNode->nodeObj[i] != 0) {
                neighbourId = aiNodeEntry->adjacent[i];
                action = 1;
                if (((startNodeId == aiNodeEntry->nodeID) && (neighbourId == excludeNodeId)) ||
                    ((excludeNodeId == aiNodeEntry->nodeID) && (neighbourId == startNodeId))) {
                    action = 0;
                    if (backEdgeCount != 0) {
                        action = 1;
                    }
                    backEdgeCount++;
                }
                if (action != 0) {
                    openIdx = 0;
                    action = 0;
                    while ((openIdx < openCount) && (neighbourId != openNodes[openIdx])) {
                        openIdx++;
                    }

                    pathCost = aiNode->distToNode[i] + currentCost;
                    if (openIdx == openCount) {
                        action = 2;
                    } else if (pathCost < bestCost[neighbourId]) {
                        action = 1;
                    }
                }
                if (action != 0) {
                    while (openIdx < openCount - 1) {
                        openNodes[openIdx] = openNodes[openIdx + 1];
                        openFirstStep[openIdx] = openFirstStep[openIdx + 1];
                        openIdx++;
                    }
                    if (action == 2) {
                        openCount++;
                    }
                    bestCost[neighbourId] = pathCost;

                    openIdx = openCount - 1;

                    openFirstStep[openIdx] = (isStartNode) ? aiNodeEntry->adjacent[i] : firstStepNode;
                    openNodes[openIdx] = aiNodeEntry->adjacent[i];

                    while ((openIdx > 0) && (bestCost[openNodes[openIdx - 1]] < bestCost[openNodes[openIdx]])) {
                        swapTemp = openNodes[openIdx];
                        openNodes[openIdx] = openNodes[openIdx - 1];
                        openNodes[openIdx - 1] = swapTemp;
                        swapTemp = openFirstStep[openIdx];
                        openFirstStep[openIdx] = openFirstStep[openIdx - 1] & 0xFF & 0xFF & 0xFF;
                        openFirstStep[openIdx - 1] = swapTemp & 0xFF;
                        openIdx--;
                    }
                }
            }
        }

        if (openCount > 0) {
            openCount--;
            aiNodeObj = (*gAINodes)[openNodes[openCount]];
            aiNodeEntry = &aiNodeObj->level_entry->aiNode;
            firstStepNode = openFirstStep[openCount];
            currentCost = bestCost[aiNodeEntry->nodeID];
            aiNode = aiNodeObj->ai_node;
            isStartNode = 0;
            if (target & AINODE_TARGET_IS_NODE) {
                if ((target & AINODE_TARGET_NODE_MASK) == openNodes[openCount]) {
                    result = firstStepNode;
                }
            } else if (target == aiNodeEntry->targetTag) {
                result = firstStepNode;
            }
            if ((openCount == 0) && (result == NODE_NONE)) {
                result = firstStepNode;
            }
        }
    } while ((result == NODE_NONE) && (openCount > 0));

    if (result != NODE_NONE) {
        aiNodeObj = (*gAINodes)[startNodeId];
        aiNodeEntry = &aiNodeObj->level_entry->aiNode;
        aiNode = aiNodeObj->ai_node;
        for (i = 0; (i < AINODE_ADJACENT_COUNT) && (result != aiNodeEntry->adjacent[i]); i++) {}
        if (i < AINODE_ADJACENT_COUNT) {
            aiNode->directions[direction] = i;
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
