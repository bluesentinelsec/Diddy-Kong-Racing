#include "waves_internal.h"

/** Sets up the water surfaces of a level: the water texture, the wave segments and their collision. */
void waves_init_level(LevelModel *level, LevelHeader *header) {
    waves_find_water_texture(level, header);
    waves_init_segments(level);
}

// determines current bounding box, batch and texture
void waves_find_water_texture(LevelModel *level, LevelHeader *header) {
    s16 numSegments;
    s32 j;
    TriangleBatchInfo *curBatch;
    s32 i;
    s32 colourID;
    LevelModelSegmentBoundingBox *bb;
    LevelModelSegment *segment;

    numSegments = level->numberOfSegments;
    curBatch = 0;

    for (i = 0; curBatch == 0 && i < numSegments; i++) {
        segment = &level->segments[i];
        for (j = 0; curBatch == 0 && j < segment->numberOfBatches; j++) {
            if ((segment->batches[j].flags & (RENDER_UNK_1000000 | RENDER_WATER | RENDER_HIDDEN)) ==
                (RENDER_UNK_1000000 | RENDER_WATER)) {
                curBatch = &segment->batches[j];
            }
        }
    }

    if (curBatch == 0) {
        i = 0;
    } else {
        i--;
    }
    bb = &level->segmentsBoundingBoxes[i];
    gWaveBoundingBoxDiffX = bb->x2 - bb->x1;
    gWaveBoundingBoxDiffZ = bb->z2 - bb->z1;
    gWaveBoundingBoxX1 = bb->x1;
    gWaveBoundingBoxZ1 = bb->z1;
    gWaveBatch = curBatch;
    gWaveTexture = level->textures[curBatch->textureIndex].texture;
    // Change these batch flags to 0, 1, 2 and 4
    colourID = (curBatch->flags & (RENDER_UNK_40000000 | RENDER_UNK_20000000 | RENDER_UNK_10000000)) >> 28;
    if (colourID > 0) {
        gWavePulseLight = header->colourCyclesByFlagIndex[colourID];
    } else {
        gWavePulseLight = NULL;
    }
}

/**
 * Lays the level's segments out on a grid of wave tiles: finds the grid's extent, allocates the per-segment
 * wave data and the wave generator tables, and records each segment's place on the grid and water height.
 */
