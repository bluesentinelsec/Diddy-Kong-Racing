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
 * Smokey's Castle Banana Spawner init func.
 * Seemingly does nothing.
 */
void obj_init_bananacreator(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->animFrame = 100;
}

/**
 * Smokey's Castle Banana Spawner loop func.
 * Spawns a banana. If it is collected, then it will spawn another 20 seconds later.
 */
void obj_loop_bananacreator(Object *obj, s32 updateRate) {
    LevelObjectEntryCommon newEntry;
    Object *newBananaObj;
    Object_Banana *newBananaObj64;

    if (obj->properties.bananaSpawner.spawn != 0) {
        obj->properties.bananaSpawner.timer -= updateRate;
    }

    if (obj->properties.bananaSpawner.timer <= 0) {
        newEntry.x = obj->trans.x_position;
        newEntry.y = (s16) obj->trans.y_position - 3;
        newEntry.z = obj->trans.z_position;
        newEntry.size = sizeof(LevelObjectEntryCommon);
        newEntry.objectID = ASSET_OBJECT_ID_COIN;
        newBananaObj = spawn_object(&newEntry, OBJECT_SPAWN_UNK01);
        obj->properties.bananaSpawner.spawn = TRUE;
        if (newBananaObj) {
            newBananaObj->level_entry = NULL;
            newBananaObj64 = newBananaObj->banana;
            newBananaObj64->spawner = obj;
            obj_spawn_effect(obj->trans.x_position, obj->trans.y_position - 14.0f, obj->trans.z_position,
                             ASSET_OBJECT_ID_BOMBEXPLOSION, SOUND_SELECT, 0.25f, 0);
            obj->properties.bananaSpawner.spawn = FALSE;
        }
        // Set respawn timer 20 seconds. Poor PAL users aren't accounted for, and must wait 24 seconds.
        obj->properties.bananaSpawner.timer = TIME_SECONDS(20);
    }
}

/**
 * Banana init func.
 * Sets the interaction properties.
 * If bananas are disabled, then delete itself.
 */
void obj_init_banana(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 30;
    obj->properties.banana.intangibleTimer = 20; // PAL users doomed to wait 24 frames.
    obj->properties.banana.destroyTimer = 16;
    if (get_filtered_cheats() & CHEAT_DISABLE_BANANAS) {
        free_object(obj);
    }
}

/**
 * Banana loop behaviour.
 * When dropped by a player, will have initial velocity to give some spread.
 * When touched by a player, will increase their banana count.
 */
