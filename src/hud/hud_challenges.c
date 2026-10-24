#include "hud_internal.h"

/*
 * Renders the HUD for the Smokey Castle challenge.
 * Shows each players portrait and their aquired treasure.
 */
void hud_main_treasure(s32 countdown, Object *obj, s32 updateRate) {
    Object_Racer *racer;
    Object **racers;
    s32 numRacers;
    s32 i;

    racer = obj->racer;
    racers = get_racer_objects(&numRacers);
    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    hud_race_start(countdown, updateRate);
    switch (gNumActivePlayers) {
        case 1:
            for (i = 0; i < numRacers; i++) {
                racer = racers[i]->racer;
                hud_treasure(racer);
                if (i == 1) {
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x += 55.0f;
                    gCurrentHud->entry[HUD_TREASURE_METRE].pos.x += 55.0f;
                }
                gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x += 55.0f;
                gCurrentHud->entry[HUD_TREASURE_METRE].pos.x += 55.0f;
            }
            if (1) {} // Fakematch
            gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x -= 275.0f;
            gCurrentHud->entry[HUD_TREASURE_METRE].pos.x -= 275.0f;
            break;
        case 3:
        case 4:
            hud_treasure(racer);
            break;
    }
    racer = obj->racer;
    rendermode_reset(&gHudDL);
    hud_bananas(racer, updateRate);
    hud_weapon(obj, updateRate);
    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
}

/**
 * The Egg Collector challenge mode displays the icons of each player and their score.
 */
void hud_main_eggs(s32 countdown, Object *obj, s32 updateRate) {
    Object_Racer *racer = obj->racer;
    if (racer->raceFinished == FALSE) {
        cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
        hud_race_start(countdown, updateRate);
        hud_weapon(obj, updateRate);
        if ((127 - (updateRate * 2)) >= gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].challengeEggs.alphaTimer) {
            gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].challengeEggs.alphaTimer += updateRate * 2;
        } else {
            gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].challengeEggs.alphaTimer =
                (gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].challengeEggs.alphaTimer + (updateRate * 2)) - 255;
        }
        // Skip rendering the portraits if in 2 player, it's done elsewhere.
        if (gNumActivePlayers != 2) {
            hud_draw_eggs(obj, updateRate);
        }
        cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
    }
}

/**
 * Position and render the portraits for the egg challenge HUD.
 */
void hud_draw_eggs(Object *racerObj, s32 updateRate) {
    Object **racers;
    s32 i;
    Object_Racer *racer;
    s32 numRacers;
    s32 finishedPlayers;
    Object_Racer *curRacer;
    s32 portraitX;
    s32 diffX;
    s32 portraitY;
    s32 diffY;

    racers = get_racer_objects(&numRacers);
    if (gNumActivePlayers == 2) {
        finishedPlayers = 0;
        for (i = 0; i < numRacers; i++) {
            curRacer = racers[i]->racer;
            if (curRacer->playerIndex != PLAYER_COMPUTER && curRacer->raceFinished) {
                finishedPlayers++;
            }
        }
        if (finishedPlayers == 2) {
            return;
        }
    }
    if (racerObj != NULL) {
        racer = racerObj->racer;
    } else {
        racer = racers[0]->racer;
    }
    if (numRacers == MAXCONTROLLERS) {
        cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
        portraitX = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x;
        portraitY = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y;
        diffX = (gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x - portraitX);
        diffY = (gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.y - portraitY);
        for (i = 0; i < MAXCONTROLLERS; i++) {
            curRacer = racers[i]->racer;
            if (gNumActivePlayers < 3 || racer->playerIndex == curRacer->playerIndex) {
                hud_eggs_portrait(curRacer, updateRate);
            }
            gHudColour = COLOUR_RGBA32(255, 255, 255, 254);
            if (gNumActivePlayers == 1) {
                gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x += 68.0f;
            } else if (gNumActivePlayers == 2) {
                gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = portraitX;
                if (osTvType == OS_TV_TYPE_PAL) {
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y += 66.0;
                    gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.y += 66.0;
                } else {
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y += 55.0f;
                    gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.y += 55.0f;
                }
            }
            gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x + diffX;
        }
        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = portraitX;
        gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x = portraitX + diffX;
        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = portraitY;
        gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.y = portraitY + diffY;
        cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
    }
}

/**
 * Render the character portrait and the number of eggs they have secured.
 * Skip the portrait in 3/4 player, unless player 4 is AI controlled.
 */