void waves_init_segments(LevelModel *model) {
    LevelModelSegmentBoundingBox *levelSegmentBoundingBoxes;
    s32 j;
    s32 originX;
    s32 segmentVertexY;
    s32 originZ;
    s32 subdivisions;
    LevelModelSegment *levelSegments;
    s32 i;
    s32 tileOffset;
    LevelModel_Alternate *otherModel;

    levelSegments = model->segments;
    levelSegmentBoundingBoxes = model->segmentsBoundingBoxes;
    subdivisions = gWaveController.subdivisions;
    if (gWaveController.doubleDensity) {
        subdivisions *= 2;
    }
    gWaveBlockBoundsX1 = levelSegmentBoundingBoxes->x1;
    gWaveBlockBoundsX2 = levelSegmentBoundingBoxes->x2;
    gWaveBlockBoundsZ1 = levelSegmentBoundingBoxes->z1;
    gWaveBlockBoundsZ2 = levelSegmentBoundingBoxes->z2;
    for (i = 1; i < model->numberOfSegments; i++) {
        if (levelSegmentBoundingBoxes[i].x1 < gWaveBlockBoundsX1) {
            gWaveBlockBoundsX1 = levelSegmentBoundingBoxes[i].x1;
        }
        if (gWaveBlockBoundsX2 < levelSegmentBoundingBoxes[i].x2) {
            gWaveBlockBoundsX2 = levelSegmentBoundingBoxes[i].x2;
        }
        if (levelSegmentBoundingBoxes[i].z1 < gWaveBlockBoundsZ1) {
            gWaveBlockBoundsZ1 = levelSegmentBoundingBoxes[i].z1;
        }
        if (gWaveBlockBoundsZ2 < levelSegmentBoundingBoxes[i].z2) {
            gWaveBlockBoundsZ2 = levelSegmentBoundingBoxes[i].z2;
        }
    }

    stubbed_printf("wave bbox diffX=%d diffZ=%d startX=%d startZ=%d boundsX1=%d boundsX2=%d boundsZ1=%d boundsZ2=%d\n",
                   gWaveBoundingBoxDiffX, gWaveBoundingBoxDiffZ, gWaveBoundingBoxX1, gWaveBoundingBoxZ1,
                   gWaveBlockBoundsX1, gWaveBlockBoundsX2, gWaveBlockBoundsZ1, gWaveBlockBoundsZ2);
    gWaveBlockPosX = gWaveBoundingBoxX1;
    if (gWaveBoundingBoxDiffX > 0) {
        while (gWaveBlockBoundsX1 < gWaveBlockPosX) {
            gWaveBlockPosX -= gWaveBoundingBoxDiffX;
        }
    }
    stubbed_printf("post X while, posX=%d\n", gWaveBlockPosX);

    gWaveBlockPosZ = gWaveBoundingBoxZ1;
    if (gWaveBoundingBoxDiffZ > 0) {
        while (gWaveBlockBoundsZ1 < gWaveBlockPosZ) {
            gWaveBlockPosZ -= gWaveBoundingBoxDiffZ;
        }
    }
    stubbed_printf("post Z while, posZ=%d\n", gWaveBlockPosZ);

    gWaveTileCountX = ((gWaveBlockBoundsX2 - gWaveBlockPosX) / gWaveBoundingBoxDiffX) + 1;
    gWaveTileCountZ = ((gWaveBlockBoundsZ2 - gWaveBlockPosZ) / gWaveBoundingBoxDiffZ) + 1;
    gWaveTileGridCount = (subdivisions * gWaveTileCountX) + 1;
    stubbed_printf("tileCountX=%d tileCountZ=%d tileGridCount=%d subdiv=%d\n", gWaveTileCountX, gWaveTileCountZ,
                   gWaveTileGridCount, subdivisions);

    if (gWaveBlockCells != NULL) {
        mempool_free(gWaveBlockCells);
    }
    gWaveBlockCells = mempool_alloc_safe(gWaveTileCountX * gWaveTileCountZ * sizeof(uintptr_t), COLOUR_TAG_CYAN);

    if (gWaveModel != NULL) {
        mempool_free(gWaveModel);
    }

    // clang-format off
    gWaveModel = mempool_alloc_safe(
        (model->numberOfSegments * sizeof(LevelModel_Alternate)) + (gWaveTileGridCount * WAVE_GEN_SLOTS) + 0x880,
        COLOUR_TAG_CYAN
    );
    // clang-format on

    gWaveGenList = (WaveGen *) ((uintptr_t) gWaveModel + model->numberOfSegments * sizeof(LevelModel_Alternate));
    gWaveGenObjs = (Object **) (gWaveGenList + sizeof(WaveGen *) * 8);
    gWaveGenSlots = (WaveGenTileSlots *) (gWaveGenObjs + WAVE_GEN_MAX);

    for (i = 0; i < (gWaveTileGridCount * WAVE_GEN_SLOTS); i++) {
        gWaveGenSlots->genIndices[i] = WAVE_GEN_SLOT_EMPTY;
    }

    for (i = 0; i < WAVE_GEN_MAX; i++) {
        gWaveGenObjs[i] = 0;
    }

    gWaveGenCount = 0;

    for (i = 0; i < (gWaveTileCountX * gWaveTileCountZ); i++) {
        gWaveBlockCells[i] = 0;
    }

    for (i = 0; i < ARRAY_COUNT(gWaveVisibilityGrid); i++) {
        gWaveVisibilityGrid[i] = 0;
    }

    gNumberOfLevelSegments = model->numberOfSegments;
    for (i = 0; i < gNumberOfLevelSegments; i++) {
        originX = levelSegmentBoundingBoxes[i].x1 - gWaveBlockPosX + 8;
        originZ = levelSegmentBoundingBoxes[i].z1 - gWaveBlockPosZ + 8;
        originX = ((originX / gWaveBoundingBoxDiffX) * gWaveBoundingBoxDiffX) + gWaveBlockPosX;
        originZ = ((originZ / gWaveBoundingBoxDiffZ) * gWaveBoundingBoxDiffZ) + gWaveBlockPosZ;
        segmentVertexY = 0;
        for (j = 0; j < levelSegments[i].numberOfBatches; j++) {
            if ((levelSegments[i].batches[j].flags & RENDER_WATER) &&
                (levelSegments[i].batches[j].flags & RENDER_UNK_0400000)) {
                segmentVertexY = levelSegments[i].vertices[levelSegments[i].batches[j].verticesOffset].y;
            }
        }

        otherModel = &gWaveModel[i];
        otherModel->block = &levelSegments[i];
        otherModel->originX = originX;
        otherModel->originY = segmentVertexY;
        otherModel->originZ = originZ;
        otherModel->tileX = (originX - gWaveBlockPosX) / gWaveBoundingBoxDiffX;
        otherModel->tileZ = (originZ - gWaveBlockPosZ) / gWaveBoundingBoxDiffZ;
        otherModel->tileIndex = otherModel->tileX + (otherModel->tileZ * gWaveTileCountX);
        otherModel->waveTileOffsetX = 0;
        otherModel->waveTileOffsetZ = 0;
        if (levelSegments[i].hasWaves != 0) {
            tileOffset = otherModel->tileX * subdivisions;
            while (tileOffset >= gWaveController.tileCount) {
                tileOffset -= gWaveController.tileCount;
            }
            otherModel->waveTileOffsetX = tileOffset;
            tileOffset = otherModel->tileZ * subdivisions;
            while (tileOffset >= gWaveController.tileCount) {
                tileOffset -= gWaveController.tileCount;
            }
            otherModel->waveTileOffsetZ = tileOffset;
            gWaveVisibilityGrid[otherModel->tileZ] |= (1 << otherModel->tileX);
        }

        for (j = 0; j < 4; j++) {
            otherModel->viewportFade[0].quadrantFade[j] = WAVE_FADE_MAX;
            otherModel->viewportFade[1].quadrantFade[j] = WAVE_FADE_MAX;
        }
    }
}

