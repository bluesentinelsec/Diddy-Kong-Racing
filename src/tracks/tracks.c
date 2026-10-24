#include "tracks_internal.h"

/************ .data ************/

//!@bug These two transition effects are marked to not clear when done, meaning they stay active the whole time.
FadeTransition gFullFadeToBlack = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_OUT, FADE_COLOR_BLACK, 40, FADE_STAY);
FadeTransition gCircleFadeToBlack = FADE_TRANSITION(FADE_CIRCLE, FADE_FLAG_OUT, FADE_COLOR_BLACK, 70, FADE_STAY);

LevelModel *gCurrentLevelModel = NULL; // Official Name: track
LevelHeader *gCurrentLevelHeader2 = NULL;

#if REGION == REGION_JP
// T.T.カメラ  -  T.T. Camera
char gJpnTTCam[] = { 0x80, 0x2D, 0x80, 0x3C, 0x80, 0x2D, 0x80, 0x3C, 0x80, 0x55, 0x80, 0x71, 0x80, 0x76 };
#endif
u8 *gVoidData = NULL;

/*******************************/

/************ .bss ************/

Gfx *gTrackDL;
Mtx *gTrackMtxPtr;
Vertex *gTrackVtxPtr;
Triangle *gTrackTriPtr;

Camera *gSceneActiveCamera;

s32 gSceneCurrentPlayerID;
Object *gSkydomeSegment;
UNUSED s32 gIsNearCurrBBox; // Set to true if the current visible segment is close to the camera.
UNUSED s32 gDisableShadows; // Never not 0.
s32 gShadowHeapFlip;        // Flips between 0 and 1 to prevent incorrect access between frames.
s32 gShadowIndex;
s32 gWaterEffectIndex;
s32 gSceneStartSegment;
s32 gSceneRenderSkyDome;
s8 gDrawLevelSegments;
u8 gVoidColourR; // R of RGB
u8 gVoidColourG; // G of RGB
u8 gVoidColourB; // B of RGB
f32 gCollisionNormalX;
f32 gCollisionNormalY;
f32 gCollisionNormalZ;
s32 gHitWall;
s32 gCollisionMode;
s32 gLevelIsCutscene; // the loaded level is one of the two cutscene race types
s32 gAntiAliasing;
s32 gTTCamPlayerID;
s32 gTTCamID;
s32 gTTCamSmoothTimer;
s32 gTrackTexAnimOffset;
u32 gTrackTexAnimFlags;
s32 gShadowVertexCount;
ShadowVertex gShadowVertices[32]; // Struct sizeof(0x10) / sizeof(16)
s32 gShadowBucketCounts[SHADOW_BUCKET_COUNT];
// The original had 120 entries, so the last bucket's top eight slots (clip_polygon_to_rect can fill
// up to 31 per bucket) ran past the end; the port gives every bucket its full size.
ShadowClipVertex gShadowBucketVertices[SHADOW_BUCKET_COUNT * SHADOW_BUCKET_SIZE];
s32 gShadowTriCount;
ShadowPolygon gShadowTris[32]; // Struct sizeof(0xC) / sizeof(12)
ShadowWaveTri gShadowWaveCells[64];
ShadowPlane gShadowWavePlanes[128];
s32 gShadowWavePlaneCount;
ShadowPlane *gShadowCurrentPlane;
TextureHeader *gNewShadowTexture;
Object *gNewShadowObj;
f32 gShadowYOffset;
s16 gNewShadowY1;
s16 gNewShadowY2;
s16 gShadowMinY;
f32 gShadowOpacity;
f32 gNewShadowScale;
f32 gNewShadowWidth;
f32 gNewShadowLength;
f32 gShadowArea;
s32 gShadowCentreTri;
s32 gShadowCentreShared;
f32 gShadowFadeStart;
f32 gShadowFadeRange;
Vec4f gFrustumPlanes[3];
// The original had 20 of each, so a point over 20 water polygons wrote the segment's own water
// surface one past the end; the port adds the slot for it.
WaterProperties gWaterSurfaceHits[WATER_SURFACE_HITS_MAX];
WaterProperties *gTrackWaves[WATER_SURFACE_HITS_MAX];
s8 gWaterSurfaceHitCount;
LevelModel *gTrackModelHeap;
s32 *gLevelModelTable;
UNUSED f32 gPrevCameraX;          // Set but never read
UNUSED f32 gPrevCameraY;          // Set but never read
UNUSED f32 gPrevCameraZ;          // Set but never read
Triangle *gShadowHeapTris[2 + 2]; // Triangle Data for shadows
Triangle *gCurrShadowTris;
Vertex *gShadowHeapVerts[2 + 2]; // Vertex Data for shadows
Vertex *gCurrShadowVerts;
ShadowHeapProperties *gShadowHeapData[2 + 2]; // General data for shadows. Texture and geometry size.
ShadowHeapProperties *gCurrShadowHeapData;
s32 gShadowTail;                // Position in the heap the shadow data ends at.
s32 gNewShadowTriCount;         // xOffset?
s32 gNewShadowVtxCount;         // yOffset?
ptrint_t *gCollisionCandidates; // Allocated 0x7D0 (N64); pointer-sized entries on the host
s8 *gCollisionSurfaces;
s32 gNumCollisionCandidates;
s32 gScenePlayerViewports;
UNUSED f32 gCurrBBoxDistanceToCamera; // Used in a comparison check, but functionally unused.
u32 gWaveBlockCount;
FogData gFogData[4];
Vec3i gScenePerspectivePos;
VoidMesh *gVoidMesh;              // 0x10 bytes struct?
VoidCheckPoint *gVoidCheckPoints; // 0xC bytes struct?
s8 *gVoidCheckPairs;
Vertex *gVoidVerts[2];
Vertex *gVoidCurrVerts;
Triangle *gVoidTris[2];
Triangle *gVoidCurrTris;
s16 gVoidCheckPairIndex;
s16 gVoidCheckPointCount;
f32 gVoidCheckSidePlaneX; // something x coordinate related
f32 gVoidCheckSidePlaneZ; // something z coordinate related
f32 gVoidCheckSidePlaneD;
f32 gVoidCheckAheadX; // something x coordinate related
f32 gVoidCheckAheadZ; // something z coordinate related
s8 gVoidVertexFlip;
s16 gVoidVertCount;
s16 gVoidPrimCount;
s16 gVoidCheckMaxPoints;
s16 gVoidPrimLimit;

