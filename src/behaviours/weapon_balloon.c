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
 * Weapon Balloon init behaviour.
 * Sets scale and rotation based off spawn info.
 * Sets model ID based on balloon ID.
 */
void obj_init_weaponballoon(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_WeaponBalloon *entry = &levelEntry->weaponBalloon;
    s32 cheats;
    Object_WeaponBalloon *balloon;
    f32 scale;

    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 4;
    obj->interactObj->hitboxRadius = 20;

    cheats = get_filtered_cheats();

    if (cheats & CHEAT_ALL_BALLOONS_ARE_BLUE) {
        entry->balloonType = BALLOON_TYPE_BOOST;
    } else if (cheats & CHEAT_ALL_BALLOONS_ARE_RED) {
        entry->balloonType = BALLOON_TYPE_MISSILE;
    } else if (cheats & CHEAT_ALL_BALLOONS_ARE_GREEN) {
        entry->balloonType = BALLOON_TYPE_TRAP;
    } else if (cheats & CHEAT_ALL_BALLOONS_ARE_YELLOW) {
        entry->balloonType = BALLOON_TYPE_SHIELD;
    } else if (cheats & CHEAT_ALL_BALLOONS_ARE_RAINBOW) {
        entry->balloonType = BALLOON_TYPE_MAGNET;
    }

    // I guess unk8 is unused?
    if (entry->unk8 >= 6) {
        entry->unk8 = 0;
    }

    if (obj->modelIndex >= obj->header->numberOfModelIds) {
        obj->modelIndex = 0;
    }

    obj->modelIndex = entry->balloonType;
    obj->properties.weaponBalloon.balloonID = obj->modelIndex;

    scale = entry->scale & 0xFF;
    if (scale < 10) {
        scale = 10;
    }
    scale /= 64;

    balloon = obj->weapon_balloon;

    obj->trans.scale = obj->header->scale * scale;
    balloon->scale = obj->trans.scale;
    balloon->respawnTime = 0;
    obj->properties.weaponBalloon.particleTimer = 0;

    if (get_filtered_cheats() & CHEAT_DISABLE_WEAPONS) {
        free_object(obj);
    }
}

/**
 * Weapon Balloon loop func.
 * Checks for any racers going over it.
 * It gives them a weapon, then marks as disappeared, before slowly regrowing and becoming tangible again.
 */
