#include "object_models.h"
#include "asset_enums.h"
#include "asset_loading.h"
#include "common.h"
#include "gzip.h"
#include "libc/math.h"
#include "macros.h"
#include "memory.h"
#include "objects.h"
#include "pc_swap.h"
#include "racer.h"
#include "textures_sprites.h"
#include "tracks.h"

#define MODEL_LOADED_MAX 70

/************ .data ************/

s32 gRenderSceneChecksum = TractionTableChecksum;
s32 gRenderSceneFuncLength = 1980;

/*******************************/

/************ .bss ************/

s32 *gObjectModelTable;
uintptr_t *gModelCache; // id / pointer pairs: pointer-sized on the host
s32 *gModelCacheFreeSlots;
s32 gModelCacheCount;
s32 gNumModelIDs;
s32 gModelCacheFreeCount;
s16 *gAnimationTable;
s32 *gObjectAnimationTable;
s32 gModelAnimOffsetID;
s16 *gAnimFracDeltas;

/******************************/

/**
 * Allocate memory for object model ID's and animation tables.
 */
void allocate_object_model_pools(void) {
    s32 i;
    s32 checksum;

    gModelCache = mempool_alloc_safe(MODEL_LOADED_MAX * ((sizeof(uintptr_t)) * 2), COLOUR_TAG_GREEN);
    gModelCacheFreeSlots = mempool_alloc_safe(100 * sizeof(uintptr_t), COLOUR_TAG_GREEN);
    gModelCacheCount = 0;
    gModelCacheFreeCount = 0;
    gObjectModelTable = (s32 *) asset_table_load(ASSET_OBJECT_MODELS_TABLE);
    {
        // Big-endian asset offset tables (helpers in linux/reimpl.c).
        extern void pc_swap32_buf(void *buf, u32 numBytes);
        pc_swap32_buf(gObjectModelTable, asset_table_size(ASSET_OBJECT_MODELS_TABLE));
    }
    gNumModelIDs = 0;
    while (gObjectModelTable[gNumModelIDs] != -1) {
        gNumModelIDs++;
    }
    gNumModelIDs--;
    gAnimationTable = (s16 *) asset_table_load(ASSET_ANIMATION_IDS);
    gObjectAnimationTable = (s32 *) asset_table_load(ASSET_OBJECT_ANIMATIONS_TABLE);
    {
        extern void pc_swap16_buf(void *buf, u32 numBytes);
        extern void pc_swap32_buf(void *buf, u32 numBytes);
        pc_swap16_buf(gAnimationTable, asset_table_size(ASSET_ANIMATION_IDS));
        pc_swap32_buf(gObjectAnimationTable, asset_table_size(ASSET_OBJECT_ANIMATIONS_TABLE));
    }
    gAnimFracDeltas = (s16 *) mempool_alloc_safe(0xC00, COLOUR_TAG_GREEN);
    gModelAnimOffsetID = 0;
}

// Inflated object models are big-endian: a 0x80-byte header whose offsets point
// at the vertex/triangle/batch/attach arrays in the same image. On the host the
// image is placed OBJECT_MODEL_PC_DELTA bytes into the allocation so a
// host-sized header fits in front, and the TextureInfo table (one pointer per
// entry) is rebuilt in host layout after the image. The header's offsets are
// rebased so the game's existing offset->pointer fixups land on the arrays.
#define OBJECT_MODEL_DISK_SIZE 0x80
#define OBJECT_MODEL_PC_DELTA (sizeof(ObjectModel) - OBJECT_MODEL_DISK_SIZE)
#define TEXTURE_INFO_DISK_SIZE 8
#define PC_ALIGN16(x) (((x) + 15) & ~15)

static const PcField sObjectModelFields[] = {
    PCU_FIELD(ObjectModel, textures, 0x00, 6, PCU_PTR), // textures..attachPoints
    PCU_FIELD(ObjectModel, numberOfAttachPoints, 0x18, 2, PCU_U16),
    PCU_FIELD(ObjectModel, collisionSpheres, 0x1C, 1, PCU_PTR),
    PCU_FIELD(ObjectModel, collisionSpheresSize, 0x20, 5, PCU_U16), // ..numberOfBatches
    PCU_FIELD(ObjectModel, pad2A, 0x2A, 2, PCU_BYTES),
    PCU_FIELD(ObjectModel, fileSize, 0x2C, 1, PCU_U32),
    PCU_FIELD(ObjectModel, references, 0x30, 2, PCU_U16),
    PCU_FIELD(ObjectModel, pad34, 0x34, 4, PCU_BYTES),
    PCU_FIELD(ObjectModel, batches, 0x38, 1, PCU_PTR),
    PCU_FIELD(ObjectModel, boundingRadius, 0x3C, 1, PCU_U32),
    PCU_FIELD(ObjectModel, normals, 0x40, 2, PCU_PTR), // normals, animations
    PCU_FIELD(ObjectModel, numberOfAnimations, 0x48, 2, PCU_U16),
    PCU_FIELD(ObjectModel, animatedVertexIndices, 0x4C, 1, PCU_PTR),
    PCU_FIELD(ObjectModel, hasAnimatedTexture, 0x50, 2, PCU_U16),
    PCU_FIELD(ObjectModel, pad, 0x54, 0x2C, PCU_BYTES),
    PCU_END,
};

static u8 *sPcModelDisk; // inflated N64 image of the model being loaded
static s32 sPcModelDiskSize;

// Inflate model `assetOffset` (compressed size `compSize`) into host scratch.
static void pc_object_model_inflate(s32 assetOffset, s32 compSize) {
    u8 *comp;

    if (asset_image_inflated()) {
        sPcModelDiskSize = compSize;
        sPcModelDisk = pc_alloc(sPcModelDiskSize + 16);
        asset_load(ASSET_OBJECT_MODELS, (uintptr_t) sPcModelDisk, assetOffset, compSize);
        return;
    }
    comp = pc_alloc(compSize);
    sPcModelDiskSize = gzip_size_uncompressed(ASSET_OBJECT_MODELS, assetOffset);
    sPcModelDisk = pc_alloc(sPcModelDiskSize + 16);
    asset_load(ASSET_OBJECT_MODELS, (uintptr_t) comp, assetOffset, compSize);
    gzip_inflate(comp, sPcModelDisk);
    pc_free(comp);
}

