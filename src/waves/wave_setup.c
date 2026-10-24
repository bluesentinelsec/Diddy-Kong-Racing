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

void waves_init_segments(LevelModel *model) {
    LevelModelSegmentBoundingBox *levelSegmentBoundingBoxes;
    s32 j;
    s32 temp_t1;
    s32 segmentVertexY;
    s32 temp_t2;
    s32 pad;
    s32 subdivisions;
    LevelModelSegment *levelSegments;
    s32 i;
    s32 var_v0;
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
        (model->numberOfSegments * sizeof(LevelModel_Alternate)) + (gWaveTileGridCount * 8) + 0x880,
        COLOUR_TAG_CYAN
    );
    // clang-format on

    gWaveGenList = (WaveGen *) ((uintptr_t) gWaveModel + model->numberOfSegments * sizeof(LevelModel_Alternate));
    gWaveGenObjs = (Object **) (gWaveGenList + sizeof(WaveGen *) * 8);
    gWaveGenSlots = (WaveGenTileSlots *) (gWaveGenObjs + 32);

    for (i = 0; i < (gWaveTileGridCount * 8); i++) {
        gWaveGenSlots->genIndices[i] = 0xFF;
    }

    // 0x20 / 32 sizeof what?
    for (i = 0; i < 32; i++) {
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
        temp_t1 = levelSegmentBoundingBoxes[i].x1 - gWaveBlockPosX + 8;
        temp_t2 = levelSegmentBoundingBoxes[i].z1 - gWaveBlockPosZ + 8;
        temp_t1 = ((temp_t1 / gWaveBoundingBoxDiffX) * gWaveBoundingBoxDiffX) + gWaveBlockPosX;
        temp_t2 = ((temp_t2 / gWaveBoundingBoxDiffZ) * gWaveBoundingBoxDiffZ) + gWaveBlockPosZ;
        segmentVertexY = 0;
        for (j = 0; j < levelSegments[i].numberOfBatches; j++) {
            if ((levelSegments[i].batches[j].flags & RENDER_WATER) &&
                (levelSegments[i].batches[j].flags & RENDER_UNK_0400000)) {
                segmentVertexY = levelSegments[i].vertices[levelSegments[i].batches[j].verticesOffset].y;
            }
        }

        otherModel = &gWaveModel[i];
        otherModel->block = &levelSegments[i];
        otherModel->originX = temp_t1;
        otherModel->originY = segmentVertexY;
        otherModel->originZ = temp_t2;
        otherModel->tileX = (temp_t1 - gWaveBlockPosX) / gWaveBoundingBoxDiffX;
        otherModel->tileZ = (temp_t2 - gWaveBlockPosZ) / gWaveBoundingBoxDiffZ;
        otherModel->tileIndex = otherModel->tileX + (otherModel->tileZ * gWaveTileCountX);
        otherModel->waveTileOffsetX = 0;
        otherModel->waveTileOffsetZ = 0;
        if (levelSegments[i].hasWaves != 0) {
            var_v0 = otherModel->tileX * subdivisions;
            while (var_v0 >= gWaveController.tileCount) {
                var_v0 -= gWaveController.tileCount;
            }
            otherModel->waveTileOffsetX = var_v0;
            var_v0 = otherModel->tileZ * subdivisions;
            while (var_v0 >= gWaveController.tileCount) {
                var_v0 -= gWaveController.tileCount;
            }
            otherModel->waveTileOffsetZ = var_v0;
            gWaveVisibilityGrid[otherModel->tileZ] |= (1 << otherModel->tileX);
        }

        for (j = 0; j < 4; j++) {
            otherModel->viewportFade[0].quadrantFade[j] = 0x80;
            otherModel->viewportFade[1].quadrantFade[j] = 0x80;
        }
    }
}

