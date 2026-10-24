#include "waves_internal.h"

/**
 * Removes a wave generator object from the list.
 */
void wavegen_destroy(Object *obj) {
    s32 i;
    s32 k;
    s32 j;
    s32 hasWave;
    WaveGenTileSlots *temp_a1;

    if (gWaveGenList == NULL) {
        return;
    }

    for (i = 0, hasWave = FALSE; i < gWaveGenCount && hasWave == FALSE; i++) {
        if (obj == gWaveGenObjs[i]) {
            hasWave = -1;
        }
    }

    if (hasWave == FALSE) {
        stubbed_printf("\nError :: can not remove a wave swell object which doesn't exist !");
        return;
    }

    i--;
    for (j = 0; j < gWaveTileGridCount; j++) {
        for (k = 0, temp_a1 = &gWaveGenSlots[j]; k < 8 && temp_a1->genIndices[k] != 0xFF; k++) {
            if (i != temp_a1->genIndices[k]) {
                continue;
            }

            while (k < 7) {
                temp_a1->genIndices[k] = temp_a1->genIndices[k + 1];
                k++;
            }
            temp_a1->genIndices[k] = 0xFF;
            k++;
        }
    }

    gWaveGenObjs[j] = NULL;
    gWaveGenCount--;
}

/**
 * Adds a wave generator point.
 */
void wavegen_add(Object *obj) {
    LevelObjectEntry_WaveGenerator *waveGen;
    s32 flags;

    waveGen = &obj->level_entry->waveGenerator;
    flags = 0;
    if (waveGen->linearAlongX != 0) {
        flags = 1;
    }

    if (waveGen->linearAlongZ != 0) {
        flags |= 2;
    }

    wavegen_register(obj, obj->trans.x_position, obj->trans.z_position, (f32) waveGen->waveSize,
                     waveGen->phaseOffset << 8, (f32) waveGen->waveSpeed / 16.0, (f32) waveGen->wavelength,
                     (f32) waveGen->amplitude / 16.0, flags);
}

/**
 * Registers a new wave generator point with the given properties.
 */
WaveGen *wavegen_register(Object *obj, f32 xPos, f32 zPos, f32 waveSize, s32 arg4, f32 arg5, f32 arg6, f32 arg7,
                          s32 flags) {
    f32 stepSize;
    s32 emptyWave;
    s32 minX;
    s32 j;
    s32 maxX;
    s32 k;
    s32 i;
    WaveGenTileSlots *temp;
    WaveGen *result;

    result = NULL;
    if (gWaveGenList != NULL) {
        for (i = 0, emptyWave = FALSE; i < 32 && emptyWave == FALSE; i++) {
            if (gWaveGenObjs[i] == NULL) {
                emptyWave = -1; // Why it is -1 is anyone's guess. Though all that's important is that it's nonzero.
            }
        }

        i--;
        if (emptyWave) {
            gWaveGenObjs[i] = obj;
            gWaveGenCount++;
            stepSize = gWaveVtxStepX;
            if (gWaveController.doubleDensity) {
                stepSize /= 2.0f;
            }

            minX = ((xPos - waveSize) - gWaveBlockPosX) / stepSize;
            if (minX >= gWaveTileGridCount) {
                stubbed_printf("\nError :: can not add another wave swell, reached limit of %d.", gWaveTileGridCount);
                return result;
            }

            maxX = ((xPos + waveSize) - gWaveBlockPosX) / stepSize;
            if (maxX < 0) {
                return result;
            }

            if (minX < 0) {
                minX = 0;
            }

            if (maxX >= gWaveTileGridCount) {
                maxX = gWaveTileGridCount - 1;
            }

            for (j = minX; j <= maxX; j++) {
                temp = &gWaveGenSlots[j];
                if (temp->genIndices[7] != 0xFF) {
                    continue;
                }

                k = 0;
                while (temp->genIndices[k] != 0xFF) {
                    k++;
                }
                temp->genIndices[k] = i;
            }
            result = &gWaveGenList[i];
            result->minZ = zPos - waveSize;
            result->maxZ = zPos + waveSize;
            result->x_position = xPos;
            result->z_position = zPos;
            result->radius = waveSize;
            result->index = i;
            result->radiusSq = waveSize * waveSize;
            result->phase = arg4;
            if (osTvType == OS_TV_TYPE_PAL) {
                result->phaseSpeed = arg5 * 20971.52; //(f64) (0x80000 / 25.0);
            } else {
                result->phaseSpeed = arg5 * 17476.27; //(f64) ((0x80000 / 1.2) / 25.0);
            }
            result->phasePerUnit = 65536.0f / arg6;
            result->amplitude = arg7;
            result->speed = arg5;
            result->isLinearX = flags & 1;
            result->isLinearZ = flags & 2;
            result->wavelength = arg6;
        }
    }

    return result;
}

