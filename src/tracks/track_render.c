#include "tracks_internal.h"

static Vec3f gFrustumPlanePoints[3][3] = {
    { { { { 50.0f, 0.0f, 32.0f } } }, { { { -50.0f, 0.0f, 32.0f } } }, { { { -50.0f, 100.0f, 32.0f } } } },
    { { { { 0.0f, 0.0f, 32.0f } } }, { { { 130.0f, 60.0f, -68.0f } } }, { { { 130.0f, -60.0f, -68.0f } } } },
    { { { { 0.0f, 0.0f, 32.0f } } }, { { { -130.0f, -60.0f, -68.0f } } }, { { { -130.0f, 60.0f, -68.0f } } } },
};

/**
 * Sets up all of the required variables for the player's view perspective.
 * This includes setting up the camera index, viewport and
 */
void initialise_player_viewport_vars(s32 updateRate) {
    s32 i;
    s32 numRacers;
    s32 viewportID;
    Object **racers;
    s32 segmentIndex;
    Object_Racer *racer;

    gSceneActiveCamera = cam_get_active_camera();
    viewportID = get_current_viewport();
    compute_scene_camera_transform_matrix();
    update_envmap_position(gScenePerspectivePos.x / 65536.0f, gScenePerspectivePos.y / 65536.0f,
                           gScenePerspectivePos.z / 65536.0f);
    segmentIndex = gSceneActiveCamera->cameraSegmentID;
    if (segmentIndex > -1 && (segmentIndex < gCurrentLevelModel->numberOfSegments)) {
        gSceneStartSegment = gCurrentLevelModel->segments[segmentIndex].visibilityBitfieldOffset;
    } else {
        gSceneStartSegment = -1;
    }
    gPrevCameraX = gSceneActiveCamera->trans.x_position;
    gPrevCameraY = gSceneActiveCamera->trans.y_position;
    gPrevCameraZ = gSceneActiveCamera->trans.z_position;
    if (gWaveBlockCount != 0) {
        waves_visibility_reset();
        racers = get_racer_objects(&numRacers);
        if (gSceneActiveCamera->mode != CAMERA_FINISH_RACE && numRacers > 0 && !check_if_showing_cutscene_camera()) {
            i = -1;
            do {
                i++;
                racer = racers[i]->racer;
            } while (i < numRacers - 1 && viewportID != racer->playerIndex);
            waves_visibility(racers[i]->trans.x_position, racers[i]->trans.y_position, racers[i]->trans.z_position,
                             get_current_viewport(), updateRate);
        } else {
            waves_visibility(gSceneActiveCamera->trans.x_position, gSceneActiveCamera->trans.y_position,
                             gSceneActiveCamera->trans.z_position, get_current_viewport(), updateRate);
        }
    }
    level_header()->levelRendered = 1;
    render_level_geometry_and_objects();
}

/**
 * Enable or disable anti aliasing.
 * Improves visual quality at the cost of performance.
 */
void set_anti_aliasing(s32 setting) {
    gAntiAliasing = setting;
}

/**
 * Lists the visible segments in segmentIds, nearest first (the BSP tree is walked from the camera's
 * side), and renders their opaque geometry. objectsVisible[segment + 1] is set for each segment
 * whose objects may be drawn; entry 0 (objects outside any segment) is always set.
 * Returns the number of visible segments.
 */
static s32 render_opaque_segments(u8 *segmentIds, u8 *objectsVisible) {
    s32 numberOfSegments;
    s32 i;

    if (gCurrentLevelModel->numberOfSegments > 1) {
        numberOfSegments = 0;
        traverse_segments_bsp_tree(0, 0, gCurrentLevelModel->numberOfSegments - 1, segmentIds, &numberOfSegments);
    } else {
        numberOfSegments = 1;
        segmentIds[0] = 0;
    }

    for (i = 1; i <= gCurrentLevelModel->numberOfSegments; i++) {
        objectsVisible[i] = FALSE; // why not a bzero?
    }

    objectsVisible[0] = TRUE;

    if (gDrawLevelSegments) {
        for (i = 0; i < numberOfSegments; i++) {
            render_level_segment(segmentIds[i], FALSE); // Render opaque segments
            objectsVisible[segmentIds[i] + 1] = TRUE;
        }
    }

    if (gCurrentLevelModel->numberOfSegments < 2) {
        objectsVisible[1] = TRUE;
    }

    return numberOfSegments;
}

