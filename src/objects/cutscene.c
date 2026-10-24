#include "objects_internal.h"

typedef struct AnimPathPoints {
    f32 x[5];
    f32 y[5];
    f32 z[5];
    f32 scale[5];
    f32 yaw[5];
    f32 pitch[5];
    f32 roll[5];
} AnimPathPoints;

/**
 * Return the index of the currently active cutscene.
 */
s16 cutscene_id(void) {
    return gCutsceneID;
}

/**
 * Set the current cutscene index.
 */
void cutscene_id_set(s32 cutsceneID) {
    gCutsceneID = cutsceneID;
}

/** Switches to a cutscene by id, flagging the change for the animation system. */
void cutscene_set(s32 cutsceneID) {
    if (cutsceneID != gCutsceneID) {
        gCutsceneID = cutsceneID;
        gPathUpdateOff = FALSE;
        gCutsceneChanged = TRUE;
        if (get_game_mode() == GAMEMODE_MENU) {
            set_frame_blackout_timer();
        }
    }
}

/**
 * Returns the index of the standard object list.
 * Goes unused, since objGetObjList exists
 */
UNUSED s32 get_object_list_index(void) {
    return gObjectListStart;
}

/** Cutscene animations: links animation entries to the objects they drive. */
void anim_objects_prepare(void) {
    Object *obj;
    s32 lastObjCount;
    s32 curObjCount;
    s32 i;
    s32 j;
    LevelObjectEntry_Animation *entryAnimation;

    for (i = 0; i < gObjectCount; i++) {
        gObjPtrList[i]->trans.flags &= ~OBJ_FLAGS_UNK_2000;
    }
    for (i = 0; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (obj != NULL && !(obj->trans.flags & OBJ_FLAGS_PARTICLE) && obj->behaviorId == BHV_ANIMATION) {
            entryAnimation = &obj->level_entry->animation;
            if (entryAnimation->channel != gCutsceneID && entryAnimation->channel != 20) {
                obj->trans.flags |= OBJ_FLAGS_UNK_2000;
                if (obj->animTarget != NULL) {
                    obj->animTarget->trans.flags |= OBJ_FLAGS_UNK_2000;
                }
            }
        }
    }
    curObjCount = gObjectCount - 1;
    lastObjCount = curObjCount;
    for (i = 0; curObjCount >= i;) {
        for (j = 0; lastObjCount >= i && (j == 0);) {
            if (gObjPtrList[i]->trans.flags & OBJ_FLAGS_UNK_2000) {
                i++;
            } else {
                j = -1;
            }
        }
        for (j = 0; curObjCount >= 0 && j == 0;) {
            if (gObjPtrList[curObjCount]->trans.flags & OBJ_FLAGS_UNK_2000) {
                j = -1;
            } else {
                curObjCount--;
            }
        }
        if (i < curObjCount) {
            obj = gObjPtrList[i];
            gObjPtrList[i] = gObjPtrList[curObjCount];
            gObjPtrList[curObjCount] = obj;
            i++;
            curObjCount--;
        }
    }
    gObjectListStart = i;
    gFirstActiveObjectId = 0;
}

/** Applies the cutscene's override positions to the objects they target. */
void override_pos_apply(s8 arg0) {
    LevelObjectEntry_OverridePos *overridePosEntry;
    Object *overridePosObj;
    Object_OverridePos *overridePos;
    s32 i;
    s32 j;
    s32 someBool;
    Object *animTarget;

    for (i = 0; i < gOverridePosCount; i++) {
        overridePosObj = gOverridePosObjects[i];
        overridePosEntry = &overridePosObj->level_entry->overridePos;
        overridePos = overridePosObj->override_pos;
        if ((overridePosEntry->cutsceneId == gCutsceneID) || ((overridePosEntry->cutsceneId == 20))) {
            for (j = 0; (j < gAnimObjectCount) &&
                        (overridePosEntry->behaviorId != gAnimObjects[j]->properties.animation.behaviourID);
                 j++) {}
            if (j != gAnimObjectCount && gAnimObjects[j]->animTarget != NULL) {
                someBool = (gAnimObjects[j]->animTarget->collisionData != NULL) ? FALSE : TRUE;
                if (arg0 != someBool) {
                    animTarget = gAnimObjects[j]->animTarget;
                    overridePos->x = animTarget->trans.x_position;
                    overridePos->y = animTarget->trans.y_position;
                    overridePos->z = animTarget->trans.z_position;
                    overridePos->anim = animTarget;
                    animTarget->trans.x_position = overridePosObj->trans.x_position;
                    animTarget->trans.y_position = overridePosObj->trans.y_position;
                    animTarget->trans.z_position = overridePosObj->trans.z_position;
                }
            } else {
                overridePos->anim = NULL;
            }
        }
    }
    gZoneJustLoaded = FALSE;
}

void override_pos_update(void) {
    s32 i;
    Object *obj;
    Object_OverridePos *obj64;

    // some flag, flips to 1 when loading a new zone
    if (gZoneJustLoaded != FALSE) {
        gZoneJustLoaded = FALSE;
        return;
    }

    // loading (boss) cutscene
    for (i = 0; i < gOverridePosCount; i++) {
        obj = gOverridePosObjects[i];
        obj64 = obj->override_pos;

        if (obj64->anim != NULL) {
            obj64->anim->trans.x_position = obj64->x;
            obj64->anim->trans.y_position = obj64->y;
            obj64->anim->trans.z_position = obj64->z;
        }
    }
}

