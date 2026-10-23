#ifndef DKR_MENU_INTERNAL_H
#define DKR_MENU_INTERNAL_H

/*
 * Shared between the menu screens in src/menu/: the includes they all use, the state the
 * screens share (defined in menu.c) and the few functions one screen calls in another.
 */
#include "asset_enums.h"
#include "asset_loading.h"
#include "audio.h"
#include "audiosfx.h"
#include "behaviours.h"
#include "camera.h"
#include "common.h"
#include "f3ddkr.h"
#include "fade_transition.h"
#include "font.h"
#include "game.h"
#include "game_text.h"
#include "joypad.h"
#include "libultra/src/audio/synstartvoiceparam.h"
#include "macros.h"
#include "math_util.h"
#include "memory.h"
#include "menu.h"
#include "object_models.h"
#include "objects.h"
#include "pc_asset.h"
#include "pc_gfx.h"
#include "printf.h"
#include "racer.h"
#include "rcp_dkr.h"
#include "save_data.h"
#include "save_layout.h"
#include "structs.h"
#include "textures_sprites.h"
#include "thread30_bgload.h"
#include "thread3_main.h"
#include "tracks.h"
#include "types.h"
#include "video.h"

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

#define STICK_DEADZONE 35
#define STICK_DELAY_AMOUNT 15

;
;
extern CharacterVoiceChannel gMenuSelectedCharacter;
;
extern CharacterSelectData (*gCurrCharacterSelectData)[10];
;
;
;
;
;
;
extern s32 sUnused_80126470;
;
;
;
;
extern s32 buttonsPressed[5];
extern SoundHandle gMenuSoundMasks[MAXCONTROLLERS];
;
extern s32 *sMenuNullElement;
;
;
;
extern char *gAudioOutputStrings[3];
extern char *gMusicTestString;
;
;
;
;
;
;
;
;
;
;
;
;
;
;
;
extern u16 (*gCheatsAssetData)[30];
;
extern s16 gOptionsMenuItemIndex;
extern s32 gResetTitleScale;
extern MenuAsset sMenuImageProperties[18];
extern FadeTransition sMenuTransitionFadeIn;
extern FadeTransition sMenuTransitionFadeOut;
extern char *gTitleMenuStrings[3];
extern s16 sGameTitleTileTextures[12];
extern DrawTexture sGameTitleTileOffsets[12];
extern char *gOptionMenuStrings[];
extern unk800DFA3C gAudioMenuStrings[];
extern char *gContPakNotPresentStrings[6];
extern char *gContPakCorruptDataRepairStrings[6];
extern char *gContPakDamagedStrings[6];
extern char *gContPakFullStrings[5];
extern char *gContPakDiffContStrings[7];
extern char *gContPakNoRoomForGhostsStrings[6];
extern char *gContPakCorruptDataStrings[6];
extern char *gContPakRumbleDetectedStrings[6];
extern char *gContPakSwitchToRumbleStrings[6];
extern char *gContPakNeed2ndAdvStrings[8];
extern MenuElement gControllerPakMenuElement[7];
extern char *gMagicCodeMenuStrings[5];
extern u8 gCharacterVolumes[10][2];
extern MenuElement gCautionMenuTextElements[14];
extern MenuElement gGameSelectTextElemsNoAdv2[7];
extern MenuElement gGameSelectTextElemsWithAdv2[9];
extern char *gFilenames[3];
extern DrawTexture gMenuSelectionArrowUp[2];
extern DrawTexture gMenuSelectionArrowLeft[2];
extern DrawTexture gMenuSelectionArrowDown[2];
extern DrawTexture gMenuSelectionArrowRight[2];
extern DrawTexture gRaceSelectionCarTex[3];
extern DrawTexture gRaceSelectionHoverTex[3];
extern DrawTexture gRaceSelectionPlaneTex[3];
extern s16 gTracksMenuBgTextureIndices[15];
extern s16 gTTVoiceLines[53];
extern char *sBadControllerPakMenuText[5];
extern char *sControllerPakFullMenuText[5];
extern char *sNoControllerPakMenuText[5];
extern char *sCorruptDataMenuText[6];
extern char *sInsertControllerPakMenuText[3];
extern char *sInsertRumblePakMenuText[4];
extern DrawTexture gMenuPortraitKrunch[2];
extern DrawTexture gMenuPortraitDiddy[2];
extern DrawTexture gMenuPortraitDrumstick[2];
extern DrawTexture gMenuPortraitBumper[2];
extern DrawTexture gMenuPortraitBanjo[2];
extern DrawTexture gMenuPortraitConker[2];
extern DrawTexture gMenuPortraitTiptup[2];
extern DrawTexture gMenuPortraitTT[2];
extern DrawTexture gMenuPortraitPipsy[2];
extern DrawTexture gMenuPortraitTimber[2];
extern DrawTexture *gRacerPortraits[10];
extern char gFirstPlace[4];
extern char gSecondPlace[4];
extern char gThirdPlace[4];
extern char gFourthPlace[4];
extern char gFifthPlace[4];
extern char gSixthPlace[4];
extern char gSeventhPlace[4];
extern char gEighthPlace[4];
extern MenuElement gRaceResultsMenuElements[8];
extern MenuElement gRaceOrderMenuElements[11];
extern MenuElement gRecordTimesMenuElements[9];

