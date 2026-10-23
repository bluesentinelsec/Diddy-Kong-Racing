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
 * Ramp switch init function.
 * Sets interaction attributes.
 */
void obj_init_rampswitch(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_RampSwitch *entry = &levelEntry->rampSwitch;
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
    obj->properties.rampSwitch.unk0 = entry->unk8;
}

/**
 * Ramp switch loop function.
 * When a player goes through this object, the switch will raise the drawbridge.
 * The distance is set to 255 so that it doesn't interact with the player again.
 */
void obj_loop_rampswitch(Object *obj, UNUSED s32 updateRate) {
    if (obj->interactObj->distance < 45) {
        start_bridge_timer(obj->properties.rampSwitch.unk0);
    }
    obj->interactObj->distance = 255;
}