static s32 pc_object_model_alloc_size(void) {
    s32 numTextures = pc_be16(sPcModelDisk, 0x22);
    return (s32) (PC_ALIGN16(OBJECT_MODEL_PC_DELTA + sPcModelDiskSize) + numTextures * sizeof(TextureInfo));
}

#define PC_REBASE(field, T) mdl->field = (T) ((uintptr_t) mdl->field + OBJECT_MODEL_PC_DELTA)

// Lay the inflated image out as a host model at `mdl` and release the scratch.
static void pc_unpack_object_model(ObjectModel *mdl) {
    u8 *image = (u8 *) mdl + OBJECT_MODEL_PC_DELTA;
    TextureInfo *texInfo;
    u32 texOff;
    s32 i;

    pc_memcpy(image, sPcModelDisk, sPcModelDiskSize);
    pc_unpack(mdl, sizeof(ObjectModel), image, OBJECT_MODEL_DISK_SIZE, sObjectModelFields);
    texOff = (u32) (uintptr_t) mdl->textures;

    // Image-relative offsets -> host-header-relative (the game adds the base).
    PC_REBASE(vertices, Vertex *);
    PC_REBASE(triangles, Triangle *);
    PC_REBASE(attachPoints, s16 *);
    PC_REBASE(collisionSpheres, s16 *);
    PC_REBASE(batches, TriangleBatchInfo *);
    PC_REBASE(animatedVertexIndices, s32 *);

    // TextureInfo entries are 8 bytes on disk and wider on the host.
    texInfo = (TextureInfo *) ((u8 *) mdl + PC_ALIGN16(OBJECT_MODEL_PC_DELTA + sPcModelDiskSize)); // pointer-aligned
    for (i = 0; i < mdl->numberOfTextures; i++) {
        const u8 *e = image + texOff + i * TEXTURE_INFO_DISK_SIZE;
        texInfo[i].texture = (TextureHeader *) (uintptr_t) pc_be32(e, 0); // texture id until load_texture
        texInfo[i].width = e[4];
        texInfo[i].height = e[5];
        texInfo[i].format = e[6];
        texInfo[i].surfaceType = (s8) e[7];
    }
    mdl->textures = (TextureInfo *) (uintptr_t) ((u8 *) texInfo - (u8 *) mdl);

    // Array contents, still addressed by offset here.
    for (i = 0; i < mdl->numberOfVertices; i++) {
        pc_swap16_buf(&((Vertex *) ((u8 *) mdl + (uintptr_t) mdl->vertices))[i], 6); // x, y, z
    }
    for (i = 0; i < mdl->numberOfTriangles; i++) {
        PC_SWAP16_AT(&((Triangle *) ((u8 *) mdl + (uintptr_t) mdl->triangles))[i], uv0, 12); // uv0/uv1/uv2
    }
    for (i = 0; i < mdl->numberOfBatches + 1; i++) { // +1: sentinel entry
        TriangleBatchInfo *batch = &((TriangleBatchInfo *) ((u8 *) mdl + (uintptr_t) mdl->batches))[i];
        PC_SWAP16_AT(batch, verticesOffset, 4); // verticesOffset, facesOffset
        PC_SWAP32_AT(batch, flags, 4);
    }
    pc_swap16_buf((u8 *) mdl + (uintptr_t) mdl->attachPoints, mdl->numberOfAttachPoints * 2);
    pc_swap16_buf((u8 *) mdl + (uintptr_t) mdl->collisionSpheres, mdl->collisionSpheresSize * 2);
    // Despite the s32* declaration, obj_animate reads this as an s16 slot map
    // with one entry per model vertex.
    pc_swap16_buf((u8 *) mdl + (uintptr_t) mdl->animatedVertexIndices, mdl->numberOfVertices * 2);

    pc_free(sPcModelDisk);
    sPcModelDisk = NULL;
}
#undef PC_REBASE

/**
 * Load the associated model ID and assign it to the objects gfx data.
 * Also loads textures and animations.
 * v79 and newer have a bugfix that revert the cache count if allocation fails.
 */
