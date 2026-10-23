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

s32 gRocketSoundTimer;

/**
 * Racer weapon init behaviour.
 * Sets hitbox info so racers can interact with it.
 * Also sets a timer to remove the object after that many frames.
 */
void obj_init_weapon(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 24;
    obj->interactObj->pushForce = 0;
    obj->properties.projectile.timer = normalise_time(480);
    obj->properties.projectile.unk4 = 0;
}

/**
 * Racer weapon loop behaviour.
 * Rockets will move forwards, or towards their victim.
 * Stationary traps will drop to the floor and stay there for a while before self-detonating.
 */
void obj_loop_weapon(Object *obj, s32 updateRate) {
    Object_Weapon *weapon = obj->weapon;
    switch (weapon->weaponID) {
        case WEAPON_ROCKET_HOMING:
        case WEAPON_ROCKET:
            weapon_projectile(obj, updateRate);
            break;
        case WEAPON_TRIPMINE:
        case WEAPON_OIL_SLICK:
        case WEAPON_BUBBLE_TRAP:
        case WEAPON_UNK_11:
            weapon_trap(obj, updateRate);
            break;
    }
    return;
}

/**
 * Firing a standard rocket makes it fly forward.
 * A homing rocket uses the checkpoint system to path towards its victim.
 * When it collides with a racer, they're launched into the air.
 */
void weapon_projectile(Object *obj, s32 updateRate) {
    Object *interactObj;
    Object_Racer *weaponOwner;
    Object_Weapon *weapon;
    Object *temp_s1_2;
    UNUSED s32 pad;
    Vec3f offset;
    f32 radius;
    f32 updateRateF;
    f32 posX;
    f32 posY;
    f32 posZ;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    s32 hasCollision;
    s32 size;
    s32 numCheckpoints;
    Object_Racer *racer;
    s8 surface;
    MtxF mtxf;
    ObjectTransform trans;

    obj->interactObj->flags |= INTERACT_FLAGS_UNK_0100;
    weapon = obj->weapon;
    posX = obj->trans.x_position;
    posY = obj->trans.y_position;
    posZ = obj->trans.z_position;
    trans.rotation.y_rotation = obj->trans.rotation.y_rotation;
    trans.rotation.x_rotation = obj->trans.rotation.x_rotation;
    trans.rotation.z_rotation = 0;
    trans.x_position = 0.0f;
    trans.y_position = 0.0f;
    trans.z_position = 0.0f;
    trans.scale = 1.0f;
    mtxf_from_transform(&mtxf, &trans);
    mtxf_transform_point(mtxf, 0.0f, 0.0f, weapon->forwardVel, &obj->x_velocity, &obj->y_velocity, &obj->z_velocity);
    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    offset.x = obj->trans.x_position + (obj->x_velocity * updateRateF);
    offset.y = obj->trans.y_position + (obj->y_velocity * updateRateF);
    offset.z = obj->trans.z_position + (obj->z_velocity * updateRateF);
    if (weapon->weaponID != WEAPON_MAGNET_LEVEL_3) {
        radius = 16.0f;
        generate_collision_candidates(1, &obj->trans.position, &offset, VEHICLE_NO_OVERRIDE);
        hasCollision = FALSE;
        surface = SURFACE_NONE;
        resolve_collisions(&obj->trans.position, &offset, &radius, &surface, 1, &hasCollision);
        if (hasCollision > 0) {
            if (get_collision_normal(&diffX, &diffY, &diffZ)) {
                obj->properties.projectile.timer = 0;
            }
        }
    }
    diffX = offset.x - posX;
    diffY = offset.y - posY;
    diffZ = offset.z - posZ;
    if (move_object(obj, diffX, diffY, diffZ)) {
        obj->properties.projectile.timer = 0;
    }
    diffX = ((diffX * diffX) + (diffZ * diffZ)) / updateRateF;
    diffZ = (weapon->forwardVel * weapon->forwardVel) / 2;
    if (diffX < diffZ) {
        obj->properties.projectile.timer = 0;
    }
    if (weapon->weaponID == WEAPON_ROCKET_HOMING) {
        numCheckpoints = get_checkpoint_count();
        if (numCheckpoints > 0) {
            if (checkpoint_is_passed(weapon->checkpoint, obj, posX, posY, posZ, &weapon->checkpointDist,
                                     (u8 *) &surface) == FALSE) {
                weapon->checkpoint++;
                if (weapon->checkpoint >= numCheckpoints) {
                    weapon->checkpoint = 0;
                }
            }
        } else {
            weapon->checkpoint = -1;
        }
    }
    if (weapon->weaponID == WEAPON_ROCKET_HOMING) {
        homing_rocket_prevent_overshoot(obj, updateRate, weapon);
    } else {
        rocket_prevent_overshoot(obj, updateRate, weapon);
    }
    // TODO: Fix these gotos.
    if (obj->interactObj->obj != NULL) {
        interactObj = obj->interactObj->obj;
        if (interactObj == weapon->owner) {
            if (obj->properties.projectile.timer < normalise_time(450) && obj->properties.projectile.timer) {
                goto block_25;
            }
            goto block_37;
        }
    block_25:
        if (weapon->weaponID == WEAPON_ROCKET_HOMING) {
            if (interactObj == weapon->target) {
                size = 75;
            } else {
                size = 20;
            }
        } else {
            size = 60;
        }
        if (obj->interactObj->distance < size) {
            if (interactObj->header->behaviorId == BHV_RACER) {
                racer = interactObj->racer;
                racer->attackType = ATTACK_EXPLOSION;
                weaponOwner = weapon->owner->racer;
                if (racer->playerIndex != PLAYER_COMPUTER || weaponOwner->playerIndex != PLAYER_COMPUTER) {
                    weaponOwner->boost_sound |= BOOST_SOUND_UNK2;
                }
                if (racer->raceFinished == FALSE) {
                    rumble_set(racer->playerIndex, RUMBLE_TYPE_9);
                }
            }
            obj_spawn_effect(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                             ASSET_OBJECT_ID_BOMBEXPLOSION, SOUND_EXPLOSION, 1.0f, 1);
            free_object(obj);
            return;
        }
    }
block_37:
    obj->properties.projectile.unk4 += updateRate;
    if (obj->attachPoints != NULL) {
        temp_s1_2 = obj->attachPoints->obj[0];
        if (obj->properties.projectile.unk4 < 8) {
            temp_s1_2->trans.scale = obj->properties.projectile.unk4 * 0.5f;
        } else if (obj->properties.projectile.unk4 < 16) {
            temp_s1_2->trans.scale = 4.0f - ((obj->properties.projectile.unk4 - 8) * 0.25f);
        } else {
            temp_s1_2->trans.scale = (sins_f(obj->properties.projectile.unk4 << 12) * 0.25f) + 2.0f;
        }
    }
    obj->properties.projectile.timer -= updateRate;
    if (obj->properties.projectile.timer < 0) {
        obj_spawn_effect(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                         ASSET_OBJECT_ID_BOMBEXPLOSION, SOUND_EXPLOSION, 1.0f, 1);
        free_object(obj);
        return;
    }
}

