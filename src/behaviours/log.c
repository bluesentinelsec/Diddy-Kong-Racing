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
 * Spinning log init behaviour.
 * Sets scale, angle and hitbox data based off the spawn info.
 */
void obj_init_log(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Log *entry = &levelEntry->log;
    f32 radius;
    obj->log = obj_wave_init(obj->segmentID, obj->trans.x_position, obj->trans.z_position);
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    obj->interactObj->hitboxKind = 2;
    obj->interactObj->hitboxRadius = 30;
    radius = entry->radius & 0xFF;
    if (radius < 10.0f) {
        radius = 10.0f;
    }
    radius /= 64;
    obj->trans.scale = obj->header->scale * radius;
    obj->modelIndex = entry->modelIndex;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
}

/**
 * Spinning log loop behaviour.
 * Exists as a physical obstruction, will start spinning around on the spot when hit.
 */
void obj_loop_log(Object *obj, s32 updateRate) {
    Object *racerObj;
    Object_Log *log;
    f32 cosine;
    f32 sine;
    Object_Racer *racer;
    f32 diffZ;
    f32 diffX;

    log = obj->log;
    if (log != NULL) {
        obj->trans.y_position = obj_wave_height(log, updateRate);
    } else {
        obj->trans.y_position = ((LevelObjectEntryCommon *) obj->level_entry)->y;
    }
    if (obj->collisionData->collidedObj != NULL) {
        obj->properties.log.velocityY++;
        racerObj = obj->collisionData->collidedObj;
        if (racerObj->behaviorId == BHV_RACER) {
            racer = racerObj->racer;
            if (racer->velocity < -4.0 && racer->raceFinished == FALSE) {
                rumble_set(racer->playerIndex, RUMBLE_TYPE_18);
            }
        }
        sine = sins_f(-obj->trans.rotation.y_rotation);
        cosine = coss_f(-obj->trans.rotation.y_rotation);

        // This looks a bit messy, but is required to match.
        diffX = obj->trans.x_position - racerObj->trans.x_position;
        diffZ = obj->trans.z_position - racerObj->trans.z_position;
        diffX = (diffX * cosine) + (diffZ * sine);
        diffZ = (racerObj->x_velocity * sine) - (racerObj->z_velocity * cosine);
        diffX *= diffZ;
        obj->properties.log.angleVel -= (s32) (diffX / 4);

        if (obj->properties.log.angleVel > 0x200) {
            obj->properties.log.angleVel = 0x200;
        }
        if (obj->properties.log.angleVel < -0x200) {
            obj->properties.log.angleVel = -0x200;
        }
    } else {
        if (obj->properties.log.velocityY > 0) {
            obj->properties.log.velocityY--;
        }
    }
    obj->trans.y_position -= (f32) obj->properties.log.velocityY * 1.0;
    if (obj->properties.log.angleVel > 0) {
        obj->properties.log.angleVel -= updateRate;
        if (obj->properties.log.angleVel < 0) {
            obj->properties.log.angleVel = 0;
        }
    }
    if (obj->properties.log.angleVel < 0) {
        obj->properties.log.angleVel += updateRate;
        if (obj->properties.log.angleVel > 0) {
            obj->properties.log.angleVel = 0;
        }
    }
    obj->trans.rotation.y_rotation += obj->properties.log.angleVel * updateRate;
    obj->collisionData->collidedObj = NULL;
}

/** Logs and buoys share one side allocation. */
void obj_destroy_log(Object *obj, UNUSED s32 keepAnimTarget) {
    if (obj->log != NULL) {
        mempool_free(obj->log);
    }
}
