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
s32 *gWaveBlockCells = NULL; // indexed by gWaveModel.tileIndex
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
s16 gWaveBlockIDs[512]; // used to index gWaveModel and as blockID for wave_block_build_vertices and
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
    s32 rowLength;
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
    rowLength = (gWaveController.subdivisions + 1);
    allocSize = (rowLength * 250 * (gWaveController.subdivisions + 1));
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
        gWaveController.subdivisions = WAVE_SUBDIVISIONS_2P;
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
        gWaveController.waveViewDist = WAVE_VIEW_DIST_NEAR;
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

/**
 * Fills gWaveHeightTable with one cycle of the sum of the level's two sine waves, and records its lowest and
 * highest values.
 */
static void waves_init_height_table(void) {
    s32 sineAngle0;
    s32 sineAngleStep0;
    s32 sineAngle1;
    s32 sineAngleStep1;
    s32 i;

    sineAngle0 = gWaveController.initSine[0].sineBase;
    sineAngleStep0 = (gWaveController.initSine[0].sineStep << 16) / gWaveController.seedSize;
    sineAngle1 = gWaveController.initSine[1].sineBase;
    sineAngleStep1 = (gWaveController.initSine[1].sineStep << 16) / gWaveController.seedSize;
    gWaveLowerY = 10000.0f;
    gWaveUpperY = -10000.0f;
    stubbed_printf("pre seedSize loop, seedSize=%d tileCount=%d subdiv=%d\n", gWaveController.seedSize,
                   gWaveController.tileCount, gWaveController.subdivisions);
    for (i = 0; i < gWaveController.seedSize; i++) {
        gWaveHeightTable[i] = (sins_f(sineAngle0) * gWaveController.initSine[0].height) +
                              (gWaveController.initSine[1].height * sins_f(sineAngle1));
        if (gWaveController.doubleDensity) {
            gWaveHeightTable[i] *= 2.0f;
        }
        if (gWaveHeightTable[i] < gWaveLowerY) {
            gWaveLowerY = gWaveHeightTable[i];
        }
        if (gWaveUpperY < gWaveHeightTable[i]) {
            gWaveUpperY = gWaveHeightTable[i];
        }
        sineAngle0 += sineAngleStep0;
        sineAngle1 += sineAngleStep1;
    };
    stubbed_printf("post seedSize loop\n");
}

/**
 * Gives every vertex of the repeating wave tile grid two random starting points in gWaveHeightTable, from a
 * fixed seed so the water looks the same every time. The game's own RNG state is saved and restored around it.
 */
static void waves_init_height_indices(void) {
    s32 i;
    s32 row;
    s32 col;

    save_rng_seed();
    set_rng_seed('WAVF');

    i = 0;
    for (row = 0; row < gWaveController.tileCount; row++) {
        for (col = 0; col < gWaveController.tileCount; col++) {
            gWaveHeightIndices[i].s[0] = rand_range(0, gWaveController.seedSize - 1);
            gWaveHeightIndices[i].s[1] = rand_range(0, gWaveController.seedSize - 1);
            i++;
        }
    }
    stubbed_printf("post tileCount^2 loop\n");
    load_rng_seed();
}

/**
 * Lays out the flat vertex grid of every visible wave cell in each vertex buffer: positions within the cell,
 * and white (or, for translucent water, black) opaque colour.
 */
static void waves_init_vertices(s32 bufferCount) {
    s32 cell;
    s32 row;
    s32 col;
    s32 k;
    s32 vertexIdx;

    vertexIdx = 0;
    row = 0;
    stubbed_printf("pre vertex loop playerCount=%d v0=%x v1=%x v2=%x v3=%x\n", bufferCount,
                   (u32) (uintptr_t) gWaveVertices[0], (u32) (uintptr_t) gWaveVertices[1],
                   (u32) (uintptr_t) gWaveVertices[2], (u32) (uintptr_t) gWaveVertices[3]);
    for (cell = 0; cell < WAVE_MAX_VISIBLE_CELLS; cell++) {
        if (0 <= gWaveController.subdivisions) {
            do {
                for (col = 0; gWaveController.subdivisions >= col; col++) {
                    for (k = 0; k < bufferCount; k++) {
                        gWaveVertices[k][vertexIdx].x = (col * gWaveVtxStepX) + 0.5;
                        gWaveVertices[k][vertexIdx].z = (row * gWaveVtxStepZ) + 0.5;
                        // this is only 0 for hot top volcano
                        if (gWaveController.xlu == FALSE) {
                            gWaveVertices[k][vertexIdx].r = 255;
                            gWaveVertices[k][vertexIdx].g = 255;
                            gWaveVertices[k][vertexIdx].b = 255;
                        } else {
                            gWaveVertices[k][vertexIdx].r = 0;
                            gWaveVertices[k][vertexIdx].g = 0;
                            gWaveVertices[k][vertexIdx].b = 0;
                        }
                        gWaveVertices[k][vertexIdx].a = 255;
                    }
                    vertexIdx++;
                }
                row++;
            } while (gWaveController.subdivisions >= row);
            row = 0;
        }
    }
    stubbed_printf("post vertex loop\n");
}

