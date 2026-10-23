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
 * Spectate Camera init func.
 * Sets the camera ID then signals to update the pathing systems which include spectate cameras.
 */
void obj_init_cameracontrol(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_CameraControl *entry = &levelEntry->cameraControl;
    obj->properties.camControl.cameraID = entry->cameraID;
    path_enable();
}

/**
 * Spectate Cameras loop func.
 * Does nothing since all the behaviour is handled globally.
 */
void obj_loop_cameracontrol(UNUSED Object *obj, UNUSED s32 updateRate) {
}
