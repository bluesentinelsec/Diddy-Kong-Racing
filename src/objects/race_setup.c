#include "objects_internal.h"

static u16 gRacerObjectTable[] = {
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
static BossRaceVehicles gBossVehicles[] = {
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

static s8 gDefaultCharacterOrder[8] = { 9, 1, 2, 3, 4, 5, 7, 0 };

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
