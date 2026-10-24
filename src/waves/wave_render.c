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

/**
 * Marks the wave cells around a viewpoint as visible in gWaveBlockCells, lists the segments they lie on in
 * gWaveVisibleBlocks, then fades the visible segments' ripples in and the rest out.
 * With double density each segment is four cells, one per quadrant, packed a byte each.
 */
void waves_visibility(s32 xPosition, s32 yPosition, s32 zPosition, s32 currentViewport, s32 updateRate) {
    s32 xPosRatio;
    s32 zPosRatio;
    s32 tempXPosRatio;
    s32 slot;
    s32 xShift;
    s32 col;
    u32 cellIndex;
    s32 cellXShift;
    s32 i;
    s32 zShift;
    s32 visibleCount;
    s32 blockDist;

    xPosRatio = (xPosition - gWaveBlockPosX) / gWaveBoundingBoxDiffX;
    zPosRatio = (zPosition - gWaveBlockPosZ) / gWaveBoundingBoxDiffZ;

    if (0) {}

    for (slot = 0; slot != WAVE_VISIBLE_BLOCKS_BODY_COUNT; slot += 4) {
        gWaveVisibleBlocks[0].blockID = -1;
        gWaveVisibleBlocks[1].blockID = -1;
        gWaveVisibleBlocksBody[slot].blockID = -1;
        gWaveVisibleBlocksBody[slot + 1].blockID = -1;
        gWaveVisibleBlocksBody[slot + 2].blockID = -1;
        gWaveVisibleBlocksBody[slot + 3].blockID = -1;
    }

    if (gWaveController.doubleDensity) {
        xPosition -= (xPosRatio * gWaveBoundingBoxDiffX) + gWaveBlockPosX;
        zPosition -= (zPosRatio * gWaveBoundingBoxDiffZ) + gWaveBlockPosZ;
        if ((gWaveBoundingBoxDiffX >> 1) < xPosition) {
            xShift = 8;
        } else {
            xShift = 0;
        }

        if ((gWaveBoundingBoxDiffZ >> 1) < zPosition) {
            zShift = 16;
        } else {
            zShift = 0;
        }

        for (xShift -= (gWaveController.waveViewDist >> 1) * 8; xShift < 0; xShift += 16) {
            xPosRatio -= 1;
        }

        for (zShift -= (gWaveController.waveViewDist >> 1) * 16; zShift < 0; zShift += 32) {
            zPosRatio -= 1;
        }

        for (blockDist = 0, visibleCount = 0; blockDist < gWaveController.waveViewDist; blockDist++) {
            if (zPosRatio >= 0 && zPosRatio < gWaveTileCountZ) {
                tempXPosRatio = xPosRatio;
                cellXShift = xShift;
                cellIndex = (zPosRatio * gWaveTileCountX) + xPosRatio;
                for (col = 0; col < gWaveController.waveViewDist; col++) {
                    // clang-format off
                    if (
                        (tempXPosRatio >= 0) &&
                        (tempXPosRatio < gWaveTileCountX) &&
                        (gWaveVisibilityGrid[zPosRatio] & (1 << tempXPosRatio))
                    ) {
                        // clang-format on
                        gWaveBlockCells[cellIndex] |= gWaveCellLookup[blockDist * gWaveController.waveViewDist + col]
                                                      << (cellXShift + zShift);
                        for (i = 0; i < gNumberOfLevelSegments; i++) {
                            if (cellIndex == gWaveModel[i].tileIndex) {
                                gWaveVisibleBlocks[visibleCount].blockID = i;
                                gWaveVisibleBlocks[visibleCount].quadrant = (cellXShift + zShift) >> 3;
                                gWaveVisibleBlocks[visibleCount].detailLevelIndex = i * gWaveTileTotal;
                                gWaveVisibleBlocks[visibleCount].tileOffsetX = gWaveModel[i].waveTileOffsetX;
                                if (cellXShift != 0) {
                                    gWaveVisibleBlocks[visibleCount].detailLevelIndex += gWaveController.subdivisions;
                                    gWaveVisibleBlocks[visibleCount].tileOffsetX += gWaveController.subdivisions;
                                    while (gWaveVisibleBlocks[visibleCount].tileOffsetX >= gWaveController.tileCount) {
                                        gWaveVisibleBlocks[visibleCount].tileOffsetX -= gWaveController.tileCount;
                                    }
                                }
                                gWaveVisibleBlocks[visibleCount].tileOffsetZ = gWaveModel[i].waveTileOffsetZ;
                                if (zShift != 0) {
                                    gWaveVisibleBlocks[visibleCount].detailLevelIndex +=
                                        ((gWaveController.subdivisions * 2) + 1) * gWaveController.subdivisions;
                                    gWaveVisibleBlocks[visibleCount].tileOffsetZ += gWaveController.subdivisions;
                                    while (gWaveVisibleBlocks[visibleCount].tileOffsetZ >= gWaveController.tileCount) {
                                        gWaveVisibleBlocks[visibleCount].tileOffsetZ -= gWaveController.tileCount;
                                    }
                                }
                                visibleCount++;
                                i = 0x7FFF;
                            }
                        }
                    }
                    cellXShift += 8;
                    if (cellXShift > 8) {
                        cellXShift -= 16;
                        tempXPosRatio++;
                        cellIndex++;
                    }
                }
            }
            zShift += 16;
            if (zShift > 16) {
                zShift -= 32;
                zPosRatio++;
            }
        }
    } else {
        xPosRatio -= gWaveController.waveViewDist >> 1;
        zPosRatio -= gWaveController.waveViewDist >> 1;
        for (blockDist = 0, visibleCount = 0; blockDist < gWaveController.waveViewDist; blockDist++, zPosRatio++) {
            if ((zPosRatio >= 0) && (zPosRatio < gWaveTileCountZ)) {
                tempXPosRatio = xPosRatio;
                cellIndex = (zPosRatio * gWaveTileCountX) + xPosRatio;
                for (col = 0; col < gWaveController.waveViewDist; col++, tempXPosRatio++, cellIndex++) {
                    // clang-format off
                    if (
                        (tempXPosRatio >= 0) &&
                        (tempXPosRatio < gWaveTileCountX) &&
                        (gWaveVisibilityGrid[zPosRatio] & (1 << tempXPosRatio))
                    ) {
                        // clang-format on
                        gWaveBlockCells[cellIndex] = gWaveCellLookup[blockDist * gWaveController.waveViewDist + col];
                        for (i = 0; i < gNumberOfLevelSegments; i++) {
                            if (cellIndex == gWaveModel[i].tileIndex) {
                                gWaveVisibleBlocks[visibleCount].blockID = i;
                                gWaveVisibleBlocks[visibleCount].quadrant = 0;
                                gWaveVisibleBlocks[visibleCount].tileOffsetX = gWaveModel[i].waveTileOffsetX;
                                gWaveVisibleBlocks[visibleCount].tileOffsetZ = gWaveModel[i].waveTileOffsetZ;
                                gWaveVisibleBlocks[visibleCount].detailLevelIndex = i * gWaveTileTotal;
                                visibleCount++;
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

/**
 * Sets the height of every vertex of a segment's visible wave cells, for translucent water.
 * Vertex colour and alpha are shaded by water depth and wave height, and by how far the segment's ripples
 * have faded out.
 */
void wave_block_build_vertices(s32 blockID, s32 viewportID) {
    s32 i;
    s32 heightIndex;
    s32 j;
    f32 detailScaleStep;
    f32 vertexY;
    s32 cellSlot;
    s32 heightBuffer;
    s32 shade;
    s32 cellVertexCount;
    s32 genOffsetX;
    s32 genOffsetZ;
    s32 tileZ;
    s32 vertexCol;
    s32 vertexIdx;
    s32 tileX;
    s32 k;
    Vertex *vertices;
    LevelModel_Alternate *model;

    detailScaleStep = (1.0f - gWaveController.minDetailScale) / 255.0f;
    model = &gWaveModel[blockID];
    cellVertexCount = (gWaveController.subdivisions + 1) * (gWaveController.subdivisions + 1);
    for (k = 0; gWaveVisibleBlocks[k].blockID != -1; k++) {
        if (blockID != gWaveVisibleBlocks[k].blockID) {
            continue;
        }

        if (gWaveController.doubleDensity) {
            cellSlot = (((u32) gWaveBlockCells[model->tileIndex] >> (gWaveVisibleBlocks[k].quadrant * 8)) & 0xFF) - 1;
            genOffsetX = (gWaveVisibleBlocks[k].quadrant & 1) * gWaveController.subdivisions;
            genOffsetZ = ((gWaveVisibleBlocks[k].quadrant & 2) >> 1) * gWaveController.subdivisions;
        } else {
            cellSlot = (gWaveBlockCells[model->tileIndex] & 0xFF) - 1;
            genOffsetX = 0;
            genOffsetZ = 0;
        }

        tileZ = gWaveVisibleBlocks[k].tileOffsetZ;
        vertices = &gWaveVertices[gWaveVertexFlip + viewportID][cellVertexCount * cellSlot];
        heightBuffer = gWaveCellIndexLookup[cellSlot];
        vertexIdx = 0;
        for (i = 0; i <= gWaveController.subdivisions; i++) {
            tileX = gWaveVisibleBlocks[k].tileOffsetX;
            heightIndex = (tileZ * gWaveController.tileCount) + tileX;
            for (j = 0; j <= gWaveController.subdivisions; j++) {
                vertexY = (gWaveHeightTable[gWaveHeightIndices[heightIndex].s[0]] +
                           gWaveHeightTable[gWaveHeightIndices[heightIndex].s[1]]) *
                          gWaveController.magnitude;
                if (gWaveGenCount > 0) {
                    vertexY += waves_get_y(blockID, j + genOffsetX, i + genOffsetZ);
                }
                vertexY *= gWaveHeightBuffers[heightBuffer][vertexIdx];
                shade = gWaveDetailLevels[gWaveVisibleBlocks[k].detailLevelIndex];

                // clang-format off
                shade <<= 1; \
                gWaveVisibleBlocks[k].detailLevelIndex++;
                // clang-format on
                if (shade < 255) {
                    vertexY *= gWaveController.minDetailScale + ((f32) shade * detailScaleStep);
                }
                shade += (s32) (vertexY * gWaveController.heightColourScale);
                if (shade > 255) {
                    shade = 255;
                } else if (shade < 0) {
                    shade = 0;
                }
                shade += ((255 - shade) *
                          model->viewportFade[viewportID >> 1].quadrantFade[gWaveVisibleBlocks[k].quadrant]) >>
                         7; // viewportFade runs 0 to WAVE_FADE_MAX
                vertices[vertexIdx].y = vertexY;

                vertexCol = shade < 192 ? 255 : ((255 - shade) * 4) & 255;
                vertices[vertexIdx].r = vertexCol;
                vertices[vertexIdx].g = vertexCol;
                vertices[vertexIdx].b = vertexCol;

                vertexCol = shade < 64 ? (shade * 4) & 255 : 255;
                vertices[vertexIdx].a = vertexCol;
                vertexIdx++;
                tileX++;
                heightIndex++;
                if (tileX >= gWaveController.tileCount) {
                    tileX -= gWaveController.tileCount;
                    heightIndex -= gWaveController.tileCount;
                }
            }
            tileZ++;
            if (tileZ >= gWaveController.tileCount) {
                tileZ -= gWaveController.tileCount;
            }
            if (gWaveController.doubleDensity) {
                gWaveVisibleBlocks[k].detailLevelIndex += gWaveController.subdivisions;
            }
        }
    }
}

/**
 * Sets the height of every vertex of a segment's visible wave cells, for opaque water.
 * Vertices are always opaque white; the height-based shade is computed but multiplied by zero.
 */
void wave_block_build_vertices_depth_shaded(s32 blockID, s32 viewportID) {
    s32 shade;
    f32 heightRange;
    f32 lowY;
    s32 vertexIdx;
    s32 cellSlot;
    s32 heightBuffer;
    f32 y;
    s32 cellVertexCount;
    s32 genOffsetX;
    s32 genOffsetZ;
    s32 tileZ;
    f32 invHeightRange;
    s32 tileX;
    s32 i;
    s32 heightIndex;
    s32 j;
    Vertex *vertices;
    LevelModel_Alternate *model;
    s32 k;

    k = 0;
    heightRange = (gWaveUpperY * 2.0f) - (gWaveLowerY * 2.0f);
    cellVertexCount = (gWaveController.subdivisions + 1) * (gWaveController.subdivisions + 1);
    model = &gWaveModel[blockID];
    invHeightRange = heightRange <= 0 ? 0 : 1.0f / heightRange;

    lowY = gWaveLowerY * 2.0f;
    for (; gWaveVisibleBlocks[k].blockID != -1; k++) {
        if (blockID != gWaveVisibleBlocks[k].blockID) {
            continue;
        }

        if (gWaveController.doubleDensity) {
            cellSlot = (((u32) gWaveBlockCells[model->tileIndex] >> (gWaveVisibleBlocks[k].quadrant * 8)) & 0xFF) - 1;
            genOffsetX = (gWaveVisibleBlocks[k].quadrant & 1) * gWaveController.subdivisions;
            genOffsetZ = ((s32) (gWaveVisibleBlocks[k].quadrant & 2) >> 1) * gWaveController.subdivisions;
        } else {
            cellSlot = (gWaveBlockCells[model->tileIndex] & 0xFF) - 1;
            genOffsetX = 0;
            genOffsetZ = 0;
        }

        tileZ = gWaveVisibleBlocks[k].tileOffsetZ;
        vertices = &gWaveVertices[gWaveVertexFlip + viewportID][cellVertexCount * cellSlot];
        heightBuffer = gWaveCellIndexLookup[cellSlot];
        for (i = 0, vertexIdx = 0; i <= gWaveController.subdivisions; i++) {
            tileX = gWaveVisibleBlocks[k].tileOffsetX;
            heightIndex = (tileZ * gWaveController.tileCount) + tileX;
            for (j = 0; j <= gWaveController.subdivisions; j++) {
                y = (gWaveHeightTable[gWaveHeightIndices[heightIndex].s[0]] +
                     gWaveHeightTable[gWaveHeightIndices[heightIndex].s[1]]) *
                    gWaveController.magnitude;
                if (gWaveGenCount > 0) {
                    y += waves_get_y(blockID, j + genOffsetX, i + genOffsetZ);
                }
                y *= gWaveHeightBuffers[heightBuffer][vertexIdx];
                vertices[vertexIdx].y = y;
                y = (y - lowY) * invHeightRange;
                if (y > 1.0f) {
                    y = 1.0f;
                } else if (y < 0) {
                    y = 0;
                }
                shade = (0 * y);
                shade += 0xFF;
                vertices[vertexIdx].r = shade;
                vertices[vertexIdx].g = shade;
                vertices[vertexIdx].b = shade;
                vertices[vertexIdx].a = 0xFF;
                vertexIdx++;
                tileX++;
                heightIndex++;
                if (tileX >= gWaveController.tileCount) {
                    tileX -= gWaveController.tileCount;
                    heightIndex -= gWaveController.tileCount;
                }
            }
            tileZ++;
            if (tileZ >= gWaveController.tileCount) {
                tileZ -= gWaveController.tileCount;
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
    s32 uvV;
    s32 nextRowIdx;
    s32 uvU;
    s32 uvIdx;
    s32 bufferPairs;
    s32 idx;
    s32 col;
    s32 tileIdx;
    s32 row;

    gWaveVertexFlip = 1 - gWaveVertexFlip;
    for (row = 0, tileIdx = 0; row < gWaveController.tileCount; row++) {
        for (col = 0; col < gWaveController.tileCount; col++) {
            gWaveHeightIndices[tileIdx].s[0] += updateRate;
            while (gWaveHeightIndices[tileIdx].s[0] >= gWaveController.seedSize) {
                gWaveHeightIndices[tileIdx].s[0] -= gWaveController.seedSize;
            }
            gWaveHeightIndices[tileIdx].s[1] += updateRate;
            while (gWaveHeightIndices[tileIdx].s[1] >= gWaveController.seedSize) {
                gWaveHeightIndices[tileIdx].s[1] -= gWaveController.seedSize;
            }
            tileIdx++;
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
    uvV = gWaveUVBaseY;
    for (i = 0, idx = 0; i <= gWaveController.subdivisions; i++) {
        uvU = gWaveUVBaseX;
        for (j = 0; j <= gWaveController.subdivisions; j++) {
            gWaveUVTable[idx].u = uvU;
            gWaveUVTable[idx].v = uvV;
            uvU += gWaveUVStepX;
            idx++;
        }
        uvV += gWaveUVStepY;
    }

    if (gWavePlayerCount != 2) {
        bufferPairs = 1;
    } else {
        bufferPairs = 2;
    }

    idx = 0;
    uvIdx = 0;
    nextRowIdx = gWaveController.subdivisions + 1;
    for (i = 0; i < gWaveController.subdivisions; i++) {
        for (j = 0; j < gWaveController.subdivisions; j++) {
            for (k = 0; k < bufferPairs; k++) {
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx].uv0.u = gWaveUVTable[uvIdx].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx].uv0.v = gWaveUVTable[uvIdx + 1].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx].uv1.u = gWaveUVTable[nextRowIdx].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx].uv1.v = gWaveUVTable[nextRowIdx].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx].uv2.u = gWaveUVTable[uvIdx + 1].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx].uv2.v = gWaveUVTable[uvIdx + 1].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx + 1].uv0.u = gWaveUVTable[uvIdx + 1].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx + 1].uv0.v = gWaveUVTable[uvIdx + 1].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx + 1].uv1.u = gWaveUVTable[nextRowIdx].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx + 1].uv1.v = gWaveUVTable[nextRowIdx + 1].v;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx + 1].uv2.u = gWaveUVTable[nextRowIdx + 1].u;
                gWaveTriangles[gWaveVertexFlip + (k << 1)][idx + 1].uv2.v = gWaveUVTable[nextRowIdx + 1].v;
            }
            uvIdx++;
            nextRowIdx++;
            idx += 2;
        }
        uvIdx++;
        nextRowIdx++;
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

/**
 * Fades the ripples of each wavy segment for a viewport: in on the cells now visible, out on the rest.
 */
void wave_blocks_decay_ripples(s32 viewportID, s32 updateRate) {
    s32 i;
    s32 j;

    updateRate <<= 3; // Fade by 8 a frame.
    for (i = 0; i < gNumberOfLevelSegments; i++) {
        if (gWaveVisibilityGrid[gWaveModel[i].tileZ] & (1 << gWaveModel[i].tileX)) {
            if (gWaveController.doubleDensity) {
                for (j = 0; j < 4; j++) {
                    if (gWaveBlockCells[gWaveModel[i].tileIndex] & (0xFF << (j << 3))) {
                        if (updateRate < gWaveModel[i].viewportFade[viewportID].quadrantFade[j]) {
                            gWaveModel[i].viewportFade[viewportID].quadrantFade[j] -= updateRate;
                        } else {
                            gWaveModel[i].viewportFade[viewportID].quadrantFade[j] = 0;
                        }
                    } else {
                        if ((gWaveModel[i].viewportFade[viewportID].quadrantFade[j] + updateRate) < WAVE_FADE_MAX) {
                            gWaveModel[i].viewportFade[viewportID].quadrantFade[j] += updateRate;
                        } else {
                            gWaveModel[i].viewportFade[viewportID].quadrantFade[j] = WAVE_FADE_MAX;
                        }
                    }
                }
            } else {
                if (gWaveBlockCells[gWaveModel[i].tileIndex]) {
                    if (updateRate < gWaveModel[i].viewportFade[viewportID].quadrantFade[0]) {
                        gWaveModel[i].viewportFade[viewportID].quadrantFade[0] -= updateRate;
                    } else {
                        gWaveModel[i].viewportFade[viewportID].quadrantFade[0] = 0;
                    }
                } else {
                    if (gWaveModel[i].viewportFade[viewportID].quadrantFade[0] + updateRate < WAVE_FADE_MAX) {
                        gWaveModel[i].viewportFade[viewportID].quadrantFade[0] += updateRate;
                    } else {
                        gWaveModel[i].viewportFade[viewportID].quadrantFade[0] = WAVE_FADE_MAX;
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
    s32 row;
    Vertex *vtx;
    Triangle *tri;
    s32 numTris;
    s32 numVerts;
    s32 j;
    s32 cellMask;
    s32 col;
    s32 vertexOffset;
    ObjectTransform transform;
    LevelModel_Alternate *model;
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
            model = &gWaveModel[gWaveBlockIDs[i]];
            transform.x_position = model->originX;
            transform.y_position = model->originY;
            transform.z_position = model->originZ;
            cellMask = gWaveBlockCells[model->tileIndex];
            if (gWaveController.doubleDensity) {
                for (row = 0; row < 2; row++) {
                    transform.x_position = model->originX;
                    for (col = 0; col < 2; col++) {
                        mtx_cam_push(&gWaveDL, &gWaveMtx, &transform, 1.0f, 0.0f);
                        // Tag the tile by identity rather than by the shared stack transform, so the host
                        // can pair its vertices with last frame's and interpolate the wave motion.
                        pc_gfx_mtx_tag(gWaveMtx - 1, model, PC_MTX_SITE_WATER);
                        if (cellMask & 0xFF) {
                            numTris = gWaveController.subdivisions << 1;
                            numVerts = gWaveController.subdivisions + 1;
                            vertexOffset = ((cellMask & 0xFF) - 1) * numVerts * numVerts;
                            for (j = 0; j < gWaveController.subdivisions; j++) {
                                vtx = &gWaveVertices[gWaveVertexFlip + viewportID][vertexOffset];
                                tri = &gWaveTriangles[gWaveVertexFlip + viewportID]
                                                     [j * (gWaveController.subdivisions << 1)];

                                gSPVertexDKR(gWaveDL++, OS_K0_TO_PHYSICAL(vtx), numVerts << 1, 0);
                                gSPPolygon(gWaveDL++, OS_K0_TO_PHYSICAL(tri), numTris, TRIN_ENABLE_TEXTURE);

                                vertexOffset += gWaveController.subdivisions + 1;
                            }
                        } else {
                            gSPVertexDKR(gWaveDL++, OS_K0_TO_PHYSICAL(&gWaveCornerVertices[gWaveVertexFlip]), 4, 0);
                            gSPPolygon(gWaveDL++, OS_K0_TO_PHYSICAL(&gWaveQuadTriangles[gWaveVertexFlip << 1]), 2,
                                       TRIN_ENABLE_TEXTURE);
                        }
                        mtx_pop(&gWaveDL);
                        cellMask >>= 8;
                        transform.x_position += gWaveBoundingBoxW * 0.5f;
                    }
                    transform.z_position += gWaveBoundingBoxH * 0.5f;
                }
            } else {
                mtx_cam_push(&gWaveDL, &gWaveMtx, &transform, 1.0f, 0.0f);
                pc_gfx_mtx_tag(gWaveMtx - 1, model, PC_MTX_SITE_WATER);
                numTris = gWaveController.subdivisions << 1;
                numVerts = gWaveController.subdivisions + 1;
                vertexOffset = ((cellMask & 0xFF) - 1) * numVerts * numVerts;
                for (j = 0; j < gWaveController.subdivisions; j++) {
                    vtx = &gWaveVertices[gWaveVertexFlip + viewportID][vertexOffset];
                    tri = &gWaveTriangles[gWaveVertexFlip + viewportID][j * (gWaveController.subdivisions << 1)];

                    gSPVertexDKR(gWaveDL++, OS_K0_TO_PHYSICAL(vtx), numVerts << 1, 0);
                    gSPPolygon(gWaveDL++, OS_K0_TO_PHYSICAL(tri), numTris, TRIN_ENABLE_TEXTURE);

                    vertexOffset += gWaveController.subdivisions + 1;
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
