#include "waves_internal.h"

/************ .data ************/

f32 *gWaveHeightTable = NULL;     // indexed by values of gWaveHeightIndices
Vec2s *gWaveHeightIndices = NULL; // holds some sort of index?
TexCoords *gWaveUVTable = NULL;
f32 *gWaveHeightBuffers[9] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

Vertex *gWaveVertices[4] = { NULL, NULL, NULL, NULL };
Triangle *gWaveTriangles[4] = { NULL, NULL, NULL, NULL };
Triangle gWaveQuadTriangles[4] = {
    { { { BACKFACE_DRAW, 0, 2, 1 } }, { { { 0, 0 } } }, { { { 0, 0 } } }, { { { 0, 0 } } } },
    { { { BACKFACE_DRAW, 1, 2, 3 } }, { { { 0, 0 } } }, { { { 0, 0 } } }, { { { 0, 0 } } } },
    { { { BACKFACE_DRAW, 0, 2, 1 } }, { { { 0, 0 } } }, { { { 0, 0 } } }, { { { 0, 0 } } } },
    { { { BACKFACE_DRAW, 1, 2, 3 } }, { { { 0, 0 } } }, { { { 0, 0 } } }, { { { 0, 0 } } } },
};

TextureHeader *gWaveTextureHeader = NULL;
s32 *gWaveBlockCells = NULL; // indexed by gWaveModel.unkC
LevelModel_Alternate *gWaveModel = NULL;
s32 gVisibleWaveTiles = 0; // Tracks an index into gWaveBlockIDs
s16 *gWaveCellLookup =
    NULL; // Points to either gWaveCellLookupSingle or gWaveCellLookupDouble and is used for gWaveBlockCells
s16 *gWaveCellIndexLookup = NULL; // Points to either gWaveCellIndexLookupSingle or gWaveCellIndexLookupDouble and is
                                  // used to index gWaveHeightBuffers

s16 gWaveCellLookupSingle[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };

s16 gWaveCellIndexLookupSingle[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 0 };

s16 gWaveCellLookupDouble[26] = { 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
                                  14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0 };

s16 gWaveCellIndexLookupDouble[26] = { 0, 1, 1, 1, 2, 3, 4, 4, 4, 5, 3, 4, 4, 4, 5, 3, 4, 4, 4, 5, 6, 7, 7, 7, 8, 0 };

u8 *gWaveDetailLevels = NULL;
s32 gWaveTileTotal = 0; // some sort of count? Relative to gWaveController.subdivisions
ColourCycle *gWavePulseLight = NULL;
WaveGenTileSlots *gWaveGenSlots = NULL; // tracks an index into gWaveGenList
s32 gWaveGenCount = 0;      // counter for something, incremented in wavegen_register, decremented in wavegen_destroy
s32 gWaveTileGridCount = 0; // used in mempool_alloc_safe size calculation, multiplied with 8
WaveGen *gWaveGenList = NULL;
Object **gWaveGenObjs = NULL; // might be a length of 32
Object *gWaveGeneratorObj = NULL;

/*******************************/

/************ .rodata ************/

/*********************************/

/************ .bss ************/

Gfx *gWaveDL;
Mtx *gWaveMtx;
WaveControl gWaveController;      // holds lots of control information used in this file
s32 gWaveVertexFlip;              // either 1 or 0, toggled in waves_update, used to index vertices and triangles
f32 gWaveLowerY;                  // is set to lowest value of gWaveHeightTable
f32 gWaveUpperY;                  // is set to highest value of gWaveHeightTable
Vertex gWaveCornerVertices[2][4]; // stores values of gWaveVertices to be used in waves_render
s32 gWavePlayerCount;             // controls whether 2 or 4 items are used in gWaveVertices / gWaveTriangles
TriangleBatchInfo *gWaveBatch;
TextureHeader *gWaveTexture;
s32 gWaveUVBaseX;      // u value for gWaveUVTable
s32 gWaveUVBaseY;      // v value for gWaveUVTable
s32 gWaveUVStepX;      // is added onto u of gWaveUVBaseX, is multiplied with texture width
s32 gWaveUVStepY;      // is added onto v of gWaveUVBaseX, is multiplied with texture height
s32 gWaveTexUVMaskX;   // bitmask / something width (related to texture)
s32 gWaveTexUVMaskY;   // bitmask / something height (related to texture)
s32 gWaveTexAnimFrame; // stores / relates to frameAdvanceDelay, used in waves_render
f32 gWaveBoundingBoxW; // copy of gWaveBoundingBoxDiffX
f32 gWaveBoundingBoxH; // copy of gWaveBoundingBoxDiffZ
s32 gWaveBoundingBoxDiffX;
s32 gWaveBoundingBoxDiffZ;
s32 gWaveBoundingBoxX1;
s32 gWaveBoundingBoxZ1;
f32 gWaveVtxStepX;      // some sort of ratio for x
f32 gWaveVtxStepZ;      // some sort of ratio for z
s32 gWaveBlockBoundsX1; // level bounding box x1
s32 gWaveBlockBoundsZ1; // level bounding box x2
s32 gWaveBlockBoundsX2; // level bounding box z1
s32 gWaveBlockBoundsZ2; // level bounding box x2
s32 gWaveBlockPosX;     // copy of gWaveBoundingBoxX1
s32 gWaveBlockPosZ;     // copy of gWaveBoundingBoxZ1
s32 gWaveTileCountX;    // used in mempool_alloc_safe size calculation
s32 gWaveTileCountZ;    // used in mempool_alloc_safe size calculation
s32 gNumberOfLevelSegments;
s32 gWaveVisibilityGrid[64];
s16 gWaveBlockIDs[512]; // used to index gWaveModel and as arg0 for wave_block_build_vertices and
                        // wave_block_build_vertices_depth_shaded
