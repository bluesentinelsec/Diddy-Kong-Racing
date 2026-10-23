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
 * Weather init function.
 * Sets the radius of the weather object.
 * Official name: weatherInit
 */
void obj_init_weather(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Weather *entry = &levelEntry->weather;
    f32 radius = entry->radius;
    radius *= radius;
    obj->properties.distance.radius = radius;
}

/**
 * Weather updater loop behaviour.
 * When passed through by the player, it will update the current weather settings.
 * Can be used to stop, start or change the intensity of the current weather.
 */
void obj_loop_weather(Object *obj, UNUSED s32 updateRate) {
    s32 currViewport;
    s32 numberOfObjects;
    Object_Racer *racer;
    Object **objects;
    Object *curObj;
    LevelObjectEntry_Weather *entry;
    f32 diffX;
    f32 diffZ;
    s32 cur;
    s32 last;
    f32 dist;

    currViewport = get_current_viewport();
    objects = get_racer_objects(&numberOfObjects);
    cur = -1;
    if (numberOfObjects != 0) {
        last = numberOfObjects - 1;
        do {
            curObj = objects[cur + 1];
            racer = curObj->racer;
        } while (++cur < last && currViewport != racer->playerIndex);

        diffX = obj->trans.x_position - curObj->trans.x_position;
        diffZ = obj->trans.z_position - curObj->trans.z_position;
        dist = obj->properties.distance.radius;
        entry = (LevelObjectEntry_Weather *) obj->level_entry;
        if ((diffX * diffX) + (diffZ * diffZ) <= dist) {
            if (((!obj->level_entry) && (!obj->level_entry)) && (!obj->level_entry)) {} // Fakematch
            weather_set(entry->velocityX * 256, entry->velocityY * 256, entry->velocityZ * 256, entry->intensity * 257, entry->opacity * 257,
                        entry->transitionTime);
        }
    }
}
