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
 * Level scenery init behaviour.
 * Set rotation and scale based off spawn info.
 * Also have the option based off spawn info to add solidity to the hitbox.
 */
void obj_init_scenery(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Scenery *entry = &levelEntry->scenery;
    f32 radius;
    obj->trans.flags |= OBJ_FLAGS_UNK_0002;
    radius = entry->radius & 0xFF;
    if (radius < 10) {
        radius = 10;
    }
    radius /= 64;
    obj->trans.scale = obj->header->scale * radius;
    obj->shadow->scale = obj->header->shadowScale * radius;
    obj->modelIndex = entry->modelIndex;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    if (entry->solid) {
        obj->interactObj->flags = INTERACT_FLAGS_SOLID;
        obj->interactObj->hitboxKind = 1;
        obj->interactObj->hitboxRadius = 20;
        obj->interactObj->pushForce = 0;
        obj->interactObj->yRangeMin = -5;
        obj->interactObj->yRangeMax = entry->solid;
    }
    if (obj->modelIndex >= obj->header->numberOfModelIds) {
        obj->modelIndex = 0;
    }
    obj->properties.scenery.interactObj = NULL;
    obj->properties.common.unk4 = 0;
}

/**
 * Level scenery loop behaviour.
 * When hit by a racer, give it an elastic behaviour as feedback.
 */
void obj_loop_scenery(Object *obj, s32 updateRate) {
    ObjPropertyScenery *properties;
    s32 particleFlagShift;

    if (obj->interactObj != NULL) {
        properties = &obj->properties.scenery;
        if (properties->hitTimer > 0) {
            properties->hitTimer -= updateRate;
        }
        if (obj->interactObj->flags & INTERACT_FLAGS_COLLIDED && properties->hitTimer <= 0) {
            audspat_play_sound_at_position(SOUND_TWANG, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
            properties->interactObj = obj->interactObj->obj;
            properties->angleVel = 0x71C;
            properties->hitTimer = 10;
            if (get_number_of_active_players() < 2) {
                if (obj->header->particleCount > 0) {
                    if (obj->header->particleCount == 1) {
                        particleFlagShift = 0;
                    } else {
                        particleFlagShift = rand_range(0, obj->header->particleCount - 1);
                    }
                    obj->particleEmittersEnabled = OBJ_EMIT_1 << particleFlagShift;
                    obj_spawn_particle(obj, LOGIC_30FPS);
                }
            }
        } else if (properties->hitTimer <= 0) {
            properties->interactObj = NULL;
        }
        obj->trans.rotation.z_rotation = properties->angleVel;
        properties->angleVel = (properties->angleVel * -200) >> 8;
    }
}
