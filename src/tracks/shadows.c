#include "tracks_internal.h"

#define FLAGS_8002E904 (RENDER_HIDDEN | RENDER_DECAL | RENDER_WATER | RENDER_NO_SHADOW)

static f32 gShadowLightLevels[10] = { 0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f };

/**
 * Render the shadow of an object on the ground as a decal.
 * Can subdivide itself to wrap around the terrain properly, as the N64 lacks stencil buffering.
 */
void shadow_render(Object *obj, ShadowData *shadow) {
    s32 i;
    s32 numVerts;
    s32 numTris;
    Vertex *vtx;
    Triangle *tri;
    s32 flags;
    UNUSED s32 tri2;
    s32 vtx2;
    s32 vtxCount;
    s32 triCount;
    s32 alpha;

    if (obj->header->shadowGroup) {
        if (shadow->meshStart != -1 && gDisableShadows == FALSE) {
            gShadowIndex = gShadowHeapFlip;
            if (obj->header->shadowGroup == SHADOW_SCENERY) {
                gShadowIndex += 2;
            }
            i = shadow->meshStart;
            gCurrShadowHeapData = gShadowHeapData[gShadowIndex];
            gCurrShadowTris = gShadowHeapTris[gShadowIndex];
            gCurrShadowVerts = gShadowHeapVerts[gShadowIndex];
            alpha = gCurrShadowVerts[gCurrShadowHeapData[i].vtxCount].a;
            flags = RENDER_FOG_ACTIVE | RENDER_Z_COMPARE;
            if (alpha == 0 || obj->opacity == 0) {
                i = shadow->meshEnd; // It'd be easier to just return...
            } else if (alpha != 255 || obj->opacity != 255) {
                flags = RENDER_FOG_ACTIVE | RENDER_SEMI_TRANSPARENT | RENDER_Z_COMPARE;
                alpha = (obj->opacity * alpha) >> 8;
                gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, alpha);
            }
            while (i < shadow->meshEnd) {
                material_set_no_tex_offset(&gTrackDL, gCurrShadowHeapData[i].texture, flags);
                // I hope we can clean this part up.
                tri2 = triCount = gCurrShadowHeapData[i].triCount; // Fakematch
                vtx2 = vtxCount = gCurrShadowHeapData[i].vtxCount;
                numTris = gCurrShadowHeapData[i + 1].triCount - triCount;
                numVerts = gCurrShadowHeapData[i + 1].vtxCount - vtxCount;
                tri = &gCurrShadowTris[triCount];
                vtx = &gCurrShadowVerts[vtx2];
                gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(vtx), numVerts, 0);
                gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(tri), numTris, 1);
                i++;
            }

            if (flags != RENDER_Z_COMPARE) {
                gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, 255);
            }
        }
    }
}

/**
 * Used only by cars, render a texture on the surface of the water where the car is
 * to give the wave effect. Works almost identically to shadows, since water can be wavy.
 */
void watereffect_render(Object *obj, WaterEffect *effect) {
    s32 i;
    s32 numVerts;
    s32 numTris;
    Vertex *vtx;
    Triangle *tri;
    s32 flags;
    UNUSED s32 triCount;
    UNUSED s32 vtxCount;

    if (obj->header->waterEffectGroup) {
        if (effect->meshStart != -1 && gDisableShadows == FALSE) {
            gWaterEffectIndex = gShadowHeapFlip;
            i = effect->meshStart;
            if (obj->header->waterEffectGroup == SHADOW_SCENERY) {
                gWaterEffectIndex += 2;
                if (get_distance_to_active_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position) >
                    768.0f) {
                    i = effect->meshEnd; // Just return.
                }
            }
            flags = RENDER_FOG_ACTIVE | RENDER_Z_COMPARE;
            gCurrShadowHeapData = gShadowHeapData[gWaterEffectIndex];
            gCurrShadowTris = gShadowHeapTris[gWaterEffectIndex];
            gCurrShadowVerts = gShadowHeapVerts[gWaterEffectIndex];
            while (i < effect->meshEnd) {
                material_set_no_tex_offset(&gTrackDL, gCurrShadowHeapData[i].texture, flags);
                triCount = gCurrShadowHeapData[i].triCount; // Fakematch
                vtxCount = gCurrShadowHeapData[i].vtxCount; // Fakematch
                numTris = gCurrShadowHeapData[i + 1].triCount - gCurrShadowHeapData[i].triCount;
                numVerts = gCurrShadowHeapData[i + 1].vtxCount - gCurrShadowHeapData[i].vtxCount;
                tri = &gCurrShadowTris[gCurrShadowHeapData[i].triCount];
                vtx = &gCurrShadowVerts[gCurrShadowHeapData[i].vtxCount];
                gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(vtx), numVerts, 0);
                gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(tri), numTris, 1);
                i++;
            }
        }
    }
}

/**
 * Updates shadow and water effect properties for each relevant object in the scene.
 * The first argument decides whether to update shadows for static objects or moving objects.
 * In multiplayer, only important objects get shadows.
 */
