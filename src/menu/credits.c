#include "menu_internal.h"

static Gfx dCreditsFade[11] = {
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPSetRenderMode(G_RM_CLD_SURF, G_RM_CLD_SURF2),
    gsSPEndDisplayList(),
};

static s16 gCreditsObjectIndices[12] = {
    0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, -1, 0
};

static s16 gCreditsImageIndices[2] = { -1, 0 };

#define CREDITS_NO_FLAG (0)
#define CREDITS_END_FLAG (0x1000)
#define CREDITS_NEW_TITLE_FLAG (0x2000)
#define CREDITS_CONTINUE_TITLE_FLAG (0x3000)
#define CREDITS_NEXT_LEVEL_FLAG (0x4000)
#define CREDITS_UNK_FLAG (0x5000)
#define CREDITS_BEST_RACE_TIMES_FLAG (0x6000)

#define CREDITS_CONTROL_DATA_BITMASK 0xF000

#define CREDITS_END (CREDITS_END_FLAG)
#define CREDITS_NEW_TITLE(seconds) (CREDITS_NEW_TITLE_FLAG | ((s16) (seconds * 60.0)))
#define CREDITS_CONTINUE_TITLE(seconds) (CREDITS_CONTINUE_TITLE_FLAG | ((s16) (seconds * 60.0)))
#define CREDITS_NEXT_LEVEL (CREDITS_NEXT_LEVEL_FLAG)
#define CREDITS_BEST_RACE_TIMES(seconds) (CREDITS_BEST_RACE_TIMES_FLAG | ((s16) (seconds * 60.0)))

// Number of seconds for each section in the credits.
#define CREDITS_DEFAULT_TITLE_TIME 2.75
#define CREDITS_MAGIC_CODE_TIME 4.5
#define CREDITS_BEST_RACE_TIMES_TIME 8.334

typedef struct CreditsBackgroundLevelData {
    s8 levelId;
    s8 unk1;
    s8 cutsceneId;
    s8 unk3;
} CreditsBackgroundLevelData;

// Data used to control the credits.
static s16 gCreditsControlData[] = {
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    0, // "CREDITS"
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    1, 2, // "Software Director", "R.Harrison"
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    3, 4, 5, 6,
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    7, 8,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    9, 10, 11, 12,
    CREDITS_CONTINUE_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    13, 14, 15,
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    16, 17,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    18, 19,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    20, // ALSO
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    21, 22,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    23, 24,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    25, 26,
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    27, 28, 29, 30, 31,
    CREDITS_CONTINUE_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    32, 33, 34, 35,
    CREDITS_CONTINUE_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    36, 37, 38,
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    39, 40, 41, 42, 43,
    CREDITS_CONTINUE_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    44, 45, 46,
    CREDITS_CONTINUE_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    47, 48, 49,
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    50, 51, 52, 53, 54,
    CREDITS_CONTINUE_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    55, 56, 57, 58,
    CREDITS_CONTINUE_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    59, 60, 61, 62,
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    63, 64, 65,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    66, 67, 68, 69, 70,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    71, 72, 73,
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    74, 75, // "Special Thanks To", "J.HOCHBERG"
    CREDITS_CONTINUE_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    76, // "H.LINCOLN"
    CREDITS_CONTINUE_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    77, // "M.ARAKAWA"
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    78, 79,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    80, 81,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    82, 83,
    CREDITS_NEXT_LEVEL,
    CREDITS_NEW_TITLE(CREDITS_DEFAULT_TITLE_TIME),
    84, // "THE END"
    CREDITS_NEW_TITLE(CREDITS_MAGIC_CODE_TIME),
    85, 86, // Magic Code Description, Magic Code
    CREDITS_NEXT_LEVEL,
    CREDITS_END, // Set to CREDITS_BEST_RACE_TIMES(CREDITS_BEST_RACE_TIMES_TIME) if you've beaten wizpig 2.
    0, 1, 2, 3,
    CREDITS_NEXT_LEVEL,
    CREDITS_BEST_RACE_TIMES(CREDITS_BEST_RACE_TIMES_TIME),
    4, 5, 6, 7,
    CREDITS_NEXT_LEVEL,
    CREDITS_BEST_RACE_TIMES(CREDITS_BEST_RACE_TIMES_TIME),
    8, 9, 10, 11,
    CREDITS_NEXT_LEVEL,
    CREDITS_BEST_RACE_TIMES(CREDITS_BEST_RACE_TIMES_TIME),
    12, 13, 14, 15,
    CREDITS_NEXT_LEVEL,
    CREDITS_BEST_RACE_TIMES(CREDITS_BEST_RACE_TIMES_TIME),
    16, 17, 18, 19,
    CREDITS_NEXT_LEVEL,
    CREDITS_END
};

