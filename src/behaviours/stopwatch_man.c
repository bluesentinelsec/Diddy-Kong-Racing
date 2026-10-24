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

SoundHandle gTTSoundMask;

/**
 * Hub world T.T init behaviour.
 * Sets hitbox data to make him solid.
 */
void obj_init_stopwatchman(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    Object_NPC *tt;
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 30;
    obj->interactObj->pushForce = 0;
    tt = obj->npc;
    tt->nodeCurrent = 0xFF;
    tt->splineProgress = 0.0f;
    gTTSoundMask = NULL;
}

/**
 * Walks towards the player until within 10 units, then turns to face them.
 * Switches to the dialogue camera once within 100 units.
 */
static void tt_approach_player(Object *obj, Object_NPC *tt, f32 distance, f32 diffX, f32 diffZ, f32 updateRateF) {
    s32 angleDiff;
    f32 targetSpeed;

    obj->animationID = 0;
    tt->nodeCurrent = 255;
    if (distance < 100.0) {
        racer_set_dialogue_camera();
    }
    if (distance > 10.0) {
        angleDiff =
            (arctan2_f(diffX / distance, diffZ / distance) - (obj->trans.rotation.y_rotation & 0xFFFF)) + 0x8000;
        WRAP(angleDiff, -0x8000, 0x8000);
        if (angleDiff > 0) {
            if (angleDiff < 0x10) {
                angleDiff = 0x10;
            }
        }
        obj->trans.rotation.y_rotation += angleDiff >> 4;
        targetSpeed = -2.0f;
        if (angleDiff > 0x800 || angleDiff < -0x800) {
            targetSpeed = -0.5f;
        }
        tt->forwardVel += (targetSpeed - tt->forwardVel) * 0.125;
        obj->x_velocity = sins_f(obj->trans.rotation.y_rotation) * tt->forwardVel;
        obj->z_velocity = coss_f(obj->trans.rotation.y_rotation) * tt->forwardVel;
        tt->animFrameF -= (tt->forwardVel * 2 * updateRateF);
    } else {
        obj->properties.tt.action = TT_MODE_TURN_TOWARDS_PLAYER;
    }
    move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF, obj->z_velocity * updateRateF);
    set_pause_lockout_timer(1);
}

/**
 * Turns to face the player while sliding into place in front of them, then introduces himself.
 */
static void tt_turn_towards_player(Object *obj, Object_NPC *tt, Object *racerObj, f32 diffX, f32 diffZ,
                                   f32 updateRateF) {
    s32 angleDiff;

    racer_set_dialogue_camera();
    obj->animationID = 0;
    tt->animFrameF += 3.0 * updateRateF;
    angleDiff = (racerObj->trans.rotation.y_rotation - (obj->trans.rotation.y_rotation & 0xFFFF)) + 0x8000;
    WRAP(angleDiff, -0x8000, 0x8000);
    if (angleDiff > 0) {
        if (angleDiff < 0x10) {
            angleDiff = 0x10;
        }
    }
    obj->trans.rotation.y_rotation += (angleDiff >> 4);
    obj->x_velocity = diffX * 0.05;
    obj->z_velocity = diffZ * 0.05;
    if (angleDiff < 0x500) {
        if (angleDiff >= -0x4FF) {
            obj->properties.tt.action = TT_MODE_DIALOGUE;
            play_tt_voice_clip(SOUND_VOICE_TT_INTRO, 1); // Hi there, I'm T.T!
        }
    }
    move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF, obj->z_velocity * updateRateF);
}

/**
 * Talking: keeps in place in front of the player. Leaving the dialogue says goodbye and restores the fog,
 * the level music and the player's engine sound.
 */
