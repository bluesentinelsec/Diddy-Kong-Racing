#include "tracks_internal.h"

typedef struct unk8002D30C_a0 {
    u8 pad00[0x04];
    struct unk8002D30C_a0 *unk04;
    struct unk8002D30C_a0 *unk08;
} unk8002D30C_a0;

/**
 * Writes back the track collision data pointers.
 */
UNUSED void get_collision_candidate_data(s32 *numCollsionCandidates, ptrint_t **collisionCandidates,
                                         s8 **collisionSurfaces) {
    *numCollsionCandidates = gNumCollisionCandidates;
    *collisionCandidates = gCollisionCandidates;
    *collisionSurfaces = gCollisionSurfaces;
}

/**
 * Change the collision response method.
 * For instance, can choose to ignore wall response.
 */
void set_collision_mode(s32 mode) {
    gCollisionMode = mode;
}

/**
 * Returns the surface normals of the current collision point.
 */
s32 get_collision_normal(f32 *outX, f32 *outY, f32 *outZ) {
    *outX = gCollisionNormalX;
    *outY = gCollisionNormalY;
    *outZ = gCollisionNormalZ;
    return gHitWall;
}

/**
 * Iterates through active waves on the track, then saves its rotation vector and height if found.
 * Returns zero if no waves are found, or if too high up.
 * Two types exist: calm, which have no means of displacement, and wavy, which do.
 */
s32 get_wave_properties(f32 yPos, f32 *waterHeight, Vec3f *rotation) {
    s32 var_a0;
    WaterProperties **wave;
    s32 i;
    s32 index;
    s32 len;
    f32 height;

    len = gWaterSurfaceHitCount;
    if (rotation != NULL) {
        rotation->f[0] = 0.0f;
        rotation->f[2] = 0.0f;
        rotation->f[1] = 1.0f;
    }
    wave = gTrackWaves;
    for (var_a0 = i = 0; i < len; i++) {
        if (wave[i]->type == SURFACE_WATER_CALM || wave[i]->type == SURFACE_WATER_WAVY) {
            var_a0++;
        }
    }
    if (var_a0 == 0) {
        return SURFACE_DEFAULT;
    }
    wave = gTrackWaves;
    index = -1;
    for (i = 0; i < len; i++) {
        height = wave[i]->waveHeight;
        if (wave[i]->type == SURFACE_WATER_CALM || wave[i]->type == SURFACE_WATER_WAVY) {
            if (yPos < height + 25.0 && (wave[i]->rot.y > 0.5 || var_a0 == 1)) {
                index = i;
            }
        } else if (index >= 0 && var_a0 >= 2 && yPos < height - 20.0) {
            index = -1;
        }
    }
    if (index < 0) {
        return SURFACE_DEFAULT;
    }
    *waterHeight = gTrackWaves[index]->waveHeight;
    if (rotation != NULL) {
        rotation->f[0] = gTrackWaves[index]->rot.x;
        rotation->f[1] = gTrackWaves[index]->rot.y;
        rotation->f[2] = gTrackWaves[index]->rot.z;
    }
    return gTrackWaves[index]->type;
}

/**
 * Finds the waves in the current level segment, sorts them by height, outputs the sorted list into
 * waterProps and returns the number of waves.
 */
