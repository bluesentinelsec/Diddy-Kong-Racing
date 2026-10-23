#include "menu_internal.h"

static s16 gCharSelectObjectIndices[2] = { -1, 0 };
static s16 gCharSelectImageIndices[2] = { -1, 0 };

// Default character select screen with the initial 8 characters.
/*    Name         Up                  Down                Left                                Right Voice ID */
static CharacterSelectData gCharacterSelectBytesDefault[] = {
    /*Krunch*/ { { NONE, NONE }, { CONKER, NONE }, { NONE, NONE, NONE, NONE }, { DIDDY, BUMPER, BANJO, NONE }, 0x0000 },
    /*Diddy*/ { { NONE, NONE }, { TIPTUP, NONE }, { KRUNCH, NONE, NONE, NONE }, { BUMPER, BANJO, NONE, NONE }, 0x0009 },
    /*Bumper*/ { { NONE, NONE }, { PIPSY, NONE }, { DIDDY, KRUNCH, NONE, NONE }, { BANJO, NONE, NONE, NONE }, 0x0001 },
    /*Banjo*/ { { NONE, NONE }, { TIMBER, NONE }, { BUMPER, DIDDY, KRUNCH, NONE }, { NONE, NONE, NONE, NONE }, 0x0005 },
    /*Conker*/
    { { KRUNCH, NONE }, { NONE, NONE }, { NONE, NONE, NONE, NONE }, { TIPTUP, PIPSY, TIMBER, NONE }, 0x0003 },
    /*Tiptup*/ { { DIDDY, NONE }, { NONE, NONE }, { CONKER, NONE, NONE, NONE }, { PIPSY, TIMBER, NONE, NONE }, 0x0002 },
    /*Pipsy*/
    { { BUMPER, NONE }, { NONE, NONE }, { TIPTUP, CONKER, NONE, NONE }, { TIMBER, NONE, NONE, NONE }, 0x0007 },
    /*Timber*/ { { BANJO, NONE }, { NONE, NONE }, { PIPSY, TIPTUP, CONKER, NONE }, { NONE, NONE, NONE, NONE }, 0x0004 }
};

// Drumstick is unlocked, but T.T is not.
/*    Name            Up                     Down                Left                                  Right Voice ID */
static CharacterSelectData gCharacterSelectBytesDrumStick[] = {
    /*Krunch*/ {
        { NONE, NONE }, { CONKER, NONE }, { NONE, NONE, NONE, NONE }, { DIDDY, DRUMSTICK, BUMPER, BANJO }, 0x0000 },
    /*Diddy*/
    { { NONE, NONE }, { CONKER, TIPTUP }, { KRUNCH, NONE, NONE, NONE }, { DRUMSTICK, BUMPER, BANJO, NONE }, 0x0009 },
    /*Bumper*/
    { { NONE, NONE }, { PIPSY, TIMBER }, { DRUMSTICK, DIDDY, KRUNCH, NONE }, { BANJO, NONE, NONE, NONE }, 0x0001 },
    /*Banjo*/
    { { NONE, NONE }, { TIMBER, NONE }, { BUMPER, DRUMSTICK, DIDDY, KRUNCH }, { NONE, NONE, NONE, NONE }, 0x0005 },
    /*Conker*/
    { { KRUNCH, DIDDY }, { NONE, NONE }, { NONE, NONE, NONE, NONE }, { TIPTUP, PIPSY, TIMBER, NONE }, 0x0003 },
    /*Tiptup*/
    { { DIDDY, DRUMSTICK }, { NONE, NONE }, { CONKER, NONE, NONE, NONE }, { PIPSY, TIMBER, NONE, NONE }, 0x0002 },
    /*Pipsy*/
    { { DRUMSTICK, BUMPER }, { NONE, NONE }, { TIPTUP, CONKER, NONE, NONE }, { TIMBER, NONE, NONE, NONE }, 0x0007 },
    /*Timber*/
    { { BUMPER, BANJO }, { NONE, NONE }, { PIPSY, TIPTUP, CONKER, NONE }, { NONE, NONE, NONE, NONE }, 0x0004 },
    /*Drumstick*/
    { { NONE, NONE }, { TIPTUP, PIPSY }, { DIDDY, KRUNCH, NONE, NONE }, { BUMPER, BANJO, NONE, NONE }, 0x0006 }
};