void obj_loop_weaponballoon(Object *weaponBalloonObj, s32 updateRate) {
    UNUSED s32 pad;
    Object_Racer *racer;
    Object_WeaponBalloon *weaponBalloon;
    s8 *powerupTable;
    UNUSED s8 pad3;
    s8 prevBalloonQuantity;
    s8 levelMask;
    Object *whatObjInteracted;
    ObjectInteraction *interactObj;
    s32 prevBalloonType;

    weaponBalloon = weaponBalloonObj->weapon_balloon;
    weaponBalloonObj->trans.scale = weaponBalloon->scale * (1.0 - (weaponBalloon->respawnTime / 90.0f));
    if (weaponBalloonObj->trans.scale < 0.001) {
        weaponBalloonObj->trans.scale = 0.001f;
    }
    if (weaponBalloonObj->trans.scale < 0.1) {
        weaponBalloonObj->trans.flags |= OBJ_FLAGS_INVISIBLE;
    } else {
        weaponBalloonObj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
    }
    if (weaponBalloonObj->properties.weaponBalloon.particleTimer > 0) {
        weaponBalloonObj->particleEmittersEnabled = OBJ_EMIT_1;
        obj_spawn_particle(weaponBalloonObj, updateRate);
        weaponBalloonObj->properties.weaponBalloon.particleTimer -= updateRate;
    }
    if (weaponBalloon->respawnTime != 0) {
        if (weaponBalloon->respawnTime != 90 || weaponBalloonObj->interactObj->distance >= 45) {
            weaponBalloon->respawnTime = (weaponBalloon->respawnTime - updateRate) - updateRate;
        }
        if (weaponBalloon->respawnTime < 0) {
            weaponBalloon->respawnTime = 0;
        }
    } else {
        interactObj = weaponBalloonObj->interactObj;
        if (interactObj->distance < 45) {
            whatObjInteracted = interactObj->obj;
            if (whatObjInteracted != NULL && whatObjInteracted->header->behaviorId == BHV_RACER) {
                racer = whatObjInteracted->racer;
                if (racer->vehicleID < VEHICLE_BOSSES || racer->playerIndex != PLAYER_COMPUTER) {
                    prevBalloonType = racer->balloon_type;
                    racer->balloon_type = weaponBalloonObj->properties.weaponBalloon.balloonID;
                    if (prevBalloonType == racer->balloon_type && racer->balloon_quantity != 0) {
                        racer->balloon_level++;
                    } else {
                        racer->balloon_level = 0;
                    }
                    // Disallow level 3 balloons in challenge mode
                    if (level_type() & RACETYPE_CHALLENGE) {
                        if (racer->balloon_level > 1) {
                            racer->balloon_level = 1;
                        }
                        // And instantly upgrade oil slicks to tripmines.
                        if (racer->balloon_type == BALLOON_TYPE_TRAP) {
                            racer->balloon_level = 1;
                        }
                    }
                    levelMask = 3; // 3 isn't an attainable level so the future check won't be true if the item's
                                   // already max level.
                    if (get_filtered_cheats() & CHEAT_MAXIMUM_POWER_UP) {
                        racer->balloon_level = 2;
                    }
                    if (racer->balloon_level > 2) {
                        racer->balloon_level = 2;
                        levelMask = 2;
                    }
                    powerupTable = (s8 *) get_misc_asset(ASSET_MISC_BALLOON_DATA);
                    prevBalloonQuantity = racer->balloon_quantity;
                    racer->balloon_quantity = powerupTable[(racer->balloon_type * 10) + (racer->balloon_level * 2) + 1];
                    racer->aiFlags |= 1;
                    if (get_number_of_active_players() < THREE_PLAYERS) {
                        weaponBalloonObj->properties.weaponBalloon.particleTimer = 16;
                    }
                    if (racer->playerIndex == PLAYER_COMPUTER) {
                        audspat_play_sound_at_position(
                            SOUND_BALLOON_POP, weaponBalloonObj->trans.x_position, weaponBalloonObj->trans.y_position,
                            weaponBalloonObj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                    } else {
                        if (levelMask == racer->balloon_level) {
                            if (racer->raceFinished == FALSE) {
                                if (prevBalloonQuantity != racer->balloon_quantity) {
                                    hud_sound_play_delayed(SOUND_VOICE_TT_POWERUP, 1.0f, racer->playerIndex);
                                    prevBalloonQuantity = racer->balloon_level;
                                    if (prevBalloonQuantity > 2) {
                                        prevBalloonQuantity = 2;
                                    }
                                    sound_play(SOUND_COLLECT_ITEM + prevBalloonQuantity, NULL);
                                } else {
                                    audspat_play_sound_at_position(
                                        SOUND_BALLOON_POP, weaponBalloonObj->trans.x_position,
                                        weaponBalloonObj->trans.y_position, weaponBalloonObj->trans.z_position,
                                        AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                                }
                            }
                        } else if (racer->raceFinished == FALSE) {
                            if (racer->balloon_level > 0) {
                                hud_sound_play_delayed(SOUND_VOICE_TT_POWERUP, 1.0f, racer->playerIndex);
                            }
                            sound_play(SOUND_COLLECT_ITEM + racer->balloon_level, NULL);
                        }
                    }
                    weaponBalloonObj->particleEmittersEnabled = OBJ_EMIT_1;
                    obj_spawn_particle(weaponBalloonObj, updateRate);
                    weaponBalloon->respawnTime = 90;
                }
            }
        }
    }
}

/**
 * Balloon Burst Effect init behaviour.
 * Does nothing.
 */
void obj_init_wballoonpop(UNUSED Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
}

/**
 * Balloon Burst Effect loop behaviour.
 * Does nothing.
 */
void obj_loop_wballoonpop(UNUSED Object *obj, UNUSED s32 updateRate) {
}