/******************************/

/************ .rodata ************/
UNUSED const char gTrackClippingErrorString[] = "Solid Clipping x0=x1 Error!!!\n";
UNUSED const char gTrackHeightOverflowString[] = "TrackGetHeight() - Overflow!!!\n";

#undef PC_REBASE

/**
 * Sets the number of expected viewports in the scene.
 * Like most other viewport vars, it's 0-3 rather than 1-4.
 * Set as an s32 for some reason.
 */
s32 set_scene_viewport_num(s32 numPorts) {
    gScenePlayerViewports = numPorts;
    return 0;
}

/**
 * Initialises the level.
 * Allocates RAM to load generate the level geometry, spawn objects and generate shadows.
 */
void init_track(u32 geometry, u32 skybox, s32 numberOfPlayers, Vehicle vehicle, u32 entranceId, u32 collectables,
                u32 objectMapId) {
    s32 i;

    gCurrentLevelHeader2 = level_header();
    gLevelIsCutscene = FALSE;
    gTTCamPlayerID = 0;
    gTTCamID = 0;
    gTTCamSmoothTimer = 0;

    if (gCurrentLevelHeader2->race_type == RACETYPE_CUTSCENE_1 ||
        gCurrentLevelHeader2->race_type == RACETYPE_CUTSCENE_2) {
        gLevelIsCutscene = TRUE;
    }

    generate_track(geometry);

    gWaveBlockCount = 0;

    if (numberOfPlayers < 2) {
        for (i = 0; i < gCurrentLevelModel->numberOfSegments; i++) {
            if (gCurrentLevelModel->segments[i].hasWaves != 0) {
                gWaveBlockCount++;
                gCurrentLevelModel->segments[i].hasWaves = 1;
            }
        }
    }

    if (is_in_two_player_adventure() &&
        (gCurrentLevelHeader2->race_type == RACETYPE_DEFAULT || gCurrentLevelHeader2->race_type & RACETYPE_CHALLENGE)) {
        i = 2;
    } else {
        i = numberOfPlayers + 1;
    }

    stubbed_printf("pre waves_init waveCount=%d\n", gWaveBlockCount);
    if (gWaveBlockCount) {
        waves_init(gCurrentLevelModel, gCurrentLevelHeader2, i);
    }
    stubbed_printf("post waves_init waveCount=%d\n", gWaveBlockCount);

    cam_set_layout(numberOfPlayers);
    skydome_spawn(skybox);
    stubbed_printf("post skydome_spawn\n");
    gTrackTexAnimOffset = 0;
    gTrackTexAnimFlags = RENDER_TEX_ANIM;
    path_enable();
    track_spawn_objects(objectMapId, 0);
    stubbed_printf("post track_spawn_objects objectMapId\n");
    track_spawn_objects(collectables, 1);
    stubbed_printf("post track_spawn_objects collectables\n");
    gScenePlayerViewports = numberOfPlayers;
    track_setup_racers(vehicle, entranceId, numberOfPlayers);
    stubbed_printf("post track_setup_racers\n");
    racerfx_alloc(72, 64);
    stubbed_printf("post racerfx_alloc\n");

    if (geometry == 0 && entranceId == 0) {
        transition_begin(&gCircleFadeToBlack);
    } else {
        transition_begin(&gFullFadeToBlack);
    }
    cam_set_layout(gScenePlayerViewports);

    numberOfPlayers = gScenePlayerViewports;
    gAntiAliasing = FALSE;
    // All shadow groups are double buffered. Environment shadows never change, so they could've single buffered them.
    for (i = 0; i < ARRAY_COUNT(gShadowHeapData); i++) {
        gShadowHeapData[i] =
            (ShadowHeapProperties *) mempool_alloc_safe(sizeof(ShadowHeapProperties) * 400, COLOUR_TAG_YELLOW);
        gShadowHeapTris[i] = (Triangle *) mempool_alloc_safe(sizeof(Triangle) * 800, COLOUR_TAG_YELLOW);
        gShadowHeapVerts[i] = (Vertex *) mempool_alloc_safe(sizeof(Vertex) * 2000, COLOUR_TAG_YELLOW);
    }
    stubbed_printf("post shadow heap alloc\n");

    gShadowHeapFlip = 0;
    shadow_update(SHADOW_SCENERY, SHADOW_SCENERY, LOGIC_NULL);
    shadow_update(SHADOW_ACTORS, SHADOW_ACTORS, LOGIC_NULL);
    gShadowHeapFlip = 1;
    shadow_update(SHADOW_SCENERY, SHADOW_SCENERY, LOGIC_NULL);
    shadow_update(SHADOW_ACTORS, SHADOW_ACTORS, LOGIC_NULL);
    gShadowHeapFlip = 0;
    stubbed_printf("post shadow_update\n");
    if (gCurrentLevelHeader2->useVoid) {
        gVoidColourR = gCurrentLevelHeader2->voidColour.red;
        gVoidColourG = gCurrentLevelHeader2->voidColour.green;
        gVoidColourB = gCurrentLevelHeader2->voidColour.blue;
        void_init(numberOfPlayers + 1);
        stubbed_printf("post void_init\n");
    }
    stubbed_printf("init_track end\n");
}

