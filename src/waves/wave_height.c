#include "waves_internal.h"

/** The wave surface height and normal at a point of a segment. */
f32 wave_height_at(s32 arg0, f32 arg1, f32 arg2, Vec3f *arg3) {
    f32 spA4;
    f32 spA0;
    f32 var_f12;
    f32 var_f2;
    f32 sp94;
    f32 sp90;
    f32 var_f16;
    f32 arg3X;
    f32 arg3Y;
    f32 arg3Z;
    s32 var_a0;
    f32 sp78;
    s32 var_v0;
    s32 sp70;
    s32 sp6C;
    s32 var_t0;
    s32 var_a3;
    s32 sp60;
    u8 *tempD_800E3178;
    s32 sp58;
    s32 pad;

    if (arg0 < 0 || arg0 >= gNumberOfLevelSegments) {
        arg0 = 0;
    }

    sp78 = gWaveModel[arg0].originY;

    sp90 = gWaveVtxStepZ;
    sp94 = gWaveVtxStepX;
    var_f16 = (1.0f - gWaveController.minDetailScale) / 127.0f;
    sp58 = arg0 * gWaveTileTotal;

    if (gWaveController.doubleDensity) {
        sp90 /= 2.0f;
        sp94 /= 2.0f;
        sp60 = (gWaveController.subdivisions * 2) + 1;
    } else {
        sp60 = gWaveController.subdivisions + 1;
    }

    arg1 -= gWaveModel[arg0].originX;
    if (arg1 < 0.0f) {
        arg1 = 0.0f;
    } else if (gWaveBoundingBoxW <= arg1) {
        arg1 = gWaveBoundingBoxW - 1; // needs to be 1 instead of 1.0f
    }

    arg2 -= gWaveModel[arg0].originZ;
    if (arg2 < 0.0f) {
        arg2 = 0.0f;
    } else if (gWaveBoundingBoxH <= arg2) {
        arg2 = gWaveBoundingBoxH - 1; // needs to be 1 instead of 1.0f
    }

    sp70 = arg1 / sp94;
    sp6C = arg2 / sp90;
    arg1 -= sp70 * sp94;
    arg2 -= sp6C * sp90;

    var_t0 = gWaveModel[arg0].waveTileOffsetX + sp70;
    while (var_t0 >= gWaveController.tileCount) {
        var_t0 -= gWaveController.tileCount;
    }

    var_a3 = gWaveModel[arg0].waveTileOffsetZ + sp6C;
    while (var_a3 >= gWaveController.tileCount) {
        var_a3 -= gWaveController.tileCount;
    }

    var_v0 = 0;
    if (arg2 != sp90 && arg1 < (((sp90 - arg2) / sp90) * sp94)) {
        var_v0 = 1;
    }

    if (var_v0) {
        var_a0 = (var_a3 * gWaveController.tileCount) + var_t0;
        spA0 = (gWaveHeightTable[gWaveHeightIndices[var_a0].s[0]] + gWaveHeightTable[gWaveHeightIndices[var_a0].s[1]]) *
               gWaveController.magnitude;
        var_a0 = sp58 + sp70 + (sp6C * sp60);
        if (gWaveGenCount > 0) {
            spA0 += waves_get_y(arg0, sp70, sp6C);
        }

        var_v0 = gWaveDetailLevels[var_a0];
        if (gWaveDetailLevels[var_a0] < 0x7F) {
            spA0 *= gWaveController.minDetailScale + (var_v0 * var_f16);
        }

        var_a0 =
            (var_a3 + 1) >= gWaveController.tileCount ? var_t0 : ((var_a3 + 1) * gWaveController.tileCount) + var_t0;
        var_f12 =
            (gWaveHeightTable[gWaveHeightIndices[var_a0].s[0]] + gWaveHeightTable[gWaveHeightIndices[var_a0].s[1]]) *
            gWaveController.magnitude;
        if (gWaveGenCount > 0) {
            var_f12 += waves_get_y(arg0, sp70, sp6C + 1);
        }
        var_a0 = sp58 + sp70 + ((sp6C + 1) * sp60);
        var_v0 = gWaveDetailLevels[var_a0];
        if (gWaveDetailLevels[var_a0] < 0x7F) {
            var_f12 *= gWaveController.minDetailScale + (var_v0 * var_f16);
        }

        if ((var_t0 + 1) >= gWaveController.tileCount) {
            var_a0 = var_a3 * gWaveController.tileCount;
        } else {
            var_a0 = (var_t0 + 0) + (var_a3 * gWaveController.tileCount) + 1;
        }
        var_f2 =
            (gWaveHeightTable[gWaveHeightIndices[var_a0].s[0]] + gWaveHeightTable[gWaveHeightIndices[var_a0].s[1]]) *
            gWaveController.magnitude;
        if (gWaveGenCount > 0) {
            var_f2 += waves_get_y(arg0, sp70 + 1, sp6C);
        }
        var_a0 = sp58 + sp70 + (sp6C * sp60);
        var_a0++;
        var_v0 = gWaveDetailLevels[var_a0];
        if (gWaveDetailLevels[var_a0] < 0x7F) {
            var_f2 *= gWaveController.minDetailScale + (var_v0 * var_f16);
        }

        if (gWaveController.doubleDensity) {
            spA0 /= 2.0f;
            var_f12 /= 2.0f;
            var_f2 /= 2.0f;
        }

        spA4 = 0.0f;
        arg3X = (spA0 - var_f2) * sp90;
        arg3Y = sp90 * sp94;
        arg3Z = (spA0 - var_f12) * sp94;
    } else {
        if ((var_t0 + 1) >= gWaveController.tileCount) {
            var_a0 = var_a3 * gWaveController.tileCount;
        } else {
            var_a0 = (var_t0 + 0) + (var_a3 * gWaveController.tileCount) + 1;
        }
        spA0 = (gWaveHeightTable[gWaveHeightIndices[var_a0].s[0]] + gWaveHeightTable[gWaveHeightIndices[var_a0].s[1]]) *
               gWaveController.magnitude;
        if (gWaveGenCount > 0) {
            spA0 += waves_get_y(arg0, sp70 + 1, sp6C);
        }
        var_a0 = sp58 + sp70 + (sp6C * sp60);
        var_a0++;
        var_v0 = gWaveDetailLevels[var_a0];
        if (gWaveDetailLevels[var_a0] < 0x7F) {
            spA0 *= gWaveController.minDetailScale + (var_v0 * var_f16);
        }

        var_a0 =
            (var_a3 + 1) >= gWaveController.tileCount ? var_t0 : ((var_a3 + 1) * gWaveController.tileCount) + var_t0;
        var_f12 =
            (gWaveHeightTable[gWaveHeightIndices[var_a0].s[0]] + gWaveHeightTable[gWaveHeightIndices[var_a0].s[1]]) *
            gWaveController.magnitude;
        if (gWaveGenCount > 0) {
            var_f12 += waves_get_y(arg0, sp70, sp6C + 1);
        }
        var_a0 = sp58 + sp70 + ((sp6C + 1) * sp60);
        var_v0 = (tempD_800E3178 = gWaveDetailLevels)[var_a0];
        if (var_v0 < 0x7F) {
            var_f12 *= gWaveController.minDetailScale + (var_v0 * var_f16);
        }

        var_a0 = (var_t0 + 1) >= gWaveController.tileCount ? 0 : (var_t0 + 1);
        if ((var_a3 + 1) < gWaveController.tileCount) {
            var_a0 += (var_a3 + 1) * gWaveController.tileCount;
        }

        var_f2 =
            (gWaveHeightTable[gWaveHeightIndices[var_a0].s[0]] + gWaveHeightTable[gWaveHeightIndices[var_a0].s[1]]) *
            gWaveController.magnitude;
        if (gWaveGenCount > 0) {
            var_f2 += waves_get_y(arg0, sp70 + 1, sp6C + 1);
        }
        var_a0 = sp58 + sp70 + ((sp6C + 1) * sp60);
        var_a0++;
        var_v0 = gWaveDetailLevels[var_a0];
        if (var_v0 < 0x7F) {
            var_f2 *= gWaveController.minDetailScale + (var_v0 * var_f16);
        }

        if (gWaveController.doubleDensity) {
            spA0 /= 2.0f;
            var_f12 /= 2.0f;
            var_f2 /= 2.0f;
        }

        spA4 = sp94;
        arg3X = sp90 * (var_f12 - var_f2);
        arg3Y = sp90 * sp94;
        arg3Z = sp94 * (spA0 - var_f2);
    }

    var_f16 = sqrtf((arg3X * arg3X) + (arg3Y * arg3Y) + (arg3Z * arg3Z));
    if (var_f16 != 0.0 && arg3Y != 0.0) {
        arg3X /= var_f16;
        arg3Y /= var_f16;
        arg3Z /= var_f16;
        sp78 -= (((arg3X * arg1) + (arg3Z * arg2)) - ((spA4 * arg3X) + (spA0 * arg3Y))) / arg3Y;
    }

    if (arg3 != NULL) {
        arg3->x = arg3X;
        arg3->y = arg3Y;
        arg3->z = arg3Z;
    }

    return sp78;
}

