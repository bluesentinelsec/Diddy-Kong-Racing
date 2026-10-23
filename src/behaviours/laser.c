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
 * Laser gun init function.
 * Sets the face direction and active behaviour modes based on entry data.
 * This includes a targeting system and rate of fire.
 */
void obj_init_lasergun(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Lasergun *entry = &levelEntry->lasergun;
    Object_LaserGun *laserGun;
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE | INTERACT_FLAGS_UNK_0020;
    obj->interactObj->hitboxKind = 0;
    laserGun = obj->lasergun;
    laserGun->targeting = entry->targeting;
    laserGun->fireRate = entry->fireRate;
    laserGun->laserDuration = entry->laserDuration;
    laserGun->radius = entry->radius;
    laserGun->fireTimer = laserGun->fireRate;
    obj->trans.rotation.y_rotation = entry->angleY << 4 << 4; // Not sure about the shift amounts here, but it
    obj->trans.rotation.x_rotation = entry->angleX << 4 << 4; // just needs to be 2 left shifts to add up to 8.
}

/**
 * Laser gun loop behaviour.
 * A static mounted gun that shoots laserbeams at wherever it's facing.
 * The fire rate is adjustable per object and can be set to target racers.
 */
void obj_loop_lasergun(Object *obj, s32 updateRate) {
    Object_LaserGun *lasergun;
    Object *laserBoltObj;
    f32 distance;
    ObjectTransform trans;
    MtxF mtx;
    Object *racerObj;
    f32 diffX;
    f32 diffZ;
    f32 diffY;
    ObjectInteraction *interactObj;
    LevelObjectEntryCommon spawnObj;

    lasergun = obj->lasergun;
    if (lasergun->fireTimer > 0) {
        lasergun->fireTimer -= updateRate;
    } else {
        interactObj = obj->interactObj;
        if (lasergun->radius >= interactObj->distance) {
            if (lasergun->targeting) {
                racerObj = interactObj->obj;
                if (racerObj != NULL && racerObj->behaviorId == BHV_RACER) {
                    diffX = obj->trans.x_position - racerObj->trans.x_position;
                    diffY = obj->trans.y_position - racerObj->trans.y_position;
                    diffZ = obj->trans.z_position - racerObj->trans.z_position;
                    distance = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
                    if (distance > 10.0f) {
                        diffX /= distance;
                        diffY /= distance;
                        diffZ /= distance;
                        obj->trans.rotation.y_rotation = (arctan2_f(diffX, diffZ) - 0x8000) & 0xFFFF;
                        obj->trans.rotation.x_rotation = arctan2_f(diffY, 1.0f) & 0xFFFF;
                    }
                }
            }
            lasergun->fireTimer = lasergun->fireRate;
            spawnObj.x = obj->trans.x_position;
            spawnObj.y = obj->trans.y_position;
            spawnObj.z = obj->trans.z_position;
            spawnObj.size = sizeof(LevelObjectEntryCommon);
            spawnObj.objectID = ASSET_OBJECT_ID_LASERBOLT;
            audspat_play_sound_at_position(SOUND_LASER_GUN, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
            laserBoltObj = spawn_object(&spawnObj, OBJECT_SPAWN_UNK01);
            if (laserBoltObj != NULL) {
                laserBoltObj->level_entry = NULL;
                laserBoltObj->trans.rotation.y_rotation = obj->trans.rotation.y_rotation;
                laserBoltObj->trans.rotation.x_rotation = obj->trans.rotation.x_rotation;
                laserBoltObj->properties.laserbolt.timer = lasergun->laserDuration;
                laserBoltObj->properties.laserbolt.obj = lasergun;
                trans.x_position = 0.0f;
                trans.y_position = 0.0f;
                trans.z_position = 0.0f;
                trans.scale = 1.0f;
                trans.rotation.y_rotation = obj->trans.rotation.y_rotation;
                trans.rotation.x_rotation = obj->trans.rotation.x_rotation;
                trans.rotation.z_rotation = 0;
                mtxf_from_transform(&mtx, &trans);
                diffX = 30.0f; // Need to use diffX to match.
                if (lasergun->targeting == FALSE) {
                    diffX = 45.0f;
                }
                mtxf_transform_point(mtx, 0.0f, 0.0f, diffX, &laserBoltObj->x_velocity, &laserBoltObj->y_velocity,
                                     &laserBoltObj->z_velocity);
            }
        }
    }
}

/**
 * Laser Bolt init behaviour.
 * Sets hitbox data so it can be interacted with.
 */
void obj_init_laserbolt(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
}

/**
 * Laser Bolt init behaviour.
 * Move in a set direction, until it either times out, hits a racer, or hits a wall.
 */
void obj_loop_laserbolt(Object *obj, s32 updateRate) {
    f32 updateRateF;

    Object *racerObj;

    Object_LaserGun *laserGun;
    Object_Racer *racer;

    s8 delete; // Boolean
    s8 surface;
    Vec3f dir;
    f32 radius;
    s32 hasCollision; // Boolean

    delete = FALSE;
    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    dir.x = obj->trans.x_position + (obj->x_velocity * updateRateF);
    dir.y = obj->trans.y_position + (obj->y_velocity * updateRateF);
    dir.z = obj->trans.z_position + (obj->z_velocity * updateRateF);
    radius = 9.0f;

    generate_collision_candidates(1, &obj->trans.position, &dir, VEHICLE_NO_OVERRIDE);
    hasCollision = FALSE;
    resolve_collisions(&obj->trans.position, &dir, &radius, &surface, 1, &hasCollision);
    if (hasCollision) {
        obj->x_velocity = (dir.x - obj->trans.x_position) / updateRateF;
        obj->y_velocity = (dir.y - obj->trans.y_position) / updateRateF;
        obj->z_velocity = (dir.z - obj->trans.z_position) / updateRateF;
    }
    move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF, obj->z_velocity * updateRateF);
    if (hasCollision) {
        obj_spawn_effect(obj->trans.x_position, obj->trans.y_position - 36.0f, obj->trans.z_position,
                         ASSET_OBJECT_ID_BOMBEXPLOSION, SOUND_NONE, 0.2, 0);
        delete = TRUE;
    }

    if (obj->properties.laserbolt.timer > 0) {
        obj->properties.laserbolt.timer -= updateRate;
    } else {
        delete = TRUE;
    }

    if (obj->interactObj->distance < 80) {
        racerObj = obj->interactObj->obj;
        if (racerObj && racerObj->behaviorId == BHV_RACER) {
            racer = racerObj->racer;
            delete = TRUE;
            if (racer->playerIndex != PLAYER_COMPUTER) {
                racer->attackType = ATTACK_EXPLOSION;
            }
            laserGun = obj->properties.laserbolt.obj;
            if (laserGun != NULL) {
                laserGun->fireTimer = 180;
            }
            delete = TRUE;
            obj_spawn_effect(obj->trans.x_position, obj->trans.y_position - 36.0f, obj->trans.z_position,
                             ASSET_OBJECT_ID_BOMBEXPLOSION, SOUND_EXPLOSION, 0.5, 0);
        }
    }
    if (delete) {
        free_object(obj);
    }
}
