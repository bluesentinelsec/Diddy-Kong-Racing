#include "hud_internal.h"

/************ .data ************/

// clang-format off
// Base positions of all onscreen hud elements
HudElement gHudElementBase[HUD_ELEMENT_COUNT] = {
    { { { { 0, 0, 0 } } }, HUD_ASSET_0, 1.0f, { { { 53.0f, 16.0f, 0.0f } } }, 0, { { 0, 127, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_3, 1.0f, { { { 54.0f, 18.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_WEAPONS, 0.4f, { { { -120.0f, -68.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_6, 1.0f, { { { 104.0f, 16.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS, 1.0f, { { { 93.0f, 31.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_4, 1.0f, { { { 104.0f, 32.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS, 1.0f, { { { 114.0f, 31.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_BANANA_ANIM, 0.5f, { { { -3.0f, 73.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS, 1.0f, { { { 182.0f, 22.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS, 1.0f, { { { 192.0f, 22.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_13, 1.0f, { { { 239.0f, 16.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS, 1.0f, { { { 206.0f, 31.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_GO_BIG, 1.0f, { { { 0.0f, 30.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_GET_READY, 1.0f, { { { 0.0f, 30.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_FINISH, 1.0f, { { { -200.0f, 30.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_MAP_DOT, 1.0f, { { { 0.0f, 0.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_CHEQUER_FLAG, 1.0f, { { { -50.0f, 87.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_RETICLE, 2.0f, { { { -50.0f, 87.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_18, 1.0f, { { { 167.0f, 22.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_GOLD_BALLOON, 1.0f, { { { -127.0f, 88.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_18, 1.0f, { { { 52.0f, 27.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS, 1.0f, { { { 67.0f, 27.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS, 1.0f, { { { 77.0f, 27.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS, 1.0f, { { { 221.0f, 48.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_6, 1.0f, { { { 188.0f, 48.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS_SMALL, 1.0f, { { { 206.0f, 48.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_20, 0.4f, { { { -105.0f, -98.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_BANANA_STATIC, 0.5f, { { { -3.0f, 73.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_23, 1.0f, { { { -8.0f, 97.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_LAP_FINAL, 1.0f, { { { 0.0f, 30.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_LAP_LAP, 1.0f, { { { 0.0f, 30.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_LAP_2, 1.0f, { { { 0.0f, 30.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_2, 1.0f, { { { 25.0f, 48.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_INDICATOR_TURN_90, 1.0f, { { { -120.0f, 30.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_34, 0.4f, { { { -105.0f, -98.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_WRONG, 1.0f, { { { 0.0f, 30.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_WAY, 1.0f, { { { 0.0f, 30.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_PRO_AM, 1.0f, { { { 80.0f, -60.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } }, // Unused
    { { { { 0, 0, 0x6490 } } }, HUD_SPRITE_SPEEDOMETRE_ARROW, 0.5f, { { { 122.0f, -71.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_47, 0.75f, { { { 247.0f, 212.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_48, 1.0f, { { { 234.0f, 196.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_49, 1.0f, { { { 232.0f, 177.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_50, 1.0f, { { { 240.0f, 159.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_51, 1.0f, { { { 256.0f, 146.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_52, 1.0f, { { { 275.0f, 143.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_53, 1.0f, { { { 246.0f, 156.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_54, 0.7272f, { { { 27.0f, 142.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_0, 1.0f, { { { -200.0f, 70.0f, 0.0f } } }, 0, { { 0, 129, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_3, 1.0f, { { { -175.0f, 72.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_WEAPON_NUMBER, 1.0f, { { { -99.0f, -89.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_PORTRAIT, 0.76f, { { { 43.0f, 20.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_66, 1.0f, { { { 40.0f, 54.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_BANANA_ANIM, 0.375f, { { { -117.0f, 46.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_28, 1.0f, { { { 51.0f, 56.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS_SMALL, 1.0f, { { { 60.0f, 57.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_ASSET_NUMBERS_SMALL, 1.0f, { { { 69.0f, 57.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_PLACE_1, 1.0f, { { { -160.0f, 5.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_PORTRAIT, 1.0f, { { { 260.0f, 16.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
    { { { { 0, 0, 0 } } }, HUD_SPRITE_PLACE_ST, 1.0f, { { { 160.0f, 5.0f, 0.0f } } }, 0, { { 0, 0, 0, 0 } } },
};
// clang-format on

HudPresets2P gHudPresets2P[] = {
    { HUD_RACE_POSITION, 53, 16 },
    { HUD_RACE_POSITION_END, 54, 18 },
    { HUD_LAP_COUNT_CURRENT, 203, 19 },
    { HUD_LAP_COUNT_SEPERATOR, 211, 19 },
    { HUD_LAP_COUNT_TOTAL, 220, 19 },
    { HUD_WEAPON_DISPLAY, -120, -31 },
    { HUD_BANANA_COUNT_ICON_STATIC, 33, 22 },
    { HUD_BANANA_COUNT_NUMBER_1, 211, 19 },
    { HUD_BANANA_COUNT_NUMBER_2, 220, 19 },
    { HUD_RACE_TIME_NUMBER, 167, 19 },
    { HUD_RACE_END_FINISH, 0, 10 },
    { HUD_LAP_COUNT_FLAG, 59, 35 },
    { HUD_MAGNET_RETICLE, 59, 35 },
    { HUD_BANANA_COUNT_ICON_SPIN, 33, 22 },
    { HUD_BANANA_COUNT_SPARKLE, 30, 42 },
    { HUD_BANANA_COUNT_X, 202, 19 },
    { HUD_RACE_START_READY, 0, 10 },
    { HUD_RACE_START_GO, 0, 10 },
    { HUD_WRONGWAY_1, 0, 10 },
    { HUD_WRONGWAY_2, 0, 10 },
    { HUD_LAP_TEXT_LAP, 0, 10 },
    { HUD_LAP_TEXT_TWO, 0, 10 },
    { HUD_LAP_TEXT_FINAL, 0, 10 },
    { HUD_CHALLENGE_FINISH_POS_1, -200, 35 },
    { HUD_CHALLENGE_FINISH_POS_2, -175, 37 },
    { HUD_WEAPON_QUANTITY, -106, -45 },
    { HUD_LAP_COUNT_LABEL, 180, 19 },
    { HUD_CHALLENGE_PORTRAIT, 263, 17 },
    { HUD_TREASURE_METRE, 247, 45 },
    { HUD_EGG_CHALLENGE_ICON, 259, 49 },
    { HUD_SILVER_COIN_TALLY, 34, 71 },
    { -1, -1, -1 },
};

HudPresets4P gHudPresets4P[] = {
    { HUD_RACE_POSITION, 45, 16, 135 },
    { HUD_LAP_COUNT_CURRENT, 94, 19, 78 },
    { HUD_LAP_COUNT_SEPERATOR, 102, 19, 86 },
    { HUD_LAP_COUNT_TOTAL, 111, 19, 95 },
    { HUD_WEAPON_DISPLAY, -47, -36, 36 },
    { HUD_BANANA_COUNT_ICON_STATIC, 0, 22, -29 },
    { HUD_BANANA_COUNT_ICON_SPIN, 0, 22, -29 },
    { HUD_BANANA_COUNT_NUMBER_1, 97, 19, 68 },
    { HUD_BANANA_COUNT_NUMBER_2, 106, 19, 77 },
    { HUD_RACE_TIME_NUMBER, 55, 19, 26 },
    { HUD_RACE_START_GO, 0, 10, 0 },
    { HUD_RACE_START_READY, 0, 10, 0 },
    { HUD_RACE_END_FINISH, -160, 10, -160 },
    { HUD_LAP_COUNT_FLAG, 102, 19, 86 },
    { HUD_MAGNET_RETICLE, 102, 19, 86 },
    { HUD_BANANA_COUNT_X, 88, 19, 59 },
    { HUD_BANANA_COUNT_SPARKLE, -3, 42, -32 },
    { HUD_TREASURE_METRE, 18, 45, 120 },
    { HUD_LAP_TEXT_LAP, -22, 10, -22 },
    { HUD_LAP_TEXT_TWO, 12, 10, 16 },
    { HUD_LAP_TEXT_FINAL, 0, 10, -39 },
    { HUD_WRONGWAY_1, 0, 10, 0 },
    { HUD_WRONGWAY_2, 0, 10, 0 },
    { HUD_CHALLENGE_FINISH_POS_1, -200, 25, -40 },
    { HUD_CHALLENGE_FINISH_POS_2, -175, 27, -15 },
    { HUD_WEAPON_QUANTITY, -33, -50, 50 },
    { HUD_LAP_COUNT_LABEL, 71, 19, 55 },
    { HUD_EGG_CHALLENGE_ICON, 60, 20, 60 },
    { -1, -1, -1, -1 },
};

// Rare likely just used an s16 array for the above data, but we can make it match with a nicer looking struct.
// Small side effect is this padding is required to maintain matching.
UNUSED s32 gPresetPadding = 0;

HudAudio gHudAudioData[2] = {
    { SOUND_CROWD, 0, 0, NULL, 0, 0, 0, 0, 0 },
    { SOUND_UNK_FF, 0, 0, NULL, 0, 0, 0, 0, 0 },
};

u8 gShowCourseDirections = TRUE;

u8 gHudMapMode[4][4] = { { 1, 1, 1, 1 }, { 2, 2, 2, 2 }, { 2, 2, 2, 2 }, { 2, 2, 2, 2 } };

s8 gHudToggleSettings[4] = { 1, 1, 1, 1 };

s8 gPrevToggleSetting = 1;

u8 gHudIndicatorTimer = 0;

ByteColour gHudMinimapColours[40] = {
    { 255, 160, 0 }, { 255, 255, 0 },   { 0, 128, 255 },   { 255, 255, 255 }, { 0, 175, 0 }, { 0, 255, 128 },
    { 255, 0, 0 },   { 255, 128, 190 }, { 128, 128, 128 }, { 0, 0, 255 },     { 0, 0, 0 },   { 0, 0, 0 },
    { 0, 0, 0 },     { 0, 0, 0 },       { 0, 0, 0 },       { 32, 0, 0 },      { 0, 0, 0 },   { 0, 0, 64 },
    { 0, 0, 0 },     { 0, 0, 0 },       { 0, 96, 0 },      { 0, 0, 0 },       { 0, 0, 0 },   { 128, 0, 0 },
    { 0, 0, 0 },     { 0, 0, 0 },       { 0, 64, 0 },      { 0, 0, 0 },       { 0, 32, 0 },  { 64, 0, 0 },
    { 0, 0, 0 },     { 64, 0, 64 },     { 0, 0, 0 },       { 0, 0, 96 },      { 0, 64, 0 },  { 0, 0, 0 },
    { 0, 128, 0 },   { 64, 0, 0 },      { 0, 0, 0 },       { 0, 0, 0 },
};

u32 gHudColour = COLOUR_RGBA32(255, 255, 255, 254);

/*******************************/

/************ .bss ************/

s8 gMinimapOpacity;
s8 gMinimapFade;
s8 gShowHUD;
s8 gMinimapXlu;

/**
 * gRaceStartShowHudStep counts up from 0->5 on race start, and is used to
 * coordinate when certain hud actions & audio clips play/happen:
 *   - set to 0 on loading of new race
 *   - 0->1 when camera shutter open animation occurs
 *   - 1->2 when hud slides into screen with "whoosh" sound
 *   - 2->3 on the "Get Ready!" voice play
 *   - 3->4 on the "Go" voice and text
 *   - 4->5 on the "Go" sliding off screen
 *   - stays 5 until the next race starts
 */
s8 gRaceStartShowHudStep;

s8 gHudPALScale;
u8 *gAssetHudElementStaleCounter;
HudData *gCurrentHud;
HudData *gPlayerHud[MAXCONTROLLERS]; // One per active player
s16 *gAssetHudElementIds;
HudElements *gAssetHudElements;
s32 gAssetHudElementIdsCount;
Gfx *gHudDL;
Mtx *gHudMtx;
Vertex *gHudVtx;
s32 gHudCurrentViewport;
s32 gHUDNumPlayers;
s32 gHudController;
s32 gMinimapSpriteWidth;
s32 gMinimapSpriteHeight;
s32 gMinimapDotOffsetX;
s32 gMinimapDotOffsetY;
s32 gHudOffsetX; // Offset value to do the slide in animation.
s32 gHudBounceX; // Offset value to do the bounce when the hud slides in.
u16 gHudBounceTimer;
f32 gHudBounceMag;
u8 gHudRaceStart;
u8 gHudSlide;
u8 gHideRaceTimer;
u8 gNumActivePlayers;
u8 gWrongWayNagPrefix;
AudioPoint *gRaceStartSoundMask;
SoundHandle gHUDVoiceSoundMask;
SoundHandle gHudBalloonSoundMask;
u16 gHudTTSoundID;
s32 sHudBoostVoiceTimer;
s32 sHudRandomVoiceTimer;
u8 gMinimapRed;
u8 gMinimapGreen;
u8 gMinimapBlue;
s32 gMinimapScreenX;
s32 gMinimapScreenY;
LevelHeader *gHudLevelHeader;
u8 sHudTTVoiceCount;
u8 sMinimapTexIndexA;
u8 sMinimapTexIndexB;
u8 gStopwatchAnimID;
u8 sHudMapVisible;
s8 D_80126D69;
s32 gWrongWayNagTimer;
u8 gHudAudioReset;
u8 gHudTimeTrialGhost;
s32 gHudVoiceTimer;
s32 gHudVoicePlayerID;
u16 gHudVoiceID;
DrawTexture gHudSprites[128];
s32 D_80127180;
Settings *gHudSettings;
u8 gHudSilverCoinRace;
u8 gAdventurePlayerFinish;
u8 gAdvRaceStartedByP2;
u8 gMinimapOpacityTarget;
s32 gStopwatchErrorX;
s32 gStopwatchErrorY;
ColourCycle *gHudPulseLight;

/******************************/

/**
 * Load basic hud assets and allocate HUD data for each player.
 * Set up the global hud elements and reset global vars.
 */
void hud_init(UNUSED s32 viewportCount) {
    s32 i;
    s32 playerCount;

    gHUDNumPlayers = cam_get_viewport_layout();
    gNumActivePlayers = cam_set_layout(gHUDNumPlayers);
    gHudSettings = get_settings();
    gHudSilverCoinRace = check_if_silver_coin_race();
    gAssetHudElementIds = (s16 *) asset_table_load(ASSET_HUD_ELEMENT_IDS);
    {
        // Big-endian s16 id table (helper in linux/reimpl.c).
        extern void pc_swap16_buf(void *buf, u32 numBytes);
        pc_swap16_buf(gAssetHudElementIds, asset_table_size(ASSET_HUD_ELEMENT_IDS));
    }
    gAssetHudElementIdsCount = 0;

    while (gAssetHudElementIds[gAssetHudElementIdsCount] != -1) {
        gAssetHudElementIdsCount++;
    }
    gAssetHudElements = mempool_alloc_safe(gAssetHudElementIdsCount * (sizeof(void *) + 1), COLOUR_TAG_BLUE);
    // Evil pointer shenanigans to store the timer in that last byte in the struct above.
    gAssetHudElementStaleCounter =
        (u8 *) ((gAssetHudElementIdsCount + (void **) gAssetHudElements)); // past the pointer table (pointer-sized)
    for (i = 0; i < gAssetHudElementIdsCount; i++) {
        gAssetHudElementStaleCounter[i] = 0;
        gAssetHudElements->entry[i] = NULL;
    }
    gAssetHudElements->entry[HUD_ELEMENT_UNK_01] =
        tex_load_sprite(gAssetHudElementIds[HUD_ELEMENT_UNK_01] & 0x3FFF, 1); // ID: 86 - 0x56
    gAssetHudElements->entry[HUD_ELEMENT_UNK_17] =
        tex_load_sprite(gAssetHudElementIds[HUD_ELEMENT_UNK_17] & 0x3FFF, 1); // ID: 291 - 0x123
    gAssetHudElements->entry[HUD_ELEMENT_UNK_08] =
        tex_load_sprite(gAssetHudElementIds[HUD_ELEMENT_UNK_08] & 0x3FFF, 1); // ID: 156 - 0x9C
    gAssetHudElements->entry[HUD_ELEMENT_UNK_11] =
        tex_load_sprite(gAssetHudElementIds[HUD_ELEMENT_UNK_11] & 0x3FFF, 1); // ID: 60 - 0x3C
    if (gNumActivePlayers != 3) {
        playerCount = gNumActivePlayers;
    } else {
        playerCount = 4; // Create four viewports if there are three players.
    }
    gPlayerHud[PLAYER_ONE] = mempool_alloc_safe(playerCount * sizeof(HudData), COLOUR_TAG_BLUE);
    gPlayerHud[PLAYER_TWO] = (HudData *) ((u8 *) gPlayerHud[PLAYER_ONE] + sizeof(HudData));
    gPlayerHud[PLAYER_THREE] = (HudData *) ((u8 *) gPlayerHud[PLAYER_TWO] + sizeof(HudData));
    gPlayerHud[PLAYER_FOUR] = (HudData *) ((u8 *) gPlayerHud[PLAYER_THREE] + sizeof(HudData));
    hud_init_element();
    sHudTTVoiceCount = 0;
    gWrongWayNagTimer = 0;
    gHudTTSoundID = SOUND_NONE;
    sMinimapTexIndexB = 4;
    sMinimapTexIndexA = 0;
    gStopwatchAnimID = 4;
    D_80126D69 = 1;
    sHudMapVisible = 1;
    gHudAudioReset = FALSE;
    gHudVoiceID = 0;
    gHudVoiceTimer = 0;
    gRaceStartSoundMask = NULL;
    gHudBalloonSoundMask = NULL;
    gMinimapXlu = 0;
    gHudPulseLight = (ColourCycle *) get_misc_asset(ASSET_MISC_58);
    pulse_light_reset(gHudPulseLight);
    sndp_set_group_volume(0, AL_SNDP_GROUP_VOLUME_MAX);
    sndp_set_group_volume(2, AL_SNDP_GROUP_VOLUME_MAX);
    for (i = 0; i < 2; i++) {
        gHudAudioData[i].volume = 0;
        gHudAudioData[i].volumeRamp = 0;
        gHudAudioData[i].playerIndex = PLAYER_COMPUTER;
        if (gHudAudioData[i].soundMask) {
            sndp_stop(gHudAudioData[i].soundMask);
        }
    }
}

/**
 * Done on race start, set all the hud elements to their default values.
 * Numerous override cases for different game modes and multiplayer.
 */
void hud_init_element(void) {
    s32 i;
    s32 j;
    s32 activePlayers;
    HudElement *hudElement;
    HudPresets4P *preset4P;
    HudPresets2P *preset2P;
    s32 k;
    s32 temp;

    temp = level_type();
    if (temp == RACETYPE_UNK1 || temp == RACETYPE_HUBWORLD) {
        gHudOffsetX = 0;
        gHudRaceStart = TRUE;
    } else {
        if (gHUDNumPlayers < 2) {
            gHudOffsetX = SCREEN_WIDTH;
            gHudBounceMag = 2.0f;
        } else {
            gHudOffsetX = SCREEN_WIDTH - SCREEN_HEIGHT_HALF;
            gHudBounceMag = 2.0f;
        }
        gHudRaceStart = FALSE;
    }
    gHudBounceTimer = 0;
    gRaceStartShowHudStep = 0;
    gHideRaceTimer = 0;
    gWrongWayNagPrefix = 0;
    gHudSlide = 0;
    gMinimapOpacity = 0;
#if VERSION == VERSION_80
    gMinimapOpacityTarget = osTvType == OS_TV_TYPE_PAL ? 61 : 51;
#else
    gMinimapOpacityTarget = osTvType == OS_TV_TYPE_PAL ? 60 : 50;
#endif
    gMinimapFade = 0;
    gShowHUD = FALSE;
    gHudPALScale = FALSE;
    gAdventurePlayerFinish = FALSE;
    sHudBoostVoiceTimer = -100;
    sHudRandomVoiceTimer = rand_range(120, 360);
    gAdvRaceStartedByP2 = is_race_started_by_player_two();
    gStopwatchErrorX = 55;
    gStopwatchErrorY = 179;
    if (osTvType == OS_TV_TYPE_PAL) {
        gStopwatchErrorY += 30;
        gStopwatchErrorX -= 4;
    }
    gHudTimeTrialGhost = timetrial_ghost_staff() == NULL ? FALSE : TRUE;
    activePlayers = gNumActivePlayers != 3 ? gNumActivePlayers : 4;
    for (j = 0; j < activePlayers; j++) {
        gCurrentHud = gPlayerHud[j];
        for (k = 0; k < ARRAY_COUNT(gHudElementBase); k++) {
            gCurrentHud->entry[k].pos.x = gHudElementBase[k].pos.x;
            gCurrentHud->entry[k].pos.y = gHudElementBase[k].pos.y;
            gCurrentHud->entry[k].pos.z = gHudElementBase[k].pos.z;
            gCurrentHud->entry[k].scale = gHudElementBase[k].scale;
            gCurrentHud->entry[k].rotation.z = gHudElementBase[k].rotation.z;
            gCurrentHud->entry[k].rotation.y = gHudElementBase[k].rotation.y;
            gCurrentHud->entry[k].rotation.x = gHudElementBase[k].rotation.x;
            gCurrentHud->entry[k].spriteID = gHudElementBase[k].spriteID;
            gCurrentHud->entry[k].spriteOffset = gHudElementBase[k].spriteOffset;
            gCurrentHud->entry[k].timer.minutes = gHudElementBase[k].timer.minutes;
            gCurrentHud->entry[k].timer.seconds = gHudElementBase[k].timer.seconds;
            gCurrentHud->entry[k].timer.hundredths = gHudElementBase[k].timer.hundredths;
            gCurrentHud->entry[k].timer.targetPos = gHudElementBase[k].timer.targetPos;
        }
        gCurrentHud->entry[HUD_SILVER_COIN_TALLY].silverCoinTally.offsetY = 5;
        if (gHUDNumPlayers == TWO_PLAYERS) {
            for (preset2P = gHudPresets2P; preset2P->assetID != -1; preset2P++) {
                gCurrentHud->entry[preset2P->assetID].pos.x = preset2P->x;
                if ((gAssetHudElementIds[gCurrentHud->entry[preset2P->assetID].spriteID] & ASSET_MASK_TEXTURE) ==
                    ASSET_MASK_TEXTURE) {
                    if (j == PLAYER_ONE) { // Top viewport
                        gCurrentHud->entry[preset2P->assetID].pos.y = preset2P->y;
                    } else { // Bottom viewport
                        gCurrentHud->entry[preset2P->assetID].pos.y = preset2P->y + 108;
                    }
                } else if (j == PLAYER_ONE) { // Top viewport
                    gCurrentHud->entry[preset2P->assetID].pos.y = preset2P->y + 60;
                } else { // Bottom viewport
                    gCurrentHud->entry[preset2P->assetID].pos.y = preset2P->y - 48;
                }
            }
            gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = 263.0f;
            gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = 17.0f;
            gCurrentHud->entry[HUD_TREASURE_METRE].pos.x = 247.0f;
            gCurrentHud->entry[HUD_TREASURE_METRE].pos.y = 45.0f;
            gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x = 259.0f;
            gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.y = 49.0f;
            gCurrentHud->entry[HUD_BATTLE_BANANA_ICON].pos.x = 103.0f;
            gCurrentHud->entry[HUD_BATTLE_BANANA_ICON].pos.y = 49.0f;
            gCurrentHud->entry[HUD_BATTLE_BANANA_X].pos.x = 264.0f;
            gCurrentHud->entry[HUD_BATTLE_BANANA_X].pos.y = 50.0f;
            gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].pos.x = 265.0f;
            gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_1].pos.y = 51.0f;
            gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_2].pos.x = 274.0f;
            gCurrentHud->entry[HUD_BATTLE_BANANA_COUNT_2].pos.y = 51.0f;
            gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = 17.0f;
            gCurrentHud->entry[HUD_SILVER_COIN_TALLY].silverCoinTally.offsetY = 3;
        } else if (gHUDNumPlayers == THREE_PLAYERS || gHUDNumPlayers == FOUR_PLAYERS) {
            for (preset4P = gHudPresets4P; preset4P->assetID != -1; preset4P++) {
                if ((gAssetHudElementIds[gCurrentHud->entry[preset4P->assetID].spriteID] & ASSET_MASK_TEXTURE) ==
                    ASSET_MASK_TEXTURE) {
                    if (j == PLAYER_ONE || j == PLAYER_THREE) { // Left viewports
                        gCurrentHud->entry[preset4P->assetID].pos.x = preset4P->xLeft;
                    } else { // Right viewports
                        gCurrentHud->entry[preset4P->assetID].pos.x = preset4P->xRight + 160;
                    }
                    if (j == PLAYER_ONE || j == PLAYER_TWO) { // Top viewports
                        gCurrentHud->entry[preset4P->assetID].pos.y = preset4P->y;
                    } else { // Bottom viewports
                        gCurrentHud->entry[preset4P->assetID].pos.y = preset4P->y + 108;
                    }
                } else {
                    if (j == PLAYER_ONE || j == PLAYER_THREE) { // Left viewports
                        gCurrentHud->entry[preset4P->assetID].pos.x = preset4P->xLeft - 80;
                    } else { // Right viewports
                        gCurrentHud->entry[preset4P->assetID].pos.x = preset4P->xRight + 80;
                    }
                    if (j == PLAYER_ONE || j == PLAYER_TWO) { // Top viewports
                        gCurrentHud->entry[preset4P->assetID].pos.y = preset4P->y + 60;
                    } else { // Bottom viewports
                        gCurrentHud->entry[preset4P->assetID].pos.y = preset4P->y - 48;
                    }
                }
            }
        }
        if (gHUDNumPlayers != ONE_PLAYER) {
            for (i = 0; i < HUD_ELEMENT_COUNT; i++) {
                if (gCurrentHud->entry[i].spriteID == HUD_ASSET_NUMBERS) {
                    gCurrentHud->entry[i].spriteID = HUD_ASSET_NUMBERS_SMALL;
                } else if (gCurrentHud->entry[i].spriteID == HUD_ASSET_4) {
                    gCurrentHud->entry[i].spriteID = HUD_ASSET_15;
                } else if (gCurrentHud->entry[i].spriteID == HUD_ASSET_SEPERATOR) {
                    gCurrentHud->entry[i].spriteID = HUD_ASSET_SEPERATOR_SMALL;
                } else if (gCurrentHud->entry[i].spriteID == HUD_ASSET_18) {
                    gCurrentHud->entry[i].spriteID = HUD_ASSET_28;
                } else if (gCurrentHud->entry[i].spriteID == HUD_ASSET_54) {
                    gCurrentHud->entry[i].scale *= 0.6;
                } else if (i != HUD_RACE_POSITION && i != HUD_RACE_POSITION_END && i != HUD_WEAPON_DISPLAY &&
                           i != HUD_PRO_AM_LOGO && i != HUD_CHALLENGE_FINISH_POS_1 && i != HUD_CHALLENGE_FINISH_POS_2 &&
                           i != HUD_LAP_COUNT_LABEL && i != HUD_CHALLENGE_PORTRAIT && i != HUD_EGG_CHALLENGE_ICON &&
                           ((i < HUD_BATTLE_BANANA_ICON || i > HUD_BATTLE_BANANA_COUNT_2) ||
                            i == HUD_BANANA_COUNT_SPARKLE || i == HUD_BANANA_COUNT_NUMBER_2)) {
                    gCurrentHud->entry[i].scale *= 0.75;
                }
            }
        }
        if (gHUDNumPlayers >= THREE_PLAYERS) {
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].scale = 1.0f;
            gCurrentHud->entry[HUD_LAP_TEXT_TWO].scale = 1.0f;
            gCurrentHud->entry[HUD_LAP_TEXT_FINAL].scale = 1.0f;
            gCurrentHud->entry[HUD_RACE_START_GO].scale = 1.0f;
            gCurrentHud->entry[HUD_RACE_START_READY].scale = 1.0f;
            gCurrentHud->entry[HUD_WRONGWAY_1].scale = 1.0f;
            gCurrentHud->entry[HUD_WRONGWAY_2].scale = 1.0f;
            gCurrentHud->entry[HUD_RACE_END_FINISH].scale = 1.0f;
            gCurrentHud->entry[HUD_LAP_TEXT_LAP].spriteID = HUD_SPRITE_LAP_LAP_SMALL;
            gCurrentHud->entry[HUD_LAP_TEXT_TWO].spriteID = HUD_SPRITE_LAP_2_SMALL;
            gCurrentHud->entry[HUD_LAP_TEXT_FINAL].spriteID = HUD_SPRITE_LAP_FINAL_SMALL;
            gCurrentHud->entry[HUD_RACE_START_GO].spriteID = HUD_SPRITE_GO_SMALL;
            gCurrentHud->entry[HUD_RACE_START_READY].spriteID = HUD_SPRITE_GET_READY_SMALL;
            gCurrentHud->entry[HUD_WRONGWAY_1].spriteID = HUD_SPRITE_WRONG_SMALL;
            gCurrentHud->entry[HUD_WRONGWAY_2].spriteID = HUD_SPRITE_WAY_SMALL;
            gCurrentHud->entry[HUD_RACE_END_FINISH].spriteID = HUD_SPRITE_FINISH_SMALL;
        } else {
            gCurrentHud->entry[HUD_WEAPON_DISPLAY].weaponDisplay.prevLevel = 1;
            // while weaponDisplay is not correct union "type" we need the value to be set to be a s8 instead of u8
            gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].weaponDisplay.prevLevel = 1;
        }
        if (is_in_time_trial()) {
            gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].pos.x += -25.0f;
            gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1].pos.x += -25.0f;
            gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_2].pos.x += -25.0f;
            gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].pos.x += -25.0f;
            gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.x += -25.0f;
            gCurrentHud->entry[HUD_BANANA_COUNT_X].pos.x += -25.0f;
            gCurrentHud->entry[HUD_LAP_COUNT_CURRENT].pos.x += -58.0f;
            gCurrentHud->entry[HUD_LAP_COUNT_SEPERATOR].pos.x += -58.0f;
            gCurrentHud->entry[HUD_LAP_COUNT_TOTAL].pos.x += -58.0f;
            gCurrentHud->entry[HUD_LAP_COUNT_LABEL].pos.x += -58.0f;
            gCurrentHud->entry[HUD_LAP_COUNT_FLAG].pos.x += -58.0f;
        }
        switch (level_type()) {
            case RACETYPE_BOSS:
                gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].pos.x += -120.0f;
                gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1].pos.x += -120.0f;
                gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_2].pos.x += -120.0f;
                gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].pos.x += -120.0f;
                gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.x += -120.0f;
                gCurrentHud->entry[HUD_BANANA_COUNT_X].pos.x += -120.0f;
                gCurrentHud->entry[HUD_LAP_COUNT_LABEL].pos.x += 28.0f;
                gCurrentHud->entry[HUD_LAP_COUNT_CURRENT].pos.x += 28.0f;
                gCurrentHud->entry[HUD_LAP_COUNT_SEPERATOR].pos.x += 28.0f;
                gCurrentHud->entry[HUD_LAP_COUNT_TOTAL].pos.x += 28.0f;
                gCurrentHud->entry[HUD_LAP_COUNT_FLAG].pos.x += 28.0f;
                break;
            case RACETYPE_CHALLENGE_BANANAS:
                if (osTvType == OS_TV_TYPE_PAL) {
                    gCurrentHud->entry[HUD_TREASURE_METRE].pos.y -= 4.0f;
                }
                if (gNumActivePlayers >= FOUR_PLAYERS) {
                    temp = (j == PLAYER_ONE) || (j == PLAYER_THREE) ? 15 : -10;
                } else {
                    temp = gNumActivePlayers == TWO_PLAYERS ? -10 : 0;
                }
                gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.x += temp;
                gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].pos.x += temp;
                gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1].pos.x += temp;
                gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_2].pos.x += temp;
                gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].pos.x += temp;
                gCurrentHud->entry[HUD_BANANA_COUNT_X].pos.x += temp;
                break;
        }
        if (is_taj_challenge()) {
            gCurrentHud->entry[HUD_LAP_COUNT_LABEL].pos.x += 28.0f;
            gCurrentHud->entry[HUD_LAP_COUNT_CURRENT].pos.x += 28.0f;
            gCurrentHud->entry[HUD_LAP_COUNT_SEPERATOR].pos.x += 28.0f;
            gCurrentHud->entry[HUD_LAP_COUNT_TOTAL].pos.x += 28.0f;
            gCurrentHud->entry[HUD_LAP_COUNT_FLAG].pos.x += 28.0f;
        }
        if (osTvType == OS_TV_TYPE_PAL) {
            for (k = 0; (k < HUD_ELEMENT_COUNT) ^ 0; k++) {
                i = k;
                gCurrentHud->entry[k].pos.y *= 1.2;
                gCurrentHud->entry[k].pos.x -= 4.0f;
                if ((gAssetHudElementIds[gCurrentHud->entry[k].spriteID] & ASSET_MASK_TEXTURE) == ASSET_MASK_TEXTURE) {
                    gCurrentHud->entry[i].pos.y -= 10.0f;
                }
            }

            for (k = HUD_SPEEDOMETRE_0; k < HUD_SILVER_COIN_TALLY; k++) {
                gCurrentHud->entry[k].pos.y += 10.0f;
            }
        }
        gCurrentHud->entry[HUD_BALLOON_COUNT_ICON].scale = 1.0f;
        gCurrentHud->entry[HUD_BALLOON_COUNT_X].spriteID = HUD_ASSET_18;
        gCurrentHud->entry[HUD_BALLOON_COUNT_NUMBER_1].spriteID = 9;
        gCurrentHud->entry[HUD_BALLOON_COUNT_NUMBER_2].spriteID = 9;
    }
    if (level_type() & RACETYPE_CHALLENGE) {
        gPrevToggleSetting = gHudToggleSettings[1];
        gHudToggleSettings[1] = 0;
    } else {
        gHudToggleSettings[1] = gPrevToggleSetting;
    }
}

/**
 * Frees all elements in the hud, and the player hud, and flushes particles
 */
void hud_free(void) {
    s32 i;
    for (i = 0; i < gAssetHudElementIdsCount; i++) {
        if (gAssetHudElements->entry[i] != NULL) {
            if ((gAssetHudElementIds[i] & ASSET_MASK_TEXTURE) == ASSET_MASK_TEXTURE) {
                tex_free(gAssetHudElements->entry[i]);
            } else if (gAssetHudElementIds[i] & ASSET_MASK_SPRITE) {
                sprite_free((Sprite *) gAssetHudElements->entry[i]);
            } else if (gAssetHudElementIds[i] & ASSET_MASK_OBJECT) {
                free_object((Object *) gAssetHudElements->entry[i]);
            } else {
                free_3d_model((ModelInstance *) gAssetHudElements->entry[i]);
            }
            gAssetHudElements->entry[i] = NULL;
        }
    }
    mempool_free(*gPlayerHud);
    mempool_free(gAssetHudElementIds);
    gAssetHudElementIdsCount = 0;
    mempool_free(gAssetHudElements);
    gParticlePtrList_flush();
}

/**
 * Return whether the race has been started from the HUDs end.
 */
u8 race_starting(void) {
    return gHudRaceStart;
}

/**
 * Root function for the HUD updating and rendering for individual players.
 * The player can toggle specific overrides and settings for displays based on their current game modes.
 */
void hud_render_player(Gfx **dList, Mtx **mtx, Vertex **vertexList, Object *obj, s32 updateRate) {
    s32 countdown;
    Object_Racer *racer;

    gHudCurrentViewport = get_current_viewport();
    if (gAdvRaceStartedByP2) {
        obj = get_racer_object_by_port(1 - gHudCurrentViewport);
    }
    gHudLevelHeader = level_header();
    if (obj == NULL) {
        if (cutscene_id() == 10) {
            obj = get_racer_object_by_port(PLAYER_ONE);
        }
    }
    if (obj != NULL && !(gHudLevelHeader->hudFlags & 2)) {
        if (get_game_mode() != GAMEMODE_MENU) {
            gHudDL = *dList;
            gHudMtx = *mtx;
            gHudVtx = *vertexList;
            D_80127180 = 0;
            if (gMinimapFade) {
                gMinimapOpacity += updateRate;
                if (gMinimapOpacity > gMinimapOpacityTarget) {
                    gMinimapOpacity = gMinimapOpacityTarget;
                }
            } else {
                gMinimapOpacity -= updateRate;
                if (gMinimapOpacity < 0) {
                    gMinimapOpacity = 0;
                }
            }
            if (gShowHUD == FALSE) {
                racer = obj->racer;
                if (gAdvRaceStartedByP2) {
                    gHudController = 1 - racer->playerIndex;
                } else {
                    gHudController = racer->playerIndex;
                }
                gCurrentHud = gPlayerHud[gHudCurrentViewport];
                if (cutscene_id() != 10) {
                    if (gHUDNumPlayers == ONE_PLAYER) {
                        if (input_pressed(gHudController) & D_CBUTTONS && racer->raceFinished == FALSE &&
                            ((gHudLevelHeader->race_type == RACETYPE_DEFAULT) ||
                             gHudLevelHeader->race_type == RACETYPE_HORSESHOE_GULCH) &&
                            gHudRaceStart) {
                            gShowCourseDirections = 1 - gShowCourseDirections;
                            sound_play((SOUND_TING_HIGHER + gShowCourseDirections), NULL);
                            if (gShowCourseDirections) {
                                gHudIndicatorTimer = 120;
                            } else {
                                gHudIndicatorTimer = 0;
                            }
                        }
                    } else if (input_pressed(gHudController) & D_CBUTTONS && racer->raceFinished == FALSE &&
                               !(gHudLevelHeader->race_type & RACETYPE_CHALLENGE) && gHudRaceStart) {
                        if (gHudMapMode[gHUDNumPlayers][racer->playerIndex] < 3) {
                            gHudMapMode[gHUDNumPlayers][racer->playerIndex]++;
                        } else {
                            gHudMapMode[gHUDNumPlayers][racer->playerIndex] = 0;
                        }
                        sound_play((SOUND_TING_HIGHEST - (gHudMapMode[gHUDNumPlayers][racer->playerIndex] == 0)), NULL);
                    }
                    if (input_pressed(gHudController) & R_CBUTTONS && racer->raceFinished == FALSE && gHudRaceStart &&
                        gMinimapOpacity == 0) {
                        gHudToggleSettings[gHUDNumPlayers] = 1 - gHudToggleSettings[gHUDNumPlayers];
                        if (gHudToggleSettings[gHUDNumPlayers] == 0) {
                            sound_play(SOUND_TING_LOW, NULL);
                        } else {
                            sound_play(SOUND_TING_HIGH, NULL);
                        }
                    }
                }
                if (gRaceStartShowHudStep == 0) {
                    if (gHudLevelHeader->race_type & RACETYPE_CHALLENGE ||
                        gHudLevelHeader->race_type == RACETYPE_DEFAULT ||
                        gHudLevelHeader->race_type == RACETYPE_HORSESHOE_GULCH ||
                        gHudLevelHeader->race_type == RACETYPE_BOSS) {
                        gHudAudioData[0].volume = 127;
                        gHudAudioData[1].volume = 127;
                        music_stop();
                        music_channel_reset_all();
                        music_play(SEQUENCE_RACE_START_FANFARE);
                        sndp_set_active_sound_limit(12);
                    } else {
                        level_music_start(1.0f);
                    }
                    gRaceStartShowHudStep += 1;
                }
                gDPPipeSync(gHudDL++);
                rsp_init(&gHudDL);
                rdp_init(&gHudDL);
                tex_enable_modes(RENDER_ALL);
                tex_disable_modes(RENDER_Z_COMPARE);
                sprite_opaque(FALSE);
                if (check_if_showing_cutscene_camera() == FALSE && gHudRaceStart == FALSE &&
                    racer->playerIndex == PLAYER_ONE) {
                    if (gHudSlide) {
                        gHudBounceX = sins_f(gHudBounceTimer) * gHudBounceMag * 8.0f;
                        gHudBounceTimer += updateRate << 0xB;
                        if (gHudBounceTimer >= 0x8000) {
                            gHudBounceTimer -= 0x8000;
                            gHudBounceMag = gHudBounceMag / 2;
                            if (gHudBounceMag <= 0.125) {
                                gHudRaceStart = TRUE;
                                gHudOffsetX = 0;
                            }
                        }
                    } else {
                        if (gRaceStartShowHudStep == 1) {
                            sound_play(SOUND_WHOOSH1, NULL);
                            gRaceStartShowHudStep += 1;
                        }
                        gHudOffsetX -= updateRate * 13;
                        if (gHudOffsetX < 0) {
                            gHudOffsetX = 0;
                        }
                        if (gHudOffsetX == 0) {
                            gHudSlide = TRUE;
                            sound_play(SOUND_EXPLOSION2, NULL);
                            gHudBounceX = 0;
                        }
                    }
                }
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, 255);
                hud_magnet_reticle(obj);
                mtx_ortho(&gHudDL, &gHudMtx);
                gDPSetEnvColor(gHudDL++, 255, 255, 255, 0);
                countdown = get_race_countdown() >> 1;
                if (is_in_time_trial()) {
                    hud_main_time_trial(countdown, obj, updateRate);
                } else {
                    if (cutscene_id() == 10) {
                        cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
                        hud_balloons(racer);
                        cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
                    } else {
                        switch (level_type()) {
                            case RACETYPE_DEFAULT:
                            case RACETYPE_HORSESHOE_GULCH:
                                if (is_taj_challenge()) {
                                    hud_main_taj(countdown, obj, updateRate);
                                } else {
                                    hud_main_race(countdown, obj, updateRate);
                                }
                                break;
                            case RACETYPE_BOSS:
                                hud_main_boss(countdown, obj, updateRate);
                                break;
                            case RACETYPE_CHALLENGE_BANANAS:
                                hud_main_treasure(countdown, obj, updateRate);
                                break;
                            case RACETYPE_CHALLENGE_BATTLE:
                                hud_main_battle(countdown, obj, updateRate);
                                break;
                            case RACETYPE_CHALLENGE_EGGS:
                                hud_main_eggs(countdown, obj, updateRate);
                                break;
                            default:
                                hud_main_hub(obj, updateRate);
                                break;
                        }
                    }
                }
                if (racer->raceFinished == TRUE) {
                    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
                    if (is_in_time_trial()) {
                        hud_time_trial_finish(racer, updateRate);
                    } else if (cam_get_viewport_layout() == VIEWPORT_LAYOUT_1_PLAYER && racer->finishPosition == 1) {
                        if (is_in_two_player_adventure()) {
                            if (level_type() == RACETYPE_BOSS) {
                                goto showFinishRace;
                            }
                            goto block_95;
                        }
                    showFinishRace:
                        hud_race_finish_1player(racer, updateRate);
                    } else {
                    block_95:
                        hud_race_finish_multiplayer(racer, updateRate);
                    }
                    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
                }
                gMinimapFade = FALSE;
                sprite_opaque(TRUE);
                if (D_80127180) {
                    gHudSprites[D_80127180].texture = NULL;
                    texrect_draw(&gHudDL, gHudSprites, 0, 0, 255, 255, 255, 255);
                }
                *dList = gHudDL;
                *mtx = gHudMtx;
                *vertexList = gHudVtx;
                tex_enable_modes(RENDER_ALL);
            }
        }
    }
}

/**
 * Clear any sounds associated with the HUD then set the data to null.
 */
void hud_audio_init(void) {
    s32 i;
    for (i = 0; i < 2; i++) {
        if (gHudAudioData[i].soundMask) {
            sndp_stop(gHudAudioData[i].soundMask);
            gHudAudioData[i].soundMask = NULL;
            gHudAudioData[i].volume = 0;
        }
    }
}

/**
 * Play delayed voice clips if there's one waiting.
 * Also play any background audio set by the HUD.
 * This includes the sound of the crowd at the beginning and end of a race.
 */
void hud_audio_update(s32 updateRate) {
    s32 temp;
    s32 i;

    if (gHudVoiceTimer > 0) {
        gHudVoiceTimer -= updateRate;
        if (gHudVoiceTimer <= 0) {
            gHudVoiceTimer = 0;
            if (gHUDVoiceSoundMask == NULL) {
                sound_play(gHudVoiceID, &gHUDVoiceSoundMask);
            }
        }
    }
    for (i = 0; i < 2; i++) {
        if (gHudAudioData[i].volume) {
            if (gHudAudioData[i].soundMask == NULL) {
                sound_play(gHudAudioData[i].soundID, &gHudAudioData[i].soundMask);
            }
            sound_volume_set_relative(gHudAudioData[i].soundID, gHudAudioData[i].soundMask, gHudAudioData[i].volume);
            if (gHudAudioData[i].volumeRamp > 0) {
                temp = gHudAudioData[i].volumeRamp * updateRate;
                if ((127 - temp) >= gHudAudioData[i].volume) {
                    gHudAudioData[i].volume += temp;
                } else {
                    gHudAudioData[i].volumeRamp = 0;
                    gHudAudioData[i].volume = 127;
                }
            } else if (gHudAudioData[i].volumeRamp < 0) {
                temp = gHudAudioData[i].volumeRamp * updateRate;
                if (-temp < gHudAudioData[i].volume) {
                    gHudAudioData[i].volume += temp;
                } else {
                    gHudAudioData[i].volumeRamp = 0;
                    gHudAudioData[i].volume = 0;
                }
            }
        } else {
            if (gHudAudioData[i].soundMask) {
                sndp_stop(gHudAudioData[i].soundMask);
                gHudAudioData[i].soundMask = 0;
            }
        }
    }
    if (gHudAudioReset && gHudAudioData[0].volume == 0) {
        gHudAudioReset = FALSE;
        if (gNumActivePlayers == 1) {
            sndp_set_active_sound_limit(8);
        } else if (gNumActivePlayers == 2) {
            sndp_set_active_sound_limit(12);
        } else {
            sndp_set_active_sound_limit(16);
        }
    }
}

/**
 * Returns the hud settings value for the mumber of active players.
 * For player 1, it's switching between the minimap and speedometre.
 * For 2 players, it's for toggling the minimap
 * For 3 players, it's for toggling between the minimap and a spectator cam in viewport 4.
 * For 4 players, it's for toggling the minimap.
 */
s8 hud_setting(void) {
    return gHudToggleSettings[gHUDNumPlayers];
}

/**
 * Renders HUD elements concerning all players.
 * This includes the minimap and score counters for challenge modes.
 */
void hud_render_general(Gfx **dList, Mtx **mtx, Vertex **vtx, s32 updateRate) {
    Object_Racer *curRacerObj;
    LevelModel *lvlMdl;
    Sprite *minimap;
    Object **objectGroup;
    Object_Racer *tempVar4;
    Object_Racer *someRacer;
    s32 i;
    s32 objectCount;
    HudElement hudElem;
    f32 sp11C;
    f32 sp118;
    f32 sp114;
    u8 sp113;
    s32 mapOpacity;
    s32 opacity;
    UNUSED s32 pad0[2];
    s32 temp_s1_2;
    s32 var_a0_5;
    s32 spF4;
    s32 spF0;
    s32 tempVar1;
    s32 temp_s1_3;
    s32 spE4;
    s32 spE0;
    UNUSED s32 pad1;
    Object *temp_v0_8;
    s32 tempVar2;
    s32 sp144;
    s32 racerCount;
    UNUSED s32 pad2;
    Camera *someObjSeg;
    Object **racerGroup;
    s32 spBC;
    s32 temp_s0_2;
    s32 temp_s0_3;
    s32 temp_s1;
    s32 position2;
    s32 temp_f16;
    s32 temp_f6;
    s32 temp_s3;
    s32 temp;
    UNUSED s32 pad3[4];

    hud_audio_update(updateRate);

    gAssetHudElementStaleCounter[HUD_ELEMENT_UNK_01] = 0;
    gAssetHudElementStaleCounter[HUD_ELEMENT_UNK_17] = 0;
    gAssetHudElementStaleCounter[HUD_ELEMENT_UNK_08] = 0;
    gAssetHudElementStaleCounter[HUD_ELEMENT_UNK_11] = 0;
    D_80127180 = 0;

    if (gMinimapXlu & 2) {
        mapOpacity = 255.0f - ((gMinimapOpacity * 255.0f) / gMinimapOpacityTarget);
    } else {
        mapOpacity = 255;
    }

    for (i = 0; i < gAssetHudElementIdsCount; i++) {
        if (gAssetHudElements->entry[i] && i != 40) {
            if (++gAssetHudElementStaleCounter[i] > 60) {
                if ((gAssetHudElementIds[i] & ASSET_MASK_TEXTURE) == ASSET_MASK_TEXTURE) {
                    tex_free((TextureHeader *) gAssetHudElements->entry[i]);
                } else if (gAssetHudElementIds[i] & ASSET_MASK_SPRITE) {
                    sprite_free((Sprite *) gAssetHudElements->entry[i]);
                } else if (gAssetHudElementIds[i] & ASSET_MASK_OBJECT) {
                    free_object((Object *) gAssetHudElements->entry[i]);
                } else {
                    free_3d_model((ModelInstance *) gAssetHudElements->entry[i]);
                }
                gAssetHudElements->entry[i] = 0;
            }
        }
    }

    gHudLevelHeader = level_header();
    objectGroup = get_racer_objects_by_port(&objectCount);
    gHudDL = *dList;
    gHudMtx = *mtx;
    gHudVtx = *vtx;
    if (gHudLevelHeader->race_type == RACETYPE_CHALLENGE_EGGS) {
        if (gNumActivePlayers == 2 && gHudToggleSettings[gHUDNumPlayers] == 0) {
            hud_draw_eggs(NULL, updateRate);
            rendermode_reset(&gHudDL);
        } else {
            // Render player 4's egg challenge portrait if they are AI controlled.
            if (gNumActivePlayers == 3) {
                tempVar4 = NULL;
                for (tempVar2 = 0; tempVar2 < objectCount; tempVar2++) {
                    if (objectGroup[tempVar2]->racer->playerIndex == PLAYER_COMPUTER) {
                        tempVar4 = objectGroup[tempVar2]->racer;
                    }
                }
                if (tempVar4 != NULL) {
                    position2 = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x;
                    temp_s1 = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y;
                    spF4 = gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x;
                    spF0 = gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.y;
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = 225.0f;
                    gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x = 221.0f;
                    gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.y = 197.0f;
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = 165.0f;
                    if (osTvType == OS_TV_TYPE_PAL) {
                        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x -= 4.0f;
                        gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x -= 4.0f;
                        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y *= 1.1;
                        gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.y *= 1.1;
                    }
                    hud_eggs_portrait(tempVar4, updateRate);
                    gHudColour = COLOUR_RGBA32(255, 255, 255, 254);
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = position2;
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = temp_s1;
                    gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x = spF4;
                    gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.y = spF0;
                }
            }
        }
    } else if (gHudLevelHeader->race_type == RACETYPE_CHALLENGE_BATTLE) {
        // Show life counter and icons in 2 player.
        if (gNumActivePlayers == 2 && gHudToggleSettings[gHUDNumPlayers] == 0) {
            cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
            sprite_opaque(FALSE);
            mtx_ortho(&gHudDL, &gHudMtx);
            hud_battle_portraits(0, updateRate);
            cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
            rendermode_reset(&gHudDL);
            sprite_opaque(TRUE);
        } else {
            // In 3/4 player, just show the life counter.
            if (gNumActivePlayers == 3) {
                curRacerObj = NULL;
                for (tempVar2 = 0; tempVar2 < objectCount; tempVar2++) {
                    if (objectGroup[tempVar2]->racer->playerIndex == PLAYER_COMPUTER) {
                        curRacerObj = objectGroup[tempVar2]->racer;
                    }
                }
                if (curRacerObj != NULL) {
                    gCurrentHud = gPlayerHud[PLAYER_FOUR];
                    spE4 = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x;
                    spE0 = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y;
                    sprite_opaque(FALSE);
                    mtx_ortho(&gHudDL, &gHudMtx);
                    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = 225.0f;
                    if (osTvType == OS_TV_TYPE_PAL) {
                        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = 181.5f;
                    } else {
                        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = 165.0f;
                    }
                    if (osTvType == OS_TV_TYPE_PAL) {
                        var_a0_5 = (66.0f - gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.x) - 4.0f;
                        temp = -114.0f - gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.y;
                    } else {
                        var_a0_5 = 66.0f - gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.x;
                        temp = -100.0f - gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.y;
                    }
                    gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.x += var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.y += temp;
                    gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].pos.x += var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].pos.y += temp;
                    gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].pos.x += var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].pos.y += temp;
                    gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1].pos.x += var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1].pos.y -= temp + 1;
                    gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_2].pos.x += var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_2].pos.y -= temp + 1;
                    gCurrentHud->entry[HUD_BANANA_COUNT_X].pos.x += var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_X].pos.y -= temp;
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].spriteID =
                        curRacerObj->characterId + HUD_SPRITE_PORTRAIT;
                    if (osTvType == OS_TV_TYPE_PAL) {
                        gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x -= 4.0f;
                        gCurrentHud->entry[HUD_EGG_CHALLENGE_ICON].pos.x -= 4.0f;
                    }
                    gHudPALScale = TRUE;
                    hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT]);
                    gHudPALScale = FALSE;
                    if (gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].bananaCountIconSpin.visualCounter == 0 &&
                        curRacerObj->bananas == 10) {
                        gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].bananaCountIconSpin.visualCounter =
                            curRacerObj->bananas;
                    }
                    hud_bananas(curRacerObj, updateRate);
                    gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.x -= var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_ICON_STATIC].pos.y -= temp;
                    gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].pos.x -= var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_ICON_SPIN].pos.y -= temp;
                    gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].pos.x -= var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_SPARKLE].pos.y -= temp;
                    gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1].pos.x -= var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_1].pos.y += temp + 1;
                    gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_2].pos.x -= var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_NUMBER_2].pos.y += temp + 1;
                    gCurrentHud->entry[HUD_BANANA_COUNT_X].pos.x -= var_a0_5;
                    gCurrentHud->entry[HUD_BANANA_COUNT_X].pos.y += temp;
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = spE4;
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = spE0;
                    sprite_opaque(TRUE);
                    rendermode_reset(&gHudDL);
                    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
                }
            }
        }
    } else if (gHudLevelHeader->race_type == RACETYPE_CHALLENGE_BANANAS) {
        if (gNumActivePlayers == 2 && gHudToggleSettings[gHUDNumPlayers] == 0) {
            temp_s0_2 = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x;
            temp_s1_2 = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y;
            temp_s3 = gCurrentHud->entry[HUD_TREASURE_METRE].pos.x;
            spBC = gCurrentHud->entry[HUD_TREASURE_METRE].pos.y;
            racerGroup = get_racer_objects(&racerCount);
            for (sp144 = 0; sp144 < racerCount; sp144++) {
                someRacer = racerGroup[sp144]->racer;
                hud_treasure(someRacer);
                if (osTvType == OS_TV_TYPE_PAL) {
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y += 66.0;
                    gCurrentHud->entry[HUD_TREASURE_METRE].pos.y += 66.0;
                } else {
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y += 55.0f;
                    gCurrentHud->entry[HUD_TREASURE_METRE].pos.y += 55.0f;
                }
            }
            gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = temp_s0_2;
            gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = temp_s1_2;
            gCurrentHud->entry[HUD_TREASURE_METRE].pos.x = temp_s3;
            gCurrentHud->entry[HUD_TREASURE_METRE].pos.y = spBC;
        } else if (gNumActivePlayers == 3) {
            tempVar4 = NULL;
            for (tempVar2 = 0; tempVar2 < objectCount; tempVar2++) {
                if (objectGroup[tempVar2]->racer->playerIndex == PLAYER_COMPUTER) {
                    tempVar4 = objectGroup[tempVar2]->racer;
                }
            }
            if (tempVar4 != NULL) {
                temp_s0_3 = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x;
                temp_s1_3 = gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y;
                temp_f16 = gCurrentHud->entry[HUD_TREASURE_METRE].pos.x;
                temp_f6 = gCurrentHud->entry[HUD_TREASURE_METRE].pos.y;
                gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = 225.0f;
                gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = 165.0f;
                gCurrentHud->entry[HUD_TREASURE_METRE].pos.x = 209.0f;
                gCurrentHud->entry[HUD_TREASURE_METRE].pos.y = 193.0f;
                if (osTvType == OS_TV_TYPE_PAL) {
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x -= 4.0f;
                    gCurrentHud->entry[HUD_TREASURE_METRE].pos.x -= 4.0f;
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y *= 1.1;
                    gCurrentHud->entry[HUD_TREASURE_METRE].pos.y *= 1.1;
                    gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y =
                        (s32) gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y;
                    gCurrentHud->entry[HUD_TREASURE_METRE].pos.y = (s32) gCurrentHud->entry[HUD_TREASURE_METRE].pos.y;
                }
                hud_treasure(tempVar4);
                gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.x = temp_s0_3;
                gCurrentHud->entry[HUD_CHALLENGE_PORTRAIT].pos.y = temp_s1_3;
                gCurrentHud->entry[HUD_TREASURE_METRE].pos.x = temp_f16;
                gCurrentHud->entry[HUD_TREASURE_METRE].pos.y = temp_f6;
            }
        }
    }
    if (D_80127180 != 0) {
        gHudSprites[D_80127180].texture = NULL;
        texrect_draw(&gHudDL, gHudSprites, 0, 0, 255, 255, 255, 255);
    }
    *dList = gHudDL;
    *mtx = gHudMtx;
    *vtx = gHudVtx;
    if ((gHudLevelHeader->hudFlags & 1)) {
        return;
    }

    sp113 = TRUE;
    for (i = 0; i < objectCount; i++) {
        someRacer = objectGroup[i]->racer;
        if (someRacer != NULL && someRacer->playerIndex != PLAYER_COMPUTER && someRacer->raceFinished == FALSE) {
            sp113 = FALSE;
        }
    }
    if ((gHUDNumPlayers == THREE_PLAYERS && is_postrace_viewport_active()) || check_if_showing_cutscene_camera() ||
        sp113) {
        goto test;
    } else if (gHudToggleSettings[gHUDNumPlayers] != 1) {
    test:
        return;
    }

    rendermode_reset(&gHudDL);
    mtx_ortho(&gHudDL, &gHudMtx);
    lvlMdl = get_current_level_model();
    if (lvlMdl == NULL) {
        return;
    }
    someObjSeg = cam_get_active_camera();
    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    minimap = (Sprite *) lvlMdl->minimapSpriteIndex;
    switch (gHUDNumPlayers) {
        case TWO_PLAYERS:
            gMinimapScreenX = 135;
            gMinimapScreenY = -gMinimapDotOffsetY / 2;
            break;
        case THREE_PLAYERS:
            if (level_type() == RACETYPE_CHALLENGE_EGGS || level_type() == RACETYPE_CHALLENGE_BATTLE ||
                level_type() == RACETYPE_CHALLENGE_BANANAS) {
                gMinimapScreenX = (gMinimapDotOffsetX / 2) - 8;
                gMinimapScreenY = -gMinimapDotOffsetY / 2;
            } else {
                gMinimapScreenX = (gMinimapDotOffsetX / 2) + 72;
                gMinimapScreenY = -60 - (gMinimapDotOffsetY / 2);
            }
            break;
        case FOUR_PLAYERS:
            gMinimapScreenX = (gMinimapDotOffsetX / 2) - 8;
            gMinimapScreenY = -gMinimapDotOffsetY / 2;
            break;
        default:
            gMinimapScreenX = 135;
            gMinimapScreenY = -98;
            break;
    }
    if (osTvType == OS_TV_TYPE_PAL) {
        gMinimapScreenY *= 1.2;
    }
    sprite_opaque(FALSE);
    hudElem.pos.x = gMinimapScreenX + gHudOffsetX + gHudBounceX;
    hudElem.pos.y = gMinimapScreenY;
    if (osTvType == OS_TV_TYPE_PAL) {
        hudElem.pos.x -= 4.0f;
    }
    hudElem.rotation.z_rotation = -someObjSeg->trans.rotation.z_rotation;
    hudElem.rotation.x_rotation = 0;
    if (get_filtered_cheats() & CHEAT_MIRRORED_TRACKS) {
        hudElem.rotation.y_rotation = -0x8000;
        hudElem.pos.x -= gMinimapDotOffsetX;
    } else {
        hudElem.rotation.y_rotation = 0;
    }
    hudElem.spriteOffset = 0;
    hudElem.pos.z = 0.0f;
    hudElem.scale = 1.0f;
    opacity = mapOpacity;
    if (mapOpacity > 160) {
        mapOpacity = 160;
    }
    if (gNumActivePlayers == 3) {
        gDPSetPrimColor(gHudDL++, 0, 0, 255, 255, 255, mapOpacity);
    } else {
        gDPSetPrimColor(gHudDL++, 0, 0, gMinimapRed, gMinimapGreen, gMinimapBlue, mapOpacity);
    }
    render_ortho_triangle_image(&gHudDL, &gHudMtx, &gHudVtx, (ObjectSegment *) &hudElem, minimap, 0);
    sp11C = (lvlMdl->upperXBounds - lvlMdl->lowerXBounds) / (f32) (lvlMdl->upperZBounds - lvlMdl->lowerZBounds);
    sp118 = coss_f((lvlMdl->minimapRotation * 0xFFFF) / 360);
    sp114 = sins_f((lvlMdl->minimapRotation * 0xFFFF) / 360);
    if (is_in_time_trial() && timetrial_valid_player_ghost()) {
        temp_v0_8 = timetrial_player_ghost();
        if (temp_v0_8 != NULL) {
            minimap_marker_pos(temp_v0_8->trans.x_position, temp_v0_8->trans.z_position, sp114, sp118, sp11C);
            gCurrentHud->entry[HUD_MINIMAP_MARKER].spriteID = HUD_SPRITE_MAP_DOT;
            gCurrentHud->entry[HUD_MINIMAP_MARKER].rotation.z_rotation = 0;
            gCurrentHud->entry[HUD_MINIMAP_MARKER].scale = 1.0f;
            tempVar1 = (opacity * (f32) temp_v0_8->opacity) * (1.0 / 128.0);
            gDPSetPrimColor(gHudDL++, 0, 0, 60, 60, 60, tempVar1);
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_MINIMAP_MARKER]);
        }
    }
    temp_v0_8 = timetrial_ghost_staff();
    if (temp_v0_8 != NULL) {
        minimap_marker_pos(temp_v0_8->trans.x_position, temp_v0_8->trans.z_position, sp114, sp118, sp11C);
        gCurrentHud->entry[HUD_MINIMAP_MARKER].rotation.z_rotation = 0;
        tempVar1 = (opacity * (f32) temp_v0_8->opacity) * (1.0 / 128.0);
        gDPSetPrimColor(gHudDL++, 0, 0, gHudMinimapColours[8].red, gHudMinimapColours[8].green,
                        gHudMinimapColours[8].blue, tempVar1);
        gCurrentHud->entry[HUD_MINIMAP_MARKER].scale = 1.0f;
        gCurrentHud->entry[HUD_MINIMAP_MARKER].spriteID = HUD_SPRITE_MAP_DOT;
        hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_MINIMAP_MARKER]);
    }
    // Draw Taj's minimap icon.
    if (gHudLevelHeader->race_type == RACETYPE_HUBWORLD) {
        temp_v0_8 = find_taj_object();
        if (temp_v0_8 != NULL) {
            gCurrentHud->entry[HUD_MINIMAP_MARKER].spriteID = HUD_SPRITE_MAP_DOT;
            minimap_marker_pos(temp_v0_8->trans.x_position, temp_v0_8->trans.z_position, sp114, sp118, sp11C);
            gCurrentHud->entry[HUD_MINIMAP_MARKER].rotation.z_rotation = 0;
            gDPSetPrimColor(gHudDL++, 0, 0, 255, 0, 255, opacity);
            gCurrentHud->entry[HUD_MINIMAP_MARKER].scale = 1.0f;
            hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_MINIMAP_MARKER]);
        }
    }
    // Draw racer minimap icons.
    // Non AI players get an arrow instead of a blob that rotates.
    for (i = objectCount - 1; i >= 0; i--) {
        temp_v0_8 = objectGroup[i];
        someRacer = objectGroup[i]->racer;
        if (someRacer != NULL) {
            minimap_marker_pos(objectGroup[i]->trans.x_position, objectGroup[i]->trans.z_position, sp114, sp118, sp11C);
            if (someRacer->playerIndex != PLAYER_COMPUTER) {
                gCurrentHud->entry[HUD_MINIMAP_MARKER].pos.y -= 1.0f;
                gCurrentHud->entry[HUD_MINIMAP_MARKER].spriteID = HUD_SPRITE_MAP_ARROW;
                gCurrentHud->entry[HUD_MINIMAP_MARKER].rotation.z_rotation =
                    (objectGroup[i]->trans.rotation.y_rotation - ((lvlMdl->minimapRotation * 0xFFFF) / 360)) & 0xFFFF;

                if (get_filtered_cheats() & CHEAT_MIRRORED_TRACKS) {
                    gCurrentHud->entry[HUD_MINIMAP_MARKER].rotation.z_rotation =
                        0xFFFF - gCurrentHud->entry[HUD_MINIMAP_MARKER].rotation.z_rotation;
                }
                sprite_opaque(TRUE);
            } else {
                gCurrentHud->entry[HUD_MINIMAP_MARKER].rotation.z_rotation = 0;
                gCurrentHud->entry[HUD_MINIMAP_MARKER].spriteID = HUD_SPRITE_MAP_DOT;
            }
            if (is_taj_challenge() && someRacer->vehicleID == VEHICLE_CARPET) {
                gDPSetPrimColor(gHudDL++, 0, 0, 255, 0, 255, opacity);
            } else {
                gDPSetPrimColor(gHudDL++, 0, 0, gHudMinimapColours[someRacer->characterId].red,
                                gHudMinimapColours[someRacer->characterId].green,
                                gHudMinimapColours[someRacer->characterId].blue, opacity);
            }
            if (!(level_type() & RACETYPE_CHALLENGE) || (!someRacer->raceFinished)) {
                if (osTvType == OS_TV_TYPE_PAL) {
                    gCurrentHud->entry[HUD_MINIMAP_MARKER].pos.x -= 4.0f;
                }
                if (level_type() == RACETYPE_CHALLENGE_BATTLE) {
                    switch (someRacer->elevation) {
                        case ELEVATION_LOW:
                            gCurrentHud->entry[HUD_MINIMAP_MARKER].scale = 0.8f;
                            break;
                        case ELEVATION_NORMAL:
                            gCurrentHud->entry[HUD_MINIMAP_MARKER].scale = 1.0f;
                            break;
                        case ELEVATION_HIGH:
                        case ELEVATION_HIGHEST:
                            gCurrentHud->entry[HUD_MINIMAP_MARKER].scale = 1.2f;
                            break;
                    }
                } else {
                    gCurrentHud->entry[HUD_MINIMAP_MARKER].scale = 1.0f;
                }
                hud_element_render(&gHudDL, &gHudMtx, &gHudVtx, &gCurrentHud->entry[HUD_MINIMAP_MARKER]);
            }
            sprite_opaque(FALSE);
        }
    }
    gDPPipeSync(gHudDL++);
    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
    sprite_opaque(TRUE);
    *dList = gHudDL;
    *mtx = gHudMtx;
    *vtx = gHudVtx;
}

/**
 * Renders a HUD element onscreen.
 * Loads the asset if it does not yet exist, and refreshes the stale counter to keep it loaded.
 * Since hud elements can be all sorts of things, the function checks what asset type it is, before
 * deciding to draw a texture rectangle, ortho tris, or a 3D model entirely.
 */
void hud_element_render(Gfx **dList, Mtx **mtx, Vertex **vtxList, HudElement *hud) {
    ModelInstance *model;
    TextureHeader *textureHeader2;
    TextureHeader *textureHeader;
    Camera *camera;
    LevelObjectEntry_Hud objEntry;
    UNUSED s32 pad1;
    Object *tempObject;
    HudDrawTexture sp88;
    UNUSED s32 pad2[3];
    HudDrawTexture sp70;
    s32 alpha;
    u32 colour;
    s32 spriteID;
    HudDrawTexture sp58;
    UNUSED s32 pad3;
    HudDrawTexture sp48;
    Sprite *sprite;
    UNUSED s32 pad4[2];
    s32 spriteElementId;

    spriteID = hud->spriteID;
    spriteElementId = gAssetHudElementIds[spriteID];
    if ((spriteElementId & ASSET_MASK_TEXTURE) == ASSET_MASK_TEXTURE) {
        objEntry.offsetY = -gMinimapOpacity;
    } else {
        objEntry.offsetY = gMinimapOpacity;
    }

    if (gAssetHudElements->entry[spriteID] == NULL) {
        if ((spriteElementId & ASSET_MASK_TEXTURE) == ASSET_MASK_TEXTURE) {
            gAssetHudElements->entry[hud->spriteID] = load_texture(spriteElementId & 0x3FFF);
        } else if (spriteElementId & ASSET_MASK_SPRITE) {
            gAssetHudElements->entry[hud->spriteID] = tex_load_sprite(spriteElementId & 0x3FFF, 1);
        } else if (spriteElementId & ASSET_MASK_OBJECT) {
            objEntry.common.objectID = spriteElementId & 0xFF;
            objEntry.common.size = ((gAssetHudElementIds[hud->spriteID] & 0x100) >> 1) | sizeof(LevelObjectEntryCommon);
            objEntry.common.x = 0;
            objEntry.common.y = 0;
            objEntry.common.z = 0;
            gAssetHudElements->entry[hud->spriteID] =
                spawn_object((LevelObjectEntryCommon *) &objEntry, OBJECT_SPAWN_NONE);
        } else {
            gAssetHudElements->entry[hud->spriteID] = object_model_init(spriteElementId, 0);
        }

        spriteID = hud->spriteID;
        if (gAssetHudElements->entry[spriteID] == NULL) {
            return;
        }
    }

    gAssetHudElementStaleCounter[spriteID] = 0;

    gHudDL = *dList;
    gHudMtx = *mtx;
    gHudVtx = *vtxList;
    spriteID = hud->spriteID;
    if (spriteID != HUD_SPEEDOMETRE_30) {
        if (spriteID != HUD_RACE_END_FINISH && spriteID != HUD_BANANA_COUNT_ICON_STATIC &&
            (spriteID < HUD_CHALLENGE_FINISH_POS_1 || spriteID >= HUD_BATTLE_BANANA_COUNT_1) &&
            spriteID != HUD_SILVER_COIN_TALLY && gMinimapXlu & 1) {
            hud->pos.y += objEntry.offsetY;
        }
        hud->pos.x += gHudOffsetX + gHudBounceX;
    }
    spriteID = hud->spriteID;
    if ((gAssetHudElementIds[spriteID] & ASSET_MASK_TEXTURE) == ASSET_MASK_TEXTURE) {
        if (spriteID >= HUD_CHALLENGE_FINISH_POS_1 && spriteID < HUD_BATTLE_BANANA_COUNT_1) {
            if (osTvType == OS_TV_TYPE_PAL) {
                hud->pos.y -= 8.0f;
            }

            if (gMinimapXlu & 2) {
                alpha = 255.0f - ((gMinimapOpacity * 255.0f) / gMinimapOpacityTarget);
            } else {
                alpha = 255;
            }
            spriteID = hud->spriteID;
            textureHeader = gAssetHudElements->entry[spriteID];
            textureHeader = (TextureHeader *) (((u8 *) textureHeader) + hud->spriteOffset * textureHeader->textureSize);
            sp88.drawTexture.texture = textureHeader;
            sp88.drawTexture.xOffset = hud->pos.x;
            sp88.drawTexture.yOffset = hud->pos.y;
            sp88.terminator = 0;
            if (osTvType == OS_TV_TYPE_PAL) {
                if (hud->spriteID == HUD_BATTLE_BANANA_X) {
                    if (alpha == 255) {
                        colour = COLOUR_RGBA32(255, 255, 255, 254);
                    } else {
                        colour = COLOUR_RGBA32(255, 255, 255, alpha);
                    }
                    sp88.drawTexture.xOffset = 0;
                    sp88.drawTexture.yOffset = 0;
                    texrect_draw_scaled(&gHudDL, &sp88.drawTexture, hud->pos.x, hud->pos.y, 1.0f, 1.1f, colour,
                                        TEXRECT_POINT);
                } else {
                    texrect_draw(&gHudDL, &sp88.drawTexture, 0, 0, 0xFF, 0xFF, 0xFF, alpha);
                }
            } else {
                texrect_draw(&gHudDL, &sp88.drawTexture, 0, 0, 0xFF, 0xFF, 0xFF, alpha);
            }
            if (osTvType == OS_TV_TYPE_PAL) {
                hud->pos.y += 8.0f;
            }
        } else if (gHudPALScale && osTvType == OS_TV_TYPE_PAL) {
            textureHeader = gAssetHudElements->entry[spriteID];
            textureHeader = (TextureHeader *) (((u8 *) textureHeader) + hud->spriteOffset * textureHeader->textureSize);
            sp70.drawTexture.texture = textureHeader;
            sp70.drawTexture.xOffset = 0;
            sp70.drawTexture.yOffset = 0;
            sp70.terminator = 0;
            texrect_draw_scaled(&gHudDL, &sp70.drawTexture, hud->pos.x, hud->pos.y, hud->scale, hud->scale * 1.1,
                                gHudColour, TEXRECT_POINT);
        } else if (hud->scale == 1.0) {
            textureHeader2 = gAssetHudElements->entry[spriteID];
            textureHeader2 =
                (TextureHeader *) (((u8 *) textureHeader2) + hud->spriteOffset * textureHeader2->textureSize);
            if (gHudColour == COLOUR_RGBA32(255, 255, 255, 254)) {
                gHudSprites[D_80127180].texture = textureHeader2;
                gHudSprites[D_80127180].xOffset = hud->pos.x;
                gHudSprites[D_80127180].yOffset = hud->pos.y;
                D_80127180++;
            } else {
                sp58.drawTexture.texture = textureHeader2;
                sp58.drawTexture.xOffset = hud->pos.x;
                sp58.drawTexture.yOffset = hud->pos.y;
                sp58.terminator = 0;
                texrect_draw(&gHudDL, &sp58.drawTexture, 0, 0, (gHudColour >> 24) & 0xFF, (gHudColour >> 16) & 0xFF,
                             (gHudColour >> 8) & 0xFF, gHudColour & 0xFF);
                rendermode_reset(&gHudDL);
            }
        } else {
            textureHeader = gAssetHudElements->entry[spriteID];
            textureHeader = (TextureHeader *) (((u8 *) textureHeader) + hud->spriteOffset * textureHeader->textureSize);
            sp48.drawTexture.texture = textureHeader;
            sp48.drawTexture.xOffset = 0;
            sp48.drawTexture.yOffset = 0;
            sp48.terminator = 0;
            texrect_draw_scaled(&gHudDL, &sp48.drawTexture, hud->pos.x, hud->pos.y, hud->scale, hud->scale, gHudColour,
                                TEXRECT_POINT);
            rendermode_reset(&gHudDL);
        }
    } else if (gAssetHudElementIds[spriteID] & ASSET_MASK_SPRITE) {
        camera = cam_get_active_camera();
        sprite = gAssetHudElements->entry[hud->spriteID];
        hud->rotation.z -= camera->trans.rotation.z;
        render_ortho_triangle_image(&gHudDL, &gHudMtx, &gHudVtx, (ObjectSegment *) hud, sprite, 0);
        hud->rotation.z += camera->trans.rotation.z;
    } else if (gAssetHudElementIds[spriteID] & ASSET_MASK_OBJECT) {
        tempObject = gAssetHudElements->entry[spriteID];
        tempObject->trans.rotation.x = hud->rotation.x;
        tempObject->trans.rotation.y = hud->rotation.y;
        tempObject->trans.rotation.z = hud->rotation.z;
        tempObject->trans.x_position = hud->pos.x;
        tempObject->trans.y_position = hud->pos.y;
        tempObject->trans.z_position = hud->pos.z;
        tempObject->trans.scale = hud->scale;
        tempObject->modelIndex = 0;
        tempObject->opacity = 0xFF;
        render_object(&gHudDL, &gHudMtx, &gHudVtx, tempObject);
    } else {
        mtx_cam_push(&gHudDL, &gHudMtx, (ObjectTransform *) hud, 1.0f, 0.0f);
        if (0) {}
        model = gAssetHudElements->entry[hud->spriteID];
        hud_draw_model(model->objModel);
        mtx_pop(&gHudDL);
    }
    spriteID = hud->spriteID;
    if (spriteID != HUD_SPRITE_PRO_AM) {
        if (spriteID != HUD_SPRITE_MAP_DOT && spriteID != HUD_SPRITE_MAP_ARROW &&
            (spriteID < HUD_ASSET_47 || spriteID >= HUD_ASSET_54) && spriteID != HUD_SPRITE_SPEEDOMETRE_ARROW &&
            gMinimapXlu & 1) {
            hud->pos.y -= objEntry.offsetY;
        }
        hud->pos.x -= gHudOffsetX + gHudBounceX;
    }
    *dList = gHudDL;
    *mtx = gHudMtx;
    *vtxList = gHudVtx;
}

/**
 * Renders a 3D model onscreen.
 * Used in conjunction with the HUD.
 */
void hud_draw_model(ObjectModel *objModel) {
    UNUSED s32 pad;
    TextureHeader *texPtr;
    s32 triOffset;
    s32 vertOffset;
    s32 numBatches;
    s32 numVerts;
    s32 numTris;
    Vertex *verts;
    Triangle *tris;
    s32 textureEnabled;
    s32 i;
    s32 texIndex;
    s32 flags;

    numBatches = objModel->numberOfBatches;

    for (i = 0; i < numBatches; i++) {
        flags = objModel->batches[i].flags;
        if (!(flags & RENDER_Z_UPDATE)) {
            vertOffset = objModel->batches[i].verticesOffset;
            triOffset = objModel->batches[i].facesOffset;
            numVerts = objModel->batches[i + 1].verticesOffset - vertOffset;
            numTris = objModel->batches[i + 1].facesOffset - triOffset;
            verts = &objModel->vertices[vertOffset];
            tris = &objModel->triangles[triOffset];
            texIndex = objModel->batches[i].textureIndex; // 0xFF = No Texture
            textureEnabled = TRIN_ENABLE_TEXTURE;
            if (texIndex == 0xFF) {
                texPtr = NULL;
            } else {
                texPtr = objModel->textures[texIndex].texture;
            }
            material_set_no_tex_offset(&gHudDL, texPtr, flags & ~RENDER_Z_COMPARE);
            gSPVertexDKR(gHudDL++, OS_K0_TO_PHYSICAL(verts), numVerts, 0);
            if (texPtr == NULL) {
                textureEnabled = TRIN_DISABLE_TEXTURE;
            }
            gSPPolygon(gHudDL++, OS_K0_TO_PHYSICAL(tris), numTris, textureEnabled);
            numBatches = objModel->numberOfBatches;
        }
    }
}

/**
 * Sets the race start HUD procedure to the first step.
 */
UNUSED void hud_reset_race_start(void) {
    gRaceStartShowHudStep = 0;
}

/**
 * Sets the visibility of the hud, hiding almost everything, leaving only the minimap.
 */
void hud_visibility(u8 setting) {
    gShowHUD = 1 - setting;
}