s32 wave_cells_in_rect(s32 arg0, ShadowWaveTri *arg1, ShadowPlane *arg2, f32 shadowXNegPosition, f32 shadowZNegPosition,
                       f32 shadowXPosition, f32 shadowZPosition) {
    s32 colCount;
    s32 startCol;
    s32 startRow;
    s32 currentRow;
    s32 currentCol;
    s32 endCol;
    s32 endRow;
    s32 colOffset;
    s32 rowOffset;
    s32 counter;
    f32 temp_fs0;
    f32 temp_fs1;
    f32 temp_fs3;
    f32 var_fs0;
    f32 var_fs1;
    s16 sp332;
    s16 spD8[300];
    s16 temp_fp;
    s16 temp_s2;
    s16 var_s3_2;
    f32 stepX;
    f32 stepZ;
    f32 temp_fs2;
    s32 var_s0;
    s32 var_s2;
    s32 var_s4;
    s32 temp_t0;

    temp_t0 = arg0 * gWaveTileTotal;
    stepX = gWaveVtxStepX;
    stepZ = gWaveVtxStepZ;
    temp_fs2 = (1.0f - gWaveController.minDetailScale) / 127.0f;
    if (gWaveController.doubleDensity != 0) {
        var_fs1 = 0.5f;
        stepX *= 0.5;
        stepZ *= 0.5;
        colCount = gWaveController.subdivisions * 2;
    } else {
        var_fs1 = 1.0f;
        colCount = gWaveController.subdivisions;
    }

    shadowXNegPosition -= gWaveModel[arg0].originX;
    shadowZNegPosition -= gWaveModel[arg0].originZ;
    shadowXPosition -= gWaveModel[arg0].originX;
    shadowZPosition -= gWaveModel[arg0].originZ;
    if (shadowXNegPosition < 0.0f) {
        startCol = 0;
    } else {
        startCol = (s32) (shadowXNegPosition / stepX);
    }
    if (shadowZNegPosition < 0.0f) {
        startRow = 0;
    } else {
        startRow = (s32) (shadowZNegPosition / stepZ);
    }
    if (gWaveBoundingBoxW <= shadowXPosition) {
        endCol = colCount;
    } else {
        endCol = (s32) (shadowXPosition / stepX) + 1;
    }
    if (gWaveBoundingBoxH <= shadowZPosition) {
        endRow = colCount;
    } else {
        endRow = (s32) (shadowZPosition / stepX) + 1;
    }
    colOffset = gWaveModel[arg0].waveTileOffsetX + startCol;
    while (colOffset >= gWaveController.tileCount) {
        colOffset -= gWaveController.tileCount;
    }
    rowOffset = gWaveModel[arg0].waveTileOffsetZ + startRow;
    while (rowOffset >= gWaveController.tileCount) {
        rowOffset -= gWaveController.tileCount;
    }

    for (currentRow = startRow, counter = 0; currentRow <= endRow; currentRow++) {
        var_s0 = colOffset;

        var_s2 = (rowOffset * gWaveController.tileCount) + var_s0;
        var_s4 = temp_t0 + startCol + (currentRow * colCount);
        for (currentCol = startCol; currentCol <= endCol; currentCol++) {

            var_fs0 =
                (gWaveHeightTable[gWaveHeightIndices[var_s2].x] + gWaveHeightTable[gWaveHeightIndices[var_s2].y]) *
                gWaveController.magnitude;
            if (gWaveGenCount > 0) {
                var_fs0 += waves_get_y(arg0, currentCol, currentRow);
            }

            if (gWaveDetailLevels[var_s4] < 0x7F) {
                var_fs0 *= gWaveController.minDetailScale + ((s32) gWaveDetailLevels[var_s4] * temp_fs2);
            }
            spD8[counter] = gWaveModel[arg0].originY + (s16) (var_fs0 * var_fs1);
            counter++;
            var_s0++;
            var_s4++;
            var_s2++;
            if (var_s0 >= gWaveController.tileCount) {
                var_s0 -= gWaveController.tileCount;
                var_s2 -= gWaveController.tileCount;
            }
        }
        rowOffset++;
        if (rowOffset >= gWaveController.tileCount) {
            rowOffset -= gWaveController.tileCount;
        }
    }

    // FAKE
    temp_fs3 = (stepX * stepZ) * (stepX * stepZ);
    colCount = (endCol - startCol) + 1;
    sp332 = gWaveModel[arg0].originZ + (s16) (startRow * stepZ);

    for (currentRow = startRow, counter = 0; currentRow < endRow; currentRow++) {
        temp_fp = gWaveModel[arg0].originZ + (s16) ((currentRow + 1) * stepZ);
        var_s3_2 = gWaveModel[arg0].originX + (s16) (startCol * stepX);

        for (currentCol = startCol; currentCol < endCol; currentCol++) {
            temp_s2 = gWaveModel[arg0].originX + (s16) ((currentCol + 1) * stepX);

            arg1[counter].x1 = var_s3_2;
            arg1[counter].x2 = var_s3_2;
            arg1[counter].x3 = temp_s2;
            arg1[counter].y1 = spD8[(currentCol - startCol) + ((currentRow - startRow) * colCount)];
            arg1[counter].y2 = spD8[(currentCol - startCol) + (((currentRow - startRow) + 1) * colCount)];
            arg1[counter].y3 = spD8[((currentCol - startCol) + 1) + ((currentRow - startRow) * colCount)];
            arg1[counter].z1 = sp332;
            arg1[counter].z2 = temp_fp;
            arg1[counter].z3 = sp332;
            temp_fs0 = (arg1[counter].y1 - arg1[counter].y3) * stepZ;
            temp_fs3 = stepX * stepZ;
            temp_fs1 = (arg1[counter].y1 - arg1[counter].y2) * stepX;
            temp_fs2 = sqrtf((temp_fs0 * temp_fs0) + (temp_fs3 * temp_fs3) + (temp_fs1 * temp_fs1));
            if (temp_fs2 > 0.0f) {
                temp_fs2 = 1 / temp_fs2;
                arg2[counter].x = temp_fs0 * temp_fs2;
                arg2[counter].y = temp_fs3 * temp_fs2;
                arg2[counter].z = temp_fs1 * temp_fs2;

                arg2[counter].planeD.w = -((arg1[counter].x1 * arg2[counter].x) + (arg1[counter].y1 * arg2[counter].y) +
                                           (arg1[counter].z1 * arg2[counter].z));
                counter++;
            }
            arg1[counter].x1 = temp_s2;
            arg1[counter].x2 = var_s3_2;
            arg1[counter].x3 = temp_s2;
            arg1[counter].y1 = spD8[((currentCol - startCol) + 1) + ((currentRow - startRow) * colCount)];
            arg1[counter].y2 = spD8[(currentCol - startCol) + (((currentRow - startRow) + 1) * colCount)];
            arg1[counter].y3 = spD8[((currentCol - startCol) + 1) + (((currentRow - startRow) + 1) * colCount)];
            arg1[counter].z1 = sp332;
            arg1[counter].z2 = temp_fp;
            arg1[counter].z3 = temp_fp;
            temp_fs0 = (arg1[counter].y2 - arg1[counter].y3) * stepZ;
            temp_fs3 = stepX * stepZ;
            temp_fs1 = (arg1[counter].y1 - arg1[counter].y3) * stepX;
            temp_fs2 = sqrtf((temp_fs0 * temp_fs0) + (temp_fs3 * temp_fs3) + (temp_fs1 * temp_fs1));
            if (temp_fs2 > 0.0f) {
                temp_fs2 = 1 / temp_fs2;
                arg2[counter].x = temp_fs0 * temp_fs2;
                arg2[counter].y = temp_fs3 * temp_fs2;
                arg2[counter].z = temp_fs1 * temp_fs2;

                arg2[counter].planeD.w = -((arg1[counter].x1 * arg2[counter].x) + (arg1[counter].y1 * arg2[counter].y) +
                                           (arg1[counter].z1 * arg2[counter].z));
                counter++;
            }

            var_s3_2 = temp_s2;
        }
        sp332 = temp_fp;
    }
    return counter;
}

