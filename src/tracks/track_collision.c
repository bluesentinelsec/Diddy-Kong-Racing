#include "tracks_internal.h"

// A binary tree node whose child links are stored as offsets until trackMakeAbsolute rebases them.
typedef struct TrackTreeNode {
    u8 pad00[0x04];
    struct TrackTreeNode *left;
    struct TrackTreeNode *right;
} TrackTreeNode;

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
    s32 waterCount;
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
    for (waterCount = i = 0; i < len; i++) {
        if (wave[i]->type == SURFACE_WATER_CALM || wave[i]->type == SURFACE_WATER_WAVY) {
            waterCount++;
        }
    }
    if (waterCount == 0) {
        return SURFACE_DEFAULT;
    }
    wave = gTrackWaves;
    index = -1;
    for (i = 0; i < len; i++) {
        height = wave[i]->waveHeight;
        if (wave[i]->type == SURFACE_WATER_CALM || wave[i]->type == SURFACE_WATER_WAVY) {
            if (yPos < height + 25.0 && (wave[i]->rot.y > 0.5 || waterCount == 1)) {
                index = i;
            }
        } else if (index >= 0 && waterCount >= 2 && yPos < height - 20.0) {
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
 * Which cells of a segment's 8x8 grid hold the point: one bit per X column in the low byte and
 * one bit per Z row in the high byte, the layout segment_compute_triangle_grid_masks stores.
 */
static s16 segment_point_grid_mask(LevelModelSegmentBoundingBox *bbox, s32 x, s32 z) {
    s16 cellSize;
    s16 bit;
    s16 gridMask;
    s16 cellMax;
    s16 cellMin;
    s32 j;

    bit = 1;
    gridMask = 0;

    cellSize = ((bbox->x2 - bbox->x1) >> 3) + 1;
    cellMax = cellSize + bbox->x1;
    cellMin = bbox->x1;

    for (j = 0; j < 8; j++) {
        if (cellMax >= x && x >= cellMin) {
            gridMask |= bit;
        }
        cellMax += cellSize;
        cellMin += cellSize;
        bit <<= 1;
    }

    cellSize = ((bbox->z2 - bbox->z1) >> 3) + 1;
    cellMax = cellSize + bbox->z1;
    cellMin = bbox->z1;

    for (j = 0; j < 8; j++) {
        if (cellMax >= z && z >= cellMin) {
            gridMask |= bit;
        }
        cellMax += cellSize;
        cellMin += cellSize;
        bit <<= 1;
    }
    return gridMask;
}

/**
 * Appends the water surface of the given segment, if it is valid, as water hit hitCount:
 * the wave height when the segment has active waves, otherwise its flat water level.
 * Returns the new hit count.
 */
static s32 add_segment_water_surface(s32 levelSegmentIndex, f32 xIn, f32 zIn, s32 hitCount) {
    LevelModelSegment *currentSegment;
    WaterProperties *wave;

    if (levelSegmentIndex >= 0 && levelSegmentIndex < gCurrentLevelModel->numberOfSegments) {
        currentSegment = &gCurrentLevelModel->segments[levelSegmentIndex];
        gWaterSurfaceHits[hitCount].type = SURFACE_WATER_WAVY;
        if (currentSegment->hasWaves && gWaveBlockCount != 0) {
            wave = hitCount + gWaterSurfaceHits; // fake?
            gWaterSurfaceHits[hitCount].waveHeight = wave_height_at(levelSegmentIndex, xIn, zIn, &wave->rot);
        } else {
            gWaterSurfaceHits[hitCount].waveHeight = currentSegment->waterHeight;
            gWaterSurfaceHits[hitCount].rot.x = 0.0f;
            gWaterSurfaceHits[hitCount].rot.y = 1.0f;
            gWaterSurfaceHits[hitCount].rot.z = 0.0f;
        }
        hitCount++;
    }
    return hitCount;
}

/**
 * Points gTrackWaves at the first hitCount water hits, sorted with the highest surface first.
 */
static void sort_water_hits_by_height(s32 hitCount) {
    s32 j;
    s32 stopSorting;
    WaterProperties *wave;
    WaterProperties **waves;

    waves = gTrackWaves;
    wave = gWaterSurfaceHits;

    // clang-format off
    for (j = 0; j < hitCount; j++) { waves[j] = &wave[j]; }
    // clang-format on

    do {
        stopSorting = TRUE;

        for (j = 0; j < (hitCount - 1); j++) {
            if (waves[j]->waveHeight < waves[j + 1]->waveHeight) {
                stopSorting = FALSE;
                wave = waves[j];
                waves[j] = waves[j + 1];
                waves[j + 1] = wave;
            }
        }

    } while (!stopSorting);
}

/**
 * Finds the waves in the current level segment, sorts them by height, outputs the sorted list into
 * waterProps and returns the number of waves.
 */
s32 get_level_segment_waves(s32 levelSegmentIndex, f32 xIn, f32 zIn, WaterProperties ***waterProps) {
    LevelModelSegmentBoundingBox *currentBoundingBox;
    Triangle *tri;
    Vertex *vert;
    s32 currentVerticesOffset;
    LevelModelSegment *currentSegment;
    s32 segmentCount;
    s16 vert1X;
    s16 vert1Z;
    s16 vert2X;
    s16 vert2Z;
    s16 vert3X;
    s16 vert3Z;
    s32 nextFaceOffset;
    s32 currentFaceOffset;
    s32 faceNum;
    s16 gridMask;
    s32 XInInt;
    s32 ZInInt;
    s32 side1;
    s32 side2;
    s32 side3;
    s32 segmentsInside[8];
    s32 yOutCount;
    s32 batchNum;
    s32 i;
    f32 A, B, C, D;
    s32 planeIndex;
    s32 surfaceType;
    s32 unused_bool;

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
        gridMask = segment_point_grid_mask(currentBoundingBox, XInInt, ZInInt);
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
                if (gridMask == (currentSegment->triangleGridMasks[faceNum] & gridMask)) {
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
                        planeIndex = currentSegment->collisionFacets[faceNum].basePlaneIndex;
                        A = currentSegment->collisionPlanes[(planeIndex * 4) + 0];
                        B = currentSegment->collisionPlanes[(planeIndex * 4) + 1];
                        C = currentSegment->collisionPlanes[(planeIndex * 4) + 2];
                        D = currentSegment->collisionPlanes[(planeIndex * 4) + 3];
                        if (B != 0.0) {
                            gWaterSurfaceHits[yOutCount].type = surfaceType;
                            gWaterSurfaceHits[yOutCount].waveHeight = -((((A * xIn) + (C * zIn)) + D) / B);
                            gWaterSurfaceHits[yOutCount].rot.x = A;
                            gWaterSurfaceHits[yOutCount].rot.y = B;
                            gWaterSurfaceHits[yOutCount].rot.z = C;
                            yOutCount++;
                            if (yOutCount >= ARRAY_COUNT(gWaterSurfaceHits)) {
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

    yOutCount = add_segment_water_surface(levelSegmentIndex, xIn, zIn, yOutCount);
    sort_water_hits_by_height(yOutCount);

    *waterProps = gTrackWaves;
    gWaterSurfaceHitCount = yOutCount;

    return yOutCount;
}

/**
 * The water surface height under an object: the wave height when the
 * segment has waves and they are wanted, otherwise the segment's water level.
 * Also returns the surface normal, flat unless waves are consulted.
 */
s32 get_object_water_height(Object *obj, f32 *waterHeight, Vec3f *normal, s32 useWaves) {
    LevelModelSegment *seg;

    if (normal != NULL) {
        normal->x = 0.0f;
        normal->z = 0.0f;
        normal->y = 1.0f;
    }
    if ((obj->segmentID < 0) || (obj->segmentID >= gCurrentLevelModel->numberOfSegments)) {
        return FALSE;
    }
    seg = &gCurrentLevelModel->segments[obj->segmentID];
    if ((seg->hasWaves != 0) && (gWaveBlockCount != 0) && (useWaves == 1)) {
        *waterHeight = wave_height_at(obj->segmentID, obj->trans.x_position, obj->trans.z_position, normal);
        return TRUE;
    } else {
        *waterHeight = seg->waterHeight;
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
    f32 swapY;
    s16 vert2X;
    s16 vert2Z;
    s16 cellSize;
    s16 vert3X;
    s16 vert3Z;
    s32 currentVerticesOffset;
    s16 nextFaceOffset;
    s16 vert1X;
    s16 currentFaceOffset;
    s16 vert1Z;
    s16 bit;
    s32 faceNum;
    s16 gridMask;
    s16 cellMax;
    s16 cellMin;
    s32 XInInt;
    s32 ZInInt;
    s32 side1;
    s32 side2;
    s32 side3;
    s32 yOutCount;
    s32 batchNum;
    s32 i;
    s32 j;
    s32 stopSorting;
    Vec4f plane;
    u16 planeIndex;

    if (levelSegmentIndex < 0 || levelSegmentIndex >= gCurrentLevelModel->numberOfSegments) {
        return 0;
    }

    vert = NULL; // fake?

    currentSegment = &gCurrentLevelModel->segments[levelSegmentIndex];
    currentBoundingBox = &gCurrentLevelModel->segmentsBoundingBoxes[levelSegmentIndex];
    bit = 1;
    gridMask = 0;
    XInInt = xIn;
    ZInInt = zIn;

    cellSize = ((currentBoundingBox->x2 - currentBoundingBox->x1) >> 3) + 1;
    cellMax = cellSize + currentBoundingBox->x1;
    cellMin = currentBoundingBox->x1;
    for (i = 0; i < 8; i++) {
        if (cellMax >= XInInt && XInInt >= cellMin) {
            gridMask |= bit;
        }
        cellMax += cellSize;
        cellMin += cellSize;
        bit *= 2;
    }

    // Same as above, but for Z
    cellSize = ((currentBoundingBox->z2 - currentBoundingBox->z1) >> 3) + 1;
    cellMax = cellSize + currentBoundingBox->z1;
    cellMin = currentBoundingBox->z1;
    for (i = 0; i < 8; i++) {
        if (cellMax >= ZInInt && ZInInt >= cellMin) {
            gridMask |= bit;
        }
        cellMax += cellSize;
        cellMin += cellSize;
        bit *= 2;
    }

    yOutCount = 0;
    for (batchNum = 0; batchNum < currentSegment->numberOfBatches; batchNum++) {
        do {
        } while (0); // fakematch
        currentFaceOffset = currentSegment->batches[batchNum].facesOffset;
        nextFaceOffset = currentSegment->batches[batchNum + 1].facesOffset;
        currentVerticesOffset = currentSegment->batches[batchNum].verticesOffset;
        for (faceNum = currentFaceOffset; faceNum < nextFaceOffset; faceNum++) {
            if (gridMask == (currentSegment->triangleGridMasks[faceNum] & gridMask)) {
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
                j = faceNum; // fake?
                if (side1 == side2 && side2 != side3) {
                    planeIndex = currentSegment->collisionFacets[faceNum].basePlaneIndex;
                    plane.x = currentSegment->collisionPlanes[4 * planeIndex + 0];
                    plane.y = currentSegment->collisionPlanes[4 * planeIndex + 1];
                    plane.z = currentSegment->collisionPlanes[4 * planeIndex + 2];
                    plane.w = currentSegment->collisionPlanes[4 * planeIndex + 3];
                    if (plane.y != 0.0 && yOutCount < maxYOut) {
                        yOut[yOutCount] = -(((plane.x * xIn) + (plane.z * zIn) + plane.w) / plane.y);
                        yOutCount++;
                    }
                }
            }
        }
    }

    do {
        stopSorting = TRUE;
        for (j = 0; j < yOutCount - 1; j++) {
            if (yOut[j] > yOut[j + 1]) {
                stopSorting = FALSE;
                swapY = yOut[j];
                yOut[j] = yOut[j + 1];
                yOut[j + 1] = swapY;
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
                                         UNUSED s32 segmentIndex) {
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

/**
 * Stores the plane of a triangle (its unit normal and distance) as collision plane planeIndex.
 */
static void track_store_face_plane(LevelModelSegment *block, s32 triIndex, s32 verticesOffset, s32 planeIndex) {
    Vertex *v;
    f32 mag;
    f32 x1, y1, z1;
    f32 x2, y2, z2;
    f32 x3, y3, z3;
    f32 nx, ny, nz;

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

    block->collisionPlanes[4 * planeIndex + 0] = nx;
    block->collisionPlanes[4 * planeIndex + 1] = ny;
    block->collisionPlanes[4 * planeIndex + 2] = nz;
    block->collisionPlanes[4 * planeIndex + 3] = -(x1 * nx + y1 * ny + z1 * nz);
}

/**
 * Builds the plane through a triangle edge that bisects the angle between the triangle's plane
 * (normal nx, ny, nz; index colPlaneIndex) and the neighbouring plane `neighbour`, stores it as
 * collision plane newPlane and points both triangles' edge entries at it (the neighbour's flagged
 * with EDGE_PLANE_FLIPPED, since it sees the plane from the other side).
 */
static void track_add_edge_plane(LevelModelSegment *block, s32 triIndex, s32 edge, s32 vertIndex, s32 nextVertIndex,
                                 s32 neighbour, s32 colPlaneIndex, f32 nx, f32 ny, f32 nz, s32 newPlane) {
    Vertex *v;
    f32 mag;
    f32 x1, y1, z1;
    f32 x2, y2, z2;
    f32 x3, y3, z3;
    f32 x5, y5, z5;
    s32 j;

    x5 = nx + block->collisionPlanes[4 * neighbour + 0];
    y5 = ny + block->collisionPlanes[4 * neighbour + 1];
    z5 = nz + block->collisionPlanes[4 * neighbour + 2];

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

    if (neighbour != colPlaneIndex) {
        for (j = 0; j < 3; j++) {
            if (colPlaneIndex == block->collisionFacets[neighbour].edgeBisectorPlane[j]) {
                block->collisionFacets[neighbour].edgeBisectorPlane[j] = newPlane | EDGE_PLANE_FLIPPED;
            }
        }
    }

    block->collisionFacets[triIndex].edgeBisectorPlane[edge] = newPlane;

    block->collisionPlanes[4 * newPlane + 0] = x5;
    block->collisionPlanes[4 * newPlane + 1] = y5;
    block->collisionPlanes[4 * newPlane + 2] = z5;
    block->collisionPlanes[4 * newPlane + 3] = -(x1 * x5 + y1 * y5 + z1 * z5);
}

/**
 * Builds a segment's collision planes: one per triangle, then (outside cutscenes) one per shared
 * edge. Returns the size in bytes they take.
 */
s32 track_init_collision(LevelModelSegment *block) {
    s32 facesOffset;
    s32 verticesOffset;
    s32 nextFacesOffset;
    s32 batchIndex;
    s32 triIndex;
    s32 i;
    s32 counter;
    s32 colPlaneIndex;
    s32 next;
    s32 numColPlanes;
    s32 vertIndex, nextVertIndex;
    f32 nx, ny, nz;

    counter = 0;

    for (batchIndex = 0; batchIndex < block->numberOfBatches; batchIndex++) {
        facesOffset = block->batches[batchIndex].facesOffset;
        verticesOffset = block->batches[batchIndex].verticesOffset;
        nextFacesOffset = block->batches[batchIndex + 1].facesOffset;

        for (triIndex = facesOffset; triIndex < nextFacesOffset; triIndex++) {
            if (block->triangles[triIndex].flags & TRI_FLAG_80) {
                continue;
            }

            track_store_face_plane(block, triIndex, verticesOffset, counter++);
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
                    track_add_edge_plane(block, triIndex, i, vertIndex, nextVertIndex, next, colPlaneIndex, nx, ny, nz,
                                         counter);
                    counter++;
                }
            }
        }
    }
    return counter * 0x10;
}

/**
 * Turns a tree's offset child links into pointers by adding offset to each, recursing down the
 * left links and iterating down the right ones.
 */
void trackMakeAbsolute(TrackTreeNode *node, s32 offset) {
    while (1) {
        if (!node) {
            return;
        }
        if (node->left) {
            node->left = (TrackTreeNode *) ((ptrint_t) node->left + offset);
        }
        if (node->right) {
            node->right = (TrackTreeNode *) ((ptrint_t) node->right + offset);
        }

        trackMakeAbsolute(node->left, offset);
        node = node->right;
    }
}
