#include "tracks.h"

#include "asset_loading.h"
#include "camera.h"
#include "collision.h"
#include "common.h"
#include "f3ddkr.h"
#include "fade_transition.h"
#include "game.h"
#include "game_ui.h"
#include "gzip.h"
#include "host.h"
#include "macros.h"
#include "math_util.h"
#include "memory.h"
#include "menu.h"
#include "objects.h"
#include "particles.h"
#include "pc_swap.h"
#include "PR/gu.h"
#include "PRinternal/viint.h"
#include "racer.h"
#include "structs.h"
#include "textures_sprites.h"
#include "thread3_main.h"
#include "types.h"
#include "video.h"
#include "waves.h"
#include "weather.h"

// Maximum size for a level model is 522.5 KiB
#define LEVEL_MODEL_MAX_SIZE 0xA0000 // host layout adds a header delta plus segment/texture tables
#define LEVEL_SEGMENT_MAX 128

#define FLAGS_8002E904 (RENDER_HIDDEN | RENDER_DECAL | RENDER_WATER | RENDER_NO_SHADOW)

/************ .data ************/

//!@bug These two transition effects are marked to not clear when done, meaning they stay active the whole time.
FadeTransition gFullFadeToBlack = FADE_TRANSITION(FADE_FULLSCREEN, FADE_FLAG_OUT, FADE_COLOR_BLACK, 40, FADE_STAY);
FadeTransition gCircleFadeToBlack = FADE_TRANSITION(FADE_CIRCLE, FADE_FLAG_OUT, FADE_COLOR_BLACK, 70, FADE_STAY);

f32 gShadowLightLevels[10] = { 0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f };

Vec3f gFrustumPlanePoints[3][3] = {
    { { { { 50.0f, 0.0f, 32.0f } } }, { { { -50.0f, 0.0f, 32.0f } } }, { { { -50.0f, 100.0f, 32.0f } } } },
    { { { { 0.0f, 0.0f, 32.0f } } }, { { { 130.0f, 60.0f, -68.0f } } }, { { { 130.0f, -60.0f, -68.0f } } } },
    { { { { 0.0f, 0.0f, 32.0f } } }, { { { -130.0f, -60.0f, -68.0f } } }, { { { -130.0f, 60.0f, -68.0f } } } },
};

LevelModel *gCurrentLevelModel = NULL; // Official Name: track
LevelHeader *gCurrentLevelHeader2 = NULL;

#if REGION == REGION_JP
// T.T.カメラ  -  T.T. Camera
char gJpnTTCam[] = { 0x80, 0x2D, 0x80, 0x3C, 0x80, 0x2D, 0x80, 0x3C, 0x80, 0x55, 0x80, 0x71, 0x80, 0x76 };
#endif
u8 *gVoidData = NULL;

u8 gFlashyBgTriIndices[24] = {
    0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4, 5,
    6, 1, 1, 0, 5, 3, 2, 7, 7, 8, 3
    // There may or may not be extra zeroes here.
};

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
unk8011B120 gShadowVertices[32]; // Struct sizeof(0x10) / sizeof(16)
s32 gShadowBucketCounts[4];
unk8011B330 gShadowBucketVertices[120]; // Struct sizeof(0x20) / sizeof(32)
s32 gShadowTriCount;
unk8011C238 gShadowTris[32]; // Struct sizeof(0xC) / sizeof(12)
unk8011C3B8 gShadowWaveCells[64];
unk8011C8B8 gShadowWavePlanes[128];
s32 gShadowWavePlaneCount;
unk8011C8B8 *gShadowCurrentPlane;
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
WaterProperties gWaterSurfaceHits[20];
WaterProperties *gTrackWaves[20];
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
s32 gShadowTail;           // Position in the heap the shadow data ends at.
s32 gNewShadowTriCount;    // xOffset?
s32 gNewShadowVtxCount;    // yOffset?
ptrint_t *gCollisionCandidates; // Allocated 0x7D0 (N64); pointer-sized entries on the host
s8 *gCollisionSurfaces;
s32 gNumCollisionCandidates;
s32 gScenePlayerViewports;
UNUSED f32 gCurrBBoxDistanceToCamera; // Used in a comparison check, but functionally unused.
u32 gWaveBlockCount;
FogData gFogData[4];
Vec3i gScenePerspectivePos;
VoidMesh *gVoidMesh;     // 0x10 bytes struct?
unk8011D478 *gVoidCheckPoints; // 0xC bytes struct?
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
                u32 arg6) {
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
    track_spawn_objects(arg6, 0);
    stubbed_printf("post track_spawn_objects arg6\n");
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
 * The root function for rendering the entire scene.
 * Handles drawing the track, objects and the majority of the HUD in single player.
 * Official name: trackDraw
 */
void render_scene(Gfx **dList, Mtx **mtx, Vertex **vtx, Triangle **tris, s32 updateRate) {
    s32 i;
    s32 numViewports;
    s32 tempUpdateRate;
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
        tempUpdateRate = 0;
    } else {
        tempUpdateRate = updateRate;
    }
    if (gWaveBlockCount) {
        waves_update(tempUpdateRate);
    }
    shadow_update(SHADOW_ACTORS, SHADOW_ACTORS, updateRate);
    for (i = 0; i < 7; i++) {
        if ((s32) gCurrentLevelHeader2->colourCycles[i] != -1) {
            update_colour_cycle(gCurrentLevelHeader2->colourCycles[i], tempUpdateRate);
        }
    }
    if (gCurrentLevelHeader2->pulseLightData != (PulsatingLightData *) -1) {
        update_pulsating_light_data(gCurrentLevelHeader2->pulseLightData, tempUpdateRate);
    }
    gDrawLevelSegments = TRUE;
    if (gCurrentLevelHeader2->race_type == RACETYPE_CUTSCENE_2) {
        gDrawLevelSegments = FALSE;
        gAntiAliasing = TRUE;
    }
    if (gCurrentLevelHeader2->race_type == RACETYPE_CUTSCENE_1 || gCurrentLevelHeader2->forceAntiAliasing) {
        gAntiAliasing = TRUE;
    }
    if (gCurrentLevelHeader2->skyDome == -1) {
        i = (gCurrentLevelHeader2->skyTexture->width << 9) - 1;
        gCurrentLevelHeader2->skyTexScrollU =
            (gCurrentLevelHeader2->skyTexScrollU + (gCurrentLevelHeader2->skyTexScrollSpeedU * tempUpdateRate)) & i;
        i = (gCurrentLevelHeader2->skyTexture->height << 9) - 1;
        gCurrentLevelHeader2->skyTexScrollV =
            (gCurrentLevelHeader2->skyTexScrollV + (gCurrentLevelHeader2->skyTexScrollSpeedV * tempUpdateRate)) & i;
        tex_animate_texture(gCurrentLevelHeader2->skyTexture, &gTrackTexAnimFlags, &gTrackTexAnimOffset, tempUpdateRate);
    }
    flip = FALSE;
    if (get_filtered_cheats() & CHEAT_MIRRORED_TRACKS) {
        flip = TRUE;
    }
#ifdef ANTI_TAMPER
    // Antipiracy measure
    if (IO_READ(0x200) != 0xAC290000) {
        flip = TRUE;
    }
#endif
    rendermode_reset(&gTrackDL);
    gDkrDisableBillboard(gTrackDL++);
    gSPClearGeometryMode(gTrackDL++, G_CULL_FRONT);
    gDPSetBlendColor(gTrackDL++, 0, 0, 0, 100);
    gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(gTrackDL++, 255, 255, 255, 0);
    rain_fog();
    update_fog(numViewports, tempUpdateRate);
    scroll_particle_textures(tempUpdateRate);
    if (gCurrentLevelModel->numberOfAnimatedTextures > 0) {
        track_tex_anim(tempUpdateRate);
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
            weather_update(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, &gTrackTriPtr, tempUpdateRate);
        }
        lensflare_override(cam_get_active_camera());
        lensflare_render(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, cam_get_active_camera());
        hud_render_player(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, get_racer_object_by_port(gSceneCurrentPlayerID),
                          updateRate);
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

/************ .rodata ************/
UNUSED const char gTrackClippingErrorString[] = "Solid Clipping x0=x1 Error!!!\n";
UNUSED const char gTrackHeightOverflowString[] = "TrackGetHeight() - Overflow!!!\n";

/**
 * Allocates control data and geometry for the void.
 * The void refers to the flat coloured background that's shaped in real time to prevent
 * being able to see through level geometry, breaking immersion.
 */
void void_init(s32 viewportCount) {
    s32 i;
    s32 sp30;
    s32 sp2C;
    s32 vtxLimit;
    s32 triLimit;
    u8 *ptr;

    gVoidCheckMaxPoints = 175;
    gVoidPrimLimit = 45;
    // Halve the primitive limit for multiplayer.
    if (viewportCount >= 2) {
        gVoidPrimLimit >>= 1;
    }

    sp30 = (gVoidCheckMaxPoints + 6) * sizeof(unk8011D478);
    sp2C = gVoidCheckMaxPoints + 5;
    vtxLimit = (gVoidPrimLimit + 5) * 4 * sizeof(Vertex);
    triLimit = (gVoidPrimLimit + 5) * 2 * sizeof(Triangle);

    gVoidMesh = mempool_alloc_safe(viewportCount * sizeof(VoidMesh), COLOUR_TAG_CYAN);
    gVoidData = mempool_alloc_safe(sp30 + sp2C + (vtxLimit + triLimit) * 2 * viewportCount, COLOUR_TAG_CYAN);

    ptr = gVoidData;

    if (ptr != NULL) {
        gVoidCheckPoints = (unk8011D478 *) ptr;
        ptr += sp30;

        gVoidCheckPairs = (s8 *) ptr;
        // Align by 8
        ptr = (u8 *) ((uintptr_t) (ptr + sp2C + 8) & ~7);

        for (i = 0; i < viewportCount; i++) {
            gVoidMesh[i].tris[0] = (Triangle *) ptr;
            ptr += triLimit;

            gVoidMesh[i].tris[1] = (Triangle *) ptr;
            ptr += triLimit;

            gVoidMesh[i].verts[0] = (Vertex *) ptr;
            ptr += vtxLimit;

            gVoidMesh[i].verts[1] = (Vertex *) ptr;
            ptr += vtxLimit;
        }
    }
    gVoidVertexFlip = 0;
}

/**
 * Free the void mesh and control data if it exists.
 */
void void_free(void) {
    if (gVoidData != NULL) {
        mempool_free(gVoidMesh);
        mempool_free(gVoidData);
        gVoidData = NULL;
    }
}

// root func for the out of bounds void rendering
void void_check(u8 *segmentIds, s32 numberOfSegments, s32 viewportIndex) {
    s16 i;
    s16 j;
    f32 yCameraSins;
    f32 yCameraCoss;
    f32 temp_f22;
    Vertex *vtx;
    Triangle *tri;
    s16 temp_s3;
    s16 var_s0;
    s16 var_s4;
    s16 breakLoop;
    s32 *ptr2;
    LevelModelSegmentBoundingBox *bbox;
    s16 sum;
    s8 sp7C[24]; // possible UB here, real size is unknown
    s32 pad;

    gVoidTris[0] = gVoidMesh[viewportIndex].tris[0];
    gVoidTris[1] = gVoidMesh[viewportIndex].tris[1];
    gVoidVerts[0] = gVoidMesh[viewportIndex].verts[0];
    gVoidVerts[1] = gVoidMesh[viewportIndex].verts[1];
    material_set_no_tex_offset(&gTrackDL, NULL, RENDER_ANTI_ALIASING | RENDER_Z_COMPARE);
    gVoidCheckPairIndex = 0;
    gVoidCheckPointCount = 0;

    yCameraSins = sins_f(-gSceneActiveCamera->trans.rotation.y_rotation);
    yCameraCoss = coss_f(-gSceneActiveCamera->trans.rotation.y_rotation);

    gVoidCheckAheadX = gSceneActiveCamera->trans.x_position + yCameraSins * 250.0;
    gVoidCheckAheadZ = gSceneActiveCamera->trans.z_position + yCameraCoss * 250.0;

    temp_f22 = -(yCameraSins * gVoidCheckAheadX + yCameraCoss * gVoidCheckAheadZ);

    gVoidCheckSidePlaneX = -yCameraCoss;
    gVoidCheckSidePlaneZ = yCameraSins;
    gVoidCheckSidePlaneD = -(gVoidCheckSidePlaneX * gVoidCheckAheadX + gVoidCheckSidePlaneZ * gVoidCheckAheadZ);

    i = 0;
    for (; i < numberOfSegments; i++) {
        bbox = &gCurrentLevelModel->segmentsBoundingBoxes[segmentIds[i]];
        breakLoop = 0;
        breakLoop += bbox->x1 * yCameraSins + yCameraCoss * bbox->z1 + temp_f22 <= 0.0;
        breakLoop += yCameraSins * bbox->x2 + yCameraCoss * bbox->z1 + temp_f22 <= 0.0;
        breakLoop += bbox->x1 * yCameraSins + yCameraCoss * bbox->z2 + temp_f22 <= 0.0;
        breakLoop += yCameraSins * bbox->x2 + yCameraCoss * bbox->z2 + temp_f22 <= 0.0;
        if (breakLoop & 3) {
            void_check_add_segment_edges(&gCurrentLevelModel->segments[segmentIds[i]], yCameraSins, yCameraCoss, temp_f22);
            if (gCurrentLevelModel->segments[segmentIds[i]].segmentFlags & 2) {
                void_check_add_bbox_edges(bbox, yCameraSins, yCameraCoss, temp_f22);
            }
        }
    }

    void_check_insert_point(300, gCurrentLevelModel->lowerYBounds - 195, 1);
    void_check_insert_point(-300, gCurrentLevelModel->lowerYBounds - 195, 1);
    void_check_insert_point(300, gCurrentLevelModel->upperYBounds + 195, 0);
    void_check_insert_point(-300, gCurrentLevelModel->upperYBounds + 195, 0);

    if (gVoidCheckPointCount >= gVoidCheckMaxPoints || gVoidCheckPointCount == 0) {
        return;
    }

    do {
        breakLoop = TRUE;
        ptr2 = (s32 *) gVoidCheckPoints;
        for (i = 0; i < gVoidCheckPointCount - 1; i++, ptr2 += 2) {
            if (gVoidCheckPoints[i + 1].sideOffset < gVoidCheckPoints[i].sideOffset) {
                s32 tmp;

                tmp = ptr2[0];
                ptr2[0] = ptr2[2];
                ptr2[2] = tmp;

                tmp = ptr2[1];
                ptr2[1] = ptr2[3];
                ptr2[3] = tmp;

                breakLoop = FALSE;
            }
        }
    } while (!breakLoop);

    var_s0 = 0;

    for (i = 0; i < gVoidCheckPointCount; i++) {
        j = gVoidCheckPoints[i].pairIndex * 2;
        if (gVoidCheckPairs[j] == -1) {
            gVoidCheckPoints[i].flags |= 2;
            gVoidCheckPairs[j] = i;
        } else {
            gVoidCheckPairs[j + 1] = i;
        }
    }
    var_s4 = temp_s3 = gVoidCheckPoints[0].sideOffset;

    vtx = gTrackVtxPtr;
    tri = gTrackTriPtr;
    gTrackVtxPtr = gVoidVerts[gVoidVertexFlip];
    gTrackTriPtr = gVoidTris[gVoidVertexFlip];

    gVoidVertexFlip = 1 - gVoidVertexFlip;
    gVoidCurrVerts = gTrackVtxPtr;
    gVoidCurrTris = gTrackTriPtr;
    gVoidVertCount = 0;
    gVoidPrimCount = 0;

    i = 0;
    while (i < gVoidCheckPointCount) {
        while (i < gVoidCheckPointCount && var_s4 == gVoidCheckPoints[i].sideOffset) {
            if (gVoidCheckPoints[i].flags & 2) {
                sp7C[var_s0] = gVoidCheckPoints[i].pairIndex;
                var_s0++;
            } else {
                for (j = 0; j < var_s0; j++) {
                    if (sp7C[j] == gVoidCheckPoints[i].pairIndex) {
                        var_s0--;
                        while (j < var_s0) {
                            sp7C[j] = sp7C[j + 1];
                            j++;
                        }
                    }
                }
            }
            i++;
        }
        if (i < gVoidCheckPointCount) {
            temp_s3 = gVoidCheckPoints[i].sideOffset;
            if (var_s4 != temp_s3) {
                void_check_sort_points(var_s0, sp7C, temp_s3, var_s4);
                var_s4 = temp_s3;
            }
        }
    }
    if (gVoidVertCount != 0) {
        gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(gVoidCurrVerts), gVoidVertCount, 0);
        gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(gVoidCurrTris), gVoidVertCount >> 1, TRIN_DISABLE_TEXTURE);
    }
    gTrackVtxPtr = vtx;
    gTrackTriPtr = tri;
}

/**
 * Void check: adds the edges of a segment bounding box where it crosses the
 * camera plane, for segments that are open to the void.
 */
void void_check_add_bbox_edges(LevelModelSegmentBoundingBox *arg0, f32 arg1, f32 arg2, f32 arg3) {
    f32 sp80[4];
    f32 sp70[4];
    f32 sp60[4];
    f32 temp;
    f32 sp54[2];
    f32 sp4C[2];
    s16 index;
    s16 nextIndex;
    s16 sp40[4];
    s16 var_t0;
    s16 temp2;

    sp80[0] = arg0->x1;
    sp70[0] = arg0->z1;
    sp80[1] = arg0->x2;
    sp70[1] = arg0->z1;
    sp80[2] = arg0->x2;
    sp70[2] = arg0->z2;
    sp80[3] = arg0->x1;
    sp70[3] = arg0->z2;

    temp2 = 0;
    for (index = 0; index < 4; index++) {
        sp60[index] = (arg1 * sp80[index]) + (sp70[index] * arg2) + arg3;
        sp40[index] = sp60[index] <= 0.0;
        temp2 += sp40[index];
    }
    if (temp2 != 0) {
        var_t0 = 0;
        if (temp2 == 4) {
            return;
        }
        for (index = 0; index < 4; index++) {
            nextIndex = index + 1;
            if (nextIndex >= 4) {
                nextIndex = 0;
            }
            if ((sp40[nextIndex] != sp40[index]) != 0U) {
                temp = sp60[index] / (sp60[index] - sp60[nextIndex]);
                sp54[var_t0] = sp80[index] + ((sp80[nextIndex] - sp80[index]) * temp);
                sp4C[var_t0] = sp70[index] + ((sp70[nextIndex] - sp70[index]) * temp);
                var_t0++;
            }
        }
        if (var_t0 != 2) {
            return;
        }

        sp60[0] = (gVoidCheckSidePlaneX * sp54[0]) + (gVoidCheckSidePlaneZ * sp4C[0]) + gVoidCheckSidePlaneD;
        sp60[1] = (gVoidCheckSidePlaneX * sp54[1]) + (gVoidCheckSidePlaneZ * sp4C[1]) + gVoidCheckSidePlaneD;

        if (sp60[1] < sp60[0]) {
            temp = sp60[0];
            sp60[0] = sp60[1];
            sp60[1] = temp;
        }

        // Returns must be on the same line.
        // clang-format off
        if (-300.0 > sp60[1]) { return; }
        if (sp60[0] > 300.0) { return; }
        // clang-format on

        if (sp60[0] < -300.0) {
            sp60[0] = -300.0f;
        }
        if (sp60[1] > 300.0) {
            sp60[1] = 300.0f;
        }
        void_check_insert_point(sp60[1], gCurrentLevelModel->lowerYBounds - 195, 0);
        void_check_insert_point(sp60[0], gCurrentLevelModel->lowerYBounds - 195, 0);
    }
}

/**
 * Void check: walks a segment's triangles and records where their edges cross
 * the camera plane.
 */
