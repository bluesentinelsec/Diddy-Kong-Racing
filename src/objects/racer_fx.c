#include "objects_internal.h"

// Boosts started for a non-racer (racer index -1) take slots counting down from the last one.
#define BOOST_OVERRIDE_FIRST_ID (NUMBER_OF_CHARACTERS - 1)

static f32 gBoostFanDirections[16] = { 0.0f, 1.0f,  0.70711f,  0.70711f,  1.0f,  0.0f, 0.70711f,  -0.70711f,
                                       0.0f, -1.0f, -0.70711f, -0.70711f, -1.0f, 0.0f, -0.70711f, 0.70711f };

/**
 * Spawns control objects for racer boost visuals, as well as shield and magnet visuals.
 * Boost geometry is made in real time, and allocated here.
 * This function is called on every level load, but only racers use the stuff here.
 */
void racerfx_alloc(s32 numberOfVertices, s32 numberOfTriangles) {
    Object_Boost *boostObj;
    LevelObjectEntry_Boost2 objEntry;
    s32 i;

    gBoostTris[0] = (Triangle *) mempool_alloc_safe(
        ((numberOfTriangles * sizeof(Triangle)) + (numberOfVertices * sizeof(Vertex))) * 2, COLOUR_TAG_BLUE);
    gBoostTris[1] = (Triangle *) ((uintptr_t) gBoostTris[0] + numberOfTriangles * sizeof(Triangle));
    gBoostVerts[0] = (Vertex *) ((uintptr_t) gBoostTris[1] + numberOfTriangles * sizeof(Triangle));
    gBoostVerts[1] = (Vertex *) ((uintptr_t) gBoostVerts[0] + numberOfVertices * sizeof(Vertex));
    gBoostVertCount = numberOfVertices;
    gNumOfBoostVerts = 0;
    gBoostTriCount = numberOfTriangles;
    gNumOfBoostTris = 0;
    gBoostVertFlip = 0;
    boostObj = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    // Makes 10 boost objects, but only 8 racers can actually exist at once.
    for (i = 0; i < NUMBER_OF_CHARACTERS; i++) {
        objEntry.common.objectID = ASSET_OBJECT_ID_BOOST;
        objEntry.common.size = sizeof(LevelObjectEntry_Boost2);
        objEntry.common.x = 0;
        objEntry.common.y = 0;
        objEntry.common.z = 0;
        objEntry.racerIndex = i;
        gBoostEffectObjects[i] = spawn_object(&objEntry.common, OBJECT_SPAWN_UNK01);
        if (gBoostEffectObjects[i] != NULL) {
            gBoostEffectObjects[i]->properties.boost.obj = NULL;
            gBoostEffectObjects[i]->properties.boost.indexes = 0;
            boostObj[i].effectState = BOOST_FX_GROW;
            boostObj[i].effectScale = 0.0f;
            boostObj[i].sprite = tex_load_sprite(boostObj[i].spriteId, 0);
            boostObj[i].tex = load_texture(boostObj[i].textureId);
            boostObj[i].pulseTimer = rand_range(0, 255);
            boostObj[i].lingerTimer = 0;
            // This is for shields, not boosts.
            gShieldSineTime[i] = rand_range(0, 255);
        }
        gBoostEffectFree[i] = TRUE;
    }
    gBoostObjOverrideID = BOOST_OVERRIDE_FIRST_ID;
    objEntry.common.objectID = ASSET_OBJECT_ID_SHIELD;
    objEntry.common.size = sizeof(LevelObjectEntry_Boost2);
    objEntry.common.x = 0;
    objEntry.common.y = 0;
    objEntry.common.z = 0;
    gShieldEffectObject = spawn_object(&objEntry.common, OBJECT_SPAWN_NONE);
    for (i = 0; i < NUMBER_OF_CHARACTERS; i++) {
        gRacerFXData[i].unused0 = 0;
        gRacerFXData[i].magnetPulseTimer = rand_range(0, 255);
        gRacerFXData[i].magnetSpinTimer = rand_range(0, 255);
        gRacerFXData[i].magnetFade = 0;
    }
    objEntry.common.objectID = ASSET_OBJECT_ID_AINODE;
    objEntry.common.size = sizeof(LevelObjectEntry_Boost2) + 0x80; // Not sure where this 0x80 comes from.
    objEntry.common.x = 0;
    objEntry.common.y = 0;
    objEntry.common.z = 0;
    gMagnetEffectObject = spawn_object(&objEntry.common, OBJECT_SPAWN_NONE);
}