/**
 * Advances the level's animated parts for this frame: waves, actor shadows, colour cycles and
 * pulsing lights. animUpdateRate is 0 while paused; shadows always use updateRate.
 */
static void update_level_animations(s32 updateRate, s32 animUpdateRate) {
    s32 i;

    if (gWaveBlockCount) {
        waves_update(animUpdateRate);
    }
    shadow_update(SHADOW_ACTORS, SHADOW_ACTORS, updateRate);
    for (i = 0; i < 7; i++) {
        if (ASSET_SLOT(gCurrentLevelHeader2->colourCycles[i]) != -1) {
            update_colour_cycle(gCurrentLevelHeader2->colourCycles[i], animUpdateRate);
        }
    }
    if (gCurrentLevelHeader2->pulseLightData != (PulsatingLightData *) -1) {
        update_pulsating_light_data(gCurrentLevelHeader2->pulseLightData, animUpdateRate);
    }
}

/**
 * Scrolls and animates the sky texture of the flashy background, for levels that use it
 * instead of a skydome.
 */
static void scroll_flashy_sky(s32 animUpdateRate) {
    s32 i;

    if (gCurrentLevelHeader2->skyDome == -1) {
        i = (gCurrentLevelHeader2->skyTexture->width << 9) - 1;
        gCurrentLevelHeader2->skyTexScrollU =
            (gCurrentLevelHeader2->skyTexScrollU + (gCurrentLevelHeader2->skyTexScrollSpeedU * animUpdateRate)) & i;
        i = (gCurrentLevelHeader2->skyTexture->height << 9) - 1;
        gCurrentLevelHeader2->skyTexScrollV =
            (gCurrentLevelHeader2->skyTexScrollV + (gCurrentLevelHeader2->skyTexScrollSpeedV * animUpdateRate)) & i;
        tex_animate_texture(gCurrentLevelHeader2->skyTexture, &gTrackTexAnimFlags, &gTrackTexAnimOffset,
                            animUpdateRate);
    }
}

