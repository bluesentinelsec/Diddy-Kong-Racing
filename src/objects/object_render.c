#include "objects_internal.h"

/**
 * A few objects use unconventional means to render. They are handled here.
 */
void render_3d_misc(Object *obj) {
    if (obj_behaviour(obj->behaviorId)->render != NULL) {
        obj_behaviour(obj->behaviorId)->render(obj);
    }
}

/**
 * Render an object as a billboard.
 * A few tweaks are made depending on the behaviour ID of the object.
 * A few exceptions will not call to render a billboarded sprite.
 */
void render_3d_billboard(Object *obj) {
    s32 intensity;
    s32 flags;
    s32 alpha;
    s32 hasPrimCol;
    s32 hasEnvCol;
    ObjectTransformExt objTrans;
    Object *bubbleTrap;
    Sprite *sprite;

    intensity = 255;
    hasPrimCol = FALSE;
    hasEnvCol = FALSE;
    flags = obj->trans.flags | RENDER_Z_UPDATE | RENDER_FOG_ACTIVE;
    if (obj->shading != NULL) {
        hasPrimCol = TRUE;
        hasEnvCol = TRUE;
        intensity = obj->shading->brightness * 255.0f;
    }

    if (obj->behaviorId == BHV_BOMB_EXPLOSION) {
        //!@bug Never true, because the type is u8.
        if (obj->opacity > 255) {
            obj->opacity = obj->properties.bombExplosion.opacity & 0xFF;
        } else {
            obj->opacity = (obj->opacity * (obj->properties.bombExplosion.opacity & 0xFF)) >> 8;
        }
    }

    alpha = obj->opacity;
    if (alpha > 255) {
        alpha = 255;
    }

    // If the behavior is a wizpig ghost, then halve it's transparency.
    if (obj->behaviorId == BHV_WIZPIG_GHOSTS) {
        alpha >>= 1;
    }

    if (alpha < 255) {
        flags |= RENDER_SEMI_TRANSPARENT;
        hasPrimCol = TRUE;
    }
    if ((obj->behaviorId == BHV_WEAPON) && (obj->trans.scale == 6.0f)) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, (intensity * 3) >> 2, intensity, intensity >> 1, alpha);
        hasPrimCol = TRUE;
    } else if (obj->behaviorId == BHV_WIZPIG_GHOSTS) { // If the behavior is a wizpig ghost
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 150, 230, 255, alpha);
        hasPrimCol = TRUE;
    } else if (hasPrimCol || alpha < 255) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, intensity, intensity, intensity, alpha);
    } else {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    }
    if (hasEnvCol) {
        gDPSetEnvColor(gObjectCurrDisplayList++, obj->shading->lightR, obj->shading->lightG, obj->shading->lightB,
                       obj->shading->lightIntensity);
    } else if (obj->behaviorId == BHV_LAVA_SPURT) {
        hasEnvCol = TRUE;
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 0, 255);
    } else {
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    }
    sprite = obj->sprites[obj->modelIndex];
    bubbleTrap = NULL;
    if (obj->behaviorId == BHV_FIREBALL_OCTOWEAPON_2) {
        bubbleTrap = obj->properties.fireball.obj;
        if (obj->properties.fireball.timer > 0) {
            bubbleTrap = obj;
        }
    }

    // 5 = OilSlick, SmokeCloud, Bomb, BubbleWeapon
    if (bubbleTrap != NULL || !(obj->behaviorId != BHV_WEAPON || obj->weapon->weaponID != WEAPON_BUBBLE_TRAP)) {
        objTrans.trans.rotation.z_rotation = 0;
        objTrans.trans.rotation.x_rotation = 0;
        objTrans.trans.rotation.y_rotation = 0;
        objTrans.trans.scale = obj->trans.scale;
        objTrans.trans.x_position = 0.0f;
        objTrans.trans.z_position = 0.0f;
        objTrans.trans.y_position = 12.0f;
        objTrans.animFrame = obj->animFrame;
        objTrans.cameraOffset = 32;
        if (bubbleTrap == NULL) {
            bubbleTrap = obj->weapon->target;
            if (bubbleTrap == NULL) {
                bubbleTrap = obj;
            }
        }
        render_bubble_trap(&bubbleTrap->trans, sprite, (Object *) &objTrans,
                           RENDER_Z_COMPARE | RENDER_SEMI_TRANSPARENT | RENDER_Z_UPDATE);
    } else {
        render_sprite_billboard(&gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList, obj, sprite,
                                flags);
    }
    if (hasPrimCol) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    }
    if (hasEnvCol) {
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    }
}

/**
 * Advance the model's animation and relight its vertices. AI racers in single player and
 * split screen use the fast shader; the stopwatch icon is always shaded fully.
 */
