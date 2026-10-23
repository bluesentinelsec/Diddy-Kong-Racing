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
#include "PR/os_cont.h"
#include "PR/os_convert.h"
#include "PR/os_system.h"
#include "PRinternal/viint.h"
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

extern char gCourseInitials[4];
;
extern char gFLapInitials[4];
;
extern Gfx *sMenuCurrDisplayList;
extern s32 gPreviousMenuID;
extern Mtx *sMenuCurrHudMat;
extern char **gTTSaveGhostPakErrorText;
extern Vertex *sMenuCurrHudVerts;
extern CharacterVoiceChannel gMenuSelectedCharacter;
extern s32 gOptionBlinkTimer;
extern s32 gIgnorePlayerInputTime;
;
extern CharacterSelectData (*gCurrCharacterSelectData)[10];
;
extern s8 gActivePlayersArray[MAXCONTROLLERS];
;
extern s32 gOpacityDecayTimer;
extern s32 gMenuStage;
extern s8 gPlayersCharacterArray[8];
;
;
extern u64 sEepromSettings;
extern s8 gControllersXAxisDirection[4];
;
extern s8 gControllersYAxisDirection[4];
;
extern s32 sUnused_80126470;
;
extern s32 gTrackSelectViewPortHalfX;
extern s32 gTrackSelectViewPortHalfY;
extern s32 gTrackSelectViewPortX;
extern s32 gTrackSelectViewportY;
extern s32 gCpakGhostData;
extern s8 gDialogueSubmenu;
extern s8 sDialogueOptionMax;
extern Settings *gSavefileData[4];
;
extern void *gMenuAssets[128];
;
extern s32 gMenuButtons[5];
;
extern s32 buttonsPressed[5];
extern SoundHandle gMenuSoundMasks[MAXCONTROLLERS];
;
extern s16 gMenuStickX[5];
extern s16 gMenuStickY[5];
extern s32 *sMenuNullElement;
;
;
extern s16 gTrackSelectIDs[5][6];
;
extern s16 gFFLUnlocked;
extern s8 gPlayerSelectVehicle[4];
extern s32 gTrackSelectCursorY;
extern char *gAudioOutputStrings[3];
extern char *gMusicTestString;
extern char *gTrackMenuHubName;
extern s32 sControllerPakNotesFree[MAXCONTROLLERS];
;
;
;
;
;
;
extern u8 sControllerPakDataPresent[MAXCONTROLLERS];
;
extern s32 gMenuOption;
extern char *gBootPakData[MAX_CPAK_FILES];
;
;
;
extern u8 sCurrentControllerPakAllFileTypes[MAX_CPAK_FILES];
;
extern u32 sCurrentControllerPakNumberOfPages[MAX_CPAK_FILES];
;
extern u32 sCurrentControllerPakFreePages;
;
;
extern PakError sControllerPakError;
;
extern char *gResultOptionText[8];
extern s32 gResultOptionCount;
extern u16 (*gCheatsAssetData)[30];
;
extern s32 gNumberOfCheats;
extern s16 gOptionsMenuItemIndex;
extern PostRaceLoadState gPostRace;
extern s32 gFileNew;
extern s8 gInAdvModeTrophyRace;
extern f32 gTrackSelectWoodFrameHeightScale;
extern s32 gResetTitleScale;
extern s32 gMenuCurIndex;
extern s32 gTrackSpecifiedWithTrackIdToLoad;
extern s32 gMenuDelay;
extern s32 gNumberOfReadyPlayers;
extern s32 gTitleScreenLoaded;
extern s32 gIsInAdventureTwo;
extern s32 gPlayerHasSeenCautionMenu;
extern char **gMenuText;
extern u8 sMenuGuiColourR;
extern u8 sMenuGuiColourG;
extern u8 sMenuGuiColourB;
extern u8 sMenuGuiColourBlendFactor;
extern s32 gIsInTracksMode;
extern s32 gNumberOfActivePlayers;
extern s32 gIsInTwoPlayerAdventure;
extern s32 gTrackIdForPreview;
extern s32 gTrackSelectRow;
extern s32 gSaveFileIndex;
extern s32 gTrackIdToLoad;
extern MenuAsset sMenuImageProperties[18];
extern MenuAsset *gMenuImages;
extern s32 sMenuMusicVolume;
extern s32 sMenuGuiOpacity;
extern FadeTransition sMenuTransitionFadeIn;
extern FadeTransition sMenuTransitionFadeOut;
extern s32 gPostraceTextOffset;
extern s32 gPostraceTimestampOffset;
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
extern s32 gActiveMagicCodes;
extern s32 gUnlockedMagicCodes;
extern char *gMagicCodeMenuStrings[5];
extern u8 gCharacterVolumes[10][2];
extern MenuElement gCautionMenuTextElements[14];
extern MenuElement gGameSelectTextElemsNoAdv2[7];
extern MenuElement gGameSelectTextElemsWithAdv2[9];
extern char *gFilenames[3];
extern s32 gMultiplayerSelectedNumberOfRacers;
extern DrawTexture gMenuSelectionArrowUp[2];
extern DrawTexture gMenuSelectionArrowLeft[2];
extern DrawTexture gMenuSelectionArrowDown[2];
extern DrawTexture gMenuSelectionArrowRight[2];
extern DrawTexture gRaceSelectionCarTex[3];
extern DrawTexture gRaceSelectionHoverTex[3];
extern DrawTexture gRaceSelectionPlaneTex[3];
extern s16 gTracksMenuBgTextureIndices[15];
extern s16 gTTVoiceLines[53];
extern s32 gTrackNameVoiceDelay;
extern s32 gMenuOptionCap;
extern s32 gMenuSubOption;
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
extern s32 sFilenameEntryPending;
extern s32 gTrophyRaceWorldId;
extern s32 gTrophyRaceRound;
extern s32 gViewingCreditsFromCheat;

s32 postrace_render(s32 updateRate);

#endif
