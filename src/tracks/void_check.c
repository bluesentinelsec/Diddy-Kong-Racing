#include "tracks_internal.h"

/**
 * Allocates control data and geometry for the void.
 * The void refers to the flat coloured background that's shaped in real time to prevent
 * being able to see through level geometry, breaking immersion.
 */
void void_init(s32 viewportCount) {
    s32 i;
    s32 sp30;
    s32 sp2C;
    s32 vtxLimit;
    s32 triLimit;
    u8 *ptr;

    gVoidCheckMaxPoints = 175;
    gVoidPrimLimit = 45;
    // Halve the primitive limit for multiplayer.
    if (viewportCount >= 2) {
        gVoidPrimLimit >>= 1;
    }

    sp30 = (gVoidCheckMaxPoints + 6) * sizeof(VoidCheckPoint);
    sp2C = gVoidCheckMaxPoints + 5;
    vtxLimit = (gVoidPrimLimit + 5) * 4 * sizeof(Vertex);
    triLimit = (gVoidPrimLimit + 5) * 2 * sizeof(Triangle);

    gVoidMesh = mempool_alloc_safe(viewportCount * sizeof(VoidMesh), COLOUR_TAG_CYAN);
    gVoidData = mempool_alloc_safe(sp30 + sp2C + (vtxLimit + triLimit) * 2 * viewportCount, COLOUR_TAG_CYAN);

    ptr = gVoidData;

    if (ptr != NULL) {
        gVoidCheckPoints = (VoidCheckPoint *) ptr;
        ptr += sp30;

        gVoidCheckPairs = (s8 *) ptr;
        // Align by 8
        ptr = (u8 *) ((uintptr_t) (ptr + sp2C + 8) & ~7);

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
    f32 temp_f22;
    Vertex *vtx;
    Triangle *tri;
    s16 temp_s3;
    s16 var_s0;
    s16 var_s4;
    s16 breakLoop;
    s32 *ptr2;
    LevelModelSegmentBoundingBox *bbox;
    s16 sum;
    s8 sp7C[24]; // possible UB here, real size is unknown
    s32 pad;

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

    temp_f22 = -(yCameraSins * gVoidCheckAheadX + yCameraCoss * gVoidCheckAheadZ);

    gVoidCheckSidePlaneX = -yCameraCoss;
    gVoidCheckSidePlaneZ = yCameraSins;
    gVoidCheckSidePlaneD = -(gVoidCheckSidePlaneX * gVoidCheckAheadX + gVoidCheckSidePlaneZ * gVoidCheckAheadZ);

    i = 0;
    for (; i < numberOfSegments; i++) {
        bbox = &gCurrentLevelModel->segmentsBoundingBoxes[segmentIds[i]];
        breakLoop = 0;
        breakLoop += bbox->x1 * yCameraSins + yCameraCoss * bbox->z1 + temp_f22 <= 0.0;
        breakLoop += yCameraSins * bbox->x2 + yCameraCoss * bbox->z1 + temp_f22 <= 0.0;
        breakLoop += bbox->x1 * yCameraSins + yCameraCoss * bbox->z2 + temp_f22 <= 0.0;
        breakLoop += yCameraSins * bbox->x2 + yCameraCoss * bbox->z2 + temp_f22 <= 0.0;
        if (breakLoop & 3) {
            void_check_add_segment_edges(&gCurrentLevelModel->segments[segmentIds[i]], yCameraSins, yCameraCoss,
                                         temp_f22);
            if (gCurrentLevelModel->segments[segmentIds[i]].segmentFlags & 2) {
                void_check_add_bbox_edges(bbox, yCameraSins, yCameraCoss, temp_f22);
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

    var_s0 = 0;

    for (i = 0; i < gVoidCheckPointCount; i++) {
        j = gVoidCheckPoints[i].pairIndex * 2;
        if (gVoidCheckPairs[j] == -1) {
            gVoidCheckPoints[i].flags |= 2;
            gVoidCheckPairs[j] = i;
        } else {
            gVoidCheckPairs[j + 1] = i;
        }
    }
    var_s4 = temp_s3 = gVoidCheckPoints[0].sideOffset;

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
        while (i < gVoidCheckPointCount && var_s4 == gVoidCheckPoints[i].sideOffset) {
            if (gVoidCheckPoints[i].flags & 2) {
                sp7C[var_s0] = gVoidCheckPoints[i].pairIndex;
                var_s0++;
            } else {
                for (j = 0; j < var_s0; j++) {
                    if (sp7C[j] == gVoidCheckPoints[i].pairIndex) {
                        var_s0--;
                        while (j < var_s0) {
                            sp7C[j] = sp7C[j + 1];
                            j++;
                        }
                    }
                }
            }
            i++;
        }
        if (i < gVoidCheckPointCount) {
            temp_s3 = gVoidCheckPoints[i].sideOffset;
            if (var_s4 != temp_s3) {
                void_check_sort_points(var_s0, sp7C, temp_s3, var_s4);
                var_s4 = temp_s3;
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
void void_check_add_bbox_edges(LevelModelSegmentBoundingBox *arg0, f32 arg1, f32 arg2, f32 arg3) {
    f32 sp80[4];
    f32 sp70[4];
    f32 sp60[4];
    f32 temp;
    f32 sp54[2];
    f32 sp4C[2];
    s16 index;
    s16 nextIndex;
    s16 sp40[4];
    s16 var_t0;
    s16 temp2;

    sp80[0] = arg0->x1;
    sp70[0] = arg0->z1;
    sp80[1] = arg0->x2;
    sp70[1] = arg0->z1;
    sp80[2] = arg0->x2;
    sp70[2] = arg0->z2;
    sp80[3] = arg0->x1;
    sp70[3] = arg0->z2;

    temp2 = 0;
    for (index = 0; index < 4; index++) {
        sp60[index] = (arg1 * sp80[index]) + (sp70[index] * arg2) + arg3;
        sp40[index] = sp60[index] <= 0.0;
        temp2 += sp40[index];
    }
    if (temp2 != 0) {
        var_t0 = 0;
        if (temp2 == 4) {
            return;
        }
        for (index = 0; index < 4; index++) {
            nextIndex = index + 1;
            if (nextIndex >= 4) {
                nextIndex = 0;
            }
            if ((sp40[nextIndex] != sp40[index]) != 0U) {
                temp = sp60[index] / (sp60[index] - sp60[nextIndex]);
                sp54[var_t0] = sp80[index] + ((sp80[nextIndex] - sp80[index]) * temp);
                sp4C[var_t0] = sp70[index] + ((sp70[nextIndex] - sp70[index]) * temp);
                var_t0++;
            }
        }
        if (var_t0 != 2) {
            return;
        }

        sp60[0] = (gVoidCheckSidePlaneX * sp54[0]) + (gVoidCheckSidePlaneZ * sp4C[0]) + gVoidCheckSidePlaneD;
        sp60[1] = (gVoidCheckSidePlaneX * sp54[1]) + (gVoidCheckSidePlaneZ * sp4C[1]) + gVoidCheckSidePlaneD;

        if (sp60[1] < sp60[0]) {
            temp = sp60[0];
            sp60[0] = sp60[1];
            sp60[1] = temp;
        }

        // Returns must be on the same line.
        // clang-format off
        if (-300.0 > sp60[1]) { return; }
        if (sp60[0] > 300.0) { return; }
        // clang-format on

        if (sp60[0] < -300.0) {
            sp60[0] = -300.0f;
        }
        if (sp60[1] > 300.0) {
            sp60[1] = 300.0f;
        }
        void_check_insert_point(sp60[1], gCurrentLevelModel->lowerYBounds - 195, 0);
        void_check_insert_point(sp60[0], gCurrentLevelModel->lowerYBounds - 195, 0);
    }
}

/**
 * Void check: walks a segment's triangles and records where their edges cross
 * the camera plane.
 */
void void_check_add_segment_edges(LevelModelSegment *segment, f32 arg1, f32 arg2, f32 arg3) {
    s16 i;
    s16 index;
    s16 verticesOffset;
    s16 nextFaceOffset;
    s16 nextIndex;
    s16 currFaceOffset;
    s16 j;
    Vertex *vert;
    s8 spF8[3];
    f32 temp;
    s16 var_s0;
    s16 var_t0;
#ifdef AVOID_UB
    f32 spE8[3]; // This really should be size of 3, but something is keeping it from matching that way.
#else
    f32 spE8[2];
#endif
    f32 spDC[3];
    f32 spD0[3];
    f32 spC4[3];
    f32 spB8[3];
    f32 spB0[2];
    f32 spA8[2];
    f32 spA0[2];

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
            var_t0 = 0;
            for (index = 0; index < 3; index++) {
                vert = &(segment->triangles[j].verticesArray[index + 1] + verticesOffset)[segment->vertices];
                spE8[index] = vert->x;
                spDC[index] = vert->y;
                spD0[index] = vert->z;
                spC4[index] = (arg1 * spE8[index]) + (arg2 * spD0[index]) + arg3;

                spF8[index] = (spC4[index] <= 0.0);
                var_t0 += (spF8[index] <= 0.0);
            }
            if ((var_t0 == 1) || (var_t0 == 2)) {
                for (var_s0 = 0, index = 0; index < 3; index++) {
                    nextIndex = index + 1;
                    if (nextIndex >= 3) {
                        nextIndex = 0;
                    }
                    if ((spF8[nextIndex] != spF8[index]) != 0) {
                        temp = spC4[index] / (spC4[index] - spC4[nextIndex]);
                        spB0[var_s0] = spE8[index] + ((spE8[nextIndex] - spE8[index]) * temp);
                        spB8[var_s0] = spDC[index] + ((spDC[nextIndex] - spDC[index]) * temp);
                        spA0[var_s0] = spB8[var_s0];
                        spA8[var_s0] = spD0[index] + ((spD0[nextIndex] - spD0[index]) * temp);
                        var_s0++;
                    }
                }

                var_s0 = 0;
                spF8[0] = 0;
                spF8[1] = 0;
                spC4[0] = (gVoidCheckSidePlaneX * spB0[0]) + (gVoidCheckSidePlaneZ * spA8[0]) + gVoidCheckSidePlaneD;
                spC4[1] = (gVoidCheckSidePlaneX * spB0[1]) + (gVoidCheckSidePlaneZ * spA8[1]) + gVoidCheckSidePlaneD;
                if (spC4[0] < -300.0) {
                    spF8[0] = 1;
                }
                if (spC4[0] > 300.0) {
                    spF8[0] |= 2;
                }
                if (spC4[1] < -300.0) {
                    spF8[1] = 1;
                }
                if (spC4[1] > 300.0) {
                    spF8[1] |= 2;
                }
                // clang-format off
                if ((spF8[0] | spF8[1]) == 0) {  var_s0 = 1; }
                // clang-format on
                else if ((spF8[1] != spF8[0]) != 0) {
                    index = 0;
                    if (spC4[1] < spC4[0]) {
                        index = 1;
                    }
                    nextIndex = 1 - index;
                    if (spF8[index] == 1) {
                        temp = ((-spC4[index] - 300.0) / (spC4[nextIndex] - spC4[index]));
                        spB8[index] = spB8[index] + ((spB8[nextIndex] - spB8[index]) * temp);
                        spC4[index] = -300.0f;
                    }
                    if (spF8[nextIndex] == 2) {
                        temp = ((spC4[nextIndex] - 300.0) / (spC4[nextIndex] - spC4[index]));
                        spB8[nextIndex] = spB8[nextIndex] + ((spB8[index] - spB8[nextIndex]) * temp);
                        spC4[nextIndex] = 300.0f;
                    }
                    var_s0 = 1;
                }
                if (var_s0 != 0) {
                    var_t0 = (segment->collisionFacets[j].basePlaneIndex << 2);
                    temp = (spB0[0] + gVoidCheckSidePlaneX) * segment->collisionPlanes[var_t0];
                    temp += spB8[0] * segment->collisionPlanes[var_t0 + 1];
                    temp += (spA8[0] + gVoidCheckSidePlaneZ) * segment->collisionPlanes[var_t0 + 2];
                    temp += segment->collisionPlanes[var_t0 + 3];
                    var_s0 = (temp > 0.0) << 2;
                    if (segment->collisionPlanes[var_t0 + 1] < 0.0f) {
                        var_s0 |= 1;
                    }
                    if (spC4[0] == spC4[1]) {
                        var_s0 |= 8;
                    }
                    void_check_insert_point(spC4[0], spB8[0], var_s0);
                    void_check_insert_point(spC4[1], spB8[1], var_s0);
                }
            }
        }
    }
}

/**
 * Void check: inserts a crossing point into the sorted point list, pairing
 * points as it goes.
 */
void void_check_insert_point(s16 arg0, s16 arg1, s32 arg2) {
    s16 i;
    s16 j;

    if (gVoidCheckPointCount < gVoidCheckMaxPoints) {
        i = 0;
        while (i < gVoidCheckPointCount && gVoidCheckPoints[i].sideOffset < arg0) {
            i++;
        }
        while (i < gVoidCheckPointCount && arg0 == gVoidCheckPoints[i].sideOffset &&
               gVoidCheckPoints[i].height < arg1) {
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
        gVoidCheckPoints[i].sideOffset = arg0;
        gVoidCheckPoints[i].height = arg1;
        gVoidCheckPoints[i].unused4 = 0;
        gVoidCheckPoints[i].pairIndex = gVoidCheckPairIndex;
        gVoidCheckPoints[i].flags = arg2;
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
void void_check_sort_points(s16 arg0, s8 *arg1, f32 arg2, f32 arg3) {
    VoidCheckPoint *next;
    VoidCheckPoint *curr;
    s16 temp3;
    s16 temp4;
    f32 curr_unk0;
    f32 curr_unk2;
    f32 next_unk0;
    f32 next_unk2;
    s32 noSwap;
    s16 i;
    s16 j;
    f32 sp94[20];
    f32 sp6C[10];
    s8 sp60[10];
    s8 temp;
    s8 temp0;
    s8 temp1;
    s8 swapByte;

    if (arg0 >= 10 || arg0 == 0) {
        return;
    }

    for (j = 0, i = 0; i < arg0;) {
        temp = arg1[i];
        curr = &gVoidCheckPoints[gVoidCheckPairs[(s16) (temp * 2)]];
        next = &gVoidCheckPoints[gVoidCheckPairs[((s16) (temp * 2)) + 1]];
        curr_unk0 = (f32) curr->sideOffset;
        curr_unk2 = (f32) curr->height;
        next_unk0 = (f32) next->sideOffset;
        next_unk2 = (f32) next->height;
        if (curr_unk0 == next_unk0) {
            return;
        }
        sp94[j++] = ((next_unk2 - curr_unk2) * ((curr_unk0 - arg3) / ((curr_unk0 - next_unk0)))) + curr_unk2;
        sp6C[i] = sp94[j - 1];
        sp60[i] = i;
        sp94[j++] = ((curr_unk2 - next_unk2) * ((arg2 - next_unk0) / ((curr_unk0 - next_unk0)))) + next_unk2;
        sp6C[i] += sp94[j - 1];
        i++;
    }

    do {
        noSwap = TRUE;
        for (i = 0; i < arg0 - 1; i++) {
            if (sp6C[sp60[i + 1]] < sp6C[sp60[i]]) {
                swapByte = arg1[i];
                arg1[i] = arg1[i + 1];
                arg1[i + 1] = swapByte;
                swapByte = sp60[i];
                sp60[i] = sp60[i + 1];
                sp60[i + 1] = swapByte;
                noSwap = FALSE;
            }
        }
    } while (noSwap == FALSE);

    for (i = 0; i < arg0 - 1; i++) {
        temp0 = gVoidCheckPoints[gVoidCheckPairs[arg1[i] * 2]].flags & 1;
        temp1 = gVoidCheckPoints[gVoidCheckPairs[arg1[i + 1] * 2]].flags & 1;
        if (temp0 && !temp1) {
            temp3 = (sp60[i] * 2);
            temp4 = (sp60[i + 1] * 2);
            void_generate_primitive(&sp94[temp3], &sp94[temp4], arg3, arg2);
        }
    }
}

s32 void_generate_primitive(f32 *arg0, f32 *arg1, f32 arg2, f32 arg3) {
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

    vertX1 = arg2 * gVoidCheckSidePlaneX + gVoidCheckAheadX;
    vertZ1 = arg2 * gVoidCheckSidePlaneZ + gVoidCheckAheadZ;
    vertX2 = arg3 * gVoidCheckSidePlaneX + gVoidCheckAheadX;
    vertZ2 = arg3 * gVoidCheckSidePlaneZ + gVoidCheckAheadZ;

    colour_r = gVoidColourR;
    colour_g = gVoidColourG;
    colour_b = gVoidColourB;
    colour_a = 0xFF;

    verts = gTrackVtxPtr;
    verts[0].x = vertX1;
    verts[0].y = arg0[0] + 2.0f;
    verts[0].z = vertZ1;
    verts[0].r = colour_r;
    verts[0].g = colour_g;
    verts[0].b = colour_b;
    verts[0].a = colour_a;

    verts[1].x = vertX2;
    verts[1].y = arg0[1] + 2.0f;
    verts[1].z = vertZ2;
    verts[1].r = colour_r;
    verts[1].g = colour_g;
    verts[1].b = colour_b;
    verts[1].a = colour_a;

    verts[2].x = vertX1;
    verts[2].y = arg1[0] - 2.0f;
    verts[2].z = vertZ1;
    verts[2].r = colour_r;
    verts[2].g = colour_g;
    verts[2].b = colour_b;
    verts[2].a = colour_a;

    verts[3].x = vertX2;
    verts[3].y = arg1[1] - 2.0f;
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