static void model_update_vertices(Object *obj, ModelInstance *modInst, Object_Racer *racer) {
    ObjectModel *objModel;
    s32 fancyShading;

    objModel = modInst->objModel;
    obj->curVertData = modInst->vertices[modInst->animationTaskNum];
    if (modInst->modelType == MODELTYPE_ANIMATED) {
        obj_animate(obj);
    }
    if (modInst->modelType != MODELTYPE_BASIC && objModel->normals != NULL) {
        fancyShading = TRUE;
        if (racer != NULL && racer->vehicleID < VEHICLE_BOSSES && racer->playerIndex == PLAYER_COMPUTER) {
            fancyShading = FALSE;
        }
        if (cam_get_viewport_layout() != VIEWPORT_LAYOUT_1_PLAYER) {
            fancyShading = FALSE;
        }
        obj->curVertData = modInst->vertices[modInst->animationTaskNum];
        if (obj->behaviorId == BHV_UNK_3F) { // 63 = stopwatchicon, stopwatchhand
            obj_shade_fancy(objModel, obj, 0, gCurrentLightIntensity);
        } else if (fancyShading) {
            obj_shade_fancy(objModel, obj, -1, gCurrentLightIntensity);
        } else {
            obj_shade_fast(objModel, obj, gCurrentLightIntensity);
        }
    }
    // Set the animation ticker for non player racers to 2, making them animate at half the framerate.
    if ((racer != NULL) && (racer->playerIndex == PLAYER_COMPUTER) && (racer->vehicleID < VEHICLE_BOSSES)) {
        modInst->animUpdateTimer = 2;
    } else {
        modInst->animUpdateTimer = 1;
    }
}

/**
 * Draw the sprites attached to the model's attach points (balloons, held eggs, wheels'
 * decorations). Each is moved onto its attach vertex for the draw and moved back after.
 */
static void model_render_attachments(Object *obj, ObjectModel *objModel, Object_Racer *racer, s32 intensity,
                                     s32 opacity) {
    Object *attached;
    Sprite *sprite;
    s32 attachPointCount;
    s32 flags;
    s32 isCargo;
    s8 index;
    f32 vtxX;
    f32 vtxY;
    f32 vtxZ;
    s32 i;

    attachPointCount = obj->attachPoints->count;
    if (racer != NULL && racer->vehicleID == VEHICLE_FLYING_CAR) {
        attachPointCount = 0;
    }
    for (i = 0; i < attachPointCount; i++) {
        attached = obj->attachPoints->obj[i];
        if (attached->trans.flags & OBJ_FLAGS_INVISIBLE) {
            continue;
        }
        index = obj->attachPoints->attachIndices[i];
        if (index < 0 || index >= objModel->numberOfAttachPoints) {
            continue;
        }
        sprite = attached->sprites[attached->modelIndex];
        vtxX = obj->curVertData[objModel->attachPoints[index]].x;
        vtxY = obj->curVertData[objModel->attachPoints[index]].y;
        vtxZ = obj->curVertData[objModel->attachPoints[index]].z;
        attached->trans.x_position += vtxX;
        attached->trans.y_position += vtxY;
        attached->trans.z_position += vtxZ;
        if (attached->header->modelType == OBJECT_MODEL_TYPE_SPRITE_BILLBOARD) {
            flags = (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_Z_UPDATE);
        } else {
            flags = (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_Z_UPDATE | RENDER_ANTI_ALIASING);
        }
        if (opacity < 255) {
            flags |= RENDER_SEMI_TRANSPARENT;
        }
        // In this instance, cargo refers to the eggs in the fire mountain challenge.
        isCargo = (attached->trans.flags & OBJ_FLAGS_UNK_0080 && attachPointCount == 3);
        if (racer != NULL && racer->transparency < 255) {
            isCargo = FALSE;
        }
        if (isCargo) {
            deferred_dl_begin(&gObjectCurrDisplayList);
            gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, intensity, intensity, intensity, opacity);
        }
        attached->properties.vehiclePart.frontFacing = render_sprite_billboard(
            &gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList, attached, sprite, flags);
        if (isCargo) {
            gSPSelectMatrixDKR(gObjectCurrDisplayList++, G_MTX_DKR_INDEX_0);
            deferred_dl_end(&gObjectCurrDisplayList);
        }
        attached->trans.x_position -= vtxX;
        attached->trans.y_position -= vtxY;
        attached->trans.z_position -= vtxZ;
    }
}

/**
 * The egg a racer carries eases towards the model's hold point and is drawn there.
 */
static void model_render_held_object(Object *obj, ObjectModel *objModel, Object_Racer *racer) {
    Object *held;
    Sprite *sprite;
    s32 flags;
    s8 index;
    f32 vtxX;
    f32 vtxY;
    f32 vtxZ;

    held = racer->held_obj;
    if (held == NULL) {
        return;
    }
    index = obj->header->heldItemAttachPoint;
    if (index < 0 || index >= objModel->numberOfAttachPoints) {
        return;
    }
    flags = (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_Z_UPDATE);
    sprite = held->sprites[held->modelIndex];
    vtxX = obj->curVertData[objModel->attachPoints[index]].x;
    vtxY = obj->curVertData[objModel->attachPoints[index]].y;
    vtxZ = obj->curVertData[objModel->attachPoints[index]].z;
    held->trans.x_position += (vtxX - held->trans.x_position) * 0.25;
    held->trans.y_position += (vtxY - held->trans.y_position) * 0.25;
    held->trans.z_position += (vtxZ - held->trans.z_position) * 0.25;
    if (held->header->modelType == OBJECT_MODEL_TYPE_SPRITE_BILLBOARD) {
        render_sprite_billboard(&gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList, held, sprite,
                                flags);
    }
}

/**
 * Renders a 3D object, with support for vehicle part entities as part of the process.
 * Loads materials, and sets environment and/or primitive colours based on the material type.
 * Computes the view matrix for the model, and calls a function to draw meshes.
 * Loops through racers to find vehicle parts, which are wheels and propellers.
 */