void void_check_add_segment_edges(LevelModelSegment *segment, f32 arg1, f32 arg2, f32 arg3) {
    s16 i;
    s16 index;
    s16 verticesOffset;
    s16 nextFaceOffset;
    s16 nextIndex;
    s16 currFaceOffset;
    s16 j;
    Vertex *vert;
    s8 spF8[3];
    f32 temp;
    s16 var_s0;
    s16 var_t0;
#ifdef AVOID_UB
    f32 spE8[3]; // This really should be size of 3, but something is keeping it from matching that way.
#else
    f32 spE8[2];
#endif
    f32 spDC[3];
    f32 spD0[3];
    f32 spC4[3];
    f32 spB8[3];
    f32 spB0[2];
    f32 spA8[2];
    f32 spA0[2];

    if (gVoidCheckPointCount >= gVoidCheckMaxPoints) {
        return;
    }

    for (i = 0; i < segment->numberOfBatches; i++) {
        currFaceOffset = segment->batches[i].facesOffset;
        verticesOffset = segment->batches[i].verticesOffset;
        nextFaceOffset = segment->batches[i + 1].facesOffset;
        if (segment->batches[i].flags & (RENDER_HIDDEN | RENDER_NO_COLLISION)) {
            currFaceOffset = nextFaceOffset;
        }
        for (j = currFaceOffset; j < nextFaceOffset; j++) {
            if ((segment->triangles[j].flags & BACKFACE_DRAW)) {
                continue;
            }
            var_t0 = 0;
            for (index = 0; index < 3; index++) {
                vert = &(segment->triangles[j].verticesArray[index + 1] + verticesOffset)[segment->vertices];
                spE8[index] = vert->x;
                spDC[index] = vert->y;
                spD0[index] = vert->z;
                spC4[index] = (arg1 * spE8[index]) + (arg2 * spD0[index]) + arg3;

                spF8[index] = (spC4[index] <= 0.0);
                var_t0 += (spF8[index] <= 0.0);
            }
            if ((var_t0 == 1) || (var_t0 == 2)) {
                for (var_s0 = 0, index = 0; index < 3; index++) {
                    nextIndex = index + 1;
                    if (nextIndex >= 3) {
                        nextIndex = 0;
                    }
                    if ((spF8[nextIndex] != spF8[index]) != 0) {
                        temp = spC4[index] / (spC4[index] - spC4[nextIndex]);
                        spB0[var_s0] = spE8[index] + ((spE8[nextIndex] - spE8[index]) * temp);
                        spB8[var_s0] = spDC[index] + ((spDC[nextIndex] - spDC[index]) * temp);
                        spA0[var_s0] = spB8[var_s0];
                        spA8[var_s0] = spD0[index] + ((spD0[nextIndex] - spD0[index]) * temp);
                        var_s0++;
                    }
                }

                var_s0 = 0;
                spF8[0] = 0;
                spF8[1] = 0;
                spC4[0] = (gVoidCheckSidePlaneX * spB0[0]) + (gVoidCheckSidePlaneZ * spA8[0]) + gVoidCheckSidePlaneD;
                spC4[1] = (gVoidCheckSidePlaneX * spB0[1]) + (gVoidCheckSidePlaneZ * spA8[1]) + gVoidCheckSidePlaneD;
                if (spC4[0] < -300.0) {
                    spF8[0] = 1;
                }
                if (spC4[0] > 300.0) {
                    spF8[0] |= 2;
                }
                if (spC4[1] < -300.0) {
                    spF8[1] = 1;
                }
                if (spC4[1] > 300.0) {
                    spF8[1] |= 2;
                }
                // clang-format off
                if ((spF8[0] | spF8[1]) == 0) {  var_s0 = 1; }
                // clang-format on
                else if ((spF8[1] != spF8[0]) != 0) {
                    index = 0;
                    if (spC4[1] < spC4[0]) {
                        index = 1;
                    }
                    nextIndex = 1 - index;
                    if (spF8[index] == 1) {
                        temp = ((-spC4[index] - 300.0) / (spC4[nextIndex] - spC4[index]));
                        spB8[index] = spB8[index] + ((spB8[nextIndex] - spB8[index]) * temp);
                        spC4[index] = -300.0f;
                    }
                    if (spF8[nextIndex] == 2) {
                        temp = ((spC4[nextIndex] - 300.0) / (spC4[nextIndex] - spC4[index]));
                        spB8[nextIndex] = spB8[nextIndex] + ((spB8[index] - spB8[nextIndex]) * temp);
                        spC4[nextIndex] = 300.0f;
                    }
                    var_s0 = 1;
                }
                if (var_s0 != 0) {
                    var_t0 = (segment->collisionFacets[j].basePlaneIndex << 2);
                    temp = (spB0[0] + gVoidCheckSidePlaneX) * segment->collisionPlanes[var_t0];
                    temp += spB8[0] * segment->collisionPlanes[var_t0 + 1];
                    temp += (spA8[0] + gVoidCheckSidePlaneZ) * segment->collisionPlanes[var_t0 + 2];
                    temp += segment->collisionPlanes[var_t0 + 3];
                    var_s0 = (temp > 0.0) << 2;
                    if (segment->collisionPlanes[var_t0 + 1] < 0.0f) {
                        var_s0 |= 1;
                    }
                    if (spC4[0] == spC4[1]) {
                        var_s0 |= 8;
                    }
                    void_check_insert_point(spC4[0], spB8[0], var_s0);
                    void_check_insert_point(spC4[1], spB8[1], var_s0);
                }
            }
        }
    }
}

/**
 * Void check: inserts a crossing point into the sorted point list, pairing
 * points as it goes.
 */
void void_check_insert_point(s16 arg0, s16 arg1, s32 arg2) {
    s16 i;
    s16 j;

    if (gVoidCheckPointCount < gVoidCheckMaxPoints) {
        i = 0;
        while (i < gVoidCheckPointCount && gVoidCheckPoints[i].sideOffset < arg0) {
            i++;
        }
        while (i < gVoidCheckPointCount && arg0 == gVoidCheckPoints[i].sideOffset && gVoidCheckPoints[i].height < arg1) {
            i++;
        }
        j = gVoidCheckPointCount;
        while (i < j) {
            gVoidCheckPoints[j].sideOffset = gVoidCheckPoints[j - 1].sideOffset;
            gVoidCheckPoints[j].height = gVoidCheckPoints[j - 1].height;
            gVoidCheckPoints[j].pairIndex = gVoidCheckPoints[j - 1].pairIndex;
            gVoidCheckPoints[j].flags = gVoidCheckPoints[j - 1].flags;
            j--;
        }
        gVoidCheckPoints[i].sideOffset = arg0;
        gVoidCheckPoints[i].height = arg1;
        gVoidCheckPoints[i].unused4 = 0;
        gVoidCheckPoints[i].pairIndex = gVoidCheckPairIndex;
        gVoidCheckPoints[i].flags = arg2;
        gVoidCheckPairs[gVoidCheckPointCount] = -1;
        if (gVoidCheckPointCount & 1) {
            gVoidCheckPairIndex++;
        }
        gVoidCheckPointCount++;
    }
}

/**
 * Void check: orders the paired crossing points so the spans between them can
 * be tested against the camera.
 */
void void_check_sort_points(s16 arg0, s8 *arg1, f32 arg2, f32 arg3) {
    unk8011D478 *next;
    unk8011D478 *curr;
    s16 temp3;
    s16 temp4;
    f32 curr_unk0;
    f32 curr_unk2;
    f32 next_unk0;
    f32 next_unk2;
    s32 noSwap;
    s16 i;
    s16 j;
    f32 sp94[20];
    f32 sp6C[10];
    s8 sp60[10];
    s8 temp;
    s8 temp0;
    s8 temp1;
    s8 swapByte;

    if (arg0 >= 10 || arg0 == 0) {
        return;
    }

    for (j = 0, i = 0; i < arg0;) {
        temp = arg1[i];
        curr = &gVoidCheckPoints[gVoidCheckPairs[(s16) (temp * 2)]];
        next = &gVoidCheckPoints[gVoidCheckPairs[((s16) (temp * 2)) + 1]];
        curr_unk0 = (f32) curr->sideOffset;
        curr_unk2 = (f32) curr->height;
        next_unk0 = (f32) next->sideOffset;
        next_unk2 = (f32) next->height;
        if (curr_unk0 == next_unk0) {
            return;
        }
        sp94[j++] = ((next_unk2 - curr_unk2) * ((curr_unk0 - arg3) / ((curr_unk0 - next_unk0)))) + curr_unk2;
        sp6C[i] = sp94[j - 1];
        sp60[i] = i;
        sp94[j++] = ((curr_unk2 - next_unk2) * ((arg2 - next_unk0) / ((curr_unk0 - next_unk0)))) + next_unk2;
        sp6C[i] += sp94[j - 1];
        i++;
    }

    do {
        noSwap = TRUE;
        for (i = 0; i < arg0 - 1; i++) {
            if (sp6C[sp60[i + 1]] < sp6C[sp60[i]]) {
                swapByte = arg1[i];
                arg1[i] = arg1[i + 1];
                arg1[i + 1] = swapByte;
                swapByte = sp60[i];
                sp60[i] = sp60[i + 1];
                sp60[i + 1] = swapByte;
                noSwap = FALSE;
            }
        }
    } while (noSwap == FALSE);

    for (i = 0; i < arg0 - 1; i++) {
        temp0 = gVoidCheckPoints[gVoidCheckPairs[arg1[i] * 2]].flags & 1;
        temp1 = gVoidCheckPoints[gVoidCheckPairs[arg1[i + 1] * 2]].flags & 1;
        if (temp0 && !temp1) {
            temp3 = (sp60[i] * 2);
            temp4 = (sp60[i + 1] * 2);
            void_generate_primitive(&sp94[temp3], &sp94[temp4], arg3, arg2);
        }
    }
}

s32 void_generate_primitive(f32 *arg0, f32 *arg1, f32 arg2, f32 arg3) {
    Vertex *verts;
    Triangle *tris;
    s32 triIndex;
    s16 vertZ1;
    s16 vertX2;
    s16 vertZ2;
    s16 vertX1;
    u8 colour_r;
    u8 colour_g;
    u8 colour_b;
    u8 colour_a;

    if (gVoidPrimCount >= gVoidPrimLimit) {
        return NULL;
    }

    if (gVoidVertCount == 24) {
        gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(gVoidCurrVerts), gVoidVertCount, 0);
        gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(gVoidCurrTris), (gVoidVertCount >> 1), TRIN_DISABLE_TEXTURE);
        gVoidCurrVerts = gTrackVtxPtr;
        gVoidVertCount = 0;
        gVoidCurrTris = gTrackTriPtr;
    }

    vertX1 = arg2 * gVoidCheckSidePlaneX + gVoidCheckAheadX;
    vertZ1 = arg2 * gVoidCheckSidePlaneZ + gVoidCheckAheadZ;
    vertX2 = arg3 * gVoidCheckSidePlaneX + gVoidCheckAheadX;
    vertZ2 = arg3 * gVoidCheckSidePlaneZ + gVoidCheckAheadZ;

    colour_r = gVoidColourR;
    colour_g = gVoidColourG;
    colour_b = gVoidColourB;
    colour_a = 0xFF;

    verts = gTrackVtxPtr;
    verts[0].x = vertX1;
    verts[0].y = arg0[0] + 2.0f;
    verts[0].z = vertZ1;
    verts[0].r = colour_r;
    verts[0].g = colour_g;
    verts[0].b = colour_b;
    verts[0].a = colour_a;

    verts[1].x = vertX2;
    verts[1].y = arg0[1] + 2.0f;
    verts[1].z = vertZ2;
    verts[1].r = colour_r;
    verts[1].g = colour_g;
    verts[1].b = colour_b;
    verts[1].a = colour_a;

    verts[2].x = vertX1;
    verts[2].y = arg1[0] - 2.0f;
    verts[2].z = vertZ1;
    verts[2].r = colour_r;
    verts[2].g = colour_g;
    verts[2].b = colour_b;
    verts[2].a = colour_a;

    verts[3].x = vertX2;
    verts[3].y = arg1[1] - 2.0f;
    verts[3].z = vertZ2;
    verts[3].r = colour_r;
    verts[3].g = colour_g;
    verts[3].b = colour_b;
    verts[3].a = colour_a;
    // @fake
    if (1) {}
    verts += 4;
    gTrackVtxPtr = verts;

    tris = gTrackTriPtr;
    triIndex = (u8) gVoidVertCount;

    // @fake, using index 0 directly doesn't match
    vertX2 = 0;
    tris[vertX2].flags = BACKFACE_DRAW;
    tris[vertX2].vi0 = triIndex + 2;
    tris[vertX2].vi1 = triIndex + 1;
    tris[vertX2].vi2 = triIndex;
    tris[vertX2].uv0.u = 1024 - 32;
    tris[vertX2].uv0.v = 1024 - 32;
    tris[vertX2].uv1.u = 1024 - 32;
    tris[vertX2].uv1.v = 0;
    tris[vertX2].uv2.u = 1;
    tris[vertX2].uv2.v = 0;

    tris[1].flags = BACKFACE_DRAW;
    tris[1].vi0 = triIndex + 3;
    tris[1].vi1 = triIndex + 1;
    tris[1].vi2 = triIndex + 2;
    tris[1].uv0.u = 1;
    tris[1].uv0.v = 1024 - 32;
    tris[1].uv1.u = 1024 - 32;
    tris[1].uv1.v = 1024 - 32;
    tris[1].uv2.u = 1;
    tris[1].uv2.v = 0;
    tris += 2;
    gTrackTriPtr = tris;

    gVoidVertCount += 4;
    gVoidPrimCount++;
    return NULL;
}

/**
 * Whether a track wall lies between the current viewport's racer and the
 * camera, by casting from the racer to the camera through the collision
 * candidates. Used to pull the camera in front of walls.
 */
s32 is_camera_behind_wall(void) {
    LevelModelSegment *segment; // spE4
    s32 ret;
    s32 var_t4;
    f32 playerDist;
    f32 camDist;
    f32 scalingFactor;
    s32 curViewport;
    ptrint_t flipSide;
    s32 numRacers; // spC4
    s32 i;
    s32 j;
    f32 var_f18;
    f32 A1, B1, C1, D1;
    f32 var_f20;
    f32 var_f22;
    f32 var_f24;
    CollisionNode *colNode;
    Object_Racer *racer;
    f32 A, B, C, D;
    Object **racerObjects; // sp80
    Object *racerObj;      // sp7C
    f32 *planes;

    racerObjects = get_racer_objects(&numRacers);
    if (numRacers == 0) {
        return FALSE;
    }
    if ((check_if_showing_cutscene_camera() != 0) || (gSceneActiveCamera->mode >= 5) ||
        (gSceneActiveCamera->mode == 3)) {
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
    for (var_t4 = 0; var_t4 < gNumCollisionCandidates && ret == FALSE; var_t4++) {
        flipSide = gCollisionCandidates[var_t4];
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
                    var_f20 = (gSceneActiveCamera->trans.x_position - racerObj->trans.x_position);
                    var_f22 = (gSceneActiveCamera->trans.y_position - racerObj->trans.y_position);
                    var_f24 = (gSceneActiveCamera->trans.z_position - racerObj->trans.z_position);

                    if (playerDist != camDist) {
                        scalingFactor = playerDist / (playerDist - camDist);
                    } else {
                        scalingFactor = 0.0f;
                    }

                    var_f20 = racerObj->trans.x_position + (var_f20 * scalingFactor);
                    var_f22 = racerObj->trans.y_position + (var_f22 * scalingFactor);
                    var_f24 = racerObj->trans.z_position + (var_f24 * scalingFactor);

                    for (j = 0, ret = TRUE; j < 3 && ret == TRUE; j++) {
                        flipSide = FALSE;
                        curViewport = colNode->closestTri[j];
                        if (curViewport & 0x8000) {
                            curViewport &= 0x7FFF;
                            flipSide = TRUE;
                        }
                        curViewport = curViewport << 2;
                        planes = &segment->collisionPlanes[curViewport];
                        A1 = planes[0];
                        B1 = planes[1];
                        C1 = planes[2];
                        D1 = planes[3];
                        var_f18 = A1 * var_f20 + B1 * var_f22 + C1 * var_f24 + D1;
                        if (flipSide) {
                            var_f18 = -var_f18;
                        }
                        if (var_f18 > 4.0f) {
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

/**
 * Handle the flipbook effect for level geometry textures.
 */
void track_tex_anim(s32 updateRate) {
    s32 segmentNumber, batchNumber;
    LevelModelSegment *segment;
    TextureHeader *texture;
    TriangleBatchInfo *batch;
    s32 temp;

    segment = gCurrentLevelModel->segments;
    for (segmentNumber = 0; segmentNumber < gCurrentLevelModel->numberOfSegments; segmentNumber++) {
        batch = segment[segmentNumber].batches;
        for (batchNumber = 0; batchNumber < segment[segmentNumber].numberOfBatches; batchNumber++) {
            if (batch[batchNumber].flags & RENDER_TEX_ANIM) {
                if (batch[batchNumber].textureIndex != 0xFF) {
                    texture = gCurrentLevelModel->textures[batch[batchNumber].textureIndex].texture;
                    if (texture->numOfTextures != 0x100 && texture->frameAdvanceDelay) {
                        temp = batch[batchNumber].texOffset << 6;
                        if (batch[batchNumber].flags & RENDER_UNK_80000000) {
                            temp |= batch[batchNumber].miscData;
                            tex_animate_texture(texture, &batch[batchNumber].flags, &temp, updateRate);
                            batch[batchNumber].miscData = temp & 0x3F;
                        } else {
                            tex_animate_texture(texture, &batch[batchNumber].flags, &temp, updateRate);
                        }
                        batch[batchNumber].texOffset = (temp >> 6) & 0xFF;
                    }
                }
            }
        }
    }
}

/**
 * Spawns and initialises the skydome object seen ingame.
 * Skipped if the object ID doesn't exist.
 * Also compares a checksum which can potentially trigger anti-tamper measures.
 * Official name: initSky
 */
void skydome_spawn(s32 objectID) {
    LevelObjectEntryCommon spawnObject;

#ifdef ANTI_TAMPER
    // Antipiracy measure
    drm_checksum_balloon();
#endif
    if (objectID == -1) {
        gSkydomeSegment = NULL;
        return;
    }
    spawnObject.x = 0;
    spawnObject.y = 0;
    spawnObject.z = 0;
    spawnObject.size = sizeof(LevelObjectEntryCommon);
    spawnObject.objectID = objectID;
    gSkydomeSegment = spawn_object(&spawnObject, OBJECT_SPAWN_UNK02);
    if (gSkydomeSegment != NULL) {
        gSkydomeSegment->level_entry = NULL;
        gSkydomeSegment->objectID = -1;
    }
}

/**
 * Sets the status to render the sky.
 * If set to false, will skip the background and skydome rendering.
 * Official name: trackSkySet
 */
void set_skydome_visbility(s32 renderSky) {
    gSceneRenderSkyDome = renderSky;
}

// This function creates the flashy sky effect in the wizpig 2 race.
void trackbg_render_flashy(void) {
    Triangle *tris;
    Vertex *verts;
    s32 vCoordMask;
    s32 uCoordMask;
    f32 scaledXSin;
    f32 scaledXCos;
    f32 var_f16;
    s16 uCoords[9];
    s16 vCoords[9];
    f32 xCos;
    f32 xSin;
    f32 pad_sp108;
    Camera *camera;
    f32 pad_sp100;
    f32 xPositions[9];
    f32 zPositions[9];
    Vec3f pos;
    s32 i;
    s32 var_v0;
    s32 var_v1;
    s32 var_a1;
    s32 var_a2;
    s32 var_a3;
    u8 *var_v0_3;
    f32 var_f14;
    s16 vertY;
    s16 vTempCoord;
    s16 uTempCoord;
    LevelHeader_70 *pad2;
    LevelHeader_70 *var_t2;
    LevelHeader_70 *levelHeader;
    TextureHeader *texHeader;
    s32 pad[4];

    verts = gTrackVtxPtr;
    tris = gTrackTriPtr;

    camera = cam_get_active_camera();
    texHeader = gCurrentLevelHeader2->skyTexture;
    uCoordMask = (texHeader->width << 5) - 1;
    vCoordMask = (texHeader->height << 5) - 1;
    xSin = sins_f(-camera->trans.rotation.x);
    xCos = coss_f(-camera->trans.rotation.x);

    scaledXSin = xSin * 1280.0f;
    scaledXCos = xCos * 1280.0f;
    pad_sp100 = 2.0f * scaledXSin;
    xPositions[0] = -scaledXCos - (xSin * 1280.0f);
    zPositions[0] = -scaledXCos + (xSin * 1280.0f);
    xPositions[1] = scaledXCos - (xSin * 1280.0f);
    zPositions[1] = -scaledXCos - (xSin * 1280.0f);
    xPositions[2] = scaledXCos + scaledXSin;
    zPositions[2] = scaledXCos - (xSin * 1280.0f);
    xPositions[3] = -scaledXCos + (xSin * 1280.0f);
    zPositions[3] = scaledXCos + (xSin * 1280.0f);
    xPositions[4] = 0.0f;
    zPositions[4] = 0.0f;

    xPositions[5] = -(xCos * 1280.0f) - (2.0f * scaledXSin);
    zPositions[5] = scaledXSin + -(2.0f * (xCos * 1280.0f));
    xPositions[6] = (xCos * 1280.0f) - (2.0f * scaledXSin);
    zPositions[6] = -(2.0f * (xCos * 1280.0f)) - scaledXSin;
    xPositions[7] = (xCos * 1280.0f) + (2.0f * scaledXSin);
    zPositions[7] = (2.0f * (xCos * 1280.0f)) - scaledXSin;
    xPositions[8] = -(xCos * 1280.0f) + (2.0f * scaledXSin);
    zPositions[8] = (2.0f * (xCos * 1280.0f)) + scaledXSin;

    scaledXCos = 1280.0f;
    var_f14 = scaledXCos * 0.25f;

    var_a1 = texHeader->width * 16 * gCurrentLevelHeader2->skyTexScaleU;
    var_a2 = texHeader->height * 16 * gCurrentLevelHeader2->skyTexScaleV;

    var_v0 = ((s32) (camera->trans.x_position * ((scaledXCos * 0.25f) / var_a1)) + (gCurrentLevelHeader2->skyTexScrollU >> 4)) &
             uCoordMask;
    var_v1 = ((s32) (camera->trans.z_position * ((scaledXCos * 0.25f) / var_a2)) + (gCurrentLevelHeader2->skyTexScrollV >> 4)) &
             vCoordMask;

    var_f14 = var_a1 * xCos;
    pos.z = var_a1 * xCos;
    pos.x = var_a1 * xCos;
    var_f16 = var_a2 * xSin;
    xCos = var_f16;
    pad_sp108 = var_f16;

    // @fake
    var_a2 = texHeader->height * 16 * gCurrentLevelHeader2->skyTexScaleV;

    uCoords[0] = (s16) (-var_f14 - pad_sp108) + var_v0;
    vCoords[0] = (s16) (var_f16 - var_f14) + var_v1;
    uCoords[1] = (s16) (var_f14 - pad_sp108) + var_v0;
    vCoords[1] = (s16) (-var_f14 - var_f16) + var_v1;
    uCoords[2] = (s16) (var_f14 + var_f16) + var_v0;
    vCoords[2] = (s16) (var_f14 - var_f16) + var_v1;
    uCoords[3] = (s16) (var_f16 - var_f14) + var_v0;
    vCoords[3] = (s16) (var_f14 + var_f16) + var_v1;

    uCoords[4] = var_v0;
    vCoords[4] = var_v1;

    uCoords[5] = (s16) (-var_f14 - (2.0f * xCos)) + var_v0;
    vCoords[5] = (s16) (var_f16 - (2.0f * var_f14)) + var_v1;
    uCoords[6] = (s16) (var_f14 - (2.0f * xCos)) + var_v0;
    vCoords[6] = (s16) ((-(2.0f * var_f14)) - var_f16) + var_v1;
    uCoords[7] = (s16) (pos.f[2] + (2.0f * xCos)) + var_v0;
    vCoords[7] = (s16) ((2.0f * pos.x) - var_f16) + var_v1;
    uCoords[8] = (s16) ((2.0f * xCos) - pos.z) + var_v0;
    vCoords[8] = (s16) ((2.0f * pos.x) + var_f16) + var_v1;

    mtx_world_origin(&gTrackDL, &gTrackMtxPtr);

    var_t2 = *gCurrentLevelHeader2->colourCycles;
    var_a2 = -1;

    if ((u32) var_t2 != -1U) {
        levelHeader = gCurrentLevelHeader2->colourCycles[1];
        if ((u32) levelHeader == -1U) {
            levelHeader = var_t2;
        }
    } else {
        var_t2 = NULL;
    }

    var_a3 = COLOUR_RGBA32(255, 255, 255, 0);
    if (var_t2 != NULL) {
        var_a2 = var_t2->rgba.word;
        var_a3 = levelHeader->rgba.word & (~0xFF);
    }

    gfx_init_basic_xlu(&gTrackDL, 1, var_a2, var_a3);
    texHeader = set_animated_texture_header(texHeader, gTrackTexAnimOffset << 8);
    gDkrDmaDisplayList(gTrackDL++, OS_K0_TO_PHYSICAL(texHeader->cmd), texHeader->numberOfCommands);
    gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(gTrackVtxPtr), 9, 0);
    gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(gTrackTriPtr), 8, 1);
    gDPPipeSync(gTrackDL++);
    if (var_t2 != NULL) {
        gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(gTrackDL++, 255, 255, 255, 0);
    }
    rendermode_reset(&gTrackDL);

    vertY = camera->trans.y_position + 192.0f;
    for (i = 0; i < 9; i++) {
        verts->x = xPositions[i] + camera->trans.x_position;
        verts->y = vertY;
        verts->z = zPositions[i] + camera->trans.z_position;
        verts->r = 0xFF;
        verts->g = 0xFF;
        verts->b = 0xFF;
        verts->a = (i <= 4) ? (255) : (0);
        verts++;
    }

    var_v0_3 = gFlashyBgTriIndices;
    for (i = 0; i < 8; i++) {
        tris->flags = BACKFACE_DRAW;
        tris->vi0 = *var_v0_3;
        tris->uv0.u = uCoords[*var_v0_3];
        tris->uv0.v = vCoords[*var_v0_3];
        var_v0_3 += 1;
        tris->vi1 = *var_v0_3;
        tris->uv1.u = uCoords[*var_v0_3];
        tris->uv1.v = vCoords[*var_v0_3];
        var_v0_3 += 1;
        tris->vi2 = *var_v0_3;
        tris->uv2.u = uCoords[*var_v0_3];
        tris->uv2.v = vCoords[*var_v0_3];
        var_v0_3 += 1;
        tris++;
    }

    gTrackVtxPtr = verts;
    gTrackTriPtr = tris;
}

/**
 * Instead of drawing the skydome with textures, draw a solid coloured background.
 * Using different colours set in the level header, the vertices are coloured and
 * it gives the background a gradient effect.
 */
void trackbg_render_gradient(void) {
    s16 z;
    UNUSED s16 pad;
    s16 y0;
    s16 y1;
    u8 headerRed0;
    u8 headerGreen0;
    u8 headerBlue0;
    u8 headerRed1;
    u8 headerGreen1;
    u8 headerBlue1;
    Vertex *verts;
    Triangle *tris;

    verts = (Vertex *) gTrackVtxPtr;
    tris = (Triangle *) gTrackTriPtr;

    headerRed0 = gCurrentLevelHeader2->BGColourTopR;
    headerGreen0 = gCurrentLevelHeader2->BGColourTopG;
    headerBlue0 = gCurrentLevelHeader2->BGColourTopB;
    headerRed1 = gCurrentLevelHeader2->BGColourBottomR;
    headerGreen1 = gCurrentLevelHeader2->BGColourBottomG;
    headerBlue1 = gCurrentLevelHeader2->BGColourBottomB;
    rendermode_reset(&gTrackDL);
    material_set_no_tex_offset(&gTrackDL, NULL, RENDER_FOG_ACTIVE);
    gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(verts), 4, 0);
    gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(tris), 2, 0);

    z = 20;
    if (osTvType == OS_TV_TYPE_PAL) {
        y0 = -180;
        y1 = 180;
    } else {
        y0 = -150;
        y1 = 150;
    }
    if (cam_get_viewport_layout() == TWO_PLAYERS) {
        y0 >>= 1;
        y1 >>= 1;
    }
    verts->x = -200;
    verts->y = y0;
    verts->z = z;
    verts->r = headerRed0;
    verts->g = headerGreen0;
    verts->b = headerBlue0;
    verts->a = 255;
    verts++;

    verts->x = 200;
    verts->y = y0;
    verts->z = z;
    verts->r = headerRed0;
    verts->g = headerGreen0;
    verts->b = headerBlue0;
    verts->a = 255;
    verts++;

    verts->x = -200;
    verts->y = y1;
    verts->z = z;
    verts->r = headerRed1;
    verts->g = headerGreen1;
    verts->b = headerBlue1;
    verts->a = 255;
    verts++;

    verts->x = 200;
    verts->y = y1;
    verts->z = z;
    verts->r = headerRed1;
    verts->g = headerGreen1;
    verts->b = headerBlue1;
    verts->a = 255;
    verts++;

    tris->flags = BACKFACE_DRAW;
    tris->vi0 = 2;
    tris->vi1 = 1;
    tris->vi2 = 0;
    tris->uv0.u = 0;
    tris->uv0.v = 0;
    tris->uv1.u = 0;
    tris->uv1.v = 0;
    tris->uv2.u = 0;
    tris->uv2.v = 0;
    tris++;

    tris->flags = BACKFACE_DRAW;
    tris->vi0 = 3;
    tris->vi1 = 2;
    tris->vi2 = 1;
    tris->uv0.u = 0;
    tris->uv0.v = 0;
    tris->uv1.u = 0;
    tris->uv1.v = 0;
    tris->uv2.u = 0;
    tris->uv2.v = 0;
    tris++;

    gTrackVtxPtr = verts;
    gTrackTriPtr = tris;
}

/**
 * Sets the position to the current camera's position then renders the skydome if set to be visible.
 */
void skydome_render(void) {
    Camera *cam;
    if (gSkydomeSegment == NULL) {
        return;
    }

    cam = cam_get_active_camera();
    if (gCurrentLevelHeader2->skyDome == 0) {
        gSkydomeSegment->trans.x_position = cam->trans.x_position;
        gSkydomeSegment->trans.y_position = cam->trans.y_position;
        gSkydomeSegment->trans.z_position = cam->trans.z_position;
    }

    mtx_world_origin(&gTrackDL, &gTrackMtxPtr);
    if (gSceneRenderSkyDome) {
        render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, gSkydomeSegment);
    }
}

