#include "host.h"
#include "menu_internal.h"

static s32 gRankingsPoints[8];

static u8 gResultsPlayers[8];

static u8 gRankingsPlayers[8];

static u8 gResultsPlayerIDs[8];

static u8 gRankingsPlayerIDs[8]; // Contains the order of racer indices that tell you what place they are in.

static s8 gRankingsPortraitIDs[16];

static s32 gRankingPlayerCount = 8; // Number of racers in trophy race? (1 to 8)

static s32 gPrevTrophyRaceRound = 0;

static s16 gTrophyRankingsRawIconPositions[] = {
    0x0040, 0x0020, 0x0082, 0x0040, 0x0020, 0x0082, 0x0040, 0x0020, 0x0082, 0x0040, 0x0020, 0x0082, 0x00DC, 0x00BC,
    0x011E, 0x00DC, 0x00BC, 0x011E, 0x00DC, 0x00BC, 0x011E, 0x00DC, 0x00BC, 0x011E, 0x0030, 0x0030, 0x0044, 0x005A,
    0x005A, 0x006E, 0x0084, 0x0084, 0x0098, 0x00AE, 0x00AE, 0x00C2, 0x0030, 0x0030, 0x0044, 0x005A, 0x005A, 0x006E,
    0x0084, 0x0084, 0x0098, 0x00AE, 0x00AE, 0x00C2, 0x0038, 0x0038, 0x0040, 0x006A, 0x006A, 0x0072, 0x009C, 0x009C,
    0x00A4, 0x00CE, 0x00CE, 0x00D6, 0x0038, 0x0038, 0x0040, 0x006A, 0x006A, 0x0072, 0x009C, 0x009C, 0x00A4, 0x00CE,
    0x00CE, 0x00D6, 0x0040, 0x0020, 0x0082, 0x0040, 0x0020, 0x0082, 0x0040, 0x0020, 0x0082, 0x00DC, 0x00BC, 0x011E,
    0x00DC, 0x00BC, 0x011E, 0x00DC, 0x00BC, 0x011E, 0x0034, 0x0034, 0x0048, 0x006F, 0x006F, 0x0083, 0x00AA, 0x00AA,
    0x00BE, 0x0034, 0x0034, 0x0048, 0x006F, 0x006F, 0x0083, 0x00AA, 0x00AA, 0x00BE, 0x004C, 0x004C, 0x0054, 0x007E,
    0x007E, 0x0086, 0x00B0, 0x00B0, 0x00B8, 0x004C, 0x004C, 0x0054, 0x007E, 0x007E, 0x0086, 0x00B0, 0x00B0, 0x00B8,
    0x008C, 0x006C, 0x00CE, 0x008C, 0x006C, 0x00CE, 0x008C, 0x006C, 0x00CE, 0x008C, 0x006C, 0x00CE
};

// Number of points to award in trophy race for each place.
static s32 gTrophyRacePointsArray[8] = { 9, 7, 5, 3, 1, 0, 0, 0 };

static s16 gTrophyRankingsObjectIndices[14] = { 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
                                         0x39, 0x3A, 0x3B, 0x00, 0x01, -1,   0x00 };

static s16 gTrophyRaceImageIndices[3] = { 0, 1, -1 };

