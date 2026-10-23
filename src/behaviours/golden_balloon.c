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
 * Golden Balloon init func.
 * Assigns the balloon ID either from the entry, or from the balloon ID table.
 */
void obj_init_goldenballoon(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_GoldenBalloon *entry = &levelEntry->goldenBalloon;
    Object_NPC *obj64;
    f32 scalef;

    if (entry->balloonID == -1) {
        entry->balloonID = object_id_alloc(obj);
    } else {
        object_id_assign(obj, entry->balloonID);
    }
    if (entry->balloonID == -1) {
        pc_log("Illegal door no!!!\n"); // Did the devs just copy-paste the door init function?
    }
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 4;
    obj->interactObj->hitboxRadius = 20;
    obj->interactObj->pushForce = 0;
    scalef = entry->scale & 0xFF;
    if (scalef < 10.0f) {
        scalef = 10.0f;
    }
    scalef /= 64;
    obj->trans.scale = obj->header->scale * scalef;
    obj64 = obj->npc;
    obj64->nodeCurrent = 255;
    obj64->splineProgress = 0.0f;
    obj->properties.goldenBalloon.action = 0;
    if (entry->challengeID) {
        if (get_settings()->tajFlags & (1 << (entry->challengeID + 2))) {
            obj->properties.goldenBalloon.action = 0;
        } else {
            obj->properties.goldenBalloon.action = 1;
        }
    }
}

/**
 * Golden Balloon loop func.
 * Made invisible every frame, but made visible during behaviour if applicable.
 * Follows a path, like Taj and T.T, until collected.
 * Auto deletes if the ID is already claimed by the save flags.
 */
void obj_loop_goldenballoon(Object *obj, s32 updateRate) {
    LevelObjectEntry *levelEntry;
    ObjectInteraction *interactObj;
    Object_Racer *racer;
    Object_NPC *npc;
    Settings *settings;
    s32 flag;
    s32 doubleSpeed;
    Object *racerObj;
    f32 updateRateF;
    f32 speedf;
    s32 isPirated;

    updateRateF = updateRate;
    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }
    isPirated = FALSE;
    speedf = updateRateF;
    settings = get_settings();
    levelEntry = obj->level_entry;
    flag = 0x10000 << levelEntry->goldenBalloon.balloonID;
    if (settings->courseFlagsPtr[settings->courseId] & flag) {
        if (obj->properties.goldenBalloon.timer > 0) {
            obj->particleEmittersEnabled = OBJ_EMIT_2;
            obj_spawn_particle(obj, updateRate);
            obj->properties.goldenBalloon.timer -= updateRate;
        } else {
            free_object(obj);
        }
    } else {
        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
        if (obj->properties.goldenBalloon.action == 0) {
            obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
            doubleSpeed = updateRate * 2;
            if (obj->opacity < 255 - doubleSpeed) {
                obj->opacity += doubleSpeed;
            } else {
                obj->opacity = 255;
            }
            interactObj = obj->interactObj;
            if (interactObj->distance < 45 && isPirated == FALSE) {
                racerObj = interactObj->obj;
                if ((racerObj && (racerObj->header->behaviorId == 1))) {
                    racer = racerObj->racer;
                    if (racer->playerIndex == PLAYER_ONE) {
                        settings->balloonsPtr[settings->worldId]++;
                        if (isPirated == 1) {} // Fakematch
                        if (settings->worldId != WORLD_CENTRAL_AREA) {
                            settings->balloonsPtr[0]++;
                        }
                        settings->courseFlagsPtr[settings->courseId] |= flag;
                        if (1) {} // Fakematch
                        sound_play_spatial(SOUND_COLLECT_BALLOON, obj->trans.x_position, obj->trans.y_position,
                                           obj->trans.z_position, NULL);
                        obj->properties.goldenBalloon.timer = 16;
                        obj->particleEmittersEnabled = OBJ_EMIT_2;
                        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
                        obj_spawn_particle(obj, updateRate);
                    }
                }
            }
            npc = obj->npc;
            obj->animationID = 0;
            npc->forwardVel = 0.0f;
            speedf = (obj->opacity < 255) ? 0 : 1;
            if (npc->nodeCurrent == 255) {
                npc->nodeCurrent =
                    ainode_find_nearest(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position, FALSE);
                if (npc->nodeCurrent != 255) {
                    npc->nodeBack2 = ainode_find_next(npc->nodeCurrent, -1, 0);
                    npc->nodeForward1 = ainode_find_next(npc->nodeBack2, npc->nodeCurrent, 0);
                    npc->nodeForward2 = ainode_find_next(npc->nodeForward1, npc->nodeBack2, 0);
                    npc->nodeBack1 = npc->nodeCurrent;
                }
            } else {
                npc_follow_ai_nodes(npc, obj, updateRateF, speedf, 0);
            }
        }
    }
}
