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
extern s32 gAdventureSaveGhost;
extern Gfx *sMenuCurrDisplayList;
extern s32 gPreviousMenuID;
extern Mtx *sMenuCurrHudMat;
extern char **gTTSaveGhostPakErrorText;
extern Vertex *sMenuCurrHudVerts;
extern unk801263C0 gMenuSelectedCharacter;
extern unk801263C0 sSoundOptionsState;
extern s32 gOptionBlinkTimer;
extern unk801263C0 gMenuCurrentCharacter;
extern s32 gIgnorePlayerInputTime;
;
extern CharacterSelectData (*gCurrCharacterSelectData)[10];
extern s32 gTrackmenuLoadedLevel;
;
extern s8 gActivePlayersArray[MAXCONTROLLERS];
;
extern s32 gOpacityDecayTimer;
extern s8 gCharselectStatus[MAXCONTROLLERS];
extern s32 gMenuStage;
extern s8 gPlayersCharacterArray[8];
;
extern s8 gCharacterIdSlots[8];
extern s32 gRankingsPoints[8];
extern u8 gResultsPlayers[8];
extern u8 gRankingsPlayers[8];
extern u8 gResultsPlayerIDs[8];
extern u8 gRankingsPlayerIDs[8];
;
extern s8 gRankingsPortraitIDs[16];
extern u64 sEepromSettings;
extern f32 sBootScreenTimer;
extern s8 gControllersXAxisDirection[4];
;
extern MenuElement *gGameSelectElements;
extern s8 gControllersYAxisDirection[4];
;
extern s32 sUnused_80126470;
;
extern s32 gTrackSelectViewPortHalfX;
extern s32 gTrackSelectViewPortHalfY;
extern s32 gTrackSelectViewPortX;
extern s32 gTrackSelectViewportY;
extern s32 gFileCopy;
extern s32 gFileErase;
extern s32 gSaveFileIndex3;
extern s32 gSaveFileIndex2;
extern s32 gFileConfirm;
extern s32 gGhostMenuOption;
extern s32 gCpakGhostData;
extern s32 gGhostMenuTotal;
extern s8 gDialogueSubmenu;
extern u8 gGhostLevelIDsPak[6];
extern s8 sCurrentMenuID;
extern u8 gGhostCharacterIDsPak[6];
extern u8 gGhostVehicleIDsPak[6];
extern u16 gGhostChecksumIDsPak[6];
extern s8 sDialogueOptionMax;
extern u8 gGhostLevelIDsMenu[6];
extern s8 gDialogueOptionYOffset;
extern u8 gGhostCharacterIDsMenu[6];
extern s8 gDialogueItemSelection;
extern u8 gGhostVehicleIDsMenu[6];
extern u16 gGhostChecksumIDsMenu[6];
extern Settings *gSavefileData[4];
extern u8 gGhostWorldIDs[8];
extern s32 gMultiplayerSelectedNumberOfRacersCopy;
;
extern void *gMenuAssets[128];
;
extern s32 gTrackmenuType;
extern s32 gMenuButtons[5];
;
extern s8 *gCinematicParams;
extern s32 buttonsPressed[5];
extern s8 *gCinematicPortraits;
extern SoundHandle gMenuSoundMasks[MAXCONTROLLERS];
;
extern s16 gMenuStickX[5];
extern s32 gCinematicEnd;
extern s16 gMenuStickY[5];
extern s32 gCinematicSkipA;
extern SoundHandle gTrackTTSoundMask;
extern s32 gCinematicSkipB;
extern SoundHandle sMenuSoundHandle;
extern s32 gCinematicMusicChangeOff;
extern s32 *sMenuNullElement;
;
extern s32 gMenuElementScaleTimer;
extern s32 gPostraceScaleIn;
extern s32 gPostraceScaleMiddle;
extern s32 gPostraceScaleOut;
extern s32 gTitleDemoIndex;
extern s16 gTitleDemoTimer;
extern s32 gTitleRevealTimer;
extern f32 gTitleAudioCounter;
extern s8 *sTitleScreenDemoIds;
;
extern unk80126878 sIntroNameEntries[8];
extern f32 sIntroNameTimer;
extern s32 gOpeningNameID;
extern s16 gTrackSelectIDs[5][6];
;
extern s16 gFFLUnlocked;
extern s32 gTrackSelectVertsFlip;
extern TrackRenderDetails gTrackSelectRenderDetails[9];
extern s8 gPlayerSelectVehicle[4];
extern s8 gPlayerSelectConfirm[4];
extern s32 gTrackSelectCursorX;
extern s32 gTrackSelectCursorY;
extern char *gAudioOutputStrings[3];
extern f32 gTrackSelectX;
extern char *gMusicTestString;
extern f32 gTrackSelectY;
extern f32 gTrackSelectTargetX;
extern f32 gTrackSelectTargetY;
extern char *gTrackMenuHubName;
extern s32 gSelectedTrackX;
extern s32 gSelectedTrackY;
extern SoundHandle gSoundOptionMask;
extern s32 gSaveMenuOptionCountLower;
extern SaveFileData *gSaveMenuFilesDest;
extern s32 gSaveMenuOptionCountUpper;
extern SaveFileData *gSaveMenuFilesSource;
extern s32 gSaveMenuRumbleNag;
extern s32 gSaveMenuRumbleConnected;
extern s32 gSaveMenuSourceState;
extern s32 gSaveMenuDestState;
extern s32 sControllerPakNotesFree[MAXCONTROLLERS];
;
extern u8 sControllerPakIssueNotFound[MAXCONTROLLERS];
;
extern u8 sControllerPakFatalErrorFound[MAXCONTROLLERS];
;
extern u8 sControllerPakNoFreeSpace[MAXCONTROLLERS];
;
extern u8 sControllerPakBadData[MAXCONTROLLERS];
;
extern char *gMenuOptionText[8];
;
extern u8 sControllerPakDataPresent[MAXCONTROLLERS];
;
extern char *sSaveMenuFilenames;
extern s32 gMenuOption;
extern s32 gSaveMenuRumbleNagSet;
extern char **gDeviceStatusStrings;
extern s32 gSaveMenuMessageLines;
extern s32 gSaveMenuMessageOption;
extern unk80126A80 *gSaveMenuText[4];
extern s32 gPostRace1Player;
extern s32 gPostRaceTimer;
extern s32 gTracksSaveGhost;
extern char *gBootPakData[MAX_CPAK_FILES];
;
extern char *sCurrentControllerPakAllFileNames[MAX_CPAK_FILES];
;
extern char *sCurrentControllerPakAllFileExtensions[MAX_CPAK_FILES];
;
extern u8 sCurrentControllerPakAllFileTypes[MAX_CPAK_FILES];
;
extern u32 sCurrentControllerPakNumberOfPages[MAX_CPAK_FILES];
;
extern u32 sCurrentControllerPakFreePages;
;
extern s32 sControllerPakMenuNumberOfRows;
;
extern TextureHeader *gMenuMosaic1;
extern TextureHeader *gMenuMosaic2;
extern s32 gMenuMosaicShift;
extern s32 gCreditsControlDataIndex;
extern PakError sControllerPakError;
;
extern s32 sCreditsPage;
extern s32 gSaveMenuOptionSource;
extern s32 sCreditsSkipped;
extern f32 gSaveMenuScrollSource;
extern s32 sCreditsFinished;
extern s32 gSaveMenuOptionDest;
extern s32 sCreditsTextOffset;
extern f32 gSaveMenuScrollDest;
extern char *gResultOptionText[8];
extern s32 gCpakWriteTimer;
extern s32 gResultOptionCount;
extern s32 gBootMenuTimer;
extern char **gPostRaceMessage;
extern s32 gBootMenuPhase;
extern s32 gPostRaceLineCount;
extern s8 gPostraceFinishState;
extern u16 (*gCheatsAssetData)[30];
;
extern s32 gNameEntryStickHeld;
extern s32 gNumberOfCheats;
extern s32 gNameEntryStickX;
extern s16 gCheatInputCurrentRow;
extern s16 gCheatInputCurrentColumn;
extern s16 gCheatInputStringLength;
extern s16 gOptionsMenuItemIndex;
extern s32 gNameSelectionDone;
extern s16 gNewCheatID;
extern f32 gNameEntryOffsetX;
extern unk80126C54 gPostRace;
extern char gCheatInput[20];
extern s32 *gNameEntryTargetX;
extern s32 gNumOnscreenMagicCodes;
extern char *gNameEntryString;
extern s32 gNameEntryAllowedLength;
extern s16 gUnlockedCheatIDs[32];
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
extern s8 unused_800DF4D8;
extern s8 gNextTajChallengeMenu;
extern s8 gNeedToCloseDialogueBox;
extern MenuAsset sMenuImageProperties[18];
extern MenuAsset *gMenuImages;
extern s32 sMenuMusicVolume;
extern s32 sMenuGuiOpacity;
extern FadeTransition sMenuTransitionFadeInFast;
extern FadeTransition sMenuTransitionFadeIn;
extern FadeTransition sMenuTransitionFadeOut;
extern s32 gPostraceState;
extern MenuElement *gTrophyRankingsMenuElements;
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
extern DrawTexture gRaceSelectionTTOn[3];
extern DrawTexture gRaceSelectionTTOff[3];
extern DrawTexture gRaceSelectionCarOptHighlight[2];
extern DrawTexture gRaceSelectionCarOpt[2];
extern DrawTexture gRaceSelectionHoverOptHighlight[2];
extern DrawTexture gRaceSelectionHoverOpt[2];
extern DrawTexture gRaceSelectionPlaneOptHighlight[2];
extern DrawTexture gRaceSelectionPlaneOpt[2];
extern DrawTexture gRaceSelectionTTOnOptHighlight[2];
extern DrawTexture gRaceSelectionTTOffOptHighlight[2];
extern DrawTexture gRaceSelectionTTOnOpt[2];
extern DrawTexture gRaceSelectionTTOffOpt[2];
extern DrawTexture gRaceSelectionPlayer1Texture[2];
extern DrawTexture gRaceSelectionPlayer2Texture[2];
extern DrawTexture gRaceSelectionPlayer3Texture[2];
extern DrawTexture gRaceSelectionPlayer4Texture[2];
extern DrawTexture gRaceSelectionVehicleTitleTexture[2];
extern DrawTexture gRaceSelectionTTTitleTexture[2];
extern DrawTexture sTrackMenuFrameTexturesA[4];
extern DrawTexture sTrackMenuFrameTexturesB[4];
extern DrawTexture gRaceSelectionTTTexture[2];
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
extern char gCurFilenameCharBeingDrawn;
extern s32 gEnterInitalsY;
extern s32 gFilenameX;
extern s32 gFilenameY;
extern s32 gFilenameFont;
extern s32 gNameEntryLength;
extern s32 sFilenameEntryArg;
extern char gCheckAdvEnterInitials[];
extern s32 sFilenameEntryPending;
extern s32 gIndexOfCurInputCharacter;
extern s32 gRankingPlayerCount;
extern s32 gTrophyRaceWorldId;
extern s32 gTrophyRaceRound;
extern s32 gPrevTrophyRaceRound;
extern s16 gTrophyRankingsRawIconPositions[];
extern s32 gViewingCreditsFromCheat;

s32 postrace_render(s32 updateRate);

#endif