/**
 * Attempts to free the boost, shield and magnet objects and assets.
 */
void racerfx_free(void) {
    Sprite *sprite;
    TextureHeader *texture;
    Object_Boost *objBoost;
    u32 i;

    if (gBoostTris[0]) {
        mempool_free(gBoostTris[0]);
        gBoostTris[0] = NULL;
        gBoostTris[1] = NULL;
        gBoostVerts[0] = NULL;
        gBoostVerts[1] = NULL;
    }
    objBoost = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    for (i = 0; i < NUMBER_OF_CHARACTERS; i++) {
        sprite = objBoost[i].sprite;
        if (sprite != NULL) {
            sprite_free(sprite);
            objBoost[i].sprite = NULL;
        }
        texture = objBoost[i].tex;
        if (texture != NULL) {
            tex_free(texture);
            objBoost[i].tex = NULL;
        }
    }
    if (gShieldEffectObject != NULL) {
        free_object(gShieldEffectObject);
    }
    gShieldEffectObject = NULL;

    if (gMagnetEffectObject != NULL) {
        free_object(gMagnetEffectObject);
    }
    gMagnetEffectObject = NULL;
    gParticlePtrList_flush();
}

/** Builds the fan of vertices and triangles for one boost flame, facing the racer's rotation. */
void boost_build_fan_mesh(Vertex *vertices, Triangle *triangles, ObjectTransform *trans, f32 fanRadius, f32 fanLength,
                          s16 angle, TextureHeader *tex) {
    s32 vertexUVs[BOOST_TRI_COUNT];
    s32 i;
    s32 height, width;
    s16 *v;
    Vec3f vertexPos;
    s32 *tri;
    f32 *ptr;

    v = (s16 *) vertices;

    vertexPos.z = -fanLength;
    vec3f_rotate_py(&trans->rotation, &vertexPos);

    // A rather strange way to fill structures

    *v++ = vertexPos.f[0] + trans->x_position;
    *v++ = vertexPos.f[1] + trans->y_position;
    *v++ = vertexPos.f[2] + trans->z_position;
    *v++ = -1;
    *v++ = -1;

    ptr = gBoostFanDirections;
    for (i = 0; i < BOOST_TRI_COUNT; i++) {
        vertexPos.x = *ptr++ * fanRadius;
        vertexPos.y = *ptr++ * fanRadius;
        vertexPos.z = 0.0f;

        vec3f_rotate(&trans->rotation, &vertexPos);

        *v++ = vertexPos.f[0] + trans->x_position;
        *v++ = vertexPos.f[1] + trans->y_position;
        *v++ = vertexPos.f[2] + trans->z_position;
        *v++ = -1;
        *v++ = -1;
    }

    width = (tex->width - 1) << 4;
    height = (tex->height - 1) << 4;

    for (i = 0; i < BOOST_TRI_COUNT; i++) {
        vertexUVs[i] = width + ((sins_s16(angle) * width) >> 16);
        vertexUVs[i] |= ((height << 16) + height * coss_s16(angle)) & 0xFFFF0000;
        angle += 0x2000;
    }

    tri = (s32 *) triangles;
    for (i = 0; i < BOOST_TRI_COUNT; i++) {
        *tri++ = DKR_TRIANGLE(BACKFACE_CULL, 0, i + 1, ((i + 1) & 7) + 1);
        *tri++ = ((width & 0xFFFF) << 16) | (height & 0xFFFF);
        *tri++ = vertexUVs[i];
        *tri++ = vertexUVs[(i + 1) & 7];
    }
}

