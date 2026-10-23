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
 * Bubbler init function.
 * Sets properties of the particle emitter.
 * Official name: bubblerInit
 */
void obj_init_bubbler(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Bubbler *entry = &levelEntry->bubbler;
    emitter_change_settings(obj->particleEmitter, entry->particleBehaviourID, entry->particlePropertyID, 0, 0, 0);
    obj->properties.bubbler.emitChance = entry->particleDensity;
}

/**
 * Bubbler Boss loop func.
 * All the racer side is handled elsewhere.
 * This rolls a random number and starts emitting particles based on that for variable density.
 */
void obj_loop_bubbler(Object *obj, s32 updateRate) {
    if (obj->properties.bubbler.emitChance >= rand_range(0, 1024)) {
        obj->particleEmittersEnabled = OBJ_EMIT_1;
    } else {
        obj->particleEmittersEnabled = OBJ_EMIT_NONE;
    }
    if (get_number_of_active_players() < 2) {
        obj_spawn_particle(obj, updateRate);
    }
}
