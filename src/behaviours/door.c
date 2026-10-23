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

s8 sTTAmuletModelIndices[8] = { 0x00, 0x00, 0x01, 0x01, 0x02, 0x00, 0x00, 0x00 };

s8 sTTAmuletModelIndicesAlt[12] = { 0x00, 0x01, 0x01, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/**
 * Door init func.
 * Sets all door parameters, which include type, direction and number of balloons needed to open
 */
void obj_init_door(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Door *entry = &levelEntry->door;
    Object_Door *door;
    f32 radius;

    door = obj->door;
    if (entry->doorID == -1) {
        entry->doorID = object_id_alloc(obj);
    } else {
        object_id_assign(obj, entry->doorID);
    }
    door->doorID = entry->doorID;
    door->doorType = entry->doorType;
    door->balloonCountUnused = entry->balloonCount;
    door->balloonCount = entry->balloonCount;
    door->radius = entry->distanceToOpen;
    if (door->doorID == -1) {
        pc_log("Illegal door no!!!\n");
    }
    obj->modelIndex = entry->modelIndex;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->closedRotation);
    door->homeY = obj->trans.y_position;
    door->jingleTimer = 0;
    obj->properties.door.closeAngle = obj->trans.rotation.y_rotation;
    obj->properties.door.openAngle = (entry->openRotation & 0x3F) << 10;
    radius = entry->scale & 0xFF;
    if (radius < 10.0f) {
        radius = 10.0f;
    }
    radius /= 64;
    obj->trans.scale = obj->header->scale * radius;
    door->textID = entry->textID;
    door->keyID = entry->keyID;
    obj->interactObj->flags = INTERACT_FLAGS_SOLID | INTERACT_FLAGS_UNK_0020;
    obj->interactObj->hitboxKind = 2;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
    if (obj->modelIndex >= obj->header->numberOfModelIds) {
        obj->modelIndex = 0;
    }
}

/**
 * Door loop func.
 * If the requirement to open the door is not met, play a jingle and show a message.
 * Since there's a number of different kinds of doors, this code handles each type.
 */