void waves_build_sine_table(void) {
    s32 i;
    s32 j;
    s32 k;
    s32 var_s0;
    s32 var_v0;
    f32 sp34[0x80];

    var_v0 = 0x4000 / gWaveController.subdivisions;
    var_s0 = 0;
    for (i = 0; i < gWaveController.subdivisions;) {
        sp34[i++] = sins_f(var_s0);
        var_s0 += var_v0;
    }
    sp34[0] = 0.0f;
    sp34[gWaveController.subdivisions] = 1.0f;

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
            gWaveHeightBuffers[1][k] = sp34[i];
            k++;
        }
    }

    k = 0;
    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = 0; j <= gWaveController.subdivisions;) {
            gWaveHeightBuffers[3][k++] = sp34[j++];
        }
    }

    k = 0;
    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = 0; j <= gWaveController.subdivisions;) {
            gWaveHeightBuffers[5][k++] = sp34[gWaveController.subdivisions - j++];
        }
    }

    k = 0;
    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = 0; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[7][k] = sp34[gWaveController.subdivisions - i];
            k++;
        }
    }

    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = i; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[0][i * (gWaveController.subdivisions + 1) + j] = sp34[i];
            gWaveHeightBuffers[0][j * (gWaveController.subdivisions + 1) + i] = sp34[i];
        }
    }

    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = i; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[2][(i * (gWaveController.subdivisions + 1)) + gWaveController.subdivisions - j] =
                sp34[i];
            gWaveHeightBuffers[2][(j * (gWaveController.subdivisions + 1)) + gWaveController.subdivisions - i] =
                sp34[i];
        }
    }

    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = i; j <= gWaveController.subdivisions; j++) {
            gWaveHeightBuffers[6][(gWaveController.subdivisions * (gWaveController.subdivisions + 1)) -
                                  (i * (gWaveController.subdivisions + 1)) + j] = sp34[i];
            gWaveHeightBuffers[6][(gWaveController.subdivisions * (gWaveController.subdivisions + 1)) -
                                  (j * (gWaveController.subdivisions + 1)) + i] = sp34[i];
        }
    }

    for (i = 0; i <= gWaveController.subdivisions; i++) {
        for (j = i; j <= gWaveController.subdivisions; j++) {
            // clang-format off
            gWaveHeightBuffers[8][(gWaveController.subdivisions * (gWaveController.subdivisions + 1)) - (i * (gWaveController.subdivisions + 1)) + gWaveController.subdivisions - j] = sp34[i];
            gWaveHeightBuffers[8][(gWaveController.subdivisions * (gWaveController.subdivisions + 1)) - (j * (gWaveController.subdivisions + 1)) + gWaveController.subdivisions - i] = sp34[i];
            // clang-format on
        }
    }
}