void hud_eggs_portrait(Object_Racer *racer, UNUSED s32 updateRate) {
    s32 i;
    s32 alpha;
    s32 prevY;
    s32 i2;

    prevY = gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x;
    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].spriteID = racer->characterId + HUD_SPRITE_PORTRAIT;
    gHudPALScale = TRUE;
    if (gNumActivePlayers < 3 || (gNumActivePlayers == 3 && racer->playerIndex == PLAYER_COMPUTER)) {
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT]);
    }
    if (gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].challengeEggs.alphaTimer < 0) {
        alpha = (gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].challengeEggs.alphaTimer * 2) + 256;
    } else {
        alpha = 255 - (gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].challengeEggs.alphaTimer * 2);
    }
    if (alpha > 254) {
        alpha = 254;
    }
    for (i = 0; i < racer->lap && i < HUD_EGG_TOTAL; i++) {
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON]);
        gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x += 12.0f;
    }
    gHudColour = alpha | COLOUR_RGBA32(255, 255, 255, 0);
    i2 = i;
    for (i = 0; i < racer->eggHudCounter && i2 < HUD_EGG_TOTAL; i++) {
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON]);
        i2++;
        gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x += 12.0f;
    }
    gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x = prevY;
    gHudPALScale = FALSE;
}

/**
 * Render the HUD for the battle mode challenge.
 * Shows each players portrait and their remaining health.
 */
void hud_main_battle(s32 countdown, Object *obj, s32 updateRate) {
    Object **racerObjs;
    Object_Racer *racer;
    s32 numRacers;
    s32 racersFinished;
    s32 i;

    racer = obj->racer;
    if (gNumActivePlayers != 1 || racer->raceFinished == FALSE) {
        cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
        hud_race_start(countdown, updateRate);
        hud_weapon(obj, updateRate);
        racerObjs = get_racer_objects(&numRacers);
        switch (gNumActivePlayers) {
            case 1:
                hud_battle_portraits(obj, updateRate);
                break;
            case 2:
                racersFinished = 0;
                for (i = 0; i < numRacers; i++) {
                    racer = racerObjs[i]->racer;
                    if (racer->playerIndex != PLAYER_COMPUTER && racer->raceFinished) {
                        racersFinished++;
                    }
                }
                if (racersFinished == 2) {
                    return;
                }
                break;
            default:
                hud_bananas(racer, updateRate);
                break;
        }
        cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
    }
}

/**
 * Renders the portraits and health in battle mode. Only called if there are 1 or 2 human players.
 * Portraits are laid out horizontally in 1-player mode and vertically in 2-player mode.
 */
void hud_battle_portraits(Object *racerObj, s32 updateRate) {
    s32 i;
    Object_Racer *racer;
    s32 numRacers;
    Object **racers;
    s32 prevChallengerPortraitX;
    s32 prevChallengerPortraitY;
    Object_Racer *curRacer;
    s32 offsetX;
    s32 offsetY;
    s32 tempX;
    s32 tempY;

    racers = get_racer_objects(&numRacers);
    if (racerObj != NULL) {
        racer = racerObj->racer;
    } else {
        racer = racers[PLAYER_ONE]->racer;
    }

    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    if (numRacers == 4) {
        offsetX = 0;
        offsetY = 0;
        prevChallengerPortraitX = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x;
        prevChallengerPortraitY = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y;
        for (i = 0; i < 4; i++) {
            curRacer = racers[i]->racer;
            tempX = (offsetX + prevChallengerPortraitX) - gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x;
            tempY = (offsetY + prevChallengerPortraitY) - gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y;
            gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x += tempX;
            gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y += tempY;
            gCurrentHud->entry[HUD_BATTLE_BANANA_ICON].pos.x += tempX;
            gCurrentHud->entry[HUD_BATTLE_BANANA_ICON].pos.y -= tempY;
            gCurrentHud->entry[HUD_BATTLE_BANANA_X].pos.x += tempX;
            gCurrentHud->entry[HUD_BATTLE_BANANA_X].pos.y += tempY;
            gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].pos.x += tempX;
            gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].pos.y += tempY;
            gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_2].pos.x += tempX;
            gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_2].pos.y += tempY;
            if (gNumActivePlayers < 3 || racer->playerIndex == curRacer->playerIndex) {
                hud_lives_render(curRacer, updateRate);
            }
            if (gNumActivePlayers == 1) {
                offsetX += 68;
            } else if (gNumActivePlayers == 2) {
                if (osTvType == OS_TV_TYPE_PAL) {
                    offsetY += 66.0;
                } else {
                    offsetY += 55;
                }
            }
        }

        tempX = prevChallengerPortraitX - gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x;
        tempY = prevChallengerPortraitY - gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y;
        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x += tempX;
        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y += tempY;
        gCurrentHud->entry[HUD_BATTLE_BANANA_ICON].pos.x += tempX;
        gCurrentHud->entry[HUD_BATTLE_BANANA_ICON].pos.y -= tempY;
        gCurrentHud->entry[HUD_BATTLE_BANANA_X].pos.x += tempX;
        gCurrentHud->entry[HUD_BATTLE_BANANA_X].pos.y += tempY;
        gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].pos.x += tempX;
        gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].pos.y += tempY;
        gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_2].pos.x += tempX;
        gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_2].pos.y += tempY;
    }

    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
}

