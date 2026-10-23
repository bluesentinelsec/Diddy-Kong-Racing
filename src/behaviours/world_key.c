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
 * Challenge Key init behaviour.
 * Sets hitbox info based off spawn info.
 * Deletes itself in tracks mode, or if it's already been collected.
 */
void obj_init_worldkey(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_WorldKey *entry = &levelEntry->worldKey;
    Settings *settings;
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 30;
    entry->keyID &= 0xF;
    obj->properties.worldKey.keyID = entry->keyID;
    settings = get_settings();
    if (!is_in_tracks_mode() && !(settings->keys & (1 << obj->properties.worldKey.keyID))) {
        return;
    }
    free_object(obj);
}

/**
 * Challenge Key loop behaviour.
 * Rotates the key and checks to see if the player grabbed it.
 */
void obj_loop_worldkey(Object *worldKeyObj, s32 updateRate) {
    Settings *settings;
    Object *playerObj;

    // Check if the player is near the key
    // "worldKeyObj->interactObj->distance" is the player's distance from the key (Up to 255).
    if (worldKeyObj->interactObj->distance < WORLD_KEY_GRAB_CHECK_RADIUS) {
        // "worldKeyObj->interactObj->obj" is only set when the player is within 255 units of
        // the key object, otherwise it is NULL.
        playerObj = worldKeyObj->interactObj->obj;
        if (playerObj != NULL) {
            if (playerObj->header->behaviorId == BHV_RACER) {
                Object_Racer *racer = playerObj->racer;
                if (racer->playerIndex != PLAYER_COMPUTER) {
                    // Player has grabbed the key!
                    music_jingle_play(SEQUENCE_KEY_COLLECT);
                    settings = get_settings();
                    settings->keys |= 1 << worldKeyObj->properties.worldKey.keyID; // Set key flag
                    free_object(worldKeyObj);                                      // Makes the key unload.
                }
            }
        }
    }

    // Rotate world key
    worldKeyObj->trans.rotation.y_rotation += updateRate * 0x100;
}
