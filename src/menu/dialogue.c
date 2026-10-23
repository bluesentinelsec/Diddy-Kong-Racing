#include "menu_internal.h"

static s32 gAdventureSaveGhost;

static s8 sCurrentMenuID;

static s8 gDialogueOptionYOffset;

static s8 gDialogueItemSelection;

static s8 unused_800DF4D8 = TRUE;

static s8 gNextTajChallengeMenu = FALSE;

static s8 gNeedToCloseDialogueBox = FALSE;

// Array of booleans, set to TRUE when you are done talking to an NPC.
// This has (atleast) 5 elements, not 4!
static s8 gDoneTalkingToNPC[5] = {
    FALSE, // Taj
    FALSE, // Unused?
    FALSE, // T.T.
    FALSE, // Taj Challenge
    FALSE  // Trophy Cabinet
};

static s8 gDialogueOptionTangible = FALSE;

static s8 gGameStatusVisible = FALSE;

// TT Game Status textures, think these are potentially multi-part images.
#if VERSION >= VERSION_79
static s16 sOptionsMenuAssetIds[10] = { 0x4C, 0x4D, 0x4E, 0x4F, 0x66, 0x67, 0x00, 0x42, -1, 0 };
#else
static s16 sOptionsMenuAssetIds[10] = { 0x4C, 0x4D, 0x4E, 0x4F, 0x5B, 0x5C, 0x00, 0x42, -1, 0 };
#endif

// TT game status textures. And these maybe just standalone ones.
static s16 sOptionsMenuImageIds[10] = { 0x0D, 0x0E, 0x0F, 0x10, 0x08, 0x09, 0x00, 0x0A, -1, 0 };

/**
 * Reenable rumble, close the menu dialogue box and clear it.
 */
void menu_dialogue_end(void) {
    rumble_init(TRUE);
    dialogue_close(7);
    dialogue_clear(7);
    gMenuFlow.optionCap = 0;
}

/**
 * See above.
 * Not sure why they didn't just call that instead.
 */
void menu_close_dialogue(void) {
    menu_dialogue_end();
}

/**
 * Mark the NPC as no longer being talked to, and begin trying to close the dialogue box.
 * Non trophy race dialogue also resets the dialogue options to default.
 */
void dialogue_npc_finish(s32 npcID) {
    if (gDoneTalkingToNPC[npcID] == FALSE) {
        if (npcID != 3) {
            sCurrentMenuID = 0;
            gDialogue.submenu = 0;
        }
        gNeedToCloseDialogueBox = TRUE;
        gDoneTalkingToNPC[npcID] = TRUE;
    }
}

/**
 * If the call to close dialogue has been made, this will begin hiding it.
 * It then also resets the menus stick inputs.
 */
void dialogue_try_close(void) {
    if (gNeedToCloseDialogueBox) {
        gNeedToCloseDialogueBox = FALSE;
        dialogue_close(1);
        reset_controller_sticks();
    }
}

/**
 * Top level function for controlling dialogue with NPC's.
 * Returning nonzero will trigger those NPC's to stop dialogue.
 */
s32 npc_dialogue_loop(u32 dialogueOption) {
    s32 result;

    gDoneTalkingToNPC[dialogueOption] = FALSE;
    if ((textbox_visible() != 0) && (dialogueOption != DIALOGUE_CHALLENGE)) {
        return 0;
    }
    if (gNeedToCloseDialogueBox) {
        return 0;
    }
    if (dialogueOption != DIALOGUE_CHALLENGE) {
        set_pause_lockout_timer(1);
    }
    result = 0;
    update_controller_sticks();
    dialogue_clear(1);
    open_dialogue_box(1);
    set_current_dialogue_background_colour(1, 0, 0, 0, 128);
    cutscene_skip_request();
    switch (dialogueOption) {
        case DIALOGUE_TAJ:
            result = taj_menu_loop(); // Taj menu
            break;
        case DIALOGUE_TT:
            result = tt_menu_loop(); // T.T. menu
            break;
        case DIALOGUE_CHALLENGE:
            result = dialogue_challenge_loop(); // Taj challenge completed/failed menu
            break;
        case DIALOGUE_TROPHY:
            result = trophy_race_cabinet_menu_loop(); // Trophy race cabinet menu
            break;
        case DIALOGUE_RACERESULT:
            result = dialogue_race_defeat();
            break;
    }
    return result;
}

/**
 * Each dialogue option runs this. If the condition is true, it makes the text partially whiter.
 * Otherwise, it stays as normal, or fades out, depending on whether the option is accessable.
 */