ModelInstance *object_model_init(s32 modelID, s32 flags) {
    s32 i;
    s32 cacheIndex;
    ObjectModel *objMdl;
    s32 compSize;
    s32 assetOffset;
    ModelInstance *instance;
    s8 textureMissing;
#if VERSION >= VERSION_79
    s8 tookFreeSlot;
    s8 cacheChanged;
#endif
    u32 compressedData;
    s32 modelSize;

    if (modelID >= gNumModelIDs) {
        stubbed_printf("Error: Model no. out of range on load. !!\n");
        modelID = 0;
    }

    // Check if the model already exists in the cache.
    for (i = 0; i < gModelCacheCount; i++) {
        if (modelID == gModelCache[ASSETCACHE_ID(i)]) {
            objMdl = (ObjectModel *) gModelCache[ASSETCACHE_PTR(i)];
            instance = model_instance_init(objMdl, flags);
            if (instance != NULL) {
                objMdl->references++;
            }
            return instance;
        }
    }

#if VERSION >= VERSION_79
    tookFreeSlot = FALSE;
    cacheChanged = FALSE;
#endif
    if (gModelCacheFreeCount > 0) {
        gModelCacheFreeCount--;
#if VERSION >= VERSION_79
        tookFreeSlot = TRUE;
#endif
        cacheIndex = gModelCacheFreeSlots[gModelCacheFreeCount];
    } else {
        cacheIndex = gModelCacheCount;
#if VERSION >= VERSION_79
        cacheChanged = TRUE;
#endif
        gModelCacheCount++;
    }

    assetOffset = gObjectModelTable[modelID];
    compSize = gObjectModelTable[modelID + 1] - assetOffset;
    pc_object_model_inflate(assetOffset, compSize);
    modelSize = pc_object_model_alloc_size();
    objMdl = (ObjectModel *) mempool_alloc(modelSize, COLOUR_TAG_RED);
    if (objMdl == NULL) {
        pc_free(sPcModelDisk);
        sPcModelDisk = NULL;
#if VERSION >= VERSION_79
        if (tookFreeSlot) {
            gModelCacheFreeCount++;
        }
        if (cacheChanged) {
            gModelCacheCount--;
        }
#endif
        return NULL;
    }
    pc_unpack_object_model(objMdl);
    objMdl->textures = (TextureInfo *) (ASSET_SLOT(objMdl->textures) + (u8 *) objMdl);
    objMdl->vertices = (Vertex *) (ASSET_SLOT(objMdl->vertices) + (u8 *) objMdl);
    objMdl->triangles = (Triangle *) (ASSET_SLOT(objMdl->triangles) + (u8 *) objMdl);
    objMdl->batches = (TriangleBatchInfo *) (ASSET_SLOT(objMdl->batches) + (u8 *) objMdl);
    objMdl->attachPoints = (s16 *) (ASSET_SLOT(objMdl->attachPoints) + (u8 *) objMdl);
    objMdl->collisionSpheres = (s16 *) (ASSET_SLOT(objMdl->collisionSpheres) + (u8 *) objMdl);
    objMdl->animatedVertexIndices = (s32 *) (ASSET_SLOT(objMdl->animatedVertexIndices) + (u8 *) objMdl);
    objMdl->references = 1;
    objMdl->collisionFacets = NULL;
    objMdl->collisionPlanes = NULL;
    objMdl->collisionFacetCount = 0;
    objMdl->texOffsetUpdateRate = 0;
    objMdl->normals = NULL;
    objMdl->numberOfAnimations = 0;
    objMdl->animations = NULL;
    textureMissing = 0;
    for (i = 0; i < objMdl->numberOfTextures; i++) {
        objMdl->textures[i].texture = load_texture(ASSET_SLOT(objMdl->textures[i].texture) | 0x8000);
        if (objMdl->textures[i].texture == NULL) {
            textureMissing = 1;
        }
    }
    if (!textureMissing) {
        for (i = 0; i < objMdl->numberOfBatches; i++) {
            if (objMdl->batches[i].textureIndex != TEX_INDEX_NO_TEXTURE &&
                objMdl->batches[i].textureIndex >= objMdl->numberOfTextures) {
                stubbed_printf("TEXTURE ERROR!!\n%d,%d\n", objMdl->batches[i].textureIndex, objMdl->numberOfTextures);
                goto block_30;
            }
        }
        if (model_init_normals(objMdl) == 0 && model_anim_init(objMdl, modelID) == 0) {
            instance = model_instance_init(objMdl, flags);
            if (instance != NULL) {
                gModelCache[ASSETCACHE_ID(cacheIndex)] = modelID;
                gModelCache[ASSETCACHE_PTR(cacheIndex)] = (uintptr_t) objMdl;
                if (gModelCacheCount < MODEL_LOADED_MAX) {
                    instance->animUpdateTimer = 0;
                    return instance;
                } else {
                    stubbed_printf("Error: Model table overflow!!\n");
                }
            }
        }
    }
block_30:
#if VERSION >= VERSION_79
    if (cacheChanged) {
        gModelCacheCount--;
    }
    if (tookFreeSlot) {
        gModelCacheFreeCount++;
    }
#endif
    free_model_data(objMdl);
    return NULL;
}

/**
 * Initialises the model instance.
 * Objects that do nothing special get a standard instance assigned,
 * otherwise, ones that utilise shading or animation will have their own copies of the vertex data.
 * Returns null if it failed.
 * Official name: createModelInstance
 */
ModelInstance *model_instance_init(ObjectModel *model, s32 flags) {
    s32 allocSize;
    s32 vertIndex;
    ModelInstance *result;
    Vertex *vertexCopy;
    Vertex *vertex;
    Vertex *mdlVertex;

    if (model == NULL) {
        stubbed_printf("WARNING :: createModelInstance called with NULL pointer\n");
    }

    if (model->numberOfAnimations != 0 && (flags & OBJECT_BEHAVIOUR_ANIMATION)) {
        allocSize = ((model->numberOfVertices * 2) * sizeof(Vertex)) + sizeof(ModelInstance);
        result = (ModelInstance *) mempool_alloc((model->numberOfAnimatedVertices * 6) + allocSize, COLOUR_TAG_BLUE);
        if (result == NULL) {
            return NULL;
        }
        result->vertices[0] = (Vertex *) ((u8 *) result + sizeof(ModelInstance));
        result->vertices[1] =
            (Vertex *) ((u8 *) result + (model->numberOfVertices * sizeof(Vertex)) + sizeof(ModelInstance));
        result->vertices[2] = (Vertex *) ((u8 *) result + allocSize);
        result->modelType = MODELTYPE_ANIMATED;
    } else if (model->normals != NULL && (flags & OBJECT_BEHAVIOUR_SHADED)) {
        allocSize = (model->numberOfVertices * sizeof(Vertex)) + sizeof(ModelInstance);
        result = (ModelInstance *) mempool_alloc(allocSize, COLOUR_TAG_BLUE);
        if (result == NULL) {
            return NULL;
        }
        vertexCopy = (Vertex *) ((u8 *) result + sizeof(ModelInstance));
        result->vertices[0] = vertexCopy;
        result->vertices[1] = vertexCopy;
        result->vertices[2] = NULL;
        result->modelType = MODELTYPE_SHADE;
    } else {
        result = (ModelInstance *) mempool_alloc(sizeof(ModelInstance), COLOUR_TAG_BLUE);
        if (result == NULL) {
            return NULL;
        }
        result->vertices[0] = model->vertices;
        result->vertices[1] = model->vertices;
        result->vertices[2] = NULL;
        result->modelType = MODELTYPE_BASIC;
    }
    result->offsetX = 0;
    result->offsetY = 0;
    result->offsetZ = 0;
    result->objModel = model;
    result->animationID = -1;
    result->animationFrame = -1;
    result->animationTaskNum = 0;
    // Shaded models need to be double buffered, so duplicate them.
    if (result->modelType != MODELTYPE_BASIC) {
        vertIndex = 0;
        vertex = result->vertices[0];
        mdlVertex = &model->vertices[0];
        do {
            vertex->x = mdlVertex->x;
            vertex->y = mdlVertex->y;
            vertex->z = mdlVertex->z;
            vertex->r = mdlVertex->r;
            vertex->g = mdlVertex->g;
            vertex->b = mdlVertex->b;
            vertex->a = mdlVertex->a;
            vertex++;
            mdlVertex++;
            vertIndex++;
        } while (vertIndex < model->numberOfVertices);

        vertIndex = 0;
        vertex = result->vertices[1];
        mdlVertex = &model->vertices[0];
        do {
            vertex->x = mdlVertex->x;
            vertex->y = mdlVertex->y;
            vertex->z = mdlVertex->z;
            vertex->r = mdlVertex->r;
            vertex->g = mdlVertex->g;
            vertex->b = mdlVertex->b;
            vertex->a = mdlVertex->a;
            vertex++;
            mdlVertex++;
            vertIndex++;
        } while (vertIndex < model->numberOfVertices);
    }
    return result;
}

