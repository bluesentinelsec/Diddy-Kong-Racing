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
 * Moving Bridge init func.
 * Sets hitbox info and the model from spawn info.
 */
void obj_init_bridge_whaleramp(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Bridge_WhaleRamp *entry = &levelEntry->bridge_whaleRamp;
    Object_Bridge_WhaleRamp *bridge = obj->bridge_whale_ramp;
    obj->modelIndex = entry->modelIndex;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    bridge->homeY = obj->trans.y_position;
    obj->interactObj->flags = INTERACT_FLAGS_SOLID | INTERACT_FLAGS_UNK_0020;
    obj->interactObj->hitboxKind = 2;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
    bridge->soundMask = NULL;
    if (obj->modelIndex >= obj->header->numberOfModelIds) {
        obj->modelIndex = 0;
    }
}

/**
 * Moving bridge loop behaviour.
 * In Whale Bay and Boulder Canyon, there exist two bridge objects you can use.
 * This function makes the whale oscillate and move upwards when a player gets near.
 * For the boulder canyon bridge, it raises and plays a bell sound while it's set to be raised.
 */
void obj_loop_bridge_whaleramp(Object *obj, s32 updateRate) {
    Object *racerObj;
    UNUSED ObjectTransform *temp_v0_4;
    f32 bobAmount;
    Object_Bridge_WhaleRamp *whaleRamp;
    LevelObjectEntry_Bridge_WhaleRamp *entry;
    f32 updateRateF;
    s32 vehicleID;
    f32 bellX1;
    f32 bellY1;
    f32 bellZ1;
    f32 bellX2;
    f32 bellY2;
    f32 bellZ2;
    Object_Racer *racer;

    whaleRamp = obj->bridge_whale_ramp;
    entry = &obj->level_entry->bridge_whaleRamp;
    updateRateF = updateRate;

    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }

    if (entry->moveMode != 3) {
        if (obj->properties.bridgeWhaleRamp.unk0 != 0) {
            bobAmount = 2.0 * (f32) entry->bobAmount;
            if (bobAmount > 0.0f) {
                if (obj->trans.y_position < whaleRamp->homeY + bobAmount) {
                    obj->trans.y_position += updateRateF * 2;
                }
            } else {
                if (whaleRamp->homeY + bobAmount < obj->trans.y_position) {
                    obj->trans.y_position -= updateRateF * 2;
                }
            }
        } else if (entry->radius > 0) {
            if (whaleRamp->homeY < obj->trans.y_position) {
                obj->trans.y_position -= updateRateF * 2;
            }
        } else {
            if (obj->trans.y_position < whaleRamp->homeY) {
                obj->trans.y_position += updateRateF * 2;
            }
        }
    } else if (obj->properties.bridgeWhaleRamp.unk0 != 0) {
        if (obj->trans.rotation.x_rotation > -0x1300) {
            obj->trans.rotation.x_rotation -= updateRate * 45;
        }
        if (whaleRamp->soundMask == NULL) {
            obj_bridge_pos(entry->bridgeId, &bellX1, &bellY1, &bellZ1);
            audspat_play_sound_at_position(SOUND_DRAWBRIDGE_BELL, bellX1, bellY1, bellZ1, AUDIO_POINT_FLAG_1,
                                           &whaleRamp->soundMask);
        }
    } else {
        if (obj->trans.rotation.x_rotation < 0) {
            obj->trans.rotation.x_rotation += updateRate * 40;
            if (whaleRamp->soundMask == NULL) {
                obj_bridge_pos(entry->bridgeId, &bellX2, &bellY2, &bellZ2);
                audspat_play_sound_at_position(SOUND_DRAWBRIDGE_BELL, bellX2, bellY2, bellZ2, AUDIO_POINT_FLAG_1,
                                               &whaleRamp->soundMask);
            }
        } else {
            obj->trans.rotation.x_rotation = 0;
            if (whaleRamp->soundMask != NULL) {
                audspat_point_stop(whaleRamp->soundMask);
            }
        }
    }

    switch (entry->moveMode) {
        case 0:
            obj->properties.bridgeWhaleRamp.unk0 = 0;
            if (obj->interactObj->distance < entry->radius) {
                obj->properties.bridgeWhaleRamp.unk0 = 1;
            }
            break;
        case 2:
            obj->properties.bridgeWhaleRamp.unk0 = 1;
            racerObj = get_racer_object(PLAYER_ONE);
            if (racerObj != NULL) {
                racer = racerObj->racer;
                switch (racer->vehicleID) {
                    default:
                        vehicleID = VEHICLE_HOVERCRAFT;
                        break;
                    case 1:
                        vehicleID = VEHICLE_PLANE;
                        break;
                    case 2:
                        vehicleID = VEHICLE_LOOPDELOOP;
                        break;
                }
                if (entry->allowedVehicles & vehicleID) {
                    obj->properties.bridgeWhaleRamp.unk0 = NULL;
                }
            }
            break;
        default:
            if (is_bridge_raised(entry->bridgeId) != 0) {
                obj->properties.bridgeWhaleRamp.unk0 = (entry->raisedDuration * 2);
            }
            if (obj->properties.bridgeWhaleRamp.unk0 > 0) {
                obj->properties.bridgeWhaleRamp.unk0 -= updateRate;
            } else {
                obj->properties.bridgeWhaleRamp.unk0 = 0;
            }
            break;
    }

    obj->interactObj->distance = 255;
    obj->interactObj->obj = NULL;
    obj->interactObj->flags &= ~INTERACT_FLAGS_PUSHING;
}
