#include "waves_internal.h"

/**
 * Set wave visiblity variables to zero.
 */
void waves_visibility_reset(void) {
    s32 i;

    gVisibleWaveTiles = 0;
    for (i = 0; i < gWaveTileCountX * gWaveTileCountZ; i++) {
        gWaveBlockCells[i] = 0;
    }
}

void waves_visibility(s32 xPosition, s32 yPosition, s32 zPosition, s32 currentViewport, s32 updateRate) {
    s32 xPosRatio;
    s32 zPosRatio;
    s32 tempXPosRatio;
    s32 var_v1;
    s32 var_s5;
    u32 var_t2;
    s32 var_t5;
    s32 i;
    s32 var_s4;
    s32 var_a3;
    s32 blockDist;

    xPosRatio = (xPosition - gWaveBlockPosX) / gWaveBoundingBoxDiffX;
    zPosRatio = (zPosition - gWaveBlockPosZ) / gWaveBoundingBoxDiffZ;

    if (0) {}

    for (var_v1 = 0; var_v1 != D_8012A600_COUNT; var_v1 += 4) {
        gWaveVisibleBlocks[0].blockID = -1;
        gWaveVisibleBlocks[1].blockID = -1;
        gWaveVisibleBlocksBody[var_v1].blockID = -1;
        gWaveVisibleBlocksBody[var_v1 + 1].blockID = -1;
        gWaveVisibleBlocksBody[var_v1 + 2].blockID = -1;
        gWaveVisibleBlocksBody[var_v1 + 3].blockID = -1;
    }

    if (gWaveController.doubleDensity) {
        xPosition -= (xPosRatio * gWaveBoundingBoxDiffX) + gWaveBlockPosX;
        zPosition -= (zPosRatio * gWaveBoundingBoxDiffZ) + gWaveBlockPosZ;
        if ((gWaveBoundingBoxDiffX >> 1) < xPosition) {
            var_v1 = 8;
        } else {
            var_v1 = 0;
        }

        if ((gWaveBoundingBoxDiffZ >> 1) < zPosition) {
            var_s4 = 16;
        } else {
            var_s4 = 0;
        }

        for (var_v1 -= (gWaveController.waveViewDist >> 1) * 8; var_v1 < 0; var_v1 += 16) {
            xPosRatio -= 1;
        }

        for (var_s4 -= (gWaveController.waveViewDist >> 1) * 16; var_s4 < 0; var_s4 += 32) {
            zPosRatio -= 1;
        }

        for (blockDist = 0, var_a3 = 0; blockDist < gWaveController.waveViewDist; blockDist++) {
            if (zPosRatio >= 0 && zPosRatio < gWaveTileCountZ) {
                tempXPosRatio = xPosRatio;
                var_t5 = var_v1;
                var_t2 = (zPosRatio * gWaveTileCountX) + xPosRatio;
                for (var_s5 = 0; var_s5 < gWaveController.waveViewDist; var_s5++) {
                    // clang-format off
                    if (
                        (tempXPosRatio >= 0) &&
                        (tempXPosRatio < gWaveTileCountX) &&
                        (gWaveVisibilityGrid[zPosRatio] & (1 << tempXPosRatio))
                    ) {
                        // clang-format on
                        gWaveBlockCells[var_t2] |= gWaveCellLookup[blockDist * gWaveController.waveViewDist + var_s5]
                                                   << (var_t5 + var_s4);
                        for (i = 0; i < gNumberOfLevelSegments; i++) {
                            if (var_t2 == gWaveModel[i].tileIndex) {
                                gWaveVisibleBlocks[var_a3].blockID = i;
                                gWaveVisibleBlocks[var_a3].quadrant = (var_t5 + var_s4) >> 3;
                                gWaveVisibleBlocks[var_a3].detailLevelIndex = i * gWaveTileTotal;
                                gWaveVisibleBlocks[var_a3].tileOffsetX = gWaveModel[i].waveTileOffsetX;
                                if (var_t5 != 0) {
                                    gWaveVisibleBlocks[var_a3].detailLevelIndex += gWaveController.subdivisions;
                                    gWaveVisibleBlocks[var_a3].tileOffsetX += gWaveController.subdivisions;
                                    while (gWaveVisibleBlocks[var_a3].tileOffsetX >= gWaveController.tileCount) {
                                        gWaveVisibleBlocks[var_a3].tileOffsetX -= gWaveController.tileCount;
                                    }
                                }
                                gWaveVisibleBlocks[var_a3].tileOffsetZ = gWaveModel[i].waveTileOffsetZ;
                                if (var_s4 != 0) {
                                    gWaveVisibleBlocks[var_a3].detailLevelIndex +=
                                        ((gWaveController.subdivisions * 2) + 1) * gWaveController.subdivisions;
                                    gWaveVisibleBlocks[var_a3].tileOffsetZ += gWaveController.subdivisions;
                                    while (gWaveVisibleBlocks[var_a3].tileOffsetZ >= gWaveController.tileCount) {
                                        gWaveVisibleBlocks[var_a3].tileOffsetZ -= gWaveController.tileCount;
                                    }
                                }
                                var_a3++;
                                i = 0x7FFF;
                            }
                        }
                    }
                    var_t5 += 8;
                    if (var_t5 > 8) {
                        var_t5 -= 16;
                        tempXPosRatio++;
                        var_t2++;
                    }
                }
            }
            var_s4 += 16;
            if (var_s4 > 16) {
                var_s4 -= 32;
                zPosRatio++;
            }
        }
    } else {
        xPosRatio -= gWaveController.waveViewDist >> 1;
        zPosRatio -= gWaveController.waveViewDist >> 1;
        for (blockDist = 0, var_a3 = 0; blockDist < gWaveController.waveViewDist; blockDist++, zPosRatio++) {
            if ((zPosRatio >= 0) && (zPosRatio < gWaveTileCountZ)) {
                tempXPosRatio = xPosRatio;
                var_t2 = (zPosRatio * gWaveTileCountX) + xPosRatio;
                for (var_s5 = 0; var_s5 < gWaveController.waveViewDist; var_s5++, tempXPosRatio++, var_t2++) {
                    // clang-format off
                    if (
                        (tempXPosRatio >= 0) &&
                        (tempXPosRatio < gWaveTileCountX) &&
                        (gWaveVisibilityGrid[zPosRatio] & (1 << tempXPosRatio))
                    ) {
                        // clang-format on
                        gWaveBlockCells[var_t2] = gWaveCellLookup[blockDist * gWaveController.waveViewDist + var_s5];
                        for (i = 0; i < gNumberOfLevelSegments; i++) {
                            if (var_t2 == gWaveModel[i].tileIndex) {
                                gWaveVisibleBlocks[var_a3].blockID = i;
                                gWaveVisibleBlocks[var_a3].quadrant = 0;
                                gWaveVisibleBlocks[var_a3].tileOffsetX = gWaveModel[i].waveTileOffsetX;
                                gWaveVisibleBlocks[var_a3].tileOffsetZ = gWaveModel[i].waveTileOffsetZ;
                                gWaveVisibleBlocks[var_a3].detailLevelIndex = i * gWaveTileTotal;
                                var_a3++;
                                i = 0x7FFF;
                            }
                        }
                    }
                }
            }
        }
    }

    wave_blocks_decay_ripples(currentViewport, updateRate);
}