void render_3d_model(Object *obj) {
    s32 intensity;
    s32 opacity;
    s32 headPushed;
    s32 hasOpacity;
    s32 hasLighting;
    s32 meshBatch;
    ModelInstance *modInst;
    Object_Racer *racer;
    ObjectModel *objModel;

    modInst = obj->modelInstances[obj->modelIndex];
    if (modInst == NULL) {
        return;
    }
    objModel = modInst->objModel;
    hasOpacity = FALSE;
    hasLighting = FALSE;
    intensity = 255;
    if (obj->shading != NULL) {
        intensity = (s32) (obj->shading->brightness * 255.0f * gCurrentLightIntensity);
        hasOpacity = TRUE;
        hasLighting = TRUE;
    }
    if (obj->behaviorId == BHV_RACER) {
        racer = obj->racer;
        object_do_player_tumble(obj);
    } else {
        racer = NULL;
    }
    if (modInst->animUpdateTimer <= 0) {
        model_update_vertices(obj, modInst, racer);
    }
    obj->curVertData = modInst->vertices[modInst->animationTaskNum];
    if (obj->behaviorId == BHV_DOOR) {
        obj_door_number(objModel, obj);
    }
    if (objModel->texOffsetUpdateRate && objModel->hasAnimatedTexture > 0) {
        obj_tex_animate(obj, objModel->texOffsetUpdateRate);
        modInst->objModel->texOffsetUpdateRate = 0;
    }
    mtx_cam_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, &obj->trans, gObjectModelScaleY, 0.0f);
    headPushed = FALSE;
    if (racer != NULL) {
        object_undo_player_tumble(obj);
        if (obj->animationID == 0 || racer->vehicleID >= VEHICLE_BOSSES) {
            mtx_head_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, modInst, racer->headAngle);
            headPushed = TRUE;
        } else {
            racer->headAngle = 0;
        }
    }
    opacity = obj->opacity;
    if (opacity > 255) {
        opacity = 255;
    }
    // If the behavior is a water zipper, then halve it's transparency.
    if (obj->behaviorId == BHV_ZIPPER_WATER) {
        opacity >>= 1;
    }
    if (opacity < 255) {
        hasOpacity = TRUE;
    }
    if (hasLighting) {
        gDPSetEnvColor(gObjectCurrDisplayList++, obj->shading->lightR, obj->shading->lightG, obj->shading->lightB,
                       obj->shading->lightIntensity);
    } else {
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    }
    if (obj->header->directionalPointLighting) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, obj->shading->shadowR, obj->shading->shadowG,
                        obj->shading->shadowB, opacity);
        directional_lighting_on();
    } else if (hasOpacity) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, intensity, intensity, intensity, opacity);
    } else {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    }
    if (opacity < 255) {
        meshBatch = render_mesh(objModel, obj, 0, RENDER_SEMI_TRANSPARENT, headPushed);
    } else {
        meshBatch = render_mesh(objModel, obj, 0, RENDER_NONE, headPushed);
    }
    if (obj->header->directionalPointLighting) {
        if (hasOpacity) {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, intensity, intensity, intensity, opacity);
        } else {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
        }
        directional_lighting_off();
    }
    if (obj->attachPoints != NULL) {
        model_render_attachments(obj, objModel, racer, intensity, opacity);
    }
    if (racer != NULL) {
        model_render_held_object(obj, objModel, racer);
    }
    // The translucent batches are drawn after everything opaque.
    if (meshBatch != -1) {
        if (obj->header->directionalPointLighting) {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, obj->shading->shadowR, obj->shading->shadowG,
                            obj->shading->shadowB, opacity);
            directional_lighting_on();
        }
        render_mesh(objModel, obj, meshBatch, RENDER_SEMI_TRANSPARENT, headPushed);
        if (obj->header->directionalPointLighting) {
            directional_lighting_off();
        }
    }
    if (hasOpacity || obj->header->directionalPointLighting) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    }
    if (hasLighting) {
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    }
    mtx_pop(&gObjectCurrDisplayList);
}

/** Deferred display lists: sections of an object's list cut out to be drawn after the track. */
void deferred_dl_reset(void) {
    gDeferredDlCount = 0;
}

void deferred_dl_flush(Gfx **dList) {
    s32 i;
    for (i = 0; i < gDeferredDlCount; i++) {
        gSPDisplayList((*dList)++, gDeferredDls[i]);
    }
}

void deferred_dl_begin(Gfx **dList) {
    if (gDeferredDlCount < 9) {
        gSPNoOp((*dList)++); // Placeholder instruction?
        gDeferredDls[gDeferredDlCount] = *dList;
    }
}

void deferred_dl_end(Gfx **dList) {
    if (gDeferredDlCount < 9) {
        gSPEndDisplayList((*dList)++);
        gSPBranchList(gDeferredDls[gDeferredDlCount] - 1, *dList);
        gDeferredDlCount++;
    }
}

/**
 * Update the object stack trace, set the draw pointers, then begin rendering the object.
 * Official Name: objPrintObject
 */
void render_object(Gfx **dList, Mtx **mtx, Vertex **verts, Object *obj) {
    f32 scale;
    if (obj->trans.flags & (OBJ_FLAGS_INVISIBLE | OBJ_FLAGS_SHADOW_ONLY)) {
        return;
    }
    update_object_stack_trace(OBJECT_DRAW, obj->objectID);
    gObjectCurrDisplayList = *dList;
    gObjectCurrMatrix = *mtx;
    gObjectCurrVertexList = *verts;
    scale = obj->trans.scale;
    render_object_parts(obj);
    obj->trans.scale = scale;
    *dList = gObjectCurrDisplayList;
    *mtx = gObjectCurrMatrix;
    *verts = gObjectCurrVertexList;
    update_object_stack_trace(OBJECT_DRAW, OBJECT_CLEAR);
}