static MenuElement gTrophyRankingsTitle[] = {
    { 320 + 160 + 1, 35, SCREEN_WIDTH_HALF + 1, 35, -159, 35, 0, 0, 0, 255, 128, ASSET_FONTS_BIGFONT, 12, 0, { 0 }, { { 0, 0, 0, 0 } } },
    { 320 + 160, 32, SCREEN_WIDTH_HALF, 32, -160, 32, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0, { 0 }, { { 0, 0, 0, 0 } } },
// Not the most glamourous approach, but the least worst one without a proper rematching solution.
#ifndef AVOID_UB
};
static MenuElement gTrophyRankingsRacers[] = {
#endif
    { 64, -192, 64, 48, 64, 288, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 32, -192, 32, 48, 32, 288, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 0, { gFirstPlace }, { { 0, 0, 0, 0 } } },
    { 130, -172, 130, 68, 130, 308, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 4, 2, { &gTrophyRacePointsArray[0] }, { { 0, 0, 0, 0 } } },
    { 64, -150, 64, 90, 64, 330, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 32, -150, 32, 90, 32, 330, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 0, { gSecondPlace }, { { 0, 0, 0, 0 } } },
    { 130, -130, 130, 110, 130, 350, 255, 192, 255, 0, 255, ASSET_FONTS_FUNFONT, 4, 2, { &gTrophyRacePointsArray[1] }, { { 0, 0, 0, 0 } } },
    { 64, -108, 64, 132, 64, 372, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 32, -108, 32, 132, 32, 372, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 0, { gThirdPlace }, { { 0, 0, 0, 0 } } },
    { 130, -88, 130, 152, 130, 392, 255, 128, 255, 0, 255, ASSET_FONTS_FUNFONT, 4, 2, { &gTrophyRacePointsArray[2] }, { { 0, 0, 0, 0 } } },
    { 64, -66, 64, 174, 64, 414, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 32, -66, 32, 174, 32, 414, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 0, { gFourthPlace }, { { 0, 0, 0, 0 } } },
    { 130, -46, 130, 194, 130, 434, 255, 64, 255, 0, 255, ASSET_FONTS_FUNFONT, 4, 2, { &gTrophyRacePointsArray[3] }, { { 0, 0, 0, 0 } } },
    { 220, 288, 220, 48, 220, -192, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 188, 288, 188, 48, 188, -192, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 0, { gFifthPlace }, { { 0, 0, 0, 0 } } },
    { 286, 308, 286, 68, 286, -172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 4, 2, { &gTrophyRacePointsArray[4] }, { { 0, 0, 0, 0 } } },
    { 220, 330, 220, 90, 220, -160, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 188, 330, 188, 90, 188, -150, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 0, { gSixthPlace }, { { 0, 0, 0, 0 } } },
    { 286, 350, 286, 110, 286, -130, 255, 192, 255, 0, 255, ASSET_FONTS_FUNFONT, 4, 2, { &gTrophyRacePointsArray[5] }, { { 0, 0, 0, 0 } } },
    { 220, 372, 220, 132, 220, -108, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 188, 372, 188, 132, 188, -108, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 0, { gSeventhPlace }, { { 0, 0, 0, 0 } } },
    { 286, 392, 286, 152, 286, -88, 255, 128, 255, 0, 255, ASSET_FONTS_FUNFONT, 4, 2, { &gTrophyRacePointsArray[6] }, { { 0, 0, 0, 0 } } },
    { 220, 414, 220, 174, 220, -66, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 188, 414, 188, 174, 188, -66, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 0,  { gEighthPlace }, { { 0, 0, 0, 0 } } },
    { 286, 434, 286, 194, 286, -46, 255, 64, 255, 0, 255, ASSET_FONTS_FUNFONT, 4, 2, { &gTrophyRacePointsArray[7] }, { { 0, 0, 0, 0 } } },
    {NULL}
};

static s16 *gTrophyRankingsIconPositions[32] = { NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &gTrophyRankingsRawIconPositions[126],
                                          &gTrophyRankingsRawIconPositions[90],
                                          &gTrophyRankingsRawIconPositions[126],
                                          &gTrophyRankingsRawIconPositions[24],
                                          NULL,
                                          NULL,
                                          &gTrophyRankingsRawIconPositions[72],
                                          &gTrophyRankingsRawIconPositions[90],
                                          NULL,
                                          NULL,
                                          &gTrophyRankingsRawIconPositions[0],
                                          &gTrophyRankingsRawIconPositions[24],
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &gTrophyRankingsRawIconPositions[126],
                                          &gTrophyRankingsRawIconPositions[108],
                                          &gTrophyRankingsRawIconPositions[126],
                                          &gTrophyRankingsRawIconPositions[48],
                                          NULL,
                                          NULL,
                                          &gTrophyRankingsRawIconPositions[72],
                                          &gTrophyRankingsRawIconPositions[108],
                                          NULL,
                                          NULL,
                                          &gTrophyRankingsRawIconPositions[0],
                                          &gTrophyRankingsRawIconPositions[48] };

/**
 *
 */
void trophyround_adventure(void) {
    Settings *settings = get_settings();
    gRaceSetup.trophyRaceWorldId = settings->worldId;
    gRaceSetup.trophyRaceRound = 0;
    settings->levelExit->fromMapId = settings->courseId;
    settings->levelExit->returnEntrance = 0;
    settings->levelExit->chosenExit = 0;
    gRaceSetup.inAdvModeTrophyRace = TRUE;
    set_time_trial_enabled(0);
}

