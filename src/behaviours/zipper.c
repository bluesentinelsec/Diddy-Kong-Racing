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
 * Air/Water Zipper init behaviour.
 * Sets scale and angle based on entry data.
 */
void obj_init_airzippers_waterzippers(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_AirZippers_WaterZippers *entry = &levelEntry->airzipper_waterzipper;
    ObjectHeader *objHeader;
    f32 radius;

    radius = entry->radius & 0xFF;
    if (radius < 10.0f) {
        radius = 10.0f;
    }
    objHeader = obj->header;
    radius /= 64;
    obj->trans.scale = objHeader->scale * radius;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    if (obj->modelIndex >= objHeader->numberOfModelIds) {
        obj->modelIndex = 0;
    }
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
    if (get_filtered_cheats() & CHEAT_TURN_OFF_ZIPPERS) {
        free_object(obj);
    }
}

/**
 * Air/Water Zipper loop behaviour.
 * Searches for a racer among its interactions.
 * If a racer passes over it, set the value to initiate a boost on their end to true.
 * Will turn invisible and intangible if there are no hovercraft or airplanes being used.
 */
void obj_loop_airzippers_waterzippers(Object *obj, UNUSED s32 updateRate) {
    Object *curRacerObj;
    Object_Racer *racer;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    s32 numObjects;
    Object **racerObjs;
    s32 i;

    if (find_non_car_racers() == FALSE) {
        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
    } else {
        obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
    }
    if (obj->interactObj->distance < 100 && !(obj->trans.flags & OBJ_FLAGS_INVISIBLE)) {
        racerObjs = get_racer_objects(&numObjects);
        for (i = 0; i < numObjects; i++) {
            curRacerObj = racerObjs[i];
            racer = curRacerObj->racer;
            if (racer->zipperDirCorrection == FALSE && racer->boostTimer < 15) {
                diffX = curRacerObj->trans.x_position - obj->trans.x_position;
                diffY = curRacerObj->trans.y_position - obj->trans.y_position;
                diffZ = curRacerObj->trans.z_position - obj->trans.z_position;
                if ((s32) sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ)) < 100) {
                    racer->zipperDirCorrection = TRUE;
                    racer->zipperObj = obj;
                }
            }
        }
    }
}

/**
 * Ground Zipper init behaviour.
 * Sets scale and angle based on entry data.
 */
void obj_init_groundzipper(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_GroundZipper *entry = &levelEntry->groundZipper;
    ObjectHeader *header;
    f32 radius;

    radius = entry->scale & 0xFF;
    if (radius < 10.0f) {
        radius = 10.0f;
    }
    radius /= 64;
    header = obj->header;
    obj->trans.scale = header->scale * radius;
    obj->shadow->scale = header->shadowScale * radius;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    if (obj->modelIndex >= obj->header->numberOfModelIds) {
        obj->modelIndex = 0;
    }
    obj->properties.zipper.radius = (s32) (28.0f * radius) + 15;
    if (obj->properties.zipper.radius < 0) {
        obj->properties.zipper.radius = 0;
    }
    if (obj->properties.zipper.radius > 255) {
        obj->properties.zipper.radius = 255;
    }
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
    obj->interactObj->yRangeMin = -0x64;
    obj->interactObj->yRangeMax = 0x64;
    if (get_filtered_cheats() & CHEAT_TURN_OFF_ZIPPERS) {
        free_object(obj);
    }
}

/**
 * Ground Zipper loop behaviour.
 * Searches for a racer among its interactions.
 * If a racer passes over it, initiate a boost, which can be empowered by releasing the A button.
 */
void obj_loop_groundzipper(Object *obj, UNUSED s32 updateRate) {
    Object *racerObj;
    Object_Racer *racer;
    f32 diffX;
    f32 diffY;
    s32 numObjects;
    f32 diffZ;
    Object **racerObjs;
    s32 i;

    obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
    obj->trans.flags |= OBJ_FLAGS_SHADOW_ONLY;
    get_racer_object(PLAYER_ONE); // Unused. I guess the developers forgot to remove this?
    if (obj->interactObj->distance < obj->properties.zipper.radius) {
        racerObjs = get_racer_objects(&numObjects);
        for (i = 0; i < numObjects; i++) {
            racerObj = racerObjs[i];
            racer = racerObj->racer;
            if (racer->boostTimer < 15 && racer->groundedWheels) {
                diffX = racerObj->trans.x_position - obj->trans.x_position;
                diffY = racerObj->trans.y_position - obj->trans.y_position;
                diffZ = racerObj->trans.z_position - obj->trans.z_position;
                if ((s32) sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ)) < obj->properties.zipper.radius) {
                    if (racer->playerIndex != PLAYER_COMPUTER) {
                        sound_play_spatial(SOUND_ZIP_PAD_BOOST, racerObj->trans.x_position, racerObj->trans.y_position,
                                           racerObj->trans.z_position, NULL);
                    }
                    racer->boostTimer = normalise_time(45);
                    racer->boostType = BOOST_LARGE;
                    if (racer->throttleReleased) {
                        racer->boostType |= BOOST_SMALL_FAST;
                    }
                    if (racer->raceFinished == FALSE) {
                        rumble_set(racer->playerIndex, RUMBLE_TYPE_8);
                    }
                }
            }
        }
    }
}
