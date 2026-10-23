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
 * Trophy Race Sign init behaviour.
 * Sets size and rotation based off spawn info.
 */
void obj_init_lighthouse_rocketsignpost(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Lighthouse_RocketSignpost *entry = &levelEntry->lighthouse_signpost;
    f32 radius = entry->radius & 0xFF;
    if (radius < 10) {
        radius = 10;
    }
    radius /= 64;
    obj->trans.scale = obj->header->scale * radius;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    if (obj->modelIndex >= obj->header->numberOfModelIds) {
        obj->modelIndex = 0;
    }
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    obj->interactObj->hitboxKind = 2;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
}

/**
 * Trophy Race Sign loop behaviour.
 * Checks if the player is nearby and if they have tried to initiate dialogue.
 * It then calls the function to check if it should start the lighthouse cutscene.
 */
void obj_loop_rocketsignpost(Object *obj, UNUSED s32 updateRate) {
    Object *playerObj;
    ObjectInteraction *interactObj;

    playerObj = get_racer_object(PLAYER_ONE);
    if (playerObj != NULL) {
        interactObj = obj->interactObj;
        if (interactObj->distance < 200) {
            if (playerObj == interactObj->obj) {
                // Detect if the player honks or slams into the signpost.
                if ((input_pressed(PLAYER_ONE) & Z_TRIG) || playerObj == obj->collisionData->collidedObj) {
                    begin_lighthouse_rocket_cutscene();
                }
            }
        }
    }
    obj->interactObj->distance = 0xFF;
}
