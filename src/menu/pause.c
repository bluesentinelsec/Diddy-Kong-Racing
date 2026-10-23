#include "host.h"
#include "menu_internal.h"

static char *gMenuOptionText[8]; // Menu Text

// Index of the last player who paused.
static s32 gLastPlayerWhoPaused = 0;

static ColourRGBA gPlayerPauseBgColour[4] = {
    { { { 64, 64, 255, 160 } } },  // Blue for player 1
    { { { 255, 64, 64, 160 } } },  // Red for player 2
    { { { 208, 192, 32, 176 } } }, // Yellow for player 3
    { { { 32, 192, 64, 176 } } },  // Green for player 4
};

static ColourRGBA gPlayerPauseOptionsTextColour[4] = {
    { { { 0, 255, 0, 128 } } }, // Green for Player 1
    { { { 0, 255, 0, 96 } } },  // Green for Player 2
    { { { 0, 0, 255, 96 } } },  // Blue for Player 3
    { { { 0, 0, 255, 128 } } }, // Blue for Player 4
};

/**
 * Sets up the pause menu options based on current map ID and mode.
 * Also marks whoever triggered the pause, so only they can have input.
 */
void menu_pause_init(void) {
    s32 raceType;
    s32 i;
    Settings *settings;

    rumble_init(FALSE);
    settings = get_settings();
    gLastPlayerWhoPaused = -1;

    for (i = 0; (get_active_player_count() > i && gLastPlayerWhoPaused < 0); i++) {
        if (input_held(i) & START_BUTTON) {
            gLastPlayerWhoPaused = i;
        }
    }
    if (gLastPlayerWhoPaused < 0) {
        gLastPlayerWhoPaused = 0;
    }
    gMenuOptionText[0] = gMenuLoaded.text[ASSET_MENU_TEXT_CONTINUE];
    gMenuFlow.optionCap = 1;
    if (gRaceSetup.trophyRaceWorldId == 0) {
        raceType = leveltable_type(settings->courseId);
        if (settings->worldId == WORLD_CENTRAL_AREA && is_taj_challenge()) {
            gMenuOptionText[1] = gMenuLoaded.text[ASSET_MENU_TEXT_ABANDONCHALLENGE];
            gMenuFlow.optionCap = 2;
        } else if (settings->worldId > WORLD_CENTRAL_AREA && raceType != RACETYPE_BOSS) {
            if (raceType & RACETYPE_CHALLENGE) {
                gMenuOptionText[1] = gMenuLoaded.text[ASSET_MENU_TEXT_RESTARTCHALLENGE];
                gMenuOptionText[2] = (!gRaceSetup.isInTracksMode) ? gMenuLoaded.text[ASSET_MENU_TEXT_RETURNTOLOBBY]
                                                                  : gMenuLoaded.text[ASSET_MENU_TEXT_SELECTTRACK];
                gMenuFlow.optionCap = 3;
            } else if (raceType == RACETYPE_DEFAULT) {
                gMenuOptionText[1] = gMenuLoaded.text[ASSET_MENU_TEXT_RESTARTRACE];
                gMenuOptionText[2] = (!gRaceSetup.isInTracksMode) ? gMenuLoaded.text[ASSET_MENU_TEXT_RETURNTOLOBBY]
                                                                  : gMenuLoaded.text[ASSET_MENU_TEXT_SELECTTRACK];
                gMenuFlow.optionCap = 3;
            }
        } else if (gRaceSetup.isInTracksMode == FALSE && raceType == RACETYPE_BOSS) {
            gMenuOptionText[1] = gMenuLoaded.text[ASSET_MENU_TEXT_RESTARTRACE];
            gMenuOptionText[2] = gMenuLoaded.text[ASSET_MENU_TEXT_RETURNTOLOBBY];
            gMenuFlow.optionCap = 3;
        }
        if (gRaceSetup.isInTracksMode == TRUE && gRaceSetup.numberOfActivePlayers == 1) {
            gMenuOptionText[gMenuFlow.optionCap++] = gMenuLoaded.text[ASSET_MENU_TEXT_SELECTCHARACTER];
        }
        gMenuOptionText[gMenuFlow.optionCap++] = gMenuLoaded.text[ASSET_MENU_TEXT_QUITGAME];
    } else {
        gMenuOptionText[gMenuFlow.optionCap++] = gMenuLoaded.text[ASSET_MENU_TEXT_QUITTROPHYRACE];
    }
    gMenuFlow.option = 0;
    gMenuFlow.blinkTimer = 0;
    gMenuInput.delay = 0;
    gMenuInput.ignoreInputTime = 1;
    gMenuFlow.subOption = 0;
    reset_controller_sticks();
}

/**
 * Render the text box and each individual option in the pause menu.
 */