static void tt_dialogue(Object *obj, Object_NPC *tt, Object_Racer *racer, LevelHeader *header, s32 dialogueID,
                        f32 diffX, f32 diffZ, f32 updateRateF) {
    obj->x_velocity = diffX * 0.05;
    obj->z_velocity = diffZ * 0.05;
    obj->animationID = 1;
    tt->animFrameF += 1.0 * updateRateF;
    racer_set_dialogue_camera();
    if (dialogueID == 3) {
        obj->properties.tt.action = TT_MODE_DIALOGUE_END;
        if (is_time_trial_enabled()) {
            play_tt_voice_clip(SOUND_VOICE_TT_SEE_YOU, 1);
        } else {
            play_tt_voice_clip(SOUND_VOICE_TT_OK, 1);
        }
        slowly_change_fog(PLAYER_ONE, tt->fogR, tt->fogG, tt->fogB, tt->fogNear, tt->fogFar, 180);
        music_play(header->music);
        music_dynamic_set(header->instruments);
        racer->vehicleSound = racer_sound_init(racer->characterId, racer->vehicleID);
    }
    obj->properties.tt.timer = 180;
    move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF, obj->z_velocity * updateRateF);
}

/**
 * Roaming: finds the nearest AI node to start from, then follows the path.
 */
static void tt_roam(Object *obj, Object_NPC *tt, f32 updateRateF) {
    f32 pathStep;

    obj->animationID = 0;
    tt->forwardVel = 0.0f;
    if (tt->nodeCurrent == NODE_NONE) {
        tt->nodeCurrent =
            ainode_find_nearest(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position, FALSE);
        if (tt->nodeCurrent != NODE_NONE) {
            tt->nodeBack2 = ainode_find_next(tt->nodeCurrent, -1, 0);
            tt->nodeForward1 = ainode_find_next(tt->nodeBack2, tt->nodeCurrent, 0);
            tt->nodeForward2 = ainode_find_next(tt->nodeForward1, tt->nodeBack2, 0);
            tt->nodeBack1 = tt->nodeCurrent;
        }
    } else {
        pathStep = npc_follow_ai_nodes(tt, obj, updateRateF, 1.0f, 0);
        tt->animFrameF += pathStep * 1.5;
    }
}

/**
 * Lifts T.T onto the surface of any water he stands in, other than calm or wavy water.
 */
static void tt_float_on_water(Object *obj) {
    s32 waterIndex;
    struct WaterProperties **water;

    waterIndex = get_level_segment_waves(obj->segmentID, obj->trans.x_position, obj->trans.z_position, &water);
    if (waterIndex != 0) {
        waterIndex--;
        while (waterIndex >= 0) {
            if ((water[waterIndex]->type != SURFACE_WATER_CALM) && (water[waterIndex]->type != SURFACE_WATER_WAVY) &&
                (water[waterIndex]->rot.y > 0.0)) {
                obj->trans.y_position = water[waterIndex]->waveHeight;
            }
            waterIndex--;
        }
    }
}

/**
 * Hub world T.T loop behaviour.
 * Handles all the behaviour for the T.T NPC found in the overworld.
 * This uses a state machine to handle whether to wander around, approach the player and engage dialogue.
 */