/** Per-frame update of the cutscene animation objects and their actors. */
void anim_objects_update(void) {
    s32 pad[3];
    LevelObjectEntry_OverridePos *overridePos;
    Object *obj;
    s32 numOfObjs;
    s32 pad2;
    s32 i;
    s32 stopLooping;
    s32 sp28;
    s16 animActorIndex1;
    s16 animActorIndex2;
    s32 var_a0;
    Object *animObj1;
    LevelObjectEntry_Animation *animation1;
    LevelObjectEntry_Animation *animation2;

    if (gCutsceneChanged) {
        for (numOfObjs = 0; numOfObjs < gAnimObjectCount; numOfObjs++) {
            obj = gAnimObjects[numOfObjs];
            animation1 = &obj->level_entry->animation;
            if (obj->animTarget != NULL && animation1->channel != 20) {
                animObj1 = obj->animTarget;
                free_object(animObj1);
                obj->animTarget = NULL;
            }
        }
    }
    if (gAnimCameraCount > 20) {
        gAnimCameraCount = 0;
    }
    anim_objects_prepare();
    numOfObjs = 0;
    for (i = 0; i < gObjectCount; i++) {
        if (gObjPtrList[i] != NULL) {
            if (!(gObjPtrList[i]->trans.flags & OBJ_FLAGS_PARTICLE)) {
                if (gObjPtrList[i]->behaviorId == BHV_OVERRIDE_POS) {
                    overridePos = &gObjPtrList[i]->level_entry->overridePos;
                    if (overridePos->cutsceneId == gCutsceneID ||
                        overridePos->cutsceneId == (CUTSCENE_SHERBET_ISLAND_BOSS | CUTSCENE_ADVENTURE_TWO)) {
                        gOverridePosObjects[numOfObjs] = gObjPtrList[i];
                        numOfObjs++;
                    }
                }
            }
        }
    }
    gOverridePosCount = numOfObjs;
    gZoneJustLoaded = TRUE;

    gAnimObjectCount = 0;
    numOfObjs = 0;
    for (i = gObjectListStart; i < gObjectCount; i++) {
        if (gObjPtrList[i] != NULL) {
            if (!(gObjPtrList[i]->trans.flags & OBJ_FLAGS_PARTICLE)) {
                if (gObjPtrList[i]->behaviorId == BHV_ANIMATION) {
                    gAnimObjects[numOfObjs] = gObjPtrList[i];
                    numOfObjs++;
                }
            }
        }
    }

    do {
        stopLooping = TRUE;
        for (i = 0; i < numOfObjs - 1; i++) {
            animation1 = &gAnimObjects[i]->level_entry->animation;
            animation2 = &gAnimObjects[i + 1]->level_entry->animation;
            animActorIndex1 = animation1->actorIndex;
            animActorIndex2 = animation2->actorIndex;

            if (animation1->channel == 20) {
                animActorIndex1 -= 400;
            }
            if (animation2->channel == 20) {
                animActorIndex2 -= 400;
            }

            if (animActorIndex2 < animActorIndex1) {
                animObj1 = gAnimObjects[i];
                gAnimObjects[i] = gAnimObjects[i + 1];
                gAnimObjects[i + 1] = animObj1;
                stopLooping = FALSE;
            } else if (animActorIndex1 == animActorIndex2) {
                if (animation2->order < animation1->order) {
                    animObj1 = gAnimObjects[i];
                    gAnimObjects[i] = gAnimObjects[i + 1];
                    gAnimObjects[i + 1] = animObj1;
                    stopLooping = FALSE;
                } else if (animation1->order == animation2->order &&
                           (gAnimObjects[i + 1]->properties.animation.action == 1 ||
                            gAnimObjects[i]->properties.animation.action == 2)) {
                    animObj1 = gAnimObjects[i];
                    gAnimObjects[i] = gAnimObjects[i + 1];
                    gAnimObjects[i + 1] = animObj1;
                    stopLooping = FALSE;
                }
            }
        }
    } while (stopLooping == FALSE);

    var_a0 = -101;
    for (i = 0; i < numOfObjs; i += 1) {
        animation1 = &gAnimObjects[i]->level_entry->animation;
        if (animation1->actorIndex != var_a0) {
            var_a0 = animation1->actorIndex;
            sp28 = 0;
        }
        animation1->order = sp28++; // It is possible that sp28 could not be initalized?
        gAnimObjects[i]->properties.animation.action = 0;
    }

    gAnimObjectCount = numOfObjs;
    if (gCutsceneChanged) {
        anim_objects_spawn_targets();
    }
    gCutsceneChanged = FALSE;
}

void anim_objects_spawn_targets(void) {
    LevelObjectEntry_Animation *animation;
    Object *obj;
    s32 i;

    for (i = 0; i < gAnimObjectCount; i++) {
        obj = gAnimObjects[i];
        animation = &obj->level_entry->animation;
        if (obj->animTarget == NULL && animation->order == 0 && animation->objectIdToSpawn != -1) {
            anim_object_spawn_target(obj, animation);
        }
        if (gAnimInitAllChannels || animation->channel != 20) {
            if (obj->animTarget != NULL) {
                obj_init_animobject(obj, obj->animTarget);
            }
        }
    }
    gAnimInitAllChannels = FALSE;
}

void obj_init_animobject(Object *animationObj, Object *animatedObj) {
    LevelObjectEntry_Animation *animEntry;
    Object_AnimatedObject *anim;
    f32 scale;

    animEntry = &animationObj->level_entry->animation;
    anim = animatedObj->animatedObject;
    scale = animEntry->scale & 0xFF;
    if (scale < 1.0f) {
        scale = 1.0f;
    }
    scale /= 64;
    animatedObj->trans.scale = animatedObj->header->scale * scale;
    animatedObj->properties.animatedObj.attachRacerID = 0;
    animatedObj->properties.animatedObj.pathIdle = 0;
    if (animEntry->attachMode >= 2 && animEntry->attachMode < 10) {
        animatedObj->properties.animatedObj.attachRacerID = animEntry->attachMode - 1;
    }
    if (animEntry->attachMode >= 10 && animEntry->attachMode < 18) {
        animatedObj->properties.animatedObj.attachRacerID = animEntry->attachMode - 9;
    }
    animatedObj->trans.x_position = animationObj->trans.x_position;
    animatedObj->trans.y_position = animationObj->trans.y_position;
    animatedObj->trans.z_position = animationObj->trans.z_position;
    animatedObj->trans.rotation.y_rotation = animationObj->trans.rotation.y_rotation;
    animatedObj->trans.rotation.z_rotation = animationObj->trans.rotation.z_rotation;
    animatedObj->trans.rotation.x_rotation = animationObj->trans.rotation.x_rotation;
    anim->nodeIndex = 0;
    anim->channel = animEntry->channel;
    anim->actorIndex = animEntry->actorIndex;
    anim->speed = (f32) animEntry->nodeSpeed * 0.1;
    anim->startDelay = normalise_time(animEntry->animationStartDelay);
    animatedObj->animationID = animEntry->objAnimIndex;
    animatedObj->animFrame = animEntry->animStartFrame;
    anim->z = animEntry->objAnimSpeed;
    anim->y = 0;
    anim->loopType = animEntry->objAnimLoopType;
    anim->rotateType = animEntry->rotateType;
    anim->targetActorIndex = animEntry->nextAnim;
    anim->linearInterp = animEntry->linearInterp;
    anim->yawSpinSpeed = animEntry->yawSpinSpeed;
    anim->rollSpinSpeed = animEntry->rollSpinSpeed;
    anim->pitchSpinSpeed = animEntry->pitchSpinSpeed;
    anim->skipButtons = animEntry->skipButtons;
    anim->modelReversing = 0;
    anim->segmentStep = 0;
    anim->segmentProgress = 0;
    animationObj->particleEmitter = NULL;
    anim->pauseCounter = normalise_time(animEntry->pauseFrameCount);
    anim->specialHide = animEntry->specialHide;
    if (animEntry->emitterMask >= 0) {
        anim->emitterMask = animEntry->emitterMask;
    }
    anim->musicSequence = animEntry->musicSequence;
    anim->soundID = animEntry->soundID;
    anim->fadeType = animEntry->fadeType;
    anim->fadeColourIndex = animEntry->soundEffect;
    anim->fadeOptions = animEntry->fadeOptions;
    anim->fadeLength = animEntry->fadeAlpha;
    anim->opacity = 0xFF;
    if (anim->soundHandle != NULL) {
        sndp_stop(anim->soundHandle);
    }
    anim->soundHandle = NULL;
    anim->musicFadeVolume = animEntry->musicFadeVolume;
    anim->animator = animationObj;
    anim->started = 0;
}