/**
 * Renders the semitransparent geometry of the visible segments farthest first, then the waves,
 * and flushes the deferred display lists.
 */
static void render_transparent_segments_and_waves(u8 *segmentIds, s32 numberOfSegments) {
    s32 i;

    if (gDrawLevelSegments) {
        for (i = numberOfSegments - 1; i >= 0; i--) {
            render_level_segment(segmentIds[i], TRUE); // Render transparent segments
        }
    }

    if (gWaveBlockCount != 0) {
        waves_render(&gTrackDL, &gTrackMtxPtr, get_current_viewport());
    }

    rendermode_reset(&gTrackDL);
    material_set_no_tex_offset(&gTrackDL, NULL, RENDER_FOG_ACTIVE | RENDER_Z_COMPARE);
    deferred_dl_flush(&gTrackDL);
}

/**
 * Find which segments can and should be rendered, then render their opaque geometry.
 * Render all objects inside visible segments then render the level's semitransparent geometry.
 * Afterwards, render particles.
 */
void render_level_geometry_and_objects(void) {
    s32 objCount;
    s32 numberOfSegments;
    s32 objFlags;
    s32 firstObjIndex;
    s32 i;
    s32 visibleFlags;
    u8 segmentIds[LEVEL_SEGMENT_MAX];
    u8 objectsVisible[LEVEL_SEGMENT_MAX];
    s32 visible;
    Object *obj;

    deferred_dl_reset();

    if (get_settings()->courseId == ASSET_LEVEL_OPENINGSEQUENCE) {
        gAntiAliasing = TRUE;
    }

    firstObjIndex = get_first_active_object(&objCount);

    numberOfSegments = render_opaque_segments(segmentIds, objectsVisible);

    rendermode_reset(&gTrackDL);
    sort_objects_by_dist(firstObjIndex, objCount - 1);
    visibleFlags = OBJ_FLAGS_INVIS_PLAYER1 << (get_current_viewport() & 1);

    for (i = firstObjIndex; i < objCount; i++) {
        obj = get_object(i);
        visible = 255;
        objFlags = obj->trans.flags;
        if (objFlags & OBJ_FLAGS_UNK_0080) {
            visible = 0;
        } else if (!(objFlags & OBJ_FLAGS_PARTICLE)) {
            visible = obj->opacity;
        }
        if (objFlags & visibleFlags) {
            visible = 0;
        }
        if (obj != NULL && visible == 255 && check_if_in_draw_range(obj) &&
            (objectsVisible[obj->segmentID + 1] || obj->cullRadius > 1000.0)) {
            if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
                render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                continue;
            } else if (obj->shadow != NULL) {
                shadow_render(obj, obj->shadow);
            }
            render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
            if (obj->waterEffect != NULL && obj->header->flags & HEADER_FLAGS_WATER_EFFECT) {
                watereffect_render(obj, obj->waterEffect);
            }
        }
    }

    for (i = objCount - 1; i >= firstObjIndex; i--) {
        obj = get_object(i);
        objFlags = obj->trans.flags;
        if (objFlags & visibleFlags) {
            visible = FALSE;
        } else {
            visible = TRUE;
        }
        if (obj != NULL && visible && objFlags & OBJ_FLAGS_UNK_0100 && objectsVisible[obj->segmentID + 1] &&
            check_if_in_draw_range(obj)) {
            if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
                render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                continue;
            } else if (obj->shadow != NULL) {
                shadow_render(obj, obj->shadow);
            }
            render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
            if (obj->waterEffect != NULL && obj->header->flags & HEADER_FLAGS_WATER_EFFECT) {
                watereffect_render(obj, obj->waterEffect);
            }
        }
    }

    render_transparent_segments_and_waves(segmentIds, numberOfSegments);

    // Particles and FX
    for (i = objCount - 1; i >= firstObjIndex; i--) {
        obj = get_object(i);
        visible = 255;
        objFlags = obj->trans.flags;
        if (objFlags & OBJ_FLAGS_UNK_0080) {
            visible = 1;
        } else if (!(objFlags & OBJ_FLAGS_PARTICLE)) {
            visible = obj->opacity;
        }
        if (objFlags & visibleFlags) {
            visible = 0;
        }
        if (obj->behaviorId == BHV_RACER && visible >= 255) {
            visible = 0;
        }
        if (obj != NULL && visible < 255 && objectsVisible[obj->segmentID + 1] && check_if_in_draw_range(obj)) {
            if (visible > 0) {
                if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
                    render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                    goto skip;
                } else if (obj->shadow != NULL) {
                    shadow_render(obj, obj->shadow);
                }
                render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                if ((obj->waterEffect != 0) && (obj->header->flags & HEADER_FLAGS_WATER_EFFECT)) {
                    watereffect_render(obj, obj->waterEffect);
                }
            }
        skip:
            if (obj->behaviorId == BHV_RACER) {
                render_racer_shield(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                render_racer_magnet(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
            }
        }
    }

    if (gVoidData != NULL && is_camera_behind_wall()) {
        void_check(segmentIds, numberOfSegments, get_current_viewport());
    }
    gAntiAliasing = FALSE;
}

