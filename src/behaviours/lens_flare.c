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
 * Lens Flare init func.
 * Calls the function to initialise the lens flare system, using presets.
 */
void obj_init_lensflare(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    lensflare_init(obj);
}

/**
 * Lens Flare Override init func.
 * Calls a function adding the object to the lens flare system,
 * so the rendering portion knows to disable lens flares when inside.
 */
void obj_init_lensflareswitch(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_LensFlareSwitch *entry = &levelEntry->lensFlareSwitch;
    lensflare_override_add(obj);
    obj->trans.scale = entry->radius;
    obj->trans.scale /= 40.0f;
}

void obj_destroy_lensflare(Object *obj, UNUSED s32 keepAnimTarget) {
    lensflare_remove(obj);
}

void obj_destroy_lensflareswitch(Object *obj, UNUSED s32 keepAnimTarget) {
    lensflare_override_remove(obj);
}