/**
 * Attempts to free the object model from RAM.
 * Official name: ModFreeModel
 */
void free_3d_model(ModelInstance *modInst) {
    s32 modelIndex;
    ObjectModel *model;
    s32 i;

    if (modInst == NULL) {
        stubbed_printf("ModFreeModel : NULL mod_inst!!\n");
        return;
    }

    model = modInst->objModel;
    model->references--;
    if (model->references > 0) { // Model is still used, so free the reference and return.
        mempool_free(modInst);
        return;
    }

    modelIndex = -1;
    for (i = 0; i < gModelCacheCount; i++) {
        if (model == (ObjectModel *) gModelCache[ASSETCACHE_PTR(i)]) {
            modelIndex = i;
        }
    }

    if (modelIndex != -1) {
        free_model_data(model);
        gModelCacheFreeSlots[gModelCacheFreeCount] = modelIndex;
        gModelCacheFreeCount++;
        gModelCache[ASSETCACHE_ID(modelIndex)] = -1;
        gModelCache[ASSETCACHE_PTR(modelIndex)] = -1;
        mempool_free(modInst);
    }
}

/**
 * Frees all associated meshes, textures and animations from the model.
 */
void free_model_data(ObjectModel *mdl) {
    s16 numTextures;

    if (mdl == NULL) {
        stubbed_printf("MOD Error: Tryed to deallocate non-existent model!!\n");
    }

    numTextures = mdl->numberOfTextures;
    if (numTextures > 0) {
        s32 texturesFreed = 0;
        s32 textureIndex = 0;
        do {
            TextureHeader *header = mdl->textures[textureIndex].texture;
            if (header != NULL) {
                tex_free(header);
                numTextures = mdl->numberOfTextures;
            }
            texturesFreed++;
            textureIndex++;
        } while (texturesFreed < numTextures);
    }

    if (mdl->collisionFacets != NULL) {
        mempool_free(mdl->collisionFacets);
    }

    if (mdl->collisionPlanes != NULL) {
        mempool_free(mdl->collisionPlanes);
    }

    if (mdl->normals != NULL) {
        mempool_free(mdl->normals);
    }
    // free the animations
    if (mdl->animations != NULL) {
        s32 animsFreed = 0;
        s32 animIndex = 0;
        if (mdl->numberOfAnimations != 0) {
            do {
                mempool_free(mdl->animations[animIndex].anim - 1);
                animsFreed++;
                animIndex++;
            } while (animsFreed < mdl->numberOfAnimations);
            mempool_free(mdl->animations);
        }
    }
    // free the model itself
    mempool_free(mdl);
}

/**
 * Counts the triangles in batches that take part in collision.
 */
static s32 model_count_collision_faces(ObjectModel *model) {
    s32 facesOffset;
    s32 nextFacesOffset;
    s32 planeCount;
    s32 i;

    planeCount = 0;
    for (i = 0; i < model->numberOfBatches; i++) {
        facesOffset = model->batches[i].facesOffset;
        nextFacesOffset = model->batches[i + 1].facesOffset;
        if (model->batches[i].flags & RENDER_NO_COLLISION) {
            continue;
        }
        planeCount += nextFacesOffset - facesOffset;
    }
    return planeCount;
}

/**
 * Writes each collision triangle's own plane (unit normal and plane constant) and points all three of
 * its edge planes at it for now. Returns the number of planes written.
 */