/**
 * If a collision target is found for a level 1 or 3 rocket, reverse the velocity and set the next position target
 * to be right on top of them. This ensures a collision is guaranteed this frame.
 * This function also calls the function that plays the incoming rocket sound.
 */
void rocket_prevent_overshoot(Object *obj, UNUSED s32 updateRate, Object_Weapon *rocket) {
    Object *interactedObj;
    f32 dist;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    s32 angle;
    s32 angleDiff;

    if (obj->interactObj->distance < 80 || rocket->hitObj != NULL) {
        interactedObj = obj->interactObj->obj;
        if (interactedObj != NULL && interactedObj != rocket->owner && interactedObj->behaviorId == BHV_RACER) {
            rocket->hitObj = interactedObj;
        }
    }
    interactedObj = rocket->hitObj;
    if (rocket->hitObj != NULL) {
        diffX = interactedObj->trans.x_position - obj->trans.x_position;
        diffY = interactedObj->trans.y_position - obj->trans.y_position;
        diffZ = interactedObj->trans.z_position - obj->trans.z_position;
        dist = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
        if (dist > 0.0f) {
            rocket->forwardVel = -25.0f;
            angle = (arctan2_f(diffX, diffZ) - 0x8000) & 0xFFFF;
            angleDiff = angle - (obj->trans.rotation.y_rotation & 0xFFFF);
            WRAP(angleDiff, -0x8000, 0x8000);
            if (angleDiff > 0x6000 || angleDiff < -0x6000) {
                obj->interactObj->obj = interactedObj;
                obj->interactObj->distance = 1;
            }
            obj->trans.rotation.x_rotation = arctan2_f(diffY, dist);
            obj->trans.rotation.y_rotation = angle;
        }
    }
    play_rocket_trailing_sound(obj, rocket, SOUND_INCOMING_ROCKET);
}