void pausemenu_render(UNUSED s32 updateRate) {
    ColourRGBA *colour;
    s32 halfTemp;
    s32 halfX;
    s32 x;
    s32 temp;
    s32 textWidth;
    s32 i;
    s32 y;
    s32 alpha;

    for (i = 0, x = SCREEN_WIDTH_HALF; i < gMenuFlow.optionCap; i++) {
        textWidth = get_text_width(gMenuOptionText[i], 0, 0) + 8;
        if (x < textWidth) {
            x = textWidth;
        }
    }
    temp = (gMenuFlow.optionCap * 16) + 28;
    if (osTvType == OS_TV_TYPE_PAL) {
        y = SCREEN_HEIGHT_HALF_PAL;
    } else {
        y = SCREEN_HEIGHT_HALF;
    }
    clear_dialogue_box_open_flag(7);
    dialogue_clear(7);
    halfTemp = temp >> 1;
    halfX = x >> 1;
    set_current_dialogue_box_coords(7, SCREEN_WIDTH_HALF - halfX, y - halfTemp, SCREEN_WIDTH_HALF + halfX,
                                    halfTemp + y);
    colour = &gPlayerPauseBgColour[input_player_id(gLastPlayerWhoPaused)];
    set_current_dialogue_background_colour(7, colour->r, colour->g, colour->b, colour->a);
    set_dialogue_font(7, ASSET_FONTS_FUNFONT);
    set_current_text_background_colour(7, 128, 128, 255, 0);
    colour = &gPlayerPauseOptionsTextColour[input_player_id(gLastPlayerWhoPaused)];
    set_current_text_colour(7, colour->r, colour->g, colour->b, colour->a, 255);
    alpha = gMenuFlow.blinkTimer * 8;
    if (alpha > 255) {
        alpha = 511 - alpha;
    }
    if (gMenuFlow.subOption != 0) {
        i = halfTemp - 26;
        if (gRaceSetup.trophyRaceWorldId != 0) {
            render_dialogue_text(7, POS_CENTRED, i + 8, gMenuLoaded.text[ASSET_MENU_TEXT_QUITTROPHYRACETITLE], 1,
                                 ALIGN_MIDDLE_CENTER);
        } else {
            render_dialogue_text(7, POS_CENTRED, i + 8, gMenuLoaded.text[ASSET_MENU_TEXT_QUITGAMETITLE], 1,
                                 ALIGN_MIDDLE_CENTER);
        }
        if (gMenuFlow.subOption == 1) {
            set_current_text_colour(7, 255, 255, 255, alpha, 255);
        } else {
            set_current_text_colour(7, 255, 255, 255, 0, 255);
        }
        render_dialogue_text(7, POS_CENTRED, i + 28, gMenuLoaded.text[ASSET_MENU_TEXT_OK], 1, ALIGN_MIDDLE_CENTER);
        if (gMenuFlow.subOption == 2) {
            set_current_text_colour(7, 255, 255, 255, alpha, 255);
        } else {
            set_current_text_colour(7, 255, 255, 255, 0, 255);
        }
        render_dialogue_text(7, POS_CENTRED, i + 44, gMenuLoaded.text[ASSET_MENU_TEXT_CANCEL], 1, ALIGN_MIDDLE_CENTER);
    } else {
        i = gLastPlayerWhoPaused + 1; // Fakematch. Seems to fix stuff?
        render_dialogue_text(7, POS_CENTRED, 12, gMenuLoaded.text[ASSET_MENU_TEXT_PAUSEOPTIONS],
                             gLastPlayerWhoPaused + 1, ALIGN_MIDDLE_CENTER);
        for (i = 0, y = 32; i < gMenuFlow.optionCap; i++, y += 16) {
            if (i == gMenuFlow.option) {
                set_current_text_colour(7, 255, 255, 255, alpha, 255);
            } else {
                set_current_text_colour(7, 255, 255, 255, 0, 255);
            }
            render_dialogue_text(7, POS_CENTRED, y, gMenuOptionText[i], 1, ALIGN_MIDDLE_CENTER);
        }
    }
    open_dialogue_box(7);
}

/**
 * Show the available options when the player pauses the game.
 * Returns a different behaviour value depending on what's selected,
 * making the main game loop do different things.
 */