/**
 * Returns whether the current wave tile is expected to be the high quality wavegen, over the standard flat plane.
 */
s32 waves_block_hq(LevelModelSegment *block) {
    s32 indexNum = 0;
    s32 result = FALSE;
    while (indexNum < gNumberOfLevelSegments && block != gWaveModel[indexNum].block) {
        indexNum++;
    };
    if (gWaveBlockCells[gWaveModel[indexNum].tileIndex]) {
        result = TRUE;
        gWaveBlockIDs[gVisibleWaveTiles++] = indexNum;
    }
    return result;
}

void wave_block_build_vertices(s32 blockID, s32 viewportID) {
    s32 i;
    s32 var_s2;
    s32 j;
    f32 temp_f22;
    f32 vertexY;
    s32 sp98;
    s32 var_v0;
    s32 sp90;
    s32 sp8C;
    s32 sp88;
    s32 sp84;
    s32 vertexCol;
    s32 vertexIdx;
    s32 var_s1;
    s32 k;
    Vertex *vertices;
    LevelModel_Alternate *sp6C;

    temp_f22 = (1.0f - gWaveController.minDetailScale) / 255.0f;
    sp6C = &gWaveModel[blockID];
    sp90 = (gWaveController.subdivisions + 1) * (gWaveController.subdivisions + 1);
    for (k = 0; gWaveVisibleBlocks[k].blockID != -1; k++) {
        if (blockID != gWaveVisibleBlocks[k].blockID) {
            continue;
        }

        if (gWaveController.doubleDensity) {
            sp98 = (((u32) gWaveBlockCells[sp6C->tileIndex] >> (gWaveVisibleBlocks[k].quadrant * 8)) & 0xFF) - 1;
            sp8C = (gWaveVisibleBlocks[k].quadrant & 1) * gWaveController.subdivisions;
            sp88 = ((gWaveVisibleBlocks[k].quadrant & 2) >> 1) * gWaveController.subdivisions;
        } else {
            sp98 = (gWaveBlockCells[sp6C->tileIndex] & 0xFF) - 1;
            sp8C = 0;
            sp88 = 0;
        }

        sp84 = gWaveVisibleBlocks[k].tileOffsetZ;
        vertices = &gWaveVertices[gWaveVertexFlip + viewportID][sp90 * sp98];
        sp98 = gWaveCellIndexLookup[sp98];
        vertexIdx = 0;
        for (i = 0; i <= gWaveController.subdivisions; i++) {
            var_s1 = gWaveVisibleBlocks[k].tileOffsetX;
            var_s2 = (sp84 * gWaveController.tileCount) + var_s1;
            for (j = 0; j <= gWaveController.subdivisions; j++) {
                vertexY = (gWaveHeightTable[gWaveHeightIndices[var_s2].s[0]] +
                           gWaveHeightTable[gWaveHeightIndices[var_s2].s[1]]) *
                          gWaveController.magnitude;
                if (gWaveGenCount > 0) {
                    vertexY += waves_get_y(blockID, j + sp8C, i + sp88);
                }
                vertexY *= gWaveHeightBuffers[sp98][vertexIdx];
                var_v0 = gWaveDetailLevels[gWaveVisibleBlocks[k].detailLevelIndex];

                // clang-format off
                var_v0 <<= 1; \
                gWaveVisibleBlocks[k].detailLevelIndex++;
                // clang-format on
                if (var_v0 < 255) {
                    vertexY *= gWaveController.minDetailScale + ((f32) var_v0 * temp_f22);
                }
                var_v0 += (s32) (vertexY * gWaveController.heightColourScale);
                if (var_v0 > 255) {
                    var_v0 = 255;
                } else if (var_v0 < 0) {
                    var_v0 = 0;
                }
                var_v0 += ((255 - var_v0) *
                           sp6C->viewportFade[viewportID >> 1].quadrantFade[gWaveVisibleBlocks[k].quadrant]) >>
                          7;
                vertices[vertexIdx].y = vertexY;

                vertexCol = var_v0 < 192 ? 255 : ((255 - var_v0) * 4) & 255;
                vertices[vertexIdx].r = vertexCol;
                vertices[vertexIdx].g = vertexCol;
                vertices[vertexIdx].b = vertexCol;

                vertexCol = var_v0 < 64 ? (var_v0 * 4) & 255 : 255;
                vertices[vertexIdx].a = vertexCol;
                vertexIdx++;
                var_s1++;
                var_s2++;
                if (var_s1 >= gWaveController.tileCount) {
                    var_s1 -= gWaveController.tileCount;
                    var_s2 -= gWaveController.tileCount;
                }
            }
            sp84++;
            if (sp84 >= gWaveController.tileCount) {
                sp84 -= gWaveController.tileCount;
            }
            if (gWaveController.doubleDensity) {
                gWaveVisibleBlocks[k].detailLevelIndex += gWaveController.subdivisions;
            }
        }
    }
}

