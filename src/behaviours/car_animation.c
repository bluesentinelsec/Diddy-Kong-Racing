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
 * Racer cutscene target init behaviour.
 * Does nothing. Not even called.
 */
UNUSED void obj_init_animcar(UNUSED Object *obj, UNUSED s32 param) {
}

/**
 * Racer cutscene target loop behaviour.
 * Exists as a target point for racer objects to automatically move towards, for cutscenes.
 */
void obj_loop_animcar(Object *obj, s32 updateRate) {
    Object *racerObj;
    s32 racerID;
    racerID = obj->properties.animatedObj.attachRacerID;
    racerObj = NULL;
    if (racerID != 0) {
        racerObj = get_racer_object(racerID - 1);
    }
    obj->properties.animatedObj.pathIdle = anim_object_follow_path(obj, updateRate, obj);
    obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
    if (obj->properties.animatedObj.pathIdle == 0 && racerObj != NULL) {
        Object_Racer *racer = racerObj->racer;
        racer->approachTarget = obj;
    }
}
