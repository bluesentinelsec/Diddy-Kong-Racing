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

void obj_loop_animobject(Object *obj, s32 updateRate) {
    anim_object_follow_path(obj, updateRate, obj);
}

/**
 * Door opener loop function.
 * Updates the global variable that indicates whether doors can be forced open.
 */
void obj_loop_dooropener(Object *obj, s32 updateRate) {
    s32 openDoors;
    Object_AnimatedObject *doorOpener;

    doorOpener = obj->animatedObject;
    openDoors = 1 - anim_object_follow_path(obj, updateRate, obj);
    if (doorOpener->startDelay > 0) {
        openDoors = FALSE;
    }
    obj_door_open(openDoors);
}

/**
 * Animated objects carry one sound handle that must be silenced with them.
 */
void obj_destroy_animated_object(Object *obj, UNUSED s32 keepAnimTarget) {
    if (obj->animatedObject->soundHandle != NULL) {
        sndp_stop(obj->animatedObject->soundHandle);
    }
}

/**
 * An animated object that butterflies may follow.
 */
void obj_destroy_tracked_animated_object(Object *obj, s32 keepAnimTarget) {
    butterfly_forget_target(obj);
    obj_destroy_animated_object(obj, keepAnimTarget);
}

/** The opacity comes from the animation, not the view distance. */
void obj_fade_animated_object(Object *obj, UNUSED s32 alpha) {
    Object_AnimatedObject *animatedObj = obj->animatedObject;
    obj->opacity = animatedObj->opacity;
}
