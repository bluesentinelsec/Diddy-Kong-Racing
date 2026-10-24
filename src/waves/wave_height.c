#include "waves_internal.h"

/** Working state shared by the steps of wave_height_at. */
typedef struct WaveHeightQuery {
    s32 blockID;
    f32 stepX;           // Width of one wave cell.
    f32 stepZ;           // Depth of one wave cell.
    f32 detailScaleStep; // How much each depth step below WAVE_FULL_SWELL_DEPTH adds to the swell.
    s32 detailBase;      // Index of the segment's first entry in gWaveDetailLevels.
    s32 detailRowStride; // Entries per row of the segment in gWaveDetailLevels.
    f32 x;               // Position within the cell.
    f32 z;
    s32 cellX; // Cell column and row within the segment.
    s32 cellZ;
    s32 tileX; // The same cell's column and row in the repeating wave tile grid.
    s32 tileZ;
    f32 anchorX; // X of the triangle corner the plane is measured from, and its height.
    f32 anchorY;
    f32 normalX; // Unnormalised surface normal of the triangle under the point.
    f32 normalY;
    f32 normalZ;
} WaveHeightQuery;

/**
 * Clamps the point into the segment and finds the wave cell it lies in, both within the segment and in the
 * repeating wave tile grid.
 */
static void wave_height_find_cell(WaveHeightQuery *q, f32 x, f32 z) {
    x -= gWaveModel[q->blockID].originX;
    if (x < 0.0f) {
        x = 0.0f;
    } else if (gWaveBoundingBoxW <= x) {
        x = gWaveBoundingBoxW - 1; // needs to be 1 instead of 1.0f
    }

    z -= gWaveModel[q->blockID].originZ;
    if (z < 0.0f) {
        z = 0.0f;
    } else if (gWaveBoundingBoxH <= z) {
        z = gWaveBoundingBoxH - 1; // needs to be 1 instead of 1.0f
    }

    q->cellX = x / q->stepX;
    q->cellZ = z / q->stepZ;
    x -= q->cellX * q->stepX;
    z -= q->cellZ * q->stepZ;

    q->tileX = gWaveModel[q->blockID].waveTileOffsetX + q->cellX;
    while (q->tileX >= gWaveController.tileCount) {
        q->tileX -= gWaveController.tileCount;
    }

    q->tileZ = gWaveModel[q->blockID].waveTileOffsetZ + q->cellZ;
    while (q->tileZ >= gWaveController.tileCount) {
        q->tileZ -= gWaveController.tileCount;
    }

    q->x = x;
    q->z = z;
}

/**
 * The point lies in the cell's first triangle (corners top-left, bottom-left and top-right):
 * samples the three corner heights and builds the triangle's normal.
 */
static void wave_height_first_triangle(WaveHeightQuery *q) {
    f32 cornerY0;
    f32 cornerY1;
    f32 cornerY2;
    s32 index;
    s32 detail;

    index = (q->tileZ * gWaveController.tileCount) + q->tileX;
    cornerY0 = (gWaveHeightTable[gWaveHeightIndices[index].s[0]] + gWaveHeightTable[gWaveHeightIndices[index].s[1]]) *
               gWaveController.magnitude;
    index = q->detailBase + q->cellX + (q->cellZ * q->detailRowStride);
    if (gWaveGenCount > 0) {
        cornerY0 += waves_get_y(q->blockID, q->cellX, q->cellZ);
    }

    detail = gWaveDetailLevels[index];
    if (gWaveDetailLevels[index] < WAVE_FULL_SWELL_DEPTH) {
        cornerY0 *= gWaveController.minDetailScale + (detail * q->detailScaleStep);
    }

    index = (q->tileZ + 1) >= gWaveController.tileCount ? q->tileX
                                                        : ((q->tileZ + 1) * gWaveController.tileCount) + q->tileX;
    cornerY1 = (gWaveHeightTable[gWaveHeightIndices[index].s[0]] + gWaveHeightTable[gWaveHeightIndices[index].s[1]]) *
               gWaveController.magnitude;
    if (gWaveGenCount > 0) {
        cornerY1 += waves_get_y(q->blockID, q->cellX, q->cellZ + 1);
    }
    index = q->detailBase + q->cellX + ((q->cellZ + 1) * q->detailRowStride);
    detail = gWaveDetailLevels[index];
    if (gWaveDetailLevels[index] < WAVE_FULL_SWELL_DEPTH) {
        cornerY1 *= gWaveController.minDetailScale + (detail * q->detailScaleStep);
    }

    if ((q->tileX + 1) >= gWaveController.tileCount) {
        index = q->tileZ * gWaveController.tileCount;
    } else {
        index = (q->tileX + 0) + (q->tileZ * gWaveController.tileCount) + 1;
    }
    cornerY2 = (gWaveHeightTable[gWaveHeightIndices[index].s[0]] + gWaveHeightTable[gWaveHeightIndices[index].s[1]]) *
               gWaveController.magnitude;
    if (gWaveGenCount > 0) {
        cornerY2 += waves_get_y(q->blockID, q->cellX + 1, q->cellZ);
    }
    index = q->detailBase + q->cellX + (q->cellZ * q->detailRowStride);
    index++;
    detail = gWaveDetailLevels[index];
    if (gWaveDetailLevels[index] < WAVE_FULL_SWELL_DEPTH) {
        cornerY2 *= gWaveController.minDetailScale + (detail * q->detailScaleStep);
    }

    if (gWaveController.doubleDensity) {
        cornerY0 /= 2.0f;
        cornerY1 /= 2.0f;
        cornerY2 /= 2.0f;
    }

    q->anchorY = cornerY0;
    q->anchorX = 0.0f;
    q->normalX = (cornerY0 - cornerY2) * q->stepZ;
    q->normalY = q->stepZ * q->stepX;
    q->normalZ = (cornerY0 - cornerY1) * q->stepX;
}

