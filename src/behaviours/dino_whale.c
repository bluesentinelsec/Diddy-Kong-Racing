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
 * Dinosaur init behaviour.
 * Walks around, playing footstep sounds and roaring when nearby.
 */
void obj_init_dino_whale(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    obj->interactObj->hitboxKind = 3;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
}

/**
 * Dinosaur loop behaviour.
 * Walks around, playing footstep sounds and roaring when nearby.
 */
void obj_loop_dino_whale(Object *obj, s32 updateRate) {
    s32 animFrame;

    if (obj->properties.dinoWhale.roarTimer > 0) {
        obj->properties.dinoWhale.roarTimer -= updateRate;
        updateRate *= 2;
    } else {
        obj->properties.dinoWhale.roarTimer = 0;
    }
    animFrame = obj->animFrame;
    anim_object_follow_path(obj, updateRate, obj);
    play_footstep_sounds(obj, 0, animFrame, SOUND_STOMP2, SOUND_STOMP3);
    if (obj->interactObj->distance < 255) {
        if (obj->properties.dinoWhale.roarTimer == 0) {
            obj->properties.dinoWhale.roarTimer = 60;
            audspat_play_sound_at_position(SOUND_VOICE_BRONTO_ROAR, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
        }
    }
}