void waves_init_collision(LevelModel *model) {
    s32 i;
    u32 pad_sp188;
    s32 sp184;
    s32 k;
    s32 var_t4;
    s32 var_t8;
    f32 y;
    f32 x;
    f32 z;
    f32 stepX;
    f32 stepZ;
    s32 collisionCount;
    s32 sp140[8];
    s32 j;
    s32 var_a1;
    s32 var_a2;
    s32 var_a3;
    s32 var_s2;
    s32 subdivisions;
    s32 var_s5;
    f32 colY[30];
    f32 *spA8;
    Vec2i *spA4;
    u8 *spA0;

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
    spA0 = mempool_alloc_safe(model->numberOfSegments * 4, COLOUR_TAG_CYAN);
    spA4 = mempool_alloc_safe((gWaveTileCountX * gWaveTileCountZ) * 8, COLOUR_TAG_CYAN);
    spA8 = mempool_alloc_safe((subdivisions * 4) + 4, COLOUR_TAG_CYAN);

    // temp assignment required for match
    pad_sp188 = -1;
    for (var_a3 = 0; var_a3 < (gWaveTileCountX * gWaveTileCountZ); var_a3++) {
        spA4[var_a3].i[0] = -1;
        spA4[var_a3].i[1] = pad_sp188;
    }

    for (k = 0; k <= subdivisions; k++) {
        spA8[k] = ((subdivisions - (k << 1)) / (f32) subdivisions) * 8.0f;
    }

    stepX = gWaveBoundingBoxW / subdivisions;
    stepZ = gWaveBoundingBoxH / subdivisions;
    stubbed_printf("pre collision_get_y loop, numSegs=%d\n", model->numberOfSegments);
    for (i = 0, var_s5 = 0; i < model->numberOfSegments; i++) {
        if (gWaveVisibilityGrid[gWaveModel[i].tileZ] & (1 << gWaveModel[i].tileX)) {
            spA4[(gWaveModel[i].tileZ * gWaveTileCountX) + gWaveModel[i].tileX].i[0] = i;
            spA4[(gWaveModel[i].tileZ * gWaveTileCountX) + gWaveModel[i].tileX].i[1] = var_s5;
            y = gWaveModel[i].originY;
            z = gWaveModel[i].originZ;
            for (sp184 = 0; sp184 <= subdivisions; sp184++) {
                x = gWaveModel[i].originX;
                for (k = 0; k <= subdivisions; k++) {
                    // var_v0 stores the length of spAC
                    collisionCount = collision_get_y(i, spA8[k] + x, spA8[sp184] + z, colY, 30);
                    if (collisionCount > 30) {
                        stubbed_printf("collision_get_y OVERFLOW count=%d seg=%d\n", collisionCount, i);
                    }
                    if (collisionCount == 0) {
                        var_a2 = 255;
                    } else {
                        j = 0;
                        while (j < collisionCount - 1 && y <= colY[j]) {
                            j++;
                        }
                        if (y < colY[j]) {
                            var_a2 = 0;
                        } else if (y == colY[j]) {
                            var_a2 = 255;
                        } else {
                            var_a2 = (y - colY[j]) * gWaveController.heightColourScale;
                            if (var_a2 > 511) {
                                var_a2 = 511;
                            }
                            var_a2 >>= 1;
                        }
                    }
                    gWaveDetailLevels[var_s5++] = var_a2;
                    x += stepX;
                }
                z += stepZ;
            }
        } else {
            // clang-format off
            // @note \ required here
            for (k = 0; k < gWaveTileTotal; k++) { \
                gWaveDetailLevels[var_s5++] = 255;
            }
            // clang-format on
        }
    }
    stubbed_printf("post collision_get_y loop\n");

    for (sp184 = 0; sp184 < gWaveTileCountZ; sp184++) {
        for (k = 0; k < gWaveTileCountX; k++) {
            var_t4 = spA4[(sp184 * gWaveTileCountX) + k].i[1];
            if (var_t4 != -1) {
                if (sp184 < (gWaveTileCountZ - 1)) {
                    collisionCount = spA4[((sp184 + 1) * gWaveTileCountX) + k].i[1];
                    if (collisionCount != -1) {
                        var_a1 = ((subdivisions + 1) * subdivisions) + var_t4 + 1;
                        j = collisionCount + 1;
                        for (var_a3 = 0; var_a3 < subdivisions - 1; var_a3++) {
                            var_a2 = (gWaveDetailLevels[var_a1] + gWaveDetailLevels[j]) >> 1;
                            gWaveDetailLevels[var_a1] = var_a2;
                            gWaveDetailLevels[j] = var_a2;
                            j++;
                            var_a1++;
                        }
                    }
                }
                if (k < (gWaveTileCountX - 1)) {
                    collisionCount = spA4[(sp184 * gWaveTileCountX) + k + 1].i[1];
                    if (collisionCount != -1) {
                        var_a1 = (subdivisions * 2) + var_t4 + 1;
                        j = collisionCount + subdivisions + 1;
                        for (var_a3 = 0; var_a3 < subdivisions - 1; var_a3++) {
                            var_a2 = (gWaveDetailLevels[var_a1] + gWaveDetailLevels[j]) >> 1;
                            gWaveDetailLevels[var_a1] = var_a2;
                            if (!j) {}
                            gWaveDetailLevels[j] = var_a2;
                            j += subdivisions;
                            j++;
                            var_a1 += subdivisions;
                            var_a1++;
                        }
                    }
                }
            }
        }
    }

    for (var_t4 = 0, i = 0; i < model->numberOfSegments; i++, var_t4 += gWaveTileTotal) {
        spA0[(i * 4) + 0] = gWaveDetailLevels[var_t4];
        spA0[(i * 4) + 1] = gWaveDetailLevels[var_t4 + (subdivisions)];
        spA0[(i * 4) + 2] = gWaveDetailLevels[var_t4 + ((subdivisions + 1) * subdivisions)];
        spA0[(i * 4) + 3] = gWaveDetailLevels[var_t4 + ((subdivisions + 1) * (subdivisions + 1)) - 1];
    }

    for (var_t4 = 0, i = 0; i < model->numberOfSegments; i++, var_t4 += gWaveTileTotal) {
        var_s5 = (gWaveModel[i].tileZ * gWaveTileCountX) + gWaveModel[i].tileX;
        if (spA4[var_s5].i[0] != -1) {
            for (var_a3 = 0; var_a3 < ARRAY_COUNT(sp140); var_a3++) {
                sp140[var_a3] = -1;
            }

            if (gWaveModel[i].tileZ > 0) {
                if (gWaveModel[i].tileX > 0) {
                    sp140[0] = spA4[var_s5 - gWaveTileCountX - 1].i[0];
                }
                sp140[1] = spA4[var_s5 - gWaveTileCountX].i[0];
                if (gWaveModel[i].tileX < gWaveTileCountX - 1) {
                    sp140[2] = spA4[var_s5 - gWaveTileCountX + 1].i[0];
                }
            }

            if (gWaveModel[i].tileX > 0) {
                sp140[3] = spA4[var_s5 - 1].i[0];
            }

            if (gWaveModel[i].tileX < (gWaveTileCountX - 1)) {
                sp140[4] = spA4[var_s5 + 1].i[0];
            }

            if (gWaveModel[i].tileZ < (gWaveTileCountZ - 1)) {
                if (gWaveModel[i].tileX > 0) {
                    sp140[5] = spA4[var_s5 + gWaveTileCountX - 1].i[0];
                }
                sp140[6] = spA4[var_s5 + gWaveTileCountX].i[0];
                if (gWaveModel[i].tileX < gWaveTileCountX - 1) {
                    sp140[7] = spA4[var_s5 + gWaveTileCountX + 1].i[0];
                }
            }

            var_a3 = 1;
            var_a2 = spA0[i * 4];
            if (sp140[0] != -1) {
                var_a3++;
                var_a2 += spA0[(sp140[0] * 4) + 3];
            }

            if (sp140[1] != -1) {
                var_a3++;
                var_a2 += spA0[(sp140[1] * 4) + 2];
            }

            if (sp140[3] != -1) {
                var_a3++;
                var_a2 += spA0[(sp140[3] * 4) + 1];
            }
            gWaveDetailLevels[var_t4] = var_a2 / var_a3;

            var_a2 = spA0[(i * 4) + 1];
            var_a3 = 1;
            if (sp140[1] != -1) {
                var_a3++;
                var_a2 += spA0[(sp140[1] * 4) + 3];
            }

            if (sp140[2] != -1) {
                var_a3++;
                var_a2 += spA0[(sp140[2] * 4) + 2];
            }

            if (sp140[4] != -1) {
                var_a3++;
                var_a2 += spA0[sp140[4] * 4];
            }
            gWaveDetailLevels[var_t4 + subdivisions] = var_a2 / var_a3;

            var_a2 = spA0[(i * 4) + 2];
            var_a3 = 1;
            if (sp140[3] != -1) {
                var_a3++;
                var_a2 += spA0[(sp140[3] * 4) + 3];
            }

            if (sp140[5] != -1) {
                var_a3++;
                var_a2 += spA0[(sp140[5] * 4) + 1];
            }

            if (sp140[6] != -1) {
                var_a3++;
                var_a2 += spA0[sp140[6] * 4];
            }
            gWaveDetailLevels[var_t4 + ((subdivisions + 1) * subdivisions)] = var_a2 / var_a3;

            var_a2 = spA0[(i * 4) + 3];
            var_a3 = 1;
            if (sp140[4] != -1) {
                var_a3++;
                var_a2 += spA0[(sp140[4] * 4) + 2];
            }

            if (sp140[6] != -1) {
                var_a3++;
                var_a2 += spA0[(sp140[6] * 4) + 1];
            }

            if (sp140[7] != -1) {
                var_a3++;
                var_a2 += spA0[sp140[7] * 4];
            }

            gWaveDetailLevels[var_t4 + ((subdivisions + 1) * (subdivisions + 1)) - 1] = var_a2 / var_a3;
        }
    }

    mempool_free(spA8);
    mempool_free(spA4);
    mempool_free(spA0);
    stubbed_printf("waves_init_collision end\n");
}
