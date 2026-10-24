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

void obj_init_overridepos(UNUSED Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
}

UNUSED void obj_loop_overridepos(UNUSED Object *obj, UNUSED s32 updateRate) {
}

/**
 * Drop the object from the list of position overrides.
 */
void obj_destroy_overridepos(Object *obj, UNUSED s32 keepAnimTarget) {
    s32 j;

    for (j = 0; j < gOverridePosCount && obj != gOverridePosObjects[j]; j++) {}
    if (j < gOverridePosCount) {
        gOverridePosCount--;
        for (; j < gOverridePosCount; j++) {
            gOverridePosObjects[j] = gOverridePosObjects[j + 1];
        }
    }
}
