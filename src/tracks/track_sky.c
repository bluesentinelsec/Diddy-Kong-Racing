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
    s32 texFrame;

    segment = gCurrentLevelModel->segments;
    for (segmentNumber = 0; segmentNumber < gCurrentLevelModel->numberOfSegments; segmentNumber++) {
        batch = segment[segmentNumber].batches;
        for (batchNumber = 0; batchNumber < segment[segmentNumber].numberOfBatches; batchNumber++) {
            if (batch[batchNumber].flags & RENDER_TEX_ANIM) {
                if (batch[batchNumber].textureIndex != 0xFF) {
                    texture = gCurrentLevelModel->textures[batch[batchNumber].textureIndex].texture;
                    if (texture->numOfTextures != 0x100 && texture->frameAdvanceDelay) {
                        texFrame = batch[batchNumber].texOffset << 6;
                        if (batch[batchNumber].flags & RENDER_UNK_80000000) {
                            texFrame |= batch[batchNumber].miscData;
                            tex_animate_texture(texture, &batch[batchNumber].flags, &texFrame, updateRate);
                            batch[batchNumber].miscData = texFrame & 0x3F;
                        } else {
                            tex_animate_texture(texture, &batch[batchNumber].flags, &texFrame, updateRate);
                        }
                        batch[batchNumber].texOffset = (texFrame >> 6) & 0xFF;
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

/**
 * Flashy background: the corner and centre positions of the sky plane's vertices around the camera,
 * rotated by the camera's pitch. Indices 0-3 are the inner square, 4 its centre and 5-8 the outer ring.
 */
static void trackbg_flashy_positions(f32 xSin, f32 xCos, f32 *xPositions, f32 *zPositions) {
    f32 scaledXSin;
    f32 scaledXCos;
    UNUSED f32 doubleScaledXSin;

    scaledXSin = xSin * 1280.0f;
    scaledXCos = xCos * 1280.0f;
    doubleScaledXSin = 2.0f * scaledXSin;
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
}

/**
 * Flashy background: the texture coordinates of the nine sky vertices, scrolled with the camera's
 * position and the level's sky scroll, and rotated with its pitch to match the positions.
 */
static void trackbg_flashy_tex_coords(Camera *camera, TextureHeader *texHeader, s32 uCoordMask, s32 vCoordMask,
                                      f32 xSin, f32 xCos, s16 *uCoords, s16 *vCoords) {
    f32 scaledXCos;
    f32 vSin;
    f32 vSinCopy;
    Vec3f uCosVec;
    s32 uCentre;
    s32 vCentre;
    s32 uSpan;
    s32 vSpan;
    f32 uCos;

    scaledXCos = 1280.0f;
    uCos = scaledXCos * 0.25f;

    uSpan = texHeader->width * 16 * gCurrentLevelHeader2->skyTexScaleU;
    vSpan = texHeader->height * 16 * gCurrentLevelHeader2->skyTexScaleV;

    uCentre = ((s32) (camera->trans.x_position * ((scaledXCos * 0.25f) / uSpan)) +
               (gCurrentLevelHeader2->skyTexScrollU >> 4)) &
              uCoordMask;
    vCentre = ((s32) (camera->trans.z_position * ((scaledXCos * 0.25f) / vSpan)) +
               (gCurrentLevelHeader2->skyTexScrollV >> 4)) &
              vCoordMask;

    uCos = uSpan * xCos;
    uCosVec.z = uSpan * xCos;
    uCosVec.x = uSpan * xCos;
    vSin = vSpan * xSin;
    xCos = vSin;
    vSinCopy = vSin;

    // @fake
    vSpan = texHeader->height * 16 * gCurrentLevelHeader2->skyTexScaleV;

    uCoords[0] = (s16) (-uCos - vSinCopy) + uCentre;
    vCoords[0] = (s16) (vSin - uCos) + vCentre;
    uCoords[1] = (s16) (uCos - vSinCopy) + uCentre;
    vCoords[1] = (s16) (-uCos - vSin) + vCentre;
    uCoords[2] = (s16) (uCos + vSin) + uCentre;
    vCoords[2] = (s16) (uCos - vSin) + vCentre;
    uCoords[3] = (s16) (vSin - uCos) + uCentre;
    vCoords[3] = (s16) (uCos + vSin) + vCentre;

    uCoords[4] = uCentre;
    vCoords[4] = vCentre;

    uCoords[5] = (s16) (-uCos - (2.0f * xCos)) + uCentre;
    vCoords[5] = (s16) (vSin - (2.0f * uCos)) + vCentre;
    uCoords[6] = (s16) (uCos - (2.0f * xCos)) + uCentre;
    vCoords[6] = (s16) ((-(2.0f * uCos)) - vSin) + vCentre;
    uCoords[7] = (s16) (uCosVec.f[2] + (2.0f * xCos)) + uCentre;
    vCoords[7] = (s16) ((2.0f * uCosVec.x) - vSin) + vCentre;
    uCoords[8] = (s16) ((2.0f * xCos) - uCosVec.z) + uCentre;
    vCoords[8] = (s16) ((2.0f * uCosVec.x) + vSin) + vCentre;
}

/**
 * Flashy background: writes the nine sky vertices at the camera's position (the outer ring fading to
 * transparent) and the eight triangles from gFlashyBgTriIndices, then advances the track's
 * vertex and triangle pointers past them.
 */
static void trackbg_flashy_emit_geometry(Vertex *verts, Triangle *tris, Camera *camera, f32 *xPositions,
                                         f32 *zPositions, s16 *uCoords, s16 *vCoords) {
    s32 i;
    u8 *triIndex;
    s16 vertY;

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

    triIndex = gFlashyBgTriIndices;
    for (i = 0; i < 8; i++) {
        tris->flags = BACKFACE_DRAW;
        tris->vi0 = *triIndex;
        tris->uv0.u = uCoords[*triIndex];
        tris->uv0.v = vCoords[*triIndex];
        triIndex += 1;
        tris->vi1 = *triIndex;
        tris->uv1.u = uCoords[*triIndex];
        tris->uv1.v = vCoords[*triIndex];
        triIndex += 1;
        tris->vi2 = *triIndex;
        tris->uv2.u = uCoords[*triIndex];
        tris->uv2.v = vCoords[*triIndex];
        triIndex += 1;
        tris++;
    }

    gTrackVtxPtr = verts;
    gTrackTriPtr = tris;
}

// This function creates the flashy sky effect in the wizpig 2 race.
void trackbg_render_flashy(void) {
    Triangle *tris;
    Vertex *verts;
    s32 vCoordMask;
    s32 uCoordMask;
    s16 uCoords[9];
    s16 vCoords[9];
    f32 xCos;
    f32 xSin;
    Camera *camera;
    f32 xPositions[9];
    f32 zPositions[9];
    s32 primColour;
    s32 envColour;
    ColourCycle *colourCycle0;
    ColourCycle *colourCycle1;
    TextureHeader *texHeader;

    verts = gTrackVtxPtr;
    tris = gTrackTriPtr;

    camera = cam_get_active_camera();
    texHeader = gCurrentLevelHeader2->skyTexture;
    uCoordMask = (texHeader->width << 5) - 1;
    vCoordMask = (texHeader->height << 5) - 1;
    xSin = sins_f(-camera->trans.rotation.x);
    xCos = coss_f(-camera->trans.rotation.x);

    trackbg_flashy_positions(xSin, xCos, xPositions, zPositions);
    trackbg_flashy_tex_coords(camera, texHeader, uCoordMask, vCoordMask, xSin, xCos, uCoords, vCoords);

    mtx_world_origin(&gTrackDL, &gTrackMtxPtr);

    colourCycle0 = *gCurrentLevelHeader2->colourCycles;
    primColour = -1;

    if ((u32) ASSET_SLOT(colourCycle0) != -1U) {
        colourCycle1 = gCurrentLevelHeader2->colourCycles[1];
        if ((u32) ASSET_SLOT(colourCycle1) == -1U) {
            colourCycle1 = colourCycle0;
        }
    } else {
        colourCycle0 = NULL;
    }

    envColour = COLOUR_RGBA32(255, 255, 255, 0);
    if (colourCycle0 != NULL) {
        primColour = colourCycle0->rgba.word;
        envColour = colourCycle1->rgba.word & (~0xFF);
    }

    gfx_init_basic_xlu(&gTrackDL, 1, primColour, envColour);
    texHeader = set_animated_texture_header(texHeader, gTrackTexAnimOffset << 8);
    gDkrDmaDisplayList(gTrackDL++, OS_K0_TO_PHYSICAL(texHeader->cmd), texHeader->numberOfCommands);
    gSPVertexDKR(gTrackDL++, OS_K0_TO_PHYSICAL(gTrackVtxPtr), 9, 0);
    gSPPolygon(gTrackDL++, OS_K0_TO_PHYSICAL(gTrackTriPtr), 8, 1);
    gDPPipeSync(gTrackDL++);
    if (colourCycle0 != NULL) {
        gDPSetPrimColor(gTrackDL++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(gTrackDL++, 255, 255, 255, 0);
    }
    rendermode_reset(&gTrackDL);

    trackbg_flashy_emit_geometry(verts, tris, camera, xPositions, zPositions, uCoords, vCoords);
}

/**
 * Instead of drawing the skydome with textures, draw a solid coloured background.
 * Using different colours set in the level header, the vertices are coloured and
 * it gives the background a gradient effect.
 */
void trackbg_render_gradient(void) {
    s16 z;
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
