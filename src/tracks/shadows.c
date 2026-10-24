#include "tracks_internal.h"

// Batches that never receive a shadow.
#define SHADOW_SKIP_BATCH_FLAGS (RENDER_HIDDEN | RENDER_DECAL | RENDER_WATER | RENDER_NO_SHADOW)
// A batch's light level (0-7) sits in bits 19-21 of its flags and indexes gShadowLightLevels.
#define BATCH_LIGHT_LEVEL(flags) (((flags) >> 19) & 7)
// Shadow triangles are flushed in batches of at most this many vertices.
#define SHADOW_BATCH_VERTS 24

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
                triCount = gCurrShadowHeapData[i].triCount; // Fakematch
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
    s32 shadowTopY;
    s32 shadowBottomY;
    s32 blockId;
    s32 gridMask;
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

    shadowTopY = (s32) obj->trans.y_position + obj->header->shadowTop;
    shadowBottomY = (s32) obj->trans.y_position + obj->header->shadowBottom;
    blockId = obj->segmentID;
    foundResult = FALSE;
    if (blockId != -1) {
        gridMask = compute_grid_overlap_mask(&gCurrentLevelModel->segmentsBoundingBoxes[blockId],
                                             obj->trans.x_position - 16.0f, obj->trans.z_position - 16.0f,
                                             obj->trans.x_position + 16.0f, obj->trans.z_position + 16.0f);
        block = &gCurrentLevelModel->segments[blockId];
        for (i = 0; i < block->numberOfBatches && !foundResult; i++) {
            if (!(block->batches[i].flags & SHADOW_SKIP_BATCH_FLAGS)) {
                batchFlags = BATCH_LIGHT_LEVEL(block->batches[i].flags);
                vertices = &block->vertices[block->batches[i].verticesOffset];
                for (j = block->batches[i].facesOffset; j < block->batches[i + 1].facesOffset && !foundResult; j++) {
                    blockId = block->triangleGridMasks[j] & gridMask;
                    if (blockId) {}
                    if (((block->triangleGridMasks[j] & gridMask) & 0xFF) &&
                        ((block->triangleGridMasks[j] & gridMask) & 0xFF00)) {
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
                        if (maxYPos >= shadowBottomY && shadowTopY >= minYPos) {
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
    s32 cheats;
    s32 inSegs[28];
    s32 i;
    s32 segs;
    f32 character_scale;
    s32 gridMask;

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
                gridMask = compute_grid_overlap_mask(&gCurrentLevelModel->segmentsBoundingBoxes[inSegs[i]],
                                                     (obj->trans.x_position - gNewShadowWidth),  // x1
                                                     (obj->trans.z_position - gNewShadowLength), // z1
                                                     (obj->trans.x_position + gNewShadowWidth),  // x2
                                                     (obj->trans.z_position + gNewShadowLength)  // z2
                );
                shadow_add_segment_triangles(&gCurrentLevelModel->segments[inSegs[i]], gridMask, isWater);
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
void shadow_add_segment_triangles(LevelModelSegment *segment, s32 gridMask, s32 isWater) {
    ShadowPlane clipPoly[8];
    Vec2f shadowRect[4];
    s32 batchIndex;
    Triangle *triangles;
    s32 nextFacesOffset;
    Vertex *vertices;
    s32 yPos;
    s32 minY;
    s32 foundIndex;
    s32 maxY;
    s32 planeIndex;
    s32 lightLevel;
    s32 clipCount;
    s32 v;
    s32 i;
    s32 curFacesOffset;

    shadowRect[0].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    shadowRect[0].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    shadowRect[1].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    shadowRect[1].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    shadowRect[2].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    shadowRect[2].y = gNewShadowObj->trans.z_position - gNewShadowLength;
    shadowRect[3].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    shadowRect[3].y = gNewShadowObj->trans.z_position - gNewShadowLength;

    for (batchIndex = 0; batchIndex < segment->numberOfBatches; batchIndex++) {
        if ((isWater && (segment->batches[batchIndex].flags & RENDER_WATER)) ||
            (!isWater && !(segment->batches[batchIndex].flags & SHADOW_SKIP_BATCH_FLAGS))) {
            curFacesOffset = segment->batches[batchIndex].facesOffset;
            nextFacesOffset = segment->batches[batchIndex + 1].facesOffset;
            vertices = &segment->vertices[segment->batches[batchIndex].verticesOffset];
            lightLevel = BATCH_LIGHT_LEVEL(segment->batches[batchIndex].flags);
            for (; curFacesOffset < nextFacesOffset; curFacesOffset++) {
                if (((segment->triangleGridMasks[curFacesOffset] & gridMask) & 0xFF) &&
                    ((segment->triangleGridMasks[curFacesOffset] & gridMask) & 0xFF00)) {
                    triangles = &segment->triangles[curFacesOffset];
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
                                clipPoly[i].x = vertices[triangles->verticesArray[i + 1]].x;
                                clipPoly[i].z = vertices[triangles->verticesArray[i + 1]].z;
                                clipPoly[i].planeD.clip.bucketVertexIndex = -1;
                            }
                            // @note while the cast to Vec4f is incorrect, polygon_overlaps_rect only reads x and z,
                            // which are both floats so this is fine as the size is the same
                            if (polygon_overlaps_rect(shadowRect[2].x, shadowRect[2].y, shadowRect[0].x,
                                                      shadowRect[0].y, 3, (Vec4f *) clipPoly) != 0) {
                                planeIndex = segment->collisionFacets[curFacesOffset].basePlaneIndex * 4;
                                gShadowCurrentPlane = (ShadowPlane *) &(segment->collisionPlanes)[planeIndex];
                                if (segment->collisionPlanes[planeIndex + 1] != 0) {
                                    if (gShadowFadeStart > 0.0f) {
                                        shadow_check_centre_in_triangle(clipPoly);
                                    }
                                    clipCount = clip_polygon_to_rect(3, clipPoly, 4, shadowRect);
                                    if (clipCount >= 3) {
                                        gShadowTris[gShadowTriCount].bucketVertexMask = 0;
                                        for (v = 0; v < clipCount; v++) {
                                            if (clipPoly[v].planeD.clip.bucketVertexIndex < 0) {
                                                foundIndex = -1;
                                                i = 0;
                                                while ((i < gShadowVertexCount) && (foundIndex == -1)) {
                                                    if ((gShadowVertices[i].x == clipPoly[v].x) &&
                                                        (gShadowVertices[i].z == clipPoly[v].z)) {
                                                        foundIndex = i;
                                                    }
                                                    i++;
                                                }
                                                if (foundIndex == -1) {
                                                    gShadowVertices[gShadowVertexCount].x = clipPoly[v].x;
                                                    gShadowVertices[gShadowVertexCount].plane = gShadowCurrentPlane;
                                                    gShadowVertices[gShadowVertexCount].z = clipPoly[v].z;
                                                    gShadowTris[gShadowTriCount].vertexIndices[v] =
                                                        gShadowVertexCount++;
                                                } else {
                                                    gShadowTris[gShadowTriCount].vertexIndices[v] = foundIndex;
                                                }
                                            } else {
                                                gShadowTris[gShadowTriCount].vertexIndices[v] =
                                                    clipPoly[v].planeD.clip.bucketVertexIndex;
                                                gShadowTris[gShadowTriCount].bucketVertexMask |= 1 << v;
                                            }
                                        }
                                        gShadowTris[gShadowTriCount].vertexCount = clipCount;
                                        gShadowTris[gShadowTriCount].lightLevel = lightLevel;
                                        gShadowTriCount += 1;
                                        if ((gShadowCentreTri >= 0) && (lightLevel != gShadowCentreTri)) {
                                            gShadowCentreShared = 0;
                                        }
                                        gShadowCentreTri = lightLevel;
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
void shadow_add_wave_triangles(s32 segmentIndex) {
    ShadowPlane clipPoly[8];
    Vec2f shadowRect[4];
    s32 corner;
    s32 minY;
    s32 maxY;
    s32 foundIndex;
    s32 cellCount;
    s32 clipCount;
    s32 cell;
    s32 v;
    s32 i;
    s32 triIndex;

    shadowRect[0].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    shadowRect[0].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    shadowRect[1].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    shadowRect[1].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    shadowRect[2].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    shadowRect[2].y = gNewShadowObj->trans.z_position - gNewShadowLength;
    shadowRect[3].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    shadowRect[3].y = gNewShadowObj->trans.z_position - gNewShadowLength;
    // clang-format off
    cellCount = wave_cells_in_rect(
        segmentIndex, gShadowWaveCells, &gShadowWavePlanes[gShadowWavePlaneCount],
        shadowRect[2].x, shadowRect[2].y,
        shadowRect[0].x, shadowRect[0].y
    );
    // clang-format on

    for (cell = 0; cell < cellCount; cell++) {
        minY = gShadowWaveCells[cell].y1;
        maxY = gShadowWaveCells[cell].y1;
        if (gShadowWaveCells[cell].y2 < minY) {
            minY = gShadowWaveCells[cell].y2;
        } else if (maxY < gShadowWaveCells[cell].y2) {
            maxY = gShadowWaveCells[cell].y2;
        }
        if (gShadowWaveCells[cell].y3 < minY) {
            minY = gShadowWaveCells[cell].y3;
        } else if (maxY < gShadowWaveCells[cell].y3) {
            maxY = gShadowWaveCells[cell].y3;
        }
        if (gNewShadowY2 >= minY) {
            if (maxY >= gNewShadowY1) {
                clipPoly[0].x = gShadowWaveCells[cell].x1;
                clipPoly[0].z = gShadowWaveCells[cell].z1;
                clipPoly[1].x = gShadowWaveCells[cell].x2;
                clipPoly[1].z = gShadowWaveCells[cell].z2;
                clipPoly[2].x = gShadowWaveCells[cell].x3;
                clipPoly[2].z = gShadowWaveCells[cell].z3;

                for (corner = 0; corner != 3; corner++) {
                    clipPoly[corner].planeD.clip.bucketVertexIndex = -1;
                }

                gShadowCurrentPlane = &gShadowWavePlanes[gShadowWavePlaneCount + cell];
                clipCount = clip_polygon_to_rect(3, clipPoly, 4, shadowRect);
                if (clipCount >= 3) {
                    triIndex = gShadowTriCount;
                    gShadowTris[triIndex].bucketVertexMask = 0;
                    for (v = 0; v < clipCount; v++) {
                        if (clipPoly[v].planeD.clip.bucketVertexIndex < 0) {
                            foundIndex = -1;
                            i = 0;
                            while (i < gShadowVertexCount && foundIndex == -1) {
                                if ((gShadowVertices[i].x == clipPoly[v].x) &&
                                    (gShadowVertices[i].z == clipPoly[v].z)) {
                                    foundIndex = i;
                                }
                                i++;
                            }
                            if (foundIndex == -1) {
                                gShadowVertices[gShadowVertexCount].x = clipPoly[v].x;
                                gShadowVertices[gShadowVertexCount].plane = gShadowCurrentPlane;
                                gShadowVertices[gShadowVertexCount].z = clipPoly[v].z;
                                gShadowTris[triIndex].vertexIndices[v] = gShadowVertexCount++;
                            } else {
                                gShadowTris[triIndex].vertexIndices[v] = foundIndex;
                            }
                        } else {
                            gShadowTris[triIndex].vertexIndices[v] = clipPoly[v].planeD.clip.bucketVertexIndex;
                            gShadowTris[triIndex].bucketVertexMask |= 1 << v;
                        }
                    }
                    gShadowTriCount = triIndex + 1;
                    gShadowTris[triIndex].vertexCount = clipCount;
                }
            }
        }
    }

    gShadowWavePlaneCount += cellCount;
}

/**
 * Drops every shadow vertex onto the plane of the triangle it came from.
 */
void shadow_project_vertices(void) {
    f32 zTerm;
    f32 xTerm;
    ShadowPlane *plane;
    s32 i, j, k;

    for (i = 0; i < gShadowVertexCount; i++) {
        plane = gShadowVertices[i].plane;
        xTerm = gShadowVertices[i].x * plane->x;
        zTerm = gShadowVertices[i].z * plane->z;
        gShadowVertices[i].y = (f32) (-(xTerm + zTerm + plane->planeD.w) / plane->y);
    }

    for (i = 0; i < ARRAY_COUNT(gShadowBucketCounts); i++) {
        // Bucket stride: the N64 source spells the 32-entry stride as sizeof(ShadowClipVertex),
        // which is 0x20 there but larger on a host with 8-byte pointers.
        for (j = 0, k = i * SHADOW_BUCKET_SIZE; j < gShadowBucketCounts[i]; j++, k++) {
            plane = gShadowBucketVertices[k].plane;
            xTerm = gShadowBucketVertices[k].x * plane->x;
            zTerm = gShadowBucketVertices[k].z * plane->z;
            gShadowBucketVertices[k].y = (f32) (-(xTerm + zTerm + plane->planeD.w) / plane->y);
        }
    }
}

/**
 * Emits the shadow (or water effect) triangles with texture coordinates from
 * the object's shadow texture, rotated with the object.
 */
void shadow_build_mesh(void) {
    s32 triIndex;
    s32 bucketMask;
    Triangle *tri;
    Vertex *vert;
    s32 alpha;
    // These must hold up to 7 vertices: clip_polygon_to_rect clips a triangle against 4
    // planes, and vertexCount (the loop bound writing these) can reach 3+4=7. They were
    // sized [6], and the overflow corrupted gcc's stack frame (infinite loop).
    s16 uCoords[8];
    s32 batchBase;
    s16 vCoords[8];
    f32 texCentre;
    f32 yRotCos;
    f32 yRotSin;
    f32 zDiff;
    f32 heightAboveFloor;
    f32 scale;
    f32 xDiff;
    s32 i;

    alpha = 255;
    yRotSin = sins_f(gNewShadowObj->trans.rotation.y_rotation);
    yRotCos = coss_f(gNewShadowObj->trans.rotation.y_rotation);
    texCentre = gNewShadowTexture->width * 16;
    scale = ((gNewShadowTexture->width * 16) / gNewShadowWidth);
    scale *= 1.4142f;
    if (gShadowFadeStart > 0.0f) {
        heightAboveFloor = gNewShadowObj->trans.y_position - gShadowMinY;
        if (gShadowFadeStart < heightAboveFloor) {
            alpha = 255;
            alpha -= (s32) ((255 * (heightAboveFloor - gShadowFadeStart)) / gShadowFadeRange);
            if (alpha < 0) {
                alpha = 0;
            }
        }
        if (heightAboveFloor > 0.0f) {
            scale *= 1.0f + (0.005f * heightAboveFloor);
        }
    }
    alpha *= gShadowOpacity;
    batchBase = SHADOW_BATCH_VERTS + 1; // Forces a new batch for the first triangle.
    for (triIndex = 0; triIndex < gShadowTriCount; triIndex++) {
        if ((gShadowTris[triIndex].vertexCount + batchBase) >= SHADOW_BATCH_VERTS) {
            gCurrShadowHeapData[gShadowTail].texture = gNewShadowTexture;
            gCurrShadowHeapData[gShadowTail].triCount = gNewShadowTriCount;
            gCurrShadowHeapData[gShadowTail].vtxCount = gNewShadowVtxCount;
            gShadowTail++;
            batchBase = 0;
        }
        bucketMask = gShadowTris[triIndex].bucketVertexMask;

        for (i = 0; i < gShadowTris[triIndex].vertexCount; i++) {
            vert = &gCurrShadowVerts[gNewShadowVtxCount];
            gNewShadowVtxCount++;
            if (bucketMask & 1) {
                vert->x = gShadowBucketVertices[gShadowTris[triIndex].vertexIndices[i]].x;
                xDiff =
                    gShadowBucketVertices[gShadowTris[triIndex].vertexIndices[i]].x - gNewShadowObj->trans.x_position;
                vert->y = (gShadowBucketVertices[gShadowTris[triIndex].vertexIndices[i]].y + gShadowYOffset);
                vert->z = gShadowBucketVertices[gShadowTris[triIndex].vertexIndices[i]].z;
                zDiff =
                    gShadowBucketVertices[gShadowTris[triIndex].vertexIndices[i]].z - gNewShadowObj->trans.z_position;
            } else {
                vert->x = gShadowVertices[gShadowTris[triIndex].vertexIndices[i]].x;
                xDiff = gShadowVertices[gShadowTris[triIndex].vertexIndices[i]].x - gNewShadowObj->trans.x_position;
                vert->y = (gShadowVertices[gShadowTris[triIndex].vertexIndices[i]].y + gShadowYOffset);
                vert->z = gShadowVertices[gShadowTris[triIndex].vertexIndices[i]].z;
                zDiff = gShadowVertices[gShadowTris[triIndex].vertexIndices[i]].z - gNewShadowObj->trans.z_position;
            }
            bucketMask >>= 1;
            vert->r = 255;
            vert->g = 255;
            vert->b = 255;
            vert->a = alpha;
            uCoords[i] = ((((xDiff * yRotCos) - (zDiff * yRotSin)) * scale) + texCentre);
            vCoords[i] = ((((zDiff * yRotCos) + (xDiff * yRotSin)) * scale) + texCentre);
        }

        for (i = 1; i < (gShadowTris[triIndex].vertexCount - 1); i++) {
            tri = &gCurrShadowTris[gNewShadowTriCount];
            gNewShadowTriCount++;
            tri->flags = BACKFACE_DRAW;
            tri->vi0 = batchBase + i;
            tri->vi1 = batchBase + i + 1;
            tri->vi2 = batchBase;
            tri->uv0.u = uCoords[i];
            tri->uv0.v = vCoords[i];
            tri->uv1.u = uCoords[i + 1];
            tri->uv1.v = vCoords[i + 1];
            tri->uv2.u = uCoords[0];
            tri->uv2.v = vCoords[0];
        }
        batchBase += gShadowTris[triIndex].vertexCount;
    }
}

/**
 * The light level at the shadow, from the lighting transition points of the
 * triangles under it; feeds the object's shading.
 */
f32 shadow_light_level(void) {
    f32 shadedArea;
    f32 x0, z0, x1, z1, x2, z2;
    s32 bucketMask;
    s32 v;
    s32 i;

    shadedArea = 0.0f;
    if (gShadowTriCount > 0) {
        if (gShadowCentreShared != 0) {
            if (gShadowCentreTri > 0) {
                shadedArea = gShadowLightLevels[gShadowCentreTri] * gShadowArea;
            }
        } else {
            for (i = 0; i < gShadowTriCount; i++) {
                if (gShadowTris[i].lightLevel > 0) {
                    bucketMask = gShadowTris[i].bucketVertexMask;
                    if (bucketMask & 1) {
                        x0 = gShadowBucketVertices[gShadowTris[i].vertexIndices[0]].x;
                        z0 = gShadowBucketVertices[gShadowTris[i].vertexIndices[0]].z;
                    } else {
                        x0 = gShadowVertices[gShadowTris[i].vertexIndices[0]].x;
                        z0 = gShadowVertices[gShadowTris[i].vertexIndices[0]].z;
                    }
                    bucketMask >>= 1;
                    if (bucketMask & 1) {
                        x1 = gShadowBucketVertices[gShadowTris[i].vertexIndices[1]].x;
                        z1 = gShadowBucketVertices[gShadowTris[i].vertexIndices[1]].z;
                    } else {
                        x1 = gShadowVertices[gShadowTris[i].vertexIndices[1]].x;
                        z1 = gShadowVertices[gShadowTris[i].vertexIndices[1]].z;
                    }
                    bucketMask >>= 1;
                    for (v = 2; v < gShadowTris[i].vertexCount; v++) {
                        if (bucketMask & 1) {
                            x2 = gShadowBucketVertices[gShadowTris[i].vertexIndices[v]].x;
                            z2 = gShadowBucketVertices[gShadowTris[i].vertexIndices[v]].z;
                        } else {
                            x2 = gShadowVertices[gShadowTris[i].vertexIndices[v]].x;
                            z2 = gShadowVertices[gShadowTris[i].vertexIndices[v]].z;
                        }
                        bucketMask >>= 1;
                        shadedArea +=
                            area_triangle_2d(x0, z0, x1, z1, x2, z2) * gShadowLightLevels[gShadowTris[i].lightLevel];
                        x1 = x2;
                        z1 = z2;
                    }
                }
            }
        }

        if (gShadowArea < shadedArea) {
            shadedArea = gShadowArea * 0.99f;
        }
    }
    return (gShadowArea - shadedArea) / gShadowArea;
}

/**
 * Whether a polygon's bounding box overlaps the rectangle (x0, z0)-(x1, z1).
 */
s32 polygon_overlaps_rect(f32 x0, f32 z0, f32 x1, f32 z1, s32 count, Vec4f *points) {
    if (count > 0) {
        f32 minX = points[0].x;
        f32 maxX = points[0].x;
        f32 minZ = points[0].z;
        f32 maxZ = points[0].z;
        s32 i;

        for (i = 1; i < count; i++) {
            if (points[i].x < minX) {
                minX = points[i].x;
            } else if (maxX < points[i].x) {
                maxX = points[i].x;
            }
            if (points[i].z < minZ) {
                minZ = points[i].z;
            } else if (maxZ < points[i].z) {
                maxZ = points[i].z;
            }
        }

        if ((x0 <= maxX) && (z0 <= maxZ) && (minX <= x1) && (minZ <= z1)) {
            return -1;
        }
    }

    return 0;
}

// inCount is always 3
// poly always has size 8 (that's why scratch is also of size 8)
// edgeCount is always 4
/**
 * Clips a polygon against the four edges of a rectangle (Sutherland-Hodgman),
 * returning the vertex count; the output can hold up to 8 vertices.
 */
s32 clip_polygon_to_rect(s32 inCount, ShadowPlane *poly, s32 edgeCount, Vec2f *rect) {
    ShadowPlane scratch[8];
    f32 edgeNormalX;
    f32 edgeNormalZ;
    f32 currDist;
    f32 nextDist;
    f32 t;
    f32 edgeD;
    s32 bucketMatch;
    s32 bucketPos;
    s32 bucketLeft;
    s32 outCount;
    s32 edge;
    s32 next;
    s32 v;
    s32 count;
    ShadowPlane *dstPoly;
    ShadowPlane *srcPoly;
    ShadowPlane *swap; // swap var for dstPoly and srcPoly

    srcPoly = poly;
    dstPoly = scratch;
    count = inCount;
    edge = 0;

    while (edge < edgeCount && count >= 3) {
        next = edge + 1;
        if (next >= edgeCount) {
            next = 0;
        }

        edgeNormalX = rect[next].y - rect[edge].y;
        edgeNormalZ = -(rect[next].x - rect[edge].x);
        if (rect[edge].x < rect[next].x) {
            edgeD = (edgeNormalX * rect[edge].x) + (rect[edge].y * edgeNormalZ);
            edgeD = -edgeD;
        } else {
            edgeD = (edgeNormalX * rect[next].x) + (rect[next].y * edgeNormalZ);
            edgeD = -edgeD;
        }

        for (v = 0, outCount = 0; v < count; v++) {
            next = v + 1;
            if (next >= count) {
                next = 0;
            }
            currDist = (edgeNormalX * srcPoly[v].x) + (srcPoly[v].z * edgeNormalZ) + edgeD;
            nextDist = (edgeNormalX * srcPoly[next].x) + (srcPoly[next].z * edgeNormalZ) + edgeD;
            if ((currDist >= 0.0f && nextDist < 0.0f) || (currDist < 0.0f && nextDist >= 0.0f)) {
                bucketLeft = gShadowBucketCounts[edge];
                bucketMatch = -1;
                bucketPos = edge << 5;
                while (bucketLeft > 0 && bucketMatch < 0) {
                    if ((gShadowBucketVertices[bucketPos].edgeStartX == srcPoly[v].x) &&
                        (gShadowBucketVertices[bucketPos].edgeStartZ == srcPoly[v].z) &&
                        (gShadowBucketVertices[bucketPos].edgeEndX == srcPoly[next].x) &&
                        (gShadowBucketVertices[bucketPos].edgeEndZ == srcPoly[next].z)) {
                        bucketMatch = bucketPos;
                    } else if ((gShadowBucketVertices[bucketPos].edgeStartX == srcPoly[next].x) &&
                               (gShadowBucketVertices[bucketPos].edgeStartZ == srcPoly[next].z) &&
                               (gShadowBucketVertices[bucketPos].edgeEndX == srcPoly[v].x) &&
                               (gShadowBucketVertices[bucketPos].edgeEndZ == srcPoly[v].z)) {
                        bucketMatch = bucketPos;
                    }
                    bucketLeft -= 1;
                    bucketPos++;
                }
                if (bucketMatch >= 0) {
                    dstPoly[outCount].planeD.clip.bucketVertexIndex = bucketMatch;
                    dstPoly[outCount].x = gShadowBucketVertices[bucketMatch].x;
                    dstPoly[outCount].z = gShadowBucketVertices[bucketMatch].z;
                    outCount++;
                } else {
                    t = currDist / (currDist - nextDist);
                    dstPoly[outCount].x = srcPoly[v].x + ((srcPoly[next].x - srcPoly[v].x) * t);
                    dstPoly[outCount].z = srcPoly[v].z + ((srcPoly[next].z - srcPoly[v].z) * t);
                    if (gShadowBucketCounts[edge] > SHADOW_BUCKET_SIZE - 1) {
                        gShadowBucketCounts[edge] = SHADOW_BUCKET_SIZE - 1;
                    }
                    bucketPos = gShadowBucketCounts[edge] + (edge << 5);
                    gShadowBucketVertices[bucketPos].edgeStartX = srcPoly[v].x;
                    gShadowBucketVertices[bucketPos].edgeStartZ = srcPoly[v].z;
                    gShadowBucketVertices[bucketPos].edgeEndX = srcPoly[next].x;
                    gShadowBucketVertices[bucketPos].edgeEndZ = srcPoly[next].z;
                    gShadowBucketVertices[bucketPos].x = dstPoly[outCount].x;
                    gShadowBucketVertices[bucketPos].z = dstPoly[outCount].z;
                    gShadowBucketVertices[bucketPos].plane = gShadowCurrentPlane;
                    gShadowBucketCounts[edge]++;
                    dstPoly[outCount].planeD.clip.bucketVertexIndex = bucketPos;
                    outCount++;
                }
            }
            if (nextDist <= 0.0f) {
                dstPoly[outCount].planeD.clip.bucketVertexIndex = srcPoly[next].planeD.clip.bucketVertexIndex;
                dstPoly[outCount].x = srcPoly[next].x;
                dstPoly[outCount].z = srcPoly[next].z;
                outCount++;
            }
        }
        count = outCount;
        edge++;

        swap = srcPoly;
        srcPoly = dstPoly;
        dstPoly = swap;
    }

    if (count >= 3) {
        if (srcPoly != poly) {
            for (v = 0; v < count; v++) {
                poly[v].x = srcPoly[v].x;
                poly[v].z = srcPoly[v].z;
                poly[v].planeD.clip.bucketVertexIndex = srcPoly[v].planeD.clip.bucketVertexIndex;
            }
        }
    } else {
        count = 0;
    }
    return count;
}

/**
 * Marks whether the shadow's centre falls inside the triangle, so the light
 * level and shadow placement can prefer that triangle.
 */
void shadow_check_centre_in_triangle(ShadowPlane tri[3]) {
    f32 diff;
    f32 shadowX;
    f32 shadowZ;
    s32 found1;
    s32 found2;
    s32 found3;

    found1 = FALSE;
    found2 = FALSE;
    found3 = FALSE;

    shadowX = gNewShadowObj->trans.x_position;
    shadowZ = gNewShadowObj->trans.z_position;

    diff = (shadowX - tri[0].x) * (tri[1].z - tri[0].z) - (tri[1].x - tri[0].x) * (shadowZ - tri[0].z);
    if (diff >= 0.0f) {
        found1 = TRUE;
    }

    diff = (shadowX - tri[1].x) * (tri[2].z - tri[1].z) - (tri[2].x - tri[1].x) * (shadowZ - tri[1].z);
    if (diff >= 0.0f) {
        found2 = TRUE;
    }

    if (found1 != found2) {
        return;
    }

    diff = (shadowX - tri[2].x) * (tri[0].z - tri[2].z) - (tri[0].x - tri[2].x) * (shadowZ - tri[2].z);
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
