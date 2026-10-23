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
 * Bomb Explosion init behaviour.
 * The properties set up here have mostly to do with the explosion animation, like the scale and the timer.
 */
void obj_init_bombexplosion(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_BombExplosion *entry = &levelEntry->bombExplosion;
    LevelObjectEntry_BombExplosion *entry2;
    obj->animFrame = 0;
    obj->trans.scale = 0.5f;
    obj->modelIndex = rand_range(0, obj->header->numberOfModelIds - 1);
    entry2 = entry; // Needed for a match.
    obj->properties.bombExplosion.timer = 0;
    obj->properties.bombExplosion.opacity = 0xFF;
    if (entry->opacity_hi) {
        obj->properties.bombExplosion.opacity |= (entry2->opacity_hi << 8) & 0xFF00;
    }
    obj->particleEmittersEnabled = OBJ_EMIT_1;
}

/**
 * Bomb Explosion loop behaviour.
 * Handles the explosion animation, spawning initially and then scaling down and fading out
 * near the end, and finally killing the explosion object.
 * Also spawns particle effects, but only if there are two or fewer human players.
 */
void obj_loop_bombexplosion(Object *obj, s32 updateRate) {
    s32 opacity_hi;

    obj->properties.bombExplosion.timer += updateRate;
    opacity_hi = (obj->properties.bombExplosion.opacity >> 8) & 0xFF;
    if (obj->properties.bombExplosion.timer > 10 && opacity_hi != 0) {
        obj->properties.bombExplosion.opacity ^= (opacity_hi << 8);
        obj_spawn_effect(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                         ASSET_OBJECT_ID_BOMBEXPLOSION, SOUND_NONE, 1.0f, opacity_hi - 1);
    }
    if (obj->properties.bombExplosion.timer < 20) {
        obj->trans.scale = ((obj->properties.bombExplosion.timer / 20.0f) * 10.0f) + 0.5f;
        obj->properties.bombExplosion.opacity |= 0xFF;
    } else if (obj->properties.bombExplosion.timer < 0x28) {
        obj->trans.scale = (((obj->properties.bombExplosion.timer - 20) / 20.0f) * 5.0f) + 10.5f;
        obj->properties.bombExplosion.opacity = 0x1EF - (obj->properties.bombExplosion.timer * 0xC);
    } else {
        free_object(obj);
    }

    if (obj->particleEmittersEnabled) {
        if (get_number_of_active_players() < THREE_PLAYERS) {
            obj_spawn_particle(obj, LOGIC_30FPS);
            obj->particleEmittersEnabled = OBJ_EMIT_NONE;
        }
    }
}