void set_option_text_colour(s32 condition) {
    if (condition == FALSE) {
        set_current_text_colour(1, 0, 0, 0x7F, 0x5A, 0xFF); // Normal
        if (gDialogueOptionTangible != FALSE) {
            set_current_text_colour(1, 0, 0, 0x7F, 0x5A, 0x78); // Locked
        }
    } else {
        set_current_text_colour(1, 0xCF, 0xCF, 0xCF, 0x5A, 0xFF); // Selected Option
    }
    gDialogueOptionTangible = (u8) FALSE;
}

/**
 * Render a line of text at a given Y position, and pass through the text ID and option ID.
 */
void render_dialogue_option(char *text, s32 yOffset, s32 optionID) {
    set_option_text_colour(gDialogue.submenu == gDialogue.optionMax);
    if (gDialogue.submenu == gDialogue.optionMax) {
        gDialogueItemSelection = optionID;
    }
    render_dialogue_text(1, POS_CENTRED, gDialogueOptionYOffset, text, 1, HORZ_ALIGN_CENTER);
    gDialogueOptionYOffset = (s8) (gDialogueOptionYOffset + yOffset);
    gDialogue.optionMax = (s8) (gDialogue.optionMax + 1);
}

/**
 * Sets the variable controlling which Taj Challenge dialogue is to be used.
 */
void set_next_taj_challenge_menu(s32 arg0) {
    gNextTajChallengeMenu = arg0;
}

/**
 * Sets the menu ID to the second passed number if it's equal to the first.
 */
void set_menu_id_if_option_equal(s32 IDToCheck, s32 IDToSet) {
    if (sCurrentMenuID == IDToCheck) {
        sCurrentMenuID = IDToSet;
    }
}

/**
 * Handles all of the rendering and logic of Taj's dialogue menu.
 */
