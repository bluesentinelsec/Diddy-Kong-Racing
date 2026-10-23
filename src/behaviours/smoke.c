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
 * Smoke init behaviour.
 * Does nothing.
 */
void obj_init_smoke(UNUSED Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
}

/**
 * Smoke loop behaviour.
 * Updates the position and animation frame of the smoke object.
 * Frees the smoke object when the animation is finished.
 */
void obj_loop_smoke(Object *obj, s32 updateRate) {
    f32 updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    obj->trans.x_position += obj->x_velocity * updateRateF;
    obj->animFrame += updateRate * 16;
    obj->trans.y_position += obj->y_velocity * updateRateF;
    obj->trans.z_position += obj->z_velocity * updateRateF;
    if (obj->animFrame > 255) {
        free_object(obj);
        obj->animFrame = 255;
    }
}

void obj_init_unknown25(UNUSED Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
}

void obj_loop_unknown25(Object *obj, s32 updateRate) {
    obj->animFrame += updateRate * 8;
    if (obj->animFrame > 255) {
        free_object(obj);
        obj->animFrame = 255;
    }
}

void obj_init_wardensmoke(UNUSED Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
}

void obj_loop_wardensmoke(Object *obj, s32 updateRate) {
    f32 updateRateF;

    updateRateF = (f32) updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    obj->animFrame += updateRate * 4;
    obj->trans.y_position += updateRateF * 0.25;
    if (obj->animFrame > 255) {
        free_object(obj);
        obj->animFrame = 255;
    }
}