Object_Log *obj_wave_init(s32 blockID, f32 x, f32 z) {
    f32 spE4;
    f32 temp_f;
    s32 pad;
    f32 var_f0;
    s32 pad2;
    f32 var_f12;
    s32 var_t4;
    f32 temp_f2;
    s32 spC4;
    s32 spC0;
    f32 var_f18;
    f32 var_f20;
    f32 var_f22;
    s32 spB0;
    s32 var_t1;
    s32 var_a0;
    s16 var_t0;
    s16 var_v1;
    s16 var_a3;
    s16 var_a1;
    f32 var_f2;
    s32 var_v0;
    f32 var_f14;
    s32 var_v0_2;
    s32 var_a2;
    // Bisected to this array: read before it is written on some path through
    // the branches below, which indexes gWaveHeightIndices with garbage and
    // blacks out Future Fun Land on DC.
    s32 sp80[3] = { 0 };
    s32 sp68[6];
    s32 sp5C[3];
    f32 sp50[3];
    Object_Log *object;

    var_t0 = 128;
    object = NULL;
    if (gWaveHeightTable != NULL && blockID >= 0 && blockID < gNumberOfLevelSegments) {
        object = mempool_alloc_safe((gWaveController.seedSize >> 1) + (sizeof(Object_Log) - 2), COLOUR_TAG_CYAN);
        object->baseHeight = gWaveModel[blockID].originY;
        object->sampleShift = 0;
        object->unused3 = 1;
        object->phase = 0;
        object->phaseCount = gWaveController.seedSize;
        var_f20 = gWaveVtxStepZ;
        var_f22 = gWaveVtxStepX;
        temp_f2 = (1.0f - gWaveController.minDetailScale) / 127.0f;
        var_f2 = temp_f2;
        spB0 = (blockID * gWaveTileTotal);
        if (gWaveController.doubleDensity) {
            var_t4 = (gWaveController.subdivisions * 2) + 1;
            var_f20 /= 2.0f;
            var_f22 /= 2.0f;
        } else {
            var_t4 = gWaveController.subdivisions + 1;
        }
        // clang-format off
        x -= gWaveModel[blockID].originX;
        if (x < 0.0f) { x = 0.0f; }
        else if (x >= gWaveBoundingBoxW) { x = gWaveBoundingBoxW - 1; }
        z -= gWaveModel[blockID].originZ;
        if (z < 0.0f) { z = 0.0f; }
        else if (z >= gWaveBoundingBoxH) { z = gWaveBoundingBoxH - 1; }
        // clang-format on
        spC4 = x / var_f22;
        spC0 = z / var_f20;
        object->tileX = spC4;
        object->tileZ = spC0;
        object->blockID = blockID;
        x -= (spC4 * var_f22);
        z -= (spC0 * var_f20);
        var_a0 = gWaveModel[blockID].waveTileOffsetX + spC4;
        while (var_a0 >= gWaveController.tileCount) {
            var_a0 -= gWaveController.tileCount;
        }

        var_v0 = gWaveModel[blockID].waveTileOffsetZ + spC0;
        while (var_v0 >= gWaveController.tileCount) {
            var_v0 -= gWaveController.tileCount;
        }

        var_t1 = FALSE;
        if (z != var_f20 && x < (((var_f20 - z) / var_f20) * var_f22)) {
            var_t1 = TRUE;
        }

        if (var_t1 != FALSE) {
            sp80[0] = (var_v0 * gWaveController.tileCount) + var_a0;

            if ((var_v0 + 1) >= gWaveController.tileCount) {
                sp80[1] = var_a0;
            } else {
                sp80[1] = ((var_v0 + 1) * gWaveController.tileCount) + var_a0;
            }

            if ((var_a0 + 1) >= gWaveController.tileCount) {
                sp80[2] = (var_v0 * gWaveController.tileCount);
            } else {
                sp80[2] = (var_v0 * gWaveController.tileCount) + var_a0 + 1;
            }

            spE4 = 0.0f;
            sp5C[0] = (spB0 + spC4) + (spC0 * var_t4);
            sp5C[1] = (spB0 + spC4) + ((spC0 + 1) * var_t4);
            sp5C[2] = (spB0 + spC4) + (spC0 * var_t4) + 1;
        } else {
            if ((var_a0 + 1) >= gWaveController.tileCount) {
                sp80[0] = var_v0 * gWaveController.tileCount;
            } else {
                sp80[0] = var_a0 + 1 + (var_v0 * gWaveController.tileCount);
            }

            if ((var_v0 + 1) >= gWaveController.tileCount) {
                sp80[1] = var_a0;
            } else {
                sp80[1] = ((var_v0 + 1) * gWaveController.tileCount) + var_a0;
            }

            if ((var_a0 + 1) >= gWaveController.tileCount) {
                sp80[2] = 0;
            } else {
                sp80[2] = var_a0 + 1;
            }

            if ((var_v0 + 1) < gWaveController.tileCount) {
                sp80[2] += (var_v0 + 1) * gWaveController.tileCount;
            }

            spE4 = var_f22;
            sp5C[0] = (spC4 + spB0) + (spC0 * var_t4) + 1;
            sp5C[1] = (spC4 + spB0) + (spC0 + 1) * var_t4;
            sp5C[2] = (spC4 + spB0) + (spC0 + 1) * var_t4 + 1;
        }

        for (var_a3 = 0; var_a3 < 3; var_a3++) {
            sp68[var_a3 * 2] = gWaveHeightIndices[sp80[var_a3]].s[0];
            sp68[var_a3 * 2 + 1] = gWaveHeightIndices[sp80[var_a3]].s[1];
            sp50[var_a3] = 1.0f;

            var_a2 = gWaveDetailLevels[sp5C[var_a3]];
            if (var_a2 < 0x7F) {
                sp50[var_a3] *= gWaveController.minDetailScale + (var_a2 * var_f2);
            }
        }

        if (gWaveController.doubleDensity) {
            sp50[0] /= 2.0;
            sp50[1] /= 2.0;
            sp50[2] /= 2.0;
        }

        for (var_a3 = 0; var_a3 < (gWaveController.seedSize >> 1); var_a3++) {
            temp_f = (gWaveHeightTable[sp68[0]] + gWaveHeightTable[sp68[1]]) * sp50[0];
            var_f12 = (gWaveHeightTable[sp68[2]] + gWaveHeightTable[sp68[3]]) * sp50[1];
            var_f0 = (gWaveHeightTable[sp68[4]] + gWaveHeightTable[sp68[5]]) * sp50[2];
            if (var_t1 != FALSE) {
                var_f2 = (temp_f - var_f0) * var_f20;
                var_f14 = (temp_f - var_f12) * var_f22;
            } else {
                var_f2 = (var_f12 - var_f0) * var_f20;
                var_f14 = (temp_f - var_f0) * var_f22;
            }
            var_f18 = var_f20 * var_f22;
            var_f0 = sqrtf((var_f2 * var_f2) + (var_f18 * var_f18) + (var_f14 * var_f14));
            var_f2 /= var_f0;
            var_f14 /= var_f0;
            var_f12 = var_f18 / var_f0;
            var_v0_2 =
                -(s32) (((var_f2 * x) + (var_f14 * z) - ((spE4 * var_f2) + (temp_f * var_f12))) * 16.0f / var_f12);
            if (var_v0_2 >= var_t0 || var_v0_2 < -var_t0) {
                var_v1 = 0;
                do {
                    var_t0 += var_t0;
                    var_v1++;
                } while (var_v0_2 >= var_t0 || var_v0_2 < -var_t0);

                for (var_a1 = 0; var_a1 < var_a3; var_a1++) {
                    object->heightSamples[var_a1] >>= var_v1;
                }
                object->sampleShift += var_v1;
            }
            object->heightSamples[var_a3] = (var_v0_2 >> object->sampleShift);
            for (var_a1 = 0; var_a1 < 6; var_a1++) {
                sp68[var_a1] += 2;
                while (sp68[var_a1] >= gWaveController.seedSize) {
                    sp68[var_a1] -= gWaveController.seedSize;
                }
            }
        }
    }

    return object;
}