/**
 * Sets up all of the required variables for the player's view perspective.
 * This includes setting up the camera index, viewport and
 */
void initialise_player_viewport_vars(s32 updateRate) {
    s32 i;
    s32 numRacers;
    s32 viewportID;
    Object **racers;
    s32 segmentIndex;
    Object_Racer *racer;

    gSceneActiveCamera = cam_get_active_camera();
    viewportID = get_current_viewport();
    compute_scene_camera_transform_matrix();
    update_envmap_position(gScenePerspectivePos.x / 65536.0f, gScenePerspectivePos.y / 65536.0f,
                           gScenePerspectivePos.z / 65536.0f);
    segmentIndex = gSceneActiveCamera->cameraSegmentID;
    if (segmentIndex > -1 && (segmentIndex < gCurrentLevelModel->numberOfSegments)) {
        gSceneStartSegment = gCurrentLevelModel->segments[segmentIndex].visibilityBitfieldOffset;
    } else {
        gSceneStartSegment = -1;
    }
    gPrevCameraX = gSceneActiveCamera->trans.x_position;
    gPrevCameraY = gSceneActiveCamera->trans.y_position;
    gPrevCameraZ = gSceneActiveCamera->trans.z_position;
    if (gWaveBlockCount != 0) {
        waves_visibility_reset();
        racers = get_racer_objects(&numRacers);
        if (gSceneActiveCamera->mode != CAMERA_FINISH_RACE && numRacers > 0 && !check_if_showing_cutscene_camera()) {
            i = -1;
            do {
                i++;
                racer = racers[i]->racer;
            } while (i < numRacers - 1 && viewportID != racer->playerIndex);
            waves_visibility(racers[i]->trans.x_position, racers[i]->trans.y_position, racers[i]->trans.z_position,
                             get_current_viewport(), updateRate);
        } else {
            waves_visibility(gSceneActiveCamera->trans.x_position, gSceneActiveCamera->trans.y_position,
                             gSceneActiveCamera->trans.z_position, get_current_viewport(), updateRate);
        }
    }
    level_header()->levelRendered = 1;
    render_level_geometry_and_objects();
}

/**
 * Enable or disable anti aliasing.
 * Improves visual quality at the cost of performance.
 */
void set_anti_aliasing(s32 setting) {
    gAntiAliasing = setting;
}

/**
 * Find which segments can and should be rendered, then render their opaque geometry.
 * Render all objects inside visible segments then render the level's semitransparent geometry.
 * Afterwards, render particles.
 */
void render_level_geometry_and_objects(void) {
    s32 objCount;
    s32 numberOfSegments;
    s32 objFlags;
    s32 sp160;
    s32 i;
    s32 visibleFlags;
    u8 segmentIds[LEVEL_SEGMENT_MAX];
    u8 objectsVisible[LEVEL_SEGMENT_MAX];
    s32 visible;
    Object *obj;

    deferred_dl_reset();

    if (get_settings()->courseId == ASSET_LEVEL_OPENINGSEQUENCE) {
        gAntiAliasing = TRUE;
    }

    sp160 = get_first_active_object(&objCount);

    if (gCurrentLevelModel->numberOfSegments > 1) {
        numberOfSegments = 0;
        traverse_segments_bsp_tree(0, 0, gCurrentLevelModel->numberOfSegments - 1, segmentIds, &numberOfSegments);
    } else {
        numberOfSegments = 1;
        segmentIds[0] = 0;
    }

    for (i = 1; i <= gCurrentLevelModel->numberOfSegments; i++) {
        objectsVisible[i] = FALSE; // why not a bzero?
    }

    objectsVisible[0] = TRUE;

    if (gDrawLevelSegments) {
        for (i = 0; i < numberOfSegments; i++) {
            render_level_segment(segmentIds[i], FALSE); // Render opaque segments
            objectsVisible[segmentIds[i] + 1] = TRUE;
        }
    }

    if (gCurrentLevelModel->numberOfSegments < 2) {
        objectsVisible[1] = TRUE;
    }

    rendermode_reset(&gTrackDL);
    sort_objects_by_dist(sp160, objCount - 1);
    visibleFlags = OBJ_FLAGS_INVIS_PLAYER1 << (get_current_viewport() & 1);

    for (i = sp160; i < objCount; i++) {
        obj = get_object(i);
        visible = 255;
        objFlags = obj->trans.flags;
        if (objFlags & OBJ_FLAGS_UNK_0080) {
            visible = 0;
        } else if (!(objFlags & OBJ_FLAGS_PARTICLE)) {
            visible = obj->opacity;
        }
        if (objFlags & visibleFlags) {
            visible = 0;
        }
        if (obj != NULL && visible == 255 && check_if_in_draw_range(obj) &&
            (objectsVisible[obj->segmentID + 1] || obj->cullRadius > 1000.0)) {
            if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
                render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                continue;
            } else if (obj->shadow != NULL) {
                shadow_render(obj, obj->shadow);
            }
            render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
            if (obj->waterEffect != NULL && obj->header->flags & HEADER_FLAGS_WATER_EFFECT) {
                watereffect_render(obj, obj->waterEffect);
            }
        }
    }

    for (i = objCount - 1; i >= sp160; i--) {
        obj = get_object(i);
        objFlags = obj->trans.flags;
        if (objFlags & visibleFlags) {
            visible = FALSE;
        } else {
            visible = TRUE;
        }
        if (obj != NULL && visible && objFlags & OBJ_FLAGS_UNK_0100 && objectsVisible[obj->segmentID + 1] &&
            check_if_in_draw_range(obj)) {
            if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
                render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                continue;
            } else if (obj->shadow != NULL) {
                shadow_render(obj, obj->shadow);
            }
            render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
            if (obj->waterEffect != NULL && obj->header->flags & HEADER_FLAGS_WATER_EFFECT) {
                watereffect_render(obj, obj->waterEffect);
            }
        }
    }

    if (gDrawLevelSegments) {
        for (i = numberOfSegments - 1; i >= 0; i--) {
            render_level_segment(segmentIds[i], TRUE); // Render transparent segments
        }
    }

    if (gWaveBlockCount != 0) {
        waves_render(&gTrackDL, &gTrackMtxPtr, get_current_viewport());
    }

    rendermode_reset(&gTrackDL);
    material_set_no_tex_offset(&gTrackDL, NULL, RENDER_FOG_ACTIVE | RENDER_Z_COMPARE);
    deferred_dl_flush(&gTrackDL);

    // Particles and FX
    for (i = objCount - 1; i >= sp160; i--) {
        obj = get_object(i);
        visible = 255;
        objFlags = obj->trans.flags;
        if (objFlags & OBJ_FLAGS_UNK_0080) {
            visible = 1;
        } else if (!(objFlags & OBJ_FLAGS_PARTICLE)) {
            visible = obj->opacity;
        }
        if (objFlags & visibleFlags) {
            visible = 0;
        }
        if (obj->behaviorId == BHV_RACER && visible >= 255) {
            visible = 0;
        }
        if (obj != NULL && visible < 255 && objectsVisible[obj->segmentID + 1] && check_if_in_draw_range(obj)) {
            if (visible > 0) {
                if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
                    render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                    goto skip;
                } else if (obj->shadow != NULL) {
                    shadow_render(obj, obj->shadow);
                }
                render_object(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                if ((obj->waterEffect != 0) && (obj->header->flags & HEADER_FLAGS_WATER_EFFECT)) {
                    watereffect_render(obj, obj->waterEffect);
                }
            }
        skip:
            if (obj->behaviorId == BHV_RACER) {
                render_racer_shield(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
                render_racer_magnet(&gTrackDL, &gTrackMtxPtr, &gTrackVtxPtr, obj);
            }
        }
    }

    if (gVoidData != NULL && is_camera_behind_wall()) {
        void_check(segmentIds, numberOfSegments, get_current_viewport());
    }
    gAntiAliasing = FALSE;
}

/**
 * Render a batch of level geometry.
 * Since opaque and transparent are done in two separate runs, it will skip over the other.
 * Has a special case for the flashing lights in Spaceport Alpha, too.
 */
void render_level_segment(s32 segmentId, s32 nonOpaque) {
    LevelModelSegment *segment;
    s32 i;
    TriangleBatchInfo *batchInfo;
    TextureHeader *texture;
    s32 renderBatch;
    s32 numberVertices;
    s32 numberTriangles;
    ptrint_t vertices;
    ptrint_t triangles;
    s32 color;
    s32 isInvisible;
    s32 levelHeaderIndex;
    s32 texOffset;
    s32 sp78;
    s32 startPos;
    s32 endPos;
    s32 batchFlags;
    s32 textureFlags;
    //! @bug: batchInfo is uninitalized (the result is recomputed below; skipped on the host)
    segment = &gCurrentLevelModel->segments[segmentId];
    sp78 = (nonOpaque && gWaveBlockCount) ? waves_block_hq(segment) : 0;
    if (nonOpaque) {
        startPos = segment->numberofOpaqueBatches;
        endPos = segment->numberOfBatches;
    } else {
        startPos = 0;
        endPos = segment->numberofOpaqueBatches;
    }
    for (i = startPos; i < endPos; i++) {
        batchInfo = &segment->batches[i];
        textureFlags = RENDER_NONE;
        isInvisible = batchInfo->flags & RENDER_HIDDEN;
        if (isInvisible) {
            continue;
        }
        batchFlags = batchInfo->flags;
        renderBatch = 0;
        if (batchInfo->textureIndex == 0xFF) {
            texture = FALSE;
        } else {
            texture = gCurrentLevelModel->textures[batchInfo->textureIndex].texture;
            textureFlags = texture->flags;
        }
        batchFlags |= RENDER_FOG_ACTIVE | RENDER_Z_COMPARE;
        if (!(batchFlags & RENDER_CUTOUT) && !(batchFlags & RENDER_DECAL)) {
            batchFlags |= gAntiAliasing;
        }
        if ((!(textureFlags & RENDER_SEMI_TRANSPARENT) && !(batchFlags & RENDER_WATER)) || batchFlags & RENDER_DECAL) {
            renderBatch = TRUE;
        }
        if (nonOpaque) {
            renderBatch = (renderBatch + 1) & 1;
        }
        if (sp78 && batchFlags & RENDER_WATER) {
            renderBatch = FALSE;
        }
        if (!renderBatch) {
            continue;
        }
        numberTriangles = batchInfo->facesOffset;
        do { // Fakematch
            numberVertices = (batchInfo + 1)->verticesOffset - batchInfo->verticesOffset;
            numberTriangles = (batchInfo + 1)->facesOffset - numberTriangles;
            vertices = (ptrint_t) &segment->vertices[batchInfo->verticesOffset];
        } while (0);
        triangles = (ptrint_t) &segment->triangles[batchInfo->facesOffset];
        texOffset = batchInfo->texOffset << 14;
        levelHeaderIndex = (batchFlags >> 28) & 7;
        if (levelHeaderIndex != (batchInfo->verticesOffset * 0)) {
            gDPSetEnvColor(
                gTrackDL++,
                ((LevelHeader_70 *) ((u8 **) (&((LevelHeader **) gCurrentLevelHeader2)[levelHeaderIndex]))[28])->rgba.r,
                ((LevelHeader_70 *) ((u8 **) (&((LevelHeader **) gCurrentLevelHeader2)[levelHeaderIndex]))[28])->rgba.g,
                ((LevelHeader_70 *) ((u8 **) (&((LevelHeader **) gCurrentLevelHeader2)[levelHeaderIndex]))[28])->rgba.b,
                ((LevelHeader_70 *) ((u8 **) (&((LevelHeader **) gCurrentLevelHeader2)[levelHeaderIndex]))[28])
                    ->rgba.a);
        } else {
            gDPSetEnvColor(gTrackDL++, 255, 255, 255, 0);
        }
        if (batchFlags & RENDER_PULSING_LIGHTS) {
            color = gCurrentLevelHeader2->pulseLightData->outColorValue;
            gDPSetPrimColor(gTrackDL++, 0, 0, color, color, color, color);
            material_set_blinking_lights(&gTrackDL, texture, batchFlags, texOffset);
            gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(vertices), numberVertices, 0);
            gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(triangles), numberTriangles, TRIN_ENABLE_TEXTURE);
            gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, 255);
        } else {
            material_set(&gTrackDL, texture, batchFlags, texOffset);
            batchFlags = TRUE;
            if (texture == NULL) {
                batchFlags = FALSE;
            }
            gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(vertices), numberVertices, 0);
            gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(triangles), numberTriangles, batchFlags);
        }
    }
}

/**
 * Parse through applicable segments in the level.
 * Uses function recursion to ensure adjacent segments remain next to each other in the list.
 */
void traverse_segments_bsp_tree(s32 nodeIndex, s32 segmentIndex, s32 segmentIndex2, u8 *segmentsOrder,
                                s32 *segmentsOrderIndex) {
    BspTreeNode *curNode;
    s32 camValue;

    curNode = &gCurrentLevelModel->segmentsBspTree[nodeIndex];
    if (curNode->splitType == 0) {
        camValue = gSceneActiveCamera->trans.x_position; // Camera X
    } else if (curNode->splitType == 1) {
        camValue = gSceneActiveCamera->trans.y_position; // Camera Y
    } else {
        camValue = gSceneActiveCamera->trans.z_position; // Camera Z
    }

    if (camValue < curNode->splitValue) {
        if (curNode->leftNode != -1) {
            traverse_segments_bsp_tree(curNode->leftNode, segmentIndex, curNode->segmentIndex - 1, segmentsOrder,
                                       segmentsOrderIndex);
        } else {
            add_segment_to_order(segmentIndex, segmentsOrderIndex, segmentsOrder);
        }

        if (curNode->rightNode != -1) {
            traverse_segments_bsp_tree(curNode->rightNode, curNode->segmentIndex, segmentIndex2, segmentsOrder,
                                       segmentsOrderIndex);
        } else {
            add_segment_to_order(segmentIndex2, segmentsOrderIndex, segmentsOrder);
        }
    } else {
        if (curNode->rightNode != -1) {
            traverse_segments_bsp_tree(curNode->rightNode, curNode->segmentIndex, segmentIndex2, segmentsOrder,
                                       segmentsOrderIndex);
        } else {
            add_segment_to_order(segmentIndex2, segmentsOrderIndex, segmentsOrder);
        }

        if (curNode->leftNode != -1) {
            traverse_segments_bsp_tree(curNode->leftNode, segmentIndex, curNode->segmentIndex - 1, segmentsOrder,
                                       segmentsOrderIndex);
        } else {
            add_segment_to_order(segmentIndex, segmentsOrderIndex, segmentsOrder);
        }
    }
}

/**
 * Add this segment index to the specified segment ordering table if the segment in question is in view of the camera.
 */
void add_segment_to_order(s32 segmentIndex, s32 *segmentsOrderIndex, u8 *segmentsOrder) {
    u32 temp;
    if (segmentIndex < gCurrentLevelModel->numberOfSegments) {
        if (gSceneStartSegment != -1) {
            temp = gCurrentLevelModel->segmentsBitfields[gSceneStartSegment + (segmentIndex >> 3)];
            temp >>= segmentIndex & 7;
            temp &= 0xFF;
        } else {
            temp = 1;
        }
        if (temp & 1 && block_visible(&gCurrentLevelModel->segmentsBoundingBoxes[segmentIndex])) {
            segmentsOrder[(*segmentsOrderIndex)++] = segmentIndex;
        }
    }
}

