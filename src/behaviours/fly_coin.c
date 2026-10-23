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
 * Smokey's Castle Treasure Visual init func.
 */
void obj_init_flycoin(UNUSED Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
}

/**
 * Smokey's Castle Treasure Visual loop func.
 * When spawned, moves with the velocity given, which makes it aim towards the treasure box.
 * Deletes itself once the timer hits zero. Will also trigger the win condition once necessary.
 */
void obj_loop_flycoin(Object *obj, s32 updateRate) {
    f32 updateRateF;
    Object_Racer *racerObj;

    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    obj->y_velocity -= 0.5 * updateRateF;
    move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF, obj->z_velocity * updateRateF);
    obj->properties.flyCoin.diff -= updateRate;
    if (obj->properties.flyCoin.diff <= 0) {
        racerObj = obj->properties.flyCoin.racer;
        racerObj->lap++;
        if (racerObj->lap >= 10) {
            racerObj->raceFinished = TRUE;
        }
        free_object(obj);
        if (racerObj->playerIndex != PLAYER_COMPUTER) {
            sound_play(SOUND_SELECT, NULL);
        }
    }
    obj->animFrame += updateRate * 8;
}