/**
 * Render a batch of level geometry.
 * Since opaque and transparent are done in two separate runs, it will skip over the other.
 * Has a special case for the flashing lights in Spaceport Alpha, too.
 */
void render_level_segment(s32 segmentId, s32 nonOpaque) {
    LevelModelSegment *segment;
    s32 i;
    TriangleBatchInfo *batchInfo;
    TextureHeader *texture;
    s32 renderBatch;
    s32 numberVertices;
    s32 numberTriangles;
    ptrint_t vertices;
    ptrint_t triangles;
    s32 color;
    s32 isInvisible;
    s32 levelHeaderIndex;
    s32 texOffset;
    s32 wavesActive;
    s32 startPos;
    s32 endPos;
    s32 batchFlags;
    s32 textureFlags;
    //! @bug: batchInfo is uninitalized (the result is recomputed below; skipped on the host)
    segment = &gCurrentLevelModel->segments[segmentId];
    wavesActive = (nonOpaque && gWaveBlockCount) ? waves_block_hq(segment) : 0;
    if (nonOpaque) {
        startPos = segment->numberofOpaqueBatches;
        endPos = segment->numberOfBatches;
    } else {
        startPos = 0;
        endPos = segment->numberofOpaqueBatches;
    }
    for (i = startPos; i < endPos; i++) {
        batchInfo = &segment->batches[i];
        textureFlags = RENDER_NONE;
        isInvisible = batchInfo->flags & RENDER_HIDDEN;
        if (isInvisible) {
            continue;
        }
        batchFlags = batchInfo->flags;
        renderBatch = 0;
        if (batchInfo->textureIndex == 0xFF) {
            texture = FALSE;
        } else {
            texture = gCurrentLevelModel->textures[batchInfo->textureIndex].texture;
            textureFlags = texture->flags;
        }
        batchFlags |= RENDER_FOG_ACTIVE | RENDER_Z_COMPARE;
        if (!(batchFlags & RENDER_CUTOUT) && !(batchFlags & RENDER_DECAL)) {
            batchFlags |= gAntiAliasing;
        }
        if ((!(textureFlags & RENDER_SEMI_TRANSPARENT) && !(batchFlags & RENDER_WATER)) || batchFlags & RENDER_DECAL) {
            renderBatch = TRUE;
        }
        if (nonOpaque) {
            renderBatch = (renderBatch + 1) & 1;
        }
        if (wavesActive && batchFlags & RENDER_WATER) {
            renderBatch = FALSE;
        }
        if (!renderBatch) {
            continue;
        }
        numberTriangles = batchInfo->facesOffset;
        do { // Fakematch
            numberVertices = (batchInfo + 1)->verticesOffset - batchInfo->verticesOffset;
            numberTriangles = (batchInfo + 1)->facesOffset - numberTriangles;
            vertices = (ptrint_t) &segment->vertices[batchInfo->verticesOffset];
        } while (0);
        triangles = (ptrint_t) &segment->triangles[batchInfo->facesOffset];
        texOffset = batchInfo->texOffset << 14;
        // Batches tagged 1..7 take their colour from that colour cycle; the original indexed the
        // header as an array of 4-byte pointers, which on a 64-bit host read the wrong bytes.
        levelHeaderIndex = (batchFlags >> 28) & 7;
        if (levelHeaderIndex != (batchInfo->verticesOffset * 0)) {
            gDPSetEnvColor(gTrackDL++, gCurrentLevelHeader2->colourCycles[levelHeaderIndex - 1]->rgba.r,
                           gCurrentLevelHeader2->colourCycles[levelHeaderIndex - 1]->rgba.g,
                           gCurrentLevelHeader2->colourCycles[levelHeaderIndex - 1]->rgba.b,
                           gCurrentLevelHeader2->colourCycles[levelHeaderIndex - 1]->rgba.a);
        } else {
            gDPSetEnvColor(gTrackDL++, 255, 255, 255, 0);
        }
        if (batchFlags & RENDER_PULSING_LIGHTS) {
            color = gCurrentLevelHeader2->pulseLightData->outColorValue;
            gDPSetPrimColor(gTrackDL++, 0, 0, color, color, color, color);
            material_set_blinking_lights(&gTrackDL, texture, batchFlags, texOffset);
            gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(vertices), numberVertices, 0);
            gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(triangles), numberTriangles, TRIN_ENABLE_TEXTURE);
            gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, 255);
        } else {
            material_set(&gTrackDL, texture, batchFlags, texOffset);
            batchFlags = TRUE;
            if (texture == NULL) {
                batchFlags = FALSE;
            }
            gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(vertices), numberVertices, 0);
            gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(triangles), numberTriangles, batchFlags);
        }
    }
}