/**
 * Draws the current player's view: fog, camera and viewport, the sky (skydome or flashy
 * background in single player, a gradient otherwise), the level and its objects, weather,
 * lens flare and the player's HUD.
 */
static void render_player_view(s32 numViewports, s32 updateRate, s32 animUpdateRate) {
    apply_fog(gSceneCurrentPlayerID);
    gDPPipeSync(gTrackDL++);
    set_active_camera(gSceneCurrentPlayerID);
    viewport_main(&gTrackDL, &gTrackMtxPtr);
    compute_frustum_planes();
    // Show detailed skydome in single player.
    if (numViewports < 2) {
        mtx_world_origin(&gTrackDL, &gTrackMtxPtr);
        if (gCurrentLevelHeader2->skyDome == -1) {
            trackbg_render_flashy();
        } else {
            skydome_render();
        }
    } else {
        mtx_perspective(&gTrackDL, &gTrackMtxPtr);
        trackbg_render_gradient();
        cam_set_projection_matrix(&gTrackDL, &gTrackMtxPtr);
        mtx_world_origin(&gTrackDL, &gTrackMtxPtr);
    }
    gDPPipeSync(gTrackDL++);
    initialise_player_viewport_vars(updateRate);
    weather_clip_planes(-1, -512);
    // Show weather effects in single player.
    if (gCurrentLevelHeader2->weatherEnable > 0 && numViewports < 2) {
        weather_update(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, &gTrackTriPtr, animUpdateRate);
    }
    lensflare_override(cam_get_active_camera());
    lensflare_render(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, cam_get_active_camera());
    hud_render_player(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, get_racer_object_by_port(gSceneCurrentPlayerID),
                      updateRate);
}

/**
 * The root function for rendering the entire scene.
 * Handles drawing the track, objects and the majority of the HUD in single player.
 * Official name: trackDraw
 */