/**
 * Render the portrait and life counter in the battle mode.
 * In 3/4 player, skips the portrait for all human players. Player 4 has a portrait if it's AI.
 */
void hud_lives_render(Object_Racer *racer, UNUSED s32 updateRate) {
    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].spriteID = racer->characterId + HUD_SPRITE_PORTRAIT;
    if (gNumActivePlayers < 3 || (gNumActivePlayers == 3 && racer->playerIndex == PLAYER_COMPUTER)) {
        gHudPALScale = TRUE;
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT]);
        gHudPALScale = FALSE;
        rdp_init(&gHudDL);
        rendermode_reset(&gHudDL);
    }
    if (racer->bananas < 10) {
        gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].spriteOffset = racer->bananas;
        if (gNumActivePlayers == 2) {
            gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].pos.x += 6.0f;
        }
    } else {
        gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].spriteOffset = racer->bananas / 10;
        gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_2].spriteOffset = racer->bananas % 10;
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_2]);
    }
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1]);
    if (gNumActivePlayers == 2 && racer->bananas < 10) {
        gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].pos.x -= 6.0f;
    }
    if (gNumActivePlayers != 2) {
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BATTLE_BANANA_X]);
        sprite_opaque(TRUE);
        gHudPALScale = TRUE;
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BATTLE_BANANA_ICON]);
        gHudPALScale = FALSE;
        sprite_opaque(FALSE);
    }
}

/**
 * When racing the boss, render the essentials, but skip the bananas.
 */
void hud_main_boss(s32 countdown, Object *obj, s32 updateRate) {
    LevelHeader *level;
    Object_Racer *racer = obj->racer;

    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    hud_wrong_way(racer, updateRate);
    hud_race_start(countdown, updateRate);
    hud_race_time(racer, updateRate);
    hud_weapon(obj, updateRate);

    level = level_header();
    if (level->laps > 1) {
        hud_lap_count(racer, updateRate);
    }

    hud_speedometre(obj, updateRate);
    hud_race_position(racer, updateRate);
    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
}

/**
 * When racing Taj in the overworld, render most of the basic race hud.
 */
void hud_main_taj(s32 countdown, Object *obj, s32 updateRate) {
    Object_Racer *racer = obj->racer;

    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    hud_wrong_way(racer, updateRate);
    hud_lap_count(racer, updateRate);
    hud_race_position(racer, updateRate);
    hud_race_time(racer, updateRate);
    hud_race_start(countdown, updateRate);
    hud_speedometre(obj, updateRate);
    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
}

/**
 * The hub world displays the player's balloon count.
 * In two player adventure, the icon of the character of the player sitting out is displayed.
 */
void hud_main_hub(Object *obj, s32 updateRate) {
    Object_Racer *racer;
    HudElement *portrait;

    if (cam_get_viewport_layout() == PLAYER_ONE) {
        racer = obj->racer;
        cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
        hud_balloons(racer);
        hud_speedometre(obj, updateRate);
        if (is_in_two_player_adventure()) {
            portrait = (HudElement *) &gCurrentHud->entry[HUD_TWO_PLAYER_ADV_PORTRAIT];
            portrait->spriteID = (get_settings()->racers[1].character + HUD_SPRITE_PORTRAIT);
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, portrait);
        }
        cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
    }
}

/**
 * Renders the icons and secured treasure in Smokey's castle.
 * In 4 player, all icons are skipped and in 3 player, player 4's icon is shown on their quadrant.
 */
void hud_treasure(Object_Racer *racer) {
    s32 i;
    s32 prevY;

    prevY = gCurrentHud->entry[HUD_TREASURE_METRE].pos.y;
    if (gNumActivePlayers < 3 || (gNumActivePlayers == 3 && racer->playerIndex == PLAYER_COMPUTER)) {
        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].spriteID = racer->characterId + HUD_SPRITE_PORTRAIT;
        gHudPALScale = TRUE;
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT]);
        gHudPALScale = FALSE;
    }
    for (i = 0; i < 10; i++) {
        if (i >= racer->lap) {
            gHudColour = COLOUR_RGBA32(128, 128, 128, 128);
        }
        gHudPALScale = TRUE;
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_TREASURE_METRE]);
        gHudPALScale = FALSE;
        gCurrentHud->entry[HUD_TREASURE_METRE].pos.y -= 3.0f;
    }
    gCurrentHud->entry[HUD_TREASURE_METRE].pos.y = prevY;
    gHudColour = COLOUR_RGBA32(255, 255, 255, 254);
    gHudPALScale = FALSE;
}