s32 taj_menu_loop(void) {
    s32 dialogueResult;
    s32 buttonsPressed;
    Settings *settings;

    settings = get_settings();
    if (gNextTajChallengeMenu && sCurrentMenuID == DIALOGUEPAGE_TAJ_TUTORIAL) {
        sCurrentMenuID = -gNextTajChallengeMenu;
    }
    if (sCurrentMenuID == DIALOGUEPAGE_TAJ_TUTORIAL) {
        sCurrentMenuID = DIALOGUEPAGE_TAJ_ROOT;
    }
    if (sCurrentMenuID >= DIALOGUEPAGE_TAJ_ROOT && sCurrentMenuID <= DIALOGUEPAGE_TAJ_CHALLENGES) {
        set_current_dialogue_box_coords(1, 24, 16, 184, 124);
        set_dialogue_font(1, ASSET_FONTS_FUNFONT);
    }
    dialogueResult = 0;
    buttonsPressed = input_pressed(PLAYER_ONE);
    gDialogue.optionMax = 0;

    switch (sCurrentMenuID) {
        case DIALOGUEPAGE_TAJ_VEHICLE_SELECT:
        case DIALOGUEPAGE_TAJ_VEHICLE_SELECT_2:
            render_dialogue_text(1, POS_CENTRED, 6, gMenuLoaded.text[ASSET_MENU_TEXT_VEHICLESELECT], 1,
                                 HORZ_ALIGN_CENTER); // VEHICLE SELECT
            gDialogueOptionYOffset = 30;
            render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_CAR], 20, 0);        // CAR
            render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_HOVERCRAFT], 20, 1); // HOVERCRAFT
            render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_PLANE], 20, 2);      // PLANE
            render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_MAINMENU], 20, 3);   // MAIN MENU
            break;
        case DIALOGUEPAGE_TAJ_CHALLENGES:
        case DIALOGUEPAGE_TAJ_CHALLENGES_2:
            render_dialogue_text(1, POS_CENTRED, 6, gMenuLoaded.text[ASSET_MENU_TEXT_CHALLENGESELECT], 1,
                                 HORZ_ALIGN_CENTER); // CHALLENGE SELECT
            gDialogueOptionYOffset = 30;
            if (settings->tajFlags & TAJ_FLAGS_CAR_CHAL_UNLOCKED) {
                gDialogueOptionTangible = settings->tajFlags & TAJ_FLAGS_CAR_CHAL_COMPLETED;
                render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_CARCHALLENGE], 20, 0); // CAR CHALLENGE
            }
            if (settings->tajFlags & TAJ_FLAGS_HOVER_CHAL_UNLOCKED) {
                gDialogueOptionTangible = settings->tajFlags & TAJ_FLAGS_HOVER_CHAL_COMPLETED;
                render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_HOVERCHALLENGE], 20, 1); // HOVER CHALLENGE
            }
            if (settings->tajFlags & TAJ_FLAGS_PLANE_CHAL_UNLOCKED) {
                gDialogueOptionTangible = settings->tajFlags & TAJ_FLAGS_PLANE_CHAL_COMPLETED;
                render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_PLANECHALLENGE], 20, 2); // PLANE CHALLENGE
            }
            render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_MAINMENU], 20, 3); // MAIN MENU
            break;
    }

    switch (sCurrentMenuID) {
        case DIALOGUEPAGE_TAJ_TUTORIAL:
            // Hello Friend!
            // If you wish to change your vehicle, then summon me with your horn.
            // Either stop on the pad or beep when I am close to you!
            // Press A To Continue
            set_current_text(ASSET_GAME_TEXT_7);
            sCurrentMenuID = DIALOGUEPAGE_TAJ_ROOT;
            unused_800DF4D8 = TRUE;
            break;
        case DIALOGUEPAGE_TAJ_ROOT:
            gNextTajChallengeMenu = 0;
#if VERSION >= VERSION_79
            // ASSET_MENU_TEXT_OPTIONS must have changed
            render_dialogue_text(1, POS_CENTRED, 6, gMenuLoaded.text[190], 1,
                                 HORZ_ALIGN_CENTER); // OPTIONS
#else
            render_dialogue_text(1, POS_CENTRED, 6, gMenuLoaded.text[ASSET_MENU_TEXT_OPTIONS], 1,
                                 HORZ_ALIGN_CENTER); // OPTIONS
#endif
            gDialogueOptionYOffset = 30;
            render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_CHANGEVEHICLE], 20, 0); // CHANGE VEHICLE
            if (settings->tajFlags & TAJ_FLAGS_UNLOCKED_A_CHALLENGE) {
                render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_CHALLENGES], 20, 1); // CHALLENGES
            }
            render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_RETURN], 20, 2); // RETURN
            handle_menu_joystick_input();
            if (buttonsPressed & B_BUTTON) {
                dialogueResult = 3;
                sound_play(SOUND_MENU_BACK3, NULL);
            } else if (buttonsPressed & A_BUTTON) {
                sound_play(SOUND_SELECT2, NULL);
                switch (gDialogueItemSelection) {
                    case 1:
                        sCurrentMenuID = DIALOGUEPAGE_TAJ_CHALLENGES;
                        gDialogue.submenu = 0;
                        play_taj_voice_clip(SOUND_VOICE_TAJ_CHALLENGE_MENU, TRUE);
                        break;
                    case 2:
                        dialogueResult = DIALOGUEPAGE_TAJ_CHALLENGES;
                        break;
                    case 0:
                        sCurrentMenuID = DIALOGUEPAGE_TAJ_VEHICLE_SELECT;
                        gDialogue.submenu = 0;
                        play_taj_voice_clip(SOUND_VOICE_TAJ_SELECT_VEHICLE, TRUE);
                        break;
                }
            }
            break;
        case DIALOGUEPAGE_TAJ_VEHICLE_SELECT:
            handle_menu_joystick_input();
            if (buttonsPressed & B_BUTTON) {
                sound_play(SOUND_MENU_BACK3, NULL);
                play_taj_voice_clip(SOUND_VOICE_TAJ_MENUBACK, TRUE);
                sCurrentMenuID = DIALOGUEPAGE_TAJ_ROOT;
                gDialogue.submenu = 0;
            } else if (buttonsPressed & A_BUTTON) {
                if (gDialogueItemSelection != 3) {
                    dialogueResult = gDialogueItemSelection | 0x80;
                    sCurrentMenuID = DIALOGUEPAGE_TAJ_VEHICLE_SELECT_2;
                } else {
                    sCurrentMenuID = DIALOGUEPAGE_TAJ_ROOT;
                    gDialogue.submenu = 0;
                    play_taj_voice_clip(SOUND_VOICE_TAJ_MENUBACK, TRUE);
                }
            }
            break;
        case DIALOGUEPAGE_TAJ_CHALLENGES:
            handle_menu_joystick_input();
            if ((buttonsPressed & B_BUTTON) || ((buttonsPressed & A_BUTTON) && (gDialogueItemSelection == 3))) {
                sound_play(SOUND_MENU_BACK3, NULL);
                play_taj_voice_clip(SOUND_VOICE_TAJ_MENUBACK2, TRUE);
                sCurrentMenuID = DIALOGUEPAGE_TAJ_ROOT;
                gDialogue.submenu = 3;
            } else if (buttonsPressed & A_BUTTON) {
                dialogueResult = gDialogueItemSelection | 0x40;
                sound_play(SOUND_SELECT2, NULL);
                sCurrentMenuID = DIALOGUEPAGE_TAJ_CHALLENGES_2;
            }
            break;
        case DIALOGUEPAGE_TAJ_CHALLENGE_PLANE: // Plane Challenge - To Test your flying skills you must race me around
                                               // the Island!!
        case DIALOGUEPAGE_TAJ_CHALLENGE_HOVER: // Hover Challenge - To Test your driving skills you must race me around
                                               // the Island!!
        case DIALOGUEPAGE_TAJ_CHALLENGE_CAR: // Car Challenge - To Test your driving skills you must race me around the
                                             // Island!!
            set_current_text(ASSET_GAME_TEXT_8 - sCurrentMenuID); // 9, 10, 11
            sCurrentMenuID = DIALOGUEPAGE_TAJ_4;
            break;
        case DIALOGUEPAGE_TAJ_CHALLENGE_LOSE:
            set_current_text(ASSET_GAME_TEXT_17); // MY SKILLS ARE GREATER! - Better luck next time. - TRY AGAIN.
            gNextTajChallengeMenu = 0;
            sCurrentMenuID = DIALOGUEPAGE_TAJ_ROOT;
            gDialogue.submenu = 3;
            break;
        case DIALOGUEPAGE_TAJ_CHALLENGE_WIN:
            set_current_text(ASSET_GAME_TEXT_21); // Well Done! - You are a skilled racer.
            gNextTajChallengeMenu = 0;
            sCurrentMenuID = DIALOGUEPAGE_TAJ_7;
            gDialogue.submenu = 0;
            break;
        case DIALOGUEPAGE_TAJ_CHALLENGE_WIN_3: // Well done! - Take this as a reward. - That was my last challenge. You
                                               // have done well.
        case DIALOGUEPAGE_TAJ_CHALLENGE_WIN_2: // Well done! - Take this as a reward. - I'll See You Later.
        case DIALOGUEPAGE_TAJ_CHALLENGE_WIN_1: // Well done! - Take this as a reward. - I'll See You Later.
            set_current_text(ASSET_GAME_TEXT_12 - sCurrentMenuID); // 18, 19, 20
            sCurrentMenuID = DIALOGUEPAGE_TAJ_6;
            gDialogue.submenu = 0;
            break;
        case DIALOGUEPAGE_TAJ_4:
            dialogueResult = (gNextTajChallengeMenu - 1) | 0x40;
            sCurrentMenuID = DIALOGUEPAGE_TAJ_5;
            dialogue_close(1);
            break;
        case DIALOGUEPAGE_TAJ_5:
            gNextTajChallengeMenu = 0;
            dialogue_close(1);
            break;
        case DIALOGUEPAGE_TAJ_6:
        case DIALOGUEPAGE_TAJ_7:
            dialogueResult = 4;
            if (sCurrentMenuID == DIALOGUEPAGE_TAJ_7) {
                dialogueResult = 3;
            }
            gNeedToCloseDialogueBox = FALSE;
            dialogue_close(1);
            gNextTajChallengeMenu = 0;
            sCurrentMenuID = DIALOGUEPAGE_TAJ_TUTORIAL;
            break;
    }
    return dialogueResult;
}

