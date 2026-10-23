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
 * Cutscene Vehicle loop behaviour.
 * Performs its set behaviour while having its own handler for the attached vehicle parts.
 */
void obj_loop_vehicleanim(Object *obj, s32 updateRate) {
    AttachPoint *attachPoint;
    Object *attachObj;

    anim_object_follow_path(obj, updateRate, obj);
    attachPoint = obj->attachPoints;
    if (attachPoint == NULL || attachPoint->count <= 0) {
        return;
    }

    attachObj = attachPoint->count == 3 ? attachPoint->obj[2] : attachPoint->obj[0];
    attachObj->trans.rotation.y_rotation = 0x4000;
    attachObj->modelIndex++;
    if (attachObj->modelIndex == attachObj->header->numberOfModelIds) {
        attachObj->modelIndex = 0;
    }
}