void wave_block_build_vertices_depth_shaded(s32 blockID, s32 arg1) {
    s32 var_v0;
    f32 temp_f26;
    s32 vertexIdx;
    s32 spA8;
    f32 y;
    s32 spA0;
    s32 sp9C;
    s32 sp98;
    s32 var_a0;
    f32 var_f28;
    s32 var_s1;
    s32 i;
    s32 var_s2;
    s32 j;
    Vertex *vertices;
    LevelModel_Alternate *sp78;
    s32 k;

    k = 0;
    temp_f26 = (gWaveUpperY * 2.0f) - (gWaveLowerY * 2.0f);
    spA0 = (gWaveController.subdivisions + 1) * (gWaveController.subdivisions + 1);
    sp78 = &gWaveModel[blockID];
    var_f28 = temp_f26 <= 0 ? 0 : 1.0f / temp_f26;

    temp_f26 = gWaveLowerY * 2.0f;
    for (; gWaveVisibleBlocks[k].blockID != -1; k++) {
        if (blockID != gWaveVisibleBlocks[k].blockID) {
            continue;
        }

        if (gWaveController.doubleDensity) {
            spA8 = (((u32) gWaveBlockCells[sp78->tileIndex] >> (gWaveVisibleBlocks[k].quadrant * 8)) & 0xFF) - 1;
            sp9C = (gWaveVisibleBlocks[k].quadrant & 1) * gWaveController.subdivisions;
            sp98 = ((s32) (gWaveVisibleBlocks[k].quadrant & 2) >> 1) * gWaveController.subdivisions;
        } else {
            spA8 = (gWaveBlockCells[sp78->tileIndex] & 0xFF) - 1;
            sp9C = 0;
            sp98 = 0;
        }

        var_a0 = gWaveVisibleBlocks[k].tileOffsetZ;
        vertices = &gWaveVertices[gWaveVertexFlip + arg1][spA0 * spA8];
        spA8 = gWaveCellIndexLookup[spA8];
        for (i = 0, vertexIdx = 0; i <= gWaveController.subdivisions; i++) {
            var_s1 = gWaveVisibleBlocks[k].tileOffsetX;
            var_s2 = (var_a0 * gWaveController.tileCount) + var_s1;
            for (j = 0; j <= gWaveController.subdivisions; j++) {
                y = (gWaveHeightTable[gWaveHeightIndices[var_s2].s[0]] +
                     gWaveHeightTable[gWaveHeightIndices[var_s2].s[1]]) *
                    gWaveController.magnitude;
                if (gWaveGenCount > 0) {
                    y += waves_get_y(blockID, j + sp9C, i + sp98);
                }
                y *= gWaveHeightBuffers[spA8][vertexIdx];
                vertices[vertexIdx].y = y;
                y = (y - temp_f26) * var_f28;
                if (y > 1.0f) {
                    y = 1.0f;
                } else if (y < 0) {
                    y = 0;
                }
                var_v0 = (0 * y);
                var_v0 += 0xFF;
                vertices[vertexIdx].r = var_v0;
                vertices[vertexIdx].g = var_v0;
                vertices[vertexIdx].b = var_v0;
                vertices[vertexIdx].a = 0xFF;
                vertexIdx++;
                var_s1++;
                var_s2++;
                if (var_s1 >= gWaveController.tileCount) {
                    var_s1 -= gWaveController.tileCount;
                    var_s2 -= gWaveController.tileCount;
                }
            }
            var_a0++;
            if (var_a0 >= gWaveController.tileCount) {
                var_a0 -= gWaveController.tileCount;
            }
            if (gWaveController.doubleDensity) {
                gWaveVisibleBlocks[k].detailLevelIndex += gWaveController.subdivisions;
            }
        }
    }
}