/**
 * The point lies in the cell's second triangle (corners top-right, bottom-left and bottom-right):
 * samples the three corner heights and builds the triangle's normal.
 */
static void wave_height_second_triangle(WaveHeightQuery *q) {
    f32 cornerY0;
    f32 cornerY1;
    f32 cornerY2;
    s32 index;
    s32 detail;

    if ((q->tileX + 1) >= gWaveController.tileCount) {
        index = q->tileZ * gWaveController.tileCount;
    } else {
        index = (q->tileX + 0) + (q->tileZ * gWaveController.tileCount) + 1;
    }
    cornerY0 = (gWaveHeightTable[gWaveHeightIndices[index].s[0]] + gWaveHeightTable[gWaveHeightIndices[index].s[1]]) *
               gWaveController.magnitude;
    if (gWaveGenCount > 0) {
        cornerY0 += waves_get_y(q->blockID, q->cellX + 1, q->cellZ);
    }
    index = q->detailBase + q->cellX + (q->cellZ * q->detailRowStride);
    index++;
    detail = gWaveDetailLevels[index];
    if (gWaveDetailLevels[index] < WAVE_FULL_SWELL_DEPTH) {
        cornerY0 *= gWaveController.minDetailScale + (detail * q->detailScaleStep);
    }

    index = (q->tileZ + 1) >= gWaveController.tileCount ? q->tileX
                                                        : ((q->tileZ + 1) * gWaveController.tileCount) + q->tileX;
    cornerY1 = (gWaveHeightTable[gWaveHeightIndices[index].s[0]] + gWaveHeightTable[gWaveHeightIndices[index].s[1]]) *
               gWaveController.magnitude;
    if (gWaveGenCount > 0) {
        cornerY1 += waves_get_y(q->blockID, q->cellX, q->cellZ + 1);
    }
    index = q->detailBase + q->cellX + ((q->cellZ + 1) * q->detailRowStride);
    detail = gWaveDetailLevels[index];
    if (detail < WAVE_FULL_SWELL_DEPTH) {
        cornerY1 *= gWaveController.minDetailScale + (detail * q->detailScaleStep);
    }

    index = (q->tileX + 1) >= gWaveController.tileCount ? 0 : (q->tileX + 1);
    if ((q->tileZ + 1) < gWaveController.tileCount) {
        index += (q->tileZ + 1) * gWaveController.tileCount;
    }

    cornerY2 = (gWaveHeightTable[gWaveHeightIndices[index].s[0]] + gWaveHeightTable[gWaveHeightIndices[index].s[1]]) *
               gWaveController.magnitude;
    if (gWaveGenCount > 0) {
        cornerY2 += waves_get_y(q->blockID, q->cellX + 1, q->cellZ + 1);
    }
    index = q->detailBase + q->cellX + ((q->cellZ + 1) * q->detailRowStride);
    index++;
    detail = gWaveDetailLevels[index];
    if (detail < WAVE_FULL_SWELL_DEPTH) {
        cornerY2 *= gWaveController.minDetailScale + (detail * q->detailScaleStep);
    }

    if (gWaveController.doubleDensity) {
        cornerY0 /= 2.0f;
        cornerY1 /= 2.0f;
        cornerY2 /= 2.0f;
    }

    q->anchorY = cornerY0;
    q->anchorX = q->stepX;
    q->normalX = q->stepZ * (cornerY1 - cornerY2);
    q->normalY = q->stepZ * q->stepX;
    q->normalZ = q->stepX * (cornerY0 - cornerY2);
}