void obj_loop_banana(Object *obj, s32 updateRate) {
    Object *racerObj;
    Object_Racer *racer;
    Vec3f targetPos;
    f32 radius;
    f32 updateRateF;
    f32 velX;
    f32 velZ;
    s32 hasCollision;
    s32 hitDist;
    s8 surface;
    Object_Banana *banana;
    ObjPropertyBanana *properties;
    AudioPoint *prevSoundMask;

    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    banana = obj->banana;
    obj->animFrame += updateRate * 8;
    properties = &obj->properties.banana;
    if (properties->status == BANANA_COLLECTED) {
        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
        properties->destroyTimer -= updateRate;
        obj->particleEmittersEnabled = OBJ_EMIT_1;
        obj_spawn_particle(obj, updateRate);
        if (properties->destroyTimer <= 0) {
            free_object(obj);
        }
    } else {
        if (banana->pickupIgnoreTimer > 0) {
            banana->pickupIgnoreTimer -= updateRate;
        } else {
            banana->pickupIgnoreTimer = 0;
            banana->pickupIgnoreObj = 0;
        }
        if (properties->status == BANANA_DROPPED) {
            targetPos.f[0] = obj->trans.x_position + (obj->x_velocity * updateRateF);
            targetPos.f[1] = obj->trans.y_position + (obj->y_velocity * updateRateF);
            targetPos.f[2] = obj->trans.z_position + (obj->z_velocity * updateRateF);
            radius = 8.0f;
            generate_collision_candidates(1, &obj->trans.position, &targetPos, VEHICLE_NO_OVERRIDE);
            hasCollision = 0;
            resolve_collisions(&obj->trans.position, &targetPos, &radius, &surface, 1, &hasCollision);
            obj->x_velocity = (targetPos.f[0] - obj->trans.x_position) / updateRateF;
            obj->y_velocity = (targetPos.f[1] - obj->trans.y_position) / updateRateF;
            obj->z_velocity = (targetPos.f[2] - obj->trans.z_position) / updateRateF;
            obj->trans.x_position = targetPos.f[0];
            obj->trans.y_position = targetPos.f[1];
            obj->trans.z_position = targetPos.f[2];
            // Bananas dropped by planes will not have gravity.
            if (banana->droppedVehicleID != VEHICLE_PLANE) {
                obj->y_velocity -= 1.0;  //!@Delta
                obj->x_velocity *= 0.95; //!@Delta
                obj->z_velocity *= 0.95; //!@Delta
            } else {
                obj->x_velocity = 0.0f;
                obj->y_velocity = 0.0f;
                obj->z_velocity = 0.0f;
            }
            velX = obj->x_velocity;
            if (velX < 0.0) {
                velX = -velX;
            }
            velZ = obj->z_velocity;
            if (velZ < 0.0) {
                velZ = -velZ;
            }
            if (hasCollision > 0 && velX < 0.5 && velZ < 0.5) {
                properties->status = BANANA_IDLE;
            }
            radius = -10000.0f;
            if (get_object_water_height(obj, &radius, NULL, 1) != 0 && obj->trans.y_position < radius) {
                properties->status = BANANA_IDLE;
                obj->trans.y_position = radius;
            }
        }
        hitDist = 70;
        if (banana->droppedVehicleID != VEHICLE_PLANE) {
            hitDist = 55; // Non-planes have smaller hitboxes.
        }

        if (properties->intangibleTimer > 0) {
            properties->intangibleTimer -= updateRate;
        } else {
            properties->intangibleTimer = 0;
        }
        if (obj->interactObj->distance < 120) {
            if (level_type() == RACETYPE_CHALLENGE_BANANAS) {
                racerObj = obj->interactObj->obj;
                if (racerObj != NULL && racerObj->header->behaviorId == BHV_RACER) {
                    racer = racerObj->racer;
                    if (racer->playerIndex == PLAYER_COMPUTER) {
                        hitDist += 30; // AI players are offered extra courtesy.
                    }
                }
            }
        }
        if (obj->interactObj->distance < hitDist && properties->intangibleTimer == 0) {
            racerObj = obj->interactObj->obj;
            if (racerObj != NULL && racerObj->header->behaviorId == BHV_RACER) {
                racer = racerObj->racer;
                if (level_type() != RACETYPE_CHALLENGE_BANANAS || racer->bananas < 2) {
                    prevSoundMask = racer->bananaSoundMask;
                    audspat_play_sound_at_position(SOUND_SELECT, racerObj->trans.x_position, racerObj->trans.y_position,
                                                   racerObj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER,
                                                   &racer->bananaSoundMask);
                    if (prevSoundMask) {
                        audspat_point_stop(prevSoundMask);
                    }
                    if (racer->playerIndex != PLAYER_COMPUTER && racer->bananas == 9) {
                        sound_play_spatial(racer->characterId + SOUND_UNK_7B, racerObj->trans.x_position,
                                           racerObj->trans.y_position, racerObj->trans.z_position, NULL);
                    }
                    racer->bananas++;
                    if (banana->spawner != NULL) {
                        banana->spawner->properties.bananaSpawner.spawn = TRUE;
                    }
                    if (get_number_of_active_players() > TWO_PLAYERS) {
                        free_object(obj);
                    } else {
                        properties->status = BANANA_COLLECTED;
                        obj->particleEmittersEnabled = OBJ_EMIT_1;
                        obj_spawn_particle(obj, updateRate);
                    }
                }
            }
        }
    }
}