/** Spawns the object an animation entry animates and binds it. */
void anim_object_spawn_target(Object *obj, LevelObjectEntry_Animation *animEntry) {
    s32 i;
    LevelObjectEntryCommon newObjEntry;
    Object *newObj;
    Object_AnimatedObject *camera;
    s32 viewportCount;

    NEW_OBJECT_ENTRY(newObjEntry, animEntry->objectIdToSpawn, 8, animEntry->common.x, animEntry->common.y,
                     animEntry->common.z);

    obj->animTarget = spawn_object(&newObjEntry, OBJECT_SPAWN_UNK01);
    newObj = obj->animTarget;
    // (newObj->behaviorId == BHV_DINO_WHALE) is Dinosaur1, Dinosaur2, Dinosaur3, Whale, and Dinoisle
    if (obj->animTarget != NULL && newObj->behaviorId == BHV_DINO_WHALE && gTimeTrialEnabled) {
        free_object(newObj);
        obj->animTarget = NULL;
        newObj = NULL;
    }
    if (newObj != NULL) {
        newObj->level_entry = NULL;
        obj_init_animobject(obj, newObj);
        if (newObj->header->behaviorId == BHV_CAMERA_ANIMATION) {
            camera = newObj->animatedObject;
            camera->cameraStage = gAnimCameraCount;
            viewportCount = cam_get_viewport_layout();
            if (race_is_adventure_2P()) {
                viewportCount = VIEWPORT_LAYOUT_2_PLAYERS;
            }
            for (i = 0; i < viewportCount;) {
                newObj = spawn_object(&newObjEntry, OBJECT_SPAWN_UNK01);
                if (newObj != NULL) {
                    newObj->level_entry = NULL;
                    obj_init_animobject(obj, newObj);
                    camera = newObj->animatedObject;
                    i++;
                    camera->cameraID = i;
                    camera->cameraStage = gAnimCameraCount;
                }
            }
            gAnimCameraCount++;
        }
    }
}

s8 anim_group_get(void) {
    return gAnimGroup;
}

void anim_group_set(s32 arg0) {
    if (arg0 != gAnimGroup) {
        gAnimObjectCount = 0;
    }
    gAnimGroup = arg0;
}

s32 anim_count_actors(s32 arg0) {
    s32 i;
    s32 count;
    if (gAnimObjectCount == 0) {
        return -1;
    }

    count = 0;
    for (i = 0; i < gAnimObjectCount; i++) {
        if (gAnimObjects[i]->properties.animation.behaviourID == arg0) {
            count++;
        }
    }

    return count;
}

/** Asks the running cutscene to end (from the pause menu). */
void cutscene_skip_request(void) {
    gCutsceneSkipRequested = 1;
}

/**
 * Find the run of animation nodes that make up actor `actorIndex`'s path. Returns FALSE
 * when there is no path of at least two nodes.
 */
static s32 anim_path_find(s32 actorIndex, s32 *first, s32 *count) {
    s32 i;
    s32 n;

    for (i = 0; i < gAnimObjectCount && actorIndex != gAnimObjects[i]->properties.animation.behaviourID; i++) {}
    if (i >= gAnimObjectCount) {
        return FALSE;
    }
    for (n = 1; (i + n) < gAnimObjectCount && actorIndex == gAnimObjects[i + n]->properties.animation.behaviourID;
         n++) {}
    *first = i;
    *count = n;
    return n >= 2;
}

/**
 * The node a path loops back to after its last node, or -1 for a path that ends.
 */
static s32 anim_path_loop_node(s32 first, s32 count) {
    LevelObjectEntry_Animation *entry;

    entry = &gAnimObjects[first + count - 1]->level_entry->animation;
    if (count > 2 && entry->goToNode >= 0 && entry->goToNode < count - 1) {
        return entry->goToNode;
    }
    return -1;
}

/**
 * The five control points around the actor's current node: the node before, the node, and
 * the three after, extrapolated past either end of a path that does not loop. A node
 * marked with unk22 takes its position from the actor's camera instead.
 */