/**
 * If a collision target is found for a level 2 rocket, reverse the velocity and set the next position target
 * to be right on top of them. This ensures a collision is guaranteed this frame.
 * This function also calls the function that plays the incoming rocket sound.
 */
void homing_rocket_prevent_overshoot(Object *obj, s32 updateRate, Object_Weapon *rocket) {
    Object *targetObj;
    Object_Racer *racer;
    f32 dist;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    f32 distY;
    s32 sp58;
    s32 angle;
    s32 sineY;
    s32 angleDiff;
    s32 shift;

    if (rocket->target != NULL) {
        targetObj = rocket->target;
        racer = targetObj->racer;
        rocket->targetTrackOffsetX = racer->trackOffsetX >> 1;
        rocket->targetTrackOffsetY = racer->trackOffsetY >> 1;
        diffX = targetObj->trans.x_position - obj->trans.x_position;
        diffY = targetObj->trans.y_position - obj->trans.y_position;
        diffZ = targetObj->trans.z_position - obj->trans.z_position;
        dist = (diffX * diffX) + (diffZ * diffZ);
        distY = diffY * diffY;
        if (dist < 10000.0 && distY > 10000.0) {
            rocket->target = NULL;
            return;
        }
        dist = sqrtf(dist + distY);
        if (dist > 300.0f && rocket->checkpoint != -1 && rocket->hitObj == NULL) {
            sp58 = homing_rocket_get_next_direction(obj, rocket->checkpoint, racer->isOnAlternateRoute, rocket->targetTrackOffsetX,
                                                    rocket->targetTrackOffsetY, rocket->checkpointDist, &diffX, &diffY, &diffZ);
            sineY = arctan2_f(diffY, 500.0f) & 0xFFFF;
            shift = 3;
        } else {
            rocket->hitObj = targetObj;
            rocket->forwardVel = -25.0f;
            sp58 = FALSE;
        }
        angle = (arctan2_f(diffX, diffZ) - 0x8000) & 0xFFFF;
        angleDiff = angle - (obj->trans.rotation.y_rotation & 0xFFFF);
        WRAP(angleDiff, -0x8000, 0x8000);
        if (sp58) {
            obj->trans.rotation.y_rotation += (angleDiff * updateRate) >> shift;
            angleDiff = sineY - (obj->trans.rotation.x_rotation & 0xFFFF);
            WRAP(angleDiff, -0x8000, 0x8000);
            obj->trans.rotation.x_rotation += (angleDiff * updateRate) >> shift;
        } else {
            if (angleDiff > 0x6000 || angleDiff < -0x6000) {
                obj->interactObj->obj = targetObj;
                obj->interactObj->distance = 1;
            }
            obj->trans.rotation.x_rotation = arctan2_f(diffY, dist) & 0xFFFFu;

            obj->trans.rotation.y_rotation = angle;
        }
    }
    play_rocket_trailing_sound(obj, rocket, SOUND_HOMING_ROCKET);
    if (get_number_of_active_players() < 3) {
        obj->particleEmittersEnabled |= OBJ_EMIT_1;
        obj_spawn_particle(obj, updateRate);
    }
}

/**
 * Set the rocket sound timer back to zero.
 */
void reset_rocket_sound_timer(void) {
    gRocketSoundTimer = 0;
}

/**
 * Reduce the rocket sound timer by 1 increment.
 */
void decrease_rocket_sound_timer(void) {
    gRocketSoundTimer--;
}

/**
 * Existing flying rockets that don't belong to that player will play a sound when flying.
 * This will check if that player is close enough to hear it, and whether to update an existing sound or play a new one.
 */