/**
 * Parse through applicable segments in the level.
 * Uses function recursion to ensure adjacent segments remain next to each other in the list.
 */
void traverse_segments_bsp_tree(s32 nodeIndex, s32 segmentIndex, s32 segmentIndex2, u8 *segmentsOrder,
                                s32 *segmentsOrderIndex) {
    BspTreeNode *curNode;
    s32 camValue;

    curNode = &gCurrentLevelModel->segmentsBspTree[nodeIndex];
    if (curNode->splitType == 0) {
        camValue = gSceneActiveCamera->trans.x_position; // Camera X
    } else if (curNode->splitType == 1) {
        camValue = gSceneActiveCamera->trans.y_position; // Camera Y
    } else {
        camValue = gSceneActiveCamera->trans.z_position; // Camera Z
    }

    if (camValue < curNode->splitValue) {
        if (curNode->leftNode != -1) {
            traverse_segments_bsp_tree(curNode->leftNode, segmentIndex, curNode->segmentIndex - 1, segmentsOrder,
                                       segmentsOrderIndex);
        } else {
            add_segment_to_order(segmentIndex, segmentsOrderIndex, segmentsOrder);
        }

        if (curNode->rightNode != -1) {
            traverse_segments_bsp_tree(curNode->rightNode, curNode->segmentIndex, segmentIndex2, segmentsOrder,
                                       segmentsOrderIndex);
        } else {
            add_segment_to_order(segmentIndex2, segmentsOrderIndex, segmentsOrder);
        }
    } else {
        if (curNode->rightNode != -1) {
            traverse_segments_bsp_tree(curNode->rightNode, curNode->segmentIndex, segmentIndex2, segmentsOrder,
                                       segmentsOrderIndex);
        } else {
            add_segment_to_order(segmentIndex2, segmentsOrderIndex, segmentsOrder);
        }

        if (curNode->leftNode != -1) {
            traverse_segments_bsp_tree(curNode->leftNode, segmentIndex, curNode->segmentIndex - 1, segmentsOrder,
                                       segmentsOrderIndex);
        } else {
            add_segment_to_order(segmentIndex, segmentsOrderIndex, segmentsOrder);
        }
    }
}

/**
 * Add this segment index to the specified segment ordering table if the segment in question is in view of the camera.
 */
void add_segment_to_order(s32 segmentIndex, s32 *segmentsOrderIndex, u8 *segmentsOrder) {
    u32 visibilityBits;
    if (segmentIndex < gCurrentLevelModel->numberOfSegments) {
        if (gSceneStartSegment != -1) {
            visibilityBits = gCurrentLevelModel->segmentsBitfields[gSceneStartSegment + (segmentIndex >> 3)];
            visibilityBits >>= segmentIndex & 7;
            visibilityBits &= 0xFF;
        } else {
            visibilityBits = 1;
        }
        if (visibilityBits & 1 && block_visible(&gCurrentLevelModel->segmentsBoundingBoxes[segmentIndex])) {
            segmentsOrder[(*segmentsOrderIndex)++] = segmentIndex;
        }
    }
}

