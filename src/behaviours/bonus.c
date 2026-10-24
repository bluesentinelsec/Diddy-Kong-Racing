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

void obj_init_bonus(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Bonus *entry = &levelEntry->bonus;
    f32 radius;
    Object_Trigger *bonus;
    radius = entry->radius & 0xFF;
    if (radius < 5) {
        radius = 5;
    }
    bonus = obj->trigger;
    radius /= 128;
    obj->trans.scale = radius;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    bonus->directionX = sins_f(obj->trans.rotation.y_rotation);
    bonus->directionY = 0.0f;
    bonus->directionZ = coss_f(obj->trans.rotation.y_rotation);
    bonus->rotationDiff = -((bonus->directionX * obj->trans.x_position) + (bonus->directionZ * obj->trans.z_position));
    bonus->radius = entry->radius;
    bonus->racerFilterFlags = entry->racerFilterFlags; // Unused?
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = entry->radius;
    obj->interactObj->pushForce = 0;
}

void obj_loop_bonus(Object *obj, UNUSED s32 updateRate) {
    Object *racerObj;
    Object_Racer *racer;
    s32 numberOfRacers;
    f32 diffX;
    f32 diffY;
    f32 halfDist;
    f32 dist;
    f32 diffZ;
    Object_Trigger *obj64;
    Object **racerObjects;
    s32 i;

    obj64 = obj->trigger;
    if (obj->interactObj->distance < obj64->radius) {
        dist = obj64->radius;
        halfDist = dist * 0.5;
        racerObjects = get_racer_objects(&numberOfRacers);
        for (i = 0; i < numberOfRacers; i++) {
            racerObj = racerObjects[i];
            racer = racerObj->racer;
            diffY = racerObj->trans.y_position - obj->trans.y_position;
            if (diffY < halfDist && -halfDist < diffY) {
                diffX = racerObj->trans.x_position - obj->trans.x_position;
                diffZ = racerObj->trans.z_position - obj->trans.z_position;
                if (sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ)) < dist) {
                    f32 sideOfLine = (obj64->directionX * racerObj->trans.x_position) +
                                     (obj64->directionZ * racerObj->trans.z_position) + obj64->rotationDiff;
                    if (sideOfLine < 0.0f) {
                        if (racer->bananas < 10) {
                            racer->bananas = 10;
                            audspat_play_sound_at_position(SOUND_SELECT, racerObj->trans.x_position,
                                                           racerObj->trans.y_position, racerObj->trans.z_position,
                                                           AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                            sound_play_spatial(racer->characterId + SOUND_UNK_7B, racerObj->trans.x_position,
                                               racerObj->trans.y_position, racerObj->trans.z_position, NULL);
                        }
                    }
                }
            }
        }
    }
}