/**
 * Checks if the active camera is currently inside this segment.
 * Has a small inner margin where it doesn't consider the camera inside.
 * Goes unused.
 */
UNUSED s32 check_if_inside_segment(Object *obj, s32 segmentIndex) {
    LevelModelSegmentBoundingBox *bb;
    s32 x, y, z;
    if (segmentIndex >= gCurrentLevelModel->numberOfSegments) {
        return FALSE;
    }
    bb = &gCurrentLevelModel->segmentsBoundingBoxes[segmentIndex];
    x = obj->trans.x_position;
    y = obj->trans.y_position;
    z = obj->trans.z_position;
    if ((x < (bb->x2 + 25)) && ((bb->x1 - 25) < x) && (z < (bb->z2 + 25)) && ((bb->z1 - 25) < z) &&
        (y < (bb->y2 + 25)) && ((bb->y1 - 25) < y)) {
        return TRUE;
    }

    return FALSE;
}

/**
 * Iterates through every existing segment to see which one the active camera is inside.
 * Uses mainly a two dimensional axis check here, instead of the function above.
 * Returns the segment currently inside.
 * Official Name: trackGetBlock
 */
s32 get_level_segment_index_from_position(f32 xPos, f32 yPos, f32 zPos) {
    LevelModelSegmentBoundingBox *bb;
    s32 i;
    s32 z = zPos;
    s32 x = xPos;
    s32 y = yPos;
    s32 minVal;
    s32 result;
    s32 heightDiff;

    if (gCurrentLevelModel == NULL) {
        return -1;
    }

    minVal = 1000000;
    result = -1;

    for (i = 0; i < gCurrentLevelModel->numberOfSegments; i++) {
        bb = &gCurrentLevelModel->segmentsBoundingBoxes[i];
        if (x < bb->x2 && bb->x1 < x && z < bb->z2 && bb->z1 < z) {
            heightDiff = (bb->y2 + bb->y1) >> 1; // Couldn't get / 2 to match, but >> 1 does.
            heightDiff = y - heightDiff;
            if (heightDiff < 0) {
                heightDiff = -heightDiff;
            }
            if (heightDiff < minVal) {
                minVal = heightDiff;
                result = i;
            }
        }
    }

    return result;
}

/**
 * Iterates through every existing segment to see which one the active camera is inside.
 * Uses mainly a two dimensional axis check here, instead of the function above.
 * Increments a counter based on if it's got a camera inside.
 * Because there's a tiny margin, multiple segments can be considered populated,
 * meaning that sometimes it will 2 instead of 1.
 */
s32 get_inside_segment_count_xz(s32 x, s32 z, s32 *arg2) {
    s32 i;
    s32 cnt = 0;
    LevelModelSegmentBoundingBox *bb;
    for (i = 0; i < gCurrentLevelModel->numberOfSegments; i++) {
        bb = gCurrentLevelModel->segmentsBoundingBoxes + i;
        if (x < bb->x2 + 4 && bb->x1 - 4 < x && z < bb->z2 + 4 && bb->z1 - 4 < z) {
            *arg2 = i;
            cnt++;
            arg2++;
        }
    }
    return cnt;
}

/**
 * Carbon copy of the above function, but takes into account the Y axis, too.
 * Official name: trackGetCubeBlockList
 */
s32 get_inside_segment_count_xyz(s32 *arg0, s16 xPos1, s16 yPos1, s16 zPos1, s16 xPos2, s16 yPos2, s16 zPos2) {
    s32 cnt;
    s32 i;
    LevelModelSegmentBoundingBox *bb;

    xPos1 -= 4;
    yPos1 -= 4;
    zPos1 -= 4;
    xPos2 += 4;
    yPos2 += 4;
    zPos2 += 4;

    i = 0;
    cnt = 0;

    while (i < gCurrentLevelModel->numberOfSegments) {
        bb = &gCurrentLevelModel->segmentsBoundingBoxes[i];
        if ((bb->x2 >= xPos1) && (xPos2 >= bb->x1) && (bb->z2 >= zPos1) && (zPos2 >= bb->z1) && (bb->y2 >= yPos1) &&
            (yPos2 >= bb->y1)) {
            cnt++;
            *arg0++ = i;
        }
        i++;
    }
    return cnt;
}

/**
 * Returns this block data.
 */
LevelModelSegment *block_get(s32 segmentID) {
    if (segmentID < 0 || gCurrentLevelModel->numberOfSegments < segmentID) {
        return NULL;
    }

    return &gCurrentLevelModel->segments[segmentID];
}

/**
 * Returns the bounding box data of this block.
 * Official name: trackBlockDim
 */
LevelModelSegmentBoundingBox *block_boundbox(s32 segmentID) {
    if (segmentID < 0 || gCurrentLevelModel->numberOfSegments < segmentID) {
        return NULL;
    }

    return &gCurrentLevelModel->segmentsBoundingBoxes[segmentID];
}

/**
 * Rebuilds the view frustum planes from three reference points per plane
 * transformed by the camera matrix.
 */
void compute_frustum_planes(void) {
    f32 ox1;
    f32 oy1;
    f32 oz1;
    f32 ox2;
    f32 oy2;
    f32 oz2;
    f32 ox3;
    f32 oy3;
    f32 oz3;
    MtxF *cameraMatrix;
    f32 inverseMagnitude;
    f32 x;
    f32 y;
    f32 z;
    s32 i;
    f32 w;

    cameraMatrix = get_projection_matrix_f32();
    for (i = 0; i < ARRAY_COUNT(gFrustumPlanes);) {
        x = gFrustumPlanePoints[i][0].x;
        y = gFrustumPlanePoints[i][0].y;
        z = gFrustumPlanePoints[i][0].z;
        ox1 = x;
        oy1 = y;
        oz1 = z;
        mtxf_transform_point(*cameraMatrix, x, y, z, &ox1, &oy1, &oz1);
        x = gFrustumPlanePoints[i][1].x;
        y = gFrustumPlanePoints[i][1].y;
        z = gFrustumPlanePoints[i][1].z;
        ox2 = x;
        oy2 = y;
        oz2 = z;
        mtxf_transform_point(*cameraMatrix, x, y, z, &ox2, &oy2, &oz2);
        x = gFrustumPlanePoints[i][2].x;
        y = gFrustumPlanePoints[i][2].y;
        z = gFrustumPlanePoints[i][2].z;
        ox3 = x;
        oy3 = y;
        oz3 = z;
        mtxf_transform_point(*cameraMatrix, x, y, z, &ox3, &oy3, &oz3);
        x = ((oz2 - oz3) * oy1) + (oy2 * (oz3 - oz1)) + (oy3 * (oz1 - oz2));
        y = ((ox2 - ox3) * oz1) + (oz2 * (ox3 - ox1)) + (oz3 * (ox1 - ox2));
        z = ((oy2 - oy3) * ox1) + (ox2 * (oy3 - oy1)) + (ox3 * (oy1 - oy2));
        inverseMagnitude = (1.0 / sqrtf((x * x) + (y * y) + (z * z)));
        if (inverseMagnitude > 0.0) {
            x *= inverseMagnitude;
            y *= inverseMagnitude;
            z *= inverseMagnitude;
        }
        w = -((ox1 * x) + (oy1 * y) + (oz1 * z));
        gFrustumPlanes[i].x = x;
        gFrustumPlanes[i].y = y;
        gFrustumPlanes[i].z = z;
        gFrustumPlanes[i].w = w;
        i++;
    }
}

/**
 * Takes a normalised (0-1) face direction of the active camera, then adds together a magnitude
 * to a total figure to determine whether or not a segment should be visible.
 * There's a large unused portion at the bottom writing to two vars, that are never later read.
 */
s32 block_visible(LevelModelSegmentBoundingBox *bb) {
    UNUSED u8 unknown[0x28];
    s64 sp48;
    s32 i, j;
    s32 isVisible;
    f32 dirX, dirY, dirZ, dirW;
    f32 x, y, z;

    for (j = 0; j < 3; j++) {
        dirX = gFrustumPlanes[j].x;
        dirY = gFrustumPlanes[j].y;
        dirZ = gFrustumPlanes[j].z;
        dirW = gFrustumPlanes[j].w;

        for (i = 0, isVisible = FALSE; i < 8 && !isVisible; i++) {
            if (i & 1) {
                sp48 = bb->x1 * dirX;
            } else {
                sp48 = bb->x2 * dirX;
            }
            if (i & 2) {
                sp48 += bb->y1 * dirY;
            } else {
                sp48 += bb->y2 * dirY;
            }
            if (i & 4) {
                sp48 += bb->z1 * dirZ;
            } else {
                sp48 += bb->z2 * dirZ;
            }
            sp48 += dirW;
            if (sp48 > 0) {
                isVisible = TRUE;
            }
        }
        if (i == 8 && !isVisible) {
            return FALSE;
        }
    }
    // From here until the "return TRUE" goes completely unused, functionally.
    x = (bb->x2 + bb->x1) >> 1;
    y = (bb->y2 + bb->y1) >> 1;
    z = (bb->z2 + bb->z1) >> 1;
    gCurrBBoxDistanceToCamera = get_distance_to_active_camera(x, y, z);
    if (gCurrBBoxDistanceToCamera < 1000.0) {
        gIsNearCurrBBox = TRUE;
    } else {
        gIsNearCurrBBox = FALSE;
    }
    return TRUE;
}

/**
 * Get the draw distance of the object, then compare it to the active camera position.
 * At the edge of its view distance, it will set its alpha based on distance, giving it a fade in or out effect.
 * Objects in range return true, objects out of range return false.
 */
s32 check_if_in_draw_range(Object *obj) {
    f32 w;
    f32 y;
    f32 fadeDist;
    f32 z;
    f32 x;
    s32 viewDistance;
    s32 alpha;
    s32 i;
    Object_AnimatedObject *animatedObj;
    f32 accum;
    s32 temp2;
    f32 dist;
    Object_Racer *racer;

    if (!(obj->trans.flags & OBJ_FLAGS_PARTICLE)) {
        alpha = 255;
        viewDistance = obj->header->drawDistance;
        if (obj->header->drawDistance) {
            if (gScenePlayerViewports == 3) {
                viewDistance *= 0.5;
            }

            dist = get_distance_to_active_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);

            if (viewDistance < dist) {
                return FALSE;
            }

            fadeDist = viewDistance * 0.8;
            if (fadeDist < dist) {
                temp2 = viewDistance - fadeDist;
                if (temp2 > 0) {
                    fadeDist = dist - fadeDist;
                    alpha = ((f32) (1.0 - ((fadeDist) / temp2)) * 255.0);
                }
                if (alpha == 0) {
                    alpha = 1;
                }
            }
        }
        if (obj_behaviour(obj->behaviorId)->fade != NULL) {
            obj_behaviour(obj->behaviorId)->fade(obj, alpha);
        } else {
            obj->opacity = alpha;
        }
        for (i = 0; i < 3; i++) {
            x = gFrustumPlanes[i].x;
            z = gFrustumPlanes[i].z;
            w = gFrustumPlanes[i].w;
            y = gFrustumPlanes[i].y;
            accum = (x * obj->trans.x_position) + (y * obj->trans.y_position) + (z * obj->trans.z_position) + w +
                    obj->cullRadius;
            if (accum < 0.0f) {
                return FALSE;
            }
        }
    }
    return TRUE;
}


/**
 * Writes back the track collision data pointers.
 */
UNUSED void get_collision_candidate_data(s32 *numCollsionCandidates, ptrint_t **collisionCandidates,
                                         s8 **collisionSurfaces) {
    *numCollsionCandidates = gNumCollisionCandidates;
    *collisionCandidates = gCollisionCandidates;
    *collisionSurfaces = gCollisionSurfaces;
}

/**
 * Change the collision response method.
 * For instance, can choose to ignore wall response.
 */
void set_collision_mode(s32 mode) {
    gCollisionMode = mode;
}

/**
 * Returns the surface normals of the current collision point.
 */
s32 get_collision_normal(f32 *outX, f32 *outY, f32 *outZ) {
    *outX = gCollisionNormalX;
    *outY = gCollisionNormalY;
    *outZ = gCollisionNormalZ;
    return gHitWall;
}

/**
 * Iterates through active waves on the track, then saves its rotation vector and height if found.
 * Returns zero if no waves are found, or if too high up.
 * Two types exist: calm, which have no means of displacement, and wavy, which do.
 */
s32 get_wave_properties(f32 yPos, f32 *waterHeight, Vec3f *rotation) {
    s32 var_a0;
    WaterProperties **wave;
    s32 i;
    s32 index;
    s32 len;
    f32 height;

    len = gWaterSurfaceHitCount;
    if (rotation != NULL) {
        rotation->f[0] = 0.0f;
        rotation->f[2] = 0.0f;
        rotation->f[1] = 1.0f;
    }
    wave = gTrackWaves;
    for (var_a0 = i = 0; i < len; i++) {
        if (wave[i]->type == SURFACE_WATER_CALM || wave[i]->type == SURFACE_WATER_WAVY) {
            var_a0++;
        }
    }
    if (var_a0 == 0) {
        return SURFACE_DEFAULT;
    }
    wave = gTrackWaves;
    index = -1;
    for (i = 0; i < len; i++) {
        height = wave[i]->waveHeight;
        if (wave[i]->type == SURFACE_WATER_CALM || wave[i]->type == SURFACE_WATER_WAVY) {
            if (yPos < height + 25.0 && (wave[i]->rot.y > 0.5 || var_a0 == 1)) {
                index = i;
            }
        } else if (index >= 0 && var_a0 >= 2 && yPos < height - 20.0) {
            index = -1;
        }
    }
    if (index < 0) {
        return SURFACE_DEFAULT;
    }
    *waterHeight = gTrackWaves[index]->waveHeight;
    if (rotation != NULL) {
        rotation->f[0] = gTrackWaves[index]->rot.x;
        rotation->f[1] = gTrackWaves[index]->rot.y;
        rotation->f[2] = gTrackWaves[index]->rot.z;
    }
    return gTrackWaves[index]->type;
}

/**
 * Finds the waves in the current level segment, sorts them by height, outputs the sorted list into
 * waterProps and returns the number of waves.
 */
