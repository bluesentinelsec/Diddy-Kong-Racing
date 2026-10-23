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
 * Overworld Frog init func.
 * Sets the home position to where it is currently.
 * Checks if this frog is supposed to be drumstick.
 * If so, delete if Drumstick is either unlocked, or all trophy races are not yet complete.
 */
void obj_init_frog(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Frog *entry = &levelEntry->frog;
    Object_Frog *frog;

    frog = obj->frog;
    frog->drumstick = entry->drumstick;
    frog->homeX = obj->trans.x_position;
    frog->homeY = obj->trans.y_position;
    frog->homeZ = obj->trans.z_position;
    frog->homeRadius = entry->homeRadius;
    frog->homeRadiusSquare = frog->homeRadius * frog->homeRadius;
    frog->action = FROG_IDLE;
    frog->hopStartX = obj->trans.x_position;
    frog->hopStartZ = obj->trans.z_position;
    frog->squishCooldown = 0;
    frog->scaleY = 1.0f;

    if (frog->drumstick) {
        obj->modelIndex = 1;
        /**
         * Don't spawn the chicken frog if drumstick is already unlocked, or
         * if the player hasn't completed the trophy races yet.
         */
        if (is_drumstick_unlocked() || (get_settings()->trophies & 0xFF) != 0xFF) {
            free_object(obj);
        }
    } else {
        obj->modelIndex = 0;
    }
}

/**
 * Overworld frog loop function.
 * Hops, skips, jumps around. Can be flattened by players.
 * If a Drumstick frog exists after beating all 4 trophy races, he can be squashed and will be unlocked as a playable
 * character.
 */
