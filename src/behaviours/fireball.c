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
 * Bubbler boss obstacle init behaviour.
 * Sets hitbox info so it can be interacted with.
 */
void obj_init_fireball_octoweapon(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
}

/**
 * Bubbler boss obstacle loop behaviour.
 * Representing the landmines and bubbletraps that Bubbler throws during the race.
 * When ran into by a player, they will suffer like they usually would when running into a landmine or a bubble trap.
 */
void obj_loop_fireball_octoweapon(Object *obj, s32 updateRate) {
    Object *someObj;
    f32 diff;
    f32 updateRateF;
    Object_Weapon *weapon;
    Object_Racer *racer;
    ObjectInteraction *interactObj;
    SoundHandle soundMask;
    f32 waveHeight;

    someObj = obj->properties.fireball.obj;
    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    if (obj->behaviorId == BHV_FIREBALL_OCTOWEAPON_2 && obj->properties.fireball.timer < 0) {
        obj->trans.x_position = 0.0f;
        obj->trans.y_position = 0.0f;
        obj->trans.z_position = 0.0f;
        ignore_bounds_check();
        move_object(obj, someObj->trans.x_position, someObj->trans.y_position, someObj->trans.z_position);
    } else {
        diff = (someObj->trans.x_position - obj->trans.x_position) * 0.1;
        if (diff > 10.0) {
            diff = 10.0f;
        }
        if (diff < -10.0) {
            diff = -10.0f;
        }
        obj->x_velocity += (diff - obj->x_velocity) * 0.125 * updateRateF;
        diff = (someObj->trans.y_position - obj->trans.y_position) * 0.1;
        if (diff > 10.0) {
            diff = 10.0f;
        }
        if (diff < -10.0) {
            diff = -10.0f;
        }
        obj->y_velocity += (diff - obj->y_velocity) * 0.125 * updateRateF;
        diff = (someObj->trans.z_position - obj->trans.z_position) * 0.1;
        if (diff > 10.0) {
            diff = 10.0f;
        }
        if (diff < -10.0) {
            diff = -10.0f;
        }
        obj->z_velocity += (diff - obj->z_velocity) * 0.125 * updateRateF;
        if (sqrtf((obj->x_velocity * obj->x_velocity) + (obj->z_velocity * obj->z_velocity)) > 0.5) {
            obj->trans.rotation.y_rotation = arctan2_f(obj->x_velocity, obj->z_velocity);
            obj->trans.rotation.x_rotation -= updateRate * 0x200;
        }
        move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF, obj->z_velocity * updateRateF);
        if (obj->objectID == ASSET_OBJECT_ID_OCTOBOMB) {
            if (get_wave_properties(obj->trans.y_position, &waveHeight, NULL)) {
                obj->trans.y_position = waveHeight;
            }
        }
    }
    obj->animFrame += updateRate * 10;
    weapon = obj->weapon;
    interactObj = obj->interactObj;
    if ((interactObj->obj)) {
        if ((interactObj->distance < 60)) {
            someObj = interactObj->obj;
            if (someObj->header->behaviorId == BHV_RACER) {
                racer = someObj->racer;
                if (racer->playerIndex != PLAYER_COMPUTER) {
                    if (obj->behaviorId == BHV_FIREBALL_OCTOWEAPON) {
                        racer->attackType = ATTACK_EXPLOSION;
                        obj->properties.fireball.timer = 20;
                        obj_spawn_effect(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                                         ASSET_OBJECT_ID_BOMBEXPLOSION, SOUND_EXPLOSION, 1.0f, 1);
                        free_object(obj);
                    } else if (obj->properties.fireball.timer > 0) {
                        racer->bubbleTrapTimer = 60;
                        obj->properties.fireball.timer = -60;
                        obj->properties.fireball.obj = someObj;
                        sound_play(SOUND_BUBBLE_RISE, (SoundHandle *) &weapon->soundMask);
                    }
                }
            }
        }
    }
    if (obj->behaviorId == BHV_FIREBALL_OCTOWEAPON) {
        obj->particleEmittersEnabled = OBJ_EMIT_1;
        obj_spawn_particle(obj, updateRate);
        obj->properties.fireball.timer -= updateRate;
        if (obj->properties.fireball.timer < 0) {
            if (obj->objectID == ASSET_OBJECT_ID_OCTOBOMB) {
                free_object(obj);
                obj_spawn_effect(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                                 ASSET_OBJECT_ID_BOMBEXPLOSION, SOUND_EXPLOSION, 1.0f, 1);
            }
            obj->trans.scale *= 0.9; //!@Delta
            if (obj->trans.scale < 0.5) {
                free_object(obj);
            }
        }
    } else {
        if (obj->properties.fireball.timer < 0) {
            obj->properties.fireball.timer += updateRate;
            if (obj->properties.fireball.timer >= 0) {
                obj->properties.fireball.timer = 0;
            }
        } else {
            obj->properties.fireball.timer -= updateRate;
            if (obj->properties.fireball.timer <= 0) {
                obj->properties.fireball.timer = 0;
            }
        }
        if (obj->properties.fireball.timer == 0) {
            soundMask = (SoundHandle) weapon->soundMask;
            if (soundMask != NULL) {
                sndp_stop(soundMask);
            }
            audspat_play_sound_at_position(SOUND_POP, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
            free_object(obj);
        }
    }
}

void obj_destroy_fireball(Object *obj, UNUSED s32 keepAnimTarget) {
    Object_Weapon *weapon = obj->weapon;
    if (weapon->soundMask != NULL) {
        audspat_point_stop(weapon->soundMask);
    }
}
