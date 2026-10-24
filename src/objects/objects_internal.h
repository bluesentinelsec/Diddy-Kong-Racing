#ifndef DKR_OBJECTS_INTERNAL_H
#define DKR_OBJECTS_INTERNAL_H

/*
 * Shared between the files of src/objects/: the includes they all use, the object system
 * state they share (defined in objects.c) and the functions one file calls in another.
 */
#include "asset_enums.h"
#include "asset_loading.h"
#include "audio_spatial.h"
#include "audio_vehicle.h"
#include "audiosfx.h"
#include "behaviours.h"
#include "camera.h"
#include "fade_transition.h"
#include "game.h"
#include "game_text.h"
#include "game_ui.h"
#include "gzip.h"
#include "host.h"
#include "joypad.h"
#include "level_object_entries.h"
#include "libc/string.h"
#include "lights.h"
#include "macros.h"
#include "math_util.h"
#include "memory.h"
#include "menu.h"
#include "object_models.h"
#include "objects.h"
#include "particles.h"
#include "pc_swap.h"
#include "printf.h"
#include "racer.h"
#include "save_data.h"
#include "structs.h"
#include "textures_sprites.h"
#include "thread3_main.h"
#include "tracks.h"
#include "types.h"
#include "vehicle_misc.h"
#include "video.h"
#include "waves.h"
#include "weather.h"

// ObjectInteraction hitboxKind values (see the field in structs.h).
#define HITBOX_KIND_VERTICAL_RANGE 1 // a cylinder: yRangeMin..yRangeMax, in units of 10
#define HITBOX_KIND_MODEL 2          // collides through its model's facets (collision_objectmodel)
#define HITBOX_KIND_MODEL_SPHERES 3  // collides through its model's collision spheres
#define HITBOX_KIND_REACH_ABOVE 4    // a sphere that reaches further up: height above it counts at 0.3

#define COLLISION_PAIR_COUNT 16 // entries in gCollisionPairs

// Object_Boost effectState: the flame swells past full size, shrinks back, then holds at full size.
#define BOOST_FX_GROW 0
#define BOOST_FX_SETTLE 1
#define BOOST_FX_ACTIVE 2

#define OBJECT_MAP_SIZE 0x3000
#define MAX_CHECKPOINTS 60
#define OBJECT_POOL_SIZE (0x15800 * 3) // host objects carry 8-byte pointers
#define OBJECT_BLUEPRINT_SIZE (0x800 * 3)
#define OBJECT_SLOT_COUNT 512
#define OBJECT_COLLISION_COUNT 20
#define AINODE_COUNT 128
#define CAMCONTROL_COUNT 20
#define BOOST_VERT_COUNT 9
#define BOOST_TRI_COUNT 8
enum CountdownSkipState {
    COUNTDOWN_SKIP_PENDING, // Level just loaded; becomes ALLOWED on the first object update
    COUNTDOWN_SKIP_ALLOWED, // A/B at a fixed point of the countdown skip the intro
    COUNTDOWN_SKIP_NEVER    // First visit to the course, or not a normal race
};