/**
 * Builds the triangle list of one wave cell (two triangles per grid square) in each triangle buffer.
 * Vertex indices are relative to the row being drawn, as waves_render loads two rows of vertices at a time.
 */
static void waves_init_triangles(s32 bufferCount) {
    s32 row;
    s32 col;
    s32 k;
    s32 triIdx;

    triIdx = 0;
    for (row = 0; row < gWaveController.subdivisions; row++) {
        for (col = 0; col < gWaveController.subdivisions; col++) {
            for (k = 0; k < bufferCount; k++) {
                gWaveTriangles[k][triIdx].flags = BACKFACE_DRAW;
                gWaveTriangles[k][triIdx].vi0 = col;
                gWaveTriangles[k][triIdx].vi1 = (col + gWaveController.subdivisions) + 1;
                gWaveTriangles[k][triIdx].vi2 = col + 1;
                triIdx++;
                gWaveTriangles[k][triIdx].flags = BACKFACE_DRAW;
                gWaveTriangles[k][triIdx].vi0 = col + 1;
                gWaveTriangles[k][triIdx].vi1 = (col + gWaveController.subdivisions) + 1;
                gWaveTriangles[k][triIdx].vi2 = (col + gWaveController.subdivisions) + 2;
                triIdx--;
            }
            triIdx += 2;
        }
    }
    stubbed_printf("post triangle loop, pre waves_build_sine_table\n");
}

/**
 * Copies the four corner vertices of a wave cell's grid into gWaveCornerVertices, for drawing a far cell as
 * a single flat quad.
 */
static void waves_init_corner_vertices(void) {
    s32 i;
    s32 lastRow;

    lastRow = (gWaveController.subdivisions + 1) * gWaveController.subdivisions;
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

        gWaveCornerVertices[i][2].x = gWaveVertices[i][lastRow].x;
        gWaveCornerVertices[i][2].y = 0;
        gWaveCornerVertices[i][2].z = gWaveVertices[i][lastRow].z;
        gWaveCornerVertices[i][2].r = gWaveVertices[i][lastRow].r;
        gWaveCornerVertices[i][2].g = gWaveVertices[i][lastRow].g;
        gWaveCornerVertices[i][2].b = gWaveVertices[i][lastRow].b;
        gWaveCornerVertices[i][2].a = gWaveVertices[i][lastRow].a;

        gWaveCornerVertices[i][3].x = gWaveVertices[i][lastRow + gWaveController.subdivisions].x;
        gWaveCornerVertices[i][3].y = 0;
        gWaveCornerVertices[i][3].z = gWaveVertices[i][lastRow + gWaveController.subdivisions].z;
        gWaveCornerVertices[i][3].r = gWaveVertices[i][lastRow + gWaveController.subdivisions].r;
        gWaveCornerVertices[i][3].g = gWaveVertices[i][lastRow + gWaveController.subdivisions].g;
        gWaveCornerVertices[i][3].b = gWaveVertices[i][lastRow + gWaveController.subdivisions].b;
        gWaveCornerVertices[i][3].a = gWaveVertices[i][lastRow + gWaveController.subdivisions].a;
    }
}

/**
 * Sets up the water of a level: reads the wave settings from the level header, allocates the wave buffers,
 * lays out the segments, then builds the height tables, vertex and triangle grids and depth table.
 */
void waves_init(LevelModel *model, LevelHeader *header, s32 playerCount) {
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
    waves_init_height_table();
    waves_init_height_indices();
    // From here playerCount is the number of vertex and triangle buffers: two per viewport.
    if (playerCount != 2) {
        playerCount = 2;
    } else {
        playerCount = 4;
    }
    waves_init_vertices(playerCount);
    waves_init_triangles(playerCount);
    waves_build_sine_table();
    stubbed_printf("post waves_build_sine_table\n");
    waves_init_corner_vertices();

    waves_init_collision(model);
    if (gWaveController.waveViewDist == WAVE_VIEW_DIST_NEAR) {
        gWaveCellLookup = gWaveCellLookupSingle;
        gWaveCellIndexLookup = gWaveCellIndexLookupSingle;
    } else {
        gWaveController.waveViewDist = WAVE_VIEW_DIST_FAR;
        gWaveCellLookup = gWaveCellLookupDouble;
        gWaveCellIndexLookup = gWaveCellIndexLookupDouble;
    }
    gWaveGenCount = 0;
    gWavePowerDivisor = 0;
    gWaveGeneratorObj = NULL;
    gWaveVertexFlip = 0;
}
