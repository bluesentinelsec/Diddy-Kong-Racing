#include "tracks_internal.h"

// Set on the first point of each pair, once the pairs are linked up in void_check.
#define VOID_POINT_PAIR_START 2

/**
 * Allocates control data and geometry for the void.
 * The void refers to the flat coloured background that's shaped in real time to prevent
 * being able to see through level geometry, breaking immersion.
 */
void void_init(s32 viewportCount) {
    s32 i;
    s32 pointsSize;
    s32 pairsSize;
    s32 vtxLimit;
    s32 triLimit;
    u8 *ptr;

    gVoidCheckMaxPoints = 175;
    gVoidPrimLimit = 45;
    // Halve the primitive limit for multiplayer.
    if (viewportCount >= 2) {
        gVoidPrimLimit >>= 1;
    }

    pointsSize = (gVoidCheckMaxPoints + 6) * sizeof(VoidCheckPoint);
    pairsSize = gVoidCheckMaxPoints + 5;
    vtxLimit = (gVoidPrimLimit + 5) * 4 * sizeof(Vertex);
    triLimit = (gVoidPrimLimit + 5) * 2 * sizeof(Triangle);

    gVoidMesh = mempool_alloc_safe(viewportCount * sizeof(VoidMesh), COLOUR_TAG_CYAN);
    gVoidData = mempool_alloc_safe(pointsSize + pairsSize + (vtxLimit + triLimit) * 2 * viewportCount, COLOUR_TAG_CYAN);

    ptr = gVoidData;

    if (ptr != NULL) {
        gVoidCheckPoints = (VoidCheckPoint *) ptr;
        ptr += pointsSize;

        gVoidCheckPairs = (s8 *) ptr;
        // Align by 8
        ptr = (u8 *) ((uintptr_t) (ptr + pairsSize + 8) & ~7);

        for (i = 0; i < viewportCount; i++) {
            gVoidMesh[i].tris[0] = (Triangle *) ptr;
            ptr += triLimit;

            gVoidMesh[i].tris[1] = (Triangle *) ptr;
            ptr += triLimit;

            gVoidMesh[i].verts[0] = (Vertex *) ptr;
            ptr += vtxLimit;

            gVoidMesh[i].verts[1] = (Vertex *) ptr;
            ptr += vtxLimit;
        }
    }
    gVoidVertexFlip = 0;
}

/**
 * Free the void mesh and control data if it exists.
 */
void void_free(void) {
    if (gVoidData != NULL) {
        mempool_free(gVoidMesh);
        mempool_free(gVoidData);
        gVoidData = NULL;
    }
}