/**
 * Initialise the trophy race intro.
 * Finds which level should be next, then loads it for the preview.
 */
void menu_trophy_race_round_init(void) {
    s32 i;
    s32 index;
    Settings *settings;
    s8 *levelIds;

    settings = get_settings();
    levelIds = (s8 *) get_misc_asset(ASSET_MISC_TRACKS_MENU_IDS); // Returns level ids array.

    if (gRaceSetup.trophyRaceRound == 0) {
        for (index = 0; index < 8; index++) {
            settings->racers[index].trophy_points = 0;
        }
    }

    // Is this a fakematch? I can't tell.
    do {
        index = levelIds[((gRaceSetup.trophyRaceWorldId - 1) * 6) + gRaceSetup.trophyRaceRound];
        if (index != -1) {
            continue;
        }
        index = (index + 1) & 3;
    } while (index == -1);

    for (i = 0; i < gRaceSetup.numberOfActivePlayers; i++) {
        gRaceSetup.playerVehicles[i] = leveltable_vehicle_default(index);
    }

    set_level_default_vehicle(leveltable_vehicle_default(index));
    load_level_for_menu(index, -1, 1);

    gMenuInput.delay = 0;
    gTrackSelect.nameVoiceDelay = 10;
    load_font(ASSET_FONTS_BIGFONT);
    music_voicelimit_set(24);
    music_play(SEQUENCE_MAIN_MENU);
    music_fade(256);
}

/**
 * Draws the trophy race intro text
 * Ex. (DINO DOMAIN / TROPHY RACE / ROUND ONE / ANCIENT LAKE)
 */
void trophyround_render(UNUSED s32 updateRate) {
    s32 yPos;
    char *worldName;
    char *levelName;
    s8 *levelIds;

    levelIds = (s8 *) get_misc_asset(ASSET_MISC_TRACKS_MENU_IDS);
    if (osTvType == OS_TV_TYPE_PAL) {
        yPos = 18;
    } else {
        yPos = 0;
    }

    worldName = level_name(level_world_id(gRaceSetup.trophyRaceWorldId));
    levelName = level_name(levelIds[((gRaceSetup.trophyRaceWorldId - 1) * 6) + gRaceSetup.trophyRaceRound]);
    set_text_background_colour(0, 0, 0, 0);
    set_text_font(ASSET_FONTS_BIGFONT);
    // Text Shadows first
    set_text_colour(0, 0, 0, 255, 128);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 1, 35, (char *) worldName, ALIGN_MIDDLE_CENTER);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 1, 67, gMenuLoaded.text[ASSET_MENU_TEXT_TROPHYRACE],
              ALIGN_MIDDLE_CENTER); // TROPHY RACE
    set_text_colour(255, 255, 255, 0, 255);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, 32, (char *) worldName, ALIGN_MIDDLE_CENTER);
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, 64, gMenuLoaded.text[ASSET_MENU_TEXT_TROPHYRACE],
              ALIGN_MIDDLE_CENTER); // TROPHY RACE
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, yPos + 176,
              gMenuLoaded.text[ASSET_MENU_TEXT_ROUNDONE + gRaceSetup.trophyRaceRound],
              ALIGN_MIDDLE_CENTER); // ROUND ONE / ROUND TWO / ROUND THREE / ROUND FOUR
    draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, yPos + 208, (char *) levelName, ALIGN_MIDDLE_CENTER);
}

/**
 * Render the trophy race hub world and round number.
 * Proceed to the race when A is pressed.
 */
