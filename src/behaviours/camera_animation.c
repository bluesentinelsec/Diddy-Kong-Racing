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
 * Camera Animation loop func.
 * Follows a path and then writes to the view perspective.
 */
void obj_loop_animcamera(Object *obj, s32 updateRate) {
    s32 temp_v0;
    s32 updateCam;
    Object_AnimatedObject *camera;

    temp_v0 = anim_object_follow_path(obj, updateRate, obj);
    obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
    camera = obj->animatedObject;
    if (temp_v0 == 0) {
        if (cam_get_viewport_layout() == VIEWPORT_LAYOUT_1_PLAYER) {
            updateCam = anim_stage_reached(camera->cameraStage);
        } else {
            updateCam = TRUE;
        }
        if (updateCam) {
            write_to_object_render_stack(camera->cameraID, obj->trans.x_position, obj->trans.y_position,
                                         obj->trans.z_position, 0x8000 - obj->trans.rotation.y_rotation,
                                         -obj->trans.rotation.x_rotation, obj->trans.rotation.z_rotation);
        }
    }
}