// root func for the out of bounds void rendering
void void_check(u8 *segmentIds, s32 numberOfSegments, s32 viewportIndex) {
    s16 i;
    s16 j;
    f32 yCameraSins;
    f32 yCameraCoss;
    f32 aheadPlaneD;
    Vertex *vtx;
    Triangle *tri;
    s16 nextOffset;
    s16 openCount;
    s16 currOffset;
    s16 breakLoop;
    s32 *ptr2;
    LevelModelSegmentBoundingBox *bbox;
    s16 sum;
    s8 openPairs[24]; // possible UB here, real size is unknown

    gVoidTris[0] = gVoidMesh[viewportIndex].tris[0];
    gVoidTris[1] = gVoidMesh[viewportIndex].tris[1];
    gVoidVerts[0] = gVoidMesh[viewportIndex].verts[0];
    gVoidVerts[1] = gVoidMesh[viewportIndex].verts[1];
    material_set_no_tex_offset(&gTrackDL, NULL, RENDER_ANTI_ALIASING | RENDER_Z_COMPARE);
    gVoidCheckPairIndex = 0;
    gVoidCheckPointCount = 0;

    yCameraSins = sins_f(-gSceneActiveCamera->trans.rotation.y_rotation);
    yCameraCoss = coss_f(-gSceneActiveCamera->trans.rotation.y_rotation);

    gVoidCheckAheadX = gSceneActiveCamera->trans.x_position + yCameraSins * 250.0;
    gVoidCheckAheadZ = gSceneActiveCamera->trans.z_position + yCameraCoss * 250.0;

    aheadPlaneD = -(yCameraSins * gVoidCheckAheadX + yCameraCoss * gVoidCheckAheadZ);

    gVoidCheckSidePlaneX = -yCameraCoss;
    gVoidCheckSidePlaneZ = yCameraSins;
    gVoidCheckSidePlaneD = -(gVoidCheckSidePlaneX * gVoidCheckAheadX + gVoidCheckSidePlaneZ * gVoidCheckAheadZ);

    i = 0;
    for (; i < numberOfSegments; i++) {
        bbox = &gCurrentLevelModel->segmentsBoundingBoxes[segmentIds[i]];
        breakLoop = 0;
        breakLoop += bbox->x1 * yCameraSins + yCameraCoss * bbox->z1 + aheadPlaneD <= 0.0;
        breakLoop += yCameraSins * bbox->x2 + yCameraCoss * bbox->z1 + aheadPlaneD <= 0.0;
        breakLoop += bbox->x1 * yCameraSins + yCameraCoss * bbox->z2 + aheadPlaneD <= 0.0;
        breakLoop += yCameraSins * bbox->x2 + yCameraCoss * bbox->z2 + aheadPlaneD <= 0.0;
        if (breakLoop & 3) {
            void_check_add_segment_edges(&gCurrentLevelModel->segments[segmentIds[i]], yCameraSins, yCameraCoss,
                                         aheadPlaneD);
            if (gCurrentLevelModel->segments[segmentIds[i]].segmentFlags & 2) {
                void_check_add_bbox_edges(bbox, yCameraSins, yCameraCoss, aheadPlaneD);
            }
        }
    }

    void_check_insert_point(300, gCurrentLevelModel->lowerYBounds - 195, 1);
    void_check_insert_point(-300, gCurrentLevelModel->lowerYBounds - 195, 1);
    void_check_insert_point(300, gCurrentLevelModel->upperYBounds + 195, 0);
    void_check_insert_point(-300, gCurrentLevelModel->upperYBounds + 195, 0);

    if (gVoidCheckPointCount >= gVoidCheckMaxPoints || gVoidCheckPointCount == 0) {
        return;
    }

    do {
        breakLoop = TRUE;
        ptr2 = (s32 *) gVoidCheckPoints;
        for (i = 0; i < gVoidCheckPointCount - 1; i++, ptr2 += 2) {
            if (gVoidCheckPoints[i + 1].sideOffset < gVoidCheckPoints[i].sideOffset) {
                s32 tmp;

                tmp = ptr2[0];
                ptr2[0] = ptr2[2];
                ptr2[2] = tmp;

                tmp = ptr2[1];
                ptr2[1] = ptr2[3];
                ptr2[3] = tmp;

                breakLoop = FALSE;
            }
        }
    } while (!breakLoop);

    openCount = 0;

    for (i = 0; i < gVoidCheckPointCount; i++) {
        j = gVoidCheckPoints[i].pairIndex * 2;
        if (gVoidCheckPairs[j] == -1) {
            gVoidCheckPoints[i].flags |= VOID_POINT_PAIR_START;
            gVoidCheckPairs[j] = i;
        } else {
            gVoidCheckPairs[j + 1] = i;
        }
    }
    currOffset = nextOffset = gVoidCheckPoints[0].sideOffset;

    vtx = gTrackVtxPtr;
    tri = gTrackTriPtr;
    gTrackVtxPtr = gVoidVerts[gVoidVertexFlip];
    gTrackTriPtr = gVoidTris[gVoidVertexFlip];

    gVoidVertexFlip = 1 - gVoidVertexFlip;
    gVoidCurrVerts = gTrackVtxPtr;
    gVoidCurrTris = gTrackTriPtr;
    gVoidVertCount = 0;
    gVoidPrimCount = 0;

    i = 0;
    while (i < gVoidCheckPointCount) {
        while (i < gVoidCheckPointCount && currOffset == gVoidCheckPoints[i].sideOffset) {
            if (gVoidCheckPoints[i].flags & VOID_POINT_PAIR_START) {
                openPairs[openCount] = gVoidCheckPoints[i].pairIndex;
                openCount++;
            } else {
                for (j = 0; j < openCount; j++) {
                    if (openPairs[j] == gVoidCheckPoints[i].pairIndex) {
                        openCount--;
                        while (j < openCount) {
                            openPairs[j] = openPairs[j + 1];
                            j++;
                        }
                    }
                }
            }
            i++;
        }
        if (i < gVoidCheckPointCount) {
            nextOffset = gVoidCheckPoints[i].sideOffset;
            if (currOffset != nextOffset) {
                void_check_sort_points(openCount, openPairs, nextOffset, currOffset);
                currOffset = nextOffset;
            }
        }
    }
    if (gVoidVertCount != 0) {
        gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(gVoidCurrVerts), gVoidVertCount, 0);
        gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(gVoidCurrTris), gVoidVertCount >> 1, TRIN_DISABLE_TEXTURE);
    }
    gTrackVtxPtr = vtx;
    gTrackTriPtr = tri;
}