/**
 * Official Name: objDoPlayerTumble
 */
void object_do_player_tumble(Object *this) {
    Object_Racer *racer;
    f32 rollCos;
    f32 offsetY;
    f32 upright;
    f32 tumbleOffsetY;

    if (this->behaviorId == BHV_RACER) {

        racer = this->racer;
        this->trans.rotation.y_rotation += racer->y_rotation_offset;
        this->trans.rotation.x_rotation += racer->x_rotation_offset;
        this->trans.rotation.z_rotation += racer->z_rotation_offset;
        offsetY = 0.0f;
        if (racer->vehicleIDPrev < VEHICLE_BOSSES) {

            offsetY = coss_f(racer->z_rotation_offset);
            rollCos = offsetY;
            upright = coss_f(racer->x_rotation_offset - racer->xRotationOffsetTarget) * rollCos;

            upright = (upright < 0.0f) ? 0.0f : upright * upright;

            tumbleOffsetY = (1.0f - upright) * 24.0f + racer->bodyOffsetY;
            if (0) {} // Fakematch
            offsetY = tumbleOffsetY;
        }
        this->trans.y_position = this->trans.y_position + offsetY;
        gObjectOffsetY = offsetY;
    }
}

/**
 * Official Name: objUndoPlayerTumble
 */
void object_undo_player_tumble(Object *obj) {
    if (obj->behaviorId == BHV_RACER) {
        Object_Racer *racer = obj->racer;
        obj->trans.rotation.y_rotation -= racer->y_rotation_offset;
        obj->trans.rotation.x_rotation -= racer->x_rotation_offset;
        obj->trans.rotation.z_rotation -= racer->z_rotation_offset;
        obj->trans.y_position -= gObjectOffsetY;
    }
}

void set_temp_model_transforms(Object *obj) {
    s32 batchNum;
    ObjectModel *objModel;
    s32 lodDistance;
    ModelInstance **modInstList;
    ModelInstance *modInst;
    u8 *bossAsset;
    f32 distanceScale;
    f32 lightIntensity;
    Object_Racer *objRacer;
    f32 modelScaleY;
    s32 firstNonEmptyModelIndex;
    s32 modelIndex;
    s32 numberOfModels;

    modelScaleY = 1.0f;
    lightIntensity = 1.0f;
    if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE)) {
        if (obj->header->behaviorId == BHV_RACER) {
            objRacer = obj->racer;
            objRacer->aiPhysicsTimer = 30;
            if (objRacer->stunTimer > 0) {
                lightIntensity = 1.0f - (objRacer->stunTimer * 0.05f);
                if (lightIntensity < 0.2f) {
                    lightIntensity = 0.2f;
                }
            }
            if (objRacer->playerIndex != PLAYER_COMPUTER && objRacer->raceFinished) {
                modelIndex = 0;
                batchNum = 0;
            } else {
                if (obj->behaviorId == BHV_TIMETRIAL_GHOST) { // Ghost Object?
                    modelIndex = 1;
                    batchNum = 0;
                } else {
                    // Loads vehicles between VEHICLE_TRICKY and VEHICLE_SMOKEY. So all boss vehicles except wizpig.
                    batchNum = objRacer->vehicleID;
                    if (objRacer->vehicleID >= NUMBER_OF_PLAYER_VEHICLES) {
                        batchNum = 0;
                    }
                    bossAsset = (u8 *) get_misc_asset(batchNum + VEHICLE_BOSSES); // 40 bytes of data u8[8][5]?
                    batchNum = 0;
                    bossAsset += cam_get_viewport_layout() * 10;
                    if (get_current_viewport() != objRacer->playerIndex) {
                        bossAsset += 5;
                    }
                    distanceScale = obj->distanceToCamera;
                    lodDistance = (s32) distanceScale >> 3;
                    if (obj->distanceToCamera < 0.0f) {
                        distanceScale = 0.0f;
                    } else if (distanceScale > 3500.0f) {
                        distanceScale = 3500.0f;
                    }
                    distanceScale /= 2700.0f;
                    distanceScale += 1.0f;
                    obj->trans.scale *= distanceScale;
                    lodDistance *= ((f32 *) get_misc_asset(ASSET_MISC_4))[objRacer->characterId];
                    // ASSET_MISC_4 is just 10 floats of 1.0f. One for each playable character.
                    if (lodDistance < -50) {
                        modelIndex = 5;
                    } else {
                        lodDistance >>= 1;
                        if (lodDistance < 0) {
                            lodDistance = 0;
                        }

                        if (lodDistance < bossAsset[0]) {
                            modelIndex = 0;
                        } else if (lodDistance < bossAsset[1]) {
                            modelIndex = 1;
                        } else if (lodDistance < bossAsset[2]) {
                            modelIndex = 2;
                        } else if (lodDistance < bossAsset[3]) {
                            modelIndex = 3;
                        } else if (lodDistance < bossAsset[4]) {
                            modelIndex = 4;
                        } else {
                            modelIndex = 5;
                        }
                    }
                }
            }

            firstNonEmptyModelIndex = 0;
            modInstList = &obj->modelInstances[firstNonEmptyModelIndex];

            while (*modInstList == NULL) {
                firstNonEmptyModelIndex++;
                modInstList++;
            }

            numberOfModels = obj->header->numberOfModelIds - 1;
            modInstList = &obj->modelInstances[numberOfModels];

            while (*modInstList == NULL) {
                numberOfModels--;
                modInstList--;
            }

            if (modelIndex < firstNonEmptyModelIndex) {
                modelIndex = firstNonEmptyModelIndex;
            }
            if (numberOfModels < modelIndex) {
                modelIndex = numberOfModels;
            }
            obj->modelIndex = modelIndex;
            if ((obj->shading != NULL) && (obj->shading->brightness < 0.6f)) {
                objRacer->lightFlags |= RACER_LIGHT_NIGHT;
            } else {
                objRacer->lightFlags &= ~RACER_LIGHT_NIGHT;
            }
            modelIndex = objRacer->lightFlags & RACER_LIGHT_TIMER;
            modInst = obj->modelInstances[obj->modelIndex];
            objModel = modInst->objModel;
            if (modelIndex != 0) {
                modelIndex--;
                if (objRacer->lightFlags & RACER_LIGHT_BRAKE) {
                    modelIndex += 1;
                    objRacer->lightFlags = (objRacer->lightFlags & ~RACER_LIGHT_UNK10) | RACER_LIGHT_UNK20;
                } else if (objRacer->lightFlags & RACER_LIGHT_NIGHT) {
                    modelIndex += 3;
                    objRacer->lightFlags = (objRacer->lightFlags & ~RACER_LIGHT_UNK20) | RACER_LIGHT_UNK10;
                } else if (objRacer->lightFlags & RACER_LIGHT_UNK20) {
                    modelIndex += 1;
                } else {
                    modelIndex += 3;
                }
            }
            modelIndex *= 4;
            for (batchNum = 0; batchNum < objModel->numberOfBatches; batchNum++) {
                if ((objModel->batches[batchNum].flags & 0x810000) == RENDER_TEX_ANIM) {
                    objModel->batches[batchNum].texOffset = modelIndex;
                }
            }
            obj->trans.x_position += objRacer->carBobX;
            obj->trans.y_position += objRacer->carBobY;
            obj->trans.z_position += objRacer->carBobZ;
            modelScaleY = objRacer->stretch_height;
        } else if (obj->behaviorId == BHV_FROG) {
            modelScaleY = obj->frog->scaleY;
        }
    }
    gObjectModelScaleY = modelScaleY;
    gCurrentLightIntensity = lightIntensity;
}

