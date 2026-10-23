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
 * Time Trial Ghost init func.
 * Sets the timestamp frame to 0
 */
void obj_init_timetrialghost(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->properties.timeTrial.timestamp = 0;
    obj->properties.timeTrial.header = obj->header;
}

/**
 * Time Trial Ghost loop func.
 * Reads the ghost data associated with the header,
 * then has the position and rotation manually set to simulate time trial playback.
 * Has particle data, but never used.
 */
void obj_loop_timetrialghost(Object *obj, s32 updateRate) {
    Object *someObj;
    Object *racerObj;
    Object_Racer *racer;
    AttachPoint *attachPoint;
    s8 vehicleID;

    obj->animationID = 0;
    obj->animFrame = 40;
    if (get_race_countdown() == 0) {
        obj->properties.timeTrial.timestamp += updateRate;
    }
    timetrial_ghost_read(obj);
    obj_spawn_particle(obj, updateRate);
    racerObj = get_racer_object(PLAYER_ONE);
    racer = racerObj->racer;
    attachPoint = obj->attachPoints;
    if (attachPoint->count == 1) {
        vehicleID = racer->vehicleID;
        if (vehicleID == VEHICLE_HOVERCRAFT || vehicleID == VEHICLE_PLANE) {
            someObj = attachPoint->obj[0];
            someObj->trans.rotation.y_rotation = 0x4000;
            someObj->modelIndex++;
            someObj->modelIndex &= 1;
        }
    }
}

void obj_fade_ghost(Object *obj, UNUSED s32 alpha) {
    Object_Racer *racer = obj->racer;
    obj->opacity = racer->transparency;
}