/**
 * Render the silver coin counter on the side of the screen.
 * When the 8th coin is collected, wait half a second then play an encouraging voice clip from T.T.
 */
void hud_silver_coins(Object_Racer *racer, s32 updateRate) {
    s32 i;
    s32 prevY;

    gCurrentHud->entry[HUD_SILVER_COIN_TALLY].pos.y =
        (s32) gCurrentHud->entry[HUD_SILVER_COIN_TALLY].pos.y; // Rounds float down to it's int value.
    prevY = gCurrentHud->entry[HUD_SILVER_COIN_TALLY].pos.y;
    for (i = 0; i < 8; i++) {
        if (i >= racer->silverCoinCount) {
            gHudColour = COLOUR_RGBA32(128, 128, 128, 128);
        }
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_SILVER_COIN_TALLY]);
        gCurrentHud->entry[HUD_SILVER_COIN_TALLY].pos.y -=
            gCurrentHud->entry[HUD_SILVER_COIN_TALLY].silverCoinTally.offsetY;
    }
    if (racer->silverCoinCount == 8) {
        if (gCurrentHud->entry[HUD_SILVER_COIN_TALLY].silverCoinTally.soundTimer < 30) {
            gCurrentHud->entry[HUD_SILVER_COIN_TALLY].silverCoinTally.soundTimer += updateRate;
        }
        if (gCurrentHud->entry[HUD_SILVER_COIN_TALLY].silverCoinTally.soundPlayed == FALSE) {
            if (gCurrentHud->entry[HUD_SILVER_COIN_TALLY].silverCoinTally.soundTimer >= 30) {
                gCurrentHud->entry[HUD_SILVER_COIN_TALLY].silverCoinTally.soundPlayed = TRUE;
                if (gHUDVoiceSoundMask == NULL) {
                    sound_play(SOUND_VOICE_TT_GO_FOR_IT, &gHUDVoiceSoundMask);
                }
            }
        }
    }
    gCurrentHud->entry[HUD_SILVER_COIN_TALLY].pos.y = prevY;
    gHudColour = COLOUR_RGBA32(255, 255, 255, 254);
}

/**
 * Render the icon, and the number of collectable balloons onscreen.
 * Has a case for when in the balloon cutscene, where it will visually increase the count by one.
 */
void hud_balloons(UNUSED Object_Racer *racer) {
    s32 balloonCount;
    s32 div;
    u32 balloonTickTimer;
    Settings *settings;

    settings = get_settings();
    if (osTvType == OS_TV_TYPE_PAL) {
        balloonTickTimer = 646;
    } else {
        balloonTickTimer = 840;
    }
    if (settings->cutsceneFlags & CUTSCENE_ADVENTURE_TWO) {
        gCurrentHud->entry[HUD_BALLOON_COUNT_ICON].spriteID =
            HUD_SPRITE_BALLOON_DIAMOND; // Use the adventure two balloon sprite.
    }
    if (cutscene_id() == 10 && get_balloon_cutscene_timer() < balloonTickTimer) {
        balloonCount = *settings->balloonsPtr - 1;
    } else {
        balloonCount = *settings->balloonsPtr;
        if (cutscene_id() == 10) {
            if (get_balloon_cutscene_timer() < balloonTickTimer + 8 && gHudBalloonSoundMask == NULL) {
                sound_play(SOUND_HUD_LAP_TICK, &gHudBalloonSoundMask);
            }
        }
    }
    if (balloonCount < 0) {
        balloonCount = 0;
    }
    div = balloonCount / 10;
    if (div != 0) {
        gCurrentHud->entry[HUD_BALLOON_COUNT_NUMBER_1].spriteOffset = div;
        gCurrentHud->entry[HUD_BALLOON_COUNT_NUMBER_2].spriteOffset = balloonCount % 10;
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BALLOON_COUNT_NUMBER_2]);
    } else {
        gCurrentHud->entry[HUD_BALLOON_COUNT_NUMBER_1].spriteOffset = balloonCount % 10;
    }
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BALLOON_COUNT_NUMBER_1]);
    enable_pal_viewport_height_adjust(TRUE);
    if (osTvType == OS_TV_TYPE_PAL) {
        sprite_opaque(TRUE);
    }
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BALLOON_COUNT_ICON]);
    sprite_opaque(FALSE);
    enable_pal_viewport_height_adjust(FALSE);
    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_BALLOON_COUNT_X]);
}
