#include "thread30_bgload.h"
#include "game.h"
#include "macros.h"
#include "thread3_main.h"

/************ .data ************/
s32 gThread30NeedToLoadLevel = FALSE;
s32 gThread30LevelIdToLoad = -1;
s32 gThread30CutsceneIdToLoad = -1;
s32 gThread30LoadDelay = 0;

/*******************************/

/**
 * On the N64 a background thread loaded the menus' preview levels. The host
 * loads them on the main thread a few frames after they are requested, which
 * is all the menus can observe.
 */
void bgload_init(void) {
    gThread30NeedToLoadLevel = FALSE;
}

/**
 * Returns the value in gThread30NeedToLoadLevel.
 */
s32 bgload_active(void) {
    return gThread30NeedToLoadLevel;
}

/**
 * If a level has been requested, count down the delay and then load it.
 */
void bgload_tick(void) {
    if (gThread30NeedToLoadLevel && gThread30LoadDelay > 0) {
        gThread30LoadDelay--;
        if (gThread30LoadDelay == 0) {
            // -1 means there won't be any racers loaded.
            load_level_for_menu(gThread30LevelIdToLoad, -1, gThread30CutsceneIdToLoad);
            gThread30NeedToLoadLevel = FALSE;
        }
    }
}

/**
 * Returns the value in gThread30LoadDelay.
 */
UNUSED s32 bgload_timer(void) {
    return gThread30LoadDelay;
}

/**
 * Sets a level id and cutscene id to be loaded. Used in the tracks menu & credits menu.
 * Returns 1 if successful, or 0 if gThread30NeedToLoadLevel was already set.
 */
s32 bgload_start(s32 levelId, s32 cutsceneId) {
    if (!gThread30NeedToLoadLevel) {
        gThread30LoadDelay = 4;
        gThread30LevelIdToLoad = levelId;
        gThread30CutsceneIdToLoad = cutsceneId;
        gThread30NeedToLoadLevel = TRUE;
        return TRUE;
    }
    return FALSE;
}
