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
 * Floating buoy init behaviour.
 * Sets hitbox data from spawn info.
 */
void obj_init_buoy_pirateship(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 arg2) {
    obj->log = obj_wave_init(obj->segmentID, obj->trans.x_position, obj->trans.z_position);
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 30;
    obj->interactObj->pushForce = 0;
}

/**
 * Floating buoy loop behaviour.
 * Exists to serve as an outer bound for water courses.
 * All it does is stays afloat on water and scrolls through each texture.
 */
void obj_loop_buoy_pirateship(Object *obj, s32 updateRate) {
    if (obj->log != NULL) {
        obj->trans.y_position = obj_wave_height(obj->log, updateRate);
    }
    obj->animFrame += updateRate * 8;
}