/**
 * The wave surface height and normal at a point of a segment.
 * Each wave cell is split into two triangles; the height is read off the plane of the one under the point.
 */
f32 wave_height_at(s32 blockID, f32 x, f32 z, Vec3f *outNormal) {
    WaveHeightQuery q;
    f32 surfaceY;
    f32 normalLength;
    s32 firstTriangle;

    if (blockID < 0 || blockID >= gNumberOfLevelSegments) {
        blockID = 0;
    }
    q.blockID = blockID;

    surfaceY = gWaveModel[blockID].originY;

    q.stepZ = gWaveVtxStepZ;
    q.stepX = gWaveVtxStepX;
    q.detailScaleStep = (1.0f - gWaveController.minDetailScale) / 127.0f;
    q.detailBase = blockID * gWaveTileTotal;

    if (gWaveController.doubleDensity) {
        q.stepZ /= 2.0f;
        q.stepX /= 2.0f;
        q.detailRowStride = (gWaveController.subdivisions * 2) + 1;
    } else {
        q.detailRowStride = gWaveController.subdivisions + 1;
    }

    wave_height_find_cell(&q, x, z);

    firstTriangle = 0;
    if (q.z != q.stepZ && q.x < (((q.stepZ - q.z) / q.stepZ) * q.stepX)) {
        firstTriangle = 1;
    }

    if (firstTriangle) {
        wave_height_first_triangle(&q);
    } else {
        wave_height_second_triangle(&q);
    }

    normalLength = sqrtf((q.normalX * q.normalX) + (q.normalY * q.normalY) + (q.normalZ * q.normalZ));
    if (normalLength != 0.0 && q.normalY != 0.0) {
        q.normalX /= normalLength;
        q.normalY /= normalLength;
        q.normalZ /= normalLength;
        surfaceY -=
            (((q.normalX * q.x) + (q.normalZ * q.z)) - ((q.anchorX * q.normalX) + (q.anchorY * q.normalY))) / q.normalY;
    }

    if (outNormal != NULL) {
        outNormal->x = q.normalX;
        outNormal->y = q.normalY;
        outNormal->z = q.normalZ;
    }

    return surfaceY;
}

/** The block of wave cells that wave_cells_in_rect covers, and how to sample it. */
typedef struct WaveRect {
    s32 blockID;
    s32 startCol; // First and last cell column and row of the block within the segment.
    s32 startRow;
    s32 endCol;
    s32 endRow;
    s32 colCount;  // Cells per row of the segment; later, vertices per row of the block.
    s32 colOffset; // The block's first column and row in the repeating wave tile grid.
    s32 rowOffset;
    s32 detailBase; // Index of the segment's first entry in gWaveDetailLevels.
    f32 stepX;      // Width and depth of one wave cell.
    f32 stepZ;
    f32 detailScaleStep; // How much each depth step below WAVE_FULL_SWELL_DEPTH adds to the swell.
    f32 heightScale;
} WaveRect;

/** Samples the height of every wave vertex of the block, row by row, into heights. */
static void wave_rect_sample_heights(WaveRect *r, s16 *heights) {
    s32 currentRow;
    s32 currentCol;
    s32 counter;
    s32 tileCol;
    s32 heightIndex;
    s32 detailIndex;
    f32 height;

    for (currentRow = r->startRow, counter = 0; currentRow <= r->endRow; currentRow++) {
        tileCol = r->colOffset;

        heightIndex = (r->rowOffset * gWaveController.tileCount) + tileCol;
        detailIndex = r->detailBase + r->startCol + (currentRow * r->colCount);
        for (currentCol = r->startCol; currentCol <= r->endCol; currentCol++) {

            height = (gWaveHeightTable[gWaveHeightIndices[heightIndex].x] +
                      gWaveHeightTable[gWaveHeightIndices[heightIndex].y]) *
                     gWaveController.magnitude;
            if (gWaveGenCount > 0) {
                height += waves_get_y(r->blockID, currentCol, currentRow);
            }

            if (gWaveDetailLevels[detailIndex] < WAVE_FULL_SWELL_DEPTH) {
                height *= gWaveController.minDetailScale + ((s32) gWaveDetailLevels[detailIndex] * r->detailScaleStep);
            }
            heights[counter] = gWaveModel[r->blockID].originY + (s16) (height * r->heightScale);
            counter++;
            tileCol++;
            detailIndex++;
            heightIndex++;
            if (tileCol >= gWaveController.tileCount) {
                tileCol -= gWaveController.tileCount;
                heightIndex -= gWaveController.tileCount;
            }
        }
        r->rowOffset++;
        if (r->rowOffset >= gWaveController.tileCount) {
            r->rowOffset -= gWaveController.tileCount;
        }
    }
}

