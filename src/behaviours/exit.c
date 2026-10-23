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
 * Exit init behaviour.
 * Sets scale, rotation and hitbox from spawn info.
 * Also applies a face direction used to account for node rotation relative to the angle difference of the racer.
 */
void obj_init_exit(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Exit *entry = &levelEntry->exit;
    f32 radius;
    Object_Exit *exit;
    radius = entry->radius & 0xFF;
    if (radius < 5) {
        radius = 5;
    }
    exit = obj->exit;
    radius /= 128;
    obj->trans.scale = radius;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    exit->directionX = sins_f(obj->trans.rotation.y_rotation);
    exit->directionY = 0.0f;
    exit->directionZ = coss_f(obj->trans.rotation.y_rotation);
    exit->rotationDiff = -((exit->directionX * obj->trans.x_position) + (exit->directionZ * obj->trans.z_position));
    exit->radius = entry->radius;
    exit->bossFlag = entry->bossFlag;
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = entry->radius;
    obj->interactObj->pushForce = 0;
}

/**
 * Exit loop behaviour.
 * Waits for a racer to interact with it, then sets the exit ID for the racer, triggering a level transition on their
 * end.
 */
void obj_loop_exit(Object *obj, UNUSED s32 updateRate) {
    Object *racerObj;
    Object_Racer *racer;
    s32 numberOfRacers;
    Settings *settings;
    f32 diffX;
    f32 diffZ;
    f32 dist;
    f32 diffY;
    Object_Exit *exit;
    s32 enableWarp;
    Object **racerObjects;
    s32 i;
    f32 rotDiff;

    exit = obj->exit;
    enableWarp = TRUE;
    settings = get_settings();
    // Disable the warp if it's for the first boss encounter, having collected every balloon.
    if (exit->bossFlag == WARP_BOSS_FIRST && settings->balloonsPtr[settings->worldId] == 8) {
        enableWarp = FALSE;
    }
    // Disable the warp if it's for the second boss encounter, having not collected every balloon.
    if (exit->bossFlag == WARP_BOSS_REMATCH && settings->balloonsPtr[settings->worldId] < 8) {
        enableWarp = FALSE;
    }
    // The above ensures only one of the boss warps is active so they don't overlap. This could also probably have been
    // done in the initialiser.
    if (enableWarp && obj->interactObj->distance < exit->radius) {
        dist = exit->radius;
        racerObjects = get_racer_objects(&numberOfRacers);
        for (i = 0; i < numberOfRacers; i++) {
            racerObj = racerObjects[i];
            racer = racerObj->racer;
            if (racer->playerIndex != PLAYER_COMPUTER && racer->exitObj == NULL) {
                diffX = racerObj->trans.x_position - obj->trans.x_position;
                diffY = racerObj->trans.y_position - obj->trans.y_position;
                diffZ = racerObj->trans.z_position - obj->trans.z_position;
                if ((sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ)) < dist)) {
                    rotDiff = (exit->directionX * racerObj->trans.x_position) +
                              (exit->directionZ * racerObj->trans.z_position) + exit->rotationDiff;
                    if (rotDiff < 0.0f) {
                        racer->exitObj = obj;
                        racer->transitionTimer = -120;
                    }
                }
            }
        }
    }
}