// Under the unlikely chance you have T.T unlocked, but not Drumstick.
/*    Name         Up                 Down                  Left                                 Right Voice ID */
static CharacterSelectData gCharacterSelectBytesTT[] = {
    /*Krunch*/ { { NONE, NONE }, { CONKER, NONE }, { NONE, NONE, NONE, NONE }, { DIDDY, BUMPER, BANJO, NONE }, 0x0000 },
    /*Diddy*/
    { { NONE, NONE }, { TIPTUP, TICTOC_8 }, { KRUNCH, NONE, NONE, NONE }, { BUMPER, BANJO, NONE, NONE }, 0x0009 },
    /*Bumper*/
    { { NONE, NONE }, { TICTOC_8, PIPSY }, { DIDDY, KRUNCH, NONE, NONE }, { BANJO, NONE, NONE, NONE }, 0x0001 },
    /*Banjo*/
    { { NONE, NONE }, { PIPSY, TIMBER }, { BUMPER, DIDDY, KRUNCH, NONE }, { NONE, NONE, NONE, NONE }, 0x0005 },
    /*Conker*/
    { { KRUNCH, NONE }, { NONE, NONE }, { NONE, NONE, NONE, NONE }, { TIPTUP, TICTOC_8, PIPSY, TIMBER }, 0x0003 },
    /*Tiptup*/
    { { KRUNCH, DIDDY }, { NONE, NONE }, { CONKER, NONE, NONE, NONE }, { TICTOC_8, PIPSY, TIMBER, NONE }, 0x0002 },
    /*Pipsy*/
    { { BUMPER, BANJO }, { NONE, NONE }, { TICTOC_8, TIPTUP, CONKER, NONE }, { TIMBER, NONE, NONE, NONE }, 0x0007 },
    /*Timber*/
    { { BANJO, NONE }, { NONE, NONE }, { PIPSY, TICTOC_8, TIPTUP, CONKER }, { NONE, NONE, NONE, NONE }, 0x0004 },
    /*T.T*/ { { DIDDY, BUMPER }, { NONE, NONE }, { TIPTUP, CONKER, NONE, NONE }, { PIPSY, TIMBER, NONE, NONE }, 0x0008 }
};

// With everyone unlocked.
/*    Name            Up                   Down                Left                                 Right Voice ID */
static CharacterSelectData gCharacterSelectBytesComplete[] = {
    /*Krunch*/ {
        { NONE, NONE }, { CONKER, NONE }, { NONE, NONE, NONE, NONE }, { DIDDY, DRUMSTICK, BUMPER, BANJO }, 0x0000 },
    /*Diddy*/
    { { NONE, NONE }, { TIPTUP, NONE }, { KRUNCH, NONE, NONE, NONE }, { DRUMSTICK, BUMPER, BANJO, NONE }, 0x0009 },
    /*Bumper*/
    { { NONE, NONE }, { PIPSY, NONE }, { DRUMSTICK, DIDDY, KRUNCH, NONE }, { BANJO, NONE, NONE, NONE }, 0x0001 },
    /*Banjo*/
    { { NONE, NONE }, { TIMBER, NONE }, { BUMPER, DRUMSTICK, DIDDY, KRUNCH }, { NONE, NONE, NONE, NONE }, 0x0005 },
    /*Conker*/
    { { KRUNCH, NONE }, { NONE, NONE }, { NONE, NONE, NONE, NONE }, { TIPTUP, TICTOC_9, PIPSY, TIMBER }, 0x0003 },
    /*Tiptup*/
    { { DIDDY, NONE }, { NONE, NONE }, { CONKER, NONE, NONE, NONE }, { TICTOC_9, PIPSY, TIMBER, NONE }, 0x0002 },
    /*Pipsy*/
    { { BUMPER, NONE }, { NONE, NONE }, { TICTOC_9, TIPTUP, CONKER, NONE }, { TIMBER, NONE, NONE, NONE }, 0x0007 },
    /*Timber*/
    { { BANJO, NONE }, { NONE, NONE }, { PIPSY, TICTOC_9, TIPTUP, CONKER }, { NONE, NONE, NONE, NONE }, 0x0004 },
    /*Drumstick*/
    { { NONE, NONE }, { TICTOC_9, NONE }, { DIDDY, KRUNCH, NONE, NONE }, { BUMPER, BANJO, NONE, NONE }, 0x0006 },
    /*T.T*/
    { { DRUMSTICK, NONE }, { TIPTUP, NONE }, { TIPTUP, CONKER, NONE, NONE }, { PIPSY, TIMBER, NONE, NONE }, 0x0008 }
    //!@bug T.T's down input selects Tiptup. It should be set to NONE.
};

