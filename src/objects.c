#include "objects.h"
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
#include "lights.h"
#include "macros.h"
#include "math_util.h"
#include "memory.h"
#include "menu.h"
#include "object_models.h"
#include "particles.h"
#include "pc_swap.h"
#include "PR/os_cont.h"
#include "PR/os_convert.h"
#include "PR/os_system.h"
#include "PR/rcp.h"
#include "PRinternal/piint.h"
#include "PRinternal/viint.h"
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

#ifndef _ALIGN16
#define _ALIGN16(a) (((uintptr_t) (a) & ~0xF) + 0x10)
#endif

#define SET_SHIFT_AND_MASK(varShift, varMask, x) \
    varShift = x;                                \
    varMask = 0xFFFF >> x;

/************ .data ************/

FadeTransition gTajChallengeTransition = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_OUT, FADE_COLOR_BLACK, 30, 15);
FadeTransition gBalloonCutsceneTransition = FADE_TRANSITION(FADE_CIRCLE, FADE_FLAG_NONE, FADE_COLOR_BLACK, 30, 15);

s32 gReadoutLogIndex = 0;
s16 gSpawnAngleOffset = 0;
Object *gGhostObjStaff = NULL;
s8 gRollingDemo = FALSE;
s32 gObjectTexAnim = FALSE;
s16 gTimeTrialTime = 10800;
s16 gTimeTrialVehicle = -1;
s16 gTimeTrialCharacter = 0;
u8 gHasGhostToSave = FALSE;
u8 gTimeTrialStaffGhost = FALSE;
u8 gBeatStaffGhost = FALSE;
s8 gLeadPlayerIndex = 0;
s8 gTwoActivePlayersInAdventure = FALSE; // Has basically the same purpose as gTwoPlayerAdvRace
s8 gSwapLeadPlayer = FALSE;
s8 gIsP2LeadPlayer = FALSE;
Vertex *gBoostVerts[2] = { 0, 0 };
Triangle *gBoostTris[2] = { 0, 0 };
Object *gShieldEffectObject = NULL;
s32 gBoostObjOverrideID = 9;
Object *gMagnetEffectObject = NULL;

f32 gBoostFanDirections[16] = { 0.0f, 1.0f,  0.70711f,  0.70711f,  1.0f,  0.0f, 0.70711f,  -0.70711f,
                       0.0f, -1.0f, -0.70711f, -0.70711f, -1.0f, 0.0f, -0.70711f, 0.70711f };

u16 gRacerObjectTable[] = {
    // Car
    ASSET_OBJECT_ID_KREMCAR,
    ASSET_OBJECT_ID_BADGERCAR,
    ASSET_OBJECT_ID_TORTCAR,
    ASSET_OBJECT_ID_CONKACAR,
    ASSET_OBJECT_ID_TIGERCAR,
    ASSET_OBJECT_ID_BANJOCAR,
    ASSET_OBJECT_ID_CHICKENCAR,
    ASSET_OBJECT_ID_MOUSECAR,
    ASSET_OBJECT_ID_SWCAR,
    ASSET_OBJECT_ID_DIDDYCAR,
    // Hover
    ASSET_OBJECT_ID_KREMLINHOVER,
    ASSET_OBJECT_ID_BADGERHOVER,
    ASSET_OBJECT_ID_TORTHOVER,
    ASSET_OBJECT_ID_CONKAHOVER,
    ASSET_OBJECT_ID_TIGERHOVER,
    ASSET_OBJECT_ID_BANJOHOVER,
    ASSET_OBJECT_ID_CHICKENHOVER,
    ASSET_OBJECT_ID_MOUSEHOVER,
    ASSET_OBJECT_ID_TICKTOCKHOVER,
    ASSET_OBJECT_ID_DIDDYHOVER,
    // Plane
    ASSET_OBJECT_ID_KREMPLANE,
    ASSET_OBJECT_ID_BADGERPLANE,
    ASSET_OBJECT_ID_TORTPLANE,
    ASSET_OBJECT_ID_CONKA,
    ASSET_OBJECT_ID_TIGPLANE,
    ASSET_OBJECT_ID_BANJOPLANE,
    ASSET_OBJECT_ID_CHICKENPLANE,
    ASSET_OBJECT_ID_MOUSEPLANE,
    ASSET_OBJECT_ID_TICKTOCKPLANE,
    ASSET_OBJECT_ID_DIDDYPLANE,
    // Car
    ASSET_OBJECT_ID_KREMCAR,
    ASSET_OBJECT_ID_BADGERCAR,
    ASSET_OBJECT_ID_TORTCAR,
    ASSET_OBJECT_ID_CONKACAR,
    ASSET_OBJECT_ID_TIGERCAR,
    ASSET_OBJECT_ID_BANJOCAR,
    ASSET_OBJECT_ID_CHICKENCAR,
    ASSET_OBJECT_ID_MOUSECAR,
    ASSET_OBJECT_ID_SWCAR,
    ASSET_OBJECT_ID_DIDDYCAR,
    // Car again?
    ASSET_OBJECT_ID_KREMCAR,
    ASSET_OBJECT_ID_BADGERCAR,
    ASSET_OBJECT_ID_TORTCAR,
    ASSET_OBJECT_ID_CONKACAR,
    ASSET_OBJECT_ID_TIGERCAR,
    ASSET_OBJECT_ID_BANJOCAR,
    ASSET_OBJECT_ID_CHICKENCAR,
    ASSET_OBJECT_ID_MOUSECAR,
    ASSET_OBJECT_ID_SWCAR,
    ASSET_OBJECT_ID_DIDDYCAR,
    // Special
    ASSET_OBJECT_ID_TRICKYTOPS,
    ASSET_OBJECT_ID_WALRUS,
    ASSET_OBJECT_ID_DRAGONBOSS,
    ASSET_OBJECT_ID_TERRYBOSS,
    ASSET_OBJECT_ID_SNOWBALLBOSS,
    ASSET_OBJECT_ID_FLYINGCARPET,
    ASSET_OBJECT_ID_OCTOPUS,
    ASSET_OBJECT_ID_WIZPIG,
    ASSET_OBJECT_ID_WIZPIGROCKET,
};

// A table of which vehicles to use for boss races.
// https://www.youtube.com/watch?v=WQJAtns_rMk
BossRaceVehicles gBossVehicles[] = {
    { VEHICLE_CAR, VEHICLE_TRICKY },         // Tricky 1
    { VEHICLE_HOVERCRAFT, VEHICLE_BLUEY },   // Bluey 1
    { VEHICLE_PLANE, VEHICLE_SMOKEY },       // Smokey 1
    { VEHICLE_CAR, VEHICLE_TRICKY },         // Tricky 2
    { VEHICLE_HOVERCRAFT, VEHICLE_BLUEY },   // Bluey 2
    { VEHICLE_HOVERCRAFT, VEHICLE_BUBBLER }, // Bubbler 1
    { VEHICLE_HOVERCRAFT, VEHICLE_BUBBLER }, // Bubbler 2
    { VEHICLE_PLANE, VEHICLE_SMOKEY },       // Smokey 2
    { VEHICLE_CAR, VEHICLE_WIZPIG },         // Wizpig 1
    { VEHICLE_PLANE, VEHICLE_ROCKET },       // Wizpig 2
};


s8 gDefaultCharacterOrder[8] = { 9, 1, 2, 3, 4, 5, 7, 0 };

s8 gNoBoundsCheck = FALSE;
u32 gMagnetColours[3] = {
    COLOUR_RGBA32(255, 64, 16, 0), // Level 1
    COLOUR_RGBA32(16, 64, 255, 0), // Level 2
    COLOUR_RGBA32(16, 255, 64, 0), // Level 3
};
FadeTransition gRaceEndFade = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_NONE, FADE_COLOR_BLACK, 40, FADE_STAY);
FadeTransition gRaceEndTransition = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_OUT, FADE_COLOR_BLACK, 40, 0);

/*******************************/

/************ .rodata ************/

UNUSED const char sObjectOutofMemString[] = "Objects out of ram(1) !!\n";
UNUSED const char sDoorNumberErrorString[] = "Door numbering error %d!!\n";
UNUSED const char sObjectScopeErrorString[] = "objGetScope: Unknown scope for object %d\n";

/*********************************/

/************ .bss ************/

s16 gReadoutLog[128];
s8 gNonDefaultVehicle;
s8 gAnimFrameParity;
s8 gAnimFrameState[2];
s8 gBossRaceIds[2];
s8 D_8011AD26;
f32 gObjectModelScaleY;
f32 gCurrentLightIntensity;
Object *gGhostObjPlayer;
s32 gTimeTrialContPak; // gTimeTrialContPak is ultimately set by ghost_load, and is almost definitely SIDeviceStatus
s8 gBossRaceMode;
s8 gAnimStage;
s8 gAnimCameraCount;
Object *gTransformObject; // Set but never read.
s8 gTransformTimer;
s8 gOverworldVehicle;
s16 gTransformPosX;
s16 gTransformPosY;
s16 gTransformPosZ;
s16 gTransformAngleY;
s16 gRaceEndTimer;
s8 gRaceEndStage;
s8 gNumRacersSaved;
UNUSED s8 unused_D_8011AD52;
s8 gCutsceneSkipRequested;
s32 *gSpawnObjectHeap;
s32 *gAssetsObjectHeadersTable;
s32 gAssetsObjectHeadersTableLength;
s32 *gAssetsMiscSection; // Official Name: Ftables
static Object_Boost sPcBoostTable[NUMBER_OF_CHARACTERS]; // ASSET_MISC_20 unpacked for the host
s32 *gAssetsMiscTable;   // Official Name: Findex
Gfx *gDeferredDls[10];
s32 gAssetsMiscTableLength; // Official Name: Fmax
s16 gDeferredDlCount;
f32 gObjectUpdateRateF;
s32 gPathUpdateOff;
s32 gEventCountdown;
s32 gRaceFinishTriggered;
s32 gEventStartTimer;
s32 gObjectsElapsedTime;
s32 gNumFinishedRacers;
s8 gFirstTimeFinish;
s8 gSetupPointVehicle;
u32 gBalloonCutsceneTimer;
s8 (*gDrawbridgeTimers)[8];
f32 gObjectOffsetY;
s8 gAnimGroup;
s8 gOverrideDoors;
Object *gOverridePosObjects[10]; // Array of OverRidePos objects
s8 gOverridePosCount;          // Number of OverRidePos objects in gOverridePosObjects
s8 gZoneJustLoaded;          // A boolean? I've seen it either as 0 or 1
s8 gIsNonCarRacers;
s8 gIsSilverCoinRace;
Object *gIdObjects[16];
ObjectHeader *(*gLoadedObjectHeaders)[ASSET_OBJECTS_COUNT];
u8 (*gObjectHeaderReferences)[ASSET_OBJECTS_COUNT];
TextureHeader *gLastObjectTextureA;
TextureHeader *gLastObjectTextureB;
Object **gObjPtrList; // Not sure about the number of elements
s32 gObjectCount;
s32 gObjectListStart;
s32 gParticleCount;
Object *gObjectMemoryPool;
Object **gCollisionObjects;
s32 gCollisionObjectCount;
Object **gAnimObjects; // Pointer to an array of Animation objects
s16 gAnimObjectCount;      // Number of Animation objects in gAnimObjects
s16 gCutsceneID;
s16 gFirstActiveObjectId;
s8 gCutsceneChanged;
s16 gTTGhostTimeToBeat;
s16 gPrevTimeTrialVehicle; // Current Vehicle being used in track?
s16 gMapDefaultVehicle;    // Vehicle enum
Gfx *gObjectCurrDisplayList;
Mtx *gObjectCurrMatrix;
Vertex *gObjectCurrVertexList;
u8 *gObjectMapSpawnList[2];
s32 gObjectMapSize[2];
s32 gObjectMapID[2];
s32 *gObjectMap[2];
s16 *gAssetsLvlObjTranslationTable;
s32 gAssetsLvlObjTranslationTableLength;
s32 gObjectMapIndex;
Object **gParticlePtrList;
s32 gFreeListCount;
CheckpointNode *gTrackCheckpoints; // Array of structs, unknown number of members
s32 gNumberOfMainCheckpoints;
s32 gNumberOfTotalCheckpoints;
s16 gTajChallengeType;
Object *(*gCameraObjList)[CAMCONTROL_COUNT]; // Camera objects with a maximum of 20
s32 gCameraObjCount;                         // The number of camera objects in the above list
Object *(*gRacers)[10];                      // Official Name: playerlist
// Similar to gRacers, but sorts the pointer by the players' current position in the race.
Object **gRacersByPosition;
// Similar to gRacers, but sorts the pointer by controller ports 1-4, then CPUs.
Object **gRacersByPort;
s32 gNumRacers;
u8 gTimeTrialEnabled;
u8 gIsTimeTrial;
s8 gIsTajChallenge;
s8 gTajRaceInit;
s8 gChallengePrevMusic;
s32 gChallengePrevInstruments;
enum CountdownSkipState {
    COUNTDOWN_SKIP_PENDING, // Level just loaded; becomes ALLOWED on the first object update
    COUNTDOWN_SKIP_ALLOWED, // A/B at a fixed point of the countdown skip the intro
    COUNTDOWN_SKIP_NEVER    // First visit to the course, or not a normal race
};
s8 gCountdownSkipState;
Object *(*gAINodes)[AINODE_COUNT];
s32 gAINodeTail[2];
s32 gInitAINodes;
f32 gElevationHeights[5];
// Effectively unused. Retail declared 4 bytes (a pointer) but
// set_world_shading fills it as a whole ShadeProperties, overrunning into the
// neighbouring globals — harmless only under retail's layout. Real storage now.
ShadeProperties gWorldShading;
Object_MidiFade *gCurrentMidiFade;
TexCoords gTexCoordScratch[32];
Vec3s gEnvmapPos[2];
CollisionPair *gCollisionPairs;
s32 gBoostVertCount;
s32 gNumOfBoostVerts;
s32 gBoostTriCount;
s32 gNumOfBoostTris;
s32 gBoostVertFlip; // indexes gBoostVerts and gBoostTris
u8 gShieldSineTime[16];
Object *gBoostEffectObjects[NUMBER_OF_CHARACTERS];
u8 gBoostVehicleIds[16]; // Vehicle IDs for the boost objects, used to determine which racer is using which boost object.
u8 gBoostTypes[16];
u8 gBoostEffectFree[16];
RacerFXData gRacerFXData[NUMBER_OF_CHARACTERS];

extern s16 gGhostMapID;

/******************************/

/**
 * Spawns control objects for racer boost visuals, as well as shield and magnet visuals.
 * Boost geometry is made in real time, and allocated here.
 * This function is called on every level load, but only racers use the stuff here.
 */
void racerfx_alloc(s32 numberOfVertices, s32 numberOfTriangles) {
    Object_Boost *boostObj;
    LevelObjectEntry_Boost2 objEntry;
    s32 i;

    gBoostTris[0] = (Triangle *) mempool_alloc_safe(
        ((numberOfTriangles * sizeof(Triangle)) + (numberOfVertices * sizeof(Vertex))) * 2, COLOUR_TAG_BLUE);
    gBoostTris[1] = (Triangle *) ((uintptr_t) gBoostTris[0] + numberOfTriangles * sizeof(Triangle));
    gBoostVerts[0] = (Vertex *) ((uintptr_t) gBoostTris[1] + numberOfTriangles * sizeof(Triangle));
    gBoostVerts[1] = (Vertex *) ((uintptr_t) gBoostVerts[0] + numberOfVertices * sizeof(Vertex));
    gBoostVertCount = numberOfVertices;
    gNumOfBoostVerts = 0;
    gBoostTriCount = numberOfTriangles;
    gNumOfBoostTris = 0;
    gBoostVertFlip = 0;
    boostObj = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    // Makes 10 boost objects, but only 8 racers can actually exist at once.
    for (i = 0; i < NUMBER_OF_CHARACTERS; i++) {
        objEntry.common.objectID = ASSET_OBJECT_ID_BOOST;
        objEntry.common.size = sizeof(LevelObjectEntry_Boost2);
        objEntry.common.x = 0;
        objEntry.common.y = 0;
        objEntry.common.z = 0;
        objEntry.racerIndex = i;
        gBoostEffectObjects[i] = spawn_object(&objEntry.common, OBJECT_SPAWN_UNK01);
        if (gBoostEffectObjects[i] != NULL) {
            gBoostEffectObjects[i]->properties.common.unk0 = 0;
            gBoostEffectObjects[i]->properties.common.unk4 = 0;
            boostObj[i].effectState = 0;
            boostObj[i].effectScale = 0.0f;
            boostObj[i].sprite = tex_load_sprite(boostObj[i].spriteId, 0);
            boostObj[i].tex = load_texture(boostObj[i].textureId);
            boostObj[i].pulseTimer = rand_range(0, 255);
            boostObj[i].lingerTimer = 0;
            // This is for shields, not boosts.
            gShieldSineTime[i] = rand_range(0, 255);
        }
        gBoostEffectFree[i] = TRUE;
    }
    gBoostObjOverrideID = 9;
    objEntry.common.objectID = ASSET_OBJECT_ID_SHIELD;
    objEntry.common.size = sizeof(LevelObjectEntry_Boost2);
    objEntry.common.x = 0;
    objEntry.common.y = 0;
    objEntry.common.z = 0;
    gShieldEffectObject = spawn_object(&objEntry.common, OBJECT_SPAWN_NONE);
    for (i = 0; i < NUMBER_OF_CHARACTERS; i++) {
        gRacerFXData[i].unused0 = 0;
        gRacerFXData[i].magnetPulseTimer = rand_range(0, 255);
        gRacerFXData[i].magnetSpinTimer = rand_range(0, 255);
        gRacerFXData[i].magnetFade = 0;
    }
    objEntry.common.objectID = ASSET_OBJECT_ID_AINODE;
    objEntry.common.size = sizeof(LevelObjectEntry_Boost2) + 0x80; // Not sure where this 0x80 comes from.
    objEntry.common.x = 0;
    objEntry.common.y = 0;
    objEntry.common.z = 0;
    gMagnetEffectObject = spawn_object(&objEntry.common, OBJECT_SPAWN_NONE);
}

/**
 * Attempts to free the boost, shield and magnet objects and assets.
 */
void racerfx_free(void) {
    Sprite *sprite;
    TextureHeader *texture;
    Object_Boost *objBoost;
    u32 i;

    if (gBoostTris[0]) {
        mempool_free(gBoostTris[0]);
        gBoostTris[0] = NULL;
        gBoostTris[1] = NULL;
        gBoostVerts[0] = NULL;
        gBoostVerts[1] = NULL;
    }
    objBoost = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    for (i = 0; i < NUMBER_OF_CHARACTERS; i++) {
        sprite = objBoost[i].sprite;
        if (sprite != NULL) {
            sprite_free(sprite);
            objBoost[i].sprite = NULL;
        }
        texture = objBoost[i].tex;
        if (texture != NULL) {
            tex_free(texture);
            objBoost[i].tex = NULL;
        }
    }
    if (gShieldEffectObject != NULL) {
        free_object(gShieldEffectObject);
    }
    gShieldEffectObject = NULL;

    if (gMagnetEffectObject != NULL) {
        free_object(gMagnetEffectObject);
    }
    gMagnetEffectObject = NULL;
    gParticlePtrList_flush();
}

/** Builds the fan of vertices and triangles for one boost flame, facing the racer's rotation. */
void boost_build_fan_mesh(Vertex *vertices, Triangle *triangles, ObjectTransform *trans, f32 arg3, f32 arg4, s16 arg5,
                   TextureHeader *tex) {
    s32 sp80[8];
    s32 i;
    s32 height, width;
    s16 *v;
    Vec3f sp64;
    s32 *tri;
    f32 *ptr;
    s32 temp;

    v = (s16 *) vertices;

    sp64.z = -arg4;
    vec3f_rotate_py(&trans->rotation, &sp64);

    // A rather strange way to fill structures

    *v++ = sp64.f[0] + trans->x_position;
    *v++ = sp64.f[1] + trans->y_position;
    *v++ = sp64.f[2] + trans->z_position;
    *v++ = -1;
    *v++ = -1;

    ptr = gBoostFanDirections;
    for (i = 0; i < 8; i++) {
        sp64.x = *ptr++ * arg3;
        sp64.y = *ptr++ * arg3;
        sp64.z = 0.0f;

        vec3f_rotate(&trans->rotation, &sp64);

        *v++ = sp64.f[0] + trans->x_position;
        *v++ = sp64.f[1] + trans->y_position;
        *v++ = sp64.f[2] + trans->z_position;
        *v++ = -1;
        *v++ = -1;
    }

    width = (tex->width - 1) << 4;
    height = (tex->height - 1) << 4;

    for (i = 0; i < 8; i++) {
        sp80[i] = width + ((sins_s16(arg5) * width) >> 16);
        sp80[i] |= ((height << 16) + height * coss_s16(arg5)) & 0xFFFF0000;
        arg5 += 0x2000;
    }

    tri = (s32 *) triangles;
    for (i = 0; i < 8; i++) {
        *tri++ = DKR_TRIANGLE(BACKFACE_CULL, 0, i + 1, ((i + 1) & 7) + 1);
        *tri++ = ((width & 0xFFFF) << 16) | (height & 0xFFFF);
        *tri++ = sp80[i];
        *tri++ = sp80[(i + 1) & 7];
    }
}

/** Starts or restyles a racer's boost effect object for its vehicle and boost type. */
void racer_set_boost_effect(Object *racerObj, s32 racerIndex, s32 vehicleIDPrev, s32 boostType, s32 arg4) {
    Vec3f sp74;
    f32 temp_f0;
    f32 var_f2;
    Object_Boost *boostAsset;
    ObjectTransform objTrans;
    Object_Boost *objBoostRacer;
    Object_Boost *objBoostType;
    Object_Boost_Inner *boostData;

    if (racerIndex == -1) {
        racerIndex = gBoostObjOverrideID;
        gBoostObjOverrideID--;
    }
    if (gBoostObjOverrideID < 0) {
        gBoostObjOverrideID = 0;
    }
    if (racerIndex >= 0 && racerIndex < NUMBER_OF_CHARACTERS) {
        boostAsset = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
        objBoostType = &boostAsset[boostType];
        objBoostRacer = &boostAsset[racerIndex];
        if (gBoostEffectObjects[racerIndex] != NULL) {
            switch (vehicleIDPrev) {
                default:
                    boostData = NULL;
                    break;
                case VEHICLE_CAR:
                    boostData = &objBoostRacer->carBoostData;
                    break;
                case VEHICLE_HOVERCRAFT:
                    boostData = &objBoostRacer->hovercraftBoostData;
                    break;
                case VEHICLE_PLANE:
                    boostData = &objBoostRacer->flyingBoostData;
                    break;
                case VEHICLE_ROCKET:
                    boostData = &objBoostRacer->flyingBoostData;
                    break;
            }
            if (boostData != NULL) {
                gBoostVehicleIds[racerIndex] = vehicleIDPrev;
                gBoostTypes[racerIndex] = boostType;
                if (objBoostRacer->effectState == 2) {
                    temp_f0 = coss_f(objBoostRacer->pulseTimer << 12);
                    var_f2 = (boostData->fanRadiusBase + (temp_f0 * boostData->fanRadiusPulse)) * objBoostRacer->effectScale;
                    temp_f0 = (boostData->fanLengthBase + (temp_f0 * boostData->fanLengthPulse)) * objBoostRacer->effectScale;
                    if ((boostType & 3) == BOOST_MEDIUM) {
                        var_f2 *= 1.09f;
                        temp_f0 *= 1.09f;
                    }
                    if ((boostType & 3) >= BOOST_LARGE) {
                        var_f2 *= 1.18f;
                        temp_f0 *= 1.18f;
                    }
                    objTrans.x_position = boostData->position.x;
                    objTrans.y_position = boostData->position.y;
                    objTrans.z_position = boostData->position.z;
                    objTrans.scale = 1.0f;
                    objTrans.rotation.y_rotation = -0x8000;
                    objTrans.rotation.x_rotation = 0;
                    objTrans.rotation.z_rotation = 0;
                    boost_build_fan_mesh(&gBoostVerts[gBoostVertFlip][gNumOfBoostVerts],
                                  &gBoostTris[gBoostVertFlip][gNumOfBoostTris], &objTrans, var_f2, temp_f0,
                                  objBoostRacer->pulseTimer << 12, objBoostType->tex);
                    gBoostEffectObjects[racerIndex]->properties.boost.indexes =
                        (racerIndex << 28) | (gNumOfBoostVerts << 14) | gNumOfBoostTris;
                    gNumOfBoostVerts += BOOST_VERT_COUNT;
                    gNumOfBoostTris += BOOST_TRI_COUNT;
                }
                gBoostEffectObjects[racerIndex]->properties.boost.obj = racerObj;
                gBoostEffectObjects[racerIndex]->trans.x_position = 0.0f;
                gBoostEffectObjects[racerIndex]->trans.y_position = 0.0f;
                gBoostEffectObjects[racerIndex]->trans.z_position = 0.0f;
                sp74.x = boostData->position.x;
                sp74.y = boostData->position.y;
                sp74.z = boostData->position.z;
                vec3f_rotate(&racerObj->trans.rotation, &sp74);
                ignore_bounds_check();
                move_object(gBoostEffectObjects[racerIndex], racerObj->trans.x_position + sp74.f[0],
                            racerObj->trans.y_position + sp74.f[1], racerObj->trans.z_position + sp74.f[2]);
            }
            if (arg4 != FALSE) {
                gBoostEffectFree[racerIndex] = FALSE;
            }
        }
    }
}

/**
 * Updates the racer FX object states.
 * This includes the shield wobble timer, and the texture frames for the magnet and shield.
 */
void racerfx_update(s32 updateRate) {
    s32 i;
    Object_Boost *boostObj;
    s32 temp;
    Object_Boost *asset20;
    f32 updateRateF;
    Object_Racer *racer;

    gBoostVertFlip = 1 - gBoostVertFlip;
    gNumOfBoostVerts = 0;
    gNumOfBoostTris = 0;
    asset20 = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    gBoostObjOverrideID = 9;
    for (i = 0; i < NUMBER_OF_CHARACTERS; i++) {
        if (gBoostEffectFree[i] && gBoostEffectObjects[i] != NULL) {
            gBoostEffectObjects[i]->properties.common.unk0 = 0;
        }
        gBoostEffectFree[i] = TRUE;
    }
    for (i = 0; i < gNumRacers; i++) {
        updateRateF = (f32) updateRate;
        if (osTvType == OS_TV_TYPE_PAL) {
            updateRateF *= 1.2f;
        }
        racer = (*gRacers)[i]->racer;
        boostObj = &asset20[racer->racerIndex];
        if (racer->shieldTimer != 0) {
            gShieldSineTime[racer->racerIndex] += updateRate;
        }
        boostObj->pulseTimer += updateRate;
        if (racer->boostTimer != 0) {
            boostObj->lingerTimer = 20;
            if (boostObj->effectState == 0) {
                boostObj->effectScale += updateRateF * 0.25f;
                updateRateF = 0.0f;
                if (boostObj->effectScale > 2.4f) {
                    boostObj->effectScale = 4.8f - boostObj->effectScale;
                    boostObj->effectState = 1;
                }
            }
            if (boostObj->effectState == 1) {
                boostObj->effectScale -= updateRateF * 0.25f;
                updateRateF = 0.0f;
                if (boostObj->effectScale < 1.0f) {
                    boostObj->effectState = 2;
                    boostObj->effectScale = 1.0f - boostObj->effectScale;
                }
            }
            if (boostObj->effectState == 2) {
                if (boostObj->effectScale < 1.0f) {
                    boostObj->effectScale += updateRateF * 0.125f;
                    if (boostObj->effectScale > 1.0f) {
                        boostObj->effectScale = 1.0f;
                    }
                }
            }
        } else {
            if (boostObj->lingerTimer > 0) {
                boostObj->lingerTimer -= updateRate;
            } else {
                if (boostObj->effectState == 2) {
                    boostObj->effectScale -= updateRateF * 0.05f;
                    updateRateF = 0.0f;
                    if (boostObj->effectScale < 0.0f) {
                        boostObj->effectState = 0;
                        boostObj->effectScale += 1.0f;
                    }
                }
                if (boostObj->effectState < 2) {
                    boostObj->effectScale -= (updateRateF * 0.1f);
                    if (boostObj->effectScale < 0.0f) {
                        boostObj->effectScale = 0.0f;
                    }
                    boostObj->effectState = 0;
                }
            }
        }
        if ((boostObj->effectState > 0) || (boostObj->effectScale > 0.0f)) {
            racer_set_boost_effect((*gRacers)[i], racer->racerIndex, racer->vehicleIDPrev, racer->boostType, 0);
        }
        temp = racer->racerIndex;
        gRacerFXData[temp].magnetPulseTimer += updateRate;
        gRacerFXData[temp].magnetSpinTimer += updateRate;
        if (racer->magnetTimer != 0) {
            if (gRacerFXData[temp].magnetFade + (updateRate << 2) < 32) {
                gRacerFXData[temp].magnetFade += (updateRate << 2);
            } else {
                gRacerFXData[temp].magnetFade = 32;
            }
        } else {
            if (gRacerFXData[temp].magnetFade - updateRate > 0) {
                gRacerFXData[temp].magnetFade -= updateRate;
            } else {
                gRacerFXData[temp].magnetFade = 0;
            }
        }
    }
    if (gMagnetEffectObject != NULL) {
        obj_tex_animate(gMagnetEffectObject, updateRate);
    }
}

/**
 * Returns the boost object with the given ID.
 * Returns a specific ID if the arg passed is BOOST_DEFAULT.
 */
Object *racerfx_get_boost(s32 boostID) {
    if (boostID == BOOST_DEFAULT) {
        boostID = gBoostObjOverrideID;
    }
    if (boostID < 0 || boostID >= NUMBER_OF_CHARACTERS) {
        return NULL;
    }
    return gBoostEffectObjects[boostID];
}

/**
 * Allocate memory for objects and object related systems.
 * This includes the objects themselves, particles, and all of the pointer lists for tracking objects.
 */
void allocate_object_pools(void) {
    s32 i;

    set_world_shading(0.67f, 0.33f, 0, -0x2000, 0);
    gObjectMemoryPool = (Object *) mempool_new_sub(OBJECT_POOL_SIZE, OBJECT_SLOT_COUNT);
    gParticlePtrList = mempool_alloc_safe(sizeof(uintptr_t) * 200, COLOUR_TAG_BLUE);
    gCollisionObjects = mempool_alloc_safe(sizeof(uintptr_t) * OBJECT_COLLISION_COUNT, COLOUR_TAG_BLUE);
    gAnimObjects = mempool_alloc_safe(sizeof(uintptr_t) * 128, COLOUR_TAG_BLUE);
    gTrackCheckpoints = mempool_alloc_safe(sizeof(CheckpointNode) * MAX_CHECKPOINTS, COLOUR_TAG_BLUE);
    gCameraObjList = mempool_alloc_safe(sizeof(uintptr_t *) * CAMCONTROL_COUNT, COLOUR_TAG_BLUE);
    gRacers = mempool_alloc_safe(sizeof(uintptr_t) * 10, COLOUR_TAG_BLUE);
    gRacersByPort = mempool_alloc_safe(sizeof(uintptr_t) * 10, COLOUR_TAG_BLUE);
    gRacersByPosition = mempool_alloc_safe(sizeof(uintptr_t) * 10, COLOUR_TAG_BLUE);
    gAINodes = mempool_alloc_safe(sizeof(uintptr_t) * AINODE_COUNT, COLOUR_TAG_BLUE);
    gDrawbridgeTimers = mempool_alloc_safe(8, COLOUR_TAG_BLUE);
    gCollisionPairs = mempool_alloc_safe(sizeof(CollisionPair) * 16, COLOUR_TAG_BLUE);
    gAssetsLvlObjTranslationTable = (s16 *) asset_table_load(ASSET_LEVEL_OBJECT_TRANSLATION_TABLE);
    {
        // Big-endian asset offset tables (helpers in linux/reimpl.c).
        extern void pc_swap16_buf(void *buf, u32 numBytes);
        pc_swap16_buf(gAssetsLvlObjTranslationTable, asset_table_size(ASSET_LEVEL_OBJECT_TRANSLATION_TABLE));
    }
    gAssetsLvlObjTranslationTableLength = (asset_table_size(ASSET_LEVEL_OBJECT_TRANSLATION_TABLE) >> 1) - 1;
    while (gAssetsLvlObjTranslationTable[gAssetsLvlObjTranslationTableLength] == 0) {
        gAssetsLvlObjTranslationTableLength--;
    }
    gSpawnObjectHeap = mempool_alloc_safe(OBJECT_BLUEPRINT_SIZE, COLOUR_TAG_BLUE);
    gAssetsObjectHeadersTable = (s32 *) asset_table_load(ASSET_OBJECT_HEADERS_TABLE);
    {
        extern void pc_swap32_buf(void *buf, u32 numBytes);
        pc_swap32_buf(gAssetsObjectHeadersTable, asset_table_size(ASSET_OBJECT_HEADERS_TABLE));
    }
    gAssetsObjectHeadersTableLength = 0;
    while (-1 != gAssetsObjectHeadersTable[gAssetsObjectHeadersTableLength]) {
        gAssetsObjectHeadersTableLength++;
    }
    gAssetsObjectHeadersTableLength--;
    gLoadedObjectHeaders = mempool_alloc_safe(gAssetsObjectHeadersTableLength * sizeof(ObjectHeader *), COLOUR_TAG_WHITE);
    gObjectHeaderReferences = mempool_alloc_safe(gAssetsObjectHeadersTableLength, COLOUR_TAG_WHITE);

    for (i = 0; i < gAssetsObjectHeadersTableLength; i++) {
        (*gObjectHeaderReferences)[i] = 0;
    }

    gAssetsMiscSection = (s32 *) asset_table_load(ASSET_MISC);
    gAssetsMiscTable = (s32 *) asset_table_load(ASSET_MISC_TABLE);
    {
        // Table only — the misc *section* is mixed-format data; each
        // get_misc_asset consumer owns the endianness of its own piece.
        extern void pc_swap32_buf(void *buf, u32 numBytes);
        extern void pc_swap16_buf(void *buf, u32 numBytes);
        pc_swap32_buf(gAssetsMiscTable, asset_table_size(ASSET_MISC_TABLE));

        // ASSET_MISC_20: the 10 Object_Boost racer-FX entries read by
        // racerfx_alloc — 27 f32s of boost data plus sprite/texture ids per
        // entry, then runtime state (unk70..tex) including two pointers. The
        // host struct is wider than the 0x80-byte image, so the entries are
        // unpacked into sPcBoostTable and get_misc_asset hands that out.
        {
            static const PcField sBoostFields[] = {
                PCU_FIELD(Object_Boost, carBoostData, 0x00, 27, PCU_U32),
                PCU_FIELD(Object_Boost, spriteId, 0x6C, 2, PCU_U16),
                PCU_FIELD(Object_Boost, effectState, 0x70, 4, PCU_BYTES),
                PCU_FIELD(Object_Boost, effectScale, 0x74, 1, PCU_U32),
                PCU_FIELD(Object_Boost, sprite, 0x78, 2, PCU_PTR),
                PCU_END,
            };
            pc_unpack_array(sPcBoostTable, sizeof(Object_Boost), &gAssetsMiscSection[gAssetsMiscTable[ASSET_MISC_20]],
                            0x80, NUMBER_OF_CHARACTERS, sBoostFields);
        }

        // ASSET_MISC_SHIELD_DATA: the RacerShieldGfx table read by
        // render_racer_shield, indexed (vehicleID * 10) + racerIndex. Mixed
        // s16/f32, so it cannot ride along with the flat f32 assets below.
        // Left big-endian the shield sat ~3000 units above the kart at a
        // nonsense scale, with a denormal turnSpeed collapsing the shear
        // matrix: invisible, though shieldTimer still blocked hits.
        {
            RacerShieldGfx *shield =
                (RacerShieldGfx *) &gAssetsMiscSection[gAssetsMiscTable[ASSET_MISC_SHIELD_DATA]];
            s32 n;
            _Static_assert(sizeof(RacerShieldGfx) == 0x10, "RacerShieldGfx layout drifted from N64");
            _Static_assert(__builtin_offsetof(RacerShieldGfx, scale) == 8,
                           "RacerShieldGfx layout drifted from N64");
            for (n = 0; n < NUMBER_OF_PLAYER_VEHICLES * NUMBER_OF_CHARACTERS; n++) {
                PC_SWAP16_AT(&shield[n], x_position, 8); // x/y/z_position, y_offset
                PC_SWAP32_AT(&shield[n], scale, 8);      // scale, turnSpeed
            }
        }
    }
    gAssetsMiscTableLength = 0;
    while (-1 != gAssetsMiscTable[gAssetsMiscTableLength]) {
        gAssetsMiscTableLength++;
    }
    {
        // The f32 misc assets. Left big-endian, every one of these reads as a
        // denormal (~0): racers ended up weightless, with no handling and a flat
        // acceleration curve, so the player could not move. The per-vehicle
        // tables reached through ObjectHeader (unk5C acceleration, unk5D wheel
        // offsets) are swapped where they are fetched, in racer.c — their index
        // is not a constant. ASSET_MISC_20 is handled above as Object_Boost.
        static const s32 sMiscF32Assets[] = {
            ASSET_MISC_4,           ASSET_MISC_8,
            ASSET_MISC_RACER_WEIGHT, ASSET_MISC_RACER_HANDLING,
            ASSET_MISC_RACER_UNUSED_11, ASSET_MISC_17,
            ASSET_MISC_18,          ASSET_MISC_MAGNET_DATA,
            ASSET_MISC_32,          ASSET_MISC_RACERACCELERATION_UNKNOWN0,
            ASSET_MISC_RACER_HITBOX_SIZE,
        };
        s32 n;

        for (n = 0; n < (s32) ARRAY_COUNT(sMiscF32Assets); n++) {
            pc_swap_misc_f32_once(sMiscF32Assets[n]);
        }
    }

    decrypt_magic_codes(
        &gAssetsMiscSection[gAssetsMiscTable[ASSET_MISC_MAGIC_CODES]],
        (gAssetsMiscTable[ASSET_MISC_TITLE_SCREEN_DEMO_IDS] - gAssetsMiscTable[ASSET_MISC_MAGIC_CODES]) *
            sizeof(s32)); // index delta -> bytes (the N64 source spelled this sizeof(s32 *))
    gObjPtrList = mempool_alloc_safe(sizeof(uintptr_t) * OBJECT_SLOT_COUNT, COLOUR_TAG_BLUE);
    gFirstTimeFinish = 0;
    gTimeTrialEnabled = 0;
    gIsTimeTrial = FALSE;
    gObjectUpdateRateF = 2.0f;
    clear_object_pointers();
}

// Decrypts cheats
void decrypt_magic_codes(s32 *data, s32 length) {
    s32 i;
    s32 j;
    u8 *ptr = (u8 *) data;
    u8 temp[4];

    for (i = 0; i < (length >> 2); i++) {
        // Swap bits according to the following pattern:
        // AABBCCDD EEFFGGHH IIJJKKLL MMNNOOPP -> AAEEIIMM BBFFJJNN CCGGKKOO DDHHLLPP
        // clang-format off
        temp[0] = ((ptr[0] & 0xC0)     ) |
                  ((ptr[1] & 0xC0) >> 2) |
                  ((ptr[2] & 0xC0) >> 4) |
                  ((ptr[3] & 0xC0) >> 6);
        temp[1] = ((ptr[0] & 0x30) << 2) |
                  ((ptr[1] & 0x30)     ) |
                  ((ptr[2] & 0x30) >> 2) |
                  ((ptr[3] & 0x30) >> 4);
        temp[2] = ((ptr[0] & 0x0C) << 4) |
                  ((ptr[1] & 0x0C) << 2) |
                  ((ptr[2] & 0x0C)     ) |
                  ((ptr[3] & 0x0C) >> 2);
        temp[3] = ((ptr[0] & 0x03) << 6) |
                  ((ptr[1] & 0x03) << 4) |
                  ((ptr[2] & 0x03) << 2) |
                  ((ptr[3] & 0x03)     );
        // clang-format on

        // Swap the odd and even bits
        for (j = 0; j < 4; j++) {
            *ptr++ = (((temp[j] & 0xAA) >> 1) | ((temp[j] & 0x55) << 1));
        }
    }

    // The decryption above is endian-safe (it works on bytes in memory order),
    // but the decrypted table is big-endian ROM data: a u16 cheat count followed
    // by a u16 offset table (two entries per cheat — code string, description).
    // The matcher (menu.c) reads those as native u16, so on a little-endian host
    // the count and every offset come out byteswapped and no cheat ever matches.
    // Swap just the header words back; the ASCII strings after them are bytes and
    // must be left alone. Header length = 1 count word + 2 offsets per cheat.
    {
        u16 *words = (u16 *) data;
        s32 count = (s32) (u16) ((words[0] << 8) | (words[0] >> 8));
        s32 headerWords = 1 + count * 2;
        s32 k;

        for (k = 0; k < headerWords; k++) {
            words[k] = (u16) ((words[k] << 8) | (words[k] >> 8));
        }
    }
}

/**
 * Set all object counters and headers to zero, effectively telling the game there are no objects currently in the
 * scene.
 */
void clear_object_pointers(void) {
    s32 i;

    D_8011AD26 = TRUE;
    gFreeListCount = 0;
    gCollisionObjectCount = 0;
    gNumberOfMainCheckpoints = 0;
    gNumberOfTotalCheckpoints = 0;
    gNumRacers = 0;
    gAnimObjectCount = 0;
    gAnimFrameParity = 0;
    gAnimFrameState[0] = 0;
    gAnimFrameState[1] = 0;

    for (i = 0; i < AINODE_COUNT; i++) {
        (*gAINodes)[i] = NULL;
    }
    for (i = 0; i < 8; i++) {
        (*gDrawbridgeTimers)[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        gCollisionPairs[i].ttl = 0;
    }

    gAINodeTail[0] = 0xFF;
    gAINodeTail[1] = 0xFF;
    gObjectCount = 0;
    gObjectListStart = 0;
    gParticleCount = 0;
    gAnimGroup = 0;
    gCutsceneID = 0;
    gCutsceneChanged = TRUE;
    gFirstActiveObjectId = 0;
    gTransformTimer = 0;
    gIsTajChallenge = FALSE;
    gTajRaceInit = 0;
    gCurrentMidiFade = NULL;
    gOverridePosCount = 0;
    gZoneJustLoaded = TRUE;
    gCutsceneSkipRequested = 0;
    gOverrideDoors = FALSE;
}

/**
 * Clear all objects from memory. Also clear rumble.
 * Swap lead player in adventure two if the other player finished ahead of the lead player.
 */
void free_all_objects(void) {
    s32 i, len;
    timetrial_free_staff_ghost();
    gIsP2LeadPlayer = FALSE;
    if (gRollingDemo) {
        rumble_init(TRUE);
    }
    gRollingDemo = FALSE;
    if (gSwapLeadPlayer && is_in_two_player_adventure()) {
        gSwapLeadPlayer = FALSE;
        swap_lead_player();
    }
    gParticlePtrList_flush();
    len = gObjectCount;
    for (i = 0; i < len; i++) {
        obj_destroy(gObjPtrList[i], 1);
    }
    gFreeListCount = 0;
    gObjectCount = 0;
    gObjectListStart = 0;
    clear_object_pointers();
    mempool_free((void *) gObjectMap[0]);
    mempool_free((void *) gObjectMap[1]);
}

// ObjectHeader is loaded from a 0x78-byte big-endian image followed by the
// arrays it references (model ids, vehicle part ids/indices, particle entries,
// light sources). The image is loaded OBJECT_HEADER_PC_DELTA bytes into the
// allocation so the host-sized header fits in front of it; the header is
// unpacked over the front and its offsets rebased so the game's existing
// offset->pointer fixups land on the arrays. The ObjectHeader24 union at 0x08
// stays byte order (its u32 reading in lights.c has a PC-side byte read).
#define OBJECT_HEADER_DISK_SIZE 0x78
#define OBJECT_HEADER_PC_DELTA (sizeof(ObjectHeader) - OBJECT_HEADER_DISK_SIZE)

static const PcField sObjectHeaderFields[] = {
    PCU_FIELD(ObjectHeader, waterEffectAnimSpeed, 0x00, 4, PCU_U32),      // waterEffectAnimSpeed, shadowScale, waterEffectScale, scale
    PCU_FIELD(ObjectHeader, modelIds, 0x10, 4, PCU_PTR),  // modelIds..objectParticles
    PCU_FIELD(ObjectHeader, pad20, 0x20, 1, PCU_U32),
    PCU_FIELD(ObjectHeader, lightSources, 0x24, 1, PCU_PTR),
    PCU_FIELD(ObjectHeader, shadeAmbient, 0x28, 2, PCU_U32),
    PCU_FIELD(ObjectHeader, flags, 0x30, 5, PCU_U16),       // flags..unk38
    PCU_FIELD(ObjectHeader, shadeLightR, 0x3A, 4, PCU_BYTES),
    PCU_FIELD(ObjectHeader, shadeAngleY, 0x3E, 10, PCU_U16), // shadeAngleY..unk50
    PCU_FIELD(ObjectHeader, instanceCount, 0x52, 14, PCU_BYTES),     // instanceCount..pad5E
    PCU_FIELD(ObjectHeader, internalName, 0x60, 16, PCU_BYTES),
    PCU_FIELD(ObjectHeader, unused70, 0x70, 8, PCU_BYTES),
    PCU_END,
};

static void pc_unpack_object_header(ObjectHeader *header) {
    u8 *disk = (u8 *) header + OBJECT_HEADER_PC_DELTA;
    s32 i;

    pc_unpack(header, sizeof(ObjectHeader), disk, OBJECT_HEADER_DISK_SIZE, sObjectHeaderFields);

    // Offsets are relative to the image; rebase them onto the host header.
    header->modelIds = (s32 *) ((uintptr_t) header->modelIds + OBJECT_HEADER_PC_DELTA);
    header->vehiclePartIds = (s32 *) ((uintptr_t) header->vehiclePartIds + OBJECT_HEADER_PC_DELTA);
    header->vehiclePartIndices = (s8 *) ((uintptr_t) header->vehiclePartIndices + OBJECT_HEADER_PC_DELTA);
    header->objectParticles = (ObjHeaderParticleEntry *) ((uintptr_t) header->objectParticles + OBJECT_HEADER_PC_DELTA);
    header->lightSources = (ObjectHeader24 *) ((uintptr_t) header->lightSources + OBJECT_HEADER_PC_DELTA);

    // Pointed-to arrays, still offsets here — swap via the header base.
    pc_swap32_buf((u8 *) header + (uintptr_t) header->modelIds, header->numberOfModelIds * 4);
    pc_swap32_buf((u8 *) header + (uintptr_t) header->vehiclePartIds, header->attachPointCount * 4);
    pc_swap32_buf((u8 *) header + (uintptr_t) header->objectParticles,
                  header->particleCount * sizeof(ObjHeaderParticleEntry));
    for (i = 0; i < header->numLightSources; i++) {
        ObjectHeader24 *light = &((ObjectHeader24 *) ((u8 *) header + (uintptr_t) header->lightSources))[i];
        PC_SWAP16_AT(light, colourAnimAssetId, 2);
        PC_SWAP16_AT(light, homeX, 12); // homeX/Y/Z, radius, unk14, unk16
    }
}

/**
 * Set the object's header.
 * Search if the intended header is already loaded and use that.
 * Otherwise, allocate space and load it into ROM and set it to that.
 */
ObjectHeader *load_object_header(s32 index) {
    s32 assetOffset;
    s32 size;
    ObjectHeader *address;

    if ((*gObjectHeaderReferences)[index] != 0) {
        (*gObjectHeaderReferences)[index]++;
        return (*gLoadedObjectHeaders)[index];
    }
    assetOffset = gAssetsObjectHeadersTable[index];
    size = gAssetsObjectHeadersTable[index + 1] - assetOffset;
    address = mempool_alloc_pool((MemoryPoolSlot *) gObjectMemoryPool, size + OBJECT_HEADER_PC_DELTA);
    if (address != NULL) {
        asset_load(ASSET_OBJECTS, (uintptr_t) address + OBJECT_HEADER_PC_DELTA, assetOffset, size);
        pc_unpack_object_header(address);
        address->lightSources = (ObjectHeader24 *) ((uintptr_t) address + (uintptr_t) address->lightSources);
        address->objectParticles =
            (ObjHeaderParticleEntry *) ((uintptr_t) address + (uintptr_t) address->objectParticles);
        address->vehiclePartIds = (s32 *) ((uintptr_t) address + (uintptr_t) address->vehiclePartIds);
        address->vehiclePartIndices = (s8 *) ((uintptr_t) address + (uintptr_t) address->vehiclePartIndices);
        address->modelIds = (s32 *) ((uintptr_t) address + (uintptr_t) address->modelIds);
        (*gLoadedObjectHeaders)[index] = address;
        (*gObjectHeaderReferences)[index] = 1;
    } else {
        return NULL;
    }
    return address;
}

/**
 * Remove this object from the loaded header's references.
 * If the reference number is zero, free the header.
 */
void try_free_object_header(s32 index) {
    if ((*gObjectHeaderReferences)[index] != 0) {
        (*gObjectHeaderReferences)[index]--;
        if ((*gObjectHeaderReferences)[index] == 0) {
            mempool_free((void *) (*gLoadedObjectHeaders)[index]);
        }
    }
}

/**
 * Converts the passed value into an accurate countdown value based on the systems region.
 * Since PAL runs at 50Hz, it therefore will reduce the timer to 5/6 to match, keeping
 * it consistent with non PAL timers, running 60Hz.
 * Official Name: objTvTimes
 */
s32 normalise_time(s32 timer) {
    if (osTvType != OS_TV_TYPE_PAL || timer < 0) {
        return timer;
    } else {
        return (timer * 5) / 6;
    }
}

// Object-map spawn entries are big-endian; the layout past the common header
// is per-behavior, mirroring run_object_init_func's dispatch (plus
// BHV_WAVE_POWER, whose entry obj_loop_wavepower reads). Entry types with only
// u8/s8 fields need no swap. Runtime-built entries (NEW_OBJECT_ENTRY) never
// pass through here and stay host-order.
static void pc_swap_spawn_entry_fields(u8 *entryBytes) {
    LevelObjectEntry *entry = (LevelObjectEntry *) entryBytes;
    s32 objType = entryBytes[0] | ((entryBytes[1] & 0x80) << 1);
    s16 headerType = gAssetsLvlObjTranslationTable[objType];
    u8 tmpHeader[OBJECT_HEADER_DISK_SIZE];
    s8 behavior;

    if (headerType >= gAssetsObjectHeadersTableLength) {
        headerType = 0;
    }
    // Peek behaviorId (an s8, endianness-free) straight from the asset into a
    // stack buffer: going through load_object_header/try_free_object_header
    // here would push a mempool free per entry onto the deferred-free queue,
    // which never drains during level load and overflows on dense maps.
    asset_load(ASSET_OBJECTS, (uintptr_t) tmpHeader, gAssetsObjectHeadersTable[headerType], OBJECT_HEADER_DISK_SIZE);
    behavior = (s8) tmpHeader[0x54]; // ObjectHeader.behaviorId

    switch (behavior) {
        case BHV_RACER:
            PC_SWAP16_AT(entry, racer.angleZ, 8); // angleZ/X/Y, playerIndex
            break;
        case BHV_FISH:
            PC_SWAP16_AT(entry, fish.pathRadius, 2);
            break;
        case BHV_AUDIO:
            PC_SWAP16_AT(entry, audio.soundId, 4); // soundId, range
            break;
        case BHV_AUDIO_LINE:
        case BHV_AUDIO_LINE_2:
            PC_SWAP16_AT(entry, audioLine.soundID, 2);
            PC_SWAP16_AT(entry, audioLine.range, 2);
            break;
        case BHV_FOG_CHANGER:
            PC_SWAP16_AT(entry, fogChanger.near, 6); // near, far, switchTimer
            break;
        case BHV_TEXTURE_SCROLL:
            PC_SWAP16_AT(entry, texScroll.textureIndex, 2);
            break;
        case BHV_LIGHT_RGBA:
            PC_SWAP16_AT(entry, rgbaLighting.radius, 14); // radius..unk1A
            break;
        case BHV_WEATHER:
            PC_SWAP16_AT(entry, weather.radius, 8); // radius..unkE
            PC_SWAP16_AT(entry, weather.transitionTime, 2);
            break;
        case BHV_LENS_FLARE:
            PC_SWAP16_AT(entry, lensFlare.angleX, 4); // angleX, angleY
            break;
        case BHV_LENS_FLARE_SWITCH:
            PC_SWAP16_AT(entry, lensFlareSwitch.radius, 2);
            break;
        case BHV_CHARACTER_FLAG:
            PC_SWAP16_AT(entry, characterFlag.angleZ, 8); // angleZ, radius, angleY, playerIndex
            break;
        case BHV_ANIMATION:
            PC_SWAP16_AT(entry, animation.objectIdToSpawn, 4); // objectIdToSpawn, animationStartDelay
            PC_SWAP16_AT(entry, animation.pauseFrameCount, 2);
            break;
        case BHV_WAVE_GENERATOR:
            PC_SWAP16_AT(entry, waveGenerator.waveSize, 6); // waveSize, unkC, unkE
            break;
        case BHV_WAVE_POWER:
            PC_SWAP16_AT(entry, wavePower.radius, 6); // radius, power, divisor
            break;
        case BHV_BUTTERFLY:
            PC_SWAP16_AT(entry, butterfly.homeRadius, 2);
            break;
        case BHV_MIDI_FADE_POINT:
            PC_SWAP16_AT(entry, midiFadePoint.innerRadius, 4); // innerRadius, outerRadius
            break;
        case BHV_MIDI_CHANNEL_SET:
            PC_SWAP16_AT(entry, midichset.channelMask, 2);
            break;
        case BHV_BUBBLER:
            PC_SWAP16_AT(entry, bubbler.particleDensity, 2);
            break;
        case BHV_RANGE_TRIGGER:
            PC_SWAP16_AT(entry, rangeTrigger.radius, 4); // radius, particleFlags
            break;
        case BHV_FROG:
            PC_SWAP16_AT(entry, frog.homeRadius, 2);
            break;
    }
}

/**
 * Load the object map into RAM, then start spawning objects into the world.
 * Also decides whether this race type should be for silver coins or not.
 */
void track_spawn_objects(s32 mapID, s32 index) {
    s32 assetSize;
    Settings *settings;
    s32 i;
    s32 *mem;
    s32 var_s0;
    u32 assetOffset;
    u32 *objMapTable;
    UNUSED s32 pad;
    u8 *compressedAsset;
    s32 temp_t3;

    settings = get_settings();
    assetOffset = settings->bosses | 0x820; // 0x820 = Wizpig 2 and some unknown 0x800 boss bit
    gIsSilverCoinRace = ((settings->courseFlagsPtr[settings->courseId] & RACE_CLEARED_SILVER_COINS) == FALSE) &&
                        (((1 << settings->worldId) & assetOffset) != 0);

    if ((settings->courseFlagsPtr[settings->courseId] & RACE_CLEARED) == FALSE) {
        gIsSilverCoinRace = FALSE;
    }
    if (is_in_tracks_mode()) {
        gIsSilverCoinRace = FALSE;
    }
    if (level_type()) {
        gIsSilverCoinRace = FALSE;
    }

    gAnimCameraCount = 0;
    mem = mempool_alloc_safe(OBJECT_MAP_SIZE, COLOUR_TAG_BLUE);
    gObjectMap[index] = mem;
    gObjectMapSpawnList[index] = (u8 *) (gObjectMap[index] + 4);
    gObjectMapSize[index] = NULL;
    gObjectMapID[index] = mapID;
    objMapTable = (u32 *) asset_table_load(ASSET_LEVEL_OBJECT_MAPS_TABLE);
    {
        // Big-endian asset offset table (helper in linux/reimpl.c).
        extern void pc_swap32_buf(void *buf, u32 numBytes);
        pc_swap32_buf(objMapTable, asset_table_size(ASSET_LEVEL_OBJECT_MAPS_TABLE));
    }
    for (i = 0; objMapTable[i] != 0xFFFFFFFF; i++) {}
    i--;
    if (mapID >= i) {
        mapID = 0;
    }
    assetOffset = objMapTable[mapID];
    assetSize = objMapTable[mapID + 1] - assetOffset;

    if (assetSize != 0) {
        if (asset_image_inflated()) {
            asset_load(ASSET_LEVEL_OBJECT_MAPS, (uintptr_t) mem, assetOffset, assetSize);
        } else {
            compressedAsset = (u8 *) mem;
            compressedAsset =
                ((compressedAsset + gzip_size_uncompressed(ASSET_LEVEL_OBJECT_MAPS, assetOffset)) - (0, assetSize)) +
                0x20;
            asset_load(ASSET_LEVEL_OBJECT_MAPS, (uintptr_t) compressedAsset, assetOffset, assetSize);
            gzip_inflate(compressedAsset, (u8 *) mem);
        }
        {
            // Inflated object map is big-endian: a u32 spawn-list byte length,
            // then variable-size entries whose common header holds three s16
            // coords. Type-specific fields past the common header are NOT
            // swapped here yet — spawn handlers reading s16s from entries get
            // byte-swapped values until each entry type is handled.
            extern void pc_swap16_buf(void *buf, u32 numBytes);
            extern void pc_swap32_buf(void *buf, u32 numBytes);
            _Static_assert(sizeof(LevelObjectEntryCommon) == 8, "LevelObjectEntryCommon layout drifted from N64");
            u8 *entry = (u8 *) (gObjectMap[index] + 4);
            s32 entryOffset;
            s32 entrySize;
            pc_swap32_buf(mem, 4);
            for (entryOffset = 0; entryOffset < *mem; entryOffset += entrySize) {
                pc_swap16_buf(entry + 2, 6); // LevelObjectEntryCommon x, y, z
                pc_swap_spawn_entry_fields(entry);
                entrySize = entry[1] & 0x3F;
                entry += entrySize;
            }
        }
        mempool_free(objMapTable);
        gObjectMapSpawnList[index] = (u8 *) (gObjectMap[index] + 4);
        gObjectMapSize[index] = *mem;
        gObjectMapIndex = index;
        for (var_s0 = 0; var_s0 < gObjectMapSize[index]; var_s0 += temp_t3) {
            spawn_object((LevelObjectEntryCommon *) gObjectMapSpawnList[index], OBJECT_SPAWN_UNK01);
            gObjectMapSpawnList[index] = &gObjectMapSpawnList[index][temp_t3 = gObjectMapSpawnList[index][1] & 0x3F];
        }
        gObjectMapSpawnList[index] = (u8 *) (gObjectMap[index] + 4);
        gCollisionObjectCount = 0;
        gNumFinishedRacers = 1;
        if (gPathUpdateOff == FALSE) {
            gParticlePtrList_flush();
            checkpoint_update_all();
            spectate_update();
            anim_objects_update();
        }
        gPathUpdateOff = TRUE;
    }
}

/** The small table of objects addressed by id (balloons, doors): clear it. */
void object_ids_clear(void) {
    s32 i; // Required to be one line to match
    // clang-format off
    for (i = 0; i < ARRAY_COUNT(gIdObjects); i++) { gIdObjects[i] = NULL; }
    // clang-format on
}

/** Registers an object under a given id if that id is free. */
void object_id_assign(Object *obj, s32 index) {
    if (gIdObjects[index] == NULL) {
        gIdObjects[index] = obj;
    } else {
        if (gIdObjects[index]) {}
    }
}

/** Registers an object under the first free id and returns it, or -1 if the table is full. */
s32 object_id_alloc(Object *obj) {
    s32 i;
    s32 NextFreeIndex;

    NextFreeIndex = -1;
    for (i = 0; i < ARRAY_COUNT(gIdObjects); i++) {
        if (gIdObjects[i] == NULL) {
            NextFreeIndex = i;
            i = ARRAY_COUNT(gIdObjects); // Why not just break?
        }
    }
    if (NextFreeIndex != -1) {
        gIdObjects[NextFreeIndex] = obj;
    }
    return NextFreeIndex;
}

typedef struct RacerSpawnPoint {
    s32 x;
    s32 y;
    s32 z;
    s32 angle;
} RacerSpawnPoint;

/**
 * Read the level's setup points for this entrance into the eight spawn slots. A setup point
 * may also dictate the vehicle for the whole grid.
 */
static void racers_collect_setup_points(u32 entranceID, RacerSpawnPoint *spawns, Vehicle *vehicle) {
    Object *obj;
    LevelObjectEntry *objEntry;
    s32 slot;
    s32 j;

    for (j = 0; j < 8; j++) {
        spawns[j].x = 0;
        spawns[j].y = 0;
        spawns[j].z = 0;
        spawns[j].angle = 0;
    }
    for (j = 0; j < gObjectCount; j++) {
        obj = gObjPtrList[j];
        if ((obj->trans.flags & OBJ_FLAGS_PARTICLE) || obj->behaviorId != BHV_SETUP_POINT) {
            continue;
        }
        if (entranceID != obj->properties.setupPoint.entranceID) {
            continue;
        }
        slot = obj->properties.setupPoint.racerIndex;
        if (slot < 8) {
            spawns[slot].x = obj->trans.x_position;
            spawns[slot].y = obj->trans.y_position;
            spawns[slot].z = obj->trans.z_position;
            spawns[slot].angle = obj->trans.rotation.y_rotation;
        }
        objEntry = obj->level_entry;
        if (objEntry->setupPoint.vehicle != -1) {
            *vehicle = objEntry->setupPoint.vehicle;
        }
    }
}

/**
 * Give every racer a distinct starting slot. Slots already claimed in the save are kept;
 * racers without a valid slot take the free ones in order.
 */
static void racers_assign_starting_positions(Settings *settings) {
    s8 slotTaken[8];
    s8 unplaced[8];
    u8 unplacedCount;
    s32 i;
    s32 j;

    for (i = 0; i < gNumRacers; i++) {
        slotTaken[i] = FALSE;
    }
    unplacedCount = 0;
    for (i = 0; i < gNumRacers; i++) {
        if (settings->racers[i].starting_position < gNumRacers) {
            if (slotTaken[settings->racers[i].starting_position] == FALSE) {
                slotTaken[settings->racers[i].starting_position] = TRUE;
                continue;
            }
        }
        unplaced[unplacedCount++] = i;
    }
    for (i = 0; i < unplacedCount; i++) {
        for (j = 0; j < gNumRacers; j++) {
            if (slotTaken[j] == FALSE) {
                slotTaken[j] = TRUE;
                settings->racers[unplaced[i]].starting_position = j;
                break;
            }
        }
    }
}

/**
 * The port (index into the settings' racers) that occupies starting slot `slot`. Hub
 * worlds, challenges and boss races seat racers in port order.
 */
static s32 racer_port_for_slot(Settings *settings, s32 slot, u8 raceType) {
    s32 j;

    if (raceType == RACETYPE_HUBWORLD || (raceType & RACETYPE_CHALLENGE) || gBossRaceMode != 0) {
        return slot;
    }
    for (j = 0; j < gNumRacers; j++) {
        if (slot == settings->racers[j].starting_position) {
            return j;
        }
    }
    return 0;
}

/**
 * The vehicle for one grid slot. Boss races and the rolling demo dictate it; otherwise AI
 * racers and both players of a shared adventure drive what player one picked.
 */
static Vehicle racer_spawn_vehicle(Vehicle vehicle, s32 slot, s32 playerIndex, s32 numPlayers,
                                   LevelHeader *levelHeader) {
    if (gBossRaceMode == 1) {
        if (slot == 0) {
            return gBossVehicles[gBossRaceIds[1]].playerVehicle;
        }
        return gBossVehicles[gBossRaceIds[1]].bossVehicle;
    }
    if (gBossRaceMode == 2) {
        return levelHeader->vehicle;
    }
    if (playerIndex == 4 || race_is_adventure_2P()) {
        return get_player_selected_vehicle(PLAYER_ONE);
    }
    if (numPlayers >= 2) {
        return get_player_selected_vehicle(playerIndex);
    }
    return vehicle;
}

static s16 racer_object_id(Settings *settings, s32 slot, s32 port, Vehicle vehicle) {
    if (gBossRaceMode == 2) {
        return gRacerObjectTable[gDefaultCharacterOrder[slot] + (vehicle * NUM_CHARACTERS)];
    }
    if (vehicle < 5) {
        return gRacerObjectTable[(settings->racers[port].character) + (vehicle * NUM_CHARACTERS)];
    }
    return gRacerObjectTable[vehicle + 45];
}

/**
 * Spawn flags for a racer: AI racers get water effects and, with several players, animation,
 * and their models start on a shifted animation frame; a lone human gets interaction. Boss
 * vehicles get neither.
 */
static s32 racer_spawn_flags(s32 playerIndex, s32 numPlayers, Vehicle vehicle) {
    s32 flags;

    flags = OBJECT_BEHAVIOUR_SHADED;
    if (playerIndex == 4) {
        model_anim_offset(1);
        flags = OBJECT_BEHAVIOUR_SHADED | OBJECT_BEHAVIOUR_WATER_EFFECT;
        if (numPlayers >= 2) {
            flags |= OBJECT_BEHAVIOUR_ANIMATION;
        }
    } else if (numPlayers == 1) {
        flags |= OBJECT_BEHAVIOUR_INTERACTIVE;
    }
    if (vehicle >= VEHICLE_BOSSES) {
        flags = OBJECT_BEHAVIOUR_SHADED;
        model_anim_offset(0);
    }
    return flags;
}

/**
 * Fill in a freshly spawned racer: vehicle, character, bananas, engine sound and push force.
 */
static void racer_init_fields(Object *racerObj, Settings *settings, s32 slot, s32 port, Vehicle vehicle, u8 raceType,
                              enum GameMode gameMode, s8 *pushForces) {
    Object_Racer *racer;

    racer = racerObj->racer;
    racerObj->level_entry = NULL;
    racer->vehicleID = vehicle;
    racer->vehicleIDPrev = vehicle;
    if (racer->vehicleID == VEHICLE_PLANE || racer->vehicleID == VEHICLE_SMOKEY ||
        racer->vehicleID == VEHICLE_PTERODACTYL) {
        gIsNonCarRacers = TRUE;
    }
    racer->startVehicleID = vehicle;
    if (racer->startVehicleID < VEHICLE_CAR || racer->startVehicleID > VEHICLE_PLANE) {
        racer->startVehicleID = 0;
    }
    racer->racerIndex = port;
    if (gBossRaceMode == 2) {
        racer->characterId = gDefaultCharacterOrder[slot];
    } else {
        racer->characterId = settings->racers[port].character;
    }
    if (raceType == RACETYPE_CHALLENGE_BATTLE) {
        racer->bananas = 8;
    } else {
        racer->bananas = 0;
    }
    if (get_filtered_cheats() & CHEAT_START_WITH_10_BANANAS && !(raceType & RACETYPE_CHALLENGE)) {
        if (racer->playerIndex != PLAYER_COMPUTER) {
            racer->bananas = 10;
        }
    }
    if ((gameMode != GAMEMODE_MENU || gBossRaceMode == 2) && vehicle < VEHICLE_BOSSES) {
        racer->vehicleSound = racer_sound_init(racer->characterId, racer->vehicleID);
    } else {
        racer->vehicleSound = NULL;
    }

    racerObj->interactObj->pushForce = pushForces[racer->characterId] + 1;
    switch (racer->vehicleID) {
        case VEHICLE_TRICKY:
        case VEHICLE_BLUEY:
        case VEHICLE_SMOKEY:
        case VEHICLE_PTERODACTYL:
        case VEHICLE_SNOWBALL:
        case VEHICLE_BUBBLER:
        case VEHICLE_WIZPIG:
        case VEHICLE_ROCKET:
            racer_special_init(racerObj, racer);
            break;
        default:
            break;
    }
}

/**
 * Spawn a time-trial ghost on the first grid slot: a translucent racer with no shadow or
 * interaction.
 */
static Object *racers_spawn_ghost(LevelObjectEntry_Racer *racerEntry, s16 objectID, RacerSpawnPoint *spawn) {
    Object *obj;

    racerEntry->common.size = ((objectID & 0x100) >> 1) | 0x10;
    racerEntry->common.objectID = objectID;
    racerEntry->common.x = spawn->x;
    racerEntry->common.y = spawn->y;
    racerEntry->common.z = spawn->z;
    racerEntry->angleY = spawn->angle;
    obj = spawn_object((LevelObjectEntryCommon *) racerEntry, 1);
    obj->level_entry = NULL;
    obj->behaviorId = BHV_TIMETRIAL_GHOST;
    obj->shadow->scale = 0.01f;
    obj->interactObj->flags = INTERACT_FLAGS_NONE;
    obj->racer->transparency = 96;
    return obj;
}

/**
 * Run each racer for ten ticks so it rests on the ground, stagger the AI animation
 * timers, and apply the size cheats.
 */
static void racers_settle(void) {
    Object *racerObj;
    Object_Racer *racer;
    s32 aiAnimParity;
    s32 j;
    s32 k;

    aiAnimParity = 0;
    for (j = 0; j < gNumRacers; j++) {
        racerObj = (*gRacers)[j];
        racer = racerObj->racer;
        for (k = 0; k < 10; k++) {
            update_player_racer(racerObj, LOGIC_30FPS);
        }
        if (racer->playerIndex == PLAYER_COMPUTER) {
            aiAnimParity++;
            aiAnimParity &= 1;
        }
        for (k = 0; k < racerObj->header->numberOfModelIds; k++) {
            if (racerObj->modelInstances[k] != NULL && racerObj->modelInstances[k]->animUpdateTimer != 0) {
                if (racer->playerIndex == PLAYER_COMPUTER) {
                    racerObj->modelInstances[k]->animUpdateTimer = (aiAnimParity * 2);
                } else {
                    racerObj->modelInstances[k]->animUpdateTimer = 0;
                }
            }
        }
        if (get_filtered_cheats() & CHEAT_BIG_CHARACTERS) {
            racerObj->trans.scale *= 1.4f;
        }
        if (get_filtered_cheats() & CHEAT_SMALL_CHARACTERS) {
            racerObj->trans.scale *= 0.714f;
        }
        racer->stretch_height_cap = 1.0f;
        racer->stretch_height = 1.0f;
    }
}

/**
 * The three on-screen position arrows of a single-player race.
 */
static void racers_spawn_position_arrows(LevelObjectEntry_Racer *racerEntry) {
    Object *obj;
    s32 j;

    for (j = 0; j < 3; j++) {
        racerEntry->common.objectID = ASSET_OBJECT_ID_POSARROW;
        racerEntry->common.size = sizeof(LevelObjectEntryCommon);
        racerEntry->common.x = 0;
        racerEntry->common.y = 0;
        racerEntry->common.z = 0;
        obj = spawn_object((LevelObjectEntryCommon *) racerEntry, OBJECT_SPAWN_UNK01);
        obj->properties.common.unk0 = j;
        obj->level_entry = NULL;
    }
}

/**
 * In the central area, once the player holds enough balloons for a Taj challenge that is
 * not yet unlocked, Taj teleports to the player to offer it.
 */
static void taj_challenge_check(Settings *settings) {
    s8 *balloonThresholds;
    s32 challenge;

    if (settings->worldId != WORLD_CENTRAL_AREA || is_in_tracks_mode() != FALSE) {
        return;
    }
    balloonThresholds = (s8 *) get_misc_asset(ASSET_MISC_16);
    challenge = 0;
    // settings->balloonsPtr[0] is the total balloon count.
    if (!(settings->tajFlags & TAJ_FLAGS_CAR_CHAL_UNLOCKED) && (settings->balloonsPtr[0] >= balloonThresholds[0])) {
        challenge = 1;
    } else if (!(settings->tajFlags & TAJ_FLAGS_HOVER_CHAL_UNLOCKED) &&
               (settings->balloonsPtr[0] >= balloonThresholds[1])) {
        challenge = 2;
    } else if (!(settings->tajFlags & TAJ_FLAGS_PLANE_CHAL_UNLOCKED) &&
               (settings->balloonsPtr[0] >= balloonThresholds[2])) {
        challenge = 3;
    }
    if (challenge) {
        set_taj_voice_line(SOUND_VOICE_TAJ_CHALLENGE_RACE);
        // MIPS masks shift counts mod 32; without &31 this is UB on the PC targets.
        settings->tajFlags |= 1 << ((challenge + 31) & 31);
        set_taj_status(TAJ_TELEPORT);
        set_next_taj_challenge_menu(challenge);
        safe_mark_write_save_file(get_save_file_index());
    }
}

/**
 * Takes the level header and decides which race type to activate.
 * Sets up the racer spawning. Initialising vehicle types, racer count, then
 * spawning them into the world in their assigned order.
 */
void track_setup_racers(Vehicle vehicle, u32 entranceID, s32 playerCount) {
    LevelObjectEntry_Racer *racerEntry;
    Object *racerObj;
    enum GameMode gameMode;
    s32 numPlayers;
    s32 cutsceneID;
    s8 *pushForces;
    s32 prevVehicle;
    s16 objectID;
    RacerSpawnPoint spawns[8];
    Settings *settings;
    s32 port;
    s32 i;
    s32 j;
    u8 raceType;
    LevelHeader *levelHeader;
    Camera *cutsceneCameraSegment;

    gNonDefaultVehicle = FALSE;
    gEventCountdown = 0;
    gFirstTimeFinish = FALSE;
    gNumRacers = 0;
    gCountdownSkipState = COUNTDOWN_SKIP_PENDING;
    set_taj_status(TAJ_WANDER);
    levelHeader = level_header();
    raceType = levelHeader->race_type;
    if (raceType == RACETYPE_CUTSCENE_1 || raceType == RACETYPE_CUTSCENE_2) {
        return;
    }
    if (raceType == RACETYPE_BOSS || raceType & RACETYPE_CHALLENGE) {
        gIsTimeTrial = FALSE;
        gTimeTrialEnabled = FALSE;
    }
    cutsceneID = -1;
    if (is_time_trial_enabled() && raceType == RACETYPE_DEFAULT) {
        cutsceneCameraSegment = cam_get_cameras();
        cutsceneID = cutsceneCameraSegment->zoom;
        cutsceneCameraSegment->zoom = 1;
    }
    gameMode = get_game_mode();
    settings = get_settings();
    pushForces = (s8 *) get_misc_asset(ASSET_MISC_3);
    gPrevTimeTrialVehicle = gSetupPointVehicle;
    if (!(settings->courseFlagsPtr[settings->courseId] & RACE_VISITED)) {
        settings->courseFlagsPtr[settings->courseId] |= RACE_VISITED;
        gCountdownSkipState = COUNTDOWN_SKIP_NEVER;
    }
    if (raceType != RACETYPE_DEFAULT) {
        gCountdownSkipState = COUNTDOWN_SKIP_NEVER;
    }
    racers_collect_setup_points(entranceID, spawns, &vehicle);
    gSetupPointVehicle = vehicle; // UB if all setup points don't have an assigned vehicle ID.
    gPrevTimeTrialVehicle = gSetupPointVehicle;

    // How many racers, and how many of them are human.
    numPlayers = playerCount + 1;
    gNumRacers = 8;
    gTwoActivePlayersInAdventure = FALSE;
    if (race_is_adventure_2P()) {
        numPlayers = 2;
        gTwoActivePlayersInAdventure = TRUE;
        set_scene_viewport_num(VIEWPORT_LAYOUT_2_PLAYERS);
    }
    if (raceType == RACETYPE_HUBWORLD) {
        gTimeTrialEnabled = 0;
    }
    gIsTimeTrial = gTimeTrialEnabled;
    if (gIsTimeTrial) {
        raceType = RACETYPE_HUBWORLD; // Time trials seat racers like a hub world: humans only.
    }
    if (raceType == RACETYPE_HUBWORLD || numPlayers >= 3) {
        gNumRacers = numPlayers;
        if (level_properties_get() == 0 && gSpawnAngleOffset != 0) {
            spawns[0].angle += gSpawnAngleOffset;
            gSpawnAngleOffset = 0;
        }
    } else if (numPlayers == 2) {
        gNumRacers = get_multiplayer_racer_count();
    }
    if (raceType & RACETYPE_CHALLENGE) {
        gNumRacers = 4;
    }
    gBossRaceMode = 0;
    if (raceType == RACETYPE_BOSS) {
        gNumRacers = 2;
        numPlayers = 1;
        gBossRaceMode = 1;
    }
    gRollingDemo = FALSE;
    if (gameMode == GAMEMODE_MENU && raceType == RACETYPE_DEFAULT) {
        gNumRacers = 6;
        gRollingDemo = TRUE;
        gBossRaceMode = 2;
    }
    racers_assign_starting_positions(settings);

    racerEntry = mempool_alloc_safe(sizeof(LevelObjectEntry_Racer), COLOUR_TAG_YELLOW);
    racerEntry->angleY = 0;
    racerEntry->angleX = 0;
    racerEntry->angleZ = 0;
    gIsNonCarRacers = levelHeader->vehicle != VEHICLE_CAR;
    gBossRaceIds[1] = levelHeader->bossRaceID;
    prevVehicle = -1;
    for (i = 0; i < gNumRacers; i++) {
        port = racer_port_for_slot(settings, i, raceType);
        racerEntry->playerIndex = port < numPlayers ? port : 4;
        if (raceType == RACETYPE_HUBWORLD && racerEntry->playerIndex == 4) {
            continue; // No AI in hub worlds.
        }
        vehicle = racer_spawn_vehicle(vehicle, i, racerEntry->playerIndex, numPlayers, levelHeader);
        objectID = racer_object_id(settings, i, port, vehicle);
        racerEntry->common.objectID = objectID;
        racerEntry->common.size = ((objectID & 0x100) >> 1) | 0x10;
        racerEntry->common.x = spawns[i].x;
        racerEntry->common.y = spawns[i].y;
        racerEntry->common.z = spawns[i].z;
        racerEntry->angleY = spawns[i].angle;
        racerObj = spawn_object((LevelObjectEntryCommon *) racerEntry,
                                racer_spawn_flags(racerEntry->playerIndex, numPlayers, vehicle));
        racerObj->trans.rotation.y_rotation = spawns[i].angle;
        (*gRacers)[i] = racerObj;
        gRacersByPosition[i] = racerObj;
        gRacersByPort[port] = racerObj;
        if (prevVehicle != -1 && prevVehicle != (s32) vehicle) {
            gNonDefaultVehicle = TRUE;
        }
        prevVehicle = vehicle;
        racer_init_fields(racerObj, settings, i, port, vehicle, raceType, gameMode, pushForces);
    }
    if (raceType != RACETYPE_BOSS) {
        gBossRaceMode = 0;
    }
    if (gBossRaceMode != 0) {
        gNonDefaultVehicle = FALSE;
    }
    // Remove unwanted objects depending on race type.
    if (get_game_mode() == GAMEMODE_INGAME) {
        for (j = 0; j < gObjectCount; j++) {
            racerObj = gObjPtrList[j];
            i = racerObj->header->flags;
            if (i & OBJECT_HEADER_NO_TIME_TRIAL && gIsTimeTrial) {
                free_object(racerObj);
            } else if (i & OBJECT_HEADER_NO_MULTIPLATER && numPlayers >= 2) {
                free_object(racerObj);
            }
        }
    }
    gGhostObjStaff = NULL;
    timetrial_free_staff_ghost();
    gTimeTrialContPak = -1;
    if (gIsTimeTrial && numPlayers == 1) {
        timetrial_reset_player_ghost();
        gTimeTrialContPak = timetrial_init_player_ghost(PLAYER_ONE);
        gHasGhostToSave = 0;
        if (gTimeTrialVehicle >= VEHICLE_BOSSES) {
            gTimeTrialVehicle = VEHICLE_CAR;
        }
        if (timetrial_valid_player_ghost()) {
            objectID = gRacerObjectTable[gTimeTrialCharacter + (gTimeTrialVehicle * NUM_CHARACTERS)];
            gGhostObjPlayer = racers_spawn_ghost(racerEntry, objectID, &spawns[0]);
        }
        if (timetrial_init_staff_ghost(level_id())) {
            objectID = gRacerObjectTable[(gMapDefaultVehicle * NUM_CHARACTERS) + 8];
            gGhostObjStaff = racers_spawn_ghost(racerEntry, objectID, &spawns[0]);
        }
    }

    gEventCountdown = 100; // Racers settle as if the countdown were running.
    racers_settle();
    if (raceType == RACETYPE_DEFAULT || raceType & RACETYPE_CHALLENGE || gIsTimeTrial || gBossRaceMode != 0) {
        gEventCountdown = 80;
    } else {
        gEventCountdown = 0;
    }
    if (raceType == RACETYPE_DEFAULT && (playerCount + 1) == 1 && is_in_adventure_two() == FALSE &&
        !race_is_adventure_2P()) {
        racers_spawn_position_arrows(racerEntry);
    }
    gRaceEndTimer = 0;
    gRaceFinishTriggered = FALSE;
    set_next_taj_challenge_menu(0);
    taj_challenge_check(settings);
    gBossRaceIds[0] = TRUE;
    if (cutsceneID >= 0) {
        cutsceneCameraSegment->zoom = cutsceneID;
    }
    // Menu demos will skip straight to the action.
    if (racetype_demo()) {
        rumble_init(FALSE);
        gEventCountdown = 0;
        level_music_start(1.0f);
    }
    //!@bug: Free timer is already 0 when loading levels.
    mempool_free_timer(0);
    mempool_free(racerEntry);
    mempool_free_timer(2);
}

/**
 * Return an error status for the controller pak.
 * Categorises multiple different controller pak messages into one for fewer cases.
 */
s32 get_contpak_error(void) {
    // gTimeTrialContPak is likely an SIDeviceStatus value, but not 100% sure yet.
    switch (gTimeTrialContPak) {
        case CONTROLLER_PAK_NOT_FOUND:
            return CONTPAK_ERROR_MISSING;
        case CONTROLLER_PAK_RUMBLE_PAK_FOUND:
            return CONTPAK_ERROR_NONE;
        case CONTROLLER_PAK_INCONSISTENT:
        case CONTROLLER_PAK_WITH_BAD_ID:
            return CONTPAK_ERROR_DAMAGED;
        case CONTROLLER_PAK_FULL:
        case CONTROLLER_PAK_NO_ROOM_FOR_GHOSTS:
            return CONTPAK_ERROR_FULL;
        case CONTROLLER_PAK_GOOD:
        case CONTROLLER_PAK_CHANGED:
        case CONTROLLER_PAK_SWITCH_TO_RUMBLE:
            return timetrial_ghost_full();
        default:
            return CONTPAK_ERROR_NONE;
    }
}

void instShowBearBar(void) {
    gSpawnAngleOffset = 0x8000;
}

/** Whether the player chose a vehicle other than the level's default (the physics run faster then). */
s8 is_nondefault_vehicle(void) {
    return gNonDefaultVehicle;
}

/**
 * Returns true if currently in a rolling demo.
 */
s8 racetype_demo(void) {
    return gRollingDemo;
}

/**
 * Returns true if we're in a race started by P2 in adventure mode.
 */
s8 is_race_started_by_player_two(void) {
    if (gTwoActivePlayersInAdventure) {
        return gLeadPlayerIndex;
    } else {
        return FALSE;
    }
}

/**
 * Returns true if P2 is the lead player in adventure mode.
 */
s8 is_player_two_in_control(void) {
    return gIsP2LeadPlayer;
}

/**
 * Swaps the lead player index during 2P adventure mode.
 */
void toggle_lead_player_index(void) {
    gLeadPlayerIndex = 1 - gLeadPlayerIndex;
    gTwoActivePlayersInAdventure = FALSE;
}

/**
 * Resets the lead player index.
 * Official name: fontUseFont (why?)
 */
void reset_lead_player_index(void) {
    gLeadPlayerIndex = 0;
    gTwoActivePlayersInAdventure = FALSE;
}

/**
 * Return true if there exist players piloting planes or hovercraft.
 * Used to determine whether to use certain zippers.
 */
s8 find_non_car_racers(void) {
    return gIsNonCarRacers;
}

/**
 * Return true if the silver coin race mode is active.
 * Used to determine whether to spawn silver coins.
 */
s8 check_if_silver_coin_race(void) {
    return gIsSilverCoinRace;
}

/**
 * Store some things about the racer object then remove it.
 */
void despawn_player_racer(Object *obj, s32 vehicleID) {
    gTransformObject = obj;
    gTransformTimer = 4;
    gOverworldVehicle = vehicleID;
    gTransformPosX = obj->trans.x_position;
    gTransformPosY = obj->trans.y_position;
    gTransformPosZ = obj->trans.z_position;
    gTransformAngleY = obj->trans.rotation.y_rotation;
    free_object(obj);
    gNumRacers = 0;
}

/**
 * Spawn a new racer object and set the initial position and rotation to what was set
 * before the old one was freed.
 */
void transform_player_vehicle(void) {
    Object *player;
    LevelObjectEntry8000E2B4 spawnObj;
    Settings *settings;
    Object_Racer *racer;
    s16 objectID;

    if (gTransformTimer == 0) {
        return;
    }
    gTransformTimer--;
    if (gTransformTimer) {
        return;
    }
    settings = get_settings();
    spawnObj.playerIndex = 0;
    spawnObj.common.size = 16;
    if (gOverworldVehicle < VEHICLE_BOSSES) {
        objectID =
            ((s16 *) gRacerObjectTable)[settings->racers[PLAYER_ONE].character + gOverworldVehicle * NUM_CHARACTERS];
    } else {
        objectID = gRacerObjectTable[gOverworldVehicle + 45];
    }
    set_level_default_vehicle(gOverworldVehicle);
    spawnObj.common.size |= (objectID & 0x100) >> 1;
    spawnObj.angleX = 0;
    spawnObj.angleZ = 0;
    spawnObj.common.objectID = objectID;
    spawnObj.common.x = gTransformPosX;
    spawnObj.common.y = gTransformPosY;
    spawnObj.common.z = gTransformPosZ;
    spawnObj.angleY = gTransformAngleY;
    set_taj_status(TAJ_DIALOGUE);
    player = spawn_object(&spawnObj.common, OBJECT_SPAWN_NO_LODS | OBJECT_SPAWN_UNK01);
    gNumRacers = 1;
    (*gRacers)[PLAYER_ONE] = player;
    gRacersByPort[PLAYER_ONE] = player;
    *gRacersByPosition = player;
    racer = player->racer;
    racer->vehicleID = gOverworldVehicle;
    racer->vehicleIDPrev = gOverworldVehicle;
    racer->racerIndex = 0;
    racer->characterId = settings->racers[PLAYER_ONE].character;
    racer->playerIndex = 0;
    racer->vehicleSound = 0;
    if (get_filtered_cheats() & CHEAT_BIG_CHARACTERS) {
        player->trans.scale *= 1.4f;
    }
    if (get_filtered_cheats() & CHEAT_SMALL_CHARACTERS) {
        player->trans.scale *= 0.714f;
    }
    player->level_entry = NULL;
    player->trans.rotation.y_rotation = gTransformAngleY;
    player->trans.y_position = gTransformPosY;
}

/**
 * Enables or Disables time trial mode.
 */
void set_time_trial_enabled(s32 status) {
    gTimeTrialEnabled = status;
}

/**
 * Returns the value in gTimeTrialEnabled.
 */
u8 is_time_trial_enabled(void) {
    return gTimeTrialEnabled;
}

/**
 * Returns true if the player is currently performaing a time trial.
 */
u8 is_in_time_trial(void) {
    return gIsTimeTrial;
}



/** Removes an entry from the level's object spawn list, shifting what follows and fixing object pointers into it. */
void level_entry_remove(LevelObjectEntryCommon *arg0) {
    u8 *src;
    u8 *dst;
    ptrint_t end;
    ptrint_t sp30[2];
    s32 size;
    s32 i;
    s32 pad;
    s32 sp1C;

    size = arg0->size & 0x3F;

    sp30[0] = (ptrint_t) gObjectMapSpawnList[0] + gObjectMapSize[0];
    sp30[1] = (ptrint_t) gObjectMapSpawnList[1] + gObjectMapSize[1];

    if ((ptrint_t) arg0 >= (ptrint_t) gObjectMapSpawnList[0] && (ptrint_t) arg0 < sp30[0]) {
        sp1C = 0;
    } else if ((ptrint_t) arg0 >= (ptrint_t) gObjectMapSpawnList[1] && (ptrint_t) arg0 < sp30[1]) {
        sp1C = 1;
    }
#ifdef AVOID_UB
    else {
        sp1C = 0;
    }
#endif

    dst = (u8 *) arg0;
    src = (u8 *) ((ptrint_t) arg0 + size);
    end = sp30[sp1C];
    if ((uintptr_t) src < (uintptr_t) end) {
        do {
            *dst++ = *src++;
        } while ((uintptr_t) src != (uintptr_t) end);
    }
    gObjectMapSize[sp1C] -= size;

    for (i = 0; i < gObjectCount; i++) {
        Object *obj = gObjPtrList[i];
        if (obj != NULL && obj->level_entry != NULL) {
            if ((ptrint_t) arg0 < (ptrint_t) obj->level_entry && (ptrint_t) obj->level_entry < end) {
                obj->level_entry = (LevelObjectEntry *) ((ptrint_t) obj->level_entry - size);
            } else if ((ptrint_t) arg0 == (ptrint_t) obj->level_entry) {
                obj->level_entry = NULL;
            }
        }
    }
}

/** Replaces one spawn-list entry with another of a possibly different size. */
void level_entry_replace(u8 *arg0, u8 *arg1) {
    s32 arg0Value;
    s32 arg0Value2;
    s32 arg1Value;
    u8 *var_a3;
    u8 *var_t0;
    u8 *temp_t2;
    u8 *var_a2;
    s32 i;
    s32 j;
    uintptr_t k;

    arg0Value = arg0[1] & 0x3F;
    arg0Value2 = arg0Value;
    arg1Value = arg1[1] & 0x3F;
    i = gObjectMapIndex;
    var_a3 = (u8 *) gObjectMap[i] + gObjectMapSize[i];
    var_a3 += 16;

    if (arg1Value < arg0Value2) {
        var_a2 = arg0 + arg1Value;
        var_t0 = arg0 + arg0Value2;
        k = (uintptr_t) var_a3;
        while (((uintptr_t) var_t0) < k) {
            *var_a2 = *var_t0;
            var_a2++;
            var_t0++;
        }
    } else if (arg0Value2 < arg1Value) {
        var_a2 = var_a3 + arg1Value;
        var_a2 -= arg0Value2;
        var_t0 = var_a3;
        k = (uintptr_t) (arg0 + arg1Value);
        while (k < ((uintptr_t) var_a2)) {
            var_a2--;
            var_t0--;
            *var_a2 = *var_t0;
        }
    }

    j = 0;
    do {
        arg0[j] = arg1[j];
        j++;
    } while (j < arg1Value);

    gObjectMapSize[i] += arg1Value - arg0Value;
}


/**
 * Returns the object at the current offset by ID.
 * Official name: objGetObject
 */
Object *get_object(s32 index) {
    if (index < 0 || index >= gObjectCount) {
        stubbed_printf("ObjList (Part) Overflow %d!!!\n");
        return 0;
    }
    return gObjPtrList[index];
}

/**
 * Return the standard object list index and how many objects are in that list.
 */
Object **objGetObjList(s32 *arg0, s32 *cnt) {
    *arg0 = gObjectListStart;
    *cnt = gObjectCount;
    return gObjPtrList;
}

/**
 * Return the number of objects currently existing.
 */
UNUSED s32 obj_count(void) {
    return gObjectCount;
}

/**
 * Return the number of particles currently existing.
 */
UNUSED s32 particle_count(void) {
    return gParticleCount;
}

void add_particle_to_entity_list(Object *obj) {
    obj->trans.flags |= OBJ_FLAGS_PARTICLE;
    readout_log_push(obj->headerType | (OBJ_FLAGS_PARTICLE | OBJ_FLAGS_INVISIBLE));
    gObjPtrList[gObjectCount++] = obj;
    if (1) {} // Fakematch
    gParticleCount++;
}

// Official Name: objSetupObject
/**
 * Some behaviours show a different model depending on the save: which character was picked,
 * how many trophies are held, whether the amulet is complete. Sets the first model index to
 * load and the number of models, and may rewrite the header's model list.
 */
static void spawn_pick_models(Object *obj, Settings *settings, s32 spawnFlags, s32 *firstModel, s32 *modelCount,
                              s32 *modelType) {
    s32 i;
    s32 trophies;

    i = 0;
    switch (obj->header->behaviorId) {
        case BHV_PARK_WARDEN:
            model_anim_offset(7);
            break;
        case BHV_ANIMATED_OBJECT_4:
            i = get_character_id_from_slot(PLAYER_ONE);
            obj->modelIndex = i;
            *modelCount = i + 1;
            break;
        case BHV_UNK_5B:
            i = (settings->trophies >> ((settings->worldId - 1) << 1));
            i = i & 0x3;
            if (i) {
                i--;
                *modelCount = i + 1;
                obj->modelIndex = i;
            }
            break;
        case BHV_DYNAMIC_LIGHT_OBJECT_2:
            i = settings->wizpigAmulet;
            *modelCount = i + 1;
            obj->modelIndex = settings->wizpigAmulet;
            break;
        case BHV_ROCKET_SIGNPOST_2:
            // Count the worlds with a gold trophy.
            trophies = settings->trophies;
            for (*modelCount = 0; *modelCount < 4; (*modelCount)++) {
                if ((trophies & 3) == 3) {
                    i++;
                }
                trophies >>= 2;
            }
            obj->modelIndex = i;
            *modelCount = i + 1;
            *modelType = trophies; //!@bug the model type is clobbered with the shifted-out trophy bits (always 0)
            break;
        case BHV_GOLDEN_BALLOON:
            *modelCount = 1;
            if (is_in_adventure_two()) {
                obj->header->modelIds[0] = obj->header->modelIds[1];
            }
            obj->header->numberOfModelIds = 1;
            break;
    }
    if (!(spawnFlags & OBJECT_SPAWN_UNK02)) {
        switch (obj->objectID) {
            case ASSET_OBJECT_ID_POLYGOLDBALOON:
                *modelCount = 1;
                if (is_in_adventure_two()) {
                    obj->header->modelIds[0] = obj->header->modelIds[1];
                }
                obj->header->numberOfModelIds = 1;
                break;
            case ASSET_OBJECT_ID_LEVELDOOR:
                if (is_in_adventure_two()) {
                    for (i = 0; i < 5; i++) {
                        obj->header->modelIds[i] = obj->header->modelIds[i + 5];
                    }
                }
                *modelCount = 5;
                obj->header->numberOfModelIds = 5;
                i = 0;
                break;
        }
    }
    *firstModel = i;
}

/**
 * Load the object's 3D models, textures or sprites. Returns FALSE when any of them is missing.
 */
static s32 spawn_load_assets(Object *obj, s32 firstModel, s32 modelCount, s32 modelType, s32 spawnFlags,
                             s32 behaviourFlags) {
    s32 i;
    s32 loaded;

    loaded = TRUE;
    for (i = firstModel; i < modelCount; i++) {
        if (modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
            if (i == 0 && (spawnFlags & OBJECT_SPAWN_UNK04)) {
                obj->modelInstances[i] = NULL;
            } else if (i == 1 && (spawnFlags & OBJECT_SPAWN_UNK08)) {
                obj->modelInstances[i] = NULL;
            } else {
                obj->modelInstances[i] = object_model_init(obj->header->modelIds[i], behaviourFlags);
                if (obj->modelInstances[i] == NULL) {
                    loaded = FALSE;
                }
            }
        } else if (modelType == OBJECT_MODEL_TYPE_MISC) {
            obj->textures[i] = load_texture(obj->header->modelIds[i]);
            if (obj->textures[i] == NULL) {
                loaded = FALSE;
            }
        } else {
            obj->sprites[i] = tex_load_sprite(obj->header->modelIds[i], 10);
            if (obj->sprites[i] == NULL) {
                loaded = FALSE;
            }
        }
    }
    return loaded;
}

/**
 * Undo a partial spawn: release the textures the shadow and water effect loaded (as many as
 * `texturesToFree` of them), the object's models and its header.
 */
static void spawn_abort(Object *obj, s32 modelCount, s32 modelType, s32 headerType, s32 texturesToFree, s32 stage) {
    if (texturesToFree > 0 && gLastObjectTextureA != NULL) {
        tex_free(gLastObjectTextureA);
    }
    if (texturesToFree > 1 && gLastObjectTextureB != NULL) {
        tex_free(gLastObjectTextureB);
    }
    objFreeAssets(obj, modelCount, modelType);
    try_free_object_header(headerType);
    stubbed_printf("ObjSetupObject(%d) Memory fail!!\n", stage);
}

/**
 * Lay the optional property blocks (shading, shadow, water effect, interaction, collision,
 * attach points, emitters, lights) out after the model instances. Returns the end address,
 * or NULL with *failStage set when a block could not be set up.
 */
static u8 *spawn_layout_properties(Object *obj, u8 *address, s32 behaviourFlags, s32 *failStage) {
    s32 size;

    address += get_object_property_size(obj, address);
    gLastObjectTextureA = NULL;
    gLastObjectTextureB = NULL;

    if (behaviourFlags & OBJECT_BEHAVIOUR_SHADED) {
        address += init_object_shading(obj, (ShadeProperties *) address);
    }
    if (behaviourFlags & OBJECT_BEHAVIOUR_SHADOW) {
        size = init_object_shadow(obj, (ShadowData *) address);
        address += size;
        if (size == 0) {
            *failStage = 5;
            return NULL;
        }
    }
    if (behaviourFlags & OBJECT_BEHAVIOUR_WATER_EFFECT) {
        size = init_object_water_effect(obj, (WaterEffect *) address);
        address += size;
        if (size == 0) {
            *failStage = 6;
            return NULL;
        }
    }
    if (behaviourFlags & OBJECT_BEHAVIOUR_INTERACTIVE) {
        address += init_object_interaction_data(obj, (ObjectInteraction *) address);
    }
    if (behaviourFlags & OBJECT_BEHAVIOUR_COLLIDABLE) {
#ifdef GBI_FLOAT_MTX
        // ObjectCollision embeds MtxF matrices[4] at offset 0; under the float
        // matrix ABI those are copied with SH4 paired fmov.d (8-byte), so the
        // struct must start 8-aligned. The raw arena address isn't guaranteed
        // aligned. N64/fixed builds keep the original layout (matching).
        address = align8(address);
#endif
        address += obj_init_collision(obj, (ObjectCollision *) address);
    }
    if (obj->header->attachPointCount > 0 && obj->header->attachPointCount < 10) {
        obj->attachPoints = (AttachPoint *) address;
        address += sizeof(AttachPoint);
    }
    if (obj->header->particleCount > 0) {
        address += obj_init_emitter(obj, (ParticleEmitter *) address);
    }
    if (obj->header->numLightSources > 0) {
        obj->lightData = (ObjectLight **) address;
        address += obj->header->numLightSources * sizeof(ObjectLight *);
    }
    return address;
}

/**
 * The object was built in the scratch heap; its property pointers still point there.
 * Rebase each one onto the object's final home.
 */
static void spawn_relocate_pointers(Object *obj) {
#define REBASE(field, type) \
    obj->field = (type) (((uintptr_t) obj + (uintptr_t) obj->field) - (uintptr_t) gSpawnObjectHeap)
    if (obj->waterEffect != NULL) {
        REBASE(waterEffect, WaterEffect *);
    }
    if (obj->shadow != NULL) {
        REBASE(shadow, ShadowData *);
    }
    if (obj->shading != NULL) {
        REBASE(shading, ShadeProperties *);
    }
    if (obj->anyBehaviorData != NULL) {
        REBASE(anyBehaviorData, void *);
    }
    if (obj->interactObj != NULL) {
        REBASE(interactObj, ObjectInteraction *);
    }
    if (obj->collisionData != NULL) {
        REBASE(collisionData, ObjectCollision *);
    }
    if (obj->attachPoints != NULL) {
        REBASE(attachPoints, AttachPoint *);
    }
    if (obj->header->particleCount > 0) {
        REBASE(particleEmitter, ParticleEmitter *);
    }
    if (obj->header->numLightSources > 0) {
        REBASE(lightData, ObjectLight **);
    }
#undef REBASE
    obj->modelInstances = (ModelInstance **) &obj[1];
}

/**
 * Build an object from a level entry. The object is assembled in a scratch heap first,
 * because its size depends on which optional property blocks the behaviour needs, then
 * copied into the object pool and registered.
 */
Object *spawn_object(LevelObjectEntryCommon *entry, s32 spawnFlags) {
    s32 objType;
    Settings *settings;
    s32 i;
    s32 behaviourFlags;
    s16 headerType;
    u8 *address;
    s32 sizeOfobj;
    s32 failStage;
    Object *curObj;
    Object *prevObj;
    s32 firstModel;
    s32 assetCount;

    settings = get_settings();
    objType = entry->objectID | ((entry->size & 0x80) << 1);
    update_object_stack_trace(OBJECT_SPAWN, objType);
    if (spawnFlags & OBJECT_SPAWN_UNK02) {
        headerType = objType;
    } else {
        headerType = gAssetsLvlObjTranslationTable[objType];
    }
    if (headerType >= gAssetsObjectHeadersTableLength) {
        headerType = 0;
    }

    for (i = 0; i < OBJECT_BLUEPRINT_SIZE / 4; i++) {
        gSpawnObjectHeap[i] = NULL;
    }

    curObj = (Object *) gSpawnObjectHeap;
    curObj->trans.flags = OBJ_FLAGS_UNK_0002;
    curObj->header = load_object_header(headerType);
    if (curObj->header == NULL) {
        return NULL;
    }
    if (curObj->header->flags & HEADER_FLAGS_UNK_0080) {
        curObj->trans.flags |= OBJ_FLAGS_UNK_0080;
    }
    if (curObj->header->behaviorId == BHV_ROCKET_SIGNPOST && (settings->cutsceneFlags & CUTSCENE_LIGHTHOUSE_ROCKET)) {
        update_object_stack_trace(OBJECT_SPAWN, -1);
        stubbed_printf("ObjSetupObject(1) Memory fail!!\n");
        return NULL;
    }

    curObj->trans.x_position = entry->x;
    curObj->trans.y_position = entry->y;
    curObj->trans.z_position = entry->z;
    curObj->segmentID = get_level_segment_index_from_position(curObj->trans.x_position, curObj->trans.y_position,
                                                              curObj->trans.z_position);

    curObj->headerType = headerType;
    curObj->level_entry = (LevelObjectEntry *) entry;
    curObj->objectID = objType;
    readout_log_push(objType);
    curObj->trans.scale = curObj->header->scale;
    curObj->cullRadius = curObj->header->cullRadius * curObj->trans.scale;
    curObj->opacity = 255;
    behaviourFlags = obj_init_property_flags(curObj->header->behaviorId);
    curObj->header->instanceCount++;

    assetCount = curObj->header->numberOfModelIds;
    objType = curObj->header->modelType;
    curObj->modelInstances = (ModelInstance **) &curObj[1];
    if (spawnFlags & OBJECT_SPAWN_NO_LODS) {
        assetCount = 1;
    }
    spawn_pick_models(curObj, settings, spawnFlags, &firstModel, &assetCount, &objType);
    if (!spawn_load_assets(curObj, firstModel, assetCount, objType, spawnFlags, behaviourFlags)) {
        spawn_abort(curObj, assetCount, objType, headerType, 0, 2);
        return NULL;
    }

    address = spawn_layout_properties(curObj, (u8 *) &curObj->modelInstances[curObj->header->numberOfModelIds],
                                      behaviourFlags, &failStage);
    if (address == NULL) {
        spawn_abort(curObj, assetCount, objType, headerType, failStage == 6 ? 1 : 0, failStage);
        return NULL;
    }

    sizeOfobj = (s32) ((uintptr_t) address - (uintptr_t) curObj);
    prevObj = curObj;
    curObj = mempool_alloc_pool((MemoryPoolSlot *) gObjectMemoryPool, sizeOfobj);
    if (curObj == NULL) {
        spawn_abort(prevObj, assetCount, objType, headerType, 2, 3);
        return NULL;
    }

    if (sizeOfobj & 0xF) {
        sizeOfobj = _ALIGN16(sizeOfobj);
    }
    sizeOfobj >>= 2;
    for (i = 0; i < sizeOfobj; i++) {
        ((u32 *) curObj)[i] = gSpawnObjectHeap[i];
    }
    spawn_relocate_pointers(curObj);

    if (spawnFlags & OBJECT_SPAWN_UNK01) {
        gObjPtrList[gObjectCount++] = curObj;
        if (gObjectCount > OBJECT_SLOT_COUNT) {
            stubbed_printf("ObjList Overflow %d!!!\n", gObjectCount);
        }
    }
    run_object_init_func(curObj, (LevelObjectEntry *) entry, 0);
    if (curObj->interactObj != NULL) {
        curObj->interactObj->x_position = curObj->trans.x_position;
        curObj->interactObj->y_position = curObj->trans.y_position;
        curObj->interactObj->z_position = curObj->trans.z_position;
    }
    if (curObj->header->attachPointCount > 0 && curObj->header->attachPointCount < 10 && obj_init_attachpoint(curObj)) {
        spawn_abort(curObj, assetCount, objType, headerType, 2, 4);
        mempool_free(curObj);
        if (spawnFlags & OBJECT_SPAWN_UNK01) {
            gObjectCount--;
        }
        return NULL;
    }
    if (curObj->header->numLightSources > 0) {
        light_setup_light_sources(curObj);
    }
    model_anim_offset(0);
    update_object_stack_trace(OBJECT_SPAWN, -1);
    return curObj;
}

/**
 * Run functions that will attempt to free the graphics data of the object
 * if there are no other references.
 */
void objFreeAssets(Object *obj, s32 count, s32 objType) {
    s32 i;
    if (objType == OBJECT_MODEL_TYPE_3D_MODEL) { // 3D model
        for (i = 0; i < count; i++) {
            if (obj->modelInstances[i] != NULL) {
                free_3d_model((ModelInstance *) obj->modelInstances[i]);
            }
        }
    } else if (objType == OBJECT_MODEL_TYPE_MISC) {
        for (i = 0; i < count; i++) {
            if (obj->textures[i] != NULL) {
                tex_free((TextureHeader *) obj->textures[i]);
            }
        }
    } else { // Sprite
        for (i = 0; i < count; i++) {
            if (obj->sprites[i] != NULL) {
                sprite_free((Sprite *) obj->sprites[i]);
            }
        }
    }
}
/**
 * Official Name: lightSetupLightSources
 */
void light_setup_light_sources(Object *obj) {
    s32 i;
    for (i = 0; i < obj->header->numLightSources; i++) {
        obj->lightData[i] = light_add_from_object_header(obj, &obj->header->lightSources[i]);
    }
}

/**
 * Sets the shading properties of the object.
 */
s32 init_object_shading(Object *obj, ShadeProperties *shadeData) {
    s32 returnSize;
    s32 i;

    obj->shading = shadeData;
    returnSize = 0;
    if (obj->header->modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
        for (i = 0; obj->modelInstances[i] == NULL; i++) {}
        if (obj->modelInstances[i] != NULL && obj->modelInstances[i]->objModel->normals != NULL) {
            set_shading_properties(obj->shading, obj->header->shadeAmbient, obj->header->shadeDiffuse, 0,
                                   obj->header->shadeAngleY, obj->header->shadeAngleZ);
            if (obj->header->shadeIntensityy != 0) {
                obj->shading->lightR = obj->header->shadeLightR;
                obj->shading->lightG = obj->header->shadeLightG;
                obj->shading->lightB = obj->header->shadeLightB;
                obj->shading->lightIntensity = obj->header->shadeIntensityy;
                obj->shading->lightDirX = -(obj->shading->shadowDirX >> 1);
                obj->shading->lightDirY = -(obj->shading->shadowDirY >> 1);
                obj->shading->lightDirZ = -(obj->shading->shadowDirZ >> 1);
            }
            returnSize = sizeof(ShadeProperties);
        }
    } else if (obj->header->modelType == OBJECT_MODEL_TYPE_SPRITE_BILLBOARD) {
        obj->shading->brightness = 1.0f;
        shadeData->lightR = 255;
        shadeData->lightG = 255;
        shadeData->lightB = 255;
        shadeData->lightIntensity = 0;
        returnSize = 8;
    }
    if (returnSize == 0) {
        obj->shading = NULL;
    }
    return (returnSize & ~3) + 4;
}

s32 obj_init_attachpoint(Object *obj) {
    Object *attachObj;
    AttachPoint *attachPoint;
    s32 i;
    s32 failed;

    attachPoint = obj->attachPoints;
    attachPoint->count = obj->header->attachPointCount;
    attachPoint->count = attachPoint->count; // Fakematch?
    failed = FALSE;
    for (i = 0; i < attachPoint->count; i++) {
        attachPoint->obj[i] = obj_spawn_attachment(obj->header->vehiclePartIds[i ^ 0]); // i ^ 0 fakematch
        if (attachPoint->obj[i] == NULL) {
            failed = TRUE;
        }
    }
    if (failed) {
        for (i = 0; i < attachPoint->count; i++) {
            attachObj = attachPoint->obj[i];
            if (attachObj != NULL) {
                objFreeAssets(attachObj, attachObj->header->numberOfModelIds, attachObj->header->modelType);
                try_free_object_header(attachObj->headerType);
                mempool_free(attachObj);
            }
        }
        return TRUE;
    }
    attachPoint->unk2C = obj->header->vehiclePartIndices;
    return FALSE;
}

s32 obj_init_emitter(Object *obj, ParticleEmitter *emitter) {
    ObjHeaderParticleEntry *particleDataEntry;
    s32 i;

    obj->particleEmitter = emitter;
    particleDataEntry = obj->header->objectParticles;
    for (i = 0; i < obj->header->particleCount; i++) {
        if ((particleDataEntry[i].upper & 0xFFFF0000) == 0xFFFF0000) {
            emitter_init(&obj->particleEmitter[i], (particleDataEntry[i].upper >> 8) & 0xFF,
                         particleDataEntry[i].upper & 0xFF);
        } else {
            emitter_init_with_pos(&obj->particleEmitter[i], (particleDataEntry[i].upper >> 0x18) & 0xFF,
                                  (particleDataEntry[i].upper >> 0x10) & 0xFF, particleDataEntry[i].upper & 0xFFFF,
                                  (particleDataEntry[i].lower >> 0x10) & 0xFFFF, particleDataEntry[i].lower & 0xFFFF);
        }
    }
    return ((obj->header->particleCount * sizeof(ParticleEmitter)) + 3) & ~3;
}

/**
 * Assigns shadow data to an object. Loads and assigns the shadow texture, too.
 * Returns zero if the texture is missing.
 */
s32 init_object_shadow(Object *obj, ShadowData *shadow) {
    ObjectHeader *objHeader;

    obj->shadow = shadow;
    shadow->texture = NULL;
    objHeader = obj->header;
    if (objHeader->shadowGroup) {
        shadow->texture = load_texture(objHeader->shadowTextureId);
        objHeader = obj->header;
    }
    shadow->scale = objHeader->shadowScale;
    shadow->meshStart = -1;
    gLastObjectTextureA = shadow->texture;
    if (obj->header->shadowGroup && shadow->texture == NULL) {
        return 0;
    }
    return sizeof(ShadowData);
}

/**
 * Assigns water effect data to an object. Loads and assigns the effect texture, too.
 * Returns zero if the texture is missing.
 */
s32 init_object_water_effect(Object *obj, WaterEffect *waterEffect) {
    obj->waterEffect = waterEffect;
    waterEffect->scale = obj->header->waterEffectScale;
    waterEffect->textureFrame = 0;
    waterEffect->animationSpeed = obj->header->waterEffectAnimSpeed >> 8;
    waterEffect->texture = NULL;
    if (obj->header->waterEffectGroup) {
        waterEffect->texture = load_texture(obj->header->waterEffectTextureId);
    }
    waterEffect->meshStart = -1;
    gLastObjectTextureB = waterEffect->texture;
    if (obj->header->waterEffectGroup && waterEffect->texture == NULL) {
        return 0;
    }
    return sizeof(WaterEffect);
}

/**
 * Writes object interatction properties to the object.
 * Returns 40, to offset the pointer position used
 */
s32 init_object_interaction_data(Object *obj, ObjectInteraction *interactObj) {
    obj->interactObj = interactObj;
    interactObj->distance = 0xFF;
    return sizeof(ObjectInteraction);
}

/**
 * Sets up collision surface data for the object model.
 */
s32 obj_init_collision(Object *obj, ObjectCollision *colData) {
    obj->collisionData = colData;
    object_init_collision(obj);
    return sizeof(ObjectCollision);
}

/**
 * Attempts to spawn an attachment object.
 * Similar to the regular object spawning function, but cut down considerably.
 */
Object *obj_spawn_attachment(s32 objID) {
    s32 modelType;
    Object *object;
    ObjectHeader *objHeader;
    s32 objSize;
    s32 i;
    s32 failedToLoadModel;
    s8 numModelIds;
    u8 *objectAsRawBytes;

    if (objID >= gAssetsObjectHeadersTableLength) {
        objID = 0;
    }
    objHeader = load_object_header(objID);
    if (objHeader == NULL) {
        stubbed_printf("obj_spawn_attachment(%d): header load failed\n", objID);
        return NULL;
    }
    // The N64 source spells sizeof(Object) as 0x80 and a model pointer as 4 bytes.
    objSize = (objHeader->numberOfModelIds * sizeof(ModelInstance *)) + sizeof(Object);
    object = (Object *) mempool_alloc(objSize, COLOUR_TAG_BLUE);
    if (object == NULL) {
        stubbed_printf("obj_spawn_attachment(%d): object alloc of %d bytes failed\n", objID, objSize);
        try_free_object_header(objID);
        return NULL;
    }

    // clang-format off
    objectAsRawBytes = (u8 *) object;
    for (i = 0; i < objSize; i++) { objectAsRawBytes[i] = 0; } // Must be one line! (Why not use bzero?)
    // clang-format on

    object->trans.flags = OBJ_FLAGS_UNK_0002;
    object->header = objHeader;
    object->headerType = objID;
    object->objectID = objID;
    object->trans.scale = objHeader->scale;
    if (objHeader->flags & HEADER_FLAGS_UNK_0080) {
        object->trans.flags |= OBJ_FLAGS_UNK_0080;
    }
    numModelIds = object->header->numberOfModelIds;
    modelType = object->header->modelType;
    object->modelInstances = (ModelInstance **) &object[1];

    failedToLoadModel = FALSE;
    if (modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
        for (i = 0; i < numModelIds; i++) {
            object->modelInstances[i] = object_model_init(object->header->modelIds[i], 0);
            if (object->modelInstances[i] == NULL) {
                failedToLoadModel = TRUE;
            }
        }
    } else {
        for (i = 0; i < numModelIds; i++) {
            object->sprites[i] = tex_load_sprite(object->header->modelIds[i], 10);
            if (object->sprites[i] == NULL) {
                failedToLoadModel = TRUE;
            }
        }
    }
    if (failedToLoadModel) {
        stubbed_printf("obj_spawn_attachment(%d): model/sprite load failed (type %d, %d ids)\n", objID, modelType,
                       numModelIds);
        objFreeAssets(object, numModelIds, modelType);
        try_free_object_header(objID);
        mempool_free(object);
        return NULL;
    }

    return object;
}

/**
 * Adds the object to the free list.
 * This object will be deallocated on the next update cycle.
 * Official Name: objFreeObject
 */
void free_object(Object *object) {
    readout_log_push(object->objectID | OBJ_FLAGS_PARTICLE);
    gParticlePtrList[gFreeListCount] = object;
    gFreeListCount++;
}

/**
 * Return the length of the object ID table.
 */
UNUSED s32 obj_table_ids(void) {
    return gAssetsLvlObjTranslationTableLength;
}

/**
 * Return true if the object ID is not higher than the header table length.
 */
UNUSED s32 obj_id_valid(s32 arg0) {
    return (gAssetsLvlObjTranslationTable[arg0] < gAssetsObjectHeadersTableLength);
}

/*
 * Clears all existing particles from the object list
 */
void gParticlePtrList_flush(void) {
    s32 j, i, search_indx;
    Object *searchObj;

    for (i = 0; i < gFreeListCount; i++) {
        search_indx = -1;
        searchObj = gParticlePtrList[i];

        for (j = 0; j < gObjectCount; j++) {
            if (searchObj == gObjPtrList[j]) {
                search_indx = j;
            }
        }

        // if object found
        if (search_indx != -1) {
            if (search_indx < gFirstActiveObjectId) {
                gFirstActiveObjectId--;
            }
            gObjectCount--;
            if (0) {} // Fakematch
            for (j = search_indx; j < gObjectCount; j++) {
                gObjPtrList[j] = gObjPtrList[j + 1];
            }
        }
        obj_destroy(searchObj, 0);
    }
    gFreeListCount = 0;
}

/**
 * Objects attached to this one (riders, held items) are owned by it: free their models,
 * headers and memory.
 */
static void obj_destroy_attachments(Object *obj) {
    Object *attached;
    s32 i;
    s32 j;

    for (i = 0; i < obj->attachPoints->count; i++) {
        attached = obj->attachPoints->obj[i];
        if (attached->header->modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
            for (j = 0; j < attached->header->numberOfModelIds; j++) {
                free_3d_model((ModelInstance *) &attached->modelInstances[j]->objModel);
            }
        } else {
            for (j = 0; j < attached->header->numberOfModelIds; j++) {
                sprite_free(attached->sprites[j]);
            }
        }
        try_free_object_header(attached->headerType);
        mempool_free(attached);
    }
}

/**
 * Release the object's own graphics: shadow and water textures, then its models, textures or
 * sprites.
 */
static void obj_free_models(Object *obj) {
    s32 j;

    if (obj->shadow != NULL && obj->shadow->texture != NULL) {
        tex_free(obj->shadow->texture);
    }
    if (obj->waterEffect != NULL && obj->waterEffect->texture != NULL) {
        tex_free(obj->waterEffect->texture);
    }
    if (obj->header->modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
        for (j = 0; j < obj->header->numberOfModelIds; j++) {
            if (obj->modelInstances[j] != NULL) {
                free_3d_model(obj->modelInstances[j]);
            }
        }
    } else if (obj->header->modelType == OBJECT_MODEL_TYPE_MISC) {
        for (j = 0; j < obj->header->numberOfModelIds; j++) {
            tex_free(obj->textures[j]);
        }
    } else {
        for (j = 0; j < obj->header->numberOfModelIds; j++) {
            sprite_free(obj->sprites[j]);
        }
    }
}

/**
 * Free the object and everything it owns. Particles have their own pool and only need
 * deallocating.
 */
void obj_destroy(Object *obj, s32 keepAnimTarget) {
    s32 i;

    if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
        particle_deallocate((Particle *) obj);
        gParticleCount--;
        return;
    }

    if (obj->attachPoints != NULL) {
        obj_destroy_attachments(obj);
    }
    if (obj->lightData != NULL) {
        for (i = 0; i < obj->header->numLightSources; i++) {
            light_remove(obj->lightData[i]);
        }
    }
    if (obj_behaviour(obj->behaviorId)->destroy != NULL) {
        obj_behaviour(obj->behaviorId)->destroy(obj, keepAnimTarget);
    }
    obj_free_models(obj);
    for (i = 0; i < obj->header->particleCount; i++) {
        emitter_cleanup(&obj->particleEmitter[i]);
    }
    try_free_object_header(obj->headerType);
    mempool_free(obj);
}

/**
 * Run the countdown while the race is starting; otherwise advance the level clock.
 */
static void obj_update_countdown(s32 updateRate) {
    gEventStartTimer = gEventCountdown;
    if (gEventCountdown > 0 && race_starting() != FALSE) {
        gEventCountdown -= updateRate;
        gObjectsElapsedTime = 0;
    } else {
        gObjectsElapsedTime += updateRate;
    }
    if (gEventCountdown <= 0) {
        gEventCountdown = 0;
    }
}

/**
 * Tick every ordinary object in the list: not particles, and not lights, weapons or fog
 * changers, which run after the racers. Inactive interactables (state 2) are skipped.
 * 3D models also get their texture scroll advanced.
 */
static void obj_update_scenery(s32 updateRate, s32 objectCount) {
    Object *obj;
    ModelInstance *modInst;
    s32 i;
    s32 j;

    for (i = gObjectListStart; i < objectCount; i++) {
        obj = gObjPtrList[i];
        if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
            continue;
        }
        if (obj->behaviorId == BHV_LIGHT_RGBA || obj->behaviorId == BHV_WEAPON || obj->behaviorId == BHV_FOG_CHANGER) {
            continue;
        }
        if (obj->interactObj == NULL || obj->interactObj->hitboxKind != 2) {
            run_object_loop_func(obj, updateRate);
        }
        if (obj->header->modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
            for (j = 0; j < obj->header->numberOfModelIds; j++) {
                modInst = obj->modelInstances[j];
                if (modInst != NULL) {
                    modInst->objModel->texOffsetUpdateRate = updateRate;
                }
            }
            if (obj->header->texScrollTextureIndex != 0xFF) {
                object_scroll_texture(obj, updateRate);
            }
        }
    }
}

/**
 * Move the racers, then let the best-placed human racer tune the AI's aggression.
 */
static void obj_update_racers(s32 updateRate) {
    Object_Racer *racer;
    s32 i;

    for (i = 0; i < gNumRacers; i++) {
        update_player_racer((*gRacers)[i], updateRate);
    }
    if (level_type() == RACETYPE_DEFAULT) {
        for (i = 0; i < gNumRacers; i++) {
            racer = gRacersByPosition[i]->racer;
            if (racer->playerIndex != -1) {
                increment_ai_behaviour_chances(gRacersByPosition[i], racer, updateRate);
                break;
            }
        }
    }
    racerfx_update(updateRate);
}

/**
 * Weapons and fog changers run after the racers they react to.
 */
static void obj_update_weapons(s32 updateRate, s32 objectCount) {
    Object *obj;
    s32 i;

    for (i = gObjectListStart; i < objectCount; i++) {
        obj = gObjPtrList[i];
        if ((!(obj->trans.flags & OBJ_FLAGS_PARTICLE) && (obj->behaviorId == BHV_WEAPON)) ||
            (obj->behaviorId == BHV_FOG_CHANGER)) {
            run_object_loop_func(obj, updateRate);
        }
    }
}

static void obj_update_particles(s32 updateRate, s32 objectCount) {
    Object *obj;
    s32 i;

    if (gParticleCount <= 0) {
        return;
    }
    for (i = gObjectListStart; i < objectCount; i++) {
        obj = gObjPtrList[i];
        if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
            particle_update((Particle *) obj, updateRate);
        }
    }
}

/**
 * Advance the dynamic lights and re-shade every lit object.
 */
static void obj_update_lighting(s32 updateRate) {
    Object *obj;
    s32 i;

    light_update_all(updateRate);
    if (light_count() <= 0) {
        return;
    }
    for (i = gObjectListStart; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE) && (obj->shading != NULL)) {
            light_update_shading(obj);
        }
    }
}

/**
 * On the first update after a level loads the intro becomes skippable; from then on, at a
 * fixed point of the countdown, A skips to the race and B (in adventure mode) leaves the
 * course. Courses visited for the first time and non-default races are never skippable.
 */
static void obj_update_intro_skip(void) {
    s32 buttons;
    s32 j;

    if (gCountdownSkipState == COUNTDOWN_SKIP_ALLOWED) {
        if (gEventCountdown == 80 && gCutsceneID == CUTSCENE_NONE) {
            buttons = 0;
            for (j = 0; j < MAXCONTROLLERS; j++) {
                buttons |= input_pressed(j);
            }
            if (buttons & A_BUTTON) {
                cutscene_set(CUTSCENE_ID_UNK_64);
            } else if ((buttons & B_BUTTON) && (get_trophy_race_world_id() == 0) && (is_in_tracks_mode() == 0)) {
                level_transition_begin(1); // FADE_BARNDOOR_HORIZONTAL?
            }
        }
    } else if (gCountdownSkipState == COUNTDOWN_SKIP_PENDING) {
        gCountdownSkipState = COUNTDOWN_SKIP_ALLOWED;
    }
}

/**
 * Updates all objects in the game, in a fixed order: collidable objects, scenery, racers,
 * weapons, particles, lights, then the race bookkeeping that depends on all of them.
 */
void obj_update(s32 updateRate) {
    Object_Racer *racer;
    s32 objectCount;
    s32 i;

    readout_log_push(-1);
    obj_update_countdown(updateRate);
    gAnimStage = 0;
    gAnimFrameParity = 1 - gAnimFrameParity;
    gAnimFrameState[gAnimFrameParity] = 0;
    for (i = 0; i < gNumRacers; i++) {
        racer = (*gRacers)[i]->racer;
        racer->prev_x_position = (*gRacers)[i]->trans.x_position;
        racer->prev_y_position = (*gRacers)[i]->trans.y_position;
        racer->prev_z_position = (*gRacers)[i]->trans.z_position;
    }
    obj_tick_anims();
    process_object_interactions();
    override_pos_update();
    // Update collidable objects first.
    for (i = 0; i < gCollisionObjectCount; i++) {
        run_object_loop_func(gCollisionObjects[i], updateRate);
    }
    override_pos_apply(TRUE);
    for (i = 0; i < gCollisionObjectCount; i++) {
        obj_collision_transform(gCollisionObjects[i]);
    }
    // Objects spawned during this update are not ticked until the next one.
    objectCount = gObjectCount;
    obj_update_scenery(updateRate, objectCount);
    obj_update_racers(updateRate);
    obj_update_weapons(updateRate, objectCount);
    obj_update_particles(updateRate, objectCount);
    obj_update_lighting(updateRate);
    override_pos_apply(FALSE);
    if (gTajRaceInit) {
        mode_init_taj_race();
    }
    if (gPathUpdateOff == FALSE) {
        gParticlePtrList_flush();
        checkpoint_update_all();
        spectate_update();
        anim_objects_update();
    }
    if (gNumRacers != 0) {
        if (gRaceEndTimer == 0) {
            race_check_finish(updateRate);
        } else {
            race_transition_adventure(updateRate);
        }
    }
    audspat_update_all(gRacersByPort, gNumRacers, updateRate);
    gPathUpdateOff = TRUE;
    gObjectUpdateRateF = (f32) updateRate;
    gBossRaceIds[0] = FALSE;
    gCutsceneSkipRequested = 0;
    transform_player_vehicle();
    dialogue_try_close();
    collision_pairs_tick();
    obj_update_intro_skip();
}

/**
 * Handles texture animation for an object.
 * Applies texture offset based on the update rate.
 */
void obj_tex_animate(Object *obj, s32 updateRate) {
    ObjectModel *model;
    TriangleBatchInfo *batches;
    s32 offset;
    TextureHeader *tex;
    s16 textureIsAnimated;
    s32 batchNumber;
    ModelInstance *modInst;

    modInst = obj->modelInstances[obj->modelIndex];
    model = modInst->objModel;
    batches = model->batches;
    textureIsAnimated = model->hasAnimatedTexture;
    for (batchNumber = 0; textureIsAnimated > 0 && batchNumber < model->numberOfBatches; batchNumber++) {
        if (batches[batchNumber].flags & RENDER_TEX_ANIM) {
            if (batches[batchNumber].textureIndex != TEX_INDEX_NO_TEXTURE) {
                tex = model->textures[batches[batchNumber].textureIndex].texture;
                offset = batches[batchNumber].texOffset;
                offset <<= 6;
                tex_animate_texture(tex, &batches[batchNumber].flags, &offset, updateRate);
                batches[batchNumber].texOffset = (offset >> 6) & 0xFF;
            }
        }
    }
}

/**
 * Sets the texture offset on the door number based on the balloon requirement.
 */
void obj_door_number(ObjectModel *model, Object *obj) {
    Object_Door *door;
    s32 current;
    s32 remaining;
    s32 i;
    TriangleBatchInfo *batch;

    if (model->hasAnimatedTexture <= 0) {
        return;
    }

    door = obj->door;
    remaining = door->balloonCount;
    current = ((remaining / 10) - 1) << 2;
    remaining = (remaining % 10) << 2;
    i = 0;
    batch = model->batches;

    while (i < model->numberOfBatches) {
        if (batch[i].flags & RENDER_TEX_ANIM) {
            if (batch[i].textureIndex != TEX_INDEX_NO_TEXTURE) {
                // Fakematch
                if (model->textures[batch[i].textureIndex].texture) {}
                if (model->textures[batch[i].textureIndex].texture->numOfTextures > 0x900) {
                    batch[i].texOffset = remaining;
                } else if (current >= 0) {
                    batch[i].texOffset = current;
                }
            }
        }
        i++;
    }
}

/**
 * Do nothing. Unused.
 */
UNUSED void do_nothing_func_80011364(UNUSED s32 unused) {
}

/**
 * Return true if paths are intended to be updated.
 * The variable they use here is backwards in terms of use.
 * Yes means no, no means yes.
 */
UNUSED s32 path_update_check(void) {
    // Ever hear of return !gPathUpdateOff?
    if (gPathUpdateOff) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/**
 * Signal to the game that checkpoints should be updated.
 */
void path_enable(void) {
    gPathUpdateOff = FALSE;
}

/**
 * Return the current race countdown timer.
 */
s32 get_race_countdown(void) {
    return gEventCountdown;
}

/**
 * Return the timer that the countdown is set to before the race starts.
 * There exists another variable in racer.c with exactly the same purpose.
 * This does not get used anywhere else.
 */
s32 get_race_start_timer(void) {
    return gEventStartTimer;
}


/**
 * When the object reaches a certain anim frame, play a sound and shake the camera to emphasise the effect of their
 * movement.
 */
s32 play_footstep_sounds(Object *obj, s32 arg1, s32 frame, s32 oddSoundId, s32 evenSoundId) {
    s8 *asset;
    f32 shakeDist;
    f32 shakeMagnitude;
    s32 animFrame;
    s32 asset0;
    s8 nextAsset;
    s32 i;
    s32 ret;
    s32 soundId;

    ret = 0;
    if (arg1 < obj->header->miscAssetCount) {
        asset = (s8 *) get_misc_asset(obj->header->miscAssetIds[arg1]);
        asset0 = asset[0];
        shakeDist = (asset[1] & 0xFF) * 8.0f;
        shakeMagnitude = asset[2];
        frame >>= 4;
        animFrame = obj->animFrame >> 4;
        for (i = 0; i < asset0; i++) {
            nextAsset = asset[i + 3];
            if ((animFrame >= nextAsset && frame < nextAsset) || (nextAsset >= animFrame && nextAsset < frame)) {
                set_camera_shake_by_distance(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position,
                                             shakeDist, shakeMagnitude);
                if (i & 1) {
                    soundId = oddSoundId; // Always set to SOUND_STOMP2
                } else {
                    soundId = evenSoundId; // Always set to SOUND_STOMP3
                }
                audspat_play_sound_at_position(soundId, obj->trans.x_position, obj->trans.y_position,
                                               obj->trans.z_position, AUDIO_POINT_FLAG_ONE_TIME_TRIGGER, NULL);
                ret = i + 1;
                i = asset0; // Come on, just use break!
            }
        }
    }
    return ret;
}

/**
 * Make the next call of move_object never mark the object as out of bounds.
 * Official Name: objMoveXYZnocheck
 */
void ignore_bounds_check(void) {
    gNoBoundsCheck = TRUE;
}

/**
 * Sets the new position of the object using the differences given.
 * Compares against the outer edges of the level geometry to decide wether or not to apply a segment ID.
 * Official Name: objMoveXYZ
 */
s32 move_object(Object *obj, f32 xPos, f32 yPos, f32 zPos) {
    s32 segmentID;
    f32 x1, x2, y1, y2, z1, z2;
    f32 newXPos;
    f32 newYPos;
    f32 newZPos;
    LevelModel *levelModel;
    LevelModelSegmentBoundingBox *box;
    s32 outsideBBox;
    s32 outOfBounds;
    s32 intXPos, intYPos, intZPos;

    levelModel = get_current_level_model();
    newXPos = obj->trans.x_position + xPos;
    newYPos = obj->trans.y_position + yPos;
    newZPos = obj->trans.z_position + zPos;
    if (levelModel == NULL) {
        gNoBoundsCheck = FALSE;
        return FALSE;
    }
    outOfBounds = FALSE;
    x2 = (levelModel->upperXBounds + 1000.0);
    //!@bug should've compared against "obj->trans.x_position"
    if (newXPos > x2) {
        outOfBounds = TRUE;
    }
    x1 = (levelModel->lowerXBounds - 1000.0);
    if (obj->trans.x_position < x1) {
        outOfBounds = TRUE;
    }
    if (1) {}
    if (1) {}
    if (1) {} // Fakematch
    y2 = (levelModel->upperYBounds + 3000.0);
    if (obj->trans.y_position > y2) {
        outOfBounds = TRUE;
    }
    y1 = (levelModel->lowerYBounds - 500.00);
    if (obj->trans.y_position < y1) {
        outOfBounds = TRUE;
    }
    z2 = (levelModel->upperZBounds + 1000.0);
    if (obj->trans.z_position > z2) {
        outOfBounds = TRUE;
    }
    z1 = (levelModel->lowerZBounds - 1000.0);
    if (obj->trans.z_position < z1) {
        outOfBounds = TRUE;
    }
    if (gNoBoundsCheck) {
        outOfBounds = FALSE;
    }

    gNoBoundsCheck = FALSE;
    if (outOfBounds) {
        obj->segmentID = -1;
        return TRUE;
    }

    obj->trans.x_position = newXPos;
    obj->trans.y_position = newYPos;
    obj->trans.z_position = newZPos;
    box = block_boundbox(obj->segmentID);

    // For some reason the XYZ positions are converted into integers for the next section
    intXPos = newXPos, intYPos = newYPos, intZPos = newZPos;

    if (box == NULL) {
        obj->segmentID = get_level_segment_index_from_position(intXPos, intYPos, intZPos);
        return FALSE;
    } else {
        outsideBBox = FALSE;
        if (box->x2 < intXPos || intXPos < box->x1) {
            outsideBBox = TRUE;
        }
        if (box->y2 < intYPos || intYPos < box->y1) {
            outsideBBox = TRUE;
        }
        if (box->z2 < intZPos || intZPos < box->z1) {
            outsideBBox = TRUE;
        }
        if (outsideBBox) {
            segmentID = get_level_segment_index_from_position(intXPos, intYPos, intZPos);
            if (segmentID != -1) {
                obj->segmentID = segmentID;
            }
        }
    }
    return FALSE;
}

/**
 * Set up the basic model view matrix, load a texture, then render the mesh.
 * A much simpler, faster way to render an object model as opposed to render_3d_model
 */
void render_misc_model(Object *obj, Vertex *verts, u32 numVertices, Triangle *triangles, u32 numTriangles,
                       TextureHeader *tex, u32 flags, u32 texOffset, f32 scaleY) {
    s32 hasTexture = FALSE;
    mtx_cam_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, &obj->trans, scaleY, 0.0f);
    gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    if (tex != NULL) {
        hasTexture = TRUE;
    }
    material_set(&gObjectCurrDisplayList, tex, flags, texOffset);
    gSPVertexDKR(gObjectCurrDisplayList++, OS_K0_TO_PHYSICAL(verts), numVertices, 0);
    gSPPolygon(gObjectCurrDisplayList++, OS_K0_TO_PHYSICAL(triangles), numTriangles, hasTexture);
    mtx_pop(&gObjectCurrDisplayList);
}

/**
 * A few objects use unconventional means to render. They are handled here.
 */
void render_3d_misc(Object *obj) {
    if (obj_behaviour(obj->behaviorId)->render != NULL) {
        obj_behaviour(obj->behaviorId)->render(obj);
    }
}

/**
 * Render an object as a billboard.
 * A few tweaks are made depending on the behaviour ID of the object.
 * A few exceptions will not call to render a billboarded sprite.
 */
void render_3d_billboard(Object *obj) {
    s32 intensity;
    s32 flags;
    s32 alpha;
    s32 hasPrimCol;
    s32 hasEnvCol;
    ObjectTransformExt objTrans;
    Object *bubbleTrap;
    Sprite *sprite;

    intensity = 255;
    hasPrimCol = FALSE;
    hasEnvCol = FALSE;
    flags = obj->trans.flags | RENDER_Z_UPDATE | RENDER_FOG_ACTIVE;
    if (obj->shading != NULL) {
        hasPrimCol = TRUE;
        hasEnvCol = TRUE;
        intensity = obj->shading->brightness * 255.0f;
    }

    if (obj->behaviorId == BHV_BOMB_EXPLOSION) {
        //!@bug Never true, because the type is u8.
        if (obj->opacity > 255) {
            obj->opacity = obj->properties.bombExplosion.opacity & 0xFF;
        } else {
            obj->opacity = (obj->opacity * (obj->properties.bombExplosion.opacity & 0xFF)) >> 8;
        }
    }

    alpha = obj->opacity;
    if (alpha > 255) {
        alpha = 255;
    }

    // If the behavior is a wizpig ghost, then halve it's transparency.
    if (obj->behaviorId == BHV_WIZPIG_GHOSTS) {
        alpha >>= 1;
    }

    if (alpha < 255) {
        flags |= RENDER_SEMI_TRANSPARENT;
        hasPrimCol = TRUE;
    }
    if ((obj->behaviorId == 5) && (obj->trans.scale == 6.0f)) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, (intensity * 3) >> 2, intensity, intensity >> 1, alpha);
        hasPrimCol = TRUE;
    } else if (obj->behaviorId == BHV_WIZPIG_GHOSTS) { // If the behavior is a wizpig ghost
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 150, 230, 255, alpha);
        hasPrimCol = TRUE;
    } else if (hasPrimCol || alpha < 255) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, intensity, intensity, intensity, alpha);
    } else {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    }
    if (hasEnvCol) {
        gDPSetEnvColor(gObjectCurrDisplayList++, obj->shading->lightR, obj->shading->lightG, obj->shading->lightB,
                       obj->shading->lightIntensity);
    } else if (obj->behaviorId == BHV_LAVA_SPURT) {
        hasEnvCol = TRUE;
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 0, 255);
    } else {
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    }
    sprite = obj->sprites[obj->modelIndex];
    bubbleTrap = NULL;
    if (obj->behaviorId == BHV_FIREBALL_OCTOWEAPON_2) {
        bubbleTrap = obj->properties.fireball.obj;
        if (obj->properties.fireball.timer > 0) {
            bubbleTrap = obj;
        }
    }

    // 5 = OilSlick, SmokeCloud, Bomb, BubbleWeapon
    if (bubbleTrap != NULL || !(obj->behaviorId != BHV_WEAPON || obj->weapon->weaponID != WEAPON_BUBBLE_TRAP)) {
        objTrans.trans.rotation.z_rotation = 0;
        objTrans.trans.rotation.x_rotation = 0;
        objTrans.trans.rotation.y_rotation = 0;
        objTrans.trans.scale = obj->trans.scale;
        objTrans.trans.x_position = 0.0f;
        objTrans.trans.z_position = 0.0f;
        objTrans.trans.y_position = 12.0f;
        objTrans.animFrame = obj->animFrame;
        objTrans.unk1A = 32;
        if (bubbleTrap == NULL) {
            bubbleTrap = obj->weapon->target;
            if (bubbleTrap == NULL) {
                bubbleTrap = obj;
            }
        }
        render_bubble_trap(&bubbleTrap->trans, sprite, (Object *) &objTrans,
                           RENDER_Z_COMPARE | RENDER_SEMI_TRANSPARENT | RENDER_Z_UPDATE);
    } else {
        render_sprite_billboard(&gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList, obj, sprite,
                                flags);
    }
    if (hasPrimCol) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    }
    if (hasEnvCol) {
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    }
}

/**
 * Advance the model's animation and relight its vertices. AI racers in single player and
 * split screen use the fast shader; the stopwatch icon is always shaded fully.
 */
static void model_update_vertices(Object *obj, ModelInstance *modInst, Object_Racer *racer) {
    ObjectModel *objModel;
    s32 fancyShading;

    objModel = modInst->objModel;
    obj->curVertData = modInst->vertices[modInst->animationTaskNum];
    if (modInst->modelType == MODELTYPE_ANIMATED) {
        obj_animate(obj);
    }
    if (modInst->modelType != MODELTYPE_BASIC && objModel->normals != NULL) {
        fancyShading = TRUE;
        if (racer != NULL && racer->vehicleID < VEHICLE_BOSSES && racer->playerIndex == PLAYER_COMPUTER) {
            fancyShading = FALSE;
        }
        if (cam_get_viewport_layout() != VIEWPORT_LAYOUT_1_PLAYER) {
            fancyShading = FALSE;
        }
        obj->curVertData = modInst->vertices[modInst->animationTaskNum];
        if (obj->behaviorId == BHV_UNK_3F) { // 63 = stopwatchicon, stopwatchhand
            obj_shade_fancy(objModel, obj, 0, gCurrentLightIntensity);
        } else if (fancyShading) {
            obj_shade_fancy(objModel, obj, -1, gCurrentLightIntensity);
        } else {
            obj_shade_fast(objModel, obj, gCurrentLightIntensity);
        }
    }
    // Set the animation ticker for non player racers to 2, making them animate at half the framerate.
    if ((racer != NULL) && (racer->playerIndex == PLAYER_COMPUTER) && (racer->vehicleID < VEHICLE_BOSSES)) {
        modInst->animUpdateTimer = 2;
    } else {
        modInst->animUpdateTimer = 1;
    }
}

/**
 * Draw the sprites attached to the model's attach points (balloons, held eggs, wheels'
 * decorations). Each is moved onto its attach vertex for the draw and moved back after.
 */
static void model_render_attachments(Object *obj, ObjectModel *objModel, Object_Racer *racer, s32 intensity,
                                     s32 opacity) {
    Object *attached;
    Sprite *sprite;
    s32 attachPointCount;
    s32 flags;
    s32 isCargo;
    s8 index;
    f32 vtxX;
    f32 vtxY;
    f32 vtxZ;
    s32 i;

    attachPointCount = obj->attachPoints->count;
    if (racer != NULL && racer->vehicleID == VEHICLE_FLYING_CAR) {
        attachPointCount = 0;
    }
    for (i = 0; i < attachPointCount; i++) {
        attached = obj->attachPoints->obj[i];
        if (attached->trans.flags & OBJ_FLAGS_INVISIBLE) {
            continue;
        }
        index = obj->attachPoints->unk2C[i];
        if (index < 0 || index >= objModel->numberOfAttachPoints) {
            continue;
        }
        sprite = attached->sprites[attached->modelIndex];
        vtxX = obj->curVertData[objModel->attachPoints[index]].x;
        vtxY = obj->curVertData[objModel->attachPoints[index]].y;
        vtxZ = obj->curVertData[objModel->attachPoints[index]].z;
        attached->trans.x_position += vtxX;
        attached->trans.y_position += vtxY;
        attached->trans.z_position += vtxZ;
        if (attached->header->modelType == OBJECT_MODEL_TYPE_SPRITE_BILLBOARD) {
            flags = (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_Z_UPDATE);
        } else {
            flags = (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_Z_UPDATE | RENDER_ANTI_ALIASING);
        }
        if (opacity < 255) {
            flags |= RENDER_SEMI_TRANSPARENT;
        }
        // In this instance, cargo refers to the eggs in the fire mountain challenge.
        isCargo = (attached->trans.flags & OBJ_FLAGS_UNK_0080 && attachPointCount == 3);
        if (racer != NULL && racer->transparency < 255) {
            isCargo = FALSE;
        }
        if (isCargo) {
            deferred_dl_begin(&gObjectCurrDisplayList);
            gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, intensity, intensity, intensity, opacity);
        }
        attached->properties.common.unk0 = render_sprite_billboard(&gObjectCurrDisplayList, &gObjectCurrMatrix,
                                                                   &gObjectCurrVertexList, attached, sprite, flags);
        if (isCargo) {
            gSPSelectMatrixDKR(gObjectCurrDisplayList++, G_MTX_DKR_INDEX_0);
            deferred_dl_end(&gObjectCurrDisplayList);
        }
        attached->trans.x_position -= vtxX;
        attached->trans.y_position -= vtxY;
        attached->trans.z_position -= vtxZ;
    }
}

/**
 * The egg a racer carries eases towards the model's hold point and is drawn there.
 */
static void model_render_held_object(Object *obj, ObjectModel *objModel, Object_Racer *racer) {
    Object *held;
    Sprite *sprite;
    s32 flags;
    s8 index;
    f32 vtxX;
    f32 vtxY;
    f32 vtxZ;

    held = racer->held_obj;
    if (held == NULL) {
        return;
    }
    index = obj->header->heldItemAttachPoint;
    if (index < 0 || index >= objModel->numberOfAttachPoints) {
        return;
    }
    flags = (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_Z_UPDATE);
    sprite = held->sprites[held->modelIndex];
    vtxX = obj->curVertData[objModel->attachPoints[index]].x;
    vtxY = obj->curVertData[objModel->attachPoints[index]].y;
    vtxZ = obj->curVertData[objModel->attachPoints[index]].z;
    held->trans.x_position += (vtxX - held->trans.x_position) * 0.25;
    held->trans.y_position += (vtxY - held->trans.y_position) * 0.25;
    held->trans.z_position += (vtxZ - held->trans.z_position) * 0.25;
    if (held->header->modelType == OBJECT_MODEL_TYPE_SPRITE_BILLBOARD) {
        render_sprite_billboard(&gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList, held, sprite,
                                flags);
    }
}

/**
 * Renders a 3D object, with support for vehicle part entities as part of the process.
 * Loads materials, and sets environment and/or primitive colours based on the material type.
 * Computes the view matrix for the model, and calls a function to draw meshes.
 * Loops through racers to find vehicle parts, which are wheels and propellers.
 */
void render_3d_model(Object *obj) {
    s32 intensity;
    s32 opacity;
    s32 headPushed;
    s32 hasOpacity;
    s32 hasLighting;
    s32 meshBatch;
    ModelInstance *modInst;
    Object_Racer *racer;
    ObjectModel *objModel;

    modInst = obj->modelInstances[obj->modelIndex];
    if (modInst == NULL) {
        return;
    }
    objModel = modInst->objModel;
    hasOpacity = FALSE;
    hasLighting = FALSE;
    intensity = 255;
    if (obj->shading != NULL) {
        intensity = (s32) (obj->shading->brightness * 255.0f * gCurrentLightIntensity);
        hasOpacity = TRUE;
        hasLighting = TRUE;
    }
    if (obj->behaviorId == BHV_RACER) {
        racer = obj->racer;
        object_do_player_tumble(obj);
    } else {
        racer = NULL;
    }
    if (modInst->animUpdateTimer <= 0) {
        model_update_vertices(obj, modInst, racer);
    }
    obj->curVertData = modInst->vertices[modInst->animationTaskNum];
    if (obj->behaviorId == BHV_DOOR) {
        obj_door_number(objModel, obj);
    }
    if (objModel->texOffsetUpdateRate && objModel->hasAnimatedTexture > 0) {
        obj_tex_animate(obj, objModel->texOffsetUpdateRate);
        modInst->objModel->texOffsetUpdateRate = 0;
    }
    mtx_cam_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, &obj->trans, gObjectModelScaleY, 0.0f);
    headPushed = FALSE;
    if (racer != NULL) {
        object_undo_player_tumble(obj);
        if (obj->animationID == 0 || racer->vehicleID >= VEHICLE_BOSSES) {
            mtx_head_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, modInst, racer->headAngle);
            headPushed = TRUE;
        } else {
            racer->headAngle = 0;
        }
    }
    opacity = obj->opacity;
    if (opacity > 255) {
        opacity = 255;
    }
    // If the behavior is a water zipper, then halve it's transparency.
    if (obj->behaviorId == BHV_ZIPPER_WATER) {
        opacity >>= 1;
    }
    if (opacity < 255) {
        hasOpacity = TRUE;
    }
    if (hasLighting) {
        gDPSetEnvColor(gObjectCurrDisplayList++, obj->shading->lightR, obj->shading->lightG, obj->shading->lightB,
                       obj->shading->lightIntensity);
    } else {
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    }
    if (obj->header->directionalPointLighting) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, obj->shading->shadowR, obj->shading->shadowG,
                        obj->shading->shadowB, opacity);
        directional_lighting_on();
    } else if (hasOpacity) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, intensity, intensity, intensity, opacity);
    } else {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    }
    if (opacity < 255) {
        meshBatch = render_mesh(objModel, obj, 0, RENDER_SEMI_TRANSPARENT, headPushed);
    } else {
        meshBatch = render_mesh(objModel, obj, 0, RENDER_NONE, headPushed);
    }
    if (obj->header->directionalPointLighting) {
        if (hasOpacity) {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, intensity, intensity, intensity, opacity);
        } else {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
        }
        directional_lighting_off();
    }
    if (obj->attachPoints != NULL) {
        model_render_attachments(obj, objModel, racer, intensity, opacity);
    }
    if (racer != NULL) {
        model_render_held_object(obj, objModel, racer);
    }
    // The translucent batches are drawn after everything opaque.
    if (meshBatch != -1) {
        if (obj->header->directionalPointLighting) {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, obj->shading->shadowR, obj->shading->shadowG,
                            obj->shading->shadowB, opacity);
            directional_lighting_on();
        }
        render_mesh(objModel, obj, meshBatch, RENDER_SEMI_TRANSPARENT, headPushed);
        if (obj->header->directionalPointLighting) {
            directional_lighting_off();
        }
    }
    if (hasOpacity || obj->header->directionalPointLighting) {
        gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    }
    if (hasLighting) {
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    }
    mtx_pop(&gObjectCurrDisplayList);
}

/** Deferred display lists: sections of an object's list cut out to be drawn after the track. */
void deferred_dl_reset(void) {
    gDeferredDlCount = 0;
}

void deferred_dl_flush(Gfx **dList) {
    s32 i;
    for (i = 0; i < gDeferredDlCount; i++) {
        gSPDisplayList((*dList)++, gDeferredDls[i]);
    }
}

void deferred_dl_begin(Gfx **dList) {
    if (gDeferredDlCount < 9) {
        gSPNoOp((*dList)++); // Placeholder instruction?
        gDeferredDls[gDeferredDlCount] = *dList;
    }
}

void deferred_dl_end(Gfx **dList) {
    if (gDeferredDlCount < 9) {
        gSPEndDisplayList((*dList)++);
        gSPBranchList(gDeferredDls[gDeferredDlCount] - 1, *dList);
        gDeferredDlCount++;
    }
}

/**
 * Update the object stack trace, set the draw pointers, then begin rendering the object.
 * Official Name: objPrintObject
 */
void render_object(Gfx **dList, Mtx **mtx, Vertex **verts, Object *obj) {
    f32 scale;
    if (obj->trans.flags & (OBJ_FLAGS_INVISIBLE | OBJ_FLAGS_SHADOW_ONLY)) {
        return;
    }
    update_object_stack_trace(OBJECT_DRAW, obj->objectID);
    gObjectCurrDisplayList = *dList;
    gObjectCurrMatrix = *mtx;
    gObjectCurrVertexList = *verts;
    scale = obj->trans.scale;
    render_object_parts(obj);
    obj->trans.scale = scale;
    *dList = gObjectCurrDisplayList;
    *mtx = gObjectCurrMatrix;
    *verts = gObjectCurrVertexList;
    update_object_stack_trace(OBJECT_DRAW, OBJECT_CLEAR);
}

/**
 * Official Name: objDoPlayerTumble
 */
void object_do_player_tumble(Object *this) {
    UNUSED s32 unused1;
    Object_Racer *sp_20;
    f32 tmp_f2;
    f32 offsetY;
    f32 tmp_f0;
    f32 temp;

    if (this->behaviorId == BHV_RACER) {

        sp_20 = this->racer;
        this->trans.rotation.y_rotation += sp_20->y_rotation_offset;
        this->trans.rotation.x_rotation += sp_20->x_rotation_offset;
        this->trans.rotation.z_rotation += sp_20->z_rotation_offset;
        offsetY = 0.0f;
        if (sp_20->vehicleIDPrev < VEHICLE_BOSSES) {

            offsetY = coss_f(sp_20->z_rotation_offset);
            tmp_f2 = offsetY;
            tmp_f0 = coss_f(sp_20->x_rotation_offset - sp_20->xRotationOffsetTarget) * tmp_f2;

            tmp_f0 = (tmp_f0 < 0.0f) ? 0.0f : tmp_f0 * tmp_f0;

            temp = (1.0f - tmp_f0) * 24.0f + sp_20->bodyOffsetY;
            if (0) {} // Fakematch
            offsetY = temp;
        }
        this->trans.y_position = this->trans.y_position + offsetY;
        gObjectOffsetY = offsetY;
    }
}

/**
 * Official Name: objUndoPlayerTumble
 */
void object_undo_player_tumble(Object *obj) {
    if (obj->behaviorId == BHV_RACER) {
        Object_Racer *racer = obj->racer;
        obj->trans.rotation.y_rotation -= racer->y_rotation_offset;
        obj->trans.rotation.x_rotation -= racer->x_rotation_offset;
        obj->trans.rotation.z_rotation -= racer->z_rotation_offset;
        obj->trans.y_position -= gObjectOffsetY;
    }
}

void set_temp_model_transforms(Object *obj) {
    s32 batchNum;
    ObjectModel *objModel;
    s32 var_v1;
    ModelInstance **modInstList;
    ModelInstance *modInst;
    u8 *bossAsset;
    f32 var_f0;
    u8 *var_a1;
    f32 ret2;
    UNUSED s32 pad;
    Object_Racer *objRacer;
    f32 ret1;
    s32 firstNonEmptyModelIndex;
    s32 modelIndex;
    s32 numberOfModels;

    ret1 = 1.0f;
    ret2 = 1.0f;
    if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE)) {
        if (obj->header->behaviorId == BHV_RACER) {
            objRacer = obj->racer;
            objRacer->aiPhysicsTimer = 30;
            if (objRacer->stunTimer > 0) {
                ret2 = 1.0f - (objRacer->stunTimer * 0.05f);
                if (ret2 < 0.2f) {
                    ret2 = 0.2f;
                }
            }
            if (objRacer->playerIndex != PLAYER_COMPUTER && objRacer->raceFinished) {
                modelIndex = 0;
                batchNum = 0;
            } else {
                if (obj->behaviorId == BHV_TIMETRIAL_GHOST) { // Ghost Object?
                    modelIndex = 1;
                    batchNum = 0;
                } else {
                    // Loads vehicles between VEHICLE_TRICKY and VEHICLE_SMOKEY. So all boss vehicles except wizpig.
                    batchNum = objRacer->vehicleID;
                    if (objRacer->vehicleID >= NUMBER_OF_PLAYER_VEHICLES) {
                        batchNum = 0;
                    }
                    bossAsset = (u8 *) get_misc_asset(batchNum + VEHICLE_BOSSES); // 40 bytes of data u8[8][5]?
                    batchNum = 0;
                    bossAsset += cam_get_viewport_layout() * 10;
                    if (get_current_viewport() != objRacer->playerIndex) {
                        bossAsset += 5;
                    }
                    var_f0 = obj->distanceToCamera;
                    var_v1 = (s32) var_f0 >> 3;
                    if (obj->distanceToCamera < 0.0f) {
                        var_f0 = 0.0f;
                    } else if (var_f0 > 3500.0f) {
                        var_f0 = 3500.0f;
                    }
                    var_f0 /= 2700.0f;
                    var_f0 += 1.0f;
                    obj->trans.scale *= var_f0;
                    var_v1 *= ((f32 *) get_misc_asset(ASSET_MISC_4))[objRacer->characterId];
                    // ASSET_MISC_4 is just 10 floats of 1.0f. One for each playable character.
                    if (var_v1 < -50) {
                        modelIndex = 5;
                    } else {
                        var_v1 >>= 1;
                        if (var_v1 < 0) {
                            var_v1 = 0;
                        }

                        if (var_v1 < bossAsset[0]) {
                            modelIndex = 0;
                        } else if (var_v1 < bossAsset[1]) {
                            modelIndex = 1;
                        } else if (var_v1 < bossAsset[2]) {
                            modelIndex = 2;
                        } else if (var_v1 < bossAsset[3]) {
                            modelIndex = 3;
                        } else if (var_v1 < bossAsset[4]) {
                            modelIndex = 4;
                        } else {
                            modelIndex = 5;
                        }
                    }
                }
            }

            firstNonEmptyModelIndex = 0;
            modInstList = &obj->modelInstances[firstNonEmptyModelIndex];

            while (*modInstList == NULL) {
                firstNonEmptyModelIndex++;
                modInstList++;
            }

            numberOfModels = obj->header->numberOfModelIds - 1;
            modInstList = &obj->modelInstances[numberOfModels];

            while (*modInstList == NULL) {
                numberOfModels--;
                modInstList--;
            }

            if (modelIndex < firstNonEmptyModelIndex) {
                modelIndex = firstNonEmptyModelIndex;
            }
            if (numberOfModels < modelIndex) {
                modelIndex = numberOfModels;
            }
            obj->modelIndex = modelIndex;
            if ((obj->shading != NULL) && (obj->shading->brightness < 0.6f)) {
                objRacer->lightFlags |= RACER_LIGHT_NIGHT;
            } else {
                objRacer->lightFlags &= ~RACER_LIGHT_NIGHT;
            }
            modelIndex = objRacer->lightFlags & RACER_LIGHT_TIMER;
            modInst = obj->modelInstances[obj->modelIndex];
            objModel = modInst->objModel;
            if (modelIndex != 0) {
                modelIndex--;
                if (objRacer->lightFlags & RACER_LIGHT_BRAKE) {
                    modelIndex += 1;
                    objRacer->lightFlags = (objRacer->lightFlags & ~RACER_LIGHT_UNK10) | RACER_LIGHT_UNK20;
                } else if (objRacer->lightFlags & RACER_LIGHT_NIGHT) {
                    modelIndex += 3;
                    objRacer->lightFlags = (objRacer->lightFlags & ~RACER_LIGHT_UNK20) | RACER_LIGHT_UNK10;
                } else if (objRacer->lightFlags & RACER_LIGHT_UNK20) {
                    modelIndex += 1;
                } else {
                    modelIndex += 3;
                }
            }
            modelIndex *= 4;
            for (batchNum = 0; batchNum < objModel->numberOfBatches; batchNum++) {
                if ((objModel->batches[batchNum].flags & 0x810000) == RENDER_TEX_ANIM) {
                    objModel->batches[batchNum].texOffset = modelIndex;
                }
            }
            obj->trans.x_position += objRacer->carBobX;
            obj->trans.y_position += objRacer->carBobY;
            obj->trans.z_position += objRacer->carBobZ;
            ret1 = objRacer->stretch_height;
        } else if (obj->behaviorId == BHV_FROG) {
            ret1 = obj->frog->scaleY;
        }
    }
    gObjectModelScaleY = ret1;
    gCurrentLightIntensity = ret2;
}

/**
 * Determine which model type the object is using, then call the related function to render it.
 * Beforehand, call a function to apply a temporary transformation, mostly for racers.
 * Afterwards, undo that.
 */
void render_object_parts(Object *obj) {
    set_temp_model_transforms(obj);
    if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
        render_particle((Particle *) obj, &gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList,
                        PARTICLE_UNK_FLAG_8000);
    } else {
        if (obj->header->modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
            render_3d_model(obj);
        } else if (obj->header->modelType == OBJECT_MODEL_TYPE_SPRITE_BILLBOARD) {
            render_3d_billboard(obj);
        } else if (obj->header->modelType == OBJECT_MODEL_TYPE_MISC) {
            render_3d_misc(obj);
        }
    }
    unset_temp_model_transforms(obj);
}

/**
 * After rendering, sets the object position back to normal.
 */
void unset_temp_model_transforms(Object *obj) {
    if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE) && obj->header->behaviorId == BHV_RACER) {
        obj->trans.x_position -= obj->racer->carBobX;
        obj->trans.y_position -= obj->racer->carBobY;
        obj->trans.z_position -= obj->racer->carBobZ;
    }
}

/** Draws a racer's boost flames from the boost asset for its vehicle. */
void render_boost_effect(Object *boostObj) {
    Vertex *vtx;
    Triangle *tri;
    ObjectTransform_800135B8 objTransform;
    Object_Boost_Inner *boostData;
    Object_Boost *boost;
    Object_Boost *asset;
    s32 hasTexture;
    s32 racerIndex;

    racerIndex = (boostObj->properties.boost.racerIndex) & 0xF;
    boost = boostObj->boost;
    switch (gBoostVehicleIds[racerIndex]) {
        case VEHICLE_CAR:
            boostData = &boost->carBoostData;
            break;
        case VEHICLE_HOVERCRAFT:
            boostData = &boost->hovercraftBoostData;
            break;
        default:
            boostData = &boost->flyingBoostData;
            break;
    }
    asset = (Object_Boost *) get_misc_asset(ASSET_MISC_20);
    asset = &asset[gBoostTypes[racerIndex]];
    object_do_player_tumble(boostObj->properties.boost.obj);
    mtx_cam_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, &boostObj->properties.boost.obj->trans, 1.0f, 0.0f);
    object_undo_player_tumble(boostObj->properties.boost.obj);
    objTransform.trans.x_position = boostData->position.x;
    objTransform.trans.y_position = boostData->position.y;
    objTransform.trans.z_position = boostData->position.z;
    objTransform.trans.scale = boostData->spriteScaleBase + (boostData->spriteScalePulse * coss_f(boost->pulseTimer << 12));
    if (boost->effectState < 2) {
        objTransform.trans.scale *= boost->effectScale;
    }
    if (gBoostTypes[racerIndex] != 0) {
        objTransform.trans.scale *= 1.15f;
    }
    objTransform.trans.rotation.z_rotation = 0;
    objTransform.trans.rotation.x_rotation = 0;
    objTransform.trans.rotation.y_rotation = 0;
    objTransform.unk18 = 0;
    gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
    render_sprite_billboard(&gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList,
                            (Object *) &objTransform, asset->sprite,
                            (RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_Z_UPDATE));
    if (boost->effectState == 2) {
        material_set(&gObjectCurrDisplayList, asset->tex,
                     (RENDER_Z_COMPARE | RENDER_SEMI_TRANSPARENT | RENDER_FOG_ACTIVE), 0);
        if (asset->tex != NULL) {
            hasTexture = TRUE;
        } else {
            hasTexture = FALSE;
        }

        vtx = &gBoostVerts[gBoostVertFlip][(boostObj->properties.boost.indexes >> 14) & 0x3FFF];
        tri = &gBoostTris[gBoostVertFlip][boostObj->properties.boost.indexes & 0x3FFF];
        gSPVertexDKR(gObjectCurrDisplayList++, OS_K0_TO_PHYSICAL(vtx), BOOST_VERT_COUNT, 0);
        gSPPolygon(gObjectCurrDisplayList++, OS_K0_TO_PHYSICAL(tri), BOOST_TRI_COUNT, hasTexture);
    }
    mtx_pop(&gObjectCurrDisplayList);
}

/**
 * Render the bubble trap weapon.
 */
void render_bubble_trap(ObjectTransform *trans, Sprite *gfxData, Object *obj, s32 flags) {
    f32 x;
    f32 y;
    f32 z;
    Camera *cameraSegment;
    f32 dist;

    vec3f_rotate(&trans->rotation, &obj->trans.position);
    obj->trans.x_position += trans->x_position;
    obj->trans.y_position += trans->y_position;
    obj->trans.z_position += trans->z_position;
    cameraSegment = cam_get_active_camera();
    x = cameraSegment->trans.x_position - obj->trans.x_position;
    y = cameraSegment->trans.y_position - obj->trans.y_position;
    z = cameraSegment->trans.z_position - obj->trans.z_position;
    dist = sqrtf((x * x) + (y * y) + (z * z));
    if (dist > 0.0) {
        dist = obj->numActiveEmitters / dist;
        x *= dist;
        y *= dist;
        z *= dist;
    }
    obj->trans.x_position += x;
    obj->trans.y_position += y;
    obj->trans.z_position += z;
    render_sprite_billboard(&gObjectCurrDisplayList, &gObjectCurrMatrix, &gObjectCurrVertexList, obj, gfxData, flags);
}

/**
 * Get the racer object data, and fetch set visual shield properties based on that racer.
 * Afterwards, render the graphics with opacity scaling with the fadetimer.
 */
void render_racer_shield(Gfx **dList, Mtx **mtx, Vertex **vtxList, Object *obj) {
    Object_Racer *racer;
    ModelInstance *modInst;
    ObjectModel *mdl;
    RacerShieldGfx *shield;
    s32 shieldType;
    s32 vehicleID;
    s32 racerIndex;
    f32 scale;
    f32 shear;

    racer = obj->racer;
    if (racer->shieldTimer > 0 && gShieldEffectObject != NULL) {
        gObjectCurrDisplayList = *dList;
        gObjectCurrMatrix = *mtx;
        gObjectCurrVertexList = *vtxList;
        racerIndex = racer->racerIndex;
        if (racerIndex > NUMBER_OF_CHARACTERS) {
            racerIndex = 0;
        }
        vehicleID = racer->vehicleID;
        if (vehicleID >= NUMBER_OF_PLAYER_VEHICLES) {
            vehicleID = VEHICLE_CAR;
        }
        shield = (RacerShieldGfx *) get_misc_asset(ASSET_MISC_SHIELD_DATA);
        vehicleID = (vehicleID * 10) + racerIndex;
        shield = shield + vehicleID;
        gShieldEffectObject->trans.x_position = shield->x_position;
        gShieldEffectObject->trans.y_position = shield->y_position;
        gShieldEffectObject->trans.z_position = shield->z_position;
        gShieldEffectObject->trans.y_position += shield->y_offset * sins_f(gShieldSineTime[racerIndex] * 0x200);
        shear = (coss_f(gShieldSineTime[racerIndex] * 0x400) * 0.05f) + 0.95f;
        gShieldEffectObject->trans.scale = shield->scale * shear;
        shear = shear * shield->turnSpeed;
        gShieldEffectObject->trans.rotation.y_rotation = gShieldSineTime[racerIndex] * 0x800;
        gShieldEffectObject->trans.rotation.x_rotation = 0x800;
        gShieldEffectObject->trans.rotation.z_rotation = 0;
        shieldType = racer->shieldType;
        if (shieldType != SHIELD_NONE) {
            shieldType--;
        }
        if (shieldType > SHIELD_LEVEL3 - 1) {
            shieldType = SHIELD_LEVEL3 - 1;
        }
        scale = ((f32) shieldType * 0.1) + 1.0f;
        gShieldEffectObject->trans.scale *= scale;
        shear *= scale;
        modInst = gShieldEffectObject->modelInstances[shieldType];
        mdl = modInst->objModel;
        gShieldEffectObject->curVertData = modInst->vertices[modInst->animationTaskNum];
        gDPSetEnvColor(gObjectCurrDisplayList++, 255, 255, 255, 0);
        if (racer->shieldTimer < 64) {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, racer->shieldTimer * 4);
        } else {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
        }
        mtx_shear_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, gShieldEffectObject, obj, shear);
        render_mesh(mdl, gShieldEffectObject, 0, RENDER_SEMI_TRANSPARENT, 0);
        gSPSelectMatrixDKR(gObjectCurrDisplayList++, G_MTX_DKR_INDEX_0);
        if (racer->shieldTimer < 64) {
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
        }
        *dList = gObjectCurrDisplayList;
        *mtx = gObjectCurrMatrix;
        *vtxList = gObjectCurrVertexList;
    }
}

/**
 * Get the racer object data, and fetch set visual magnet properties based on that racer.
 * Afterwards, render the graphics with opacity set by the properties.
 */
void render_racer_magnet(Gfx **dList, Mtx **mtx, Vertex **vtxList, Object *obj) {
    Object_Racer *racer;
    ModelInstance *modInst;
    ObjectModel *mdl;
    f32 *magnet;
    s32 vehicleID;
    s32 racerIndex;
    s32 opacity;
    f32 shear;
    UNUSED s32 pad;

    racer = obj->racer;
    racerIndex = racer->racerIndex;
    if (gRacerFXData[racerIndex].magnetFade != 0) {
        if (gMagnetEffectObject != NULL) {
            gObjectCurrDisplayList = *dList;
            gObjectCurrMatrix = *mtx;
            gObjectCurrVertexList = *vtxList;
            magnet = (f32 *) get_misc_asset(ASSET_MISC_MAGNET_DATA);
            vehicleID = racer->vehicleID;
            if (vehicleID < VEHICLE_CAR || vehicleID >= NUMBER_OF_PLAYER_VEHICLES) {
                vehicleID = VEHICLE_CAR;
            }
            magnet = &magnet[vehicleID * 5];
            racerIndex = racer->racerIndex;
            if (racerIndex > NUMBER_OF_CHARACTERS) {
                racerIndex = 0;
            }
            gMagnetEffectObject->trans.x_position = magnet[0];
            gMagnetEffectObject->trans.y_position = magnet[1];
            gMagnetEffectObject->trans.z_position = magnet[2];
            magnet += 3;
            shear = (coss_f((gRacerFXData[racerIndex].magnetPulseTimer * 0x400)) * 0.02f) + 0.98f;
            gMagnetEffectObject->trans.scale = magnet[0] * shear;
            magnet += 1;
            shear = magnet[0] * shear;
            gMagnetEffectObject->trans.rotation.y_rotation = gRacerFXData[racerIndex].magnetSpinTimer * 0x1000;
            gMagnetEffectObject->trans.rotation.x_rotation = 0;
            gMagnetEffectObject->trans.rotation.z_rotation = 0;
            modInst = gMagnetEffectObject->modelInstances[0];
            mdl = modInst->objModel;
            gMagnetEffectObject->curVertData = modInst->vertices[modInst->animationTaskNum];
            opacity = ((gRacerFXData[racerIndex].magnetPulseTimer * 8) & 0x7F) + 0x80;
            gfx_init_basic_xlu(&gObjectCurrDisplayList, DRAW_BASIC_2CYCLE, COLOUR_RGBA32(255, 255, 255, opacity),
                               gMagnetColours[racer->magnetModelID]);
            mtx_shear_push(&gObjectCurrDisplayList, &gObjectCurrMatrix, gMagnetEffectObject, obj, shear);
            gObjectTexAnim = TRUE;
            render_mesh(mdl, gMagnetEffectObject, 0, RENDER_SEMI_TRANSPARENT, 0);
            gObjectTexAnim = FALSE;
            gSPSelectMatrixDKR(gObjectCurrDisplayList++, G_MTX_DKR_INDEX_0);
            gDPSetPrimColor(gObjectCurrDisplayList++, 0, 0, 255, 255, 255, 255);
            rendermode_reset(&gObjectCurrDisplayList);
            *dList = gObjectCurrDisplayList;
            *mtx = gObjectCurrMatrix;
            *vtxList = gObjectCurrVertexList;
        }
    }
}

/** Offsets the texture coordinates of an object's model by the header's scroll rates times arg1. */
void object_scroll_texture(Object *obj, s32 arg1) {
    ObjectHeader *objHeader;
    s16 width;
    s16 height;
    s32 i;
    s32 j;
    s32 k;
    s32 end;
    s16 objHeader72;
    s16 objHeader73;
    ObjectModel *objMdl;
    ModelInstance *modInst;
    TextureInfo *texInfo;
    Triangle *tri;
    s16 temp;
    s16 temp2;
    s16 newU1;
    s16 newU2;
    s16 newV1;
    s16 newV2;

    objHeader = obj->header;
    objHeader73 = objHeader->texScrollModelIndex;
    objHeader72 = objHeader->texScrollTextureIndex;
    temp = (s16) (objHeader->texScrollSpeedU * arg1);
    temp2 = (s16) (objHeader->texScrollSpeedV * arg1);
    if ((objHeader73 == 0xFF) || (objHeader73 < objHeader->numberOfModelIds)) {
        if (objHeader73 == 0xFF) {
            end = objHeader->numberOfModelIds;
            objHeader73 = 0;
        } else {
            end = objHeader73 + 1;
        }
        for (i = objHeader73; i < end; i++) {
            modInst = obj->modelInstances[i];
            objMdl = modInst->objModel;
            if (objHeader72 < objMdl->numberOfTextures) {
                width = objMdl->textures[objHeader72].texture->width << 5;
                height = objMdl->textures[objHeader72].texture->height << 5;
                for (j = 0; j < objMdl->numberOfBatches; j++) {
                    if (objHeader72 == objMdl->batches[j].textureIndex) {
                        for (k = objMdl->batches[j].facesOffset; k < objMdl->batches[j + 1].facesOffset; k++) {
                            tri = &objMdl->triangles[k];
                            newU1 = (tri->uv1.u - tri->uv0.u);
                            newV1 = (tri->uv1.v - tri->uv0.v);
                            newU2 = (tri->uv2.u - tri->uv0.u);
                            newV2 = (tri->uv2.v - tri->uv0.v);
                            // s16 casts required
                            tri->uv0.u = (tri->uv0.u + temp) & (s16) (width - 1);
                            tri->uv0.v = (tri->uv0.v + temp2) & (s16) (height - 1);
                            tri->uv1.u = tri->uv0.u + newU1;
                            tri->uv1.v = tri->uv0.v + newV1;
                            tri->uv2.u = tri->uv0.u + newU2;
                            tri->uv2.v = tri->uv0.v + newV2;
                        }
                    }
                }
            }
        }
    }
}

/**
 * Loop through every object.
 * Check which ones have 3D models and count down the update timer.
 * The object will update its animation at 0.
 */
void obj_tick_anims(void) {
    s32 i = gObjectListStart;
    s32 j;
    Object *currObj;
    ModelInstance *modInst;

    for (; i < gObjectCount; i++) {
        currObj = gObjPtrList[i];
        if (!(currObj->trans.flags & OBJ_FLAGS_PARTICLE) && currObj->header->modelType == OBJECT_MODEL_TYPE_3D_MODEL) {
            for (j = 0; j < currObj->header->numberOfModelIds; j++) {
                modInst = currObj->modelInstances[j];
                if (modInst != NULL && modInst->animUpdateTimer > 0) {
                    modInst->animUpdateTimer &= 3;
                    modInst->animUpdateTimer--;
                }
            }
        }
    }
}

/**
 * Renders every triangle batch in an objects mesh.
 * If vertOffset is true, then draw in two passes, utilising the head matrix and vertex ID offset in the batch.
 */
s32 render_mesh(ObjectModel *objModel, Object *obj, s32 startIndex, s32 flags, s32 overrideVerts) {
    s32 i;
    s32 textureIndex;
    s32 triOffset;
    TextureHeader *texToSet;
    s32 endLoop;
    s32 numTris;
    s32 texEnabled;
    s32 texOffset;
    s32 numVertices;
    Vertex *vtx;
    s32 offsetStartVertex;
    s32 texToSetFlags;
    Triangle *tris;
    s32 vertOffset;
    Gfx *dList;

    dList = gObjectCurrDisplayList;
    i = startIndex;
    endLoop = FALSE;
    while (i < objModel->numberOfBatches && !endLoop) {
        if (!(objModel->batches[i].flags & RENDER_SEMI_TRANSPARENT) || flags & RENDER_SEMI_TRANSPARENT) {
            // Hidden/Invisible geometry
            textureIndex = objModel->batches[i].flags & RENDER_HIDDEN;
            // Probably a fakematch to use textureIndex here, but it works.
            if (!textureIndex) {
                vertOffset = objModel->batches[i].verticesOffset;
                triOffset = objModel->batches[i].facesOffset;
                numVertices = objModel->batches[i + 1].verticesOffset - vertOffset;
                offsetStartVertex = (overrideVerts) ? objModel->batches[i].vertOverride : numVertices;
                numTris = objModel->batches[i + 1].facesOffset - triOffset;
                tris = &objModel->triangles[triOffset];
                vtx = &obj->curVertData[vertOffset];
                textureIndex = objModel->batches[i].textureIndex;
                // textureIndex of 0xFF is no texture
                if (textureIndex == 0xFF) {
                    texOffset = 0;
                    texToSet = NULL;
                    texEnabled = FALSE;
                } else {
                    texOffset = objModel->batches[i].texOffset << 14;
                    texEnabled = TRUE;
                    texToSet = objModel->textures[textureIndex].texture;
                }
                texToSetFlags = objModel->batches[i].flags | RENDER_FOG_ACTIVE;
                if (flags & RENDER_SEMI_TRANSPARENT &&
                    !(objModel->batches[i].flags & (flags & ~RENDER_SEMI_TRANSPARENT))) {
                    texToSetFlags |= RENDER_SEMI_TRANSPARENT;
                }
                if (gObjectTexAnim == FALSE) {
                    material_set(&dList, texToSet, texToSetFlags, texOffset);
                } else {
                    texToSet = set_animated_texture_header(texToSet, texOffset);
                    gDkrDmaDisplayList(gObjectCurrDisplayList++, OS_K0_TO_PHYSICAL(texToSet->cmd),
                                       texToSet->numberOfCommands);
                }
                if (offsetStartVertex == numVertices) {
                    gSPVertexDKR(dList++, OS_K0_TO_PHYSICAL(vtx), numVertices, 0);
                } else {
                    if (offsetStartVertex > 0) {
                        gSPVertexDKR(dList++, OS_K0_TO_PHYSICAL(vtx), offsetStartVertex, 0);
                        gSPSelectMatrixDKR(dList++, G_MTX_DKR_INDEX_2);
                        gSPVertexDKR(dList++, OS_K0_TO_PHYSICAL(&vtx[offsetStartVertex]),
                                     (numVertices - offsetStartVertex), 1);
                    } else {
                        gSPSelectMatrixDKR(dList++, G_MTX_DKR_INDEX_2);
                        gSPVertexDKR(dList++, OS_K0_TO_PHYSICAL(vtx), numVertices, 0);
                    }
                    gSPSelectMatrixDKR(dList++, G_MTX_DKR_INDEX_1);
                }
                gSPPolygon(dList++, OS_K0_TO_PHYSICAL(tris), numTris, texEnabled);
            }
            i++;
        } else {
            endLoop = TRUE;
        }
    }
    if (i >= objModel->numberOfBatches) {
        i = -1;
    }
    gObjectCurrDisplayList = dList;
    return i;
}

s32 get_first_active_object(s32 *retObjCount) {
    s32 i, j;
    s32 minIndex, maxIndex;
    s32 breakLoop;

    *retObjCount = gObjectCount;
    if (gFirstActiveObjectId != 0) {
        // Already sorted
        return gFirstActiveObjectId;
    }

    i = gObjectListStart;
    j = gObjectCount - 1;
    minIndex = i;
    maxIndex = j;

    while (i <= j) {
        breakLoop = 0;
        while (i <= maxIndex && breakLoop == 0) {
            if (!(gObjPtrList[i]->trans.flags & OBJ_FLAGS_PARTICLE)) {
                if (gObjPtrList[i]->header->flags & HEADER_FLAGS_UNK_0001) {
                    i++;
                } else {
                    // Break the loop if neither OBJ_FLAGS_PARTICLE nor bit 1 in header->flags is set
                    breakLoop = -1;
                }
            } else {
                i++;
            }
        }

        breakLoop = 0;
        while (j >= minIndex && breakLoop == 0) {
            if (gObjPtrList[j]->trans.flags & OBJ_FLAGS_PARTICLE) {
                // Break the loop if OBJ_FLAGS_PARTICLE is set
                breakLoop = -1;
            } else if (!(gObjPtrList[j]->header->flags & HEADER_FLAGS_UNK_0001)) {
                j--;
            } else {
                // Break the loop if bit 1 in header->flags is set
                breakLoop = -1;
            }
        }

        if (i < j) {
            // Swap active and inactive objects
            Object *tempObject = gObjPtrList[i];
            gObjPtrList[i] = gObjPtrList[j];
            gObjPtrList[j] = tempObject;
            i++;
            j--;
        }
    }

    gFirstActiveObjectId = i;
    return i;
}



/**
 * Takes every object and sorts the main object list by distance to the camera.
 */
void sort_objects_by_dist(s32 startIndex, s32 lastIndex) {
    s32 i;
    s32 didNotSwap;
    Object *obj;

    if (lastIndex < startIndex) {
        return;
    }

    for (i = startIndex; i <= lastIndex; i++) {
        obj = gObjPtrList[i];
        if (obj != NULL) {
            if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
                // get_distance_to_camera calculates the distance to the camera from a XYZ location.
                obj->distanceToCamera =
                    -get_distance_to_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
            } else if (obj->header->flags & HEADER_FLAGS_UNK_0080) {
                obj->distanceToCamera += -16000.0f;
            } else {
                obj->distanceToCamera =
                    -get_distance_to_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
            }
        } else {
            //!@bug obj is NULL here, so it would probably cause a crash. Thankfully, gObjPtrList shouldn't have NULL
            //! objects in it.
            obj->distanceToCamera = 0.0f;
        }
    }

    // Keep swapping until all objects are sorted by the distance to the camera.
    do {
        didNotSwap = TRUE;
        for (i = startIndex; i < lastIndex; i++) {
            if (gObjPtrList[i]->distanceToCamera < gObjPtrList[i + 1]->distanceToCamera) {
                obj = gObjPtrList[i];
                gObjPtrList[i] = gObjPtrList[i + 1];
                gObjPtrList[i + 1] = obj;
                didNotSwap = FALSE;
            }
        }
    } while (!didNotSwap);
}

/**
 * Go through each object and detect potential interactions between each.
 * Add candidates to a list and calcualte their distances.
 */
void process_object_interactions(void) {
    Object *obj2;
    Object *obj;
    ObjectInteraction *objInteract;
    ObjectInteraction *objInteract2;
    f32 xDiff;
    f32 zDiff;
    f32 radius;
    s32 j;
    s32 i;
    s32 objsWithInteractives;
    Object *objList[257]; // 257 seems random, but it works for now.

    objsWithInteractives = 0;
    for (i = gObjectListStart; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE)) {
            objInteract = obj->interactObj;
            if (objInteract != NULL) {
                objList[objsWithInteractives] = obj;
                objsWithInteractives++;
                if (objInteract->hitboxKind != 2) {
                    objInteract->obj = NULL;
                    objInteract->flags &= ~(INTERACT_FLAGS_COLLIDED | INTERACT_FLAGS_PUSHING);
                    objInteract->distance = 255;
                }
            }
        }
    }

    gCollisionObjectCount = 0;

    for (i = 0; i < objsWithInteractives; i++) {
        obj = objList[i];
        objInteract = obj->interactObj;
        if (objInteract->hitboxKind == 2 && gCollisionObjectCount < 20) {
            gCollisionObjects[gCollisionObjectCount] = obj;
            gCollisionObjectCount++;
        }
        if (objInteract->flags & INTERACT_FLAGS_UNK_0004) {
            for (j = 0; j < objsWithInteractives; j++) {
                if (i != j) {
                    obj2 = objList[j];
                    objInteract2 = obj2->interactObj;
                    if (objInteract2->flags & (INTERACT_FLAGS_SOLID | INTERACT_FLAGS_TANGIBLE)) {
                        if (objInteract2->hitboxKind == 3) {
                            object_collide_with_object(obj, obj2);
                        } else if (objInteract2->hitboxKind != 2) {
                            xDiff = obj->trans.x_position - obj2->trans.x_position;
                            zDiff = obj->trans.z_position - obj2->trans.z_position;
                            if (objInteract2->flags & INTERACT_FLAGS_UNK_0020) {
                                radius = 0x400000; // 4194304.0f;
                            } else {
                                radius = 0x40000; // 262144.0f;
                            }
                            if (((xDiff * xDiff) + (zDiff * zDiff)) < radius) {
                                object_apply_interaction(obj, obj2);
                            }
                        }
                    }
                }
            }
        }
        if (objInteract->flags & INTERACT_FLAGS_UNK_0100) {
            for (j = 0; j < objsWithInteractives; j++) {
                if (i != j) {
                    obj2 = objList[j];
                    objInteract2 = obj2->interactObj;
                    if (objInteract2->hitboxKind == 3) {
                        object_collide_with_object(obj, obj2);
                    }
                }
            }
        }
    }

    for (i = 0; i < objsWithInteractives; i++) {
        obj = objList[i];
        objInteract = obj->interactObj;
        objInteract->x_position = obj->trans.x_position;
        objInteract->y_position = obj->trans.y_position;
        objInteract->z_position = obj->trans.z_position;
    }
}

/**
 * Remember the nearest object each side has met this frame. The distance saturates at
 * 255; INTERACT_FLAGS_UNK_0020 counts it at an eighth.
 */
static void interaction_track_nearest(ObjectInteraction *interact, Object *other, f32 distance) {
    s32 d;

    d = (s32) (f32) (s32) distance;
    if (interact->flags & INTERACT_FLAGS_UNK_0020) {
        d >>= 3;
    }
    if (d > 255) {
        d = 255;
    }
    if (interact->distance >= d) {
        interact->obj = other;
        interact->distance = d;
    }
}

/**
 * When `obj` has moved more than a unit since its interaction position was recorded,
 * use the distance from `other` to the closest point on that movement instead, so a fast
 * object cannot tunnel through. Returns the distance to use.
 */
static f32 interaction_closest_approach(Object *obj, ObjectInteraction *interact, Object *other, s32 flat,
                                        f32 distance) {
    f32 moveX;
    f32 moveY;
    f32 moveZ;
    f32 t;
    f32 px;
    f32 py;
    f32 pz;

    moveX = obj->trans.x_position - interact->x_position;
    moveY = obj->trans.y_position - interact->y_position;
    moveZ = obj->trans.z_position - interact->z_position;
    if (flat) {
        moveY = 0.0f;
    }
    t = moveX * moveX + moveY * moveY + moveZ * moveZ;
    if (t > 1.0) {
        t = ((other->trans.x_position - interact->x_position) * moveX +
             (other->trans.y_position - interact->y_position) * moveY +
             (other->trans.z_position - interact->z_position) * moveZ) /
            t;
        if (t >= 0.0f && t <= 1.0) {
            px = interact->x_position + t * moveX;
            py = interact->y_position + t * moveY;
            pz = interact->z_position + t * moveZ;
            distance = sqrtf((px - other->trans.x_position) * (px - other->trans.x_position) +
                             (py - other->trans.y_position) * (py - other->trans.y_position) +
                             (pz - other->trans.z_position) * (pz - other->trans.z_position));
        }
    }
    return distance;
}

/**
 * Push `obj` out of an immovable object. A racer that hits it hard enough also loses speed
 * and is bumped sideways for a few frames: hovercraft glance off, other vehicles bounce.
 */
static void interaction_push_into_solid(Object *obj, Object *other, ObjectInteraction *interactOther, f32 pushX,
                                        f32 pushY, f32 pushZ, f32 overlap, f32 invUpdateRate) {
    Object_Racer *racer;
    s32 bumped;
    f32 side;

    obj->trans.x_position -= pushX;
    obj->trans.y_position -= pushY;
    obj->trans.z_position -= pushZ;
    pushX *= invUpdateRate;
    pushZ *= invUpdateRate;
    if (obj->behaviorId != BHV_RACER) {
        return;
    }
    racer = obj->racer;
    bumped = FALSE;
    if (racer->vehicleID == VEHICLE_HOVERCRAFT) {
        if (overlap > 0.3) {
            if (overlap > 1.0) {
                bumped = TRUE;
            }
            if (bumped) {
                obj->x_velocity *= 0.8;
                obj->z_velocity *= 0.8;
            }
            if (bumped) {
                interactOther->flags |= INTERACT_FLAGS_COLLIDED;
                // Which side of the velocity line the obstacle is on decides the bump direction.
                side = (obj->trans.x_position * obj->z_velocity - obj->trans.z_position * obj->x_velocity);
                side = (other->trans.x_position * obj->z_velocity - other->trans.z_position * obj->x_velocity) - side;
                racer->bumpTimer = 7;
                if (side >= 0.0f) {
                    racer->bumpVelZ = obj->x_velocity * 0.1;
                    racer->bumpVelX = -obj->z_velocity * 0.1;
                } else {
                    racer->bumpVelZ = -obj->x_velocity * 0.1;
                    racer->bumpVelX = obj->z_velocity * 0.1;
                }
            }
        }
    } else {
        if (overlap > 0.3) {
            if (overlap > 1.0) {
                bumped = TRUE;
            }
            obj->x_velocity -= pushX;
            obj->z_velocity -= pushZ;
            racer->velocity = overlap * 0.25;
            racer->lateral_velocity = 0.0f;
        }
        if (bumped) {
            interactOther->flags |= INTERACT_FLAGS_COLLIDED;
            side = obj->trans.x_position * obj->z_velocity - obj->trans.z_position * obj->x_velocity;
            side = other->trans.x_position * obj->z_velocity - other->trans.z_position * obj->x_velocity - side;
            if (side >= 0.0f) {
                side = 2.0f;
            } else {
                side = -2.0f;
            }
            racer->bumpTimer = 7;
            racer->bumpVelX = racer->ox3 * side * racer->velocity;
            racer->bumpVelZ = racer->oz3 * side * racer->velocity;
        }
    }
    if (bumped && racer->playerIndex != -1) {
        racer_update_local_velocity(obj, racer);
    }
}

/**
 * Two movable objects share the push. Racers also trade velocity, weighted by their push
 * forces, and a shielded racer spins the other out.
 */
static void interaction_push_apart(Object *obj, Object *other, ObjectInteraction *interact,
                                   ObjectInteraction *interactOther, f32 pushX, f32 pushY, f32 pushZ, f32 overlap,
                                   f32 invUpdateRate) {
    Object_Racer *racer;
    Object_Racer *racerOther;
    f32 weight;
    f32 weightObj;
    s32 bumped;

    pushX *= 0.5;
    pushY *= 0.5;
    pushZ *= 0.5;
    if (interact->pushForce != 0) {
        obj->trans.x_position -= pushX;
        obj->trans.y_position -= pushY;
        obj->trans.z_position -= pushZ;
    }
    other->trans.x_position += pushX;
    other->trans.y_position += pushY;
    other->trans.z_position += pushZ;

    pushX *= invUpdateRate;
    pushZ *= invUpdateRate;
    if (obj->behaviorId != BHV_RACER || other->behaviorId != BHV_RACER) {
        return;
    }
    weight = 1.0;
    weight += (f32) (interact->pushForce - interactOther->pushForce) * 0.3;

    racer = obj->racer;
    racerOther = other->racer;
    if (racer->shieldType != 0 && racerOther->shieldType == 0) {
        racerOther->spinout_timer = racer->shieldType;
    }
    if (racerOther->shieldType != 0 && racer->shieldType == 0) {
        racer->spinout_timer = racerOther->shieldType;
    }

    bumped = FALSE;
    if (racer->vehicleID == VEHICLE_HOVERCRAFT) {
        bumped = TRUE;
    } else if (overlap > 0.1) {
        bumped = TRUE;
        pushX *= 0.5;
        pushZ *= 0.5;
    }
    if (bumped) {
        if (interact->pushForce != 0) {
            weightObj = 2.0 - weight;
            obj->x_velocity -= pushX * weightObj;
            obj->z_velocity -= pushZ * weightObj;
            racer_update_local_velocity(obj, racer);
        }
        other->x_velocity += pushX * weight;
        other->z_velocity += pushZ * weight;
        racer_update_local_velocity(other, racerOther);
    }
}

/**
 * Resolve one pair of interacting objects: record each as the other's nearest, then if
 * `other` is solid and their hitboxes overlap, separate them along the line between
 * their interaction positions and apply the bump.
 */
void object_apply_interaction(Object *obj, Object *other) {
    f32 distance;
    f32 minDistance;
    f32 overlap;
    f32 invUpdateRate;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 sepX;
    f32 sepY;
    f32 sepZ;
    f32 sepLength;
    ObjectInteraction *interact;
    ObjectInteraction *interactOther;

    dx = other->trans.x_position - obj->trans.x_position;
    dy = other->trans.y_position - obj->trans.y_position;
    dz = other->trans.z_position - obj->trans.z_position;

    interact = obj->interactObj;
    interactOther = other->interactObj;
    invUpdateRate = 1 / gObjectUpdateRateF;

    if (interactOther->hitboxKind == 1) {
        dy = -dy;
        if (dy < interactOther->yRangeMin * 10.0f || interactOther->yRangeMax * 10.0f < dy) {
            return;
        }
        dy = 0.0f;
    }
    if (interactOther->hitboxKind == 4 && dy < 0.0f) {
        dy *= 0.3;
    }

    distance = sqrtf(dx * dx + dy * dy + dz * dz);
#if VERSION == VERSION_80
    if (distance > 4000.0f) {
        return;
    }
    if (distance < -4000.0f) {
        return;
    }
#endif
    interaction_track_nearest(interact, other, distance);
    interaction_track_nearest(interactOther, obj, distance);

    if (!(interactOther->flags & INTERACT_FLAGS_SOLID)) {
        return;
    }
    minDistance = interactOther->hitboxRadius + interact->hitboxRadius;
    distance = interaction_closest_approach(obj, interact, other, interactOther->hitboxKind == 1, distance);
    if (!(distance < minDistance && distance > 0.0f)) {
        return;
    }

    // Separate along the line between the interaction positions, or between the objects
    // themselves when those coincide.
    sepX = interactOther->x_position - interact->x_position;
    sepY = interactOther->y_position - interact->y_position;
    sepZ = interactOther->z_position - interact->z_position;
    if (interactOther->hitboxKind == 1) {
        sepY = 0.0f;
    }
    sepLength = sqrtf(sepX * sepX + sepY * sepY + sepZ * sepZ);
    if (sepLength > 0.0f) {
        dx = sepX / sepLength;
        dy = sepY / sepLength;
        dz = sepZ / sepLength;
    } else {
        dx /= distance;
        dy /= distance;
        dz /= distance;
    }
    overlap = sepLength - distance;
    if (overlap < 0.0f) {
        overlap = -overlap;
    }
    dx *= overlap;
    dy *= overlap;
    dz *= overlap;
    overlap *= invUpdateRate;
    interact->flags |= INTERACT_FLAGS_PUSHING;
    interactOther->flags |= INTERACT_FLAGS_PUSHING;
    if (interactOther->pushForce == 0) {
        interaction_push_into_solid(obj, other, interactOther, dx, dy, dz, overlap, invUpdateRate);
        return;
    }
    interaction_push_apart(obj, other, interact, interactOther, dx, dy, dz, overlap, invUpdateRate);
}

/** Splits the racer's world velocity into forward and lateral components in its own frame. */
void racer_update_local_velocity(Object *obj, Object_Racer *racer) {
    s32 shakeMagnitude;
    s32 volume;
    s32 angle;
    f32 startVelocity;
    f32 cosAngle;
    f32 sinAngle;

    startVelocity = racer->velocity;
    angle = racer->steerVisualRotation;
    if (racer->vehicleID == VEHICLE_CAR) {
        if (racer->drift_direction != 0) {
            angle += racer->driftHeadingOffset;
            angle = (s16) angle;
        }
    }
    cosAngle = coss_f(-angle);
    sinAngle = sins_f(-angle);
    racer->lateral_velocity = (obj->x_velocity * cosAngle) + (obj->z_velocity * sinAngle);
    racer->velocity = (-obj->x_velocity * sinAngle) + (obj->z_velocity * cosAngle);
    if (racer->playerIndex != PLAYER_COMPUTER) {
        volume = (startVelocity - racer->velocity) * 14.0f;
        if (volume < 0) {
            volume = -volume;
        }
        volume += 35;
        if (volume > 127) {
            volume = 127;
        }
        if (racer->crashSoundCooldown == 0) {
            sound_play(SOUND_CRASH_CHARACTER, &racer->crashSoundHandle);
            sound_volume_set_relative(SOUND_CRASH_CHARACTER, racer->crashSoundHandle, volume);
        }
        if (racer->crashSoundCooldown == 0 && volume > 55) {
            if (!racer->raceFinished) {
                rumble_set(racer->playerIndex, RUMBLE_TYPE_18);
            }
            racer->driveAnimFlags |= 8;
        }
        if (volume > 55) {
            play_random_character_voice(obj, SOUND_VOICE_CHARACTER_NEGATIVE, 8, 1);
        }
        shakeMagnitude = (startVelocity - racer->velocity);
        if (shakeMagnitude < 0) {
            shakeMagnitude = -shakeMagnitude;
        }
        if (shakeMagnitude > 3) {
            shakeMagnitude = 3;
        }
        racer->crashSoundCooldown = 30;
        set_active_camera(racer->playerIndex);
        cam_get_active_camera()->shakeMagnitude = shakeMagnitude;
    }
}

/** Resolves a collision between an object and another object's collision model. */
void object_collide_with_object(Object *obj0, Object *obj1) {
    ObjectModel *objModel;
    s32 i;
    f32 temp;

#ifdef AVOID_UB
    MtxF obj1TransformMtx;
#else
    // THIS IS A HACK! Supposed to be a MtxF, but the stack ended up being too big.
    f32 pad[2];
    f32 obj1TransformMtx[4][3];
#endif

    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    ObjectInteraction *obj1Interact;
    ObjectInteraction *obj0Interact;
    Object_Racer *racer;
    f32 distance;
    f32 radius;
    ModelInstance *modInst;

    if (obj1->curVertData != NULL) {
        modInst = obj1->modelInstances[0];
        objModel = modInst->objModel;
        xDiff = obj0->trans.x_position - obj1->trans.x_position;
        yDiff = obj0->trans.y_position - obj1->trans.y_position;
        zDiff = obj0->trans.z_position - obj1->trans.z_position;
        if (!((objModel->boundingRadius + 50.0) < sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)))) {
            obj0Interact = obj0->interactObj;
            obj1Interact = obj1->interactObj;
            mtxf_from_transform((MtxF *) obj1TransformMtx, &obj1->trans);
            for (i = 0; i < objModel->collisionSpheresSize; i += 2) {
                xDiff = obj1->curVertData[objModel->collisionSpheres[i]].x;
                yDiff = obj1->curVertData[objModel->collisionSpheres[i]].y;
                zDiff = obj1->curVertData[objModel->collisionSpheres[i]].z;
                mtxf_transform_point((float (*)[4]) obj1TransformMtx, xDiff, yDiff, zDiff, &xDiff, &yDiff, &zDiff);
                temp = (((f32) objModel->collisionSpheres[i + 1] / 64) * obj1->trans.scale) * 50.0;
                xDiff -= obj0->trans.x_position;
                yDiff -= obj0->trans.y_position;
                zDiff -= obj0->trans.z_position;
                distance = sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff));
                temp += obj1Interact->hitboxRadius;
                if (distance < temp && distance > 0.0f) {
                    obj0Interact->flags |= INTERACT_FLAGS_PUSHING;
                    obj1Interact->flags |= INTERACT_FLAGS_PUSHING;
                    obj0Interact->obj = obj1;
                    obj1Interact->obj = obj0;
                    obj0Interact->distance = 0;
                    obj1Interact->distance = 0;
                    radius = (temp - distance) / distance;
                    distance = 2; // Needed
                    radius /= distance;
                    xDiff *= radius;
                    yDiff *= radius;
                    zDiff *= radius;
                    obj0->trans.x_position -= xDiff;
                    obj0->trans.y_position -= yDiff;
                    obj0->trans.z_position -= zDiff;

                    if (obj0->behaviorId == BHV_RACER) {
                        racer = obj0->racer;
                        if (!racer->raceFinished) {
                            rumble_set(racer->playerIndex, RUMBLE_TYPE_18);
                        }
                        if (racer->vehicleID == VEHICLE_HOVERCRAFT) {
                            if (radius > 0.1) {
                                obj0->x_velocity -= xDiff;
                                obj0->z_velocity -= zDiff;
                            }
                        } else if (radius > 0.3) {
                            obj0->x_velocity -= xDiff;
                            obj0->z_velocity -= zDiff;
                            racer->velocity = radius * 4.0f;
                            racer->lateral_velocity = 0.0f;
                        }
                    }
                }
            }
        }
    }
}

/** Prepares an object's collision transform and its models' collision data. */
void object_init_collision(Object *obj) {
    s32 i;

    obj->collisionData->mtxFlip = 0;
    obj_collision_transform(obj);
    obj_collision_transform(obj); // Not sure why they do this a second time.
    for (i = 0; i < obj->header->numberOfModelIds; i++) {
        if (obj->modelInstances[i] != NULL) {
            model_init_collision(obj->modelInstances[i]->objModel);
        }
    }
}

/**
 * Find the first butterfly node within range and return a pointer to it.
 */
Object *obj_butterfly_node(f32 x, f32 y, f32 z, f32 maxDistCheck, s32 dontCheckYAxis) {
    f32 diffY;
    f32 diffZ;
    f32 diffX;
    f32 distance;
    s32 i;
    Object *curObj;

    for (i = 0; i < gObjectCount; i++) {
        curObj = gObjPtrList[i];
        if (!(curObj->trans.flags & OBJ_FLAGS_PARTICLE) && curObj->behaviorId == BHV_ANIMATED_OBJECT_3) {
            diffX = curObj->trans.x_position - x;
            diffZ = curObj->trans.z_position - z;
            if (!dontCheckYAxis) {
                diffY = curObj->trans.y_position - y;
                distance = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
            } else {
                distance = sqrtf((diffX * diffX) + (diffZ * diffZ));
            }
            if (distance < maxDistCheck) {
                return curObj;
            }
        }
    }
    return NULL;
}

/**
 * Compare distance between itself and every racer.
 * Any racer within the radius is sorted from nearest to furthest.
 * Returns the number of racers within the radius.
 */
s32 obj_dist_racer(f32 x, f32 y, f32 z, f32 radius, s32 is2dCheck, Object **sortObj) {
    f32 distances[8];
    s32 i;
    s32 j;
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    s32 result;
    Object *racerObj;
    Object_Racer *racer;
    Object *swapObj;
    f32 swapf;

    result = 0;
    if (gNumRacers > 0) {
        for (i = 0; i < gNumRacers; i++) {
            racerObj = (*gRacers)[i];
            racer = racerObj->racer;
            if (racer->playerIndex >= 0 && racer->playerIndex < 4) {
                if (is2dCheck) {
                    xDiff = racerObj->trans.x_position - x;
                    zDiff = racerObj->trans.z_position - z;
                    yDiff = sqrtf((xDiff * xDiff) + (zDiff * zDiff));
                } else {
                    xDiff = racerObj->trans.x_position - x;
                    yDiff = racerObj->trans.y_position - y;
                    zDiff = racerObj->trans.z_position - z;
                    yDiff = sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff));
                }
                if (yDiff < radius) {
                    distances[result] = yDiff;
                    sortObj[result] = (*gRacers)[i];
                    result++;
                }
            }
        }
        if (result >= 2) {
            for (i = result - 1; i > 0; i--) {
                for (j = 0; j < i; j++) {
                    if (distances[j + 1] < distances[j]) {
                        swapf = distances[j];
                        swapObj = sortObj[j];
                        distances[j] = distances[j + 1];
                        sortObj[j] = sortObj[j + 1];
                        distances[j + 1] = swapf;
                        sortObj[j + 1] = swapObj;
                    }
                }
            }
        }
    }
    return result;
}

/**
 * Updates the object's collision transformation matrices.
 */
void obj_collision_transform(Object *obj) {
    ObjectTransform trans;
    s32 i;
    f32 inverseScale;
    MtxF *curMtx;
    MtxF inverseMtx;
    ObjectCollision *colData;

    colData = obj->collisionData;
    colData->mtxFlip = (colData->mtxFlip + 1) & 1;
#ifdef AVOID_UB
    curMtx = &colData->matrices[colData->mtxFlip];
#else
    curMtx = (MtxF *) &colData->_matrices[colData->mtxFlip << 1];
#endif
    trans.rotation.y_rotation = -obj->trans.rotation.y_rotation;
    trans.rotation.x_rotation = -obj->trans.rotation.x_rotation;
    trans.rotation.z_rotation = -obj->trans.rotation.z_rotation;
    trans.scale = 1.0f;
    trans.x_position = -obj->trans.x_position;
    trans.y_position = -obj->trans.y_position;
    trans.z_position = -obj->trans.z_position;
    mtxf_from_inverse_transform(curMtx, &trans);
    inverseScale = 1.0 / obj->trans.scale;
    i = 0;
    // Zero out the matrix.
    while (i < 16) {
        ((f32 *) inverseMtx)[i] = 0.0f;
        i++;
    }
    inverseMtx[0][0] = inverseScale;
    inverseMtx[1][1] = inverseScale;
    inverseMtx[2][2] = inverseScale;
    inverseMtx[3][3] = 1.0f;
    mtxf_mul(curMtx, &inverseMtx, curMtx);
    trans.scale = obj->trans.scale;
    trans.rotation.y_rotation = obj->trans.rotation.y_rotation;
    trans.rotation.x_rotation = obj->trans.rotation.x_rotation;
    trans.rotation.z_rotation = obj->trans.rotation.z_rotation;
    trans.x_position = obj->trans.x_position;
    trans.y_position = obj->trans.y_position;
    trans.z_position = obj->trans.z_position;
#ifdef AVOID_UB
    mtxf_from_transform((MtxF *) colData->matrices[(colData->mtxFlip + 2)], &trans);
#else
    mtxf_from_transform((MtxF *) colData->_matrices[(colData->mtxFlip + 2) << 1], &trans);
#endif
    colData->collidedObj = NULL;
}

s32 collision_objectmodel(Object *obj, s32 arg1, s32 *arg2, Vec3f *arg3, f32 *arg4, f32 *arg5, s8 *surface) {
    ModelInstance *modInst;
    s32 sp170;
    s32 sp16C;
    s32 sp168;
    s32 j;
    s32 sp160;
    f32 dist;
    Object *sp158;
    ObjectModel *sp154;
    f32 temp;
    CollisionPair *sp14C;
    f32 sp13C[4];
    f32 sp12C[4];
    f32 sp11C[4];
    s32 i;
    ObjectCollision *collision;
    s32 tempv0;
    f32 sp100[4];
    f32 spF0[4];
    f32 spE0[4];
    MtxF *spDC;
    s32 spB4[10];
    f32 sp8C[10];

    sp160 = 0;

    for (sp170 = 0; sp170 < gCollisionObjectCount; sp170++) {
        sp158 = gCollisionObjects[sp170];
        modInst = sp158->modelInstances[sp158->modelIndex];
        sp154 = modInst->objModel;

        temp = sp158->trans.x_position - obj->trans.x_position;
        dist = sqrtf(
            (temp) * (temp) +
            (sp158->trans.y_position - obj->trans.y_position) * (sp158->trans.y_position - obj->trans.y_position) +
            (sp158->trans.z_position - obj->trans.z_position) * (sp158->trans.z_position - obj->trans.z_position));

        j = dist;
        if (sp158->interactObj->flags & 0x20) {
            j >>= 3;
        }
        if (j > 255) {
            j = 255;
        }
        if (sp158->interactObj->distance > j) {
            sp158->interactObj->distance = j;
            sp158->interactObj->obj = obj;
        }

        if (dist - 25.0f < sp154->boundingRadius * sp158->trans.scale) {
            spB4[sp160] = sp170;
            sp8C[sp160] = dist;

            // clang-format off
            for (i = sp160; i > 0 && sp8C[i - 1] < sp8C[i]; i--) {\
                temp = sp8C[i];\
                sp16C = spB4[i];\
                sp8C[i] = sp8C[i - 1];\
                spB4[i] = spB4[i - 1];\
                sp8C[i - 1] = temp;
                spB4[i - 1] = sp16C;
            }
            // clang-format on

            sp160++;
        }
    }

    sp168 = 0;
    sp170 = 0;
    while (sp170 < sp160) {
        sp158 = gCollisionObjects[spB4[sp170]];
        modInst = sp158->modelInstances[sp158->modelIndex];
        sp154 = modInst->objModel;
        collision = sp158->collisionData;
#ifdef AVOID_UB
        spDC = &collision->matrices[((sp158->collisionData->mtxFlip + 1) & 1)];
#else
        spDC = (MtxF *) &collision->_matrices[((sp158->collisionData->mtxFlip + 1) & 1) << 1];
#endif

        sp14C = collision_pair_take(obj, sp158);
        if (sp14C != NULL) {
            for (i = 0, j = 0; j < arg1; j++, i += 3) {
                sp13C[j] = sp14C->prevLocalPoints[i + 0];
                sp12C[j] = sp14C->prevLocalPoints[i + 1];
                sp11C[j] = sp14C->prevLocalPoints[i + 2];
                mtxf_transform_point(*spDC, arg4[i], arg4[i + 1], arg4[i + 2], &sp100[j], &spF0[j], &spE0[j]);
            }
        } else {
            for (i = 0, j = 0; j < arg1; j++, i++) {
                mtxf_transform_point(*spDC, arg3[i].x, arg3[i].y, arg3[i].z, &sp13C[j], &sp12C[j], &sp11C[j]);
            }
        }

        for (i = 0, j = 0; j < arg1; j++, i += 3) {
            mtxf_transform_point(*spDC, arg4[i], arg4[i + 1], arg4[i + 2], &sp100[j], &spF0[j], &spE0[j]);
        }

        arg2[0] = 0;
        tempv0 = model_resolve_collisions(sp154, arg1, arg2, sp13C, sp12C, sp11C, sp100, spF0, spE0, arg5, surface,
                               1.0 / sp158->trans.scale);
        if (tempv0 != 0) {

            // @fake
            if (!j) {}

            sp158->collisionData->collidedObj = obj;
        }

        if (gBossRaceIds[0] == 0) {
            sp14C = collision_pair_add(obj, sp158);
        }

#ifdef AVOID_UB
        spDC = &sp158->collisionData->matrices[(sp158->collisionData->mtxFlip + 2)];
#else
        spDC = (MtxF *) &sp158->collisionData->_matrices[(sp158->collisionData->mtxFlip + 2) << 1];
#endif

        // @fake
        if (sp158) {}

        sp16C = 1;
        for (i = 0, j = 0; j < arg1; j++, i += 3, sp16C <<= 1) {
            if (sp14C != NULL) {
                sp14C->prevLocalPoints[i + 0] = sp100[j];
                sp14C->prevLocalPoints[i + 1] = spF0[j];
                sp14C->prevLocalPoints[i + 2] = spE0[j];
            }
            if (tempv0 & sp16C) {
                mtxf_transform_point(*spDC, sp100[j], spF0[j], spE0[j], &arg4[i + 0], &arg4[i + 1], &arg4[i + 2]);
            }
        }

        sp168 |= tempv0;
        sp170++;
    }

    arg2[0] = 0;
    for (i = 0; i < 4; i++) {
        if (sp168 & (1 << i)) {
            (*arg2)++;
        }
    }

    return sp168;
}

/** Recent collision pairs, each with a two-tick cooldown so a contact is handled once. */
CollisionPair *collision_pair_take(Object *arg0, Object *arg1) {
    CollisionPair *entry;
    s16 i;

    for (i = 0; i < 16; i++) {
        entry = &gCollisionPairs[i];
        if (entry->ttl != 0 && entry->objA == arg0 && entry->objB == arg1) {
            entry->ttl = 0;
            return entry;
        }
    }
    return NULL;
}

CollisionPair *collision_pair_add(Object *obj1, Object *obj2) {
    CollisionPair *entry;
    s16 i;

    for (i = 0; i < 16; i++) {
        entry = &gCollisionPairs[i];
        if (entry->ttl == 0) {
            entry->objA = obj1;
            entry->objB = obj2;
            entry->ttl = 2;
            return entry;
        }
    }
    return NULL;
}

u32 collision_pairs_tick(void) {
    s16 i = 0;
    while (i < 16) {
        CollisionPair *temp = &gCollisionPairs[i];
        if (temp->ttl != 0) {
            temp->ttl--;
        }
        i++;
    }
#ifdef AVOID_UB
    return 0;
#endif
}

/** Resolves a set of moving points against an object model's collision facets. */
s32 model_resolve_collisions(ObjectModel *arg0, s32 arg1, s32 *numCollisions, f32 *originPointsX, f32 *originPointsY,
                  f32 *originPointsZ, f32 *targetPointsX, f32 *targetPointsY, f32 *targetPointsZ, f32 *collisionRadii,
                  s8 *surfaces, f32 scale) {
    f32 *planes;
    s32 i, j, k;
    f32 sum1, sum2;
    f32 t;
    u32 var_a2;
    s32 counter;
    s32 spF8;
    s32 var_s6;
    CollisionFacetPlanes *node;
    UNUSED s32 pad;
    s32 closestTri;
    f32 A, B, C, D;
    f32 A1, B1, C1, D1;
    s32 redoLoop;
    f32 spC0;
    f32 x1, y1, z1;
    f32 x3, y3, z3;
    f32 x2, y2, z2;

    spF8 = 0;
    planes = arg0->collisionPlanes;
    var_s6 = 1;

    for (i = 0; i < arg1; i++) {
        x1 = targetPointsX[i];
        y1 = targetPointsY[i];
        z1 = targetPointsZ[i];
        x2 = originPointsX[i];
        y2 = originPointsY[i];
        z2 = originPointsZ[i];
        spC0 = collisionRadii[i] * scale;

        counter = 0;
        do {
            redoLoop = FALSE;
            for (j = 0; j < arg0->collisionFacetCount; j++) {
                node = &arg0->collisionFacets[j];
                closestTri = node->basePlaneIndex;

                A = planes[4 * closestTri + 0];
                B = planes[4 * closestTri + 1];
                C = planes[4 * closestTri + 2];
                D = planes[4 * closestTri + 3];
                sum1 = A * x2 + B * y2 + C * z2 + D;
                sum2 = A * x1 + B * y1 + C * z1 + D;
                sum1 -= spC0;
                sum2 -= spC0;
                if (sum1 >= -0.1 && sum2 < -0.1) {
                    x3 = (x1 - x2);
                    y3 = (y1 - y2);
                    z3 = (z1 - z2);
                    if (sum1 != sum2) {
                        t = sum1 / (sum1 - sum2);
                    } else {
                        t = 0.0f;
                    }

                    var_a2 = TRUE;
                    x3 = x3 * t + x2;
                    y3 = y3 * t + y2;
                    z3 = z3 * t + z2;

                    for (k = 0; k < 3 && var_a2 == TRUE; k++) {
                        closestTri = node->edgeBisectorPlane[k];

                        A1 = planes[4 * closestTri + 0];
                        B1 = planes[4 * closestTri + 1];
                        C1 = planes[4 * closestTri + 2];
                        D1 = planes[4 * closestTri + 3];
                        t = A1 * x3 + B1 * y3 + C1 * z3 + D1;
                        if (t > 4.0f) {
                            var_a2 = FALSE;
                        }
                    }

                    if (var_a2) {
                        redoLoop = TRUE;
                        if (B > 0.707) {
                            y1 = (spC0 - (A * x1 + C * z1 + D)) / B;
                        } else {
                            x1 -= sum2 * A;
                            y1 -= sum2 * B;
                            z1 -= sum2 * C;
                        }
                        counter++;
                        if (counter > 10) {
                            redoLoop = FALSE;
                            x1 = x2;
                            y1 = y2;
                            z1 = z2;
                        }
                        surfaces[i] = 0;
                        targetPointsX[i] = x1;
                        targetPointsY[i] = y1;
                        targetPointsZ[i] = z1;

                        j = arg0->collisionFacetCount; // break
                    }
                }
            }
        } while (redoLoop);

        if (counter > 0) {
            (*numCollisions)++;
            spF8 |= var_s6;
        }
        var_s6 <<= 1;
    }

    return spF8;
}

/**
 * Sets the active Taj challenge.
 */
void set_taj_challenge_type(s32 vehicleID) {
    gTajChallengeType = vehicleID;
    gPathUpdateOff = FALSE;
}

/**
 * Returns which Taj challenge is currently active.
 */
UNUSED s16 get_taj_challenge_type(void) {
    return gTajChallengeType;
}

static f32 distance_between_objects(Object *a, Object *b) {
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;

    xDiff = a->trans.x_position - b->trans.x_position;
    yDiff = a->trans.y_position - b->trans.y_position;
    zDiff = a->trans.z_position - b->trans.z_position;
    return sqrtf(((xDiff * xDiff) + (yDiff * yDiff)) + (zDiff * zDiff));
}

/**
 * Gather the checkpoint objects of the current challenge type into gTrackCheckpoints.
 * Alternate-route checkpoints are numbered 255 above their main-route twin. Returns how
 * many alternates were found.
 */
static s32 checkpoints_collect(void) {
    Object *obj;
    LevelObjectEntry_Checkpoint *checkpointEntry;
    s32 checkpointID;
    s32 alternateCount;
    s32 i;

    alternateCount = 0;
    gNumberOfMainCheckpoints = 0;
    for (i = 0; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE) && obj->behaviorId == BHV_CHECKPOINT &&
            gNumberOfMainCheckpoints < MAX_CHECKPOINTS) {
            checkpointEntry = &obj->level_entry->checkpoint;
            if (checkpointEntry->vehicleType == gTajChallengeType) {
                gTrackCheckpoints[gNumberOfMainCheckpoints].obj = obj;
                checkpointID = checkpointEntry->index;
                if (checkpointEntry->isAltCheckpoint) {
                    checkpointID += 255;
                    alternateCount++;
                }
                gTrackCheckpoints[gNumberOfMainCheckpoints].checkpointID = checkpointID;
                gTrackCheckpoints[gNumberOfMainCheckpoints].altRouteID = -1;
                gNumberOfMainCheckpoints++;
            }
        }
    }
    return alternateCount;
}

/**
 * Bubble-sort the checkpoints by ID so the main route comes first in order, then the
 * alternates. Returns TRUE and the offending ID if two checkpoints share one.
 */
static s32 checkpoints_sort(s32 *duplicateID) {
    Object *obj;
    s32 tempCheckpointID;
    s32 duplicate;
    s32 isSorted;
    s32 i;

    duplicate = FALSE;
    do {
        isSorted = TRUE;
        for (i = 0; i < gNumberOfMainCheckpoints - 1; i++) {
            if (gTrackCheckpoints[i].checkpointID == gTrackCheckpoints[i + 1].checkpointID) {
                duplicate = TRUE;
                *duplicateID = gTrackCheckpoints[i].checkpointID;
            }
            if (gTrackCheckpoints[i + 1].checkpointID < gTrackCheckpoints[i].checkpointID) {
                tempCheckpointID = gTrackCheckpoints[i].checkpointID;
                obj = gTrackCheckpoints[i].obj;
                gTrackCheckpoints[i].checkpointID = gTrackCheckpoints[i + 1].checkpointID;
                gTrackCheckpoints[i].obj = gTrackCheckpoints[i + 1].obj;
                gTrackCheckpoints[i + 1].checkpointID = tempCheckpointID;
                gTrackCheckpoints[i + 1].obj = obj;
                isSorted = FALSE;
            }
        }
    } while (!isSorted);
    return duplicate;
}

/**
 * Pair each alternate checkpoint with the main-route checkpoint it replaces.
 */
static void checkpoints_link_alternates(void) {
    s32 mainID;
    s32 i;
    s32 j;

    for (i = gNumberOfMainCheckpoints; i < gNumberOfTotalCheckpoints; i++) {
        mainID = gTrackCheckpoints[i].checkpointID - 255;
        for (j = 0; j < gNumberOfMainCheckpoints; j++) {
            if (mainID == gTrackCheckpoints[j].checkpointID) {
                gTrackCheckpoints[j].altRouteID = i;
                gTrackCheckpoints[i].altRouteID = j;
                break;
            }
        }
    }
}

/**
 * Derive a checkpoint's plane (its facing and offset), size and the distances to the next
 * checkpoint on the main and alternate routes. From here on checkpointID holds the
 * checkpoint's radius, not its number.
 */
static void checkpoint_init_node(s32 i) {
    CheckpointNode *checkpoint;
    Object *obj;
    LevelObjectEntry_Checkpoint *checkpointEntry;
    MtxF mtx;
    ObjectTransform transform;
    f32 ox;
    f32 oy;
    f32 oz;
    s32 next;
    s32 altRouteId;

    checkpoint = &gTrackCheckpoints[i];
    obj = checkpoint->obj;
    checkpointEntry = &obj->level_entry->checkpoint;
    transform.rotation.y_rotation = obj->trans.rotation.y_rotation;
    transform.rotation.x_rotation = obj->trans.rotation.x_rotation;
    transform.rotation.z_rotation = obj->trans.rotation.z_rotation;
    transform.scale = 1.0f;
    transform.x_position = 0.0f;
    transform.y_position = 0.0f;
    transform.z_position = 0.0f;
    mtxf_from_transform(&mtx, &transform);
    mtxf_transform_point(mtx, 0.0f, 0.0f, 1.0f, &ox, &oy, &oz);
    checkpoint->rotationXFrac = ox;
    checkpoint->rotationYFrac = oy;
    checkpoint->rotationZFrac = oz;
    checkpoint->planeOffset =
        -(((obj->trans.x_position * ox) + (obj->trans.y_position * oy)) + (obj->trans.z_position * oz));
    checkpoint->x = obj->trans.x_position;
    checkpoint->y = obj->trans.y_position;
    checkpoint->z = obj->trans.z_position;
    checkpoint->scale = obj->trans.scale * 2;
    checkpoint->checkpointID = obj->trans.scale * 128.0f;

    if (i < gNumberOfMainCheckpoints) {
        next = i + 1;
    } else {
        next = gTrackCheckpoints[i].altRouteID + 1;
    }
    if (next == gNumberOfMainCheckpoints) {
        next = 0;
    }
    checkpoint->distance = distance_between_objects(obj, gTrackCheckpoints[next].obj);
    altRouteId = gTrackCheckpoints[next].altRouteID;
    if (altRouteId != -1) {
        checkpoint->altDistance = distance_between_objects(obj, gTrackCheckpoints[altRouteId].obj);
    } else {
        checkpoint->altDistance = checkpoint->distance;
    }

    checkpoint->laneOffsetX[0] = checkpointEntry->lane0OffsetX;
    checkpoint->laneOffsetY[0] = checkpointEntry->lane0OffsetY;
    checkpoint->laneAction[0] = checkpointEntry->lane0Action;
    checkpoint->laneOffsetX[1] = checkpointEntry->lane1OffsetX;
    checkpoint->laneOffsetY[1] = checkpointEntry->lane1OffsetY;
    checkpoint->laneAction[1] = checkpointEntry->lane1Action;
    checkpoint->laneOffsetX[2] = checkpointEntry->lane2OffsetX;
    checkpoint->laneOffsetY[2] = checkpointEntry->lane2OffsetY;
    checkpoint->laneAction[2] = checkpointEntry->lane2Action;
    checkpoint->laneOffsetX[3] = checkpointEntry->lane3OffsetX;
    checkpoint->laneOffsetY[3] = checkpointEntry->lane3OffsetY;
    checkpoint->laneAction[3] = checkpointEntry->lane3Action;
    checkpoint->indicatorType = checkpointEntry->indicatorType;
}

/**
 * Updates information pertaining to all checkpoints in the current race.
 */
void checkpoint_update_all(void) {
    s32 alternateCount;
    s32 duplicateID;
    s32 i;

    alternateCount = checkpoints_collect();
    if (checkpoints_sort(&duplicateID)) {
        set_render_printf_position(20, 220);
        render_printf("Error: Multiple checkpoint no: %d !!\n", duplicateID);
    }
    gNumberOfTotalCheckpoints = gNumberOfMainCheckpoints;
    gNumberOfMainCheckpoints -= alternateCount;
    checkpoints_link_alternates();
    for (i = 0; i < gNumberOfTotalCheckpoints; i++) {
        checkpoint_init_node(i);
    }
}

/**
 * Signed distance of a point from the checkpoint's plane, positive on the far side.
 */
static f32 checkpoint_plane_distance(CheckpointNode *node, f32 x, f32 y, f32 z) {
    return node->rotationXFrac * x + node->rotationYFrac * y + node->rotationZFrac * z + node->planeOffset;
}

/**
 * How far along the segment from `prev` to `current` the object is, as a fraction, found by
 * projecting the object onto each plane along `dir`. Also returns the distance still to go
 * to the current plane.
 */
static f32 checkpoint_progress(CheckpointNode *current, CheckpointNode *prev, Object *obj, f32 dirX, f32 dirY, f32 dirZ,
                               f32 *toNextOut) {
    f32 toNext;
    f32 nextLength;
    f32 fromPrev;
    f32 length;

    toNext = checkpoint_plane_distance(current, obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
    nextLength = current->rotationXFrac * dirX + current->rotationYFrac * dirY + current->rotationZFrac * dirZ;
    nextLength = -toNext / nextLength;

    fromPrev = checkpoint_plane_distance(prev, obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
    length = prev->rotationXFrac * dirX + prev->rotationYFrac * dirY + prev->rotationZFrac * dirZ;
    length = fromPrev / length;

    if (nextLength + length != 0.0) {
        length = nextLength / (nextLength + length);
    } else {
        length = 0.0f;
    }
    *toNextOut = nextLength;
    return length;
}

/**
 * Checks to see whether a racer or homing missile passed a checkpoint.
 * Return values:
 * - 0: Homing missile passed the checkpoint.
 * - -100: Racer passed the checkpoint.
 * - Otherwise: No checkpoint has been passed.
 */
s32 checkpoint_is_passed(s32 checkpointIndex, Object *obj, f32 objX, f32 objY, f32 objZ, f32 *checkpointDistance,
                         u8 *isOnAlternateRoute) {
    s32 retLength;
    s32 isAltCheckpoint;
    f32 distX, distY, distZ;
    f32 length;
    f32 nextLength;
    Object_Racer *racer;
    CheckpointNode *currentCheckpoint;
    CheckpointNode *prevCheckpoint;
    CheckpointNode *altRouteCheckpoint;

    if (gNumberOfMainCheckpoints == 0) {
        return 1;
    }

    currentCheckpoint = &gTrackCheckpoints[checkpointIndex];
    if (checkpointIndex != 0) {
        prevCheckpoint = &gTrackCheckpoints[checkpointIndex - 1];
    } else {
        prevCheckpoint = &gTrackCheckpoints[gNumberOfMainCheckpoints - 1];
    }

    if (*isOnAlternateRoute) {
        if (currentCheckpoint->altRouteID != -1) {
            currentCheckpoint = &gTrackCheckpoints[currentCheckpoint->altRouteID];
        }
        if (prevCheckpoint->altRouteID != -1) {
            prevCheckpoint = &gTrackCheckpoints[prevCheckpoint->altRouteID];
        }
    }

    // Entering an alternate route: switch to its checkpoint once within its radius.
    isAltCheckpoint = FALSE;
    if (!(*isOnAlternateRoute) && prevCheckpoint->altRouteID == -1 && currentCheckpoint->altRouteID != -1) {
        altRouteCheckpoint = &gTrackCheckpoints[currentCheckpoint->altRouteID];
        distX = altRouteCheckpoint->x - obj->trans.x_position;
        distY = altRouteCheckpoint->y - obj->trans.y_position;
        distZ = altRouteCheckpoint->z - obj->trans.z_position;
        if (sqrtf(distX * distX + distY * distY + distZ * distZ) < altRouteCheckpoint->checkpointID) {
            currentCheckpoint = altRouteCheckpoint;
            isAltCheckpoint = TRUE;
        }
    }

    // Unit direction from the previous checkpoint to the current one.
    distX = currentCheckpoint->x - prevCheckpoint->x;
    distY = currentCheckpoint->y - prevCheckpoint->y;
    distZ = currentCheckpoint->z - prevCheckpoint->z;
    length = sqrtf(distX * distX + distY * distY + distZ * distZ);
    if (length > 0.0) {
        distX *= 1.0f / length;
        distY *= 1.0f / length;
        distZ *= 1.0f / length;
    }

    length = checkpoint_progress(currentCheckpoint, prevCheckpoint, obj, distX, distY, distZ, &nextLength);
    *checkpointDistance = length;

    if (obj->behaviorId == BHV_RACER) {
        racer = obj->racer;
        if (racer->playerIndex == PLAYER_COMPUTER) {
            if (length < -0.3) {
                return -100;
            }
            if (length > 1.3) {
                return -100;
            }
        }
    }

    if (nextLength > 0) {
        retLength = length * 100.0f;
        if (retLength == 0) {
            retLength++;
        }
        return retLength;
    }

    // The object is past the plane of the current checkpoint.
    if (isAltCheckpoint) {
        *isOnAlternateRoute = TRUE;
    } else if (currentCheckpoint->altRouteID == -1) {
        *isOnAlternateRoute = FALSE;
    }
    distY = checkpoint_plane_distance(currentCheckpoint, objX, objY, objZ);
    if (distY > 0) {
        if (obj->behaviorId == BHV_RACER) {
            racer = obj->racer;
            if (currentCheckpoint->indicatorType != 0) {
                racer->indicator_type = currentCheckpoint->indicatorType;
                racer->indicator_timer = 120;
            }
        }
        prevCheckpoint = currentCheckpoint;
        checkpointIndex++;
        if (checkpointIndex == gNumberOfMainCheckpoints) {
            checkpointIndex = 0;
        }
        currentCheckpoint = &gTrackCheckpoints[checkpointIndex];
        //!@bug distY now holds the plane distance, not the direction's Y component.
        length = checkpoint_progress(currentCheckpoint, prevCheckpoint, obj, distX, distY, distZ, &nextLength);
        *checkpointDistance = length;
    } else {
        *checkpointDistance = 0.0;
    }
    return 0;
}

/**
 * Search and return Taj's overworld object.
 * Used for drawing his minimap position.
 */
Object *find_taj_object(void) {
    s32 i;
    Object *current_obj;
    for (i = gObjectListStart; i < gObjectCount; i++) {
        current_obj = gObjPtrList[i];
        if (!(current_obj->trans.flags & OBJ_FLAGS_PARTICLE) && (current_obj->behaviorId == BHV_PARK_WARDEN)) {
            return current_obj;
        }
    }
    return NULL;
}

/**
 * Fade channels by the player's distance from a midi fade point: full inside the inner
 * radius, gone at the outer one. Only acts while its sequence is playing.
 */
static void midi_fade_point_apply(Object *racerObj, Object *obj) {
    Object_MidiFadePoint *point;
    f32 distance;
    f32 range;
    s32 fade;
    s32 i;

    distance = sqrtf(
        ((racerObj->trans.x_position - obj->trans.x_position) * (racerObj->trans.x_position - obj->trans.x_position)) +
        ((racerObj->trans.y_position - obj->trans.y_position) * (racerObj->trans.y_position - obj->trans.y_position)) +
        ((racerObj->trans.z_position - obj->trans.z_position) * (racerObj->trans.z_position - obj->trans.z_position)));
    point = obj->midi_fade_point;
    if (distance >= point->outerRadius || point->sequenceID != music_current_sequence()) {
        return;
    }
    if (distance <= point->innerRadius) {
        fade = 0;
    } else {
        distance -= point->innerRadius;
        range = (point->outerRadius - point->innerRadius);
        fade = (127.0f * distance) / range;
    }
    for (i = 0; i < 16; i++) {
        switch (point->channelModes[i]) {
            case 1:
                if (fade >= 0x7B) {
                    music_channel_off(i);
                } else {
                    music_channel_fade_set(i, (0x7F - fade));
                    music_channel_on(i);
                }
                break;
            case 2:
                if ((music_channel_fade(i) > 0) && (music_channel_active(i) == 0)) {
                    music_channel_fade_set(i, fade);
                }
                break;
            default:
                break;
        }
    }
}

/**
 * Start a midi fade when the player's movement this frame crosses its plane inside its
 * box. The crossing direction picks which channel modes apply.
 */
static void midi_fade_check_crossing(Object *racerObj, Object *obj, f32 xPos, f32 yPos, f32 zPos) {
    Object_MidiFade *midiFade;
    f32 prevSide;
    f32 currSide;
    f32 moveX;
    f32 moveY;
    f32 moveZ;
    f32 t;
    f32 hitX;
    s8 direction;

    midiFade = obj->midi_fade;
    prevSide =
        (midiFade->planeX * xPos) + (midiFade->planeY * yPos) + (midiFade->planeZ * zPos) + midiFade->planeOffset;
    currSide = (midiFade->planeX * racerObj->trans.x_position) + (midiFade->planeY * racerObj->trans.y_position) +
               (midiFade->planeZ * racerObj->trans.z_position) + midiFade->planeOffset;
    if (prevSide > 0.0f && currSide <= 0.0f) {
        direction = 1;
    } else if (currSide > 0.0f && prevSide <= 0.0f) {
        direction = -1;
    } else {
        return;
    }
    moveX = racerObj->trans.x_position - xPos;
    moveY = racerObj->trans.y_position - yPos;
    // Where along the movement the plane was crossed.
    t = (-midiFade->planeX * xPos - midiFade->planeY * yPos - midiFade->planeZ * zPos - midiFade->planeOffset) /
        (midiFade->planeX * moveX + midiFade->planeY * moveY + midiFade->planeZ * (racerObj->trans.z_position - zPos));
    hitX = t * moveX;
    if (!((midiFade->minX <= hitX + xPos) && (hitX + xPos <= midiFade->maxX))) {
        return;
    }
    moveZ = racerObj->trans.z_position - zPos;
    if (!((midiFade->minY <= (t * moveY) + yPos) && ((t * moveY) + yPos <= midiFade->maxY))) {
        return;
    }
    if (!((midiFade->minZ <= (t * (moveZ)) + zPos) && ((t * (moveZ)) + zPos <= midiFade->maxZ))) {
        return;
    }
    midiFade->direction = direction;
    midiFade->progress = 0;
    midiFade->timer = 0;
    gCurrentMidiFade = midiFade;
}

/**
 * Switch the music's channel mask when the player comes within range of a channel set.
 */
static void midi_channel_set_apply(Object *racerObj, Object *obj) {
    Object_MidiChannelSet *channelSet;
    f32 distance;
    u8 radius;

    channelSet = obj->midi_channel_set;
    distance = sqrtf(
        ((racerObj->trans.x_position - obj->trans.x_position) * (racerObj->trans.x_position - obj->trans.x_position)) +
        ((racerObj->trans.y_position - obj->trans.y_position) * (racerObj->trans.y_position - obj->trans.y_position)) +
        ((racerObj->trans.z_position - obj->trans.z_position) * (racerObj->trans.z_position - obj->trans.z_position)));
    radius = channelSet->radius;
    if ((distance < (radius * 4)) && (channelSet->channelMask != music_channel_get_mask()) &&
        (channelSet->sequenceID == music_current_sequence())) {
        music_dynamic_set(channelSet->channelMask);
    }
}

/**
 * Step the running midi fade: progress runs 0..254 over its duration, and each channel
 * follows its mode for the crossing direction. The fade is dropped once complete.
 */
static void midi_fade_advance(s32 updateRate) {
    Object_MidiFade *midiFade;
    u16 durationFrames;
    s32 progress;
    s32 level;
    s8 mode;
    s32 i;

    midiFade = gCurrentMidiFade;
    if (midiFade->sequenceID != music_current_sequence()) {
        return;
    }
    midiFade->timer += updateRate;
    durationFrames = midiFade->duration * gVideoRefreshRate;
    if (durationFrames < midiFade->timer) {
        midiFade->timer = durationFrames;
    }
    progress = (midiFade->timer * 254.0f) / durationFrames;
    if (progress < 0xFE) {
        midiFade->progress = progress;
    } else {
        midiFade->progress = 0xFE;
    }
    for (i = 0; i < 16; i++) {
        mode = midiFade->channelModes[i];
        if (midiFade->direction == -1) {
            mode >>= 2;
        }
        mode &= 3;
        switch (mode) {
            case 1:
                music_channel_on(i);
                music_channel_fade_set(i, 0x7FU);
                break;
            case 0:
                music_channel_off(i);
                break;
            case 3:
                if (midiFade->progress >= 0x80) {
                    level = (midiFade->progress - 0x7F) & 0xFF;
                    music_channel_on(i);
                    if (music_channel_fade(i) < level) {
                        music_channel_fade_set(i, level);
                    }
                }
                break;
            case 2:
                if (midiFade->progress < 0x7F) {
                    level = (0x7F - midiFade->progress) & 0xFF;
                    if (level < music_channel_fade(i)) {
                        music_channel_fade_set(i, level);
                    }
                } else {
                    music_channel_off(i);
                }
                break;
            default:
                break;
        }
    }
    if (midiFade->progress == 0xFE) {
        gCurrentMidiFade = 0;
    }
}

/**
 * Fades music channels by player one's position relative to the level's midi fade
 * objects. (xPos, yPos, zPos) is where the racer was last frame.
 */
void update_midi_fade_points(Object *racerObj, f32 xPos, f32 yPos, f32 zPos, s32 updateRate) {
    Object *obj;
    s32 i;

    if (racerObj->racer->playerIndex != 0) {
        return;
    }
    if (cam_get_viewport_layout() != 0) {
        return;
    }
    for (i = gObjectListStart; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
            continue;
        }
        if (obj->behaviorId == BHV_MIDI_FADE_POINT) {
            midi_fade_point_apply(racerObj, obj);
        } else if (obj->behaviorId == BHV_MIDI_FADE) {
            midi_fade_check_crossing(racerObj, obj, xPos, yPos, zPos);
        } else if (obj->behaviorId == BHV_MIDI_CHANNEL_SET) {
            midi_channel_set_apply(racerObj, obj);
        }
    }
    if (gCurrentMidiFade != 0) {
        midi_fade_advance(updateRate);
    }
}

/**
 * Calculates the direction a homing rocket takes toward the next checkpoint.
 * Returns true if a direction was calculated, or false if there are no checkpoints.
 */
s32 homing_rocket_get_next_direction(Object *obj, s32 checkpoint, u8 isOnAlternateRoute, s32 arg3, s32 arg4,
                                     f32 checkpointDist, f32 *outX, f32 *outY, f32 *outZ) {
    s32 numCheckpoints;
    s32 checkpointIndex;
    s32 i;
    f32 xData[4];
    f32 yData[4];
    f32 zData[4];
    f32 xSpline;
    f32 temp2;
    f32 zSpline;
    f32 ySpline;
    f32 dx;
    f32 dy;
    f32 dz;
    CheckpointNode *checkpointNode;

    numCheckpoints = gNumberOfMainCheckpoints;
    if (numCheckpoints == 0) {
        return FALSE;
    }
    checkpointIndex = checkpoint - 2;
    if (checkpointIndex < 0) {
        checkpointIndex += numCheckpoints;
    }
    for (i = 0; i < 4; i++) {
        checkpointNode = find_next_checkpoint_node(checkpointIndex, isOnAlternateRoute);
        xData[i] = checkpointNode->x + (checkpointNode->scale * checkpointNode->rotationZFrac * arg3);
        yData[i] = checkpointNode->y + (checkpointNode->scale * arg4);
        zData[i] = checkpointNode->z + (checkpointNode->scale * -checkpointNode->rotationXFrac * arg3);
        checkpointIndex += 1;
        if (checkpointIndex == numCheckpoints) {
            checkpointIndex = 0;
        }
    }
    temp2 = (1.0 - checkpointDist);
    if (temp2 < 0.0f) {
        temp2 = 0.0f;
    }
    if (temp2 > 1.0) {
        temp2 = 1.0f;
    }
    xSpline = cubic_spline_interpolation(xData, 0, temp2, &dx);
    ySpline = cubic_spline_interpolation(yData, 0, temp2, &dy);
    zSpline = cubic_spline_interpolation(zData, 0, temp2, &dz);
    temp2 = sqrtf((dx * dx) + (dy * dy) + (dz * dz));
    if (temp2 != 0.0f) {
        temp2 = 500.0 / temp2;
        dx *= temp2;
        dy *= temp2;
        dz *= temp2;
    }
    *outX = (xSpline + dx) - obj->trans.x_position;
    *outY = (ySpline + dy) - obj->trans.y_position;
    *outZ = (zSpline + dz) - obj->trans.z_position;
    return TRUE;
}

/**
 * A challenge racer's score for ranking those who have not finished: bananas lost in battle,
 * laps elsewhere, with eggs counting progress towards the next lap. Clamped to 0..10.
 */
static s8 challenge_score(Object_Racer *racer, s8 raceType) {
    s8 score;

    if (raceType == RACETYPE_CHALLENGE_BATTLE) {
        score = 10 - racer->bananas;
    } else {
        score = racer->lap;
        if (raceType == RACETYPE_CHALLENGE_EGGS) {
            score *= 3;
            if (racer->eggHudCounter != 0) {
                score += 2;
            } else if (racer->held_obj != NULL) {
                score += 1;
            }
        }
    }
    if (score > 10) {
        score = 10;
    }
    if (score < 0) {
        score = 0;
    }
    return score;
}

/**
 * Hand out the remaining finish positions by score, best first. In battle mode the last to
 * finish wins, so the order is flipped.
 */
static void challenge_rank_remaining(Object_Racer **racer, s8 *score, s8 raceType) {
    s16 best;
    s16 bestIndex;
    s32 i;

    do {
        bestIndex = -1;
        best = -1;
        for (i = 0; i < gNumRacers; i++) {
            if (!racer[i]->raceFinished && score[i] >= best) {
                best = score[i];
                bestIndex = i;
            }
        }
        if (bestIndex != -1) {
            if (raceType == RACETYPE_CHALLENGE_BATTLE) {
                racer[bestIndex]->finishPosition = 5 - gNumFinishedRacers;
            } else {
                racer[bestIndex]->finishPosition = gNumFinishedRacers;
            }
            gNumFinishedRacers++;
            racer[bestIndex]->raceFinished = TRUE;
        }
    } while (bestIndex != -1);
}

/**
 * Winning a challenge for the first time in adventure mode awards a T.T. amulet piece.
 * Returns the new amulet count, or 0 if nothing was awarded.
 */
static s32 challenge_award_amulet(Settings *settings, Object_Racer **racer) {
    s32 amulet;

    if (is_in_tracks_mode()) {
        return 0;
    }
    if (racer[0]->finishPosition != 1 && !(is_in_two_player_adventure() && racer[1]->finishPosition == 1)) {
        return 0;
    }
    if (settings->courseFlagsPtr[settings->courseId] & RACE_CLEARED) {
        return 0;
    }
    settings->courseFlagsPtr[settings->courseId] |= RACE_CLEARED;
    amulet = settings->ttAmulet + 1;
    if (amulet > 4) {
        amulet = 4;
    }
    settings->ttAmulet = amulet;
    return amulet;
}

/**
 * Record the finish order as next race's starting positions, play the victory or defeat
 * jingle, and note whether player two now leads a shared adventure.
 */
static void challenge_store_results(Settings *settings, Object_Racer **racer) {
    s32 sequence;
    s32 nextPosition;
    s32 i;

    for (i = 0; i < 8; i++) {
        settings->racers[i].starting_position = -1;
    }
    sequence = SEQUENCE_BATTLE_LOSE;
    for (i = 0; i < gNumRacers; i++) {
        if (racer[i]->playerIndex != PLAYER_COMPUTER && racer[i]->finishPosition == 1) {
            sequence = SEQUENCE_BATTLE_VICTORY;
        }
        settings->racers[i].starting_position = racer[i]->finishPosition - 1;
    }
    music_play(sequence);
    nextPosition = 4;
    for (i = 0; i < 8; i++) {
        if (settings->racers[i].starting_position == -1) {
            settings->racers[i].starting_position = nextPosition;
            nextPosition++;
        }
    }
    gSwapLeadPlayer = FALSE;
    if (is_in_two_player_adventure() &&
        settings->racers[PLAYER_TWO].starting_position < settings->racers[PLAYER_ONE].starting_position) {
        gSwapLeadPlayer = TRUE;
    }
}

/**
 * In a shared adventure the player who placed better leads from here on.
 */
static void race_apply_lead_player_swap(void) {
    if (!is_in_two_player_adventure()) {
        return;
    }
    if (gSwapLeadPlayer) {
        gSwapLeadPlayer = FALSE;
        swap_lead_player();
        if (gLeadPlayerIndex != 0) {
            gIsP2LeadPlayer = TRUE;
        }
    } else if (gLeadPlayerIndex != 0) {
        gIsP2LeadPlayer = TRUE;
    }
}

/**
 * Win conditions for the Taj and trophy challenges (battle, eggs, treasure hunt). Battle
 * eliminates racers as they run out of bananas; the challenge ends when the humans are done
 * or three racers have finished, and the rest are ranked by score.
 */
static void race_check_finish_challenge(Settings *settings, LevelHeader *header) {
    Object_Racer *racer[4];
    s8 score[4];
    s16 numHumanRacers;
    s16 numHumanRacersFinished;
    s16 numFinishedRacers;
    s32 amulet;
    s32 i;

    if (header->race_type == RACETYPE_CHALLENGE_EGGS) {
        racer_update_eggs(*gRacers);
    }
    if (gRaceFinishTriggered != FALSE) {
        return;
    }
    numHumanRacers = 0;
    numHumanRacersFinished = 0;
    numFinishedRacers = 0;
    for (i = 0; i < gNumRacers; i++) {
        racer[i] = (*gRacers)[i]->racer;
        // Manage eliminated racers in deathmatch.
        if (header->race_type == RACETYPE_CHALLENGE_BATTLE && racer[i]->bananas <= 0 && !racer[i]->raceFinished) {
            racer[i]->raceFinished = TRUE;
            racer[i]->balloon_quantity = 0;
            racer_sound_free((*gRacers)[i]);
            (*gRacers)[i]->trans.flags |= OBJ_FLAGS_INVISIBLE;
            (*gRacers)[i]->interactObj->flags = INTERACT_FLAGS_NONE;
            racer[i]->finishPosition = 5 - gNumFinishedRacers;
            gNumFinishedRacers++;
        }
        if (racer[i]->playerIndex != PLAYER_COMPUTER) {
            if (racer[i]->raceFinished) {
                numHumanRacersFinished++;
            }
            numHumanRacers++;
        }
        if (racer[i]->raceFinished) {
            numFinishedRacers++;
            if (racer[i]->finishPosition == 0) {
                racer[i]->finishPosition = gNumFinishedRacers;
                gNumFinishedRacers++;
            }
        }
    }
    if (!((header->race_type != RACETYPE_CHALLENGE_BATTLE && numFinishedRacers > 0) ||
          ((((numHumanRacers == 1 && numHumanRacersFinished == 1) ||
             (numHumanRacers >= 2 && numHumanRacersFinished >= numHumanRacers)) ||
            numFinishedRacers >= 3)))) {
        return;
    }
    for (i = 0; i < gNumRacers; i++) {
        score[i] = challenge_score(racer[i], header->race_type);
    }
    challenge_rank_remaining(racer, score, header->race_type);
    gSwapLeadPlayer = FALSE;
    amulet = challenge_award_amulet(settings, racer);
    challenge_store_results(settings, racer);
    if (amulet == 0) {
        race_apply_lead_player_swap();
        postrace_start(0, 30);
    } else {
        level_properties_push(SPECIAL_MAP_ID_NO_LEVEL, 0, VEHICLE_CAR, CUTSCENE_ID_NONE);
        level_properties_push(ASSET_LEVEL_TTAMULETSEQUENCE, 0, VEHICLE_NO_OVERRIDE, settings->ttAmulet - 1);
        race_finish_adventure(TRUE);
    }
    gRaceFinishTriggered = TRUE;
}

/**
 * Rank every racer against every other by checkpoint progress (finished racers first) and
 * record it as racerOrder. The HUD position only follows once the order has held for a
 * couple of frames.
 */
static void race_update_positions(LevelHeader *header) {
    Object_Racer *racer;
    Object_Racer *other;
    s32 prevOrder;
    s32 order;
    s32 i;
    s32 j;

    for (i = 0; i < gNumRacers; i++) {
        order = 1;
        racer = (*gRacers)[i]->racer;
        prevOrder = racer->racerOrder;
        for (j = 0; j < gNumRacers; j++) {
            if (j == i) {
                continue;
            }
            other = (*gRacers)[j]->racer;
            if (racer->raceFinished == FALSE && other->raceFinished != FALSE) {
                order++;
            } else if (racer->courseCheckpoint < other->courseCheckpoint) {
                order++;
            } else if (racer->courseCheckpoint == other->courseCheckpoint) {
                if (other->checkpointRank < racer->checkpointRank) {
                    order++;
                }
                if (racer->checkpointRank == other->checkpointRank && i < j) {
                    order++;
                }
            }
        }
        racer->racerOrder = order;
        if (racer->lap < header->laps) {
            if (prevOrder == racer->racerOrder) {
                if (racer->positionStableTimer < 2) {
                    if (racer->vehicleID != VEHICLE_LOOPDELOOP) {
                        racer->positionStableTimer++;
                    }
                } else if (racer->racerOrder != racer->racePosition) {
                    racer->positionChangeTimer = 10;
                    racer->racePosition = racer->racerOrder;
                }
            } else {
                racer->positionStableTimer = 0;
            }
        }
    }
}

/**
 * Racers that have completed the laps are marked finished and given the next finish
 * position. Also counts the humans and the finishers.
 */
static void race_mark_finished_laps(LevelHeader *header, s16 *numHumanRacers, s16 *numHumanRacersFinished,
                                    s16 *numFinishedRacers) {
    Object_Racer *racer;
    s32 i;

    for (i = 0; i < gNumRacers; i++) {
        racer = (*gRacers)[i]->racer;
        if (racer->lap >= header->laps && racer->raceFinished == FALSE) {
            if (get_game_mode() != GAMEMODE_UNUSED_4) {
                racer->raceFinished = TRUE;
                racer->finishPosition = gNumFinishedRacers;
                if (gNumFinishedRacers == 1 && racer->playerIndex == PLAYER_COMPUTER) {
                    sound_play(SOUND_WHOOSH5, NULL);
                }
                gNumFinishedRacers++;
            }
        }
        if (racer->playerIndex != PLAYER_COMPUTER) {
            (*numHumanRacers)++;
            if (racer->raceFinished) {
                (*numHumanRacersFinished)++;
                (*numFinishedRacers)++;
            }
        } else if (racer->raceFinished) {
            (*numFinishedRacers)++;
        }
    }
}

/**
 * Rebuild gRacersByPosition: finished racers by finish position, the rest by current
 * order. Racers that collide on a slot are dropped into the first free one.
 */
static void race_sort_by_position(void) {
    Object_Racer *racer;
    s32 position;
    s32 listed;
    s32 i;
    s32 j;

    for (i = 0; i < gNumRacers; i++) {
        gRacersByPosition[i] = 0;
    }
    for (i = 0; i < gNumRacers; i++) {
        racer = (*gRacers)[i]->racer;
        if (racer->raceFinished) {
            position = racer->finishPosition - 1;
        } else {
            position = racer->racerOrder - 1;
        }
        gRacersByPosition[position] = (*gRacers)[i];
    }
    for (i = 0; i < gNumRacers; i++) {
        listed = FALSE;
        for (j = 0; j < gNumRacers; j++) {
            if (gRacersByPosition[j] == (*gRacers)[i]) {
                listed = TRUE;
                break;
            }
        }
        if (listed == FALSE) {
            for (j = 0; j < gNumRacers; j++) {
                if (gRacersByPosition[j] == 0) {
                    gRacersByPosition[j] = (*gRacers)[i];
                    break;
                }
            }
        }
    }
}

/**
 * Once the race is decided, the racers still going are finished in their current order and
 * the humans among them get the finish-line camera.
 */
static void race_finish_remaining_racers(void) {
    Object_Racer *racer;
    s32 i;

    for (i = 0; i < gNumRacers; i++) {
        racer = gRacersByPosition[i]->racer;
        if (racer->raceFinished == FALSE) {
            if (racer->playerIndex >= 0) {
                set_active_camera(racer->playerIndex);
                cam_get_active_camera_no_cutscenes()->mode = CAMERA_FINISH_CHALLENGE;
            }
            racer->raceFinished = TRUE;
            racer->finishPosition = gNumFinishedRacers;
            gNumFinishedRacers++;
        }
    }
}

/**
 * The end of a normal, Horseshoe Gulch or boss race. The race is over when every human has
 * finished (or all but one racer, with several humans), or when a shared adventure's first
 * finisher has already cleared the course. A first clear in adventure mode awards a balloon
 * and plays the sequence; otherwise the results screen follows.
 */
static void race_finish_default(Settings *settings, s16 numHumanRacers, s16 numHumanRacersFinished,
                                s16 numFinishedRacers) {
    Object_Racer *racer;
    s32 adventureClearedEarly;
    s32 awardBalloon;
    s32 i;

    adventureClearedEarly = FALSE;
    if (is_in_two_player_adventure() && numHumanRacersFinished > 0 && get_trophy_race_world_id() == 0 &&
        set_course_finish_flags(settings) != 0) {
        adventureClearedEarly = TRUE;
    }
    if (!((numHumanRacersFinished == numHumanRacers ||
           (numHumanRacers >= 2 && numFinishedRacers >= (gNumRacers - 1))) ||
          adventureClearedEarly)) {
        return;
    }
    if (numHumanRacersFinished != numHumanRacers) {
        race_finish_remaining_racers();
    }
    if (!gIsTimeTrial) {
        for (i = 0; i < gNumRacers; i++) {
            racer = gRacersByPosition[i]->racer;
            settings->racers[racer->racerIndex].starting_position = i;
        }
    }
    gSwapLeadPlayer = FALSE;
    if (is_in_two_player_adventure() &&
        (settings->racers[PLAYER_TWO].starting_position < settings->racers[PLAYER_ONE].starting_position)) {
        gSwapLeadPlayer = TRUE;
    }
    racer = (*gRacersByPosition)->racer;
    gFirstTimeFinish = FALSE;
    if ((settings->gNumRacers == 1 || is_in_two_player_adventure()) && racer->playerIndex != PLAYER_COMPUTER &&
        !is_in_tracks_mode() && get_trophy_race_world_id() == 0) {
        gFirstTimeFinish = TRUE;
    }
    awardBalloon = FALSE;
    if (gFirstTimeFinish && !adventureClearedEarly) {
        awardBalloon = set_course_finish_flags(settings);
    }
    if (adventureClearedEarly) {
        gFirstTimeFinish = TRUE;
        awardBalloon = TRUE;
    }
    if (!awardBalloon) {
        race_apply_lead_player_swap();
        postrace_start(gFirstTimeFinish, 30);
    } else {
        settings->balloonsPtr[settings->worldId]++;
        if (settings->worldId != 0) {
            settings->balloonsPtr[0]++;
        }
        race_finish_adventure(TRUE);
    }
    gRaceFinishTriggered = -1; // -1 doesn't do anything different.
    if (get_number_of_active_players() == 1) {
        race_finish_time_trial();
    }
}

/**
 * Check the win conditions of the current race.
 * This varies based on the race type, so this does a multitude of different possible things.
 * When the race is finished, it will then for the most part trigger the next menu,
 * but adventure mode will start the balloon cutscene if it has not yet been awarded.
 */
void race_check_finish(s32 updateRate) {
    Settings *settings;
    LevelHeader *header;
    Object_Racer *racer;
    s16 numHumanRacers;
    s16 numHumanRacersFinished;
    s16 numFinishedRacers;
    s8 raceType;

    header = level_header();
    settings = get_settings();
    raceType = header->race_type;
    if (raceType != RACETYPE_DEFAULT && raceType != RACETYPE_HORSESHOE_GULCH && raceType != RACETYPE_BOSS) {
        if (raceType & RACETYPE_CHALLENGE) {
            race_check_finish_challenge(settings, header);
        }
        return;
    }

    race_update_positions(header);
    numHumanRacers = 0;
    numHumanRacersFinished = 0;
    numFinishedRacers = 0;
    race_mark_finished_laps(header, &numHumanRacers, &numHumanRacersFinished, &numFinishedRacers);
    race_sort_by_position();

    if (gIsTajChallenge && numHumanRacersFinished != 0) {
        mode_end_taj_race(CHALLENGE_END_FINISH);
    } else if (gBossRaceMode != 0 && numFinishedRacers != 0) {
        // The boss race ends for the player as soon as either racer crosses the line.
        racer = (*gRacers)[0]->racer;
        if (!racer->raceFinished) {
            racer->raceFinished = TRUE;
            racer->finishPosition = gNumFinishedRacers;
            gNumFinishedRacers++;
        }
    } else if (gRaceFinishTriggered == FALSE) {
        race_finish_default(settings, numHumanRacers, numHumanRacersFinished, numFinishedRacers);
    }
}

/**
 * Mark the course as finished for the appropriate mode.
 * Check if it's the first race or the silver coin race before deciding which flag to write.
 * Return whether something was written.
 */
s8 set_course_finish_flags(Settings *settings) {
    Object_Racer *racer;

    racer = gRacersByPosition[PLAYER_ONE]->racer;
    if (racer->playerIndex == PLAYER_COMPUTER) {
        return FALSE;
    }
    gFirstTimeFinish = FALSE;
    if (!(settings->courseFlagsPtr[settings->courseId] & RACE_CLEARED)) {
        if (gIsTimeTrial == FALSE) {
            gFirstTimeFinish = TRUE;
            settings->courseFlagsPtr[settings->courseId] |= RACE_CLEARED;
        }
    } else if (gIsSilverCoinRace && racer->silverCoinCount >= 8 && gIsTimeTrial == FALSE) {
        gFirstTimeFinish = TRUE;
        settings->courseFlagsPtr[settings->courseId] |= RACE_CLEARED_SILVER_COINS;
    }
    return gFirstTimeFinish;
}

/**
 * Sets the countdown ready for the level to fade out.
 * Begins the timer that exits the level in 5 seconds.
 */
void race_finish_adventure(UNUSED s32 unusedArg) {
    gRaceEndTimer = 300;
    gRaceEndStage = 0;
    unused_D_8011AD52 = unusedArg;
}

/**
 * Begins counting down, once it reaches 0, start stopping all the race behaviours.
 * This function is also where the Taj Balloon cutscene is also handled.
 * After that's done, write to save and send the player back to the hub.
 */
void race_transition_adventure(s32 updateRate) {
    s32 i;
    Object_Racer *racer;
    Object *prevPort0Racer;
    s32 sp30;
    Object *prevRacer0Obj;
    Settings *settings;
    u32 cutsceneTimerLimit;

    set_pause_lockout_timer(1);
    sp30 = gRaceEndTimer;
    gRaceEndTimer -= updateRate;
    if (gRaceEndTimer <= 0) {
        gRaceEndTimer = -1;
    }
    if (sp30 > 50 && gRaceEndTimer <= 50) {
        transition_begin(&gRaceEndFade);
    }
    sp30 = 0;
    if (gRaceEndStage == 0 && gRaceEndTimer == -1) {
        for (i = 0; i < gNumRacers; i++) {
            racer = (*gRacers)[i]->racer;
            racer->magnetTargetObj = NULL;
            if (racer->playerIndex != PLAYER_COMPUTER && racer->finishPosition == 1) {
                sp30 = i;
            }
            if (racer->magnetSoundMask != NULL) {
                sndp_stop(racer->magnetSoundMask);
            }
            if (racer->shieldSoundMask != NULL) {
                audspat_point_stop(racer->shieldSoundMask);
            }
        }
        prevPort0Racer = (*gRacers)[0];
        (*gRacers)[0] = (*gRacers)[sp30];
        (*gRacers)[sp30] = prevPort0Racer;
        racer_sound_free((*gRacers)[0]);
        hud_audio_init();
        reset_rocket_sound_timer();
        sndp_stop_all_looped();
        if (is_in_two_player_adventure()) {
            set_scene_viewport_num(0);
            cam_set_layout(VIEWPORT_LAYOUT_1_PLAYER);
            prevRacer0Obj = (*gRacers)[0];
            prevPort0Racer = gRacersByPort[0];
            racer = prevRacer0Obj->racer;
            gRacersByPort[0] = prevRacer0Obj;
            gRacersByPort[1] = prevPort0Racer;
            if (gSwapLeadPlayer != 0) {
                gSwapLeadPlayer = 0;
                swap_lead_player();
                racer->playerIndex = 0;
            }
        }
        gNumRacersSaved = gNumRacers;
        gRaceEndStage = 1;
    }
    if (gRaceEndStage == 1) {
        hud_visibility(FALSE);
        gNumRacersSaved--;
        if (gNumRacersSaved > 0) {
            i = 0;
            while (i < gNumRacers && gRacersByPosition[i] != (*gRacers)[gNumRacersSaved]) {
                i++;
            }
            if (i < gNumRacers) {
                for (; i < (gNumRacers - 1); i++) {
                    gRacersByPosition[i] = gRacersByPosition[i + 1];
                }
            }
            free_object((*gRacers)[gNumRacersSaved]);
            (*gRacers)[gNumRacersSaved] = NULL;
            gNumRacers--;

        } else {
            gRaceEndStage = 2;
        }
    }
    if (gRaceEndStage == 2) {
        prevPort0Racer = (*gRacers)[0];
        racer = prevPort0Racer->racer;
        racer_find_start_node(prevPort0Racer, racer);
        racer->raceFinished = FALSE;
        gRaceEndStage = 3;
        cutscene_set(CUTSCENE_ID_UNK_A);
        gBalloonCutsceneTimer = 0;
        anim_objects_update();
    }
    if (gRaceEndStage == 3) {
        transition_begin(&gRaceEndTransition);
        gRaceEndStage = 4;
        set_anti_aliasing(TRUE);
    }
    if (gRaceEndStage == 4) {
        set_anti_aliasing(TRUE);
        disable_racer_input();
        if (!(level_type() & RACETYPE_CHALLENGE_BATTLE)) {
            if (osTvType == OS_TV_TYPE_PAL) {
                cutsceneTimerLimit = 415;
            } else {
                cutsceneTimerLimit = 540;
            }
            gBalloonCutsceneTimer += updateRate;
            if (gBalloonCutsceneTimer < cutsceneTimerLimit) {
                minimap_fade(1);
            } else {
                hud_visibility(1);
            }
        }
        i = input_pressed(PLAYER_ONE) & A_BUTTON;
        settings = get_settings();
        if (!(settings->cutsceneFlags & 0x40000)) {
            i = 0;
        }
        if (anim_prev_frame_state() != 0 || (i != 0 && check_fadeout_transition() == 0)) {
            if (i != 0) {
                transition_begin(&gBalloonCutsceneTransition);
            }
            level_transition_begin(2);
            gRaceEndStage = 5;
            settings->cutsceneFlags |= 0x40000;
        }
    }
}

/**
 * Returns the race finish timer.
 */
s16 race_finish_timer(void) {
    return gRaceEndTimer;
}

/**
 * Return the timer used for the collectable balloon cutscene.
 */
u32 get_balloon_cutscene_timer(void) {
    return gBalloonCutsceneTimer;
}

/**
 * Checks if the fastest lap or the race time is faster than the current record.
 * Save those if they are, and if the staff ghost is not enabled, enable it for the next attempt.
 * The staff ghost comparison is also called here.
 */
void race_finish_time_trial(void) {
    s32 bestCourseTime;
    s32 bestRacerTime;
    s32 i;
    s32 courseTime;
    s32 vehicleID;
    s32 curRacerLapTime;
    s32 j;
    Object_Racer *curRacer;
    Object_Racer *bestRacer;
    Settings *settings;
    LevelHeader *levelHeader;

    levelHeader = level_header();
    settings = get_settings();
    settings->timeTrialRacer = 0;
    settings->bestLapRacerAndLap[1] = 0;
    settings->bestLapRacerAndLap[0] = 0;
    bestCourseTime = 36001;
    bestRacerTime = 36001;
    bestRacer = gRacersByPosition[0]->racer;
    for (i = 0; i < gNumRacers; i++) {
        curRacer = gRacersByPosition[i]->racer;
        if (curRacer->racerIndex >= 0) {
            if (curRacer->racerIndex < get_number_of_active_players()) {
                settings->racers[curRacer->racerIndex].best_times = 0;
                vehicleID = curRacer->vehicleIDPrev;
                if (vehicleID >= VEHICLE_CAR && vehicleID < NUMBER_OF_PLAYER_VEHICLES) {
                    courseTime = 0;
                    for (j = 0; j < levelHeader->laps && j < 5; j++) {
                        settings->racers[curRacer->racerIndex].lap_times[j] = curRacer->lap_times[j];
                        curRacerLapTime = curRacer->lap_times[j];
                        courseTime += curRacerLapTime;
                        if (curRacerLapTime < bestRacerTime) {
                            settings->bestLapRacerAndLap[1] = j;
                            settings->bestLapRacerAndLap[0] = curRacer->racerIndex;
                            bestRacerTime = curRacerLapTime;
                        }
                    }
                    settings->racers[curRacer->racerIndex].course_time = courseTime;
                    if (courseTime < bestCourseTime) {
                        bestCourseTime = courseTime;
                        settings->timeTrialRacer = curRacer->racerIndex;
                        bestRacer = curRacer;
                    }
                }
            }
        }
    }
    settings->display_times = FALSE;
    if (gIsTimeTrial) {
        vehicleID = gPrevTimeTrialVehicle;
        if (vehicleID >= NUMBER_OF_PLAYER_VEHICLES || vehicleID < VEHICLE_CAR) {
            vehicleID = VEHICLE_CAR;
        }
        settings->display_times = TRUE;
        if (settings->bestLapRacerAndLap[0] == 0) {
            if ((settings->flapTimesPtr[vehicleID][settings->courseId] == NULL) ||
                (bestRacerTime < settings->flapTimesPtr[vehicleID][settings->courseId])) {
                settings->flapTimesPtr[vehicleID][settings->courseId] = bestRacerTime;
                settings->racers[settings->bestLapRacerAndLap[0]].best_times |= 1 << settings->bestLapRacerAndLap[1];
            }
        }
        if (settings->timeTrialRacer == 0) {
            if ((settings->courseTimesPtr[vehicleID][settings->courseId] == NULL) ||
                (bestCourseTime < settings->courseTimesPtr[vehicleID][settings->courseId])) {
                settings->courseTimesPtr[vehicleID][settings->courseId] = bestCourseTime;
                settings->racers[settings->timeTrialRacer].best_times |= 0x80;
            }
        }
        if (((!vehicleID) && (!vehicleID)) && (!vehicleID)) {} // Fakematch
        if (settings->timeTrialRacer == 0) {
            if (bestCourseTime < 10800 && (vehicleID != gTimeTrialVehicle || timetrial_map_id() != level_id() ||
                                           bestCourseTime < gTimeTrialTime)) {
                gTimeTrialTime = bestCourseTime;
                gTimeTrialVehicle = gPrevTimeTrialVehicle;
                gTimeTrialCharacter = settings->racers[0].character;
                timetrial_swap_player_ghost(level_id());
                gHasGhostToSave = TRUE;
            }
            if (osTvType == OS_TV_TYPE_PAL) {
                bestCourseTime = (bestCourseTime * 6) / 5;
            }
            if (bestCourseTime < gTTGhostTimeToBeat) {
                if (gTimeTrialStaffGhost) {
                    tt_ghost_beaten(level_id(), &bestRacer->playerIndex);
                } else {
                    hud_time_trial_message(&bestRacer->playerIndex);
                }
            } else {
                hud_time_trial_message(&bestRacer->playerIndex);
            }
        }
    }
}

/**
 * Returns true if the player ghost data is valid for playback.
 */
s32 timetrial_valid_player_ghost(void) {
    if (timetrial_map_id() != level_id()) {
        return FALSE;
    } else {
        if (gTimeTrialVehicle != gPrevTimeTrialVehicle) {
            return FALSE;
        } else {
            return TRUE;
        }
    }
}

/**
 * Return the player ghost object.
 */
Object *timetrial_player_ghost(void) {
    return gGhostObjPlayer;
}

/**
Pretty sure this determines whether or not you're eligible to race TT ghost in track select
when TT is on. It looks like it checks some ghost data makes sure you've got a ghost for that level
with the default vehicle,
Returns 0 if TT ghost was loaded successfully.
*/
s32 timetrial_load_staff_ghost(s32 mapId) {
    TTGhostTable *ghostTable;
    TTGhostTable *prevGhostTable;
    s32 ret;
    TTGhostTable *nextGhostTable;

    gMapDefaultVehicle = leveltable_vehicle_default(mapId);
    ghostTable = (TTGhostTable *) asset_table_load(ASSET_TTGHOSTS_TABLE);
    {
        // Big-endian entries: mapId/defaultVehicleId are u8, ghostOffset is s32.
        s32 n;
        _Static_assert(sizeof(TTGhostTable) == 8, "TTGhostTable layout drifted from N64");
        for (n = 0; n < (s32) (asset_table_size(ASSET_TTGHOSTS_TABLE) / sizeof(TTGhostTable)); n++) {
            pc_swap32_buf(&ghostTable[n].ghostOffset, 4);
        }
    }

    nextGhostTable = ghostTable;
    do {
        prevGhostTable = nextGhostTable;
        if ((prevGhostTable->mapId == mapId) && (prevGhostTable->defaultVehicleId == gMapDefaultVehicle)) {
            break;
        }
        nextGhostTable++;
    } while (prevGhostTable->mapId != 0xFF);

    ret = 1;

    if (prevGhostTable->mapId != 0xFF) {
        ret = load_tt_ghost(nextGhostTable->ghostOffset, nextGhostTable[1].ghostOffset - nextGhostTable->ghostOffset,
                            &gTTGhostTimeToBeat);
    }

    mempool_free(ghostTable);
    return ret;
}

/**
 * Return true if this object is the time trial ghost.
 */
s32 timetrial_staff_ghost_check(Object *obj) {
    return obj == gGhostObjStaff;
}

/**
 * Free ghost data then save the players victory.
 * Check if the player has beaten every time and unlock TT.
 * Otherwise, tell them to try another track.
 */
void tt_ghost_beaten(s32 arg0, s16 *playerId) {
    s32 trackIdCount;
    s8 *mainTrackIds;

    gGhostObjStaff = NULL;
    timetrial_free_staff_ghost();
    gTimeTrialStaffGhost = FALSE;
    mainTrackIds = (s8 *) get_misc_asset(ASSET_MISC_MAIN_TRACKS_IDS);
    trackIdCount = 0;
    while (mainTrackIds[trackIdCount] != -1 && mainTrackIds[trackIdCount] != arg0) {
        trackIdCount++;
    }
    if (gBeatStaffGhost) {
        // Save that TT has been beaten for this track.
        set_eeprom_settings_value(16 << trackIdCount);
        // Check if TT has been beaten for all tracks.
        if ((get_eeprom_settings() & 0xFFFFF0) == 0xFFFFF0) {
            set_magic_code_flags(CHEAT_CONTROL_TT);
            sound_play(SOUND_VOICE_TT_BEAT_ALL_TIMES, NULL);
            sound_play_delayed(SOUND_VOICE_TT_UNLOCKED, NULL, 1.5f);
            set_current_text(
                ASSET_GAME_TEXT_84); // Text for "You have beaten all my times!" and then "Now you can PICK me!"
        } else {
            sound_play(SOUND_VOICE_TT_WELL_DONE, NULL);
            sound_play_delayed(SOUND_VOICE_TT_TRY_ANOTHER_TRACK, NULL, 1.0f);
            set_current_text(ASSET_GAME_TEXT_83); // Text for "Well Done! Now try another track."
        }
        gBeatStaffGhost = FALSE;
        return;
    }
    hud_time_trial_message(playerId);
}

/**
 * Compare if the course record is enough to unlock the staff ghost.
 * Also check if the ghost tiself has been beaten.
 * Store both results and return if there should be a ghost.
 */
u8 timetrial_init_staff_ghost(s32 trackId) {
    s32 i;
    s8 *mainTrackIds;
    u16 *staffTime;
    Settings *settings;

    gBeatStaffGhost = FALSE;
    gTimeTrialStaffGhost = FALSE;
    settings = get_settings();
    if (leveltable_vehicle_default(trackId) == (Vehicle) gPrevTimeTrialVehicle) {
        mainTrackIds = (s8 *) get_misc_asset(ASSET_MISC_MAIN_TRACKS_IDS);
        staffTime = (u16 *) get_misc_asset(ASSET_MISC_GHOST_UNLOCK_TIMES);
        for (i = 0; mainTrackIds[i] != -1 && trackId != mainTrackIds[i]; i++) {}
        if (mainTrackIds[i] != -1) {
            if (staffTime[i] >= settings->courseTimesPtr[gPrevTimeTrialVehicle][trackId]) {
                // Check if TT has been beaten?
                if (!(get_eeprom_settings() & ((1 << 4) << i))) {
                    gBeatStaffGhost = TRUE;
                }
                if (timetrial_load_staff_ghost(trackId) == 0) {
                    gTimeTrialStaffGhost = TRUE;
                }
            }
        }
    }
    return gTimeTrialStaffGhost;
}

/**
 * Return the time trial staff ghost object.
 */
Object *timetrial_ghost_staff(void) {
    return gGhostObjStaff;
}

/**
 * Return true if the tt ghost is unbeaten for this track.
 */
s32 timetrial_staff_unbeaten(void) {
    return gBeatStaffGhost == FALSE;
}

/**
 * Calls a function to start loading the player ghost data from the controller pak.
 * Returns the controller pak status. 0 means good.
 */
s32 timetrial_init_player_ghost(s32 playerID) {
    s16 characterID;
    s16 time;
    s32 cpakStatus;
    s32 ghostMapID;

    ghostMapID = timetrial_map_id();
    if (level_id() != ghostMapID || gTimeTrialVehicle != gPrevTimeTrialVehicle) {
        cpakStatus = timetrial_load_player_ghost(playerID, level_id(), gPrevTimeTrialVehicle, &characterID, &time);
        if (cpakStatus == CONTROLLER_PAK_GOOD) {
            gTimeTrialVehicle = gPrevTimeTrialVehicle;
            gTimeTrialCharacter = characterID;
            gTimeTrialTime = time;
        }
        return cpakStatus;
    }
    return timetrial_load_player_ghost(playerID, level_id(), gPrevTimeTrialVehicle, NULL, NULL);
}

/**
 * Call a function to write the ghost data to the controller pak.
 * Returns the controller pak status. 0 is good.
 */
SIDeviceStatus timetrial_save_player_ghost(s32 controllerIndex) {
    return timetrial_write_player_ghost(controllerIndex, timetrial_map_id(), gTimeTrialVehicle, gTimeTrialCharacter,
                                        gTimeTrialTime);
}

/**
 * Returns whether there's valid ghost data to save.
 */
u8 has_ghost_to_save(void) {
    return gHasGhostToSave;
}

/**
 * Resets the variables used for ghost data saving.
 */
void set_ghost_none(void) {
    gGhostMapID = -1;
    gHasGhostToSave = FALSE;
}

/**
 * Finds the opponent to this racer in a relative position to them and calculates the distance to them.
 * The position argument is relative to the racer's current position and represents the number of
 * places ahead (positive) or behind (negative) the opponent is. So, for instance, if position is 1,
 * find the opponent one place ahead of the racer; if it's -1, find the opponent one place behind.
 */
Object *racer_find_nearest_opponent_relative(Object_Racer *racer, s32 position, f32 *distance) {
    UNUSED s32 pad;
    Object *tempRacerObj;
    position = (racer->racerOrder - position) - 1;
    if (position < 0 || position >= gNumRacers) {
        return NULL;
    }
    tempRacerObj = gRacersByPosition[position];
    if (tempRacerObj == NULL) {
        return NULL;
    }
    *distance = racer_calc_distance_to_opponent(racer, tempRacerObj->racer);
    return tempRacerObj;
}

/**
 * Calculates the distance between two racers. Traverses through the checkpoints
 * between the two racers to calculate the total distance.
 * If the second racer is ahead, the distance is positive, otherwise negative.
 */
f32 racer_calc_distance_to_opponent(Object_Racer *racer1, Object_Racer *racer2) {
    Object_Racer *temp_racer;
    f32 totalDistance;
    s32 r1_ccp;
    UNUSED s32 pad;
    s32 reverseOrder;
    s32 checkpointID;

    if (gNumberOfMainCheckpoints <= 0) {
        return 0.0f;
    }
    totalDistance = 0.0f;
    reverseOrder = FALSE;
    if (racer2->courseCheckpoint < racer1->courseCheckpoint) {
        temp_racer = racer1;
        racer1 = racer2;
        racer2 = temp_racer;
        reverseOrder = TRUE;
    }
    checkpointID = racer1->nextCheckpoint;
    for (r1_ccp = racer1->courseCheckpoint; r1_ccp < racer2->courseCheckpoint; r1_ccp++) {
        totalDistance += gTrackCheckpoints[checkpointID++].distance;
        if (checkpointID == gNumberOfMainCheckpoints) {
            checkpointID = 0;
        }
    }
    checkpointID = racer1->nextCheckpoint - 1;
    if (checkpointID < 0) {
        checkpointID = gNumberOfMainCheckpoints - 1;
    }
    totalDistance += (gTrackCheckpoints[checkpointID].distance * racer1->checkpoint_distance);
    checkpointID = racer2->nextCheckpoint - 1;
    if (checkpointID < 0) {
        checkpointID = gNumberOfMainCheckpoints - 1;
    }
    totalDistance -= (gTrackCheckpoints[checkpointID].distance * racer2->checkpoint_distance);
    if (reverseOrder) {
        totalDistance = -totalDistance;
    }
    return totalDistance;
}

/**
 * Traverses from the racer's position through the upcoming checkpoints
 * to the starting line and calculates the total distance.
 */
UNUSED f32 race_calc_distance_to_start_line(Object_Racer *racer) {
    f32 distLeft;
    s32 checkpointID;

    if (gNumberOfMainCheckpoints <= 0) {
        return 0.0f;
    }
    distLeft = 0.0f;
    for (checkpointID = racer->nextCheckpoint; checkpointID < gNumberOfMainCheckpoints; checkpointID++) {
        distLeft += gTrackCheckpoints[checkpointID].distance;
    }
    checkpointID = racer->nextCheckpoint - 1;
    if (checkpointID < 0) {
        checkpointID = gNumberOfMainCheckpoints - 1;
    }
    distLeft += (gTrackCheckpoints[checkpointID].distance * racer->checkpoint_distance);
    return distLeft;
}

/**
 * Returns a pointer to a specific checkpoint.
 */
CheckpointNode *get_checkpoint_node(s32 checkpointID) {
    return &gTrackCheckpoints[checkpointID];
}

/**
 * Takes the position along the checkpoint path, and finds the next applicable node.
 * If an alternative path is available, use that node instead.
 */
CheckpointNode *find_next_checkpoint_node(s32 splinePos, s32 isAlternate) {
    CheckpointNode *checkpointNode = &gTrackCheckpoints[splinePos];
    if (isAlternate != 0 && checkpointNode->altRouteID != -1) {
        checkpointNode = &gTrackCheckpoints[checkpointNode->altRouteID];
    }
    return checkpointNode;
}

/**
 * Returns the number of active checkpoints in the current level.
 */
s32 get_checkpoint_count(void) {
    return gNumberOfMainCheckpoints;
}

/**
 * Returns the group of racer objects.
 * Official Name: objGetPlayerlist
 */
Object **get_racer_objects(s32 *numRacers) {
    *numRacers = gNumRacers;
    return *gRacers;
}

/**
 * Returns the group of racer objects, ordered by player index.
 */
Object **get_racer_objects_by_port(s32 *numRacers) {
    *numRacers = gNumRacers;
    return gRacersByPort;
}

/**
 * Returns the group of racer objects, ordered by current race position.
 */
Object **get_racer_objects_by_position(s32 *numRacers) {
    *numRacers = gNumRacers;
    return gRacersByPosition;
}

/**
 * Returns the racer object specified by the ID
 */
Object *get_racer_object(s32 index) {
    if (gNumRacers == 0) {
        return NULL;
    }
    if (index < 0 || index >= gNumRacers) {
        return NULL;
    }
    return (*gRacers)[index];
}

/**
 * Returns the racer object specified by the player ID.
 * Official name: objGetPlayerNo
 */
Object *get_racer_object_by_port(s32 index) {
    if (gNumRacers == 0) {
        return NULL;
    }
    if (index < 0 || index >= gNumRacers) {
        return NULL;
    }
    return gRacersByPort[index];
}

/**
 * Unused function that would've iterated through all active checkpoints to render their visual nodes.
 * The function it calls is completely stubbed out.
 */
UNUSED void debug_render_checkpoints(Gfx **dList, Mtx **mtx, Vertex **vtx) {
    s32 i;

    material_set_no_tex_offset(dList, NULL, RENDER_Z_COMPARE);
    if (gNumberOfMainCheckpoints > 3) {
        for (i = 0; i < gNumberOfMainCheckpoints; i++) {
            // Ground path
            debug_render_checkpoint_node(i, 0, dList, mtx, vtx);
        }
        for (i = 0; i < gNumberOfMainCheckpoints; i++) {
            // Air path
            debug_render_checkpoint_node(i, 1, dList, mtx, vtx);
        }
    }
}

/**
 * Would've rendered an individual checkpoint node. On https://noclip.website, with dev objects enabled, you can see a
 * visual representation of what these checkpoints would've looked like ingame.
 */
UNUSED void debug_render_checkpoint_node(UNUSED s32 checkpointID, UNUSED s32 pathID, UNUSED Gfx **dList,
                                         UNUSED Mtx **mtx, UNUSED Vertex **vtx) {
}

/**
 * Loop through every existing spectate camera and sort them by index.
 */
void spectate_update(void) {
    Object *objPtr;
    Object *temp;
    s32 continueLoop;
    s32 i;

    gCameraObjCount = 0;
    for (i = 0; i < gObjectCount; i++) {
        objPtr = gObjPtrList[i];
        if (!(objPtr->trans.flags & OBJ_FLAGS_PARTICLE)) {
            if (objPtr->behaviorId == BHV_CAMERA_CONTROL) {
                if (gCameraObjCount < CAMCONTROL_COUNT) {
                    (*gCameraObjList)[gCameraObjCount] = objPtr;
                    gCameraObjCount++;
                }
            }
        }
    }

    do {
        continueLoop = TRUE;
        for (i = 0; i < gCameraObjCount - 1; i++) {
            objPtr = (*gCameraObjList)[i + 1];
            temp = (*gCameraObjList)[i];
            if (temp->properties.camControl.cameraID > objPtr->properties.camControl.cameraID) {
                (*gCameraObjList)[i] = (*gCameraObjList)[i + 1];
                (*gCameraObjList)[i + 1] = temp;
                continueLoop = FALSE;
            }
        }
    } while (!continueLoop);
}

Object *spectate_object(s32 cameraIndex) {
    if (cameraIndex < 0 || cameraIndex >= gCameraObjCount) {
        return NULL;
    }
    return (*gCameraObjList)[cameraIndex];
}

/**
 * Take the current camera passed through the function and compare distances between the next and previous camera.
 * Set the camera to be whichever's closest to the object.
 */
Object *spectate_nearest(Object *obj, s32 *cameraId) {
    Object *nextCamera;
    Object *prevCamera;
    Object *currCamera;
    s32 *cameraIndex;
    f32 x;
    f32 y;
    f32 z;
    f32 prevCameraXYZ;
    f32 currCameraXYZ;
    f32 nextCameraXYZ;
    s32 cameraIndex_Curr;
    s32 cameraIndex_Prev;
    s32 cameraIndex_Next;
    cameraIndex = cameraId;
    if (gCameraObjCount == 0) {
        return NULL;
    }
    cameraIndex_Next = *cameraIndex + 1;
    cameraIndex_Curr = *cameraIndex;
    cameraIndex_Prev = *cameraIndex - 1;
    if (cameraIndex_Next >= gCameraObjCount) {
        cameraIndex_Next = 0;
    }
    if (cameraIndex_Prev < 0) {
        cameraIndex_Prev = gCameraObjCount - 1;
    }
    currCamera = (*gCameraObjList)[cameraIndex_Curr];
    nextCamera = (*gCameraObjList)[cameraIndex_Next];
    prevCamera = (*gCameraObjList)[cameraIndex_Prev];
    x = currCamera->trans.x_position - obj->trans.x_position;
    y = currCamera->trans.y_position - obj->trans.y_position;
    z = currCamera->trans.z_position - obj->trans.z_position;
    currCameraXYZ = (x * x) + (y * y) + (z * z);
    x = nextCamera->trans.x_position - obj->trans.x_position;
    y = nextCamera->trans.y_position - obj->trans.y_position;
    z = nextCamera->trans.z_position - obj->trans.z_position;
    nextCameraXYZ = (x * x) + (y * y) + (z * z);
    x = prevCamera->trans.x_position - obj->trans.x_position;
    y = prevCamera->trans.y_position - obj->trans.y_position;
    z = prevCamera->trans.z_position - obj->trans.z_position;
    prevCameraXYZ = (x * x) + (y * y) + (z * z);

    if (nextCameraXYZ < currCameraXYZ) {
        *cameraId = cameraIndex_Next;
        currCamera = nextCamera;
        currCameraXYZ = nextCameraXYZ;
    }
    if (prevCameraXYZ < currCameraXYZ) {
        *cameraId = cameraIndex_Prev;
        currCamera = prevCamera;
    }
    return currCamera;
}

/**
 * Take every existing AI node and find each neighbouring node.
 * Afterwards, sort them by height so the game can generate elevation thresholds.
 */
void ainode_update(void) {
    LevelObjectEntry_AiNode *aiNodeEntry;
    Object *obj;
    s16 nodeCount;
    Object *nextAiNodeObj;
    f32 diffX;
    f32 diffZ;
    f32 diffY;
    s32 i;
    s32 j;
    s8 index;  // Must be an s8
    u8 index2; // Must be an u8
    s16 swap;
    Object_AiNode *aiNodeObj64;
    s8 nodeIDs[AINODE_COUNT];
    s8 elevations[AINODE_COUNT];

    if (gInitAINodes == FALSE) {
        return;
    }
    gInitAINodes = FALSE;
    for (i = 0; i < AINODE_COUNT; i++) {
        (*gAINodes)[i] = NULL;
    }
    nodeCount = 0;
    // Store each existing node ID in the temporary vars.
    for (i = 0; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE) && obj->behaviorId == BHV_AINODE) {
            aiNodeEntry = &obj->level_entry->aiNode;
            index2 = aiNodeEntry->nodeID;
            if (!(index2 & AINODE_COUNT)) {
                (*gAINodes)[index2] = obj;
                nodeIDs[nodeCount] = aiNodeEntry->nodeID;
                elevations[nodeCount] = aiNodeEntry->elevation & 3;
                nodeCount++;
            }
        }
    }
    if (nodeCount == 0) {
        return;
    }
    // Find and set each neighbouring node and distance for each existing node.
    for (i = 0; i < AINODE_COUNT; i++) {
        obj = (*gAINodes)[i];
        if (obj != NULL) {
            aiNodeObj64 = obj->ai_node;
            aiNodeEntry = &obj->level_entry->aiNode;
            for (j = 0; j < 4; j++) {
                index2 = aiNodeEntry->adjacent[j];
                if (!(index2 & AINODE_COUNT)) {
                    nextAiNodeObj = (*gAINodes)[index2];
                    aiNodeObj64->nodeObj[j] = nextAiNodeObj;
                    if (nextAiNodeObj == NULL) {
                        aiNodeEntry->adjacent[j] = -1;
                    } else {
                        diffX = obj->trans.x_position - nextAiNodeObj->trans.x_position;
                        diffY = obj->trans.y_position - nextAiNodeObj->trans.y_position;
                        diffZ = obj->trans.z_position - nextAiNodeObj->trans.z_position;
                        aiNodeObj64->distToNode[j] = sqrtf((diffX * diffX) + (diffY * diffY) + (diffZ * diffZ));
                    }
                }
            }
        }
    }
    // Sort them by height
    do {
        j = TRUE;
        for (i = 0; i < nodeCount - 1; i++) {
            if ((*gAINodes)[nodeIDs[i + 1]]->trans.y_position < (*gAINodes)[nodeIDs[i]]->trans.y_position) {
                swap = nodeIDs[i];
                nodeIDs[i] = nodeIDs[i + 1];
                nodeIDs[i + 1] = swap;
                swap = elevations[i];
                elevations[i] = elevations[i + 1];
                elevations[i + 1] = swap;
                j = FALSE;
            }
        }
    } while (!j); // Keep doing this until no more swaps are needed.

    if (1) {} // Fakematch

    for (i = 0; i < 5; i++) {
        gElevationHeights[i] = -20000.0f;
    }

    index = elevations[0];
    for (i = 0; i < nodeCount - 1;) {
        while (i < nodeCount - 1 && index >= elevations[i]) {
            i++;
        }
        if (index < elevations[i]) {
            index = elevations[i];
            gElevationHeights[index] =
                ((*gAINodes)[nodeIDs[i]]->trans.y_position + (*gAINodes)[nodeIDs[i - 1]]->trans.y_position) * 0.5;
        } else {
            i = nodeCount;
        }
    }
    gElevationHeights[0] = -10000.0f;
    gElevationHeights[4] = -gElevationHeights[0];
}

/**
 * Compare heights against the thresholds.
 * Elevation level is set based on position.
 */
s16 obj_elevation(f32 yPos) {
    s16 i = 0;
    s16 elevation = 0;
    for (; i < 4; i++) {
        if (gElevationHeights[i] != -20000.0f && gElevationHeights[i] < yPos) {
            elevation = i;
        }
    }
    return elevation;
}

/**
 * Loop through the AI Node list and add this new object to the list if it does not already exist.
 */
s32 ainode_register(Object *obj) {
    s32 i;
    for (i = 0; i < AINODE_COUNT; i++) {
        if ((*gAINodes)[i] == NULL) {
            (*gAINodes)[i] = obj;
            return i;
        }
    }
    return -1;
}

/**
 * Search through each AI node and find the one closest to the coordinates given.
 * Can choose to include or ignore elevation.
 */
s32 ainode_find_nearest(f32 diffX, f32 diffY, f32 diffZ, s32 useElevation) {
    UNUSED f32 pad[6];
    s32 elevation;
    f32 len;
    f32 x;
    f32 z;
    f32 y;
    f32 dist;
    s32 findDist;
    s32 numSteps;
    s32 result;
    Object *obj;
    LevelObjectEntry_AiNode *levelObj;

    if (useElevation) {
        elevation = obj_elevation(diffY);
    }
    dist = 50000.0;
    result = 0xFF;
    for (numSteps = 0; numSteps != AINODE_COUNT; numSteps++) {
        obj = (*gAINodes)[numSteps];
        if (obj) {
            levelObj = &((obj->level_entry)->aiNode);
            findDist = TRUE;
            if (useElevation && elevation != levelObj->elevation) {
                findDist = FALSE;
            }
            if (useElevation == 2 && levelObj->unk8 != 3) {
                findDist = FALSE;
            }
            if (findDist) {
                x = obj->trans.x_position - diffX;
                y = obj->trans.y_position - diffY;
                z = obj->trans.z_position - diffZ;
                len = sqrtf((x * x) + (y * y) + (z * z));
                if (len < dist) {
                    dist = len;
                    result = numSteps;
                }
            }
        }
    }
    return result;
}

/** Moves an NPC along the AI node path and returns its progress. */
f32 npc_follow_ai_nodes(Object_NPC *npc, Object *npcParentObj, f32 updateRateF, f32 speedF, s32 direction) {
    f32 xPositions[5];
    f32 yPositions[5];
    f32 zPositions[5];
    f32 xDiff2;
    f32 yDiff2;
    f32 zDiff2;
    Object *aiNode;
    f32 dist;
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    f32 tempYDiff;
    UNUSED s32 pad_sp84;
    s32 i;
    f32 var_f20_2;
    s32 var_s0;
    s32 someBool;

    if (osTvType == OS_TV_TYPE_PAL) {
        updateRateF *= 1.2;
    }

    for (i = 0; i < 5; i++) {
        if (npc->nodeData[i] == 0xFF) {
            return 0.0f;
        }

        aiNode = ainode_get(npc->nodeData[i]);
        if (aiNode == NULL) {
            return 0.0f;
        }

        xPositions[i] = aiNode->trans.x_position;
        yPositions[i] = aiNode->trans.y_position;
        zPositions[i] = aiNode->trans.z_position;
    }

    xDiff2 = catmull_rom_interpolation(xPositions, 0, npc->splineProgress);
    yDiff2 = catmull_rom_interpolation(yPositions, 0, npc->splineProgress);
    zDiff2 = catmull_rom_interpolation(zPositions, 0, npc->splineProgress);
    someBool = FALSE;
    if (npc->splineStep == 0.0f) {
        npc->splineStep = 0.01f;
    }

    for (var_s0 = 0; var_s0 != 2; var_s0++) {
        var_f20_2 = npc->splineProgress + (npc->splineStep * updateRateF);
        if (var_f20_2 >= 1.0) {
            someBool = TRUE;
            var_f20_2 -= 1.0;
        }
        xDiff = catmull_rom_interpolation(xPositions, someBool, var_f20_2);
        yDiff = catmull_rom_interpolation(yPositions, someBool, var_f20_2);
        zDiff = catmull_rom_interpolation(zPositions, someBool, var_f20_2);
        xDiff -= xDiff2;
        yDiff -= yDiff2;
        zDiff -= zDiff2;
        if (var_s0 == 0) {
            someBool = FALSE;
            dist = sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)) / updateRateF;
            if (dist != 0.0f) {
                npc->splineStep *= (speedF / dist);
            }
        }
    }
    npc->splineProgress = var_f20_2;
    xDiff2 = xDiff + xDiff2;
    tempYDiff = yDiff2 = yDiff + yDiff2;
    zDiff2 = zDiff + zDiff2;

    xDiff = xDiff2 - npcParentObj->trans.x_position;
    yDiff = yDiff2 - npcParentObj->trans.y_position;
    zDiff = zDiff2 - npcParentObj->trans.z_position;

    xDiff2 = xDiff;
    yDiff2 = yDiff;
    zDiff2 = zDiff;

    dist = sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff));
    if (dist != 0.0f) {
        dist = 255.0 / dist;
        xDiff *= dist;
        yDiff *= dist;
        zDiff *= dist;
    }
    dist = sqrtf((xDiff2 * xDiff2) + (yDiff2 * yDiff2) + (zDiff2 * zDiff2)) / 16;
    if (speedF < dist) {
        dist = speedF;
    }
    if (dist >= 1.0) {
        var_s0 = (arctan2_f(xDiff, zDiff) - (npcParentObj->trans.rotation.y_rotation & 0xFFFF)) - 0x8000;
        if (var_s0 > 0x8000) {
            var_s0 -= 0xFFFF;
        }
        if (var_s0 < -0x8000) {
            var_s0 += 0xFFFF;
        }
        npcParentObj->trans.rotation.y_rotation += ((var_s0 * (s32) updateRateF)) >> 4;
        var_s0 = arctan2_f(yDiff, 255.0f) - (npcParentObj->trans.rotation.x_rotation & 0xFFFF);
        if (var_s0 > 0x8000) {
            var_s0 -= 0xFFFF;
        }
        if (var_s0 < -0x8000) {
            var_s0 += 0xFFFF;
        }
        npcParentObj->trans.rotation.x_rotation += ((var_s0 * (s32) updateRateF)) >> 4;
    }

    npcParentObj->trans.rotation.z_rotation = 0;
    xDiff = sins_f((s16) (npcParentObj->trans.rotation.y_rotation + 0x8000)) * dist;
    move_object(npcParentObj, xDiff * updateRateF, 0.0f,
                coss_f((npcParentObj->trans.rotation.y_rotation + 0x8000)) * dist * updateRateF);
    npcParentObj->trans.y_position = tempYDiff;
    dist = dist * updateRateF * 2;
    if (someBool != 0) {
        npc->nodeData[0] = npc->nodeData[1];
        npc->nodeData[1] = npc->nodeData[2];
        npc->nodeData[2] = npc->nodeData[3];
        npc->nodeData[3] = npc->nodeData[4];
        npc->nodeData[4] = ainode_find_next(npc->nodeData[3], npc->nodeData[2], direction);
    }

    return dist;
}

s32 ainode_find_next(s32 nodeId, s32 nextNodeId, s32 direction) {
    Object *aiNodeObj;
    LevelObjectEntry_AiNode *entry;
    Object_AiNode *aiNode;
    s32 nextIndex;
    s32 i;
    s32 someCount;

    if (nodeId < -1 || nodeId >= AINODE_COUNT) {
        return NODE_NONE;
    }
    aiNodeObj = (*gAINodes)[nodeId];
    if (aiNodeObj == NULL) {
        return NODE_NONE;
    }

    entry = &aiNodeObj->level_entry->aiNode;
    aiNode = aiNodeObj->ai_node;
    direction = direction & 3;
    someCount = 0;
    nextIndex = (aiNode->directions[direction] + 1) & 3;

    for (i = 0; i < 4; i++) {
        if (entry->adjacent[nextIndex] != NODE_NONE && entry->adjacent[nextIndex] != nextNodeId) {
            aiNode->directions[direction] = nextIndex;
            i = 4; // break
            someCount++;
        }
        nextIndex = (nextIndex + 1) & 3;
    }

    if (someCount == 0) {
        return NODE_NONE;
    } else {
        return entry->adjacent[aiNode->directions[direction]];
    }
}

/** Chooses the next AI node for a racer from the node graph. */
s16 ainode_pick_next(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s16 result;
    s32 sp370;
    s16 var_t1;
    s16 sp36C;
    s16 temp2;
    s16 i;
    LevelObjectEntry_AiNode *aiNodeEntry;
    s8 someBool;
    s8 var_s3;
    s8 var_s5;
    s8 temp;
    s32 var_ra;
    Object_AiNode *aiNode;
    Object *aiNodeObj;
    s32 sp154[AINODE_COUNT];
    s8 spD4[AINODE_COUNT];
    s8 sp54[AINODE_COUNT];

    // Only matches with do {} while?
    i = 0;
    do {
        sp154[i] = 0;
        i++;
    } while (i < AINODE_COUNT);

    aiNodeObj = (*gAINodes)[arg0];
    aiNode = aiNodeObj->ai_node;
    aiNodeEntry = &aiNodeObj->level_entry->aiNode;
    var_t1 = 0;
    var_ra = 0;
    result = NODE_NONE;
    var_s3 = 0;
    someBool = 1;
    var_s5 = 0;
    do {
        for (i = 0; i < 4; i++) {
            if (aiNode->nodeObj[i] != 0) {
                temp2 = aiNodeEntry->adjacent[i];
                temp = 1;
                if (((arg0 == aiNodeEntry->nodeID) && (temp2 == arg2)) ||
                    ((arg2 == aiNodeEntry->nodeID) && (temp2 == arg0))) {
                    temp = 0;
                    if (var_s5 != 0) {
                        temp = 1;
                    }
                    var_s5++;
                }
                if (temp != 0) {
                    sp36C = 0;
                    temp = 0;
                    while ((sp36C < var_t1) && (temp2 != spD4[sp36C])) {
                        sp36C++;
                    }

                    sp370 = aiNode->distToNode[i] + var_ra;
                    if (sp36C == var_t1) {
                        temp = 2;
                    } else if (sp370 < sp154[temp2]) {
                        temp = 1;
                    }
                }
                if (temp != 0) {
                    while (sp36C < var_t1 - 1) {
                        spD4[sp36C] = spD4[sp36C + 1];
                        sp54[sp36C] = sp54[sp36C + 1];
                        sp36C++;
                    }
                    if (temp == 2) {
                        var_t1++;
                    }
                    sp154[temp2] = sp370;

                    sp36C = var_t1 - 1;

                    sp54[sp36C] = (someBool) ? aiNodeEntry->adjacent[i] : var_s3;
                    spD4[sp36C] = aiNodeEntry->adjacent[i];

                    while ((sp36C > 0) && (sp154[spD4[sp36C - 1]] < sp154[spD4[sp36C]])) {
                        temp = spD4[sp36C];
                        spD4[sp36C] = spD4[sp36C - 1];
                        spD4[sp36C - 1] = temp;
                        temp = sp54[sp36C];
                        sp54[sp36C] = sp54[sp36C - 1] & 0xFF & 0xFF & 0xFF;
                        sp54[sp36C - 1] = temp & 0xFF;
                        sp36C--;
                    }
                }
            }
        }

        if (var_t1 > 0) {
            var_t1--;
            aiNodeObj = (*gAINodes)[spD4[var_t1]];
            aiNodeEntry = &aiNodeObj->level_entry->aiNode;
            var_s3 = sp54[var_t1];
            var_ra = sp154[aiNodeEntry->nodeID];
            aiNode = aiNodeObj->ai_node;
            someBool = 0;
            if (arg1 & 0x100) {
                if ((arg1 & 0x7F) == spD4[var_t1]) {
                    result = var_s3;
                }
            } else if (arg1 == aiNodeEntry->unk8) {
                result = var_s3;
            }
            if ((var_t1 == 0) && (result == 0xFF)) {
                result = var_s3;
            }
        }
    } while ((result == NODE_NONE) && (var_t1 > 0));

    if (result != 0xFF) {
        aiNodeObj = (*gAINodes)[arg0];
        aiNodeEntry = &aiNodeObj->level_entry->aiNode;
        aiNode = aiNodeObj->ai_node;
        for (i = 0; (i < 4) && (result != aiNodeEntry->adjacent[i]); i++) {}
        if (i < 4) {
            aiNode->directions[arg3] = i;
        }
    }
    return result;
}

/**
 * Signal that AI nodes exist, so the game knows to initialise them.
 */
void ainode_enable(void) {
    gInitAINodes = TRUE;
}

/**
 * If the node ID is new, set the tail ID to it.
 */
void ainode_tail_set(s32 nodeID) {
    if (nodeID != gAINodeTail[0]) {
        gAINodeTail[1] = gAINodeTail[0];
        gAINodeTail[0] = nodeID;
    }
}

/**
 * Return the last created AI node.
 */
UNUSED Object *ainode_tail(s32 *nodeID) {
    *nodeID = gAINodeTail[1];
    return gAINodes[0][gAINodeTail[1]];
}

/**
 * Return the AI node assigned to the given ID.
 */
Object *ainode_get(s32 nodeID) {
    if (nodeID >= 0 && nodeID < AINODE_COUNT) {
        return gAINodes[0][nodeID];
    }
    return NULL;
}


/**
 * Applies shading properties to a global variable.
 * Presumably intended for level geometry, which supports shading, but never uses it.
 */
void set_world_shading(f32 ambient, f32 diffuse, s16 angleX, s16 angleY, s16 angleZ) {
    set_shading_properties(&gWorldShading, ambient, diffuse, angleX, angleY, angleZ);
}

/**
 * Add values onto the existing properties of an objects shading.
 * Resets the shading based off the new values.
 */
UNUSED void add_shading_properties(Object *obj, f32 ambientChange, f32 diffuseChange, s16 angleX, s16 angleY,
                                   s16 angleZ) {
    if (obj->shading != NULL) {
        obj->shading->ambient += ambientChange;
        if (obj->shading->ambient < 0.0f) {
            obj->shading->ambient = 0.0f;
        } else if (obj->shading->ambient > 1.0f) {
            obj->shading->ambient = 1.0f;
        }
        obj->shading->diffuse += diffuseChange;
        if (obj->shading->diffuse < 0.0f) {
            obj->shading->diffuse = 0.0f;
        }
        if (obj->shading->diffuse >= 2.0f) {
            obj->shading->diffuse = 1.99f;
        }
        set_shading_properties(obj->shading, obj->shading->ambient, obj->shading->diffuse,
                               (obj->shading->shadeAngleX + angleX), (obj->shading->shadeAngleY + angleY),
                               (obj->shading->shadeAngleZ + angleZ));
        if (obj->header->shadeIntensityy != 0) {
            obj->shading->lightR = obj->header->shadeLightR;
            obj->shading->lightG = obj->header->shadeLightG;
            obj->shading->lightB = obj->header->shadeLightB;
            obj->shading->lightIntensity = obj->header->shadeIntensityy;
            obj->shading->lightDirX = -(obj->shading->shadowDirX >> 1);
            obj->shading->lightDirY = -(obj->shading->shadowDirY >> 1);
            obj->shading->lightDirZ = -(obj->shading->shadowDirZ >> 1);
        }
    }
}

void set_shading_properties(ShadeProperties *arg0, f32 ambient, f32 diffuse, s16 angleX, s16 angleY, s16 angleZ) {
    Vec3s angle;
    Vec3f velocityPos;

    arg0->shadeAngleX = angleX;
    arg0->ambient = ambient;
    arg0->diffuse = diffuse;
    arg0->brightness = 1.0f;
    arg0->shadeAngleY = angleY;
    arg0->shadeAngleZ = angleZ;
    angle.z_rotation = angleX;
    angle.y_rotation = angleZ;
    angle.x_rotation = angleY;
    velocityPos.z = -16384.0f;
    velocityPos.x = 0.0f;
    velocityPos.y = 0.0f;
    vec3f_rotate(&angle, &velocityPos);
    arg0->shadowDirX = -velocityPos.x;
    arg0->shadowDirY = -velocityPos.y;
    arg0->shadowDirZ = -velocityPos.z;
    arg0->shadowR = 0;
    arg0->shadowG = 0;
    arg0->shadowB = 0;
}

/**
 * Take the normalised length of the position set by the perspective and set the world angle for the envmap.
 * Official name: setObjectViewNormal
 */
void update_envmap_position(f32 x, f32 y, f32 z) {
    f32 vecLength = sqrtf((x * x) + (y * y) + (z * z));
    f32 normalizedLength;
    if (vecLength != 0.0f) {
        normalizedLength = -8192.0f / vecLength;
        x *= normalizedLength;
        y *= normalizedLength;
        z *= normalizedLength;
    }
    gEnvmapPos[0].x = x;
    gEnvmapPos[0].y = y;
    gEnvmapPos[0].z = z;
}

/**
 * If the triangle batch allows for it, compute envmap normals for the mesh.
 * Some objects will prefer some extra additions on top before calculating, like light intensity.
 */
void obj_shade_fancy(ObjectModel *model, Object *object, s32 arg2, f32 intensity) {
    s16 environmentMappingEnabled;
    s32 dynamicLightingEnabled;
    s16 i;

    dynamicLightingEnabled = 0;
    environmentMappingEnabled = 0;

    for (i = 0; i < model->numberOfBatches; i++) {
        if (model->batches[i].miscData != BATCH_VTX_COL) {
            dynamicLightingEnabled = -1; // This is a bit weird, but I guess it works.
        }
        if (model->batches[i].flags & RENDER_ENVMAP) {
            environmentMappingEnabled = -1;
        }
    }

    if (dynamicLightingEnabled) {
        // Calculates dynamic lighting for the object
        if (object->header->directionalPointLighting) {
            // Dynamic directional lighting for some objects (Intro diddy, Taj, T.T., Bosses)
            calc_dynamic_lighting_for_object_1(object, model, arg2, object, intensity, 1.0f);
        } else {
            // Dynamic ambient lighting for other objects (Racers, Rare logo, Wizpig face, etc.)
            calc_dynamic_lighting_for_object_2(object, model, arg2, intensity);
        }
    }

    if (environmentMappingEnabled) {
        // Calculates environment mapping for the object
        calc_env_mapping_for_object(model, object->trans.rotation.z_rotation, object->trans.rotation.x_rotation,
                                    object->trans.rotation.y_rotation);
    }
}

void calc_dynamic_lighting_for_object_1(Object *object, ObjectModel *model, s16 arg2, Object *anotherObject,
                                        f32 intensity, f32 arg5) {
    s16 normIdx;
    s16 j;
    s16 i;
    Vec3s objRot;
    s32 s6;
    s32 lightDirX, lightDirY, lightDirZ;    // 16.16 fixed point, normalized
    s32 shadowDirX, shadowDirY, shadowDirZ; // 16.16 fixed point, normalized
    s32 diffuseFactor;
    s32 ambientFactor;
    s32 lightIntensity;
    s32 shadeStrength;
    Vec3f direction;
    Vertex *vertices;
    Vec3s *normals;

    if (object->shading == NULL) {
        return;
    }

    vertices = object->curVertData;
    normals = model->normals;
    normIdx = 0;

    direction.x = -(object->shading->lightDirX << 3);
    direction.y = -(object->shading->lightDirY << 3);
    direction.z = -(object->shading->lightDirZ << 3);
    objRot.y_rotation = -object->trans.rotation.y_rotation;
    objRot.x_rotation = -object->trans.rotation.x_rotation;
    objRot.z_rotation = -object->trans.rotation.z_rotation;
    vec3f_rotate_ypr(&objRot, &direction);

    if (object->header->shadeIntensityy != 0 && arg2) {
        mtxf_transform_dir(get_projection_matrix_f32(), &direction, &direction);
    }

    lightDirX = -direction.x;
    lightDirY = -direction.y;
    lightDirZ = -direction.z;
    s6 = object->shading->lightIntensity;

    direction.x = object->shading->shadowDirX << 2;
    direction.y = object->shading->shadowDirY << 2;
    direction.z = object->shading->shadowDirZ << 2;

    if (arg2) {
        mtxf_transform_dir(get_projection_matrix_f32(), &direction, &direction);
    }
    vec3f_rotate_ypr(&objRot, &direction);

    shadowDirX = direction.x;
    shadowDirY = direction.y;
    shadowDirZ = direction.z;

    ambientFactor = object->shading->ambient * object->shading->brightness * 255.0f * intensity;
    diffuseFactor = object->shading->diffuse * object->shading->brightness * 255.0f * intensity;

    for (i = 0; i < model->numberOfBatches; i++) {
        if (model->batches[i].miscData != BATCH_VTX_COL) { // 0xFF means use vertex colors
            for (j = model->batches[i].verticesOffset; j < model->batches[i + 1].verticesOffset; j++) {
                // calculate lighting
                lightIntensity = (normals[normIdx].x * lightDirX + normals[normIdx].y * lightDirY +
                                  normals[normIdx].z * lightDirZ) >>
                                 13;
                if (lightIntensity > 0) {
                    lightIntensity = (lightIntensity * s6) >> 16;
                    if (lightIntensity > 255) {
                        lightIntensity = 255;
                    }
                } else {
                    lightIntensity = 0;
                }

                // calculate shading
                shadeStrength = (normals[normIdx].x * shadowDirX + normals[normIdx].y * shadowDirY +
                                 normals[normIdx].z * shadowDirZ) >>
                                13;
                if (shadeStrength > 0) {
                    shadeStrength = (shadeStrength * diffuseFactor) >> 16;
                    shadeStrength += ambientFactor;
                    if (shadeStrength > 255) {
                        shadeStrength = 255;
                    }
                } else {
                    shadeStrength = ambientFactor;
                }

                vertices[j].r = lightIntensity;
                vertices[j].g = lightIntensity;
                vertices[j].b = lightIntensity;
                vertices[j].a = shadeStrength;
                normIdx++;
            }
        } else if (model->batches[i].flags & RENDER_ENVMAP) {
            normIdx += model->batches[i + 1].verticesOffset - model->batches[i].verticesOffset;
        }
    }
}

void calc_env_mapping_for_object(ObjectModel *model, s16 zRot, s16 xRot, s16 yRot) {
    MtxS objRotMtxS32;
    MtxF objRotMtxF32;
    ObjectTransform objTrans;
    s16 k;
    s16 count;
    Triangle *triangles;
    Vec3s *model40Entries;
    s32 sp70;
    TextureHeader *tex;
    s16 shiftS;
    s16 maskS;
    s16 shiftT;
    s16 maskT;
    s16 i;
    s16 j;
    s16 var_v0;
    s16 var_v1;

    count = 0;
    triangles = model->triangles;
    model40Entries = model->normals;
    objTrans.rotation.z_rotation = zRot;
    objTrans.rotation.x_rotation = xRot;
    objTrans.rotation.y_rotation = yRot;
    objTrans.x_position = 0.0f;
    objTrans.y_position = 0.0f;
    objTrans.z_position = 0.0f;
    objTrans.scale = 1.0f;
    mtxf_from_transform(&objRotMtxF32, &objTrans);
    mtxf_to_mtxs(&objRotMtxF32, &objRotMtxS32);
    for (i = 0; i < model->numberOfBatches; i++) {
        if (model->batches[i].flags & RENDER_ENVMAP) {
            sp70 = ((model->batches[i].flags & RENDER_UNK_0020000) | RENDER_ENVMAP) ^ RENDER_ENVMAP;
            tex = model->textures[model->batches[i].textureIndex].texture;
            k = 0;

            switch (tex->width) {
                case 0x80:
                    SET_SHIFT_AND_MASK(shiftS, maskS, 4);
                    break;
                case 0x40:
                    SET_SHIFT_AND_MASK(shiftS, maskS, 5);
                    break;
                case 0x20:
                    SET_SHIFT_AND_MASK(shiftS, maskS, 6);
                    break;
                default:
                    SET_SHIFT_AND_MASK(shiftS, maskS, 7);
                    break;
            }
            switch (tex->height) {
                case 0x80:
                    SET_SHIFT_AND_MASK(shiftT, maskT, 4);
                    break;
                case 0x40:
                    SET_SHIFT_AND_MASK(shiftT, maskT, 5);
                    break;
                case 0x20:
                    SET_SHIFT_AND_MASK(shiftT, maskT, 6);
                    break;
                default:
                    SET_SHIFT_AND_MASK(shiftT, maskT, 7);
                    break;
            }

            for (j = model->batches[i].verticesOffset; j < model->batches[i + 1].verticesOffset; j++, k++) {
                gEnvmapPos[1].x = model40Entries[count].x;
                gEnvmapPos[1].y = model40Entries[count].y;
                gEnvmapPos[1].z = model40Entries[count].z;
                mtxs_transform_dir(&objRotMtxS32, &gEnvmapPos[1]);
                if (sp70 == 0) {
                    vec3s_reflect(&gEnvmapPos[0], &gEnvmapPos[1]);
                }
                count++;
                var_v0 = gEnvmapPos[1].x;
                var_v1 = gEnvmapPos[1].y;
                if (var_v0 > 0) {
                    var_v0--;
                }
                if (var_v1 > 0) {
                    var_v1--;
                }
                var_v0 = (var_v0 << 2) + 0x8000;
                var_v1 = (var_v1 << 2) + 0x8000;
                gTexCoordScratch[k].u = (var_v0 >> shiftS) & maskS;
                gTexCoordScratch[k].v = (var_v1 >> shiftT) & maskT;
            }

            for (j = model->batches[i].facesOffset; j < model->batches[i + 1].facesOffset; j++) {
                triangles[j].uv0.u = gTexCoordScratch[triangles[j].vi0].u;
                triangles[j].uv0.v = gTexCoordScratch[triangles[j].vi0].v;
                triangles[j].uv1.u = gTexCoordScratch[triangles[j].vi1].u;
                triangles[j].uv1.v = gTexCoordScratch[triangles[j].vi1].v;
                triangles[j].uv2.u = gTexCoordScratch[triangles[j].vi2].u;
                triangles[j].uv2.v = gTexCoordScratch[triangles[j].vi2].v;
            }
        } else if (model->batches[i].miscData < BATCH_VTX_COL) {
            count += model->batches[i + 1].verticesOffset - model->batches[i].verticesOffset;
        }
    }
}

/**
 * Find the racer object representing the player and directly set position and angle to new values.
 */
UNUSED void set_racer_position_and_angle(s16 player, s16 *x, s16 *y, s16 *z, s16 *angleZ, s16 *angleX, s16 *angleY) {
    Object *obj;
    Object_Racer *racer;
    s32 i;

    for (i = 0; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE)) {
            if (obj->behaviorId == BHV_RACER) {
                racer = obj->racer;
                if (player == racer->playerIndex) {
                    *x = obj->trans.x_position;
                    *y = obj->trans.y_position;
                    *z = obj->trans.z_position;
                    *angleZ = obj->trans.rotation.z_rotation;
                    *angleX = obj->trans.rotation.x_rotation;
                    *angleY = obj->trans.rotation.y_rotation;
                    i = gObjectCount; // Feels like it should be a break instead.
                }
            }
        }
    }
}

/**
 * Returns a pointer to the asset in the misc. section. If index is out of range, then this
 * function just returns the pointer to gAssetsMiscSection.
 * Official name: objGetTable
 */
s32 *get_misc_asset(s32 index) {
    if (index < 0 || index >= gAssetsMiscTableLength) {
        return gAssetsMiscSection;
    }
    if (index == ASSET_MISC_20) {
        return (s32 *) sPcBoostTable; // host-layout copy, see obj_init
    }
    return (s32 *) &gAssetsMiscSection[gAssetsMiscTable[index]];
}

/**
 * Byte-swap one misc asset that is an array of f32, exactly once.
 *
 * The misc *table* is swapped in obj_init(), but the section it points into is
 * mixed-format, so each consumer owns the endianness of its own piece — and the
 * f32 ones never did. Read big-endian on a little-endian host, a racer stat of
 * 8.0 (0x41000000) becomes 0x00000041: a denormal, i.e. zero. Every weight,
 * handling value, acceleration curve and wheel offset was reading as zero.
 *
 * Size comes from the table (this entry to the next), so nothing is guessed.
 * Callers may re-fetch these pointers every frame, hence swap-once: a blind swap
 * would flip the bytes back and forth.
 */
void pc_swap_misc_f32_once(s32 index) {
    extern void pc_swap32_buf(void *buf, u32 numBytes);
    static u8 sSwapped[256];
    u8 *start;
    u8 *end;

    if (index <= 0 || index >= gAssetsMiscTableLength || index >= (s32) ARRAY_COUNT(sSwapped)) {
        return;
    }
    if (sSwapped[index]) {
        return;
    }
    sSwapped[index] = TRUE;

    // Address the raw section directly: get_misc_asset(ASSET_MISC_20) returns
    // the host-side boost table, which is not adjacent to anything here.
    start = (u8 *) &gAssetsMiscSection[gAssetsMiscTable[index]];
    end = (u8 *) &gAssetsMiscSection[gAssetsMiscTable[index + 1]];
    if (end <= start) {
        return;
    }
    pc_swap32_buf(start, (u32) (end - start) & ~3u);
}

/**
 * If the bridge is raised, decrement its timer and return the remaining time.
 */
s32 is_bridge_raised(s32 index) {
    if (index >= 0 && index < 8) {
        if (gDrawbridgeTimers[0][index] > 0) {
            gDrawbridgeTimers[0][index]--;
        }
        return gDrawbridgeTimers[0][index];
    }
    return 0;
}

/**
 * Starts the bridge timer when a racer hits the bell switch.
 */
void start_bridge_timer(s32 index) {
    if (index >= 0 && index < 8) {
        gDrawbridgeTimers[0][index] = 8;
    }
}

/**
 * When the sound timer hits the correct value, write the objects position to the arguments.
 */
void obj_bridge_pos(s32 timing, f32 *x, f32 *y, f32 *z) {
    s32 i;
    Object *current_obj;
    *x = -32000.0f;
    *y = -32000.0f;
    *z = -32000.0f;
    for (i = 0; i < gObjectCount; i++) {
        current_obj = gObjPtrList[i];

        if (current_obj != NULL && !(current_obj->trans.flags & OBJ_FLAGS_PARTICLE) &&
            current_obj->behaviorId == BHV_RAMP_SWITCH && current_obj->properties.common.unk0 == timing) {
            *x = current_obj->trans.x_position;
            *y = current_obj->trans.y_position;
            *z = current_obj->trans.z_position;
        }
    }
}

/**
 * Return the index of the currently active cutscene.
 */
s16 cutscene_id(void) {
    return gCutsceneID;
}

/**
 * Set the current cutscene index.
 */
void cutscene_id_set(s32 cutsceneID) {
    gCutsceneID = cutsceneID;
}

/** Switches to a cutscene by id, flagging the change for the animation system. */
void cutscene_set(s32 cutsceneID) {
    if (cutsceneID != gCutsceneID) {
        gCutsceneID = cutsceneID;
        gPathUpdateOff = FALSE;
        gCutsceneChanged = TRUE;
        if (get_game_mode() == GAMEMODE_MENU) {
            set_frame_blackout_timer();
        }
    }
}

/**
 * Returns the index of the standard object list.
 * Goes unused, since objGetObjList exists
 */
UNUSED s32 get_object_list_index(void) {
    return gObjectListStart;
}

/** Cutscene animations: links animation entries to the objects they drive. */
void anim_objects_prepare(void) {
    Object *obj;
    s32 lastObjCount;
    s32 curObjCount;
    s32 i;
    s32 j;
    LevelObjectEntry_Animation *entryAnimation;

    for (i = 0; i < gObjectCount; i++) {
        gObjPtrList[i]->trans.flags &= ~OBJ_FLAGS_UNK_2000;
    }
    for (i = 0; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (obj != NULL && !(obj->trans.flags & OBJ_FLAGS_PARTICLE) && obj->behaviorId == BHV_ANIMATION) {
            entryAnimation = &obj->level_entry->animation;
            if (entryAnimation->channel != gCutsceneID && entryAnimation->channel != 20) {
                obj->trans.flags |= OBJ_FLAGS_UNK_2000;
                if (obj->animTarget != NULL) {
                    obj->animTarget->trans.flags |= OBJ_FLAGS_UNK_2000;
                }
            }
        }
    }
    curObjCount = gObjectCount - 1;
    lastObjCount = curObjCount;
    for (i = 0; curObjCount >= i;) {
        for (j = 0; lastObjCount >= i && (j == 0);) {
            if (gObjPtrList[i]->trans.flags & OBJ_FLAGS_UNK_2000) {
                i++;
            } else {
                j = -1;
            }
        }
        for (j = 0; curObjCount >= 0 && j == 0;) {
            if (gObjPtrList[curObjCount]->trans.flags & OBJ_FLAGS_UNK_2000) {
                j = -1;
            } else {
                curObjCount--;
            }
        }
        if (i < curObjCount) {
            obj = gObjPtrList[i];
            gObjPtrList[i] = gObjPtrList[curObjCount];
            gObjPtrList[curObjCount] = obj;
            i++;
            curObjCount--;
        }
    }
    gObjectListStart = i;
    gFirstActiveObjectId = 0;
}

/** Applies the cutscene's override positions to the objects they target. */
void override_pos_apply(s8 arg0) {
    LevelObjectEntry_OverridePos *overridePosEntry;
    Object *overridePosObj;
    Object_OverridePos *overridePos;
    s32 i;
    s32 j;
    s32 someBool;
    Object *animTarget;

    for (i = 0; i < gOverridePosCount; i++) {
        overridePosObj = gOverridePosObjects[i];
        overridePosEntry = &overridePosObj->level_entry->overridePos;
        overridePos = overridePosObj->override_pos;
        if ((overridePosEntry->cutsceneId == gCutsceneID) || ((overridePosEntry->cutsceneId == 20))) {
            for (j = 0;
                 (j < gAnimObjectCount) && (overridePosEntry->behaviorId != gAnimObjects[j]->properties.animation.behaviourID);
                 j++) {}
            if (j != gAnimObjectCount && gAnimObjects[j]->animTarget != NULL) {
                someBool = (gAnimObjects[j]->animTarget->collisionData != NULL) ? FALSE : TRUE;
                if (arg0 != someBool) {
                    animTarget = gAnimObjects[j]->animTarget;
                    overridePos->x = animTarget->trans.x_position;
                    overridePos->y = animTarget->trans.y_position;
                    overridePos->z = animTarget->trans.z_position;
                    overridePos->anim = animTarget;
                    animTarget->trans.x_position = overridePosObj->trans.x_position;
                    animTarget->trans.y_position = overridePosObj->trans.y_position;
                    animTarget->trans.z_position = overridePosObj->trans.z_position;
                }
            } else {
                overridePos->anim = NULL;
            }
        }
    }
    gZoneJustLoaded = FALSE;
}

void override_pos_update(void) {
    s32 i;
    Object *obj;
    Object_OverridePos *obj64;

    // some flag, flips to 1 when loading a new zone
    if (gZoneJustLoaded != FALSE) {
        gZoneJustLoaded = FALSE;
        return;
    }

    // loading (boss) cutscene
    for (i = 0; i < gOverridePosCount; i++) {
        obj = gOverridePosObjects[i];
        obj64 = obj->override_pos;

        if (obj64->anim != NULL) {
            obj64->anim->trans.x_position = obj64->x;
            obj64->anim->trans.y_position = obj64->y;
            obj64->anim->trans.z_position = obj64->z;
        }
    }
}

/** Per-frame update of the cutscene animation objects and their actors. */
void anim_objects_update(void) {
    s32 pad[3];
    LevelObjectEntry_OverridePos *overridePos;
    Object *obj;
    s32 numOfObjs;
    s32 pad2;
    s32 i;
    s32 stopLooping;
    s32 sp28;
    s16 animActorIndex1;
    s16 animActorIndex2;
    s32 var_a0;
    Object *animObj1;
    LevelObjectEntry_Animation *animation1;
    LevelObjectEntry_Animation *animation2;

    if (gCutsceneChanged) {
        for (numOfObjs = 0; numOfObjs < gAnimObjectCount; numOfObjs++) {
            obj = gAnimObjects[numOfObjs];
            animation1 = &obj->level_entry->animation;
            if (obj->animTarget != NULL && animation1->channel != 20) {
                animObj1 = obj->animTarget;
                free_object(animObj1);
                obj->animTarget = NULL;
            }
        }
    }
    if (gAnimCameraCount > 20) {
        gAnimCameraCount = 0;
    }
    anim_objects_prepare();
    numOfObjs = 0;
    for (i = 0; i < gObjectCount; i++) {
        if (gObjPtrList[i] != NULL) {
            if (!(gObjPtrList[i]->trans.flags & OBJ_FLAGS_PARTICLE)) {
                if (gObjPtrList[i]->behaviorId == BHV_OVERRIDE_POS) {
                    overridePos = &gObjPtrList[i]->level_entry->overridePos;
                    if (overridePos->cutsceneId == gCutsceneID ||
                        overridePos->cutsceneId == (CUTSCENE_SHERBET_ISLAND_BOSS | CUTSCENE_ADVENTURE_TWO)) {
                        gOverridePosObjects[numOfObjs] = gObjPtrList[i];
                        numOfObjs++;
                    }
                }
            }
        }
    }
    gOverridePosCount = numOfObjs;
    gZoneJustLoaded = TRUE;

    gAnimObjectCount = 0;
    numOfObjs = 0;
    for (i = gObjectListStart; i < gObjectCount; i++) {
        if (gObjPtrList[i] != NULL) {
            if (!(gObjPtrList[i]->trans.flags & OBJ_FLAGS_PARTICLE)) {
                if (gObjPtrList[i]->behaviorId == BHV_ANIMATION) {
                    gAnimObjects[numOfObjs] = gObjPtrList[i];
                    numOfObjs++;
                }
            }
        }
    }

    do {
        stopLooping = TRUE;
        for (i = 0; i < numOfObjs - 1; i++) {
            animation1 = &gAnimObjects[i]->level_entry->animation;
            animation2 = &gAnimObjects[i + 1]->level_entry->animation;
            animActorIndex1 = animation1->actorIndex;
            animActorIndex2 = animation2->actorIndex;

            if (animation1->channel == 20) {
                animActorIndex1 -= 400;
            }
            if (animation2->channel == 20) {
                animActorIndex2 -= 400;
            }

            if (animActorIndex2 < animActorIndex1) {
                animObj1 = gAnimObjects[i];
                gAnimObjects[i] = gAnimObjects[i + 1];
                gAnimObjects[i + 1] = animObj1;
                stopLooping = FALSE;
            } else if (animActorIndex1 == animActorIndex2) {
                if (animation2->order < animation1->order) {
                    animObj1 = gAnimObjects[i];
                    gAnimObjects[i] = gAnimObjects[i + 1];
                    gAnimObjects[i + 1] = animObj1;
                    stopLooping = FALSE;
                } else if (animation1->order == animation2->order &&
                           (gAnimObjects[i + 1]->properties.animation.action == 1 ||
                            gAnimObjects[i]->properties.animation.action == 2)) {
                    animObj1 = gAnimObjects[i];
                    gAnimObjects[i] = gAnimObjects[i + 1];
                    gAnimObjects[i + 1] = animObj1;
                    stopLooping = FALSE;
                }
            }
        }
    } while (stopLooping == FALSE);

    var_a0 = -101;
    for (i = 0; i < numOfObjs; i += 1) {
        animation1 = &gAnimObjects[i]->level_entry->animation;
        if (animation1->actorIndex != var_a0) {
            var_a0 = animation1->actorIndex;
            sp28 = 0;
        }
        animation1->order = sp28++; // It is possible that sp28 could not be initalized?
        gAnimObjects[i]->properties.animation.action = 0;
    }

    gAnimObjectCount = numOfObjs;
    if (gCutsceneChanged) {
        anim_objects_spawn_targets();
    }
    gCutsceneChanged = FALSE;
}

void anim_objects_spawn_targets(void) {
    LevelObjectEntry_Animation *animation;
    Object *obj;
    s32 i;

    for (i = 0; i < gAnimObjectCount; i++) {
        obj = gAnimObjects[i];
        animation = &obj->level_entry->animation;
        if (obj->animTarget == NULL && animation->order == 0 && animation->objectIdToSpawn != -1) {
            anim_object_spawn_target(obj, animation);
        }
        if (D_8011AD26 || animation->channel != 20) {
            if (obj->animTarget != NULL) {
                obj_init_animobject(obj, obj->animTarget);
            }
        }
    }
    D_8011AD26 = FALSE;
}

void obj_init_animobject(Object *animationObj, Object *animatedObj) {
    LevelObjectEntry_Animation *animEntry;
    Object_AnimatedObject *anim;
    f32 scale;

    animEntry = &animationObj->level_entry->animation;
    anim = animatedObj->animatedObject;
    scale = animEntry->scale & 0xFF;
    if (scale < 1.0f) {
        scale = 1.0f;
    }
    scale /= 64;
    animatedObj->trans.scale = animatedObj->header->scale * scale;
    animatedObj->properties.animatedObj.attachRacerID = 0;
    animatedObj->properties.animatedObj.pathIdle = 0;
    if (animEntry->attachMode >= 2 && animEntry->attachMode < 10) {
        animatedObj->properties.animatedObj.attachRacerID = animEntry->attachMode - 1;
    }
    if (animEntry->attachMode >= 10 && animEntry->attachMode < 18) {
        animatedObj->properties.animatedObj.attachRacerID = animEntry->attachMode - 9;
    }
    animatedObj->trans.x_position = animationObj->trans.x_position;
    animatedObj->trans.y_position = animationObj->trans.y_position;
    animatedObj->trans.z_position = animationObj->trans.z_position;
    animatedObj->trans.rotation.y_rotation = animationObj->trans.rotation.y_rotation;
    animatedObj->trans.rotation.z_rotation = animationObj->trans.rotation.z_rotation;
    animatedObj->trans.rotation.x_rotation = animationObj->trans.rotation.x_rotation;
    anim->nodeIndex = 0;
    anim->channel = animEntry->channel;
    anim->actorIndex = animEntry->actorIndex;
    anim->speed = (f32) animEntry->nodeSpeed * 0.1;
    anim->startDelay = normalise_time(animEntry->animationStartDelay);
    animatedObj->animationID = animEntry->objAnimIndex;
    animatedObj->animFrame = animEntry->animStartFrame;
    anim->z = animEntry->objAnimSpeed;
    anim->y = 0;
    anim->loopType = animEntry->objAnimLoopType;
    anim->rotateType = animEntry->rotateType;
    anim->targetActorIndex = animEntry->nextAnim;
    anim->linearInterp = animEntry->linearInterp;
    anim->yawSpinSpeed = animEntry->yawSpinSpeed;
    anim->rollSpinSpeed = animEntry->rollSpinSpeed;
    anim->pitchSpinSpeed = animEntry->pitchSpinSpeed;
    anim->skipButtons = animEntry->skipButtons;
    anim->modelReversing = 0;
    anim->segmentStep = 0;
    anim->segmentProgress = 0;
    animationObj->particleEmitter = NULL;
    anim->pauseCounter = normalise_time(animEntry->pauseFrameCount);
    anim->specialHide = animEntry->specialHide;
    if (animEntry->emitterMask >= 0) {
        anim->emitterMask = animEntry->emitterMask;
    }
    anim->musicSequence = animEntry->musicSequence;
    anim->soundID = animEntry->soundID;
    anim->fadeType = animEntry->fadeType;
    anim->fadeColourIndex = animEntry->soundEffect;
    anim->fadeOptions = animEntry->fadeOptions;
    anim->fadeLength = animEntry->fadeAlpha;
    anim->opacity = 0xFF;
    if (anim->soundHandle != NULL) {
        sndp_stop(anim->soundHandle);
    }
    anim->soundHandle = NULL;
    anim->musicFadeVolume = animEntry->musicFadeVolume;
    anim->animator = animationObj;
    anim->started = 0;
}

/** Spawns the object an animation entry animates and binds it. */
void anim_object_spawn_target(Object *obj, LevelObjectEntry_Animation *animEntry) {
    s32 i;
    LevelObjectEntryCommon newObjEntry;
    Object *newObj;
    Object_AnimatedObject *camera;
    s32 viewportCount;

    NEW_OBJECT_ENTRY(newObjEntry, animEntry->objectIdToSpawn, 8, animEntry->common.x, animEntry->common.y,
                     animEntry->common.z);

    obj->animTarget = spawn_object(&newObjEntry, OBJECT_SPAWN_UNK01);
    newObj = obj->animTarget;
    // (newObj->behaviorId == BHV_DINO_WHALE) is Dinosaur1, Dinosaur2, Dinosaur3, Whale, and Dinoisle
    if (obj->animTarget != NULL && newObj->behaviorId == BHV_DINO_WHALE && gTimeTrialEnabled) {
        free_object(newObj);
        obj->animTarget = NULL;
        newObj = NULL;
    }
    if (newObj != NULL) {
        newObj->level_entry = NULL;
        obj_init_animobject(obj, newObj);
        if (newObj->header->behaviorId == BHV_CAMERA_ANIMATION) {
            camera = newObj->animatedObject;
            camera->cameraStage = gAnimCameraCount;
            viewportCount = cam_get_viewport_layout();
            if (race_is_adventure_2P()) {
                viewportCount = VIEWPORT_LAYOUT_2_PLAYERS;
            }
            for (i = 0; i < viewportCount;) {
                newObj = spawn_object(&newObjEntry, OBJECT_SPAWN_UNK01);
                if (newObj != NULL) {
                    newObj->level_entry = NULL;
                    obj_init_animobject(obj, newObj);
                    camera = newObj->animatedObject;
                    i++;
                    camera->cameraID = i;
                    camera->cameraStage = gAnimCameraCount;
                }
            }
            gAnimCameraCount++;
        }
    }
}

s8 anim_group_get(void) {
    return gAnimGroup;
}

void anim_group_set(s32 arg0) {
    if (arg0 != gAnimGroup) {
        gAnimObjectCount = 0;
    }
    gAnimGroup = arg0;
}

s32 anim_count_actors(s32 arg0) {
    s32 i;
    s32 count;
    if (gAnimObjectCount == 0) {
        return -1;
    }

    count = 0;
    for (i = 0; i < gAnimObjectCount; i++) {
        if (gAnimObjects[i]->properties.animation.behaviourID == arg0) {
            count++;
        }
    }

    return count;
}

/** Asks the running cutscene to end (from the pause menu). */
void cutscene_skip_request(void) {
    gCutsceneSkipRequested = 1;
}

typedef struct AnimPathPoints {
    f32 x[5];
    f32 y[5];
    f32 z[5];
    f32 scale[5];
    f32 yaw[5];
    f32 pitch[5];
    f32 roll[5];
} AnimPathPoints;

/**
 * Find the run of animation nodes that make up actor `actorIndex`'s path. Returns FALSE
 * when there is no path of at least two nodes.
 */
static s32 anim_path_find(s32 actorIndex, s32 *first, s32 *count) {
    s32 i;
    s32 n;

    for (i = 0; i < gAnimObjectCount && actorIndex != gAnimObjects[i]->properties.animation.behaviourID; i++) {}
    if (i >= gAnimObjectCount) {
        return FALSE;
    }
    for (n = 1; (i + n) < gAnimObjectCount && actorIndex == gAnimObjects[i + n]->properties.animation.behaviourID;
         n++) {}
    *first = i;
    *count = n;
    return n >= 2;
}

/**
 * The node a path loops back to after its last node, or -1 for a path that ends.
 */
static s32 anim_path_loop_node(s32 first, s32 count) {
    LevelObjectEntry_Animation *entry;

    entry = &gAnimObjects[first + count - 1]->level_entry->animation;
    if (count > 2 && entry->goToNode >= 0 && entry->goToNode < count - 1) {
        return entry->goToNode;
    }
    return -1;
}

/**
 * The five control points around the actor's current node: the node before, the node, and
 * the three after, extrapolated past either end of a path that does not loop. A node
 * marked with unk22 takes its position from the actor's camera instead.
 */
static void anim_path_control_points(s32 first, s32 count, s32 loopNode, s32 nodeIndex, s8 cameraID,
                                     AnimPathPoints *pts) {
    LevelObjectEntry_Animation *entry;
    ObjectTransform *trans;
    s32 node;
    s32 q;
    s32 j;

    node = nodeIndex - 1;
    for (j = 0; j < 5; j++, node++) {
        if (node == -1) {
            if (loopNode != 0) {
                q = first;
                pts->x[j] = gAnimObjects[q]->trans.x_position +
                            (gAnimObjects[q]->trans.x_position - gAnimObjects[q + 1]->trans.x_position);
                pts->y[j] = gAnimObjects[q]->trans.y_position +
                            (gAnimObjects[q]->trans.y_position - gAnimObjects[q + 1]->trans.y_position);
                pts->z[j] = gAnimObjects[q]->trans.z_position +
                            (gAnimObjects[q]->trans.z_position - gAnimObjects[q + 1]->trans.z_position);
            } else {
                q = first + count - 1;
                pts->x[j] = gAnimObjects[q]->trans.x_position;
                pts->y[j] = gAnimObjects[q]->trans.y_position;
                pts->z[j] = gAnimObjects[q]->trans.z_position;
            }
            pts->yaw[j] = gAnimObjects[q]->trans.rotation.y_rotation;
            pts->pitch[j] = gAnimObjects[q]->trans.rotation.x_rotation;
            pts->roll[j] = gAnimObjects[q]->trans.rotation.z_rotation;
            pts->scale[j] = gAnimObjects[q]->trans.scale;
        } else if (node >= count) {
            if (loopNode == -1) {
                node = count - 1;
                q = first + node;
                entry = &gAnimObjects[q]->level_entry->animation;
                if (entry->attachMode == 1) {
                    set_active_camera(cameraID);
                    trans = &cam_get_active_camera_no_cutscenes()->trans;
                } else {
                    trans = &gAnimObjects[q]->trans;
                }
                pts->x[j] = trans->x_position + (trans->x_position - gAnimObjects[q - 1]->trans.x_position);
                pts->y[j] = trans->y_position + (trans->y_position - gAnimObjects[q - 1]->trans.y_position);
                pts->z[j] = trans->z_position + (trans->z_position - gAnimObjects[q - 1]->trans.z_position);
                pts->pitch[j] = trans->rotation.x_rotation;
                pts->roll[j] = trans->rotation.z_rotation;
                pts->yaw[j] = gAnimObjects[q]->trans.rotation.y_rotation;
                pts->scale[j] = gAnimObjects[q]->trans.scale;
            } else {
                q = first + loopNode + node - count;
                pts->x[j] = gAnimObjects[q]->trans.x_position;
                pts->y[j] = gAnimObjects[q]->trans.y_position;
                pts->z[j] = gAnimObjects[q]->trans.z_position;
                pts->yaw[j] = gAnimObjects[q]->trans.rotation.y_rotation;
                pts->pitch[j] = gAnimObjects[q]->trans.rotation.x_rotation;
                pts->roll[j] = gAnimObjects[q]->trans.rotation.z_rotation;
                pts->scale[j] = gAnimObjects[q]->trans.scale;
            }
        } else {
            q = node + first;
            entry = &gAnimObjects[q]->level_entry->animation;
            if (entry->attachMode == 1) {
                set_active_camera(cameraID);
                trans = &cam_get_active_camera_no_cutscenes()->trans;
            } else {
                trans = &gAnimObjects[q]->trans;
            }
            pts->x[j] = trans->x_position;
            pts->y[j] = trans->y_position;
            pts->z[j] = trans->z_position;
            pts->pitch[j] = trans->rotation.x_rotation;
            pts->roll[j] = trans->rotation.z_rotation;
            pts->yaw[j] = gAnimObjects[q]->trans.rotation.y_rotation;
            if (entry->attachMode == 1) {
                pts->pitch[j] = -pts->pitch[j];
            }
            pts->scale[j] = gAnimObjects[q]->trans.scale;
        }
    }
}

/**
 * Make consecutive angles continuous so the spline does not sweep the long way round,
 * even though that pushes them past the s16 range.
 */
static void anim_path_unwrap_angles(f32 *angles) {
    f32 shift;
    f32 delta;
    s32 j;
    s32 k;

    for (j = 1; j < 5; j++) {
        shift = 0;
        delta = angles[j] - angles[j - 1];
        if (delta > 32768.0) {
            shift -= 65536.0;
        } else if (delta < -32768.0) {
            shift += 65536.0;
        }
        for (k = j; k < 5; k++) {
            angles[k] += shift;
        }
    }
}

/**
 * Rotation interpolated from the node rotations (rotateType other than 1, 2 or 3).
 * The unwrap above leaves angles outside the s16 range on purpose, so the store relies
 * on wrapping mod 65536: f32->s16 out of range is UB (x87 fistps would write 0x8000),
 * so go through s32, which wraps everywhere.
 */
static void anim_path_apply_node_rotation(Object *obj, Object_AnimatedObject *anim, AnimPathPoints *pts, s32 segment,
                                          f32 t) {
    anim_path_unwrap_angles(pts->yaw);
    anim_path_unwrap_angles(pts->pitch);
    anim_path_unwrap_angles(pts->roll);
    if (anim->linearInterp == 0) {
        obj->trans.rotation.y_rotation = (s16) (s32) catmull_rom_interpolation(pts->yaw, segment, t);
        obj->trans.rotation.x_rotation = (s16) (s32) catmull_rom_interpolation(pts->pitch, segment, t);
        obj->trans.rotation.z_rotation = (s16) (s32) catmull_rom_interpolation(pts->roll, segment, t);
    } else {
        obj->trans.rotation.y_rotation = (s16) (s32) lerp(pts->yaw, segment, t);
        obj->trans.rotation.x_rotation = (s16) (s32) lerp(pts->pitch, segment, t);
        obj->trans.rotation.z_rotation = (s16) (s32) lerp(pts->roll, segment, t);
    }
}

/**
 * Face along the path's tangent (rotateType 2).
 */
static void anim_path_face_along(Object *obj, Object_AnimatedObject *anim, AnimPathPoints *pts, s32 segment, f32 t,
                                 f32 *tangentZ) {
    f32 dx;
    f32 dy;
    f32 dz;
    f32 length;

    if (anim->linearInterp == 0) {
        cubic_spline_interpolation(pts->x, segment, t, &dx);
        cubic_spline_interpolation(pts->y, segment, t, &dy);
        cubic_spline_interpolation(pts->z, segment, t, &dz);
    } else {
        lerp_and_get_derivative(pts->x, segment, t, &dx);
        lerp_and_get_derivative(pts->y, segment, t, &dy);
        lerp_and_get_derivative(pts->z, segment, t, &dz);
    }
    length = sqrtf((dx * dx) + (dy * dy) + (dz * dz));
    if (length != 0) {
        length = 100.0 / length;
        dx *= length;
        dy *= length;
        dz *= length;
    }
    obj->trans.rotation.y_rotation = arctan2_f(dx, dz) - 0x8000;
    obj->trans.rotation.x_rotation = arctan2_f(dy, 100.0f);
    *tangentZ = dz;
}

/**
 * Face the actor that another animation path is driving (rotateType 3).
 */
static void anim_path_face_actor(Object *obj, s8 actorIndex) {
    ObjectTransform *trans;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 length;
    s32 j;

    for (j = 0; j < gAnimObjectCount && actorIndex != gAnimObjects[j]->properties.animation.behaviourID; j++) {}
    if (j == gAnimObjectCount) {
        return;
    }
    trans = &gAnimObjects[j]->animTarget->trans;
    if (trans == NULL) {
        return;
    }
    dx = trans->x_position - obj->trans.x_position;
    dy = trans->y_position - obj->trans.y_position;
    dz = trans->z_position - obj->trans.z_position;
    length = sqrtf((dx * dx) + (dy * dy) + (dz * dz));
    if (length > 0) {
        obj->trans.rotation.y_rotation = arctan2_f(dx, dz) - 0x8000;
        obj->trans.rotation.x_rotation = arctan2_f(dy, length);
    }
}

/**
 * An actor waiting with a negative start delay starts when one of its skip buttons is
 * pressed on any controller.
 */
static void anim_actor_check_skip(Object_AnimatedObject *anim) {
    s32 buttons;
    s32 pressed;
    s32 i;

    buttons = 0;
    if (anim->skipButtons & 1) {
        buttons = A_BUTTON;
    }
    if (anim->skipButtons & 2) {
        buttons |= B_BUTTON;
    }
    if (anim->skipButtons & 4) {
        buttons |= CONT_START;
    }
    pressed = 0;
    for (i = 0; i < MAXCONTROLLERS; i++) {
        pressed |= input_pressed(i);
    }
    if (pressed & buttons) {
        anim->startDelay = 1;
    }
}

/**
 * Music and sound cues attached to the current node: a sequence to start, a one-shot
 * sound (0xFF stops the current one) and a music fade.
 */
static void anim_actor_play_cues(Object_AnimatedObject *anim) {
    s32 soundID;

    if (anim->musicSequence > 0) {
        if (anim->musicSequence != music_current_sequence()) {
            music_play(anim->musicSequence);
            music_change_off();
        }
        anim->musicSequence = -2;
        music_volume_reset();
    } else if (anim->musicSequence == -2) {
        music_change_on();
        anim->musicSequence = -1;
    }
    if (anim->soundID != 0) {
        soundID = anim->soundID & 0xFF;
        if (anim->currentSound == 0) {
            if (soundID == 0xFF) {
                if (anim->soundHandle != NULL) {
                    sndp_stop(anim->soundHandle);
                }
            } else {
                if (anim->soundHandle != NULL) {
                    sndp_stop(anim->soundHandle);
                    soundID = anim->soundID & 0xFF;
                }
                sound_play(soundID, &anim->soundHandle);
            }
            anim->soundID = 0;
        }
    }
    if (anim->musicFadeVolume != 0) {
        music_fade(anim->musicFadeVolume << 8);
        anim->musicFadeVolume = 0;
    }
}

/**
 * Fade the actor in or out over time and hide it once fully out.
 */
static void anim_actor_update_opacity(Object *obj, Object_AnimatedObject *anim, s32 updateRate) {
    s32 opacity;

    opacity = (u8) anim->opacity;
    if (anim->fadeOptions & 1) {
        if (anim->fadeOptions & 2) {
            opacity = 0;
        }
        if ((updateRate * 8) < opacity) {
            opacity -= (updateRate * 8);
        } else {
            opacity = 0;
            obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
        }
    } else {
        if (anim->fadeOptions & 2) {
            opacity = 0xFF;
        }
        opacity += updateRate * 8;
        if (opacity > 0xFF) {
            opacity = 0xFF;
        }
        obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
    }
    anim->opacity = opacity;
}

/**
 * A screen transition cued by the current node: 8 and above change the fog, 6 and 7
 * flash white or blue, the rest are colour fades from the misc asset.
 */
static void anim_actor_update_fade(Object_AnimatedObject *anim) {
    FadeTransition fadeTransition;
    s8 *miscAsset;
    s32 fadeType;
    s32 fogNear;
    s32 fogFar;

    fadeType = anim->fadeType & 0x7F;
    if (fadeType == 0x7F) {
        return;
    }
    if (fadeType >= 8) {
        miscAsset = ((s8 *) get_misc_asset(ASSET_MISC_13) + (fadeType * 5));
        miscAsset -= 0x28;
        fogNear = (miscAsset[0] & 0xFF) + 900;
        fogFar = (miscAsset[1] & 0xFF) + 900;
        slowly_change_fog(0, miscAsset[2] & 0xFF, miscAsset[3] & 0xFF, miscAsset[4] & 0xFF, fogNear, fogFar,
                          normalise_time(6) * anim->fadeLength);
    } else if (fadeType >= 6) {
        fadeTransition.type = FADE_FLAG_INVERT;
        if (fadeType == 7) {
            fadeTransition.red = 200;
            fadeTransition.green = 200;
            fadeTransition.blue = 255;
        } else {
            fadeTransition.red = 255;
            fadeTransition.green = 255;
            fadeTransition.blue = 255;
        }
        fadeTransition.duration = 7;
        fadeTransition.endTimer = 3;
        transition_begin(&fadeTransition);
    } else {
        fadeTransition.type = anim->fadeType;
        miscAsset = (s8 *) get_misc_asset(ASSET_MISC_14) + (anim->fadeColourIndex * 3);
        fadeTransition.red = miscAsset[0];
        fadeTransition.green = miscAsset[1];
        fadeTransition.blue = miscAsset[2];
        if (anim->fadeType & 0x80) {
            fadeTransition.endTimer = 0;
        } else {
            fadeTransition.endTimer = 0xFFFF;
        }
        fadeTransition.duration = normalise_time(6) * anim->fadeLength;
        if (check_fadeout_transition() == 0 || (fadeTransition.type & 0x80)) {
            transition_begin(&fadeTransition);
        }
    }
    anim->fadeType = 0xFF;
}

/**
 * Advance the target model's animation frame (16ths) by the actor's rate, following its
 * loop type: 0 loop, 1 ping-pong, 2 play once, 3 play once then back.
 */
static void anim_actor_advance_model_frame(Object *obj, Object_AnimatedObject *anim, Object *target, f32 rate) {
    ObjectModel *model;
    s32 lastFrame;

    target->animationID = obj->animationID;
    if ((s16) anim->y != target->animFrame) {
        anim->y = target->animFrame;
    }
    if (target->modelInstances[target->modelIndex] != NULL) {
        model = target->modelInstances[target->modelIndex]->objModel;
        if (target->animationID >= 0 && target->animationID < model->numberOfAnimations) {
            lastFrame = (model->animations[target->animationID].animLength - 1) << 4;
            switch (anim->loopType) {
                case 0:
                    anim->y += anim->z * rate;
                    if (anim->y >= lastFrame) {
                        anim->y -= lastFrame;
                    }
                    break;
                case 2:
                    anim->y += anim->z * rate;
                    if (anim->y >= lastFrame) {
                        anim->y = lastFrame - 1.0f;
                    }
                    break;
                case 1:
                    if (anim->modelReversing == 0) {
                        anim->y += anim->z * rate;
                        if (anim->y >= lastFrame) {
                            anim->modelReversing = 1;
                            anim->y = lastFrame - 1.0f;
                        }
                    } else {
                        anim->y -= anim->z * rate;
                        if (anim->y <= 0) {
                            anim->y = 0;
                            anim->modelReversing = 0;
                        }
                    }
                    break;
                case 3:
                    if (anim->modelReversing == 0) {
                        anim->y += anim->z * rate;
                        if (anim->y >= lastFrame) {
                            anim->modelReversing = 1;
                            anim->y = lastFrame - 1.0f;
                        }
                    } else {
                        anim->y -= anim->z * rate;
                        if (anim->y <= 0) {
                            anim->y = 0;
                        }
                    }
                    break;
            }
        }
    }
    target->animFrame = anim->y;
}

/**
 * Drive a cutscene actor along its path of animation nodes for one frame: handle its
 * start delay and cues, step its model animation, then move it along the spline at the
 * node speed and rotate it by its rotate type. Returns 1 when the actor has nothing to do.
 */
s32 anim_object_follow_path(Object *obj, s32 updateRate, Object *target) {
    Object_AnimatedObject *anim;
    LevelObjectEntry_Animation *entry;
    AnimPathPoints pts;
    f32 rate;
    f32 t;
    f32 moveX;
    f32 moveY;
    f32 moveZ;
    f32 moved;
    f32 invBaseScale;
    f32 nodeSpeed;
    f32 nextNodeSpeed;
    s32 first;
    s32 count;
    s32 loopNode;
    s32 segment;
    s32 pass;
    s32 node;
    s32 nextNode;

    if (gCutsceneID < 0) {
        return 1;
    }
    if (updateRate >= 9) {
        updateRate = 8;
    }
    rate = updateRate;
    anim = obj->animatedObject;
    if (osTvType == 0) {
        rate *= 1.2;
    }
    if (anim->startDelay < 0) {
        anim_actor_check_skip(anim);
    }
    if ((anim->startDelay >= 0) && (anim->started == 0)) {
        anim->startDelay -= updateRate;
        if (anim->startDelay <= 0) {
            anim->started = 1;
            entry = &anim->animator->level_entry->animation;
            anim_actor_init(obj, anim, entry);
            anim->startDelay = 0;
            anim_actor_apply_entry(obj, entry, anim, -1);
        }
    }
    if (anim->startDelay != 0) {
        if (anim->specialHide != 0) {
            obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
            anim->opacity = 0;
            return 1;
        }
        return 0;
    }

    obj->trans.flags &= ~OBJ_FLAGS_INVISIBLE;
    anim_actor_play_cues(anim);
    anim_actor_update_opacity(obj, anim, updateRate);
    anim_actor_update_fade(anim);

    if (anim->rotateType == 1) {
        obj->trans.rotation.y_rotation += (s16) (anim->yawSpinSpeed * (f32) (rate * 8.0));
        obj->trans.rotation.x_rotation += (s16) (anim->rollSpinSpeed * (f32) (rate * 8.0));
        obj->trans.rotation.z_rotation += (s16) (anim->pitchSpinSpeed * (f32) (rate * 8.0));
    }
    if (target != NULL && target->header->modelType == 0) {
        anim_actor_advance_model_frame(obj, anim, target, rate);
    }

    if (anim->speed <= 0.0) {
        return anim_actor_update(obj, updateRate);
    }
    if (!anim_path_find(anim->actorIndex, &first, &count)) {
        return anim_actor_update(obj, updateRate);
    }
    loopNode = anim_path_loop_node(first, count);
    invBaseScale = 1.0 / gAnimObjects[0]->header->scale;
    if (loopNode == -1 && anim->nodeIndex >= count - 1) {
        return anim_actor_update(obj, updateRate);
    }
    if (anim->pauseCounter >= 0) {
        if (gCutsceneSkipRequested == 0) {
            anim->pauseCounter -= updateRate;
        }
        return 0;
    }

    anim_path_control_points(first, count, loopNode, anim->nodeIndex, anim->cameraID, &pts);

    // Two passes: the first measures how far the current step would move the actor and
    // rescales the step to hold the node speed; the second takes the corrected step.
    if (anim->segmentStep == 0) {
        anim->segmentStep = 0.01f;
    }
    for (segment = 0, pass = 0; pass < 2; pass++) {
        t = anim->segmentProgress + (anim->segmentStep * rate);
        if (t >= 1.00) {
            segment = 1;
            t -= 1.0;
        }
        if (anim->linearInterp == 0) {
            moveX = catmull_rom_interpolation(pts.x, segment, t);
            moveY = catmull_rom_interpolation(pts.y, segment, t);
            moveZ = catmull_rom_interpolation(pts.z, segment, t);
        } else {
            moveX = lerp(pts.x, segment, t);
            moveY = lerp(pts.y, segment, t);
            moveZ = lerp(pts.z, segment, t);
        }
        moveX -= obj->trans.x_position;
        moveY -= obj->trans.y_position;
        moveZ -= obj->trans.z_position;
        if (pass != 1) {
            segment = 0;
            moved = sqrtf((moveX * moveX) + (moveY * moveY) + (moveZ * moveZ)) / rate;
            if (moved != 0) {
                anim->segmentStep *= anim->speed / moved;
            }
        }
    }

    obj->trans.scale = catmull_rom_interpolation(pts.scale, segment, t) * invBaseScale * obj->header->scale;
    // Reaching the last node of a path that ends: land exactly on it.
    if (segment != 0 && loopNode == -1 && count == anim->nodeIndex + 2) {
        moveX = catmull_rom_interpolation(pts.x, 0, 1.0f);
        moveY = catmull_rom_interpolation(pts.y, 0, 1.0f);
        moveZ = catmull_rom_interpolation(pts.z, 0, 1.0f);
        moveX -= obj->trans.x_position;
        moveY -= obj->trans.y_position;
        moveZ -= obj->trans.z_position;
    }
    obj->x_velocity = moveX / rate;
    obj->y_velocity = moveY / rate;
    obj->z_velocity = moveZ / rate;
    move_object(obj, moveX, moveY, moveZ);

    switch (anim->rotateType) {
        case 1:
            break;
        case 2:
            anim_path_face_along(obj, anim, &pts, segment, t, &moveZ);
            break;
        case 3:
            break;
        default:
            anim_path_apply_node_rotation(obj, anim, &pts, segment, t);
            break;
    }

    // Node speed: the current node's, blended towards the next node's across the segment.
    node = anim->nodeIndex;
    anim->segmentProgress = t;
    if (loopNode != -1 && node >= count) {
        node = (node - count) + loopNode;
    }
    entry = &gAnimObjects[first + node]->level_entry->animation;
    nodeSpeed = (f32) entry->nodeSpeed * 0.1;
    if (nodeSpeed < 0) {
        nodeSpeed = anim->x;
    } else {
        anim->x = nodeSpeed;
    }
    if (nodeSpeed >= 0) {
        if (segment == 0) {
            nextNode = node + 1;
            if (loopNode != -1 && nextNode >= count) {
                nextNode = nextNode - count + loopNode;
            }
            if (nextNode < count) {
                entry = &gAnimObjects[first + nextNode]->level_entry->animation;
                if (entry->nodeSpeed >= 0) {
                    nextNodeSpeed = (f32) entry->nodeSpeed * 0.1;
                } else {
                    nextNodeSpeed = nodeSpeed;
                }
            } else {
                nextNodeSpeed = moveZ; //!@bug uninitialised in the original: it reads the stale z move
            }
            anim->speed = ((nextNodeSpeed - nodeSpeed) * t) + nodeSpeed;
        }
    }

    if (segment != 0) {
        anim->nodeIndex += 1;
        if (loopNode == -1) {
            if (anim->nodeIndex >= count) {
                anim->nodeIndex = count - 1;
            } else {
                entry = &gAnimObjects[first + anim->nodeIndex]->level_entry->animation;
                anim_actor_apply_entry(obj, entry, anim, first);
            }
        } else {
            if (count < anim->nodeIndex) {
                anim->nodeIndex = loopNode + 1;
            }
            node = anim->nodeIndex;
            if (anim->nodeIndex >= count) {
                node = (anim->nodeIndex - count) + loopNode;
            }
            entry = &gAnimObjects[first + node]->level_entry->animation;
            anim_actor_apply_entry(obj, entry, anim, first);
        }
    }
    if (anim->rotateType == 3) {
        anim_path_face_actor(obj, anim->targetActorIndex);
    }
    obj->particleEmittersEnabled = anim->emitterMask;
    obj_spawn_particle(obj, updateRate);
    return 0;
}

s32 anim_stage_reached(s8 arg0) {
    if (arg0 >= gAnimStage) {
        gAnimStage = arg0;
        return TRUE;
    }
    return FALSE;
}

void anim_actor_init(Object *obj, Object_AnimatedObject *animObj, LevelObjectEntry_Animation *entry) {
    Camera *camera;
    ObjectTransform *animObjTrans;

    animObjTrans = &animObj->animator->trans;
    if (obj->behaviorId == BHV_CAMERA_ANIMATION) {
        animObj->cameraStage = gAnimCameraCount;
        gAnimCameraCount++;
    }
    if (entry->attachMode == 18) {
        set_active_camera(animObj->cameraID);
        camera = cam_get_active_camera_no_cutscenes();
        animObjTrans->x_position = camera->trans.x_position;
        animObjTrans->y_position = camera->trans.y_position;
        animObjTrans->z_position = camera->trans.z_position;
        animObjTrans->rotation.y_rotation = (0x8000 - camera->trans.rotation.y_rotation);
        animObjTrans->rotation.x_rotation = -camera->trans.rotation.x_rotation;
        animObjTrans->rotation.z_rotation = camera->trans.rotation.z_rotation;
    }
    if ((entry->attachMode >= 10) && (entry->attachMode < 18)) {
        Object *seg = (*gRacers)[entry->attachMode - 10];
        if (seg != NULL) {
            animObjTrans->x_position = seg->trans.x_position;
            animObjTrans->y_position = seg->trans.y_position;
            animObjTrans->z_position = seg->trans.z_position;
            animObjTrans->rotation.y_rotation = seg->trans.rotation.y_rotation;
            animObjTrans->rotation.x_rotation = seg->trans.rotation.x_rotation;
            animObjTrans->rotation.z_rotation = seg->trans.rotation.z_rotation;
        }
    }
}

void anim_actor_apply_entry(Object *obj, LevelObjectEntry_Animation *entry, Object_AnimatedObject *animObj, UNUSED s32 index) {
    s32 initialAnimFrame;

    initialAnimFrame = entry->objAnimIndex;
    if (initialAnimFrame >= 0) {
        if (initialAnimFrame != obj->animationID) {
            obj->animFrame = entry->animStartFrame;
        }
        obj->animationID = entry->objAnimIndex;
        animObj->z = entry->objAnimSpeed;
        animObj->loopType = entry->objAnimLoopType;
    }
    if (entry->emitterMask >= 0) {
        animObj->emitterMask = entry->emitterMask;
    }
    animObj->pauseCounter = normalise_time(entry->pauseFrameCount);
    animObj->linearInterp = entry->linearInterp;
    animObj->specialHide = entry->specialHide;
    animObj->musicSequence = entry->musicSequence;
    animObj->musicFadeVolume = entry->musicFadeVolume;
    animObj->soundID = entry->soundID;
    animObj->fadeType = entry->fadeType;
    animObj->fadeColourIndex = entry->soundEffect;
    animObj->fadeOptions = entry->fadeOptions;
    animObj->fadeLength = entry->fadeAlpha;
    if (entry->messageId != 255) {
        set_current_text(entry->messageId);
    }
    if (entry->cutsceneID >= 0) {
        cutscene_set(entry->cutsceneID);
        return;
    }
    if (entry->triggerActorIndex >= 0) {
        anim_actor_trigger(entry->triggerActorIndex);
    }
    if (entry->signalCutsceneEnd >= 0) {
        gAnimFrameState[gAnimFrameParity]++;
    }
}

void anim_actor_trigger(s32 arg0) {
    s32 i;
    arg0 &= 0xFF; //?

    for (i = 0; i < gAnimObjectCount && (arg0 != (gAnimObjects[i]->properties.animation.behaviourID & 0xFF)); i++) {}

    if (i < gAnimObjectCount) {
        if (gAnimObjects[i]->animTarget != NULL) {
            if (gAnimObjects[i]->animTarget->animatedObject->startDelay < 0) {
                gAnimObjects[i]->animTarget->animatedObject->startDelay = 1;
            }
        }
    }
}

s8 anim_prev_frame_state(void) {
    return gAnimFrameState[1 - gAnimFrameParity];
}

s8 anim_actor_update(Object *obj, s32 updateRate) {
    s32 i;
    Object_AnimatedObject *animObj;

    animObj = obj->animatedObject;
    if (animObj->specialHide != 0) {
        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
    }
    if (animObj->pauseCounter == -1) {
        return animObj->specialHide;
    }
    if (animObj->pauseCounter >= 0) {
        animObj->pauseCounter -= updateRate;
    }
    if (animObj->pauseCounter == -1) {
        animObj->pauseCounter = -2;
    }
    if (animObj->pauseCounter <= 0) {
        obj->trans.flags |= OBJ_FLAGS_INVISIBLE;
        for (i = 0; (i < gAnimObjectCount && animObj->actorIndex != gAnimObjects[i]->properties.animation.behaviourID); i++) {
            if (FALSE) {} // FAKEMATCH
        }
        obj_init_animobject(gAnimObjects[i], obj);
        return 1;
    }
    return 0;
}

/**
 * Place the actor of animation path `actorIndex` at its current point on the path, for
 * the camera and other things that read actor positions. Returns TRUE when the actor is
 * not on a path.
 */
s32 anim_actor_is_active(s32 actorIndex) {
    Object_AnimatedObject *anim;
    Object *actor;
    AnimPathPoints pts;
    f32 t;
    f32 moveX;
    f32 moveY;
    f32 moveZ;
    f32 invBaseScale;
    s32 first;
    s32 count;
    s32 loopNode;

    if (gCutsceneID < 0) {
        return TRUE;
    }
    if (!anim_path_find(actorIndex, &first, &count)) {
        return TRUE;
    }
    actor = gAnimObjects[first]->animTarget;
    if (actor == NULL) {
        return TRUE;
    }
    anim = actor->animatedObject;
    invBaseScale = 1.0 / gAnimObjects[first]->header->scale;
    loopNode = anim_path_loop_node(first, count);
    anim_path_control_points(first, count, loopNode, anim->nodeIndex, anim->cameraID, &pts);

    t = anim->segmentProgress;
    if (anim->linearInterp == 0) {
        moveX = catmull_rom_interpolation(pts.x, 0, t);
        moveY = catmull_rom_interpolation(pts.y, 0, t);
        moveZ = catmull_rom_interpolation(pts.z, 0, t);
    } else {
        moveX = lerp(pts.x, 0, t);
        moveY = lerp(pts.y, 0, t);
        moveZ = lerp(pts.z, 0, t);
    }
    moveX -= actor->trans.x_position;
    moveY -= actor->trans.y_position;
    moveZ -= actor->trans.z_position;
    move_object(actor, moveX, moveY, moveZ);
    actor->trans.scale = catmull_rom_interpolation(pts.scale, 0, t) * invBaseScale * actor->header->scale;

    switch (anim->rotateType) {
        case 1:
            break;
        case 2:
            anim_path_face_along(actor, anim, &pts, 0, t, &moveZ);
            break;
        case 3:
            anim_path_face_actor(actor, anim->targetActorIndex);
            break;
        default:
            anim_path_apply_node_rotation(actor, anim, &pts, 0, t);
            break;
    }
    return FALSE;
}

f32 catmull_rom_interpolation(f32 *data, s32 index, f32 x) {
    f32 ret;
    f32 c, b, a;

    a = (-0.5 * data[index]) + (1.5 * data[index + 1]) + (-1.5 * data[index + 2]) + (0.5 * data[index + 3]);
    b = (1.0 * data[index]) + (-2.5 * data[index + 1]) + (2.0 * data[index + 2]) + (-0.5 * data[index + 3]);
    c = (data[index + 2] * 0.5) + (0.0 * data[index + 1]) + (-0.5 * data[index]) + (0.0 * data[index + 3]);

    ret = (1.0 * data[index + 1]);
    ret = (((((a * x) + b) * x) + c) * x) + ret;

    return ret;
}

/**
 * Interpolates x along a spline and returns the resultant progress along the spline.
 */
f32 cubic_spline_interpolation(f32 *data, s32 index, f32 x, f32 *derivative) {
    f32 ret;
    f32 c, b, a;

    a = (-0.5 * data[index]) + (1.5 * data[index + 1]) + (-1.5 * data[index + 2]) + (0.5 * data[index + 3]);
    b = (1.0 * data[index]) + (-2.5 * data[index + 1]) + (2.0 * data[index + 2]) + (-0.5 * data[index + 3]);
    c = (data[index + 2] * 0.5) + (0.0 * data[index + 1]) + (-0.5 * data[index]) + (0.0 * data[index + 3]);

    ret = (1.0 * data[index + 1]);
    *derivative = (((a * 3 * x) + (2 * b)) * x) + c;
    ret = (((((a * x) + b) * x) + c) * x) + ret;

    return ret;
}

f32 catmull_rom_derivative(f32 *data, s32 index, f32 x) {
    f32 derivative;
    f32 c, b, a;

    a = (-0.5 * data[index]) + (1.5 * data[index + 1]) + (-1.5 * data[index + 2]) + (0.5 * data[index + 3]);
    b = (1.0 * data[index]) + (-2.5 * data[index + 1]) + (2.0 * data[index + 2]) + (-0.5 * data[index + 3]);
    c = (data[index + 2] * 0.5) + (0.0 * data[index + 1]) + (-0.5 * data[index]) + (0.0 * data[index + 3]);

    derivative = (((a * 3 * x) + (2 * b)) * x) + c;

    return derivative;
}

/**
 * Imprecise method, which does not guarantee v = v1 when t = 1. (From Wikipedia)
 */
f32 lerp(f32 *data, u32 index, f32 t) {
    f32 result = data[index + 1] + t * ((data[index + 2] - data[index + 1]));
    return result;
}

/**
 * Peforms the lerp, and also returns the distance between the two points.
 */
f32 lerp_and_get_derivative(f32 *data, u32 index, f32 t, f32 *derivative) {
    f32 lerp;
    f32 vector;
    vector = data[index + 2] - data[index + 1];
    lerp = data[index + 1] + (vector * t);
    *derivative = vector;
    return lerp;
}


/**
 * Prepares the player racer for a Taj Challenge race.
 * Prevents them from pausing for 10 frames.
 */
void init_racer_for_challenge(s32 vehicleID) {
    Object_Racer *racer;

    gTajRaceInit = 3;
    racer = get_racer_object(PLAYER_ONE)->racer;
    racer->courseCheckpoint = 0;
    racer->nextCheckpoint = 0;
    racer->lap = 0;
    racer->trackOffsetX = 0;
    set_taj_challenge_type(vehicleID);
    set_pause_lockout_timer(10);
}

/**
 * Initialise the relevant variables related to races.
 * These are usually done on level load, but Taj challenges don't reset the level.
 */
void mode_init_taj_race(void) {
    CheckpointNode *checkpointNode;
    UNUSED s32 pad;
    s32 j;
    s32 i;
    s32 lvlSeg;
    LevelHeader *levelHeader;
    LevelObjectEntry_Racer newRacerEntry;
    Settings *settings;
    Object *racerObj;
    Object_Racer *racer;
    f32 yOut[8];

    gTajRaceInit -= 1;
    if (gTajRaceInit == 0) {
        levelHeader = level_header();
        gChallengePrevMusic = levelHeader->music;
        gChallengePrevInstruments = levelHeader->instruments;
        levelHeader->music = SEQUENCE_TAJS_RACES;
        levelHeader->instruments = 0xFFFF;
        racerObj = get_racer_object(PLAYER_ONE);
        racer = racerObj->racer;
        gIsTajChallenge = racer->vehicleID + 1;
        checkpointNode = racer_find_start_node(racerObj, racer);
        racer->cameraYaw = 0x8000 - racer->steerVisualRotation;
        racer->wrongWayCounter = 0;
        racer->startInput = 0;
        racer->courseCheckpoint = 0;
        racer->nextCheckpoint = 0;
        racer->lap = 0;
        racer->countLap = 0;
        racer->lap_times[0] = 0;
        racer->lap_times[1] = 0;
        racer->lap_times[2] = 0;
        racer->trackOffsetX = 0;
        settings = get_settings();
        gEventCountdown = 80;
        gRaceFinishTriggered = FALSE;
        gNumFinishedRacers = 1;
        levelHeader->laps = 3;
        levelHeader->race_type = RACETYPE_DEFAULT;
        hud_init_element();
        // clang-format off
        for (i = 0; i < ARRAY_COUNT(racer->lap_times); i++) { racer->lap_times[i] = 0; } // Must be a single line.
        // clang-format on
        newRacerEntry.common.x = (checkpointNode->x + (checkpointNode->rotationZFrac * 35.0f));
        newRacerEntry.common.z = (checkpointNode->z - (checkpointNode->rotationXFrac * 35.0f));
        lvlSeg =
            get_level_segment_index_from_position(newRacerEntry.common.x, checkpointNode->y, newRacerEntry.common.z);
        newRacerEntry.common.y =
            collision_get_y(lvlSeg, newRacerEntry.common.x, newRacerEntry.common.z, yOut, 8) ? yOut[0] : checkpointNode->y;
        newRacerEntry.common.size = 16;
        newRacerEntry.angleY = racer->steerVisualRotation;
        newRacerEntry.angleX = 0;
        newRacerEntry.angleZ = 0;
        newRacerEntry.playerIndex = 4;
        newRacerEntry.common.objectID = ASSET_OBJECT_ID_FLYINGCARPET;
        model_anim_offset(0);
        racerObj = spawn_object(&newRacerEntry.common, OBJECT_SPAWN_UNK01);
        (*gRacers)[1] = racerObj;
        gRacersByPosition[1] = racerObj;
        gRacersByPort[1] = racerObj;
        racerObj->level_entry = NULL;
        gNumRacers = 2;
        racer = racerObj->racer;
        i = 0; // Fakematch
        racer->vehicleID = VEHICLE_CARPET;
        racer->vehicleIDPrev = racer->vehicleID;
        racer->racerIndex = 1;
        racer->characterId = settings->racers[0].character;
        racer->stretch_height_cap = 1.0f;
        racer->stretch_height = 1.0f;
        racer->transparency = 0xFF;
        racer->vehicleSound = NULL;
        racerObj->interactObj->pushForce = 2;

        for (j = gObjectListStart; j < gObjectCount; j++) {
            if (!(gObjPtrList[j]->trans.flags & OBJ_FLAGS_PARTICLE) && gObjPtrList[j]->behaviorId == BHV_PARK_WARDEN) {
                racer->aiNodeObj = gObjPtrList[j];
            }
        }
        set_pause_lockout_timer(20);
        transition_begin(&gTajChallengeTransition);
    }
}

/**
 * Finds which golden balloon should be moved, and move it onto Taj's current position.
 * Sets opacity to 0, so the balloon with smoothly fade in.
 */
void obj_taj_create_balloon(s32 blockID, f32 x, f32 y, f32 z) {
    s32 i;
    Object *obj;
    Settings *settings = get_settings();

    for (i = 0; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (obj->behaviorId == BHV_GOLDEN_BALLOON && obj->level_entry != NULL &&
            obj->level_entry->goldenBalloon.challengeID > 0) {
            if (settings->tajFlags && settings->tajFlags & (1 << (obj->level_entry->goldenBalloon.challengeID + 2))) {
                obj->trans.x_position = x;
                obj->trans.y_position = y + 10.0;
                obj->trans.z_position = z;
                obj->segmentID = blockID;
                obj->properties.common.unk0 = 0;
                obj->opacity = 0;
            }
        }
    }
}

/**
 * Revert changes set by the game when starting the Taj challenge.
 * This includes the music set, as well as the extra racer spawned.
 * Has a different response depending on whether the challenge was aborted or finished.
 */
void mode_end_taj_race(s32 reason) {
    s32 flags;
    s32 i;
    Object_Racer *racer;
    Settings *settings;
    Object *obj;
    LevelHeader *levelHeader;

    levelHeader = level_header();
    levelHeader->race_type = RACETYPE_HUBWORLD;
    levelHeader->music = gChallengePrevMusic;
    levelHeader->instruments = gChallengePrevInstruments;
    minimap_opacity_set(1);

    // Only works with do {} while?
    i = 0;
    do {
        racer = (*gRacers)[i]->racer;
        if (!racer) {}
        racer->raceFinished = 0;
        racer->lap = 0;
        racer->nextCheckpoint = 0;
        racer->courseCheckpoint = 0;
    } while (++i < gNumRacers);

    free_object((*gRacers)[1]);
    gRacersByPosition[0] = (*gRacers)[0];
    gNumRacers = 1;
    for (i = gObjectListStart; i < gObjectCount; i++) {
        if (!(gObjPtrList[i]->trans.flags & OBJ_FLAGS_PARTICLE) && gObjPtrList[i]->behaviorId == BHV_PARK_WARDEN) {
            obj = gObjPtrList[i];
        }
    }
    racer = (*gRacers)[0]->racer;
    if (racer->challengeMarker != NULL) {
        free_object(racer->challengeMarker);
        racer->challengeMarker = NULL;
    }
    if (reason == CHALLENGE_END_FINISH) {
        if (racer->finishPosition == 1) {
            settings = get_settings();
            flags = racer->vehicleID;
            flags = 1 << (flags + 3);
            if (settings->tajFlags & flags) {
                set_next_taj_challenge_menu(5);
            } else {
                set_next_taj_challenge_menu(racer->vehicleID + 6);
                settings->tajFlags |= flags;
                safe_mark_write_save_file(get_save_file_index());
            }
        } else {
            set_next_taj_challenge_menu(4);
        }
        obj->properties.common.unk0 = 31;
        set_taj_status(TAJ_TELEPORT);
    } else {
        music_change_on();
        music_stop();
        set_next_taj_challenge_menu(0);
        audspat_jingle_on();
        if (reason == CHALLENGE_END_OOB) {
            set_current_text(ASSET_GAME_TEXT_0);
        }
        gEventCountdown = 0;
        gEventStartTimer = 0;
        obj->properties.common.unk0 = 20;
    }
    music_change_on();
    hud_audio_init();
    level_music_start(1.0f);
    gIsTajChallenge = FALSE;
}

/** The checkpoint or setup point a racer starts from. */
CheckpointNode *racer_find_start_node(Object *obj, Object_Racer *racer) {
    CheckpointNode *lastCheckpointNode;
    Camera *activeCameraSegment;
    s32 yOutCount;
    f32 yOut[9];
    Object *ptrList;
    s32 i;

    if (gNumberOfMainCheckpoints == 0) {
        lastCheckpointNode = NULL;
        for (i = 0; i < gObjectCount; i++) {
            ptrList = gObjPtrList[i];
            if (!(ptrList->trans.flags & OBJ_FLAGS_PARTICLE) && (ptrList->behaviorId == BHV_SETUP_POINT)) {
                if (ptrList->properties.setupPoint.racerIndex == 0) {
                    obj->trans.x_position = ptrList->trans.x_position;
                    obj->trans.y_position = ptrList->trans.y_position;
                    obj->trans.z_position = ptrList->trans.z_position;
                    obj->segmentID = ptrList->segmentID;
                    i = gObjectCount;
                }
            }
        }
    } else {
        lastCheckpointNode = &gTrackCheckpoints[gNumberOfMainCheckpoints - 1];
        obj->trans.x_position = lastCheckpointNode->x - (lastCheckpointNode->rotationZFrac * 35.0f);
        obj->trans.y_position = lastCheckpointNode->y;
        obj->trans.z_position = lastCheckpointNode->z + (lastCheckpointNode->rotationXFrac * 35.0f);
        obj->segmentID =
            get_level_segment_index_from_position(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
    }
    yOutCount = collision_get_y(obj->segmentID, obj->trans.x_position, obj->trans.z_position, yOut, 9);
    if (yOutCount != 0) {
        obj->trans.y_position = yOut[yOutCount - 1];
    }
    racer->prev_x_position = obj->trans.x_position;
    racer->prev_y_position = obj->trans.y_position;
    racer->prev_z_position = obj->trans.z_position;
    if (lastCheckpointNode != NULL) {
        racer->steerVisualRotation = arctan2_f(lastCheckpointNode->rotationXFrac, lastCheckpointNode->rotationZFrac);
    } else {
        racer->steerVisualRotation = ptrList->trans.rotation.y_rotation;
    }
    racer->nextCheckpoint = 0;
    racer->courseCheckpoint = racer->lap * gNumberOfMainCheckpoints;
    obj->trans.rotation.y_rotation = racer->steerVisualRotation;
    racer->wheelCollisionPos[0] = obj->trans.x_position;
    racer->wheelCollisionPos[1] = obj->trans.y_position + 15.0f;
    racer->wheelCollisionPos[2] = obj->trans.z_position;
    racer->wheelCollisionPos[3] = obj->trans.x_position;
    racer->wheelCollisionPos[4] = obj->trans.y_position + 15.0f;
    racer->wheelCollisionPos[5] = obj->trans.z_position;
    racer->wheelCollisionPos[6] = obj->trans.x_position;
    racer->wheelCollisionPos[7] = obj->trans.y_position + 15.0f;
    racer->wheelCollisionPos[8] = obj->trans.z_position;
    racer->wheelCollisionPos[9] = obj->trans.x_position;
    racer->wheelCollisionPos[10] = obj->trans.y_position + 15.0f;
    racer->wheelCollisionPos[11] = obj->trans.z_position;
    obj->interactObj->x_position = obj->trans.x_position;
    obj->interactObj->y_position = obj->trans.y_position;
    obj->interactObj->z_position = obj->trans.z_position;
    // fake
    if (1) {}
    if (1) {}
    racer->velocity = 0.0f;
    racer->lateral_velocity = 0.0f;
    obj->x_velocity = 0.0f;
    obj->z_velocity = 0.0f;
    racer->vehicleID = racer->vehicleIDPrev;
    if (racer->playerIndex != -1) {
        set_active_camera(racer->playerIndex);
        activeCameraSegment = cam_get_active_camera_no_cutscenes();
        activeCameraSegment->trans.x_position = obj->trans.x_position;
        activeCameraSegment->trans.y_position = obj->trans.y_position;
        activeCameraSegment->trans.z_position = obj->trans.z_position;
    }
    return lastCheckpointNode;
}

/**
 * Returns true if a taj challenge is currently active.
 */
s8 is_taj_challenge(void) {
    return gIsTajChallenge;
}

/**
 * Searches for the furthest teleport anchor and returns it.
 */
Object *find_furthest_telepoint(f32 x, f32 z) {
    Object *tempObj;
    Object *bestObj;
    f32 diffX;
    f32 diffZ;
    f32 distance;
    f32 bestDist;
    s32 i;

    bestDist = 0.0f;
    i = 0;
    bestObj = NULL;
    if (gObjectCount > 0) {
        do {
            tempObj = gObjPtrList[i];
            if (!(tempObj->trans.flags & OBJ_FLAGS_PARTICLE) && tempObj->behaviorId == BHV_TAJ_TELEPOINT) {
                diffX = tempObj->trans.x_position - x;
                diffZ = tempObj->trans.z_position - z;
                tempObj = gObjPtrList[i]; // fakematch
                distance = sqrtf((diffX * diffX) + (diffZ * diffZ));
                if (bestDist < distance) {
                    bestDist = distance;
                    bestObj = tempObj;
                }
            }
            i += 1;
        } while (i < gObjectCount);
    }
    return bestObj;
}

/** The boss race in progress, 1-based, or 0 outside boss races. */
s32 boss_race_number(void) {
    if (gBossRaceMode != 0) {
        return gBossRaceIds[1] + 1;
    } else if (level_type() == RACETYPE_BOSS) {
        return gBossRaceIds[1] + 1;
    }
    return 0;
}

/**
 * Return whether doors can be forced open.
 */
s8 obj_door_override(void) {
    return gOverrideDoors;
}

/**
 * Set a value that decides whether doors can be forced open.
 */
void obj_door_open(s32 setting) {
    gOverrideDoors = setting;
}

/**
 * Return the size of the object property struct intended to be used with the object.
 */
s32 get_object_property_size(Object *obj, void *obj64) {
    const ObjectBehaviour *behaviour = obj_behaviour(obj->header->behaviorId);
    s32 padding;
    s32 ret;

    if (behaviour->dataSize == 0) {
        obj->anyBehaviorData = NULL;
        return 4;
    }
    padding = 0;
    if (behaviour->dataAlign16) {
        padding = 0x10 - (s32) ((uintptr_t) obj64 & 0xF);
    }
    obj->anyBehaviorData = (void *) ((uintptr_t) obj64 + padding);
    ret = padding + behaviour->dataSize;
    return (ret & ~3) + 4;
}

/**
 * Run when an object is created.
 * Used to do one-time things like initialising variables
 * Arg2 is always zero. Effectively unused.
 */
void run_object_init_func(Object *obj, LevelObjectEntry *entry, s32 param) {
    obj->behaviorId = obj->header->behaviorId;
    if (obj_behaviour(obj->behaviorId)->init != NULL) {
        obj_behaviour(obj->behaviorId)->init(obj, entry, param);
    }
}

/**
 * Set initialisation property flags based off object ID.
 * This includes things like shadow data, interaction and visuals.
 */
s32 obj_init_property_flags(s32 behaviorId) {
    return obj_behaviour(behaviorId)->flags;
}

/**
 * Run every frame for most objects with set behaviours.
 * One big switch statement for whichever object.
 */
void run_object_loop_func(Object *obj, s32 updateRate) {
    update_object_stack_trace(OBJECT_UPDATE, obj->objectID);
    if (obj_behaviour(obj->behaviorId)->loop != NULL) {
        obj_behaviour(obj->behaviorId)->loop(obj, updateRate);
    }
    update_object_stack_trace(OBJECT_UPDATE, OBJECT_CLEAR);
}

s32 gObjectStackTrace[3] = { OBJECT_CLEAR, OBJECT_CLEAR, OBJECT_CLEAR };

/**
 * Note which object is being spawned, updated or drawn, for the readout log.
 */
void update_object_stack_trace(s32 index, s32 value) {
    if (index >= OBJECT_SPAWN && index <= OBJECT_DRAW) {
        gObjectStackTrace[index] = value;
    }
}

/** A ring of the last 128 readout values, kept for the crash screen and audio diagnostics. */
s16 *readout_log_get(s32 *currentCount, s32 *maxCount) {
    *currentCount = gReadoutLogIndex;
    *maxCount = ARRAY_COUNT(gReadoutLog);
    return gReadoutLog;
}

void readout_log_push(s16 arg0) {
    gReadoutLog[gReadoutLogIndex++] = arg0;
    if (gReadoutLogIndex >= ARRAY_COUNT(gReadoutLog)) {
        gReadoutLogIndex = 0;
    }
}

UNUSED const char sReadOutErrorString[] = "RO error %d!!\n";
UNUSED const char sPureAnguishString[] = "ARGHHHHHHHHH\n";