// One table: retail's fixed .bss layout placed gWaveVisibleBlocksBody immediately after
// gWaveVisibleBlocks, and the visibility builder/walkers index straight through both
// (the k walk in wave_block_build_vertices/wave_block_build_vertices_depth_shaded only stops at a -1 sentinel).
// Split declarations only worked by layout luck; merged so adjacency is
// guaranteed on every compiler.
WaveVisibleBlock gWaveVisibleBlocks[26];
f32 gWavePowerBase;
f32 gWaveMagnitude;
s32 gWavePowerDivisor;

/*****************************/

#define FREE_MEM(mem)          \
    tempMem = (s32 *) mem;     \
    if (tempMem != NULL) {     \
        mempool_free(tempMem); \
        mem = NULL;            \
    }
#define FREE_TEX(tex)      \
    tempTex = tex;         \
    if (tempTex != NULL) { \
        tex_free(tempTex); \
        tex = NULL;        \
    }

const char gWavesOverlapErrorMsg[] = "\nError :: more than eight swells overlap on column %d.";

/**
 * Free all Wavegen data from memory.
 * Has safety checks to ensure what it's freeing exists, first.
 */
void waves_free(void) {
    TextureHeader *tempTex;
    s32 *tempMem;
    FREE_MEM(gWaveHeightTable);
    FREE_MEM(gWaveHeightIndices);
    FREE_MEM(gWaveUVTable);
    FREE_MEM(gWaveHeightBuffers[0]);
    FREE_MEM(gWaveVertices[0]);
    FREE_MEM(gWaveTriangles[0]);
    FREE_TEX(gWaveTextureHeader);
    FREE_MEM(gWaveBlockCells);
    FREE_MEM(gWaveModel);
    FREE_MEM(gWaveDetailLevels);
    gWaveGenList = NULL;
    gWaveGenObjs = NULL;
    gWaveGenSlots = NULL;
    gWaveGenCount = 0;
}

/**
 * Allocates and sets up pointers for all wave geometry and data.
 * Allocates a set for each viewport, up to 2.
 * Calls waves_free as a precautionary.
 */