/**
 * Fills the nine height-scale grids in gWaveHeightBuffers: a quarter sine ramp from 0 to 1 laid across a wave
 * cell in each direction, so cells at the edge of the view fade the swell out towards their outer side.
 * Buffer 4 is the centre and stays flat at 1.
 */
void waves_build_sine_table(void) {
    s32 i;
    s32 j;
    s32 k;
    s32 angle;
    s32 angleStep;
    f32 sineRamp[0x80];

    angleStep = 0x4000 / gWaveController.subdivisions;
    angle = 0;
    for (i = 0; i < gWaveController.subdivisions;) {
        sineRamp[i++] = sins_f(angle);
        angle += angleStep;
    }
    sineRamp[0] = 0.0f;
    sineRamp[gWaveController.subdivisions] = 1.0f;

    k = 0;
    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = 0; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[4][k] = 1.0f;
            k++;
        }
    }

    k = 0;
    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = 0; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[1][k] = sineRamp[i];
            k++;
        }
    }

    k = 0;
    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = 0; j <= gWaveController.subdivisions;) {
            gWaveHeightBuffers[3][k++] = sineRamp[j++];
        }
    }

    k = 0;
    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = 0; j <= gWaveController.subdivisions;) {
            gWaveHeightBuffers[5][k++] = sineRamp[gWaveController.subdivisions - j++];
        }
    }

    k = 0;
    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = 0; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[7][k] = sineRamp[gWaveController.subdivisions - i];
            k++;
        }
    }

    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = i; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[0][i * (gWaveController.subdivisions + 1) + j] = sineRamp[i];
            gWaveHeightBuffers[0][j * (gWaveController.subdivisions + 1) + i] = sineRamp[i];
        }
    }

    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = i; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[2][(i * (gWaveController.subdivisions + 1)) + gWaveController.subdivisions - j] =
                sineRamp[i];
            gWaveHeightBuffers[2][(j * (gWaveController.subdivisions + 1)) + gWaveController.subdivisions - i] =
                sineRamp[i];
        }
    }

    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = i; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[6][(gWaveController.subdivisions * (gWaveController.subdivisions + 1)) -
                                  (i * (gWaveController.subdivisions + 1)) + j] = sineRamp[i];
            gWaveHeightBuffers[6][(gWaveController.subdivisions * (gWaveController.subdivisions + 1)) -
                                  (j * (gWaveController.subdivisions + 1)) + i] = sineRamp[i];
        }
    }

    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = i; j <= gWaveController.subdivisions; j++) {
            // clang-format off
            gWaveHeightBuffers[8][(gWaveController.subdivisions * (gWaveController.subdivisions + 1)) - (i * (gWaveController.subdivisions + 1)) + gWaveController.subdivisions - j] = sineRamp[i];
            gWaveHeightBuffers[8][(gWaveController.subdivisions * (gWaveController.subdivisions + 1)) - (j * (gWaveController.subdivisions + 1)) + gWaveController.subdivisions - i] = sineRamp[i];
            // clang-format on
        }
    }
}

