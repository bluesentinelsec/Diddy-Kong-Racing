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
 * Smokey's castle treasure box init function.
 * Sets up player ID, making sure the value doesn't go over 4 players.
 */
void obj_init_treasuresucker(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_TreasureSucker *entry = &levelEntry->treasureSucker;
    obj->animFrame = 120;
    obj->properties.treasureSucker.playerID = (entry->playerID - 1) & 3;
}

/**
 * Smokey's castle treasure box loop function.
 * When the player approaches carrying bananas, it spawns visual objects to enter it, then takes them from the player.
 */
void obj_loop_treasuresucker(Object *obj, s32 updateRate) {
    Object *racerObj;
    f32 scale;
    Object_Racer *racer;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    Object *newObj;
    LevelObjectEntryCommon spawnObj;
    s32 doSpawnObj;

    racerObj = get_racer_object(obj->properties.treasureSucker.playerID);
    if (racerObj != NULL) {
        racer = racerObj->racer;
        doSpawnObj = FALSE;
        if (racer->bananas != 0 && obj->properties.treasureSucker.spawnTimer == 0) {
            diffX = obj->trans.x_position - racerObj->trans.x_position;
            diffY = obj->trans.y_position - racerObj->trans.y_position;
            diffZ = obj->trans.z_position - racerObj->trans.z_position;
            diffX = (diffX * diffX) + (diffY * diffY) + (diffZ * diffZ); // Required to match.
            if (diffX < 225 * 225) {
                obj->properties.treasureSucker.spawnTimer = 8;
                doSpawnObj = TRUE;
            }
        }
        if (obj->properties.treasureSucker.spawnTimer > 0) {
            obj->properties.treasureSucker.spawnTimer -= updateRate;
            if (obj->properties.treasureSucker.spawnTimer <= 0) {
                if (racer->bananas > 0) {
                    racer->bananas--;
                    if (racer->bananas != 0) {
                        doSpawnObj = TRUE;
                    }
                    obj->properties.treasureSucker.spawnTimer = 8;
                } else {
                    obj->properties.treasureSucker.spawnTimer = 0;
                }
            }
        }
        if (doSpawnObj) {
            spawnObj.x = racerObj->trans.x_position;
            spawnObj.y = (s16) racerObj->trans.y_position + 10;
            spawnObj.z = racerObj->trans.z_position;
            spawnObj.size = sizeof(LevelObjectEntryCommon);
            spawnObj.objectID = ASSET_OBJECT_ID_FLYCOIN;
            newObj = spawn_object(&spawnObj, OBJECT_SPAWN_UNK01);
            if (newObj != NULL) {
                newObj->level_entry = NULL;
                newObj->y_velocity = 10.0f;
                scale = (newObj->y_velocity * 2) / 0.5;
                newObj->x_velocity = (obj->trans.x_position - racerObj->trans.x_position) / scale;
                newObj->z_velocity = (obj->trans.z_position - racerObj->trans.z_position) / scale;
                newObj->properties.flyCoin.racer = racer;
                newObj->properties.flyCoin.diff = scale;
            }
        }
    }
}