static void anim_path_control_points(s32 first, s32 count, s32 loopNode, s32 nodeIndex, s8 cameraID,
                                     AnimPathPoints *pts) {
    LevelObjectEntry_Animation *entry;
    ObjectTransform *trans;
    s32 node;
    s32 q;
    s32 j;

    node = nodeIndex - 1;
    for (j = 0; j < 5; j++, node++) {
        if (node == -1) {
            if (loopNode != 0) {
                q = first;
                pts->x[j] = gAnimObjects[q]->trans.x_position +
                            (gAnimObjects[q]->trans.x_position - gAnimObjects[q + 1]->trans.x_position);
                pts->y[j] = gAnimObjects[q]->trans.y_position +
                            (gAnimObjects[q]->trans.y_position - gAnimObjects[q + 1]->trans.y_position);
                pts->z[j] = gAnimObjects[q]->trans.z_position +
                            (gAnimObjects[q]->trans.z_position - gAnimObjects[q + 1]->trans.z_position);
            } else {
                q = first + count - 1;
                pts->x[j] = gAnimObjects[q]->trans.x_position;
                pts->y[j] = gAnimObjects[q]->trans.y_position;
                pts->z[j] = gAnimObjects[q]->trans.z_position;
            }
            pts->yaw[j] = gAnimObjects[q]->trans.rotation.y_rotation;
            pts->pitch[j] = gAnimObjects[q]->trans.rotation.x_rotation;
            pts->roll[j] = gAnimObjects[q]->trans.rotation.z_rotation;
            pts->scale[j] = gAnimObjects[q]->trans.scale;
        } else if (node >= count) {
            if (loopNode == -1) {
                node = count - 1;
                q = first + node;
                entry = &gAnimObjects[q]->level_entry->animation;
                if (entry->attachMode == 1) {
                    set_active_camera(cameraID);
                    trans = &cam_get_active_camera_no_cutscenes()->trans;
                } else {
                    trans = &gAnimObjects[q]->trans;
                }
                pts->x[j] = trans->x_position + (trans->x_position - gAnimObjects[q - 1]->trans.x_position);
                pts->y[j] = trans->y_position + (trans->y_position - gAnimObjects[q - 1]->trans.y_position);
                pts->z[j] = trans->z_position + (trans->z_position - gAnimObjects[q - 1]->trans.z_position);
                pts->pitch[j] = trans->rotation.x_rotation;
                pts->roll[j] = trans->rotation.z_rotation;
                pts->yaw[j] = gAnimObjects[q]->trans.rotation.y_rotation;
                pts->scale[j] = gAnimObjects[q]->trans.scale;
            } else {
                q = first + loopNode + node - count;
                pts->x[j] = gAnimObjects[q]->trans.x_position;
                pts->y[j] = gAnimObjects[q]->trans.y_position;
                pts->z[j] = gAnimObjects[q]->trans.z_position;
                pts->yaw[j] = gAnimObjects[q]->trans.rotation.y_rotation;
                pts->pitch[j] = gAnimObjects[q]->trans.rotation.x_rotation;
                pts->roll[j] = gAnimObjects[q]->trans.rotation.z_rotation;
                pts->scale[j] = gAnimObjects[q]->trans.scale;
            }
        } else {
            q = node + first;
            entry = &gAnimObjects[q]->level_entry->animation;
            if (entry->attachMode == 1) {
                set_active_camera(cameraID);
                trans = &cam_get_active_camera_no_cutscenes()->trans;
            } else {
                trans = &gAnimObjects[q]->trans;
            }
            pts->x[j] = trans->x_position;
            pts->y[j] = trans->y_position;
            pts->z[j] = trans->z_position;
            pts->pitch[j] = trans->rotation.x_rotation;
            pts->roll[j] = trans->rotation.z_rotation;
            pts->yaw[j] = gAnimObjects[q]->trans.rotation.y_rotation;
            if (entry->attachMode == 1) {
                pts->pitch[j] = -pts->pitch[j];
            }
            pts->scale[j] = gAnimObjects[q]->trans.scale;
        }
    }
}

/**
 * Make consecutive angles continuous so the spline does not sweep the long way round,
 * even though that pushes them past the s16 range.
 */
static void anim_path_unwrap_angles(f32 *angles) {
    f32 shift;
    f32 delta;
    s32 j;
    s32 k;

    for (j = 1; j < 5; j++) {
        shift = 0;
        delta = angles[j] - angles[j - 1];
        if (delta > 32768.0) {
            shift -= 65536.0;
        } else if (delta < -32768.0) {
            shift += 65536.0;
        }
        for (k = j; k < 5; k++) {
            angles[k] += shift;
        }
    }
}

/**
 * Rotation interpolated from the node rotations (rotateType other than 1, 2 or 3).
 * The unwrap above leaves angles outside the s16 range on purpose, so the store relies
 * on wrapping mod 65536: f32->s16 out of range is UB (x87 fistps would write 0x8000),
 * so go through s32, which wraps everywhere.
 */
static void anim_path_apply_node_rotation(Object *obj, Object_AnimatedObject *anim, AnimPathPoints *pts, s32 segment,
                                          f32 t) {
    anim_path_unwrap_angles(pts->yaw);
    anim_path_unwrap_angles(pts->pitch);
    anim_path_unwrap_angles(pts->roll);
    if (anim->linearInterp == 0) {
        obj->trans.rotation.y_rotation = (s16) (s32) catmull_rom_interpolation(pts->yaw, segment, t);
        obj->trans.rotation.x_rotation = (s16) (s32) catmull_rom_interpolation(pts->pitch, segment, t);
        obj->trans.rotation.z_rotation = (s16) (s32) catmull_rom_interpolation(pts->roll, segment, t);
    } else {
        obj->trans.rotation.y_rotation = (s16) (s32) lerp(pts->yaw, segment, t);
        obj->trans.rotation.x_rotation = (s16) (s32) lerp(pts->pitch, segment, t);
        obj->trans.rotation.z_rotation = (s16) (s32) lerp(pts->roll, segment, t);
    }
}

/**
 * Face along the path's tangent (rotateType 2).
 */
static void anim_path_face_along(Object *obj, Object_AnimatedObject *anim, AnimPathPoints *pts, s32 segment, f32 t,
                                 f32 *tangentZ) {
    f32 dx;
    f32 dy;
    f32 dz;
    f32 length;

    if (anim->linearInterp == 0) {
        cubic_spline_interpolation(pts->x, segment, t, &dx);
        cubic_spline_interpolation(pts->y, segment, t, &dy);
        cubic_spline_interpolation(pts->z, segment, t, &dz);
    } else {
        lerp_and_get_derivative(pts->x, segment, t, &dx);
        lerp_and_get_derivative(pts->y, segment, t, &dy);
        lerp_and_get_derivative(pts->z, segment, t, &dz);
    }
    length = sqrtf((dx * dx) + (dy * dy) + (dz * dz));
    if (length != 0) {
        length = 100.0 / length;
        dx *= length;
        dy *= length;
        dz *= length;
    }
    obj->trans.rotation.y_rotation = arctan2_f(dx, dz) - 0x8000;
    obj->trans.rotation.x_rotation = arctan2_f(dy, 100.0f);
    *tangentZ = dz;
}

/**
 * Face the actor that another animation path is driving (rotateType 3).
 */