void render_scene(Gfx **dList, Mtx **mtx, Vertex **vtx, Triangle **tris, s32 updateRate) {
    s32 numViewports;
    s32 animUpdateRate;
    s8 flip;
    s32 posX;
    s32 posY;
    s32 j;

    gTrackDL = *dList;
    gTrackMtxPtr = *mtx;
    gTrackVtxPtr = *vtx;
    gTrackTriPtr = *tris;
    gSceneRenderSkyDome = TRUE;
    gDisableShadows = FALSE;
    gIsNearCurrBBox = FALSE;
    numViewports = cam_set_layout(gScenePlayerViewports);
    if (is_game_paused()) {
        animUpdateRate = 0;
    } else {
        animUpdateRate = updateRate;
    }
    update_level_animations(updateRate, animUpdateRate);
    gDrawLevelSegments = TRUE;
    if (gCurrentLevelHeader2->race_type == RACETYPE_CUTSCENE_2) {
        gDrawLevelSegments = FALSE;
        gAntiAliasing = TRUE;
    }
    if (gCurrentLevelHeader2->race_type == RACETYPE_CUTSCENE_1 || gCurrentLevelHeader2->forceAntiAliasing) {
        gAntiAliasing = TRUE;
    }
    scroll_flashy_sky(animUpdateRate);
    flip = FALSE;
    if (get_filtered_cheats() & CHEAT_MIRRORED_TRACKS) {
        flip = TRUE;
    }
    rendermode_reset(&gTrackDL);
    gDkrDisableBillboard(gTrackDL++);
    gSPClearGeometryMode(gTrackDL++, G_CULL_FRONT);
    gDPSetBlendColor(gTrackDL++, 0, 0, 0, 100);
    gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(gTrackDL++, 255, 255, 255, 0);
    rain_fog();
    update_fog(numViewports, animUpdateRate);
    scroll_particle_textures(animUpdateRate);
    if (gCurrentLevelModel->numberOfAnimatedTextures > 0) {
        track_tex_anim(animUpdateRate);
    }
    for (j = gSceneCurrentPlayerID = 0; j < numViewports; gSceneCurrentPlayerID++, j = gSceneCurrentPlayerID) {
        if (gCurrentLevelHeader2 && !gCurrentLevelHeader2 && !gCurrentLevelHeader2) {} // Fakematch
        if (j == 0) {
            if (is_player_two_in_control() && numViewports == 1) {
                gSceneCurrentPlayerID = PLAYER_TWO;
            }
        }
        if (flip) {
            gSPSetGeometryMode(gTrackDL++, G_CULL_FRONT);
        }
        render_player_view(numViewports, updateRate, animUpdateRate);
    }
    // Show TT Cam toggle for the fourth viewport when playing 3 player.
    if (numViewports == 3 && level_type() != RACETYPE_CHALLENGE_EGGS && level_type() != RACETYPE_CHALLENGE_BATTLE &&
        level_type() != RACETYPE_CHALLENGE_BANANAS) {
        if (hud_setting() == 0) {
            if (flip) {
                gSPSetGeometryMode(gTrackDL++, G_CULL_FRONT);
            }
            apply_fog(PLAYER_FOUR);
            gDPPipeSync(gTrackDL++);
            set_active_camera(PLAYER_FOUR);
            disable_cutscene_camera();
            ttcam_update(updateRate);
            viewport_main(&gTrackDL, &gTrackMtxPtr);
            compute_frustum_planes();
            mtx_perspective(&gTrackDL, &gTrackMtxPtr);
            trackbg_render_gradient();
            cam_set_projection_matrix(&gTrackDL, &gTrackMtxPtr);
            mtx_world_origin(&gTrackDL, &gTrackMtxPtr);
            gDPPipeSync(gTrackDL++);
            initialise_player_viewport_vars(updateRate);
            weather_clip_planes(-1, -512);
            lensflare_override(cam_get_active_camera());
            lensflare_render(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, cam_get_active_camera());
            set_text_font(FONT_COLOURFUL);
            if (osTvType == OS_TV_TYPE_PAL) {
                posX = SCREEN_WIDTH_HALF + 6;
                posY = SCREEN_HEIGHT_HALF_PAL + 6;
            } else {
                posX = SCREEN_WIDTH_HALF + 10;
                posY = SCREEN_HEIGHT_HALF + 5;
            }
#if REGION == REGION_JP
            draw_text(&gTrackDL, posX, posY, gJpnTTCam, ALIGN_TOP_LEFT);
#else
            draw_text(&gTrackDL, posX, posY, "TT CAM", ALIGN_TOP_LEFT);
#endif
        } else {
            set_active_camera(PLAYER_FOUR);
            ttcam_update(updateRate);
        }
    }
    viewport_reset(&gTrackDL);
    gDPPipeSync(gTrackDL++);
    gDkrDisableBillboard(gTrackDL++);
    gShadowHeapFlip = 1 - gShadowHeapFlip;
    *dList = gTrackDL;
    *mtx = gTrackMtxPtr;
    *vtx = gTrackVtxPtr;
    *tris = gTrackTriPtr;
}

/**
 * Whether a track wall lies between the current viewport's racer and the
 * camera, by casting from the racer to the camera through the collision
 * candidates. Used to pull the camera in front of walls.
 */