/**
 * Void check: adds the edges of a segment bounding box where it crosses the
 * camera plane, for segments that are open to the void.
 */
void void_check_add_bbox_edges(LevelModelSegmentBoundingBox *bbox, f32 sinY, f32 cosY, f32 planeD) {
    f32 cornerX[4];
    f32 cornerZ[4];
    f32 cornerDist[4];
    f32 temp;
    f32 crossX[2];
    f32 crossZ[2];
    s16 index;
    s16 nextIndex;
    s16 cornerBehind[4];
    s16 crossCount;
    s16 behindCount;

    cornerX[0] = bbox->x1;
    cornerZ[0] = bbox->z1;
    cornerX[1] = bbox->x2;
    cornerZ[1] = bbox->z1;
    cornerX[2] = bbox->x2;
    cornerZ[2] = bbox->z2;
    cornerX[3] = bbox->x1;
    cornerZ[3] = bbox->z2;

    behindCount = 0;
    for (index = 0; index < 4; index++) {
        cornerDist[index] = (sinY * cornerX[index]) + (cornerZ[index] * cosY) + planeD;
        cornerBehind[index] = cornerDist[index] <= 0.0;
        behindCount += cornerBehind[index];
    }
    if (behindCount != 0) {
        crossCount = 0;
        if (behindCount == 4) {
            return;
        }
        for (index = 0; index < 4; index++) {
            nextIndex = index + 1;
            if (nextIndex >= 4) {
                nextIndex = 0;
            }
            if ((cornerBehind[nextIndex] != cornerBehind[index]) != 0U) {
                temp = cornerDist[index] / (cornerDist[index] - cornerDist[nextIndex]);
                crossX[crossCount] = cornerX[index] + ((cornerX[nextIndex] - cornerX[index]) * temp);
                crossZ[crossCount] = cornerZ[index] + ((cornerZ[nextIndex] - cornerZ[index]) * temp);
                crossCount++;
            }
        }
        if (crossCount != 2) {
            return;
        }

        cornerDist[0] = (gVoidCheckSidePlaneX * crossX[0]) + (gVoidCheckSidePlaneZ * crossZ[0]) + gVoidCheckSidePlaneD;
        cornerDist[1] = (gVoidCheckSidePlaneX * crossX[1]) + (gVoidCheckSidePlaneZ * crossZ[1]) + gVoidCheckSidePlaneD;

        if (cornerDist[1] < cornerDist[0]) {
            temp = cornerDist[0];
            cornerDist[0] = cornerDist[1];
            cornerDist[1] = temp;
        }

        // Returns must be on the same line.
        // clang-format off
        if (-300.0 > cornerDist[1]) { return; }
        if (cornerDist[0] > 300.0) { return; }
        // clang-format on

        if (cornerDist[0] < -300.0) {
            cornerDist[0] = -300.0f;
        }
        if (cornerDist[1] > 300.0) {
            cornerDist[1] = 300.0f;
        }
        void_check_insert_point(cornerDist[1], gCurrentLevelModel->lowerYBounds - 195, 0);
        void_check_insert_point(cornerDist[0], gCurrentLevelModel->lowerYBounds - 195, 0);
    }
}

/**
 * Void check: walks a segment's triangles and records where their edges cross
 * the camera plane.
 */
