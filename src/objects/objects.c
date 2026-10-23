#include "objects_internal.h"

#ifndef _ALIGN16
#define _ALIGN16(a) (((uintptr_t) (a) & ~0xF) + 0x10)
#endif

/************ .data ************/

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
s32 *gAssetsMiscSection;                                 // Official Name: Ftables
static Object_Boost sPcBoostTable[NUMBER_OF_CHARACTERS]; // ASSET_MISC_20 unpacked for the host
s32 *gAssetsMiscTable;                                   // Official Name: Findex
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
s8 gOverridePosCount;            // Number of OverRidePos objects in gOverridePosObjects
s8 gZoneJustLoaded;              // A boolean? I've seen it either as 0 or 1
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
s16 gAnimObjectCount;  // Number of Animation objects in gAnimObjects
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
u8 gBoostVehicleIds[16]; // Vehicle IDs for the boost objects, used to determine which racer is using which boost
                         // object.
u8 gBoostTypes[16];
u8 gBoostEffectFree[16];
RacerFXData gRacerFXData[NUMBER_OF_CHARACTERS];

/******************************/

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
    PCU_FIELD(ObjectHeader, waterEffectAnimSpeed, 0x00, 4,
              PCU_U32),                                  // waterEffectAnimSpeed, shadowScale, waterEffectScale, scale
    PCU_FIELD(ObjectHeader, modelIds, 0x10, 4, PCU_PTR), // modelIds..objectParticles
    PCU_FIELD(ObjectHeader, pad20, 0x20, 1, PCU_U32),
    PCU_FIELD(ObjectHeader, lightSources, 0x24, 1, PCU_PTR),
    PCU_FIELD(ObjectHeader, shadeAmbient, 0x28, 2, PCU_U32),
    PCU_FIELD(ObjectHeader, flags, 0x30, 5, PCU_U16), // flags..unk38
    PCU_FIELD(ObjectHeader, shadeLightR, 0x3A, 4, PCU_BYTES),
    PCU_FIELD(ObjectHeader, shadeAngleY, 0x3E, 10, PCU_U16),     // shadeAngleY..unk50
    PCU_FIELD(ObjectHeader, instanceCount, 0x52, 14, PCU_BYTES), // instanceCount..pad5E
    PCU_FIELD(ObjectHeader, internalName, 0x60, 16, PCU_BYTES),
    PCU_FIELD(ObjectHeader, unused70, 0x70, 8, PCU_BYTES),
    PCU_END,
};

s32 gObjectStackTrace[3] = { OBJECT_CLEAR, OBJECT_CLEAR, OBJECT_CLEAR };

UNUSED const char sReadOutErrorString[] = "RO error %d!!\n";
UNUSED const char sPureAnguishString[] = "ARGHHHHHHHHH\n";

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
    gLoadedObjectHeaders =
        mempool_alloc_safe(gAssetsObjectHeadersTableLength * sizeof(ObjectHeader *), COLOUR_TAG_WHITE);
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
            RacerShieldGfx *shield = (RacerShieldGfx *) &gAssetsMiscSection[gAssetsMiscTable[ASSET_MISC_SHIELD_DATA]];
            s32 n;
            _Static_assert(sizeof(RacerShieldGfx) == 0x10, "RacerShieldGfx layout drifted from N64");
            _Static_assert(__builtin_offsetof(RacerShieldGfx, scale) == 8, "RacerShieldGfx layout drifted from N64");
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
            ASSET_MISC_4,
            ASSET_MISC_8,
            ASSET_MISC_RACER_WEIGHT,
            ASSET_MISC_RACER_HANDLING,
            ASSET_MISC_RACER_UNUSED_11,
            ASSET_MISC_17,
            ASSET_MISC_18,
            ASSET_MISC_MAGNET_DATA,
            ASSET_MISC_32,
            ASSET_MISC_RACERACCELERATION_UNKNOWN0,
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

/** Resolves a set of moving points against an object model's collision facets. */
s32 model_resolve_collisions(ObjectModel *arg0, s32 arg1, s32 *numCollisions, f32 *originPointsX, f32 *originPointsY,
                             f32 *originPointsZ, f32 *targetPointsX, f32 *targetPointsY, f32 *targetPointsZ,
                             f32 *collisionRadii, s8 *surfaces, f32 scale) {
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
 * Would've rendered an individual checkpoint node. On https://noclip.website, with dev objects enabled, you can see a
 * visual representation of what these checkpoints would've looked like ingame.
 */
UNUSED void debug_render_checkpoint_node(UNUSED s32 checkpointID, UNUSED s32 pathID, UNUSED Gfx **dList,
                                         UNUSED Mtx **mtx, UNUSED Vertex **vtx) {
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