s32 menu_trophy_race_round_loop(s32 updateRate) {
    s8 *trackMenuIds;
    s16 ttVoiceLine;
    s32 temp;

    trackMenuIds = (s8 *) get_misc_asset(ASSET_MISC_TRACKS_MENU_IDS); // tracks_menu_ids
    if (gTrackSelect.nameVoiceDelay != 0) {
        gTrackSelect.nameVoiceDelay -= updateRate;
        if (gTrackSelect.nameVoiceDelay <= 0) {
            // This is mostly likely supposed to be a multi dimensional array
            temp = trackMenuIds[(((gRaceSetup.trophyRaceWorldId - 1) * 6) + gRaceSetup.trophyRaceRound)];
            ttVoiceLine = gTTVoiceLines[temp];
            if (ttVoiceLine != -1) {
                sound_play(ttVoiceLine, NULL);
            }
            gTrackSelect.nameVoiceDelay = 0;
        }
    }
    if (gMenuInput.delay != 0) {
        gMenuInput.delay += updateRate;
    }
    if (gMenuInput.delay < 22) {
        trophyround_render(updateRate);
    }
    if ((gMenuInput.ignoreInputTime == 0) && (gMenuInput.delay == 0)) {
        menu_input();
        if ((gMenuInput.buttons[PLAYER_MENU] & (A_BUTTON | START_BUTTON)) != 0) {
            transition_begin(&sMenuTransitionFadeIn);
            gMenuInput.delay = 1;
            music_fade(-128);
        }
    }
    if (gMenuInput.delay > 30) {
        trophyround_free();
        gRaceSetup.trackIdToLoad =
            trackMenuIds[(((gRaceSetup.trophyRaceWorldId - 1) * 6) + gRaceSetup.trophyRaceRound)];
        gRaceSetup.trackSpecified = 1;
        return gRaceSetup.numberOfActivePlayers;
    }
    gMenuInput.ignoreInputTime = 0;
    return MENU_RESULT_CONTINUE;
}

/**
 * Free any assets associated with the trophy race intro menu.
 */
void trophyround_free(void) {
    unload_font(ASSET_FONTS_BIGFONT);
}

void trophy_rankings_init(s32 isRankings) {
    Settings *settings;
    s16 **iconPositions;
    s16 *yPositions;
    s16 *xPositions;
    s32 yOffset;
    s32 menuElemIndex;
    s32 racerIndex;
    s32 temp;
    s32 greenAmount;
    char *titleText;

    settings = get_settings();
    titleText = (isRankings) ? gMenuLoaded.text[ASSET_MENU_TEXT_RANKINGS] : gMenuLoaded.text[ASSET_MENU_TEXT_RACEORDER];
    gTrophyRankingsTitle[0].t.asciiText = titleText;
    gTrophyRankingsTitle[1].t.asciiText = titleText;
    racerIndex = (gRankingPlayerCount - 1);
    racerIndex <<= 1;
    yOffset = 240;
    if (osTvType == OS_TV_TYPE_PAL) {
        racerIndex += 16;
        yOffset = 264;
    }
    iconPositions = gTrophyRankingsIconPositions;
    xPositions = gTrophyRankingsIconPositions[racerIndex++];
    yPositions = iconPositions[racerIndex];
    menuElemIndex = 2;
    if (xPositions != NULL && yPositions != NULL) {
        for (racerIndex = 0; racerIndex < gRankingPlayerCount; racerIndex++) {
            for (temp = 0; temp < 3; temp++) {
                gTrophyRankingsTitle[menuElemIndex + temp].left = *xPositions;
                gTrophyRankingsTitle[menuElemIndex + temp].center = *xPositions;
                gTrophyRankingsTitle[menuElemIndex + temp].right = *xPositions;
                xPositions++;
                if ((gRankingPlayerCount >= 5) && (racerIndex >= (gRankingPlayerCount >> 1))) {
                    gTrophyRankingsTitle[menuElemIndex + temp].top = *yPositions - yOffset;
                    gTrophyRankingsTitle[menuElemIndex + temp].middle = *yPositions;
                    gTrophyRankingsTitle[menuElemIndex + temp].bottom = *yPositions + yOffset;
                    yPositions++;
                } else {
                    gTrophyRankingsTitle[menuElemIndex + temp].top = *yPositions + yOffset;
                    gTrophyRankingsTitle[menuElemIndex + temp].middle = *yPositions;
                    gTrophyRankingsTitle[menuElemIndex + temp].bottom = *yPositions - yOffset;
                    yPositions++;
                }
            }

            // Regalloc issue here.
            temp = gRankingPlayerCount & 3;
            if (temp) {
                temp = racerIndex;
                if (racerIndex >= 3) {
                    temp = racerIndex - 3;
                }
            } else {
                temp = racerIndex & 3;
            }
            greenAmount = 255 - (temp << 6);

            gTrophyRankingsTitle[menuElemIndex + 2].filterGreen = greenAmount;
            if (isRankings) {
                gTrophyRankingsTitle[menuElemIndex].t.drawTexture =
                    gRacerPortraits[settings->racers[gRankingsPlayerIDs[racerIndex]].character];
                gTrophyRankingsTitle[menuElemIndex + 2].t.element = &settings->racers[gRankingsPlayerIDs[racerIndex]];
            } else {
                gTrophyRankingsTitle[menuElemIndex].t.drawTexture =
                    gRacerPortraits[settings->racers[gResultsPlayerIDs[racerIndex]].character];
                gTrophyRankingsTitle[menuElemIndex + 2].t.number = &gTrophyRacePointsArray[racerIndex];
            }
            menuElemIndex += 3;
        }
    }
    gTrophyRankingsTitle[menuElemIndex].t.asciiText = NULL;
}