/**
 * Determine which model type the object is using, then call the related function to render it.
 * Beforehand, call a function to apply a temporary transformation, mostly for racers.
 * Afterwards, undo that.
 */
void render_object_parts(Object *obj) {
    set_temp_model_transforms(obj);
    if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
        render_particle((Particle *) obj, &gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList,
                        PARTICLE_UNK_FLAG_8000);
    } else {
        if (obj->header->modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
            render_3d_model(obj);
        } else if (obj->header->modelType == OBJECT_MODEL_TYPE_SPRITE_BILLBOARD) {
            render_3d_billboard(obj);
        } else if (obj->header->modelType == OBJECT_MODEL_TYPE_MISC) {
            render_3d_misc(obj);
        }
    }
    unset_temp_model_transforms(obj);
}

/**
 * After rendering, sets the object position back to normal.
 */
void unset_temp_model_transforms(Object *obj) {
    if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE) && obj->header->behaviorId == BHV_RACER) {
        obj->trans.x_position -= obj->racer->carBobX;
        obj->trans.y_position -= obj->racer->carBobY;
        obj->trans.z_position -= obj->racer->carBobZ;
    }
}

/** Draws a racer's boost flames from the boost asset for its vehicle. */
void render_boost_effect(Object *boostObj) {
    Vertex *vtx;
    Triangle *tri;
    SpriteBillboardTransform objTransform;
    Object_Boost_Inner *boostData;
    Object_Boost *boost;
    Object_Boost *asset;
    s32 hasTexture;
    s32 racerIndex;

    racerIndex = (boostObj->properties.boost.racerIndex) & 0xF;
    boost = boostObj->boost;
    switch (gBoostVehicleIds[racerIndex]) {
        case VEHICLE_CAR:
            boostData = &boost->carBoostData;
            break;
        case VEHICLE_HOVERCRAFT:
            boostData = &boost->hovercraftBoostData;
            break;
        default:
            boostData = &boost->flyingBoostData;
            break;
    }
    asset = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    asset = &asset[gBoostTypes[racerIndex]];
    object_do_player_tumble(boostObj->properties.boost.obj);
    mtx_cam_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, &boostObj->properties.boost.obj->trans, 1.0f, 0.0f);
    object_undo_player_tumble(boostObj->properties.boost.obj);
    objTransform.trans.x_position = boostData->position.x;
    objTransform.trans.y_position = boostData->position.y;
    objTransform.trans.z_position = boostData->position.z;
    objTransform.trans.scale =
        boostData->spriteScaleBase + (boostData->spriteScalePulse * coss_f(boost->pulseTimer << 12));
    if (boost->effectState < BOOST_FX_ACTIVE) {
        objTransform.trans.scale *= boost->effectScale;
    }
    if (gBoostTypes[racerIndex] != 0) {
        objTransform.trans.scale *= 1.15f;
    }
    objTransform.trans.rotation.z_rotation = 0;
    objTransform.trans.rotation.x_rotation = 0;
    objTransform.trans.rotation.y_rotation = 0;
    objTransform.animFrame = 0;
    gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    render_sprite_billboard(&gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList,
                            (Object *) &objTransform, asset->sprite,
                            (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_Z_UPDATE));
    if (boost->effectState == BOOST_FX_ACTIVE) {
        material_set(&gObjectCurrDisplayList, asset->tex,
                     (RENDER_Z_COMPARE | RENDER_SEMI_TRANSPARENT | RENDER_FOG_ACTIVE), 0);
        if (asset->tex != NULL) {
            hasTexture = TRUE;
        } else {
            hasTexture = FALSE;
        }

        vtx = &gBoostVerts[gBoostVertFlip][(boostObj->properties.boost.indexes >> 14) & 0x3FFF];
        tri = &gBoostTris[gBoostVertFlip][boostObj->properties.boost.indexes & 0x3FFF];
        gSPVertexDKR(gObjectCurrDisplayList++, OS_K0_TO_PHYSICAL(vtx), BOOST_VERT_COUNT, 0);
        gSPPolygon(gObjectCurrDisplayList++, OS_K0_TO_PHYSICAL(tri), BOOST_TRI_COUNT, hasTexture);
    }
    mtx_pop(&gObjectCurrDisplayList);
}