/**
 * Writes two triangles per cell of the block, with their planes, skipping any whose normal comes out zero.
 * Returns how many were written.
 */
static s32 wave_rect_build_tris(WaveRect *r, s16 *heights, ShadowWaveTri *tris, ShadowPlane *planes) {
    s32 currentRow;
    s32 currentCol;
    s32 counter;
    f32 normalX;
    f32 normalZ;
    f32 normalY;
    f32 normalLength;
    s16 rowZ;
    s16 nextRowZ;
    s16 nextColX;
    s16 colX;

    // FAKE
    normalY = (r->stepX * r->stepZ) * (r->stepX * r->stepZ);
    r->colCount = (r->endCol - r->startCol) + 1;
    rowZ = gWaveModel[r->blockID].originZ + (s16) (r->startRow * r->stepZ);

    for (currentRow = r->startRow, counter = 0; currentRow < r->endRow; currentRow++) {
        nextRowZ = gWaveModel[r->blockID].originZ + (s16) ((currentRow + 1) * r->stepZ);
        colX = gWaveModel[r->blockID].originX + (s16) (r->startCol * r->stepX);

        for (currentCol = r->startCol; currentCol < r->endCol; currentCol++) {
            nextColX = gWaveModel[r->blockID].originX + (s16) ((currentCol + 1) * r->stepX);

            tris[counter].x1 = colX;
            tris[counter].x2 = colX;
            tris[counter].x3 = nextColX;
            tris[counter].y1 = heights[(currentCol - r->startCol) + ((currentRow - r->startRow) * r->colCount)];
            tris[counter].y2 = heights[(currentCol - r->startCol) + (((currentRow - r->startRow) + 1) * r->colCount)];
            tris[counter].y3 = heights[((currentCol - r->startCol) + 1) + ((currentRow - r->startRow) * r->colCount)];
            tris[counter].z1 = rowZ;
            tris[counter].z2 = nextRowZ;
            tris[counter].z3 = rowZ;
            normalX = (tris[counter].y1 - tris[counter].y3) * r->stepZ;
            normalY = r->stepX * r->stepZ;
            normalZ = (tris[counter].y1 - tris[counter].y2) * r->stepX;
            normalLength = sqrtf((normalX * normalX) + (normalY * normalY) + (normalZ * normalZ));
            if (normalLength > 0.0f) {
                normalLength = 1 / normalLength;
                planes[counter].x = normalX * normalLength;
                planes[counter].y = normalY * normalLength;
                planes[counter].z = normalZ * normalLength;

                planes[counter].planeD.w =
                    -((tris[counter].x1 * planes[counter].x) + (tris[counter].y1 * planes[counter].y) +
                      (tris[counter].z1 * planes[counter].z));
                counter++;
            }
            tris[counter].x1 = nextColX;
            tris[counter].x2 = colX;
            tris[counter].x3 = nextColX;
            tris[counter].y1 = heights[((currentCol - r->startCol) + 1) + ((currentRow - r->startRow) * r->colCount)];
            tris[counter].y2 = heights[(currentCol - r->startCol) + (((currentRow - r->startRow) + 1) * r->colCount)];
            tris[counter].y3 =
                heights[((currentCol - r->startCol) + 1) + (((currentRow - r->startRow) + 1) * r->colCount)];
            tris[counter].z1 = rowZ;
            tris[counter].z2 = nextRowZ;
            tris[counter].z3 = nextRowZ;
            normalX = (tris[counter].y2 - tris[counter].y3) * r->stepZ;
            normalY = r->stepX * r->stepZ;
            normalZ = (tris[counter].y1 - tris[counter].y3) * r->stepX;
            normalLength = sqrtf((normalX * normalX) + (normalY * normalY) + (normalZ * normalZ));
            if (normalLength > 0.0f) {
                normalLength = 1 / normalLength;
                planes[counter].x = normalX * normalLength;
                planes[counter].y = normalY * normalLength;
                planes[counter].z = normalZ * normalLength;

                planes[counter].planeD.w =
                    -((tris[counter].x1 * planes[counter].x) + (tris[counter].y1 * planes[counter].y) +
                      (tris[counter].z1 * planes[counter].z));
                counter++;
            }

            colX = nextColX;
        }
        rowZ = nextRowZ;
    }
    return counter;
}