// Set from charselect_prev()
// Is either 0, 1, or 2. However it is never set to 2?
// Set to 0 upon entering the Character Select menu normally.
// Set to 1 upon entering the Char Select from the
//   "Select Character" option on the "Pause Options" screen in a race.
static s32 gEnteredCharSelectFrom = 0;

// Set from charselect_prev()
// Set to the value *arg1 when arg0 is 2, but that never happens.
// Not read from anywhere, so I'd consider this to be unused.
static UNUSED s32 unused_800DFFD4 = -1;

/**
 * Set the previous menu type for the character select.
 * This ensures when leaving the character select menu, you return to where you were.
 */
void charselect_prev(s32 context, UNUSED s32 *arg1) {
    switch (context) {
        default:
            gEnteredCharSelectFrom = 0;
            return;
        case 1:
        case 3: // Take you straight back to tracks mode.
            gEnteredCharSelectFrom = 1;
            return;
        case 2: // Unused
            gEnteredCharSelectFrom = 2;
            unused_800DFFD4 = *arg1;
            return;
    }
}

/**
 * Initialises the character select menu. Checks which characters are available and sets the vars used for selection.
 * Sets up sound channels with the background music based on which characters are selected.
 */
void menu_character_select_init(void) {
    s32 breakTheLoop;
    s32 i;
    u8 *channelVolumes;

    breakTheLoop = FALSE;
    input_assign_players();
    if (is_drumstick_unlocked()) {
        if (is_tt_unlocked()) {
            gCurrCharacterSelectData = (CharacterSelectData(*)[10]) & gCharacterSelectBytesComplete;
        } else {
            gCurrCharacterSelectData = (CharacterSelectData(*)[10]) & gCharacterSelectBytesDrumStick;
        }
    } else if (is_tt_unlocked()) {
        gCurrCharacterSelectData = (CharacterSelectData(*)[10]) & gCharacterSelectBytesTT;
    } else {
        gCurrCharacterSelectData = (CharacterSelectData(*)[10]) & gCharacterSelectBytesDefault;
    }
    for (i = 0; i < ARRAY_COUNT(gCharselectStatus); i++) {
        gCharselectStatus[i] = CHARSELECT_STATUS_UNCONFIRMED;
    }
    gNumberOfReadyPlayers = 0;
    gMenuDelay = 0;
    gOptionBlinkTimer = 0;
    for (i = 0; i < ARRAY_COUNT(gActivePlayersArray) && !breakTheLoop; i++) {
        if (gActivePlayersArray[i] != 0) {
            breakTheLoop = TRUE;
            gMenuCurrentCharacter.channelIndex = (*gCurrCharacterSelectData)[gPlayersCharacterArray[i]].voiceID;
            gMenuCurrentCharacter.volume = 0x7F;
            gMenuCurrentCharacter.switchDelay = 1;
        }
    }
    gMenuSelectedCharacter.channelIndex = -1;
    gMenuSelectedCharacter.volume = 0;
    gMenuSelectedCharacter.switchDelay = 0;
    sSoundOptionsState.channelIndex = -1;
    sSoundOptionsState.volume = 0;
    sSoundOptionsState.switchDelay = 0;
    music_play(SEQUENCE_CHOOSE_YOUR_RACER);
    for (i = 0; i < NUM_CHARACTERS; i++) {
        channelVolumes = gCharacterVolumes[i];
        if (i != gMenuCurrentCharacter.channelIndex) {
            music_channel_off(channelVolumes[0]);
            music_channel_off(channelVolumes[1]);
        }
    }
    music_channel_off(6);
    music_change_off();
    menu_assetgroup_load(gCharSelectObjectIndices);
    menu_imagegroup_load(gCharSelectImageIndices);
    transition_begin(&sMenuTransitionFadeOut);
    load_font(ASSET_FONTS_BIGFONT);
}