void obj_loop_frog(Object *obj, s32 updateRate) {
    Object_Frog *frog;
    s32 i;
    s32 hopping;
    s32 var_v1;
    UNUSED s32 pad0[29]; // Wtf is this large gap here?
    f32 colY[8];
    UNUSED s32 pad2[4];
    f32 cosine;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    f32 updateRateF;
    Object *racerObj;

    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    frog = obj->frog;

    switch (frog->action) {
        case FROG_IDLE:
            hopping = FALSE;
            if (frog->squishCooldown > 0) {
                frog->squishCooldown -= updateRate;
            }
            if (obj_dist_racer(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position, 96.0f, 1,
                               &racerObj) > 0) {
                diffX = obj->trans.x_position - racerObj->trans.x_position;
                diffY = obj->trans.y_position - racerObj->trans.y_position;
                diffZ = obj->trans.z_position - racerObj->trans.z_position;
                if (frog->squishCooldown <= 0 && (diffX * diffX) + (diffY * diffY) + (diffZ * diffZ) < 40.0f * 40.0f) {
                    if (frog->drumstick) {
                        audspat_play_sound_at_position(SOUND_VOICE_DRUMSTICK_POSITIVE2, obj->trans.x_position,
                                                       obj->trans.y_position, obj->trans.z_position,
                                                       AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                        set_eeprom_settings_value(2);
                        set_magic_code_flags(CHEAT_CONTROL_DRUMSTICK);
                        set_drumstick_unlock_transition();
                        free_object(obj);
                        break;
                    } else {
                        frog->action = FROG_SQUISH;
                        audspat_play_sound_at_position(SOUND_SPLAT, obj->trans.x_position, obj->trans.y_position,
                                                       obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                    }
                } else {
                    frog->forwardVel = 72.0f;
                    if (frog->drumstick) {
                        frog->forwardVel *= 1.34f;
                    }
                    frog->hopDirection = arctan2_f(-diffX, -diffZ) + 0x8000;
                    hopping = TRUE;
                }
            } else {
                frog->hopTimer -= updateRate;
                if (frog->hopTimer < 0) {
                    hopping = TRUE;
                    frog->forwardVel = rand_range(40, 72);
                    frog->hopDirection = rand_range(-0x4000, 0x4000) + obj->trans.rotation.y_rotation;
                }
            }
            if (hopping) {
                for (i = 0, var_v1 = FALSE; i < 4 && var_v1 == FALSE; i++) {
                    frog->hopTargetX = sins_f(frog->hopDirection) * frog->forwardVel;
                    frog->hopTargetZ = coss_f(frog->hopDirection) * -frog->forwardVel;
                    diffX = (obj->trans.x_position + frog->hopTargetX) - frog->homeX;
                    diffZ = (obj->trans.z_position + frog->hopTargetZ) - frog->homeZ;
                    if ((diffX * diffX) + (diffZ * diffZ) < frog->homeRadiusSquare) {
                        var_v1 = TRUE;
                    } else {
                        frog->hopDirection += 0x4000;
                    }
                }
                if (var_v1 == FALSE) {
                    diffX = frog->homeX - obj->trans.x_position;
                    diffZ = frog->homeZ - obj->trans.z_position;
                    frog->hopDirection = arctan2_f(diffX, diffZ);
                }
                if (frog->drumstick) {
                    audspat_play_sound_at_position(SOUND_VOICE_DRUMSTICK_POSITIVE6, obj->trans.x_position,
                                                   obj->trans.y_position, obj->trans.z_position,
                                                   AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                } else {
                    audspat_play_sound_at_position(SOUND_RIBBIT, obj->trans.x_position, obj->trans.y_position,
                                                   obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                }
                frog->action = FROG_HOP;
                frog->hopStartX = obj->trans.x_position;
                frog->hopStartZ = obj->trans.z_position;
                frog->hopFrame = 32;
                obj->trans.rotation.y_rotation = -frog->hopDirection;
            }
        default:
            break;
        case FROG_HOP:
            frog->hopFrame -= updateRate;
            if (frog->squishCooldown > 0) {
                frog->squishCooldown -= updateRate;
            }
            if (frog->hopFrame < 0) {
                frog->action = FROG_IDLE;
                frog->hopTimer = rand_range(0, 300);
                frog->hopFrame = 0;
            }
            obj->animFrame = ((32 - frog->hopFrame) << 3) / 3;
            cosine = (coss_f(frog->hopFrame << 10) + 1.0f) * 0.5f;
            obj->trans.x_position = frog->hopStartX;
            obj->trans.z_position = frog->hopStartZ;
            obj->x_velocity = frog->hopTargetX * cosine;
            obj->z_velocity = frog->hopTargetZ * cosine;
            ignore_bounds_check();
            move_object(obj, obj->x_velocity, 0.0f, obj->z_velocity);
            if (collision_get_y(obj->segmentID, obj->trans.x_position, obj->trans.z_position, colY, 8) != 0) {
                obj->trans.y_position = 0.0f;
                ignore_bounds_check();
                move_object(obj, 0.0f, (f32) colY[0], 0.0f);
            }
            if (frog->squishCooldown <= 0 && (frog->hopFrame < 6 || frog->hopFrame >= 27)) {
                if (obj_dist_racer(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position, 40.0f, 0,
                                   &racerObj)) {
                    if (frog->drumstick) {
                        audspat_play_sound_at_position(SOUND_VOICE_DRUMSTICK_POSITIVE2, obj->trans.x_position,
                                                       obj->trans.y_position, obj->trans.z_position,
                                                       AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                        set_eeprom_settings_value(2);
                        set_magic_code_flags(CHEAT_CONTROL_DRUMSTICK);
                        set_drumstick_unlock_transition();
                        free_object(obj);
                    } else {
                        frog->action = FROG_SQUISH;
                        audspat_play_sound_at_position(SOUND_SPLAT, obj->trans.x_position, obj->trans.y_position,
                                                       obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                    }
                }
            }
            break;
        case FROG_SQUISH:
            frog->scaleY -= 0.15f * updateRateF;
            if (frog->scaleY < 0.05f) {
                frog->scaleY = 0.05f;
                frog->action = FROG_FLAT;
                frog->hopTimer = 40;
            }
            break;
        case FROG_FLAT:
            frog->hopTimer -= updateRate;
            if (frog->hopTimer < 0) {
                frog->action = FROG_UNSQUISH;
                audspat_play_sound_at_position(SOUND_PLOP2, obj->trans.x_position, obj->trans.y_position,
                                               obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
            }
            break;
        case FROG_UNSQUISH:
            if (frog->scaleY > 1.0f) {
                frog->scaleY = 1.0f;
                frog->action = FROG_IDLE;
                frog->hopTimer = 0;
                frog->squishCooldown = 60;
            } else {
                frog->scaleY += 0.15f * updateRateF;
            }
            break;
    }
}
