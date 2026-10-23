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
 * Racer Spawnpoint init func.
 * Sets the spawn ID as well as racer index.
 */
void obj_init_setuppoint(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_SetupPoint *entry = &levelEntry->setupPoint;
    obj->properties.setupPoint.racerIndex = entry->racerIndex;
    obj->properties.setupPoint.entranceID = entry->entranceID;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
}

/**
 * Racer Spawnpoint loop func.
 * Does nothing by itself; the game will use these as an anchor for spawning racer objects.
 */
void obj_loop_setuppoint(UNUSED Object *obj, UNUSED s32 updateRate) {
}