/**
 * Draws the "Player Select" and "OK?" text in the character select menu.
 */
void charselect_render_text(UNUSED s32 updateRate) {
    s32 yPos;
    if (gMenuDelay > -23 && gMenuDelay < 23) {
        set_text_font(ASSET_FONTS_BIGFONT);
        set_text_background_colour(0, 0, 0, 0);
        set_text_colour(0, 0, 0, 255, 128);
        // Draw "Player Select" text drop shadow
        draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF + 1, 35, gMenuText[ASSET_MENU_TEXT_PLAYERSELECT],
                  ALIGN_MIDDLE_CENTER);
        set_text_colour(255, 255, 255, 0, 255);
        // Draw "Player Select" text
        draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF, 32, gMenuText[ASSET_MENU_TEXT_PLAYERSELECT],
                  ALIGN_MIDDLE_CENTER);
        if (gNumberOfReadyPlayers == gNumberOfActivePlayers && gNumberOfActivePlayers > 0) {
            yPos = 208;
            if (osTvType == OS_TV_TYPE_PAL) {
                yPos = 234;
            }
            draw_text(&sMenuCurrDisplayList, SCREEN_WIDTH_HALF, yPos, "OK?", ALIGN_MIDDLE_CENTER);
        }
        rendermode_reset(&sMenuCurrDisplayList);
        cam_set_fov(40.0f);
    }
}

/**
 * If a controller not already with control presses Start or A,
 * they get assigned a character in the menu to select.
 */
void charselect_new_player(void) {
    s32 i;
    s32 j;
    s32 takenChar;
    s32 var_a2;

    for (i = 0; i < MAXCONTROLLERS; i++) {
        if (!gActivePlayersArray[i]) {
            var_a2 = -1;
            if (gMenuButtons[i] & (A_BUTTON | START_BUTTON)) {
                do {
                    var_a2++;
                    takenChar = TRUE;
                    for (j = 0; (j < ARRAY_COUNT(gActivePlayersArray)) && takenChar; j++) {
                        if (gActivePlayersArray[j] && var_a2 == gPlayersCharacterArray[j]) {
                            takenChar = FALSE;
                        }
                    }
                } while (takenChar == FALSE);
                gPlayersCharacterArray[i] = var_a2;
                gActivePlayersArray[i] = TRUE;
                gNumberOfActivePlayers++;
                gMenuCurrentCharacter.channelIndex = (*gCurrCharacterSelectData)[gPlayersCharacterArray[i]].voiceID;
                gMenuCurrentCharacter.volume = 0;
                gMenuCurrentCharacter.switchDelay = 20;
                sound_play(SOUND_SELECT2, NULL);
            }
        }
    }
}

/**
 * Mark the selected character as claimed and play their character selection sound.
 * Alternatively, if the B button was pressed, unmark the character and play their deselection sound.
 */