/** Starts or restyles a racer's boost effect object for its vehicle and boost type. */
void racer_set_boost_effect(Object *racerObj, s32 racerIndex, s32 vehicleIDPrev, s32 boostType, s32 keepActive) {
    Vec3f fanOffset;
    f32 fanLength;
    f32 fanRadius;
    Object_Boost *boostAsset;
    ObjectTransform objTrans;
    Object_Boost *objBoostRacer;
    Object_Boost *objBoostType;
    Object_Boost_Inner *boostData;

    if (racerIndex == -1) {
        racerIndex = gBoostObjOverrideID;
        gBoostObjOverrideID--;
    }
    if (gBoostObjOverrideID < 0) {
        gBoostObjOverrideID = 0;
    }
    if (racerIndex >= 0 && racerIndex < NUMBER_OF_CHARACTERS) {
        boostAsset = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
        objBoostType = &boostAsset[boostType];
        objBoostRacer = &boostAsset[racerIndex];
        if (gBoostEffectObjects[racerIndex] != NULL) {
            switch (vehicleIDPrev) {
                default:
                    boostData = NULL;
                    break;
                case VEHICLE_CAR:
                    boostData = &objBoostRacer->carBoostData;
                    break;
                case VEHICLE_HOVERCRAFT:
                    boostData = &objBoostRacer->hovercraftBoostData;
                    break;
                case VEHICLE_PLANE:
                    boostData = &objBoostRacer->flyingBoostData;
                    break;
                case VEHICLE_ROCKET:
                    boostData = &objBoostRacer->flyingBoostData;
                    break;
            }
            if (boostData != NULL) {
                gBoostVehicleIds[racerIndex] = vehicleIDPrev;
                gBoostTypes[racerIndex] = boostType;
                if (objBoostRacer->effectState == BOOST_FX_ACTIVE) {
                    // fanLength holds the pulse cosine until it is overwritten with the length below.
                    fanLength = coss_f(objBoostRacer->pulseTimer << 12);
                    fanRadius = (boostData->fanRadiusBase + (fanLength * boostData->fanRadiusPulse)) *
                                objBoostRacer->effectScale;
                    fanLength = (boostData->fanLengthBase + (fanLength * boostData->fanLengthPulse)) *
                                objBoostRacer->effectScale;
                    if ((boostType & 3) == BOOST_MEDIUM) {
                        fanRadius *= 1.09f;
                        fanLength *= 1.09f;
                    }
                    if ((boostType & 3) >= BOOST_LARGE) {
                        fanRadius *= 1.18f;
                        fanLength *= 1.18f;
                    }
                    objTrans.x_position = boostData->position.x;
                    objTrans.y_position = boostData->position.y;
                    objTrans.z_position = boostData->position.z;
                    objTrans.scale = 1.0f;
                    objTrans.rotation.y_rotation = -0x8000;
                    objTrans.rotation.x_rotation = 0;
                    objTrans.rotation.z_rotation = 0;
                    boost_build_fan_mesh(&gBoostVerts[gBoostVertFlip][gNumOfBoostVerts],
                                         &gBoostTris[gBoostVertFlip][gNumOfBoostTris], &objTrans, fanRadius, fanLength,
                                         objBoostRacer->pulseTimer << 12, objBoostType->tex);
                    gBoostEffectObjects[racerIndex]->properties.boost.indexes =
                        (racerIndex << 28) | (gNumOfBoostVerts << 14) | gNumOfBoostTris;
                    gNumOfBoostVerts += BOOST_VERT_COUNT;
                    gNumOfBoostTris += BOOST_TRI_COUNT;
                }
                gBoostEffectObjects[racerIndex]->properties.boost.obj = racerObj;
                gBoostEffectObjects[racerIndex]->trans.x_position = 0.0f;
                gBoostEffectObjects[racerIndex]->trans.y_position = 0.0f;
                gBoostEffectObjects[racerIndex]->trans.z_position = 0.0f;
                fanOffset.x = boostData->position.x;
                fanOffset.y = boostData->position.y;
                fanOffset.z = boostData->position.z;
                vec3f_rotate(&racerObj->trans.rotation, &fanOffset);
                ignore_bounds_check();
                move_object(gBoostEffectObjects[racerIndex], racerObj->trans.x_position + fanOffset.f[0],
                            racerObj->trans.y_position + fanOffset.f[1], racerObj->trans.z_position + fanOffset.f[2]);
            }
            if (keepActive != FALSE) {
                gBoostEffectFree[racerIndex] = FALSE;
            }
        }
    }
}

/**
 * Updates the racer FX object states.
 * This includes the shield wobble timer, and the texture frames for the magnet and shield.
 */