static void anim_path_face_actor(Object *obj, s8 actorIndex) {
    ObjectTransform *trans;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 length;
    s32 j;

    for (j = 0; j < gAnimObjectCount && actorIndex != gAnimObjects[j]->properties.animation.behaviourID; j++) {}
    if (j == gAnimObjectCount) {
        return;
    }
    trans = &gAnimObjects[j]->animTarget->trans;
    if (trans == NULL) {
        return;
    }
    dx = trans->x_position - obj->trans.x_position;
    dy = trans->y_position - obj->trans.y_position;
    dz = trans->z_position - obj->trans.z_position;
    length = sqrtf((dx * dx) + (dy * dy) + (dz * dz));
    if (length > 0) {
        obj->trans.rotation.y_rotation = arctan2_f(dx, dz) - 0x8000;
        obj->trans.rotation.x_rotation = arctan2_f(dy, length);
    }
}

/**
 * An actor waiting with a negative start delay starts when one of its skip buttons is
 * pressed on any controller.
 */
static void anim_actor_check_skip(Object_AnimatedObject *anim) {
    s32 buttons;
    s32 pressed;
    s32 i;

    buttons = 0;
    if (anim->skipButtons & 1) {
        buttons = A_BUTTON;
    }
    if (anim->skipButtons & 2) {
        buttons |= B_BUTTON;
    }
    if (anim->skipButtons & 4) {
        buttons |= CONT_START;
    }
    pressed = 0;
    for (i = 0; i < MAXCONTROLLERS; i++) {
        pressed |= input_pressed(i);
    }
    if (pressed & buttons) {
        anim->startDelay = 1;
    }
}

/**
 * Music and sound cues attached to the current node: a sequence to start, a one-shot
 * sound (0xFF stops the current one) and a music fade.
 */
static void anim_actor_play_cues(Object_AnimatedObject *anim) {
    s32 soundID;

    if (anim->musicSequence > 0) {
        if (anim->musicSequence != music_current_sequence()) {
            music_play(anim->musicSequence);
            music_change_off();
        }
        anim->musicSequence = -2;
        music_volume_reset();
    } else if (anim->musicSequence == -2) {
        music_change_on();
        anim->musicSequence = -1;
    }
    if (anim->soundID != 0) {
        soundID = anim->soundID & 0xFF;
        if (anim->currentSound == 0) {
            if (soundID == 0xFF) {
                if (anim->soundHandle != NULL) {
                    sndp_stop(anim->soundHandle);
                }
            } else {
                if (anim->soundHandle != NULL) {
                    sndp_stop(anim->soundHandle);
                    soundID = anim->soundID & 0xFF;
                }
                sound_play(soundID, &anim->soundHandle);
            }
            anim->soundID = 0;
        }
    }
    if (anim->musicFadeVolume != 0) {
        music_fade(anim->musicFadeVolume << 8);
        anim->musicFadeVolume = 0;
    }
}

/**
 * Fade the actor in or out over time and hide it once fully out.
 */
static void anim_actor_update_opacity(Object *obj, Object_AnimatedObject *anim, s32 updateRate) {
    s32 opacity;

    opacity = (u8) anim->opacity;
    if (anim->fadeOptions & 1) {
        if (anim->fadeOptions & 2) {
            opacity = 0;
        }
        if ((updateRate * 8) < opacity) {
            opacity -= (updateRate * 8);
        } else {
            opacity = 0;
            obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
        }
    } else {
        if (anim->fadeOptions & 2) {
            opacity = 0xFF;
        }
        opacity += updateRate * 8;
        if (opacity > 0xFF) {
            opacity = 0xFF;
        }
        obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
    }
    anim->opacity = opacity;
}

/**
 * A screen transition cued by the current node: 8 and above change the fog, 6 and 7
 * flash white or blue, the rest are colour fades from the misc asset.
 */
static void anim_actor_update_fade(Object_AnimatedObject *anim) {
    FadeTransition fadeTransition;
    s8 *miscAsset;
    s32 fadeType;
    s32 fogNear;
    s32 fogFar;

    fadeType = anim->fadeType & 0x7F;
    if (fadeType == 0x7F) {
        return;
    }
    if (fadeType >= 8) {
        miscAsset = ((s8 *) get_misc_asset(ASSET_MISC_13) + (fadeType * 5));
        miscAsset -= 0x28;
        fogNear = (miscAsset[0] & 0xFF) + 900;
        fogFar = (miscAsset[1] & 0xFF) + 900;
        slowly_change_fog(0, miscAsset[2] & 0xFF, miscAsset[3] & 0xFF, miscAsset[4] & 0xFF, fogNear, fogFar,
                          normalise_time(6) * anim->fadeLength);
    } else if (fadeType >= 6) {
        fadeTransition.type = FADE_FLAG_INVERT;
        if (fadeType == 7) {
            fadeTransition.red = 200;
            fadeTransition.green = 200;
            fadeTransition.blue = 255;
        } else {
            fadeTransition.red = 255;
            fadeTransition.green = 255;
            fadeTransition.blue = 255;
        }
        fadeTransition.duration = 7;
        fadeTransition.endTimer = 3;
        transition_begin(&fadeTransition);
    } else {
        fadeTransition.type = anim->fadeType;
        miscAsset = (s8 *) get_misc_asset(ASSET_MISC_14) + (anim->fadeColourIndex * 3);
        fadeTransition.red = miscAsset[0];
        fadeTransition.green = miscAsset[1];
        fadeTransition.blue = miscAsset[2];
        if (anim->fadeType & 0x80) {
            fadeTransition.endTimer = 0;
        } else {
            fadeTransition.endTimer = 0xFFFF;
        }
        fadeTransition.duration = normalise_time(6) * anim->fadeLength;
        if (check_fadeout_transition() == 0 || (fadeTransition.type & 0x80)) {
            transition_begin(&fadeTransition);
        }
    }
    anim->fadeType = 0xFF;
}

/**
 * Advance the target model's animation frame (16ths) by the actor's rate, following its
 * loop type: 0 loop, 1 ping-pong, 2 play once, 3 play once then back.
 */
