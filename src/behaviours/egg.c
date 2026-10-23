#include "behaviours.h"

#include "audio.h"
#include "audio_spatial.h"
#include "audio_vehicle.h"
#include "audiosfx.h"
#include "camera.h"
#include "collision.h"
#include "common.h"
#include "fade_transition.h"
#include "font.h"
#include "game.h"
#include "game_text.h"
#include "game_ui.h"
#include "host.h"
#include "joypad.h"
#include "lights.h"
#include "macros.h"
#include "math_util.h"
#include "menu.h"
#include "object_models.h"
#include "objects.h"
#include "particles.h"
#include "PR/os_cont.h"
#include "PRinternal/viint.h"
#include "printf.h"
#include "racer.h"
#include "structs.h"
#include "textures_sprites.h"
#include "thread3_main.h"
#include "tracks.h"
#include "types.h"
#include "video.h"
#include "waves.h"
#include "weather.h"

/**
 * Fire Mountain egg init function.
 * Sets up hitbox data so it can be picked up.
 */
void obj_init_collectegg(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
}

/**
 * Fire Mountain egg loop function.
 * Spawn on SURFACE_EGG_SPAWN, and if dropped on surfaces SURFACE_EGG_01 to SURFACE_EGG_04, begin ticking down to hatch.
 */
void obj_loop_collectegg(Object *obj, s32 updateRate) {
    Object_CollectEgg *egg;
    Object *racerObj;
    Object_Racer *racer;
    Vec3f targetPos;
    f32 radius;
    f32 updateRateF;
    s32 hasCollision;
    s8 surface;

    egg = obj->egg;
    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    switch (egg->status) {
        case EGG_SPAWNED:
            try_to_collect_egg(obj, egg);
            break;
        case EGG_MOVING:
            obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
            targetPos.f[0] = obj->trans.x_position + (obj->x_velocity * updateRateF);
            targetPos.f[1] = obj->trans.y_position + (obj->y_velocity * updateRateF);
            targetPos.f[2] = obj->trans.z_position + (obj->z_velocity * updateRateF);
            radius = 9.0f;
            generate_collision_candidates(1, &obj->trans.position, &targetPos, VEHICLE_NO_OVERRIDE);
            hasCollision = FALSE;
            surface = SURFACE_DEFAULT;
            resolve_collisions(&obj->trans.position, &targetPos, &radius, &surface, 1, &hasCollision);
            obj->x_velocity = (targetPos.f[0] - obj->trans.x_position) / updateRateF;
            obj->y_velocity = (targetPos.f[1] - obj->trans.y_position) / updateRateF;
            obj->z_velocity = (targetPos.f[2] - obj->trans.z_position) / updateRateF;
            obj->trans.x_position = targetPos.f[0];
            obj->trans.y_position = targetPos.f[1];
            obj->trans.z_position = targetPos.f[2];
            obj->y_velocity -= 0.5;
            obj->x_velocity *= 0.98;
            obj->z_velocity *= 0.98;
            obj->y_velocity *= 0.95;
            // Destroy the egg if it lands on an invalid surface.
            if (obj->trans.y_position < -2000.0f ||
                (hasCollision && (surface < SURFACE_EGG_SPAWN || surface > SURFACE_EGG_04))) {
                if (egg->spawnerObj != NULL) {
                    egg->spawnerObj->properties.eggSpawner.egg = NULL;
                }
                free_object(obj);
            }
            if (hasCollision && surface == SURFACE_EGG_SPAWN) {
                egg->status = EGG_SPAWNED;
                obj->x_velocity = 0.0f;
                obj->z_velocity = 0.0f;
            }
            if (hasCollision && surface >= SURFACE_EGG_01 && surface <= SURFACE_EGG_04) {
                egg->racerID = surface - SURFACE_EGG_01;
                egg->status = EGG_IN_BASE;
                racerObj = get_racer_object(egg->racerID);
                if (racerObj != NULL) {
                    racer = racerObj->racer;
                    racer->eggHudCounter += 1;
                }
                egg->hatchTimer = 600;
                obj->x_velocity = 0.0f;
                obj->z_velocity = 0.0f;
            }
            break;
        case EGG_IN_BASE:
            egg->hatchTimer -= updateRate;
            racerObj = get_racer_object(egg->racerID);
            if (racerObj != NULL) {
                racer = racerObj->racer;
            }
            if (egg->hatchTimer <= 0) {
                if (racerObj != NULL) {
                    racer->lap += 1;
                }
                egg->status = EGG_HATCHED;
                egg->spawnerObj->properties.eggSpawner.egg = NULL;
            }
            if (egg->hatchTimer < 540) {
                try_to_collect_egg(obj, egg);
            }
            if (racerObj != NULL && egg->status != EGG_IN_BASE) {
                racer->eggHudCounter -= 1;
            }
            break;
        case EGG_HATCHED:
            racerObj = get_racer_object(egg->racerID);
            if (racerObj != NULL) {
                racer = racerObj->racer;
                if (racer->lap >= 3) {
                    racer->raceFinished = TRUE;
                }
            }
            obj->animFrame = 128;
            break;
    }
}