/**
 * Samples the water depth under every wave vertex of each segment that has waves, writing it to
 * gWaveDetailLevels (255 where there is no ground below). Segments without waves get 255 throughout.
 * Records each wavy tile's segment and the start of its depths in tileSegments.
 */
static void wave_collision_sample_depths(LevelModel *model, s32 subdivisions, f32 *sampleOffsets, Vec2i *tileSegments) {
    s32 i;
    s32 row;
    s32 k;
    s32 j;
    f32 y;
    f32 x;
    f32 z;
    f32 stepX;
    f32 stepZ;
    s32 collisionCount;
    s32 depth;
    s32 detailIndex;
    f32 colY[30];

    stepX = gWaveBoundingBoxW / subdivisions;
    stepZ = gWaveBoundingBoxH / subdivisions;
    stubbed_printf("pre collision_get_y loop, numSegs=%d\n", model->numberOfSegments);
    for (i = 0, detailIndex = 0; i < model->numberOfSegments; i++) {
        if (gWaveVisibilityGrid[gWaveModel[i].tileZ] & (1 << gWaveModel[i].tileX)) {
            tileSegments[(gWaveModel[i].tileZ * gWaveTileCountX) + gWaveModel[i].tileX].i[0] = i;
            tileSegments[(gWaveModel[i].tileZ * gWaveTileCountX) + gWaveModel[i].tileX].i[1] = detailIndex;
            y = gWaveModel[i].originY;
            z = gWaveModel[i].originZ;
            for (row = 0; row <= subdivisions; row++) {
                x = gWaveModel[i].originX;
                for (k = 0; k <= subdivisions; k++) {
                    collisionCount = collision_get_y(i, sampleOffsets[k] + x, sampleOffsets[row] + z, colY, 30);
                    if (collisionCount > 30) {
                        stubbed_printf("collision_get_y OVERFLOW count=%d seg=%d\n", collisionCount, i);
                    }
                    if (collisionCount == 0) {
                        depth = 255;
                    } else {
                        // Skip the surfaces at or above the water.
                        j = 0;
                        while (j < collisionCount - 1 && y <= colY[j]) {
                            j++;
                        }
                        if (y < colY[j]) {
                            depth = 0;
                        } else if (y == colY[j]) {
                            depth = 255;
                        } else {
                            depth = (y - colY[j]) * gWaveController.heightColourScale;
                            if (depth > 511) {
                                depth = 511;
                            }
                            depth >>= 1;
                        }
                    }
                    gWaveDetailLevels[detailIndex++] = depth;
                    x += stepX;
                }
                z += stepZ;
            }
        } else {
            // clang-format off
            // @note \ required here
            for (k = 0; k < gWaveTileTotal; k++) { \
                gWaveDetailLevels[detailIndex++] = 255;
            }
            // clang-format on
        }
    }
    stubbed_printf("post collision_get_y loop\n");
}

/**
 * Where two wavy tiles meet, averages the depths of the vertices along the shared edge (except its ends),
 * so the swell matches on both sides.
 */
static void wave_collision_blend_tile_edges(s32 subdivisions, Vec2i *tileSegments) {
    s32 row;
    s32 col;
    s32 tileStart;
    s32 neighbourStart;
    s32 index;
    s32 neighbourIndex;
    s32 depth;
    s32 n;

    for (row = 0; row < gWaveTileCountZ; row++) {
        for (col = 0; col < gWaveTileCountX; col++) {
            tileStart = tileSegments[(row * gWaveTileCountX) + col].i[1];
            if (tileStart != -1) {
                if (row < (gWaveTileCountZ - 1)) {
                    neighbourStart = tileSegments[((row + 1) * gWaveTileCountX) + col].i[1];
                    if (neighbourStart != -1) {
                        index = ((subdivisions + 1) * subdivisions) + tileStart + 1;
                        neighbourIndex = neighbourStart + 1;
                        for (n = 0; n < subdivisions - 1; n++) {
                            depth = (gWaveDetailLevels[index] + gWaveDetailLevels[neighbourIndex]) >> 1;
                            gWaveDetailLevels[index] = depth;
                            gWaveDetailLevels[neighbourIndex] = depth;
                            neighbourIndex++;
                            index++;
                        }
                    }
                }
                if (col < (gWaveTileCountX - 1)) {
                    neighbourStart = tileSegments[(row * gWaveTileCountX) + col + 1].i[1];
                    if (neighbourStart != -1) {
                        index = (subdivisions * 2) + tileStart + 1;
                        neighbourIndex = neighbourStart + subdivisions + 1;
                        for (n = 0; n < subdivisions - 1; n++) {
                            depth = (gWaveDetailLevels[index] + gWaveDetailLevels[neighbourIndex]) >> 1;
                            gWaveDetailLevels[index] = depth;
                            if (!neighbourIndex) {}
                            gWaveDetailLevels[neighbourIndex] = depth;
                            neighbourIndex += subdivisions;
                            neighbourIndex++;
                            index += subdivisions;
                            index++;
                        }
                    }
                }
            }
        }
    }
}

