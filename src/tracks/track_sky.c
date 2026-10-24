#include "tracks_internal.h"

static u8 gFlashyBgTriIndices[24] = {
    0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4, 5,
    6, 1, 1, 0, 5, 3, 2, 7, 7, 8, 3
    // There may or may not be extra zeroes here.
};

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
    ColourCycle *pad2;
    ColourCycle *var_t2;
    ColourCycle *levelHeader;
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

    var_v0 = ((s32) (camera->trans.x_position * ((scaledXCos * 0.25f) / var_a1)) +
              (gCurrentLevelHeader2->skyTexScrollU >> 4)) &
             uCoordMask;
    var_v1 = ((s32) (camera->trans.z_position * ((scaledXCos * 0.25f) / var_a2)) +
              (gCurrentLevelHeader2->skyTexScrollV >> 4)) &
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

    if ((u32) ASSET_SLOT(var_t2) != -1U) {
        levelHeader = gCurrentLevelHeader2->colourCycles[1];
        if ((u32) ASSET_SLOT(levelHeader) == -1U) {
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