/**
 * Render the bubble trap weapon.
 */
void render_bubble_trap(ObjectTransform *trans, Sprite *gfxData, Object *obj, s32 flags) {
    f32 x;
    f32 y;
    f32 z;
    Camera *cameraSegment;
    f32 dist;

    vec3f_rotate(&trans->rotation, &obj->trans.position);
    obj->trans.x_position += trans->x_position;
    obj->trans.y_position += trans->y_position;
    obj->trans.z_position += trans->z_position;
    cameraSegment = cam_get_active_camera();
    x = cameraSegment->trans.x_position - obj->trans.x_position;
    y = cameraSegment->trans.y_position - obj->trans.y_position;
    z = cameraSegment->trans.z_position - obj->trans.z_position;
    dist = sqrtf((x * x) + (y * y) + (z * z));
    if (dist > 0.0) {
        dist = obj->numActiveEmitters / dist;
        x *= dist;
        y *= dist;
        z *= dist;
    }
    obj->trans.x_position += x;
    obj->trans.y_position += y;
    obj->trans.z_position += z;
    render_sprite_billboard(&gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList, obj, gfxData, flags);
}

/**
 * Get the racer object data, and fetch set visual shield properties based on that racer.
 * Afterwards, render the graphics with opacity scaling with the fadetimer.
 */
void render_racer_shield(Gfx **dList, Mtx **mtx, Vertex **vtxList, Object *obj) {
    Object_Racer *racer;
    ModelInstance *modInst;
    ObjectModel *mdl;
    RacerShieldGfx *shield;
    s32 shieldType;
    s32 vehicleID;
    s32 racerIndex;
    f32 scale;
    f32 shear;

    racer = obj->racer;
    if (racer->shieldTimer > 0 && gShieldEffectObject != NULL) {
        gObjectCurrDisplayList = *dList;
        gObjectCurrMatrix = *mtx;
        gObjectCurrVertexList = *vtxList;
        racerIndex = racer->racerIndex;
        if (racerIndex > NUMBER_OF_CHARACTERS) {
            racerIndex = 0;
        }
        vehicleID = racer->vehicleID;
        if (vehicleID >= NUMBER_OF_PLAYER_VEHICLES) {
            vehicleID = VEHICLE_CAR;
        }
        shield = (RacerShieldGfx *) get_misc_asset(ASSET_MISC_SHIELD_DATA);
        vehicleID = (vehicleID * NUMBER_OF_CHARACTERS) + racerIndex;
        shield = shield + vehicleID;
        gShieldEffectObject->trans.x_position = shield->x_position;
        gShieldEffectObject->trans.y_position = shield->y_position;
        gShieldEffectObject->trans.z_position = shield->z_position;
        gShieldEffectObject->trans.y_position += shield->y_offset * sins_f(gShieldSineTime[racerIndex] * 0x200);
        shear = (coss_f(gShieldSineTime[racerIndex] * 0x400) * 0.05f) + 0.95f;
        gShieldEffectObject->trans.scale = shield->scale * shear;
        shear = shear * shield->turnSpeed;
        gShieldEffectObject->trans.rotation.y_rotation = gShieldSineTime[racerIndex] * 0x800;
        gShieldEffectObject->trans.rotation.x_rotation = 0x800;
        gShieldEffectObject->trans.rotation.z_rotation = 0;
        shieldType = racer->shieldType;
        if (shieldType != SHIELD_NONE) {
            shieldType--;
        }
        if (shieldType > SHIELD_LEVEL3 - 1) {
            shieldType = SHIELD_LEVEL3 - 1;
        }
        scale = ((f32) shieldType * 0.1) + 1.0f;
        gShieldEffectObject->trans.scale *= scale;
        shear *= scale;
        modInst = gShieldEffectObject->modelInstances[shieldType];
        mdl = modInst->objModel;
        gShieldEffectObject->curVertData = modInst->vertices[modInst->animationTaskNum];
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
        if (racer->shieldTimer < 64) {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, racer->shieldTimer * 4);
        } else {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
        }
        mtx_shear_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, gShieldEffectObject, obj, shear);
        render_mesh(mdl, gShieldEffectObject, 0, RENDER_SEMI_TRANSPARENT, 0);
        gSPSelectMatrixDKR(gObjectCurrDisplayList++, G_MTX_DKR_INDEX_0);
        if (racer->shieldTimer < 64) {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
        }
        *dList = gObjectCurrDisplayList;
        *mtx = gObjectCurrMatrix;
        *vtxList = gObjectCurrVertexList;
    }
}

