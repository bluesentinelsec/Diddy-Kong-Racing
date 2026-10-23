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
 * Hit tester init function.
 * Sets up interaction properties such as flags and hitbox radius.
 * Curiously, the hit tester entry parameter is unused.
 */
void obj_init_hittester(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_SOLID | INTERACT_FLAGS_UNK_0080;
    obj->interactObj->hitboxKind = 2;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
}

void obj_loop_hittester(Object *obj, s32 updateRate) {
    anim_object_follow_path(obj, updateRate, obj);
}

/**
 * Dynamically Lit Prop init behaviour.
 * Sets hitbox to add solidity.
 */
void obj_init_dynamic_lighting_object(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    obj->interactObj->hitboxKind = 2;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
}