/**
 * Updates the animations and UV mapping of the waves.
 */
void waves_update(s32 updateRate) {
    s32 i;
    s32 j;
    s32 k;
    s32 var_a2;
    s32 var_ra;
    s32 var_v1;
    s32 var_t5;
    s32 var_t2;
    s32 var_s0;
    s32 k_2;
    s32 j_2;
    s32 i_2;

    gWaveVertexFlip = 1 - gWaveVertexFlip;
    for (i_2 = 0, j_2 = 0; i_2 < gWaveController.tileCount; i_2++) {
        for (k_2 = 0; k_2 < gWaveController.tileCount; k_2++) {
            gWaveHeightIndices[j_2].s[0] += updateRate;
            while (gWaveHeightIndices[j_2].s[0] >= gWaveController.seedSize) {
                gWaveHeightIndices[j_2].s[0] -= gWaveController.seedSize;
            }
            gWaveHeightIndices[j_2].s[1] += updateRate;
            while (gWaveHeightIndices[j_2].s[1] >= gWaveController.seedSize) {
                gWaveHeightIndices[j_2].s[1] -= gWaveController.seedSize;
            }
            j_2++;
        }
    }

    gWaveTexAnimFrame += gWaveTextureHeader->frameAdvanceDelay * updateRate;
    if (gWaveTexAnimFrame < 0) {
        gWaveTexAnimFrame = 0;
    } else {
        while (gWaveTexAnimFrame >= gWaveTextureHeader->numOfTextures) {
            gWaveTexAnimFrame -= gWaveTextureHeader->numOfTextures;
        }
    }

    gWaveUVBaseX = ((gWaveController.uvScrollX * updateRate) + gWaveUVBaseX) & gWaveTexUVMaskX;
    gWaveUVBaseY = ((gWaveController.uvScrollY * updateRate) + gWaveUVBaseY) & gWaveTexUVMaskY;
    var_a2 = gWaveUVBaseY;
    for (i = 0, var_s0 = 0; i <= gWaveController.subdivisions; i++) {
        var_v1 = gWaveUVBaseX;
        for (j = 0; j <= gWaveController.subdivisions; j++) {
            gWaveUVTable[var_s0].u = var_v1;
            gWaveUVTable[var_s0].v = var_a2;
            var_v1 += gWaveUVStepX;
            var_s0++;
        }
        var_a2 += gWaveUVStepY;
    }

    if (gWavePlayerCount != 2) {
        var_t2 = 1;
    } else {
        var_t2 = 2;
    }

    var_s0 = 0;
    var_t5 = 0;
    var_ra = gWaveController.subdivisions + 1;
    for (i = 0; i < gWaveController.subdivisions; i++) {
        for (j = 0; j < gWaveController.subdivisions; j++) {
            for (k = 0; k < var_t2; k++) {
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0].uv0.u = gWaveUVTable[var_t5].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0].uv0.v = gWaveUVTable[var_t5 + 1].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0].uv1.u = gWaveUVTable[var_ra].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0].uv1.v = gWaveUVTable[var_ra].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0].uv2.u = gWaveUVTable[var_t5 + 1].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0].uv2.v = gWaveUVTable[var_t5 + 1].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0 + 1].uv0.u = gWaveUVTable[var_t5 + 1].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0 + 1].uv0.v = gWaveUVTable[var_t5 + 1].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0 + 1].uv1.u = gWaveUVTable[var_ra].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0 + 1].uv1.v = gWaveUVTable[var_ra + 1].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0 + 1].uv2.u = gWaveUVTable[var_ra + 1].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][var_s0 + 1].uv2.v = gWaveUVTable[var_ra + 1].v;
            }
            var_t5++;
            var_ra++;
            var_s0 += 2;
        }
        var_t5++;
        var_ra++;
    }

    gWaveQuadTriangles[2 * gWaveVertexFlip].uv0.u = gWaveUVTable[0].u;
    gWaveQuadTriangles[2 * gWaveVertexFlip].uv0.v = gWaveUVTable[0].v;
    gWaveQuadTriangles[2 * gWaveVertexFlip].uv1.u =
        gWaveUVTable[gWaveController.subdivisions * (gWaveController.subdivisions + 1)].u;
    gWaveQuadTriangles[2 * gWaveVertexFlip].uv1.v =
        gWaveUVTable[gWaveController.subdivisions * (gWaveController.subdivisions + 1)].v;
    gWaveQuadTriangles[2 * gWaveVertexFlip].uv2.u = gWaveUVTable[gWaveController.subdivisions].u;
    gWaveQuadTriangles[2 * gWaveVertexFlip].uv2.v = gWaveUVTable[gWaveController.subdivisions].v;
    gWaveQuadTriangles[2 * gWaveVertexFlip + 1].uv0.u = gWaveUVTable[gWaveController.subdivisions].u;
    gWaveQuadTriangles[2 * gWaveVertexFlip + 1].uv0.v = gWaveUVTable[gWaveController.subdivisions].v;
    gWaveQuadTriangles[2 * gWaveVertexFlip + 1].uv1.u =
        gWaveUVTable[gWaveController.subdivisions * (gWaveController.subdivisions + 1)].u;
    gWaveQuadTriangles[2 * gWaveVertexFlip + 1].uv1.v =
        gWaveUVTable[gWaveController.subdivisions * (gWaveController.subdivisions + 1)].v;
    gWaveQuadTriangles[2 * gWaveVertexFlip + 1].uv2.u =
        gWaveUVTable[gWaveController.subdivisions * (gWaveController.subdivisions + 1) + gWaveController.subdivisions]
            .u;
    gWaveQuadTriangles[2 * gWaveVertexFlip + 1].uv2.v =
        gWaveUVTable[gWaveController.subdivisions * (gWaveController.subdivisions + 1) + gWaveController.subdivisions]
            .v;

    if (gWaveGenCount > 0) {
        wave_gens_update(updateRate);
    }

    if (gWavePowerDivisor <= 0) {
        return;
    }

    if (updateRate < gWavePowerDivisor) {
        gWaveController.magnitude += (f32) updateRate * gWaveMagnitude;
        gWavePowerDivisor -= updateRate;
    } else {
        gWaveController.magnitude = gWavePowerBase;
        gWavePowerDivisor = 0;
    }
}

