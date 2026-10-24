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
 * Walks towards the player until within 10 units, then turns to face them.
 * Switches to the dialogue camera once within 100 units.
 */
static void taj_approach_player(Object *obj, Object_NPC *taj, f32 distance, f32 xPosDiff, f32 zPosDiff,
                                f32 updateRateF) {
    f32 targetSpeed;
    s32 arctan;

    obj->animationID = 0;
    taj->nodeCurrent = 0xFF;
    if (distance < 100.0) {
        racer_set_dialogue_camera();
    }
    if (distance > 10.0) {
        arctan =
            (arctan2_f(xPosDiff / distance, zPosDiff / distance) - (obj->trans.rotation.y_rotation & 0xFFFF)) + 0x8000;
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
        targetSpeed = -2.0f;
        if (arctan > 0x800 || arctan < -0x800) {
            targetSpeed = -0.5f;
        }
        taj->forwardVel += (targetSpeed - taj->forwardVel) * 0.125;
        obj->x_velocity = sins_f(obj->trans.rotation.y_rotation) * taj->forwardVel;
        obj->z_velocity = coss_f(obj->trans.rotation.y_rotation) * taj->forwardVel;
        taj->animFrameF -= taj->forwardVel * 2 * updateRateF;
    } else {
        obj->properties.taj.action = TAJ_MODE_TURN_TOWARDS_PLAYER;
    }
    move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF, obj->z_velocity * updateRateF);
}

/**
 * Turns to face the player while sliding into place in front of them,
 * then says hello and greets them once facing them up close.
 */
static void taj_turn_towards_player(Object *obj, Object_NPC *taj, Object *racerObj, f32 distance, f32 xPosDiff,
                                    f32 zPosDiff, f32 updateRateF) {
    s32 arctan;

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
    move_object(obj, obj->x_velocity * updateRateF, obj->y_velocity * updateRateF, obj->z_velocity * updateRateF);
}

/**
 * Plays the greeting animation while facing the player, then starts the dialogue.
 */
static void taj_greet_player(Object *obj, Object_NPC *taj, Object *racerObj, f32 updateRateF) {
    s32 arctan;

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
}

/**
 * Talking: waits on the dialogue menu's result. Leaving says goodbye and restores the fog and level music.
 * Choosing another vehicle starts the transformation, and choosing a challenge either transforms the player
 * first or starts the challenge straight away.
 */
static void taj_dialogue(Object *obj, Object_NPC *taj, Object_Racer *racer, LevelHeader *levelHeader, s32 dialogueID,
                         f32 updateRateF, s8 *spawnSmoke) {
    obj->animationID = 4;
    taj->animFrameF += updateRateF * 1.0;
    racer_set_dialogue_camera();
    if (dialogueID == 3 || dialogueID == 4) {
        obj->properties.taj.action = (dialogueID == 4) ? TAJ_MODE_END_DIALOGUE_CHALLENGE : TAJ_MODE_END_DIALOGUE;
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
            *spawnSmoke = TRUE;
            play_taj_voice_clip(SOUND_WHOOSH4, TRUE);
            taj->animFrameF = 0.0f;
        }
    }
}

/**
 * Casts the transformation spell: sparkles mid animation, then fades the player out and
 * swaps them into the chosen vehicle.
 */
static void taj_transform_begin(Object *obj, Object_NPC *taj, Object *racerObj, Object_Racer *racer, s32 updateRate,
                                f32 updateRateF) {
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
}

/**
 * Fades the player back in after transforming, then starts the chosen challenge or returns to the dialogue.
 */
