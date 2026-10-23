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

f32 gNPCPosY;
SoundHandle gTajSoundMask;
s8 gTajDialogueChoice;
s16 gTajSoundID; // Taj Voice clips

FadeTransition gTajTransformTransitionEnd = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_INVERT, FADE_COLOR_WHITE, 7, 3);
FadeTransition gTajTransition = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_NONE, FADE_COLOR_BLACK, 30, FADE_STAY);

/**
 * Hub world Taj init behaviour.
 * Sets hitbox data to give him solidity.
 */
void obj_init_parkwarden(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    Object_NPC *taj;
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 30;
    obj->interactObj->pushForce = 0;
    taj = obj->npc;
    taj->nodeCurrent = 0xFF;
    taj->splineProgress = 0.0f;
    taj->prevMusicChannelMask = 0;
    taj->musicLayerTimer = 0;
    taj->musicFade = 0;
    taj->musicLayerState = 0;
    gTajSoundMask = NULL;
    gTajSoundID = SOUND_VOICE_TAJ_HELLO;
}

/**
 * Sets the sound ID of Taj's next voice line when spoken to.
 */
void set_taj_voice_line(s16 soundID) {
    // Set to SOUND_VOICE_TAJ_CHALLENGE_RACE in func_800CC7C
    gTajSoundID = soundID;
}

/**
 * Hub world Taj loop behaviour.
 * Handles all the behaviour for the Taj NPC found in the overworld.
 * This uses a state machine to handle whether to wander around, approach the player, change their vehicle and teleport
 * away.
 */