static s32 model_build_face_planes(ObjectModel *model) {
    s32 facesOffset;
    s32 verticesOffset;
    s32 nextFacesOffset;
    s32 planeCount;
    s32 i;
    s32 j;
    Vertex *v;
    f32 x1, y1, z1;
    f32 x2, y2, z2;
    f32 x3, y3, z3;
    f32 nx, ny, nz;
    f32 mag;
    s32 planeOffset;

    planeCount = 0;
    for (i = 0; i < model->numberOfBatches; i++) {
        facesOffset = model->batches[i].facesOffset;
        verticesOffset = model->batches[i].verticesOffset;
        nextFacesOffset = model->batches[i + 1].facesOffset;
        if (model->batches[i].flags & RENDER_NO_COLLISION) {
            nextFacesOffset = facesOffset - 1;
        }

        for (j = facesOffset; j < nextFacesOffset; j++) {
            v = &model->vertices[model->triangles[j].vi0 + verticesOffset];
            x1 = v->x;
            y1 = v->y;
            z1 = v->z;

            v = &model->vertices[model->triangles[j].vi1 + verticesOffset];
            x2 = v->x;
            y2 = v->y;
            z2 = v->z;

            v = &model->vertices[model->triangles[j].vi2 + verticesOffset];
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

            planeOffset = planeCount << 2;
            *(model->collisionPlanes + planeOffset + 0) = nx;
            *(model->collisionPlanes + planeOffset + 1) = ny;
            *(model->collisionPlanes + planeOffset + 2) = nz;
            *(model->collisionPlanes + planeOffset + 3) = -(x1 * nx + y1 * ny + z1 * nz);

            model->collisionFacets[planeCount].basePlaneIndex = planeCount;
            model->collisionFacets[planeCount].edgeBisectorPlane[0] = planeCount;
            model->collisionFacets[planeCount].edgeBisectorPlane[1] = planeCount;
            model->collisionFacets[planeCount].edgeBisectorPlane[2] = planeCount;

            planeCount++;
        }
    }
    return planeCount;
}

/**
 * Adds a plane along each collision triangle edge, tilted by the neighbouring triangle's normal
 * (found by model_find_edges), and points the triangle's edge planes at them. The planes are written
 * after the `planeCount` face planes.
 */
static void model_build_edge_planes(ObjectModel *model, s32 planeCount) {
    s32 facesOffset;
    s32 verticesOffset;
    s32 nextFacesOffset;
    s32 i;
    s32 j;
    Vertex *v;
    s32 v1, v2;
    f32 x5, y5, z5;
    f32 x1, y1, z1;
    f32 x2, y2, z2;
    f32 x3, y3, z3;
    f32 nx, ny, nz;
    f32 mag;
    s32 facetIndex;
    s32 k;
    s32 l;
    s32 index;
    s32 planeOffset;

    facetIndex = 0;
    for (i = 0; i < model->numberOfBatches; i++) {
        facesOffset = model->batches[i].facesOffset;
        verticesOffset = model->batches[i].verticesOffset;
        nextFacesOffset = model->batches[i + 1].facesOffset;
        if (model->batches[i].flags & RENDER_NO_COLLISION) {
            nextFacesOffset = facesOffset - 1;
        }

        for (j = facesOffset; j < nextFacesOffset; j++, facetIndex++) {
            index = model->collisionFacets[facetIndex].basePlaneIndex << 2;
            nx = *(model->collisionPlanes + index + 0);
            ny = *(model->collisionPlanes + index + 1);
            nz = *(model->collisionPlanes + index + 2);

            for (k = 0; k < 3; k++) {
                l = k + 1;
                if (l >= 3) {
                    l = 0;
                }

                v1 = model->triangles[j].verticesArray[1 + k] + verticesOffset;
                v2 = model->triangles[j].verticesArray[1 + l] + verticesOffset;

                index = model->collisionFacets[facetIndex].edgeBisectorPlane[k] << 2;
                x5 = nx + *(model->collisionPlanes + index + 0);
                y5 = ny + *(model->collisionPlanes + index + 1);
                z5 = nz + *(model->collisionPlanes + index + 2);

                v = &model->vertices[v1];
                x1 = v->x;
                y1 = v->y;
                z1 = v->z;

                v = &model->vertices[v2];
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

                model->collisionFacets[facetIndex].edgeBisectorPlane[k] = planeCount;

                planeOffset = planeCount << 2;
                *(model->collisionPlanes + planeOffset + 0) = x5;
                *(model->collisionPlanes + planeOffset + 1) = y5;
                *(model->collisionPlanes + planeOffset + 2) = z5;
                *(model->collisionPlanes + planeOffset + 3) = -(x1 * x5 + y1 * y5 + z1 * z5);

                planeCount++;
            }
        }
    }
}

/**
 * Builds the model's collision facets: one plane per collision triangle plus one per edge.
 */
void model_init_collision(ObjectModel *model) {
    s32 planeCount;

    if (model->collisionFacets != NULL) {
        return;
    }

    planeCount = model_count_collision_faces(model);

    model->collisionFacets =
        (CollisionFacetPlanes *) mempool_alloc(planeCount * sizeof(CollisionFacetPlanes), COLOUR_TAG_RED);

    if (model->collisionFacets == NULL) {
        return;
    }

    model->collisionPlanes = (f32 *) mempool_alloc(planeCount * (sizeof(f32) * 16), COLOUR_TAG_RED);
    if (model->collisionPlanes == NULL) {
        mempool_free(model->collisionFacets);
        model->collisionFacets = NULL;
        return;
    }

    planeCount = model_build_face_planes(model);

    model->collisionFacetCount = planeCount;

    model_find_edges(model);

    model_build_edge_planes(model, planeCount);
}

void model_find_edges(ObjectModel *mdl) {
    s32 count;
    s32 i;
    s32 vertOffset;
    s32 triIndex;
    s32 startTri;
    s32 endTri;
    s32 nextVertIndex;
    s32 vertIndex;
    s32 vertIndex0;
    s32 vertIndex1;
    s32 result;
    s32 edgeVertIndex;
    s32 edgeBatchIndex;

    count = 0;
    for (i = 0; i < mdl->numberOfBatches; i++) {
        startTri = mdl->batches[i].facesOffset;
        vertOffset = mdl->batches[i].verticesOffset;
        endTri = mdl->batches[i + 1].facesOffset;
        if (mdl->batches[i].flags & RENDER_NO_COLLISION) {
            endTri = startTri - 1;
        }

        for (triIndex = startTri; triIndex < endTri; triIndex++, count++) {
            mdl->collisionFacets[count].basePlaneIndex = count;

            for (vertIndex = 0; vertIndex < 3; vertIndex++) {
                nextVertIndex = vertIndex + 1;
                if (nextVertIndex >= 3) {
                    nextVertIndex = 0;
                }

                vertIndex0 = mdl->triangles[triIndex].verticesArray[vertIndex + 1] + vertOffset;
                vertIndex1 = mdl->triangles[triIndex].verticesArray[nextVertIndex + 1] + vertOffset;

                result =
                    model_find_triangle_edge(mdl, triIndex, vertIndex0, vertIndex1, &edgeBatchIndex, &edgeVertIndex);
                if (result != -1) {
                    mdl->collisionFacets[count].edgeBisectorPlane[vertIndex] = result;
                } else {
                    mdl->collisionFacets[count].edgeBisectorPlane[vertIndex] = count;
                }
            }
        }
    }
}