s32 get_level_segment_waves(s32 levelSegmentIndex, f32 xIn, f32 zIn, WaterProperties ***waterProps) {
    LevelModelSegmentBoundingBox *currentBoundingBox;
    Triangle *tri;
    Vertex *vert;
    s16 temp_a2;
    s32 currentVerticesOffset;
    LevelModelSegment *currentSegment;
    s32 j;
    s32 segmentCount;
    s16 vert1X;
    s16 vert1Z;
    s16 vert2X;
    s16 vert2Z;
    s16 vert3X;
    s16 vert3Z;
    s32 nextFaceOffset;
    s16 var_a1;
    s32 currentFaceOffset;
    s32 faceNum;
    s16 var_s1;
    s16 var_t0;
    s16 var_t1;
    s32 XInInt;
    s32 ZInInt;
    s32 side1;
    s32 side2;
    s32 side3;
    s32 segmentsInside[8];
    s32 yOutCount;
    s32 batchNum;
    s32 i;
    s32 stopSorting;
    f32 A, B, C, D;
    s32 temp;
    s32 surfaceType;
    s32 unused_bool;
    s32 pad;
    WaterProperties *wave;
    WaterProperties **waves;

    unused_bool = FALSE;
    gWaterSurfaceHitCount = 0;
    *waterProps = NULL;
    XInInt = xIn;
    ZInInt = zIn;
    segmentCount = get_inside_segment_count_xz(XInInt, ZInInt, segmentsInside);
    if (segmentCount == 0 || segmentCount >= 8) {
        return 0;
    }

    yOutCount = 0;
    for (i = 0; i < segmentCount; i++) {
        currentSegment = &gCurrentLevelModel->segments[segmentsInside[i]];
        currentBoundingBox = &gCurrentLevelModel->segmentsBoundingBoxes[segmentsInside[i]];

        var_a1 = 1;
        var_s1 = 0;

        temp_a2 = ((currentBoundingBox->x2 - currentBoundingBox->x1) >> 3) + 1;
        var_t0 = temp_a2 + currentBoundingBox->x1;
        var_t1 = currentBoundingBox->x1;

        for (j = 0; j < 8; j++) {
            if (var_t0 >= XInInt && XInInt >= var_t1) {
                var_s1 |= var_a1;
            }
            var_t0 += temp_a2;
            var_t1 += temp_a2;
            var_a1 <<= 1;
        }

        temp_a2 = ((currentBoundingBox->z2 - currentBoundingBox->z1) >> 3) + 1;
        var_t0 = temp_a2 + currentBoundingBox->z1;
        var_t1 = currentBoundingBox->z1;

        for (j = 0; j < 8; j++) {
            if (var_t0 >= ZInInt && ZInInt >= var_t1) {
                var_s1 |= var_a1;
            }
            var_t0 += temp_a2;
            var_t1 += temp_a2;
            var_a1 <<= 1;
        }

        for (batchNum = 0; batchNum < currentSegment->numberOfBatches; batchNum++) {
            surfaceType = gCurrentLevelModel->textures[currentSegment->batches[batchNum].textureIndex].surfaceType;
            currentFaceOffset = currentSegment->batches[batchNum].facesOffset;
            currentVerticesOffset = currentSegment->batches[batchNum].verticesOffset;
            nextFaceOffset = currentSegment->batches[batchNum + 1].facesOffset;
            if ((surfaceType != SURFACE_WATER_CALM && surfaceType != SURFACE_WATER_UNK_F) &&
                (currentSegment->batches[batchNum].flags & (RENDER_HIDDEN | RENDER_NO_COLLISION))) {
                currentFaceOffset = nextFaceOffset;
            }
            for (faceNum = currentFaceOffset; faceNum < nextFaceOffset; faceNum++) {
                if (var_s1 == (currentSegment->triangleGridMasks[faceNum] & var_s1)) {
                    tri = &currentSegment->triangles[faceNum];
                    vert = &currentSegment->vertices[tri->verticesArray[1] + currentVerticesOffset];
                    vert1X = vert->x;
                    vert1Z = vert->z;
                    vert = &currentSegment->vertices[tri->verticesArray[2] + currentVerticesOffset];
                    vert2X = vert->x;
                    vert2Z = vert->z;
                    vert = &currentSegment->vertices[tri->verticesArray[3] + currentVerticesOffset];
                    vert3X = vert->x;
                    vert3Z = vert->z;
                    side1 = (((XInInt - vert2X) * (vert3Z - vert2Z)) - ((vert3X - vert2X) * (ZInInt - vert2Z))) >= 0;
                    side2 = (((XInInt - vert1X) * (vert2Z - vert1Z)) - ((vert2X - vert1X) * (ZInInt - vert1Z))) >= 0;
                    side3 = (((XInInt - vert1X) * (vert3Z - vert1Z)) - ((vert3X - vert1X) * (ZInInt - vert1Z))) >= 0;
                    if (side1 == side2 && side2 != side3) {
                        temp = currentSegment->collisionFacets[faceNum].basePlaneIndex;
                        A = currentSegment->collisionPlanes[(temp * 4) + 0];
                        B = currentSegment->collisionPlanes[(temp * 4) + 1];
                        C = currentSegment->collisionPlanes[(temp * 4) + 2];
                        D = currentSegment->collisionPlanes[(temp * 4) + 3];
                        if (B != 0.0) {
                            gWaterSurfaceHits[yOutCount].type = surfaceType;
                            gWaterSurfaceHits[yOutCount].waveHeight = -((((A * xIn) + (C * zIn)) + D) / B);
                            gWaterSurfaceHits[yOutCount].rot.x = A;
                            gWaterSurfaceHits[yOutCount].rot.y = B;
                            gWaterSurfaceHits[yOutCount].rot.z = C;
                            yOutCount++;
                            if (yOutCount >= 20) {
                                batchNum = currentSegment->numberOfBatches;
                                faceNum = nextFaceOffset;
                                i = segmentCount;
                            } else if (unused_bool) {
                                // Fake to set to this to basically any variable in currentSegment as a break for
                                // optimization. This code should never run.
                                currentSegment->numberOfBatches = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    if (levelSegmentIndex >= 0 && levelSegmentIndex < gCurrentLevelModel->numberOfSegments) {
        currentSegment = &gCurrentLevelModel->segments[levelSegmentIndex];
        gWaterSurfaceHits[yOutCount].type = SURFACE_WATER_WAVY;
        if (currentSegment->hasWaves && gWaveBlockCount != 0) {
            wave = yOutCount + gWaterSurfaceHits; // fake?
            gWaterSurfaceHits[yOutCount].waveHeight = wave_height_at(levelSegmentIndex, xIn, zIn, &wave->rot);
        } else {
            gWaterSurfaceHits[yOutCount].waveHeight = currentSegment->waterHeight;
            gWaterSurfaceHits[yOutCount].rot.x = 0.0f;
            gWaterSurfaceHits[yOutCount].rot.y = 1.0f;
            gWaterSurfaceHits[yOutCount].rot.z = 0.0f;
        }
        yOutCount++;
    }

    waves = gTrackWaves;
    wave = gWaterSurfaceHits;

    // clang-format off
    for (j = 0; j < yOutCount; j++) { waves[j] = &wave[j]; }
    // clang-format on

    do {
        stopSorting = TRUE;

        for (j = 0; j < (yOutCount - 1); j++) {
            if (waves[j]->waveHeight < waves[j + 1]->waveHeight) {
                stopSorting = FALSE;
                wave = waves[j];
                waves[j] = waves[j + 1];
                waves[j + 1] = wave;
            }
        }

    } while (!stopSorting);

    *waterProps = gTrackWaves;
    gWaterSurfaceHitCount = yOutCount;

    return yOutCount;
}

/**
 * The water surface height under an object: the wave height when the
 * segment has waves and they are wanted, otherwise the segment's water level.
 * Also returns the surface normal, flat unless waves are consulted.
 */
s32 get_object_water_height(Object *obj, f32 *arg1, Vec3f *arg2, s32 arg3) {
    LevelModelSegment *seg;

    if (arg2 != NULL) {
        arg2->x = 0.0f;
        arg2->z = 0.0f;
        arg2->y = 1.0f;
    }
    if ((obj->segmentID < 0) || (obj->segmentID >= gCurrentLevelModel->numberOfSegments)) {
        return FALSE;
    }
    seg = &gCurrentLevelModel->segments[obj->segmentID];
    if ((seg->hasWaves != 0) && (gWaveBlockCount != 0) && (arg3 == 1)) {
        *arg1 = wave_height_at(obj->segmentID, obj->trans.x_position, obj->trans.z_position, arg2);
        return TRUE;
    } else {
        *arg1 = seg->waterHeight;
        return TRUE;
    }
}

/**
 * Searches for intersecting surfaces, then returns the Y values of all the intersecting points, in order.
 * Writes at most maxYOut values to yOut; surfaces beyond that are dropped.
 */
s32 collision_get_y(s32 levelSegmentIndex, f32 xIn, f32 zIn, f32 *yOut, s32 maxYOut) {
    LevelModelSegment *currentSegment;
    LevelModelSegmentBoundingBox *currentBoundingBox;
    Triangle *tri;
    Vertex *vert;
    f32 temp_f2_2;
    s16 vert2X;
    s16 vert2Z;
    s16 temp_a2;
    s16 vert3X;
    s16 vert3Z;
    s32 currentVerticesOffset;
    s16 nextFaceOffset;
    s16 vert1X;
    s16 currentFaceOffset;
    s16 vert1Z;
    s16 var_a1;
    s32 faceNum;
    s16 var_s1;
    s16 var_t0;
    s16 var_t1;
    s32 XInInt;
    s32 ZInInt;
    s32 temp_ra_1;
    s32 temp_ra_2;
    s32 temp_ra_3;
    s32 yOutCount;
    s32 batchNum;
    s32 i;
    s32 var_v0;
    s32 stopSorting;
    TriangleBatchInfo *currentBatch;
    f32 *temp_v1_4;
    Vec4f tempVec4f;
    u16 temp;

    if (levelSegmentIndex < 0 || levelSegmentIndex >= gCurrentLevelModel->numberOfSegments) {
        return 0;
    }

    vert = NULL; // fake?

    currentSegment = &gCurrentLevelModel->segments[levelSegmentIndex];
    currentBoundingBox = &gCurrentLevelModel->segmentsBoundingBoxes[levelSegmentIndex];
    var_a1 = 1;
    var_s1 = 0;
    XInInt = xIn;
    ZInInt = zIn;

    temp_a2 = ((currentBoundingBox->x2 - currentBoundingBox->x1) >> 3) + 1;
    var_t0 = temp_a2 + currentBoundingBox->x1;
    var_t1 = currentBoundingBox->x1;
    for (i = 0; i < 8; i++) {
        if (var_t0 >= XInInt && XInInt >= var_t1) {
            var_s1 |= var_a1;
        }
        var_t0 += temp_a2;
        var_t1 += temp_a2;
        var_a1 *= 2;
    }

    // Same as above, but for Z
    temp_a2 = ((currentBoundingBox->z2 - currentBoundingBox->z1) >> 3) + 1;
    var_t0 = temp_a2 + currentBoundingBox->z1;
    var_t1 = currentBoundingBox->z1;
    for (i = 0; i < 8; i++) {
        if (var_t0 >= ZInInt && ZInInt >= var_t1) {
            var_s1 |= var_a1;
        }
        var_t0 += temp_a2;
        var_t1 += temp_a2;
        var_a1 *= 2;
    }

    yOutCount = 0;
    for (batchNum = 0; batchNum < currentSegment->numberOfBatches; batchNum++) {
        do {
        } while (0); // fakematch
        currentFaceOffset = currentSegment->batches[batchNum].facesOffset;
        nextFaceOffset = currentSegment->batches[batchNum + 1].facesOffset;
        currentVerticesOffset = currentSegment->batches[batchNum].verticesOffset;
        for (faceNum = currentFaceOffset; faceNum < nextFaceOffset; faceNum++) {
            if (var_s1 == (currentSegment->triangleGridMasks[faceNum] & var_s1)) {
                tri = &currentSegment->triangles[faceNum];

                vert = &currentSegment->vertices[tri->verticesArray[1] + currentVerticesOffset];
                vert1X = vert->x;
                vert1Z = vert->z;

                vert = &currentSegment->vertices[tri->verticesArray[2] + currentVerticesOffset];
                vert2X = vert->x;
                vert2Z = vert->z;

                vert = &currentSegment->vertices[tri->verticesArray[3] + currentVerticesOffset];
                vert3X = vert->x;
                vert3Z = vert->z;

                temp_ra_1 = (((XInInt - vert2X) * (vert3Z - vert2Z)) - ((vert3X - vert2X) * (ZInInt - vert2Z))) >= 0;
                temp_ra_2 = (((XInInt - vert1X) * (vert2Z - vert1Z)) - ((vert2X - vert1X) * (ZInInt - vert1Z))) >= 0;
                temp_ra_3 = (((XInInt - vert1X) * (vert3Z - vert1Z)) - ((vert3X - vert1X) * (ZInInt - vert1Z))) >= 0;
                var_v0 = faceNum; // fake?
                if (temp_ra_1 == temp_ra_2 && temp_ra_2 != temp_ra_3) {
                    temp = currentSegment->collisionFacets[faceNum].basePlaneIndex;
                    tempVec4f.x = currentSegment->collisionPlanes[4 * temp + 0];
                    tempVec4f.y = currentSegment->collisionPlanes[4 * temp + 1];
                    tempVec4f.z = currentSegment->collisionPlanes[4 * temp + 2];
                    tempVec4f.w = currentSegment->collisionPlanes[4 * temp + 3];
                    if (tempVec4f.y != 0.0 && yOutCount < maxYOut) {
                        yOut[yOutCount] = -(((tempVec4f.x * xIn) + (tempVec4f.z * zIn) + tempVec4f.w) / tempVec4f.y);
                        yOutCount++;
                    }
                }
            }
        }
    }

    do {
        stopSorting = TRUE;
        for (var_v0 = 0; var_v0 < yOutCount - 1; var_v0++) {
            if (yOut[var_v0] > yOut[var_v0 + 1]) {
                stopSorting = FALSE;
                temp_f2_2 = yOut[var_v0];
                yOut[var_v0] = yOut[var_v0 + 1];
                yOut[var_v0 + 1] = temp_f2_2;
            }
        }
    } while (!stopSorting);

    return yOutCount;
}

// Decompressed level models are big-endian. The 0x4C-byte LevelModel header and
// the 0x44-byte LevelModelSegment records both carry pointer slots, so on the
// host the image is placed LEVEL_MODEL_PC_DELTA bytes into the track heap, the
// header is unpacked over the front, and the segment and TextureInfo tables are
// rebuilt in host layout after the image. modelSize is bumped past those tables
// so generate_track's trailing per-segment allocations follow them.
#define LEVEL_MODEL_DISK_SIZE 0x4C
#define LEVEL_MODEL_PC_DELTA (sizeof(LevelModel) - LEVEL_MODEL_DISK_SIZE)
#define LEVEL_SEGMENT_DISK_SIZE 0x44
#define TEXTURE_INFO_DISK_SIZE 8
#define PC_ALIGN16(x) (((x) + 15) & ~15)

static const PcField sLevelModelFields[] = {
    PCU_FIELD(LevelModel, textures, 0x00, 6, PCU_PTR), // textures..segmentsBspTree
    PCU_FIELD(LevelModel, numberOfTextures, 0x18, 4, PCU_U16),
    PCU_FIELD(LevelModel, minimapSpriteIndex, 0x20, 1, PCU_SPTR), // becomes a Sprite * at runtime
    PCU_FIELD(LevelModel, minimapRotation, 0x24, 2, PCU_U16),
    PCU_FIELD(LevelModel, minimapXScale, 0x28, 2, PCU_U32),
    PCU_FIELD(LevelModel, minimapOffsetXAdv1, 0x30, 4, PCU_U16),
    PCU_FIELD(LevelModel, minimapColor, 0x38, 1, PCU_U32),
    PCU_FIELD(LevelModel, lowerXBounds, 0x3C, 6, PCU_U16),
    PCU_FIELD(LevelModel, modelSize, 0x48, 1, PCU_U32),
    PCU_END,
};

static const PcField sLevelSegmentFields[] = {
    PCU_FIELD(LevelModelSegment, vertices, 0x00, 2, PCU_PTR), // vertices, triangles
    PCU_FIELD(LevelModelSegment, unused8, 0x08, 1, PCU_U32),
    PCU_FIELD(LevelModelSegment, batches, 0x0C, 4, PCU_PTR), // batches, unk10, collisionFacets, collisionPlanes
    PCU_FIELD(LevelModelSegment, numberOfVertices, 0x1C, 3, PCU_U16),
    PCU_FIELD(LevelModelSegment, pad22, 0x22, 6, PCU_BYTES),
    PCU_FIELD(LevelModelSegment, visibilityBitfieldOffset, 0x28, 1, PCU_U16),
    PCU_FIELD(LevelModelSegment, unused2A, 0x2A, 2, PCU_BYTES), // unused2A, hasWaves
    PCU_FIELD(LevelModelSegment, unused2C, 0x2C, 1, PCU_PTR),
    PCU_FIELD(LevelModelSegment, unused30, 0x30, 2, PCU_U16),
    PCU_FIELD(LevelModelSegment, waterBatchIndices, 0x34, 1, PCU_PTR),
    PCU_FIELD(LevelModelSegment, waterHeight, 0x38, 1, PCU_U16),
    PCU_FIELD(LevelModelSegment, pad3A, 0x3A, 2, PCU_BYTES),
    PCU_FIELD(LevelModelSegment, segmentFlags, 0x3C, 1, PCU_U32),
    PCU_FIELD(LevelModelSegment, numberofOpaqueBatches, 0x40, 4, PCU_BYTES),
    PCU_END,
};

#define PC_REBASE(obj, field, T) (obj)->field = (T) ((uintptr_t) (obj)->field + LEVEL_MODEL_PC_DELTA)

// Inflate level model `assetOffset` (compressed size `compSize`) into the track
// heap at `heap`, in host layout. Afterwards every pointer slot holds a
// heap-relative offset, exactly what LOCAL_OFFSET_TO_RAM_ADDRESS expects.
static void pc_load_level_model(LevelModel *heap, s32 assetOffset, s32 compSize) {
    u8 *image = (u8 *) heap + LEVEL_MODEL_PC_DELTA;
    u8 *comp;
    s32 diskSize;
    LevelModelSegment *segs;
    TextureInfo *texInfo;
    u32 segOff, texOff, bbOff, bspOff;
    u32 end;
    s32 k;

    if (asset_image_inflated()) {
        diskSize = compSize;
        asset_load(ASSET_LEVEL_MODELS, (uintptr_t) image, assetOffset, compSize);
    } else {
        diskSize = gzip_size_uncompressed(ASSET_LEVEL_MODELS, assetOffset);
        comp = pc_alloc(compSize);
        asset_load(ASSET_LEVEL_MODELS, (uintptr_t) comp, assetOffset, compSize);
        gzip_inflate(comp, image);
        pc_free(comp);
    }

    pc_unpack(heap, sizeof(LevelModel), image, LEVEL_MODEL_DISK_SIZE, sLevelModelFields);
    texOff = (u32) (uintptr_t) heap->textures;
    segOff = (u32) (uintptr_t) heap->segments;
    bbOff = (u32) (uintptr_t) heap->segmentsBoundingBoxes;
    bspOff = (u32) (uintptr_t) heap->segmentsBspTree;
    PC_REBASE(heap, segmentsBoundingBoxes, LevelModelSegmentBoundingBox *);
    PC_REBASE(heap, unusedC, u8 *);
    PC_REBASE(heap, segmentsBitfields, u8 *);
    PC_REBASE(heap, segmentsBspTree, BspTreeNode *);

    // Host segment table after the image.
    end = (u32) PC_ALIGN16(LEVEL_MODEL_PC_DELTA + diskSize); // host tables need pointer alignment
    segs = (LevelModelSegment *) ((u8 *) heap + end);
    pc_unpack_array(segs, sizeof(LevelModelSegment), image + segOff, LEVEL_SEGMENT_DISK_SIZE,
                    heap->numberOfSegments, sLevelSegmentFields);
    for (k = 0; k < heap->numberOfSegments; k++) {
        PC_REBASE(&segs[k], vertices, Vertex *);
        PC_REBASE(&segs[k], triangles, Triangle *);
        PC_REBASE(&segs[k], batches, TriangleBatchInfo *);
        PC_REBASE(&segs[k], collisionFacets, CollisionFacetPlanes *);
        // unk10, collisionPlanes, unk2C, unk34 are assigned at runtime.
    }
    heap->segments = (LevelModelSegment *) (uintptr_t) end;
    end += PC_ALIGN16((u32) (heap->numberOfSegments * sizeof(LevelModelSegment)));

    // Host TextureInfo table (8 bytes per entry on disk).
    texInfo = (TextureInfo *) ((u8 *) heap + end);
    for (k = 0; k < heap->numberOfTextures; k++) {
        const u8 *e = image + texOff + k * TEXTURE_INFO_DISK_SIZE;
        texInfo[k].texture = (TextureHeader *) (uintptr_t) pc_be32(e, 0); // texture id until load_texture
        texInfo[k].width = e[4];
        texInfo[k].height = e[5];
        texInfo[k].format = e[6];
        texInfo[k].surfaceType = (s8) e[7];
    }
    heap->textures = (TextureInfo *) (uintptr_t) end;
    end += PC_ALIGN16((u32) (heap->numberOfTextures * sizeof(TextureInfo)));
    heap->modelSize = end; // trailing per-segment allocations start here

    // Pointer-free tables in the image: swap in place.
    pc_swap16_buf(image + bbOff, heap->numberOfSegments * sizeof(LevelModelSegmentBoundingBox));
    for (k = 0; k < heap->numberOfSegments - 1; k++) { // BSP interior nodes: one fewer than leaves
        BspTreeNode *node = (BspTreeNode *) (image + bspOff + k * sizeof(BspTreeNode));
        PC_SWAP16_AT(node, leftNode, 4); // leftNode, rightNode
        PC_SWAP16_AT(node, splitValue, 2);
    }
}
#undef PC_REBASE

static void pc_swap_level_segment_contents(LevelModelSegment *seg) {
    s32 i;

    for (i = 0; i < seg->numberOfVertices; i++) {
        pc_swap16_buf(&seg->vertices[i], 6); // x, y, z; r/g/b/a are u8
    }
    for (i = 0; i < seg->numberOfTriangles; i++) {
        PC_SWAP16_AT(&seg->triangles[i], uv0, 12); // uv0/uv1/uv2, 6 x s16; flags/indices are u8
    }
    for (i = 0; i < seg->numberOfBatches + 1; i++) { // +1: sentinel entry
        TriangleBatchInfo *b = &seg->batches[i];
        PC_SWAP16_AT(b, verticesOffset, 4); // verticesOffset, facesOffset
        PC_SWAP32_AT(b, flags, 4);
    }
    // One facet-planes entry (4 x u16 plane indices) per triangle.
    for (i = 0; i < seg->numberOfTriangles; i++) {
        pc_swap16_buf(&seg->collisionFacets[i], sizeof(CollisionFacetPlanes));
    }
}

// Loads a level track from the index in the models table.
void generate_track(s32 modelId) {
    s32 i, k;
    ptrint_t j;
    s32 temp_s4;
    s32 temp;
    s32 mdl;

    set_texture_colour_tag(COLOUR_TAG_GREEN);
    gTrackModelHeap = mempool_alloc_safe(LEVEL_MODEL_MAX_SIZE, COLOUR_TAG_YELLOW);
    gCurrentLevelModel = gTrackModelHeap;
    gCollisionCandidates = mempool_alloc_safe(MAX_COLLISION_CANDIDATES * sizeof(ptrint_t), COLOUR_TAG_YELLOW);
    gCollisionSurfaces = mempool_alloc_safe(MAX_COLLISION_CANDIDATES, COLOUR_TAG_YELLOW);
    gNumCollisionCandidates = 0;
    gLevelModelTable = (s32 *) asset_table_load(ASSET_LEVEL_MODELS_TABLE);
    {
        // Big-endian asset offset table (helper in linux/reimpl.c).
        extern void pc_swap32_buf(void *buf, u32 numBytes);
        pc_swap32_buf(gLevelModelTable, asset_table_size(ASSET_LEVEL_MODELS_TABLE));
    }

    for (i = 0; gLevelModelTable[i] != -1; i++) {}
    i--;
    if (modelId >= i) {
        modelId = 0;
    }

    mdl = gLevelModelTable[modelId];
    temp_s4 = gLevelModelTable[modelId + 1] - mdl;

    pc_load_level_model(gCurrentLevelModel, mdl, temp_s4);
    stubbed_printf("post gzip_inflate segs=%d tex=%d\n", gCurrentLevelModel->numberOfSegments,
                   gCurrentLevelModel->numberOfTextures);
    mempool_free(gLevelModelTable); // Done with the level models table, so free it.

    LOCAL_OFFSET_TO_RAM_ADDRESS(TextureInfo *, gCurrentLevelModel->textures);
    LOCAL_OFFSET_TO_RAM_ADDRESS(LevelModelSegment *, gCurrentLevelModel->segments);
    LOCAL_OFFSET_TO_RAM_ADDRESS(LevelModelSegmentBoundingBox *, gCurrentLevelModel->segmentsBoundingBoxes);
    LOCAL_OFFSET_TO_RAM_ADDRESS(u8 *, gCurrentLevelModel->unusedC);
    LOCAL_OFFSET_TO_RAM_ADDRESS(u8 *, gCurrentLevelModel->segmentsBitfields);
    LOCAL_OFFSET_TO_RAM_ADDRESS(BspTreeNode *, gCurrentLevelModel->segmentsBspTree);

    for (k = 0; k < gCurrentLevelModel->numberOfSegments; k++) {
        LOCAL_OFFSET_TO_RAM_ADDRESS(Vertex *, gCurrentLevelModel->segments[k].vertices);
        LOCAL_OFFSET_TO_RAM_ADDRESS(Triangle *, gCurrentLevelModel->segments[k].triangles);
        LOCAL_OFFSET_TO_RAM_ADDRESS(TriangleBatchInfo *, gCurrentLevelModel->segments[k].batches);
        LOCAL_OFFSET_TO_RAM_ADDRESS(CollisionFacetPlanes *, gCurrentLevelModel->segments[k].collisionFacets);
        pc_swap_level_segment_contents(&gCurrentLevelModel->segments[k]);
    }
    for (k = 0; k < gCurrentLevelModel->numberOfTextures; k++) {
        gCurrentLevelModel->textures[k].texture =
            load_texture(((s32) gCurrentLevelModel->textures[k].texture) | 0x8000);
    }
    j = (ptrint_t) gCurrentLevelModel + gCurrentLevelModel->modelSize;
    for (k = 0; k < gCurrentLevelModel->numberOfSegments; k++) {
        gCurrentLevelModel->segments[k].triangleGridMasks = (s16 *) j;
        j = (ptrint_t) align16(((u8 *) (gCurrentLevelModel->segments[k].numberOfTriangles * 2)) + j);
        gCurrentLevelModel->segments[k].collisionPlanes = (f32 *) j;
        j = (ptrint_t) & ((u8 *) j)[track_init_collision(&gCurrentLevelModel->segments[k])];
        segment_compute_triangle_grid_masks(&gCurrentLevelModel->segments[k], &gCurrentLevelModel->segmentsBoundingBoxes[k], k);
        gCurrentLevelModel->segments[k].unused30 = 0;
        gCurrentLevelModel->segments[k].waterBatchIndices = (s16 *) j;
        segment_find_water_batches(&gCurrentLevelModel->segments[k]);
        j = (ptrint_t) align16(((u8 *) (gCurrentLevelModel->segments[k].numWaterBatches * 2)) + j);
    }
    temp_s4 = (s32) (j - (ptrint_t) gCurrentLevelModel);
    if (temp_s4 > LEVEL_MODEL_MAX_SIZE) {
        pc_log("ERROR!! TrackMem overflow .. %d\n", temp_s4);
    }
    mempool_free_timer(0);
    mempool_free(gTrackModelHeap);
    mempool_alloc_fixed(temp_s4, (u8 *) gTrackModelHeap, COLOUR_TAG_YELLOW);
    mempool_free_timer(2);
    stubbed_printf("pre minimap_init\n");
    minimap_init(gCurrentLevelModel);
    stubbed_printf("post minimap_init\n");

    for (i = 0; i < gCurrentLevelModel->numberOfSegments; i++) {
        for (temp_s4 = 0; temp_s4 < gCurrentLevelModel->segments[i].numberOfBatches; temp_s4++) {
            for (k = gCurrentLevelModel->segments[i].batches[temp_s4].verticesOffset;
                 k < gCurrentLevelModel->segments[i].batches[temp_s4 + 1].verticesOffset; k++) {
                // Why do this? Why not just set the vertex colors in the model itself?
                if (gCurrentLevelModel->segments[i].vertices[k].r == 1 &&
                    gCurrentLevelModel->segments[i].vertices[k].g == 1) {
                    gCurrentLevelModel->segments[i].vertices[k].a = gCurrentLevelModel->segments[i].vertices[k].b;
                    gCurrentLevelModel->segments[i].vertices[k].r = 0x80;
                    gCurrentLevelModel->segments[i].vertices[k].g = 0x80;
                    gCurrentLevelModel->segments[i].vertices[k].b = 0x80;
                    gCurrentLevelModel->segments[i].batches[temp_s4].flags |= 0x08000000;
                }
            }
        }
    }
    set_texture_colour_tag(COLOUR_TAG_MAGENTA);
    stubbed_printf("generate_track end\n");
}

/**
 * Collects a segment's water batches and the highest water level among them.
 */
void segment_find_water_batches(LevelModelSegment *segment) {
    s32 curVertY;
    s32 numVerts;
    s32 i;

    segment->waterHeight = -10000;
    numVerts = 0;
    for (i = 0; i < segment->numberOfBatches; i++) {
        if (segment->batches[i].flags & 0x2000) {
            segment->waterBatchIndices[numVerts++] = i;
            curVertY = segment->vertices[segment->batches[i].verticesOffset].y;
            if (segment->waterHeight == -10000 || segment->waterHeight < curVertY) {
                segment->waterHeight = curVertY;
            }
        }
    }
    segment->numWaterBatches = numVerts;
}

/**
 * Returns the current loaded level geometry
 * Official Name: trackGetTrack
 */
LevelModel *get_current_level_model(void) {
    return gCurrentLevelModel;
}

/**
 * Frees and unloads level data from RAM.
 */
void free_track(void) {
    s32 i;

    racerfx_free();
    if (gWaveBlockCount != 0) {
        waves_free();
    }
    for (i = 0; i < gCurrentLevelModel->numberOfTextures; i++) {
        tex_free(gCurrentLevelModel->textures[i].texture);
    }
    mempool_free(gTrackModelHeap);
    mempool_free(gCollisionCandidates);
    mempool_free(gCollisionSurfaces);
    sprite_free((Sprite *) gCurrentLevelModel->minimapSpriteIndex);
    for (i = 0; i < ARRAY_COUNT(gShadowHeapData); i++) {
        mempool_free(gShadowHeapData[i]);
        mempool_free(gShadowHeapTris[i]);
        mempool_free(gShadowHeapVerts[i]);
    }
    void_free();
    if (gSkydomeSegment != NULL) {
        free_object(gSkydomeSegment);
        gParticlePtrList_flush();
    }
    free_all_objects();
    gCurrentLevelModel = NULL;
}

/**
 * For each triangle in a segment, which cells of the segment's 8x8 grid it
 * touches, as the mask compute_grid_overlap_mask produces for a rectangle.
 */
void segment_compute_triangle_grid_masks(LevelModelSegment *segment, LevelModelSegmentBoundingBox *bbox, UNUSED s32 arg2) {
    Vertex *vert;
    s16 boxDelta;
    s32 vertZ;
    s32 vertX;
    s16 boxMin;
    s16 bit;
    s16 maxX;
    s32 j;
    s16 minX;
    s16 val;
    s16 maxZ;
    s16 minZ;
    s16 k;
    s16 boxMax;
    s32 i;
    s32 l;
    s32 startTri;
    s32 endTri;
    s32 vertsOffset;

    for (i = 0; i < segment->numberOfBatches; i++) {
        startTri = segment->batches[i].facesOffset;
        endTri = segment->batches[i + 1].facesOffset;
        vertsOffset = segment->batches[i].verticesOffset;
        for (j = startTri; j < endTri; j++) {
            if (segment->triangles[j].flags & TRI_FLAG_80) {
                segment->triangleGridMasks[j] = 0;
            } else {
                maxX = -32000;
                maxZ = -32000;
                minZ = 32000;
                minX = 32000;

                for (l = 0; l < 3; l++) {
                    vert = &segment->vertices[segment->triangles[j].verticesArray[l + 1] + vertsOffset];
                    k = vert->x;
                    vertX = k; // This is probably fake, but it matches.
                    vertZ = vert->z;
                    if (maxX < vertX) {
                        maxX = vertX;
                    }
                    if (vertX < minX) {
                        minX = vertX;
                    }
                    if (maxZ < vertZ) {
                        maxZ = vertZ;
                    }
                    if (vertZ < minZ) {
                        minZ = vertZ;
                    }
                }
                boxDelta = ((bbox->x2 - bbox->x1) >> 3) + 1;
                bit = 1;
                boxMax = boxDelta + bbox->x1;
                boxMin = bbox->x1;
                val = 0;
                for (k = 0; k < 8; k++) {
                    if (boxMax >= minX && maxX >= boxMin) {
                        val |= bit;
                    }
                    boxMax += boxDelta;
                    boxMin += boxDelta;
                    bit <<= 1;
                }
                boxDelta = ((bbox->z2 - bbox->z1) >> 3) + 1;
                boxMax = boxDelta + bbox->z1;
                boxMin = bbox->z1;
                for (k = 0; k < 8; k++) {
                    if (boxMax >= minZ && maxZ >= boxMin) {
                        val |= bit;
                    }
                    boxMax += boxDelta;
                    boxMin += boxDelta;
                    bit <<= 1;
                }
                segment->triangleGridMasks[j] = val;
            }
        }
    }
}

s32 track_init_collision(LevelModelSegment *block) {
    s32 facesOffset;
    s32 verticesOffset;
    s32 nextFacesOffset;
    Vertex *v;
    s32 batchIndex;
    s32 triIndex;
    s32 i;
    f32 mag;
    s32 counter;
    s32 colPlaneIndex;
    s32 next;
    s32 numColPlanes;
    s32 vertIndex, nextVertIndex;
    f32 x1, y1, z1;
    f32 x2, y2, z2;
    f32 x3, y3, z3;
    f32 nx, ny, nz;
    f32 x5, y5, z5;
    s32 j;

    counter = 0;

    for (batchIndex = 0; batchIndex < block->numberOfBatches; batchIndex++) {
        facesOffset = block->batches[batchIndex].facesOffset;
        verticesOffset = block->batches[batchIndex].verticesOffset;
        nextFacesOffset = block->batches[batchIndex + 1].facesOffset;

        for (triIndex = facesOffset; triIndex < nextFacesOffset; triIndex++) {
            if (block->triangles[triIndex].flags & TRI_FLAG_80) {
                continue;
            }

            v = &block->vertices[block->triangles[triIndex].vi0 + verticesOffset];
            x1 = v->x;
            y1 = v->y;
            z1 = v->z;

            v = &block->vertices[block->triangles[triIndex].vi1 + verticesOffset];
            x2 = v->x;
            y2 = v->y;
            z2 = v->z;

            v = &block->vertices[block->triangles[triIndex].vi2 + verticesOffset];
            x3 = v->x;
            y3 = v->y;
            z3 = v->z;

            nx = y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2);
            ny = z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2);
            nz = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);

            mag = sqrtf(nx * nx + ny * ny + nz * nz);
            if (mag > 0.0) {
                nx /= mag;
                ny /= mag;
                nz /= mag;
            }

            // looks like a macro
            {
                s32 temp = counter++;
                block->collisionPlanes[4 * temp + 0] = nx;
                block->collisionPlanes[4 * temp + 1] = ny;
                block->collisionPlanes[4 * temp + 2] = nz;
                block->collisionPlanes[4 * temp + 3] = -(x1 * nx + y1 * ny + z1 * nz);
            }
        }
    }

    numColPlanes = counter;

    if (gLevelIsCutscene) {
        return counter * 0x10;
    }

    for (batchIndex = 0; batchIndex < block->numberOfBatches; batchIndex++) {
        facesOffset = block->batches[batchIndex].facesOffset;
        verticesOffset = block->batches[batchIndex].verticesOffset;
        nextFacesOffset = block->batches[batchIndex + 1].facesOffset;

        if (block->batches[batchIndex].flags & RENDER_NO_COLLISION) {
            facesOffset = nextFacesOffset;
        }

        for (triIndex = facesOffset; triIndex < nextFacesOffset; triIndex++) {
            if (block->triangles[triIndex].flags & TRI_FLAG_80) {
                continue;
            }

            colPlaneIndex = block->collisionFacets[triIndex].basePlaneIndex;
            nx = block->collisionPlanes[4 * colPlaneIndex + 0];
            ny = block->collisionPlanes[4 * colPlaneIndex + 1];
            nz = block->collisionPlanes[4 * colPlaneIndex + 2];

            for (i = 0; i < 3; i++) {
                next = i + 1;
                if (next > 2) {
                    next = 0;
                }

                vertIndex = block->triangles[triIndex].verticesArray[1 + i] + verticesOffset;
                nextVertIndex = block->triangles[triIndex].verticesArray[1 + next] + verticesOffset;

                next = block->collisionFacets[triIndex].edgeBisectorPlane[i];
                if (next < numColPlanes) {
                    {
                        s32 temp = next;
                        x5 = nx + block->collisionPlanes[4 * temp + 0];
                        y5 = ny + block->collisionPlanes[4 * temp + 1];
                        z5 = nz + block->collisionPlanes[4 * temp + 2];
                    }

                    v = &block->vertices[vertIndex];
                    x1 = v->x;
                    y1 = v->y;
                    z1 = v->z;

                    v = &block->vertices[nextVertIndex];
                    x2 = v->x;
                    y2 = v->y;
                    z2 = v->z;

                    x3 = x5 * 10.0f + x1;
                    y3 = y5 * 10.0f + y1;
                    z3 = z5 * 10.0f + z1;

                    x5 = y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2);
                    y5 = z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2);
                    z5 = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);

                    mag = sqrtf(x5 * x5 + y5 * y5 + z5 * z5);
                    if (mag > 0.0) {
                        x5 /= mag;
                        y5 /= mag;
                        z5 /= mag;
                    }

                    if (next != colPlaneIndex) {
                        for (j = 0; j < 3; j++) {
                            if (colPlaneIndex == block->collisionFacets[next].edgeBisectorPlane[j]) {
                                block->collisionFacets[next].edgeBisectorPlane[j] = counter | 0x8000;
                            }
                        }
                    }

                    block->collisionFacets[triIndex].edgeBisectorPlane[i] = counter;

                    {
                        s32 temp = counter++;
                        block->collisionPlanes[4 * temp + 0] = x5;
                        block->collisionPlanes[4 * temp + 1] = y5;
                        block->collisionPlanes[4 * temp + 2] = z5;
                        block->collisionPlanes[4 * temp + 3] = -(x1 * x5 + y1 * y5 + z1 * z5);
                    }
                }
            }
        }
    }
    return counter * 0x10;
}

typedef struct unk8002D30C_a0 {
    u8 pad00[0x04];
    struct unk8002D30C_a0 *unk04;
    struct unk8002D30C_a0 *unk08;
} unk8002D30C_a0;

void trackMakeAbsolute(unk8002D30C_a0 *arg0, s32 arg1) {
    while (1) {
        if (!arg0) {
            return;
        }
        if (arg0->unk04) {
            arg0->unk04 = (unk8002D30C_a0 *) ((ptrint_t) arg0->unk04 + arg1);
        }
        if (arg0->unk08) {
            arg0->unk08 = (unk8002D30C_a0 *) ((ptrint_t) arg0->unk08 + arg1);
        }

        trackMakeAbsolute(arg0->unk04, arg1);
        arg0 = arg0->unk08;
    }
}

/**
 * Render the shadow of an object on the ground as a decal.
 * Can subdivide itself to wrap around the terrain properly, as the N64 lacks stencil buffering.
 */
void shadow_render(Object *obj, ShadowData *shadow) {
    s32 i;
    s32 numVerts;
    s32 numTris;
    Vertex *vtx;
    Triangle *tri;
    s32 flags;
    UNUSED s32 tri2;
    s32 vtx2;
    s32 vtxCount;
    s32 triCount;
    s32 alpha;

    if (obj->header->shadowGroup) {
        if (shadow->meshStart != -1 && gDisableShadows == FALSE) {
            gShadowIndex = gShadowHeapFlip;
            if (obj->header->shadowGroup == SHADOW_SCENERY) {
                gShadowIndex += 2;
            }
            i = shadow->meshStart;
            gCurrShadowHeapData = gShadowHeapData[gShadowIndex];
            gCurrShadowTris = gShadowHeapTris[gShadowIndex];
            gCurrShadowVerts = gShadowHeapVerts[gShadowIndex];
            alpha = gCurrShadowVerts[gCurrShadowHeapData[i].vtxCount].a;
            flags = RENDER_FOG_ACTIVE | RENDER_Z_COMPARE;
            if (alpha == 0 || obj->opacity == 0) {
                i = shadow->meshEnd; // It'd be easier to just return...
            } else if (alpha != 255 || obj->opacity != 255) {
                flags = RENDER_FOG_ACTIVE | RENDER_SEMI_TRANSPARENT | RENDER_Z_COMPARE;
                alpha = (obj->opacity * alpha) >> 8;
                gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, alpha);
            }
            while (i < shadow->meshEnd) {
                material_set_no_tex_offset(&gTrackDL, gCurrShadowHeapData[i].texture, flags);
                // I hope we can clean this part up.
                tri2 = triCount = gCurrShadowHeapData[i].triCount; // Fakematch
                vtx2 = vtxCount = gCurrShadowHeapData[i].vtxCount;
                numTris = gCurrShadowHeapData[i + 1].triCount - triCount;
                numVerts = gCurrShadowHeapData[i + 1].vtxCount - vtxCount;
                tri = &gCurrShadowTris[triCount];
                vtx = &gCurrShadowVerts[vtx2];
                gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(vtx), numVerts, 0);
                gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(tri), numTris, 1);
                i++;
            }

            if (flags != RENDER_Z_COMPARE) {
                gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, 255);
            }
        }
    }
}