// List of amazing people.
static char *gCreditsArray[87] = { 
    "CREDITS",
    "Software Director",
    "R.Harrison",
    "Software Engineers",
    "P.Mountain", "J.Pegg", "R.Gale",
    "Art Director",
    "K.Bayliss",
    "3D Artists",
    "L.Musgrave", "K.Rabbette", "D.Smith", "J.Christensen", "B.Smyth", "P.Cunningham",
    "Music",
    "D.Wise",
    "Sound Effects",
    "G.Norgate",
    "ALSO",
    "Additional Design",
    "M.Wakeley", 
    "Documentation", 
    "L.Loveday", 
#if VERSION >= VERSION_79
    "Liaison", 
#else
    "Liason", 
#endif
    "E.Hochberg",
    "Character Voices",
    "K.Bayliss", "J.Christensen", "E.Fischer", "K.Rabbette", "L.Ray", "L.Schuneman",
    "C.Seavor", "D.Smith", "J.Stamper", "K.Stamper", "C.Sutherland",
    "Rare Quality Assurance",
    "H.Ward", "G.Richards", "S.Farmer", "J.Williams", "D.Wong", "G.Jones", "M.Penny",
    "S.Brand", "S.Malpass", "A.Wilson",
    "NOA Thanks To",
    "M.Fukuda", "K.Lobb", "M.Kelbaugh", "A.Williams", "R.Johnson", "M.Forrest",
    "P.Sandhop", "H.Sterchi", "T.Hertzog", "D.Bridgham", "G.Richardson", "B.Smith",
    "NOA Thanks To",
    "NOA Tree House", "NOA Tree Branch",
    "NCL Thank You's",
    "K.Terasaki", "M.Goto", "H.Yamada",
    "Mario Club",
    "NOE Thank You's",
    "J.Kraft", "J.Bardakoff",
    "Special Thanks To",
    "J.Hochberg", "H.Lincoln", "M.Arakawa",
    "Game Director",
    "L.Schuneman",
    "Producer",
    "C.Stamper",
    "Executive Producer",
    "T.Stamper",
    NULL,
    NULL,
    NULL };

// Developer's best times for each track
static char *gCreditsBestTimesArray[20] = {
    /***** Dino Domain *****/ "0:53:43", "1:21:55", "0:54:13", "1:22:48",
    /** Snowflake Mountain */ "1:04:11", "1:13:51", "1:25:01", "0:55:05",
    /**** Sherbet Island ***/ "1:37:63", "1:56:60", "0:57:81", "1:28:01",
    /**** Dragon Forest ****/ "1:51:15", "1:31:80", "2:03:16", "0:58:25",
    /*** Future Fun Land ***/ "2:00:38", "2:05:68", "1:52:96", "1:55:00"
};

static char *gCreditsLastMessageArray[5] = {
    "THE END?", "TO BE CONTINUED ...", // Appears after beating the first wizpig race.
    "THE END",                         // Appears after beating the second wizpig race.
    "NO CHEAT", "THIS TIME.",          // Appears if you use the "WHODIDTHIS" cheat.
};

