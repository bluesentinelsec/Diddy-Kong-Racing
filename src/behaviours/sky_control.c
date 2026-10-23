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
 * Skydome control init behaviour.
 * Sets hitbox data and on or off setting based off spawn info.
 */
void obj_init_skycontrol(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_SkyControl *entry = &levelEntry->skyControl;
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = entry->radius;
    obj->properties.skyControl.setting = entry->setting;
    obj->properties.skyControl.radius = entry->radius;
}

/**
 * Skydome control loop behaviour.
 * When a player goes through this object, enable or disable rendering the skydome depending on setting.
 * Used for transitions between indoors and outdoors.
 */
void obj_loop_skycontrol(Object *obj, UNUSED s32 updateRate) {
    if (obj->interactObj->distance < obj->properties.skyControl.radius) {
        set_skydome_visbility(obj->properties.skyControl.setting);
    }
}
