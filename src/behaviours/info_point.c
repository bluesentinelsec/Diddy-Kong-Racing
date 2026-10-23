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

void obj_init_infopoint(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_InfoPoint *entry = &levelEntry->infoPoint;
    if (entry->hitbox[1] != 0) {
        obj->interactObj->flags = INTERACT_FLAGS_SOLID | INTERACT_FLAGS_UNK_0020;
    } else {
        obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE | INTERACT_FLAGS_UNK_0020;
    }
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 15;
    obj->interactObj->pushForce = 0;
    obj->properties.infoPoint.radius = (entry->hitbox[2] << 16) | entry->hitbox[0]; // Not sure about the values here.
    obj->properties.infoPoint.visible = entry->hitbox[1];
    obj->trans.rotation.y_rotation = entry->unkB << 10; // Not sure about the values here.
}

void obj_loop_infopoint(Object *obj, UNUSED s32 updateRate) {
    s16 player;
    ObjectInteraction *interactObj;
    Object *playerObj;

    if (obj->properties.infoPoint.visible == FALSE) {
        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
    } else {
        obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
    }

    interactObj = obj->interactObj;
    if (interactObj->distance < ((obj->properties.infoPoint.radius >> 16) & 0xFF)) {
        playerObj = interactObj->obj;
        if (playerObj->header->behaviorId == BHV_RACER) {
            Object_Racer *racer = playerObj->racer;
            player = racer->playerIndex;
            if ((player != PLAYER_COMPUTER) && (input_pressed(player) & Z_TRIG)) {
                set_current_text(obj->properties.infoPoint.radius & 0xFF);
            }
        }
    }
}
