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

void obj_init_trigger(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Trigger *entry = &levelEntry->trigger;
    f32 radius;
    Object_Trigger *trigger;

    if (entry->index == -1) {
        entry->index = object_id_alloc(obj);
    } else {
        object_id_assign(obj, entry->index);
    }
    if (entry->index == -1) {
        pc_log("Illegal door no!!!\n");
    }
    radius = (s32) entry->scale & 0xFF;
    if (radius < 5.0f) {
        radius = 5.0f;
    }
    trigger = obj->trigger;
    radius /= 128;
    obj->trans.scale = radius;
    obj->trans.rotation.y_rotation = U8_ANGLE_TO_U16(entry->angleY);
    trigger->directionX = sins_f(obj->trans.rotation.y_rotation);
    trigger->directionY = 0.0f;
    trigger->directionZ = coss_f(obj->trans.rotation.y_rotation);
    trigger->rotationDiff =
        -((trigger->directionX * obj->trans.x_position) + (trigger->directionZ * obj->trans.z_position));
    trigger->radius = entry->scale;
    trigger->racerFilterFlags = entry->racerFilterFlags;
    obj->interactObj->flags = INTERACT_FLAGS_TANGIBLE;
    obj->interactObj->hitboxKind = 0;
    obj->interactObj->hitboxRadius = entry->scale;
    obj->interactObj->pushForce = 0;
}

void obj_loop_trigger(Object *obj, UNUSED s32 updateRate) {
    s32 i;
    s32 curRaceType;
    s32 numRacers;
    LevelObjectEntry_Trigger *triggerEntry;
    Object *racerObj;
    Object **racers;
    Object_Trigger *trigger;
    Object_Racer *racer;
    Settings *settings;
    f32 diffX;
    f32 diffY;
    f32 diffZ;
    f32 radiusF;
    s32 flags;
    f32 distance;
    s32 courseFlags;

    triggerEntry = &obj->level_entry->trigger;
    trigger = obj->trigger;
    settings = get_settings();
    courseFlags = settings->courseFlagsPtr[settings->courseId];
    curRaceType = level_type();
    if (triggerEntry->index >= 0) {
        flags = 0x10000 << triggerEntry->index;
        if (obj->interactObj->distance < trigger->radius) {
            if (((u8) curRaceType != RACETYPE_HUBWORLD) || !(courseFlags & flags)) {
                radiusF = trigger->radius;
                racers = get_racer_objects(&numRacers);
                for (i = 0; i < numRacers; i++) {
                    racerObj = racers[i];
                    racer = racerObj->racer;
                    if ((!(trigger->racerFilterFlags & 1) && racer->playerIndex == PLAYER_COMPUTER) ||
                        (!(trigger->racerFilterFlags & 2) && racer->playerIndex != PLAYER_COMPUTER)) {
                        diffX = racerObj->trans.x_position - obj->trans.x_position;
                        diffY = racerObj->trans.y_position - obj->trans.y_position;
                        diffZ = racerObj->trans.z_position - obj->trans.z_position;
                        distance = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
                        if (distance < radiusF) {
                            distance = (trigger->directionX * racerObj->trans.x_position) +
                                       (trigger->directionZ * racerObj->trans.z_position) + trigger->rotationDiff;
                            if (distance < 0.0f) {
                                settings->courseFlagsPtr[settings->courseId] |= flags;
                                if (triggerEntry->textId != 0xFF) {
                                    set_current_text(triggerEntry->textId);
                                }
                                if (triggerEntry->animActorId != 0xFF) {
                                    anim_actor_trigger(triggerEntry->animActorId + 0x80);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