void obj_loop_parkwarden(Object *obj, s32 updateRate) {
    f32 updateRateF;
    f32 xPosDiff;
    f32 updateRateF2;
    f32 zPosDiff;
    f32 distance;
    f32 tempPosY;
    struct WaterProperties **water;
    Object *racerObj;
    s32 sp3C;
    Object *telepoint;
    s32 var_a2;
    Object **racerObjs;
    s32 dialogueID;
    s32 numRacers;
    Object_NPC *taj;
    Object_Racer *racer;
    u32 buttonsPressed;
    s8 spawnSmoke;
    LevelHeader *levelHeader;
    Camera *temp_v0_22;
    f32 var_f2;
    s32 arctan;
    s32 temp;

    spawnSmoke = FALSE;
    tempPosY = obj->trans.y_position;

    updateRateF2 = updateRate;
    updateRateF = updateRateF2;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    taj = obj->npc;
    levelHeader = level_header();
    obj->particleEmittersEnabled = OBJ_EMIT_NONE;
    if (obj->animFrame == 0 && taj->animFrameF > 1.0) {
        taj->animFrameF = 0.0f;
    }
    distance = 0.0f;
    obj->x_velocity = 0.0f;
    obj->z_velocity = 0.0f;
    racerObj = get_racer_object(PLAYER_ONE);
    if (racerObj != NULL) {
        racer = racerObj->racer;
        xPosDiff = (racerObj->trans.x_position - (racer->ox1 * 50.0f)) - obj->trans.x_position;
        zPosDiff = (racerObj->trans.z_position - (racer->oz1 * 50.0f)) - obj->trans.z_position;
        distance = sqrtf((xPosDiff * xPosDiff) + (zPosDiff * zPosDiff));
    }
    buttonsPressed = input_pressed(PLAYER_ONE);
    var_a2 = FALSE;
    if ((obj->properties.taj.action == NULL) && (distance < 300.0) &&
        (((obj->interactObj->flags & INTERACT_FLAGS_PUSHING) && (racerObj == obj->interactObj->obj)) ||
         (buttonsPressed & Z_TRIG))) {
        if (buttonsPressed & Z_TRIG) {
            play_char_horn_sound(racerObj, racer);
        }
        arctan = arctan2_f(racerObj->trans.x_position - obj->trans.x_position,
                           racerObj->trans.z_position - obj->trans.z_position) -
                 (racerObj->trans.rotation.y_rotation & 0xFFFF);
        if (arctan > 0x8000) {
            arctan = -0xFFFF;
        }
        if (arctan < -0x8000) {
            arctan = 0xFFFF;
        }
        var_a2 = FALSE;
        if (arctan > -0x2000 && arctan < 0x2000) {
            var_a2 = TRUE;
        }
    }
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    if ((should_taj_teleport() || var_a2) &&
        (obj->properties.taj.action == TAJ_MODE_ROAM || obj->properties.taj.action == TAJ_MODE_UNK1F)) {
        music_channel_reset_all();
        music_voicelimit_set(24);
        music_play(SEQUENCE_ENTRANCED);
        if (racerObj != NULL) {
            racer_sound_free(racerObj);
            racer->vehicleSound = NULL;
        }
        audspat_jingle_off();
        arctan = arctan2_f(racerObj->trans.x_position - obj->trans.x_position,
                           racerObj->trans.z_position - obj->trans.z_position);
        arctan -= (racerObj->trans.rotation.y_rotation & 0xFFFF);
        if ((arctan > 0x8000)) {
            // Probably had a debug statement here.
            if ((obj->properties.taj.action) && (!obj->properties.taj.action) && (!obj->properties.taj.action)) {
            } // Fakematch
        }
        if (var_a2) {
            obj->properties.taj.action = TAJ_MODE_APPROACH_PLAYER;
        } else {
            obj->properties.taj.action = TAJ_MODE_TELEPORT_TO_PLAYER_BEGIN;
            spawnSmoke = TRUE;
        }
        get_fog_settings(PLAYER_ONE, &taj->fogNear, &taj->fogFar, &taj->fogR, &taj->fogG, &taj->fogB);
        slowly_change_fog(PLAYER_ONE, 255, 0, 120, 960, 1100, 240);
        taj->animFrameF = 0.0f;
    }

    if (!(obj->properties.taj.action == TAJ_MODE_ROAM || obj->properties.taj.action == TAJ_MODE_RACE ||
          obj->properties.taj.action == TAJ_MODE_TELEPORT_AWAY_BEGIN ||
          obj->properties.taj.action == TAJ_MODE_TELEPORT_AWAY_END)) {
        disable_racer_input();
        minimap_fade(3);
    }

    switch (obj->properties.taj.action) {
        case TAJ_MODE_GREET_PLAYER:
        case TAJ_MODE_DIALOGUE:
        case TAJ_MODE_TRANSFORM_BEGIN:
        case TAJ_MODE_TRANSFORM_END:
            dialogueID = npc_dialogue_loop(DIALOGUE_TAJ);
            break;
        default:
            dialogue_npc_finish(0);
            dialogueID = 0;
            break;
    }

    switch (obj->properties.taj.action) {
        case TAJ_MODE_APPROACH_PLAYER:
        case TAJ_MODE_TURN_TOWARDS_PLAYER:
        case TAJ_MODE_GREET_PLAYER:
        case TAJ_MODE_DIALOGUE:
        case TAJ_MODE_END_DIALOGUE:
        case TAJ_MODE_TELEPORT_TO_PLAYER_BEGIN:
        case TAJ_MODE_TELEPORT_TO_PLAYER_END:
        case TAJ_MODE_SET_CHALLENGE:
        case TAJ_MODE_TELEPORT_AWAY_BEGIN:
        case TAJ_MODE_TELEPORT_AWAY_END:
            set_pause_lockout_timer(1);
            break;
    }
    if (obj->properties.taj.action != TAJ_MODE_ROAM && dialogueID != 0 && obj->properties.taj.action < 4) {
        obj->properties.taj.action = TAJ_MODE_DIALOGUE;
    }
    switch (obj->properties.taj.action) {
        case TAJ_MODE_APPROACH_PLAYER:
            obj->animationID = 0;
            taj->nodeCurrent = 0xFF;
            if (distance < 100.0) {
                racer_set_dialogue_camera();
            }
            if (distance > 10.0) {
                arctan =
                    (arctan2_f(xPosDiff / distance, zPosDiff / distance) - (obj->trans.rotation.y_rotation & 0xFFFF)) +
                    0x8000;
                if (arctan > 0x8000) {
                    arctan -= 0xFFFF;
                }
                if (arctan < -0x8000) {
                    arctan += 0xFFFF;
                }
                if (arctan > 0 && arctan < 16) {
                    arctan = 16;
                }
                obj->trans.rotation.y_rotation += (arctan >> 4);
                var_f2 = -2.0f;
                if (arctan > 0x800 || arctan < -0x800) {
                    var_f2 = -0.5f;
                }
                taj->forwardVel += (var_f2 - taj->forwardVel) * 0.125;
                obj->x_velocity = sins_f(obj->trans.rotation.y_rotation) * taj->forwardVel;
                obj->z_velocity = coss_f(obj->trans.rotation.y_rotation) * taj->forwardVel;
                taj->animFrameF -= taj->forwardVel * 2 * updateRateF;
            } else {
                obj->properties.taj.action = TAJ_MODE_TURN_TOWARDS_PLAYER;
            }
            move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF,
                        obj->z_velocity * updateRateF);
            break;
        case TAJ_MODE_TURN_TOWARDS_PLAYER:
            racer_set_dialogue_camera();
            obj->animationID = 0;
            taj->animFrameF += updateRateF * 2.0;
            arctan = (racerObj->trans.rotation.y_rotation - (obj->trans.rotation.y_rotation & 0xFFFF)) + 0x8000;
            if (arctan > 0x8000) {
                arctan -= 0xFFFF;
            }
            if (arctan < -0x8000) {
                arctan += 0xFFFF;
            }
            if (arctan > 0 && arctan < 16) {
                arctan = 16;
            }
            obj->trans.rotation.y_rotation += arctan >> 3;
            if (arctan < 0x400 && arctan > -0x400 && distance < 2.0) {
                obj->properties.taj.action = TAJ_MODE_GREET_PLAYER;
                taj->animFrameF = 0;
                play_taj_voice_clip(gTajSoundID, TRUE);
                gTajSoundID = SOUND_VOICE_TAJ_HELLO;
            }
            obj->x_velocity = xPosDiff * 0.125;
            obj->y_velocity = 0;
            obj->z_velocity = zPosDiff * 0.125;
            move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF,
                        obj->z_velocity * updateRateF);
            break;
        case TAJ_MODE_GREET_PLAYER:
            obj->animationID = 1;
            taj->forwardVel = 0.0f;
            taj->animFrameF += updateRateF * 1.0;
            if (taj->animFrameF > 77.0) {
                taj->animFrameF = 77.0;
                taj->unused18 = -1.0f;
                obj->properties.taj.action = TAJ_MODE_DIALOGUE;
            }
            arctan = (racerObj->trans.rotation.y_rotation - (obj->trans.rotation.y_rotation & 0xFFFF)) + 0x8000;
            if (arctan > 0x8000) {
                arctan -= 0xFFFF;
            }
            if (arctan < -0x8000) {
                arctan += 0xFFFF;
            }
            if (arctan > 0 && arctan < 16) {
                arctan = 16;
            }
            obj->trans.rotation.y_rotation = obj->trans.rotation.y_rotation + (arctan >> 4);
            racer_set_dialogue_camera();
            break;
        case TAJ_MODE_DIALOGUE:
            obj->animationID = 4;
            taj->animFrameF += updateRateF * 1.0;
            racer_set_dialogue_camera();
            if (dialogueID == 3 || dialogueID == 4) {
                obj->properties.taj.action =
                    (dialogueID == 4) ? TAJ_MODE_END_DIALOGUE_CHALLENGE : TAJ_MODE_END_DIALOGUE;
                taj->animFrameF = 0.1f;
                obj->animationID = 2;
                taj->faceRacerTimer = 0;
                play_taj_voice_clip(SOUND_VOICE_TAJ_BYE, TRUE);
                slowly_change_fog(PLAYER_ONE, taj->fogR, taj->fogG, taj->fogB, taj->fogNear, taj->fogFar, 180);
                music_voicelimit_set(levelHeader->voiceLimit);
                music_play(levelHeader->music);
                music_dynamic_set(levelHeader->instruments);
                audspat_jingle_on();
            }
            if (dialogueID & 0x80) {
                gTajDialogueChoice = dialogueID & 0x7F;
                if (gTajDialogueChoice != racer->vehicleID) {
                    obj->properties.taj.action = TAJ_MODE_TRANSFORM_BEGIN;
                    taj->animFrameF = 0;
                    // Voice clips: Abrakadabra, Alakazam, Alakazoom?
                    play_taj_voice_clip((racer->vehicleID + SOUND_VOICE_TAJ_ABRAKADABRA), TRUE);
                } else {
                    set_menu_id_if_option_equal(0x62, 2);
                }
            }
            if (dialogueID & 0x40) {
                gTajDialogueChoice = dialogueID & 0xF;
                if (gTajDialogueChoice != racer->vehicleID) {
                    gTajDialogueChoice |= 0x80;
                    obj->properties.taj.action = TAJ_MODE_TRANSFORM_BEGIN;
                    taj->animFrameF = 0.0f;
                    // Voice clips: Abrakadabra, Alakazam, Alakazoom?
                    play_taj_voice_clip((racer->vehicleID + SOUND_VOICE_TAJ_ABRAKADABRA), TRUE);
                } else {
                    obj->properties.taj.action = TAJ_MODE_SET_CHALLENGE;
                    transition_begin(&gTajTransition);
                    spawnSmoke = TRUE;
                    play_taj_voice_clip(SOUND_WHOOSH4, TRUE);
                    taj->animFrameF = 0.0f;
                }
            }
            break;
        case TAJ_MODE_TRANSFORM_BEGIN:
            obj->animationID = 5;
            racer_set_dialogue_camera();
            taj->animFrameF += updateRateF * 2.0;
            if (taj->animFrameF > 25.0) {
                obj->particleEmittersEnabled = OBJ_EMIT_1 | OBJ_EMIT_2 | OBJ_EMIT_4;
            }
            if (taj->animFrameF > 50.0) {
                obj->particleEmittersEnabled = OBJ_EMIT_NONE;
            }
            if (taj->animFrameF > 60.0) {
                taj->animFrameF = 60.0f;
                if (racer->transparency > (updateRate * 16)) {
                    racer->transparency -= (updateRate * 16);
                } else {
                    racer->transparency = 0;
                    despawn_player_racer(racerObj, gTajDialogueChoice & 0xF);
                    obj->properties.taj.action = TAJ_MODE_TRANSFORM_END;
                    sound_play(SOUND_CYMBAL, NULL);
                    transition_begin(&gTajTransformTransitionEnd);
                }
            }
            break;
        case TAJ_MODE_TRANSFORM_END:
            racer_set_dialogue_camera();
            if (racerObj != NULL) {
                if (taj->animFrameF != 0.0f) {
                    taj->animFrameF += 8.0;
                }
                var_a2 = updateRate;
                if (var_a2 > 4) {
                    var_a2 = 4;
                }
                var_a2 <<= 5;
                if (racer->transparency < (255 - (var_a2))) {
                    racer->transparency += var_a2;
                } else {
                    racer->transparency = 255;
                    if (taj->animFrameF == 0.0) {
                        if (gTajDialogueChoice & 0x80) {
                            transition_begin(&gTajTransition);
                            spawnSmoke = TRUE;
                            obj->properties.taj.action = TAJ_MODE_SET_CHALLENGE;
                            sound_play(SOUND_WHOOSH4, NULL);
                            taj->animFrameF = 0.0f;
                        } else {
                            obj->properties.taj.action = TAJ_MODE_DIALOGUE;
                            set_menu_id_if_option_equal(0x62, 2);
                        }
                    }
                }
            }
            break;
        case TAJ_MODE_END_DIALOGUE:
        case TAJ_MODE_END_DIALOGUE_CHALLENGE:
            if (taj->animFrameF != 0.0) {
                taj->animFrameF = taj->animFrameF + (0.5 * updateRateF);
            }
            if (taj->animFrameF == 0) {
                spawnSmoke = TRUE;
                if (obj->properties.taj.action == TAJ_MODE_END_DIALOGUE_CHALLENGE) {
                    obj_taj_create_balloon(obj->segmentID, obj->trans.x_position - (racer->ox1 * 50.0f),
                                           obj->trans.y_position, obj->trans.z_position - (racer->oz1 * 50.0f));
                }
                obj->properties.taj.action = TAJ_MODE_TELEPORT_AWAY_BEGIN;
                sound_play(SOUND_WHOOSH4, NULL);
                racer->vehicleSound = racer_sound_init(racer->characterId, racer->vehicleID);
            }
            break;
        case TAJ_MODE_TELEPORT_TO_PLAYER_BEGIN:
            obj->animationID = 3;
            taj->nodeCurrent = 0xFF;
            taj->forwardVel = 0.0f;
            taj->animFrameF += updateRateF * 2.0;
            if (taj->animFrameF > 79.0f) {
                taj->animFrameF = 79.0f;
            }

            var_a2 = updateRate * 8;
            if (taj->animFrameF < 20.0f) {
                var_a2 = 0;
            }
            if (obj->opacity > var_a2) {
                obj->opacity -= var_a2;
            } else {
                sound_play(SOUND_WHOOSH4, NULL);
                spawnSmoke = TRUE;
                obj->opacity = 0;
                obj->properties.taj.action = TAJ_MODE_TELEPORT_TO_PLAYER_END;
                obj->trans.x_position = racerObj->trans.x_position - (racer->ox1 * 150.0f);
                obj->trans.z_position = racerObj->trans.z_position - (racer->oz1 * 150.0f);
                obj->segmentID = get_level_segment_index_from_position(obj->trans.x_position, obj->trans.y_position,
                                                                       obj->trans.z_position);
                obj->trans.rotation.y_rotation = racerObj->trans.rotation.y_rotation + 0x8000;
            }
            break;
        case TAJ_MODE_TELEPORT_TO_PLAYER_END:
            obj->animationID = 3;
            taj->animFrameF -= updateRateF * 2.0;
            if (taj->animFrameF < 0.0f) {
                taj->animFrameF = 0.0f;
            }
            var_a2 = updateRate * 4;
            if (obj->opacity < (255 - var_a2)) {
                obj->opacity += var_a2;
            } else {
                obj->opacity = 255;
                obj->properties.taj.action = TAJ_MODE_APPROACH_PLAYER;
            }
            break;
        case TAJ_MODE_SET_CHALLENGE:
            obj->animationID = 3;
            taj->nodeCurrent = 0xFF;
            taj->forwardVel = 0.0f;
            taj->animFrameF += updateRateF * 2.0;
            if (taj->animFrameF > 79.0f) {
                taj->animFrameF = 79.0f;
            }
            var_a2 = updateRate * 8;
            if (taj->animFrameF < 20.0f) {
                var_a2 = 0;
            }
            if (obj->opacity > var_a2) {
                obj->opacity -= var_a2;
            } else {
                racer->vehicleSound = racer_sound_init(racer->characterId, racer->vehicleID);
                slowly_change_fog(PLAYER_ONE, taj->fogR, taj->fogG, taj->fogB, taj->fogNear, taj->fogFar, 180);
                music_voicelimit_set(levelHeader->voiceLimit);
                music_play(levelHeader->music);
                music_dynamic_set(levelHeader->instruments);
                init_racer_for_challenge(racer->vehicleID);
                telepoint = find_furthest_telepoint(obj->trans.x_position, obj->trans.z_position);
                if (telepoint != NULL) {
                    obj->trans.x_position = telepoint->trans.x_position;
                    obj->trans.z_position = telepoint->trans.z_position;
                    obj->segmentID = telepoint->segmentID;
                    obj->trans.rotation.y_rotation = racerObj->trans.rotation.y_rotation + 0x8000;
                }
                obj->properties.taj.action = TAJ_MODE_RACE;
            }
            break;
        case TAJ_MODE_TELEPORT_AWAY_BEGIN:
            obj->animationID = 3;
            taj->nodeCurrent = 0xFF;
            taj->forwardVel = 0.0f;
            taj->animFrameF += updateRateF * 2.0;
            if (taj->animFrameF > 79.0f) {
                taj->animFrameF = 79.0f;
            }
            var_a2 = updateRate * 8;
            if (taj->animFrameF < 20.0f) {
                var_a2 = 0;
            }
            if (obj->opacity > var_a2) {
                obj->opacity -= var_a2;
            } else {
                obj->opacity = 0;
                obj->properties.taj.action = TAJ_MODE_TELEPORT_AWAY_END;
                telepoint = find_furthest_telepoint(obj->trans.x_position, obj->trans.z_position);
                if (telepoint != NULL) {
                    obj->trans.x_position = telepoint->trans.x_position;
                    obj->trans.z_position = telepoint->trans.z_position;
                    obj->segmentID = telepoint->segmentID;
                    obj->trans.rotation.y_rotation = racerObj->trans.rotation.y_rotation + 0x8000;
                }
            }
            break;
        case TAJ_MODE_TELEPORT_AWAY_END:
            obj->animationID = 3;
            taj->animFrameF -= updateRateF * 2.0;
            if (taj->animFrameF < 0.0f) {
                taj->animFrameF = 0.0f;
            }
            var_a2 = updateRate * 4;
            if (obj->opacity < (255 - var_a2)) {
                obj->opacity += var_a2;
            } else {
                obj->opacity = 255;
                obj->properties.taj.action = TAJ_MODE_ROAM;
            }
            break;
        case TAJ_MODE_RACE:
            obj->interactObj->flags = INTERACT_FLAGS_NONE;
            obj->animationID = 6;
            obj->opacity = 255;
            taj->animFrameF += updateRateF * 1.0;
            break;
        default:
            obj->animationID = 0;
            taj->forwardVel = 0.0f;
            if (taj->nodeCurrent == NODE_NONE) {
                taj->nodeCurrent =
                    ainode_find_nearest(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position, FALSE);
                if (taj->nodeCurrent != NODE_NONE) {
                    taj->nodeBack2 = ainode_find_next(taj->nodeCurrent, -1, 0);
                    taj->nodeForward1 = ainode_find_next(taj->nodeBack2, taj->nodeCurrent, 0);
                    taj->nodeForward2 = ainode_find_next(taj->nodeForward1, taj->nodeBack2, 0);
                    taj->nodeBack1 = taj->nodeCurrent;
                }
            } else {
                if ((distance < 55.0f) && (taj->faceRacerTimer == 0) && (racerObj != NULL)) {
                    taj->faceRacerTimer = 240;
                    taj->faceRacerAngle = (s16) (arctan2_f(xPosDiff / distance, zPosDiff / distance) + 0x4000);
                }
                if (taj->faceRacerTimer > 0) {
                    taj->faceRacerTimer -= updateRate;
                } else {
                    taj->faceRacerTimer = 0;
                }
                if (taj->faceRacerTimer < 120) {
                    taj->animFrameF += npc_follow_ai_nodes(taj, obj, updateRateF, 1.0f, 0);
                } else {
                    var_a2 = taj->faceRacerAngle - (obj->trans.rotation.y_rotation & 0xFFFF);
                    if (var_a2 > 0x8000) {
                        var_a2 -= 0xFFFF;
                    }
                    if (var_a2 < -0x8000) {
                        var_a2 += 0xFFFF;
                    }
                    obj->trans.rotation.y_rotation += ((var_a2 * updateRate) >> 4);
                    xPosDiff = sins_f(obj->trans.rotation.y_rotation + 0x8000);
                    zPosDiff = coss_f(obj->trans.rotation.y_rotation + 0x8000);
                    move_object(obj, (updateRateF2 * xPosDiff) * 1.1, 0.0f, (updateRateF2 * zPosDiff) * 1.1);
                    taj->animFrameF += updateRate * 2.2;
                }
            }
            racerObjs = get_racer_objects(&numRacers);
            if (racerObjs[PLAYER_ONE] != NULL) {
                xPosDiff = racerObjs[PLAYER_ONE]->trans.x_position - obj->trans.x_position;
                distance = racerObjs[PLAYER_ONE]->trans.y_position - obj->trans.y_position;
                zPosDiff = racerObjs[PLAYER_ONE]->trans.z_position - obj->trans.z_position;
                var_f2 = sqrtf((xPosDiff * xPosDiff) + (distance * distance) + (zPosDiff * zPosDiff));
                if (var_f2 < 1000.0f) {
                    var_f2 = 1000.0f - var_f2;
                    sp3C = (127.0f * var_f2) / 1000.0f;
                    temp_v0_22 = cam_get_cameras();
                    xPosDiff = obj->trans.x_position - temp_v0_22->trans.x_position;
                    zPosDiff = obj->trans.z_position - temp_v0_22->trans.z_position;
                    arctan = audspat_calculate_spatial_pan(xPosDiff, zPosDiff, temp_v0_22->trans.rotation.y_rotation);
                    temp = arctan;
                    music_channel_fade_set(10, sp3C);
                    music_channel_fade_set(11, sp3C);
                    music_channel_fade_set(15, sp3C);
                    music_channel_pan_set(10, temp);
                    music_channel_pan_set(11, temp);
                    music_channel_pan_set(15, temp);
                    music_channel_on(10);
                    music_channel_on(11);
                    music_channel_on(15);
                    music_channel_fade_set(3, 127 - sp3C);
                } else {
                    music_channel_off(10);
                    music_channel_off(11);
                    music_channel_off(15);
                }
            }
            switch (taj->musicLayerState) {
                case 0:
                    if (taj->musicFade > updateRate << 7) {
                        taj->musicFade -= updateRate << 7;
                        music_channel_on(14);
                        music_channel_fade_set(14, taj->musicFade >> 8);
                        taj->musicLayerDelay = 0;
                    } else {
                        taj->musicFade = 0;
                        music_channel_off(14);
                        if (taj->musicLayerDelay == 0) {
                            taj->musicLayerDelay = rand_range(600, 900);
                            taj->musicLayerTimer = 0;
                        }
                    }
                    if ((taj->musicLayerDelay != 0) && ((music_channel_get_mask() & ~0x4000) == 0xB)) {
                        taj->musicLayerTimer += updateRate;
                        if (taj->musicLayerDelay < taj->musicLayerTimer) {
                            taj->musicLayerState = 1;
                            taj->musicLayerTimer = rand_range(600, 900);
                        }
                    } else {
                        taj->musicLayerTimer = 0;
                        taj->musicLayerDelay = 0;
                    }
                    break;
                case 1:
                    if ((music_channel_get_mask() & ~0x4000) == 0xB) {
                        taj->musicFade += (updateRate * 128);
                        if (taj->musicFade > 0x7F00) {
                            taj->musicFade = 0x7F00;
                        }
                        taj->musicLayerTimer -= updateRate;
                        if (taj->musicLayerTimer < 0) {
                            taj->musicLayerState = 0;
                        }
                        music_channel_on(14);
                        music_channel_fade_set(14, taj->musicFade >> 8);
                    } else {
                        taj->musicLayerState = 0;
                        taj->musicLayerTimer = 0;
                        taj->musicLayerDelay = 0;
                    }
                    break;
            }
            taj->prevMusicChannelMask = music_channel_get_mask() & 0xBFFF;
            break;
    }
    obj->trans.y_position = tempPosY;
    var_a2 = get_level_segment_waves(obj->segmentID, obj->trans.x_position, obj->trans.z_position, &water);
    if (var_a2 != 0) {
        var_a2--;
        while (var_a2 >= 0) {
            if ((water[var_a2]->type != SURFACE_WATER_CALM) && (water[var_a2]->type != SURFACE_WATER_WAVY) &&
                (water[var_a2]->rot.y > 0.0)) {
                obj->trans.y_position = water[var_a2]->waveHeight;
            }
            var_a2--;
        }
    }
    obj->trans.rotation.x_rotation = 0;
    obj->trans.rotation.z_rotation = 0;
    if (obj->properties.taj.action != TAJ_MODE_ROAM) {
        gNPCPosY = obj->trans.y_position;
    }
    if (spawnSmoke) {
        obj_spawn_effect(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                         ASSET_OBJECT_ID_WARDENSMOKE, SOUND_NONE, 1.0f, 0);
    }
    obj->animFrame = taj->animFrameF * 1.0;
    object_update_model_animation(obj);
    obj_spawn_particle(obj, updateRate);
}

/**
 * If Taj is currently talking, clear the audio associated with gTajSoundMask,
 * then play the new sound ID in its place.
 */
void play_taj_voice_clip(u16 soundID, s32 interrupt) {
    if (gTajSoundMask && interrupt & 1) {
        sndp_stop(gTajSoundMask);
        gTajSoundMask = NULL;
    }
    if (!gTajSoundMask) {
        sound_play(soundID, &gTajSoundMask);
    }
}

void obj_loop_gbparkwarden(UNUSED Object *obj, UNUSED s32 updateRate) {
}

/**
 * Return the Y pos of an NPC when they're in dialogue.
 */
f32 get_npc_pos_y(void) {
    return gNPCPosY;
}