void shadow_update(s32 group, s32 waterGroup, s32 updateRate) {
    s32 objIndex;
    s32 objectCount;
    Object *obj;
    ObjectHeader *objHeader;
    f32 dist;
    s32 radius;
    s32 numViewports;
    Object **objects;
    s32 skipShading;
    TextureHeader *waterTex;
    ShadowData *shadow;
    WaterEffect *waterEffect;
    s32 playerIndex;

    gShadowIndex = gShadowHeapFlip;
    if (group == SHADOW_SCENERY) {
        gShadowIndex += 2;
    }
    gCurrShadowTris = (Triangle *) gShadowHeapTris[gShadowIndex];
    gCurrShadowVerts = (Vertex *) gShadowHeapVerts[gShadowIndex];
    gCurrShadowHeapData = (ShadowHeapProperties *) gShadowHeapData[gShadowIndex];
    gShadowTail = 0;
    gNewShadowTriCount = 0;
    gNewShadowVtxCount = 0;
    numViewports = cam_get_viewport_layout();
    objects = objGetObjList(&objIndex, &objectCount);
    while (objIndex < objectCount) {
        obj = objects[objIndex];
        objHeader = obj->header;
        waterEffect = obj->waterEffect;
        shadow = obj->shadow;
        objIndex += 1;
        if ((obj->trans.flags & OBJ_FLAGS_PARTICLE)) {
            continue;
        }
        if (shadow != NULL && shadow->scale > 0.0f && group == objHeader->shadowGroup) {
            shadow->meshStart = -1;
        }
        if (obj->trans.flags & OBJ_FLAGS_INVISIBLE) {
            shadow = NULL;
        }
        if ((shadow != NULL && objHeader->shadowGroup == SHADOW_ACTORS) ||
            (waterEffect != NULL && objHeader->waterEffectGroup == SHADOW_ACTORS)) {
            dist = get_distance_to_active_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
        } else {
            dist = 0;
        }
        if (shadow != NULL && shadow->scale > 0.0f && group == objHeader->shadowGroup) {
            gShadowOpacity = 1.0f;
            shadow->meshStart = -1;
            skipShading = FALSE;
            // Multiplayer
            if (objHeader->shadowGroup == SHADOW_ACTORS && numViewports >= TWO_PLAYERS &&
                numViewports <= FOUR_PLAYERS) {
                if (obj->behaviorId == BHV_RACER) {
                    playerIndex = obj->racer->playerIndex;
                    if (playerIndex != PLAYER_COMPUTER) {
                        shadow_generate(obj, FALSE);
                        skipShading = TRUE;
                    }
                } else if (obj->behaviorId == BHV_WEAPON) {
                    shadow_generate(obj, FALSE);
                    skipShading = TRUE;
                }
            } else { // Single Player
                radius = objHeader->shadowFadeMin;
                if (dist < radius) {
                    if (objHeader->shadowFadeMax < dist) {
                        gShadowOpacity = (radius - dist) / (radius - objHeader->shadowFadeMax);
                    }
                    shadow_generate(obj, FALSE);
                    skipShading = TRUE;
                }
            }
            if (skipShading == FALSE && obj->shading != NULL) {
                shadow_find_surface_under_object(obj);
            }
        }
        if (waterEffect != NULL && waterEffect->scale > 0.0f && waterGroup == objHeader->waterEffectGroup) {
            waterEffect->meshStart = -1;
            gShadowOpacity = 1.0f;
            waterTex = waterEffect->texture;
            if (waterTex != NULL && updateRate && waterTex->numOfTextures != 0x100) {
                waterEffect->textureFrame += waterEffect->animationSpeed;
                while (waterTex->numOfTextures < waterEffect->textureFrame) {
                    waterEffect->textureFrame -= waterTex->numOfTextures;
                }
            }

            // Multiplayer
            if (objHeader->shadowGroup == SHADOW_ACTORS && numViewports >= TWO_PLAYERS &&
                numViewports <= FOUR_PLAYERS) {
                if (obj->behaviorId == BHV_RACER) {
                    playerIndex = obj->racer->playerIndex;
                    if (playerIndex != PLAYER_COMPUTER) {
                        shadow_generate(obj, TRUE);
                    }
                } else if (obj->behaviorId == BHV_WEAPON) {
                    shadow_generate(obj, TRUE);
                }
            } else { // Single Player
                if (dist < objHeader->shadowFadeMin) {
                    if (objHeader->shadowFadeMax < dist) {
                        gShadowOpacity =
                            (objHeader->shadowFadeMin - dist) / (objHeader->shadowFadeMin - objHeader->shadowFadeMax);
                    }
                    shadow_generate(obj, TRUE);
                }
            }
        }
    }
    gCurrShadowHeapData[gShadowTail].triCount = gNewShadowTriCount;
    gCurrShadowHeapData[gShadowTail].vtxCount = gNewShadowVtxCount;
}

/**
 * Looks for a surface under the object within its shadow's height range, so
 * the shadow and water effect can be placed on it.
 */