s32 is_camera_behind_wall(void) {
    LevelModelSegment *segment;
    s32 ret;
    s32 candidateIndex;
    f32 playerDist;
    f32 camDist;
    f32 scalingFactor;
    s32 curViewport;
    ptrint_t flipSide;
    s32 numRacers;
    s32 i;
    s32 j;
    f32 edgeDist;
    f32 A1, B1, C1, D1;
    f32 hitX;
    f32 hitY;
    f32 hitZ;
    CollisionNode *colNode;
    Object_Racer *racer;
    f32 A, B, C, D;
    Object **racerObjects;
    Object *racerObj;
    f32 *planes;

    racerObjects = get_racer_objects(&numRacers);
    if (numRacers == 0) {
        return FALSE;
    }
    if ((check_if_showing_cutscene_camera() != 0) || (gSceneActiveCamera->mode >= CAMERA_FINISH_CHALLENGE) ||
        (gSceneActiveCamera->mode == CAMERA_FIXED)) {
        return FALSE;
    }
    curViewport = get_current_viewport();
    racerObj = NULL;
    for (i = 0; i < numRacers; i++) {
        racer = racerObjects[i]->racer;
        if (curViewport == racer->playerIndex) {
            racerObj = racerObjects[i];
            i = numRacers; // Come on! Just use break!
        }
    }
    if (racerObj == NULL) {
        return FALSE;
    }
    generate_collision_candidates(1, &racerObj->trans.position, &gSceneActiveCamera->trans.position,
                                  VEHICLE_NO_OVERRIDE);
    ret = FALSE;
    for (candidateIndex = 0; candidateIndex < gNumCollisionCandidates && ret == FALSE; candidateIndex++) {
        flipSide = gCollisionCandidates[candidateIndex];
        if (COLLISION_ENTRY_IS_SEGMENT(flipSide)) {
            // this is segment Entry
            segment = COLLISION_ENTRY_SEGMENT(flipSide);
        } else {
            colNode = (CollisionNode *) flipSide;
            curViewport = colNode->colPlaneIndex << 2;
            planes = &segment->collisionPlanes[curViewport];
            A = planes[0];
            B = planes[1];
            C = planes[2];
            D = planes[3];

            camDist = A * gSceneActiveCamera->trans.x_position + B * gSceneActiveCamera->trans.y_position +
                      C * gSceneActiveCamera->trans.z_position + D - 14.0;
            if (camDist < -0.1) {
                playerDist = A * racerObj->trans.x_position + B * racerObj->trans.y_position +
                             C * racerObj->trans.z_position + D;
                if (playerDist >= -0.1) {
                    hitX = (gSceneActiveCamera->trans.x_position - racerObj->trans.x_position);
                    hitY = (gSceneActiveCamera->trans.y_position - racerObj->trans.y_position);
                    hitZ = (gSceneActiveCamera->trans.z_position - racerObj->trans.z_position);

                    if (playerDist != camDist) {
                        scalingFactor = playerDist / (playerDist - camDist);
                    } else {
                        scalingFactor = 0.0f;
                    }

                    hitX = racerObj->trans.x_position + (hitX * scalingFactor);
                    hitY = racerObj->trans.y_position + (hitY * scalingFactor);
                    hitZ = racerObj->trans.z_position + (hitZ * scalingFactor);

                    for (j = 0, ret = TRUE; j < 3 && ret == TRUE; j++) {
                        flipSide = FALSE;
                        curViewport = colNode->closestTri[j];
                        if (curViewport & EDGE_PLANE_FLIPPED) {
                            curViewport &= EDGE_PLANE_INDEX_MASK;
                            flipSide = TRUE;
                        }
                        curViewport = curViewport << 2;
                        planes = &segment->collisionPlanes[curViewport];
                        A1 = planes[0];
                        B1 = planes[1];
                        C1 = planes[2];
                        D1 = planes[3];
                        edgeDist = A1 * hitX + B1 * hitY + C1 * hitZ + D1;
                        if (flipSide) {
                            edgeDist = -edgeDist;
                        }
                        if (edgeDist > 4.0f) {
                            ret = FALSE;
                        }
                    }
                }
            }
        }
    }
    return ret;
}

/**
 * Sets up the camera placement for the 4th viewport when using T.T Cam in 3 player.
 * It utilises spectate points then points at the race leader.
 * Uses lookat smoothing when changing which player, otherwise, snaps if the camera itself changes.
 */