static void taj_transform_end(Object *obj, Object_NPC *taj, Object *racerObj, Object_Racer *racer, s32 updateRate,
                              s8 *spawnSmoke) {
    s32 fadeStep;

    racer_set_dialogue_camera();
    if (racerObj != NULL) {
        if (taj->animFrameF != 0.0f) {
            taj->animFrameF += 8.0;
        }
        fadeStep = updateRate;
        if (fadeStep > 4) {
            fadeStep = 4;
        }
        fadeStep <<= 5;
        if (racer->transparency < (255 - (fadeStep))) {
            racer->transparency += fadeStep;
        } else {
            racer->transparency = 255;
            if (taj->animFrameF == 0.0) {
                if (gTajDialogueChoice & 0x80) {
                    transition_begin(&gTajTransition);
                    *spawnSmoke = TRUE;
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
}

/**
 * Finishes the goodbye animation, then vanishes in a puff of smoke, leaving a challenge balloon
 * behind if a challenge was accepted, and restores the player's engine sound.
 */
static void taj_end_dialogue(Object *obj, Object_NPC *taj, Object_Racer *racer, f32 updateRateF, s8 *spawnSmoke) {
    if (taj->animFrameF != 0.0) {
        taj->animFrameF = taj->animFrameF + (0.5 * updateRateF);
    }
    if (taj->animFrameF == 0) {
        *spawnSmoke = TRUE;
        if (obj->properties.taj.action == TAJ_MODE_END_DIALOGUE_CHALLENGE) {
            obj_taj_create_balloon(obj->segmentID, obj->trans.x_position - (racer->ox1 * 50.0f), obj->trans.y_position,
                                   obj->trans.z_position - (racer->oz1 * 50.0f));
        }
        obj->properties.taj.action = TAJ_MODE_TELEPORT_AWAY_BEGIN;
        sound_play(SOUND_WHOOSH4, NULL);
        racer->vehicleSound = racer_sound_init(racer->characterId, racer->vehicleID);
    }
}

/**
 * Fades out with the teleport animation, then reappears 150 units in front of the player, facing them.
 */
static void taj_teleport_to_player_begin(Object *obj, Object_NPC *taj, Object *racerObj, Object_Racer *racer,
                                         s32 updateRate, f32 updateRateF, s8 *spawnSmoke) {
    s32 fadeStep;

    obj->animationID = 3;
    taj->nodeCurrent = 0xFF;
    taj->forwardVel = 0.0f;
    taj->animFrameF += updateRateF * 2.0;
    if (taj->animFrameF > 79.0f) {
        taj->animFrameF = 79.0f;
    }

    fadeStep = updateRate * 8;
    if (taj->animFrameF < 20.0f) {
        fadeStep = 0;
    }
    if (obj->opacity > fadeStep) {
        obj->opacity -= fadeStep;
    } else {
        sound_play(SOUND_WHOOSH4, NULL);
        *spawnSmoke = TRUE;
        obj->opacity = 0;
        obj->properties.taj.action = TAJ_MODE_TELEPORT_TO_PLAYER_END;
        obj->trans.x_position = racerObj->trans.x_position - (racer->ox1 * 150.0f);
        obj->trans.z_position = racerObj->trans.z_position - (racer->oz1 * 150.0f);
        obj->segmentID =
            get_level_segment_index_from_position(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
        obj->trans.rotation.y_rotation = racerObj->trans.rotation.y_rotation + 0x8000;
    }
}

/**
 * Fades back in after teleporting to the player, then walks up to them.
 */
static void taj_teleport_to_player_end(Object *obj, Object_NPC *taj, s32 updateRate, f32 updateRateF) {
    s32 fadeStep;

    obj->animationID = 3;
    taj->animFrameF -= updateRateF * 2.0;
    if (taj->animFrameF < 0.0f) {
        taj->animFrameF = 0.0f;
    }
    fadeStep = updateRate * 4;
    if (obj->opacity < (255 - fadeStep)) {
        obj->opacity += fadeStep;
    } else {
        obj->opacity = 255;
        obj->properties.taj.action = TAJ_MODE_APPROACH_PLAYER;
    }
}

/**
 * Fades out, then starts the challenge race: restores the player's engine sound, the fog and level music,
 * and moves to the telepoint furthest away to race from.
 */
static void taj_set_challenge(Object *obj, Object_NPC *taj, Object *racerObj, Object_Racer *racer,
                              LevelHeader *levelHeader, s32 updateRate, f32 updateRateF) {
    Object *telepoint;
    s32 fadeStep;

    obj->animationID = 3;
    taj->nodeCurrent = 0xFF;
    taj->forwardVel = 0.0f;
    taj->animFrameF += updateRateF * 2.0;
    if (taj->animFrameF > 79.0f) {
        taj->animFrameF = 79.0f;
    }
    fadeStep = updateRate * 8;
    if (taj->animFrameF < 20.0f) {
        fadeStep = 0;
    }
    if (obj->opacity > fadeStep) {
        obj->opacity -= fadeStep;
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
}

/**
 * Fades out, then moves to the telepoint furthest away.
 */
static void taj_teleport_away_begin(Object *obj, Object_NPC *taj, Object *racerObj, s32 updateRate, f32 updateRateF) {
    Object *telepoint;
    s32 fadeStep;

    obj->animationID = 3;
    taj->nodeCurrent = 0xFF;
    taj->forwardVel = 0.0f;
    taj->animFrameF += updateRateF * 2.0;
    if (taj->animFrameF > 79.0f) {
        taj->animFrameF = 79.0f;
    }
    fadeStep = updateRate * 8;
    if (taj->animFrameF < 20.0f) {
        fadeStep = 0;
    }
    if (obj->opacity > fadeStep) {
        obj->opacity -= fadeStep;
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
}

/**
 * Fades back in after teleporting away, then goes back to roaming.
 */
static void taj_teleport_away_end(Object *obj, Object_NPC *taj, s32 updateRate, f32 updateRateF) {
    s32 fadeStep;

    obj->animationID = 3;
    taj->animFrameF -= updateRateF * 2.0;
    if (taj->animFrameF < 0.0f) {
        taj->animFrameF = 0.0f;
    }
    fadeStep = updateRate * 4;
    if (obj->opacity < (255 - fadeStep)) {
        obj->opacity += fadeStep;
    } else {
        obj->opacity = 255;
        obj->properties.taj.action = TAJ_MODE_ROAM;
    }
}

/**
 * Roaming: finds the nearest AI node to start from, then follows the path. When the player comes within
 * 55 units, turns to face them for a couple of seconds before carrying on.
 */
static void taj_roam_path(Object *obj, Object_NPC *taj, Object *racerObj, f32 distance, f32 xPosDiff, f32 zPosDiff,
                          s32 updateRate, f32 updateRateF, f32 updateRateF2) {
    s32 turnAngle;

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
            turnAngle = taj->faceRacerAngle - (obj->trans.rotation.y_rotation & 0xFFFF);
            if (turnAngle > 0x8000) {
                turnAngle -= 0xFFFF;
            }
            if (turnAngle < -0x8000) {
                turnAngle += 0xFFFF;
            }
            obj->trans.rotation.y_rotation += ((turnAngle * updateRate) >> 4);
            xPosDiff = sins_f(obj->trans.rotation.y_rotation + 0x8000);
            zPosDiff = coss_f(obj->trans.rotation.y_rotation + 0x8000);
            move_object(obj, (updateRateF2 * xPosDiff) * 1.1, 0.0f, (updateRateF2 * zPosDiff) * 1.1);
            taj->animFrameF += updateRate * 2.2;
        }
    }
}

/**
 * Roaming: within 1000 units of the player, fades in Taj's music channels, louder the closer he is and panned
 * to where he is on screen, and fades the main melody down to match. Further away they are turned off.
 */
static void taj_roam_music_proximity(Object *obj) {
    f32 xPosDiff;
    f32 zPosDiff;
    f32 yPosDiff;
    f32 racerDist;
    Object **racerObjs;
    s32 numRacers;
    s32 tajVolume;
    Camera *camera;
    s32 arctan;
    s32 musicPan;

    racerObjs = get_racer_objects(&numRacers);
    if (racerObjs[PLAYER_ONE] != NULL) {
        xPosDiff = racerObjs[PLAYER_ONE]->trans.x_position - obj->trans.x_position;
        yPosDiff = racerObjs[PLAYER_ONE]->trans.y_position - obj->trans.y_position;
        zPosDiff = racerObjs[PLAYER_ONE]->trans.z_position - obj->trans.z_position;
        racerDist = sqrtf((xPosDiff * xPosDiff) + (yPosDiff * yPosDiff) + (zPosDiff * zPosDiff));
        if (racerDist < 1000.0f) {
            racerDist = 1000.0f - racerDist;
            tajVolume = (127.0f * racerDist) / 1000.0f;
            camera = cam_get_cameras();
            xPosDiff = obj->trans.x_position - camera->trans.x_position;
            zPosDiff = obj->trans.z_position - camera->trans.z_position;
            arctan = audspat_calculate_spatial_pan(xPosDiff, zPosDiff, camera->trans.rotation.y_rotation);
            musicPan = arctan;
            music_channel_fade_set(10, tajVolume);
            music_channel_fade_set(11, tajVolume);
            music_channel_fade_set(15, tajVolume);
            music_channel_pan_set(10, musicPan);
            music_channel_pan_set(11, musicPan);
            music_channel_pan_set(15, musicPan);
            music_channel_on(10);
            music_channel_on(11);
            music_channel_on(15);
            music_channel_fade_set(3, 127 - tajVolume);
        } else {
            music_channel_off(10);
            music_channel_off(11);
            music_channel_off(15);
        }
    }
}

/**
 * Roaming: every 10 to 15 seconds while the music plays only its base channels, fades in an extra layer
 * on channel 14 for 10 to 15 seconds, then fades it back out.
 */
static void taj_roam_music_layer(Object_NPC *taj, s32 updateRate) {
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
}

/**
 * Lifts Taj onto the surface of any water he stands in, other than calm or wavy water.
 */
static void taj_float_on_water(Object *obj) {
    struct WaterProperties **water;
    s32 waterIndex;

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
 * Returns TRUE when the player, within 300 units and facing Taj, bumps into him or honks the horn (Z).
 */
static s32 taj_summoned_by_player(Object *obj, Object *racerObj, Object_Racer *racer, f32 distance) {
    u32 buttonsPressed;
    s32 arctan;
    s32 summoned;

    buttonsPressed = input_pressed(PLAYER_ONE);
    summoned = FALSE;
    if ((obj->properties.taj.action == TAJ_MODE_ROAM) && (distance < 300.0) &&
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
        summoned = FALSE;
        if (arctan > -0x2000 && arctan < 0x2000) {
            summoned = TRUE;
        }
    }
    return summoned;
}

/**
 * Starts an encounter: plays the entranced music, silences the player's engine, dims the fog,
 * and either walks up to the player (when summoned) or teleports to them.
 */
static void taj_begin_encounter(Object *obj, Object_NPC *taj, Object *racerObj, Object_Racer *racer, s32 summoned,
                                s8 *spawnSmoke) {
    s32 arctan;

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
    if (summoned) {
        obj->properties.taj.action = TAJ_MODE_APPROACH_PLAYER;
    } else {
        obj->properties.taj.action = TAJ_MODE_TELEPORT_TO_PLAYER_BEGIN;
        *spawnSmoke = TRUE;
    }
    get_fog_settings(PLAYER_ONE, &taj->fogNear, &taj->fogFar, &taj->fogR, &taj->fogG, &taj->fogB);
    slowly_change_fog(PLAYER_ONE, 255, 0, 120, 960, 1100, 240);
    taj->animFrameF = 0.0f;
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
    Object *racerObj;
    s32 summoned;
    s32 dialogueID;
    Object_NPC *taj;
    Object_Racer *racer;
    s8 spawnSmoke;
    LevelHeader *levelHeader;

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
    summoned = taj_summoned_by_player(obj, racerObj, racer, distance);
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    if ((should_taj_teleport() || summoned) &&
        (obj->properties.taj.action == TAJ_MODE_ROAM || obj->properties.taj.action == TAJ_MODE_UNK1F)) {
        taj_begin_encounter(obj, taj, racerObj, racer, summoned, &spawnSmoke);
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
    if (obj->properties.taj.action != TAJ_MODE_ROAM && dialogueID != 0 &&
        obj->properties.taj.action < TAJ_MODE_DIALOGUE) {
        obj->properties.taj.action = TAJ_MODE_DIALOGUE;
    }
    switch (obj->properties.taj.action) {
        case TAJ_MODE_APPROACH_PLAYER:
            taj_approach_player(obj, taj, distance, xPosDiff, zPosDiff, updateRateF);
            break;
        case TAJ_MODE_TURN_TOWARDS_PLAYER:
            taj_turn_towards_player(obj, taj, racerObj, distance, xPosDiff, zPosDiff, updateRateF);
            break;
        case TAJ_MODE_GREET_PLAYER:
            taj_greet_player(obj, taj, racerObj, updateRateF);
            break;
        case TAJ_MODE_DIALOGUE:
            taj_dialogue(obj, taj, racer, levelHeader, dialogueID, updateRateF, &spawnSmoke);
            break;
        case TAJ_MODE_TRANSFORM_BEGIN:
            taj_transform_begin(obj, taj, racerObj, racer, updateRate, updateRateF);
            break;
        case TAJ_MODE_TRANSFORM_END:
            taj_transform_end(obj, taj, racerObj, racer, updateRate, &spawnSmoke);
            break;
        case TAJ_MODE_END_DIALOGUE:
        case TAJ_MODE_END_DIALOGUE_CHALLENGE:
            taj_end_dialogue(obj, taj, racer, updateRateF, &spawnSmoke);
            break;
        case TAJ_MODE_TELEPORT_TO_PLAYER_BEGIN:
            taj_teleport_to_player_begin(obj, taj, racerObj, racer, updateRate, updateRateF, &spawnSmoke);
            break;
        case TAJ_MODE_TELEPORT_TO_PLAYER_END:
            taj_teleport_to_player_end(obj, taj, updateRate, updateRateF);
            break;
        case TAJ_MODE_SET_CHALLENGE:
            taj_set_challenge(obj, taj, racerObj, racer, levelHeader, updateRate, updateRateF);
            break;
        case TAJ_MODE_TELEPORT_AWAY_BEGIN:
            taj_teleport_away_begin(obj, taj, racerObj, updateRate, updateRateF);
            break;
        case TAJ_MODE_TELEPORT_AWAY_END:
            taj_teleport_away_end(obj, taj, updateRate, updateRateF);
            break;
        case TAJ_MODE_RACE:
            obj->interactObj->flags = INTERACT_FLAGS_NONE;
            obj->animationID = 6;
            obj->opacity = 255;
            taj->animFrameF += updateRateF * 1.0;
            break;
        default:
            taj_roam_path(obj, taj, racerObj, distance, xPosDiff, zPosDiff, updateRate, updateRateF, updateRateF2);
            taj_roam_music_proximity(obj);
            taj_roam_music_layer(taj, updateRate);
            taj->prevMusicChannelMask = music_channel_get_mask() & 0xBFFF;
            break;
    }
    obj->trans.y_position = tempPosY;
    taj_float_on_water(obj);
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