void wave_blocks_decay_ripples(s32 arg0, s32 arg1) {
    s32 i;
    s32 j;

    arg1 <<= 3;
    for (i = 0; i < gNumberOfLevelSegments; i++) {
        if (gWaveVisibilityGrid[gWaveModel[i].tileZ] & (1 << gWaveModel[i].tileX)) {
            if (gWaveController.doubleDensity) {
                for (j = 0; j < 4; j++) {
                    if (gWaveBlockCells[gWaveModel[i].tileIndex] & (0xFF << (j << 3))) {
                        if (arg1 < gWaveModel[i].viewportFade[arg0].quadrantFade[j]) {
                            gWaveModel[i].viewportFade[arg0].quadrantFade[j] -= arg1;
                        } else {
                            gWaveModel[i].viewportFade[arg0].quadrantFade[j] = 0;
                        }
                    } else {
                        if ((gWaveModel[i].viewportFade[arg0].quadrantFade[j] + arg1) < 0x80) {
                            gWaveModel[i].viewportFade[arg0].quadrantFade[j] += arg1;
                        } else {
                            gWaveModel[i].viewportFade[arg0].quadrantFade[j] = 0x80;
                        }
                    }
                }
            } else {
                if (gWaveBlockCells[gWaveModel[i].tileIndex]) {
                    if (arg1 < gWaveModel[i].viewportFade[arg0].quadrantFade[0]) {
                        gWaveModel[i].viewportFade[arg0].quadrantFade[0] -= arg1;
                    } else {
                        gWaveModel[i].viewportFade[arg0].quadrantFade[0] = 0;
                    }
                } else {
                    if (gWaveModel[i].viewportFade[arg0].quadrantFade[0] + arg1 < 0x80) {
                        gWaveModel[i].viewportFade[arg0].quadrantFade[0] += arg1;
                    } else {
                        gWaveModel[i].viewportFade[arg0].quadrantFade[0] = 0x80;
                    }
                }
            }
        }
    }
}