void ttcam_update(s32 updateRate) {
    s16 angleDiff;
    f32 xDelta;
    f32 yDelta;
    f32 zDelta;
    f32 xzSqr;
    Camera *camera;
    Object *thisObject;
    Object **racerGroup;
    Object *lastObject;
    Object *objectFirstPlace;
    Object_Racer *currentRacer;
    Object_Racer *lastRacer;
    Object_Racer *racerFirstPlace;
    s32 numRacers;
    s32 i;
    s32 cameraId;
    Object *camObj;

    racerGroup = get_racer_objects(&numRacers);
    lastRacer = NULL;
    for (i = 0; i < numRacers; i++) {
        if (1) {} // fake
        if (racerGroup[i] != NULL) {
            currentRacer = racerGroup[i]->racer;
            cameraId = currentRacer->cameraIndex;
            spectate_nearest(racerGroup[i], &cameraId);
            currentRacer->cameraIndex = cameraId;
            if (currentRacer->raceFinished) {
                if (currentRacer->finishPosition == 1) {
                    racerFirstPlace = currentRacer;
                    objectFirstPlace = racerGroup[i];
                }
            } else if (lastRacer == NULL) {
                lastRacer = currentRacer;
                lastObject = racerGroup[i];
            } else if (currentRacer->racePosition < lastRacer->racePosition) {
                lastRacer = currentRacer;
                lastObject = racerGroup[i];
            }
        }
    }
    if (lastRacer != NULL) {
        currentRacer = lastRacer;
        thisObject = lastObject;
    } else {
        currentRacer = racerFirstPlace;
        thisObject = objectFirstPlace;
    }
    camObj = spectate_object(currentRacer->cameraIndex);
    if (gTTCamID != currentRacer->cameraIndex) {
        gTTCamSmoothTimer = 0;
    } else if (gTTCamPlayerID != currentRacer->playerIndex) {
        gTTCamSmoothTimer = 180;
        gTTCamPlayerID = currentRacer->playerIndex;
    }
    if (camObj != NULL) {
        camera = cam_get_active_camera_no_cutscenes();
        camera->trans.x_position = camObj->trans.x_position;
        camera->trans.y_position = camObj->trans.y_position;
        camera->trans.z_position = camObj->trans.z_position;
        xDelta = camera->trans.x_position - thisObject->trans.x_position;
        yDelta = camera->trans.y_position - thisObject->trans.y_position;
        zDelta = camera->trans.z_position - thisObject->trans.z_position;
        xzSqr = sqrtf((xDelta * xDelta) + (zDelta * zDelta));
        if (gTTCamSmoothTimer != 0) {
            angleDiff = ((s32) (-atan2s(xDelta, zDelta) - camera->trans.rotation.y_rotation) + 0x8000);
            //!@bug Never true, since angleDiff is signed. Should be >=.
            if (angleDiff > 0x8000) {
                angleDiff = -(0xFFFF - angleDiff);
            }
            camera->trans.rotation.y_rotation += ((s32) (angleDiff / (16.0f * (gTTCamSmoothTimer / 180.0f)))) & 0xFFFF;
            angleDiff = atan2s(yDelta, xzSqr) - camera->trans.rotation.x_rotation;
            //!@bug Never true, since angleDiff is signed. Should be >=.
            if (angleDiff > 0x8000) {
                angleDiff = -(0xFFFF - angleDiff);
            }
            camera->trans.rotation.x_rotation += ((s32) (angleDiff / (16.0f * (gTTCamSmoothTimer / 180.0f)))) & 0xFFFF;
            gTTCamSmoothTimer -= updateRate;
            if (gTTCamSmoothTimer < 0) {
                gTTCamSmoothTimer = 0;
            }
        } else {
            camera->trans.rotation.y_rotation = 0x8000 - atan2s(xDelta, zDelta);
            camera->trans.rotation.x_rotation = atan2s(yDelta, xzSqr);
        }
        camera->trans.rotation.z_rotation = 0;
        camera->cameraSegmentID = get_level_segment_index_from_position(camera->trans.x_position, currentRacer->oy1,
                                                                        camera->trans.z_position);
        gTTCamID = currentRacer->cameraIndex;
    }
}
