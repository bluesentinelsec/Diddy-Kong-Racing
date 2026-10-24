#include "hud_internal.h"

/**
 * Unused function that plays whichever T.T voice line is passed through.
 * Only if the game is currently running and no voice line is already playing.
 */
UNUSED void hud_sound_play(u16 soundId) {
    if (gHUDVoiceSoundMask == NULL && !(is_game_paused())) {
        sound_play(soundId, &gHUDVoiceSoundMask);
    }
}

/**
 * Set the sound ID and delay (in seconds) of the voice clip that will be played.
 */
void hud_sound_play_delayed(u16 soundID, f32 delay, s32 playerIndex) {
    if (gHudVoiceTimer == 0) {
        gHudVoiceID = soundID;
        gHudVoiceTimer = delay * 60.0f;
        gHudVoicePlayerID = playerIndex;
    }
}

/**
 * Stop the queued voice clip from playing from the HUD.
 */
void hud_sound_stop(u16 soundID, s32 playerIndex) {
    if (soundID == gHudVoiceID && gHudVoicePlayerID == playerIndex) {
        gHudVoiceTimer = 0;
    }
}

/**
 * Renders the icon in the bottom left of the current weapon.
 * The icon will spin and grow as it appears, then spins and shrinks as it disappears.
 */
void hud_weapon(Object *obj, s32 updateRate) {
    Object_Racer *racer;
    s32 weaponSpriteOffset;

    racer = obj->racer;
    if (racer->raceFinished == FALSE) {
        enable_pal_viewport_height_adjust(TRUE);
        weaponSpriteOffset = (racer->balloon_type * 3) + (racer->balloon_level);
        if (gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.prevLevel != racer->balloon_level) {
            if (racer->balloon_level == 0) {
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.scale = 120;
            } else if (gNumActivePlayers == 1) {
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.scale = 0;
            } else {
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.scale = 120;
            }
            gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.prevLevel = racer->balloon_level;
        }
        if (racer->balloon_quantity > 0) {
            if (gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation < 16 && racer->weaponDisplayLock == 0) {
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].rotation.z_rotation =
                    gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation * 0x1000;
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].scale =
                    (gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation * 0.04687) + 0.25;
            } else {
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].rotation.z_rotation = 0;
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].scale = 1.0f;
            }
            gCurrentHud->entry[HUD_WEAPON_DISPLAY].spriteOffset = weaponSpriteOffset;
            gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation += updateRate;
            if (gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation > 16) {
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation = 16;
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.scale += updateRate;
                if (gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.scale > 120) {
                    gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.scale = 120;
                } else if (gHUDNumPlayers == ONE_PLAYER) {
                    gCurrentHud->entry[HUD_WEAPON_DISPLAY].scale +=
                        0.18 *
                        sins_f(((f32) gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.scale * 682.6583 * 4.0));
                }
            }
            if (gHUDNumPlayers > ONE_PLAYER) {
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].scale *= 0.75;
            }
            if (gCurrentHud->entry[HUD_WEAPON_DISPLAY].scale != 1.0) {
                sprite_opaque(TRUE);
            }
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_WEAPON_DISPLAY]);
            sprite_opaque(FALSE);
            if (racer->balloon_quantity > 3) {
                gCurrentHud->entry[HUD_WEAPON_QUANTITY].weaponDisplay.hideTimer = -128;
            }
            if (racer->balloon_quantity < 3) {
                gCurrentHud->entry[HUD_WEAPON_QUANTITY].weaponDisplay.hideTimer -= updateRate;
            }
            if (racer->balloon_quantity >= 3 ||
                ((((gCurrentHud->entry[HUD_WEAPON_QUANTITY].weaponDisplay.hideTimer + 128) % 32) < 20) &&
                 racer->balloon_quantity != 1)) {
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 160);
                gCurrentHud->entry[HUD_WEAPON_QUANTITY].spriteOffset = racer->balloon_quantity - 1;
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_WEAPON_QUANTITY]);
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
            }
        } else {
            if (gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation > 0) {
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].rotation.z_rotation =
                    gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation * 0x1000;
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].scale =
                    (gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation * 0.04687) + 0.25;
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation -= updateRate;
                gCurrentHud->entry[HUD_WEAPON_DISPLAY].spriteOffset = weaponSpriteOffset;
                if (gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation < 0) {
                    gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.rotation = 0;
                }
                if (gHUDNumPlayers != ONE_PLAYER) {
                    gCurrentHud->entry[HUD_WEAPON_DISPLAY].scale /= 2;
                }
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_WEAPON_DISPLAY]);
            }
        }
        gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
        enable_pal_viewport_height_adjust(FALSE);
    }
}

/**
 * If currently using a magnet, render a reticle over the racer that's being targeted.
 */
void hud_magnet_reticle(Object *racerObj) {
    Sprite *entry;
    HudElement *hud;
    Object_Racer *racer;

    if (entry) {} // Fakematch
    racer = racerObj->racer;
    if (racer->magnetTargetObj != NULL && get_current_viewport() == racer->playerIndex) {
        hud = &gCurrentHud->entry[HUD_MAGNET_RETICLE];
        hud->pos.x = racer->magnetTargetObj->trans.x_position;
        hud->pos.y = racer->magnetTargetObj->trans.y_position;
        hud->pos.z = racer->magnetTargetObj->trans.z_position;
        entry = gAssetHudElements->entry[hud->spriteID];
        if (entry == NULL) {
            return;
        }
        gAssetHudElementStaleCounter[hud->spriteID] = 0;
        viewport_main(&gHudDL, &gHudMtx);
        mtx_world_origin(&gHudDL, &gHudMtx);
        render_sprite_billboard(&gHudDL, &gHudMtx, &gHudVtx, (Object *) hud, entry, RENDER_Z_UPDATE);
    }
}