/**
 * Builds the wave triangles (and their planes) of a segment that cover a rectangle, for shadows cast on water.
 * Returns how many were written.
 */
s32 wave_cells_in_rect(s32 blockID, ShadowWaveTri *tris, ShadowPlane *planes, f32 shadowXNegPosition,
                       f32 shadowZNegPosition, f32 shadowXPosition, f32 shadowZPosition) {
    WaveRect r;
    s16 heights[300];

    r.blockID = blockID;
    r.detailBase = blockID * gWaveTileTotal;
    r.stepX = gWaveVtxStepX;
    r.stepZ = gWaveVtxStepZ;
    r.detailScaleStep = (1.0f - gWaveController.minDetailScale) / 127.0f;
    if (gWaveController.doubleDensity != 0) {
        r.heightScale = 0.5f;
        r.stepX *= 0.5;
        r.stepZ *= 0.5;
        r.colCount = gWaveController.subdivisions * 2;
    } else {
        r.heightScale = 1.0f;
        r.colCount = gWaveController.subdivisions;
    }

    shadowXNegPosition -= gWaveModel[blockID].originX;
    shadowZNegPosition -= gWaveModel[blockID].originZ;
    shadowXPosition -= gWaveModel[blockID].originX;
    shadowZPosition -= gWaveModel[blockID].originZ;
    if (shadowXNegPosition < 0.0f) {
        r.startCol = 0;
    } else {
        r.startCol = (s32) (shadowXNegPosition / r.stepX);
    }
    if (shadowZNegPosition < 0.0f) {
        r.startRow = 0;
    } else {
        r.startRow = (s32) (shadowZNegPosition / r.stepZ);
    }
    if (gWaveBoundingBoxW <= shadowXPosition) {
        r.endCol = r.colCount;
    } else {
        r.endCol = (s32) (shadowXPosition / r.stepX) + 1;
    }
    if (gWaveBoundingBoxH <= shadowZPosition) {
        r.endRow = r.colCount;
    } else {
        r.endRow = (s32) (shadowZPosition / r.stepX) + 1;
    }
    r.colOffset = gWaveModel[blockID].waveTileOffsetX + r.startCol;
    while (r.colOffset >= gWaveController.tileCount) {
        r.colOffset -= gWaveController.tileCount;
    }
    r.rowOffset = gWaveModel[blockID].waveTileOffsetZ + r.startRow;
    while (r.rowOffset >= gWaveController.tileCount) {
        r.rowOffset -= gWaveController.tileCount;
    }

    wave_rect_sample_heights(&r, heights);
    return wave_rect_build_tris(&r, heights, tris, planes);
}

/** Working state shared by the steps of obj_wave_init. */
typedef struct WaveLogCell {
    f32 stepX; // Width and depth of one wave cell.
    f32 stepZ;
    f32 detailScaleStep; // How much each depth step below WAVE_FULL_SWELL_DEPTH adds to the swell.
    s32 detailBase;      // Index of the segment's first entry in gWaveDetailLevels.
    s32 detailRowStride; // Entries per row of the segment in gWaveDetailLevels.
    f32 x;               // Position within the cell.
    f32 z;
    s32 cellX; // Cell column and row within the segment.
    s32 cellZ;
    s32 tileX; // The same cell's column and row in the repeating wave tile grid.
    s32 tileZ;
    s32 firstTriangle;    // Whether the point lies in the cell's first triangle rather than its second.
    f32 anchorX;          // X of the triangle corner the plane is measured from.
    s32 cornerTiles[3];   // The triangle's three corners in the wave tile grid...
    s32 detailIndices[3]; // ...and in gWaveDetailLevels.
    s32 seedIndices[6];   // Two gWaveHeightTable entries per corner.
    f32 cornerScale[3];   // Each corner's depth damping.
} WaveLogCell;

/**
 * Clamps the point into the segment, finds the wave cell under it and which of the cell's two triangles
 * it lies in. Records the cell on the log.
 */