/**
 * Initialise trophy race points screen.
 * Sets the order of the racer portraits based on points position.
 */
void menu_trophy_race_rankings_init(void) {
    s32 i;
    s32 j;
    s32 ranking[8];
    s32 tempForSwap;
    Settings *settings;
    s8 *trackMenuIds;

    settings = get_settings();
    trackMenuIds = (s8 *) get_misc_asset(ASSET_MISC_TRACKS_MENU_IDS);
    gMenuFlow.stage = RANKINGS_ENTER;
    gMenuInput.delay = 0;
    gMenuFlow.blinkTimer = 0;
    gMenuFlow.opacityDecayTimer = 0;
    reset_controller_sticks();
    menu_assetgroup_load(gTrophyRankingsObjectIndices);
    menu_imagegroup_load(gTrophyRaceImageIndices);
    gPrevTrophyRaceRound = gRaceSetup.trophyRaceRound;
    do {
        if (++gRaceSetup.trophyRaceRound >= 4) {
            break;
        }
    } while (trackMenuIds[((gRaceSetup.trophyRaceWorldId - 1) * 6) + gRaceSetup.trophyRaceRound] == -1);

    if (gRaceSetup.trophyRaceRound < 4) {
        gResults.optionText[0] = gMenuLoaded.text[ASSET_MENU_TEXT_CONTINUE];
        gResults.optionText[1] = gMenuLoaded.text[ASSET_MENU_TEXT_QUITTROPHYRACE];
        gResults.optionCount = 3;
    } else {
        gResults.optionText[0] = gMenuLoaded.text[ASSET_MENU_TEXT_TROPHYCEREMONY];
        gResults.optionCount = 1;
    }
    gMenuFlow.option = 0;
    menu_racer_portraits();

    if (gRaceSetup.numberOfActivePlayers > 2) {
        gRankingPlayerCount = gRaceSetup.numberOfActivePlayers;
    } else if ((gRaceSetup.numberOfActivePlayers == 2) || (is_in_two_player_adventure())) {
        gRankingPlayerCount = get_multiplayer_racer_count();
    } else {
        gRankingPlayerCount = 8;
    }
    for (i = 0; i < gRankingPlayerCount; i++) {
        for (j = 0; j < gRankingPlayerCount; j++) {
            if (i == settings->racers[j].starting_position) {
                gResultsPlayerIDs[i] = j;
                if (j < gRaceSetup.numberOfActivePlayers) {
                    gResultsPlayers[i] = TRUE;
                } else {
                    gResultsPlayers[i] = FALSE;
                }
            }
        }
    }
    for (i = 0; i < gRankingPlayerCount; i++) {
        gRankingsPoints[i] = gTrophyRacePointsArray[settings->racers[i].starting_position];
    }
    for (i = 0; i < gRankingPlayerCount; i++) {
        gRankingsPlayerIDs[i] = i;
        ranking[i] = settings->racers[i].trophy_points + gRankingsPoints[i];
    }

    for (i = gRankingPlayerCount - 1; i > 0; i--) {
        for (j = 0; j < i; j++) {
            if (ranking[j] < ranking[j + 1]) {
                tempForSwap = ranking[j];
                ranking[j] = ranking[j + 1];
                ranking[j + 1] = tempForSwap;
                tempForSwap = gRankingsPlayerIDs[j];
                gRankingsPlayerIDs[j] = gRankingsPlayerIDs[j + 1];
                gRankingsPlayerIDs[j + 1] = tempForSwap;
            }
        }
    }

    if (is_in_two_player_adventure()) {
        j = 2;
    } else {
        j = gRaceSetup.numberOfActivePlayers;
    }
    for (i = 0; i < gRankingPlayerCount; i++) {
        if (gRankingsPlayerIDs[i] < j) {
            gRankingsPlayers[i] = TRUE;
        } else {
            gRankingsPlayers[i] = FALSE;
        }
    }
    load_font(ASSET_FONTS_BIGFONT);
    music_voicelimit_set(24);
    music_play(SEQUENCE_MAIN_MENU);
    music_fade(256);
    trophy_rankings_init(0);
    postrace_offsets(gTrophyRankingsTitle, 0.5f, 20.0f, 0.5f, 0, 0);
}