/**
 * Loads a texture into texture memory.
 * Can offset the texture address in bytes, since waves use multi-texturing.
 */
void wave_load_material(TextureHeader *tex, s32 rtile) {
    s32 txmask;
    s32 tmem;
    u32 texWidth;

    texWidth = tex->width;
    tmem = 0;
    if (texWidth == 16) {
        txmask = 4;
        if (rtile != G_TX_RENDERTILE) {
            tmem = 384;
        }
    } else if (texWidth == 32) {
        txmask = 5;
        if (rtile != G_TX_RENDERTILE) {
            tmem = 256;
        }
    } else {
        //!@bug: txmask is not set, so it's value will be UB. I wonder if they accidentally set the width to 3 instead
        //! of the mask?
        texWidth = 3;
        if (rtile != G_TX_RENDERTILE) {
            tmem = 384;
        }
    }

    // difference is G_IM_SIZ_32b vs G_IM_SIZ_16b
    if (TEX_FORMAT(tex->format) == TEX_FORMAT_RGBA32) {
        gDPLoadMultiBlock(gWaveDL++, OS_K0_TO_PHYSICAL(tex + 1), tmem, rtile, G_IM_FMT_RGBA, G_IM_SIZ_32b, texWidth,
                          texWidth, 0, 0, 0, txmask, txmask, 0, 0);
    } else {
        gDPLoadMultiBlock(gWaveDL++, OS_K0_TO_PHYSICAL(tex + 1), tmem, rtile, G_IM_FMT_RGBA, G_IM_SIZ_16b, texWidth,
                          texWidth, 0, 0, 0, txmask, txmask, 0, 0);
    }
}