/**
 * Get the racer object data, and fetch set visual magnet properties based on that racer.
 * Afterwards, render the graphics with opacity set by the properties.
 */
void render_racer_magnet(Gfx **dList, Mtx **mtx, Vertex **vtxList, Object *obj) {
    Object_Racer *racer;
    ModelInstance *modInst;
    ObjectModel *mdl;
    f32 *magnet;
    s32 vehicleID;
    s32 racerIndex;
    s32 opacity;
    f32 shear;

    racer = obj->racer;
    racerIndex = racer->racerIndex;
    if (gRacerFXData[racerIndex].magnetFade != 0) {
        if (gMagnetEffectObject != NULL) {
            gObjectCurrDisplayList = *dList;
            gObjectCurrMatrix = *mtx;
            gObjectCurrVertexList = *vtxList;
            magnet = (f32 *) get_misc_asset(ASSET_MISC_MAGNET_DATA);
            vehicleID = racer->vehicleID;
            if (vehicleID < VEHICLE_CAR || vehicleID >= NUMBER_OF_PLAYER_VEHICLES) {
                vehicleID = VEHICLE_CAR;
            }
            magnet = &magnet[vehicleID * 5];
            racerIndex = racer->racerIndex;
            if (racerIndex > NUMBER_OF_CHARACTERS) {
                racerIndex = 0;
            }
            gMagnetEffectObject->trans.x_position = magnet[0];
            gMagnetEffectObject->trans.y_position = magnet[1];
            gMagnetEffectObject->trans.z_position = magnet[2];
            magnet += 3;
            shear = (coss_f((gRacerFXData[racerIndex].magnetPulseTimer * 0x400)) * 0.02f) + 0.98f;
            gMagnetEffectObject->trans.scale = magnet[0] * shear;
            magnet += 1;
            shear = magnet[0] * shear;
            gMagnetEffectObject->trans.rotation.y_rotation = gRacerFXData[racerIndex].magnetSpinTimer * 0x1000;
            gMagnetEffectObject->trans.rotation.x_rotation = 0;
            gMagnetEffectObject->trans.rotation.z_rotation = 0;
            modInst = gMagnetEffectObject->modelInstances[0];
            mdl = modInst->objModel;
            gMagnetEffectObject->curVertData = modInst->vertices[modInst->animationTaskNum];
            opacity = ((gRacerFXData[racerIndex].magnetPulseTimer * 8) & 0x7F) + 0x80;
            gfx_init_basic_xlu(&gObjectCurrDisplayList, DRAW_BASIC_2CYCLE, COLOUR_RGBA32(255, 255, 255, opacity),
                               gMagnetColours[racer->magnetModelID]);
            mtx_shear_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, gMagnetEffectObject, obj, shear);
            gObjectTexAnim = TRUE;
            render_mesh(mdl, gMagnetEffectObject, 0, RENDER_SEMI_TRANSPARENT, 0);
            gObjectTexAnim = FALSE;
            gSPSelectMatrixDKR(gObjectCurrDisplayList++, G_MTX_DKR_INDEX_0);
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
            rendermode_reset(&gObjectCurrDisplayList);
            *dList = gObjectCurrDisplayList;
            *mtx = gObjectCurrMatrix;
            *vtxList = gObjectCurrVertexList;
        }
    }
}

/** Offsets the texture coordinates of an object's model by the header's scroll rates times updateRate. */
void object_scroll_texture(Object *obj, s32 updateRate) {
    ObjectHeader *objHeader;
    s16 width;
    s16 height;
    s32 i;
    s32 j;
    s32 k;
    s32 end;
    s16 texIndex;
    s16 modelIndex;
    ObjectModel *objMdl;
    ModelInstance *modInst;
    TextureInfo *texInfo;
    Triangle *tri;
    s16 scrollU;
    s16 scrollV;
    s16 newU1;
    s16 newU2;
    s16 newV1;
    s16 newV2;

    objHeader = obj->header;
    modelIndex = objHeader->texScrollModelIndex;
    texIndex = objHeader->texScrollTextureIndex;
    scrollU = (s16) (objHeader->texScrollSpeedU * updateRate);
    scrollV = (s16) (objHeader->texScrollSpeedV * updateRate);
    if ((modelIndex == 0xFF) || (modelIndex < objHeader->numberOfModelIds)) {
        if (modelIndex == 0xFF) {
            end = objHeader->numberOfModelIds;
            modelIndex = 0;
        } else {
            end = modelIndex + 1;
        }
        for (i = modelIndex; i < end; i++) {
            modInst = obj->modelInstances[i];
            objMdl = modInst->objModel;
            if (texIndex < objMdl->numberOfTextures) {
                width = objMdl->textures[texIndex].texture->width << 5;
                height = objMdl->textures[texIndex].texture->height << 5;
                for (j = 0; j < objMdl->numberOfBatches; j++) {
                    if (texIndex == objMdl->batches[j].textureIndex) {
                        for (k = objMdl->batches[j].facesOffset; k < objMdl->batches[j + 1].facesOffset; k++) {
                            tri = &objMdl->triangles[k];
                            newU1 = (tri->uv1.u - tri->uv0.u);
                            newV1 = (tri->uv1.v - tri->uv0.v);
                            newU2 = (tri->uv2.u - tri->uv0.u);
                            newV2 = (tri->uv2.v - tri->uv0.v);
                            // s16 casts required
                            tri->uv0.u = (tri->uv0.u + scrollU) & (s16) (width - 1);
                            tri->uv0.v = (tri->uv0.v + scrollV) & (s16) (height - 1);
                            tri->uv1.u = tri->uv0.u + newU1;
                            tri->uv1.v = tri->uv0.v + newV1;
                            tri->uv2.u = tri->uv0.u + newU2;
                            tri->uv2.v = tri->uv0.v + newV2;
                        }
                    }
                }
            }
        }
    }
}

