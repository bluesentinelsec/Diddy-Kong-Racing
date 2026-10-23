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

/**
 * Effect box init behaviour.
 * Does nothing.
 */
void obj_init_effectbox(UNUSED Object *obj, UNUSED LevelObjectEntry *levelEntry, UNUSED s32 param) {
}

/**
 * Effect box loop behaviour.
 * Searches for any racers that are inside the region.
 * Applies an effect to their weight and velocity.
 */
void obj_loop_effectbox(Object *obj, UNUSED s32 updateRate) {
    Object **racers;
    LevelObjectEntry_EffectBox *effectBoxEntry;
    s32 numRacers;
    Object *curRacerObj;
    Object_Racer *curRacer;
    f32 sinAngle;
    f32 zDiff;
    f32 yExtentsHalf;
    f32 xDiff;
    f32 yExtents;
    f32 cosAngle;
    f32 xExtents;
    f32 zExtents;
    f32 yDiff;
    s32 i;

    effectBoxEntry = (LevelObjectEntry_EffectBox *) obj->level_entry;
    racers = get_racer_objects(&numRacers);
    cosAngle = coss_f(-(effectBoxEntry->angleY * 256));
    sinAngle = sins_f(-(effectBoxEntry->angleY * 256));
    xExtents = effectBoxEntry->halfWidthX * 3;
    yExtents = effectBoxEntry->halfHeightY * 3;
    zExtents = effectBoxEntry->halfDepthZ * 3;
    for (i = 0; i < numRacers; i++) {
        curRacerObj = racers[i];
        xDiff = curRacerObj->trans.x_position - obj->trans.x_position;
        yDiff = curRacerObj->trans.y_position - obj->trans.y_position;
        zDiff = curRacerObj->trans.z_position - obj->trans.z_position;
        if (-yExtents < yDiff && yDiff < yExtents) {
            yExtentsHalf = (xDiff * cosAngle) + (zDiff * sinAngle);
            if (-xExtents < yExtentsHalf && yExtentsHalf < xExtents) {
                zDiff = (-xDiff * sinAngle) + (zDiff * cosAngle);
                if (-zExtents < zDiff && zDiff < zExtents) {
                    yExtentsHalf = yExtents / 2;
                    curRacer = curRacerObj->racer;
                    curRacer->effectBoxType = effectBoxEntry->effectType;
                    curRacer->effectBoxStrength = effectBoxEntry->effectStrength;
                    if (yExtentsHalf < yDiff && curRacer->effectBoxType == 1) {
                        xDiff = (yDiff - yExtentsHalf) / yExtentsHalf;
                        yDiff = (1.0 - xDiff);
                        curRacer->effectBoxStrength *= yDiff;
                    }
                }
            }
        }
    }
}