/**
 * Checks if the active camera is currently inside this segment.
 * Has a small inner margin where it doesn't consider the camera inside.
 * Goes unused.
 */
UNUSED s32 check_if_inside_segment(Object *obj, s32 segmentIndex) {
    LevelModelSegmentBoundingBox *bb;
    s32 x, y, z;
    if (segmentIndex >= gCurrentLevelModel->numberOfSegments) {
        return FALSE;
    }
    bb = &gCurrentLevelModel->segmentsBoundingBoxes[segmentIndex];
    x = obj->trans.x_position;
    y = obj->trans.y_position;
    z = obj->trans.z_position;
    if ((x < (bb->x2 + 25)) && ((bb->x1 - 25) < x) && (z < (bb->z2 + 25)) && ((bb->z1 - 25) < z) &&
        (y < (bb->y2 + 25)) && ((bb->y1 - 25) < y)) {
        return TRUE;
    }

    return FALSE;
}

/**
 * Iterates through every existing segment to see which one the active camera is inside.
 * Uses mainly a two dimensional axis check here, instead of the function above.
 * Returns the segment currently inside.
 * Official Name: trackGetBlock
 */
s32 get_level_segment_index_from_position(f32 xPos, f32 yPos, f32 zPos) {
    LevelModelSegmentBoundingBox *bb;
    s32 i;
    s32 z = zPos;
    s32 x = xPos;
    s32 y = yPos;
    s32 minVal;
    s32 result;
    s32 heightDiff;

    if (gCurrentLevelModel == NULL) {
        return -1;
    }

    minVal = 1000000;
    result = -1;

    for (i = 0; i < gCurrentLevelModel->numberOfSegments; i++) {
        bb = &gCurrentLevelModel->segmentsBoundingBoxes[i];
        if (x < bb->x2 && bb->x1 < x && z < bb->z2 && bb->z1 < z) {
            heightDiff = (bb->y2 + bb->y1) >> 1; // Couldn't get / 2 to match, but >> 1 does.
            heightDiff = y - heightDiff;
            if (heightDiff < 0) {
                heightDiff = -heightDiff;
            }
            if (heightDiff < minVal) {
                minVal = heightDiff;
                result = i;
            }
        }
    }

    return result;
}

/**
 * Iterates through every existing segment to see which one the active camera is inside.
 * Uses mainly a two dimensional axis check here, instead of the function above.
 * Increments a counter based on if it's got a camera inside.
 * Because there's a tiny margin, multiple segments can be considered populated,
 * meaning that sometimes it will 2 instead of 1.
 */
s32 get_inside_segment_count_xz(s32 x, s32 z, s32 *segmentsOut) {
    s32 i;
    s32 cnt = 0;
    LevelModelSegmentBoundingBox *bb;
    for (i = 0; i < gCurrentLevelModel->numberOfSegments; i++) {
        bb = gCurrentLevelModel->segmentsBoundingBoxes + i;
        if (x < bb->x2 + 4 && bb->x1 - 4 < x && z < bb->z2 + 4 && bb->z1 - 4 < z) {
            *segmentsOut = i;
            cnt++;
            segmentsOut++;
        }
    }
    return cnt;
}

/**
 * Carbon copy of the above function, but takes into account the Y axis, too.
 * Official name: trackGetCubeBlockList
 */
s32 get_inside_segment_count_xyz(s32 *segmentsOut, s16 xPos1, s16 yPos1, s16 zPos1, s16 xPos2, s16 yPos2, s16 zPos2) {
    s32 cnt;
    s32 i;
    LevelModelSegmentBoundingBox *bb;

    xPos1 -= 4;
    yPos1 -= 4;
    zPos1 -= 4;
    xPos2 += 4;
    yPos2 += 4;
    zPos2 += 4;

    i = 0;
    cnt = 0;

    while (i < gCurrentLevelModel->numberOfSegments) {
        bb = &gCurrentLevelModel->segmentsBoundingBoxes[i];
        if ((bb->x2 >= xPos1) && (xPos2 >= bb->x1) && (bb->z2 >= zPos1) && (zPos2 >= bb->z1) && (bb->y2 >= yPos1) &&
            (yPos2 >= bb->y1)) {
            cnt++;
            *segmentsOut++ = i;
        }
        i++;
    }
    return cnt;
}

