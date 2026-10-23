#include "host.h"
#include "menu_internal.h"

MenuFrame gMenuFrame = {
    .guiColourR = 0xFF,
    .guiColourG = 0xFF,
    .guiColourB = 0xFF,
    .guiColourBlendFactor = 0,
    .guiOpacity = 0xFF,
};

MenuInput gMenuInput = {
    .delay = 0,
};

MenuFlow gMenuFlow = {
    .curIndex = 0,
    .subOption = 0,
    .optionCap = 0,
};

MenuLoaded gMenuLoaded = {
    .images = NULL,
    .text = NULL,
};

RaceSetup gRaceSetup = {
    .numberOfActivePlayers = 1,
    .numberOfReadyPlayers = 0,
    .multiplayerRacerCount = 0,
    .isInTracksMode = TRUE,
    .isInAdventureTwo = FALSE,
    .isInTwoPlayerAdventure = 0,
    .trackIdToLoad = 0,
    .trackSpecified = 0,
    .trophyRaceWorldId = 0,
    .trophyRaceRound = 0,
    .inAdvModeTrophyRace = FALSE,
    .saveFileIndex = 0,
    .activeMagicCodes = 0,
    .unlockedMagicCodes = 0,
    .hasSeenCautionMenu = FALSE,
    .titleScreenLoaded = FALSE,
    .viewingCreditsFromCheat = FALSE,
};

TrackSelectState gTrackSelect = {
    .woodFrameHeightScale = 1.0f,
    .row = 0,
    .previewTrackId = ASSET_LEVEL_CENTRALAREAHUB,
    .musicVolume = 0x7F,
    .nameVoiceDelay = 0,
};

ControllerPakState gControllerPak;

ResultsState gResults = {
    .filenameEntryPending = FALSE,
    .textOffset = 0,
    .timestampOffset = 0,
};

DialogueState gDialogue;

/**
 * @file Contains all the code used for every menu in the game.
 */

Triangle *sMenuCurrHudTris;
CharacterVoiceChannel gMenuSelectedCharacter;

UNUSED s32 sUnused_801263C8; // Set to 0 in menu_init, and never again.
CharacterSelectData (*gCurrCharacterSelectData)[10];

s8 gControllersXAxisDelay[4];
s8 gControllersYAxisDelay[4];
s8 gControllersXAxis[4];
s8 gControllersYAxis[4];
s32 sUnused_80126470; // Set to 0 in the magic codes menu and 0xD000 elsewhere, otherwise unused.

u8 gMenuAssetActive[128];
s32 buttonsPressed[5];
SoundHandle gMenuSoundMasks[MAXCONTROLLERS]; // Soundmask values
UNUSED s8 sUnused_80126828;                  // Set to 0 in menu_init, and never used again.
s32 *sMenuNullElement;                       // Never set, but it's read? Is it part of a larger struct being set?
//!@bug: menu_track_select_init() fills this with a `for (i = 0; i < 5; i++)`
// loop, so it writes five rows into a four-row array. Retail really is 48 bytes
// (gTrackSelect.trackIds = 0x801268E8, gTrackSelect.fflUnlocked = 0x80126918), so the console
// select opens, and evidently survives it. The PC build lays its globals out
// differently, so those 12 bytes land on something else entirely — which is
// real, drifting corruption. The fifth row is never read back (every read
// indexes by gTrackSelect.cursorY, which stops at 3), so giving the write a row
// of its own is behaviour-neutral and keeps the stray store in bounds.
char *gAudioOutputStrings[3];
char *gMusicTestString;
MenuPanelGeometry *gMenuGeometry;
u16 (*gCheatsAssetData)[30]; // Cheat table.
s16 gOptionsMenuItemIndex;
#if VERSION >= VERSION_79
// Created a replacement for this var in v2?
PostRaceLoadState gPostRaceOld;
#else
#endif
#if VERSION >= VERSION_79
// Created a replacement for this var in v2?
#endif

// END OF US/PAL BSS ORDER

/******************************/

/************ .data ************/

// A boolean, set to TRUE if player has entered trophy race through adventure mode.

// Height scale of the wooden frames in the track select menu.

s32 gResetTitleScale = TRUE;

// Currently selected index for all menus.

s32 unused_800DF464 = 4;
UNUSED s32 gMenuDisableObjAnim = FALSE; // Never set, but read once, always returning false.

// Used as a short delay before printing the missing controller text.
s32 gMissingControllerDelay = 0;

s32 gCurrentMenuId = 0;
s32 unused_800DF474 = 0;

// A boolean, set to TRUE when a track is loaded using gRaceSetup.trackIdToLoad.

UNUSED s32 unused_800DF48C = 0;
UNUSED s32 unused_800DF490 = 0;
s32 *gMenuTextLangTable = NULL;

// Flags for menu/HUD sprites. Used in menu_element_render()
// Seems like it doesn't matter what you set it as?
s32 gMenuSpriteFlags = 0;

s32 unused_800DF4D0 = 0;

s32 unused_800DF4F0[] = { 0x4000, 0x8000, 0x1000, 0x2000, 0x8000, 0x10, 0x400, 0x00 };

