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
 * Lava Spurt init behaviour.
 * Set the timer between animations, and also set the timer to wait before playing the first animation.
 */
void obj_init_lavaspurt(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_LavaSpurt *entry = &levelEntry->lavaSpurt;
    obj->properties.lavaSpurt.actionTimer = entry->initialTimer * 2;
    obj->properties.lavaSpurt.delayTimer = entry->delayTimer;
}

/**
 * Lava Spurt loop behaviour.
 * Count down until zero, then play an animation, after which, set the down time to a random amount with an offset based
 * on spawn properties.
 */
void obj_loop_lavaspurt(Object *obj, s32 updateRate) {
    if (obj->properties.lavaSpurt.actionTimer > 0) {
        obj->properties.lavaSpurt.actionTimer -= updateRate;
        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
    } else {
        obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
        obj->animFrame += updateRate * 4;
        if (obj->animFrame > 255) {
            obj->animFrame = 0;
            obj->properties.lavaSpurt.actionTimer = rand_range(0, 30) + obj->properties.lavaSpurt.delayTimer;
        }
    }
}