/**
 * Finds the wave height and returns it for the given object.
 */
f32 obj_wave_height(Object_Log *log, s32 updateRate) {
    s32 var_t0;
    f32 y;

    log->phase += updateRate;
    while (log->phase >= log->phaseCount) {
        log->phase -= log->phaseCount;
    }
    var_t0 = log->heightSamples[log->phase >> 1];
    if (log->phase & 1) {
        if (log->phase + 1 >= log->phaseCount) {
            var_t0 += log->heightSamples[0];
        } else {
            var_t0 += log->heightSamples[(log->phase >> 1) + 1];
        }
        if (log->sampleShift > 0) {
            var_t0 <<= (log->sampleShift - 1);
        } else {
            var_t0 >>= 1;
        }
    } else {
        var_t0 <<= log->sampleShift;
    }
    y = (((f32) var_t0 / 16.0) + (f32) log->baseHeight);
    y *= gWaveController.magnitude;
    y += waves_get_y(log->blockID, log->tileX, log->tileZ);
    return y;
}

// height related calculation?
f32 waves_get_y(s32 blockID, s32 arg1, s32 arg2) {
    f32 dist;
    f32 distSq;
    f32 diffX;
    f32 diffZ;
    f32 temp_f24;
    f32 zeroCheck;
    f32 temp_f30;
    f32 stepX;
    f32 y;
    f32 stepZ;
    u32 var_s0;
    s32 temp_a1;
    s32 i;
    s32 subdivisons;
    s32 temp_0;
    WaveGenTileSlots *temp_a3;
    WaveGen *gen;

    y = 0.0f;
    if (gWaveGenCount <= 0) {
        return y;
    }

    subdivisons = gWaveController.subdivisions;
    stepX = gWaveVtxStepX;
    stepZ = gWaveVtxStepZ;
    if (gWaveController.doubleDensity) {
        subdivisons *= 2;
        stepX /= 2.0f;
        stepZ /= 2.0f;
    }
    temp_a1 = (gWaveModel[blockID].tileX * subdivisons) + arg1;
    temp_a3 = &gWaveGenSlots[temp_a1];
    if (temp_a3->genIndices[0] != 0xFF) {
        temp_0 = arg2 + (subdivisons * gWaveModel[blockID].tileZ);
        temp_f30 = gWaveBlockPosX + (temp_a1 * stepX);
        temp_f24 = gWaveBlockPosZ + (temp_0 * stepZ);
        zeroCheck = 0;
        i = 0;
        do {
            gen = &gWaveGenList[temp_a3->genIndices[i]];
            if (gen->minZ <= temp_f24 && temp_f24 <= gen->maxZ) {
                diffX = temp_f30 - gen->x_position;
                diffZ = temp_f24 - gen->z_position;
                distSq = (diffX * diffX) + (diffZ * diffZ);
                if (distSq < gen->radiusSq) {
                    dist = sqrtf(distSq);
                    var_s0 = gen->phase;
                    if (gen->isLinearX) {
                        if (diffX < zeroCheck) {
                            var_s0 -= (s32) (diffX * gen->phasePerUnit);
                        } else {
                            var_s0 += (s32) (diffX * gen->phasePerUnit);
                        }
                    } else if (gen->isLinearZ) {
                        if (diffX < zeroCheck) {
                            var_s0 -= (s32) (diffZ * gen->phasePerUnit);
                        } else {
                            var_s0 += (s32) (diffZ * gen->phasePerUnit);
                        }
                    } else {
                        var_s0 += (s32) (dist * gen->phasePerUnit);
                    }
                    diffX = coss_f((dist * 65536.0f) / gen->radius);
                    y += gen->amplitude * sins_f(var_s0) * diffX;
                }
            }
            i++;
        } while (i < 8 && temp_a3->genIndices[i] != 0xFF);
    }

    return y;
}
