#include "tracks_internal.h"

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
    PCU_FIELD(LevelModelSegment, batches, 0x0C, 4,
              PCU_PTR), // batches, triangleGridMasks, collisionFacets, collisionPlanes
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
    pc_unpack_array(segs, sizeof(LevelModelSegment), image + segOff, LEVEL_SEGMENT_DISK_SIZE, heap->numberOfSegments,
                    sLevelSegmentFields);
    for (k = 0; k < heap->numberOfSegments; k++) {
        PC_REBASE(&segs[k], vertices, Vertex *);
        PC_REBASE(&segs[k], triangles, Triangle *);
        PC_REBASE(&segs[k], batches, TriangleBatchInfo *);
        PC_REBASE(&segs[k], collisionFacets, CollisionFacetPlanes *);
        // triangleGridMasks, collisionPlanes, unused2C, waterBatchIndices are assigned at runtime.
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
    s32 compressedSize;
    s32 trackSize;
    s32 batch;
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
    compressedSize = gLevelModelTable[modelId + 1] - mdl;

    pc_load_level_model(gCurrentLevelModel, mdl, compressedSize);
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
            load_texture(ASSET_SLOT(gCurrentLevelModel->textures[k].texture) | 0x8000);
    }
    j = (ptrint_t) gCurrentLevelModel + gCurrentLevelModel->modelSize;
    for (k = 0; k < gCurrentLevelModel->numberOfSegments; k++) {
        gCurrentLevelModel->segments[k].triangleGridMasks = (s16 *) j;
        j = (ptrint_t) align16(((u8 *) (uintptr_t) (gCurrentLevelModel->segments[k].numberOfTriangles * 2)) + j);
        gCurrentLevelModel->segments[k].collisionPlanes = (f32 *) j;
        j = (ptrint_t) & ((u8 *) j)[track_init_collision(&gCurrentLevelModel->segments[k])];
        segment_compute_triangle_grid_masks(&gCurrentLevelModel->segments[k],
                                            &gCurrentLevelModel->segmentsBoundingBoxes[k], k);
        gCurrentLevelModel->segments[k].unused30 = 0;
        gCurrentLevelModel->segments[k].waterBatchIndices = (s16 *) j;
        segment_find_water_batches(&gCurrentLevelModel->segments[k]);
        j = (ptrint_t) align16(((u8 *) (uintptr_t) (gCurrentLevelModel->segments[k].numWaterBatches * 2)) + j);
    }
    trackSize = (s32) (j - (ptrint_t) gCurrentLevelModel);
    if (trackSize > LEVEL_MODEL_MAX_SIZE) {
        pc_log("ERROR!! TrackMem overflow .. %d\n", trackSize);
    }
    mempool_free_timer(0);
    mempool_free(gTrackModelHeap);
    mempool_alloc_fixed(trackSize, (u8 *) gTrackModelHeap, COLOUR_TAG_YELLOW);
    mempool_free_timer(2);
    stubbed_printf("pre minimap_init\n");
    minimap_init(gCurrentLevelModel);
    stubbed_printf("post minimap_init\n");

    for (i = 0; i < gCurrentLevelModel->numberOfSegments; i++) {
        for (batch = 0; batch < gCurrentLevelModel->segments[i].numberOfBatches; batch++) {
            for (k = gCurrentLevelModel->segments[i].batches[batch].verticesOffset;
                 k < gCurrentLevelModel->segments[i].batches[batch + 1].verticesOffset; k++) {
                // Vertices marked r = g = 1 carry their alpha in b: move it to a, make them neutral grey
                // and have the batch use vertex alpha.
                // Why do this? Why not just set the vertex colors in the model itself?
                if (gCurrentLevelModel->segments[i].vertices[k].r == 1 &&
                    gCurrentLevelModel->segments[i].vertices[k].g == 1) {
                    gCurrentLevelModel->segments[i].vertices[k].a = gCurrentLevelModel->segments[i].vertices[k].b;
                    gCurrentLevelModel->segments[i].vertices[k].r = 0x80;
                    gCurrentLevelModel->segments[i].vertices[k].g = 0x80;
                    gCurrentLevelModel->segments[i].vertices[k].b = 0x80;
                    gCurrentLevelModel->segments[i].batches[batch].flags |= RENDER_VTX_ALPHA;
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
        if (segment->batches[i].flags & RENDER_WATER) {
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
