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
 * Torch / Mist init behaviour.
 * Sets scale and animation speed based off spawn info.
 */
void obj_init_torch_mist(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Torch_Mist *entry = &levelEntry->torch_mist;
    f32 radius = entry->radius & 0xFF;
    if (radius < 10.0f) {
        radius = 10.0f;
    }
    radius /= 64;
    obj->trans.scale = obj->header->scale * radius;
    obj->properties.torchMist.speed = entry->animSpeed;
}

/**
 * Torch / Mist loop behaviour.
 * Updates the animation frame based on spawn info's animation speed.
 */
void obj_loop_torch_mist(Object *obj, s32 updateRate) {
    obj->animFrame += obj->properties.torchMist.speed * updateRate;
}