static void anim_actor_advance_model_frame(Object *obj, Object_AnimatedObject *anim, Object *target, f32 rate) {
    ObjectModel *model;
    s32 lastFrame;

    target->animationID = obj->animationID;
    if ((s16) anim->y != target->animFrame) {
        anim->y = target->animFrame;
    }
    if (target->modelInstances[target->modelIndex] != NULL) {
        model = target->modelInstances[target->modelIndex]->objModel;
        if (target->animationID >= 0 && target->animationID < model->numberOfAnimations) {
            lastFrame = (model->animations[target->animationID].animLength - 1) << 4;
            switch (anim->loopType) {
                case 0:
                    anim->y += anim->z * rate;
                    if (anim->y >= lastFrame) {
                        anim->y -= lastFrame;
                    }
                    break;
                case 2:
                    anim->y += anim->z * rate;
                    if (anim->y >= lastFrame) {
                        anim->y = lastFrame - 1.0f;
                    }
                    break;
                case 1:
                    if (anim->modelReversing == 0) {
                        anim->y += anim->z * rate;
                        if (anim->y >= lastFrame) {
                            anim->modelReversing = 1;
                            anim->y = lastFrame - 1.0f;
                        }
                    } else {
                        anim->y -= anim->z * rate;
                        if (anim->y <= 0) {
                            anim->y = 0;
                            anim->modelReversing = 0;
                        }
                    }
                    break;
                case 3:
                    if (anim->modelReversing == 0) {
                        anim->y += anim->z * rate;
                        if (anim->y >= lastFrame) {
                            anim->modelReversing = 1;
                            anim->y = lastFrame - 1.0f;
                        }
                    } else {
                        anim->y -= anim->z * rate;
                        if (anim->y <= 0) {
                            anim->y = 0;
                        }
                    }
                    break;
            }
        }
    }
    target->animFrame = anim->y;
}

/**
 * Drive a cutscene actor along its path of animation nodes for one frame: handle its
 * start delay and cues, step its model animation, then move it along the spline at the
 * node speed and rotate it by its rotate type. Returns 1 when the actor has nothing to do.
 */
s32 anim_object_follow_path(Object *obj, s32 updateRate, Object *target) {
    Object_AnimatedObject *anim;
    LevelObjectEntry_Animation *entry;
    AnimPathPoints pts;
    f32 rate;
    f32 t;
    f32 moveX;
    f32 moveY;
    f32 moveZ;
    f32 moved;
    f32 invBaseScale;
    f32 nodeSpeed;
    f32 nextNodeSpeed;
    s32 first;
    s32 count;
    s32 loopNode;
    s32 segment;
    s32 pass;
    s32 node;
    s32 nextNode;

    if (gCutsceneID < 0) {
        return 1;
    }
    if (updateRate >= 9) {
        updateRate = 8;
    }
    rate = updateRate;
    anim = obj->animatedObject;
    if (osTvType == 0) {
        rate *= 1.2;
    }
    if (anim->startDelay < 0) {
        anim_actor_check_skip(anim);
    }
    if ((anim->startDelay >= 0) && (anim->started == 0)) {
        anim->startDelay -= updateRate;
        if (anim->startDelay <= 0) {
            anim->started = 1;
            entry = &anim->animator->level_entry->animation;
            anim_actor_init(obj, anim, entry);
            anim->startDelay = 0;
            anim_actor_apply_entry(obj, entry, anim, -1);
        }
    }
    if (anim->startDelay != 0) {
        if (anim->specialHide != 0) {
            obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
            anim->opacity = 0;
            return 1;
        }
        return 0;
    }

    obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
    anim_actor_play_cues(anim);
    anim_actor_update_opacity(obj, anim, updateRate);
    anim_actor_update_fade(anim);

    if (anim->rotateType == 1) {
        obj->trans.rotation.y_rotation += (s16) (anim->yawSpinSpeed * (f32) (rate * 8.0));
        obj->trans.rotation.x_rotation += (s16) (anim->rollSpinSpeed * (f32) (rate * 8.0));
        obj->trans.rotation.z_rotation += (s16) (anim->pitchSpinSpeed * (f32) (rate * 8.0));
    }
    if (target != NULL && target->header->modelType == 0) {
        anim_actor_advance_model_frame(obj, anim, target, rate);
    }

    if (anim->speed <= 0.0) {
        return anim_actor_update(obj, updateRate);
    }
    if (!anim_path_find(anim->actorIndex, &first, &count)) {
        return anim_actor_update(obj, updateRate);
    }
    loopNode = anim_path_loop_node(first, count);
    invBaseScale = 1.0 / gAnimObjects[0]->header->scale;
    if (loopNode == -1 && anim->nodeIndex >= count - 1) {
        return anim_actor_update(obj, updateRate);
    }
    if (anim->pauseCounter >= 0) {
        if (gCutsceneSkipRequested == 0) {
            anim->pauseCounter -= updateRate;
        }
        return 0;
    }

    anim_path_control_points(first, count, loopNode, anim->nodeIndex, anim->cameraID, &pts);

    // Two passes: the first measures how far the current step would move the actor and
    // rescales the step to hold the node speed; the second takes the corrected step.
    if (anim->segmentStep == 0) {
        anim->segmentStep = 0.01f;
    }
    for (segment = 0, pass = 0; pass < 2; pass++) {
        t = anim->segmentProgress + (anim->segmentStep * rate);
        if (t >= 1.00) {
            segment = 1;
            t -= 1.0;
        }
        if (anim->linearInterp == 0) {
            moveX = catmull_rom_interpolation(pts.x, segment, t);
            moveY = catmull_rom_interpolation(pts.y, segment, t);
            moveZ = catmull_rom_interpolation(pts.z, segment, t);
        } else {
            moveX = lerp(pts.x, segment, t);
            moveY = lerp(pts.y, segment, t);
            moveZ = lerp(pts.z, segment, t);
        }
        moveX -= obj->trans.x_position;
        moveY -= obj->trans.y_position;
        moveZ -= obj->trans.z_position;
        if (pass != 1) {
            segment = 0;
            moved = sqrtf((moveX * moveX) + (moveY * moveY) + (moveZ * moveZ)) / rate;
            if (moved != 0) {
                anim->segmentStep *= anim->speed / moved;
            }
        }
    }

    obj->trans.scale = catmull_rom_interpolation(pts.scale, segment, t) * invBaseScale * obj->header->scale;
    // Reaching the last node of a path that ends: land exactly on it.
    if (segment != 0 && loopNode == -1 && count == anim->nodeIndex + 2) {
        moveX = catmull_rom_interpolation(pts.x, 0, 1.0f);
        moveY = catmull_rom_interpolation(pts.y, 0, 1.0f);
        moveZ = catmull_rom_interpolation(pts.z, 0, 1.0f);
        moveX -= obj->trans.x_position;
        moveY -= obj->trans.y_position;
        moveZ -= obj->trans.z_position;
    }
    obj->x_velocity = moveX / rate;
    obj->y_velocity = moveY / rate;
    obj->z_velocity = moveZ / rate;
    move_object(obj, moveX, moveY, moveZ);

    switch (anim->rotateType) {
        case 1:
            break;
        case 2:
            anim_path_face_along(obj, anim, &pts, segment, t, &moveZ);
            break;
        case 3:
            break;
        default:
            anim_path_apply_node_rotation(obj, anim, &pts, segment, t);
            break;
    }

    // Node speed: the current node's, blended towards the next node's across the segment.
    node = anim->nodeIndex;
    anim->segmentProgress = t;
    if (loopNode != -1 && node >= count) {
        node = (node - count) + loopNode;
    }
    entry = &gAnimObjects[first + node]->level_entry->animation;
    nodeSpeed = (f32) entry->nodeSpeed * 0.1;
    if (nodeSpeed < 0) {
        nodeSpeed = anim->x;
    } else {
        anim->x = nodeSpeed;
    }
    if (nodeSpeed >= 0) {
        if (segment == 0) {
            nextNode = node + 1;
            if (loopNode != -1 && nextNode >= count) {
                nextNode = nextNode - count + loopNode;
            }
            if (nextNode < count) {
                entry = &gAnimObjects[first + nextNode]->level_entry->animation;
                if (entry->nodeSpeed >= 0) {
                    nextNodeSpeed = (f32) entry->nodeSpeed * 0.1;
                } else {
                    nextNodeSpeed = nodeSpeed;
                }
            } else {
                nextNodeSpeed = moveZ; //!@bug uninitialised in the original: it reads the stale z move
            }
            anim->speed = ((nextNodeSpeed - nodeSpeed) * t) + nodeSpeed;
        }
    }

    if (segment != 0) {
        anim->nodeIndex += 1;
        if (loopNode == -1) {
            if (anim->nodeIndex >= count) {
                anim->nodeIndex = count - 1;
            } else {
                entry = &gAnimObjects[first + anim->nodeIndex]->level_entry->animation;
                anim_actor_apply_entry(obj, entry, anim, first);
            }
        } else {
            if (count < anim->nodeIndex) {
                anim->nodeIndex = loopNode + 1;
            }
            node = anim->nodeIndex;
            if (anim->nodeIndex >= count) {
                node = (anim->nodeIndex - count) + loopNode;
            }
            entry = &gAnimObjects[first + node]->level_entry->animation;
            anim_actor_apply_entry(obj, entry, anim, first);
        }
    }
    if (anim->rotateType == 3) {
        anim_path_face_actor(obj, anim->targetActorIndex);
    }
    obj->particleEmittersEnabled = anim->emitterMask;
    obj_spawn_particle(obj, updateRate);
    return 0;
}