void charselect_pick(void) {
    s32 i;
    s32 characterSelected;
    s32 buttonsPressedAllPlayers;

    buttonsPressedAllPlayers = 0;
    for (i = 0; i < ARRAY_COUNT(gActivePlayersArray); i++) {
        if (gActivePlayersArray[i]) {
            buttonsPressedAllPlayers |= gMenuButtons[i];
        }
    }

    if (buttonsPressedAllPlayers & (A_BUTTON | START_BUTTON)) {
        // Character Selected
        gMenuDelay = 1;
        mark_read_all_save_files();
        transition_begin(&sMenuTransitionFadeIn);
        characterSelected = -1;
        //!@bug: This loop condition is doing a bitwise & instead of a boolean &&
        for (i = 0; (i < MAXCONTROLLERS) & (characterSelected < 0); i++) {
            if (gMenuButtons[i] & (A_BUTTON | START_BUTTON)) {
                characterSelected = i;
            }
        }
        if (gMenuSoundMasks[characterSelected] != 0) {
            sndp_stop(gMenuSoundMasks[characterSelected]);
        }
        sound_play((*gCurrCharacterSelectData)[gPlayersCharacterArray[characterSelected]].voiceID +
                       SOUND_VOICE_CHARACTER_SELECTED,
                   &gMenuSoundMasks[characterSelected]);
        if (gNumberOfActivePlayers > 2 ||
            (gNumberOfActivePlayers > 1 && !(gActiveMagicCodes & CHEAT_TWO_PLAYER_ADVENTURE)) ||
            gEnteredCharSelectFrom == 1) {
            music_fade(-128);
        }
    } else {
        // Character Deselected
        for (i = 0; i < ARRAY_COUNT(gActivePlayersArray); i++) {
            if (gActivePlayersArray[i] && gCharselectStatus[i] != CHARSELECT_STATUS_UNCONFIRMED) {
                if (gMenuButtons[i] & B_BUTTON) {
                    gCharselectStatus[i] = CHARSELECT_STATUS_UNCONFIRMED;
                    gNumberOfReadyPlayers--;
                    if (gMenuSoundMasks[i] != 0) {
                        sndp_stop(gMenuSoundMasks[i]);
                    }
                    sound_play(((*gCurrCharacterSelectData)[gPlayersCharacterArray[i]].voiceID +
                                SOUND_VOICE_CHARACTER_DESELECTED),
                               &gMenuSoundMasks[i]);
                }
            }
        }
    }
}

/**
 * Handle input for existing controllers on the character select menu.
 * Pressing A marks the character as claimed, and attempting to move the cursor
 * where it can't will make a sound communicating that.
 */
void charselect_input(s8 *activePlayers) {
    CharacterSelectData *charSelectData;
    s32 found;
    s32 i;
    s32 j;

    for (i = 0; i < MAXCONTROLLERS; i++) {
        if (activePlayers[i] != 0) {
            if (gCharselectStatus[i] != CHARSELECT_STATUS_UNCONFIRMED) {
                if (gMenuButtons[i] & B_BUTTON) {
                    gCharselectStatus[i] = CHARSELECT_STATUS_UNCONFIRMED;
                    gNumberOfReadyPlayers -= 1;
                    if (gMenuSoundMasks[i] != 0) {
                        sndp_stop(gMenuSoundMasks[i]);
                    }
                    sound_play(((*gCurrCharacterSelectData)[gPlayersCharacterArray[i]].voiceID +
                                SOUND_VOICE_CHARACTER_DESELECTED),
                               &gMenuSoundMasks[i]);
                }
            } else {
                if (gMenuButtons[i] & B_BUTTON) {
                    gNumberOfActivePlayers--;
                    gActivePlayersArray[i] = 0;
                    if (gNumberOfActivePlayers > 0) {
                        if (gMenuSelectedCharacter.channelIndex ==
                            (*gCurrCharacterSelectData)[gPlayersCharacterArray[i]].voiceID) {
                            if (gMenuCurrentCharacter.switchDelay <= 0) {
                                for (found = FALSE, j = 0; j < ARRAY_COUNT(gActivePlayersArray) && !found; j++) {
                                    if (gActivePlayersArray[j]) {
                                        found = TRUE;
                                        gMenuCurrentCharacter.channelIndex =
                                            (*gCurrCharacterSelectData)[gPlayersCharacterArray[j]].voiceID;
                                        gMenuCurrentCharacter.volume = 0;
                                        gMenuCurrentCharacter.switchDelay = 20;
                                    }
                                }
                            }
                        }
                    }
                    gPlayersCharacterArray[i] = -1;
                    if (gNumberOfActivePlayers <= 0) {
                        gMenuDelay = -1;
                        transition_begin(&sMenuTransitionFadeIn);
                    }
                } else if (gMenuButtons[i] & (A_BUTTON | START_BUTTON)) {
                    gCharselectStatus[i] = CHARSELECT_STATUS_CONFIRMED;
                    gNumberOfReadyPlayers++;
                    if (gMenuSoundMasks[i] != 0) {
                        sndp_stop(gMenuSoundMasks[i]);
                    }
                    sound_play(
                        ((*gCurrCharacterSelectData)[gPlayersCharacterArray[i]].voiceID + SOUND_VOICE_CHARACTER_SELECT),
                        &gMenuSoundMasks[i]);
                } else {
                    charSelectData = (*gCurrCharacterSelectData) + gPlayersCharacterArray[i];
                    if (gMenuStickY[i] > 0) {
                        charselect_move(i, charSelectData->upInput, ARRAY_COUNT(charSelectData->upInput),
                                        SOUND_MENU_PICK3, SOUND_HORN_DRUMSTICK);
                    } else if (gMenuStickY[i] < 0) {
                        charselect_move(i, charSelectData->downInput, ARRAY_COUNT(charSelectData->downInput),
                                        SOUND_MENU_PICK3, SOUND_HORN_DRUMSTICK);
                    } else if (gMenuStickX[i] < 0) {
                        charselect_move(i, charSelectData->rightInput, ARRAY_COUNT(charSelectData->rightInput),
                                        SOUND_MENU_PICK3, SOUND_HORN_DRUMSTICK);
                    } else if (gMenuStickX[i] > 0) {
                        charselect_move(i, charSelectData->leftInput, ARRAY_COUNT(charSelectData->leftInput),
                                        SOUND_MENU_PICK3, SOUND_HORN_DRUMSTICK);
                    }
                    if (charSelectData->voiceID != (*gCurrCharacterSelectData)[gPlayersCharacterArray[i]].voiceID) {
                        gMenuCurrentCharacter.channelIndex =
                            (*gCurrCharacterSelectData)[gPlayersCharacterArray[i]].voiceID;
                        gMenuCurrentCharacter.volume = 0;
                        gMenuCurrentCharacter.switchDelay = 20;
                    }
                }
            }
        }
    }
}