extern s16 gSpawnAngleOffset;
extern Object *gGhostObjStaff;
extern s8 gRollingDemo;
extern s32 gObjectTexAnim;
extern s16 gTimeTrialTime;
extern s16 gTimeTrialVehicle;
extern s16 gTimeTrialCharacter;
extern u8 gHasGhostToSave;
extern u8 gTimeTrialStaffGhost;
extern u8 gBeatStaffGhost;
extern s8 gLeadPlayerIndex;
extern s8 gTwoActivePlayersInAdventure;
extern s8 gSwapLeadPlayer;
extern s8 gIsP2LeadPlayer;
extern Vertex *gBoostVerts[2];
extern Triangle *gBoostTris[2];
extern Object *gShieldEffectObject;
extern s32 gBoostObjOverrideID;
extern Object *gMagnetEffectObject;
extern s8 gNoBoundsCheck;
extern u32 gMagnetColours[3];
extern FadeTransition gRaceEndFade;
extern FadeTransition gRaceEndTransition;
extern s8 gNonDefaultVehicle;
extern s8 gAnimFrameParity;
extern s8 gAnimFrameState[2];
extern s8 gBossRaceIds[2];
extern s8 gAnimInitAllChannels; // Next anim_objects_spawn_targets also re-inits channel 20 targets.
extern f32 gObjectModelScaleY;
extern f32 gCurrentLightIntensity;
extern Object *gGhostObjPlayer;
extern s32 gTimeTrialContPak;
extern s8 gBossRaceMode;
extern s8 gAnimStage;
extern s8 gAnimCameraCount;
extern Object *gTransformObject;
extern s8 gTransformTimer;
extern s8 gOverworldVehicle;
extern s16 gTransformPosX;
extern s16 gTransformPosY;
extern s16 gTransformPosZ;
extern s16 gTransformAngleY;
extern s16 gRaceEndTimer;
extern s8 gRaceEndStage;
extern s8 gNumRacersSaved;
extern s8 gRaceFinishUnusedArg;
extern s8 gCutsceneSkipRequested;
extern Gfx *gDeferredDls[10];
extern s16 gDeferredDlCount;
extern f32 gObjectUpdateRateF;
extern s32 gPathUpdateOff;
extern s32 gEventCountdown;
extern s32 gRaceFinishTriggered;
extern s32 gEventStartTimer;
extern s32 gNumFinishedRacers;
extern s8 gFirstTimeFinish;
extern s8 gSetupPointVehicle;
extern u32 gBalloonCutsceneTimer;
extern f32 gObjectOffsetY;
extern s8 gAnimGroup;
extern Object *gOverridePosObjects[10];
extern s8 gOverridePosCount;
extern s8 gZoneJustLoaded;
extern s8 gIsNonCarRacers;
extern s8 gIsSilverCoinRace;
extern Object **gObjPtrList;
extern s32 gObjectCount;
extern s32 gObjectListStart;
extern Object **gCollisionObjects;
extern s32 gCollisionObjectCount;
extern Object **gAnimObjects;
extern s16 gAnimObjectCount;
extern s16 gCutsceneID;
extern s16 gFirstActiveObjectId;
extern s8 gCutsceneChanged;
extern s16 gTTGhostTimeToBeat;
extern s16 gPrevTimeTrialVehicle;
extern s16 gMapDefaultVehicle;
extern Gfx *gObjectCurrDisplayList;
extern Mtx *gObjectCurrMatrix;
extern Vertex *gObjectCurrVertexList;
extern CheckpointNode *gTrackCheckpoints;
extern s32 gNumberOfMainCheckpoints;
extern s32 gNumberOfTotalCheckpoints;
extern s16 gTajChallengeType;
extern Object *(*gCameraObjList)[CAMCONTROL_COUNT];
extern s32 gCameraObjCount;
extern Object *(*gRacers)[10];
extern Object **gRacersByPosition;
extern Object **gRacersByPort;
extern s32 gNumRacers;
extern u8 gTimeTrialEnabled;
extern u8 gIsTimeTrial;
extern s8 gIsTajChallenge;
extern s8 gTajRaceInit;
extern s8 gChallengePrevMusic;
extern s32 gChallengePrevInstruments;
extern s8 gCountdownSkipState;
extern Object *(*gAINodes)[AINODE_COUNT];
extern s32 gAINodeTail[2];
extern s32 gInitAINodes;
extern f32 gElevationHeights[5];
extern ShadeProperties gWorldShading;
extern Object_MidiFade *gCurrentMidiFade;
extern TexCoords gTexCoordScratch[32];
extern Vec3s gEnvmapPos[2];
extern CollisionPair *gCollisionPairs;
extern s32 gBoostVertCount;
extern s32 gNumOfBoostVerts;
extern s32 gBoostTriCount;
extern s32 gNumOfBoostTris;
extern s32 gBoostVertFlip;
extern u8 gShieldSineTime[16];
extern Object *gBoostEffectObjects[NUMBER_OF_CHARACTERS];
extern u8 gBoostVehicleIds[16];
extern u8 gBoostTypes[16];
extern u8 gBoostEffectFree[16];
extern RacerFXData gRacerFXData[NUMBER_OF_CHARACTERS];

void racerfx_update(s32 updateRate);
s32 normalise_time(s32 timer);
void transform_player_vehicle(void);
Object *spawn_object(LevelObjectEntryCommon *entry, s32 spawnFlags);
void free_object(Object *object);
void gParticlePtrList_flush(void);
void obj_tex_animate(Object *obj, s32 updateRate);
void obj_door_number(ObjectModel *model, Object *obj);
void ignore_bounds_check(void);
s32 move_object(Object *obj, f32 xPos, f32 yPos, f32 zPos);
void object_scroll_texture(Object *obj, s32 updateRate);
void obj_tick_anims(void);
void process_object_interactions(void);
void object_init_collision(Object *obj);
void obj_collision_transform(Object *obj);
u32 collision_pairs_tick(void);
s32 model_resolve_collisions(ObjectModel *model, s32 numPoints, s32 *numCollisions, f32 *originPointsX,
                             f32 *originPointsY, f32 *originPointsZ, f32 *targetPointsX, f32 *targetPointsY,
                             f32 *targetPointsZ, f32 *collisionRadii, s8 *surfaces, f32 scale);
void set_taj_challenge_type(s32 vehicleID);
f32 distance_between_objects(Object *a, Object *b);
void checkpoint_update_all(void);
void race_check_finish(s32 updateRate);
void race_transition_adventure(s32 updateRate);
s32 timetrial_valid_player_ghost(void);
u8 timetrial_init_staff_ghost(s32 trackId);
s32 timetrial_init_player_ghost(s32 playerID);
CheckpointNode *find_next_checkpoint_node(s32 splinePos, s32 isAlternate);
Object *get_racer_object(s32 index);
void debug_render_checkpoint_node(s32 checkpointID, s32 pathID, Gfx **dList, Mtx **mtx, Vertex **vtx);
void spectate_update(void);
void set_world_shading(f32 ambient, f32 diffuse, s16 angleX, s16 angleY, s16 angleZ);
void set_shading_properties(ShadeProperties *shading, f32 ambient, f32 diffuse, s16 angleX, s16 angleY, s16 angleZ);
void obj_shade_fancy(ObjectModel *model, Object *object, s32 cameraRelative, f32 intensity);
s32 *get_misc_asset(s32 index);
void cutscene_set(s32 cutsceneID);
void override_pos_apply(s8 collidablePass);
void override_pos_update(void);
void anim_objects_update(void);
s8 anim_prev_frame_state(void);
f32 catmull_rom_interpolation(f32 *data, s32 index, f32 x);
f32 cubic_spline_interpolation(f32 *data, s32 index, f32 x, f32 *derivative);
void mode_init_taj_race(void);
void update_object_stack_trace(s32 index, s32 value);

#endif