MenuAsset sMenuImageProperties[18] = {
    { { { { { 0, 0, 0 } } }, { 0x00 }, 1.0f, { { { { 0.0f, 0.0f, -32.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x01 }, 1.0f, { { { { 0.0f, 0.0f, -32.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x02 }, 1.0f, { { { { 0.0f, 0.0f, -32.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x03 }, 1.0f, { { { { 0.0f, 0.0f, -32.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x05 }, 0.215f, { { { { 0.0f, 0.0f, -500.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x06 }, 0.215f, { { { { 0.0f, 0.0f, -500.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x07 }, 0.215f, { { { { 0.0f, 0.0f, -500.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x04 }, 0.125f, { { { { 0.0f, -40.0f, -500.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
#if VERSION >= VERSION_79
    { { { { { 0, 0, 0 } } }, { 0x66 }, 0.75f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x67 }, 0.75f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
#else
    { { { { { 0, 0, 0 } } }, { 0x5B }, 0.75f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x5C }, 0.75f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
#endif
    { { { { { 0, 0, 0 } } }, { 0x42 }, 1.0f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x40 }, 1.0f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x41 }, 1.0f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x4C }, 1.0f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x4D }, 1.0f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x4E }, 1.0f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
    { { { { { 0, 0, 0 } } }, { 0x4F }, 1.0f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
#if VERSION >= VERSION_79
    { { { { { 0, 0, 0 } } }, { 0x68 }, 1.0f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
#else
    { { { { { 0, 0, 0 } } }, { 0x5D }, 1.0f, { { { { 0.0f, 0.0f, 0.0f } } } } }, 0, 0, 0, 0, 0, 0, 0 },
#endif
};

s16 *gAssetsMenuElementIds[1] = { NULL };
s16 gMenuElementIdCount = 0;
s16 gMenuObjectsCount = 0;

s32 unused_800DF768 = 1;

FadeTransition sMenuTransitionFadeIn =
    FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_NONE, FADE_COLOR_BLACK, 18, FADE_STAY);
FadeTransition sMenuTransitionFadeOut = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_OUT, FADE_COLOR_BLACK, 18, 0);
UNUSED FadeTransition sMenuTransitionFadeInWhite =
    FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_NONE, FADE_COLOR_WHITE, 18, FADE_STAY);
UNUSED FadeTransition sMenuTransitionFadeOutWhite =
    FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_OUT, FADE_COLOR_WHITE, 18, 0);

char *gTitleMenuStrings[3] = { 0, 0, 0 };

// Version text shown on the title screen? See 1:15 in https://www.youtube.com/watch?v=OHSCLcA74ao.
char gVersionDisplayText[20] = "VERSION XXXXXXXX";

// "Diddy Kong Racing" logo texture indices
s16 sGameTitleTileTextures[12] = { TEXTURE_TITLE_SEGMENT_01, TEXTURE_TITLE_SEGMENT_02, TEXTURE_TITLE_SEGMENT_03,
                                   TEXTURE_TITLE_SEGMENT_04, TEXTURE_TITLE_SEGMENT_05, TEXTURE_TITLE_SEGMENT_06,
                                   TEXTURE_TITLE_SEGMENT_07, TEXTURE_TITLE_SEGMENT_08, TEXTURE_TITLE_SEGMENT_09,
                                   TEXTURE_TITLE_SEGMENT_10, TEXTURE_TITLE_SEGMENT_11, -1 };

// Final value is all NULL so that the texrect_draw function knows when to stop.
DrawTexture sGameTitleTileOffsets[12] = { { NULL, -75, -32 }, { NULL, -60, -32 }, { NULL, -45, -32 },
                                          { NULL, -30, -32 }, { NULL, -15, -32 }, { NULL, 0, -32 },
                                          { NULL, 15, -32 },  { NULL, 30, -32 },  { NULL, 45, -32 },
                                          { NULL, 60, -32 },  { NULL, 75, -32 },  { NULL, 0, 0 } };

UNUSED u8 unused_800DFA0C[] = { 0, 0, 15, 120 };

char *gOptionMenuStrings[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

unk800DFA3C gAudioMenuStrings[] = {
    { 160, 80, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, NULL },
    { 160, 104, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, NULL },
    { 160, 144, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, NULL },
    { 160, 192, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, NULL },
    { 161, 35, 0, 0, 0, 255, 128, ASSET_FONTS_BIGFONT, 12, NULL },
    { 160, 32, 255, 128, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, NULL },
    { 160, 188, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, NULL },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

// Strings related to the controller pak.
char *gContPakNotPresentStrings[6] = { 0, 0, 0, 0, 0, 0 };
char *gContPakCorruptDataRepairStrings[6] = { 0, 0, 0, 0, 0, 0 };
char *gContPakDamagedStrings[6] = { 0, 0, 0, 0, 0, 0 };
char *gContPakFullStrings[5] = { 0, 0, 0, 0, 0 };
char *gContPakDiffContStrings[7] = { 0, 0, 0, 0, 0, 0, 0 };
char *gContPakNoRoomForGhostsStrings[6] = { 0, 0, 0, 0, 0, 0 };
char *gContPakCorruptDataStrings[6] = { 0, 0, 0, 0, 0, 0 };
char *gContPakRumbleDetectedStrings[6] = { 0, 0, 0, 0, 0, 0 };
char *gContPakSwitchToRumbleStrings[6] = { 0, 0, 0, 0, 0, 0 };
char *gContPakNeed2ndAdvStrings[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

UNUSED DrawTexture unused_800DFC08[] = { { NULL, 0, -1 } };

UNUSED s8 unused_800DFC70[] = { 0x40, 0x40, 0x04, 0x04, -1 };

MenuElement gControllerPakMenuElement[7] = {
    { 161, 32, 161, 33, 161, 32, 0, 0, 0, 255, 128, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 160, 30, 160, 30, 160, 30, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 160, 112, 160, 112, 160, 112, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 160, 128, 160, 128, 160, 128, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 160, 192, 160, 192, 160, 192, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 160, 208, 160, 208, 160, 208, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ASSET_FONTS_FUNFONT, 0, 0, { NULL }, { { 0, 0, 0, 0 } } },
};

char *gMagicCodeMenuStrings[5] = { 0, 0, 0, 0, 0 };

u8 gCharacterVolumes[10][2] = {
    { 0x0F, 0x64 }, { 0x0C, 0x07 }, { 0x09, 0x64 }, { 0x0A, 0x64 }, { 0x08, 0x64 },
    { 0x0B, 0x64 }, { 0x0D, 0x64 }, { 0x0E, 0x64 }, { 0x05, 0x64 }, { 0x04, 0x64 },
};

// Not sure what this is
// Used for gCurrCharacterSelectData when Neither T.T. Nor Drumstick are unlocked
// CharacterSelectData

#define NONE 0xFF
#define KRUNCH 0
#define DIDDY 1
#define BUMPER 2
#define BANJO 3
#define CONKER 4
#define TIPTUP 5
#define PIPSY 6
#define TIMBER 7
#define DRUMSTICK 8
#define TICTOC_8 8 // T.T's unlocked but Drumstick is not
#define TICTOC_9 9 // Everyone unlocked.

/**
 * Diddy Kong Racing's character select menu refers to these arrays below to know how
 * to handle some certain things. This includes which direction selects which index, which voice ID
 * they use, and which audio voices the BGM uses. It has a different table for each possible combination
 * of which characters are available to select. This system is far from conventional.
 * Unfortunately, because each entry is explicitly 14 bytes, it means a struct would not match, because
 * a struct would align itself to 16 bytes, creating an extra 2 bytes of padding inbetween.
 *
 * The layout shows which character is in each cardinal direction. Presumably so it knows
 * where to place extra cursors when a new player joins.
 * Frankly, this is a comically overcomplicated method.
 */

UNUSED s32 unused_800DFFCC = 0;

// clang-format off

MenuElement gCautionMenuTextElements[14] = {
    { SCREEN_WIDTH_HALF + 1, 35, SCREEN_WIDTH_HALF + 1, 35, SCREEN_WIDTH_HALF + 1, 35, 0, 0, 0, 255, 128, ASSET_FONTS_BIGFONT, 12,0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 32, SCREEN_WIDTH_HALF, 32, SCREEN_WIDTH_HALF, 32, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 72, SCREEN_WIDTH_HALF, 68, SCREEN_WIDTH_HALF, 72, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 86, SCREEN_WIDTH_HALF, 82, SCREEN_WIDTH_HALF, 86, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 100, SCREEN_WIDTH_HALF, 96, SCREEN_WIDTH_HALF, 100, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 114, SCREEN_WIDTH_HALF, 110, SCREEN_WIDTH_HALF, 114, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 128, SCREEN_WIDTH_HALF, 124, SCREEN_WIDTH_HALF, 128, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 142, SCREEN_WIDTH_HALF, 138, SCREEN_WIDTH_HALF, 142, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 156, SCREEN_WIDTH_HALF, 152, SCREEN_WIDTH_HALF, 156, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 156, SCREEN_WIDTH_HALF, 166, SCREEN_WIDTH_HALF, 156, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 186, SCREEN_WIDTH_HALF, 180, SCREEN_WIDTH_HALF, 186, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 200, SCREEN_WIDTH_HALF, 194, SCREEN_WIDTH_HALF, 200, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 200, SCREEN_WIDTH_HALF, 208, SCREEN_WIDTH_HALF, 200, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    {NULL},
};

MenuElement gGameSelectTextElemsNoAdv2[7] = {
    { SCREEN_WIDTH_HALF + 1, 35, SCREEN_WIDTH_HALF + 1, 35, SCREEN_WIDTH_HALF + 1, 35, 0, 0, 0, 255, 128, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 32, SCREEN_WIDTH_HALF, 32, SCREEN_WIDTH_HALF, 32, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { -96, 39, -96, 39, -96, 39, 176, 224, 192, 255, 255, ASSET_FONTS_FUNFONT, 0, 7, { NULL }, { { 192, 44, 4, 4 } } },
    { SCREEN_WIDTH_HALF, 104, SCREEN_WIDTH_HALF, 104, SCREEN_WIDTH_HALF, 104, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { -96, -9, -96, -9, -96, -9, 176, 224, 192, 255, 255, ASSET_FONTS_FUNFONT, 0, 7, { NULL }, { { 192, 44, 4, 4 } } },
    { SCREEN_WIDTH_HALF, 152, SCREEN_WIDTH_HALF, 152, SCREEN_WIDTH_HALF, 152, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0,  { NULL }, { { 0, 0, 0, 0 } } },
    {NULL},
};

MenuElement gGameSelectTextElemsWithAdv2[9] = {
    { SCREEN_WIDTH_HALF + 1, 35, SCREEN_WIDTH_HALF + 1, 35, SCREEN_WIDTH_HALF + 1, 35, 0, 0, 0, 255, 128, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { SCREEN_WIDTH_HALF, 32, SCREEN_WIDTH_HALF, 32, SCREEN_WIDTH_HALF, 32, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { -112, 55, -112, 55, -112, 55, 176, 224, 192, 255, 255, ASSET_FONTS_FUNFONT, 0, 7, { NULL }, { { 224, 44, 4, 4 } } },
    { SCREEN_WIDTH_HALF, 88, SCREEN_WIDTH_HALF, 88, SCREEN_WIDTH_HALF, 88, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { -112, 7, -112, 7, -112, 7, 176, 224, 192, 255, 255, ASSET_FONTS_FUNFONT, 0, 7, { NULL }, { { 224, 44, 4, 4 } } },
    { SCREEN_WIDTH_HALF, 136, SCREEN_WIDTH_HALF, 136, SCREEN_WIDTH_HALF, 136, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { -112, -41, -112, -41, -112, -41, 176, 224, 192, 255, 255, ASSET_FONTS_FUNFONT, 0, 7, { NULL }, { { 224, 44, 4, 4 } } },
    { SCREEN_WIDTH_HALF, 184, SCREEN_WIDTH_HALF, 184, SCREEN_WIDTH_HALF, 184, 255, 255, 255, 0, 255, ASSET_FONTS_BIGFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    {NULL},
};

char *gFilenames[3] = { NULL, NULL, NULL };

u16 unused_800E03BC[8] = {
    0x004C, 0x0070, 0x00F4, 0x0070, 0x004C, 0x00D0, 0x00F4, 0x00D0,
};

// Either 0 (2 racers), 1 (4 racers), or 2 (6 racers)

DrawTexture gMenuSelectionArrowUp[2] = { { NULL, -12, -8 }, { NULL, 0, 0 } };
DrawTexture gMenuSelectionArrowLeft[2] = { { NULL, -8, -12 }, { NULL, 0, 0 } };
DrawTexture gMenuSelectionArrowDown[2] = { { NULL, -12, -8 }, { NULL, 0, 0 } };
DrawTexture gMenuSelectionArrowRight[2] = { { NULL, -8, -12 }, { NULL, 0, 0 } };

DrawTexture gRaceSelectionCarTex[3] = { { NULL, 0, 0 }, { NULL, 0, 32 }, { NULL, 0, 0 } };
DrawTexture gRaceSelectionHoverTex[3] = { { NULL, 0, 0 }, { NULL, 0, 32 }, { NULL, 0, 0 } };
DrawTexture gRaceSelectionPlaneTex[3] = { { NULL, 0, 0 }, { NULL, 0, 32 }, { NULL, 0, 0 } };

DrawTexture *gMenuSelectionArrows[4] = { gMenuSelectionArrowUp, gMenuSelectionArrowLeft, gMenuSelectionArrowDown,
                                         gMenuSelectionArrowRight };

// Paired X / Y Offsets. X Is First, Y is Second. For NTSC.
s16 gTracksMenuArrowPositionsNTSC[8] = {
    0x0000, 0xFFC2, 0x0055, 0x0000, 0x0000, 0x003E, 0xFFAB, 0x0000,
};

// Paired X / Y Offsets. X Is First, Y is Second. For PAL.
s16 gTracksMenuArrowPositionsPAL[8] = {
    0x0000, 0xFFB6, 0x0055, 0x0000, 0x0000, 0x004A, 0xFFAB, 0x0000,
};

// Often access like D_800E0710[i * 3 + 1].
// Texture indices for the backgrounds in the Track Select
// Seems like the 3rd number of a group of 3 is not used for anything?
s16 gTracksMenuBgTextureIndices[15] = {
    0x0E, 0x0F, 0x00, // Dino domain
    0x10, 0x11, 0x20, // Sherbet Island
    0x12, 0x13, 0x00, // Snowflake Mountain
    0x14, 0x15, 0x20, // Dragon Forest
    0x16, 0x17, 0x20  // Future Fun Land
};

// ID for T.Ts voice lines for each track id.
// T.T. will say nothing for tracks with -1
s16 gTTVoiceLines[53] = { -1,
                          -1,
                          SOUND_VOICE_TT_DRAGON_FOREST,
                          SOUND_VOICE_TT_FOSSIL_CANYON,
                          SOUND_VOICE_TT_PIRATE_LAGOON,
                          SOUND_VOICE_TT_ANCIENT_LAKE,
                          SOUND_VOICE_TT_WALRUS_COVE,
                          SOUND_VOICE_TT_HOT_TOP_VOLCANO,
                          SOUND_VOICE_TT_WHALE_BAY,
                          SOUND_VOICE_TT_SNOWBALL_VALLEY,
                          SOUND_VOICE_TT_CRESCENT_ISLAND,
                          SOUND_VOICE_TT_FIRE_MOUNTAIN,
                          SOUND_VOICE_TT_DINO_DOMAIN,
                          SOUND_VOICE_TT_EVERFROST_PEAK,
                          SOUND_VOICE_TT_SHERBET_ISLAND,
                          SOUND_VOICE_TT_SPACEPORT_ALPHA,
                          -1,
                          SOUND_VOICE_TT_SPACEDUST_VALLEY,
                          SOUND_VOICE_TT_GREENWOOD_VILLAGE,
                          SOUND_VOICE_TT_BOULDER_CANYON,
                          SOUND_VOICE_TT_WINDMILL_PLAINS,
                          -1,
                          -1,
                          -1,
                          SOUND_VOICE_TT_SNOWFLAKE_MOUNTAIN,
                          SOUND_VOICE_TT_SMOKEY_CASTLE,
                          SOUND_VOICE_TT_DARKWATER_BEACH,
                          SOUND_VOICE_TT_ICICLE_PYRAMID,
                          SOUND_VOICE_TT_FROSTY_VILLAGE,
                          SOUND_VOICE_TT_JUNGLE_FALLS,
                          SOUND_VOICE_TT_TREASURE_CAVES,
                          SOUND_VOICE_TT_HAUNTED_WOODS,
                          SOUND_VOICE_TT_DARKMOON_CAVERNS,
                          SOUND_VOICE_TT_STAR_CITY,
                          SOUND_VOICE_TT_TROPHY_RACE,
                          SOUND_VOICE_TT_FUTURE_FUN_LAND,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1,
                          -1 };

// BAD CONTROLLER PAK / If you wish to change / Controller Pak or Rumble Pak, / please do so now.
char *sBadControllerPakMenuText[5] = { 0, 0, 0, 0, 0 };

// CONTROLLER PAK FULL / If you wish to change / Controller Pak or Rumble Pak, / please do so now.
char *sControllerPakFullMenuText[5] = { 0, 0, 0, 0, 0 };

// NO CONTROLLER PAK / If you wish to change / Controller Pak or Rumble Pak, / please do so now.
char *sNoControllerPakMenuText[5] = { 0, 0, 0, 0, 0 };

// CORRUPT DATA / If you wish to change / Controller Pak or Rumble Pak, / please do so now.
char *sCorruptDataMenuText[6] = { 0, 0, 0, 0, 0, 0 };

// If you wish to use / the Controller Pak / insert it now!
char *sInsertControllerPakMenuText[3] = { 0, 0, 0 };

// If you wish to use / the Rumble Pak / insert it now!
char *sInsertRumblePakMenuText[4] = { 0, 0, 0, 0 };

DrawTexture gMenuPortraitKrunch[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
DrawTexture gMenuPortraitDiddy[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
DrawTexture gMenuPortraitDrumstick[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
DrawTexture gMenuPortraitBumper[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
DrawTexture gMenuPortraitBanjo[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
DrawTexture gMenuPortraitConker[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
DrawTexture gMenuPortraitTiptup[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
DrawTexture gMenuPortraitTT[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
DrawTexture gMenuPortraitPipsy[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };
DrawTexture gMenuPortraitTimber[2] = { { NULL, 0, 0 }, { NULL, 0, 0 } };

DrawTexture *gRacerPortraits[10] = { gMenuPortraitKrunch, gMenuPortraitDiddy, gMenuPortraitDrumstick,
                                     gMenuPortraitBumper, gMenuPortraitBanjo, gMenuPortraitConker,
                                     gMenuPortraitTiptup, gMenuPortraitTT,    gMenuPortraitPipsy,
                                     gMenuPortraitTimber };

s16 unused_800E0B18[74] = { 0x0140, 0x017C, 0x01B8, 0x01F4, 0x0230, 0x026C, 0x02A8, 0x02E4, 0x01E0, 0x0000, 0x0018,
                            0x0039, 0x005A, 0x007B, 0x009C, 0x00BD, 0x00DE, 0x00FF, 0x00A0, 0x0000, 0x0140, 0x01B8,
                            0x0230, 0x02A8, 0x01E0, 0x0000, 0x0018, 0x0065, 0x00B3, 0x0100, 0x00A0, 0x0000, 0x0140,
                            0x01F4, 0x02A8, 0x01E0, 0x0028, 0x008C, 0x00F0, 0x00A0, 0x01E0, 0x00C0, 0x00A0, 0x00C0,
                            0xFF60, 0x00C0, 0xFF80, 0xFFEC, 0xFFE4, 0xFFE6, 0x0050, 0xFFF8, 0xFFE4, 0xFFF4, 0xFFE4,
                            0x0006, 0xFFE4, 0x0018, 0x0050, 0x0008, 0x0000, 0x0078, 0x0000, 0x003C, 0x0000, 0x001E,
                            0x0000, 0x0384, 0x0000, 0x001E, 0x0000, 0x001E, 0x0000, 0x0384 };

char gFirstPlace[4] = "1ST";
char gSecondPlace[4] = "2ND";
char gThirdPlace[4] = "3RD";
char gFourthPlace[4] = "4TH";
char gFifthPlace[4] = "5TH";
char gSixthPlace[4] = "6TH";
char gSeventhPlace[4] = "7TH";
char gEighthPlace[4] = "8TH";

MenuElement gRaceResultsMenuElements[8] = {
    { 352, 172, 32, 172, -288, 172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 452, 166, 132, 166, -188, 166, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 560, 184, 240, 184, -80, 184, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 452, 180, 132, 180, -188, 180, 255, 192, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 1, { NULL }, { { 0, 0, 0, 0 } } },
    { 452, 198, 132, 198, -188, 198, 255, 192, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 1, { NULL }, { { 0, 0, 0, 0 } } },
    { 452, 216, 132, 216, -188, 216, 255, 192, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 1, { NULL }, { { 0, 0, 0, 0 } } },
    { 560, 200, 240, 200, -80, 200, 128, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 1, { NULL }, { { 0, 0, 0, 0 } } },
    {NULL}
};

MenuElement gRaceOrderMenuElements[11] = {
    { 575, 172, 255, 172, -65, 172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 542, 172, 222, 172, -98, 172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 509, 172, 189, 172, -131, 172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 476, 172, 156, 172, -164, 172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 443, 172, 123, 172, -197, 172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 410, 172, 90, 172, -230, 172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 377, 172, 57, 172, -263, 172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 344, 172, 24, 172, -296, 166, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 3, { &gMenuPortraitKrunch }, { { 0, 0, 0, 0 } } },
    { 481, 166, 161, 166, -159, 166, 0, 0, 0, 255, 128, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 479, 164, 159, 164, -161, 164, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    {NULL}

};

MenuElement gRecordTimesMenuElements[9] = {
    { 481, 174, 161, 174, -159, 174, 0, 0, 0, 255, 128, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 479, 172, 159, 172, -161, 172, 255, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 12, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 368, 192, 48, 192, -272, 192, 255, 64, 64, 96, 255, ASSET_FONTS_FUNFONT, 8, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 506, 187, 186, 187, -134, 187, 255, 192, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 1, { NULL }, { { 0, 0, 0, 0 } } },
    { 578, 192, 258, 192, -62, 192, 255, 128, 255, 96, 255, ASSET_FONTS_FUNFONT, 12, 0, { &gResults.courseInitials }, { { 0, 0, 0, 0 } } },
    { 368, 212, 48, 212, -272, 212, 255, 64, 64, 96, 255, ASSET_FONTS_FUNFONT, 8, 0, { NULL }, { { 0, 0, 0, 0 } } },
    { 506, 207, 186, 207, -134, 207, 128, 255, 255, 0, 255, ASSET_FONTS_FUNFONT, 0, 1, { NULL }, { { 0, 0, 0, 0 } } },
    { 578, 212, 258, 212, -62, 212, 255, 128, 255, 96, 255, ASSET_FONTS_FUNFONT, 12, 0, { &gResults.flapInitials }, { { 0, 0, 0, 0 } } }, {NULL}
};

u32 unused_800E0FF4[4] = { 0x01FFFFFF, 0x0012FFFF, 0x81FFFFFF, 0x00120000 };

// clang-format on

// clang-format off

// clang-format off

// clang-format on

Gfx dMenuHudSettings[] = {
    gsDPPipeSync(),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsSPClearGeometryMode(G_ZBUFFER | G_FOG),
    gsSPEndDisplayList(),
};

Gfx dMenuHudDrawModes[][2] = {
    {
        gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
        gsDPSetOtherMode(DKR_OMH_1CYC_POINT_NOPERSP, DKR_OML_COMMON | G_RM_XLU_SURF | G_RM_XLU_SURF2),
    },
    {
        gsDPSetCombineMode(G_CC_MODULATEIA, G_CC_MODULATEIA),
        gsDPSetOtherMode(DKR_OMH_1CYC_BILERP, DKR_OML_COMMON | G_RM_XLU_SURF | G_RM_XLU_SURF2),
    },
    {
        gsDPSetCombineMode(G_CC_PRIMITIVE, G_CC_PRIMITIVE),
        gsDPSetOtherMode(DKR_OMH_1CYC_POINT_NOPERSP, DKR_OML_COMMON | G_RM_XLU_SURF | G_RM_XLU_SURF2),
    }
};

// Triangle indices for the wood panels used in multiple menus.
u8 gWoodPanelsIndices[32] = { 0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,
                              10, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 0,  0 };

// UV coordinate indices the wood panels.
u8 gWoodPanelTexCoords[5][12] = { { 0, 0, 3, 0, 2, 1, 0, 0, 2, 1, 1, 1 },
                                  { 2, 1, 3, 0, 3, 3, 2, 1, 3, 3, 2, 2 },
                                  { 1, 2, 2, 2, 3, 3, 1, 2, 3, 3, 0, 3 },
                                  { 0, 0, 1, 1, 1, 2, 0, 0, 1, 2, 0, 3 },
                                  { 1, 1, 2, 1, 2, 2, 1, 1, 2, 2, 1, 2 } };

// Position offsets for the wood panels. It has a X Width, X BorderWidth, Y Width, Y BorderWidth pattern.
s8 gWoodPanelVertCoords[][4] = {
    { 0, 0, 0, 0 },  { 1, 0, 0, 0 },  { 1, -1, 0, -1 }, { 0, 1, 0, -1 },  { 1, -1, 0, -1 },
    { 1, 0, 0, 0 },  { 1, 0, -1, 0 }, { 1, -1, -1, 1 }, { 0, 1, -1, 1 },  { 1, -1, -1, 1 },
    { 1, 0, -1, 0 }, { 0, 0, -1, 0 }, { 0, 0, 0, 0 },   { 0, 1, 0, -1 },  { 0, 1, -1, 1 },
    { 0, 0, -1, 0 }, { 0, 1, 0, -1 }, { 1, -1, 0, -1 }, { 1, -1, -1, 1 }, { 0, 1, -1, 1 }
};

// Colour filter, used for the shadows on the side of the panels.
s16 gWoodPanelVertColours[5][4] = {
    { 216, 216, 216, 256 }, // Colour for top part of the panel
    { 176, 176, 176, 256 }, // Colour for right part of the panel
    { 96, 96, 96, 256 },    // Colour for bottom part of the panel
    { 136, 136, 136, 256 }, // Colour for left part of the panel
    { 256, 256, 256, 256 }  // Colour for the center of the panel
};

Vertex *gWoodPanelVertices[2] = { NULL, NULL };

Triangle *gWoodPanelTriangles[2] = { NULL, NULL };

s32 gMenuTrisFlip = 0;
s32 gWoodPanelCount = 0;
s32 gWoodPanelAllocCount = 0;

// U tex scale for wood panels (32 is 1.0x)
s32 gWoodPanelTexScaleU = 32;
// V tex scale for wood panels (32 is 1.0x)
s32 gWoodPanelTexScaleV = 32;

// Related to the file select wood panel highlight.
s16 sPanelCornerSigns[16] = { 1, 1, -1, 1, -1, 1, -1, -1, 1, -1, -1, -1, 1, 1, 1, -1 };

/*******************************/

void menu_panel_draw_textured(Gfx **dList, s32 startX, s32 startY, s32 width, s32 height, s32 borderWidth, s32 borderHeight,
                   s32 colour, TextureHeader *tex) {
    s32 uVals[4];
    s32 vVals[4];
    Vertex *vertices;
    Triangle *triangles;
    s32 i;
    s32 j;
    s32 r, g, b, a;
    s32 r0, g0, b0, a0;
    s8(*texCoords)[4];
    s16(*texColors)[4];
    u8(*woodPanelTexCoords)[12];
    UNUSED s32 pad;

    gMenuGeometry[gWoodPanelCount].texture[gMenuTrisFlip] = tex;
    if (tex != NULL) {
        woodPanelTexCoords = gWoodPanelTexCoords;
        uVals[0] = 0;
        uVals[1] = gWoodPanelTexScaleU * borderWidth;
        uVals[2] = (width - borderWidth) * gWoodPanelTexScaleU;
        uVals[3] = gWoodPanelTexScaleU * width;
        vVals[0] = 0;
        j = gWoodPanelTexScaleV; // fake
        vVals[1] = j * borderHeight;
        vVals[2] = (height - borderHeight) * gWoodPanelTexScaleV;
        vVals[3] = gWoodPanelTexScaleV * height;
        triangles = gMenuGeometry[gWoodPanelCount].triangles[gMenuTrisFlip];
        for (i = 0; i < 10; i += 2) {
            if (1) {}
            if (1) {}
            if (1) {} // fake
            triangles[0].uv0.u = uVals[(*woodPanelTexCoords)[0]];
            triangles[0].uv0.v = vVals[(*woodPanelTexCoords)[1]];
            triangles[0].uv1.u = uVals[(*woodPanelTexCoords)[2]];
            triangles[0].uv1.v = vVals[(*woodPanelTexCoords)[3]];
            triangles[0].uv2.u = uVals[(*woodPanelTexCoords)[4]];
            triangles[0].uv2.v = vVals[(*woodPanelTexCoords)[5]];
            triangles[1].uv0.u = uVals[(*woodPanelTexCoords)[6]];
            triangles[1].uv0.v = vVals[(*woodPanelTexCoords)[7]];
            triangles[1].uv1.u = uVals[(*woodPanelTexCoords)[8]];
            triangles[1].uv1.v = vVals[(*woodPanelTexCoords)[9]];
            triangles[1].uv2.u = uVals[(*woodPanelTexCoords)[10]];
            triangles[1].uv2.v = vVals[(*woodPanelTexCoords)[11]];
            woodPanelTexCoords += 1;
            triangles += 2;
        }
    }
    r0 = ((colour >> 24) & 0xFF);
    g0 = ((colour >> 16) & 0xFF);
    b0 = ((colour >> 8) & 0xFF);
    a0 = (colour & 0xFF);
    vertices = gMenuGeometry[gWoodPanelCount].vertices[gMenuTrisFlip];
    for (texColors = gWoodPanelVertColours, texCoords = gWoodPanelVertCoords, i = 0; i < 5; i++) {
        r = ((*texColors)[0] * r0) >> 8;
        g = ((*texColors)[1] * g0) >> 8;
        b = ((*texColors)[2] * b0) >> 8;
        a = ((*texColors)[3] * a0) >> 8;
        texColors += 1;
        for (j = 0; j < 4; j++) {
            vertices->x = startX;
            vertices->x += (*texCoords)[0] * width;
            vertices->x += (*texCoords)[1] * borderWidth;
            vertices->y = startY;
            vertices->y += (*texCoords)[2] * height;
            vertices->y += (*texCoords)[3] * borderHeight;
            vertices->z = 0;
            vertices->r = r;
            vertices->g = g;
            vertices->b = b;
            vertices->a = a;
            texCoords += 1;
            vertices++;
        }
    }
    if (dList != NULL) {
        gMenuGeometry[gWoodPanelCount].drawnImmediately[gMenuTrisFlip] = 1;
        gSPDisplayList((*dList)++, &dMenuHudSettings);
        if (tex != NULL) {
            gDkrDmaDisplayList((*dList)++, OS_K0_TO_PHYSICAL(&dMenuHudDrawModes[1]), 2);
            gDkrDmaDisplayList((*dList)++, OS_K0_TO_PHYSICAL(tex->cmd), tex->numberOfCommands);
            i = TRUE; // texEnabled
        } else {
            gDkrDmaDisplayList((*dList)++, OS_K0_TO_PHYSICAL(&dMenuHudDrawModes[0]), 2);
            i = FALSE; // texEnabled
        }
        gDPPipeSync((*dList)++);
        gSPVertexDKR((*dList)++, OS_K0_TO_PHYSICAL(gMenuGeometry[gWoodPanelCount].vertices[gMenuTrisFlip]), 20, 0);
        gSPPolygon((*dList)++, OS_K0_TO_PHYSICAL(gMenuGeometry[gWoodPanelCount].triangles[gMenuTrisFlip]), 10, i);
        rendermode_reset(dList);
    } else {
        gMenuGeometry[gWoodPanelCount].drawnImmediately[gMenuTrisFlip] = 0;
    }
    gWoodPanelCount++;
}

void menu_panel_draw_bordered(Gfx **dList, s32 startX, s32 startY, s32 width, s32 height, s32 borderWidth, s32 borderHeight,
                   s32 colour0, s32 colour1, s32 colour2, s32 colour3) {
    s32 temp_ra;
    s32 temp_t3;
    s32 temp_t4;
    s32 temp_t5;
    s32 recUly;
    s32 recUlx;
    s32 uly;
    s32 recLrx;
    s32 borderLineCount;
    s32 i;
    s32 recLry;
    s32 ulx;
    s32 y0Orig;
    s32 primColour;
    s32 j;
    s32 index;

    gSPDisplayList((*dList)++, &dMenuHudSettings);
    gDkrDmaDisplayList((*dList)++, OS_K0_TO_PHYSICAL(&dMenuHudDrawModes[2]), 2);

    // Must be a while loop to match.
    i = 0;
    while (i < 4) {
        index = i << 2;
        temp_t3 = sPanelCornerSigns[index + 0];
        temp_t4 = sPanelCornerSigns[index + 1];
        temp_t5 = sPanelCornerSigns[index + 2];
        temp_ra = sPanelCornerSigns[index + 3];
        ulx = startX;
        y0Orig = startY;
        uly = y0Orig;

        switch (i) {
            case 0:
                primColour = colour0;
                recLrx = startX + width;
                recLry = y0Orig + 1;
                borderLineCount = borderHeight;
                break;
            case 1:
                ulx = (startX + (u32) width) - 1;
                primColour = colour1;
                uly = y0Orig + 1;
                recLrx = startX + width;
                recLry = (y0Orig + height) - 1;
                borderLineCount = borderWidth;
                break;
            case 2:
                primColour = colour2;
                uly = (y0Orig + height) - 1;
                recLrx = startX + width;
                recLry = y0Orig + height;
                borderLineCount = borderHeight;
                break;
            default:
                primColour = colour3;
                uly = y0Orig + 1;
                recLrx = startX + 1;
                recLry = (y0Orig + height) - 1;
                borderLineCount = borderWidth;
                break;
        }

        gDPSetPrimColor((*dList)++, 0, 0, primColour >> 24, primColour >> 16, primColour >> 8, primColour);

        for (j = 0; j < borderLineCount; j++) {
            if (recLrx >= 0 && recLry >= 0) {
                if (ulx < 0) {
                    recUlx = 0;
                } else {
                    recUlx = ulx;
                }
                if (uly < 0) {
                    recUly = 0;
                } else {
                    recUly = uly;
                }
                gDPFillRectangle((*dList)++, recUlx, recUly, recLrx, recLry);
            }
            ulx += temp_t3;
            uly += temp_t4;
            recLrx += temp_t5;
            recLry += temp_ra;
        }
        i++;
    }

    gDPPipeSync((*dList)++);
    gDPSetPrimColor((*dList)++, 0, 0, 255, 255, 255, 255);
    rendermode_reset(dList);
}

/**
 * Draw text with a drop shadow on a dialogue box.
 * Note: dialogueBoxID is always -1, so the global dialogue box is always used.
 */

/**
 * Initialise the title screen menu.
 * Load the title textures and reset player allocation.
 * Set the current level and demo.
 */

/**
 * Searches the controller pak to see if it has enough space to support the destination file.
 */
SIDeviceStatus savemenu_check_space(s32 controllerIndex, s32 *pakState, SaveFileData *file, s32 *index, s32 fileSize,
                                    UNUSED s32 arg5) {
    SIDeviceStatus ret = CONTROLLER_PAK_GOOD;

    if (*pakState) {
        ret = get_free_space(controllerIndex, &file[*index].fileSize, &gControllerPak.notesFree[controllerIndex]);
        if (ret == CONTROLLER_PAK_GOOD) {
            if ((s32) file[*index].fileSize >= fileSize && gControllerPak.notesFree[controllerIndex] > 0) {
                file[*index].saveFileType = SAVE_FILE_TYPE_CPAK_EMPTY;
                file[*index].controllerIndex = controllerIndex;
                (*index)++;
            }
        } else {
            SIDeviceStatus status = ret & 0xFF; // The upper bytes could be controllerIndex, so focus on the status
            if (*pakState < 0 && status == CONTROLLER_PAK_RUMBLE_PAK_FOUND) {
                *pakState = 0;
                ret = CONTROLLER_PAK_GOOD;
            } else if (status != CONTROLLER_PAK_WITH_BAD_ID && status != CONTROLLER_PAK_INCONSISTENT &&
                       status != CONTROLLER_PAK_BAD_DATA) {
                ret = CONTROLLER_PAK_GOOD;
            }
        }
    }

    return ret;
}

// These vars have to be defined after cheatmenu_checksum since that function has a static var in it.

/**
 * Render the text and frame around the viewport onscreen.
 * This includes the world name, the frame around the viewport,
 * and the image that goes over the viewport when the level is not loaded.
 */
void trackmenu_render_2D(s32 x, s32 y, char *hubName, char *trackName, s32 rectOpacity, s32 imageId, s32 copyViewPort,
                         DrawTexture *arg7, s32 arg8) {
    s32 xTemp;
    s32 yTemp;
    s32 opacity;
    s32 i;
    s32 sp6C;
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;
    f32 sp58;
    f32 sp54;
    s16 *offsets;
    s32 temp;

    sp6C = 0;
    xTemp = x + 160;
    yTemp = gTrackSelect.viewportHalfY - y;
    if (osTvType == OS_TV_TYPE_PAL) {
        sp6C = 12;
    }
    set_text_font(ASSET_FONTS_BIGFONT);
    set_text_background_colour(0, 0, 0, 0);
    if (gMenuInput.delay > 0) {
        opacity = 255 - (gMenuInput.delay * 16);
        if (opacity < 0) {
            opacity = 0;
        }
    } else {
        opacity = 255;
    }
    if (hubName != gTrackSelect.hubName) {
        if (hubName == level_name(level_world_id(WORLD_SNOWFLAKE_MOUNTAIN))) {
            set_kerning(TRUE);
        }
        set_text_colour(0, 0, 0, 255, opacity / 2);
        draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF + 1, (yTemp - sp6C) - 85, hubName, ALIGN_MIDDLE_CENTER);
        set_text_colour(255, 255, 255, 0, opacity);
        draw_text(&gMenuFrame.displayList, SCREEN_WIDTH_HALF, (yTemp - sp6C) - 88, hubName, ALIGN_MIDDLE_CENTER);
        gTrackSelect.hubName = hubName;
        set_kerning(FALSE);
    }
    set_text_colour(255, 255, 255, 0, opacity);
    draw_text(&gMenuFrame.displayList, xTemp, sp6C + yTemp + 88, trackName, ALIGN_MIDDLE_CENTER);
    if (rectOpacity > 0) {
        if (((yTemp - (gTrackSelect.viewportY >> 2)) < gTrackSelect.viewportY) &&
            (((gTrackSelect.viewportY >> 2) + yTemp) > 0)) {
            sp58 = 1.25f;
            sp54 = 1.25f;
            if (osTvType == OS_TV_TYPE_PAL) {
                sp54 *= 1.1;
            }
            temp = xTemp - 80;
            if (copyViewPort) {
                copy_viewport_frame_size_to_coords(0, &x1, &y1, &x2, &y2);
                temp = x1;
                sp58 = (x2 - temp) / 128.0f;
                sp54 = (y2 - y1) / 96.0f;
            } else {
                x2 = xTemp + 80;
            }
            x1 = temp;
            if (temp < SCREEN_WIDTH && xTemp > 0) {
                texrect_draw_scaled(&gMenuFrame.displayList, arg7, xTemp, yTemp, sp58, sp54,
                                    (rectOpacity & 0xFF) | ~0xFF, TEXRECT_FLIP_X);
            }
            if (xTemp < SCREEN_WIDTH && x2 > 0) {
                texrect_draw_scaled(&gMenuFrame.displayList, arg7, xTemp, yTemp, sp58, sp54,
                                    (rectOpacity & 0xFF) | ~0xFF, 0);
            }
            rendermode_reset(&gMenuFrame.displayList);
        }
    }
    gMenuLoaded.images[imageId].trans.x_position = x;
    gMenuLoaded.images[imageId].trans.y_position = y;
    if (osTvType == OS_TV_TYPE_PAL) {
        gTrackSelect.woodFrameHeightScale = 1.2f;
        offsets = gTracksMenuArrowPositionsPAL;
    } else {
        offsets = gTracksMenuArrowPositionsNTSC;
    }
    menu_element_render(imageId);
    gTrackSelect.woodFrameHeightScale = 1.0f;
    for (i = 0; i < 4; i++) {
        if ((1 << i) & arg8) {
            texrect_draw(&gMenuFrame.displayList, gMenuSelectionArrows[i], offsets[(i << 1)] + xTemp + 1,
                         offsets[(i << 1) + 1] + yTemp + 1, 0, 0, 0, 128);
            texrect_draw(&gMenuFrame.displayList, gMenuSelectionArrows[i], offsets[(i << 1)] + xTemp - 1,
                         offsets[(i << 1) + 1] + yTemp - 1, 255, 255, 255, 255);
        }
    }
    rendermode_reset(&gMenuFrame.displayList);
}

#define STICK_DEADZONE 35
#define STICK_DELAY_AMOUNT 15

void load_menu_text(s32 language) {
    char **menuText;
    char **temp;
    s32 langIndex;
    s32 size;
    char **fake;

    if (gMenuTextLangTable == NULL) {
        gMenuTextLangTable = (s32 *) asset_table_load(ASSET_MENU_TEXT_TABLE);
        {
            // Big-endian table: [0] string count, [1..4] per-language offsets.
            extern void pc_swap32_buf(void *buf, u32 numBytes);
            pc_swap32_buf(gMenuTextLangTable, asset_table_size(ASSET_MENU_TEXT_TABLE));
        }
    }

    switch (language) {
        case LANGUAGE_JAPANESE:
            langIndex = 4;
            break;
        case LANGUAGE_GERMAN:
            langIndex = 3;
            break;
        case LANGUAGE_FRENCH:
            langIndex = 2;
            break;
        default: // English
            langIndex = 1;
            break;
    }

    size = gMenuTextLangTable[langIndex + 1];
    langIndex = gMenuTextLangTable[langIndex];
    size -= langIndex;
    temp = gMenuLoaded.text;

    if (temp == NULL) {
        return;
    }

    {
        // The blob leads with a big-endian string-offset table and the strings
        // follow. On N64 it is loaded over the pointer table itself and the
        // offsets are turned into pointers in place; host pointers are wider
        // than the 4-byte entries, so keep the blob in its own buffer and point
        // the table into it.
        static u8 *sPcMenuTextBlob = NULL;
        if (sPcMenuTextBlob != NULL) {
            mempool_free(sPcMenuTextBlob);
        }
        sPcMenuTextBlob = mempool_alloc_safe(size, COLOUR_TAG_WHITE);
        asset_load(ASSET_MENU_TEXT, (uintptr_t) sPcMenuTextBlob, langIndex, size);
        for (langIndex = 0; langIndex < gMenuTextLangTable[0]; langIndex++) {
            u32 offset = pc_be32(sPcMenuTextBlob, langIndex * 4);
            gMenuLoaded.text[langIndex] = (offset == 0xFFFFFFFF) ? NULL : (char *) sPcMenuTextBlob + offset;
        }
    }

    menuText = gMenuLoaded.text;
    gAudioOutputStrings[0] = menuText[ASSET_MENU_TEXT_STEREO];                       // "STEREO"
    gAudioOutputStrings[1] = menuText[ASSET_MENU_TEXT_MONO];                         // "MONO"
    gAudioOutputStrings[2] = menuText[ASSET_MENU_TEXT_HEADPHONES];                   // "HEADPHONES"
    gAudioMenuStrings[1].text = menuText[ASSET_MENU_TEXT_SFXVOLUME];                 // "SFX VOLUME"
    gAudioMenuStrings[2].text = menuText[ASSET_MENU_TEXT_MUSICVOLUME];               // "MUSIC VOLUME"
    gAudioMenuStrings[3].text = menuText[ASSET_MENU_TEXT_RETURN];                    // "RETURN"
    gAudioMenuStrings[4].text = menuText[ASSET_MENU_TEXT_AUDIOOPTIONS];              // "AUDIO OPTIONS"
    gAudioMenuStrings[5].text = menuText[ASSET_MENU_TEXT_AUDIOOPTIONS];              // "AUDIO OPTIONS"
    gMusicTestString = menuText[ASSET_MENU_TEXT_MUSICTEST];                          // "MUSIC TEST 00"
    gMagicCodeMenuStrings[0] = menuText[ASSET_MENU_TEXT_ENTERCODE];                  // "ENTER CODE"
    gMagicCodeMenuStrings[1] = menuText[ASSET_MENU_TEXT_CLEARALLCODES];              // "CLEAR ALL CODES"
    gMagicCodeMenuStrings[2] = menuText[ASSET_MENU_TEXT_CODELIST];                   // "CODE LIST"
    gMagicCodeMenuStrings[3] = menuText[ASSET_MENU_TEXT_RETURN];                     // "RETURN"
    gRaceResultsMenuElements[1].t.asciiText = menuText[ASSET_MENU_TEXT_LAPTIMES];    // "LAP TIMES"
    gRaceResultsMenuElements[2].t.asciiText = menuText[ASSET_MENU_TEXT_OVERALLTIME]; // "OVERALL TIME"
    gRaceOrderMenuElements[8].t.asciiText = menuText[ASSET_MENU_TEXT_RACEORDER];     // "RACE ORDER"
    gRaceOrderMenuElements[9].t.asciiText = menuText[ASSET_MENU_TEXT_RACEORDER];     // "RACE ORDER"
    gRecordTimesMenuElements[0].t.asciiText = menuText[ASSET_MENU_TEXT_RECORDTIMES]; // "RECORD TIMES"
    gRecordTimesMenuElements[1].t.asciiText = menuText[ASSET_MENU_TEXT_RECORDTIMES]; // "RECORD TIMES"
    gRecordTimesMenuElements[2].t.asciiText = menuText[ASSET_MENU_TEXT_BESTTIME];    // "BEST TIME"
    gRecordTimesMenuElements[5].t.asciiText = menuText[ASSET_MENU_TEXT_BESTLAP];     // "BEST LAP"
    gOptionMenuStrings[0] = menuText[ASSET_MENU_TEXT_LANGUAGE];                      // "ENGLISH"
    if (gRaceSetup.eepromSettings & 0x2000000) {
        gOptionMenuStrings[1] = menuText[ASSET_MENU_TEXT_SUBTITLESON]; // "SUBTITLES ON"
    } else {
        gOptionMenuStrings[1] = menuText[ASSET_MENU_TEXT_SUBTITLESOFF]; // "SUBTITLES OFF"
    }
    gOptionMenuStrings[2] = menuText[ASSET_MENU_TEXT_AUDIOOPTIONS];                          // "AUDIO OPTIONS"
    gOptionMenuStrings[3] = menuText[ASSET_MENU_TEXT_SAVEOPTIONS];                           // "SAVE OPTIONS"
    gOptionMenuStrings[4] = menuText[ASSET_MENU_TEXT_MAGICCODES];                            // "MAGIC CODES"
    gOptionMenuStrings[5] = menuText[ASSET_MENU_TEXT_RETURN];                                // "RETURN"
    gFilenames[0] = menuText[ASSET_MENU_TEXT_GAMEA];                                         // "GAME A"
    gFilenames[1] = menuText[ASSET_MENU_TEXT_GAMEB];                                         // "GAME B"
    gFilenames[2] = menuText[ASSET_MENU_TEXT_GAMEC];                                         // "GAME C"
    gContPakNotPresentStrings[0] = menuText[ASSET_MENU_TEXT_CONTPAKX];                       // "CONTROLLER PAK ~"
    gContPakNotPresentStrings[1] = menuText[ASSET_MENU_TEXT_CONTPAKNOTPRESENT];              // "IS NOT PRESENT."
    gContPakNotPresentStrings[3] = menuText[ASSET_MENU_TEXT_CANCEL];                         // "CANCEL"
    gContPakCorruptDataRepairStrings[0] = menuText[ASSET_MENU_TEXT_CONTPAKX];                // "CONTROLLER PAK ~"
    gContPakCorruptDataRepairStrings[1] = menuText[ASSET_MENU_TEXT_CONTPAKHASCORRUPTDATA_0]; // "CONTAINS CORRUPT DATA."
    gContPakCorruptDataRepairStrings[3] = menuText[ASSET_MENU_TEXT_CONTPAKHASCORRUPTDATA_1]; // "ATTEMPT TO REPAIR"
    gContPakCorruptDataRepairStrings[4] = menuText[ASSET_MENU_TEXT_CANCEL];                  // "CANCEL"
    gContPakDamagedStrings[0] = menuText[ASSET_MENU_TEXT_CONTPAKX];                          // "CONTROLLER PAK ~"
    gContPakDamagedStrings[1] = menuText[ASSET_MENU_TEXT_CONTPAKISDAMAGED_0];                // "IRREPARABLY DAMAGED."
    gContPakDamagedStrings[3] = menuText[ASSET_MENU_TEXT_CONTPAKISDAMAGED_1];                // "REFORMAT PAK"
    gContPakDamagedStrings[4] = menuText[ASSET_MENU_TEXT_CANCEL];                            // "CANCEL"
    gContPakFullStrings[0] = menuText[ASSET_MENU_TEXT_CONTPAKX];                             // "CONTROLLER PAK ~"
    gContPakFullStrings[1] = menuText[ASSET_MENU_TEXT_CONTPAKISFULL];                        // "FULL."
    gContPakFullStrings[3] = menuText[ASSET_MENU_TEXT_CONTINUE];                             // "CONTINUE"
    gContPakDiffContStrings[0] = menuText[ASSET_MENU_TEXT_CONTPAKX];                         // "CONTROLLER PAK ~"
    gContPakDiffContStrings[1] = menuText[ASSET_MENU_TEXT_CONTPAKDIFFERENT_0];               // "DIFFERENT CONTROLLER"
    gContPakDiffContStrings[2] = menuText[ASSET_MENU_TEXT_CONTPAKDIFFERENT_1];               // "PAK IS INSERTED."
    gContPakDiffContStrings[4] = menuText[ASSET_MENU_TEXT_CANCEL];                           // "CANCEL"
    gContPakNoRoomForGhostsStrings[0] = menuText[ASSET_MENU_TEXT_CONTPAKX];                  // "CONTROLLER PAK ~"
    gContPakNoRoomForGhostsStrings[1] = menuText[ASSET_MENU_TEXT_CANNOTSTOREANYMOREGHOSTS_0]; // "CANNOT STORE ANY"
    gContPakNoRoomForGhostsStrings[2] = menuText[ASSET_MENU_TEXT_CANNOTSTOREANYMOREGHOSTS_1]; // "MORE GHOSTS."
    gContPakNoRoomForGhostsStrings[4] = menuText[ASSET_MENU_TEXT_CONTINUE];                   // "CONTINUE"
    gContPakCorruptDataStrings[0] = menuText[ASSET_MENU_TEXT_CONTPAKX];                       // "CONTROLLER PAK ~"
    gContPakCorruptDataStrings[1] = menuText[ASSET_MENU_TEXT_CORRUPTDATA_0];                  // "CORRUPT DATA."
    gContPakCorruptDataStrings[3] = menuText[ASSET_MENU_TEXT_TRYAGAIN];                       // "TRY AGAIN"
    gContPakCorruptDataStrings[4] = menuText[ASSET_MENU_TEXT_CANCEL];                         // "CANCEL"
    gContPakRumbleDetectedStrings[0] = menuText[ASSET_MENU_TEXT_RUMBLEPAKDETECTED_0];         // "Rumble Pak Detected"
    gContPakRumbleDetectedStrings[1] = menuText[ASSET_MENU_TEXT_RUMBLEPAKDETECTED_1];   // "Insert any Controller Paks"
    gContPakRumbleDetectedStrings[2] = menuText[ASSET_MENU_TEXT_RUMBLEPAKDETECTED_2];   // "you wish to use now!"
    gContPakRumbleDetectedStrings[4] = menuText[ASSET_MENU_TEXT_CONTINUE];              // "CONTINUE"
    gContPakSwitchToRumbleStrings[0] = menuText[ASSET_MENU_TEXT_INSERTDEVICE_0];        // "If you wish to use any"
    gContPakSwitchToRumbleStrings[1] = menuText[ASSET_MENU_TEXT_INSERTDEVICE_1];        // "Rumble Paks then please"
    gContPakSwitchToRumbleStrings[2] = menuText[ASSET_MENU_TEXT_INSERTDEVICE_2];        // "insert them now."
    gContPakSwitchToRumbleStrings[4] = menuText[ASSET_MENU_TEXT_CONTINUE];              // "CONTINUE"
    gContPakNeed2ndAdvStrings[0] = menuText[ASSET_MENU_TEXT_ADVTWOLOADERROR_0];         // "SORRY, BUT YOU"
    gContPakNeed2ndAdvStrings[1] = menuText[ASSET_MENU_TEXT_ADVTWOLOADERROR_1];         // "CAN'T LOAD GAMES"
    gContPakNeed2ndAdvStrings[2] = menuText[ASSET_MENU_TEXT_ADVTWOLOADERROR_2];         // "FOR THE SECOND"
    gContPakNeed2ndAdvStrings[3] = menuText[ASSET_MENU_TEXT_ADVTWOLOADERROR_3];         // "ADVENTURE UNTIL"
    gContPakNeed2ndAdvStrings[4] = menuText[ASSET_MENU_TEXT_ADVTWOLOADERROR_4];         // "YOU HAVE COMPLETED"
    gContPakNeed2ndAdvStrings[5] = menuText[ASSET_MENU_TEXT_ADVTWOLOADERROR_5];         // "THE FIRST."
    gContPakNeed2ndAdvStrings[7] = menuText[ASSET_MENU_TEXT_CONTINUE];                  // "CONTINUE"
    gTitleMenuStrings[0] = menuText[ASSET_MENU_TEXT_START];                             // "START"
    gTitleMenuStrings[1] = menuText[ASSET_MENU_TEXT_OPTIONS];                           // "OPTIONS"
    gGameSelectTextElemsNoAdv2[0].t.asciiText = menuText[ASSET_MENU_TEXT_GAMESELECT];   // "GAME SELECT"
    gGameSelectTextElemsNoAdv2[1].t.asciiText = menuText[ASSET_MENU_TEXT_GAMESELECT];   // "GAME SELECT"
    gGameSelectTextElemsNoAdv2[3].t.asciiText = menuText[ASSET_MENU_TEXT_ADVENTURE];    // "ADVENTURE"
    gGameSelectTextElemsNoAdv2[5].t.asciiText = menuText[ASSET_MENU_TEXT_TRACKS];       // "TRACKS"
    gGameSelectTextElemsWithAdv2[0].t.asciiText = menuText[ASSET_MENU_TEXT_GAMESELECT]; // "GAME SELECT"
    gGameSelectTextElemsWithAdv2[1].t.asciiText = menuText[ASSET_MENU_TEXT_GAMESELECT]; // "GAME SELECT"
    gGameSelectTextElemsWithAdv2[3].t.asciiText = menuText[ASSET_MENU_TEXT_ADVENTURE];  // "ADVENTURE"
    gGameSelectTextElemsWithAdv2[5].t.asciiText = menuText[ASSET_MENU_TEXT_ADVENTURETWO]; // "ADVENTURE TWO"
    gGameSelectTextElemsWithAdv2[7].t.asciiText = menuText[ASSET_MENU_TEXT_TRACKS];       // "TRACKS"
    sBadControllerPakMenuText[0] = menuText[ASSET_MENU_TEXT_BADCONTPAK];                  // "BAD CONTROLLER PAK"
    sControllerPakFullMenuText[0] = menuText[ASSET_MENU_TEXT_CONTPAKFULL];                // "CONTROLLER PAK FULL"
    sNoControllerPakMenuText[0] = menuText[ASSET_MENU_TEXT_NOCONTPAK];                    // 'NO CONTROLLER PAK"
    sCorruptDataMenuText[0] = menuText[ASSET_MENU_TEXT_CORRUPTDATA_0];                    // "CORRUPT DATA."
    sCorruptDataMenuText[4] = NULL;
    sCorruptDataMenuText[5] = NULL;

    // Three messages
    //"If you wish to change" / "Controller Pak or Rumble Pak," / "please do so now."
    for (langIndex = 0; langIndex <= 2; langIndex++) {
        sBadControllerPakMenuText[langIndex + 1] = menuText[ASSET_MENU_TEXT_CANNOTSAVEGHOSTDATA_0 + langIndex];
        sControllerPakFullMenuText[langIndex + 1] = menuText[ASSET_MENU_TEXT_CANNOTSAVEGHOSTDATA_0 + langIndex];
        sNoControllerPakMenuText[langIndex + 1] = menuText[ASSET_MENU_TEXT_CANNOTSAVEGHOSTDATA_0 + langIndex];
        sCorruptDataMenuText[langIndex + 1] = menuText[ASSET_MENU_TEXT_CANNOTSAVEGHOSTDATA_0 + langIndex];
    }

    for (langIndex = 0; sCorruptDataMenuText[langIndex] != NULL; langIndex++) {}

    sCorruptDataMenuText[langIndex] = menuText[ASSET_MENU_TEXT_CORRUPTDATA_1];  // "TRY AGAIN!"
    sInsertControllerPakMenuText[0] = menuText[ASSET_MENU_TEXT_INSERTDEVICE_3]; // "If you wish to use"
    sInsertControllerPakMenuText[1] = menuText[ASSET_MENU_TEXT_INSERTDEVICE_4]; // "the Controller Pak"
#if VERSION == VERSION_77
    sInsertControllerPakMenuText[2] = menuText[ASSET_MENU_TEXT_INSERTDEVICE_6]; // "insert it now!"
    sInsertRumblePakMenuText[0] = menuText[ASSET_MENU_TEXT_INSERTDEVICE_3];     // "If you wish to use"
    sInsertRumblePakMenuText[1] = menuText[ASSET_MENU_TEXT_INSERTDEVICE_5];     // "the Rumble Pak"
    sInsertRumblePakMenuText[2] = menuText[ASSET_MENU_TEXT_INSERTDEVICE_6];     // "insert it now!"
#else
    // TODO: Figure out what these text values are for these versions.
    sInsertControllerPakMenuText[2] = menuText[161];
    sInsertRumblePakMenuText[0] = menuText[162];
    sInsertRumblePakMenuText[1] = menuText[186];
    sInsertRumblePakMenuText[2] = menuText[187];
#endif
    gCautionMenuTextElements[0].t.asciiText = menuText[ASSET_MENU_TEXT_CAUTION]; // "CAUTION"
    gCautionMenuTextElements[1].t.asciiText = menuText[ASSET_MENU_TEXT_CAUTION]; // "CAUTION"

    for (langIndex = 0; langIndex <= 10; langIndex++) {
        // Caution message lines starting from gMenuLoaded.text[166]
        gCautionMenuTextElements[langIndex + 2].t.element = menuText[ASSET_MENU_TEXT_CAUTIONMESSAGE_0 + langIndex];
    }
}

/**
 * Free the geometry used by the 3D menu buttons.
 */
void menu_button_free(void) {
    if (gWoodPanelTriangles[0] != NULL) {
        mempool_free(gWoodPanelTriangles[0]);
        gWoodPanelTriangles[0] = NULL;
    }
    gWoodPanelTriangles[1] = NULL;
    gMenuGeometry = NULL;
    gWoodPanelVertices[0] = NULL;
    gWoodPanelVertices[1] = NULL;
    gWoodPanelCount = 0;
    gWoodPanelAllocCount = 0;
}

// Allocate and initalize a certain number of wooden panels used for the UI.
void menu_panels_alloc(s32 numberOfPanels) {
    s32 triListIndex;
    s32 triItemIndex;
    s32 IndicesIndex;
    s32 panelIndex;
    s32 i;
    s32 triangleCount;
    s32 vertexSize;
    s32 menuGeometrySize;

    if (gMenuGeometry != NULL) {
        menu_button_free();
    }

    gWoodPanelTexScaleU = 32; // 32 = 1.0x scale
    gWoodPanelTexScaleV = 32; // 32 = 1.0x scale

    menuGeometrySize = sizeof(MenuPanelGeometry) * numberOfPanels;
    vertexSize = sizeof(Vertex) * (20 * numberOfPanels);
    triangleCount = sizeof(Triangle) * (10 * numberOfPanels);

    gWoodPanelTriangles[0] =
        (Triangle *) mempool_alloc_safe(((vertexSize + triangleCount) * 2) + menuGeometrySize, COLOUR_TAG_WHITE);
    gWoodPanelTriangles[1] = (Triangle *) ((uintptr_t) gWoodPanelTriangles[0] + triangleCount);

    gMenuGeometry = (MenuPanelGeometry *) ((uintptr_t) gWoodPanelTriangles[1] + triangleCount);

    gWoodPanelVertices[0] = (Vertex *) ((uintptr_t) gMenuGeometry + menuGeometrySize);
    gWoodPanelVertices[1] = (Vertex *) ((uintptr_t) gWoodPanelVertices[0] + vertexSize);

    panelIndex = 0;
    triangleCount = 0;
    for (i = 0; i < numberOfPanels; i++) {
        gMenuGeometry[i].vertices[0] = &gWoodPanelVertices[0][panelIndex];
        gMenuGeometry[i].vertices[1] = &gWoodPanelVertices[1][panelIndex];
        gMenuGeometry[i].triangles[0] = &gWoodPanelTriangles[0][triangleCount];
        gMenuGeometry[i].triangles[1] = &gWoodPanelTriangles[1][triangleCount];
        gMenuGeometry[i].texture[0] = 0;
        gMenuGeometry[i].texture[1] = 0;
        gMenuGeometry[i].drawnImmediately[0] = 0;
        gMenuGeometry[i].drawnImmediately[1] = 0;
        panelIndex += 20;
        triangleCount += 10;
    }

    triItemIndex = 0;
    for (i = 0; i < numberOfPanels; i++) {
        panelIndex = 0;
        for (IndicesIndex = 0; IndicesIndex < 10; triItemIndex++, panelIndex += 3, IndicesIndex++) {
            for (triListIndex = 0; triListIndex < 2; triListIndex++) {
                gWoodPanelTriangles[triListIndex][triItemIndex].verticesArray[0] = BACKFACE_DRAW;
                gWoodPanelTriangles[triListIndex][triItemIndex].verticesArray[1] = gWoodPanelsIndices[panelIndex];
                gWoodPanelTriangles[triListIndex][triItemIndex].verticesArray[2] = gWoodPanelsIndices[panelIndex + 1];
                gWoodPanelTriangles[triListIndex][triItemIndex].verticesArray[3] = gWoodPanelsIndices[panelIndex + 2];
                gWoodPanelTriangles[triListIndex][triItemIndex].uv0.u = 0;
                gWoodPanelTriangles[triListIndex][triItemIndex].uv0.v = 0;
                gWoodPanelTriangles[triListIndex][triItemIndex].uv1.u = 0;
                gWoodPanelTriangles[triListIndex][triItemIndex].uv1.v = 0;
                gWoodPanelTriangles[triListIndex][triItemIndex].uv2.u = 0;
                gWoodPanelTriangles[triListIndex][triItemIndex].uv2.v = 0;
            }
        }
    }
    gMenuTrisFlip = 0;
    gWoodPanelCount = 0;
    gWoodPanelAllocCount = numberOfPanels;
}

/**
 * Resize the UV's of the menu panels.
 */
UNUSED void menu_button_uvs(f32 u, f32 v) {
    gWoodPanelTexScaleU = u * 32.0f;
    gWoodPanelTexScaleV = v * 32.0f;
}

void menu_panels_render(Gfx **dList) {
    UNUSED s16 pad;
    s32 i;
    s32 var_t0;
    TextureHeader *tex;
    TextureHeader *lastTex;

    gSPDisplayList((*dList)++, &dMenuHudSettings);
    var_t0 = -1;
    lastTex = NULL;

    for (i = 0; i < gWoodPanelCount; i++) {
        if (!gMenuGeometry[i].drawnImmediately[gMenuTrisFlip]) {
            tex = gMenuGeometry[i].texture[gMenuTrisFlip];
            if (tex != NULL) {
                if (var_t0 != 1) {
                    var_t0 = 1;
                    gDkrDmaDisplayList((*dList)++, OS_K0_TO_PHYSICAL(&dMenuHudDrawModes[1]), 2);
                }
                if (lastTex != tex) {
                    gDkrDmaDisplayList((*dList)++, OS_K0_TO_PHYSICAL(tex->cmd), tex->numberOfCommands);
                    lastTex = tex;
                }
            } else if (var_t0 != 0) {
                var_t0 = 0;
                gDkrDmaDisplayList((*dList)++, OS_K0_TO_PHYSICAL(&dMenuHudDrawModes[0]), 2);
            }
            gDPPipeSync((*dList)++);
            gSPVertexDKR((*dList)++, OS_K0_TO_PHYSICAL(gMenuGeometry[i].vertices[gMenuTrisFlip]), 20, 0);
            gSPPolygon((*dList)++, OS_K0_TO_PHYSICAL(gMenuGeometry[i].triangles[gMenuTrisFlip]), 10, var_t0);
        }
    }
    gWoodPanelCount = 0;
    gMenuTrisFlip = 1 - gMenuTrisFlip;
    rendermode_reset(dList);
}

/**
 * Resets the menu geometry after drawing, then flips the geometry index for the next frame.
 */
void menu_geometry_end(void) {
    gWoodPanelCount = 0;
    gMenuTrisFlip = 1 - gMenuTrisFlip;
}

/**
 * Sets the title reveal timer to zero when the menu boots, meaning it may not automatically appear.
 */
void reset_title_logo_scale(void) {
    gResetTitleScale = TRUE;
}

/**
 * Core function for initialising all menu elements.
 * Gets the current menu ID, then runs the init function of the respective menu element.
 */
void menu_init(u32 menuId) {
    s32 i;

    gCurrentMenuId = menuId;
    reset_controller_sticks();
    gMenuInput.ignoreInputTime = 1;
    gMenuFlow.blinkTimer = 0;
    sUnused_801263C8 = 0;
    sUnused_80126828 = 0;

    // clang-format off
    // Needs to be one line
    for (i = 0; i < ARRAY_COUNT(gMenuSoundMasks); i++) { gMenuSoundMasks[i] = NULL; }
    // clang-format on

    music_stop();
    switch (gCurrentMenuId) {
        case MENU_LOGOS:
            menu_logos_screen_init();
            break;
        case MENU_TITLE:
            menu_title_screen_init();
            break;
        case MENU_OPTIONS:
            menu_options_init();
            break;
        case MENU_AUDIO_OPTIONS:
            menu_audio_options_init();
            break;
        case MENU_SAVE_OPTIONS:
            menu_save_options_init();
            break;
        case MENU_MAGIC_CODES:
            menu_magic_codes_init();
            break;
        case MENU_MAGIC_CODES_LIST:
            menu_magic_codes_list_init();
            break;
        case MENU_CHARACTER_SELECT:
            menu_character_select_init();
            break;
        case MENU_GAME_SELECT:
            menu_game_select_init();
            break;
        case MENU_FILE_SELECT:
            menu_file_select_init();
            break;
        case MENU_TRACK_SELECT:
            menu_track_select_init();
            break;
        case MENU_TRACK_SELECT_ADVENTURE:
            menu_adventure_track_init();
            break;
        case MENU_RESULTS:
            menu_results_init();
            break;
        case MENU_TROPHY_RACE_ROUND:
            menu_trophy_race_round_init();
            break;
        case MENU_TROPHY_RACE_RANKINGS:
            menu_trophy_race_rankings_init();
            break;
        case MENU_NEWGAME_CINEMATIC:
            menu_cinematic_init();
            break;
        case MENU_GHOST_DATA:
            menu_ghost_data_init();
            break;
        case MENU_CREDITS:
            menu_credits_init();
            break;
        case MENU_BOOT:
            menu_boot_init();
            break;
        case MENU_CAUTION:
            menu_caution_init();
            break;
    }
    sUnused_80126470 = 0xD000;
}

/**
 * Runs every frame. Calls the loop function of the current menu id
 */
s32 menu_loop(Gfx **currDisplayList, Mtx **currHudMat, Vertex **currHudVerts, Triangle **currHudTris, s32 updateRate) {
    s32 ret;

    gMenuFrame.displayList = *currDisplayList;
    gMenuFrame.hudMat = *currHudMat;
    gMenuFrame.hudVerts = *currHudVerts;
    sMenuCurrHudTris = *currHudTris;

    update_controller_sticks();
    switch (gCurrentMenuId) {
        case MENU_LOGOS:
            ret = menu_logo_screen_loop(updateRate);
            break;
        case MENU_TITLE:
            ret = menu_title_screen_loop(updateRate);
            break;
        case MENU_OPTIONS:
            ret = menu_options_loop(updateRate);
            break;
        case MENU_AUDIO_OPTIONS:
            ret = menu_audio_options_loop(updateRate);
            break;
        case MENU_SAVE_OPTIONS:
            ret = menu_save_options_loop(updateRate);
            break;
        case MENU_MAGIC_CODES:
            ret = menu_magic_codes_loop(updateRate);
            break;
        case MENU_MAGIC_CODES_LIST:
            ret = menu_magic_codes_list_loop(updateRate);
            break;
        case MENU_CHARACTER_SELECT:
            ret = menu_character_select_loop(updateRate);
            break;
        case MENU_FILE_SELECT:
            ret = menu_file_select_loop(updateRate);
            break;
        case MENU_GAME_SELECT:
            ret = menu_game_select_loop(updateRate);
            break;
        case MENU_TRACK_SELECT:
            ret = menu_track_select_loop(updateRate);
            break;
        case MENU_TRACK_SELECT_ADVENTURE:
            ret = menu_adventure_track_loop(updateRate);
            break;
        case MENU_RESULTS:
            ret = menu_results_loop(updateRate);
            break;
        case MENU_TROPHY_RACE_ROUND:
            ret = menu_trophy_race_round_loop(updateRate);
            break;
        case MENU_TROPHY_RACE_RANKINGS:
            ret = menu_trophy_race_rankings_loop(updateRate);
            break;
        case MENU_NEWGAME_CINEMATIC:
            ret = menu_cinematic_loop(updateRate);
            break;
        case MENU_GHOST_DATA:
            ret = menu_ghost_data_loop(updateRate);
            break;
        case MENU_CREDITS:
            ret = menu_credits_loop(updateRate);
            break;
        case MENU_BOOT:
            ret = menu_boot_loop(updateRate);
            break;
        case MENU_CAUTION:
            ret = menu_caution_loop(updateRate);
            break;
#ifdef AVOID_UB
        default:
            ret = MENU_RESULT_CONTINUE;
#endif
    }
    *currDisplayList = gMenuFrame.displayList;
    *currHudMat = gMenuFrame.hudMat;
    *currHudVerts = gMenuFrame.hudVerts;
    *currHudTris = sMenuCurrHudTris;
    return ret;
}

/**
 * Print a time onscreen.
 */
void menu_timestamp_render(s32 frameCount, s32 xPos, s32 yPos, u8 red, u8 green, u8 blue, u8 fontID) {
    s32 minutes;
    s32 seconds;
    s32 hundredths;
    s32 xOffset1;
    s32 imageIndex2;
    s32 imageIndex;
    s32 xOffset2;
    s32 xOffset3;

    gMenuFrame.guiColourR = red;
    gMenuFrame.guiColourG = green;
    gMenuFrame.guiColourB = blue;
    if (fontID == ASSET_FONTS_FUNFONT) {
        gMenuLoaded.images[0].trans.y_position = yPos;
        gMenuLoaded.images[1].trans.y_position = yPos - 2;
        imageIndex = 0;
        imageIndex2 = 1;
        xPos -= 39;
        xOffset1 = 12;
        xOffset2 = 11;
        xOffset3 = 10;
    } else {
        gMenuLoaded.images[2].trans.y_position = yPos;
        gMenuLoaded.images[3].trans.y_position = yPos - 1;
        imageIndex = 2;
        imageIndex2 = 3;
        xPos -= 28;
        xOffset1 = 9;
        xOffset2 = 9;
        xOffset3 = 5;
    }
    get_timestamp_from_frames(frameCount, &minutes, &seconds, &hundredths);
    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    sprite_opaque(FALSE);

    gMenuLoaded.images[imageIndex].spriteOffset = minutes / 10;
    gMenuLoaded.images[imageIndex].trans.x_position = xPos;
    menu_element_render(imageIndex);

    xPos += xOffset1;
    gMenuLoaded.images[imageIndex].spriteOffset = minutes % 10;
    gMenuLoaded.images[imageIndex].trans.x_position = xPos;
    menu_element_render(imageIndex);

    xPos += xOffset2;
    gMenuLoaded.images[imageIndex2].trans.x_position = xPos;
    menu_element_render(imageIndex2);

    xPos += xOffset3;
    gMenuLoaded.images[imageIndex].spriteOffset = seconds / 10;
    gMenuLoaded.images[imageIndex].trans.x_position = xPos;
    menu_element_render(imageIndex);

    xPos += xOffset1;
    gMenuLoaded.images[imageIndex].spriteOffset = seconds % 10;
    gMenuLoaded.images[imageIndex].trans.x_position = xPos;
    menu_element_render(imageIndex);

    xPos += xOffset2;
    gMenuLoaded.images[imageIndex2].trans.x_position = xPos;
    menu_element_render(imageIndex2);

    xPos += xOffset3;
    gMenuLoaded.images[imageIndex].spriteOffset = hundredths / 10;
    gMenuLoaded.images[imageIndex].trans.x_position = xPos;
    menu_element_render(imageIndex);

    xPos += xOffset1;
    gMenuLoaded.images[imageIndex].spriteOffset = hundredths % 10;
    gMenuLoaded.images[imageIndex].trans.x_position = xPos;
    menu_element_render(imageIndex);

    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
    sprite_opaque(TRUE);
    gMenuFrame.guiColourR = 255;
    gMenuFrame.guiColourG = 255;
    gMenuFrame.guiColourB = 255;
}

/**
 * Convert the number to a string and then render as an element at the given position.
 */
void menu_number_render(s32 number, s32 x, s32 y, s32 r, s32 g, s32 b, s32 a, UNUSED s32 font, s32 alignment) {
    s32 strLen;
    s32 powerOfTen;
    s32 i;
    s32 hasProcessedDigit;
    s32 curDigit;
    UNUSED s32 pad[2];
    u8 digits[10];

    strLen = 0;
    powerOfTen = 1000000000;
    hasProcessedDigit = FALSE;

    while (powerOfTen >= 10) {
        if (number >= powerOfTen) {
            curDigit = number / powerOfTen;
            number -= curDigit * powerOfTen;
            digits[strLen++] = curDigit;
            hasProcessedDigit = TRUE;
        } else if (hasProcessedDigit) {
            digits[strLen++] = 0;
        }
        powerOfTen /= 10;
    }

    digits[strLen++] = number;
    if (alignment & HORZ_ALIGN_CENTER) {
        x -= strLen * 12;
        x += 6;
    } else if (alignment & HORZ_ALIGN_RIGHT) {
        x -= (strLen * 11) >> 1;
    } else {
        x += 6;
    }
    if (alignment & VERT_ALIGN_MIDDLE) {
        y += 7;
    } else if (alignment & 0x10) {
        y -= 7;
    }
    gMenuFrame.guiColourR = r;
    gMenuFrame.guiColourG = g;
    gMenuFrame.guiColourB = b;
    gMenuFrame.guiOpacity = a;
    sprite_opaque(0);
    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
    if (powerOfTen && number) {} // Fakematch
    gMenuLoaded.images[0].trans.y_position = y;
    for (i = 0; i < strLen; i++) {
        gMenuLoaded.images[0].trans.x_position = x;
        gMenuLoaded.images[0].spriteOffset = digits[i];
        menu_element_render(0);
        x += 12;
    }
    sprite_opaque(1);
    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
    gMenuFrame.guiColourR = 255;
    gMenuFrame.guiColourG = 255;
    gMenuFrame.guiColourB = 255;
    gMenuFrame.guiOpacity = 255;
}

/**
 * Render the group of menu elements onscreen.
 * Elements can be a range of things, so this checks what it is,
 * then calls a specific function to render that element.
 */
void draw_menu_elements(s32 state, MenuElement *elems, f32 scale) {
    s32 shouldResetRenderSettings;
    s32 xPos, yPos;

    shouldResetRenderSettings = FALSE;
    if (state == 4) {
        return;
    }

    mtx_ortho(&gMenuFrame.displayList, &gMenuFrame.hudMat);
    while (elems->t.element != NULL) {
        if ((elems->t.element != &sMenuNullElement)) {                      // fakematch
            if (state == ((elems->t.element != (&sMenuNullElement)) * 0)) { // fakematch
                xPos = ((s32) ((elems->center - elems->left) * scale)) + elems->left;
                yPos = ((s32) ((elems->middle - elems->top) * scale)) + elems->top;
            } else if (state == 1) {
                xPos = elems->center;
                yPos = elems->middle;
            } else {
                xPos = ((s32) ((elems->right - elems->center) * scale)) + elems->center;
                yPos = ((s32) ((elems->bottom - elems->middle) * scale)) + elems->middle;
            }
            switch (elems->elementType) {
                case 0: // ascii text
                    set_text_background_colour(
                        elems->details.background.backgroundRed, elems->details.background.backgroundGreen,
                        elems->details.background.backgroundBlue, elems->details.background.backgroundAlpha);
                    set_text_colour(elems->filterRed, elems->filterGreen, elems->filterBlue, elems->filterBlendFactor,
                                    elems->opacity);
                    set_text_font(elems->textFont);
                    draw_text(&gMenuFrame.displayList, xPos, yPos + gResults.textOffset, elems->t.asciiText,
                              elems->textAlignFlags);
                    break;
                case 1:
                    if (shouldResetRenderSettings) {
                        shouldResetRenderSettings = FALSE;
                        rendermode_reset(&gMenuFrame.displayList);
                    }
                    gMenuFrame.guiOpacity = elems->opacity;
                    menu_timestamp_render(*elems->t.numberU16, xPos - SCREEN_WIDTH_HALF,
                                          (-yPos - gResults.timestampOffset) + SCREEN_HEIGHT_HALF, elems->filterRed,
                                          elems->filterGreen, elems->filterBlue, elems->textFont);
                    break;
                case 2: // Number
                    if (shouldResetRenderSettings) {
                        shouldResetRenderSettings = FALSE;
                        rendermode_reset(&gMenuFrame.displayList);
                    }
                    menu_number_render(*elems->t.number, xPos - SCREEN_WIDTH_HALF,
                                       (-yPos - gResults.timestampOffset) + SCREEN_HEIGHT_HALF, elems->filterRed,
                                       elems->filterGreen, elems->filterBlue, elems->opacity, elems->textFont,
                                       elems->textAlignFlags);
                    break;
                case 3:
                    texrect_draw(&gMenuFrame.displayList, elems->t.drawTexture, xPos, yPos + gResults.textOffset,
                                 elems->filterRed, elems->filterGreen, elems->filterBlue, elems->opacity);
                    shouldResetRenderSettings = TRUE;
                    break;
                case 4:
                    texrect_draw_scaled(&gMenuFrame.displayList, elems->t.element, xPos, yPos + gResults.textOffset,
                                        elems->details.texture.width / 256.0f, elems->details.texture.height / 256.0f,
                                        (elems->filterRed << 24) | (elems->filterGreen << 16) |
                                            (elems->filterBlue << 8) | elems->opacity,
                                        elems->textAlignFlags);
                    shouldResetRenderSettings = TRUE;
                    break;
                case 5:
                    if (shouldResetRenderSettings) {
                        shouldResetRenderSettings = FALSE;
                        rendermode_reset(&gMenuFrame.displayList);
                    }
                    cam_set_sprite_anim_mode(SPRITE_ANIM_FRAME_INDEX);
                    sprite_opaque(FALSE);
                    gMenuLoaded.images[elems->t.assetID].trans.x_position = xPos - SCREEN_WIDTH_HALF;
                    gMenuLoaded.images[elems->t.assetID].trans.y_position =
                        (-yPos - gResults.timestampOffset) + SCREEN_HEIGHT_HALF;
                    gMenuLoaded.images[elems->t.assetID].spriteOffset = elems->textFont;
                    gMenuLoaded.images[elems->t.assetID].trans.rotation.z = elems->details.background.backgroundRed;
                    gMenuLoaded.images[elems->t.assetID].trans.rotation.y = elems->details.background.backgroundGreen;
                    gMenuLoaded.images[elems->t.assetID].trans.rotation.x = elems->details.background.backgroundBlue;
                    gMenuLoaded.images[elems->t.assetID].trans.scale =
                        elems->details.background.backgroundAlpha / 256.0f;
                    gMenuFrame.guiColourR = elems->filterRed;
                    gMenuFrame.guiColourG = elems->filterGreen;
                    gMenuFrame.guiColourB = elems->filterBlue;
                    gMenuFrame.guiColourBlendFactor = elems->filterBlendFactor;
                    gMenuFrame.guiOpacity = elems->opacity;
                    menu_element_render(elems->t.assetID);
                    cam_set_sprite_anim_mode(SPRITE_ANIM_NORMALIZED);
                    sprite_opaque(TRUE);
                    break;
                case 6:
                    menu_panel_draw_bordered(&gMenuFrame.displayList, xPos, yPos + gResults.timestampOffset,
                                             elems->details.texture.width, elems->details.texture.height,
                                             elems->details.texture.borderWidth, elems->details.texture.borderHeight,
                                             elems->filterRed, elems->filterGreen, elems->filterBlue, elems->opacity);
                    break;
                case 7: // Texture
                    menu_panel_draw_textured(&gMenuFrame.displayList, xPos, yPos + gResults.timestampOffset,
                                             elems->details.texture.width, elems->details.texture.height,
                                             elems->details.texture.borderWidth, elems->details.texture.borderHeight,
                                             (elems->filterRed << 24) | (elems->filterGreen << 16) |
                                                 (elems->filterBlue << 8) | elems->opacity,
                                             elems->t.element);
                    break;
            }
        }
        elems++; // Go onto the next element.
    }
    if (shouldResetRenderSettings) {
        rendermode_reset(&gMenuFrame.displayList);
    }
    gMenuFrame.guiColourR = 255;
    gMenuFrame.guiColourG = 255;
    gMenuFrame.guiColourB = 255;
    gMenuFrame.guiColourBlendFactor = 0;
    gMenuFrame.guiOpacity = 255;
}

/**
 * Render text onscreen saying that no controller is inserted.
 * A timer counts up making it flash on and off
 */
void menu_missing_controller(Gfx **dList, s32 updateRate) {
    s32 posY;

    gMissingControllerDelay += updateRate;

    if (gMissingControllerDelay & 0x10) {
        load_menu_text(get_language());
        set_text_font(ASSET_FONTS_FUNFONT);
        set_text_colour(255, 255, 255, 0, 0xFF);
        set_text_background_colour(0, 0, 0, 0);
        posY = 208;
        if (osTvType == OS_TV_TYPE_PAL) {
            posY = 234;
        }
        draw_text(dList, POS_CENTRED, posY, gMenuLoaded.text[ASSET_MENU_TEXT_CONTROLLERNOTCONNECTED],
                  ALIGN_MIDDLE_CENTER);
    }
}

/**
 * Update tracked input for the menu.
 * Adds all inputs together, since it wants every player to feel involved.
 */
void menu_input(void) {
    s32 i;
    s32 buttonsHeld[MAXCONTROLLERS + 1];

    if (gMenuInput.ignoreInputTime == 0) {
        for (i = 0; i < MAXCONTROLLERS + 1; i++) {
            buttonsHeld[i] = 0;
            gMenuInput.stickX[i] = 0;
            gMenuInput.stickY[i] = 0;
        }
        for (i = 0; i < MAXCONTROLLERS; i++) {
            buttonsHeld[i] = input_held(i);
            gMenuInput.stickX[i] = gMenuInput.xAxisDirection[i];
            gMenuInput.stickY[i] = gMenuInput.yAxisDirection[i];
            if (i < gRaceSetup.numberOfActivePlayers) {
                buttonsHeld[PLAYER_MENU] |= buttonsHeld[i];
                gMenuInput.stickX[PLAYER_MENU] += gMenuInput.xAxisDirection[i];
                gMenuInput.stickY[PLAYER_MENU] += gMenuInput.yAxisDirection[i];
            }
        }
        for (i = 0; i < MAXCONTROLLERS + 1; i++) {
            gMenuInput.buttons[i] = buttonsHeld[i] & ~buttonsPressed[i];
            buttonsPressed[i] = buttonsHeld[i];
        }
        return;
    }
    for (i = 0; i < MAXCONTROLLERS + 1; i++) {
        buttonsPressed[i] = (START_BUTTON | B_BUTTON | A_BUTTON);
        gMenuInput.buttons[i] = 0;
        gMenuInput.stickX[i] = 0;
        gMenuInput.stickY[i] = 0;
    }
}

/**
 * Explicitly says to unload the ASSET_FONTS_BIGFONT type.
 */
void menu_unload_bigfont(void) {
    unload_font(ASSET_FONTS_BIGFONT);
}

/**
 * Store the coordinates of the first camera and then move it to the middle of the scene, then update the viewport.
 * Write the original coordinates back once done.
 */
void menu_camera_centre(void) {
    Camera *cam;
    s16 angleY;
    s16 angleX;
    s16 angleZ;
    f32 posX;
    f32 posY;
    f32 posZ;

    cam_set_layout(VIEWPORT_LAYOUT_1_PLAYER);
    set_active_camera(0);

    cam = cam_get_active_camera();

    angleY = cam->trans.rotation.y_rotation;
    angleX = cam->trans.rotation.x_rotation;
    angleZ = cam->trans.rotation.z_rotation;
    posX = cam->trans.x_position;
    posY = cam->trans.y_position;
    posZ = cam->trans.z_position;
    cam->trans.rotation.z_rotation = 0;
    cam->trans.rotation.x_rotation = 0;
    cam->trans.rotation.y_rotation = 0x8000;
    cam->trans.x_position = -32.0f;
    cam->trans.y_position = -32.0f;
    cam->trans.z_position = -32.0f;

    update_envmap_position(0, 0, -1);
    viewport_main(&gMenuFrame.displayList, &gMenuFrame.hudMat);

    cam->trans.rotation.y_rotation = angleY;
    cam->trans.rotation.x_rotation = angleX;
    cam->trans.rotation.z_rotation = angleZ;
    cam->trans.x_position = posX;
    cam->trans.y_position = posY;
    cam->trans.z_position = posZ;
}

/**
 * Reset all menu related joystick inputs for each player.
 */
void reset_controller_sticks(void) {
    s32 i;
    for (i = 0; i < 4; i++) {
        gMenuInput.xAxisDirection[i] = 0;
        gMenuInput.yAxisDirection[i] = 0;
        gControllersXAxisDelay[i] = 0;
        gControllersYAxisDelay[i] = 0;
        gControllersXAxis[i] = 0;
        gControllersYAxis[i] = 0;
    }
}

/**
 * Reads the stick inputs, then with the aid of a maximum range and a deadzone, writes it to the menu inputs.
 * Stick input delay is not time corrected, meaning holding the stick is slower at lower framerates.
 */
void update_controller_sticks(void) {
    s32 XClamp, YClamp;
    s32 i;
    for (i = 0; i < 4; i++) {
        XClamp = input_clamp_stick_x(i);
        YClamp = input_clamp_stick_y(i);
        gMenuInput.xAxisDirection[i] = 0;
        gMenuInput.yAxisDirection[i] = 0;

        if (XClamp < -STICK_DEADZONE && gControllersXAxis[i] >= -STICK_DEADZONE) {
            gMenuInput.xAxisDirection[i] = -1;
            gControllersXAxisDelay[i] = 0;
        }
        if (XClamp > STICK_DEADZONE && gControllersXAxis[i] <= STICK_DEADZONE) {
            gMenuInput.xAxisDirection[i] = 1;
            gControllersXAxisDelay[i] = 0;
        }
        if (YClamp < -STICK_DEADZONE && gControllersYAxis[i] >= -STICK_DEADZONE) {
            gMenuInput.yAxisDirection[i] = -1;
            gControllersYAxisDelay[i] = 0;
        }
        if (YClamp > STICK_DEADZONE && gControllersYAxis[i] <= STICK_DEADZONE) {
            gMenuInput.yAxisDirection[i] = 1;
            gControllersYAxisDelay[i] = 0;
        }

        gControllersYAxis[i] = YClamp;
        if (gControllersYAxis[i] < -STICK_DEADZONE) {
            gControllersYAxisDelay[i]++; //!@Delta
        } else if (gControllersYAxis[i] > STICK_DEADZONE) {
            gControllersYAxisDelay[i]++; //!@Delta
        } else {
            gControllersYAxisDelay[i] = 0;
        }

        if (gControllersYAxisDelay[i] > STICK_DELAY_AMOUNT) {
            gControllersYAxis[i] = 0;
            gControllersYAxisDelay[i] = 0;
        }

        gControllersXAxis[i] = XClamp;
        if (gControllersXAxis[i] < -STICK_DEADZONE) {
            gControllersXAxisDelay[i]++; //!@Delta
        } else if (gControllersXAxis[i] > STICK_DEADZONE) {
            gControllersXAxisDelay[i]++; //!@Delta
        } else {
            gControllersXAxisDelay[i] = 0;
        }

        if (gControllersXAxisDelay[i] > STICK_DELAY_AMOUNT) {
            gControllersXAxis[i] = 0;
            gControllersXAxisDelay[i] = 0;
        }
    }
}

/**
 * Returns the value in gRaceSetup.numberOfActivePlayers.
 */
s32 get_number_of_active_players(void) {
    return gRaceSetup.numberOfActivePlayers;
}

/**
 * Returns the number of active players.
 * However, it will explicitly return 2 players if you're on a track in 2 player adventure mode,
 * as it would otherwise return 1.
 */
s32 get_active_player_count(void) {
    LevelHeader *header = level_header();
    if (gRaceSetup.isInTwoPlayerAdventure && !gRaceSetup.isInTracksMode) {
        if (header->race_type == RACETYPE_DEFAULT || header->race_type & RACETYPE_CHALLENGE) {
            return 2;
        }
    }
    return gRaceSetup.numberOfActivePlayers;
}

/**
 * Returns the number of racers for a multiplayer match.
 */
s32 get_multiplayer_racer_count(void) {
    if (!gRaceSetup.isInTracksMode) {
        return 6; // Two player adventure is fixed at 6 racers
    }
    if (gRaceSetup.trophyRaceWorldId) {
        return 6; // Trophy race is fixed at 6 racers
    }
    // gRaceSetup.multiplayerRacerCount is either 0 (2 racers), 1 (4 racers), or 2 (6 racers)
    return (gRaceSetup.multiplayerRacerCount + 1) << 1; // Doesn't match with multiply.
}

/**
 * Loops through each asset in the group and frees it from memory.
 */
void menu_assetgroup_free(s16 *assetGroup) {
    s32 index = 0;
    while (assetGroup[index] != -1) {
        menu_asset_free(assetGroup[index++]);
    }
}

/**
 * Check what type of asset this is, then free it accordingly.
 * Mark the asset ID as nonexistent after, and if that was the last existing asset,
 * also free the asset table.
 */
void menu_asset_free(s32 assetID) {
    if (gMenuAssetActive[assetID]) {
        if (gMenuLoaded.assets[assetID] != NULL) {
            if (((*gAssetsMenuElementIds)[assetID] & ASSET_MASK_TEXTURE) == ASSET_MASK_TEXTURE &&
                gMenuLoaded.assets[assetID] != NULL) {
                mempool_free_timer(0);
                tex_free(gMenuLoaded.assets[assetID]);
                mempool_free_timer(2);
            } else {
                if ((*gAssetsMenuElementIds)[assetID] & ASSET_MASK_SPRITE) {
                    sprite_free((Sprite *) gMenuLoaded.assets[assetID]);
                } else {
                    if ((*gAssetsMenuElementIds)[assetID] & ASSET_MASK_OBJECT) {
                        free_object((Object *) gMenuLoaded.assets[assetID]);
                    } else {
                        free_3d_model((ModelInstance *) gMenuLoaded.assets[assetID]);
                    }
                }
            }
        }
        gMenuLoaded.assets[assetID] = NULL;
        gMenuAssetActive[assetID] = FALSE;
        gMenuObjectsCount--;
        gParticlePtrList_flush();
    }
    if (gMenuObjectsCount == 0) {
        if (gMenuLoaded.images != NULL) {
            mempool_free(gMenuLoaded.images);
            gMenuLoaded.images = NULL;
        }
        if (*gAssetsMenuElementIds != NULL) {
            mempool_free(*gAssetsMenuElementIds);
            *gAssetsMenuElementIds = NULL;
            gMenuElementIdCount = 0;
        }
    }
}

/**
 * Loops through every asset in the group and loads it into memory.
 */
void menu_assetgroup_load(s16 *textureIndex) {
    s32 index = 0;
    while (textureIndex[index] != -1) {
        menu_asset_load(textureIndex[index++]);
    }
}

/**
 * Check what type of asset it is, then load it accordingly.
 * Mark the asset ID as existing after, and if it doesn't already exist,
 * load the asset table.
 */
void menu_asset_load(s32 assetID) {
    s32 i;
    LevelObjectEntryCommon entry;

    if (*gAssetsMenuElementIds == NULL) {
        *gAssetsMenuElementIds = (s16 *) asset_table_load(ASSET_MENU_ELEMENT_IDS);
        {
            // Big-endian s16 id table (helper in linux/reimpl.c).
            extern void pc_swap16_buf(void *buf, u32 numBytes);
            pc_swap16_buf(*gAssetsMenuElementIds, asset_table_size(ASSET_MENU_ELEMENT_IDS));
        }
        for (gMenuElementIdCount = 0; (*gAssetsMenuElementIds)[gMenuElementIdCount] != -1; gMenuElementIdCount++) {}
        gMenuObjectsCount = 0;
        for (i = 0; i < gMenuElementIdCount; i++) {
            gMenuAssetActive[i] = FALSE;
        }
    }
    if (((!assetID) && (!assetID)) && (!assetID)) {
        stubbed_printf("loadFrontEndItem() - Item no %d out of range 0-%d\n", assetID, sizeof(gMenuAssetActive));
    } // Fakematch
    if (!gMenuAssetActive[assetID]) {
        i = (*gAssetsMenuElementIds)[assetID];

        if ((i & ASSET_MASK_TEXTURE) == ASSET_MASK_TEXTURE) {
            gMenuLoaded.assets[assetID] = load_texture(i & 0x3FFF);
        } else if (i & ASSET_MASK_SPRITE) {
            gMenuLoaded.assets[assetID] = tex_load_sprite(i & 0x3FFF, 0);
        } else if (i & ASSET_MASK_OBJECT) {
            if (gMenuElementIdCount) {} // Fakematch
            entry.objectID = i & 0xFFFF;
            entry.size = sizeof(LevelObjectEntryCommon);
            entry.x = 0;
            entry.y = 0;
            entry.z = 0;
            gMenuLoaded.assets[assetID] = spawn_object(&entry, OBJECT_SPAWN_NONE);
        } else {
            gMenuLoaded.assets[assetID] = object_model_init(i & 0x3FFF, 0);
        }

        gMenuAssetActive[assetID] = TRUE;
        gMenuObjectsCount++;
    }
}

/**
 * Loop through every image in the group and load it into memory.
 */
void menu_imagegroup_load(s16 *imageSet) {
    s32 index = 0;
    while (imageSet[index] != -1) {
        menu_image_load(imageSet[index++]);
    }
}

/**
 * Load a menu image into memory, then set the base position.
 */
void menu_image_load(s32 imageID) {
    if (gMenuLoaded.images == NULL) {
        gMenuLoaded.images = mempool_alloc_safe(sizeof(MenuAsset) * 18, COLOUR_TAG_RED);
    }

    gMenuLoaded.images[imageID].trans.rotation.x = sMenuImageProperties[imageID].trans.rotation.x;
    gMenuLoaded.images[imageID].trans.rotation.y = sMenuImageProperties[imageID].trans.rotation.y;
    gMenuLoaded.images[imageID].trans.rotation.z = sMenuImageProperties[imageID].trans.rotation.z;
    gMenuLoaded.images[imageID].trans.spriteID = sMenuImageProperties[imageID].trans.spriteID;
    gMenuLoaded.images[imageID].trans.x_position = sMenuImageProperties[imageID].trans.x_position;
    gMenuLoaded.images[imageID].trans.y_position = sMenuImageProperties[imageID].trans.y_position;
    gMenuLoaded.images[imageID].trans.z_position = sMenuImageProperties[imageID].trans.z_position;
    gMenuLoaded.images[imageID].trans.scale = sMenuImageProperties[imageID].trans.scale;
    gMenuLoaded.images[imageID].spriteOffset = sMenuImageProperties[imageID].spriteOffset;
    gMenuLoaded.images[imageID].randomSeedA = rand_range(0, 0xFFFF);
    gMenuLoaded.images[imageID].randomSeedB = rand_range(0, 0xFFFF);
    gMenuLoaded.images[imageID].randomSeedC = rand_range(0, 0xFFFF);
    gMenuLoaded.images[imageID].animFrame = sMenuImageProperties[imageID].animFrame;
}

/**
 * Educated guess says this was the mass free function for menu images.
 */
UNUSED void menu_imagegroup_free(void) {
}

/**
 * Render a menu element.
 * They can be a few different things, so check what it is and then call a function to render it.
 */
void menu_element_render(s32 elementID) {
    Object *object;
    MenuAsset *asset;
    Sprite *sprite;
    ModelInstance *model;

    if (gMenuLoaded.assets[gMenuLoaded.images[elementID].trans.spriteID] != NULL) {
        if (((*gAssetsMenuElementIds)[gMenuLoaded.images[elementID].trans.spriteID] & ASSET_MASK_TEXTURE) !=
            ASSET_MASK_TEXTURE) {
            if ((*gAssetsMenuElementIds)[gMenuLoaded.images[elementID].trans.spriteID] & ASSET_MASK_OBJECT) {
                if (0) {} // Fakematch
                object = (Object *) gMenuLoaded.assets[gMenuLoaded.images[elementID].trans.spriteID];
                asset = (MenuAsset *) &gMenuLoaded.images[elementID];
                object->trans.rotation.y_rotation = asset->trans.rotation.y_rotation;
                object->trans.rotation.x_rotation = asset->trans.rotation.x_rotation;
                object->trans.rotation.z_rotation = asset->trans.rotation.z_rotation;
                object->trans.x_position = asset->trans.x_position;
                object->trans.y_position = asset->trans.y_position;
                object->trans.z_position = asset->trans.z_position;
                object->trans.scale = asset->trans.scale;
                if (gMenuDisableObjAnim == FALSE) {
                    object->animFrame = asset->animFrame;
                    object->modelIndex = asset->spriteOffset;
                }
                object->opacity = gMenuFrame.guiOpacity;
                render_object(&gMenuFrame.displayList, &gMenuFrame.hudMat, &gMenuFrame.hudVerts, object);
            } else {
                if ((*gAssetsMenuElementIds)[gMenuLoaded.images[elementID].trans.spriteID] & ASSET_MASK_SPRITE) {
                    sprite = (Sprite *) gMenuLoaded.assets[gMenuLoaded.images[elementID].trans.spriteID];
                    gDPSetPrimColor(gMenuFrame.displayList++, 0, 0, gMenuFrame.guiColourR, gMenuFrame.guiColourG,
                                    gMenuFrame.guiColourB, gMenuFrame.guiOpacity);
                    gDPSetEnvColor(gMenuFrame.displayList++, 255, 255, 255, 0);
                    render_ortho_triangle_image(&gMenuFrame.displayList, &gMenuFrame.hudMat, &gMenuFrame.hudVerts,
                                                (ObjectSegment *) (&gMenuLoaded.images[elementID]), sprite,
                                                gMenuSpriteFlags);
                    gDPSetPrimColor(gMenuFrame.displayList++, 0, 0, 255, 255, 255, 255);
                } else {
                    if (gMenuFrame.guiOpacity < 255) {
                        gDPSetPrimColor(gMenuFrame.displayList++, 0, 0, 255, 255, 255, gMenuFrame.guiOpacity);
                    } else {
                        gDPSetPrimColor(gMenuFrame.displayList++, 0, 0, 255, 255, 255, 255);
                    };
                    gDPSetEnvColor(gMenuFrame.displayList++, 255, 255, 255, 0);
                    mtx_cam_push(&gMenuFrame.displayList, &gMenuFrame.hudMat, &gMenuLoaded.images[elementID].trans,
                                 gTrackSelect.woodFrameHeightScale, 0);
                    // Elements 4-6 are the wood/iron/gold frames around the widened course-preview
                    // viewport. Element 7 is the vehicle icon frame and must keep its original aspect.
                    if (elementID >= 4 && elementID <= 6) {
                        pc_gfx_mtx_tag(gMenuFrame.hudMat - 1, &gMenuLoaded.images[elementID].trans,
                                       PC_MTX_SITE_WIDE_SURFACE);
                    }
                    model = gMenuLoaded.assets[gMenuLoaded.images[elementID].trans.spriteID];
                    render_track_selection_viewport_border(model->objModel);
                    mtx_pop(&gMenuFrame.displayList);
                    if (gMenuFrame.guiOpacity < 255) {
                        gDPSetPrimColor(gMenuFrame.displayList++, 0, 0, 255, 255, 255, 255);
                    }
                }
            }
        }
    }
}

/**
 * Sets dialogue option when the joystick is moved.
 * Will wrap around if it goes out of bounds.
 */
void handle_menu_joystick_input(void) {
    if (gMenuInput.yAxisDirection[0] < 0) {
        gDialogue.submenu = gDialogue.submenu + 1;
        sound_play(SOUND_MENU_PICK, NULL);
    } else if (gMenuInput.yAxisDirection[0] > 0) {
        gDialogue.submenu = gDialogue.submenu - 1;
        sound_play(SOUND_MENU_PICK, NULL);
    }

    if (gDialogue.submenu < 0) {
        gDialogue.submenu = gDialogue.optionMax - 1;
    }

    if (gDialogue.submenu >= gDialogue.optionMax) {
        gDialogue.submenu = 0;
    }
}