void void_check_add_segment_edges(LevelModelSegment *segment, f32 sinY, f32 cosY, f32 planeD) {
    s16 i;
    s16 index;
    s16 verticesOffset;
    s16 nextFaceOffset;
    s16 nextIndex;
    s16 currFaceOffset;
    s16 j;
    Vertex *vert;
    s8 sideFlags[3];
    f32 temp;
    s16 frontCount;
    s16 crossCount;
    s16 inRange;
    s16 planeIndex;
    s16 pointFlags;
#ifdef AVOID_UB
    f32 vertX[3]; // This really should be size of 3, but something is keeping it from matching that way.
#else
    f32 vertX[2];
#endif
    f32 vertY[3];
    f32 vertZ[3];
    f32 planeDist[3];
    f32 crossY[3];
    f32 crossX[2];
    f32 crossZ[2];
    f32 unusedCrossY[2];

    if (gVoidCheckPointCount >= gVoidCheckMaxPoints) {
        return;
    }

    for (i = 0; i < segment->numberOfBatches; i++) {
        currFaceOffset = segment->batches[i].facesOffset;
        verticesOffset = segment->batches[i].verticesOffset;
        nextFaceOffset = segment->batches[i + 1].facesOffset;
        if (segment->batches[i].flags & (RENDER_HIDDEN | RENDER_NO_COLLISION)) {
            currFaceOffset = nextFaceOffset;
        }
        for (j = currFaceOffset; j < nextFaceOffset; j++) {
            if ((segment->triangles[j].flags & BACKFACE_DRAW)) {
                continue;
            }
            frontCount = 0;
            for (index = 0; index < 3; index++) {
                vert = &(segment->triangles[j].verticesArray[index + 1] + verticesOffset)[segment->vertices];
                vertX[index] = vert->x;
                vertY[index] = vert->y;
                vertZ[index] = vert->z;
                planeDist[index] = (sinY * vertX[index]) + (cosY * vertZ[index]) + planeD;

                sideFlags[index] = (planeDist[index] <= 0.0);
                frontCount += (sideFlags[index] <= 0.0);
            }
            if ((frontCount == 1) || (frontCount == 2)) {
                for (crossCount = 0, index = 0; index < 3; index++) {
                    nextIndex = index + 1;
                    if (nextIndex >= 3) {
                        nextIndex = 0;
                    }
                    if ((sideFlags[nextIndex] != sideFlags[index]) != 0) {
                        temp = planeDist[index] / (planeDist[index] - planeDist[nextIndex]);
                        crossX[crossCount] = vertX[index] + ((vertX[nextIndex] - vertX[index]) * temp);
                        crossY[crossCount] = vertY[index] + ((vertY[nextIndex] - vertY[index]) * temp);
                        unusedCrossY[crossCount] = crossY[crossCount];
                        crossZ[crossCount] = vertZ[index] + ((vertZ[nextIndex] - vertZ[index]) * temp);
                        crossCount++;
                    }
                }

                inRange = 0;
                sideFlags[0] = 0;
                sideFlags[1] = 0;
                planeDist[0] =
                    (gVoidCheckSidePlaneX * crossX[0]) + (gVoidCheckSidePlaneZ * crossZ[0]) + gVoidCheckSidePlaneD;
                planeDist[1] =
                    (gVoidCheckSidePlaneX * crossX[1]) + (gVoidCheckSidePlaneZ * crossZ[1]) + gVoidCheckSidePlaneD;
                if (planeDist[0] < -300.0) {
                    sideFlags[0] = 1;
                }
                if (planeDist[0] > 300.0) {
                    sideFlags[0] |= 2;
                }
                if (planeDist[1] < -300.0) {
                    sideFlags[1] = 1;
                }
                if (planeDist[1] > 300.0) {
                    sideFlags[1] |= 2;
                }
                // clang-format off
                if ((sideFlags[0] | sideFlags[1]) == 0) {  inRange = 1; }
                // clang-format on
                else if ((sideFlags[1] != sideFlags[0]) != 0) {
                    index = 0;
                    if (planeDist[1] < planeDist[0]) {
                        index = 1;
                    }
                    nextIndex = 1 - index;
                    if (sideFlags[index] == 1) {
                        temp = ((-planeDist[index] - 300.0) / (planeDist[nextIndex] - planeDist[index]));
                        crossY[index] = crossY[index] + ((crossY[nextIndex] - crossY[index]) * temp);
                        planeDist[index] = -300.0f;
                    }
                    if (sideFlags[nextIndex] == 2) {
                        temp = ((planeDist[nextIndex] - 300.0) / (planeDist[nextIndex] - planeDist[index]));
                        crossY[nextIndex] = crossY[nextIndex] + ((crossY[index] - crossY[nextIndex]) * temp);
                        planeDist[nextIndex] = 300.0f;
                    }
                    inRange = 1;
                }
                if (inRange != 0) {
                    planeIndex = (segment->collisionFacets[j].basePlaneIndex << 2);
                    temp = (crossX[0] + gVoidCheckSidePlaneX) * segment->collisionPlanes[planeIndex];
                    temp += crossY[0] * segment->collisionPlanes[planeIndex + 1];
                    temp += (crossZ[0] + gVoidCheckSidePlaneZ) * segment->collisionPlanes[planeIndex + 2];
                    temp += segment->collisionPlanes[planeIndex + 3];
                    pointFlags = (temp > 0.0) << 2;
                    if (segment->collisionPlanes[planeIndex + 1] < 0.0f) {
                        pointFlags |= 1;
                    }
                    if (planeDist[0] == planeDist[1]) {
                        pointFlags |= 8;
                    }
                    void_check_insert_point(planeDist[0], crossY[0], pointFlags);
                    void_check_insert_point(planeDist[1], crossY[1], pointFlags);
                }
            }
        }
    }
}

