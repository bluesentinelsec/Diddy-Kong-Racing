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

// Door type bits.
#define DOOR_TYPE_PROXIMITY 1 // Opens only while a racer is near; otherwise it stays open once unlocked.
#define DOOR_TYPE_SLIDING 2   // Slides up instead of swinging on hinges.

// Frames before a locked door can play its refusal jingle again.
#define DOOR_JINGLE_COOLDOWN 300

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
 * A human racer has driven into a locked door: plays the "no trophy for you" jingle and shows the door's
 * message, unless a textbox is already up or the jingle played recently.
 */
static void door_refuse_entry(Object *doorObj, Object_Door *door, ObjectInteraction *racerObjInter) {
    Object_Racer *racer;
    Object *racerObj;

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
            door->jingleCooldown = DOOR_JINGLE_COOLDOWN;
        }
        if (textbox_visible()) {
            door->jingleCooldown = DOOR_JINGLE_COOLDOWN;
        }
    }
}

/**
 * Once the refusal jingle has finished and its timer runs out, brings the level music back.
 * Also counts down the cooldown before the jingle can play again.
 */
static void door_jingle_update(Object_Door *door, s32 updateRate) {
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
}

/**
 * Returns the vehicle bit (1 car, 2 hovercraft, 4 plane) of a racer within opening range,
 * if the door is blocked to that vehicle. Returns 0 otherwise.
 */
static s32 door_blocked_vehicle(Object *doorObj, LevelObjectEntry_Door *doorEntry, Object_Door *door) {
    s32 keyBits;
    Object_Racer *racer;
    Object *racerObj;
    ObjectInteraction *racerObjInter;

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
    return keyBits;
}

/**
 * Picks the model of an unlocked level door from its race's progress: the race cleared,
 * the world's boss beaten (which also sets the door's balloon count from its override),
 * and the silver coin challenge cleared.
 */
static void door_level_door_model(Object *doorObj, LevelObjectEntry_Door *doorEntry, Object_Door *door,
                                  Settings *settings, s32 isUnlocked) {
    if (isUnlocked != 0 && doorEntry->common.objectID == ASSET_OBJECT_ID_LEVELDOOR) {
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
}

/**
 * Slides a sliding door up (opening, up to 130 units) or back down (closing).
 * Returns TRUE while it is moving.
 */
static s32 door_slide(Object *doorObj, Object_Door *door, f32 updateRateF) {
    s32 playSound;

    playSound = FALSE;
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
    return playSound;
}

/**
 * Picks the model of the world's boss door from boss progress: beaten with all 8 of the world's
 * balloons, or beaten a second time. Future Fun Land's door uses the next model along.
 */
static void door_boss_door_model(Object *doorObj, LevelObjectEntry_Door *doorEntry, Settings *settings) {
    s32 model;

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
}

/**
 * Swings a hinged door towards its open or closed angle, an eighth of the remaining angle per frame.
 * Returns TRUE while it is moving.
 */
static s32 door_swing(Object *doorObj, Object_Door *door) {
    s32 playSound;
    s16 angleVel;

    playSound = FALSE;
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
    return playSound;
}

/**
 * Plays the door sound while it moves. Once it stops, the door counts as closed and the sound stops.
 */
static void door_update_sound(Object *doorObj, Object_Door *door, s32 playSound) {
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
}

/**
 * Sets the door's unlocked flag in the course flags: from its key if it needs one, otherwise from
 * whether the player has enough balloons (in total, or in this world for local balloon doors).
 */
static void door_update_unlock_flag(Object_Door *door, LevelObjectEntry_Door *doorEntry, Settings *settings,
                                    s32 doorIDFlag) {
    s32 keyBits;

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

/**
 * Door loop func.
 * If the requirement to open the door is not met, play a jingle and show a message.
 * Since there's a number of different kinds of doors, this code handles each type.
 */
void obj_loop_door(Object *doorObj, s32 updateRate) {
    s32 playSound;
    volatile s32 doorIDFlag; // ???
    s32 dist;
    s32 blockedVehicle;
    Object_Door *door;
    Settings *settings;
    ObjectInteraction *racerObjInter;
    LevelObjectEntry_Door *doorEntry;
    f32 updateRateF;
    s32 courseFlags;
    s32 isUnlocked;

    doorEntry = &doorObj->level_entry->door;
    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    settings = get_settings();
    courseFlags = settings->courseFlagsPtr[settings->courseId];
    door = doorObj->door;
    if (door->doorID >= 0) {
        doorIDFlag = 0x10000 << door->doorID;
        racerObjInter = doorObj->interactObj;
        dist = racerObjInter->distance;
        if (!(door->doorType & DOOR_TYPE_PROXIMITY)) {
            dist = 0; // Door is forced open
        }
        isUnlocked = courseFlags & doorIDFlag;
        if (isUnlocked == 0 && racerObjInter->distance < door->radius) {
            door_refuse_entry(doorObj, door, racerObjInter);
        }
        door_jingle_update(door, updateRate);
        blockedVehicle = door_blocked_vehicle(doorObj, doorEntry, door);
        if (doorEntry->common.objectID == ASSET_OBJECT_ID_BIGBOSSDOOR ||
            doorEntry->common.objectID == ASSET_OBJECT_ID_BOSSDOOR) {
            if (obj_door_override()) {
                dist = 0;
            }
        }
        if (door->openDir == DOOR_CLOSED) {
            if (blockedVehicle == 0 && isUnlocked != 0 && dist < door->radius) {
                door->openDir = DOOR_OPENING;
            } else if (door->radius + 10 < dist) {
                door->openDir = DOOR_CLOSING;
            }
        }
        playSound = FALSE;
        if (door->doorType & DOOR_TYPE_SLIDING) {
            door_level_door_model(doorObj, doorEntry, door, settings, isUnlocked);
            playSound = door_slide(doorObj, door, updateRateF);
        } else {
            door_boss_door_model(doorObj, doorEntry, settings);
            playSound = door_swing(doorObj, door);
        }
        door_update_sound(doorObj, door, playSound);
        door_update_unlock_flag(door, doorEntry, settings, doorIDFlag);
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
                ttDoor->jingleCooldown = DOOR_JINGLE_COOLDOWN;
            }
            if (textbox_visible()) {
                ttDoor->jingleCooldown = DOOR_JINGLE_COOLDOWN;
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