s32 model_find_triangle_edge(ObjectModel *mdl, s32 skipTri, s32 edgeVert0, s32 edgeVert1, s32 *outBatchIndex,
                             s32 *outVertexIndex) {
    s32 i;
    s32 endTri;
    s32 count;
    Triangle *tri;
    s16 vertOffset;
    s16 startTri;
    s32 triIndex;
    s32 nextVertIndex;
    s32 vertIndex;
    s32 vertIndex0;
    s32 vertIndex1;

    count = 0;
    for (i = 0; i < mdl->numberOfBatches; i++) {
        startTri = mdl->batches[i].facesOffset;
        vertOffset = mdl->batches[i].verticesOffset;
        endTri = mdl->batches[i + 1].facesOffset;
        if (mdl->batches[i].flags & RENDER_NO_COLLISION) {
            endTri = startTri - 1;
        }
        for (triIndex = startTri; triIndex < endTri; triIndex++, count++) {
            if (triIndex != skipTri) {
                for (vertIndex = 0; vertIndex < 3; vertIndex++) {
                    nextVertIndex = vertIndex + 1;
                    if (nextVertIndex >= 3) {
                        nextVertIndex = 0;
                    }

                    vertIndex0 = mdl->triangles[triIndex].verticesArray[vertIndex + 1] + vertOffset;
                    vertIndex1 = mdl->triangles[triIndex].verticesArray[nextVertIndex + 1] + vertOffset;
                    if (vertices_edge_matches(mdl->vertices, edgeVert0, edgeVert1, vertIndex0, vertIndex1) != 0) {
                        *outVertexIndex = vertIndex;
                        *outBatchIndex = i;
                        return count;
                    }
                }
            }
        }
    }

    return -1;
}

s32 vertices_edge_matches(Vertex *vertices, s32 i1, s32 i2, s32 i3, s32 i4) {
    Vertex *a;
    Vertex *b;
    Vertex *c;
    Vertex *d;

#define NEARBY(a, b, x) (b->x - 4 < a->x && a->x < b->x + 4)

    if ((i1 == i3 && i2 == i4) || (i1 == i4 && i2 == i3)) {
        return 1;
    }

    a = &vertices[i1];
    b = &vertices[i2];
    c = &vertices[i3];
    d = &vertices[i4];

    if (NEARBY(a, c, x) && NEARBY(a, c, y) && NEARBY(a, c, z)) {
        if (NEARBY(b, d, x) && NEARBY(b, d, y) && NEARBY(b, d, z)) {
            return 2;
        }
    } else if (NEARBY(a, d, x) && NEARBY(a, d, y) && NEARBY(a, d, z)) {
        if (NEARBY(b, c, x) && NEARBY(b, c, y) && NEARBY(b, c, z)) {
            return 2;
        }
    }

    return 0;

#undef NEARBY
}

/**
 * Counts the vertices in batches that need normals: those not lit by vertex colours, and environment
 * mapped ones.
 */
static s16 model_count_normal_vertices(ObjectModel *model) {
    TriangleBatchInfo *batches;
    s16 i;
    s16 k;

    batches = model->batches;
    k = 0;
    for (i = 0; i < model->numberOfBatches; i++) {
        if (batches[i].miscData != BATCH_VTX_COL || (batches[i].flags & RENDER_ENVMAP)) {
            k += batches[i + 1].verticesOffset - batches[i].verticesOffset;
        }
    }
    return k;
}

/**
 * Calculates every triangle's unit face normal, stored as floating point.
 */
static void model_face_normals(ObjectModel *model, Vec3f *floatNorms) {
    Vertex *vertices;
    Triangle *triangles;
    TriangleBatchInfo *batches;
    s16 i;
    s16 j;
    s16 k;
    s16 vertIndex;
    s16 vertOffset;
    f32 length;

    batches = model->batches;
    vertices = model->vertices;
    triangles = model->triangles;
    for (i = 0; i < model->numberOfBatches; i++) {
        vertOffset = batches[i].verticesOffset;
        for (j = batches[i].facesOffset; j < batches[i + 1].facesOffset; j++) {
            f32 faceX[3];
            f32 faceY[3];
            f32 faceZ[3];
            for (k = 0; k < 3; k++) {
                vertIndex = triangles[j].verticesArray[k + 1] + vertOffset;
                faceX[k] = vertices[vertIndex].x;
                faceY[k] = vertices[vertIndex].y;
                faceZ[k] = vertices[vertIndex].z;
            }

            floatNorms[j].x =
                (faceZ[0] - faceZ[2]) * (faceY[0] - faceY[1]) - (faceY[0] - faceY[2]) * (faceZ[0] - faceZ[1]);
            floatNorms[j].y =
                (faceZ[0] - faceZ[1]) * (faceX[0] - faceX[2]) - (faceX[0] - faceX[1]) * (faceZ[0] - faceZ[2]);
            floatNorms[j].z =
                (faceX[0] - faceX[1]) * (faceY[0] - faceY[2]) - (faceX[0] - faceX[2]) * (faceY[0] - faceY[1]);
            length = sqrtf(floatNorms[j].x * floatNorms[j].x + floatNorms[j].y * floatNorms[j].y +
                           floatNorms[j].z * floatNorms[j].z);
            if (length != 0.0f) {
                floatNorms[j].x /= length;
                floatNorms[j].y /= length;
                floatNorms[j].z /= length;
            }
        }
    }
}

