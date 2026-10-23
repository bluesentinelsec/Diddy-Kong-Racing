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
 * Future Fun Land Teleport init behaviour.
 * Sets hitbox data based off spawn info.
 * If the player has seen the lighthouse cutscene, enable the object.
 */
void obj_init_teleport(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 15;
    obj->interactObj->pushForce = 0;
    if (get_settings()->cutsceneFlags & CUTSCENE_LIGHTHOUSE_ROCKET) {
        obj->properties.lighthouse.active = TRUE;
    }
}

/**
 * Future Fun Land Teleport loop behaviour.
 * Triggers a warp to the space hub world when entered, if active.
 */
void obj_loop_teleport(Object *obj, UNUSED s32 updateRate) {
    if (obj->properties.lighthouse.active) {
        LevelObjectEntry_Teleport *level_entry = &obj->level_entry->teleport;
        if (obj->interactObj->distance < 120) {
            begin_level_teleport(level_entry->levelID);
            obj->properties.lighthouse.active = FALSE;
            sound_play(SOUND_WHOOSH2, NULL);
            sound_play_delayed(SOUND_VOICE_TT_FUTURE_FUN_LAND, NULL, 1.0f);
        }
    }
}