/**
 * Used only by cars, render a texture on the surface of the water where the car is
 * to give the wave effect. Works almost identically to shadows, since water can be wavy.
 */
void watereffect_render(Object *obj, WaterEffect *effect) {
    s32 i;
    s32 numVerts;
    s32 numTris;
    Vertex *vtx;
    Triangle *tri;
    s32 flags;
    UNUSED s32 triCount;
    UNUSED s32 vtxCount;

    if (obj->header->waterEffectGroup) {
        if (effect->meshStart != -1 && gDisableShadows == FALSE) {
            gWaterEffectIndex = gShadowHeapFlip;
            i = effect->meshStart;
            if (obj->header->waterEffectGroup == SHADOW_SCENERY) {
                gWaterEffectIndex += 2;
                if (get_distance_to_active_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position) >
                    768.0f) {
                    i = effect->meshEnd; // Just return.
                }
            }
            flags = RENDER_FOG_ACTIVE | RENDER_Z_COMPARE;
            gCurrShadowHeapData = gShadowHeapData[gWaterEffectIndex];
            gCurrShadowTris = gShadowHeapTris[gWaterEffectIndex];
            gCurrShadowVerts = gShadowHeapVerts[gWaterEffectIndex];
            while (i < effect->meshEnd) {
                material_set_no_tex_offset(&gTrackDL, gCurrShadowHeapData[i].texture, flags);
                triCount = gCurrShadowHeapData[i].triCount; // Fakematch
                vtxCount = gCurrShadowHeapData[i].vtxCount; // Fakematch
                numTris = gCurrShadowHeapData[i + 1].triCount - gCurrShadowHeapData[i].triCount;
                numVerts = gCurrShadowHeapData[i + 1].vtxCount - gCurrShadowHeapData[i].vtxCount;
                tri = &gCurrShadowTris[gCurrShadowHeapData[i].triCount];
                vtx = &gCurrShadowVerts[gCurrShadowHeapData[i].vtxCount];
                gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(vtx), numVerts, 0);
                gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(tri), numTris, 1);
                i++;
            }
        }
    }
}

/**
 * Updates shadow and water effect properties for each relevant object in the scene.
 * The first argument decides whether to update shadows for static objects or moving objects.
 * In multiplayer, only important objects get shadows.
 */