/**
 * Gives each vertex that needs a normal a slot in the normal table, sharing the slot of an earlier
 * vertex at the same position in a batch of the same kind (same misc data, or both environment
 * mapped). Batches with misc data 0xFE get a slot per vertex; other vertices get -1.
 * Returns the number of slots.
 */
static s16 model_assign_normal_slots(ObjectModel *model, s16 *vertexSlots) {
    Vertex *vertices;
    TriangleBatchInfo *batches;
    s16 i;
    s16 vertIndex;
    s16 prevBatch;
    s16 matchSlot;
    s16 otherVert;
    s16 slotCount;
    s16 vx, vy, vz;

    batches = model->batches;
    vertices = model->vertices;
    slotCount = 0;

    for (i = 0; i < model->numberOfBatches; i++) {
        for (vertIndex = batches[i].verticesOffset; vertIndex < batches[i + 1].verticesOffset; vertIndex++) {
            if (batches[i].miscData < 0xFE) {
                matchSlot = -1;
                vx = vertices[vertIndex].x;
                vy = vertices[vertIndex].y;
                vz = vertices[vertIndex].z;

                for (prevBatch = 0; prevBatch <= i && matchSlot < 0; prevBatch++) {
                    if (batches[i].miscData == batches[prevBatch].miscData) {
                        otherVert = batches[prevBatch].verticesOffset;
                        while (((prevBatch == i && otherVert < vertIndex) ||
                                (prevBatch != i && otherVert < batches[prevBatch + 1].verticesOffset)) &&
                               matchSlot < 0) {
                            if (vx == vertices[otherVert].x && vy == vertices[otherVert].y &&
                                vz == vertices[otherVert].z) {
                                matchSlot = vertexSlots[otherVert];
                            }
                            otherVert++;
                        }
                    }
                }
                if (matchSlot < 0) {
                    vertexSlots[vertIndex] = slotCount++;
                } else {
                    vertexSlots[vertIndex] = matchSlot;
                }
            } else if (batches[i].miscData == 0xFE) {
                vertexSlots[vertIndex] = slotCount++;
            } else if (batches[i].flags & RENDER_ENVMAP) {
                matchSlot = -1;
                vx = vertices[vertIndex].x;
                vy = vertices[vertIndex].y;
                vz = vertices[vertIndex].z;
                for (prevBatch = 0; prevBatch <= i && matchSlot < 0; prevBatch++) {
                    if (batches[prevBatch].flags & RENDER_ENVMAP) {
                        otherVert = batches[prevBatch].verticesOffset;
                        while (((prevBatch == i && otherVert < vertIndex) ||
                                (prevBatch != i && otherVert < batches[prevBatch + 1].verticesOffset)) &&
                               matchSlot < 0) {
                            if (vx == vertices[otherVert].x && vy == vertices[otherVert].y &&
                                vz == vertices[otherVert].z) {
                                matchSlot = vertexSlots[otherVert];
                            }
                            otherVert++;
                        }
                    }
                }

                if (matchSlot < 0) {
                    vertexSlots[vertIndex] = slotCount++;
                } else {
                    vertexSlots[vertIndex] = matchSlot;
                }
            } else {
                vertexSlots[vertIndex] = -1;
            }
        }
    }
    return slotCount;
}

/**
 * Sums the face normals of the triangles around each normal slot, then scales each sum to a
 * length of 0x2000.
 */
static void model_sum_vertex_normals(ObjectModel *model, Vec3f *floatNorms, s16 *vertexSlots, Vec3f *slotNormals,
                                     s16 slotCount) {
    Triangle *triangles;
    TriangleBatchInfo *batches;
    s16 i;
    s16 j;
    s16 k;
    s16 slot;
    s16 vertOffset;
    f32 length;

    batches = model->batches;
    triangles = model->triangles;
    for (k = 0; k < slotCount; k++) {
        slotNormals[k].x = 0.0f;
        slotNormals[k].y = 0.0f;
        slotNormals[k].z = 0.0f;
    }

    for (i = 0; i < model->numberOfBatches; i++) {
        if (batches[i].miscData != BATCH_VTX_COL || (batches[i].flags & RENDER_ENVMAP)) {
            vertOffset = batches[i].verticesOffset;
            for (j = batches[i].facesOffset; j < batches[i + 1].facesOffset; j++) {
                for (k = 0; k < 3; k++) {
                    slot = triangles[j].verticesArray[k + 1] + vertOffset;
                    slot = vertexSlots[slot];

                    if (slot >= 0) {
                        slotNormals[slot].x += floatNorms[j].x;
                        slotNormals[slot].y += floatNorms[j].y;
                        slotNormals[slot].z += floatNorms[j].z;
                    }
                }
            }
        }
    }

    for (k = 0; k < slotCount; k++) {
        length = sqrtf(slotNormals[k].x * slotNormals[k].x + slotNormals[k].y * slotNormals[k].y +
                       slotNormals[k].z * slotNormals[k].z);
        if (length != 0.0f) {
            slotNormals[k].x /= length * (1.0f / 0x2000);
            slotNormals[k].y /= length * (1.0f / 0x2000);
            slotNormals[k].z /= length * (1.0f / 0x2000);
        }
    }
}