/**
 * Returns this block data.
 */
LevelModelSegment *block_get(s32 segmentID) {
    if (segmentID < 0 || gCurrentLevelModel->numberOfSegments < segmentID) {
        return NULL;
    }

    return &gCurrentLevelModel->segments[segmentID];
}

/**
 * Returns the bounding box data of this block.
 * Official name: trackBlockDim
 */
LevelModelSegmentBoundingBox *block_boundbox(s32 segmentID) {
    if (segmentID < 0 || gCurrentLevelModel->numberOfSegments < segmentID) {
        return NULL;
    }

    return &gCurrentLevelModel->segmentsBoundingBoxes[segmentID];
}

/**
 * Rebuilds the view frustum planes from three reference points per plane
 * transformed by the camera matrix.
 */
void compute_frustum_planes(void) {
    f32 ox1;
    f32 oy1;
    f32 oz1;
    f32 ox2;
    f32 oy2;
    f32 oz2;
    f32 ox3;
    f32 oy3;
    f32 oz3;
    MtxF *cameraMatrix;
    f32 inverseMagnitude;
    f32 x;
    f32 y;
    f32 z;
    s32 i;
    f32 w;

    cameraMatrix = get_projection_matrix_f32();
    for (i = 0; i < ARRAY_COUNT(gFrustumPlanes);) {
        x = gFrustumPlanePoints[i][0].x;
        y = gFrustumPlanePoints[i][0].y;
        z = gFrustumPlanePoints[i][0].z;
        ox1 = x;
        oy1 = y;
        oz1 = z;
        mtxf_transform_point(*cameraMatrix, x, y, z, &ox1, &oy1, &oz1);
        x = gFrustumPlanePoints[i][1].x;
        y = gFrustumPlanePoints[i][1].y;
        z = gFrustumPlanePoints[i][1].z;
        ox2 = x;
        oy2 = y;
        oz2 = z;
        mtxf_transform_point(*cameraMatrix, x, y, z, &ox2, &oy2, &oz2);
        x = gFrustumPlanePoints[i][2].x;
        y = gFrustumPlanePoints[i][2].y;
        z = gFrustumPlanePoints[i][2].z;
        ox3 = x;
        oy3 = y;
        oz3 = z;
        mtxf_transform_point(*cameraMatrix, x, y, z, &ox3, &oy3, &oz3);
        x = ((oz2 - oz3) * oy1) + (oy2 * (oz3 - oz1)) + (oy3 * (oz1 - oz2));
        y = ((ox2 - ox3) * oz1) + (oz2 * (ox3 - ox1)) + (oz3 * (ox1 - ox2));
        z = ((oy2 - oy3) * ox1) + (ox2 * (oy3 - oy1)) + (ox3 * (oy1 - oy2));
        inverseMagnitude = (1.0 / sqrtf((x * x) + (y * y) + (z * z)));
        if (inverseMagnitude > 0.0) {
            x *= inverseMagnitude;
            y *= inverseMagnitude;
            z *= inverseMagnitude;
        }
        w = -((ox1 * x) + (oy1 * y) + (oz1 * z));
        gFrustumPlanes[i].x = x;
        gFrustumPlanes[i].y = y;
        gFrustumPlanes[i].z = z;
        gFrustumPlanes[i].w = w;
        i++;
    }
}

/**
 * Takes a normalised (0-1) face direction of the active camera, then adds together a magnitude
 * to a total figure to determine whether or not a segment should be visible.
 * There's a large unused portion at the bottom writing to two vars, that are never later read.
 */