static void wave_log_find_cell(WaveLogCell *cell, Object_Log *object, s32 blockID, f32 x, f32 z) {
    // clang-format off
    x -= gWaveModel[blockID].originX;
    if (x < 0.0f) { x = 0.0f; }
    else if (x >= gWaveBoundingBoxW) { x = gWaveBoundingBoxW - 1; }
    z -= gWaveModel[blockID].originZ;
    if (z < 0.0f) { z = 0.0f; }
    else if (z >= gWaveBoundingBoxH) { z = gWaveBoundingBoxH - 1; }
    // clang-format on
    cell->cellX = x / cell->stepX;
    cell->cellZ = z / cell->stepZ;
    object->tileX = cell->cellX;
    object->tileZ = cell->cellZ;
    object->blockID = blockID;
    x -= (cell->cellX * cell->stepX);
    z -= (cell->cellZ * cell->stepZ);
    cell->tileX = gWaveModel[blockID].waveTileOffsetX + cell->cellX;
    while (cell->tileX >= gWaveController.tileCount) {
        cell->tileX -= gWaveController.tileCount;
    }

    cell->tileZ = gWaveModel[blockID].waveTileOffsetZ + cell->cellZ;
    while (cell->tileZ >= gWaveController.tileCount) {
        cell->tileZ -= gWaveController.tileCount;
    }

    cell->firstTriangle = FALSE;
    if (z != cell->stepZ && x < (((cell->stepZ - z) / cell->stepZ) * cell->stepX)) {
        cell->firstTriangle = TRUE;
    }
    cell->x = x;
    cell->z = z;
}

/** Finds the three corners of the triangle under the point, in the wave tile grid and in the depth table. */
static void wave_log_corner_indices(WaveLogCell *cell) {
    if (cell->firstTriangle != FALSE) {
        cell->cornerTiles[0] = (cell->tileZ * gWaveController.tileCount) + cell->tileX;

        if ((cell->tileZ + 1) >= gWaveController.tileCount) {
            cell->cornerTiles[1] = cell->tileX;
        } else {
            cell->cornerTiles[1] = ((cell->tileZ + 1) * gWaveController.tileCount) + cell->tileX;
        }

        if ((cell->tileX + 1) >= gWaveController.tileCount) {
            cell->cornerTiles[2] = (cell->tileZ * gWaveController.tileCount);
        } else {
            cell->cornerTiles[2] = (cell->tileZ * gWaveController.tileCount) + cell->tileX + 1;
        }

        cell->anchorX = 0.0f;
        cell->detailIndices[0] = (cell->detailBase + cell->cellX) + (cell->cellZ * cell->detailRowStride);
        cell->detailIndices[1] = (cell->detailBase + cell->cellX) + ((cell->cellZ + 1) * cell->detailRowStride);
        cell->detailIndices[2] = (cell->detailBase + cell->cellX) + (cell->cellZ * cell->detailRowStride) + 1;
    } else {
        if ((cell->tileX + 1) >= gWaveController.tileCount) {
            cell->cornerTiles[0] = cell->tileZ * gWaveController.tileCount;
        } else {
            cell->cornerTiles[0] = cell->tileX + 1 + (cell->tileZ * gWaveController.tileCount);
        }

        if ((cell->tileZ + 1) >= gWaveController.tileCount) {
            cell->cornerTiles[1] = cell->tileX;
        } else {
            cell->cornerTiles[1] = ((cell->tileZ + 1) * gWaveController.tileCount) + cell->tileX;
        }

        if ((cell->tileX + 1) >= gWaveController.tileCount) {
            cell->cornerTiles[2] = 0;
        } else {
            cell->cornerTiles[2] = cell->tileX + 1;
        }

        if ((cell->tileZ + 1) < gWaveController.tileCount) {
            cell->cornerTiles[2] += (cell->tileZ + 1) * gWaveController.tileCount;
        }

        cell->anchorX = cell->stepX;
        cell->detailIndices[0] = (cell->cellX + cell->detailBase) + (cell->cellZ * cell->detailRowStride) + 1;
        cell->detailIndices[1] = (cell->cellX + cell->detailBase) + (cell->cellZ + 1) * cell->detailRowStride;
        cell->detailIndices[2] = (cell->cellX + cell->detailBase) + (cell->cellZ + 1) * cell->detailRowStride + 1;
    }
}