void play_rocket_trailing_sound(Object *obj, struct Object_Weapon *weapon, u16 soundID) {
    Object *racer;
    Object **racerGroup;
    f32 distance;
    s32 numRacers;
    f32 diffX;
    f32 diffZ;
    f32 diffY;
    s32 shouldPlaySound;
    s32 i;

    shouldPlaySound = FALSE;
    racerGroup = get_racer_objects_by_port(&numRacers);
    for (i = 0; i < numRacers; i++) {
        racer = racerGroup[i];
        if (weapon->owner != racerGroup[i] && racer->racer->playerIndex != PLAYER_COMPUTER) {
            diffX = racer->trans.x_position - obj->trans.x_position;
            diffY = racer->trans.y_position - obj->trans.y_position;
            diffZ = racer->trans.z_position - obj->trans.z_position;
            distance = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
            if (distance <= sound_distance(soundID)) {
                shouldPlaySound = TRUE;
            }
        }
    }
    if (shouldPlaySound) {
        if (weapon->soundMask == NULL) {
            if (gRocketSoundTimer < 8) {
                audspat_play_sound_at_position(soundID, obj->trans.x_position, obj->trans.y_position,
                                               obj->trans.z_position, AUDIO_POINT_FLAG_1, &weapon->soundMask);
                gRocketSoundTimer++;
            }
        } else {
            audspat_point_set_position(weapon->soundMask, obj->trans.x_position, obj->trans.y_position,
                                       obj->trans.z_position);
        }
    } else {
        if (weapon->soundMask) {
            audspat_point_stop(weapon->soundMask);
            weapon->soundMask = NULL;
            gRocketSoundTimer -= 1;
        }
    }
}

/**
 * Handles all the trap weapon behaviour.
 * When dropped, will fall to the ground, provided the user wasn't in a plane.
 * Once on the ground, will count up until it reaches the target and consider itself armed.
 * The timer is used for scaling the trap, which the bubble and oil slick use.
 * If triggered, the trap will perform different behaviours depending on what it is, then remove itself.
 */