/**
 * Shows the dialogue options after you finish an unbeaten race in P2 or lower.
 */
s32 dialogue_race_defeat(void) {
    s32 state;
    s32 playerInput;

    set_current_dialogue_box_coords(1, 24, 16, 184, 135);
    set_dialogue_font(1, ASSET_FONTS_FUNFONT);
    state = 0;
    playerInput = input_pressed(PLAYER_ONE);
    gDialogue.optionMax = 0;
    gNextTajChallengeMenu = 0;
    render_dialogue_text(1, POS_CENTRED, 6, gMenuLoaded.text[ASSET_MENU_TEXT_LOSETOTAJ_0], 1,
                         HORZ_ALIGN_CENTER); // BETTER LUCK
    render_dialogue_text(1, POS_CENTRED, 20, gMenuLoaded.text[ASSET_MENU_TEXT_LOSETOTAJ_1], 1,
                         HORZ_ALIGN_CENTER); // NEXT TIME!
    gDialogueOptionYOffset = 50;
    render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_TRYAGAIN], 20, 0); // TRY AGAIN
    render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_EXIT], 20, 1);     // EXIT
    handle_menu_joystick_input();
    if (playerInput & A_BUTTON) {
        sound_play(SOUND_SELECT2, NULL);
        switch (gDialogueItemSelection) {
            case 0:
                state = 1;
                break;
            case 1:
                state = 2;
                break;
        }
    }
    return state;
}

/**
 * Main function for TT dialogue.
 * Handles the controls for navigating the main options, selecting time trial and viewing the game status.
 * Also has unused functions to handle pak swapping as well as displaying the messages about them.
 */