// Cheats that are shown in the credits.
static s32 gCheatsInCreditsArray[21] = { CHEAT_BIG_CHARACTERS,
                                  CHEAT_SMALL_CHARACTERS,
                                  CHEAT_MUSIC_MENU,
                                  CHEAT_START_WITH_10_BANANAS,
                                  CHEAT_HORN_CHEAT,
                                  CHEAT_DISPLAY_CREDITS,
                                  CHEAT_DISABLE_WEAPONS,
                                  CHEAT_DISABLE_BANANAS,
                                  CHEAT_BANANAS_REDUCE_SPEED,
                                  CHEAT_NO_LIMIT_TO_BANANAS,
                                  CHEAT_ALL_BALLOONS_ARE_RED,
                                  CHEAT_ALL_BALLOONS_ARE_GREEN,
                                  CHEAT_ALL_BALLOONS_ARE_BLUE,
                                  CHEAT_ALL_BALLOONS_ARE_YELLOW,
                                  CHEAT_ALL_BALLOONS_ARE_RAINBOW,
                                  CHEAT_MAXIMUM_POWER_UP,
                                  CHEAT_TURN_OFF_ZIPPERS,
                                  CHEAT_SELECT_SAME_PLAYER,
                                  CHEAT_FOUR_WHEEL_DRIVER,
                                  CHEAT_TWO_PLAYER_ADVENTURE,
                                  CHEAT_ULTIMATE_AI };
#define NUMBER_CHEATS_IN_CREDITS sizeof(gCheatsInCreditsArray) / sizeof(s32)

static MenuElement gCreditsMenuElements[9] = {
    { 480, 104, 160, 104, -160, 104, 255, 0, 255, 48, 255, ASSET_FONTS_FUNFONT, 4, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 480, 132, 160, 132, -160, 132, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 4, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 480, 104, 160, 104, -160, 104, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 4, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 480, 132, 160, 132, -160, 132, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 4, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 480, 104, 160, 104, -160, 104, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 4, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 480, 132, 160, 132, -160, 132, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 4, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 480, 104, 160, 104, -160, 104, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 4, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 480, 132, 160, 132, -160, 132, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 4, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { NULL }, { { 0, 0, 0, 0 } } },
};

/**
 * Initialise credits sequence.
 * Sets the ending text depending on when the credits were called.
 */
void menu_credits_init(void) {
    s32 cheat;
    s32 cheatIndex;
    u16 *cheatOffsets;
    Settings *settings;

    settings = get_settings();
    gMenuDelay = 0;
    gCreditsControlDataIndex = 0;
    sCreditsPage = 0;
    gMenuStage = 0;
    gOpacityDecayTimer = 40;
    gMenuCurIndex = 0;
    sCreditsSkipped = 0;
    sCreditsFinished = 0;
    bgdraw_fillcolour(0, 0, 0);
    if (osTvType == OS_TV_TYPE_PAL) {
        viewport_menu_set(0, 0, 38, SCREEN_WIDTH, 224);
        set_viewport_properties(0, VIEWPORT_AUTO, VIEWPORT_AUTO, SCREEN_WIDTH, SCREEN_HEIGHT_PAL);
    } else {
        viewport_menu_set(0, 0, 40, SCREEN_WIDTH, 196);
        set_viewport_properties(0, VIEWPORT_AUTO, VIEWPORT_AUTO, SCREEN_WIDTH, SCREEN_HEIGHT);
    }
    copy_viewports_to_stack();
    camEnableUserView(0, TRUE);
    menu_assetgroup_load(gCreditsObjectIndices);
    menu_imagegroup_load(gCreditsImageIndices);
    menu_racer_portraits();
    load_font(ASSET_FONTS_BIGFONT);
    music_voicelimit_set(24);
    gCreditsControlData[130] = CREDITS_END; // DONT show developer times
    if (gViewingCreditsFromCheat) {
        music_play(SEQUENCE_DARKMOON_CAVERNS);
        gCreditsArray[84] = gCreditsLastMessageArray[2]; // "THE END"
        gCreditsArray[85] = gCreditsLastMessageArray[3]; // "NO CHEAT"
        gCreditsArray[86] = gCreditsLastMessageArray[4]; // "THIS TIME."
        gViewingCreditsFromCheat = FALSE;
    } else {
        if (settings->bosses & 0x20) { // WIZPIG 2
            music_play(SEQUENCE_CRESCENT_ISLAND);
            gCreditsArray[84] = gCreditsLastMessageArray[1];                                  // "TO BE CONTINUED ..."
            gCreditsControlData[130] = CREDITS_BEST_RACE_TIMES(CREDITS_BEST_RACE_TIMES_TIME); // Show developer times.
            sCreditsPage = 9;
        } else {
            music_play(SEQUENCE_DARKMOON_CAVERNS);
            gCreditsArray[84] = gCreditsLastMessageArray[0]; // "THE END?"
        }

        cheat = gCheatsInCreditsArray[rand_range(0, NUMBER_CHEATS_IN_CREDITS - 1)];
        cheatIndex = -1;
        while (cheat != 0) {
            cheat >>= 1;
            cheatIndex++;
        }

        // The first u16 of gCheatsAssetData is the total number of cheats.
        // After that is the offsets to the cheat strings.
        cheatOffsets = *gCheatsAssetData + 1;
#if VERSION == VERSION_79
#define CHEATINDEXADD 2
#define CHEATINDEXMULT 3
#elif VERSION == VERSION_80
#define CHEATINDEXADD 1
#define CHEATINDEXMULT 3
#else
#define CHEATINDEXADD 1
#define CHEATINDEXMULT 2
#endif
        gCreditsArray[85] =
            (char *) (*gCheatsAssetData) + (cheatOffsets)[(cheatIndex * CHEATINDEXMULT) + CHEATINDEXADD]; // Cheat name
        gCreditsArray[86] = (char *) (*gCheatsAssetData) + (cheatOffsets)[(cheatIndex * CHEATINDEXMULT)]; // Cheat code
    }
    music_change_off();
    enable_new_screen_transitions();
    set_gIntDisFlag(TRUE);
}