void shadow_find_surface_under_object(Object *obj) {
    s32 sp94;
    s32 sp90;
    s32 blockId;
    s32 var_t3;
    s32 k;
    u32 batchFlags;
    s32 foundResult;
    s32 i;
    LevelModelSegment *block;
    Triangle *triangle;
    Vertex *vertices;
    s32 minYPos;
    s32 maxYPos;
    s32 j;

    sp94 = (s32) obj->trans.y_position + obj->header->shadowTop;
    sp90 = (s32) obj->trans.y_position + obj->header->shadowBottom;
    blockId = obj->segmentID;
    foundResult = FALSE;
    if (blockId != -1) {
        var_t3 = compute_grid_overlap_mask(&gCurrentLevelModel->segmentsBoundingBoxes[blockId],
                                           obj->trans.x_position - 16.0f, obj->trans.z_position - 16.0f,
                                           obj->trans.x_position + 16.0f, obj->trans.z_position + 16.0f);
        block = &gCurrentLevelModel->segments[blockId];
        for (i = 0; i < block->numberOfBatches && !foundResult; i++) {
            if (!(block->batches[i].flags & (RENDER_HIDDEN | RENDER_DECAL | RENDER_WATER | RENDER_NO_SHADOW))) {
                batchFlags = (block->batches[i].flags >> 19) & 7;
                vertices = &block->vertices[block->batches[i].verticesOffset];
                for (j = block->batches[i].facesOffset; j < block->batches[i + 1].facesOffset && !foundResult; j++) {
                    blockId = block->triangleGridMasks[j] & var_t3;
                    if (blockId) {}
                    if (((block->triangleGridMasks[j] & var_t3) & 0xFF) &&
                        ((block->triangleGridMasks[j] & var_t3) & 0xFF00)) {
                        triangle = &block->triangles[j];
                        minYPos = vertices[triangle->verticesArray[1]].y;
                        maxYPos = minYPos;
                        for (k = 1; k < 3; k++) {
                            if (vertices[triangle->verticesArray[k + 1]].y < minYPos) {
                                minYPos = vertices[triangle->verticesArray[k + 1]].y;
                            } else if (maxYPos < vertices[triangle->verticesArray[k + 1]].y) {
                                maxYPos = vertices[triangle->verticesArray[k + 1]].y;
                            }
                        }
                        if (maxYPos >= sp90 && sp94 >= minYPos) {
                            if (tri2d_xz_contains_point(obj->trans.x_position, obj->trans.z_position,
                                                        (Vec3s *) &vertices[triangle->verticesArray[1]],
                                                        (Vec3s *) &vertices[triangle->verticesArray[2]],
                                                        (Vec3s *) &vertices[triangle->verticesArray[3]])) {
                                foundResult = TRUE;
                                obj->shading->brightness +=
                                    (((1.0f - gShadowLightLevels[batchFlags]) - obj->shading->brightness) * 0.2);
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * Generate shadow geometry for an object.
 * Handles water effects too, isWater is true.
 */
void shadow_generate(Object *obj, s32 isWater) {
    f32 dist;
    s32 yPos;
    f32 xPos;
    f32 zPos;
    UNUSED s32 *pad;
    s32 cheats;
    s32 inSegs[28];
    s32 i;
    s32 segs;
    f32 character_scale;
    s32 test;

    yPos = obj->trans.y_position;
    character_scale = 1.0f;
    if (obj->behaviorId == BHV_RACER) {
        cheats = get_filtered_cheats();
        if (cheats & CHEAT_BIG_CHARACTERS) {
            character_scale = 1.4f;
        } else if (cheats & CHEAT_SMALL_CHARACTERS) {
            character_scale = 0.714f;
        }
    }

    gNewShadowObj = obj;
    gShadowYOffset = 2.0f;

    if (isWater) {
        gShadowWavePlaneCount = 0;
        obj->waterEffect->meshStart = gShadowTail;
        gNewShadowTexture = set_animated_texture_header(obj->waterEffect->texture, obj->waterEffect->textureFrame << 8);
        gNewShadowY2 = obj->header->waterEffectTop + yPos;
        gNewShadowY1 = obj->header->waterEffectBottom + yPos;
        if (gWaveBlockCount == 0 || cam_get_viewport_layout() < VIEWPORT_LAYOUT_2_PLAYERS) {
            gShadowYOffset = 0;
        }
        gNewShadowScale = (obj->waterEffect->scale * character_scale);
        gNewShadowWidth = gNewShadowScale * 10.0f;
        gNewShadowLength = gNewShadowScale * 10.0f;
        gShadowFadeStart = -1.0f;
    } else {
        obj->shadow->meshStart = gShadowTail;
        gNewShadowTexture = obj->shadow->texture;
        gNewShadowY2 = obj->header->shadowTop + yPos;
        gNewShadowY1 = obj->header->shadowBottom + yPos;
        if (obj->behaviorId != BHV_RACER) {
            dist = obj->distanceToCamera;
            if (dist < 0.0) {
                dist = -dist;
            }
            dist -= 512.0;
            if (dist < 0.0) {
                dist = 0.0;
            }
            if (dist > 1024.0) {
                dist = 1024.0;
            }
            gShadowYOffset += (dist * 0.005f);
        }
        gNewShadowScale = (obj->shadow->scale * character_scale);
        gNewShadowWidth = gNewShadowScale * 10.0f;
        gNewShadowLength = gNewShadowScale * 10.0f;
        gShadowArea = 4.0f * gNewShadowWidth * gNewShadowLength;
        gShadowFadeStart = (obj->header->shadowBottom * 0.125f);
        if (gShadowFadeStart < 0.0f) {
            gShadowFadeStart = -gShadowFadeStart;
        }
        gShadowFadeRange = (7.0f * gShadowFadeStart);
        gShadowMinY = -0x8000;
    }
    gNewShadowScale = 144.0f / gNewShadowScale;
    xPos = gNewShadowObj->trans.x_position;
    zPos = gNewShadowObj->trans.z_position;
    segs = get_inside_segment_count_xyz(inSegs, (xPos - gNewShadowWidth), gNewShadowY1, (zPos - gNewShadowLength),
                                        (xPos + gNewShadowWidth), gNewShadowY2, (zPos + gNewShadowLength));
    gShadowTriCount = 0;
    gShadowVertexCount = 0;
    for (i = 0; i < ARRAY_COUNT(gShadowBucketCounts); i++) {
        gShadowBucketCounts[i] = 0;
    }
    gShadowCentreTri = -1;
    gShadowCentreShared = -1;
    for (i = 0; i < segs; i++) {
        if (inSegs[i] >= 0) {
            if (isWater && (gCurrentLevelModel->segments[inSegs[i]].hasWaves != 0) && (gWaveBlockCount != 0)) {
                shadow_add_wave_triangles(inSegs[i]);
            } else {
                test = compute_grid_overlap_mask(&gCurrentLevelModel->segmentsBoundingBoxes[inSegs[i]],
                                                 (obj->trans.x_position - gNewShadowWidth),  // x1
                                                 (obj->trans.z_position - gNewShadowLength), // z1
                                                 (obj->trans.x_position + gNewShadowWidth),  // x2
                                                 (obj->trans.z_position + gNewShadowLength)  // z2
                );
                shadow_add_segment_triangles(&gCurrentLevelModel->segments[inSegs[i]], test, isWater);
            }
        }
    }
    if (gShadowTriCount > 0) {
        if ((obj->shading != NULL) && isWater == FALSE) {
            obj->shading->brightness = shadow_light_level();
        }
        shadow_project_vertices();
        shadow_build_mesh();
    }
    if (isWater == FALSE) {
        obj->shadow->meshEnd = gShadowTail;
    } else {
        obj->waterEffect->meshEnd = gShadowTail;
    }
}

/**
 * Adds the triangles of a segment that the shadow rectangle overlaps to the
 * shadow mesh being built, clipped to the rectangle.
 */
void shadow_add_segment_triangles(LevelModelSegment *arg0, s32 arg1, s32 arg2) {
    ShadowPlane sp100[8];
    Vec2f spD0[4];
    s32 spAC;
    Triangle *triangles;
    s32 nextFacesOffset;
    Vertex *vertices;
    s32 yPos;
    s32 minY;
    s32 foundIndex;
    s32 maxY;
    s32 temp_t6;
    s32 sp88;
    s32 someCount;
    s32 i2;
    s32 i;
    s32 curFacesOffset;

    spD0[0].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    spD0[0].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    spD0[1].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    spD0[1].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    spD0[2].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    spD0[2].y = gNewShadowObj->trans.z_position - gNewShadowLength;
    spD0[3].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    spD0[3].y = gNewShadowObj->trans.z_position - gNewShadowLength;

    for (spAC = 0; spAC < arg0->numberOfBatches; spAC++) {
        if ((arg2 && (arg0->batches[spAC].flags & RENDER_WATER)) ||
            (!arg2 && !(arg0->batches[spAC].flags & FLAGS_8002E904))) {
            curFacesOffset = arg0->batches[spAC].facesOffset;
            nextFacesOffset = arg0->batches[spAC + 1].facesOffset;
            vertices = &arg0->vertices[arg0->batches[spAC].verticesOffset];
            sp88 = (arg0->batches[spAC].flags >> 0x13) & 7;
            for (; curFacesOffset < nextFacesOffset; curFacesOffset++) {
                if (((arg0->triangleGridMasks[curFacesOffset] & arg1) & 0xFF) &&
                    ((arg0->triangleGridMasks[curFacesOffset] & arg1) & 0xFF00)) {
                    triangles = &arg0->triangles[curFacesOffset];
                    maxY = minY = vertices[triangles->verticesArray[1]].y;
                    for (i = 1; i < 3; i++) {
                        yPos = vertices[triangles->verticesArray[i + 1]].y;
                        if (yPos < minY) {
                            minY = yPos;
                        } else if (maxY < yPos) {
                            maxY = yPos;
                        }
                    }
                    if (gNewShadowY2 >= minY) {
                        if (maxY >= gNewShadowY1) {
                            for (i = 0; i < 3; i++) {
                                sp100[i].x = vertices[triangles->verticesArray[i + 1]].x;
                                sp100[i].z = vertices[triangles->verticesArray[i + 1]].z;
                                sp100[i].planeD.clip.bucketVertexIndex = -1;
                            }
                            // @note while the cast to Vec4f is incorrect, polygon_overlaps_rect x uses unk0 and z which
                            // are both floats so this is fine as the size is the same
                            if (polygon_overlaps_rect(spD0[2].x, spD0[2].y, spD0[0].x, spD0[0].y, 3, (Vec4f *) sp100) !=
                                0) {
                                temp_t6 = arg0->collisionFacets[curFacesOffset].basePlaneIndex * 4;
                                gShadowCurrentPlane = (ShadowPlane *) &(arg0->collisionPlanes)[temp_t6];
                                if (arg0->collisionPlanes[temp_t6 + 1] != 0) {
                                    if (gShadowFadeStart > 0.0f) {
                                        shadow_check_centre_in_triangle(sp100);
                                    }
                                    someCount = clip_polygon_to_rect(3, sp100, 4, spD0);
                                    if (someCount >= 3) {
                                        gShadowTris[gShadowTriCount].bucketVertexMask = 0;
                                        for (i2 = 0; i2 < someCount; i2++) {
                                            if (sp100[i2].planeD.clip.bucketVertexIndex < 0) {
                                                foundIndex = -1;
                                                i = 0;
                                                while ((i < gShadowVertexCount) && (foundIndex == -1)) {
                                                    if ((gShadowVertices[i].x == sp100[i2].x) &&
                                                        (gShadowVertices[i].z == sp100[i2].z)) {
                                                        foundIndex = i;
                                                    }
                                                    i++;
                                                }
                                                if (foundIndex == -1) {
                                                    gShadowVertices[gShadowVertexCount].x = sp100[i2].x;
                                                    gShadowVertices[gShadowVertexCount].plane = gShadowCurrentPlane;
                                                    gShadowVertices[gShadowVertexCount].z = sp100[i2].z;
                                                    gShadowTris[gShadowTriCount].vertexIndices[i2] =
                                                        gShadowVertexCount++;
                                                } else {
                                                    gShadowTris[gShadowTriCount].vertexIndices[i2] = foundIndex;
                                                }
                                            } else {
                                                gShadowTris[gShadowTriCount].vertexIndices[i2] =
                                                    sp100[i2].planeD.clip.bucketVertexIndex;
                                                gShadowTris[gShadowTriCount].bucketVertexMask |= 1 << i2;
                                            }
                                        }
                                        gShadowTris[gShadowTriCount].vertexCount = someCount;
                                        gShadowTris[gShadowTriCount].lightLevel = sp88;
                                        gShadowTriCount += 1;
                                        if ((gShadowCentreTri >= 0) && (sp88 != gShadowCentreTri)) {
                                            gShadowCentreShared = 0;
                                        }
                                        gShadowCentreTri = sp88;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * Adds the wave surface under the shadow rectangle to the water effect mesh.
 */
void shadow_add_wave_triangles(s32 arg0) {
    ShadowPlane spA8[8];
    Vec2f sp88[4];
    s32 var_v0;
    s32 var_a0;
    s32 var_a1;
    s32 temp_v0;
    s32 temp_v0_3;
    s32 var_s4;
    s32 var_t1;
    s32 var_v1;
    s32 tempIdx;

    sp88[0].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    sp88[0].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    sp88[1].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    sp88[1].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    sp88[2].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    sp88[2].y = gNewShadowObj->trans.z_position - gNewShadowLength;
    sp88[3].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    sp88[3].y = gNewShadowObj->trans.z_position - gNewShadowLength;
    // clang-format off
    temp_v0 = wave_cells_in_rect(
        arg0, gShadowWaveCells, &gShadowWavePlanes[gShadowWavePlaneCount],
        sp88[2].x, sp88[2].y,
        sp88[0].x, sp88[0].y
    );
    // clang-format on

    for (var_s4 = 0; var_s4 < temp_v0; var_s4++) {
        var_a0 = gShadowWaveCells[var_s4].y1;
        var_a1 = gShadowWaveCells[var_s4].y1;
        if (gShadowWaveCells[var_s4].y2 < var_a0) {
            var_a0 = gShadowWaveCells[var_s4].y2;
        } else if (var_a1 < gShadowWaveCells[var_s4].y2) {
            var_a1 = gShadowWaveCells[var_s4].y2;
        }
        if (gShadowWaveCells[var_s4].y3 < var_a0) {
            var_a0 = gShadowWaveCells[var_s4].y3;
        } else if (var_a1 < gShadowWaveCells[var_s4].y3) {
            var_a1 = gShadowWaveCells[var_s4].y3;
        }
        if (gNewShadowY2 >= var_a0) {
            if (var_a1 >= gNewShadowY1) {
                spA8[0].x = gShadowWaveCells[var_s4].x1;
                spA8[0].z = gShadowWaveCells[var_s4].z1;
                spA8[1].x = gShadowWaveCells[var_s4].x2;
                spA8[1].z = gShadowWaveCells[var_s4].z2;
                spA8[2].x = gShadowWaveCells[var_s4].x3;
                spA8[2].z = gShadowWaveCells[var_s4].z3;

                for (var_v0 = 0; var_v0 != 3; var_v0++) {
                    spA8[var_v0].planeD.clip.bucketVertexIndex = -1;
                }

                gShadowCurrentPlane = &gShadowWavePlanes[gShadowWavePlaneCount + var_s4];
                temp_v0_3 = clip_polygon_to_rect(3, spA8, 4, sp88);
                if (temp_v0_3 >= 3) {
                    tempIdx = gShadowTriCount;
                    gShadowTris[tempIdx].bucketVertexMask = 0;
                    for (var_t1 = 0; var_t1 < temp_v0_3; var_t1++) {
                        if (spA8[var_t1].planeD.clip.bucketVertexIndex < 0) {
                            var_a1 = -1;
                            var_v1 = 0;
                            while (var_v1 < gShadowVertexCount && var_a1 == -1) {
                                if ((gShadowVertices[var_v1].x == spA8[var_t1].x) &&
                                    (gShadowVertices[var_v1].z == spA8[var_t1].z)) {
                                    var_a1 = var_v1;
                                }
                                var_v1++;
                            }
                            if (var_a1 == -1) {
                                gShadowVertices[gShadowVertexCount].x = spA8[var_t1].x;
                                gShadowVertices[gShadowVertexCount].plane = gShadowCurrentPlane;
                                gShadowVertices[gShadowVertexCount].z = spA8[var_t1].z;
                                gShadowTris[tempIdx].vertexIndices[var_t1] = gShadowVertexCount++;
                            } else {
                                gShadowTris[tempIdx].vertexIndices[var_t1] = var_a1;
                            }
                        } else {
                            gShadowTris[tempIdx].vertexIndices[var_t1] = spA8[var_t1].planeD.clip.bucketVertexIndex;
                            gShadowTris[tempIdx].bucketVertexMask |= 1 << var_t1;
                        }
                    }
                    gShadowTriCount = tempIdx + 1;
                    gShadowTris[tempIdx].vertexCount = temp_v0_3;
                }
            }
        }
    }

    gShadowWavePlaneCount += temp_v0;
}

/**
 * Drops every shadow vertex onto the plane of the triangle it came from.
 */
void shadow_project_vertices(void) {
    f32 temp_f12;
    f32 temp_f16;
    ShadowPlane *var_v0;
    s32 i, j, k;
    ShadowClipVertex *curr;

    for (i = 0; i < gShadowVertexCount; i++) {
        var_v0 = gShadowVertices[i].plane;
        temp_f16 = gShadowVertices[i].x * var_v0->x;
        temp_f12 = gShadowVertices[i].z * var_v0->z;
        gShadowVertices[i].y = (f32) (-(temp_f16 + temp_f12 + var_v0->planeD.w) / var_v0->y);
    }

    for (i = 0; i < ARRAY_COUNT(gShadowBucketCounts); i++) {
        // Bucket stride: the N64 source spells the 32-entry stride as sizeof(ShadowClipVertex),
        // which is 0x20 there but larger on a host with 8-byte pointers.
        for (j = 0, k = i * 0x20; j < gShadowBucketCounts[i]; j++, k++) {
            var_v0 = gShadowBucketVertices[k].plane;
            temp_f16 = gShadowBucketVertices[k].x * var_v0->x;
            temp_f12 = gShadowBucketVertices[k].z * var_v0->z;
            gShadowBucketVertices[k].y = (f32) (-(temp_f16 + temp_f12 + var_v0->planeD.w) / var_v0->y);
        }
    }
}

/**
 * Emits the shadow (or water effect) triangles with texture coordinates from
 * the object's shadow texture, rotated with the object.
 */
void shadow_build_mesh(void) {
    s32 spAC;
    s32 var_t0;
    Triangle *tri;
    Vertex *vert;
    s32 alpha;
    // These must hold up to 7 vertices: clip_polygon_to_rect clips a triangle against 4
    // planes, and unk0 (the loop bound writing these) can reach 3+4=7. They were
    // sized [6], and the overflow corrupted gcc's stack frame (infinite loop).
    s16 sp90[8];
    s32 var_s2;
    s16 sp80[8];
    f32 temp_f18;
    f32 yRotCos;
    f32 yRotSin;
    f32 zDiff;
    f32 someY;
    f32 scale;
    f32 xDiff;
    s32 i;
    UNUSED s32 pad;

    alpha = 255;
    yRotSin = sins_f(gNewShadowObj->trans.rotation.y_rotation);
    yRotCos = coss_f(gNewShadowObj->trans.rotation.y_rotation);
    temp_f18 = gNewShadowTexture->width * 16;
    scale = ((gNewShadowTexture->width * 16) / gNewShadowWidth);
    scale *= 1.4142f;
    if (gShadowFadeStart > 0.0f) {
        someY = gNewShadowObj->trans.y_position - gShadowMinY;
        if (gShadowFadeStart < someY) {
            alpha = 255;
            alpha -= (s32) ((255 * (someY - gShadowFadeStart)) / gShadowFadeRange);
            if (alpha < 0) {
                alpha = 0;
            }
        }
        if (someY > 0.0f) {
            scale *= 1.0f + (0.005f * someY);
        }
    }
    alpha *= gShadowOpacity;
    var_s2 = 25;
    for (spAC = 0; spAC < gShadowTriCount; spAC++) {
        if ((gShadowTris[spAC].vertexCount + var_s2) >= 24) {
            gCurrShadowHeapData[gShadowTail].texture = gNewShadowTexture;
            gCurrShadowHeapData[gShadowTail].triCount = gNewShadowTriCount;
            gCurrShadowHeapData[gShadowTail].vtxCount = gNewShadowVtxCount;
            gShadowTail++;
            var_s2 = 0;
        }
        var_t0 = gShadowTris[spAC].bucketVertexMask;

        for (i = 0; i < gShadowTris[spAC].vertexCount; i++) {
            vert = &gCurrShadowVerts[gNewShadowVtxCount];
            gNewShadowVtxCount++;
            if (var_t0 & 1) {
                vert->x = gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].x;
                xDiff = gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].x - gNewShadowObj->trans.x_position;
                vert->y = (gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].y + gShadowYOffset);
                vert->z = gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].z;
                zDiff = gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].z - gNewShadowObj->trans.z_position;
            } else {
                vert->x = gShadowVertices[gShadowTris[spAC].vertexIndices[i]].x;
                xDiff = gShadowVertices[gShadowTris[spAC].vertexIndices[i]].x - gNewShadowObj->trans.x_position;
                vert->y = (gShadowVertices[gShadowTris[spAC].vertexIndices[i]].y + gShadowYOffset);
                vert->z = gShadowVertices[gShadowTris[spAC].vertexIndices[i]].z;
                zDiff = gShadowVertices[gShadowTris[spAC].vertexIndices[i]].z - gNewShadowObj->trans.z_position;
            }
            var_t0 >>= 1;
            vert->r = 255;
            vert->g = 255;
            vert->b = 255;
            vert->a = alpha;
            sp90[i] = ((((xDiff * yRotCos) - (zDiff * yRotSin)) * scale) + temp_f18);
            sp80[i] = ((((zDiff * yRotCos) + (xDiff * yRotSin)) * scale) + temp_f18);
        }

        for (i = 1; i < (gShadowTris[spAC].vertexCount - 1); i++) {
            tri = &gCurrShadowTris[gNewShadowTriCount];
            gNewShadowTriCount++;
            tri->flags = 0x40;
            tri->vi0 = var_s2 + i;
            tri->vi1 = var_s2 + i + 1;
            tri->vi2 = var_s2;
            tri->uv0.u = sp90[i];
            tri->uv0.v = sp80[i];
            tri->uv1.u = sp90[i + 1];
            tri->uv1.v = sp80[i + 1];
            tri->uv2.u = sp90[0];
            tri->uv2.v = sp80[0];
        }
        var_s2 += gShadowTris[spAC].vertexCount;
    }
}

/**
 * The light level at the shadow, from the lighting transition points of the
 * triangles under it; feeds the object's shading.
 */
f32 shadow_light_level(void) {
    f32 var_f2;
    f32 x0, z0, x1, z1, x2, z2;
    s32 temp_t5;
    s32 var_s2;
    s32 i;

    var_f2 = 0.0f;
    if (gShadowTriCount > 0) {
        if (gShadowCentreShared != 0) {
            if (gShadowCentreTri > 0) {
                var_f2 = gShadowLightLevels[gShadowCentreTri] * gShadowArea;
            }
        } else {
            for (i = 0; i < gShadowTriCount; i++) {
                if (gShadowTris[i].lightLevel > 0) {
                    temp_t5 = gShadowTris[i].bucketVertexMask;
                    if (temp_t5 & 1) {
                        x0 = gShadowBucketVertices[gShadowTris[i].vertexIndices[0]].x;
                        z0 = gShadowBucketVertices[gShadowTris[i].vertexIndices[0]].z;
                    } else {
                        x0 = gShadowVertices[gShadowTris[i].vertexIndices[0]].x;
                        z0 = gShadowVertices[gShadowTris[i].vertexIndices[0]].z;
                    }
                    temp_t5 >>= 1;
                    if (temp_t5 & 1) {
                        x1 = gShadowBucketVertices[gShadowTris[i].vertexIndices[1]].x;
                        z1 = gShadowBucketVertices[gShadowTris[i].vertexIndices[1]].z;
                    } else {
                        x1 = gShadowVertices[gShadowTris[i].vertexIndices[1]].x;
                        z1 = gShadowVertices[gShadowTris[i].vertexIndices[1]].z;
                    }
                    temp_t5 >>= 1;
                    for (var_s2 = 2; var_s2 < gShadowTris[i].vertexCount; var_s2++) {
                        if (temp_t5 & 1) {
                            x2 = gShadowBucketVertices[gShadowTris[i].vertexIndices[var_s2]].x;
                            z2 = gShadowBucketVertices[gShadowTris[i].vertexIndices[var_s2]].z;
                        } else {
                            x2 = gShadowVertices[gShadowTris[i].vertexIndices[var_s2]].x;
                            z2 = gShadowVertices[gShadowTris[i].vertexIndices[var_s2]].z;
                        }
                        temp_t5 >>= 1;
                        var_f2 +=
                            area_triangle_2d(x0, z0, x1, z1, x2, z2) * gShadowLightLevels[gShadowTris[i].lightLevel];
                        x1 = x2;
                        z1 = z2;
                    }
                }
            }
        }

        if (gShadowArea < var_f2) {
            var_f2 = gShadowArea * 0.99f;
        }
    }
    return (gShadowArea - var_f2) / gShadowArea;
}

/**
 * Whether a polygon's bounding box overlaps the rectangle (x0, z0)-(x1, x2).
 */
s32 polygon_overlaps_rect(f32 x0, f32 z0, f32 x1, f32 x2, s32 count, Vec4f *arg5) {
    if (count > 0) {
        f32 minX = arg5[0].x;
        f32 maxX = arg5[0].x;
        f32 minZ = arg5[0].z;
        f32 maxZ = arg5[0].z;
        s32 i;

        for (i = 1; i < count; i++) {
            if (arg5[i].x < minX) {
                minX = arg5[i].x;
            } else if (maxX < arg5[i].x) {
                maxX = arg5[i].x;
            }
            if (arg5[i].z < minZ) {
                minZ = arg5[i].z;
            } else if (maxZ < arg5[i].z) {
                maxZ = arg5[i].z;
            }
        }

        if ((x0 <= maxX) && (z0 <= maxZ) && (minX <= x1) && (minZ <= x2)) {
            return -1;
        }
    }

    return 0;
}

// arg0 is always 3
// arg1 always has size 8 (that's why spE0 is also of size 8)
// arg2 is always 4
/**
 * Clips a polygon against the four edges of a rectangle (Sutherland-Hodgman),
 * returning the vertex count; the output can hold up to 8 vertices.
 */
s32 clip_polygon_to_rect(s32 arg0, ShadowPlane *arg1, s32 arg2, Vec2f *arg3) {
    ShadowPlane spE0[8];
    f32 temp_f12;
    f32 temp_f14;
    f32 temp_f16;
    f32 temp_f22;
    f32 temp_f24;
    f32 var_f2;
    s32 var_a1;
    s32 var_v1_3;
    s32 var_a0;
    s32 var_t2;
    UNUSED s32 var_t5;
    s32 var_v0;
    s32 var_v1;
    s32 var_t1;
    s32 var_s2;
    ShadowPlane *var_s0;
    ShadowPlane *var_s3;
    ShadowPlane *swap; // swap var for var_s0 and var_s3

    var_s3 = arg1;
    var_s0 = spE0;
    var_s2 = arg0;
    var_v0 = 0;

    while (var_v0 < arg2 && var_s2 >= 3) {
        var_v1 = var_v0 + 1;
        if (var_v1 >= arg2) {
            var_v1 = 0;
        }

        temp_f12 = arg3[var_v1].y - arg3[var_v0].y;
        temp_f14 = -(arg3[var_v1].x - arg3[var_v0].x);
        if (arg3[var_v0].x < arg3[var_v1].x) {
            var_f2 = (temp_f12 * arg3[var_v0].x) + (arg3[var_v0].y * temp_f14);
            var_f2 = -var_f2;
        } else {
            var_f2 = (temp_f12 * arg3[var_v1].x) + (arg3[var_v1].y * temp_f14);
            var_f2 = -var_f2;
        }

        for (var_t1 = 0, var_t2 = 0; var_t1 < var_s2; var_t1++) {
            var_v1 = var_t1 + 1;
            if (var_v1 >= var_s2) {
                var_v1 = 0;
            }
            temp_f16 = (temp_f12 * var_s3[var_t1].x) + (var_s3[var_t1].z * temp_f14) + var_f2;
            temp_f22 = (temp_f12 * var_s3[var_v1].x) + (var_s3[var_v1].z * temp_f14) + var_f2;
            if ((temp_f16 >= 0.0f && temp_f22 < 0.0f) || (temp_f16 < 0.0f && temp_f22 >= 0.0f)) {
                var_a0 = gShadowBucketCounts[var_v0];
                var_a1 = -1;
                var_v1_3 = var_v0 << 5;
                while (var_a0 > 0 && var_a1 < 0) {
                    if ((gShadowBucketVertices[var_v1_3].edgeStartX == var_s3[var_t1].x) &&
                        (gShadowBucketVertices[var_v1_3].edgeStartZ == var_s3[var_t1].z) &&
                        (gShadowBucketVertices[var_v1_3].edgeEndX == var_s3[var_v1].x) &&
                        (gShadowBucketVertices[var_v1_3].edgeEndZ == var_s3[var_v1].z)) {
                        var_a1 = var_v1_3;
                    } else if ((gShadowBucketVertices[var_v1_3].edgeStartX == var_s3[var_v1].x) &&
                               (gShadowBucketVertices[var_v1_3].edgeStartZ == var_s3[var_v1].z) &&
                               (gShadowBucketVertices[var_v1_3].edgeEndX == var_s3[var_t1].x) &&
                               (gShadowBucketVertices[var_v1_3].edgeEndZ == var_s3[var_t1].z)) {
                        var_a1 = var_v1_3;
                    }
                    var_a0 -= 1;
                    var_v1_3++;
                }
                if (var_a1 >= 0) {
                    var_s0[var_t2].planeD.clip.bucketVertexIndex = var_a1;
                    var_s0[var_t2].x = gShadowBucketVertices[var_a1].x;
                    var_s0[var_t2].z = gShadowBucketVertices[var_a1].z;
                    var_t2++;
                } else {
                    temp_f24 = temp_f16 / (temp_f16 - temp_f22);
                    var_s0[var_t2].x = var_s3[var_t1].x + ((var_s3[var_v1].x - var_s3[var_t1].x) * temp_f24);
                    var_s0[var_t2].z = var_s3[var_t1].z + ((var_s3[var_v1].z - var_s3[var_t1].z) * temp_f24);
                    if (gShadowBucketCounts[var_v0] > 0x1F) {
                        gShadowBucketCounts[var_v0] = 0x1F;
                    }
                    var_v1_3 = gShadowBucketCounts[var_v0] + (var_v0 << 5);
                    gShadowBucketVertices[var_v1_3].edgeStartX = var_s3[var_t1].x;
                    gShadowBucketVertices[var_v1_3].edgeStartZ = var_s3[var_t1].z;
                    gShadowBucketVertices[var_v1_3].edgeEndX = var_s3[var_v1].x;
                    gShadowBucketVertices[var_v1_3].edgeEndZ = var_s3[var_v1].z;
                    gShadowBucketVertices[var_v1_3].x = var_s0[var_t2].x;
                    gShadowBucketVertices[var_v1_3].z = var_s0[var_t2].z;
                    gShadowBucketVertices[var_v1_3].plane = gShadowCurrentPlane;
                    gShadowBucketCounts[var_v0]++;
                    var_s0[var_t2].planeD.clip.bucketVertexIndex = var_v1_3;
                    var_t2++;
                }
            }
            if (temp_f22 <= 0.0f) {
                var_s0[var_t2].planeD.clip.bucketVertexIndex = var_s3[var_v1].planeD.clip.bucketVertexIndex;
                var_s0[var_t2].x = var_s3[var_v1].x;
                var_s0[var_t2].z = var_s3[var_v1].z;
                var_t2++;
            }
        }
        var_s2 = var_t2;
        var_v0++;

        swap = var_s3;
        var_s3 = var_s0;
        var_s0 = swap;
    }

    if (var_s2 >= 3) {
        if (var_s3 != arg1) {
            for (var_t1 = 0; var_t1 < var_s2; var_t1++) {
                arg1[var_t1].x = var_s3[var_t1].x;
                arg1[var_t1].z = var_s3[var_t1].z;
                arg1[var_t1].planeD.clip.bucketVertexIndex = var_s3[var_t1].planeD.clip.bucketVertexIndex;
            }
        }
    } else {
        var_s2 = 0;
    }
    return var_s2;
}

/**
 * Marks whether the shadow's centre falls inside the triangle, so the light
 * level and shadow placement can prefer that triangle.
 */
void shadow_check_centre_in_triangle(ShadowPlane arg0[3]) {
    f32 diff;
    f32 shadowX;
    f32 shadowZ;
    f32 var_f6;
    f32 var_f8;
    f32 var_f10;
    s32 found1;
    s32 found2;
    s32 found3;

    found1 = FALSE;
    found2 = FALSE;
    found3 = FALSE;

    shadowX = gNewShadowObj->trans.x_position;
    shadowZ = gNewShadowObj->trans.z_position;

    diff = (shadowX - arg0[0].x) * (arg0[1].z - arg0[0].z) - (arg0[1].x - arg0[0].x) * (shadowZ - arg0[0].z);
    if (diff >= 0.0f) {
        found1 = TRUE;
    }

    diff = (shadowX - arg0[1].x) * (arg0[2].z - arg0[1].z) - (arg0[2].x - arg0[1].x) * (shadowZ - arg0[1].z);
    if (diff >= 0.0f) {
        found2 = TRUE;
    }

    if (found1 != found2) {
        return;
    }

    diff = (shadowX - arg0[2].x) * (arg0[0].z - arg0[2].z) - (arg0[0].x - arg0[2].x) * (shadowZ - arg0[2].z);
    if (diff >= 0.0f) {
        found3 = TRUE;
    }
    if (found2 != found3) {
        return;
    }

    diff = -(gShadowCurrentPlane->x * shadowX + gShadowCurrentPlane->z * shadowZ + gShadowCurrentPlane->planeD.w) /
           gShadowCurrentPlane->y;
    if (gShadowMinY < diff) {
        gShadowMinY = diff;
    }
}