/**
 * Egg Spawner init behaviour.
 * Does nothing.
 */
void obj_init_eggcreator(UNUSED Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
}

/**
 * Egg Spawner Sign init behaviour.
 * Action is set to 0 if it has no egg child object.
 * Spawns an egg and tracks its existence.
 */
void obj_loop_eggcreator(Object *obj, UNUSED s32 updateRate) {
    LevelObjectEntryCommon spawnObj;
    Object *eggObj;

    if (obj->properties.eggSpawner.egg == NULL) {
        spawnObj.x = obj->trans.x_position;
        spawnObj.y = obj->trans.y_position;
        spawnObj.z = obj->trans.z_position;
        spawnObj.size = sizeof(LevelObjectEntryCommon);
        spawnObj.objectID = ASSET_OBJECT_ID_COLLECTEGG;
        eggObj = spawn_object(&spawnObj, OBJECT_SPAWN_UNK01);
        if (eggObj != NULL) {
            Object_CollectEgg *egg = eggObj->egg;
            egg->spawnerObj = obj;
            obj->properties.eggSpawner.egg = eggObj;
            eggObj->level_entry = NULL;
        }
    }
}

/**
 * If interacted by a racer, attach itself to the player and mark as invisible
 * if the player is not already carrying an egg.
 */
void try_to_collect_egg(Object *obj, Object_CollectEgg *egg) {
    Object_Racer *racer;
    Object *interactedObj;
    MtxF mat;
    ObjectTransform transF;

    if (obj->interactObj->distance < 40) {
        interactedObj = obj->interactObj->obj;
        if (interactedObj->header->behaviorId == BHV_RACER) {
            racer = interactedObj->racer;
            if (racer->held_obj == NULL) {
                egg->status = EGG_COLLECTED;
                obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
                racer->held_obj = obj;
                transF.rotation.y_rotation = -interactedObj->trans.rotation.y_rotation;
                transF.rotation.x_rotation = -interactedObj->trans.rotation.x_rotation;
                transF.rotation.z_rotation = -interactedObj->trans.rotation.z_rotation;
                transF.scale = 1.0f;
                transF.x_position = -interactedObj->trans.x_position;
                transF.y_position = -interactedObj->trans.y_position;
                transF.z_position = -interactedObj->trans.z_position;
                mtxf_from_inverse_transform(&mat, &transF);
                mtxf_transform_point(mat, obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                                     &obj->trans.x_position, &obj->trans.y_position, &obj->trans.z_position);
                obj->trans.x_position /= interactedObj->trans.scale;
                obj->trans.y_position /= interactedObj->trans.scale;
                obj->trans.z_position /= interactedObj->trans.scale;
            }
        }
    }
}