void weapon_trap(Object *weaponObj, s32 updateRate) {
    Object *weaponInteractObj;
    Object_Racer *weaponHit;
    Object_Racer *weaponOwner;
    Vec3f intendedPos;
    f32 radius;
    f32 updateRateF;
    s32 hasCollision;
    s32 hitDist;
    s8 surface;
    Object_Weapon *weapon;
    ObjPropertyWeapon *weaponProperties;

    weapon = weaponObj->weapon;
    weaponOwner = weapon->owner->racer;
    updateRateF = updateRate;
    weaponProperties = &weaponObj->properties.weapon;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    if (weaponProperties->status == WEAPON_DROPPED) {
        intendedPos.x = weaponObj->trans.x_position + (weaponObj->x_velocity * updateRateF);
        intendedPos.y = weaponObj->trans.y_position + (weaponObj->y_velocity * updateRateF);
        intendedPos.z = weaponObj->trans.z_position + (weaponObj->z_velocity * updateRateF);
        radius = 9.0f;
        generate_collision_candidates(1, &weaponObj->trans.position, &intendedPos, VEHICLE_NO_OVERRIDE);
        hasCollision = FALSE;
        surface = SURFACE_NONE;
        resolve_collisions(&weaponObj->trans.position, &intendedPos, &radius, &surface, 1, &hasCollision);
        weaponObj->x_velocity = (intendedPos.x - weaponObj->trans.x_position) / updateRateF;
        weaponObj->y_velocity = (intendedPos.y - weaponObj->trans.y_position) / updateRateF;
        weaponObj->z_velocity = (intendedPos.z - weaponObj->trans.z_position) / updateRateF;
        weaponObj->trans.x_position = intendedPos.x;
        weaponObj->trans.y_position = intendedPos.y;
        weaponObj->trans.z_position = intendedPos.z;
        if (hasCollision || weaponOwner->vehicleID == VEHICLE_PLANE) {
            weaponObj->x_velocity = 0.0f;
            weaponObj->z_velocity = 0.0f;
            weaponProperties->status = WEAPON_ARMED;
            weaponProperties->submerged = 0;
            weaponProperties->scale = 0;
            if (weapon->weaponID == WEAPON_BUBBLE_TRAP) {
                weaponObj->trans.y_position += 16.0f;
            }
        }
        radius = -10000.0f;
        if (get_object_water_height(weaponObj, &radius, NULL, 1) && (weaponObj->trans.y_position < radius)) {
            weaponProperties->status = WEAPON_ARMED;
            weaponProperties->submerged = 1;
            weaponProperties->scale = 0;
            if (weapon->weaponID == WEAPON_BUBBLE_TRAP) {
                weaponObj->trans.y_position += 16.0f;
            }
        }
    }
    if (weapon->weaponID == WEAPON_OIL_SLICK) {
        if (weaponProperties->status != WEAPON_DROPPED) {
            if (weaponProperties->status == WEAPON_ARMED) {
                weaponProperties->scale += updateRate;
                if (weaponProperties->scale > 12) {
                    weaponProperties->scale = 12;
                }
            } else {
                weaponProperties->scale -= updateRate;
                if (weaponProperties->scale <= 0) {
                    weaponProperties->scale = 1;
                    free_object(weaponObj);
                }
            }
            if (weaponOwner->vehicleID != VEHICLE_PLANE) {
                if (weaponProperties->submerged != 0) {
                    weaponObj->waterEffect->scale = weaponProperties->scale * 0.6f;
                } else {
                    weaponObj->shadow->scale = weaponProperties->scale * 0.6f;
                }
                weaponObj->trans.flags |= OBJ_FLAGS_SHADOW_ONLY;
            }
        }
    }
    if (weapon->weaponID == WEAPON_BUBBLE_TRAP) {
        if (weapon->target != NULL) {
            weaponObj->trans.x_position = 0.0f;
            weaponObj->trans.y_position = 0.0f;
            weaponObj->trans.z_position = 0.0f;
            ignore_bounds_check();
            move_object(weaponObj, weapon->target->trans.x_position, weapon->target->trans.y_position,
                        weapon->target->trans.z_position);
        }
        if (weaponProperties->status == WEAPON_ARMED || weaponProperties->status == WEAPON_TRIGGERED) {
            weaponProperties->scale += updateRate;
            if (weaponProperties->scale > 20) {
                weaponProperties->scale = 20;
            }
        }
        if (weaponProperties->status == WEAPON_TRIGGERED) {
            weaponProperties->submerged += updateRate;
            if (weaponProperties->submerged > 120) {
                weaponProperties->status = WEAPON_DESTROY;
                if (weapon->soundMask != NULL) {
                    audspat_point_stop(weapon->soundMask);
                    weapon->soundMask = NULL;
                }
                audspat_play_sound_at_position(SOUND_POP, weaponObj->trans.x_position, weaponObj->trans.y_position,
                                               weaponObj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
            }
        }
        if (weaponProperties->status == WEAPON_DESTROY) {
            weaponObj->particleEmittersEnabled = OBJ_EMIT_1;
            obj_spawn_particle(weaponObj, updateRate);
            weaponObj->trans.flags |= OBJ_FLAGS_INVISIBLE;
            weaponProperties->scale -= updateRate;
            if (weaponProperties->scale < 1) {
                weaponProperties->scale = 1;
                free_object(weaponObj);
            }
        }
        if (weaponProperties->status != WEAPON_DROPPED) {
            weaponObj->trans.scale = weaponProperties->scale * 0.075f;
        }
        weaponObj->animFrame += updateRate * 16;
    }
    if (weaponProperties->status < WEAPON_TRIGGERED) {
        hitDist = 60;
        if (weaponOwner->vehicleID != VEHICLE_PLANE) {
            hitDist = 34;
            weaponObj->y_velocity -= 2.0;
            weaponObj->x_velocity -= (weaponObj->x_velocity / 8);
            weaponObj->z_velocity -= (weaponObj->z_velocity / 8);
        } else {
            weaponObj->y_velocity = 0.0f;
            weaponObj->animFrame += updateRate * 8;
        }
        if (weapon->weaponID == WEAPON_BUBBLE_TRAP || weapon->weaponID == WEAPON_OIL_SLICK) {
            hitDist += hitDist >> 1;
        }
        weaponInteractObj = weaponObj->interactObj->obj;
        if (weaponInteractObj != NULL) {
            if (weapon->owner != weaponObj->interactObj->obj || weaponProperties->decayTimer < normalise_time(450)) {
                if (weaponObj->interactObj->distance < hitDist) {
                    weaponInteractObj = weaponObj->interactObj->obj;
                    if (weaponInteractObj->header->behaviorId == BHV_RACER) {
                        weaponHit = weaponInteractObj->racer;
                        weaponHit->attackType = ATTACK_EXPLOSION;
                        if (weapon->weaponID == WEAPON_TRIPMINE) {
                            obj_spawn_effect(weaponObj->trans.x_position, weaponObj->trans.y_position,
                                             weaponObj->trans.z_position, ASSET_OBJECT_ID_BOMBEXPLOSION,
                                             SOUND_EXPLOSION, 1.0f, 1);
                        } else if (weapon->weaponID == WEAPON_BUBBLE_TRAP) {
                            if (weaponHit->shieldTimer > 0 && weaponHit->shieldType >= SHIELD_LEVEL3) {
                                weaponProperties->status = WEAPON_DESTROY;

                                audspat_play_sound_at_position(SOUND_POP, weaponObj->trans.x_position,
                                                               weaponObj->trans.y_position, weaponObj->trans.z_position,
                                                               AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                            } else {
                                weapon->target = weaponInteractObj;

                                audspat_play_sound_at_position(SOUND_BUBBLE, weaponInteractObj->trans.x_position,
                                                               weaponInteractObj->trans.y_position,
                                                               weaponInteractObj->trans.z_position,
                                                               AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, &weapon->soundMask);
                                weaponHit->attackType = ATTACK_BUBBLE;
                                weaponProperties->status = WEAPON_TRIGGERED;
                                weaponProperties->submerged = 0;
                            }
                        } else if (weapon->weaponID == WEAPON_OIL_SLICK) {
                            weaponHit->attackType = ATTACK_SPIN;
                            weaponProperties->status = WEAPON_TRIGGERED;
                        }
                        if (!weaponHit->raceFinished) {
                            rumble_set(weaponHit->playerIndex, RUMBLE_TYPE_13);
                        }
                        weaponOwner = weapon->owner->racer;
                        if (weaponHit->playerIndex != PLAYER_COMPUTER || weaponOwner->playerIndex != PLAYER_COMPUTER) {
                            weaponOwner->boost_sound |= BOOST_SOUND_UNK2;
                        }
                        if (weapon->weaponID != WEAPON_OIL_SLICK && weapon->weaponID != WEAPON_BUBBLE_TRAP) {
                            free_object(weaponObj);
                            return;
                        }
                    }
                }
            }
        }
        weaponProperties->decayTimer -= updateRate;
        if (weaponProperties->status < WEAPON_TRIGGERED) {
            if (weaponProperties->decayTimer < normalise_time(-1320)) {
                if (weapon->weaponID == WEAPON_OIL_SLICK) {
                    weaponProperties->status = WEAPON_TRIGGERED;
                } else if (weapon->weaponID == WEAPON_BUBBLE_TRAP) {
                    weaponProperties->status = WEAPON_DESTROY;
                    audspat_play_sound_at_position(SOUND_POP, weaponObj->trans.x_position, weaponObj->trans.y_position,
                                                   weaponObj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER,
                                                   NULL);
                } else {
                    obj_spawn_effect(weaponObj->trans.x_position, weaponObj->trans.y_position,
                                     weaponObj->trans.z_position, ASSET_OBJECT_ID_BOMBEXPLOSION, SOUND_EXPLOSION, 1.0f,
                                     1);
                    free_object(weaponObj);
                }
            }
        }
    }
}

/**
 * Create a special effect at the given position.
 * Plays a sound if one is given.
 */
void obj_spawn_effect(f32 x, f32 y, f32 z, s32 objectID, s32 soundID, f32 scale, s32 arg6) {
    LevelObjectEntry8003FC44 spawnObj;
    Object *newObj;

    spawnObj.common.x = x;
    spawnObj.common.y = (s16) y + 36;
    spawnObj.common.z = z;
    spawnObj.common.size = 10;
    spawnObj.common.objectID = objectID;
    spawnObj.explosionRepeats = arg6;
    newObj = spawn_object((LevelObjectEntryCommon *) &spawnObj, OBJECT_SPAWN_UNK01);
    if (newObj != NULL) {
        newObj->level_entry = NULL;
        newObj->x_velocity = 0.0f;
        newObj->y_velocity = 0.0f;
        newObj->z_velocity = 0.0f;
        newObj->trans.scale = newObj->trans.scale * 3.5 * scale;
    }
    if (soundID != SOUND_NONE) {
        audspat_play_sound_at_position(soundID, x, y, z, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
    }
}

/**
 * Silence the weapon's trailing sound; a rocket also gives back its sound slot.
 */
void obj_destroy_weapon(Object *obj, UNUSED s32 keepAnimTarget) {
    Object_Weapon *weapon = obj->weapon;
    if (weapon->soundMask != NULL) {
        audspat_point_stop(weapon->soundMask);
        weapon->soundMask = NULL;
        if (obj->behaviorId == BHV_WEAPON_2) {
            decrease_rocket_sound_timer();
        }
    }
}