void obj_loop_stopwatchman(Object *obj, s32 updateRate) {
    f32 updateRateF;
    f32 diffX;
    f32 diffZ;
    f32 distance;
    f32 tempPosY;
    Object_NPC *tt;
    Object *racerObj;
    s32 angleDiff;
    s32 buttonsPressed;
    s32 dialogueID;
    Object_Racer *racer;
    LevelHeader *header;

    tempPosY = obj->trans.y_position;
    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    tt = obj->npc;
    if (obj->animFrame == 0) {
        if (tt->animFrameF > 1.0) {
            tt->animFrameF = 0.0f;
        }
    }
    header = level_header();
    distance = 0.0f;
    obj->x_velocity = 0.0f;
    obj->z_velocity = 0.0f;
    racerObj = get_racer_object(PLAYER_ONE);
    if (racerObj != NULL) {
        racer = racerObj->racer;
        diffX = ((racerObj->trans.x_position - (racer->ox1 * 50.0f)) - (racer->ox3 * 5.0f)) - obj->trans.x_position;
        diffZ = ((racerObj->trans.z_position - (racer->oz1 * 50.0f)) - (racer->oz3 * 5.0f)) - obj->trans.z_position;
        distance = sqrtf((diffX * diffX) + (diffZ * diffZ));
        angleDiff = arctan2_f(racerObj->trans.x_position - obj->trans.x_position,
                              racerObj->trans.z_position - obj->trans.z_position) -
                    (racerObj->trans.rotation.y_rotation & 0xFFFF);
        if (angleDiff > 0x8000) {
            angleDiff = -0xFFFF;
        }
        if (angleDiff < -0x8000) {
            angleDiff = 0xFFFF;
        }
        if (racer->exitObj != NULL) {
            obj->properties.tt.action = TT_MODE_ROAM;
            obj->properties.tt.timer = 1;
        }
    }
    buttonsPressed = input_pressed(PLAYER_ONE);
    if (obj->properties.tt.action == TT_MODE_ROAM && distance < 300.0 && obj->properties.tt.timer == 0) {
        if (angleDiff > -0x2000 && angleDiff < 0x2000) {
            if ((obj->interactObj->flags & INTERACT_FLAGS_PUSHING && racerObj == obj->interactObj->obj) ||
                buttonsPressed & Z_TRIG) {
                if (buttonsPressed & Z_TRIG) {
                    play_char_horn_sound(racerObj, racer);
                }
                obj->properties.tt.action = TT_MODE_APPROACH_PLAYER;
                get_fog_settings(PLAYER_ONE, &tt->fogNear, &tt->fogFar, &tt->fogR, &tt->fogG, &tt->fogB);
                slowly_change_fog(PLAYER_ONE, 128, 128, 255, 900, 998, 240);
                music_channel_reset_all();
                music_play(SEQUENCE_TTS_THEME);
                if (racerObj != NULL) {
                    racer_sound_free(racerObj);
                    racer->vehicleSound = NULL;
                }
            }
        }
    }
    if (obj->properties.tt.action != TT_MODE_ROAM) {
        disable_racer_input();
        minimap_fade(3);
    }
    if (obj->properties.tt.action >= TT_MODE_TURN_TOWARDS_PLAYER) {
        dialogueID = npc_dialogue_loop(DIALOGUE_TT);
    } else {
        dialogue_npc_finish(2);
        dialogueID = 0;
    }
    switch (obj->properties.tt.action) {
        case TT_MODE_APPROACH_PLAYER:
            tt_approach_player(obj, tt, distance, diffX, diffZ, updateRateF);
            break;
        case TT_MODE_TURN_TOWARDS_PLAYER:
            tt_turn_towards_player(obj, tt, racerObj, diffX, diffZ, updateRateF);
            break;
        case TT_MODE_DIALOGUE:
            tt_dialogue(obj, tt, racer, header, dialogueID, diffX, diffZ, updateRateF);
            break;
        case TT_MODE_DIALOGUE_END:
            tt->animFrameF += 1.0 * updateRateF;
            racer_set_dialogue_camera();
            if (obj->properties.tt.timer < 140) {
                obj->properties.tt.timer += 60;
                obj->properties.tt.action = TT_MODE_ROAM;
                obj->animationID = 0;
            }
            break;
        default:
            tt_roam(obj, tt, updateRateF);
            break;
    }
    obj->trans.y_position = tempPosY;
    tt_float_on_water(obj);
    obj->trans.rotation.x_rotation = 0;
    obj->trans.rotation.z_rotation = 0;
    if (obj->properties.tt.action != TT_MODE_ROAM) {
        gNPCPosY = obj->trans.y_position;
    }
    obj->animFrame = 1.0 * tt->animFrameF;
    object_update_model_animation(obj);
    if (0) {} // Fakematch
    if (obj->properties.tt.timer > 0) {
        obj->properties.tt.timer -= updateRate;
    } else {
        obj->properties.tt.timer = 0;
    }
}

/**
 * If TT is currently talking, clear the audio associated with gTTSoundMask,
 * then play the new sound ID in its place.
 */
void play_tt_voice_clip(u16 soundID, s32 interrupt) {
    if (gTTSoundMask && interrupt & 1) {
        sndp_stop(gTTSoundMask);
        gTTSoundMask = NULL;
    }
    if (gTTSoundMask == NULL) {
        sound_play(soundID, &gTTSoundMask);
    }
}