s32 get_level_segment_waves(s32 levelSegmentIndex, f32 xIn, f32 zIn, WaterProperties ***waterProps) {
    LevelModelSegmentBoundingBox *currentBoundingBox;
    Triangle *tri;
    Vertex *vert;
    s16 temp_a2;
    s32 currentVerticesOffset;
    LevelModelSegment *currentSegment;
    s32 j;
    s32 segmentCount;
    s16 vert1X;
    s16 vert1Z;
    s16 vert2X;
    s16 vert2Z;
    s16 vert3X;
    s16 vert3Z;
    s32 nextFaceOffset;
    s16 var_a1;
    s32 currentFaceOffset;
    s32 faceNum;
    s16 var_s1;
    s16 var_t0;
    s16 var_t1;
    s32 XInInt;
    s32 ZInInt;
    s32 side1;
    s32 side2;
    s32 side3;
    s32 segmentsInside[8];
    s32 yOutCount;
    s32 batchNum;
    s32 i;
    s32 stopSorting;
    f32 A, B, C, D;
    s32 temp;
    s32 surfaceType;
    s32 unused_bool;
    s32 pad;
    WaterProperties *wave;
    WaterProperties **waves;

    unused_bool = FALSE;
    gWaterSurfaceHitCount = 0;
    *waterProps = NULL;
    XInInt = xIn;
    ZInInt = zIn;
    segmentCount = get_inside_segment_count_xz(XInInt, ZInInt, segmentsInside);
    if (segmentCount == 0 || segmentCount >= 8) {
        return 0;
    }

    yOutCount = 0;
    for (i = 0; i < segmentCount; i++) {
        currentSegment = &gCurrentLevelModel->segments[segmentsInside[i]];
        currentBoundingBox = &gCurrentLevelModel->segmentsBoundingBoxes[segmentsInside[i]];

        var_a1 = 1;
        var_s1 = 0;

        temp_a2 = ((currentBoundingBox->x2 - currentBoundingBox->x1) >> 3) + 1;
        var_t0 = temp_a2 + currentBoundingBox->x1;
        var_t1 = currentBoundingBox->x1;

        for (j = 0; j < 8; j++) {
            if (var_t0 >= XInInt && XInInt >= var_t1) {
                var_s1 |= var_a1;
            }
            var_t0 += temp_a2;
            var_t1 += temp_a2;
            var_a1 <<= 1;
        }

        temp_a2 = ((currentBoundingBox->z2 - currentBoundingBox->z1) >> 3) + 1;
        var_t0 = temp_a2 + currentBoundingBox->z1;
        var_t1 = currentBoundingBox->z1;

        for (j = 0; j < 8; j++) {
            if (var_t0 >= ZInInt && ZInInt >= var_t1) {
                var_s1 |= var_a1;
            }
            var_t0 += temp_a2;
            var_t1 += temp_a2;
            var_a1 <<= 1;
        }

        for (batchNum = 0; batchNum < currentSegment->numberOfBatches; batchNum++) {
            surfaceType = gCurrentLevelModel->textures[currentSegment->batches[batchNum].textureIndex].surfaceType;
            currentFaceOffset = currentSegment->batches[batchNum].facesOffset;
            currentVerticesOffset = currentSegment->batches[batchNum].verticesOffset;
            nextFaceOffset = currentSegment->batches[batchNum + 1].facesOffset;
            if ((surfaceType != SURFACE_WATER_CALM && surfaceType != SURFACE_WATER_UNK_F) &&
                (currentSegment->batches[batchNum].flags & (RENDER_HIDDEN | RENDER_NO_COLLISION))) {
                currentFaceOffset = nextFaceOffset;
            }
            for (faceNum = currentFaceOffset; faceNum < nextFaceOffset; faceNum++) {
                if (var_s1 == (currentSegment->triangleGridMasks[faceNum] & var_s1)) {
                    tri = &currentSegment->triangles[faceNum];
                    vert = &currentSegment->vertices[tri->verticesArray[1] + currentVerticesOffset];
                    vert1X = vert->x;
                    vert1Z = vert->z;
                    vert = &currentSegment->vertices[tri->verticesArray[2] + currentVerticesOffset];
                    vert2X = vert->x;
                    vert2Z = vert->z;
                    vert = &currentSegment->vertices[tri->verticesArray[3] + currentVerticesOffset];
                    vert3X = vert->x;
                    vert3Z = vert->z;
                    side1 = (((XInInt - vert2X) * (vert3Z - vert2Z)) - ((vert3X - vert2X) * (ZInInt - vert2Z))) >= 0;
                    side2 = (((XInInt - vert1X) * (vert2Z - vert1Z)) - ((vert2X - vert1X) * (ZInInt - vert1Z))) >= 0;
                    side3 = (((XInInt - vert1X) * (vert3Z - vert1Z)) - ((vert3X - vert1X) * (ZInInt - vert1Z))) >= 0;
                    if (side1 == side2 && side2 != side3) {
                        temp = currentSegment->collisionFacets[faceNum].basePlaneIndex;
                        A = currentSegment->collisionPlanes[(temp * 4) + 0];
                        B = currentSegment->collisionPlanes[(temp * 4) + 1];
                        C = currentSegment->collisionPlanes[(temp * 4) + 2];
                        D = currentSegment->collisionPlanes[(temp * 4) + 3];
                        if (B != 0.0) {
                            gWaterSurfaceHits[yOutCount].type = surfaceType;
                            gWaterSurfaceHits[yOutCount].waveHeight = -((((A * xIn) + (C * zIn)) + D) / B);
                            gWaterSurfaceHits[yOutCount].rot.x = A;
                            gWaterSurfaceHits[yOutCount].rot.y = B;
                            gWaterSurfaceHits[yOutCount].rot.z = C;
                            yOutCount++;
                            if (yOutCount >= 20) {
                                batchNum = currentSegment->numberOfBatches;
                                faceNum = nextFaceOffset;
                                i = segmentCount;
                            } else if (unused_bool) {
                                // Fake to set to this to basically any variable in currentSegment as a break for
                                // optimization. This code should never run.
                                currentSegment->numberOfBatches = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    if (levelSegmentIndex >= 0 && levelSegmentIndex < gCurrentLevelModel->numberOfSegments) {
        currentSegment = &gCurrentLevelModel->segments[levelSegmentIndex];
        gWaterSurfaceHits[yOutCount].type = SURFACE_WATER_WAVY;
        if (currentSegment->hasWaves && gWaveBlockCount != 0) {
            wave = yOutCount + gWaterSurfaceHits; // fake?
            gWaterSurfaceHits[yOutCount].waveHeight = wave_height_at(levelSegmentIndex, xIn, zIn, &wave->rot);
        } else {
            gWaterSurfaceHits[yOutCount].waveHeight = currentSegment->waterHeight;
            gWaterSurfaceHits[yOutCount].rot.x = 0.0f;
            gWaterSurfaceHits[yOutCount].rot.y = 1.0f;
            gWaterSurfaceHits[yOutCount].rot.z = 0.0f;
        }
        yOutCount++;
    }

    waves = gTrackWaves;
    wave = gWaterSurfaceHits;

    // clang-format off
    for (j = 0; j < yOutCount; j++) { waves[j] = &wave[j]; }
    // clang-format on

    do {
        stopSorting = TRUE;

        for (j = 0; j < (yOutCount - 1); j++) {
            if (waves[j]->waveHeight < waves[j + 1]->waveHeight) {
                stopSorting = FALSE;
                wave = waves[j];
                waves[j] = waves[j + 1];
                waves[j + 1] = wave;
            }
        }

    } while (!stopSorting);

    *waterProps = gTrackWaves;
    gWaterSurfaceHitCount = yOutCount;

    return yOutCount;
}

/**
 * The water surface height under an object: the wave height when the
 * segment has waves and they are wanted, otherwise the segment's water level.
 * Also returns the surface normal, flat unless waves are consulted.
 */
s32 get_object_water_height(Object *obj, f32 *arg1, Vec3f *arg2, s32 arg3) {
    LevelModelSegment *seg;

    if (arg2 != NULL) {
        arg2->x = 0.0f;
        arg2->z = 0.0f;
        arg2->y = 1.0f;
    }
    if ((obj->segmentID < 0) || (obj->segmentID >= gCurrentLevelModel->numberOfSegments)) {
        return FALSE;
    }
    seg = &gCurrentLevelModel->segments[obj->segmentID];
    if ((seg->hasWaves != 0) && (gWaveBlockCount != 0) && (arg3 == 1)) {
        *arg1 = wave_height_at(obj->segmentID, obj->trans.x_position, obj->trans.z_position, arg2);
        return TRUE;
    } else {
        *arg1 = seg->waterHeight;
        return TRUE;
    }
}

/**
 * Searches for intersecting surfaces, then returns the Y values of all the intersecting points, in order.
 * Writes at most maxYOut values to yOut; surfaces beyond that are dropped.
 */
s32 collision_get_y(s32 levelSegmentIndex, f32 xIn, f32 zIn, f32 *yOut, s32 maxYOut) {
    LevelModelSegment *currentSegment;
    LevelModelSegmentBoundingBox *currentBoundingBox;
    Triangle *tri;
    Vertex *vert;
    f32 temp_f2_2;
    s16 vert2X;
    s16 vert2Z;
    s16 temp_a2;
    s16 vert3X;
    s16 vert3Z;
    s32 currentVerticesOffset;
    s16 nextFaceOffset;
    s16 vert1X;
    s16 currentFaceOffset;
    s16 vert1Z;
    s16 var_a1;
    s32 faceNum;
    s16 var_s1;
    s16 var_t0;
    s16 var_t1;
    s32 XInInt;
    s32 ZInInt;
    s32 temp_ra_1;
    s32 temp_ra_2;
    s32 temp_ra_3;
    s32 yOutCount;
    s32 batchNum;
    s32 i;
    s32 var_v0;
    s32 stopSorting;
    TriangleBatchInfo *currentBatch;
    f32 *temp_v1_4;
    Vec4f tempVec4f;
    u16 temp;

    if (levelSegmentIndex < 0 || levelSegmentIndex >= gCurrentLevelModel->numberOfSegments) {
        return 0;
    }

    vert = NULL; // fake?

    currentSegment = &gCurrentLevelModel->segments[levelSegmentIndex];
    currentBoundingBox = &gCurrentLevelModel->segmentsBoundingBoxes[levelSegmentIndex];
    var_a1 = 1;
    var_s1 = 0;
    XInInt = xIn;
    ZInInt = zIn;

    temp_a2 = ((currentBoundingBox->x2 - currentBoundingBox->x1) >> 3) + 1;
    var_t0 = temp_a2 + currentBoundingBox->x1;
    var_t1 = currentBoundingBox->x1;
    for (i = 0; i < 8; i++) {
        if (var_t0 >= XInInt && XInInt >= var_t1) {
            var_s1 |= var_a1;
        }
        var_t0 += temp_a2;
        var_t1 += temp_a2;
        var_a1 *= 2;
    }

    // Same as above, but for Z
    temp_a2 = ((currentBoundingBox->z2 - currentBoundingBox->z1) >> 3) + 1;
    var_t0 = temp_a2 + currentBoundingBox->z1;
    var_t1 = currentBoundingBox->z1;
    for (i = 0; i < 8; i++) {
        if (var_t0 >= ZInInt && ZInInt >= var_t1) {
            var_s1 |= var_a1;
        }
        var_t0 += temp_a2;
        var_t1 += temp_a2;
        var_a1 *= 2;
    }

    yOutCount = 0;
    for (batchNum = 0; batchNum < currentSegment->numberOfBatches; batchNum++) {
        do {
        } while (0); // fakematch
        currentFaceOffset = currentSegment->batches[batchNum].facesOffset;
        nextFaceOffset = currentSegment->batches[batchNum + 1].facesOffset;
        currentVerticesOffset = currentSegment->batches[batchNum].verticesOffset;
        for (faceNum = currentFaceOffset; faceNum < nextFaceOffset; faceNum++) {
            if (var_s1 == (currentSegment->triangleGridMasks[faceNum] & var_s1)) {
                tri = &currentSegment->triangles[faceNum];

                vert = &currentSegment->vertices[tri->verticesArray[1] + currentVerticesOffset];
                vert1X = vert->x;
                vert1Z = vert->z;

                vert = &currentSegment->vertices[tri->verticesArray[2] + currentVerticesOffset];
                vert2X = vert->x;
                vert2Z = vert->z;

                vert = &currentSegment->vertices[tri->verticesArray[3] + currentVerticesOffset];
                vert3X = vert->x;
                vert3Z = vert->z;

                temp_ra_1 = (((XInInt - vert2X) * (vert3Z - vert2Z)) - ((vert3X - vert2X) * (ZInInt - vert2Z))) >= 0;
                temp_ra_2 = (((XInInt - vert1X) * (vert2Z - vert1Z)) - ((vert2X - vert1X) * (ZInInt - vert1Z))) >= 0;
                temp_ra_3 = (((XInInt - vert1X) * (vert3Z - vert1Z)) - ((vert3X - vert1X) * (ZInInt - vert1Z))) >= 0;
                var_v0 = faceNum; // fake?
                if (temp_ra_1 == temp_ra_2 && temp_ra_2 != temp_ra_3) {
                    temp = currentSegment->collisionFacets[faceNum].basePlaneIndex;
                    tempVec4f.x = currentSegment->collisionPlanes[4 * temp + 0];
                    tempVec4f.y = currentSegment->collisionPlanes[4 * temp + 1];
                    tempVec4f.z = currentSegment->collisionPlanes[4 * temp + 2];
                    tempVec4f.w = currentSegment->collisionPlanes[4 * temp + 3];
                    if (tempVec4f.y != 0.0 && yOutCount < maxYOut) {
                        yOut[yOutCount] = -(((tempVec4f.x * xIn) + (tempVec4f.z * zIn) + tempVec4f.w) / tempVec4f.y);
                        yOutCount++;
                    }
                }
            }
        }
    }

    do {
        stopSorting = TRUE;
        for (var_v0 = 0; var_v0 < yOutCount - 1; var_v0++) {
            if (yOut[var_v0] > yOut[var_v0 + 1]) {
                stopSorting = FALSE;
                temp_f2_2 = yOut[var_v0];
                yOut[var_v0] = yOut[var_v0 + 1];
                yOut[var_v0 + 1] = temp_f2_2;
            }
        }
    } while (!stopSorting);

    return yOutCount;
}

/**
 * For each triangle in a segment, which cells of the segment's 8x8 grid it
 * touches, as the mask compute_grid_overlap_mask produces for a rectangle.
 */
void segment_compute_triangle_grid_masks(LevelModelSegment *segment, LevelModelSegmentBoundingBox *bbox,
                                         UNUSED s32 arg2) {
    Vertex *vert;
    s16 boxDelta;
    s32 vertZ;
    s32 vertX;
    s16 boxMin;
    s16 bit;
    s16 maxX;
    s32 j;
    s16 minX;
    s16 val;
    s16 maxZ;
    s16 minZ;
    s16 k;
    s16 boxMax;
    s32 i;
    s32 l;
    s32 startTri;
    s32 endTri;
    s32 vertsOffset;

    for (i = 0; i < segment->numberOfBatches; i++) {
        startTri = segment->batches[i].facesOffset;
        endTri = segment->batches[i + 1].facesOffset;
        vertsOffset = segment->batches[i].verticesOffset;
        for (j = startTri; j < endTri; j++) {
            if (segment->triangles[j].flags & TRI_FLAG_80) {
                segment->triangleGridMasks[j] = 0;
            } else {
                maxX = -32000;
                maxZ = -32000;
                minZ = 32000;
                minX = 32000;

                for (l = 0; l < 3; l++) {
                    vert = &segment->vertices[segment->triangles[j].verticesArray[l + 1] + vertsOffset];
                    k = vert->x;
                    vertX = k; // This is probably fake, but it matches.
                    vertZ = vert->z;
                    if (maxX < vertX) {
                        maxX = vertX;
                    }
                    if (vertX < minX) {
                        minX = vertX;
                    }
                    if (maxZ < vertZ) {
                        maxZ = vertZ;
                    }
                    if (vertZ < minZ) {
                        minZ = vertZ;
                    }
                }
                boxDelta = ((bbox->x2 - bbox->x1) >> 3) + 1;
                bit = 1;
                boxMax = boxDelta + bbox->x1;
                boxMin = bbox->x1;
                val = 0;
                for (k = 0; k < 8; k++) {
                    if (boxMax >= minX && maxX >= boxMin) {
                        val |= bit;
                    }
                    boxMax += boxDelta;
                    boxMin += boxDelta;
                    bit <<= 1;
                }
                boxDelta = ((bbox->z2 - bbox->z1) >> 3) + 1;
                boxMax = boxDelta + bbox->z1;
                boxMin = bbox->z1;
                for (k = 0; k < 8; k++) {
                    if (boxMax >= minZ && maxZ >= boxMin) {
                        val |= bit;
                    }
                    boxMax += boxDelta;
                    boxMin += boxDelta;
                    bit <<= 1;
                }
                segment->triangleGridMasks[j] = val;
            }
        }
    }
}

s32 track_init_collision(LevelModelSegment *block) {
    s32 facesOffset;
    s32 verticesOffset;
    s32 nextFacesOffset;
    Vertex *v;
    s32 batchIndex;
    s32 triIndex;
    s32 i;
    f32 mag;
    s32 counter;
    s32 colPlaneIndex;
    s32 next;
    s32 numColPlanes;
    s32 vertIndex, nextVertIndex;
    f32 x1, y1, z1;
    f32 x2, y2, z2;
    f32 x3, y3, z3;
    f32 nx, ny, nz;
    f32 x5, y5, z5;
    s32 j;

    counter = 0;

    for (batchIndex = 0; batchIndex < block->numberOfBatches; batchIndex++) {
        facesOffset = block->batches[batchIndex].facesOffset;
        verticesOffset = block->batches[batchIndex].verticesOffset;
        nextFacesOffset = block->batches[batchIndex + 1].facesOffset;

        for (triIndex = facesOffset; triIndex < nextFacesOffset; triIndex++) {
            if (block->triangles[triIndex].flags & TRI_FLAG_80) {
                continue;
            }

            v = &block->vertices[block->triangles[triIndex].vi0 + verticesOffset];
            x1 = v->x;
            y1 = v->y;
            z1 = v->z;

            v = &block->vertices[block->triangles[triIndex].vi1 + verticesOffset];
            x2 = v->x;
            y2 = v->y;
            z2 = v->z;

            v = &block->vertices[block->triangles[triIndex].vi2 + verticesOffset];
            x3 = v->x;
            y3 = v->y;
            z3 = v->z;

            nx = y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2);
            ny = z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2);
            nz = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);

            mag = sqrtf(nx * nx + ny * ny + nz * nz);
            if (mag > 0.0) {
                nx /= mag;
                ny /= mag;
                nz /= mag;
            }

            // looks like a macro
            {
                s32 temp = counter++;
                block->collisionPlanes[4 * temp + 0] = nx;
                block->collisionPlanes[4 * temp + 1] = ny;
                block->collisionPlanes[4 * temp + 2] = nz;
                block->collisionPlanes[4 * temp + 3] = -(x1 * nx + y1 * ny + z1 * nz);
            }
        }
    }

    numColPlanes = counter;

    if (gLevelIsCutscene) {
        return counter * 0x10;
    }

    for (batchIndex = 0; batchIndex < block->numberOfBatches; batchIndex++) {
        facesOffset = block->batches[batchIndex].facesOffset;
        verticesOffset = block->batches[batchIndex].verticesOffset;
        nextFacesOffset = block->batches[batchIndex + 1].facesOffset;

        if (block->batches[batchIndex].flags & RENDER_NO_COLLISION) {
            facesOffset = nextFacesOffset;
        }

        for (triIndex = facesOffset; triIndex < nextFacesOffset; triIndex++) {
            if (block->triangles[triIndex].flags & TRI_FLAG_80) {
                continue;
            }

            colPlaneIndex = block->collisionFacets[triIndex].basePlaneIndex;
            nx = block->collisionPlanes[4 * colPlaneIndex + 0];
            ny = block->collisionPlanes[4 * colPlaneIndex + 1];
            nz = block->collisionPlanes[4 * colPlaneIndex + 2];

            for (i = 0; i < 3; i++) {
                next = i + 1;
                if (next > 2) {
                    next = 0;
                }

                vertIndex = block->triangles[triIndex].verticesArray[1 + i] + verticesOffset;
                nextVertIndex = block->triangles[triIndex].verticesArray[1 + next] + verticesOffset;

                next = block->collisionFacets[triIndex].edgeBisectorPlane[i];
                if (next < numColPlanes) {
                    {
                        s32 temp = next;
                        x5 = nx + block->collisionPlanes[4 * temp + 0];
                        y5 = ny + block->collisionPlanes[4 * temp + 1];
                        z5 = nz + block->collisionPlanes[4 * temp + 2];
                    }

                    v = &block->vertices[vertIndex];
                    x1 = v->x;
                    y1 = v->y;
                    z1 = v->z;

                    v = &block->vertices[nextVertIndex];
                    x2 = v->x;
                    y2 = v->y;
                    z2 = v->z;

                    x3 = x5 * 10.0f + x1;
                    y3 = y5 * 10.0f + y1;
                    z3 = z5 * 10.0f + z1;

                    x5 = y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2);
                    y5 = z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2);
                    z5 = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);

                    mag = sqrtf(x5 * x5 + y5 * y5 + z5 * z5);
                    if (mag > 0.0) {
                        x5 /= mag;
                        y5 /= mag;
                        z5 /= mag;
                    }

                    if (next != colPlaneIndex) {
                        for (j = 0; j < 3; j++) {
                            if (colPlaneIndex == block->collisionFacets[next].edgeBisectorPlane[j]) {
                                block->collisionFacets[next].edgeBisectorPlane[j] = counter | 0x8000;
                            }
                        }
                    }

                    block->collisionFacets[triIndex].edgeBisectorPlane[i] = counter;

                    {
                        s32 temp = counter++;
                        block->collisionPlanes[4 * temp + 0] = x5;
                        block->collisionPlanes[4 * temp + 1] = y5;
                        block->collisionPlanes[4 * temp + 2] = z5;
                        block->collisionPlanes[4 * temp + 3] = -(x1 * x5 + y1 * y5 + z1 * z5);
                    }
                }
            }
        }
    }
    return counter * 0x10;
}

void trackMakeAbsolute(unk8002D30C_a0 *arg0, s32 arg1) {
    while (1) {
        if (!arg0) {
            return;
        }
        if (arg0->unk04) {
            arg0->unk04 = (unk8002D30C_a0 *) ((ptrint_t) arg0->unk04 + arg1);
        }
        if (arg0->unk08) {
            arg0->unk08 = (unk8002D30C_a0 *) ((ptrint_t) arg0->unk08 + arg1);
        }

        trackMakeAbsolute(arg0->unk04, arg1);
        arg0 = arg0->unk08;
    }
}