s32 tt_menu_loop(void) {
    s32 currentOption;
    s32 buttonsPressed;
    s32 i;
    s32 yPos1;
    u8 result;
    Settings *settings;

    settings = get_settings();

    if (is_in_two_player_adventure()) {
        settings->cutsceneFlags |= CUTSCENE_TT_HELP;
    }
    if (!(settings->cutsceneFlags & CUTSCENE_TT_HELP)) {
        sCurrentMenuID = TT_MENU_INTRODUCTION;
    }
        if ((sCurrentMenuID != TT_MENU_GAME_STATUS) && (sCurrentMenuID != TT_MENU_INTRODUCTION)) {
        currentOption = 120;
        if (has_ghost_to_save()) {
            currentOption = 136;
        }
        set_current_dialogue_box_coords(1, 24, 16, 192, currentOption);
    } else {
        set_current_dialogue_box_coords(1, 24, 16, 184, 220);
    }
    set_dialogue_font(1, ASSET_FONTS_FUNFONT);
    currentOption = 0;
    buttonsPressed = input_pressed(PLAYER_ONE) << 0;
    gDialogue.optionMax = 0;
    gDialogueOptionYOffset = 32;
    switch (sCurrentMenuID) {
        case TT_MENU_ROOT:
        case TT_MENU_EXIT:
#if VERSION >= VERSION_79
            // ASSET_MENU_TEXT_OPTIONS must have changed
            render_dialogue_text(1, POS_CENTRED, 6, gMenuLoaded.text[189], 1,
                                 HORZ_ALIGN_CENTER); // OPTIONS
#else
            render_dialogue_text(1, POS_CENTRED, 6, gMenuLoaded.text[ASSET_MENU_TEXT_OPTIONS], 1,
                                 HORZ_ALIGN_CENTER); // OPTIONS
#endif
            render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_STATUS], 20, 3); // STATUS
            if (!is_in_two_player_adventure()) {
                if (is_time_trial_enabled()) {
                    render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_TIMETRIALON], 20, 0); // TIME TRIAL ON
                } else {
                    render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_TIMETRIALOFF], 20, 0); // TIME TRIAL OFF
                }
                if (has_ghost_to_save()) {
                    render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_SAVEGHOST], 20, 1); // SAVE GHOST
                }
            }
            render_dialogue_option(gMenuLoaded.text[ASSET_MENU_TEXT_RETURN], 20, 2); // RETURN
            handle_menu_joystick_input();
            if (gDialogueItemSelection == 0) {
                if (gMenuInput.xAxisDirection[0] > 0) {
                    if (!is_time_trial_enabled()) {
                        play_tt_voice_clip(SOUND_VOICE_TT_TIME_TRIAL_ON, TRUE);
                    }
                    set_time_trial_enabled(TRUE);
                } else if (gMenuInput.xAxisDirection[0] < 0) {
                    if (is_time_trial_enabled() == 1) {
                        play_tt_voice_clip(SOUND_VOICE_TT_TIME_TRIAL_OFF, TRUE);
                    }
                    set_time_trial_enabled(FALSE);
                }
            }
            if ((buttonsPressed & A_BUTTON) && (sCurrentMenuID != TT_MENU_EXIT)) {
                if (gMenuFlow.previousMenuID && !gMenuFlow.previousMenuID && !gMenuFlow.previousMenuID) {} // Fakematch
                switch (gDialogueItemSelection) {
                    case 1:
                        sound_play(SOUND_SELECT2, NULL);
                        gAdventureSaveGhost = 0;
                        gMenuFlow.previousMenuID = TT_MENU_ROOT;
                        sCurrentMenuID = TT_MENU_SAVE_GHOST;
                        break;
                    case 3:
                        sound_play(SOUND_SELECT2, NULL);
                        play_tt_voice_clip(SOUND_VOICE_TT_GAME_STATUS, TRUE);
                        menu_assetgroup_load(sOptionsMenuAssetIds);
                        menu_imagegroup_load(sOptionsMenuImageIds);
                        gGameStatusVisible = TRUE;
                        sCurrentMenuID = TT_MENU_GAME_STATUS;
                        break;
                }
                currentOption = gDialogueItemSelection + 1;
            } else if (buttonsPressed & B_BUTTON) {
                sound_play(SOUND_MENU_BACK3, NULL);
                currentOption = 3;
            }
            if (sCurrentMenuID == TT_MENU_GAME_STATUS) {
                currentOption = 0;
            }
            if (currentOption == 3) {
                sCurrentMenuID = TT_MENU_EXIT;
            }
            if (sCurrentMenuID == TT_MENU_EXIT) {
                currentOption = 3;
            }
            break;
        case TT_MENU_CONT_PAK_ERROR_1:
            i = 0;
            while (gResults.ghostPakErrorText[i] != NULL) {
                i++;
            }
            yPos1 = 58;
            if (i > 0) {
                i--;
                yPos1 -= (i << 2 << 1); // Might be fake?
                i = 0;
                while (gResults.ghostPakErrorText[i] != NULL) {
                    render_dialogue_text(1, POS_CENTRED, yPos1, gResults.ghostPakErrorText[i], 1, HORZ_ALIGN_CENTER);
                    yPos1 += 16;
                    i++;
                }
            }
            if (buttonsPressed & (A_BUTTON | B_BUTTON)) {
                sCurrentMenuID = TT_MENU_ROOT;
            }
            break;
        case TT_MENU_GAME_STATUS:
            if (buttonsPressed & (A_BUTTON | B_BUTTON)) {
                sCurrentMenuID = TT_MENU_ROOT;
                gGameStatusVisible = FALSE;
                menu_assetgroup_free(sOptionsMenuAssetIds);
                play_tt_voice_clip(SOUND_VOICE_TT_OKAY, TRUE);
            }
            break;
        case TT_MENU_INTRODUCTION: {
            /*
                0: HI, I'M TT.
                1:
                2: YOU CAN ASK ME
                3: ABOUT YOUR GAME
                4: STATUS OR SELECT
                5: MY TIME TRIAL MODE
                6: TO PRACTICE A
                7: LEVEL.
                8:
                9: I CAN ALSO SAVE
                10: YOUR BEST GHOSTS
                11: {NULL TO END THE CHAT}
            */
            // TT Has 12 different intro text lines. This will render them one by one until it hits the NULL line.
            i = ASSET_MENU_TEXT_TTINTRO_0;
            while (gMenuLoaded.text[i] != NULL) {
                render_dialogue_text(1, POS_CENTRED, 6 + ((i - 52) * 16), gMenuLoaded.text[i], 1, HORZ_ALIGN_CENTER);
                i++;
            }
            if (buttonsPressed & (A_BUTTON | B_BUTTON)) {
                settings->cutsceneFlags |= CUTSCENE_TT_HELP;
                sCurrentMenuID = TT_MENU_ROOT;
            }
        } break;
        case TT_MENU_INSERT_CONT_PAK:
            render_dialogue_text(1, POS_CENTRED, 34, gMenuLoaded.text[ASSET_MENU_TEXT_INSERTDEVICE_3], 1,
                                 HORZ_ALIGN_CENTER); // If you wish to use
            render_dialogue_text(1, POS_CENTRED, 50, gMenuLoaded.text[ASSET_MENU_TEXT_INSERTDEVICE_4], 1,
                                 HORZ_ALIGN_CENTER); // the Controller Pak