/**
 * First ensures that one of the AI players is Diddy Kong.
 * Afterwards, assigns the rest of the character slots a random character.
 * Keeps looping until every character ID is unique.
 */
void charselect_assign_ai(s32 charSlot) {
    s32 foundIt;
    s32 i, j;
    s32 numCharacters;

    numCharacters = 7;
    if (is_drumstick_unlocked()) {
        numCharacters++;
    }
    if (is_tt_unlocked()) {
        numCharacters++;
    }

    if (charSlot < 8) {
        foundIt = FALSE;
        for (i = 0; i < charSlot && !foundIt; i++) {
            if (gCharacterIdSlots[i] == CHARACTER_DIDDY) { // I think 9 is the character id for Diddy Kong?
                foundIt = TRUE;
            }
        }
        if (!foundIt) {
            gCharacterIdSlots[charSlot] = CHARACTER_DIDDY;
            charSlot++;
        }
    }

    for (i = charSlot; i < 8; i++) {
        do {
            gCharacterIdSlots[i] = (*gCurrCharacterSelectData)[rand_range(0, numCharacters)].voiceID;
            for (j = 0, foundIt = FALSE; j < i; j++) {
                if (gCharacterIdSlots[j] == gCharacterIdSlots[i]) {
                    foundIt = TRUE;
                }
            }
        } while (foundIt);
    }
}

/**
 * Handle the character select menu, letting players pick their character.
 * When finished, this will also assign all the AI racers their character IDs.
 */
