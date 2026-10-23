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

u8 gCharacterSelectMainActorIDs[8] = { 2, 5, 6, 3, 10, 8, 7, 9 };
u8 gCharacterSelectMainActorIDsPlusDrumstick[9] = { 2, 5, 6, 3, 10, 8, 7, 9, 11 };
u8 gCharacterSelectMainActorIDsPlusTT[9] = { 2, 5, 6, 3, 10, 8, 7, 9, 12 };
u8 gCharacterSelectAllActorIDs[10] = { 2, 5, 6, 3, 10, 8, 7, 9, 11, 12 };
u8 gSignHoldingFrames[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

u8 gSignHoldingIndices[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/**
 * Character select init behaviour.
 * Does nothing. Not even called.
 */
UNUSED void obj_init_char_select(UNUSED s32 arg0, UNUSED s32 arg1) {
}

/**
 * Character select loop behaviour.
 * Animates characters dancing, chooses which player sign they're holding up etc.
 */
void obj_loop_char_select(Object *charSelectObj, s32 updateRate) {
    s32 i2;
    s32 i;
    f32 animFrame;
    ObjectModel *objMdl;
    Object_AnimatedObject *charSelect;
    s32 charCount;
    s32 playerIndex;
    s8 *status = NULL;
    ModelInstance *modInst;
    u8 playerSelectIndices[4];
    u8 numCursors;
    u8 *actorIDs;

    playerIndex = 0;
    anim_object_follow_path(charSelectObj, updateRate, NULL);
    charSelect = charSelectObj->animatedObject;
    charSelectObj->particleEmittersEnabled = OBJ_EMIT_NONE;
    if (charSelect != NULL) {
        modInst = charSelectObj->modelInstances[charSelectObj->modelIndex];
        if (modInst != NULL) {
            objMdl = modInst->objModel;
            if (is_drumstick_unlocked()) {
                if (is_tt_unlocked()) {
                    actorIDs = gCharacterSelectAllActorIDs;
                    charCount = 10;
                } else {
                    actorIDs = gCharacterSelectMainActorIDsPlusDrumstick;
                    charCount = 9;
                }
            } else if (is_tt_unlocked()) {
                actorIDs = gCharacterSelectMainActorIDsPlusTT;
                charCount = 9;
            } else {
                actorIDs = gCharacterSelectMainActorIDs;
                charCount = 8;
            }

            for (i = 0; i < charCount && !playerIndex; i++) {
                if (charSelect->actorIndex == actorIDs[i]) {
                    playerIndex = 1;
                }
            }
            i--;
            if (playerIndex) {
                charSelectObj->animationID = 1;
                for (playerIndex = 0, numCursors = 0; playerIndex < MAXCONTROLLERS; playerIndex++) {
                    if (get_player_character(playerIndex) == i) {
                        playerSelectIndices[numCursors++] = playerIndex;
                    }
                }
                gSignHoldingFrames[i] += updateRate;
                if (gSignHoldingFrames[i] >= 16) {
                    gSignHoldingFrames[i] &= 0xF;
                    gSignHoldingIndices[i]++;
                }
                if (gSignHoldingIndices[i] >= numCursors) {
                    gSignHoldingIndices[i] = 0;
                }
                if (numCursors > 0) {
                    status = charselect_status();
                    for (i2 = 0; i2 < numCursors; i2++) {
                        if (status[playerSelectIndices[i2]] == CHARSELECT_STATUS_CONFIRMED) {
                            // Emit particle effects for every player confirming their selection of this character.
                            charSelectObj->particleEmittersEnabled = OBJ_EMIT_1;
                            obj_spawn_particle(charSelectObj, LOGIC_30FPS);
                        }
                    }

                    for (i2 = 0; i2 < objMdl->numberOfBatches; i2++) {
                        // Unneccessary check for textureIndex to be greater than or equal to zero since it's a u8 and
                        // can't be less.
                        if (objMdl->batches[i2].textureIndex >= 0 && objMdl->batches[i2].textureIndex < 4) {
                            objMdl->batches[i2].textureIndex = playerSelectIndices[gSignHoldingIndices[i]];
                        }
                    }
                    charSelectObj->animationID = 0;
                }
            }
            if (charSelectObj->animationID >= 0) {
                if ((charSelectObj->animationID < objMdl->numberOfAnimations)) {
                    i = (objMdl->animations[charSelectObj->animationID].animLength - 1) << 4;
                    if (charSelect->loopType == 1) {
                        animFrame = music_animation_fraction();
                        if (animFrame > 0.5) {
                            animFrame -= 0.5;
                            animFrame *= 2.0;
                            charSelectObj->animFrame = i - (animFrame * i);
                        } else {
                            animFrame *= 2;
                            charSelectObj->animFrame = animFrame * i;
                        }
                    }
                }
            }
        }
    }
}