#if VERSION >= VERSION_79
            // ASSET_MENU_TEXT_INSERTDEVICE_6 must have changed
            render_dialogue_text(1, POS_CENTRED, 66, gMenuLoaded.text[161], 1,
                                 HORZ_ALIGN_CENTER); // insert it now!
#else
            render_dialogue_text(1, POS_CENTRED, 66, gMenuLoaded.text[ASSET_MENU_TEXT_INSERTDEVICE_6], 1,
                                 HORZ_ALIGN_CENTER); // insert it now!
#endif
            if (buttonsPressed & (A_BUTTON | START_BUTTON)) {
                sound_play(SOUND_SELECT2, NULL);
                gAdventureSaveGhost = 0;
                gMenuFlow.previousMenuID = TT_MENU_INSERT_RUMBLE_PAK;
                sCurrentMenuID = TT_MENU_SAVE_GHOST;
            } else if (buttonsPressed & B_BUTTON) {
                sound_play(SOUND_MENU_BACK3, NULL);
                sCurrentMenuID = TT_MENU_ROOT;
            }
            break;
        case TT_MENU_INSERT_RUMBLE_PAK:
#if VERSION >= VERSION_79
            render_dialogue_text(1, POS_CENTRED, 34, gMenuLoaded.text[162], 1,
                                 HORZ_ALIGN_CENTER); // If you wish to use
            render_dialogue_text(1, POS_CENTRED, 50, gMenuLoaded.text[186], 1,
                                 HORZ_ALIGN_CENTER); // the Rumble Pak
            render_dialogue_text(1, POS_CENTRED, 66, gMenuLoaded.text[187], 1,
                                 HORZ_ALIGN_CENTER); // insert it now!
#else
            render_dialogue_text(1, POS_CENTRED, 34, gMenuLoaded.text[ASSET_MENU_TEXT_INSERTDEVICE_3], 1,
                                 HORZ_ALIGN_CENTER); // If you wish to use
            render_dialogue_text(1, POS_CENTRED, 50, gMenuLoaded.text[ASSET_MENU_TEXT_INSERTDEVICE_5], 1,
                                 HORZ_ALIGN_CENTER); // the Rumble Pak
            render_dialogue_text(1, POS_CENTRED, 66, gMenuLoaded.text[ASSET_MENU_TEXT_INSERTDEVICE_6], 1,
                                 HORZ_ALIGN_CENTER); // insert it now!