/**
 * Renders waves in the current viewport.
 */
void waves_render(Gfx **dList, Mtx **mtx, s32 viewportID) {
    s32 sp11C;
    Vertex *vtx;
    Triangle *tri;
    s32 numTris;
    s32 numVerts;
    s32 j;
    s32 sp104;
    s32 var_fp;
    s32 var_t0;
    ObjectTransform transform;
    LevelModel_Alternate *spE0;
    s32 i;
    TextureHeader *tex1;
    TextureHeader *tex2;

    if (viewportID != VIEWPORT_LAYOUT_2_PLAYERS || gWavePlayerCount != 2) {
        viewportID = 0;
    } else {
        viewportID = 2;
    }

    if (gVisibleWaveTiles > 0) {
        gWaveDL = *dList;
        gWaveMtx = *mtx;
        i = 0;
        rendermode_reset(&gWaveDL);
        gSPSetGeometryMode(gWaveDL++, G_ZBUFFER);
        // Low Quality water. Sets up material data here too.
        if (gWaveController.xlu) {
            gSPClearGeometryMode(gWaveDL++, G_FOG);
            tex1 = set_animated_texture_header(gWaveTextureHeader, gWaveTexAnimFrame * (16 * 16));
            tex2 = set_animated_texture_header(gWaveTexture, gWaveBatch->texOffset * (128 * 128));
            wave_load_material(tex1, 1);
            wave_load_material(tex2, 0);
            gDPSetCombineMode(gWaveDL++, G_CC_BLENDTEX_MODULATEA_1_PRIM, G_CC_BLENDI_ENV_ALPHA_MODULATEA2);
            if (TEX_FORMAT(tex1->format) == TEX_FORMAT_RGBA32 &&
                cam_get_viewport_layout() <= VIEWPORT_LAYOUT_1_PLAYER) {
                gDPSetOtherMode(gWaveDL++, DKR_OMH_2CYC_BILERP, DKR_OML_COMMON | G_RM_AA_ZB_XLU_INTER2);
            } else {
                gDPSetOtherMode(gWaveDL++, DKR_OMH_2CYC_BILERP, DKR_OML_COMMON | G_RM_AA_ZB_OPA_SURF2);
            }
            gDPSetPrimColor(gWaveDL++, 0, 0, 255, 255, 255, 0);
            if (gWavePulseLight != NULL) {
                gDPSetEnvColor(gWaveDL++, gWavePulseLight->rgba.r, gWavePulseLight->rgba.g, gWavePulseLight->rgba.b,
                               gWavePulseLight->rgba.a);
            } else {
                gDPSetEnvColor(gWaveDL++, 255, 255, 255, 0);
            }
        } else {
            gSPSetGeometryMode(gWaveDL++, G_FOG);
            tex1 = set_animated_texture_header(gWaveTexture, gWaveBatch->texOffset * (128 * 128));
            gDkrDmaDisplayList(gWaveDL++, OS_K0_TO_PHYSICAL(tex1->cmd), tex1->numberOfCommands);
            gDPSetCombineMode(gWaveDL++, G_CC_BLENDT_ENV_ALPHA_A_PRIM, G_CC_MODULATEIDECALA2);
            gDPSetOtherMode(gWaveDL++, DKR_OMH_2CYC_BILERP, DKR_OML_COMMON | G_RM_FOG_SHADE_A | G_RM_AA_ZB_OPA_SURF2);
            gDPSetPrimColor(gWaveDL++, 0, 0, 255, 255, 255, 255);
            if (gWavePulseLight != NULL) {
                gDPSetEnvColor(gWaveDL++, gWavePulseLight->rgba.r, gWavePulseLight->rgba.g, gWavePulseLight->rgba.b,
                               gWavePulseLight->rgba.a);
            } else {
                gDPSetEnvColor(gWaveDL++, 255, 255, 255, 0);
            }
        }
        if (gWaveController.doubleDensity) {
            transform.scale = 0.5f;
        } else {
            transform.scale = 1.0f;
        }

        transform.rotation.x = transform.rotation.y = transform.rotation.z = 0;
        // High Quality water
        for (; i < gVisibleWaveTiles; i++) {
            if (gWaveController.xlu) {
                wave_block_build_vertices(gWaveBlockIDs[i], viewportID);
            } else {
                wave_block_build_vertices_depth_shaded(gWaveBlockIDs[i], viewportID);
            }
            spE0 = &gWaveModel[gWaveBlockIDs[i]];
            transform.x_position = spE0->originX;
            transform.y_position = spE0->originY;
            transform.z_position = spE0->originZ;
            sp104 = gWaveBlockCells[spE0->tileIndex];
            if (gWaveController.doubleDensity) {
                for (sp11C = 0; sp11C < 2; sp11C++) {
                    transform.x_position = spE0->originX;
                    for (var_fp = 0; var_fp < 2; var_fp++) {
                        mtx_cam_push(&gWaveDL, &gWaveMtx, &transform, 1.0f, 0.0f);
                        // Tag the tile by identity rather than by the shared stack transform, so the host
                        // can pair its vertices with last frame's and interpolate the wave motion.
                        pc_gfx_mtx_tag(gWaveMtx - 1, spE0, PC_MTX_SITE_WATER);
                        if (sp104 & 0xFF) {
                            numTris = gWaveController.subdivisions << 1;
                            numVerts = gWaveController.subdivisions + 1;
                            var_t0 = ((sp104 & 0xFF) - 1) * numVerts * numVerts;
                            for (j = 0; j < gWaveController.subdivisions; j++) {
                                vtx = &gWaveVertices[gWaveVertexFlip + viewportID][var_t0];
                                tri = &gWaveTriangles[gWaveVertexFlip + viewportID]
                                                     [j * (gWaveController.subdivisions << 1)];

                                gSPVertexDKR(gWaveDL++, OS_K0_TO_PHYSICAL(vtx), numVerts << 1, 0);
                                gSPPolygon(gWaveDL++, OS_K0_TO_PHYSICAL(tri), numTris, TRIN_ENABLE_TEXTURE);

                                var_t0 += gWaveController.subdivisions + 1;
                            }
                        } else {
                            gSPVertexDKR(gWaveDL++, OS_K0_TO_PHYSICAL(&gWaveCornerVertices[gWaveVertexFlip]), 4, 0);
                            gSPPolygon(gWaveDL++, OS_K0_TO_PHYSICAL(&gWaveQuadTriangles[gWaveVertexFlip << 1]), 2,
                                       TRIN_ENABLE_TEXTURE);
                        }
                        mtx_pop(&gWaveDL);
                        sp104 >>= 8;
                        transform.x_position += gWaveBoundingBoxW * 0.5f;
                    }
                    transform.z_position += gWaveBoundingBoxH * 0.5f;
                }
            } else {
                mtx_cam_push(&gWaveDL, &gWaveMtx, &transform, 1.0f, 0.0f);
                pc_gfx_mtx_tag(gWaveMtx - 1, spE0, PC_MTX_SITE_WATER);
                numTris = gWaveController.subdivisions << 1;
                numVerts = gWaveController.subdivisions + 1;
                var_t0 = ((sp104 & 0xFF) - 1) * numVerts * numVerts;
                for (j = 0; j < gWaveController.subdivisions; j++) {
                    vtx = &gWaveVertices[gWaveVertexFlip + viewportID][var_t0];
                    tri = &gWaveTriangles[gWaveVertexFlip + viewportID][j * (gWaveController.subdivisions << 1)];

                    gSPVertexDKR(gWaveDL++, OS_K0_TO_PHYSICAL(vtx), numVerts << 1, 0);
                    gSPPolygon(gWaveDL++, OS_K0_TO_PHYSICAL(tri), numTris, TRIN_ENABLE_TEXTURE);

                    var_t0 += gWaveController.subdivisions + 1;
                }
                mtx_pop(&gWaveDL);
            }
        }
        if (gWaveController.xlu) {
            gSPSetGeometryMode(gWaveDL++, G_FOG);
            gDPSetPrimColor(gWaveDL++, 0, 0, 255, 255, 255, 255);
        }
        *dList = gWaveDL;
        *mtx = gWaveMtx;
    }
    gVisibleWaveTiles = 0;
}
