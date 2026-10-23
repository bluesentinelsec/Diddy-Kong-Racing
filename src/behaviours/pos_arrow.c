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

void obj_init_posarrow(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
}

void obj_loop_posarrow(Object *obj, UNUSED s32 updateRate) {
    Object_Racer *racer;
    Object **racerObjects;
    Object *racerObj;
    s32 numRacers;

    obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
    racerObjects = get_racer_objects_by_position(&numRacers);
    if (obj->properties.posArrow.playerID < numRacers) {
        racerObj = racerObjects[obj->properties.posArrow.playerID];
        racer = racerObj->racer;
        if (racer->playerIndex == PLAYER_COMPUTER) {
            obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
            racer->posArrowObj = obj;
        }
        obj->animFrame = obj->properties.posArrow.playerID * 127;
    }
}
