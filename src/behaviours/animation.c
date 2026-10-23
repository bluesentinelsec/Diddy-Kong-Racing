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
#include "PR/os_cont.h"
#include "PRinternal/viint.h"
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

void obj_init_animation(Object *obj, LevelObjectEntry *levelEntry, s32 arg2) {
    LevelObjectEntry_Animation *entry = &levelEntry->animation;
    Object *animTarget;
    s8 tempOrderIndex;
    f32 scalef;

    scalef = entry->scale & 0xFF;
    if (scalef < 1.0f) {
        scalef = 1.0f;
    }
    scalef /= 64;
    obj->trans.scale = obj->header->scale * scalef;
    obj->trans.rotation.y_rotation = entry->y_rotation << 8;
    obj->trans.rotation.x_rotation = entry->x_rotation << 8;
    obj->trans.rotation.z_rotation = entry->z_rotation << 8;
    if (entry->actorIndex == -2) {
        entry->actorIndex = anim_group_get();
    } else {
        if (entry->actorIndex < 0 && entry->actorIndex >= -2) {
            entry->actorIndex = 0;
        }
        anim_group_set(entry->actorIndex);
    }
    if (entry->channel == -1) {
        entry->channel = cutscene_id();
    }
    if (entry->channel == 20) {
        entry->actorIndex |= 0x80;
    }
    tempOrderIndex = anim_count_actors(entry->actorIndex);
    if (entry->order == -2) {
        entry->order = tempOrderIndex;
        if (entry->order < 0) {
            entry->order = 0;
        }
    } else {
        if ((tempOrderIndex >= 0) && (entry->order >= tempOrderIndex)) {
            entry->order = tempOrderIndex;
        }
        if (entry->order < 0) {
            entry->order = 0;
        }
    }
    path_enable();
    obj->properties.animation.behaviourID = entry->actorIndex;
    obj->properties.animation.action = arg2;
    if (arg2 != 0 && (input_pressed(PLAYER_ONE) & R_CBUTTONS)) {
        obj->properties.animation.action = 2;
    }
    if (((cutscene_id() == entry->channel) || (entry->channel == 20)) && (obj->animTarget == NULL) &&
        (entry->order == 0) && (entry->objectIdToSpawn != -1)) {
        anim_object_spawn_target(obj, entry);
    }
    animTarget = obj->animTarget;
    if (obj->animTarget != NULL) {
        obj_init_animobject(obj, animTarget);
        if (entry->order != 0 || animTarget->objectID != entry->objectIdToSpawn) {
            free_object(animTarget);
            obj->animTarget = NULL;
        }
    }
    if (anim_actor_is_active(obj->properties.animation.behaviourID)) {
        anim_objects_spawn_targets();
    }
}

/**
 * The animation owns the object it animates unless the caller asked to keep it.
 */
void obj_destroy_animation(Object *obj, s32 keepAnimTarget) {
    if (obj->animTarget != NULL && keepAnimTarget == 0) {
        free_object(obj->animTarget);
    }
}