s32 anim_stage_reached(s8 arg0) {
    if (arg0 >= gAnimStage) {
        gAnimStage = arg0;
        return TRUE;
    }
    return FALSE;
}

void anim_actor_init(Object *obj, Object_AnimatedObject *animObj, LevelObjectEntry_Animation *entry) {
    Camera *camera;
    ObjectTransform *animObjTrans;

    animObjTrans = &animObj->animator->trans;
    if (obj->behaviorId == BHV_CAMERA_ANIMATION) {
        animObj->cameraStage = gAnimCameraCount;
        gAnimCameraCount++;
    }
    if (entry->attachMode == 18) {
        set_active_camera(animObj->cameraID);
        camera = cam_get_active_camera_no_cutscenes();
        animObjTrans->x_position = camera->trans.x_position;
        animObjTrans->y_position = camera->trans.y_position;
        animObjTrans->z_position = camera->trans.z_position;
        animObjTrans->rotation.y_rotation = (0x8000 - camera->trans.rotation.y_rotation);
        animObjTrans->rotation.x_rotation = -camera->trans.rotation.x_rotation;
        animObjTrans->rotation.z_rotation = camera->trans.rotation.z_rotation;
    }
    if ((entry->attachMode >= 10) && (entry->attachMode < 18)) {
        Object *seg = (*gRacers)[entry->attachMode - 10];
        if (seg != NULL) {
            animObjTrans->x_position = seg->trans.x_position;
            animObjTrans->y_position = seg->trans.y_position;
            animObjTrans->z_position = seg->trans.z_position;
            animObjTrans->rotation.y_rotation = seg->trans.rotation.y_rotation;
            animObjTrans->rotation.x_rotation = seg->trans.rotation.x_rotation;
            animObjTrans->rotation.z_rotation = seg->trans.rotation.z_rotation;
        }
    }
}

void anim_actor_apply_entry(Object *obj, LevelObjectEntry_Animation *entry, Object_AnimatedObject *animObj,
                            UNUSED s32 index) {
    s32 initialAnimFrame;

    initialAnimFrame = entry->objAnimIndex;
    if (initialAnimFrame >= 0) {
        if (initialAnimFrame != obj->animationID) {
            obj->animFrame = entry->animStartFrame;
        }
        obj->animationID = entry->objAnimIndex;
        animObj->z = entry->objAnimSpeed;
        animObj->loopType = entry->objAnimLoopType;
    }
    if (entry->emitterMask >= 0) {
        animObj->emitterMask = entry->emitterMask;
    }
    animObj->pauseCounter = normalise_time(entry->pauseFrameCount);
    animObj->linearInterp = entry->linearInterp;
    animObj->specialHide = entry->specialHide;
    animObj->musicSequence = entry->musicSequence;
    animObj->musicFadeVolume = entry->musicFadeVolume;
    animObj->soundID = entry->soundID;
    animObj->fadeType = entry->fadeType;
    animObj->fadeColourIndex = entry->soundEffect;
    animObj->fadeOptions = entry->fadeOptions;
    animObj->fadeLength = entry->fadeAlpha;
    if (entry->messageId != 255) {
        set_current_text(entry->messageId);
    }
    if (entry->cutsceneID >= 0) {
        cutscene_set(entry->cutsceneID);
        return;
    }
    if (entry->triggerActorIndex >= 0) {
        anim_actor_trigger(entry->triggerActorIndex);
    }
    if (entry->signalCutsceneEnd >= 0) {
        gAnimFrameState[gAnimFrameParity]++;
    }
}