/**
 * Update the player portrait highlight.
 * If the championship order is visible, render the portraits.
 */
void rankings_render_order(s32 updateRate) {
    s32 highlight;
    s32 stage;
    s32 fade;
    s32 i;
    UNUSED s32 pad;

    gMenuFlow.blinkTimer = (gMenuFlow.blinkTimer + updateRate) & 0x3F;
    highlight = gMenuFlow.blinkTimer * 8;
    if (highlight > 255) {
        highlight = 511 - highlight;
    }
    for (i = 0; i < gRankingPlayerCount; i++) {
        fade = 255;
        // Highlight human players if there are fewer than four.
        if (gRaceSetup.numberOfActivePlayers < 3 && ((gMenuFlow.stage == POSTRACE_ENTER && gResultsPlayers[i]) ||
                                                     (gMenuFlow.stage != POSTRACE_ENTER && gRankingsPlayers[i]))) {
            fade = (highlight >> 1) + 128;
        }
#ifdef AVOID_UB
        gTrophyRankingsTitle[2 + (i * 3)].filterRed = fade;
        gTrophyRankingsTitle[2 + (i * 3)].filterGreen = fade;
        gTrophyRankingsTitle[2 + (i * 3)].filterBlue = fade;
#else
        gTrophyRankingsRacers[i * 3].filterRed = fade;
        gTrophyRankingsRacers[i * 3].filterGreen = fade;
        gTrophyRankingsRacers[i * 3].filterBlue = fade;
#endif
    }
    stage = gMenuFlow.stage;
    if (stage == RANKINGS_ORDER || stage == RANKINGS_EXIT) {
        draw_menu_elements(1, gTrophyRankingsTitle, 1.0f);
    }
}

/**
 * Show the trophy race rankings.
 * Start with the finish order, add the points,
 * then order the portraits in championship order.
 */