s32 block_visible(LevelModelSegmentBoundingBox *bb) {
    s64 planeDist;
    s32 i, j;
    s32 isVisible;
    f32 dirX, dirY, dirZ, dirW;
    f32 x, y, z;

    for (j = 0; j < 3; j++) {
        dirX = gFrustumPlanes[j].x;
        dirY = gFrustumPlanes[j].y;
        dirZ = gFrustumPlanes[j].z;
        dirW = gFrustumPlanes[j].w;

        for (i = 0, isVisible = FALSE; i < 8 && !isVisible; i++) {
            if (i & 1) {
                planeDist = bb->x1 * dirX;
            } else {
                planeDist = bb->x2 * dirX;
            }
            if (i & 2) {
                planeDist += bb->y1 * dirY;
            } else {
                planeDist += bb->y2 * dirY;
            }
            if (i & 4) {
                planeDist += bb->z1 * dirZ;
            } else {
                planeDist += bb->z2 * dirZ;
            }
            planeDist += dirW;
            if (planeDist > 0) {
                isVisible = TRUE;
            }
        }
        if (i == 8 && !isVisible) {
            return FALSE;
        }
    }
    // From here until the "return TRUE" goes completely unused, functionally.
    x = (bb->x2 + bb->x1) >> 1;
    y = (bb->y2 + bb->y1) >> 1;
    z = (bb->z2 + bb->z1) >> 1;
    gCurrBBoxDistanceToCamera = get_distance_to_active_camera(x, y, z);
    if (gCurrBBoxDistanceToCamera < 1000.0) {
        gIsNearCurrBBox = TRUE;
    } else {
        gIsNearCurrBBox = FALSE;
    }
    return TRUE;
}

/**
 * Get the draw distance of the object, then compare it to the active camera position.
 * At the edge of its view distance, it will set its alpha based on distance, giving it a fade in or out effect.
 * Objects in range return true, objects out of range return false.
 */
s32 check_if_in_draw_range(Object *obj) {
    f32 w;
    f32 y;
    f32 fadeDist;
    f32 z;
    f32 x;
    s32 viewDistance;
    s32 alpha;
    s32 i;
    Object_AnimatedObject *animatedObj;
    f32 accum;
    s32 fadeRange;
    f32 dist;
    Object_Racer *racer;

    if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE)) {
        alpha = 255;
        viewDistance = obj->header->drawDistance;
        if (obj->header->drawDistance) {
            if (gScenePlayerViewports == FOUR_PLAYERS) {
                viewDistance *= 0.5;
            }

            dist = get_distance_to_active_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);

            if (viewDistance < dist) {
                return FALSE;
            }

            fadeDist = viewDistance * 0.8;
            if (fadeDist < dist) {
                fadeRange = viewDistance - fadeDist;
                if (fadeRange > 0) {
                    fadeDist = dist - fadeDist;
                    alpha = ((f32) (1.0 - ((fadeDist) / fadeRange)) * 255.0);
                }
                if (alpha == 0) {
                    alpha = 1;
                }
            }
        }
        if (obj_behaviour(obj->behaviorId)->fade != NULL) {
            obj_behaviour(obj->behaviorId)->fade(obj, alpha);
        } else {
            obj->opacity = alpha;
        }
        for (i = 0; i < 3; i++) {
            x = gFrustumPlanes[i].x;
            z = gFrustumPlanes[i].z;
            w = gFrustumPlanes[i].w;
            y = gFrustumPlanes[i].y;
            accum = (x * obj->trans.x_position) + (y * obj->trans.y_position) + (z * obj->trans.z_position) + w +
                    obj->cullRadius;
            if (accum < 0.0f) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

/**
 * Updates the stored perspective of the camera, as well as the envmap values derived from it.
 */
UNUSED void update_perspective_and_envmap(void) {
    gSceneActiveCamera = cam_get_active_camera();
    compute_scene_camera_transform_matrix();
    update_envmap_position((f32) gScenePerspectivePos.x / 65536.0f, (f32) gScenePerspectivePos.y / 65536.0f,
                           (f32) gScenePerspectivePos.z / 65536.0f);
}

/**
 * Take the current camera position and calculate the perspective position, for envmapping.
 */
void compute_scene_camera_transform_matrix(void) {
    MtxF mtx;
    ObjectTransform trans;

    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = -65536.0f;

    trans.rotation.z_rotation = gSceneActiveCamera->trans.rotation.z_rotation;
    trans.rotation.x_rotation = gSceneActiveCamera->trans.rotation.x_rotation;
    trans.rotation.y_rotation = gSceneActiveCamera->trans.rotation.y_rotation;
    trans.x_position = 0.0f;
    trans.y_position = 0.0f;
    trans.z_position = 0.0f;
    trans.scale = 1.0f;

    mtxf_from_transform(&mtx, &trans);
    mtxf_transform_point(mtx, x, y, z, &x, &y, &z);

    // Store x/y/z as integers
    gScenePerspectivePos.x = x;
    gScenePerspectivePos.y = y;
    gScenePerspectivePos.z = z;
}