/** Reads each corner's current height table entries and its depth damping. */
static void wave_log_corner_seeds(WaveLogCell *cell) {
    s16 i;
    s32 detail;

    for (i = 0; i < 3; i++) {
        cell->seedIndices[i * 2] = gWaveHeightIndices[cell->cornerTiles[i]].s[0];
        cell->seedIndices[i * 2 + 1] = gWaveHeightIndices[cell->cornerTiles[i]].s[1];
        cell->cornerScale[i] = 1.0f;

        detail = gWaveDetailLevels[cell->detailIndices[i]];
        if (detail < WAVE_FULL_SWELL_DEPTH) {
            cell->cornerScale[i] *= gWaveController.minDetailScale + (detail * cell->detailScaleStep);
        }
    }

    if (gWaveController.doubleDensity) {
        cell->cornerScale[0] /= 2.0;
        cell->cornerScale[1] /= 2.0;
        cell->cornerScale[2] /= 2.0;
    }
}

/**
 * Bakes the log's bobbing cycle: the surface height under it at every other step of the height table,
 * in 1/16 units. Samples are shifted right as far as needed to fit, and the shift is kept on the log.
 */
static void wave_log_bake_samples(WaveLogCell *cell, Object_Log *object) {
    f32 cornerY0;
    f32 cornerY1;
    f32 cornerY2;
    f32 normalX;
    f32 normalY;
    f32 normalZ;
    f32 normalYUnscaled;
    f32 normalLength;
    s32 sample;
    s16 sampleLimit;
    s16 extraShift;
    s16 i;
    s16 j;

    sampleLimit = 128;
    for (i = 0; i < (gWaveController.seedSize >> 1); i++) {
        cornerY0 =
            (gWaveHeightTable[cell->seedIndices[0]] + gWaveHeightTable[cell->seedIndices[1]]) * cell->cornerScale[0];
        cornerY1 =
            (gWaveHeightTable[cell->seedIndices[2]] + gWaveHeightTable[cell->seedIndices[3]]) * cell->cornerScale[1];
        cornerY2 =
            (gWaveHeightTable[cell->seedIndices[4]] + gWaveHeightTable[cell->seedIndices[5]]) * cell->cornerScale[2];
        if (cell->firstTriangle != FALSE) {
            normalX = (cornerY0 - cornerY2) * cell->stepZ;
            normalZ = (cornerY0 - cornerY1) * cell->stepX;
        } else {
            normalX = (cornerY1 - cornerY2) * cell->stepZ;
            normalZ = (cornerY0 - cornerY2) * cell->stepX;
        }
        normalYUnscaled = cell->stepZ * cell->stepX;
        normalLength = sqrtf((normalX * normalX) + (normalYUnscaled * normalYUnscaled) + (normalZ * normalZ));
        normalX /= normalLength;
        normalZ /= normalLength;
        normalY = normalYUnscaled / normalLength;
        sample =
            -(s32) (((normalX * cell->x) + (normalZ * cell->z) - ((cell->anchorX * normalX) + (cornerY0 * normalY))) *
                    16.0f / normalY);
        if (sample >= sampleLimit || sample < -sampleLimit) {
            extraShift = 0;
            do {
                sampleLimit += sampleLimit;
                extraShift++;
            } while (sample >= sampleLimit || sample < -sampleLimit);

            for (j = 0; j < i; j++) {
                object->heightSamples[j] >>= extraShift;
            }
            object->sampleShift += extraShift;
        }
        object->heightSamples[i] = (sample >> object->sampleShift);
        for (j = 0; j < 6; j++) {
            cell->seedIndices[j] += 2;
            while (cell->seedIndices[j] >= gWaveController.seedSize) {
                cell->seedIndices[j] -= gWaveController.seedSize;
            }
        }
    }
}

/**
 * Creates the bobbing state for a log floating on the waves of a segment at the given point.
 * Returns NULL if the level has no waves or the segment is out of range.
 */
Object_Log *obj_wave_init(s32 blockID, f32 x, f32 z) {
    // Zeroed: cornerTiles was bisected as read before it is written on some path, which indexes
    // gWaveHeightIndices with garbage and blacks out Future Fun Land on DC.
    WaveLogCell cell = { 0 };
    Object_Log *object;

    object = NULL;
    if (gWaveHeightTable != NULL && blockID >= 0 && blockID < gNumberOfLevelSegments) {
        object = mempool_alloc_safe((gWaveController.seedSize >> 1) + (sizeof(Object_Log) - 2), COLOUR_TAG_CYAN);
        object->baseHeight = gWaveModel[blockID].originY;
        object->sampleShift = 0;
        object->unused3 = 1;
        object->phase = 0;
        object->phaseCount = gWaveController.seedSize;
        cell.stepZ = gWaveVtxStepZ;
        cell.stepX = gWaveVtxStepX;
        cell.detailScaleStep = (1.0f - gWaveController.minDetailScale) / 127.0f;
        cell.detailBase = (blockID * gWaveTileTotal);
        if (gWaveController.doubleDensity) {
            cell.detailRowStride = (gWaveController.subdivisions * 2) + 1;
            cell.stepZ /= 2.0f;
            cell.stepX /= 2.0f;
        } else {
            cell.detailRowStride = gWaveController.subdivisions + 1;
        }
        wave_log_find_cell(&cell, object, blockID, x, z);
        wave_log_corner_indices(&cell);
        wave_log_corner_seeds(&cell);
        wave_log_bake_samples(&cell, object);
    }

    return object;
}