#endif
            if (buttonsPressed & (A_BUTTON | B_BUTTON | START_BUTTON)) {
                sCurrentMenuID = TT_MENU_ROOT;
            }
            break;
        case TT_MENU_SAVE_GHOST:
            render_dialogue_text(1, POS_CENTRED, 50, gMenuLoaded.text[ASSET_MENU_TEXT_PLEASEWAIT], 1,
                                 HORZ_ALIGN_CENTER); // PLEASE WAIT
            gAdventureSaveGhost++;
            if (gAdventureSaveGhost >= 5) {
                result = timetrial_save_player_ghost(0);
                if (result == CONTROLLER_PAK_CHANGED) {
                    result = timetrial_save_player_ghost(0);
                }
                switch (result) {
                    case CONTROLLER_PAK_GOOD:
                        sCurrentMenuID = gMenuFlow.previousMenuID;
                        break;
                    case CONTROLLER_PAK_RUMBLE_PAK_FOUND:
                        sCurrentMenuID = TT_MENU_INSERT_CONT_PAK;
                        break;
                    case CONTROLLER_PAK_NOT_FOUND:
                        // NO CONTROLLER PAK
                        // If you wish to change / Controller Pak or Rumble Pak, / please do so now.
                        gResults.ghostPakErrorText = sNoControllerPakMenuText;
                        sCurrentMenuID = TT_MENU_CONT_PAK_ERROR_1;
                        break;
                    case CONTROLLER_PAK_FULL:
                    case CONTROLLER_PAK_NO_ROOM_FOR_GHOSTS:
                        // CONTROLLER PAK FULL
                        // If you wish to change / Controller Pak or Rumble Pak, / please do so now.
                        gResults.ghostPakErrorText = sControllerPakFullMenuText;
                        sCurrentMenuID = TT_MENU_CONT_PAK_ERROR_1;
                        break;
                    case CONTROLLER_PAK_BAD_DATA:
                        // CORRUPT DATA.
                        // If you wish to change / Controller Pak or Rumble Pak, / please do so now.
                        // TRY AGAIN!
                        gResults.ghostPakErrorText = sCorruptDataMenuText;
                        sCurrentMenuID = TT_MENU_CONT_PAK_ERROR_1;
                        break;
                    case CONTROLLER_PAK_INCONSISTENT:
                    case CONTROLLER_PAK_WITH_BAD_ID:
                    case CONTROLLER_PAK_CHANGED:
                    case CONTROLLER_PAK_SWITCH_TO_RUMBLE:
                    default:
                        // BAD CONTROLLER PAK
                        // If you wish to change / Controller Pak or Rumble Pak, / please do so now.
                        gResults.ghostPakErrorText = sBadControllerPakMenuText;
                        sCurrentMenuID = TT_MENU_CONT_PAK_ERROR_1;
                        break;
                }
            }
            break;
    }
    if ((sCurrentMenuID == TT_MENU_CONT_PAK_ERROR_1) || (sCurrentMenuID == TT_MENU_CONT_PAK_ERROR_2) ||
        (sCurrentMenuID == TT_MENU_CONT_PAK_ERROR_3)) {
        render_dialogue_text(1, POS_CENTRED, 6, gMenuLoaded.text[ASSET_MENU_TEXT_PAKERROR], 1,
                             HORZ_ALIGN_CENTER); // PAK ERROR
    }

    return currentOption;
}

/**
 * Render all the 2D elements for T.T's game status screen.
 */
