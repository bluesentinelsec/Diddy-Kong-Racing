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
 * Wizpig rocket loop function.
 * Updates the boost flames coming out of the rocket.
 */
void obj_loop_pigrocketeer(Object *obj, s32 updateRate) {
    Object *boostObj;
    Object_Boost *boost;

    anim_object_follow_path(obj, updateRate, obj);
    boostObj = racerfx_get_boost(BOOST_DEFAULT);

    if (boostObj != NULL) {
        boost = boostObj->boost;
        boostObj->properties.common.unk0 = 0;
        if (boost != 0) {
            boost->pulseTimer += updateRate;
            boost->effectState = 2;
            boost->effectScale = 1.0f;
            racer_set_boost_effect(obj, -1, 2, 1, 1);
        }
    }
}