/**
 * Void check: inserts a crossing point into the sorted point list, pairing
 * points as it goes.
 */
void void_check_insert_point(s16 sideOffset, s16 height, s32 flags) {
    s16 i;
    s16 j;

    if (gVoidCheckPointCount < gVoidCheckMaxPoints) {
        i = 0;
        while (i < gVoidCheckPointCount && gVoidCheckPoints[i].sideOffset < sideOffset) {
            i++;
        }
        while (i < gVoidCheckPointCount && sideOffset == gVoidCheckPoints[i].sideOffset &&
               gVoidCheckPoints[i].height < height) {
            i++;
        }
        j = gVoidCheckPointCount;
        while (i < j) {
            gVoidCheckPoints[j].sideOffset = gVoidCheckPoints[j - 1].sideOffset;
            gVoidCheckPoints[j].height = gVoidCheckPoints[j - 1].height;
            gVoidCheckPoints[j].pairIndex = gVoidCheckPoints[j - 1].pairIndex;
            gVoidCheckPoints[j].flags = gVoidCheckPoints[j - 1].flags;
            j--;
        }
        gVoidCheckPoints[i].sideOffset = sideOffset;
        gVoidCheckPoints[i].height = height;
        gVoidCheckPoints[i].unused4 = 0;
        gVoidCheckPoints[i].pairIndex = gVoidCheckPairIndex;
        gVoidCheckPoints[i].flags = flags;
        gVoidCheckPairs[gVoidCheckPointCount] = -1;
        if (gVoidCheckPointCount & 1) {
            gVoidCheckPairIndex++;
        }
        gVoidCheckPointCount++;
    }
}

/**
 * Void check: orders the paired crossing points so the spans between them can
 * be tested against the camera.
 */
void void_check_sort_points(s16 openCount, s8 *openPairs, f32 nextOffset, f32 currOffset) {
    VoidCheckPoint *next;
    VoidCheckPoint *curr;
    s16 heightIndex0;
    s16 heightIndex1;
    f32 currSide;
    f32 currHeight;
    f32 nextSide;
    f32 nextHeight;
    s32 noSwap;
    s16 i;
    s16 j;
    f32 spanHeights[20];
    f32 spanMidSum[10];
    s8 order[10];
    s8 pair;
    s8 currFlag;
    s8 nextFlag;
    s8 swapByte;

    if (openCount >= 10 || openCount == 0) {
        return;
    }

    for (j = 0, i = 0; i < openCount;) {
        pair = openPairs[i];
        curr = &gVoidCheckPoints[gVoidCheckPairs[(s16) (pair * 2)]];
        next = &gVoidCheckPoints[gVoidCheckPairs[((s16) (pair * 2)) + 1]];
        currSide = (f32) curr->sideOffset;
        currHeight = (f32) curr->height;
        nextSide = (f32) next->sideOffset;
        nextHeight = (f32) next->height;
        if (currSide == nextSide) {
            return;
        }
        spanHeights[j++] =
            ((nextHeight - currHeight) * ((currSide - currOffset) / ((currSide - nextSide)))) + currHeight;
        spanMidSum[i] = spanHeights[j - 1];
        order[i] = i;
        spanHeights[j++] =
            ((currHeight - nextHeight) * ((nextOffset - nextSide) / ((currSide - nextSide)))) + nextHeight;
        spanMidSum[i] += spanHeights[j - 1];
        i++;
    }

    do {
        noSwap = TRUE;
        for (i = 0; i < openCount - 1; i++) {
            if (spanMidSum[order[i + 1]] < spanMidSum[order[i]]) {
                swapByte = openPairs[i];
                openPairs[i] = openPairs[i + 1];
                openPairs[i + 1] = swapByte;
                swapByte = order[i];
                order[i] = order[i + 1];
                order[i + 1] = swapByte;
                noSwap = FALSE;
            }
        }
    } while (noSwap == FALSE);

    for (i = 0; i < openCount - 1; i++) {
        currFlag = gVoidCheckPoints[gVoidCheckPairs[openPairs[i] * 2]].flags & 1;
        nextFlag = gVoidCheckPoints[gVoidCheckPairs[openPairs[i + 1] * 2]].flags & 1;
        if (currFlag && !nextFlag) {
            heightIndex0 = (order[i] * 2);
            heightIndex1 = (order[i + 1] * 2);
            void_generate_primitive(&spanHeights[heightIndex0], &spanHeights[heightIndex1], currOffset, nextOffset);
        }
    }
}

