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
 * Race silver coin init behaviour.
 * Checks if in the correct modes, then sets the flag to active, otherwise inactive.
 * If the flag is inactive, destroy the object.
 * Rareware duplicated this function just to check for adventure 2...
 */
void obj_init_silvercoin_adv2(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 30;
    obj->properties.silverCoin.action = SILVER_COIN_INACTIVE;
    obj->properties.silverCoin.timer = 16;
    if (!is_in_tracks_mode()) {
        if (check_if_silver_coin_race() && is_in_adventure_two()) {
            obj->properties.silverCoin.action = SILVER_COIN_ACTIVE;
        } else {
            obj->properties.silverCoin.action = SILVER_COIN_INACTIVE;
        }
    }
    if (obj->properties.silverCoin.action == SILVER_COIN_INACTIVE) {
        obj->trans.flags |= OBJ_FLAGS_INVIS_PLAYER1 | OBJ_FLAGS_INVIS_PLAYER2;
        free_object(obj);
    }
}

/**
 * Race silver coin init behaviour.
 * Checks if in the correct modes, then sets the flag to active, otherwise inactive.
 * If the flag is inactive, destroy the object.
 */
void obj_init_silvercoin(Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = 30;
    obj->properties.silverCoin.action = SILVER_COIN_INACTIVE;
    obj->properties.silverCoin.timer = 0;
    if (!is_in_tracks_mode()) {
        if (check_if_silver_coin_race() && !is_in_adventure_two()) {
            obj->properties.silverCoin.action = SILVER_COIN_ACTIVE;
        } else {
            obj->properties.silverCoin.action = SILVER_COIN_INACTIVE;
        }
    }
    if (obj->properties.silverCoin.action == SILVER_COIN_INACTIVE) {
        obj->trans.flags |= OBJ_FLAGS_INVIS_PLAYER2 | OBJ_FLAGS_INVIS_PLAYER1;
        free_object(obj);
    }
}

/**
 * Race silver coin loop behaviour.
 * Can only be collected by players. Sets a flag equal to the player number when collected.
 * Under normal gameplay, that's 1 and 2, since the silver coin challenge can be done with at most two players.
 * When collected, the coins are hidden for that player, rather than being removed outright.
 */
void obj_loop_silvercoin(Object *obj, s32 updateRate) {
    ObjectInteraction *interactObj;
    Object_Racer *racer;
    Object *racerObj;
    s32 twoPlayerAdv;

    twoPlayerAdv = race_is_adventure_2P();
    if ((twoPlayerAdv && obj->properties.silverCoin.action != SILVER_COIN_INACTIVE) ||
        (!twoPlayerAdv && obj->properties.silverCoin.action == SILVER_COIN_ACTIVE)) {
        interactObj = obj->interactObj;
        if (interactObj->distance < 80) {
            racerObj = interactObj->obj;
            if (racerObj != NULL && racerObj->header->behaviorId == BHV_RACER) {
                racer = racerObj->racer;
                if (racer->playerIndex != PLAYER_COMPUTER) {
                    if (racer->raceFinished == FALSE &&
                        !(obj->properties.silverCoin.action & (SILVER_COIN_COLLECTED << racer->playerIndex))) {
                        obj->properties.silverCoin.action |= SILVER_COIN_COLLECTED << racer->playerIndex;
                        obj->properties.silverCoin.timer = 16;
                        obj->trans.flags |= OBJ_FLAGS_INVIS_PLAYER1 << racer->playerIndex;
                        music_jingle_play(SEQUENCE_SILVER_COIN_1 + racer->silverCoinCount);
                        racer->silverCoinCount++;
                    }
                }
            }
        }
        obj->animFrame += 8 * updateRate;
    }
    if (obj->properties.silverCoin.timer > 0) {
        obj->properties.silverCoin.timer -= updateRate;
        obj->particleEmittersEnabled = OBJ_EMIT_1;
        obj_spawn_particle(obj, updateRate);
    }
}