void shadow_update(s32 group, s32 waterGroup, s32 updateRate) {
    s32 objIndex;
    s32 objectCount;
    Object *obj;
    ObjectHeader *objHeader;
    f32 dist;
    s32 radius;
    s32 numViewports;
    Object **objects;
    s32 skipShading;
    TextureHeader *waterTex;
    ShadowData *shadow;
    WaterEffect *waterEffect;
    s32 playerIndex;

    gShadowIndex = gShadowHeapFlip;
    if (group == SHADOW_SCENERY) {
        gShadowIndex += 2;
    }
    gCurrShadowTris = (Triangle *) gShadowHeapTris[gShadowIndex];
    gCurrShadowVerts = (Vertex *) gShadowHeapVerts[gShadowIndex];
    gCurrShadowHeapData = (ShadowHeapProperties *) gShadowHeapData[gShadowIndex];
    gShadowTail = 0;
    gNewShadowTriCount = 0;
    gNewShadowVtxCount = 0;
    numViewports = cam_get_viewport_layout();
    objects = objGetObjList(&objIndex, &objectCount);
    while (objIndex < objectCount) {
        obj = objects[objIndex];
        objHeader = obj->header;
        waterEffect = obj->waterEffect;
        shadow = obj->shadow;
        objIndex += 1;
        if ((obj->trans.flags & OBJ_FLAGS_PARTICLE)) {
            continue;
        }
        if (shadow != NULL && shadow->scale > 0.0f && group == objHeader->shadowGroup) {
            shadow->meshStart = -1;
        }
        if (obj->trans.flags & OBJ_FLAGS_INVISIBLE) {
            shadow = NULL;
        }
        if ((shadow != NULL && objHeader->shadowGroup == SHADOW_ACTORS) ||
            (waterEffect != NULL && objHeader->waterEffectGroup == SHADOW_ACTORS)) {
            dist = get_distance_to_active_camera(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
        } else {
            dist = 0;
        }
        if (shadow != NULL && shadow->scale > 0.0f && group == objHeader->shadowGroup) {
            gShadowOpacity = 1.0f;
            shadow->meshStart = -1;
            skipShading = FALSE;
            // Multiplayer
            if (objHeader->shadowGroup == SHADOW_ACTORS && numViewports >= TWO_PLAYERS &&
                numViewports <= FOUR_PLAYERS) {
                if (obj->behaviorId == BHV_RACER) {
                    playerIndex = obj->racer->playerIndex;
                    if (playerIndex != PLAYER_COMPUTER) {
                        shadow_generate(obj, FALSE);
                        skipShading = TRUE;
                    }
                } else if (obj->behaviorId == BHV_WEAPON) {
                    shadow_generate(obj, FALSE);
                    skipShading = TRUE;
                }
            } else { // Single Player
                radius = objHeader->shadowFadeMin;
                if (dist < radius) {
                    if (objHeader->shadowFadeMax < dist) {
                        gShadowOpacity = (radius - dist) / (radius - objHeader->shadowFadeMax);
                    }
                    shadow_generate(obj, FALSE);
                    skipShading = TRUE;
                }
            }
            if (skipShading == FALSE && obj->shading != NULL) {
                shadow_find_surface_under_object(obj);
            }
        }
        if (waterEffect != NULL && waterEffect->scale > 0.0f && waterGroup == objHeader->waterEffectGroup) {
            waterEffect->meshStart = -1;
            gShadowOpacity = 1.0f;
            waterTex = waterEffect->texture;
            if (waterTex != NULL && updateRate && waterTex->numOfTextures != 0x100) {
                waterEffect->textureFrame += waterEffect->animationSpeed;
                while (waterTex->numOfTextures < waterEffect->textureFrame) {
                    waterEffect->textureFrame -= waterTex->numOfTextures;
                }
            }

            // Multiplayer
            if (objHeader->shadowGroup == SHADOW_ACTORS && numViewports >= TWO_PLAYERS &&
                numViewports <= FOUR_PLAYERS) {
                if (obj->behaviorId == BHV_RACER) {
                    playerIndex = obj->racer->playerIndex;
                    if (playerIndex != PLAYER_COMPUTER) {
                        shadow_generate(obj, TRUE);
                    }
                } else if (obj->behaviorId == BHV_WEAPON) {
                    shadow_generate(obj, TRUE);
                }
            } else { // Single Player
                if (dist < objHeader->shadowFadeMin) {
                    if (objHeader->shadowFadeMax < dist) {
                        gShadowOpacity =
                            (objHeader->shadowFadeMin - dist) / (objHeader->shadowFadeMin - objHeader->shadowFadeMax);
                    }
                    shadow_generate(obj, TRUE);
                }
            }
        }
    }
    gCurrShadowHeapData[gShadowTail].triCount = gNewShadowTriCount;
    gCurrShadowHeapData[gShadowTail].vtxCount = gNewShadowVtxCount;
}

/**
 * Looks for a surface under the object within its shadow's height range, so
 * the shadow and water effect can be placed on it.
 */
void shadow_find_surface_under_object(Object *obj) {
    s32 sp94;
    s32 sp90;
    s32 blockId;
    s32 var_t3;
    s32 k;
    u32 batchFlags;
    s32 foundResult;
    s32 i;
    LevelModelSegment *block;
    Triangle *triangle;
    Vertex *vertices;
    s32 minYPos;
    s32 maxYPos;
    s32 j;

    sp94 = (s32) obj->trans.y_position + obj->header->shadowTop;
    sp90 = (s32) obj->trans.y_position + obj->header->shadowBottom;
    blockId = obj->segmentID;
    foundResult = FALSE;
    if (blockId != -1) {
        var_t3 = compute_grid_overlap_mask(&gCurrentLevelModel->segmentsBoundingBoxes[blockId],
                                           obj->trans.x_position - 16.0f, obj->trans.z_position - 16.0f,
                                           obj->trans.x_position + 16.0f, obj->trans.z_position + 16.0f);
        block = &gCurrentLevelModel->segments[blockId];
        for (i = 0; i < block->numberOfBatches && !foundResult; i++) {
            if (!(block->batches[i].flags & (RENDER_HIDDEN | RENDER_DECAL | RENDER_WATER | RENDER_NO_SHADOW))) {
                batchFlags = (block->batches[i].flags >> 19) & 7;
                vertices = &block->vertices[block->batches[i].verticesOffset];
                for (j = block->batches[i].facesOffset; j < block->batches[i + 1].facesOffset && !foundResult; j++) {
                    blockId = block->triangleGridMasks[j] & var_t3;
                    if (blockId) {}
                    if (((block->triangleGridMasks[j] & var_t3) & 0xFF) && ((block->triangleGridMasks[j] & var_t3) & 0xFF00)) {
                        triangle = &block->triangles[j];
                        minYPos = vertices[triangle->verticesArray[1]].y;
                        maxYPos = minYPos;
                        for (k = 1; k < 3; k++) {
                            if (vertices[triangle->verticesArray[k + 1]].y < minYPos) {
                                minYPos = vertices[triangle->verticesArray[k + 1]].y;
                            } else if (maxYPos < vertices[triangle->verticesArray[k + 1]].y) {
                                maxYPos = vertices[triangle->verticesArray[k + 1]].y;
                            }
                        }
                        if (maxYPos >= sp90 && sp94 >= minYPos) {
                            if (tri2d_xz_contains_point(obj->trans.x_position, obj->trans.z_position,
                                                        (Vec3s *) &vertices[triangle->verticesArray[1]],
                                                        (Vec3s *) &vertices[triangle->verticesArray[2]],
                                                        (Vec3s *) &vertices[triangle->verticesArray[3]])) {
                                foundResult = TRUE;
                                obj->shading->brightness += (((1.0f - gShadowLightLevels[batchFlags]) - obj->shading->brightness) * 0.2);
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * Generate shadow geometry for an object.
 * Handles water effects too, isWater is true.
 */
void shadow_generate(Object *obj, s32 isWater) {
    f32 dist;
    s32 yPos;
    f32 xPos;
    f32 zPos;
    UNUSED s32 *pad;
    s32 cheats;
    s32 inSegs[28];
    s32 i;
    s32 segs;
    f32 character_scale;
    s32 test;

    yPos = obj->trans.y_position;
    character_scale = 1.0f;
    if (obj->behaviorId == BHV_RACER) {
        cheats = get_filtered_cheats();
        if (cheats & CHEAT_BIG_CHARACTERS) {
            character_scale = 1.4f;
        } else if (cheats & CHEAT_SMALL_CHARACTERS) {
            character_scale = 0.714f;
        }
    }

    gNewShadowObj = obj;
    gShadowYOffset = 2.0f;

    if (isWater) {
        gShadowWavePlaneCount = 0;
        obj->waterEffect->meshStart = gShadowTail;
        gNewShadowTexture = set_animated_texture_header(obj->waterEffect->texture, obj->waterEffect->textureFrame << 8);
        gNewShadowY2 = obj->header->waterEffectTop + yPos;
        gNewShadowY1 = obj->header->waterEffectBottom + yPos;
        if (gWaveBlockCount == 0 || cam_get_viewport_layout() < VIEWPORT_LAYOUT_2_PLAYERS) {
            gShadowYOffset = 0;
        }
        gNewShadowScale = (obj->waterEffect->scale * character_scale);
        gNewShadowWidth = gNewShadowScale * 10.0f;
        gNewShadowLength = gNewShadowScale * 10.0f;
        gShadowFadeStart = -1.0f;
    } else {
        obj->shadow->meshStart = gShadowTail;
        gNewShadowTexture = obj->shadow->texture;
        gNewShadowY2 = obj->header->shadowTop + yPos;
        gNewShadowY1 = obj->header->shadowBottom + yPos;
        if (obj->behaviorId != BHV_RACER) {
            dist = obj->distanceToCamera;
            if (dist < 0.0) {
                dist = -dist;
            }
            dist -= 512.0;
            if (dist < 0.0) {
                dist = 0.0;
            }
            if (dist > 1024.0) {
                dist = 1024.0;
            }
            gShadowYOffset += (dist * 0.005f);
        }
        gNewShadowScale = (obj->shadow->scale * character_scale);
        gNewShadowWidth = gNewShadowScale * 10.0f;
        gNewShadowLength = gNewShadowScale * 10.0f;
        gShadowArea = 4.0f * gNewShadowWidth * gNewShadowLength;
        gShadowFadeStart = (obj->header->shadowBottom * 0.125f);
        if (gShadowFadeStart < 0.0f) {
            gShadowFadeStart = -gShadowFadeStart;
        }
        gShadowFadeRange = (7.0f * gShadowFadeStart);
        gShadowMinY = -0x8000;
    }
    gNewShadowScale = 144.0f / gNewShadowScale;
    xPos = gNewShadowObj->trans.x_position;
    zPos = gNewShadowObj->trans.z_position;
    segs = get_inside_segment_count_xyz(inSegs, (xPos - gNewShadowWidth), gNewShadowY1, (zPos - gNewShadowLength),
                                        (xPos + gNewShadowWidth), gNewShadowY2, (zPos + gNewShadowLength));
    gShadowTriCount = 0;
    gShadowVertexCount = 0;
    for (i = 0; i < ARRAY_COUNT(gShadowBucketCounts); i++) {
        gShadowBucketCounts[i] = 0;
    }
    gShadowCentreTri = -1;
    gShadowCentreShared = -1;
    for (i = 0; i < segs; i++) {
        if (inSegs[i] >= 0) {
            if (isWater && (gCurrentLevelModel->segments[inSegs[i]].hasWaves != 0) && (gWaveBlockCount != 0)) {
                shadow_add_wave_triangles(inSegs[i]);
            } else {
                test = compute_grid_overlap_mask(&gCurrentLevelModel->segmentsBoundingBoxes[inSegs[i]],
                                                 (obj->trans.x_position - gNewShadowWidth),  // x1
                                                 (obj->trans.z_position - gNewShadowLength), // z1
                                                 (obj->trans.x_position + gNewShadowWidth),  // x2
                                                 (obj->trans.z_position + gNewShadowLength)  // z2
                );
                shadow_add_segment_triangles(&gCurrentLevelModel->segments[inSegs[i]], test, isWater);
            }
        }
    }
    if (gShadowTriCount > 0) {
        if ((obj->shading != NULL) && isWater == FALSE) {
            obj->shading->brightness = shadow_light_level();
        }
        shadow_project_vertices();
        shadow_build_mesh();
    }
    if (isWater == FALSE) {
        obj->shadow->meshEnd = gShadowTail;
    } else {
        obj->waterEffect->meshEnd = gShadowTail;
    }
}

/**
 * Adds the triangles of a segment that the shadow rectangle overlaps to the
 * shadow mesh being built, clipped to the rectangle.
 */
void shadow_add_segment_triangles(LevelModelSegment *arg0, s32 arg1, s32 arg2) {
    unk8011C8B8 sp100[8];
    Vec2f spD0[4];
    s32 spAC;
    Triangle *triangles;
    s32 nextFacesOffset;
    Vertex *vertices;
    s32 yPos;
    s32 minY;
    s32 foundIndex;
    s32 maxY;
    s32 temp_t6;
    s32 sp88;
    s32 someCount;
    s32 i2;
    s32 i;
    s32 curFacesOffset;

    spD0[0].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    spD0[0].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    spD0[1].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    spD0[1].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    spD0[2].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    spD0[2].y = gNewShadowObj->trans.z_position - gNewShadowLength;
    spD0[3].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    spD0[3].y = gNewShadowObj->trans.z_position - gNewShadowLength;

    for (spAC = 0; spAC < arg0->numberOfBatches; spAC++) {
        if ((arg2 && (arg0->batches[spAC].flags & RENDER_WATER)) ||
            (!arg2 && !(arg0->batches[spAC].flags & FLAGS_8002E904))) {
            curFacesOffset = arg0->batches[spAC].facesOffset;
            nextFacesOffset = arg0->batches[spAC + 1].facesOffset;
            vertices = &arg0->vertices[arg0->batches[spAC].verticesOffset];
            sp88 = (arg0->batches[spAC].flags >> 0x13) & 7;
            for (; curFacesOffset < nextFacesOffset; curFacesOffset++) {
                if (((arg0->triangleGridMasks[curFacesOffset] & arg1) & 0xFF) && ((arg0->triangleGridMasks[curFacesOffset] & arg1) & 0xFF00)) {
                    triangles = &arg0->triangles[curFacesOffset];
                    maxY = minY = vertices[triangles->verticesArray[1]].y;
                    for (i = 1; i < 3; i++) {
                        yPos = vertices[triangles->verticesArray[i + 1]].y;
                        if (yPos < minY) {
                            minY = yPos;
                        } else if (maxY < yPos) {
                            maxY = yPos;
                        }
                    }
                    if (gNewShadowY2 >= minY) {
                        if (maxY >= gNewShadowY1) {
                            for (i = 0; i < 3; i++) {
                                sp100[i].x = vertices[triangles->verticesArray[i + 1]].x;
                                sp100[i].z = vertices[triangles->verticesArray[i + 1]].z;
                                sp100[i].unkC_union.s.unkE = -1;
                            }
                            // @note while the cast to Vec4f is incorrect, polygon_overlaps_rect x uses unk0 and z which
                            // are both floats so this is fine as the size is the same
                            if (polygon_overlaps_rect(spD0[2].x, spD0[2].y, spD0[0].x, spD0[0].y, 3, (Vec4f *) sp100) != 0) {
                                temp_t6 = arg0->collisionFacets[curFacesOffset].basePlaneIndex * 4;
                                gShadowCurrentPlane = (unk8011C8B8 *) &(arg0->collisionPlanes)[temp_t6];
                                if (arg0->collisionPlanes[temp_t6 + 1] != 0) {
                                    if (gShadowFadeStart > 0.0f) {
                                        shadow_check_centre_in_triangle(sp100);
                                    }
                                    someCount = clip_polygon_to_rect(3, sp100, 4, spD0);
                                    if (someCount >= 3) {
                                        gShadowTris[gShadowTriCount].bucketVertexMask = 0;
                                        for (i2 = 0; i2 < someCount; i2++) {
                                            if (sp100[i2].unkC_union.s.unkE < 0) {
                                                foundIndex = -1;
                                                i = 0;
                                                while ((i < gShadowVertexCount) && (foundIndex == -1)) {
                                                    if ((gShadowVertices[i].x == sp100[i2].x) &&
                                                        (gShadowVertices[i].z == sp100[i2].z)) {
                                                        foundIndex = i;
                                                    }
                                                    i++;
                                                }
                                                if (foundIndex == -1) {
                                                    gShadowVertices[gShadowVertexCount].x = sp100[i2].x;
                                                    gShadowVertices[gShadowVertexCount].unkC = gShadowCurrentPlane;
                                                    gShadowVertices[gShadowVertexCount].z = sp100[i2].z;
                                                    gShadowTris[gShadowTriCount].vertexIndices[i2] = gShadowVertexCount++;
                                                } else {
                                                    gShadowTris[gShadowTriCount].vertexIndices[i2] = foundIndex;
                                                }
                                            } else {
                                                gShadowTris[gShadowTriCount].vertexIndices[i2] = sp100[i2].unkC_union.s.unkE;
                                                gShadowTris[gShadowTriCount].bucketVertexMask |= 1 << i2;
                                            }
                                        }
                                        gShadowTris[gShadowTriCount].vertexCount = someCount;
                                        gShadowTris[gShadowTriCount].lightLevel = sp88;
                                        gShadowTriCount += 1;
                                        if ((gShadowCentreTri >= 0) && (sp88 != gShadowCentreTri)) {
                                            gShadowCentreShared = 0;
                                        }
                                        gShadowCentreTri = sp88;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * Adds the wave surface under the shadow rectangle to the water effect mesh.
 */
void shadow_add_wave_triangles(s32 arg0) {
    unk8011C8B8 spA8[8];
    Vec2f sp88[4];
    s32 var_v0;
    s32 var_a0;
    s32 var_a1;
    s32 temp_v0;
    s32 temp_v0_3;
    s32 var_s4;
    s32 var_t1;
    s32 var_v1;
    s32 tempIdx;

    sp88[0].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    sp88[0].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    sp88[1].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    sp88[1].y = gNewShadowObj->trans.z_position + gNewShadowLength;
    sp88[2].x = gNewShadowObj->trans.x_position - gNewShadowWidth;
    sp88[2].y = gNewShadowObj->trans.z_position - gNewShadowLength;
    sp88[3].x = gNewShadowObj->trans.x_position + gNewShadowWidth;
    sp88[3].y = gNewShadowObj->trans.z_position - gNewShadowLength;
    // clang-format off
    temp_v0 = wave_cells_in_rect(
        arg0, gShadowWaveCells, &gShadowWavePlanes[gShadowWavePlaneCount],
        sp88[2].x, sp88[2].y,
        sp88[0].x, sp88[0].y
    );
    // clang-format on

    for (var_s4 = 0; var_s4 < temp_v0; var_s4++) {
        var_a0 = gShadowWaveCells[var_s4].y1;
        var_a1 = gShadowWaveCells[var_s4].y1;
        if (gShadowWaveCells[var_s4].y2 < var_a0) {
            var_a0 = gShadowWaveCells[var_s4].y2;
        } else if (var_a1 < gShadowWaveCells[var_s4].y2) {
            var_a1 = gShadowWaveCells[var_s4].y2;
        }
        if (gShadowWaveCells[var_s4].y3 < var_a0) {
            var_a0 = gShadowWaveCells[var_s4].y3;
        } else if (var_a1 < gShadowWaveCells[var_s4].y3) {
            var_a1 = gShadowWaveCells[var_s4].y3;
        }
        if (gNewShadowY2 >= var_a0) {
            if (var_a1 >= gNewShadowY1) {
                spA8[0].x = gShadowWaveCells[var_s4].x1;
                spA8[0].z = gShadowWaveCells[var_s4].z1;
                spA8[1].x = gShadowWaveCells[var_s4].x2;
                spA8[1].z = gShadowWaveCells[var_s4].z2;
                spA8[2].x = gShadowWaveCells[var_s4].x3;
                spA8[2].z = gShadowWaveCells[var_s4].z3;

                for (var_v0 = 0; var_v0 != 3; var_v0++) {
                    spA8[var_v0].unkC_union.s.unkE = -1;
                }

                gShadowCurrentPlane = &gShadowWavePlanes[gShadowWavePlaneCount + var_s4];
                temp_v0_3 = clip_polygon_to_rect(3, spA8, 4, sp88);
                if (temp_v0_3 >= 3) {
                    tempIdx = gShadowTriCount;
                    gShadowTris[tempIdx].bucketVertexMask = 0;
                    for (var_t1 = 0; var_t1 < temp_v0_3; var_t1++) {
                        if (spA8[var_t1].unkC_union.s.unkE < 0) {
                            var_a1 = -1;
                            var_v1 = 0;
                            while (var_v1 < gShadowVertexCount && var_a1 == -1) {
                                if ((gShadowVertices[var_v1].x == spA8[var_t1].x) &&
                                    (gShadowVertices[var_v1].z == spA8[var_t1].z)) {
                                    var_a1 = var_v1;
                                }
                                var_v1++;
                            }
                            if (var_a1 == -1) {
                                gShadowVertices[gShadowVertexCount].x = spA8[var_t1].x;
                                gShadowVertices[gShadowVertexCount].unkC = gShadowCurrentPlane;
                                gShadowVertices[gShadowVertexCount].z = spA8[var_t1].z;
                                gShadowTris[tempIdx].vertexIndices[var_t1] = gShadowVertexCount++;
                            } else {
                                gShadowTris[tempIdx].vertexIndices[var_t1] = var_a1;
                            }
                        } else {
                            gShadowTris[tempIdx].vertexIndices[var_t1] = spA8[var_t1].unkC_union.s.unkE;
                            gShadowTris[tempIdx].bucketVertexMask |= 1 << var_t1;
                        }
                    }
                    gShadowTriCount = tempIdx + 1;
                    gShadowTris[tempIdx].vertexCount = temp_v0_3;
                }
            }
        }
    }

    gShadowWavePlaneCount += temp_v0;
}

/**
 * Drops every shadow vertex onto the plane of the triangle it came from.
 */
void shadow_project_vertices(void) {
    f32 temp_f12;
    f32 temp_f16;
    unk8011C8B8 *var_v0;
    s32 i, j, k;
    unk8011B330 *curr;

    for (i = 0; i < gShadowVertexCount; i++) {
        var_v0 = gShadowVertices[i].unkC;
        temp_f16 = gShadowVertices[i].x * var_v0->x;
        temp_f12 = gShadowVertices[i].z * var_v0->z;
        gShadowVertices[i].y = (f32) (-(temp_f16 + temp_f12 + var_v0->unkC_union.w) / var_v0->y);
    }

    for (i = 0; i < ARRAY_COUNT(gShadowBucketCounts); i++) {
        // Bucket stride: the N64 source spells the 32-entry stride as sizeof(unk8011B330),
        // which is 0x20 there but larger on a host with 8-byte pointers.
        for (j = 0, k = i * 0x20; j < gShadowBucketCounts[i]; j++, k++) {
            var_v0 = gShadowBucketVertices[k].plane;
            temp_f16 = gShadowBucketVertices[k].x * var_v0->x;
            temp_f12 = gShadowBucketVertices[k].z * var_v0->z;
            gShadowBucketVertices[k].y = (f32) (-(temp_f16 + temp_f12 + var_v0->unkC_union.w) / var_v0->y);
        }
    }
}

/**
 * Emits the shadow (or water effect) triangles with texture coordinates from
 * the object's shadow texture, rotated with the object.
 */
void shadow_build_mesh(void) {
    s32 spAC;
    s32 var_t0;
    Triangle *tri;
    Vertex *vert;
    s32 alpha;
    // These must hold up to 7 vertices: clip_polygon_to_rect clips a triangle against 4
    // planes, and unk0 (the loop bound writing these) can reach 3+4=7. They were
    // sized [6], and the overflow corrupted gcc's stack frame (infinite loop).
    s16 sp90[8];
    s32 var_s2;
    s16 sp80[8];
    f32 temp_f18;
    f32 yRotCos;
    f32 yRotSin;
    f32 zDiff;
    f32 someY;
    f32 scale;
    f32 xDiff;
    s32 i;
    UNUSED s32 pad;

    alpha = 255;
    yRotSin = sins_f(gNewShadowObj->trans.rotation.y_rotation);
    yRotCos = coss_f(gNewShadowObj->trans.rotation.y_rotation);
    temp_f18 = gNewShadowTexture->width * 16;
    scale = ((gNewShadowTexture->width * 16) / gNewShadowWidth);
    scale *= 1.4142f;
    if (gShadowFadeStart > 0.0f) {
        someY = gNewShadowObj->trans.y_position - gShadowMinY;
        if (gShadowFadeStart < someY) {
            alpha = 255;
            alpha -= (s32) ((255 * (someY - gShadowFadeStart)) / gShadowFadeRange);
            if (alpha < 0) {
                alpha = 0;
            }
        }
        if (someY > 0.0f) {
            scale *= 1.0f + (0.005f * someY);
        }
    }
    alpha *= gShadowOpacity;
    var_s2 = 25;
    for (spAC = 0; spAC < gShadowTriCount; spAC++) {
        if ((gShadowTris[spAC].vertexCount + var_s2) >= 24) {
            gCurrShadowHeapData[gShadowTail].texture = gNewShadowTexture;
            gCurrShadowHeapData[gShadowTail].triCount = gNewShadowTriCount;
            gCurrShadowHeapData[gShadowTail].vtxCount = gNewShadowVtxCount;
            gShadowTail++;
            var_s2 = 0;
        }
        var_t0 = gShadowTris[spAC].bucketVertexMask;

        for (i = 0; i < gShadowTris[spAC].vertexCount; i++) {
            vert = &gCurrShadowVerts[gNewShadowVtxCount];
            gNewShadowVtxCount++;
            if (var_t0 & 1) {
                vert->x = gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].x;
                xDiff = gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].x - gNewShadowObj->trans.x_position;
                vert->y = (gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].y + gShadowYOffset);
                vert->z = gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].z;
                zDiff = gShadowBucketVertices[gShadowTris[spAC].vertexIndices[i]].z - gNewShadowObj->trans.z_position;
            } else {
                vert->x = gShadowVertices[gShadowTris[spAC].vertexIndices[i]].x;
                xDiff = gShadowVertices[gShadowTris[spAC].vertexIndices[i]].x - gNewShadowObj->trans.x_position;
                vert->y = (gShadowVertices[gShadowTris[spAC].vertexIndices[i]].y + gShadowYOffset);
                vert->z = gShadowVertices[gShadowTris[spAC].vertexIndices[i]].z;
                zDiff = gShadowVertices[gShadowTris[spAC].vertexIndices[i]].z - gNewShadowObj->trans.z_position;
            }
            var_t0 >>= 1;
            vert->r = 255;
            vert->g = 255;
            vert->b = 255;
            vert->a = alpha;
            sp90[i] = ((((xDiff * yRotCos) - (zDiff * yRotSin)) * scale) + temp_f18);
            sp80[i] = ((((zDiff * yRotCos) + (xDiff * yRotSin)) * scale) + temp_f18);
        }

        for (i = 1; i < (gShadowTris[spAC].vertexCount - 1); i++) {
            tri = &gCurrShadowTris[gNewShadowTriCount];
            gNewShadowTriCount++;
            tri->flags = 0x40;
            tri->vi0 = var_s2 + i;
            tri->vi1 = var_s2 + i + 1;
            tri->vi2 = var_s2;
            tri->uv0.u = sp90[i];
            tri->uv0.v = sp80[i];
            tri->uv1.u = sp90[i + 1];
            tri->uv1.v = sp80[i + 1];
            tri->uv2.u = sp90[0];
            tri->uv2.v = sp80[0];
        }
        var_s2 += gShadowTris[spAC].vertexCount;
    }
}

/**
 * The light level at the shadow, from the lighting transition points of the
 * triangles under it; feeds the object's shading.
 */
f32 shadow_light_level(void) {
    f32 var_f2;
    f32 x0, z0, x1, z1, x2, z2;
    s32 temp_t5;
    s32 var_s2;
    s32 i;

    var_f2 = 0.0f;
    if (gShadowTriCount > 0) {
        if (gShadowCentreShared != 0) {
            if (gShadowCentreTri > 0) {
                var_f2 = gShadowLightLevels[gShadowCentreTri] * gShadowArea;
            }
        } else {
            for (i = 0; i < gShadowTriCount; i++) {
                if (gShadowTris[i].lightLevel > 0) {
                    temp_t5 = gShadowTris[i].bucketVertexMask;
                    if (temp_t5 & 1) {
                        x0 = gShadowBucketVertices[gShadowTris[i].vertexIndices[0]].x;
                        z0 = gShadowBucketVertices[gShadowTris[i].vertexIndices[0]].z;
                    } else {
                        x0 = gShadowVertices[gShadowTris[i].vertexIndices[0]].x;
                        z0 = gShadowVertices[gShadowTris[i].vertexIndices[0]].z;
                    }
                    temp_t5 >>= 1;
                    if (temp_t5 & 1) {
                        x1 = gShadowBucketVertices[gShadowTris[i].vertexIndices[1]].x;
                        z1 = gShadowBucketVertices[gShadowTris[i].vertexIndices[1]].z;
                    } else {
                        x1 = gShadowVertices[gShadowTris[i].vertexIndices[1]].x;
                        z1 = gShadowVertices[gShadowTris[i].vertexIndices[1]].z;
                    }
                    temp_t5 >>= 1;
                    for (var_s2 = 2; var_s2 < gShadowTris[i].vertexCount; var_s2++) {
                        if (temp_t5 & 1) {
                            x2 = gShadowBucketVertices[gShadowTris[i].vertexIndices[var_s2]].x;
                            z2 = gShadowBucketVertices[gShadowTris[i].vertexIndices[var_s2]].z;
                        } else {
                            x2 = gShadowVertices[gShadowTris[i].vertexIndices[var_s2]].x;
                            z2 = gShadowVertices[gShadowTris[i].vertexIndices[var_s2]].z;
                        }
                        temp_t5 >>= 1;
                        var_f2 += area_triangle_2d(x0, z0, x1, z1, x2, z2) * gShadowLightLevels[gShadowTris[i].lightLevel];
                        x1 = x2;
                        z1 = z2;
                    }
                }
            }
        }

        if (gShadowArea < var_f2) {
            var_f2 = gShadowArea * 0.99f;
        }
    }
    return (gShadowArea - var_f2) / gShadowArea;
}

/**
 * Whether a polygon's bounding box overlaps the rectangle (x0, z0)-(x1, x2).
 */
s32 polygon_overlaps_rect(f32 x0, f32 z0, f32 x1, f32 x2, s32 count, Vec4f *arg5) {
    if (count > 0) {
        f32 minX = arg5[0].x;
        f32 maxX = arg5[0].x;
        f32 minZ = arg5[0].z;
        f32 maxZ = arg5[0].z;
        s32 i;

        for (i = 1; i < count; i++) {
            if (arg5[i].x < minX) {
                minX = arg5[i].x;
            } else if (maxX < arg5[i].x) {
                maxX = arg5[i].x;
            }
            if (arg5[i].z < minZ) {
                minZ = arg5[i].z;
            } else if (maxZ < arg5[i].z) {
                maxZ = arg5[i].z;
            }
        }

        if ((x0 <= maxX) && (z0 <= maxZ) && (minX <= x1) && (minZ <= x2)) {
            return -1;
        }
    }

    return 0;
}

// arg0 is always 3
// arg1 always has size 8 (that's why spE0 is also of size 8)
// arg2 is always 4
/**
 * Clips a polygon against the four edges of a rectangle (Sutherland-Hodgman),
 * returning the vertex count; the output can hold up to 8 vertices.
 */
s32 clip_polygon_to_rect(s32 arg0, unk8011C8B8 *arg1, s32 arg2, Vec2f *arg3) {
    unk8011C8B8 spE0[8];
    f32 temp_f12;
    f32 temp_f14;
    f32 temp_f16;
    f32 temp_f22;
    f32 temp_f24;
    f32 var_f2;
    s32 var_a1;
    s32 var_v1_3;
    s32 var_a0;
    s32 var_t2;
    UNUSED s32 var_t5;
    s32 var_v0;
    s32 var_v1;
    s32 var_t1;
    s32 var_s2;
    unk8011C8B8 *var_s0;
    unk8011C8B8 *var_s3;
    unk8011C8B8 *swap; // swap var for var_s0 and var_s3

    var_s3 = arg1;
    var_s0 = spE0;
    var_s2 = arg0;
    var_v0 = 0;

    while (var_v0 < arg2 && var_s2 >= 3) {
        var_v1 = var_v0 + 1;
        if (var_v1 >= arg2) {
            var_v1 = 0;
        }

        temp_f12 = arg3[var_v1].y - arg3[var_v0].y;
        temp_f14 = -(arg3[var_v1].x - arg3[var_v0].x);
        if (arg3[var_v0].x < arg3[var_v1].x) {
            var_f2 = (temp_f12 * arg3[var_v0].x) + (arg3[var_v0].y * temp_f14);
            var_f2 = -var_f2;
        } else {
            var_f2 = (temp_f12 * arg3[var_v1].x) + (arg3[var_v1].y * temp_f14);
            var_f2 = -var_f2;
        }

        for (var_t1 = 0, var_t2 = 0; var_t1 < var_s2; var_t1++) {
            var_v1 = var_t1 + 1;
            if (var_v1 >= var_s2) {
                var_v1 = 0;
            }
            temp_f16 = (temp_f12 * var_s3[var_t1].x) + (var_s3[var_t1].z * temp_f14) + var_f2;
            temp_f22 = (temp_f12 * var_s3[var_v1].x) + (var_s3[var_v1].z * temp_f14) + var_f2;
            if ((temp_f16 >= 0.0f && temp_f22 < 0.0f) || (temp_f16 < 0.0f && temp_f22 >= 0.0f)) {
                var_a0 = gShadowBucketCounts[var_v0];
                var_a1 = -1;
                var_v1_3 = var_v0 << 5;
                while (var_a0 > 0 && var_a1 < 0) {
                    if ((gShadowBucketVertices[var_v1_3].edgeStartX == var_s3[var_t1].x) &&
                        (gShadowBucketVertices[var_v1_3].edgeStartZ == var_s3[var_t1].z) &&
                        (gShadowBucketVertices[var_v1_3].edgeEndX == var_s3[var_v1].x) &&
                        (gShadowBucketVertices[var_v1_3].edgeEndZ == var_s3[var_v1].z)) {
                        var_a1 = var_v1_3;
                    } else if ((gShadowBucketVertices[var_v1_3].edgeStartX == var_s3[var_v1].x) &&
                               (gShadowBucketVertices[var_v1_3].edgeStartZ == var_s3[var_v1].z) &&
                               (gShadowBucketVertices[var_v1_3].edgeEndX == var_s3[var_t1].x) &&
                               (gShadowBucketVertices[var_v1_3].edgeEndZ == var_s3[var_t1].z)) {
                        var_a1 = var_v1_3;
                    }
                    var_a0 -= 1;
                    var_v1_3++;
                }
                if (var_a1 >= 0) {
                    var_s0[var_t2].unkC_union.s.unkE = var_a1;
                    var_s0[var_t2].x = gShadowBucketVertices[var_a1].x;
                    var_s0[var_t2].z = gShadowBucketVertices[var_a1].z;
                    var_t2++;
                } else {
                    temp_f24 = temp_f16 / (temp_f16 - temp_f22);
                    var_s0[var_t2].x = var_s3[var_t1].x + ((var_s3[var_v1].x - var_s3[var_t1].x) * temp_f24);
                    var_s0[var_t2].z = var_s3[var_t1].z + ((var_s3[var_v1].z - var_s3[var_t1].z) * temp_f24);
                    if (gShadowBucketCounts[var_v0] > 0x1F) {
                        gShadowBucketCounts[var_v0] = 0x1F;
                    }
                    var_v1_3 = gShadowBucketCounts[var_v0] + (var_v0 << 5);
                    gShadowBucketVertices[var_v1_3].edgeStartX = var_s3[var_t1].x;
                    gShadowBucketVertices[var_v1_3].edgeStartZ = var_s3[var_t1].z;
                    gShadowBucketVertices[var_v1_3].edgeEndX = var_s3[var_v1].x;
                    gShadowBucketVertices[var_v1_3].edgeEndZ = var_s3[var_v1].z;
                    gShadowBucketVertices[var_v1_3].x = var_s0[var_t2].x;
                    gShadowBucketVertices[var_v1_3].z = var_s0[var_t2].z;
                    gShadowBucketVertices[var_v1_3].plane = gShadowCurrentPlane;
                    gShadowBucketCounts[var_v0]++;
                    var_s0[var_t2].unkC_union.s.unkE = var_v1_3;
                    var_t2++;
                }
            }
            if (temp_f22 <= 0.0f) {
                var_s0[var_t2].unkC_union.s.unkE = var_s3[var_v1].unkC_union.s.unkE;
                var_s0[var_t2].x = var_s3[var_v1].x;
                var_s0[var_t2].z = var_s3[var_v1].z;
                var_t2++;
            }
        }
        var_s2 = var_t2;
        var_v0++;

        swap = var_s3;
        var_s3 = var_s0;
        var_s0 = swap;
    }

    if (var_s2 >= 3) {
        if (var_s3 != arg1) {
            for (var_t1 = 0; var_t1 < var_s2; var_t1++) {
                arg1[var_t1].x = var_s3[var_t1].x;
                arg1[var_t1].z = var_s3[var_t1].z;
                arg1[var_t1].unkC_union.s.unkE = var_s3[var_t1].unkC_union.s.unkE;
            }
        }
    } else {
        var_s2 = 0;
    }
    return var_s2;
}

/**
 * Marks whether the shadow's centre falls inside the triangle, so the light
 * level and shadow placement can prefer that triangle.
 */
void shadow_check_centre_in_triangle(unk8011C8B8 arg0[3]) {
    f32 diff;
    f32 shadowX;
    f32 shadowZ;
    f32 var_f6;
    f32 var_f8;
    f32 var_f10;
    s32 found1;
    s32 found2;
    s32 found3;

    found1 = FALSE;
    found2 = FALSE;
    found3 = FALSE;

    shadowX = gNewShadowObj->trans.x_position;
    shadowZ = gNewShadowObj->trans.z_position;

    diff = (shadowX - arg0[0].x) * (arg0[1].z - arg0[0].z) - (arg0[1].x - arg0[0].x) * (shadowZ - arg0[0].z);
    if (diff >= 0.0f) {
        found1 = TRUE;
    }

    diff = (shadowX - arg0[1].x) * (arg0[2].z - arg0[1].z) - (arg0[2].x - arg0[1].x) * (shadowZ - arg0[1].z);
    if (diff >= 0.0f) {
        found2 = TRUE;
    }

    if (found1 != found2) {
        return;
    }

    diff = (shadowX - arg0[2].x) * (arg0[0].z - arg0[2].z) - (arg0[0].x - arg0[2].x) * (shadowZ - arg0[2].z);
    if (diff >= 0.0f) {
        found3 = TRUE;
    }
    if (found2 != found3) {
        return;
    }

    diff = -(gShadowCurrentPlane->x * shadowX + gShadowCurrentPlane->z * shadowZ + gShadowCurrentPlane->unkC_union.w) / gShadowCurrentPlane->y;
    if (gShadowMinY < diff) {
        gShadowMinY = diff;
    }
}

/**
 * Instantly update current fog properties.
 * Official Name: trackSetFog
 */
void set_fog(s32 fogIdx, s16 near, s16 far, u8 red, u8 green, u8 blue) {
    s32 tempNear;
    FogData *fogData;

    fogData = &gFogData[fogIdx];

    if (far < near) {
        tempNear = near;
        near = far;
        far = tempNear;
    }

    if (far > 1023) {
        far = 1023;
    }
    if (near >= far - 5) {
        near = far - 5;
    }

    fogData->addFog.near = 0;
    fogData->addFog.far = 0;
    fogData->addFog.r = 0;
    fogData->addFog.g = 0;
    fogData->addFog.b = 0;
    fogData->fog.r = red << 16;
    fogData->fog.g = green << 16;
    fogData->fog.b = blue << 16;
    fogData->fog.near = near << 16;
    fogData->fog.far = far << 16;
    fogData->intendedFog.r = red;
    fogData->intendedFog.g = green;
    fogData->intendedFog.b = blue;
    fogData->intendedFog.near = near;
    fogData->intendedFog.far = far;
    fogData->switchTimer = 0;
    fogData->fogChanger = NULL;
}

/**
 * Writes the current fog settings to the arguments.
 * Pre-shifts the data, so the raw values are correct.
 * Official Name: trackGetFog
 */
void get_fog_settings(s32 playerID, s16 *near, s16 *far, u8 *r, u8 *g, u8 *b) {
    *near = gFogData[playerID].fog.near >> 16;
    *far = gFogData[playerID].fog.far >> 16;
    *r = gFogData[playerID].fog.r >> 16;
    *g = gFogData[playerID].fog.g >> 16;
    *b = gFogData[playerID].fog.b >> 16;
}

/**
 * Sets the fog of the player ID to the default values.
 * Current fog attributes are rightshifted 16 bytes.
 * Official Name: trackSetFogOff
 */
void reset_fog(s32 playerID) {
    gFogData[playerID].addFog.near = 0;
    gFogData[playerID].addFog.far = 0;
    gFogData[playerID].addFog.r = 0;
    gFogData[playerID].addFog.g = 0;
    gFogData[playerID].addFog.b = 0;
    gFogData[playerID].fog.near = 1018 << 16;
    gFogData[playerID].fog.far = 1023 << 16;
    gFogData[playerID].intendedFog.r = gFogData[playerID].fog.r >> 16;
    gFogData[playerID].intendedFog.g = gFogData[playerID].fog.g >> 16;
    gFogData[playerID].intendedFog.b = gFogData[playerID].fog.b >> 16;
    gFogData[playerID].intendedFog.near = 1018;
    gFogData[playerID].intendedFog.far = 1023;
    gFogData[playerID].switchTimer = 0;
    gFogData[playerID].fogChanger = NULL;
}

/**
 * If the fog override timer is active, apply that and slowly degrade.
 * Otherwise, set the current fog to the intended fog settings.
 */
void update_fog(s32 viewportCount, s32 updateRate) {
    s32 i;
    for (i = 0; i < viewportCount; i++) {
        if (gFogData[i].switchTimer > 0) {
            if (updateRate < gFogData[i].switchTimer) {
                gFogData[i].fog.r += gFogData[i].addFog.r * updateRate;
                gFogData[i].fog.g += gFogData[i].addFog.g * updateRate;
                gFogData[i].fog.b += gFogData[i].addFog.b * updateRate;
                gFogData[i].fog.near += gFogData[i].addFog.near * updateRate;
                gFogData[i].fog.far += gFogData[i].addFog.far * updateRate;
                gFogData[i].switchTimer -= updateRate;
            } else {
                gFogData[i].fog.r = gFogData[i].intendedFog.r << 16;
                gFogData[i].fog.g = gFogData[i].intendedFog.g << 16;
                gFogData[i].fog.b = gFogData[i].intendedFog.b << 16;
                gFogData[i].fog.near = gFogData[i].intendedFog.near << 16;
                gFogData[i].fog.far = gFogData[i].intendedFog.far << 16;
                gFogData[i].switchTimer = 0;
            }
        }
    }
}

/**
 * Sets the fog settings for the active viewport based on the parameters of the environment data.
 */
void apply_fog(s32 playerID) {
    gDPSetFogColor(gTrackDL++, gFogData[playerID].fog.r >> 0x10, gFogData[playerID].fog.g >> 0x10,
                   gFogData[playerID].fog.b >> 0x10, 0xFF);
    gSPFogPosition(gTrackDL++, (gFogData[playerID].fog.near >> 0x10), (gFogData[playerID].fog.far >> 0x10));
}

/**
 * Sets the active viewport's fog target when passed through.
 * Used in courses to make less, or more dense.
 * @bug: Timer doesn't account for PAL, meaning fog will scroll 20% slower on PAL systems.
 * Official Name: trackChangeFog
 */
void obj_loop_fogchanger(Object *obj, UNUSED s32 updateRate) {
    s32 nearTemp;
    s32 fogNear;
    s32 views;
    s32 playerIndex;
    s32 index;
    UNUSED s32 pad;
    s32 fogFar;
    s32 i;
    s32 fogR;
    s32 fogG;
    s32 fogB;
    f32 x;
    f32 z;
    s32 switchTimer;
    LevelObjectEntry_FogChanger *fogChanger;
    Object **racers;
    Object_Racer *racer;
    UNUSED s32 pad2;
    FogData *fog;
    Camera *camera;

    racers = NULL;
    fogChanger = (LevelObjectEntry_FogChanger *) obj->level_entry;
    camera = NULL;

    if (check_if_showing_cutscene_camera()) {
        camera = cam_get_cameras();
        views = cam_get_viewport_layout() + 1;
    } else {
        racers = get_racer_objects(&views);
    }

    for (i = 0; i < views; i++) {
        index = PLAYER_COMPUTER;
        if (racers != NULL) {
            racer = racers[i]->racer;
            playerIndex = racer->playerIndex;
            if (playerIndex >= PLAYER_ONE && playerIndex <= PLAYER_FOUR && obj != gFogData[playerIndex].fogChanger) {
                index = playerIndex;
                x = racers[i]->trans.x_position;
                z = racers[i]->trans.z_position;
            }
        } else if (i <= PLAYER_FOUR && obj != gFogData[i].fogChanger) {
            index = i;
            x = camera[i].trans.x_position;
            z = camera[i].trans.z_position;
        }
        if (index != PLAYER_COMPUTER) {
            x -= obj->trans.x_position;
            z -= obj->trans.z_position;
            if (1) {} // Fakematch
            if ((x * x) + (z * z) < obj->properties.distance.radius) {
                fogNear = fogChanger->near;
                fogFar = fogChanger->far;
                fogR = fogChanger->r;
                fogG = fogChanger->g;
                fogB = fogChanger->b;
                switchTimer = fogChanger->switchTimer;
                // Swap near and far if they're the wrong way around.
                if (fogFar < fogNear) {
                    nearTemp = fogNear;
                    fogNear = fogFar;
                    fogFar = nearTemp;
                }
                if (fogFar > 1023) {
                    fogFar = 1023;
                }
                if (fogNear >= fogFar - 5) {
                    fogNear = fogFar - 5;
                }
                fog = &gFogData[index];
                fog->intendedFog.r = fogR;
                fog->intendedFog.g = fogG;
                fog->intendedFog.b = fogB;
                fog->intendedFog.near = fogNear;
                fog->intendedFog.far = fogFar;
                fog->addFog.r = ((fogR << 16) - fog->fog.r) / switchTimer;
                fog->addFog.g = ((fogG << 16) - fog->fog.g) / switchTimer;
                fog->addFog.b = ((fogB << 16) - fog->fog.b) / switchTimer;
                fog->addFog.near = ((fogNear << 16) - fog->fog.near) / switchTimer;
                fog->addFog.far = ((fogFar << 16) - fog->fog.far) / switchTimer;
                fog->switchTimer = switchTimer;
                fog->fogChanger = obj;
            }
        }
    }
}

/**
 * Set the fog properties from the current values to the target, over a time specified by switchTimer.
 * @bug: Timer doesn't account for PAL, meaning fog will scroll 20% slower on PAL systems.
 * Official Name: trackFadeFog
 */
void slowly_change_fog(s32 fogIdx, s32 red, s32 green, s32 blue, s32 near, s32 far, s32 switchTimer) {
    s32 temp;
    FogData *fogData;

    fogData = &gFogData[fogIdx];

    if (far < near) {
        temp = near;
        near = far;
        far = temp;
    }

    if (far > 1023) {
        far = 1023;
    }
    if (near >= far - 5) {
        near = far - 5;
    }

    fogData->intendedFog.r = red;
    fogData->intendedFog.g = green;
    fogData->intendedFog.b = blue;
    fogData->intendedFog.near = near;
    fogData->intendedFog.far = far;
    fogData->addFog.r = ((red << 16) - fogData->fog.r) / switchTimer;
    fogData->addFog.g = ((green << 16) - fogData->fog.g) / switchTimer;
    fogData->addFog.b = ((blue << 16) - fogData->fog.b) / switchTimer;
    fogData->addFog.near = ((near << 16) - fogData->fog.near) / switchTimer;
    fogData->addFog.far = ((far << 16) - fogData->fog.far) / switchTimer;
    fogData->switchTimer = switchTimer;
    fogData->fogChanger = NULL;
}

/**
 * Updates the stored perspective of the camera, as well as the envmap values derived from it.
 */
UNUSED void update_perspective_and_envmap(void) {
    gSceneActiveCamera = cam_get_active_camera();
    compute_scene_camera_transform_matrix();
    update_envmap_position((f32) gScenePerspectivePos.x / 65536.0f, (f32) gScenePerspectivePos.y / 65536.0f,
                           (f32) gScenePerspectivePos.z / 65536.0f);
}

/**
 * Take the current camera position and calculate the perspective position, for envmapping.
 */
void compute_scene_camera_transform_matrix(void) {
    MtxF mtx;
    ObjectTransform trans;

    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = -65536.0f;

    trans.rotation.z_rotation = gSceneActiveCamera->trans.rotation.z_rotation;
    trans.rotation.x_rotation = gSceneActiveCamera->trans.rotation.x_rotation;
    trans.rotation.y_rotation = gSceneActiveCamera->trans.rotation.y_rotation;
    trans.x_position = 0.0f;
    trans.y_position = 0.0f;
    trans.z_position = 0.0f;
    trans.scale = 1.0f;

    mtxf_from_transform(&mtx, &trans);
    mtxf_transform_point(mtx, x, y, z, &x, &y, &z);

    // Store x/y/z as integers
    gScenePerspectivePos.x = x;
    gScenePerspectivePos.y = y;
    gScenePerspectivePos.z = z;
}