void dialogue_tt_gamestatus(void) {
    s32 numBalloonsTensDigit;
    s32 y;
    s32 i;
    s32 x;
    s32 numOfItem;
    Settings *settings;
    s32 flag;
    s16 tempX;

    if (gGameStatusVisible == TRUE) {
        gGameStatusVisible++;
        return;
    }
    settings = get_settings();
    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    if (osTvType == OS_TV_TYPE_PAL) {
        y = 10;
    } else {
        y = 20;
    }
    numOfItem = 8;
    if (settings->cutsceneFlags & 4) {
        numOfItem = 9;
    }
    tempX = -74;
    gMenuLoaded.images[numOfItem].trans.x_position = tempX;
    gMenuLoaded.images[numOfItem].trans.y_position = 65 - y;
    menu_element_render(numOfItem);
    sprite_opaque(FALSE);
    tempX = -29;
    gMenuLoaded.images[0].trans.x_position = tempX;
    gMenuLoaded.images[0].trans.y_position = 98 - y;
    gMenuLoaded.images[0].trans.x_position -= 6.0f;

    flag = settings->balloonsPtr[0] / 10; // settings->balloonsPtr[0] is the total balloon count.
    numBalloonsTensDigit = flag;
    if (numBalloonsTensDigit) {
        gMenuLoaded.images[0].spriteOffset = numBalloonsTensDigit;
        menu_element_render(0);
        gMenuLoaded.images[0].trans.x_position += 12.0f;
    }
    gMenuLoaded.images[0].spriteOffset = settings->balloonsPtr[0] % 10;
    menu_element_render(0);
    tempX = -49;
    gMenuLoaded.images[10].trans.x_position = tempX;
    gMenuLoaded.images[10].trans.y_position = 98 - y;
    menu_element_render(10);
    sprite_opaque(TRUE);
    y += 50;
    tempX = -89;
    gMenuLoaded.images[15].trans.x_position = tempX;
    gMenuLoaded.images[15].trans.y_position = 95 - y;
    gMenuLoaded.images[15].spriteOffset = settings->wizpigAmulet;
    menu_element_render(15);
    tempX = -29;
    gMenuLoaded.images[16].trans.x_position = tempX;
    gMenuLoaded.images[16].trans.y_position = 95 - y;
    gMenuLoaded.images[16].spriteOffset = settings->ttAmulet;
    menu_element_render(16);

    for (flag = 3, numOfItem = 0, i = 0; i < 16; flag <<= 2, i++) {
        if (flag == (settings->trophies & flag)) {
            numOfItem++;
        }
    }
    y += 50;
    for (x = 20, i = 0; i < 4; i++, x += 30) {
        gMenuLoaded.images[13].trans.x_position = (x - 124);
        gMenuLoaded.images[13].trans.y_position = (95 - y);
        if (i < numOfItem) {
            gMenuLoaded.images[13].spriteOffset = 1;
        } else {
            gMenuLoaded.images[13].spriteOffset = 0;
        }
        menu_element_render(13);
    }
    for (flag = 1, i = 0, numOfItem = 0; i < 16; flag <<= 1, i++) {
        if (flag == (settings->keys & flag)) {
            numOfItem++;
        }
    }
    y += 45;
    for (x = 20, i = 0; i < 4; i++, x += 30) {
        gMenuLoaded.images[14].trans.x_position = (x - 124);
        gMenuLoaded.images[14].trans.y_position = (95 - (y));
        if (i < numOfItem) {
            gMenuLoaded.images[14].spriteOffset = 1;
        } else {
            gMenuLoaded.images[14].spriteOffset = 0;
        }
        menu_element_render(14);
    }
    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
}

/**
 * Dialogue menu for the trophy cabinets in each world hub.
 */
s32 trophy_race_cabinet_menu_loop(void) {
    s32 currentOption;
    s32 buttonsPressed;

    set_current_dialogue_box_coords(1, 24, 16, 184, SCREEN_HEIGHT_HALF);
    set_dialogue_font(1, ASSET_FONTS_FUNFONT);
    currentOption = 0;
    buttonsPressed = input_pressed(PLAYER_ONE);
    render_dialogue_text(1, POS_CENTRED, 6, gMenuLoaded.text[ASSET_MENU_TEXT_TROPHYRACE], 1,
                         HORZ_ALIGN_CENTER); // TROPHY RACE
    if (gMenuInput.yAxisDirection[0] < 0) {
        gDialogue.submenu++;
    } else if (gMenuInput.yAxisDirection[0] > 0) {
        gDialogue.submenu--;
    }
    if (gDialogue.submenu < 0) {
        gDialogue.submenu = 0;
    }
    if (gDialogue.submenu > 1) {
        gDialogue.submenu = 1;
    }
    set_option_text_colour(gDialogue.submenu == 0);
    render_dialogue_text(1, POS_CENTRED, 30, gMenuLoaded.text[ASSET_MENU_TEXT_ENTERTROPHYRACE], 1,
                         HORZ_ALIGN_CENTER); // ENTER TROPHY RACE
    set_option_text_colour(gDialogue.submenu == 1);
    render_dialogue_text(1, POS_CENTRED, 50, gMenuLoaded.text[ASSET_MENU_TEXT_EXIT], 1, HORZ_ALIGN_CENTER); // EXIT
    if (buttonsPressed & A_BUTTON) {
        currentOption = gDialogue.submenu + 1;
    }
    if (buttonsPressed & B_BUTTON) {
        currentOption = 2;
    }
    return currentOption;
}

/**
 * Blank function called when a dialogue box is opened.
 */
void dialogue_open_stub(void) {
}

/**
 * Blank function called when a dialogue box is closed.
 */
void dialogue_close_stub(void) {
}

/**
 * Renders ortho geometry if the current dialogue box has anything to draw.
 */
f32 dialogue_ortho(UNUSED DialogueBoxBackground *textbox, Gfx **dList, Mtx **mat, Vertex **verts) {
    gMenuFrame.displayList = *dList;
    gMenuFrame.hudMat = *mat;
    gMenuFrame.hudVerts = *verts;
    mtx_ortho(&gMenuFrame.displayList, &gMenuFrame.hudMat);
    if (gGameStatusVisible && sCurrentMenuID == MENU_UNUSED_4) {
        dialogue_tt_gamestatus();
    }
    *dList = gMenuFrame.displayList;
    *mat = gMenuFrame.hudMat;
    *verts = gMenuFrame.hudVerts;
    return TRUE;
}