void obj_loop_door(Object *doorObj, s32 updateRate) {
    s32 playSound;
    s16 angleVel;
    volatile s32 doorIDFlag; // ???
    s32 dist;
    s32 keyBits;
    Object_Racer *racer;
    Object_Door *door;
    Settings *settings;
    Object *racerObj;
    ObjectInteraction *racerObjInter;
    LevelObjectEntry_Door *doorEntry;
    f32 updateRateF;
    s32 model;
    s32 sp28;

    doorEntry = &doorObj->level_entry->door;
    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    settings = get_settings();
    playSound = settings->courseFlagsPtr[settings->courseId];
    door = doorObj->door;
    if (door->doorID >= 0) {
        doorIDFlag = 0x10000 << door->doorID;
        racerObjInter = doorObj->interactObj;
        dist = racerObjInter->distance;
        if (!(door->doorType & 1)) {
            dist = 0; // Door is forced open
        }
        sp28 = playSound & doorIDFlag;
        if (sp28 == 0 && racerObjInter->distance < door->radius) {
            racerObj = racerObjInter->obj;
            if (racerObj != NULL && racerObj->header->behaviorId == BHV_RACER) {
                racer = racerObj->racer;
                if (racer->playerIndex != PLAYER_COMPUTER && racerObj == doorObj->collisionData->collidedObj) {
                    if (door->textID != -1 && textbox_visible() == FALSE && door->jingleCooldown == 0) {
                        music_fade(-8);
                        door->jingleTimer = 140; // PAL users once again forsaken.
                        music_jingle_voicelimit_set(16);
                        audspat_jingle_off();
                        music_jingle_play(SEQUENCE_NO_TROPHY_FOR_YOU);
                        set_textbox_display_value(door->balloonCount);
                        set_current_text(door->textID & 0xFF);
                    }
                    door->jingleCooldown = 300;
                }
                if (textbox_visible()) {
                    door->jingleCooldown = 300;
                }
            }
        }
        if (door->jingleTimer != 0 && music_jingle_playing() == SEQUENCE_NONE) {
            if (updateRate < door->jingleTimer) {
                door->jingleTimer -= updateRate;
            } else {
                door->jingleTimer = 0;
                music_fade(8);
                music_jingle_voicelimit_set(6);
                audspat_jingle_on();
            }
        }
        if (door->jingleCooldown > 0) {
            door->jingleCooldown -= updateRate;
        } else {
            door->jingleCooldown = 0;
        }
        racerObjInter = doorObj->interactObj;
        keyBits = 0;
        if (racerObjInter->distance < door->radius) {
            racerObj = racerObjInter->obj;
            if (racerObj != NULL && racerObj->header->behaviorId == BHV_RACER) {
                racer = racerObj->racer;
                switch (racer->vehicleID) {
                    case VEHICLE_HOVERCRAFT:
                        keyBits = 2;
                        break;
                    case VEHICLE_PLANE:
                        keyBits = 4;
                        break;
                    default:
                        keyBits = 1;
                        break;
                }
            }
        }
        keyBits &= doorEntry->blockedVehicleMask;
        if (doorEntry->common.objectID == ASSET_OBJECT_ID_BIGBOSSDOOR ||
            doorEntry->common.objectID == ASSET_OBJECT_ID_BOSSDOOR) {
            if (obj_door_override()) {
                dist = 0;
            }
        }
        if (door->openDir == DOOR_CLOSED) {
            if (keyBits == 0 && sp28 != 0 && dist < door->radius) {
                door->openDir = DOOR_OPENING;
            } else if (door->radius + 10 < dist) {
                door->openDir = DOOR_CLOSING;
            }
        }
        playSound = FALSE;
        if (door->doorType & 2) {
            if (sp28 != 0 && doorEntry->common.objectID == ASSET_OBJECT_ID_LEVELDOOR) {
                if (settings->courseFlagsPtr[doorEntry->levelID] & RACE_CLEARED) {
                    if (settings->worldId == WORLD_FUTURE_FUN_LAND || settings->bosses & (1 << settings->worldId)) {
                        door->balloonCount = doorEntry->balloonCountOverride;
                        if (door->balloonCount >= 10) {
                            doorObj->modelIndex = 3;
                        } else {
                            doorObj->modelIndex = 2;
                        }
                    } else {
                        doorObj->modelIndex = 0;
                    }
                }
                if (settings->courseFlagsPtr[doorEntry->levelID] & RACE_CLEARED_SILVER_COINS) {
                    doorObj->modelIndex = 1;
                }
            }
            if (door->openDir == DOOR_OPENING) {
                if (doorObj->trans.y_position < door->homeY + 130.0) {
                    playSound = TRUE;
                    doorObj->trans.y_position += 2.0 * updateRateF;
                }
            } else if (door->openDir == DOOR_CLOSING) {
                if (door->homeY < doorObj->trans.y_position) {
                    playSound = TRUE;
                    doorObj->trans.y_position -= 2.0 * updateRateF;
                }
            }
        } else {
            if (doorEntry->common.objectID == ASSET_OBJECT_ID_BOSSDOOR) {
                doorObj->modelIndex = 0;
                if (settings->bosses & (1 << settings->worldId)) {
                    if (settings->balloonsPtr[settings->worldId] == 8) {
                        doorObj->modelIndex = 1;
                    }
                }
                if (settings->bosses & (1 << (settings->worldId + 6))) {
                    doorObj->modelIndex = 2;
                }
                if (settings->worldId == WORLD_FUTURE_FUN_LAND) {
                    model = doorObj->modelIndex;
                    if (model < 2) {
                        doorObj->modelIndex = model + 1;
                    }
                }
            }
            angleVel = 0;
            if (door->openDir == DOOR_OPENING) {
                angleVel = doorObj->trans.rotation.y_rotation - doorObj->properties.door.openAngle;
            } else if (door->openDir == DOOR_CLOSING) {
                angleVel = doorObj->trans.rotation.y_rotation - doorObj->properties.door.closeAngle;
            }
            angleVel >>= 3;
            if (angleVel > 0x200) {
                angleVel = 0x200;
            }
            if (angleVel < -0x200) {
                angleVel = -0x200;
            }
            doorObj->trans.rotation.y_rotation -= angleVel; //@!Delta
            if (angleVel != 0) {
                playSound = TRUE;
            }
        }
        if (playSound) {
            if (door->soundMask == NULL) {
                audspat_play_sound_at_position(SOUND_DOOR_OPEN, doorObj->trans.x_position, doorObj->trans.y_position,
                                               doorObj->trans.z_position, AUDIO_POINT_FLAG_1, &door->soundMask);
            }
        } else {
            door->openDir = DOOR_CLOSED;
            if (door->soundMask != NULL) {
                audspat_point_stop((AudioPoint *) door->soundMask);
                door->soundMask = NULL;
            }
        }
        if (door->keyID >= 0) {
            keyBits = 1 << door->keyID;
            if (settings->keys & keyBits) {
                settings->courseFlagsPtr[settings->courseId] |= doorIDFlag;
            }
        } else {
            if (doorEntry->localBalloons == FALSE) {
                if (*settings->balloonsPtr >= door->balloonCount) {
                    settings->courseFlagsPtr[settings->courseId] |= doorIDFlag;
                } else {
                    settings->courseFlagsPtr[settings->courseId] &= ~doorIDFlag;
                }
            } else if (settings->balloonsPtr[settings->worldId] >= door->balloonCount) {
                settings->courseFlagsPtr[settings->courseId] |= doorIDFlag;
            } else {
                settings->courseFlagsPtr[settings->courseId] &= ~doorIDFlag;
            }
        }
    }
    doorObj->interactObj->distance = 0xFF;
    doorObj->interactObj->obj = NULL;
    racerObjInter = doorObj->interactObj;
    racerObjInter->flags &= ~INTERACT_FLAGS_PUSHING;
    doorObj->collisionData->collidedObj = NULL;
}

