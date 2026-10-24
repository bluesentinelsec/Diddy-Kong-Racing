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
 * Trophy Cabinet loop behaviour.
 * Sets rotation and hitbox data so it can be collided with.
 */
void obj_init_trophycab(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_TrophyCab *entry = &levelEntry->trophyCabinet;
    obj->interactObj->flags = INTERACT_FLAGS_SOLID;
    obj->interactObj->hitboxKind = 2;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
}

/**
 * Trophy Cabinet loop behaviour.
 * Displays the dialogue when ran into.
 * Has unused behaviour that presumably would've handled activating the dialogue from here.
 */
void obj_loop_trophycab(Object *obj, s32 updateRate) {
    Settings *settings;
    JingleState *jingle_state;
    UNUSED f32 dist;
    LevelObjectEntryCommon newObject;
    LevelHeader *header;
    Object *tempObj;
    f32 diffX;
    s32 dialogueID;
    f32 diffZ;
    s32 worldBalloons;
    s32 bossFlags;

    settings = get_settings();
    header = level_header();
    jingle_state = obj->jingle_state;
    if (obj->properties.trophyCabinet.trophy == FALSE) {
        if (header->race_type != RACETYPE_CUTSCENE_2 && header->race_type != RACETYPE_CUTSCENE_1) {
            obj->properties.trophyCabinet.trophy = TRUE;
            if ((settings->trophies >> (((settings->worldId - 1) ^ 0) * 2)) & 3) { // Fakematch
                newObject.objectID = ASSET_OBJECT_ID_TROPHY;
                newObject.x = obj->level_entry->animation.common.x;
                newObject.y = obj->level_entry->animation.common.y;
                newObject.z = obj->level_entry->animation.common.z;
                newObject.size = sizeof(LevelObjectEntryCommon);
                tempObj = spawn_object(&newObject, OBJECT_SPAWN_UNK01);
                if (tempObj != NULL) {
                    tempObj->level_entry = NULL;
                    tempObj->trans.rotation.y_rotation = obj->trans.rotation.y_rotation;
                }
            }
        }
    }
    obj->shading->diffuse = 0.612f;
    obj->shading->ambient = 0.0f;
    tempObj = get_racer_object(PLAYER_ONE);
    if (tempObj != NULL) {
        diffX = obj->trans.x_position - tempObj->trans.x_position;
        diffZ = obj->trans.z_position - tempObj->trans.z_position;
        dist = sqrtf((diffX * diffX) + (diffZ * diffZ)); // unused
        bossFlags = settings->bosses;
        bossFlags |= 0x800;
        worldBalloons = (settings->balloonsPtr[settings->worldId] >= 8);
        if (worldBalloons) {
            worldBalloons = ((1 << (settings->worldId + 6)) & bossFlags) != 0;
        }
        if (obj->properties.trophyCabinet.action == NULL && textbox_visible() == FALSE) {
            if (obj->collisionData->collidedObj != NULL) {
                if (jingle_state->cooldown == 0) {
                    if (worldBalloons) {
                        obj->properties.trophyCabinet.action = 1;
                        sound_play(SOUND_VOICE_TT_TROPHY_RACE, NULL);
                        hud_speedometre_reset();
                    } else {
                        // Text for "TROPHY RACE" "TO ENTER THE TROPHY RACE, YOU MUST COMPLETE ALL THE TASKS FROM THIS
                        // WORLD. KEEP RACING!"
                        set_current_text(ASSET_GAME_TEXT_4);
                        jingle_state->cooldown = 180;
                        jingle_state->timer = 140;
                        music_jingle_voicelimit_set(16);
                        music_fade(-8);
                        music_jingle_play(SEQUENCE_NO_TROPHY_FOR_YOU);
                    }
                }
            }
        }
        if (jingle_state->timer && music_jingle_playing() == SEQUENCE_NONE) {
            if (updateRate < jingle_state->timer) {
                jingle_state->timer -= updateRate;
            } else {
                jingle_state->timer = 0;
                music_fade(8);
                music_jingle_voicelimit_set(6);
            }
        }
        if (obj->collisionData->collidedObj != NULL || textbox_visible()) {
            jingle_state->cooldown = 180;
        }
        if (jingle_state->cooldown > 0) {
            jingle_state->cooldown -= updateRate;
        } else {
            jingle_state->cooldown = 0;
        }
        if (obj->properties.trophyCabinet.action == 1) {
            minimap_opacity_set(3);
            hud_visibility(0);
            dialogueID = npc_dialogue_loop(DIALOGUE_TROPHY);
            if (dialogueID) {
                obj->properties.trophyCabinet.action = 0;
                dialogue_npc_finish(4);
                if (dialogueID == 1) {
                    begin_trophy_race_teleport();
                    obj->properties.trophyCabinet.action = 2;
                } else {
                    hud_visibility(1);
                }
            }
            disable_racer_input();
        }
        obj->collisionData->collidedObj = NULL;
        if (worldBalloons) {
            obj->shading->ambient = 0.552f;
        }
    }
}