/**
 * Render a fading rectangle over the screen during the credits to serve as a transition between levels.
 */
void credits_fade(s32 x1, s32 y1, s32 x2, s32 y2, s32 a) {
    if (a > 255) {
        a = 255;
    }
    if (a < 0) {
        a = 0;
    }

    gSPDisplayList(sMenuCurrDisplayList++, dCreditsFade);
    gDPSetPrimColor(sMenuCurrDisplayList++, 0, 0, 0, 0, 0, a);
    gDPSetCombineMode(sMenuCurrDisplayList++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPFillRectangle(sMenuCurrDisplayList++, x1, y1, x1 + x2, y1 + y2);
    gDPPipeSync(sMenuCurrDisplayList++);
    gDPSetPrimColor(sMenuCurrDisplayList++, 0, 0, 255, 255, 255, 255);

    rendermode_reset(&sMenuCurrDisplayList);
}

/**
 * Handles the credits for the game
 */
s32 menu_credits_loop(s32 updateRate) {
    UNUSED s32 pad_sp7C[3];
    s32 nextIndex;
    UNUSED s32 pad_sp70[2];
    s32 breakLoop;
    s32 isCreditsEnd;
    UNUSED s32 pad_sp64;
    CreditsBackgroundLevelData *creditsBackgroundLevelData;
    s8 *mainTrackIds;
    s8 isShowingBestRaceTimes;
    CreditsBackgroundLevelData *tempBackgroundLevelData;
    s32 i;
    s32 textPos;
    s32 buttonsPressedAllPlayers;
    s32 controlDataLength;
    s32 creditsMenuElementIndex;
    s32 var_s5;
    s32 var_s4;
    s32 textLineHeight;
    s32 halvedFbSize;

    isCreditsEnd = FALSE;
    mainTrackIds = (s8 *) get_misc_asset(ASSET_MISC_MAIN_TRACKS_IDS);
    creditsBackgroundLevelData = (CreditsBackgroundLevelData *) get_misc_asset(ASSET_MISC_69);
    bgload_tick();
    if (gMenuDelay == 0) {
        disable_new_screen_transitions();
        transition_begin(NULL);
        enable_new_screen_transitions();
    }

    if (osTvType == OS_TV_TYPE_PAL) {
        credits_fade(0, 38, SCREEN_WIDTH, 186, gOpacityDecayTimer * 8);
    } else {
        credits_fade(0, 40, SCREEN_WIDTH, 156, gOpacityDecayTimer * 8);
    }

    if (gOpacityDecayTimer > 0) {
        gMenuCurIndex += updateRate << 8;
        if (gOpacityDecayTimer >= 40) {
            var_s4 = 0;
        } else {
            var_s4 = 40 - gOpacityDecayTimer;
        }
        var_s4 = (var_s4 * 5) + 72;

        var_s5 = gMenuCurIndex;
        halvedFbSize = fb_size();
        halvedFbSize >>= 17;
        halvedFbSize &= 0x7FFF;
        textPos = halvedFbSize;
        for (nextIndex = 0; nextIndex < ARRAY_COUNT(gRacerPortraits); nextIndex++) {
            texrect_draw(&sMenuCurrDisplayList, gRacerPortraits[nextIndex], ((sins_s16(var_s5) * var_s4) >> 16) + 140,
                         (((coss_s16(var_s5) * var_s4) >> 16) + textPos) - 20, 255, 255, 255, 255);
            var_s5 += 0x1999;
        }
        rendermode_reset(&sMenuCurrDisplayList);
    }

    if (sCreditsFinished != FALSE) {
        sCreditsFinished = postrace_render(updateRate) == MENU_RESULT_CONTINUE;
    }

    if (sCreditsSkipped == FALSE && sCreditsFinished == FALSE) {
        breakLoop = FALSE;
        do {
            isShowingBestRaceTimes = (gCreditsControlData[gCreditsControlDataIndex] & CREDITS_CONTROL_DATA_BITMASK) ==
                                     CREDITS_BEST_RACE_TIMES_FLAG;

            // check what to display
            switch (gCreditsControlData[gCreditsControlDataIndex] & CREDITS_CONTROL_DATA_BITMASK) {
                case CREDITS_END_FLAG:
                    gCreditsControlDataIndex = 0;
                    isCreditsEnd = TRUE;
                    gIgnorePlayerInputTime = 0;
                    break;

                case CREDITS_BEST_RACE_TIMES_FLAG: /* fallthrough */
                case CREDITS_NEW_TITLE_FLAG:
                    // Keep everything but the current credits control flag
                    sCreditsTextOffset =
                        gCreditsControlData[gCreditsControlDataIndex] & 0xFFFFFFFF & ~CREDITS_CONTROL_DATA_BITMASK;
                    // advance credits control data index by 1
                    nextIndex = ++gCreditsControlDataIndex;
                    // find next control command (basically skip any values < 0x1000)
                    while ((gCreditsControlData[gCreditsControlDataIndex] & CREDITS_CONTROL_DATA_BITMASK) ==
                           CREDITS_NO_FLAG) {
                        gCreditsControlDataIndex++;
                    }

                    // if tv type is pal we want to offset the position slightly
                    textPos = osTvType == OS_TV_TYPE_PAL ? SCREEN_HEIGHT_HALF + 14 : SCREEN_HEIGHT_HALF;

                    var_s5 = FONT_COLOURFUL;
                    textLineHeight = 20;
                    controlDataLength = gCreditsControlDataIndex - nextIndex;
                    // only one thing to show for example "CREDITS"
                    if (controlDataLength == 1) {
                        textPos -= 14;
                        var_s5 = FONT_LARGE;
                    } else if (isShowingBestRaceTimes) {
                        textPos -= (controlDataLength * 16) - 3;
                        textLineHeight = 32;
                    } else {
                        // we're showing multiple things
                        textPos -= (controlDataLength * 16) - 8;
                    }

                    gCreditsMenuElements[0].left = 480;
                    if ((gCreditsControlData[gCreditsControlDataIndex] & CREDITS_CONTROL_DATA_BITMASK) ==
                        CREDITS_CONTINUE_TITLE_FLAG) {
                        gCreditsMenuElements[0].right = SCREEN_WIDTH_HALF;
                    } else {
                        gCreditsMenuElements[0].right = -SCREEN_WIDTH_HALF;
                    }

                    for (creditsMenuElementIndex = 0, var_s4 = nextIndex; var_s4 < gCreditsControlDataIndex; var_s4++) {
                        gCreditsMenuElements[creditsMenuElementIndex].top = textPos;
                        gCreditsMenuElements[creditsMenuElementIndex].middle = textPos;
                        gCreditsMenuElements[creditsMenuElementIndex].bottom = textPos;
                        if (isShowingBestRaceTimes) {
                            // Best time for a level. Lists level in the first row and then the time in the next row
                            gCreditsMenuElements[creditsMenuElementIndex].textFont = FONT_COLOURFUL;
                            gCreditsMenuElements[creditsMenuElementIndex].filterGreen = 0;
                            gCreditsMenuElements[creditsMenuElementIndex].filterBlendFactor = 48;
                            gCreditsMenuElements[creditsMenuElementIndex].t.asciiText =
                                level_name(mainTrackIds[gCreditsControlData[var_s4]]);

                            creditsMenuElementIndex++;
                            gCreditsMenuElements[creditsMenuElementIndex].top = textPos + 14;
                            gCreditsMenuElements[creditsMenuElementIndex].middle = textPos + 14;
                            gCreditsMenuElements[creditsMenuElementIndex].bottom = textPos + 14;
                            gCreditsMenuElements[creditsMenuElementIndex].textFont = FONT_COLOURFUL;
                            gCreditsMenuElements[creditsMenuElementIndex].t.asciiText =
                                gCreditsBestTimesArray[gCreditsControlData[var_s4]];
                            creditsMenuElementIndex++;
                        } else {
                            // every other element should have a little more green and a little less red/orange
                            if (creditsMenuElementIndex & 1) {
                                gCreditsMenuElements[creditsMenuElementIndex].filterGreen = 255;
                                gCreditsMenuElements[creditsMenuElementIndex].filterBlendFactor = 0;
                            }
                            gCreditsMenuElements[creditsMenuElementIndex].textFont = var_s5;
                            gCreditsMenuElements[creditsMenuElementIndex].t.asciiText =
                                gCreditsArray[gCreditsControlData[var_s4]];
                            creditsMenuElementIndex++;
                        }
                        // after the first iteration, whatever was the font previously, set it to large now
                        // this way to title like "Software Director" is colourful while the parts after it are "just"
                        // large
                        var_s5 = FONT_LARGE;
                        // move text position up (which puts the next entry a further down on the actual screen)
                        textPos += textLineHeight;
                        // Since every element now uses large font increase the text line height
                        textLineHeight = 32;
                    }

                    gCreditsMenuElements[creditsMenuElementIndex].t.element = NULL;
                    postrace_offsets(gCreditsMenuElements, 0.5f, (f32) sCreditsTextOffset / 60.0f, 0.5f, 0, 0);
                    sCreditsFinished = postrace_render(0) == MENU_RESULT_CONTINUE;
                    breakLoop = TRUE;
                    break;

                // Continuation of the previous "title" e.g. "Character voices"
                case CREDITS_CONTINUE_TITLE_FLAG:
                    // Keep everything but the current credits control flag
                    sCreditsTextOffset =
                        gCreditsControlData[gCreditsControlDataIndex] & 0xFFFFFFFF & ~CREDITS_CONTROL_DATA_BITMASK;
                    // advance credits control data index by 1
                    nextIndex = ++gCreditsControlDataIndex;
                    // find next control command (basically skip any values < 0x1000)
                    while ((gCreditsControlData[gCreditsControlDataIndex] & CREDITS_CONTROL_DATA_BITMASK) ==
                           CREDITS_NO_FLAG) {
                        gCreditsControlDataIndex++;
                    }
                    gCreditsMenuElements[0].left = SCREEN_WIDTH_HALF;
                    if ((gCreditsControlData[gCreditsControlDataIndex] & CREDITS_CONTROL_DATA_BITMASK) ==
                        CREDITS_CONTINUE_TITLE_FLAG) {
                        gCreditsMenuElements[0].right = SCREEN_WIDTH_HALF;
                    } else {
                        gCreditsMenuElements[0].right = -SCREEN_WIDTH_HALF;
                    }

                    for (var_s4 = nextIndex; var_s4 < gCreditsControlDataIndex; var_s4++) {
                        // always add 1 to keep first line (title)
                        // this doesn't require setting fonts or anything else as it was already setup before, only the
                        // text has to change
                        gCreditsMenuElements[1 + var_s4 - nextIndex].t.asciiText =
                            gCreditsArray[gCreditsControlData[var_s4]];
                    }

                    // clear text of last element
                    // this *technically* has the bug that it would not properly clear the text of the last n elements
                    // if n is > 1 for example if the first screen has 1 title and 3 names and the next screen has only
                    // 1 name (+ 1 title) it would not properly clear the second name of the previous page
                    gCreditsMenuElements[var_s4 - nextIndex + 1].t.asciiText = NULL;
                    postrace_offsets(gCreditsMenuElements, 0.5f, (f32) sCreditsTextOffset / 60.0f, 0.5f, 0, 0);
                    sCreditsFinished = postrace_render(0) == MENU_RESULT_CONTINUE;
                    breakLoop = TRUE;
                    break;

                // continue to the next level in the credits
                case CREDITS_NEXT_LEVEL_FLAG:
                    gCreditsControlDataIndex++;
                    // show the characters in a circle?
                    sCreditsSkipped = TRUE;
                    breakLoop = TRUE;
                    break;
                // ??? advance command index, this is 0x5000 so it might be 0x4000 (next level) + 0x1000 (end)
                case CREDITS_UNK_FLAG:
                    gCreditsControlDataIndex++;
                    break;
            }
        } while (breakLoop == FALSE);
    }

    // collect all pressed buttons of all players
    buttonsPressedAllPlayers = 0;
    if (gIgnorePlayerInputTime == 0 && gMenuDelay == 0) {
        for (nextIndex = 0; nextIndex < MAXCONTROLLERS; nextIndex++) {
            buttonsPressedAllPlayers |= input_pressed(nextIndex);
        }
    }

    // handles the animations (if any) between levels in the credits
    switch (gMenuStage) {
        case 0:
            tempBackgroundLevelData = &creditsBackgroundLevelData[sCreditsPage];
            bgload_start(tempBackgroundLevelData->levelId, tempBackgroundLevelData->cutsceneId);
            gMenuStage = 1;
            gOpacityDecayTimer = 40;
            break;
        case 1:
            if (bgload_active() == FALSE) {
                gMenuStage = 2;
                gOptionBlinkTimer = 40;
                sCreditsSkipped = FALSE;
            }
            break;
        case 2:
            gOptionBlinkTimer -= updateRate;
            gOpacityDecayTimer -= updateRate;
            if (gOptionBlinkTimer <= 0) {
                gOptionBlinkTimer += 600;
                gMenuStage = 3;
            }
            break;
        case 3:
            gOptionBlinkTimer -= updateRate;
            if (gOpacityDecayTimer > 0) {
                gOpacityDecayTimer -= updateRate;
            } else {
                gOpacityDecayTimer = 0;
            }
            if (gOptionBlinkTimer <= 0) {
                gOptionBlinkTimer = 40;
                gMenuStage = 4;
            }
            break;
        case 4:
            gOptionBlinkTimer -= updateRate;
            gOpacityDecayTimer += updateRate;
            if (gOptionBlinkTimer <= 0) {
                gMenuStage = 0;
                sCreditsPage++;
                if ((sCreditsPage + creditsBackgroundLevelData)->levelId < 0) {
                    sCreditsPage = 0;
                    gIgnorePlayerInputTime = 0;
                }
            }
            break;
    }

    // check if the credits should end
    if ((buttonsPressedAllPlayers & (A_BUTTON | START_BUTTON)) || (buttonsPressedAllPlayers & B_BUTTON) ||
        isCreditsEnd) {
        gMenuDelay = 1;
        disable_new_screen_transitions();
        transition_begin(&sMenuTransitionFadeIn);
        enable_new_screen_transitions();
        music_fade(-128);
    }
    if (gMenuDelay > 0) {
        gMenuDelay += updateRate;
        if (bgload_active() == FALSE && gMenuDelay > 30) {
            music_change_on();
            credits_free();
            load_level_for_menu(ASSET_LEVEL_FRONTEND, ZERO_PLAYERS, CUTSCENE_NONE);
            menu_init(MENU_LOGOS);
        }
    }
    return 0;
}

/**
 * Unload associated assets with the credits scene.
 */
void credits_free(void) {
    music_voicelimit_set(18);
    disable_new_screen_transitions();
    camDisableUserView(0, FALSE);
    set_viewport_properties(0, VIEWPORT_AUTO, VIEWPORT_AUTO, VIEWPORT_AUTO, VIEWPORT_AUTO);
    menu_assetgroup_free(gCreditsObjectIndices);
    unload_font(ASSET_FONTS_BIGFONT);
    set_gIntDisFlag(FALSE);
}