s32 menu_pause_loop(UNUSED Gfx **dl, s32 updateRate) {
    s8 temp;
    s32 playerId;
    s32 buttonsPressed;

    if (gMenuFlow.optionCap == 0) {
        sound_volume_change(VOLUME_NORMAL);
        return PAUSE_NONE;
    }

    sound_volume_change(VOLUME_LOWER);
    gMenuFlow.blinkTimer = (gMenuFlow.blinkTimer + updateRate) & 0x3F;
    update_controller_sticks();

    buttonsPressed = 0;
    if (gMenuInput.ignoreInputTime == 0) {
        buttonsPressed = input_pressed(gLastPlayerWhoPaused);
    }

    if (gMenuInput.delay == 0) {
        if (gMenuFlow.subOption != 0) {
            if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
                sound_play(SOUND_SELECT2, NULL);
                if (gMenuFlow.subOption == 1) {
                    gMenuInput.delay = 1;
                } else {
                    gMenuFlow.subOption = 0;
                }
            } else if (buttonsPressed & B_BUTTON) {
                sound_play(SOUND_SELECT2, NULL);
                gMenuFlow.subOption = 0;
            } else {
                temp = gMenuFlow.subOption;
                playerId = gLastPlayerWhoPaused;
                if (gMenuInput.yAxisDirection[playerId] != 0) {
                    gMenuFlow.subOption = 3 - gMenuFlow.subOption;
                }
                if (temp != gMenuFlow.subOption) {
                    sound_play(SOUND_MENU_PICK2, NULL);
                }
            }
        } else if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
            sound_play(SOUND_SELECT2, NULL);
            if (gMenuOptionText[gMenuFlow.option] == gMenuLoaded.text[ASSET_MENU_TEXT_QUITGAME] ||
                (gRaceSetup.trophyRaceWorldId != 0 &&
                 gMenuOptionText[gMenuFlow.option] == gMenuLoaded.text[ASSET_MENU_TEXT_QUITTROPHYRACE])) {
                gMenuFlow.subOption = 2;
            } else {
                gMenuInput.delay = 1;
            }
        } else {
            temp = gMenuFlow.option;
            playerId = gLastPlayerWhoPaused;
            buttonsPressed = gMenuInput.yAxisDirection[playerId];
            if (buttonsPressed < 0) {
                gMenuFlow.option++;
            }
            if (buttonsPressed > 0) {
                gMenuFlow.option--;
            }
            if (gMenuFlow.option < 0) {
                gMenuFlow.option = gMenuFlow.optionCap - 1;
            }
            if (gMenuFlow.option >= gMenuFlow.optionCap) {
                gMenuFlow.option = 0;
            }
            if (temp != gMenuFlow.option) {
                sound_play(SOUND_MENU_PICK2, NULL);
            }
        }
    } else {
        gMenuInput.delay++;
        if (gMenuInput.delay >= 4) {
            menu_dialogue_end();
            if (gMenuFlow.subOption == 1) {
                if (gRaceSetup.trophyRaceWorldId != 0) {
                    gRaceSetup.trophyRaceWorldId = 0;
                    if (gRaceSetup.isInTracksMode == FALSE) {
                        return PAUSE_QUIT_LOBBY;
                    }
                    return PAUSE_QUIT_TRACKS;
                }
                return PAUSE_OPT_4;
            }
            if (gMenuOptionText[gMenuFlow.option] == gMenuLoaded.text[ASSET_MENU_TEXT_CONTINUE]) {
                sound_volume_change(VOLUME_NORMAL);
                return PAUSE_CONTINUE;
            }
            if (gMenuOptionText[gMenuFlow.option] == gMenuLoaded.text[ASSET_MENU_TEXT_RESTARTRACE] ||
                gMenuOptionText[gMenuFlow.option] == gMenuLoaded.text[ASSET_MENU_TEXT_RESTARTCHALLENGE]) {
                if (gRaceSetup.isInTracksMode == FALSE && gTTVoiceLines[get_ingame_map_id()] != -1) {
                    sound_play_delayed(gTTVoiceLines[get_ingame_map_id()], NULL, 1.0f);
                }
                return PAUSE_RESET;
            }
            if (gMenuOptionText[gMenuFlow.option] == gMenuLoaded.text[ASSET_MENU_TEXT_RETURNTOLOBBY]) {
                return PAUSE_QUIT_LOBBY;
            }
            if (gMenuOptionText[gMenuFlow.option] == gMenuLoaded.text[ASSET_MENU_TEXT_SELECTTRACK]) {
                return PAUSE_QUIT_TRACKS;
            }
            if (gMenuOptionText[gMenuFlow.option] == gMenuLoaded.text[ASSET_MENU_TEXT_SELECTCHARACTER]) {
                return PAUSE_QUIT_CHARSELECT;
            }
            if (gMenuOptionText[gMenuFlow.option] == gMenuLoaded.text[ASSET_MENU_TEXT_ABANDONCHALLENGE]) {
                sound_volume_change(VOLUME_NORMAL);
                return PAUSE_QUIT_CHALLENGE;
            }
            return PAUSE_CONTINUE;
        }
    }

    pausemenu_render(updateRate);
    gMenuInput.ignoreInputTime = 0;
    return PAUSE_NONE;
}