void anim_actor_trigger(s32 arg0) {
    s32 i;
    arg0 &= 0xFF; //?

    for (i = 0; i < gAnimObjectCount && (arg0 != (gAnimObjects[i]->properties.animation.behaviourID & 0xFF)); i++) {}

    if (i < gAnimObjectCount) {
        if (gAnimObjects[i]->animTarget != NULL) {
            if (gAnimObjects[i]->animTarget->animatedObject->startDelay < 0) {
                gAnimObjects[i]->animTarget->animatedObject->startDelay = 1;
            }
        }
    }
}

s8 anim_prev_frame_state(void) {
    return gAnimFrameState[1 - gAnimFrameParity];
}

s8 anim_actor_update(Object *obj, s32 updateRate) {
    s32 i;
    Object_AnimatedObject *animObj;

    animObj = obj->animatedObject;
    if (animObj->specialHide != 0) {
        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
    }
    if (animObj->pauseCounter == -1) {
        return animObj->specialHide;
    }
    if (animObj->pauseCounter >= 0) {
        animObj->pauseCounter -= updateRate;
    }
    if (animObj->pauseCounter == -1) {
        animObj->pauseCounter = -2;
    }
    if (animObj->pauseCounter <= 0) {
        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
        for (i = 0; (i < gAnimObjectCount && animObj->actorIndex != gAnimObjects[i]->properties.animation.behaviourID);
             i++) {
            if (FALSE) {} // FAKEMATCH
        }
        obj_init_animobject(gAnimObjects[i], obj);
        return 1;
    }
    return 0;
}

/**
 * Place the actor of animation path `actorIndex` at its current point on the path, for
 * the camera and other things that read actor positions. Returns TRUE when the actor is
 * not on a path.
 */
s32 anim_actor_is_active(s32 actorIndex) {
    Object_AnimatedObject *anim;
    Object *actor;
    AnimPathPoints pts;
    f32 t;
    f32 moveX;
    f32 moveY;
    f32 moveZ;
    f32 invBaseScale;
    s32 first;
    s32 count;
    s32 loopNode;

    if (gCutsceneID < 0) {
        return TRUE;
    }
    if (!anim_path_find(actorIndex, &first, &count)) {
        return TRUE;
    }
    actor = gAnimObjects[first]->animTarget;
    if (actor == NULL) {
        return TRUE;
    }
    anim = actor->animatedObject;
    invBaseScale = 1.0 / gAnimObjects[first]->header->scale;
    loopNode = anim_path_loop_node(first, count);
    anim_path_control_points(first, count, loopNode, anim->nodeIndex, anim->cameraID, &pts);

    t = anim->segmentProgress;
    if (anim->linearInterp == 0) {
        moveX = catmull_rom_interpolation(pts.x, 0, t);
        moveY = catmull_rom_interpolation(pts.y, 0, t);
        moveZ = catmull_rom_interpolation(pts.z, 0, t);
    } else {
        moveX = lerp(pts.x, 0, t);
        moveY = lerp(pts.y, 0, t);
        moveZ = lerp(pts.z, 0, t);
    }
    moveX -= actor->trans.x_position;
    moveY -= actor->trans.y_position;
    moveZ -= actor->trans.z_position;
    move_object(actor, moveX, moveY, moveZ);
    actor->trans.scale = catmull_rom_interpolation(pts.scale, 0, t) * invBaseScale * actor->header->scale;

    switch (anim->rotateType) {
        case 1:
            break;
        case 2:
            anim_path_face_along(actor, anim, &pts, 0, t, &moveZ);
            break;
        case 3:
            anim_path_face_actor(actor, anim->targetActorIndex);
            break;
        default:
            anim_path_apply_node_rotation(actor, anim, &pts, 0, t);
            break;
    }
    return FALSE;
}

f32 catmull_rom_interpolation(f32 *data, s32 index, f32 x) {
    f32 ret;
    f32 c, b, a;

    a = (-0.5 * data[index]) + (1.5 * data[index + 1]) + (-1.5 * data[index + 2]) + (0.5 * data[index + 3]);
    b = (1.0 * data[index]) + (-2.5 * data[index + 1]) + (2.0 * data[index + 2]) + (-0.5 * data[index + 3]);
    c = (data[index + 2] * 0.5) + (0.0 * data[index + 1]) + (-0.5 * data[index]) + (0.0 * data[index + 3]);

    ret = (1.0 * data[index + 1]);
    ret = (((((a * x) + b) * x) + c) * x) + ret;

    return ret;
}

/**
 * Interpolates x along a spline and returns the resultant progress along the spline.
 */
f32 cubic_spline_interpolation(f32 *data, s32 index, f32 x, f32 *derivative) {
    f32 ret;
    f32 c, b, a;

    a = (-0.5 * data[index]) + (1.5 * data[index + 1]) + (-1.5 * data[index + 2]) + (0.5 * data[index + 3]);
    b = (1.0 * data[index]) + (-2.5 * data[index + 1]) + (2.0 * data[index + 2]) + (-0.5 * data[index + 3]);
    c = (data[index + 2] * 0.5) + (0.0 * data[index + 1]) + (-0.5 * data[index]) + (0.0 * data[index + 3]);

    ret = (1.0 * data[index + 1]);
    *derivative = (((a * 3 * x) + (2 * b)) * x) + c;
    ret = (((((a * x) + b) * x) + c) * x) + ret;

    return ret;
}

f32 catmull_rom_derivative(f32 *data, s32 index, f32 x) {
    f32 derivative;
    f32 c, b, a;

    a = (-0.5 * data[index]) + (1.5 * data[index + 1]) + (-1.5 * data[index + 2]) + (0.5 * data[index + 3]);
    b = (1.0 * data[index]) + (-2.5 * data[index + 1]) + (2.0 * data[index + 2]) + (-0.5 * data[index + 3]);
    c = (data[index + 2] * 0.5) + (0.0 * data[index + 1]) + (-0.5 * data[index]) + (0.0 * data[index + 3]);

    derivative = (((a * 3 * x) + (2 * b)) * x) + c;

    return derivative;
}

/**
 * Imprecise method, which does not guarantee v = v1 when t = 1. (From Wikipedia)
 */
f32 lerp(f32 *data, u32 index, f32 t) {
    f32 result = data[index + 1] + t * ((data[index + 2] - data[index + 1]));
    return result;
}

/**
 * Peforms the lerp, and also returns the distance between the two points.
 */
f32 lerp_and_get_derivative(f32 *data, u32 index, f32 t, f32 *derivative) {
    f32 lerp;
    f32 vector;
    vector = data[index + 2] - data[index + 1];
    lerp = data[index + 1] + (vector * t);
    *derivative = vector;
    return lerp;
}