/** Copies each segment's four corner depths (top-left, top-right, bottom-left, bottom-right) aside. */
static void wave_collision_store_corners(LevelModel *model, s32 subdivisions, u8 *cornerDepths) {
    s32 i;
    s32 tileStart;

    for (tileStart = 0, i = 0; i < model->numberOfSegments; i++, tileStart += gWaveTileTotal) {
        cornerDepths[(i * 4) + 0] = gWaveDetailLevels[tileStart];
        cornerDepths[(i * 4) + 1] = gWaveDetailLevels[tileStart + (subdivisions)];
        cornerDepths[(i * 4) + 2] = gWaveDetailLevels[tileStart + ((subdivisions + 1) * subdivisions)];
        cornerDepths[(i * 4) + 3] = gWaveDetailLevels[tileStart + ((subdivisions + 1) * (subdivisions + 1)) - 1];
    }
}

/**
 * Finds the segments on the eight tiles around a tile, -1 where there is none:
 * 0-2 the row above, 3 and 4 left and right, 5-7 the row below.
 */
static void wave_collision_find_neighbours(s32 segment, s32 tileSlot, Vec2i *tileSegments, s32 *neighbours) {
    s32 n;

    for (n = 0; n < 8; n++) {
        neighbours[n] = -1;
    }

    if (gWaveModel[segment].tileZ > 0) {
        if (gWaveModel[segment].tileX > 0) {
            neighbours[0] = tileSegments[tileSlot - gWaveTileCountX - 1].i[0];
        }
        neighbours[1] = tileSegments[tileSlot - gWaveTileCountX].i[0];
        if (gWaveModel[segment].tileX < gWaveTileCountX - 1) {
            neighbours[2] = tileSegments[tileSlot - gWaveTileCountX + 1].i[0];
        }
    }

    if (gWaveModel[segment].tileX > 0) {
        neighbours[3] = tileSegments[tileSlot - 1].i[0];
    }

    if (gWaveModel[segment].tileX < (gWaveTileCountX - 1)) {
        neighbours[4] = tileSegments[tileSlot + 1].i[0];
    }

    if (gWaveModel[segment].tileZ < (gWaveTileCountZ - 1)) {
        if (gWaveModel[segment].tileX > 0) {
            neighbours[5] = tileSegments[tileSlot + gWaveTileCountX - 1].i[0];
        }
        neighbours[6] = tileSegments[tileSlot + gWaveTileCountX].i[0];
        if (gWaveModel[segment].tileX < gWaveTileCountX - 1) {
            neighbours[7] = tileSegments[tileSlot + gWaveTileCountX + 1].i[0];
        }
    }
}

/**
 * Sets each wavy segment's four corner depths to the average of the matching corners of every tile that
 * shares that corner, so the swell matches where four tiles meet.
 */
