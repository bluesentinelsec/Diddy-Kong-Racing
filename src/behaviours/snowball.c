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

void obj_init_unknown96(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_SOLID | INTERACT_FLAGS_UNK_0080;
    obj->interactObj->hitboxKind = 2;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
}

/**
 * Rolling snowball init behaviour.
 * Sets hitbox to add solidity.
 */
void obj_init_snowball(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    obj->interactObj->hitboxKind = 2;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
}

/**
 * Rolling snowball loop behaviour.
 * Plays sound at the snowball's position, presumably the sound of it rolling.
 */
void obj_loop_snowball(Object *obj, s32 updateRate) {
    Object_AnimatedObject *animated = obj->animatedObject;
    if (animated->currentSound == SOUND_NONE) {
        if (animated->soundID != SOUND_NONE) {
            animated->currentSound = animated->soundID & 0xFF;
        }
    }
    if (animated->currentSound != SOUND_NONE) {
        if (animated->soundMask == NULL) {
            audspat_play_sound_at_position(animated->currentSound, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position, AUDIO_POINT_FLAG_1,
                                           (AudioPoint **) &animated->soundMask);
        } else {
            audspat_point_set_position((AudioPoint *) animated->soundMask, obj->trans.x_position, obj->trans.y_position,
                                       obj->trans.z_position);
        }
    }
    anim_object_follow_path(obj, updateRate, obj);
}

void obj_destroy_snowball(Object *obj, s32 keepAnimTarget) {
    Object_AnimatedObject *snowball = obj->animatedObject;
    if (snowball->soundMask != NULL) {
        audspat_point_stop((AudioPoint *) snowball->soundMask);
    }
    obj_destroy_animated_object(obj, keepAnimTarget);
}