/**
 * Finds the wave height and returns it for the given object.
 */
f32 obj_wave_height(Object_Log *log, s32 updateRate) {
    s32 sample;
    f32 y;

    log->phase += updateRate;
    while (log->phase >= log->phaseCount) {
        log->phase -= log->phaseCount;
    }
    sample = log->heightSamples[log->phase >> 1];
    if (log->phase & 1) {
        if (log->phase + 1 >= log->phaseCount) {
            sample += log->heightSamples[0];
        } else {
            sample += log->heightSamples[(log->phase >> 1) + 1];
        }
        if (log->sampleShift > 0) {
            sample <<= (log->sampleShift - 1);
        } else {
            sample >>= 1;
        }
    } else {
        sample <<= log->sampleShift;
    }
    y = (((f32) sample / 16.0) + (f32) log->baseHeight);
    y *= gWaveController.magnitude;
    y += waves_get_y(log->blockID, log->tileX, log->tileZ);
    return y;
}

/**
 * The extra wave height that the wave generators (swells) add at a vertex of a segment's wave grid.
 */
f32 waves_get_y(s32 blockID, s32 cellX, s32 cellZ) {
    f32 dist;
    f32 distSq;
    f32 diffX;
    f32 diffZ;
    f32 worldZ;
    f32 zeroCheck;
    f32 worldX;
    f32 stepX;
    f32 y;
    f32 stepZ;
    u32 angle;
    s32 gridX;
    s32 i;
    s32 subdivisions;
    s32 gridZ;
    WaveGenTileSlots *slots;
    WaveGen *gen;

    y = 0.0f;
    if (gWaveGenCount <= 0) {
        return y;
    }

    subdivisions = gWaveController.subdivisions;
    stepX = gWaveVtxStepX;
    stepZ = gWaveVtxStepZ;
    if (gWaveController.doubleDensity) {
        subdivisions *= 2;
        stepX /= 2.0f;
        stepZ /= 2.0f;
    }
    gridX = (gWaveModel[blockID].tileX * subdivisions) + cellX;
    slots = &gWaveGenSlots[gridX];
    if (slots->genIndices[0] != WAVE_GEN_SLOT_EMPTY) {
        gridZ = cellZ + (subdivisions * gWaveModel[blockID].tileZ);
        worldX = gWaveBlockPosX + (gridX * stepX);
        worldZ = gWaveBlockPosZ + (gridZ * stepZ);
        zeroCheck = 0;
        i = 0;
        do {
            gen = &gWaveGenList[slots->genIndices[i]];
            if (gen->minZ <= worldZ && worldZ <= gen->maxZ) {
                diffX = worldX - gen->x_position;
                diffZ = worldZ - gen->z_position;
                distSq = (diffX * diffX) + (diffZ * diffZ);
                if (distSq < gen->radiusSq) {
                    dist = sqrtf(distSq);
                    angle = gen->phase;
                    if (gen->isLinearX) {
                        if (diffX < zeroCheck) {
                            angle -= (s32) (diffX * gen->phasePerUnit);
                        } else {
                            angle += (s32) (diffX * gen->phasePerUnit);
                        }
                    } else if (gen->isLinearZ) {
                        if (diffX < zeroCheck) {
                            angle -= (s32) (diffZ * gen->phasePerUnit);
                        } else {
                            angle += (s32) (diffZ * gen->phasePerUnit);
                        }
                    } else {
                        angle += (s32) (dist * gen->phasePerUnit);
                    }
                    diffX = coss_f((dist * 65536.0f) / gen->radius);
                    y += gen->amplitude * sins_f(angle) * diffX;
                }
            }
            i++;
        } while (i < WAVE_GEN_SLOTS && slots->genIndices[i] != WAVE_GEN_SLOT_EMPTY);
    }

    return y;
}
