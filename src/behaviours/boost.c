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

void obj_init_boost(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Boost2 *entry = &levelEntry->boost2;
    Object_Boost *asset20 = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    obj->boost = &asset20[entry->racerIndex];
    obj->level_entry = NULL;
}

void obj_render_boost(Object *obj) {
    if (obj->properties.common.unk0 && (obj->boost->effectState > 0 || obj->boost->effectScale > 0.0f)) {
        render_boost_effect(obj);
    }
}