/**
 * T.T Door init func.
 * A much simpler version of the regular door, but has many of the same attributes.
 */
void obj_init_ttdoor(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_TTDoor *entry = &levelEntry->ttDoor;
    Object_Door *ttDoor;
    f32 radius;

    obj->modelIndex = 0;
    ttDoor = obj->door;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    ttDoor->doorType = entry->doorType;
    ttDoor->textID = entry->textID;
    ttDoor->homeY = obj->trans.y_position;
    ttDoor->jingleTimer = 0;
    ttDoor->radius = entry->radius;
    obj->properties.door.closeAngle = obj->trans.rotation.y_rotation;
    obj->properties.door.openAngle = (entry->openRotation & 0x3F) << 0xA;
    radius = entry->scale & 0xFF;
    if (radius < 10) {
        radius = 10;
    }
    radius /= 64;
    obj->trans.scale = obj->header->scale * radius;
    obj->interactObj->flags = INTERACT_FLAGS_SOLID | INTERACT_FLAGS_UNK_0020;
    obj->interactObj->hitboxKind = 2;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
    if (obj->modelIndex >= obj->header->numberOfModelIds) {
        obj->modelIndex = 0;
    }
}

/**
 * T.T Door loop behaviour.
 * Checks if the player has complete the T.T amulet and has all 47 balloons.
 * If so, it will open, otherwise it opens dialogue telling the player to get them.
 */
void obj_loop_ttdoor(Object *obj, s32 updateRate) {
    Settings *settings;
    Object_Door *ttDoor;
    Object *racerObj;
    Object_Racer *racer;
    s16 angle;
    s32 openDoor;

    ttDoor = obj->door;
    settings = get_settings();
    if (ttDoor->doorType == 0) {
        obj->modelIndex = sTTAmuletModelIndices[settings->ttAmulet];
    } else {
        obj->modelIndex = sTTAmuletModelIndicesAlt[settings->ttAmulet];
    }
    if (obj->interactObj->distance < ttDoor->radius && (settings->ttAmulet < 4 || *settings->balloonsPtr < 47)) {
        racerObj = obj->interactObj->obj;
        if (racerObj != NULL && racerObj->header->behaviorId == BHV_RACER) {
            racer = racerObj->racer;
            if (racer->playerIndex != PLAYER_COMPUTER && racerObj == obj->collisionData->collidedObj) {
                if (ttDoor->textID != -1 && textbox_visible() == 0 && ttDoor->jingleCooldown == 0) {
                    music_fade(-8);
                    ttDoor->jingleTimer = 140;
                    music_jingle_voicelimit_set(16);
                    music_jingle_play(SEQUENCE_NO_TROPHY_FOR_YOU);
                    set_current_text(ttDoor->textID & 0xFF);
                }
                ttDoor->jingleCooldown = 300;
            }
            if (textbox_visible()) {
                ttDoor->jingleCooldown = 300;
            }
        }
    }
    if (ttDoor->jingleTimer && music_jingle_playing() == SEQUENCE_NONE) {
        if (updateRate < ttDoor->jingleTimer) {
            ttDoor->jingleTimer -= updateRate;
        } else {
            ttDoor->jingleTimer = 0.0f;
            music_fade(8);
            music_jingle_voicelimit_set(6);
        }
    }
    if (ttDoor->jingleCooldown > 0) {
        ttDoor->jingleCooldown -= updateRate;
    } else {
        ttDoor->jingleCooldown = 0;
    }
    openDoor = TRUE;
    if (settings->ttAmulet >= 4 && obj->interactObj->distance < ttDoor->radius && *settings->balloonsPtr >= 47) {
        angle = obj->trans.rotation.y_rotation - obj->properties.door.openAngle;
    } else {
        angle = obj->trans.rotation.y_rotation - obj->properties.door.closeAngle;
    }
    angle >>= 3;
    if (angle > 0x200) {
        angle = 0x200;
    }
    if (angle < -0x200) {
        angle = -0x200;
    }
    obj->trans.rotation.y_rotation -= angle;
    if (angle == 0) {
        openDoor = FALSE;
    }
    if (openDoor) {
        if (ttDoor->soundMask == NULL) {
            audspat_play_sound_at_position(SOUND_DOOR_OPEN, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position, AUDIO_POINT_FLAG_1, &ttDoor->soundMask);
        }
    } else if (ttDoor->soundMask != NULL) {
        audspat_point_stop((AudioPoint *) ttDoor->soundMask);
        ttDoor->soundMask = NULL;
    }
    obj->interactObj->distance = 0xFF;
    obj->interactObj->obj = NULL;
    obj->interactObj->flags &= 0xFFF7;
    obj->collisionData->collidedObj = NULL;
}