s32 menu_character_select_loop(s32 updateRate) {
    s32 i;
    s32 charSlot;
    s32 confirmOffset;
    s8 activePlayers[4];
    s32 j;

    charselect_render_text(updateRate);
    charselect_music_channels(updateRate);
    menu_input();

    for (i = 0; i < ARRAY_COUNT(gCharselectStatus); i++) {
        // Automatically advance all "confirmed" character selections to "ready".
        // This prevents particle effects from spawning repeatedly on selection.
        if (gCharselectStatus[i] == CHARSELECT_STATUS_CONFIRMED) {
            gCharselectStatus[i] = CHARSELECT_STATUS_READY;
        }
    }
    gIgnorePlayerInputTime = 0;
    if (gMenuDelay == 0) {
        // clang-format off
        // THIS MUST BE ON ONE LINE!
        for (i = 0; i < ARRAY_COUNT(gActivePlayersArray); i++) { activePlayers[i] = gActivePlayersArray[i]; }
        // clang-format on
        charselect_new_player();
        if (gNumberOfReadyPlayers == gNumberOfActivePlayers) {
            charselect_pick(); // Cancel/Confirm selected character?
        } else {
            charselect_input(activePlayers); // Move and select characters?
        }
        return MENU_RESULT_CONTINUE; // This return needs to be here.
    } else if (gMenuDelay > 0) {
        gMenuDelay += updateRate;
        if (gMenuDelay > 30) {
            // This offset will determine whether or not the game goes straight into the tracks menu.
            confirmOffset = 0;
            if (gEnteredCharSelectFrom == 0) {
                confirmOffset++;
                if (gActiveMagicCodes & CHEAT_TWO_PLAYER_ADVENTURE) {
                    confirmOffset++;
                }
            }
            charselect_free();

            charSlot = 0;
            for (j = 0; j < ARRAY_COUNT(gActivePlayersArray); j++) {
                if (gActivePlayersArray[j]) {
                    gCharacterIdSlots[charSlot] = (*gCurrCharacterSelectData)[gPlayersCharacterArray[j]].voiceID;
                    charSlot++;
                }
            }

            charselect_assign_ai(charSlot);
            charselect_assign_players(gActivePlayersArray);

            gIsInTracksMode = TRUE;
            if (confirmOffset >= gNumberOfActivePlayers) {
                music_change_off();
                load_level_for_menu(ASSET_LEVEL_OPTIONSBACKGROUND, -1, 0);
                if (gNumberOfActivePlayers == 1 && !gPlayerHasSeenCautionMenu) {
                    menu_init(MENU_CAUTION);
                } else {
                    menu_init(MENU_GAME_SELECT);
                }
            } else {
                music_change_on();
                trackmenu_set_records();
                init_racer_headers();
                menu_init(MENU_TRACK_SELECT);
            }
        }
    } else if (gMenuDelay < 0) {
        gMenuDelay -= updateRate;
        if (gMenuDelay < -30) {
            music_change_on();
            charselect_free();
            gNumberOfActivePlayers = 1;
            menu_init(MENU_TITLE);
        }
    }
    return MENU_RESULT_CONTINUE;
}

/**
 * Attempt to move the character select cursor elsewhere.
 * Read the potential direction from the character table and see if there's another claimed character in the way.
 * Play a success or a fail sound based on whether the cursor ended up moving.
 */
void charselect_move(s32 playerID, s8 *direction, s32 bounds, u16 menuPickSoundId, u16 menuPickFailedSoundId) {
    s32 sameCharSelected;
    s32 j;
    s32 i;

    j = 0;
    sameCharSelected = TRUE;
    while (sameCharSelected && j < bounds && direction[j] != -1) {
        sameCharSelected = FALSE;
        // Run this block if the DOUBLEVISION cheat isn't active.
        if (!(get_filtered_cheats() & CHEAT_SELECT_SAME_PLAYER)) {
            for (i = 0; i < MAXCONTROLLERS && !sameCharSelected; i++) {
                if (i != playerID && gPlayersCharacterArray[i] == direction[j]) {
                    sameCharSelected = TRUE;
                }
            }
            if (sameCharSelected) {
                j++;
            }
        }
    }
    if (!sameCharSelected) {
        gPlayersCharacterArray[playerID] = direction[j];
        // menuPickSoundId seems to always be passed SOUND_MENU_PICK3?
        sound_play(menuPickSoundId, NULL);
    } else {
        // menuPickFailedSoundId seems to always be passed SOUND_HORN_DRUMSTICK?
        sound_play(menuPickFailedSoundId, NULL);
    }
}

/**
 * Free all assets associated with the character select menu.
 */
void charselect_free(void) {
    menu_assetgroup_free(gCharSelectObjectIndices);
    mempool_free_timer(0);
    unload_font(ASSET_FONTS_BIGFONT);
    mempool_free_timer(2);
    gEnteredCharSelectFrom = 0;
}

