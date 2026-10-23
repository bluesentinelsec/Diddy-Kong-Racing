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
 * Checkpoint initialisation function.
 * Sets direction and scale based off spawn info.
 * Uses bit shifting to convert a u8 angle to an s16 angle.
 */
void obj_init_checkpoint(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 arg2) {
    LevelObjectEntry_Checkpoint *entry = &levelEntry->checkpoint;
    f32 scale = (s32) (entry->scale & 0xFF);
    if (scale < 5.0f) {
        scale = 5.0f;
    }
    scale /= 64;
    obj->trans.scale = scale;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    path_enable();
}

/**
 * Checkpoint initialisation function.
 * Does nothing, since the racers themselves iterate checkpoints and use their data.
 */
void obj_loop_checkpoint(UNUSED Object *obj, UNUSED s32 updateRate) {
}