static void wave_collision_blend_corners(LevelModel *model, s32 subdivisions, Vec2i *tileSegments, u8 *cornerDepths) {
    s32 i;
    s32 tileStart;
    s32 tileSlot;
    s32 depthSum;
    s32 count;
    s32 neighbours[8];

    for (tileStart = 0, i = 0; i < model->numberOfSegments; i++, tileStart += gWaveTileTotal) {
        tileSlot = (gWaveModel[i].tileZ * gWaveTileCountX) + gWaveModel[i].tileX;
        if (tileSegments[tileSlot].i[0] != -1) {
            wave_collision_find_neighbours(i, tileSlot, tileSegments, neighbours);

            count = 1;
            depthSum = cornerDepths[i * 4];
            if (neighbours[0] != -1) {
                count++;
                depthSum += cornerDepths[(neighbours[0] * 4) + 3];
            }

            if (neighbours[1] != -1) {
                count++;
                depthSum += cornerDepths[(neighbours[1] * 4) + 2];
            }

            if (neighbours[3] != -1) {
                count++;
                depthSum += cornerDepths[(neighbours[3] * 4) + 1];
            }
            gWaveDetailLevels[tileStart] = depthSum / count;

            depthSum = cornerDepths[(i * 4) + 1];
            count = 1;
            if (neighbours[1] != -1) {
                count++;
                depthSum += cornerDepths[(neighbours[1] * 4) + 3];
            }

            if (neighbours[2] != -1) {
                count++;
                depthSum += cornerDepths[(neighbours[2] * 4) + 2];
            }

            if (neighbours[4] != -1) {
                count++;
                depthSum += cornerDepths[neighbours[4] * 4];
            }
            gWaveDetailLevels[tileStart + subdivisions] = depthSum / count;

            depthSum = cornerDepths[(i * 4) + 2];
            count = 1;
            if (neighbours[3] != -1) {
                count++;
                depthSum += cornerDepths[(neighbours[3] * 4) + 3];
            }

            if (neighbours[5] != -1) {
                count++;
                depthSum += cornerDepths[(neighbours[5] * 4) + 1];
            }

            if (neighbours[6] != -1) {
                count++;
                depthSum += cornerDepths[neighbours[6] * 4];
            }
            gWaveDetailLevels[tileStart + ((subdivisions + 1) * subdivisions)] = depthSum / count;

            depthSum = cornerDepths[(i * 4) + 3];
            count = 1;
            if (neighbours[4] != -1) {
                count++;
                depthSum += cornerDepths[(neighbours[4] * 4) + 2];
            }

            if (neighbours[6] != -1) {
                count++;
                depthSum += cornerDepths[(neighbours[6] * 4) + 1];
            }

            if (neighbours[7] != -1) {
                count++;
                depthSum += cornerDepths[neighbours[7] * 4];
            }

            gWaveDetailLevels[tileStart + ((subdivisions + 1) * (subdivisions + 1)) - 1] = depthSum / count;
        }
    }
}

/**
 * Builds gWaveDetailLevels: the water depth under every wave vertex of every segment, found with collision
 * checks and then blended across tile edges and corners. Shallow water damps the swell.
 */
void waves_init_collision(LevelModel *model) {
    u32 unset;
    s32 k;
    s32 n;
    s32 subdivisions;
    f32 *sampleOffsets;
    Vec2i *tileSegments;
    u8 *cornerDepths;

    stubbed_printf("waves_init_collision enter\n");
    subdivisions = gWaveController.subdivisions;
    if (gWaveController.doubleDensity) {
        subdivisions *= 2;
    }
    gWaveTileTotal = (subdivisions + 1) * (subdivisions + 1);
    if (gWaveDetailLevels != NULL) {
        mempool_free(gWaveDetailLevels);
    }
    gWaveDetailLevels = mempool_alloc_safe(model->numberOfSegments * gWaveTileTotal, COLOUR_TAG_CYAN);
    cornerDepths = mempool_alloc_safe(model->numberOfSegments * 4, COLOUR_TAG_CYAN);
    tileSegments = mempool_alloc_safe((gWaveTileCountX * gWaveTileCountZ) * 8, COLOUR_TAG_CYAN);
    sampleOffsets = mempool_alloc_safe((subdivisions * 4) + 4, COLOUR_TAG_CYAN);

    // temp assignment required for match
    unset = -1;
    for (n = 0; n < (gWaveTileCountX * gWaveTileCountZ); n++) {
        tileSegments[n].i[0] = -1;
        tileSegments[n].i[1] = unset;
    }

    // Sample points are pulled in towards the middle of the segment, by 8 units at its edges.
    for (k = 0; k <= subdivisions; k++) {
        sampleOffsets[k] = ((subdivisions - (k << 1)) / (f32) subdivisions) * 8.0f;
    }

    wave_collision_sample_depths(model, subdivisions, sampleOffsets, tileSegments);
    wave_collision_blend_tile_edges(subdivisions, tileSegments);
    wave_collision_store_corners(model, subdivisions, cornerDepths);
    wave_collision_blend_corners(model, subdivisions, tileSegments, cornerDepths);

    mempool_free(sampleOffsets);
    mempool_free(tileSegments);
    mempool_free(cornerDepths);
    stubbed_printf("waves_init_collision end\n");
}