// Returns 0 if successful, or 1 if an error occured.
s32 model_init_normals(ObjectModel *model) {
    Vec3f *floatNorms;
    Vec3s *normals;
    s16 vertIndex;
    s16 slot;
    s16 packedIndex;
    s16 normalVertexCount;
    s16 slotCount;
    Vec3f *slotNormals;
    s16 *vertexSlots;

    model->normals = NULL;

    normalVertexCount = model_count_normal_vertices(model);

    if (normalVertexCount > 0) {
        floatNorms = (Vec3f *) mempool_alloc(model->numberOfTriangles * sizeof(Vec3f), COLOUR_TAG_ORANGE);
        if (floatNorms == NULL) {
            return 1;
        }

        normals = (Vec3s *) mempool_alloc(normalVertexCount * sizeof(Vec3s), COLOUR_TAG_ORANGE);
        if (normals == NULL) {
            mempool_free(floatNorms);
            return 1;
        }

        model_face_normals(model, floatNorms);

        vertexSlots = (s16 *) mempool_alloc(model->numberOfVertices * sizeof(s16), COLOUR_TAG_ORANGE);
        if (vertexSlots == NULL) {
            mempool_free(floatNorms);
            mempool_free(normals);
            return 1;
        }

        slotCount = model_assign_normal_slots(model, vertexSlots);

        slotNormals = (Vec3f *) mempool_alloc(slotCount * sizeof(Vec3f), COLOUR_TAG_ORANGE);
        if (slotNormals == NULL) {
            mempool_free(floatNorms);
            mempool_free(normals);
            mempool_free(vertexSlots);
            return 1;
        }

        model_sum_vertex_normals(model, floatNorms, vertexSlots, slotNormals, slotCount);

        packedIndex = 0;
        for (vertIndex = 0; vertIndex < model->numberOfVertices; vertIndex++) {
            slot = vertexSlots[vertIndex];
            if (slot >= 0) {
                normals[packedIndex].x = slotNormals[slot].x;
                normals[packedIndex].y = slotNormals[slot].y;
                normals[packedIndex].z = slotNormals[slot].z;
                packedIndex++;
            }
        }

        model->normals = normals;
        mempool_free(vertexSlots);
        mempool_free(slotNormals);
        mempool_free(floatNorms);
    }
    return 0;
}

/**
 * Sets the animation end offset for new models when loading.
 * This lets you choose to load more animations than the model normally would.
 */
void model_anim_offset(s32 offset) {
    gModelAnimOffsetID = offset;
}

/**
 * Checks the animation table for the model ID for animation entries.
 * Zero entries just set the anim count to zero and return early.
 * Otherwise, allocate space for animation data for each animation and loads each into each data slot.
 * Returns nonzero if something went wrong.
 */
s32 model_anim_init(ObjectModel *model, s32 modelID) {
    s32 j;
    s32 end;
    ObjectModel_44 *allocAnimData;
    s32 start;
    s32 size;
    s32 assetOffset;
    s32 assetSize;
    s32 i;
    s32 i2;
    uintptr_t animAddress;
    s32 *animWords;

    start = gAnimationTable[modelID];
    end = gAnimationTable[modelID + 1];
    if (start == end) {
        model->numberOfAnimations = 0;
        return 0;
    }
    if (gModelAnimOffsetID != 0) {
        if (start + gModelAnimOffsetID < end) {
            end = start + gModelAnimOffsetID;
        }
    }
    model->numberOfAnimations = end - start;
    allocAnimData =
        (ObjectModel_44 *) mempool_alloc(model->numberOfAnimations * sizeof(ObjectModel_44), COLOUR_TAG_RED);
    model->animations = allocAnimData;
    if (allocAnimData == NULL) {
        return 1;
    }
    i = 0;
    i2 = 0;
    do {
        assetOffset = gObjectAnimationTable[start];
        animAddress = gObjectAnimationTable[start + 1] - assetOffset;
        assetSize = animAddress;
        if (asset_image_inflated()) {
            size = assetSize + 0x80;
        } else {
            size = gzip_size_uncompressed(ASSET_OBJECT_ANIMATIONS, assetOffset) + 0x80;
        }
        model->animations[i].animData = (u8 *) mempool_alloc(size, COLOUR_TAG_RED);
        if (model->animations[i].animData == NULL) {
            for (j = 0; j < i2; j++) {
                mempool_free(model->animations[j].animData);
            }
            mempool_free(model->animations);
            model->animations = NULL;
            return 1;
        }
        if (asset_image_inflated()) {
            asset_load(ASSET_OBJECT_ANIMATIONS, (uintptr_t) model->animations[i].anim, assetOffset, assetSize);
        } else {
            animAddress = (uintptr_t) (model->animations[i].animData + size) - assetSize;
            asset_load(ASSET_OBJECT_ANIMATIONS, animAddress, assetOffset, assetSize);
            gzip_inflate((u8 *) animAddress, (u8 *) model->animations[i].anim);
        }
        animWords = model->animations[i].anim;
        // Big-endian length word.
        pc_swap32_buf(animWords, 4);
        model->animations[i].animLength = *animWords;
        model->animations[i].anim++;
        // Keyframe-0 base pose: numberOfAnimatedVertices x 3 big-endian s16
        // deltas at animData+0xC (obj_animate). Keyframe headers are read
        // bytewise there and the per-keyframe deltas are s8 — neither needs
        // swapping.
        pc_swap16_buf(model->animations[i].animData + 0xC, model->numberOfAnimatedVertices * 6);
        i++;
        start++;
        i2++;
    } while (start < end);

    return 0;
}

void object_update_model_animation(Object *obj) {
    ObjectModel *mdl;
    ModelInstance *modInst;
    s32 limit;

    if (obj->modelIndex < 0) {
        obj->modelIndex = 0;
    }
    limit = obj->header->numberOfModelIds - 1;
    if (limit < obj->modelIndex) {
        obj->modelIndex = limit;
    }
    modInst = obj->modelInstances[obj->modelIndex];
    mdl = modInst->objModel;
    if (modInst->objModel->animations != NULL) {
        if (obj->animationID < 0) {
            obj->animationID = 0;
        }
        if (obj->animationID >= mdl->numberOfAnimations) {
            obj->animationID = mdl->numberOfAnimations - 1;
        }
        if (mdl->numberOfAnimations > 0) {
            limit = mdl->animations[obj->animationID].animLength - 2;
        } else {
            limit = 0;
        }
        if (obj->animFrame >> 4 < 0) {
            obj->animFrame = limit;
        }
        if (limit < obj->animFrame >> 4) {
            obj->animFrame = 0;
            modInst->animationID = -1;
        }
    }
}