void waves_alloc(void) {
    s32 temp;
    s32 allocSize;
    s32 i;

    waves_free();
    gWaveHeightTable = (f32 *) mempool_alloc_safe(gWaveController.seedSize * sizeof(f32), COLOUR_TAG_CYAN);
    gWaveHeightIndices = (Vec2s *) mempool_alloc_safe(
        (gWaveController.tileCount * sizeof(Vec2s *)) * gWaveController.tileCount, COLOUR_TAG_CYAN);
    gWaveUVTable = (TexCoords *) mempool_alloc_safe(((gWaveController.subdivisions + 1) * sizeof(TexCoords *)) *
                                                        (gWaveController.subdivisions + 1),
                                                    COLOUR_TAG_CYAN);
    allocSize = ((gWaveController.subdivisions + 1) << 2) * (gWaveController.subdivisions + 1);
    gWaveHeightBuffers[0] = mempool_alloc_safe(allocSize * ARRAY_COUNT(gWaveHeightBuffers), COLOUR_TAG_CYAN);
    for (i = 1; i < ARRAY_COUNT(gWaveHeightBuffers); i++) {
        gWaveHeightBuffers[i] = (f32 *) (((uintptr_t) gWaveHeightBuffers[0]) + (allocSize * i));
    }
    temp = (gWaveController.subdivisions + 1);
    allocSize = (temp * 250 * (gWaveController.subdivisions + 1));
    if (gWavePlayerCount != 2) { // 1 player
        gWaveVertices[0] = (Vertex *) mempool_alloc_safe(allocSize << 1, COLOUR_TAG_CYAN);
        gWaveVertices[1] = (Vertex *) (((uintptr_t) gWaveVertices[0]) + allocSize);
    } else { // 2 Player
        gWaveVertices[0] = (Vertex *) mempool_alloc_safe(allocSize << 2, COLOUR_TAG_CYAN);
        gWaveVertices[1] = (Vertex *) (((uintptr_t) gWaveVertices[0]) + allocSize);
        gWaveVertices[2] = (Vertex *) (((uintptr_t) gWaveVertices[1]) + allocSize);
        gWaveVertices[3] = (Vertex *) (((uintptr_t) gWaveVertices[2]) + allocSize);
    }
    allocSize = (gWaveController.subdivisions * 32) * gWaveController.subdivisions;
    if (gWavePlayerCount != 2) { // 1 Player
        gWaveTriangles[0] = mempool_alloc_safe(allocSize << 1, COLOUR_TAG_CYAN);
        gWaveTriangles[1] = (Triangle *) (((uintptr_t) gWaveTriangles[0]) + allocSize);
    } else { // 2 Player
        gWaveTriangles[0] = (Triangle *) mempool_alloc_safe(allocSize << 2, COLOUR_TAG_CYAN);
        gWaveTriangles[1] = (Triangle *) (((uintptr_t) gWaveTriangles[0]) + allocSize);
        gWaveTriangles[2] = (Triangle *) (((uintptr_t) gWaveTriangles[1]) + allocSize);
        gWaveTriangles[3] = (Triangle *) (((uintptr_t) gWaveTriangles[2]) + allocSize);
    }
    gWaveTextureHeader = load_texture(gWaveController.textureId);
}

/**
 * Initialise wave controller variables using the level header.
 * Two player waves hardset the subdivision level to 4.
 */
void waves_init_header(LevelHeader *header) {
    if (gWavePlayerCount != 2) {
        gWaveController.subdivisions = header->waveSubdivisons;
    } else {
        gWaveController.subdivisions = 4;
    }
    gWaveController.tileCount = header->waveTileCount;
    gWaveController.initSine[0].sineStep = header->waveSineStep0;
    gWaveController.initSine[0].height = header->waveSineHeight0 / 256.0f;
    gWaveController.initSine[0].sineBase = header->waveSineBase0 << 8;
    gWaveController.initSine[1].sineStep = header->waveSineStep1;
    gWaveController.initSine[1].height = header->waveSineHeight1 / 256.0f;
    gWaveController.initSine[1].sineBase = header->waveSineBase1 << 8;
    gWaveController.seedSize = header->waveSeedSize & ~1; // Always an even number.
    if (gWavePlayerCount != 2) {
        gWaveController.waveViewDist = header->waveViewDist;
    } else {
        gWaveController.waveViewDist = 3;
    }
    gWaveController.doubleDensity = header->waveDoubleDensity;
    gWaveController.textureId = header->waveTexID & 0xFFFF;
    gWaveController.uvScaleX = header->waveUVScaleX;
    gWaveController.uvScaleY = header->waveUVScaleY;
    gWaveController.uvScrollX = header->waveUVScrollX;
    gWaveController.uvScrollY = header->waveUVScrollY;
    gWaveController.magnitude = header->wavePower / 256.0f;
    gWaveController.minDetailScale = header->waveMinDetailScale / 256.0f;
    gWaveController.heightColourScale = header->waveHeightColourScale / 256.0f;
    gWaveController.xlu = header->wavesXlu;
}