UNUSED void wavegen_scale(WaveGen *gen, f32 radiusAdd, f32 arg2, f32 arg3, f32 arg4) {
    if (gen == NULL) {
        return;
    }

    gen->radius += radiusAdd;
    if (gen->radius < 1.0) {
        gen->radius = 1.0f;
    }

    gen->minZ = (gen->z_position - gen->radius);
    gen->maxZ = (gen->z_position + gen->radius);
    gen->speed += arg2;
    if (osTvType == OS_TV_TYPE_PAL) {
        gen->phaseSpeed = gen->speed * 20971.52; //(f64) (0x80000 / 25.0);
    } else {
        gen->phaseSpeed = gen->speed * 17476.27; //(f64) ((0x80000 / 1.2) / 25.0);
    }

    gen->wavelength = gen->wavelength + arg3;
    if (gen->wavelength < 1.0) {
        gen->wavelength = 1.0f;
    }

    gen->phasePerUnit = 65536.0f / gen->wavelength; // 0x10000
    gen->amplitude += arg4;
}

void wave_gens_update(s32 updateRate) {
    s32 i;

    for (i = 0; i < 32; i++) {
        if (gWaveGenObjs[i] != NULL) {
            gWaveGenList[i].phase += (0, gWaveGenList[i].phaseSpeed * updateRate) >> 4;
        }
    }
}

/**
 * Wave Power loop func.
 * Waits for racers to pass through, then sets itself as the current baseline for how strong the waves should be.
 */
void obj_loop_wavepower(Object *obj, UNUSED s32 updateRate) {
    LevelObjectEntry_WavePower *entry;
    s32 numRacers;
    Object *racerObj;
    Object_Racer *racer;
    Object **racers;
    s32 i;
    f32 diffY;
    f32 diffZ;
    f32 diffX;
    f32 distance;

    if (obj == gWaveGeneratorObj) {
        return;
    }

    racers = get_racer_objects(&numRacers);
    if (numRacers <= 0) {
        return;
    }

    racerObj = NULL;
    for (i = 0; i < numRacers && racerObj == NULL; i++) {
        racer = racers[i]->racer;
        if (racer->playerIndex == PLAYER_ONE) {
            racerObj = racers[i];
        }
    }

    if (racerObj == NULL) {
        return;
    }

    entry = &obj->level_entry->wavePower;
    distance = entry->radius;
    distance *= distance;
    diffX = racerObj->trans.x_position - obj->trans.x_position;
    diffY = racerObj->trans.y_position - obj->trans.y_position;
    diffZ = racerObj->trans.z_position - obj->trans.z_position;
    if ((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ) < distance) {
        gWavePowerBase = entry->power / 256.0f;
        gWaveMagnitude = (gWavePowerBase - gWaveController.magnitude) / (f32) entry->divisor;
        gWavePowerDivisor = entry->divisor;
        gWaveGeneratorObj = obj;
    }
}