s32 menu_trophy_race_rankings_loop(s32 updateRate) {
    s32 i;
    s32 temp6;
    s32 buttonsPressed;
    s32 prevOption;
    s32 ret;
    s8 *params;
    s32 sp34;
    s16 playSound;
    s16 temp7;
    Settings *settings;
    s32 temp0;

    ret = MENU_RESULT_CONTINUE;
    settings = get_settings();
    if (gMenuInput.delay > -20 && gMenuInput.delay < 20) {
        rankings_render_order(updateRate);
    }
    update_controller_sticks();
    switch (gMenuFlow.stage) { // gMenuFlow.stage = current Trophy Race Rankings state?
        case POSTRACE_ENTER:
            if (postrace_render(updateRate)) {
                gMenuFlow.stage = RANKINGS_SWAP;
                trophy_rankings_init(1);
                postrace_offsets(gTrophyRankingsTitle, 0.5f, 0.0f, 0.0f, 0, 0);
            }
            break;
        case RANKINGS_SWAP:
            if (postrace_render(updateRate)) {
                gMenuFlow.stage = RANKINGS_ORDER;
                draw_menu_elements(1, gTrophyRankingsTitle, 1.0f);
            }
            break;
        case RANKINGS_ORDER:
            gMenuFlow.opacityDecayTimer += updateRate;
            if (gMenuFlow.opacityDecayTimer > 10) {
                gMenuFlow.opacityDecayTimer -= 10;
                playSound = FALSE;
                for (i = 0; i < gRankingPlayerCount; i++) {
                    if (gRankingsPoints[i] > 0) {
                        gRankingsPoints[i]--;
                        playSound = TRUE;
                        settings->racers[i].trophy_points++;
                    }
                }
                if (playSound) {
                    sound_play(SOUND_TING_HIGH, NULL);
                }
            }

            buttonsPressed = 0;
            prevOption = gMenuFlow.option;
            for (i = 0; i < gRaceSetup.numberOfActivePlayers; i++) {
                buttonsPressed |= input_pressed(i);
                if (gMenuInput.yAxisDirection[i] < 0) {
                    gMenuFlow.option++;
                }
                if (gMenuInput.yAxisDirection[i] > 0) {
                    gMenuFlow.option--;
                }
            }
            if (gMenuFlow.option < 0) {
                gMenuFlow.option = 0;
            }
            if (gMenuFlow.option >= gResults.optionCount) {
                gMenuFlow.option = gResults.optionCount - 1;
            }
            if (prevOption != gMenuFlow.option) {
                sound_play(SOUND_MENU_PICK2, NULL);
            }
            if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
                music_fade(-128);
                transition_begin(&sMenuTransitionFadeIn);
                gMenuFlow.stage = RANKINGS_EXIT;
                for (i = 0; i < gRankingPlayerCount; i++) {
                    settings->racers[i].trophy_points += gRankingsPoints[i];
                }
            }
            break;
        case RANKINGS_EXIT:
            gMenuInput.delay += updateRate;
            if (gMenuInput.delay > 30) {
                rankings_free();
                dialogue_close(7);
                dialogue_clear(7);
                if (gRaceSetup.trophyRaceRound < 4) {
                    menu_init(MENU_TROPHY_RACE_ROUND);
                } else {
#ifdef AVOID_UB
                    // This value isn't set if certain conditions are met.
                    sp34 = 0;
#endif
                    for (temp6 = 0, i = 0; i < gRankingPlayerCount; i++) {
                        if (gRankingsPlayers[i] != 0) {
                            temp7 = settings->racers[gRankingsPlayerIDs[i]].character;
                            if (temp6 == 0) {
                                sp34 = i;
                                gRankingsPortraitIDs[temp6++] = temp7;
                                continue;
                            }
                            if (settings->racers[gRankingsPlayerIDs[i]].trophy_points ==
                                settings->racers[gRankingsPlayerIDs[sp34]].trophy_points) {
                                gRankingsPortraitIDs[temp6++] = temp7;
                            }
                        }
                    }

                    if (gRaceSetup.numberOfActivePlayers == 1 && !is_in_two_player_adventure()) {
                        temp6 = 0;
                    }
                    gRankingsPortraitIDs[temp6] = -1;
                    if (gRaceSetup.isInTracksMode == TRUE) {
                        if (sp34 >= 3) {
                            menu_init(MENU_TRACK_SELECT);
                        } else {
                            ret = MENU_RESULT_SWITCH(MENU_RESULT_TRACKS_MODE);
                        }
                    } else {
                        ret = MENU_RESULT_RETURN_TO_GAME;
                        settings->courseId = level_world_id(settings->worldId);
                        if (gRaceSetup.inAdvModeTrophyRace) {
                            gRaceSetup.inAdvModeTrophyRace = FALSE;
                            ret = MENU_RESULT_START_LEVEL(settings->courseId);
                            if (sp34 < 3) {
                                temp0 = settings->worldId - 1;
                                temp0 <<= 1;
                                prevOption = ((3 - sp34) & 3);
                                temp6 = (settings->trophies >> temp0) & 3;
                                if (temp6 < prevOption) {
                                    settings->trophies &= ~(3 << temp0);
                                    settings->trophies |= (prevOption << temp0);
                                    safe_mark_write_save_file(get_save_file_index());
                                }
                            }
                        }
                    }
                    if (sp34 < 3) {
                        params = (s8 *) get_misc_asset(ASSET_MISC_CINEMATIC_TROPHY);
                        temp0 = ((gRaceSetup.trophyRaceWorldId * 3) + sp34) - 3;
                        cinematic_start(params, temp0, ret, 0, 0, gRankingsPortraitIDs);
                        ret = MENU_RESULT_CONTINUE;
                        menu_init(MENU_NEWGAME_CINEMATIC);
                    }
                    gRaceSetup.trophyRaceWorldId = 0;
                }
            }
            break;
    }
    gMenuInput.ignoreInputTime = 0;
    return ret;
}

/**
 * Free all assets associated with the trophy race rankings screen.
 */
void rankings_free(void) {
    menu_assetgroup_free(gTrophyRankingsObjectIndices);
    unload_font(ASSET_FONTS_BIGFONT);
}

/**
 * Return the hub world ID of the trophy race.
 */
s32 get_trophy_race_world_id(void) {
    return gRaceSetup.trophyRaceWorldId;
}
