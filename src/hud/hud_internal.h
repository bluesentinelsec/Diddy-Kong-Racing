#ifndef DKR_HUD_INTERNAL_H
#define DKR_HUD_INTERNAL_H

/*
 * Shared between the files of src/hud/: the includes they all use, the HUD state they share
 * (defined in hud.c) and the functions one file calls in another.
 */
#include "asset_loading.h"
#include "audio.h"
#include "audio_spatial.h"
#include "audiosfx.h"
#include "camera.h"
#include "common.h"
#include "f3ddkr.h"
#include "game.h"
#include "game_text.h"
#include "game_ui.h"
#include "host.h"
#include "joypad.h"
#include "macros.h"
#include "math_util.h"
#include "menu.h"
#include "object_models.h"
#include "objects.h"
#include "printf.h"
#include "racer.h"
#include "rcp_dkr.h"
#include "structs.h"
#include "textures_sprites.h"
#include "thread3_main.h"
#include "tracks.h"
#include "types.h"

// The cutscene that shows only the player's balloon count (see hud_balloons).
#define CUTSCENE_ID_BALLOON_COUNT 10

// LevelHeader hudFlags bits.
#define LEVEL_HUD_FLAG_NO_MINIMAP 1    // No minimap (challenge counters are still drawn).
#define LEVEL_HUD_FLAG_NO_PLAYER_HUD 2 // No per-player HUD at all.

extern HudAudio gHudAudioData[2];
extern u8 gShowCourseDirections;
extern u8 gHudMapMode[4][4];
extern s8 gHudToggleSettings[4];
extern u8 gHudIndicatorTimer;
extern u32 gHudColour;
extern s8 gMinimapOpacity;
extern s8 gMinimapFade;
extern s8 gMinimapXlu;
extern s8 gRaceStartShowHudStep;
extern s8 gHudPALScale;
extern u8 *gAssetHudElementStaleCounter;
extern HudData *gCurrentHud;
extern s16 *gAssetHudElementIds;
extern HudElements *gAssetHudElements;
extern Gfx *gHudDL;
extern Mtx *gHudMtx;
extern Vertex *gHudVtx;
extern s32 gHudCurrentViewport;
extern s32 gHUDNumPlayers;
extern s32 gHudController;
extern s32 gMinimapSpriteWidth;
extern s32 gMinimapSpriteHeight;
extern s32 gMinimapDotOffsetX;
extern s32 gMinimapDotOffsetY;
extern s32 gHudOffsetX;
extern s32 gHudBounceX;
extern u8 gHudRaceStart;
extern u8 gHideRaceTimer;
extern u8 gNumActivePlayers;
extern u8 gWrongWayNagPrefix;
extern AudioPoint *gRaceStartSoundMask;
extern SoundHandle gHUDVoiceSoundMask;
extern SoundHandle gHudBalloonSoundMask;
extern u16 gHudTTSoundID;
extern s32 sHudBoostVoiceTimer;
extern s32 sHudRandomVoiceTimer;
extern u8 gMinimapRed;
extern u8 gMinimapGreen;
extern u8 gMinimapBlue;
extern s32 gMinimapScreenX;
extern s32 gMinimapScreenY;
extern LevelHeader *gHudLevelHeader;
extern u8 sHudTTVoiceCount;
extern u8 sMinimapTexIndexA;
extern u8 sMinimapTexIndexB;
extern u8 gStopwatchAnimID;
extern u8 sHudMapVisible;
extern s8 gStopwatchAnimStep;
extern s32 gWrongWayNagTimer;
extern u8 gHudAudioReset;
extern u8 gHudTimeTrialGhost;
extern s32 gHudVoiceTimer;
extern s32 gHudVoicePlayerID;
extern u16 gHudVoiceID;
extern Settings *gHudSettings;
extern u8 gHudSilverCoinRace;
extern u8 gAdventurePlayerFinish;
extern u8 gMinimapOpacityTarget;
extern s32 gStopwatchErrorX;
extern s32 gStopwatchErrorY;
extern ColourCycle *gHudPulseLight;

void hud_main_race(s32 countdown, Object *obj, s32 updateRate);
void hud_course_arrows(Object_Racer *racer, s32 updateRate);
void hud_main_treasure(s32 countdown, Object *obj, s32 updateRate);
void hud_main_eggs(s32 countdown, Object *obj, s32 updateRate);
void hud_draw_eggs(Object *racerObj, s32 updateRate);
void hud_eggs_portrait(Object_Racer *racer, s32 updateRate);
void hud_main_battle(s32 countdown, Object *obj, s32 updateRate);
void hud_battle_portraits(Object *racerObj, s32 updateRate);
void hud_main_boss(s32 countdown, Object *obj, s32 updateRate);
void hud_main_taj(s32 countdown, Object *obj, s32 updateRate);
void hud_main_hub(Object *obj, s32 updateRate);
void hud_main_time_trial(s32 countdown, Object *playerRacerObj, s32 updateRate);
void hud_speedometre(Object *obj, s32 updateRate);
void hud_race_start(s32 countdown, s32 updateRate);
void hud_bananas(Object_Racer *racer, s32 updateRate);
void hud_treasure(Object_Racer *racer);
void hud_silver_coins(Object_Racer *racer, s32 updateRate);
void hud_race_finish_1player(Object_Racer *racer, s32 updateRate);
void hud_race_position(Object_Racer *racer, s32 updateRate);
void hud_lap_count(Object_Racer *racer, s32 updateRate);
void hud_wrong_way(Object_Racer *obj, s32 updateRate);
void hud_race_finish_multiplayer(Object_Racer *racer, s32 updateRate);
void hud_time_trial_message(s16 *playerID);
void hud_time_trial_finish(Object_Racer *racer, s32 updateRate);
void hud_balloons(Object_Racer *racer);
void hud_sound_play_delayed(u16 soundID, f32 delay, s32 playerIndex);
void hud_weapon(Object *obj, s32 updateRate);
void hud_magnet_reticle(Object *racerObj);
void hud_race_time(Object_Racer *racer, s32 updateRate);
void hud_timer_render(s32 x, s32 y, s32 minutes, s32 seconds, s32 hundredths, s32 smallFont);
void minimap_marker_pos(f32 x, f32 z, f32 angleSin, f32 angleCos, f32 modelAspectRatio);
void hud_element_render(Gfx **dList, Mtx **mtx, Vertex **vtxList, HudElement *hud);

#endif