void waves_init(LevelModel *model, LevelHeader *header, s32 playerCount) {
    s32 k;
    s32 sineVar2;
    s32 j_2;
    s32 var_s3;
    s32 var_s5;
    s32 sineVar1;
    s32 sineStep1;
    s32 i;
    s32 sineStep2;
    s32 j;
    s32 i_2;
    s32 var_t0;
    s32 var_v1;
    s32 var_t7;
    s32 var_a1;
    s32 var_t2;

    gWavePlayerCount = playerCount;
    waves_init_header(header);
    waves_alloc();
    waves_init_level(model, header);
    gWaveBoundingBoxW = gWaveBoundingBoxDiffX;
    gWaveBoundingBoxH = gWaveBoundingBoxDiffZ;
    gWaveVtxStepX = gWaveBoundingBoxW / gWaveController.subdivisions;
    gWaveVtxStepZ = gWaveBoundingBoxH / gWaveController.subdivisions;
    gWaveUVBaseX = 0;
    gWaveUVBaseY = 0;
    gWaveUVStepX = ((gWaveTexture->width * gWaveController.uvScaleX) * 32) / gWaveController.subdivisions;
    gWaveUVStepY = ((gWaveTexture->height * gWaveController.uvScaleY) * 32) / gWaveController.subdivisions;
    gWaveTexUVMaskX = (gWaveTexture->width * 32) - 1;
    gWaveTexUVMaskY = (gWaveTexture->height * 32) - 1;
    gWaveTexAnimFrame = 0;
    sineVar1 = gWaveController.initSine[0].sineBase;
    sineStep1 = (gWaveController.initSine[0].sineStep << 16) / gWaveController.seedSize;
    sineVar2 = gWaveController.initSine[1].sineBase;
    sineStep2 = (gWaveController.initSine[1].sineStep << 16) / gWaveController.seedSize;
    gWaveLowerY = 10000.0f;
    gWaveUpperY = -10000.0f;
    stubbed_printf("pre seedSize loop, seedSize=%d tileCount=%d subdiv=%d\n", gWaveController.seedSize,
                   gWaveController.tileCount, gWaveController.subdivisions);
    for (i_2 = 0; i_2 < gWaveController.seedSize; i_2++) {
        gWaveHeightTable[i_2] = (sins_f(sineVar1) * gWaveController.initSine[0].height) +
                                (gWaveController.initSine[1].height * sins_f(sineVar2));
        if (gWaveController.doubleDensity) {
            gWaveHeightTable[i_2] *= 2.0f;
        }
        if (gWaveHeightTable[i_2] < gWaveLowerY) {
            gWaveLowerY = gWaveHeightTable[i_2];
        }
        if (gWaveUpperY < gWaveHeightTable[i_2]) {
            gWaveUpperY = gWaveHeightTable[i_2];
        }
        sineVar1 += sineStep1;
        sineVar2 += sineStep2;
    };
    stubbed_printf("post seedSize loop\n");
    save_rng_seed();
    set_rng_seed('WAVF');

    var_s5 = 0;
    for (i_2 = 0; i_2 < gWaveController.tileCount; i_2++) {
        for (j_2 = 0; j_2 < gWaveController.tileCount; j_2++) {
            gWaveHeightIndices[var_s5].s[0] = rand_range(0, gWaveController.seedSize - 1);
            gWaveHeightIndices[var_s5].s[1] = rand_range(0, gWaveController.seedSize - 1);
            var_s5++;
        }
    }
    stubbed_printf("post tileCount^2 loop\n");
    var_s5 = 0;
    i_2 = 0;
    load_rng_seed();
    if (playerCount != 2) {
        playerCount = 2;
    } else {
        playerCount = 4;
    }
    stubbed_printf("pre vertex loop playerCount=%d v0=%x v1=%x v2=%x v3=%x\n", playerCount,
                   (u32) (uintptr_t) gWaveVertices[0], (u32) (uintptr_t) gWaveVertices[1],
                   (u32) (uintptr_t) gWaveVertices[2], (u32) (uintptr_t) gWaveVertices[3]);
    for (var_s3 = 0; var_s3 < 25; var_s3++) {
        if (0 <= gWaveController.subdivisions) {
            do {
                for (j_2 = 0; gWaveController.subdivisions >= j_2; j_2++) {
                    for (k = 0; k < playerCount; k++) {
                        gWaveVertices[k][var_s5].x = (j_2 * gWaveVtxStepX) + 0.5;
                        gWaveVertices[k][var_s5].z = (i_2 * gWaveVtxStepZ) + 0.5;
                        // this is only 0 for hot top volcano
                        if (gWaveController.xlu == FALSE) {
                            gWaveVertices[k][var_s5].r = 255;
                            gWaveVertices[k][var_s5].g = 255;
                            gWaveVertices[k][var_s5].b = 255;
                        } else {
                            gWaveVertices[k][var_s5].r = 0;
                            gWaveVertices[k][var_s5].g = 0;
                            gWaveVertices[k][var_s5].b = 0;
                        }
                        gWaveVertices[k][var_s5].a = 255;
                    }
                    var_s5++;
                }
                i_2++;
            } while (gWaveController.subdivisions >= i_2);
            i_2 = 0;
        }
    }
    stubbed_printf("post vertex loop\n");

    var_s5 = 0;
    for (i_2 = 0; i_2 < gWaveController.subdivisions; i_2++) {
        for (j_2 = 0; j_2 < gWaveController.subdivisions; j_2++) {
            for (k = 0; k < playerCount; k++) {
                gWaveTriangles[k][var_s5].flags = BACKFACE_DRAW;
                gWaveTriangles[k][var_s5].vi0 = j_2;
                gWaveTriangles[k][var_s5].vi1 = (j_2 + gWaveController.subdivisions) + 1;
                gWaveTriangles[k][var_s5].vi2 = j_2 + 1;
                var_s5++;
                gWaveTriangles[k][var_s5].flags = BACKFACE_DRAW;
                gWaveTriangles[k][var_s5].vi0 = j_2 + 1;
                gWaveTriangles[k][var_s5].vi1 = (j_2 + gWaveController.subdivisions) + 1;
                gWaveTriangles[k][var_s5].vi2 = (j_2 + gWaveController.subdivisions) + 2;
                var_s5--;
            }
            var_s5 += 2;
        }
    }
    stubbed_printf("post triangle loop, pre waves_build_sine_table\n");
    waves_build_sine_table();
    stubbed_printf("post waves_build_sine_table\n");

    var_s5 = (gWaveController.subdivisions + 1) * gWaveController.subdivisions;
    for (i = 0; i < ARRAY_COUNT(gWaveCornerVertices); i++) {
        gWaveCornerVertices[i][0].x = gWaveVertices[i][0].x;
        gWaveCornerVertices[i][0].y = 0;
        gWaveCornerVertices[i][0].z = gWaveVertices[i][0].z;
        gWaveCornerVertices[i][0].r = gWaveVertices[i][0].r;
        gWaveCornerVertices[i][0].g = gWaveVertices[i][0].g;
        gWaveCornerVertices[i][0].b = gWaveVertices[i][0].b;
        gWaveCornerVertices[i][0].a = gWaveVertices[i][0].a;

        gWaveCornerVertices[i][1].x = gWaveVertices[i][gWaveController.subdivisions].x;
        gWaveCornerVertices[i][1].y = 0;
        gWaveCornerVertices[i][1].z = gWaveVertices[i][gWaveController.subdivisions].z;
        gWaveCornerVertices[i][1].r = gWaveVertices[i][gWaveController.subdivisions].r;
        gWaveCornerVertices[i][1].g = gWaveVertices[i][gWaveController.subdivisions].g;
        gWaveCornerVertices[i][1].b = gWaveVertices[i][gWaveController.subdivisions].b;
        gWaveCornerVertices[i][1].a = gWaveVertices[i][gWaveController.subdivisions].a;

        gWaveCornerVertices[i][2].x = gWaveVertices[i][var_s5].x;
        gWaveCornerVertices[i][2].y = 0;
        gWaveCornerVertices[i][2].z = gWaveVertices[i][var_s5].z;
        gWaveCornerVertices[i][2].r = gWaveVertices[i][var_s5].r;
        gWaveCornerVertices[i][2].g = gWaveVertices[i][var_s5].g;
        gWaveCornerVertices[i][2].b = gWaveVertices[i][var_s5].b;
        gWaveCornerVertices[i][2].a = gWaveVertices[i][var_s5].a;

        gWaveCornerVertices[i][3].x = gWaveVertices[i][var_s5 + gWaveController.subdivisions].x;
        gWaveCornerVertices[i][3].y = 0;
        gWaveCornerVertices[i][3].z = gWaveVertices[i][var_s5 + gWaveController.subdivisions].z;
        gWaveCornerVertices[i][3].r = gWaveVertices[i][var_s5 + gWaveController.subdivisions].r;
        gWaveCornerVertices[i][3].g = gWaveVertices[i][var_s5 + gWaveController.subdivisions].g;
        gWaveCornerVertices[i][3].b = gWaveVertices[i][var_s5 + gWaveController.subdivisions].b;
        gWaveCornerVertices[i][3].a = gWaveVertices[i][var_s5 + gWaveController.subdivisions].a;
    }

    waves_init_collision(model);
    if (gWaveController.waveViewDist == 3) {
        gWaveCellLookup = gWaveCellLookupSingle;
        gWaveCellIndexLookup = gWaveCellIndexLookupSingle;
    } else {
        gWaveController.waveViewDist = 5;
        gWaveCellLookup = gWaveCellLookupDouble;
        gWaveCellIndexLookup = gWaveCellIndexLookupDouble;
    }
    gWaveGenCount = 0;
    gWavePowerDivisor = 0;
    gWaveGeneratorObj = NULL;
    gWaveVertexFlip = 0;
}