/**
 * Loop through every object.
 * Check which ones have 3D models and count down the update timer.
 * The object will update its animation at 0.
 */
void obj_tick_anims(void) {
    s32 i = gObjectListStart;
    s32 j;
    Object *currObj;
    ModelInstance *modInst;

    for (; i < gObjectCount; i++) {
        currObj = gObjPtrList[i];
        if (!(currObj->trans.flags & OBJ_FLAGS_PARTICLE) && currObj->header->modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
            for (j = 0; j < currObj->header->numberOfModelIds; j++) {
                modInst = currObj->modelInstances[j];
                if (modInst != NULL && modInst->animUpdateTimer > 0) {
                    modInst->animUpdateTimer &= 3;
                    modInst->animUpdateTimer--;
                }
            }
        }
    }
}

/**
 * Renders every triangle batch in an objects mesh.
 * If vertOffset is true, then draw in two passes, utilising the head matrix and vertex ID offset in the batch.
 */
s32 render_mesh(ObjectModel *objModel, Object *obj, s32 startIndex, s32 flags, s32 overrideVerts) {
    s32 i;
    s32 textureIndex;
    s32 triOffset;
    TextureHeader *texToSet;
    s32 endLoop;
    s32 numTris;
    s32 texEnabled;
    s32 texOffset;
    s32 numVertices;
    Vertex *vtx;
    s32 offsetStartVertex;
    s32 texToSetFlags;
    Triangle *tris;
    s32 vertOffset;
    Gfx *dList;

    dList = gObjectCurrDisplayList;
    i = startIndex;
    endLoop = FALSE;
    while (i < objModel->numberOfBatches && !endLoop) {
        if (!(objModel->batches[i].flags & RENDER_SEMI_TRANSPARENT) || flags & RENDER_SEMI_TRANSPARENT) {
            // Hidden/Invisible geometry
            textureIndex = objModel->batches[i].flags & RENDER_HIDDEN;
            // Probably a fakematch to use textureIndex here, but it works.
            if (!textureIndex) {
                vertOffset = objModel->batches[i].verticesOffset;
                triOffset = objModel->batches[i].facesOffset;
                numVertices = objModel->batches[i + 1].verticesOffset - vertOffset;
                offsetStartVertex = (overrideVerts) ? objModel->batches[i].vertOverride : numVertices;
                numTris = objModel->batches[i + 1].facesOffset - triOffset;
                tris = &objModel->triangles[triOffset];
                vtx = &obj->curVertData[vertOffset];
                textureIndex = objModel->batches[i].textureIndex;
                // textureIndex of 0xFF is no texture
                if (textureIndex == 0xFF) {
                    texOffset = 0;
                    texToSet = NULL;
                    texEnabled = FALSE;
                } else {
                    texOffset = objModel->batches[i].texOffset << 14;
                    texEnabled = TRUE;
                    texToSet = objModel->textures[textureIndex].texture;
                }
                texToSetFlags = objModel->batches[i].flags | RENDER_FOG_ACTIVE;
                if (flags & RENDER_SEMI_TRANSPARENT &&
                    !(objModel->batches[i].flags & (flags & ~RENDER_SEMI_TRANSPARENT))) {
                    texToSetFlags |= RENDER_SEMI_TRANSPARENT;
                }
                if (gObjectTexAnim == FALSE) {
                    material_set(&dList, texToSet, texToSetFlags, texOffset);
                } else {
                    texToSet = set_animated_texture_header(texToSet, texOffset);
                    gDkrDmaDisplayList(gObjectCurrDisplayList++, OS_K0_TO_PHYSICAL(texToSet->cmd),
                                       texToSet->numberOfCommands);
                }
                if (offsetStartVertex == numVertices) {
                    gSPVertexDKR(dList++, OS_K0_TO_PHYSICAL(vtx), numVertices, 0);
                } else {
                    if (offsetStartVertex > 0) {
                        gSPVertexDKR(dList++, OS_K0_TO_PHYSICAL(vtx), offsetStartVertex, 0);
                        gSPSelectMatrixDKR(dList++, G_MTX_DKR_INDEX_2);
                        gSPVertexDKR(dList++, OS_K0_TO_PHYSICAL(&vtx[offsetStartVertex]),
                                     (numVertices - offsetStartVertex), 1);
                    } else {
                        gSPSelectMatrixDKR(dList++, G_MTX_DKR_INDEX_2);
                        gSPVertexDKR(dList++, OS_K0_TO_PHYSICAL(vtx), numVertices, 0);
                    }
                    gSPSelectMatrixDKR(dList++, G_MTX_DKR_INDEX_1);
                }
                gSPPolygon(dList++, OS_K0_TO_PHYSICAL(tris), numTris, texEnabled);
            }
            i++;
        } else {
            endLoop = TRUE;
        }
    }
    if (i >= objModel->numberOfBatches) {
        i = -1;
    }
    gObjectCurrDisplayList = dList;
    return i;
}