/** The display list and HUD geometry the current frame draws into, and the GUI tint. */
typedef struct MenuFrame {
    Gfx *displayList;
    Mtx *hudMat;
    Vertex *hudVerts;
    u8 guiColourR;
    u8 guiColourG;
    u8 guiColourB;
    u8 guiColourBlendFactor;
    s32 guiOpacity;
} MenuFrame;
/** Controller state as the menus read it: buttons, sticks and the per-port axis direction with its repeat delay. */
typedef struct MenuInput {
    s32 buttons[5];
    s16 stickX[5];
    s16 stickY[5];
    s8 xAxisDirection[4];
    s8 yAxisDirection[4];
    s32 ignoreInputTime;
    s32 delay;
} MenuInput;
/** Where the current screen is: its stage, cursor and option indices, and the blink and fade timers. */
typedef struct MenuFlow {
    s32 stage;
    s32 previousMenuID;
    s32 curIndex;
    s32 option;
    s32 subOption;
    s32 optionCap;
    s32 blinkTimer;
    s32 opacityDecayTimer;
} MenuFlow;
/** What the current screen has loaded: its asset slots, images and text table. */
typedef struct MenuLoaded {
    void *assets[128];
    MenuAsset *images;
    char **text;
} MenuLoaded;
/** What the player has chosen on the way to a race: players, characters, vehicles, mode, track, save file and cheats.
 */
typedef struct RaceSetup {
    s8 activePlayers[MAXCONTROLLERS];
    s8 playerCharacters[8];
    s32 numberOfActivePlayers;
    s32 numberOfReadyPlayers;
    s8 playerVehicles[4];
    s32 multiplayerRacerCount;
    s32 isInTracksMode;
    s32 isInAdventureTwo;
    s32 isInTwoPlayerAdventure;
    s32 trackIdToLoad;
    s32 trackSpecified;
    s32 trophyRaceWorldId;
    s32 trophyRaceRound;
    s8 inAdvModeTrophyRace;
    s32 saveFileIndex;
    Settings *saveFiles[4];
    s32 fileNew;
    s32 numberOfCheats;
    s32 activeMagicCodes;
    s32 unlockedMagicCodes;
    s32 hasSeenCautionMenu;
    s32 titleScreenLoaded;
    // Eeeprom save data bits stored at address 0xF
    // bit 0      = Adventure Two is Unlocked
    // bit 1      = Used to set the CHEAT_CONTROL_DRUMSTICK magic code flag. Drumstick is unlocked flag?
    // bits 2-3   = Current language value
    // bits 4-23  = Used to set the CHEAT_CONTROL_TT magic code flag. Could be tracks where TT has been beat?
    // bit 24     = Unknown, but it's set as a default high bit.
    // bit 25     = Seems to be a flag for whether subtitles are enabled or not.
    // bits 26-55 = Unknown, but it could be a set of flags for unlocked tracks
    // bits 56-63 = Checksum bits. These are calculated from the previous bits,
    //              and validated using calculate_eeprom_settings_checksum.
    //              If the checksum fails, the settings are reset to just
    //              bits 24/25 being high.
    u64 eepromSettings;
    s32 cpakGhostData;
    s32 viewingCreditsFromCheat;
} RaceSetup;
/** State the track select screen shares with the results screen and the drawing helpers. */
typedef struct TrackSelectState {
    s32 viewportX;
    s32 viewportY;
    s32 viewportHalfX;
    s32 viewportHalfY;
    f32 woodFrameHeightScale;
    s32 row;
    s32 cursorY;
    s16 trackIds[5][6];
    s16 fflUnlocked;
    char *hubName;
    s32 previewTrackId;
    s32 musicVolume;
    s32 nameVoiceDelay;
} TrackSelectState;
/** What the boot and save screens learned about the controller paks. */
typedef struct ControllerPakState {
    s32 notesFree[MAXCONTROLLERS];
    u8 dataPresent[MAXCONTROLLERS];
    char *fileText[MAX_CPAK_FILES];
    u8 fileTypes[MAX_CPAK_FILES];
    u32 filePages[MAX_CPAK_FILES];
    u32 freePages;
    PakError error;
} ControllerPakState;
/** Post-race results: the option list, the initials being entered and the asset load step. */
typedef struct ResultsState {
    char *optionText[8];
    s32 optionCount;
    char courseInitials[4];
    char flapInitials[4];
    s32 filenameEntryPending;
    s32 textOffset;
    s32 timestampOffset;
    PostRaceLoadState load;
    char **ghostPakErrorText;
} ResultsState;
/** The NPC dialogue submenu and its option count. */
typedef struct DialogueState {
    s8 submenu;
    s8 optionMax;
} DialogueState;

extern MenuFrame gMenuFrame;
extern MenuInput gMenuInput;
extern MenuFlow gMenuFlow;
extern MenuLoaded gMenuLoaded;
extern RaceSetup gRaceSetup;
extern TrackSelectState gTrackSelect;
extern ControllerPakState gControllerPak;
extern ResultsState gResults;
extern DialogueState gDialogue;

s32 postrace_render(s32 updateRate);

#endif