/**
 * Raise and lower music channel volumes based on which character is last selected.
 */
void charselect_music_channels(s32 updateRate) {
    if (gMenuCurrentCharacter.switchDelay > 0) {
        gMenuCurrentCharacter.switchDelay -= updateRate;
        if (gMenuCurrentCharacter.switchDelay <= 0) {
            if (sSoundOptionsState.channelIndex >= 0) {
                music_channel_off(gCharacterVolumes[sSoundOptionsState.channelIndex][0]);
                music_channel_off(gCharacterVolumes[0][sSoundOptionsState.channelIndex * 2 + 1]);
            }
            sSoundOptionsState.channelIndex = gMenuSelectedCharacter.channelIndex;
            if (gMenuSelectedCharacter.channelIndex >= 0) {
                sSoundOptionsState.volume = gMenuSelectedCharacter.volume;
            }
            gMenuSelectedCharacter.channelIndex = gMenuCurrentCharacter.channelIndex;
            if (gMenuSelectedCharacter.channelIndex >= 0) {
                gMenuSelectedCharacter.volume = gMenuCurrentCharacter.volume;
                music_channel_on(gCharacterVolumes[gMenuSelectedCharacter.channelIndex][0]);
                music_channel_on(gCharacterVolumes[0][gMenuSelectedCharacter.channelIndex * 2 + 1]);
            }
        }
    }
    if (gMenuSelectedCharacter.channelIndex >= 0) {
        gMenuSelectedCharacter.volume += updateRate * 4;
        if (gMenuSelectedCharacter.volume > 127) {
            gMenuSelectedCharacter.volume = 127;
        }
        music_channel_fade_set(gCharacterVolumes[gMenuSelectedCharacter.channelIndex][0], gMenuSelectedCharacter.volume);
        music_channel_fade_set(gCharacterVolumes[0][gMenuSelectedCharacter.channelIndex * 2 + 1],
                               gMenuSelectedCharacter.volumeLowByte);
    }
    if (sSoundOptionsState.channelIndex >= 0) {
        sSoundOptionsState.volume -= updateRate * 4;
        if (sSoundOptionsState.channelIndex != gMenuSelectedCharacter.channelIndex) {
            if (sSoundOptionsState.volume < 0) {
                music_channel_off(gCharacterVolumes[sSoundOptionsState.channelIndex][0]);
                music_channel_off(gCharacterVolumes[0][sSoundOptionsState.channelIndex * 2 + 1]);
            } else {
                music_channel_fade_set(gCharacterVolumes[sSoundOptionsState.channelIndex][0], sSoundOptionsState.volume);
                music_channel_fade_set(gCharacterVolumes[0][sSoundOptionsState.channelIndex * 2 + 1], sSoundOptionsState.volumeLowByte);
            }
        }
        if (sSoundOptionsState.volume < 0) {
            sSoundOptionsState.channelIndex = -1;
        }
    }
}

/**
 * Reset the character id number slots to the default.
 */
void reset_character_id_slots(void) {
    s32 i;
    gTrackSelectRow = 1; // 1 = Dino Domain
    for (i = 0; i < 8; i++) {
        gCharacterIdSlots[i] = i;
    }
}

/**
 * Gets the character id from a slot index. Slot should be a value within [0,7]
 */
s8 get_character_id_from_slot(s32 slot) {
    return gCharacterIdSlots[slot];
}

/**
 * Unused duplicate of get_character_id_from_slot()
 */
s8 get_character_id_from_slot_unused(s32 slot) {
    return gCharacterIdSlots[slot];
}

/**
 * Returns the character select controller ID status.
 * 0 = choosing, 1 = chosen, and then set to 2.
 */
s8 *charselect_status(void) {
    return gCharselectStatus;
}

/**
 * Returns the character Id from a controller index.
 */
s8 get_player_character(s32 controllerIndex) {
    if (controllerIndex < 0 || controllerIndex >= MAXCONTROLLERS) {
        return -1;
    }
    if (!gActivePlayersArray[controllerIndex]) {
        return -1;
    }
    return gPlayersCharacterArray[controllerIndex];
}
