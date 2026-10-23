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
 * Vehicle mode changer initialisation function.
 * Sets direction and vehicleID based off spawn info.
 */
void obj_init_modechange(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_ModeChange *entry = &levelEntry->modeChange;
    f32 radius;
    Object_Trigger *obj64;
    radius = entry->radius & 0xFF;
    if (radius < 5) {
        radius = 5;
    }
    obj64 = obj->trigger;
    radius /= 128;
    obj->trans.scale = radius;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    obj64->directionX = sins_f(obj->trans.rotation.y_rotation);
    obj64->directionY = 0.0f;
    obj64->directionZ = coss_f(obj->trans.rotation.y_rotation);
    obj64->rotationDiff = -((obj64->directionX * obj->trans.x_position) + (obj64->directionZ * obj->trans.z_position));
    obj64->radius = entry->radius;
    obj64->vehicleID = entry->vehicleID;
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = entry->radius;
    obj->interactObj->pushForce = 0;
}

/**
 * Vehicle mode changer loop behaviour.
 * Racers that pass through will have their vehicle type changed. This is usually used for loop-de-loops.
 */
void obj_loop_modechange(Object *obj, UNUSED s32 updateRate) {
    Object *racerObj;
    Object **racerObjects;
    s32 numRacers;
    Object_Racer *racer;
    Object_Trigger *trigger;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    s32 node;
    s32 i;
    f32 radiusF;
    f32 dist;

    trigger = obj->trigger;
    if (obj->interactObj->distance < trigger->radius) {
        radiusF = trigger->radius;
        racerObjects = get_racer_objects(&numRacers);
        for (i = 0; i < numRacers; i++) {
            racerObj = racerObjects[i];
            racer = racerObj->racer;
            if (racer->vehicleID != trigger->vehicleID) {
                diffX = racerObj->trans.x_position - obj->trans.x_position;
                diffY = racerObj->trans.y_position - obj->trans.y_position;
                diffZ = racerObj->trans.z_position - obj->trans.z_position;
                dist = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
                if (dist < radiusF) {
                    dist = ((trigger->directionX * racerObj->trans.x_position) +
                            (trigger->directionZ * racerObj->trans.z_position) + trigger->rotationDiff);
                    if (dist < 0.0f) {
                        racer->trickType = 0;
                        if (trigger->vehicleID == VEHICLE_CAR) {
                            racer->vehicleID = racer->vehicleIDPrev;
                        } else {
                            racer->vehicleID = trigger->vehicleID;
                        }
                        if (trigger->vehicleID == VEHICLE_LOOPDELOOP) {
                            if (racer->raceFinished == FALSE) {
                                rumble_set(racer->playerIndex, RUMBLE_TYPE_8);
                            }
                            node = ainode_find_nearest(racerObj->trans.x_position, racerObj->trans.y_position,
                                                       racerObj->trans.z_position, FALSE);
                            if (node != NODE_NONE) {
                                racer->nodeCurrent = ainode_get(node);
                            } else {
                                racer->nodeCurrent = NULL;
                            }
                            racer->challengeMarker = NULL;
                            racer->unk19A = 0;
                        }
                        racer->unk198 = obj->trans.rotation.y_rotation;
                    }
                }
            }
        }
    }
}
