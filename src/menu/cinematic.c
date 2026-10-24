#include "menu_internal.h"

static s8 *gCinematicParams;

static s8 *gCinematicPortraits;

static s32 gCinematicEnd;

static s32 gCinematicSkipA;

static s32 gCinematicSkipB;

static s32 gCinematicMusicChangeOff;

static s16 gCinematicObjectIndices[12] = { 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, -1, 0 };

/**
 * Set the parameters of the cutscene from the arguments, and decide wether the player can skip
 * the cutscenes.
 */
void cinematic_start(s8 *params, s32 cinematicIndex, s32 endFlags, s32 skipFlagsA, s32 skipFlagsB, s8 *portraits) {
    s32 index;

    index = 0;
    while (index < cinematicIndex) {
        index++;
        while (-1 != *params) {
            params += 3;
        }
        params++;
    }

    if (index == 0 && (s8 *) get_misc_asset(ASSET_MISC_CINEMATIC_RACE) == params) {
        gCinematicMusicChangeOff = TRUE;
    } else {
        gCinematicMusicChangeOff = FALSE;
    }

    gCinematicParams = params;
    gCinematicEnd = endFlags;
    gCinematicSkipA = skipFlagsA;
    gCinematicSkipB = skipFlagsB;
    gCinematicPortraits = portraits;
}

/**
 * Load a new level and trigger a cutscene.
 */
void menu_cinematic_init(void) {
    if (gCinematicPortraits != NULL) {
        menu_assetgroup_load(gCinematicObjectIndices);
        menu_racer_portraits();
    }
    load_level_for_menu(gCinematicParams[CINEMATIC_LEVELID], gCinematicParams[CINEMATIC_PLAYERS],
                        gCinematicParams[CINEMATIC_CUTSCENE]);
    gMenuInput.delay = 0;
    gMenuFlow.stage = 0;
}

/**
 * Wait for a signal, which can come from a few sources depending on what cutscene is playing.
 * After that signal, exit this menu mode.
 */
s32 menu_cinematic_loop(UNUSED s32 updateRate) {
    s32 i;
    s32 buttonsPressed;

    buttonsPressed = 0;
    if (gMenuInput.ignoreInputTime == 0) {
        for (i = 0; i < gRaceSetup.numberOfActivePlayers; i++) {
            buttonsPressed |= input_pressed(i);
        }
    }
    if (anim_prev_frame_state() != 0) {
        gCinematicParams += 3;
        if (gCinematicParams[CINEMATIC_LEVELID] > -1) {
            load_level_for_menu(gCinematicParams[CINEMATIC_LEVELID], gCinematicParams[CINEMATIC_PLAYERS],
                                gCinematicParams[CINEMATIC_CUTSCENE]);
        } else {
            if (gCinematicMusicChangeOff) {
                music_change_off();
            }
            cinematic_free();
            return gCinematicEnd;
        }
    }
    if (gCinematicSkipA && buttonsPressed & (A_BUTTON | START_BUTTON)) {
        cinematic_free();
        return gCinematicSkipA;
    }
    if (gCinematicSkipB && (buttonsPressed & B_BUTTON)) {
        cinematic_free();
        return gCinematicSkipB;
    }
    if (gCinematicPortraits != NULL) {
        for (i = 0; gCinematicPortraits[i] != -1; i++) {
            texrect_draw(&gMenuFrame.displayList, gRacerPortraits[gCinematicPortraits[i]], 24, 16 + (44 * i), 255, 255,
                         255, 255);
        }
    }
    gMenuInput.ignoreInputTime = 0;
    return MENU_RESULT_CONTINUE;
}

/**
 * Free all assets associated with the intro cutscene.
 */
void cinematic_free(void) {
    if (gCinematicPortraits != NULL) {
        menu_assetgroup_free(gCinematicObjectIndices);
    }
}