void racerfx_update(s32 updateRate) {
    s32 i;
    Object_Boost *boostObj;
    s32 racerIndex;
    Object_Boost *asset20;
    f32 updateRateF;
    Object_Racer *racer;

    gBoostVertFlip = 1 - gBoostVertFlip;
    gNumOfBoostVerts = 0;
    gNumOfBoostTris = 0;
    asset20 = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    gBoostObjOverrideID = BOOST_OVERRIDE_FIRST_ID;
    for (i = 0; i < NUMBER_OF_CHARACTERS; i++) {
        if (gBoostEffectFree[i] && gBoostEffectObjects[i] != NULL) {
            gBoostEffectObjects[i]->properties.boost.obj = NULL;
        }
        gBoostEffectFree[i] = TRUE;
    }
    for (i = 0; i < gNumRacers; i++) {
        updateRateF = (f32) updateRate;
        if (osTvType == OS_TV_TYPE_PAL) {
            updateRateF *= 1.2f;
        }
        racer = (*gRacers)[i]->racer;
        boostObj = &asset20[racer->racerIndex];
        if (racer->shieldTimer != 0) {
            gShieldSineTime[racer->racerIndex] += updateRate;
        }
        boostObj->pulseTimer += updateRate;
        if (racer->boostTimer != 0) {
            boostObj->lingerTimer = 20;
            if (boostObj->effectState == BOOST_FX_GROW) {
                boostObj->effectScale += updateRateF * 0.25f;
                updateRateF = 0.0f;
                if (boostObj->effectScale > 2.4f) {
                    boostObj->effectScale = 4.8f - boostObj->effectScale;
                    boostObj->effectState = BOOST_FX_SETTLE;
                }
            }
            if (boostObj->effectState == BOOST_FX_SETTLE) {
                boostObj->effectScale -= updateRateF * 0.25f;
                updateRateF = 0.0f;
                if (boostObj->effectScale < 1.0f) {
                    boostObj->effectState = BOOST_FX_ACTIVE;
                    boostObj->effectScale = 1.0f - boostObj->effectScale;
                }
            }
            if (boostObj->effectState == BOOST_FX_ACTIVE) {
                if (boostObj->effectScale < 1.0f) {
                    boostObj->effectScale += updateRateF * 0.125f;
                    if (boostObj->effectScale > 1.0f) {
                        boostObj->effectScale = 1.0f;
                    }
                }
            }
        } else {
            if (boostObj->lingerTimer > 0) {
                boostObj->lingerTimer -= updateRate;
            } else {
                if (boostObj->effectState == BOOST_FX_ACTIVE) {
                    boostObj->effectScale -= updateRateF * 0.05f;
                    updateRateF = 0.0f;
                    if (boostObj->effectScale < 0.0f) {
                        boostObj->effectState = BOOST_FX_GROW;
                        boostObj->effectScale += 1.0f;
                    }
                }
                if (boostObj->effectState < BOOST_FX_ACTIVE) {
                    boostObj->effectScale -= (updateRateF * 0.1f);
                    if (boostObj->effectScale < 0.0f) {
                        boostObj->effectScale = 0.0f;
                    }
                    boostObj->effectState = BOOST_FX_GROW;
                }
            }
        }
        if ((boostObj->effectState > 0) || (boostObj->effectScale > 0.0f)) {
            racer_set_boost_effect((*gRacers)[i], racer->racerIndex, racer->vehicleIDPrev, racer->boostType, 0);
        }
        racerIndex = racer->racerIndex;
        gRacerFXData[racerIndex].magnetPulseTimer += updateRate;
        gRacerFXData[racerIndex].magnetSpinTimer += updateRate;
        if (racer->magnetTimer != 0) {
            if (gRacerFXData[racerIndex].magnetFade + (updateRate << 2) < 32) {
                gRacerFXData[racerIndex].magnetFade += (updateRate << 2);
            } else {
                gRacerFXData[racerIndex].magnetFade = 32;
            }
        } else {
            if (gRacerFXData[racerIndex].magnetFade - updateRate > 0) {
                gRacerFXData[racerIndex].magnetFade -= updateRate;
            } else {
                gRacerFXData[racerIndex].magnetFade = 0;
            }
        }
    }
    if (gMagnetEffectObject != NULL) {
        obj_tex_animate(gMagnetEffectObject, updateRate);
    }
}

/**
 * Returns the boost object with the given ID.
 * Returns a specific ID if the arg passed is BOOST_DEFAULT.
 */
Object *racerfx_get_boost(s32 boostID) {
    if (boostID == BOOST_DEFAULT) {
        boostID = gBoostObjOverrideID;
    }
    if (boostID < 0 || boostID >= NUMBER_OF_CHARACTERS) {
        return NULL;
    }
    return gBoostEffectObjects[boostID];
}