s32 void_generate_primitive(f32 *topHeights, f32 *bottomHeights, f32 startOffset, f32 endOffset) {
    Vertex *verts;
    Triangle *tris;
    s32 triIndex;
    s16 vertZ1;
    s16 vertX2;
    s16 vertZ2;
    s16 vertX1;
    u8 colour_r;
    u8 colour_g;
    u8 colour_b;
    u8 colour_a;

    if (gVoidPrimCount >= gVoidPrimLimit) {
        return NULL;
    }

    if (gVoidVertCount == 24) {
        gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(gVoidCurrVerts), gVoidVertCount, 0);
        gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(gVoidCurrTris), (gVoidVertCount >> 1), TRIN_DISABLE_TEXTURE);
        gVoidCurrVerts = gTrackVtxPtr;
        gVoidVertCount = 0;
        gVoidCurrTris = gTrackTriPtr;
    }

    vertX1 = startOffset * gVoidCheckSidePlaneX + gVoidCheckAheadX;
    vertZ1 = startOffset * gVoidCheckSidePlaneZ + gVoidCheckAheadZ;
    vertX2 = endOffset * gVoidCheckSidePlaneX + gVoidCheckAheadX;
    vertZ2 = endOffset * gVoidCheckSidePlaneZ + gVoidCheckAheadZ;

    colour_r = gVoidColourR;
    colour_g = gVoidColourG;
    colour_b = gVoidColourB;
    colour_a = 0xFF;

    verts = gTrackVtxPtr;
    verts[0].x = vertX1;
    verts[0].y = topHeights[0] + 2.0f;
    verts[0].z = vertZ1;
    verts[0].r = colour_r;
    verts[0].g = colour_g;
    verts[0].b = colour_b;
    verts[0].a = colour_a;

    verts[1].x = vertX2;
    verts[1].y = topHeights[1] + 2.0f;
    verts[1].z = vertZ2;
    verts[1].r = colour_r;
    verts[1].g = colour_g;
    verts[1].b = colour_b;
    verts[1].a = colour_a;

    verts[2].x = vertX1;
    verts[2].y = bottomHeights[0] - 2.0f;
    verts[2].z = vertZ1;
    verts[2].r = colour_r;
    verts[2].g = colour_g;
    verts[2].b = colour_b;
    verts[2].a = colour_a;

    verts[3].x = vertX2;
    verts[3].y = bottomHeights[1] - 2.0f;
    verts[3].z = vertZ2;
    verts[3].r = colour_r;
    verts[3].g = colour_g;
    verts[3].b = colour_b;
    verts[3].a = colour_a;
    // @fake
    if (1) {}
    verts += 4;
    gTrackVtxPtr = verts;

    tris = gTrackTriPtr;
    triIndex = (u8) gVoidVertCount;

    // @fake, using index 0 directly doesn't match
    vertX2 = 0;
    tris[vertX2].flags = BACKFACE_DRAW;
    tris[vertX2].vi0 = triIndex + 2;
    tris[vertX2].vi1 = triIndex + 1;
    tris[vertX2].vi2 = triIndex;
    tris[vertX2].uv0.u = 1024 - 32;
    tris[vertX2].uv0.v = 1024 - 32;
    tris[vertX2].uv1.u = 1024 - 32;
    tris[vertX2].uv1.v = 0;
    tris[vertX2].uv2.u = 1;
    tris[vertX2].uv2.v = 0;

    tris[1].flags = BACKFACE_DRAW;
    tris[1].vi0 = triIndex + 3;
    tris[1].vi1 = triIndex + 1;
    tris[1].vi2 = triIndex + 2;
    tris[1].uv0.u = 1;
    tris[1].uv0.v = 1024 - 32;
    tris[1].uv1.u = 1024 - 32;
    tris[1].uv1.v = 1024 - 32;
    tris[1].uv2.u = 1;
    tris[1].uv2.v = 0;
    tris += 2;
    gTrackTriPtr = tris;

    gVoidVertCount += 4;
    gVoidPrimCount++;
    return NULL;
}
