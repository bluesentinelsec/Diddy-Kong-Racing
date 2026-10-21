#include "particles.h"
#include "asset_loading.h"
#include "camera.h"
#include "libc/math.h"
#include "math_util.h"
#include "objects.h"
#include "textures_sprites.h"
#include "tracks.h"
#include <ultra64.h>
#include "pc_swap.h"

/************ .rodata ************/


/*********************************/

/************ .data ************/

s32 gNumTriangleParticles = 0;
s32 gTriangleParticleBufferFull = FALSE;
Particle *gTriangleParticleBuffer = NULL;
s32 gNumRectangleParticles = 0;
s32 gRectangleParticleBufferFull = FALSE;
Particle *gRectangleParticleBuffer = NULL;
s32 gNumSpriteParticles = 0;
s32 gSpriteParticleBufferFull = FALSE;
Particle *gSpriteParticleBuffer = NULL;
s32 gNumLineParticles = 0;
s32 gLineParticleBufferFull = FALSE;
Particle *gLineParticleBuffer = NULL;
s32 gNumPointParticles = 0;
s32 gPointParticleBufferFull = FALSE;
PointParticle *gPointParticleBuffer = NULL;

UNUSED s32 sParticleSpawnCounter = 0; // Only ever and checked for being less than 512.
Vertex *gParticleVertexBuffer = NULL;
Triangle *gParticleTriangleBuffer = NULL;
s32 gParticlesAssetTableCount = 0;
s32 *gParticlesAssets = NULL;

ParticleDescriptor **gParticlesAssetTable = NULL;
s32 gParticleBehavioursAssetTableCount = 0;
s32 *gParticleBehavioursAssets = NULL;
ParticleBehaviour **gParticleBehavioursAssetTable = NULL;
static ParticleBehaviour *sPcBehaviours = NULL; // host-layout behaviours, see init_particle_assets
ColourRGBA gParticleOverrideColor[2] = { { { { 0 } } }, { { { 0 } } } };

Triangle gLineParticleTriangles[5] = {
    // Use these 4 triangles if the particle is fully rendered
    { { { BACKFACE_DRAW, 0, 1, 2 } }, { { { 256, 0 } } }, { { { 0, 480 } } }, { { { 256, 480 } } } },
    { { { BACKFACE_DRAW, 0, 2, 3 } }, { { { 256, 0 } } }, { { { 256, 480 } } }, { { { 480, 480 } } } },
    { { { BACKFACE_DRAW, 1, 2, 4 } }, { { { 0, 0 } } }, { { { 256, 0 } } }, { { { 0, 480 } } } },
    { { { BACKFACE_DRAW, 2, 3, 5 } }, { { { 256, 0 } } }, { { { 480, 0 } } }, { { { 480, 480 } } } },
    // Use this one triangle if the particle is partially rendered
    { { { BACKFACE_DRAW, 0, 1, 3 } }, { { { 256, 0 } } }, { { { 0, 480 } } }, { { { 480, 480 } } } }
};

Vec3s gLineParticleVCoords[5] = {
    { { { 0, 511, 511 } } }, { { { 0, 511, 511 } } }, { { { 0, 0, 511 } } },
    { { { 0, 0, 511 } } },   { { { 0, 511, 511 } } },
};

Triangle gPointParticleTriangles[8] = {
    { { { BACKFACE_CULL, 0, 1, 5 } }, { { { 0, 0 } } }, { { { 128, 0 } } }, { { { 128, 511 } } } },
    { { { BACKFACE_CULL, 0, 5, 4 } }, { { { 0, 0 } } }, { { { 128, 511 } } }, { { { 0, 511 } } } },
    { { { BACKFACE_CULL, 1, 2, 6 } }, { { { 128, 0 } } }, { { { 256, 0 } } }, { { { 256, 511 } } } },
    { { { BACKFACE_CULL, 1, 6, 5 } }, { { { 128, 0 } } }, { { { 256, 511 } } }, { { { 128, 511 } } } },
    { { { BACKFACE_CULL, 2, 3, 7 } }, { { { 256, 0 } } }, { { { 384, 0 } } }, { { { 384, 511 } } } },
    { { { BACKFACE_CULL, 2, 7, 6 } }, { { { 256, 0 } } }, { { { 384, 511 } } }, { { { 256, 511 } } } },
    { { { BACKFACE_CULL, 3, 0, 4 } }, { { { 384, 0 } } }, { { { 512, 0 } } }, { { { 512, 511 } } } },
    { { { BACKFACE_CULL, 3, 4, 7 } }, { { { 384, 0 } } }, { { { 512, 511 } } }, { { { 384, 511 } } } },
};

Vec3s gPointParticleVCoords[8] = {
    { { { 0, 0, 511 } } }, { { { 0, 511, 511 } } }, { { { 0, 0, 511 } } }, { { { 0, 511, 511 } } },
    { { { 0, 0, 511 } } }, { { { 0, 511, 511 } } }, { { { 0, 0, 511 } } }, { { { 0, 511, 511 } } },
};

s16 gParticleTextureScrollOffset = 0;

f32 gParticleGravityTable[8] = { 0.0f, 0.1, 0.2f, 0.3f, 0.45f, 0.525f, 0.6f, 0.8f };

s32 gMaxTriangleParticles = 0;
s32 gMaxRectangleParticles = 0;
s32 gMaxSpriteParticles = 0;
s32 gMaxLineParticles = 0;
s32 gMaxPointParticles = 0;
Sprite **gParticleDummys = NULL; // Placeholder sprite refs to keep them loaded in memory.
s32 gParticleDummyCount = 0;     // Number of sprites in gParticleDummys

Vec2s gParticleCoordListTri[3] = {
    { { { 0, 8 } } },
    { { { 7, -4 } } },
    { { { -7, -4 } } },
};

Vec2s gParticleCoordListQuad[4] = {
    { { { -6, 6 } } },
    { { { 6, 6 } } },
    { { { 6, -6 } } },
    { { { -6, -6 } } },
};

ColourRGBA gVehicleTrackMarkColors[16] = {
    { { { 0, 0, 0, 0 } } },        // SURFACE_DEFAULT
    { { { 192, 8, 64, 255 } } },   // SURFACE_GRASS
    { { { 255, 96, 8, 255 } } },   // SURFACE_SAND
    { { { 64, 64, 64, 255 } } },   // SURFACE_ZIP_PAD
    { { { 0, 0, 0, 0 } } },        // SURFACE_STONE
    { { { 0, 0, 0, 0 } } },        // SURFACE_EGG_SPAWN
    { { { 0, 0, 0, 0 } } },        // SURFACE_EGG_01
    { { { 0, 0, 0, 0 } } },        // SURFACE_EGG_02
    { { { 0, 0, 0, 0 } } },        // SURFACE_EGG_03
    { { { 0, 0, 0, 0 } } },        // SURFACE_EGG_04
    { { { 0, 0, 0, 0 } } },        // SURFACE_FROZEN_WATER
    { { { 0, 0, 0, 0 } } },        // SURFACE_UNK0B
    { { { 0, 0, 0, 0 } } },        // SURFACE_TAJ_PAD
    { { { 16, 112, 255, 255 } } }, // SURFACE_SNOW
    { { { 0, 0, 0, 0 } } },        // SURFACE_UNK0E
    { { { 0, 0, 0, 0 } } },        // SURFACE_UNK0F
};

ColourRGBA sParticleColourRamp[10] = {
    { { { 64, 64, 64, 255 } } },  { { { 64, 64, 64, 255 } } },  { { { 0, 45, 0, 255 } } },
    { { { 0, 45, 0, 255 } } },    { { { 64, 60, 10, 255 } } },  { { { 64, 60, 10, 255 } } },
    { { { 64, 64, 255, 255 } } }, { { { 64, 64, 255, 255 } } }, { { { 64, 64, 64, 255 } } },
    { { { 64, 64, 64, 255 } } },
};

s32 gCurrentHovercraftParticleOpacity = 256;

/*******************************/

/************ .bss ************/

s32 gParticleUpdateRate;
s16 gHovercraftParticleOpacities[8];
// printf.c
// thread0_epc
/******************************/

/**
 * Deallocate all memory buffers used by particles.
 */
void reset_particles(void) {
    free_particle_buffers();
    free_particle_vertices_triangles();
    particle_free_dummy();
}

/**
 * Deallocate all memory buffers used by particles and unload all related assets
 */
void reset_particles_with_assets(void) {
    free_particle_buffers();
    free_particle_vertices_triangles();
    free_particle_assets();
    particle_free_dummy();
}

/**
 * Free dummy particles from RAM.
 */
void particle_free_dummy(void) {
    s32 i;

    if (gParticleDummys != NULL) {
        for (i = 0; i < gParticleDummyCount; i++) {
            sprite_free(gParticleDummys[i]);
        }
        mempool_free(gParticleDummys);
        gParticleDummys = NULL;
    }
}

/**
 * Free buffers allocated for particle objects.
 */
void free_particle_buffers(void) {
    if (gTriangleParticleBuffer != NULL) {
        mempool_free(gTriangleParticleBuffer);
        gTriangleParticleBuffer = NULL;
    }
    if (gRectangleParticleBuffer != NULL) {
        mempool_free(gRectangleParticleBuffer);
        gRectangleParticleBuffer = NULL;
    }
    if (gSpriteParticleBuffer != NULL) {
        mempool_free(gSpriteParticleBuffer);
        gSpriteParticleBuffer = NULL;
    }
    if (gLineParticleBuffer != NULL) {
        mempool_free(gLineParticleBuffer);
        gLineParticleBuffer = NULL;
    }
    if (gPointParticleBuffer != NULL) {
        mempool_free(gPointParticleBuffer);
        gPointParticleBuffer = NULL;
    }
}

/**
 * Free buffers allocated for triangles and vertices used in particle models.
 */
void free_particle_vertices_triangles(void) {
    if (gParticleVertexBuffer != NULL) {
        mempool_free(gParticleVertexBuffer);
        gParticleVertexBuffer = NULL;
    }
    if (gParticleTriangleBuffer != NULL) {
        mempool_free(gParticleTriangleBuffer);
        gParticleTriangleBuffer = NULL;
    }
}

/**
 * Unload all assets related to particles.
 */
void free_particle_assets(void) {
    if (sPcBehaviours != NULL) {
        mempool_free(sPcBehaviours);
        sPcBehaviours = NULL;
    }
    if (gParticlesAssets != NULL) {
        mempool_free(gParticlesAssets);
        gParticlesAssets = NULL;
    }
    if (gParticlesAssetTable != NULL) {
        mempool_free(gParticlesAssetTable);
        gParticlesAssetTable = NULL;
    }
    if (gParticleBehavioursAssets != NULL) {
        mempool_free(gParticleBehavioursAssets);
        gParticleBehavioursAssets = NULL;
    }
    if (gParticleBehavioursAssetTable != NULL) {
        mempool_free(gParticleBehavioursAssetTable);
        gParticleBehavioursAssetTable = NULL;
    }
}

// Particle assets are big-endian. ParticleDescriptor carries no pointers and is
// swapped in place; ParticleBehaviour ends in a pointer slot (colourLoop), so
// behaviours are unpacked into a host-layout array (sPcBehaviours) that the
// table points at instead of the raw image.
#define PARTICLE_BEHAVIOUR_DISK_SIZE 0xA0

static const PcField sParticleDescriptorFields[] = {
    PCU_FIELD(ParticleDescriptor, kind, 0x00, 2, PCU_BYTES),
    PCU_FIELD(ParticleDescriptor, flags, 0x02, 5, PCU_U16), // flags, textureID, textureFrameStep, lifeTime, lifeTimeRange
    PCU_FIELD(ParticleDescriptor, opacity, 0x0C, 2, PCU_BYTES),
    PCU_FIELD(ParticleDescriptor, opacityTimer, 0x0E, 1, PCU_U16),
    PCU_FIELD(ParticleDescriptor, scale, 0x10, 1, PCU_U32),
    PCU_FIELD(ParticleDescriptor, colour, 0x14, 4, PCU_BYTES),
    PCU_END,
};

static const PcField sParticleBehaviourFields[] = {
    PCU_FIELD(ParticleBehaviour, flags, 0x00, 1, PCU_U32),
    PCU_FIELD(ParticleBehaviour, emitterPos, 0x04, 3, PCU_U32),
    PCU_FIELD(ParticleBehaviour, sourceDistance, 0x10, 1, PCU_U32),
    PCU_FIELD(ParticleBehaviour, sourceRotation, 0x14, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, maxParticlesFromSamePos, 0x1A, 1, PCU_U16),
    PCU_FIELD(ParticleBehaviour, sourceAngularVelocity, 0x1C, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, emissionDirection, 0x22, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, maxParticlesInSameDir, 0x28, 1, PCU_U16),
    PCU_FIELD(ParticleBehaviour, emissionDirAngularVelocity, 0x2A, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, velocityModifier, 0x30, 3, PCU_U32),
    PCU_FIELD(ParticleBehaviour, emissionSpeed, 0x3C, 1, PCU_U32),
    PCU_FIELD(ParticleBehaviour, spawnInterval, 0x40, 2, PCU_U16), // spawnInterval, burstCount
    PCU_FIELD(ParticleBehaviour, rotation, 0x44, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, angularVelocity, 0x4A, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, scale, 0x50, 3, PCU_U32), // scale, scaleVelocity, movementParam
    PCU_FIELD(ParticleBehaviour, randomizationFlags, 0x5C, 2, PCU_U32), // randomizationFlags, sourceDistanceRange
    PCU_FIELD(ParticleBehaviour, sourceDirRange, 0x64, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, emissionDirRange, 0x6A, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, emissionSpeedRange, 0x70, 1, PCU_U32),
    PCU_FIELD(ParticleBehaviour, velocityModifierRange, 0x74, 3, PCU_U32),
    PCU_FIELD(ParticleBehaviour, rotationRange, 0x80, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, angularVelocityRange, 0x86, 3, PCU_U16),
    PCU_FIELD(ParticleBehaviour, scaleRange, 0x8C, 3, PCU_U32), // scaleRange, scaleVelocityRange, movementParamRange
    PCU_FIELD(ParticleBehaviour, colourRangeR, 0x98, 4, PCU_BYTES),
    PCU_FIELD(ParticleBehaviour, colourLoop, 0x9C, 1, PCU_SPTR), // misc-asset index until converted below
    PCU_END,
};

static void pc_swap32p(void *p) {
    pc_swap32_buf(p, 4);
}

// Colour loops live in the shared misc asset and can be referenced by more
// than one behaviour — swap each one only once.
static void pc_swap_colour_loop(ColorLoopEntry *cl) {
    static ColorLoopEntry *swapped[128];
    static s32 swappedCount = 0;
    s32 i;

    for (i = 0; i < swappedCount; i++) {
        if (swapped[i] == cl) {
            return;
        }
    }
    if (swappedCount < 128) {
        swapped[swappedCount++] = cl;
    }
    pc_swap32p(&cl[0].numEntries);
    // Entries [2 .. numEntries+1] are indexed at render time; each entry's
    // numEntries field is a BE s32, r/g/b/a are u8.
    for (i = 1; i < cl[0].numEntries + 2; i++) {
        pc_swap32p(&cl[i].numEntries);
    }
}

// Public entry point sharing the swap-once table above. The per-level HUD colour
// cycles reached through LevelHeader.unk74 (game.c) are the same misc-asset
// colour loops, but only the subset referenced by a particle behaviour gets
// swapped in the loop below — the rest reach pulse_light_reset still big-endian and
// its count walks off the pool. Routing those through here swaps each loop
// exactly once regardless of which consumer sees it first.
void pc_swap_colour_loop_shared(void *cl) {
    pc_swap_colour_loop((ColorLoopEntry *) cl);
}

/**
 * Load all assets related to particles.
 */
void init_particle_assets(void) {
    s32 i;

    free_particle_assets();
    gParticlesAssetTable = (ParticleDescriptor **) asset_table_load_pointers(ASSET_PARTICLES_TABLE);
    gParticlesAssetTableCount = -1;
    while (((s32) gParticlesAssetTable[gParticlesAssetTableCount + 1]) != -1) {
        gParticlesAssetTableCount++;
    }

    gParticlesAssets = (s32 *) asset_table_load(ASSET_PARTICLES);
    for (i = 0; i < gParticlesAssetTableCount; i++) {
        gParticlesAssetTable[i] = (ParticleDescriptor *) (((u8 *) gParticlesAssets) + ((s32) gParticlesAssetTable[i]));
        pc_swap_record(gParticlesAssetTable[i], sizeof(ParticleDescriptor), sParticleDescriptorFields);
    }

    gParticleBehavioursAssetTable = (ParticleBehaviour **) asset_table_load_pointers(ASSET_PARTICLE_BEHAVIORS_TABLE);
    gParticleBehavioursAssetTableCount = -1;
    while (((s32) gParticleBehavioursAssetTable[gParticleBehavioursAssetTableCount + 1]) != -1) {
        gParticleBehavioursAssetTableCount++;
    }

    gParticleBehavioursAssets = (s32 *) asset_table_load(ASSET_PARTICLE_BEHAVIORS);
    sPcBehaviours = mempool_alloc_safe(
        (gParticleBehavioursAssetTableCount > 0 ? gParticleBehavioursAssetTableCount : 1) * sizeof(ParticleBehaviour),
        COLOUR_TAG_GREY);
    for (i = 0; i < gParticleBehavioursAssetTableCount; i++) {
        pc_unpack(&sPcBehaviours[i], sizeof(ParticleBehaviour),
                  (u8 *) gParticleBehavioursAssets + (uintptr_t) gParticleBehavioursAssetTable[i],
                  PARTICLE_BEHAVIOUR_DISK_SIZE, sParticleBehaviourFields);
        gParticleBehavioursAssetTable[i] = &sPcBehaviours[i];
        if (((u32) gParticleBehavioursAssetTable[i]->colourLoop) != 0xFFFFFFFF) {
            gParticleBehavioursAssetTable[i]->colourLoop =
                (ColorLoopEntry *) get_misc_asset((s32) gParticleBehavioursAssetTable[i]->colourLoop);
            pc_swap_colour_loop(gParticleBehavioursAssetTable[i]->colourLoop);
        }
    }
}

/**
 * Allocate buffers for particle objects.
 * Generate particle shapes.
 * Load sprites from the dummy sprite ID list.
 */
void init_particle_buffers(s32 maxTriangleParticles, s32 maxRectangleParticles, s32 maxSpriteParticles,
                           s32 maxLineParticles, s32 maxPointParticles) {
    s16 *assetBuffer;
    s32 i;
    Vertex *verticesTemp;
    Triangle *trianglesTemp;
    ParticleModel *particleModelsBuffer;

    gParticleOverrideColor->word = 0;

    if (maxTriangleParticles < 0) {
        maxTriangleParticles = 0x10;
    }
    if (maxRectangleParticles < 0) {
        maxRectangleParticles = 0x10;
    }
    if (maxSpriteParticles < 0) {
        maxSpriteParticles = 0xD0;
    }
    if (maxLineParticles < 0) {
        maxLineParticles = 0xA0;
    }
    if (maxPointParticles < 0) {
        maxPointParticles = 0x40;
    }

    gMaxTriangleParticles = maxTriangleParticles;
    gTriangleParticleBufferFull = FALSE;
    gMaxRectangleParticles = maxRectangleParticles;
    gRectangleParticleBufferFull = FALSE;
    gMaxSpriteParticles = maxSpriteParticles;
    gSpriteParticleBufferFull = FALSE;
    gMaxLineParticles = maxLineParticles;
    gLineParticleBufferFull = FALSE;
    gMaxPointParticles = maxPointParticles;
    gPointParticleBufferFull = FALSE;
    free_particle_vertices_triangles();

    // Line and point models share the static triangle tables, so only triangle and
    // rectangle particles need triangles of their own.
    gParticleVertexBuffer = (Vertex *) mempool_alloc_safe(
        ((maxTriangleParticles * 3) + (maxRectangleParticles * 4) + (maxLineParticles * 6) + (maxPointParticles * 16)) *
            sizeof(Vertex),
        COLOUR_TAG_SEMITRANS_GREY);
    gParticleTriangleBuffer = (Triangle *) mempool_alloc_safe(
        (maxTriangleParticles + (maxRectangleParticles * 2)) * sizeof(Triangle), COLOUR_TAG_SEMITRANS_GREY);

    sParticleSpawnCounter = 0;
    free_particle_buffers();

    if (gMaxTriangleParticles > 0) {
        gNumTriangleParticles = 0;
        gTriangleParticleBuffer = (Particle *) mempool_alloc_safe(
            maxTriangleParticles * (sizeof(Particle) + sizeof(ParticleModel)), COLOUR_TAG_SEMITRANS_GREY);
    }
    if (gMaxRectangleParticles > 0) {
        gNumRectangleParticles = 0;
        gRectangleParticleBuffer = (Particle *) mempool_alloc_safe(
            maxRectangleParticles * (sizeof(Particle) + sizeof(ParticleModel)), COLOUR_TAG_SEMITRANS_GREY);
    }
    if (gMaxSpriteParticles > 0) {
        gNumSpriteParticles = 0;
        gSpriteParticleBuffer =
            (Particle *) mempool_alloc_safe(maxSpriteParticles * sizeof(Particle), COLOUR_TAG_SEMITRANS_GREY);
    }
    if (gMaxLineParticles > 0) {
        gNumLineParticles = 0;
        gLineParticleBuffer = (Particle *) mempool_alloc_safe(
            maxLineParticles * (sizeof(Particle) + sizeof(ParticleModel)), COLOUR_TAG_SEMITRANS_GREY);
    }
    if (gMaxPointParticles > 0) {
        gNumPointParticles = 0;
        gPointParticleBuffer = (PointParticle *) mempool_alloc_safe(
            maxPointParticles * (sizeof(PointParticle) + sizeof(ParticleModel)), COLOUR_TAG_SEMITRANS_GREY);
    }

    verticesTemp = gParticleVertexBuffer;
    trianglesTemp = gParticleTriangleBuffer;

    particleModelsBuffer =
        (ParticleModel *) ((u8 *) gTriangleParticleBuffer + (gMaxTriangleParticles * (sizeof(Particle))));
    for (i = 0; i < gMaxTriangleParticles; i++) {
        gTriangleParticleBuffer[i].model = &particleModelsBuffer[i];
        init_triangle_particle_model(gTriangleParticleBuffer[i].model, &verticesTemp, &trianglesTemp);
    }

    particleModelsBuffer =
        (ParticleModel *) ((u8 *) gRectangleParticleBuffer + (gMaxRectangleParticles * sizeof(Particle)));
    for (i = 0; i < gMaxRectangleParticles; i++) {
        gRectangleParticleBuffer[i].model = &particleModelsBuffer[i];
        init_rectangle_particle_model(gRectangleParticleBuffer[i].model, &verticesTemp, &trianglesTemp);
    }

    particleModelsBuffer = (ParticleModel *) (((u8 *) gLineParticleBuffer) + (gMaxLineParticles * sizeof(Particle)));
    for (i = 0; i < gMaxLineParticles; i++) {
        gLineParticleBuffer[i].model = &particleModelsBuffer[i];
        trianglesTemp = gLineParticleTriangles;
        init_line_particle_model(gLineParticleBuffer[i].model, &verticesTemp, &trianglesTemp);
    }

    particleModelsBuffer =
        (ParticleModel *) (((u8 *) gPointParticleBuffer) + (gMaxPointParticles * sizeof(PointParticle)));
    for (i = 0; i < gMaxPointParticles; i++) {
        gPointParticleBuffer[i].base.model = &particleModelsBuffer[i];
        trianglesTemp = gPointParticleTriangles;
        init_point_particle_model(gPointParticleBuffer[i].base.model, &verticesTemp, &trianglesTemp);
    }

    for (i = 0; i < gMaxTriangleParticles; i++) {
        gTriangleParticleBuffer[i].kind = PARTICLE_KIND_NONE;
    }
    for (i = 0; i < gMaxRectangleParticles; i++) {
        gRectangleParticleBuffer[i].kind = PARTICLE_KIND_NONE;
    }
    for (i = 0; i < gMaxSpriteParticles; i++) {
        gSpriteParticleBuffer[i].kind = PARTICLE_KIND_NONE;
    }
    for (i = 0; i < gMaxLineParticles; i++) {
        gLineParticleBuffer[i].kind = PARTICLE_KIND_NONE;
    }
    for (i = 0; i < gMaxPointParticles; i++) {
        gPointParticleBuffer[i].base.kind = PARTICLE_KIND_NONE;
    }

    if (gParticleDummys == NULL) {
        assetBuffer = (s16 *) asset_table_load(ASSET_DUMMY_PARTICLE_IDS);
        {
            // Big-endian s16 sprite-id table (helper in linux/reimpl.c).
            extern void pc_swap16_buf(void *buf, u32 numBytes);
            pc_swap16_buf(assetBuffer, asset_table_size(ASSET_DUMMY_PARTICLE_IDS));
        }
        gParticleDummyCount = 0;
        while (assetBuffer[gParticleDummyCount] != -1) {
            gParticleDummyCount++;
        }

        gParticleDummys = mempool_alloc_safe(gParticleDummyCount * sizeof(Sprite *), COLOUR_TAG_BLUE);
        for (i = 0; i < gParticleDummyCount; i++) {
            gParticleDummys[i] = (Sprite *) tex_load_sprite(assetBuffer[i] & 0x3FFF, 1);
        }
        mempool_free(assetBuffer);
    }
}

/**
 * Generate a triangle shaped particle mesh.
 * Use a lookup table to generate the vertices.
 */
void init_triangle_particle_model(ParticleModel *model, Vertex **vtx, Triangle **triangles) {
    s16 i;
    Vertex *tempVtx;
    Triangle *tempTri;
    Vec2s *coordList;

    model->vertexCount = 3;
    model->vertices = *vtx;
    tempVtx = *vtx;
    coordList = &gParticleCoordListTri[0];
    for (i = 0; i < 3; i++) {
        tempVtx->x = coordList->x;
        tempVtx->y = coordList->y;
        coordList++;
        tempVtx->z = 0;
        tempVtx->r = 255;
        tempVtx->g = 255;
        tempVtx->b = 255;
        tempVtx->a = 255;
        tempVtx++;
    }
    *vtx = tempVtx;
    model->triangleCount = 1;
    model->triangles = *triangles;
    tempTri = *triangles;
    tempTri->flags = BACKFACE_DRAW;
    tempTri->vi0 = 2;
    tempTri->vi1 = 1;
    tempTri->vi2 = 0;
    tempTri++;
    *triangles = tempTri;
}

/**
 * Generate a square shaped particle mesh.
 * Use a lookup table to generate the vertices.
 */
void init_rectangle_particle_model(ParticleModel *model, Vertex **vtx, Triangle **triangles) {
    s16 i;
    Vertex *tempVtx;
    Triangle *tempTri;
    Vec2s *coordList;

    model->vertexCount = 4;
    model->vertices = *vtx;
    tempVtx = *vtx;
    coordList = &gParticleCoordListQuad[0];
    for (i = 0; i < 4; i++) {
        tempVtx->x = coordList->x;
        tempVtx->y = coordList->y;
        coordList++;
        tempVtx->z = 0;
        tempVtx->r = 255;
        tempVtx->g = 255;
        tempVtx->b = 255;
        tempVtx->a = 255;
        tempVtx++;
    }
    *vtx = tempVtx;
    model->triangleCount = 2;
    model->triangles = *triangles;
    tempTri = *triangles;
    tempTri[0].flags = BACKFACE_DRAW;
    tempTri[0].vi0 = 3;
    tempTri[0].uv0.u = 0;
    tempTri[0].vi1 = 1;
    tempTri[0].uv1.v = 0;
    tempTri[0].vi2 = 0;
    tempTri[0].uv2.u = 0;
    tempTri[0].uv2.v = 0;
    tempTri[1].flags = BACKFACE_DRAW;
    tempTri[1].vi0 = 3;
    tempTri[1].uv0.u = 0;
    tempTri[1].vi1 = 2;
    tempTri[1].vi2 = 1;
    tempTri[1].uv2.v = 0;
    tempTri += 2;
    *triangles = tempTri;
}

/**
 * Initializes the model for a line particle without setting coordinates.
 */
void init_line_particle_model(ParticleModel *model, Vertex **vtx, Triangle **triangles) {
    s32 i;
    Vertex *tempVtx;

    model->vertexCount = 6;
    model->triangleCount = 4;
    model->vertices = *vtx;
    model->triangles = *triangles;
    tempVtx = *vtx;
    for (i = 0; i < 6; i++) {
        tempVtx->r = 255;
        tempVtx->g = 255;
        tempVtx->b = 255;
        tempVtx->a = 255;
        tempVtx++;
    }
    *vtx = tempVtx;
}

/**
 * Initializes the model for a point particle without setting coordinates.
 */
void init_point_particle_model(ParticleModel *model, Vertex **vtx, Triangle **triangles) {
    s32 i;
    Vertex *tempVtx;

    model->vertexCount = 8;
    model->triangleCount = 8;
    model->vertices = *vtx;
    model->triangles = *triangles;
    tempVtx = *vtx;
    for (i = 0; i < 16; i++) {
        tempVtx->r = 255;
        tempVtx->g = 255;
        tempVtx->b = 255;
        tempVtx->a = 255;
        tempVtx++;
    }
    *vtx = tempVtx;
}

/**
 * Sets texture coordinates for the triangle of a triangle particle.
 */
void set_triangle_texture_coords(Particle *particle) {
    ParticleModel *model;
    Triangle *tri;
    s16 texV, texU;

    model = particle->model;
    tri = model->triangles;
    texU = (model->texture->width - 1) << 5;
    texV = (model->texture->height - 1) << 5;

    tri->uv0.u = texU >> 1;
    tri->uv0.v = 0;
    tri->uv1.u = 0;
    tri->uv1.v = texV;
    tri->uv2.u = texU;
    tri->uv2.v = texV;
}

/**
 * Sets texture coordinates for the triangle of a rectangle particle.
 */
void set_rectangle_texture_coords(Particle *particle) {
    ParticleModel *model;
    Triangle *tris;
    s16 texV, texU;

    model = particle->model;
    tris = model->triangles;
    texU = (model->texture->width - 1) << 5;
    texV = (model->texture->height - 1) << 5;

    tris[0].uv0.v = texV;
    tris[0].uv1.u = texU;
    tris[1].uv0.v = texU;
    tris[1].uv1.u = texU;
    tris[1].uv1.v = texV;
    tris[1].uv2.u = texU;
}

/**
 * Sets the particle type, behaviour, and position for the emitter.
 * Ignores the emitter position specified for the current behaviour.
 * Does nothing if the particle type and behaviour are the same as previously set.
 */
void emitter_change_settings(ParticleEmitter *emitter, s32 behaviourID, s32 particleID, s16 posX, s16 posY, s16 posZ) {
    ParticleBehaviour *behaviour;
    if (particleID >= gParticlesAssetTableCount) {
        particleID = 0;
    }
    if (behaviourID >= gParticleBehavioursAssetTableCount) {
        behaviourID = 0;
    }
    behaviour = gParticleBehavioursAssetTable[behaviourID];
    if (emitter->descriptorID != particleID || behaviour != emitter->behaviour) {
        emitter_cleanup(emitter);
        emitter_init_with_pos(emitter, behaviourID, particleID, posX, posY, posZ);
    }
}

/**
 * Initializes the emitter with the specified particle type and behaviour.
 */
void emitter_init(ParticleEmitter *emitter, s32 behaviourID, s32 particleID) {
    ParticleBehaviour *behaviour;

    if (behaviourID < gParticleBehavioursAssetTableCount) {
        behaviour = gParticleBehavioursAssetTable[behaviourID];
        emitter_init_with_pos(emitter, behaviourID, particleID, behaviour->emitterPos.x, behaviour->emitterPos.y,
                              behaviour->emitterPos.z);
    }
}

/**
 * Main function to initialize the emitter.
 * Sets the geometry used for generating particles, depending on the particle type.
 */
void emitter_init_with_pos(ParticleEmitter *emitter, s32 behaviourID, s32 particleID, s16 posX, s16 posY, s16 posZ) {
    ParticleBehaviour *behaviour;

    behaviour = gParticleBehavioursAssetTable[behaviourID];
    emitter->descriptorID = particleID;
    emitter->behaviour = behaviour;
    emitter->position.x = posX;
    emitter->position.y = posY;
    emitter->position.z = posZ;
    emitter->colourIndex = 0;

    if (behaviour->flags & PARTICLE_LINE) {
        emitter->flags = PARTICLE_LINE;
        emitter->lineOpacity = 0;
        emitter->lineRefPoint.x = 0.0f;
        emitter->lineRefPoint.y = 0.0f;
        emitter->lineRefPoint.z = 0.0f;
    } else if (behaviour->flags & PARTICLE_POINT) {
        emitter->pointCount = 0;
        emitter->flags = PARTICLE_POINT;
        // Maximum number of points equals the particle's lifetime
        if (gParticlesAssetTable[particleID]->lifeTime <= 255) {
            emitter->maxPointCount = gParticlesAssetTable[particleID]->lifeTime;
        } else {
            emitter->maxPointCount = 255;
        }
        emitter->refPoints = (PointParticle **) mempool_alloc_safe(emitter->maxPointCount * sizeof(PointParticle *),
                                                                   COLOUR_TAG_SEMITRANS_GREY);
        emitter->pointSourceRotation.y_rotation = behaviour->sourceRotation.y_rotation;
        emitter->pointSourceRotation.x_rotation = behaviour->sourceRotation.x_rotation;
        emitter->pointEmissionDirection.y_rotation = behaviour->emissionDirection.y_rotation;
        emitter->pointEmissionDirection.x_rotation = behaviour->emissionDirection.x_rotation;
    } else {
        emitter->flags = PARTICLE_FLAGS_NONE;
        emitter->sourceRotation.y_rotation = behaviour->sourceRotation.y_rotation;
        emitter->sourceRotation.x_rotation = behaviour->sourceRotation.x_rotation;
        emitter->sourceRotation.z_rotation = behaviour->sourceRotation.z_rotation;
        emitter->emissionDirection.y_rotation = behaviour->emissionDirection.y_rotation;
        emitter->emissionDirection.x_rotation = behaviour->emissionDirection.x_rotation;
        emitter->emissionDirection.z_rotation = behaviour->emissionDirection.z_rotation;
    }
}

/**
 * Scrolls textures downward for point and line particles.
 * Texture state repeats in an 8-frame loop.
 */
void scroll_particle_textures(s32 updateRate) {
    s32 i;
    s32 j;

    gParticleTextureScrollOffset = (gParticleTextureScrollOffset + (updateRate << 6)) & 0x1FF;

    for (i = 0, j = 0; i < ARRAY_COUNT(gLineParticleTriangles); i++) {
        gLineParticleTriangles[i].uv0.v = gLineParticleVCoords[j].s[0] + gParticleTextureScrollOffset;
        gLineParticleTriangles[i].uv1.v = gLineParticleVCoords[j].s[1] + gParticleTextureScrollOffset;
        gLineParticleTriangles[i].uv2.v = gLineParticleVCoords[j].s[2] + gParticleTextureScrollOffset;
        j++;
    }

    for (i = 0, j = 0; i < ARRAY_COUNT(gPointParticleTriangles); i++) {
        gPointParticleTriangles[i].uv0.v = gPointParticleVCoords[j].s[0] + gParticleTextureScrollOffset;
        gPointParticleTriangles[i].uv1.v = gPointParticleVCoords[j].s[1] + gParticleTextureScrollOffset;
        gPointParticleTriangles[i].uv2.v = gPointParticleVCoords[j].s[2] + gParticleTextureScrollOffset;
        j++;
    }
}

/**
 * Initializes the emitter if it has been activated for an object.
 * Sets the geometry used for generating particles, depending on the particle type.
 */
void obj_enable_emitter(Object *obj, s32 emitterIndex) {
    s32 i;
    ParticleEmitter *emitter;
    ParticleBehaviour *behaviour;

    emitter = &obj->particleEmitter[emitterIndex];
    behaviour = emitter->behaviour;
    emitter->timeFromLastSpawn = 0;

    if (emitter->flags & PARTICLE_LINE) {
        emitter->lineRefPoint.x = obj->trans.x_position;
        emitter->lineRefPoint.y = obj->trans.y_position;
        emitter->lineRefPoint.z = obj->trans.z_position;
    } else if (emitter->flags & PARTICLE_POINT) {
        emitter->point_opacity = gParticlesAssetTable[emitter->descriptorID]->colour.a << 8;

        if (emitter->pointCount > 0) { // Useless if statement, since the loop already does this.
            for (i = 0; i < emitter->pointCount; i++) {
                PointParticle *temp = emitter->refPoints[i];
                temp->base.destroyTimer = 0;
            }
        }
        if (behaviour->flags & PARTICLE_SOURCE_OFFSET_ENABLED) {
            emitter->pointSourceRotation.y_rotation = behaviour->sourceRotation.y_rotation;
            emitter->pointSourceRotation.x_rotation = behaviour->sourceRotation.x_rotation;
        }
        if (behaviour->flags & PARTICLE_SOURCE_EMITS_WITH_VELOCITY) {
            emitter->pointEmissionDirection.y_rotation = behaviour->emissionDirection.y_rotation;
            emitter->pointEmissionDirection.x_rotation = behaviour->emissionDirection.x_rotation;
        }
    } else {
        if (behaviour->flags & PARTICLE_SOURCE_OFFSET_ENABLED) {
            emitter->sourceRotationCounter = 0;
            emitter->sourceRotation.y_rotation = behaviour->sourceRotation.y_rotation;
            emitter->sourceRotation.x_rotation = behaviour->sourceRotation.x_rotation;
            emitter->sourceRotation.z_rotation = behaviour->sourceRotation.z_rotation;
        }
        if (behaviour->flags & PARTICLE_SOURCE_EMITS_WITH_VELOCITY) {
            emitter->emissionDirRotationCounter = 0;
            emitter->emissionDirection.y_rotation = behaviour->emissionDirection.y_rotation;
            emitter->emissionDirection.x_rotation = behaviour->emissionDirection.x_rotation;
            emitter->emissionDirection.z_rotation = behaviour->emissionDirection.z_rotation;
        }
    }
    emitter->flags &= ~PARTICLE_POINT_EMITTER_DISABLED;
    emitter->flags |= PARTICLE_EMITTER_ENABLED | PARTICLE_EMITTER_AWAITING_SPAWN;
    obj->numActiveEmitters++;
}

/**
 * Deinitializes the emitter if it was deactivated for a given object.
 */
void obj_disable_emitter(Object *obj, s32 emitterIndex) {
    ParticleEmitter *emitter;

    emitter = &obj->particleEmitter[emitterIndex];

    emitter->flags &= ~PARTICLE_EMITTER_ENABLED;
    obj->numActiveEmitters--;
}

/**
 * Car dust (emitters 0-9) is tinted towards the surface colour ramp the harder the car
 * counter-steers in a drift. Below a small threshold the descriptor colour is kept.
 */
static void car_dust_override_colour(Object_Racer *racer, ParticleDescriptor *descriptor, s32 emitterIndex) {
    ColourRGBA *ramp;
    s32 slide;
    s32 blend;
    s32 alphaBlend;
    s32 alpha;

    slide = racer->driftCounterSteerTimer;
    if (slide < 0) {
        slide = -slide;
    }
    slide -= 24;
    if (slide <= 0) {
        return;
    }
    if (slide > 32) {
        slide = 32;
    }
    ramp = &sParticleColourRamp[emitterIndex];
    alphaBlend = slide << 4;
    blend = alphaBlend - ((slide * slide) >> 2);
    gParticleOverrideColor[0].word = ((((ramp->r - descriptor->colour.r) * blend) >> 8) + descriptor->colour.r) << 24;
    gParticleOverrideColor[0].word |= (descriptor->colour.g + ((((ramp->g - descriptor->colour.g) * blend)) >> 8))
                                      << 16;
    gParticleOverrideColor[0].word |= (descriptor->colour.b + ((((ramp->b - descriptor->colour.b) * blend)) >> 8)) << 8;
    if (slide > 16) {
        alphaBlend = 256;
    }
    alpha = descriptor->colour.a;
    gParticleOverrideColor[0].word |= alpha + (((ramp->a - alpha) * alphaBlend) >> 8);
}

/**
 * Car emitters 10-13 lay tyre marks, each following one wheel's surface colour.
 * Returns FALSE when that wheel is off the ground and no mark should be laid.
 */
static s32 car_tyre_mark_colour(Object_Racer *racer, s32 emitterIndex) {
    s32 wheel;

    switch (emitterIndex) {
        case 10:
            wheel = 2;
            break;
        case 11:
            wheel = 3;
            break;
        case 12:
            wheel = 0;
            break;
        case 13:
            wheel = 1;
            break;
        default:
            return TRUE;
    }
    gParticleOverrideColor[0].word = gVehicleTrackMarkColors[racer->wheel_surfaces[wheel] & 0xF].word;
    return racer->wheel_surfaces[wheel] != SURFACE_NONE;
}

/**
 * Hovercraft spray (emitters 2 and 3) fades in over a few frames after the effect starts.
 */
static void hovercraft_particle_fade_in(Object_Racer *racer, s32 updateRate) {
    s32 opacity;

    opacity = gHovercraftParticleOpacities[racer->racerIndex & 7];
    opacity += updateRate * 4;
    if (opacity > 256) {
        opacity = 256;
    }
    gHovercraftParticleOpacities[racer->racerIndex & 7] = opacity;
    gCurrentHovercraftParticleOpacity = opacity;
}

/**
 * Keep an active vehicle emitter running. Line and point emitters are fed every frame;
 * the others spawn on their behaviour's interval.
 */
static void vehicle_emitter_tick(Object *racerObj, s32 emitterIndex, s32 updateRate) {
    ParticleEmitter *emitter;

    emitter = &racerObj->particleEmitter[emitterIndex];
    if (!(emitter->flags & PARTICLE_EMITTER_ENABLED)) {
        obj_enable_emitter(racerObj, emitterIndex);
    }
    if (emitter->flags & (PARTICLE_LINE | PARTICLE_POINT)) {
        obj_trigger_emitter(racerObj, emitter);
    } else {
        emitter->timeFromLastSpawn += updateRate;
        if (emitter->timeFromLastSpawn >= emitter->behaviour->spawnInterval) {
            obj_trigger_emitter(racerObj, emitter);
        }
    }
}

/**
 * Shut down a vehicle emitter whose effect has stopped. A point emitter stays enabled
 * until its trail has died out; a line emitter drops its opacity first.
 */
static void vehicle_emitter_release(Object *racerObj, s32 emitterIndex) {
    ParticleEmitter *emitter;

    emitter = &racerObj->particleEmitter[emitterIndex];
    if (emitter->flags & PARTICLE_LINE) {
        emitter->lineOpacity = 0;
        obj_disable_emitter(racerObj, emitterIndex);
    } else if (emitter->flags & PARTICLE_POINT) {
        emitter->flags |= PARTICLE_POINT_EMITTER_DISABLED;
        if (emitter->pointCount == 0) {
            obj_disable_emitter(racerObj, emitterIndex);
        }
    } else {
        obj_disable_emitter(racerObj, emitterIndex);
    }
}

/**
 * Creates particles for vehicle-related effects like dust, tyre marks, and wing trails.
 * The colour of the particles is determined by the surface type the vehicle is moving on, which is why this logic is
 * separated.
 */
void update_vehicle_particles(Object *racerObj, s32 updateRate) {
    Object_Racer *racer;
    ParticleEmitter *emitter;
    s32 i;
    s32 emitting;
    s8 vehicleId;
    u32 emittersEnabled;

    racer = racerObj->racer;
    emittersEnabled = racerObj->particleEmittersEnabled;
    vehicleId = racer->vehicleID;
    object_do_player_tumble(racerObj);
    for (i = 0; i < racerObj->header->particleCount; i++) {
        emitter = &racerObj->particleEmitter[i];
        if (emittersEnabled & OBJ_EMIT_1) {
            emitting = TRUE;
            switch (vehicleId) {
                case VEHICLE_CAR:
                    if (i < 10) {
                        car_dust_override_colour(racer, gParticlesAssetTable[emitter->descriptorID], i);
                    } else {
                        emitting = car_tyre_mark_colour(racer, i);
                    }
                    break;
                case VEHICLE_PLANE:
                    // Wing-tip trails take the colour of the surface under each wheel.
                    if (i == 0 || i == 1) {
                        gParticleOverrideColor[0].word = gVehicleTrackMarkColors[racer->wheel_surfaces[i] & 0xF].word;
                    }
                    break;
                case VEHICLE_HOVERCRAFT:
                    if (i == 2 || i == 3) {
                        hovercraft_particle_fade_in(racer, updateRate);
                    }
                    break;
            }
            if (emitting) {
                vehicle_emitter_tick(racerObj, i, updateRate);
            }
        } else if (emitter->flags & PARTICLE_EMITTER_ENABLED) {
            vehicle_emitter_release(racerObj, i);
            if (i == 2 || i == 3) {
                gHovercraftParticleOpacities[racer->racerIndex & 7] = 0;
            }
        }
        gParticleOverrideColor[0].word = 0;
        emittersEnabled >>= 1;
        gCurrentHovercraftParticleOpacity = 256;
    }

    object_undo_player_tumble(racerObj);
}

/**
 * Update all particle emitters attached to the object and spawn new particles
 */
void obj_spawn_particle(Object *obj, s32 updateRate) {
    s32 new_opacity;
    s32 i;
    u32 bits;

    bits = obj->particleEmittersEnabled;
    for (i = 0; i < obj->header->particleCount; i++) {
        if (bits & 1) {
            if (!(obj->particleEmitter[i].flags & PARTICLE_EMITTER_ENABLED)) {
                obj_enable_emitter(obj, i);
            }
            if (obj->particleEmitter[i].flags & PARTICLE_LINE) {
                obj_trigger_emitter(obj, &obj->particleEmitter[i]);
            } else if ((obj->particleEmitter[i].flags & PARTICLE_POINT)) {
                obj_trigger_emitter(obj, &obj->particleEmitter[i]);
            } else {
                obj->particleEmitter[i].timeFromLastSpawn += updateRate;
                if (obj->particleEmitter[i].timeFromLastSpawn >= obj->particleEmitter[i].behaviour->spawnInterval) {
                    obj_trigger_emitter(obj, &obj->particleEmitter[i]);
                }
            }
        } else {
            if (obj->particleEmitter[i].flags & PARTICLE_EMITTER_ENABLED) {
                if (obj->particleEmitter[i].flags & PARTICLE_LINE) {
                    // Line particles are not immediately disabled, but instead, they gradually fade away over a few
                    // frames.
                    ParticleEmitter *emitter = &obj->particleEmitter[i];
                    new_opacity = emitter->lineOpacity - 64;
                    if (new_opacity < 0) {
                        new_opacity = 0;
                    }
                    emitter->lineOpacity = new_opacity;
                    obj_trigger_emitter(obj, emitter);
                    emitter->lineOpacity = new_opacity;
                    if (new_opacity == 0) {
                        obj_disable_emitter(obj, i);
                    }
                } else if ((obj->particleEmitter[i].flags & PARTICLE_POINT)) {
                    ParticleEmitter *emitter = &obj->particleEmitter[i];
                    obj->particleEmitter[i].flags |= PARTICLE_POINT_EMITTER_DISABLED;
                    if (emitter->pointCount == 0) {
                        obj_disable_emitter(obj, i);
                    }
                } else {
                    obj_disable_emitter(obj, i);
                }
            }
        }
        bits >>= 1;
    }
}

/**
 * Generates particles from a specific emitter attached to the object.
 * The particles are created according to the emitter's settings and behaviour.
 */
void obj_trigger_emitter(Object *obj, ParticleEmitter *emitter) {
    s32 i;
    ParticleBehaviour *behaviour;

    behaviour = emitter->behaviour;
    if (emitter->flags & PARTICLE_LINE) {
        Particle *particle = create_line_particle(obj, emitter);
        if (particle != NULL) {
            add_particle_to_entity_list((Object *) particle);
        }
        emitter->flags &= ~PARTICLE_EMITTER_AWAITING_SPAWN;
        if (emitter->lineOpacity + 64 > 255) {
            emitter->lineOpacity = 255;
        } else {
            emitter->lineOpacity += 64;
        }
    } else if (emitter->flags & PARTICLE_POINT) {
        if (emitter->pointCount < emitter->maxPointCount) {
            PointParticle *particle = create_point_particle(obj, emitter);
            emitter->flags &= ~PARTICLE_EMITTER_AWAITING_SPAWN;
            if (particle != NULL) {
                add_particle_to_entity_list((Object *) particle);
                particle->pointIndex = emitter->pointCount;
                particle->base.descFlags |= PARTICLE_DESC_FLAG_2000;
                emitter->refPoints[emitter->pointCount] = particle;
                emitter->pointCount++;
            }
        }
    } else {
        while (emitter->timeFromLastSpawn >= behaviour->spawnInterval) {
            emitter->timeFromLastSpawn -= behaviour->spawnInterval;
            for (i = 0; i < behaviour->burstCount; i++) {
                Particle *particle = create_general_particle(obj, emitter);
                if (particle != NULL) {
                    add_particle_to_entity_list((Object *) particle);
                    particle_update(particle, emitter->timeFromLastSpawn);
                }
                emitter->flags &= ~PARTICLE_EMITTER_AWAITING_SPAWN;
            }
        }
    }
}

/**
 * Initializes the particle velocity.
 */
void setup_particle_velocity(Particle *particle, Object *obj, ParticleEmitter *emitter, ParticleBehaviour *behaviour) {
    s32 randomizationFlags;
    Vec3f sourceVel;
    Vec3s angle;

    if (behaviour->flags &
        (PARTICLE_VELOCITY_RELATIVE_TO_PARENT | PARTICLE_VELOCITY_ABSOLUTE | PARTICLE_VELOCITY_SCALED_FROM_PARENT)) {
        particle->velocity.x = behaviour->velocityModifier.x;
        particle->velocity.y = behaviour->velocityModifier.y;
        particle->velocity.z = behaviour->velocityModifier.z;
    } else {
        particle->velocity.x = 0.0f;
        particle->velocity.y = 0.0f;
        particle->velocity.z = 0.0f;
    }

    randomizationFlags = behaviour->randomizationFlags &
                         (PARTICLE_RANDOM_VELOCITY_Z | PARTICLE_RANDOM_VELOCITY_Y | PARTICLE_RANDOM_VELOCITY_X);
    if (randomizationFlags) {
        if (randomizationFlags & PARTICLE_RANDOM_VELOCITY_X) {
            particle->velocity.x +=
                (f32) rand_range(-behaviour->velocityModifierRange.x, behaviour->velocityModifierRange.x) *
                0.00001525878906;
        }
        if (randomizationFlags & PARTICLE_RANDOM_VELOCITY_Y) {
            particle->velocity.y +=
                (f32) rand_range(-behaviour->velocityModifierRange.y, behaviour->velocityModifierRange.y) *
                0.00001525878906;
        }
        if (randomizationFlags & PARTICLE_RANDOM_VELOCITY_Z) {
            particle->velocity.z +=
                (f32) rand_range(-behaviour->velocityModifierRange.z, behaviour->velocityModifierRange.z) *
                0.00001525878906;
        }
    }

    switch (behaviour->flags & (PARTICLE_VELOCITY_RELATIVE_TO_PARENT | PARTICLE_VELOCITY_ABSOLUTE |
                                PARTICLE_VELOCITY_SCALED_FROM_PARENT)) {
        case PARTICLE_VELOCITY_RELATIVE_TO_PARENT:
            particle->velocity.x += obj->x_velocity;
            particle->velocity.y += obj->y_velocity;
            particle->velocity.z += obj->z_velocity;
            break;
        case PARTICLE_VELOCITY_SCALED_FROM_PARENT:
            particle->velocity.x *= obj->x_velocity;
            particle->velocity.y *= obj->y_velocity;
            particle->velocity.z *= obj->z_velocity;
            break;
    }

    if (behaviour->flags & PARTICLE_SOURCE_EMITS_WITH_VELOCITY) {
        sourceVel.x = 0.0f;
        sourceVel.y = 0.0f;
        sourceVel.z = -behaviour->emissionSpeed;

        randomizationFlags = behaviour->randomizationFlags;
        if (randomizationFlags & PARTICLE_RANDOM_EMISSION_SPEED) {
            sourceVel.z +=
                (f32) rand_range(-behaviour->emissionSpeedRange, behaviour->emissionSpeedRange) * 0.00001525878906;
        }

        if (randomizationFlags & (PARTICLE_RANDOM_EMISSION_DIR_PITCH | PARTICLE_RANDOM_EMISSION_DIR_YAW)) {
            angle.y_rotation = emitter->emissionDirection.y_rotation;
            if (randomizationFlags & PARTICLE_RANDOM_EMISSION_DIR_YAW) {
                angle.y_rotation +=
                    rand_range(-behaviour->emissionDirRange.y_rotation, behaviour->emissionDirRange.y_rotation);
            }
            angle.x_rotation = emitter->emissionDirection.x_rotation;
            if (randomizationFlags & PARTICLE_RANDOM_EMISSION_DIR_PITCH) {
                angle.x_rotation +=
                    rand_range(-behaviour->emissionDirRange.x_rotation, behaviour->emissionDirRange.x_rotation);
            }
            vec3f_rotate_py(&angle, &sourceVel);
        } else {
            vec3f_rotate_py(&emitter->emissionDirection, &sourceVel);
        }
        vec3f_rotate(&particle->parentObj->trans.rotation, &sourceVel);

        particle->velocity.x += sourceVel.x;
        particle->velocity.y += sourceVel.y;
        particle->velocity.z += sourceVel.z;
    }
}

/**
 * Initializes the particle position.
 */
void setup_particle_position(Particle *particle, Object *obj, ParticleEmitter *emitter, ParticleBehaviour *behaviour) {
    s32 randomizationFlags;
    Vec3f sourcePos;
    Vec3s sourceDir;

    particle->localPos.x = emitter->position.x;
    particle->localPos.y = emitter->position.y;
    particle->localPos.z = emitter->position.z;

    // particle->movementParam shares memory with other fields; use the appropriate one based on movement type
    particle->movementParam = behaviour->movementParam;
    if (behaviour->randomizationFlags & PARTICLE_RANDOM_MOVEMENT_PARAM) {
        particle->movementParam += (f32) rand_range(-behaviour->movementParamRange, behaviour->movementParamRange) *
                                   0.00001525878906; // 0.00001525878906 ~= 1.0/65536.0
    }

    if (behaviour->flags & PARTICLE_SOURCE_OFFSET_ENABLED) {
        sourcePos.x = 0.0f;
        sourcePos.y = 0.0f;
        sourcePos.z = -behaviour->sourceDistance;
        randomizationFlags = behaviour->randomizationFlags;
        if (randomizationFlags & PARTICLE_RANDOM_SOURCE_DISTANCE) {
            sourcePos.z +=
                (f32) rand_range(-behaviour->sourceDistanceRange, behaviour->sourceDistanceRange) * 0.00001525878906;
        }
        if (randomizationFlags & (PARTICLE_RANDOM_SOURCE_PITCH | PARTICLE_RANDOM_SOURCE_YAW)) {
            sourceDir.y_rotation = emitter->sourceRotation.y_rotation;
            if (randomizationFlags & PARTICLE_RANDOM_SOURCE_YAW) {
                sourceDir.y_rotation +=
                    rand_range(-behaviour->sourceDirRange.y_rotation, behaviour->sourceDirRange.y_rotation);
            }
            sourceDir.x_rotation = emitter->sourceRotation.x_rotation;
            if (randomizationFlags & PARTICLE_RANDOM_SOURCE_PITCH) {
                sourceDir.x_rotation +=
                    rand_range(-behaviour->sourceDirRange.x_rotation, behaviour->sourceDirRange.x_rotation);
            }
            vec3f_rotate_py(&sourceDir, &sourcePos);
        } else {
            vec3f_rotate(&emitter->sourceRotation, &sourcePos);
        }
        particle->localPos.x += sourcePos.x;
        particle->localPos.y += sourcePos.y;
        particle->localPos.z += sourcePos.z;
    }
    if (particle->movementType != PARTICLE_MOVEMENT_BASIC_PARENT) {
        vec3f_rotate(&obj->trans.rotation, &particle->localPos);
    }
    particle->trans.x_position = particle->localPos.x;
    particle->trans.y_position = particle->localPos.y;
    particle->trans.z_position = particle->localPos.z;
    if (particle->movementType == PARTICLE_MOVEMENT_BASIC_PARENT) {
        vec3f_rotate(&obj->trans.rotation, &particle->trans.position);
    }

    particle->trans.x_position += obj->trans.x_position;
    particle->trans.y_position += obj->trans.y_position;
    particle->trans.z_position += obj->trans.z_position;
}

/**
 * Flag the particle for the semi-transparent render pass. PARTICLE_DESC_FLAG_1000 selects
 * the later of the two translucent passes.
 */
static void particle_mark_translucent(Particle *particle) {
    if (particle->descFlags & PARTICLE_DESC_FLAG_1000) {
        particle->trans.flags |= OBJ_FLAGS_UNK_0100;
    } else {
        particle->trans.flags |= OBJ_FLAGS_UNK_0080;
    }
}

/**
 * Shaded particles take their parent's lighting; everything else renders at full brightness.
 */
static void particle_set_brightness(Particle *particle, Object *obj) {
    if ((particle->descFlags & PARTICLE_SHADED) && obj->shading != NULL) {
        particle->brightness = obj->shading->unk0 * 255.0f;
    } else {
        particle->brightness = 255;
    }
}

/**
 * Rotation comes from the behaviour, either absolute or relative to the parent object.
 */
static void particle_set_rotation(Particle *particle, Object *obj, ParticleBehaviour *behaviour) {
    if (behaviour->flags & PARTICLE_ROTATION_ABSOLUTE) {
        particle->trans.rotation.y_rotation = behaviour->rotation.y_rotation;
        particle->trans.rotation.x_rotation = behaviour->rotation.x_rotation;
        particle->trans.rotation.z_rotation = behaviour->rotation.z_rotation;
    } else {
        particle->trans.rotation.y_rotation = obj->trans.rotation.y_rotation + behaviour->rotation.y_rotation;
        particle->trans.rotation.x_rotation = obj->trans.rotation.x_rotation + behaviour->rotation.x_rotation;
        particle->trans.rotation.z_rotation = obj->trans.rotation.z_rotation + behaviour->rotation.z_rotation;
    }
}

static f32 magnitude3f(f32 x, f32 y, f32 z) {
    return sqrtf((x * x) + (y * y) + (z * z));
}

/**
 * Point and line particles share one texture setup: load the descriptor's texture into the
 * model, and start a backward-only animation on its last frame.
 */
static void particle_load_model_texture(Particle *particle, ParticleDescriptor *descriptor) {
    ParticleModel *model;

    model = particle->model;
    if (descriptor->textureID == -1) {
        model->texture = NULL;
        return;
    }
    model->texture = load_texture(descriptor->textureID);
    if (model->texture == NULL) {
        return;
    }
    if (model->texture->flags & 4) {
        particle_mark_translucent(particle);
    }
    if ((particle->descFlags & (PARTICLE_TEXTURE_ANIM_FORWARD_ENABLED | PARTICLE_TEXTURE_ANIM_BACKWARD_ENABLED)) ==
        PARTICLE_TEXTURE_ANIM_BACKWARD_ENABLED) {
        particle->textureFrame = model->texture->numOfTextures - 1;
    }
}

/**
 * Count down the particle's life and fade it towards its end opacity; frees the particle
 * when its timer runs out.
 */
static void particle_age(Particle *particle) {
    particle->destroyTimer -= gParticleUpdateRate;
    if (particle->destroyTimer <= 0) {
        free_object((Object *) particle);
        return;
    }
    if (particle->opacityTimer == 0) {
        particle->opacity += gParticleUpdateRate * particle->opacityVel;
        if (particle->opacity < 255) {
            particle_mark_translucent(particle);
        }
    } else {
        particle->opacityTimer -= gParticleUpdateRate;
        if (particle->opacityTimer < 0) {
            particle->opacity -= particle->opacityTimer * particle->opacityVel;
            particle->opacityTimer = 0;
        }
    }
}

static void vertex_set(Vertex *vtx, f32 x, f32 y, f32 z, u8 r, u8 g, u8 b, u8 a) {
    vtx->x = x;
    vtx->y = y;
    vtx->z = z;
    vtx->r = r;
    vtx->g = g;
    vtx->b = b;
    vtx->a = a;
}

static void vertex_copy(Vertex *dst, Vertex *src) {
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
    dst->r = src->r;
    dst->g = src->g;
    dst->b = src->b;
    dst->a = src->a;
}

/**
 * Create point particle and initialize it's fields.
 */
PointParticle *create_point_particle(Object *obj, ParticleEmitter *emitter) {
    ParticleDescriptor *descriptor;
    PointParticle *particle;
    ParticleBehaviour *behaviour;

    descriptor = gParticlesAssetTable[emitter->descriptorID];
    if (descriptor->kind != PARTICLE_KIND_POINT) {
        return NULL;
    }
    behaviour = emitter->behaviour;
    particle = (PointParticle *) particle_allocate(PARTICLE_KIND_POINT);
    if (particle == NULL) {
        return particle;
    }

    particle->base.segmentID = obj->segmentID;
    particle->base.trans.flags = OBJ_FLAGS_PARTICLE;
    particle->base.movementType = descriptor->movementType;
    particle->base.descFlags = descriptor->flags;
    particle->base.parentObj = obj;
    particle->pointEmitter = emitter;
    particle->base.trans.scale = descriptor->scale * behaviour->scale;
    particle->base.scaleVelocity = descriptor->scale * behaviour->scaleVelocity;
    particle->base.destroyTimer = descriptor->lifeTime;
    particle->base.colour.r = descriptor->colour.r;
    particle->base.colour.g = descriptor->colour.g;
    particle->base.colour.b = descriptor->colour.b;
    particle->base.colour.a = descriptor->colour.a;

    particle_set_brightness(&particle->base, obj);

    particle->base.opacityTimer = descriptor->opacityTimer;
    if (emitter->flags & PARTICLE_OVERRIDE_OPACITY_FROM_DESCRIPTOR) {
        particle->base.opacity = emitter->point_opacity;
    } else {
        particle->base.opacity = descriptor->opacity << 8;
    }
    if (descriptor->opacity < 255) {
        particle_mark_translucent(&particle->base);
    }
    if (particle->base.opacityTimer < particle->base.destroyTimer) {
        particle->base.opacityVel = ((descriptor->opacityVel << 8) - (particle->base.opacity & 0xFFFF)) /
                                    (particle->base.destroyTimer - particle->base.opacityTimer);
    } else {
        particle->base.opacityVel = 0;
    }

    setup_particle_position(&particle->base, obj, emitter, behaviour);
    particle_set_rotation(&particle->base, obj, behaviour);
    particle->base.angularVelocity.y_rotation = behaviour->angularVelocity.y_rotation;
    particle->base.angularVelocity.x_rotation = behaviour->angularVelocity.x_rotation;
    particle->base.angularVelocity.z_rotation = behaviour->angularVelocity.z_rotation;

    setup_particle_velocity(&particle->base, obj, emitter, behaviour);

    particle->base.gravity = gParticleGravityTable[(particle->base.descFlags >> 4) & 7];

    if (particle->base.movementType == PARTICLE_MOVEMENT_FORWARD) {
        particle->base.forwardVel =
            magnitude3f(particle->base.velocity.x, particle->base.velocity.y, particle->base.velocity.z);
    }

    if (behaviour->flags & PARTICLE_SOURCE_ROTATION_ENABLED) {
        emitter->pointSourceRotation.y_rotation += behaviour->sourceAngularVelocity.y_rotation;
        emitter->pointSourceRotation.x_rotation += behaviour->sourceAngularVelocity.x_rotation;
    }
    if (behaviour->flags & PARTICLE_ROTATING_DIRECTION) {
        emitter->pointEmissionDirection.y_rotation += behaviour->emissionDirAngularVelocity.y_rotation;
        emitter->pointEmissionDirection.x_rotation += behaviour->emissionDirAngularVelocity.x_rotation;
    }

    particle->base.textureFrameStep = descriptor->textureFrameStep;
    particle->base.textureFrame = 0;
    particle_load_model_texture(&particle->base, descriptor);
    particle->modelFrame = 0;
    particle->meshRegenerated = FALSE;
    if (particle->base.model->texture == NULL) {
        // Only point particles with textures are supported
        particle_deallocate(&particle->base);
        return NULL;
    }
    return particle;
}

/**
 * Create line particle and initialize its fields.
 */
Particle *create_line_particle(Object *obj, ParticleEmitter *emitter) {
    ParticleDescriptor *descriptor;
    Particle *particle;
    ParticleModel *model;
    ParticleBehaviour *behaviour;
    ColorLoopEntry *colourLoop;

    descriptor = gParticlesAssetTable[emitter->descriptorID];
    if (descriptor->kind != PARTICLE_KIND_LINE) {
        return NULL;
    }
    behaviour = emitter->behaviour;
    colourLoop = behaviour->colourLoop;
    particle = particle_allocate(PARTICLE_KIND_LINE);
    if (particle == NULL) {
        return NULL;
    }
    particle->segmentID = obj->segmentID;
    particle->trans.flags = OBJ_FLAGS_PARTICLE;
    particle->movementType = descriptor->movementType;
    particle->descFlags = descriptor->flags;
    particle->parentObj = obj;
    particle->lineEmitter = emitter;
    particle->trans.scale = descriptor->scale * behaviour->scale;
    particle->scaleVelocity = descriptor->scale * behaviour->scaleVelocity;
    particle->destroyTimer = descriptor->lifeTime;

    if (gParticleOverrideColor->word != 0) {
        particle->colour.word = gParticleOverrideColor->word;
    } else {
        particle->colour.r = descriptor->colour.r;
        particle->colour.g = descriptor->colour.g;
        particle->colour.b = descriptor->colour.b;
    }
    particle->colour.a = descriptor->colour.a;

    particle_set_brightness(particle, obj);

    particle->opacityTimer = descriptor->opacityTimer;
    particle->opacity = descriptor->opacity << 8;
    if (descriptor->opacity < 255) {
        particle_mark_translucent(particle);
    }
    if (particle->opacityTimer < particle->destroyTimer) {
        particle->opacityVel =
            (((descriptor->opacityVel - descriptor->opacity) << 8) / (particle->destroyTimer - particle->opacityTimer));
    } else {
        particle->opacityVel = 0;
    }

    emitter->lineRefPoint.x = emitter->position.x;
    emitter->lineRefPoint.y = emitter->position.y;
    emitter->lineRefPoint.z = emitter->position.z;
    vec3f_rotate(&obj->trans.rotation, &emitter->lineRefPoint);
    emitter->lineRefPoint.x += obj->trans.x_position;
    emitter->lineRefPoint.y += obj->trans.y_position;
    emitter->lineRefPoint.z += obj->trans.z_position;

    particle->lineCreationPhase = 0;
    particle->textureFrameStep = descriptor->textureFrameStep;
    particle->textureFrame = 0;
    particle_load_model_texture(particle, descriptor);

    // The first vertex sits at the emitter; update_line_particle grows the chevron from it.
    model = particle->model;
    if ((u32) colourLoop != -1U) {
        emitter->colourIndex++;
        if (emitter->colourIndex >= colourLoop[0].numEntries) {
            emitter->colourIndex = 0;
        }
        vertex_set(&model->vertices[0], emitter->lineRefPoint.x, emitter->lineRefPoint.y, emitter->lineRefPoint.z,
                   colourLoop[emitter->colourIndex + 2].r, colourLoop[emitter->colourIndex + 2].g,
                   colourLoop[emitter->colourIndex + 2].b, emitter->lineOpacity);
    } else {
        vertex_set(&model->vertices[0], emitter->lineRefPoint.x, emitter->lineRefPoint.y, emitter->lineRefPoint.z,
                   particle->colour.r, particle->colour.g, particle->colour.b, emitter->lineOpacity);
    }
    particle->lineOrientation = descriptor->lineOrientation;
    particle->trans.x_position = obj->trans.x_position;
    particle->trans.y_position = obj->trans.y_position;
    particle->trans.z_position = obj->trans.z_position;
    return particle;
}

/**
 * Scale and scale velocity come from the behaviour, randomised when asked, and are
 * multiplied by the descriptor scale. The velocity can instead follow the parent's speed.
 */
static void general_particle_init_scale(Particle *particle, Object *obj, ParticleBehaviour *behaviour,
                                        ParticleDescriptor *descriptor) {
    f32 scale;

    scale = behaviour->scale;
    if (behaviour->randomizationFlags & PARTICLE_RANDOM_SCALE) {
        scale += (f32) rand_range(-behaviour->scaleRange, behaviour->scaleRange) * 0.00001525878906;
    }
    particle->trans.scale = descriptor->scale * scale;

    scale = behaviour->scaleVelocity;
    if (behaviour->randomizationFlags & PARTICLE_RANDOM_SCALE_VELOCITY) {
        scale += (f32) rand_range(-behaviour->scaleVelocityRange, behaviour->scaleVelocityRange) * 0.00001525878906;
    }
    if (behaviour->flags & PARTICLE_SCALE_VELOCITY_INHERITS_PARENT_SPEED) {
        particle->scaleVelocity = magnitude3f(obj->x_velocity, obj->y_velocity, obj->z_velocity) * scale * 0.1f;
    } else {
        particle->scaleVelocity = descriptor->scale * scale;
    }
}

/**
 * Colour priority: the vehicle override colour, then the next entry of the behaviour's
 * colour loop, then the descriptor colour. Channels may then be randomised.
 */
static void general_particle_init_colour(Particle *particle, ParticleEmitter *emitter, ParticleBehaviour *behaviour,
                                         ParticleDescriptor *descriptor) {
    ColorLoopEntry *colourLoop;
    s32 randomizationFlags;

    colourLoop = behaviour->colourLoop;
    if (gParticleOverrideColor[0].word != 0) {
        particle->colour.word = gParticleOverrideColor[0].word;
    } else if ((s32) colourLoop != -1) {
        emitter->colourIndex++;
        if (emitter->colourIndex >= colourLoop->numEntries) {
            emitter->colourIndex = 0;
        }
        particle->colour.r = colourLoop[emitter->colourIndex + 2].r;
        particle->colour.g = colourLoop[emitter->colourIndex + 2].g;
        particle->colour.b = colourLoop[emitter->colourIndex + 2].b;
        particle->colour.a = colourLoop[emitter->colourIndex + 2].a;
    } else {
        particle->colour.r = descriptor->colour.r;
        particle->colour.g = descriptor->colour.g;
        particle->colour.b = descriptor->colour.b;
        particle->colour.a = descriptor->colour.a;
    }
    randomizationFlags = behaviour->randomizationFlags & (PARTICLE_RANDOM_COLOUR_ALPHA | PARTICLE_RANDOM_COLOUR_BLUE |
                                                          PARTICLE_RANDOM_COLOUR_GREEN | PARTICLE_RANDOM_COLOUR_RED);
    if (randomizationFlags) {
        if (randomizationFlags & PARTICLE_RANDOM_COLOUR_RED) {
            particle->colour.r += rand_range(-behaviour->colourRangeR, behaviour->colourRangeR);
        }
        if (randomizationFlags & PARTICLE_RANDOM_COLOUR_GREEN) {
            particle->colour.g += rand_range(-behaviour->colourRangeG, behaviour->colourRangeG);
        }
        if (randomizationFlags & PARTICLE_RANDOM_COLOUR_BLUE) {
            particle->colour.b += rand_range(-behaviour->colourRangeB, behaviour->colourRangeB);
        }
        if (randomizationFlags & PARTICLE_RANDOM_COLOUR_ALPHA) {
            particle->colour.a += rand_range(-behaviour->colourRangeA, behaviour->colourRangeA);
        }
    }
}

/**
 * Opacity is 8.8 fixed point, scaled by the hovercraft fade-in, and ramps from the descriptor's
 * start value to its end value once the opacity timer has elapsed.
 */
static void general_particle_init_opacity(Particle *particle, ParticleDescriptor *descriptor) {
    particle->opacityTimer = descriptor->opacityTimer;
    particle->opacity = descriptor->opacity * gCurrentHovercraftParticleOpacity;
    if (descriptor->opacity < 255) {
        particle_mark_translucent(particle);
    }
    if (particle->opacityTimer < particle->destroyTimer) {
        particle->opacityVel = (descriptor->opacityVel - descriptor->opacity) * gCurrentHovercraftParticleOpacity /
                               (particle->destroyTimer - particle->opacityTimer);
    } else {
        particle->opacityVel = 0;
    }
}

/**
 * Pick the starting texture frame (8.8 fixed point): random when the behaviour asks for it,
 * the last frame for a backward-only animation, else the first.
 */
static void general_particle_init_texture_frame(Particle *particle, ParticleBehaviour *behaviour, s32 frameCount,
                                                s32 lastFrame) {
    s32 backwardOnly;

    backwardOnly =
        (particle->descFlags & (PARTICLE_TEXTURE_ANIM_FORWARD_ENABLED | PARTICLE_TEXTURE_ANIM_BACKWARD_ENABLED)) ==
        PARTICLE_TEXTURE_ANIM_BACKWARD_ENABLED;
    if (behaviour->flags & PARTICLE_RANDOM_TEXTURE_FRAME) {
        particle->textureFrame = rand_range(0, frameCount - 1) << 8;
        if (backwardOnly) {
            particle->textureFrame |= 0xFF;
        }
    } else if (backwardOnly) {
        particle->textureFrame = lastFrame;
    } else {
        particle->textureFrame = 0;
    }
}

/**
 * Load the descriptor's sprite (sprite particles) or texture (triangle and rectangle
 * particles) and set up the texture coordinates. A missing texture leaves the particle
 * untextured; the caller drops it.
 */
static void general_particle_load_texture(Particle *particle, ParticleBehaviour *behaviour,
                                          ParticleDescriptor *descriptor) {
    TextureHeader *texture;

    particle->textureFrameStep = descriptor->textureFrameStep;
    if (particle->kind == PARTICLE_KIND_SPRITE) {
        particle->sprite = (Sprite *) tex_load_sprite(descriptor->textureID, 0);
        if (particle->sprite == NULL) {
            return;
        }
        if (particle->sprite->textures[0]->flags & 4) {
            particle_mark_translucent(particle);
        }
        general_particle_init_texture_frame(particle, behaviour, particle->sprite->numberOfFrames,
                                            (particle->sprite->numberOfFrames << 8) - 1);
        return;
    }
    if (particle->kind != PARTICLE_KIND_RECTANGLE && particle->kind != PARTICLE_KIND_TRIANGLE) {
        return;
    }
    texture = load_texture(descriptor->textureID);
    particle->model->texture = texture;
    if (texture == NULL) {
        return;
    }
    if (texture->flags & 4) {
        particle_mark_translucent(particle);
    }
    general_particle_init_texture_frame(particle, behaviour, texture->numOfTextures >> 8, texture->numOfTextures - 1);
    if (particle->kind == PARTICLE_KIND_TRIANGLE) {
        set_triangle_texture_coords(particle);
    } else {
        set_rectangle_texture_coords(particle);
    }
}

/**
 * Rotation and angular velocity from the behaviour, each axis randomised when asked.
 */
static void general_particle_init_rotation(Particle *particle, Object *obj, ParticleBehaviour *behaviour) {
    s32 randomizationFlags;

    particle_set_rotation(particle, obj, behaviour);
    randomizationFlags =
        behaviour->randomizationFlags & (PARTICLE_RANDOM_ROLL | PARTICLE_RANDOM_PITCH | PARTICLE_RANDOM_YAW);
    if (randomizationFlags != 0) {
        if (randomizationFlags & PARTICLE_RANDOM_YAW) {
            particle->trans.rotation.y_rotation +=
                rand_range(-behaviour->rotationRange.y_rotation, behaviour->rotationRange.y_rotation);
        }
        if (randomizationFlags & PARTICLE_RANDOM_PITCH) {
            particle->trans.rotation.x_rotation +=
                rand_range(-behaviour->rotationRange.x_rotation, behaviour->rotationRange.x_rotation);
        }
        if (randomizationFlags & PARTICLE_RANDOM_ROLL) {
            particle->trans.rotation.z_rotation +=
                rand_range(-behaviour->rotationRange.z_rotation, behaviour->rotationRange.z_rotation);
        }
    }

    particle->angularVelocity.y_rotation = behaviour->angularVelocity.y_rotation;
    particle->angularVelocity.x_rotation = behaviour->angularVelocity.x_rotation;
    particle->angularVelocity.z_rotation = behaviour->angularVelocity.z_rotation;
    randomizationFlags =
        behaviour->randomizationFlags &
        (PARTICLE_RANDOM_ROLL_VELOCITY | PARTICLE_RANDOM_PITCH_VELOCITY | PARTICLE_RANDOM_YAW_VELOCITY);
    if (randomizationFlags) {
        if (randomizationFlags & PARTICLE_RANDOM_YAW_VELOCITY) {
            particle->angularVelocity.y_rotation +=
                rand_range(-behaviour->angularVelocityRange.y_rotation, behaviour->angularVelocityRange.y_rotation);
        }
        if (randomizationFlags & PARTICLE_RANDOM_PITCH_VELOCITY) {
            particle->angularVelocity.x_rotation +=
                rand_range(-behaviour->angularVelocityRange.x_rotation, behaviour->angularVelocityRange.x_rotation);
        }
        if (randomizationFlags & PARTICLE_RANDOM_ROLL_VELOCITY) {
            particle->angularVelocity.z_rotation +=
                rand_range(-behaviour->angularVelocityRange.z_rotation, behaviour->angularVelocityRange.z_rotation);
        }
    }
}

/**
 * Once enough particles have been spawned from one source position or direction, step the
 * emitter's source and emission rotations by their angular velocities.
 */
static void emitter_advance_rotation(ParticleEmitter *emitter, ParticleBehaviour *behaviour) {
    if (behaviour->flags & PARTICLE_SOURCE_ROTATION_ENABLED) {
        emitter->sourceRotationCounter++;
        if (emitter->sourceRotationCounter >= behaviour->maxParticlesFromSamePos) {
            emitter->sourceRotation.y_rotation += behaviour->sourceAngularVelocity.y_rotation;
            emitter->sourceRotation.x_rotation += behaviour->sourceAngularVelocity.x_rotation;
            emitter->sourceRotation.z_rotation += behaviour->sourceRotation.z_rotation; //!@bug wrong field used
            emitter->sourceRotationCounter -= behaviour->maxParticlesFromSamePos;
        }
    }
    if (behaviour->flags & PARTICLE_ROTATING_DIRECTION) {
        emitter->emissionDirRotationCounter++;
        if (emitter->emissionDirRotationCounter >= behaviour->maxParticlesInSameDir) {
            emitter->emissionDirection.y_rotation += behaviour->emissionDirAngularVelocity.y_rotation;
            emitter->emissionDirection.x_rotation += behaviour->emissionDirAngularVelocity.x_rotation;
            emitter->emissionDirection.z_rotation += behaviour->emissionDirAngularVelocity.z_rotation;
            emitter->emissionDirRotationCounter -= behaviour->maxParticlesInSameDir;
        }
    }
}

/**
 * Creates a triangle, rectangle, or sprite particle and initializes its fields.
 * The steps draw random numbers in a fixed order: scale, lifetime, colour, texture frame,
 * position, rotation, velocity.
 */
Particle *create_general_particle(Object *obj, ParticleEmitter *emitter) {
    ParticleDescriptor *descriptor;
    Particle *particle;
    ParticleBehaviour *behaviour;

    descriptor = gParticlesAssetTable[emitter->descriptorID];
    if (descriptor->kind == PARTICLE_KIND_LINE || descriptor->kind == PARTICLE_KIND_POINT) {
        return NULL;
    }

    behaviour = emitter->behaviour;
    particle = particle_allocate(descriptor->kind);
    if (particle == NULL) {
        return particle;
    }
    particle->segmentID = obj->segmentID;
    particle->trans.flags = OBJ_FLAGS_PARTICLE;
    particle->movementType = descriptor->movementType;
    particle->descFlags = descriptor->flags;
    particle->parentObj = obj;
    particle_set_brightness(particle, obj);

    general_particle_init_scale(particle, obj, behaviour, descriptor);
    particle->destroyTimer = rand_range(-descriptor->lifeTimeRange, descriptor->lifeTimeRange) + descriptor->lifeTime;
    general_particle_init_colour(particle, emitter, behaviour, descriptor);
    general_particle_init_opacity(particle, descriptor);
    general_particle_load_texture(particle, behaviour, descriptor);
    setup_particle_position(particle, obj, emitter, behaviour);
    general_particle_init_rotation(particle, obj, behaviour);
    setup_particle_velocity(particle, obj, emitter, behaviour);

    particle->gravity = gParticleGravityTable[(particle->descFlags >> 4) & 7];
    if (particle->movementType == PARTICLE_MOVEMENT_FORWARD) {
        particle->forwardVel = magnitude3f(particle->velocity.x, particle->velocity.y, particle->velocity.z);
    }

    emitter_advance_rotation(emitter, behaviour);

    if (particle->model == NULL) {
        particle_deallocate(particle);
        return NULL;
    }
    return particle;
}

/**
 * Hand out the first free slot of one particle buffer, or NULL when the buffer is full.
 * Slots are `stride` bytes apart and start with a Particle whose kind is PARTICLE_KIND_NONE
 * while free. The last slot is never used.
 */
static Particle *particle_buffer_take(void *buffer, s32 stride, s32 *count, s32 max, s32 *full, s32 kind) {
    Particle *particle;
    s32 i;

    if (buffer == NULL) {
        return NULL;
    }
    if (*count >= max - 1) {
        *full = TRUE;
        return NULL;
    }
    i = 0;
    while (((Particle *) ((u8 *) buffer + (i * stride)))->kind != PARTICLE_KIND_NONE) {
        i++;
    }
    particle = (Particle *) ((u8 *) buffer + (i * stride));
    particle->kind = kind;
    (*count)++;
    return particle;
}

/**
 * Allocates a particle of the given kind from the buffer.
 * Returns NULL if the buffer is full.
 */
Particle *particle_allocate(s32 kind) {
    Particle *particle;

    particle = NULL;
    switch (kind) {
        case PARTICLE_KIND_SPRITE:
            particle = particle_buffer_take(gSpriteParticleBuffer, sizeof(Particle), &gNumSpriteParticles,
                                            gMaxSpriteParticles, &gSpriteParticleBufferFull, kind);
            break;
        case PARTICLE_KIND_TRIANGLE:
            particle = particle_buffer_take(gTriangleParticleBuffer, sizeof(Particle), &gNumTriangleParticles,
                                            gMaxTriangleParticles, &gTriangleParticleBufferFull, kind);
            break;
        case PARTICLE_KIND_RECTANGLE:
            particle = particle_buffer_take(gRectangleParticleBuffer, sizeof(Particle), &gNumRectangleParticles,
                                            gMaxRectangleParticles, &gRectangleParticleBufferFull, kind);
            break;
        case PARTICLE_KIND_LINE:
            particle = particle_buffer_take(gLineParticleBuffer, sizeof(Particle), &gNumLineParticles,
                                            gMaxLineParticles, &gLineParticleBufferFull, kind);
            break;
        case PARTICLE_KIND_POINT:
            particle = particle_buffer_take(gPointParticleBuffer, sizeof(PointParticle), &gNumPointParticles,
                                            gMaxPointParticles, &gPointParticleBufferFull, kind);
            break;
    }
    return particle;
}

static void particle_free_texture(Particle *particle) {
    if (particle->model->texture != NULL) {
        tex_free(particle->model->texture);
    }
}

/**
 * Deallocates the specified particle and frees its associated resources.
 */
void particle_deallocate(Particle *particle) {
    switch (particle->kind) {
        case PARTICLE_KIND_SPRITE:
            if (gNumSpriteParticles <= 0) {
                return;
            }
            if (particle->sprite != NULL) {
                sprite_free(particle->sprite);
            }
            gNumSpriteParticles--;
            break;
        case PARTICLE_KIND_TRIANGLE:
            if (gNumTriangleParticles <= 0) {
                return;
            }
            particle_free_texture(particle);
            gNumTriangleParticles--;
            break;
        case PARTICLE_KIND_RECTANGLE:
            if (gNumRectangleParticles <= 0) {
                return;
            }
            particle_free_texture(particle);
            gNumRectangleParticles--;
            break;
        case PARTICLE_KIND_LINE:
            if (gNumLineParticles <= 0) {
                return;
            }
            particle_free_texture(particle);
            gNumLineParticles--;
            break;
        case PARTICLE_KIND_POINT:
            if (gNumPointParticles <= 0) {
                return;
            }
            delete_point_particle_from_sequence((PointParticle *) particle);
            particle_free_texture(particle);
            gNumPointParticles--;
            break;
        default:
            return;
    }
    particle->kind = PARTICLE_KIND_NONE;
}

/**
 * Frees all memory associated with the emitter.
 */
void emitter_cleanup(ParticleEmitter *emitter) {
    PointParticle *pointParticle;
    s32 i;

    if (emitter->flags & PARTICLE_POINT) {
        if (emitter->refPoints != NULL) {
            for (i = 0; i < emitter->pointCount; i++) {
                pointParticle = emitter->refPoints[i];
                pointParticle->base.destroyTimer = 0;
                pointParticle->pointEmitter = NULL;
            }
            mempool_free(emitter->refPoints);
            emitter->refPoints = NULL;
        }
    }
}

/**
 * Call a function to update the current particle texture frame if applicable.
 * Applies movement, gets block ID and then shifts the opacity.
 */
void particle_update(Particle *particle, s32 updateRate) {
    LevelModelSegmentBoundingBox *boundingBox;
    PointParticle *pointParticle;

    gParticleUpdateRate = updateRate;
    pointParticle = NULL;
    if (particle->kind == PARTICLE_KIND_LINE) {
        update_line_particle(particle);
        return;
    }

    if (particle->descFlags & (PARTICLE_TEXTURE_ANIM_FORWARD_ENABLED | PARTICLE_TEXTURE_ANIM_BACKWARD_ENABLED)) {
        if (gParticleUpdateRate > 0) {
            update_particle_texture_frame(particle);
        }
    }
    if (particle->kind == PARTICLE_KIND_POINT) {
        pointParticle = (PointParticle *) particle;
        pointParticle->modelFrame = 1 - pointParticle->modelFrame;
        pointParticle->meshRegenerated = 0;
    }
    // A point particle whose emitter has been torn down stays where it is.
    if (pointParticle == NULL || pointParticle->pointEmitter != NULL) {
        if (particle->movementType == PARTICLE_MOVEMENT_ACCELERATION) {
            move_particle_with_acceleration(particle);
        } else if (particle->movementType == PARTICLE_MOVEMENT_ATTACHED_TO_PARENT) {
            move_particle_attached_to_parent(particle);
        } else if (particle->movementType == PARTICLE_MOVEMENT_BASIC_PARENT) {
            move_particle_basic_parent(particle);
        } else if (particle->movementType == PARTICLE_MOVEMENT_FORWARD) {
            move_particle_forward(particle);
        } else {
            // PARTICLE_MOVEMENT_BASIC
            move_particle_basic(particle);
        }
    }

    boundingBox = block_boundbox(particle->segmentID);
    if (boundingBox != NULL) {
        if (particle->trans.x_position < boundingBox->x1 || boundingBox->x2 < particle->trans.x_position ||
            particle->trans.y_position < boundingBox->y1 || boundingBox->y2 < particle->trans.y_position ||
            particle->trans.z_position < boundingBox->z1 || boundingBox->z2 < particle->trans.z_position) {
            particle->segmentID = get_level_segment_index_from_position(
                particle->trans.x_position, particle->trans.y_position, particle->trans.z_position);
        }
    } else {
        particle->segmentID = get_level_segment_index_from_position(
            particle->trans.x_position, particle->trans.y_position, particle->trans.z_position);
    }

    particle_age(particle);
}

/**
 * Removes a point particle from the array managed by its parent emitter.
 */
void delete_point_particle_from_sequence(PointParticle *particle) {
    ParticleEmitter *emitter;
    s32 i;

    emitter = particle->pointEmitter;
    if (emitter == NULL || emitter->pointCount == 0) {
        return;
    }
    if (particle != emitter->refPoints[particle->pointIndex]) {
        return;
    }
    emitter->pointCount--;
    for (i = particle->pointIndex; i < emitter->pointCount; i++) {
        emitter->refPoints[i] = emitter->refPoints[i + 1];
        emitter->refPoints[i]->pointIndex = i;
    }
}

/**
 * Half-width vector of the line's chevron: along one axis of the parent's frame, or
 * perpendicular to the parent's velocity when PARTICLE_LINE_ALONG_VELOCITY is set.
 */
static void line_particle_offset(Particle *particle, Object *obj, ParticleEmitter *emitter, Vec3f *vtxOffset) {
    f32 tempf;
    f32 scale;

    if (emitter->behaviour->flags & PARTICLE_SCALE_VELOCITY_INHERITS_PARENT_SPEED) {
        scale = magnitude3f(obj->x_velocity, obj->y_velocity, obj->z_velocity);
        scale = scale * particle->trans.scale * 0.1f;
    } else {
        scale = particle->trans.scale;
    }
    if (!(particle->descFlags & PARTICLE_LINE_ALONG_VELOCITY)) {
        vtxOffset->x = 0.0f;
        vtxOffset->y = 0.0f;
        vtxOffset->z = 0.0f;
        switch (particle->lineOrientation) {
            default:
                vtxOffset->x = scale;
                break;
            case 2:
                vtxOffset->z = scale;
                break;
            case 1:
                vtxOffset->y = scale;
                break;
        }
        vec3f_rotate(&obj->trans.rotation, vtxOffset);
        return;
    }
    vtxOffset->x = obj->x_velocity;
    vtxOffset->y = obj->y_velocity;
    vtxOffset->z = obj->z_velocity;
    tempf = ((vtxOffset->x * vtxOffset->x) + (vtxOffset->y * vtxOffset->y)) + (vtxOffset->z * vtxOffset->z);
    if (tempf < 0.01f) {
        tempf = 1.0f;
    } else {
        tempf = scale / sqrtf(tempf);
    }
    vtxOffset->x *= tempf;
    vtxOffset->y *= tempf;
    vtxOffset->z *= tempf;
    switch (particle->lineOrientation) {
        case 0:
            tempf = vtxOffset->x;
            vtxOffset->x = -vtxOffset->z;
            vtxOffset->z = tempf;
            break;
        case 1:
            tempf = vtxOffset->y;
            vtxOffset->y = -vtxOffset->z;
            vtxOffset->z = tempf;
            break;
    }
}

/**
 * Second step of the line mesh: vertices 1 and 3 either side of the emitter's current
 * position and vertex 2 at it. The sides keep vertex 0's colour; the centre takes the
 * next colour-loop entry.
 */
static void line_particle_begin_mesh(Particle *particle, ParticleEmitter *emitter, Vec3f *vtxOffset) {
    Vertex *verts;
    Vec3f *ref;
    ColorLoopEntry *colourLoop;

    verts = particle->model->vertices;
    ref = &emitter->lineRefPoint;
    colourLoop = emitter->behaviour->colourLoop;
    vertex_set(&verts[1], vtxOffset->x + ref->x, vtxOffset->y + ref->y, vtxOffset->z + ref->z, verts[0].r, verts[0].g,
               verts[0].b, emitter->lineOpacity);
    if ((s32) colourLoop != -1) {
        vertex_set(&verts[2], ref->x, ref->y, ref->z, colourLoop[emitter->colourIndex + 2].r,
                   colourLoop[emitter->colourIndex + 2].g, colourLoop[emitter->colourIndex + 2].b,
                   emitter->lineOpacity);
    } else {
        vertex_set(&verts[2], ref->x, ref->y, ref->z, particle->colour.r, particle->colour.g, particle->colour.b,
                   emitter->lineOpacity);
    }
    vertex_set(&verts[3], ref->x - vtxOffset->x, ref->y - vtxOffset->y, ref->z - vtxOffset->z, verts[0].r, verts[0].g,
               verts[0].b, emitter->lineOpacity);
    particle->lineCreationPhase = 1;
}

/**
 * Third step of the line mesh: vertices 4 and 5 either side of the emitter's new position,
 * coloured like the centre vertex. The chevron is complete after this.
 */
static void line_particle_finish_mesh(Particle *particle, ParticleEmitter *emitter, Vec3f *vtxOffset) {
    Vertex *verts;
    Vec3f *ref;

    verts = particle->model->vertices;
    ref = &emitter->lineRefPoint;
    vertex_set(&verts[4], vtxOffset->x + ref->x, vtxOffset->y + ref->y, vtxOffset->z + ref->z, verts[2].r, verts[2].g,
               verts[2].b, emitter->lineOpacity);
    vertex_set(&verts[5], ref->x - vtxOffset->x, ref->y - vtxOffset->y, ref->z - vtxOffset->z, verts[2].r, verts[2].g,
               verts[2].b, emitter->lineOpacity);
    particle->lineCreationPhase = 2;
}

/**
 * Line particles have their own update function because they behave differently from other particle types.
 * Instead of moving, they generate their mesh in three steps and remain stationary until their lifetime ends.
 *
 * In the first step, only vertex 0 exists and nothing is rendered. In the second step the
 * vehicle (and so the emitter) has moved, and three more vertices are added: one at the
 * emitter and two to its sides. In the third step the emitter has moved again and two more
 * side vertices complete a chevron, so consecutive particles from one emitter merge into a
 * continuous line.
 */
void update_line_particle(Particle *particle) {
    Vec3f vtxOffset;
    ParticleModel *model;
    Object *obj;
    ParticleEmitter *emitter;

    obj = particle->parentObj;
    model = NULL;
    emitter = NULL;
    if (obj != NULL) {
        emitter = particle->lineEmitter;
        model = particle->model;
    }

    if (particle->lineCreationPhase < 2 && obj != NULL) {
        line_particle_offset(particle, obj, emitter, &vtxOffset);
        if (model != NULL && particle->lineCreationPhase == 0) {
            line_particle_begin_mesh(particle, emitter, &vtxOffset);
        } else if (model != NULL) {
            line_particle_finish_mesh(particle, emitter, &vtxOffset);
        }
    } else {
        particle_age(particle);
    }
    if (model != NULL && model->texture != NULL &&
        (particle->descFlags & (PARTICLE_TEXTURE_ANIM_FORWARD_ENABLED | PARTICLE_TEXTURE_ANIM_BACKWARD_ENABLED)) &&
        gParticleUpdateRate > 0) {
        update_particle_texture_frame(particle);
    }
}

/**
 * Updates the current animation frame based on the animation flags.
 */
void update_particle_texture_frame(Particle *particle) {
    s32 currentlyBackward;
    s32 keepGoing;
    s32 i;
    s32 textureCount;
    s32 backwardEnabled;
    s32 isLooped;
    s32 forwardEnabled;

    keepGoing = -1;
    i = 0; // required to match
    textureCount = particle->kind == PARTICLE_KIND_SPRITE ? particle->sprite->numberOfFrames * 256
                                                          : particle->model->texture->numOfTextures;

    forwardEnabled = particle->descFlags & PARTICLE_TEXTURE_ANIM_FORWARD_ENABLED;
    backwardEnabled = particle->descFlags & PARTICLE_TEXTURE_ANIM_BACKWARD_ENABLED;
    isLooped = particle->descFlags & PARTICLE_TEXTURE_ANIM_LOOP;
    currentlyBackward = particle->descFlags & PARTICLE_CURRENT_ANIMATION_BACKWARD;

    for (i = 0; (i++ < gParticleUpdateRate) && keepGoing;) {
        if (!currentlyBackward) {
            particle->textureFrame += particle->textureFrameStep;
            if (particle->textureFrame >= textureCount) {
                if (backwardEnabled) {
                    //!@bug the last frame is displayed twice
                    particle->textureFrame = 2 * textureCount - 1 - particle->textureFrame;
                    currentlyBackward = TRUE;
                    particle->descFlags |= PARTICLE_CURRENT_ANIMATION_BACKWARD;
                } else if (isLooped) {
                    particle->textureFrame -= textureCount;
                } else {
                    particle->textureFrame = textureCount - 1;
                    keepGoing = FALSE;
                    particle->descFlags &=
                        ~(PARTICLE_TEXTURE_ANIM_FORWARD_ENABLED | PARTICLE_TEXTURE_ANIM_BACKWARD_ENABLED);
                }
            }
        } else {
            particle->textureFrame -= particle->textureFrameStep;
            if (particle->textureFrame < 0) {
                if (isLooped) {
                    if (forwardEnabled) {
                        particle->textureFrame = -particle->textureFrame;
                        currentlyBackward = FALSE;
                        particle->descFlags &= ~PARTICLE_CURRENT_ANIMATION_BACKWARD;
                    } else {
                        particle->textureFrame += textureCount;
                    }
                } else {
                    particle->textureFrame = 0;
                    keepGoing = FALSE;
                    particle->descFlags &=
                        ~(PARTICLE_TEXTURE_ANIM_FORWARD_ENABLED | PARTICLE_TEXTURE_ANIM_BACKWARD_ENABLED);
                }
            }
        }
    }
}

/**
 * Apply translation, rotation and scale based off the velocities of the particle.
 * Applies further translation based on the position of the parent.
 */
void move_particle_basic_parent(Particle *particle) {
    s32 i;
    Object *parent;

    i = gParticleUpdateRate;
    while (i-- > 0) {
        particle->localPos.x += particle->velocity.x;
        particle->localPos.y += particle->velocity.y;
        particle->velocity.y -= particle->gravity;
        particle->localPos.z += particle->velocity.z;
        particle->trans.rotation.y_rotation += particle->angularVelocity.y_rotation;
        particle->trans.rotation.x_rotation += particle->angularVelocity.x_rotation;
        particle->trans.rotation.z_rotation += particle->angularVelocity.z_rotation;
        particle->trans.scale += particle->scaleVelocity;
    }
    particle->trans.x_position = particle->localPos.x;
    particle->trans.y_position = particle->localPos.y;
    particle->trans.z_position = particle->localPos.z;
    parent = particle->parentObj;
    if (parent) {
        particle->trans.x_position += parent->trans.x_position;
        particle->trans.y_position += parent->trans.y_position;
        particle->trans.z_position += parent->trans.z_position;
    }
}

/**
 * Applies rotation and scale based on the particle's velocities.
 * Translates the particle downward in the object's local coordinate system by downOffset.
 * Applies additional translation based on the parent's position.
 */
void move_particle_attached_to_parent(Particle *particle) {
    s32 i = gParticleUpdateRate;
    Object *parent;

    while (i-- > 0) {
        particle->trans.rotation.y_rotation += particle->angularVelocity.y_rotation;
        particle->trans.rotation.x_rotation += particle->angularVelocity.x_rotation;
        particle->trans.rotation.z_rotation += particle->angularVelocity.z_rotation;
        particle->trans.scale += particle->scaleVelocity;
    }

    particle->trans.x_position = 0.0f;
    particle->trans.y_position = -particle->downOffset;
    particle->trans.z_position = 0.0f;
    vec3f_rotate(&particle->trans.rotation, &particle->trans.position);
    particle->trans.x_position += particle->localPos.x;
    particle->trans.y_position += particle->localPos.y;
    particle->trans.z_position += particle->localPos.z;

    parent = particle->parentObj;
    if (parent != NULL) {
        particle->trans.x_position += parent->trans.x_position;
        particle->trans.y_position += parent->trans.y_position;
        particle->trans.z_position += parent->trans.z_position;
    }
}

/**
 * Apply translation, rotation and scale based off the velocities of the particle.
 * Apply additional acceleration always directed downward in the local coordinate system
 */
void move_particle_with_acceleration(Particle *particle) {
    s32 i = 0;
    Vec3f acceleration;

    while (i++ < gParticleUpdateRate) {
        particle->trans.x_position += particle->velocity.x;
        particle->trans.y_position += particle->velocity.y;
        particle->trans.z_position += particle->velocity.z;
        particle->trans.scale += particle->scaleVelocity;
        particle->trans.rotation.y_rotation += particle->angularVelocity.y_rotation;
        particle->trans.rotation.x_rotation += particle->angularVelocity.x_rotation;
        particle->trans.rotation.z_rotation += particle->angularVelocity.z_rotation;

        acceleration.x = 0.0f;
        acceleration.y = -particle->downAcceleration;
        acceleration.z = 0.0f;
        vec3f_rotate(&particle->trans.rotation, &acceleration);
        particle->velocity.x += acceleration.x;
        particle->velocity.y += acceleration.y;
        particle->velocity.y -= particle->gravity;
        particle->velocity.z += acceleration.z;
    }
}

/**
 * Apply translation, rotation and scale based off the velocities of the particle.
 */
void move_particle_basic(Particle *particle) {
    s32 i = 0;

    while (i++ < gParticleUpdateRate) {
        particle->trans.x_position += particle->velocity.x;
        particle->trans.y_position += particle->velocity.y;
        particle->velocity.y -= particle->gravity;
        particle->trans.z_position += particle->velocity.z;
        particle->trans.scale += particle->scaleVelocity;
        particle->trans.rotation.y_rotation += particle->angularVelocity.y_rotation;
        particle->trans.rotation.x_rotation += particle->angularVelocity.x_rotation;
        particle->trans.rotation.z_rotation += particle->angularVelocity.z_rotation;
    }
}

/**
 * Apply translation, rotation and scale based off the velocities of the particle.
 * Velocity is set from forward velocity.
 * The particle always moves forward in the local coordinate system
 */
void move_particle_forward(Particle *particle) {
    s32 i = 0;

    while (i++ < gParticleUpdateRate) {
        particle->velocity.x = 0.0f;
        particle->velocity.y = 0.0f;
        particle->velocity.z = -particle->forwardVel;
        vec3f_rotate_py(&particle->trans.rotation, &particle->velocity);
        particle->trans.x_position += particle->velocity.x;
        // Gravity is subtracted here, but velocity isn't actually affected by it — possible bug?
        // Might cause the particle to drift downward instead of moving straight forward.
        particle->trans.y_position += particle->velocity.y - particle->gravity;
        particle->trans.z_position += particle->velocity.z;
        particle->trans.scale += particle->scaleVelocity;
        particle->trans.rotation.y_rotation += particle->angularVelocity.y_rotation;
        particle->trans.rotation.x_rotation += particle->angularVelocity.x_rotation;
        particle->trans.rotation.z_rotation += particle->angularVelocity.z_rotation;
    }
}

/**
 * Iterates through every object and renders it as a particle if applicable.
 * Only particles with the PARTICLE_UNK_FLAG_8000 flag set will be rendered.
 */
UNUSED void render_active_particles(Gfx **dList, Mtx **mtx, Vertex **vtx) {
    Particle **objects;
    s32 minObjIndex;
    s32 maxObjIndex;

    objects = (Particle **) objGetObjList(&minObjIndex, &maxObjIndex);
    for (; minObjIndex < maxObjIndex; minObjIndex++) {
        if (objects[minObjIndex]->trans.flags & OBJ_FLAGS_PARTICLE) {
            if (objects[minObjIndex]->descFlags & PARTICLE_UNK_FLAG_8000) {
                render_particle(objects[minObjIndex], dList, mtx, vtx, 0);
            }
        }
    }
}

/**
 * Load a texture then render a sprite or a billboard.
 */
void render_particle(Particle *particle, Gfx **dList, Mtx **mtx, Vertex **vtx, s32 flags) {
    s32 renderFlags;
    s32 alpha;
    s32 temp;
    ParticleModel *model;
    Vertex *tempvtx;

    renderFlags = (RENDER_FOG_ACTIVE | RENDER_Z_COMPARE);

    // Never true
    if ((particle->descFlags & flags) && sParticleSpawnCounter < 512) {
        return;
    }
    alpha = (particle->opacity >> 8) & 0xFF;
    if (alpha <= 0) {
        return;
    }
    if (particle->kind != PARTICLE_KIND_POINT && particle->kind != PARTICLE_KIND_LINE) {
        gDPSetEnvColor((*dList)++, particle->colour.r, particle->colour.g, particle->colour.b, particle->colour.a);
        if (alpha != 255) {
            renderFlags = (RENDER_Z_UPDATE | RENDER_FOG_ACTIVE | RENDER_SEMI_TRANSPARENT | RENDER_Z_COMPARE);
            gDPSetPrimColor((*dList)++, 0, 0, particle->brightness, particle->brightness, particle->brightness, alpha);
        } else {
            gDPSetPrimColor((*dList)++, 0, 0, 255, 255, 255, 255);
        }
        if (particle->kind == PARTICLE_KIND_SPRITE) {
            temp = particle->textureFrame;
            particle->textureFrame >>= 8;
            particle->textureFrame = (particle->textureFrame * 255) / (particle->sprite->numberOfFrames);
            render_sprite_billboard(dList, mtx, vtx, (Object *) particle, particle->sprite, renderFlags);
            particle->textureFrame = temp;
        } else {
            model = particle->model;
            if (model->texture) {
                mtx_cam_push(dList, mtx, &particle->trans, 1.0f, 0.0f);
                material_set(dList, model->texture, renderFlags, particle->textureFrame << 8);
                gSPVertexDKR((*dList)++, OS_K0_TO_PHYSICAL(model->vertices), model->vertexCount, 0);
                gSPPolygon((*dList)++, OS_K0_TO_PHYSICAL(model->triangles), model->triangleCount, TRIN_ENABLE_TEXTURE);
                mtx_pop(dList);
            }
        }
        // Restore primitive and environment colour
        if (alpha != 255 || particle->brightness != 255) {
            gDPSetPrimColor((*dList)++, 0, 0, 255, 255, 255, 255);
        }
        if (particle->colour.a) {
            gDPSetEnvColor((*dList)++, 255, 255, 255, 0);
        }
    } else {
        renderFlags =
            (RENDER_VTX_ALPHA | RENDER_Z_UPDATE | RENDER_FOG_ACTIVE | RENDER_Z_COMPARE | RENDER_ANTI_ALIASING);
        gDPSetEnvColor((*dList)++, 255, 255, 255, 0);
        if (particle->kind == PARTICLE_KIND_POINT) {
            if (particle->destroyTimer > 0) {
                gDPSetPrimColor((*dList)++, 0, 0, particle->brightness, particle->brightness, particle->brightness,
                                255);
                if (((PointParticle *) particle)->meshRegenerated == 0) {
                    regenerate_point_particles_mesh((PointParticle *) particle);
                }
                model = particle->model;
                temp = ((PointParticle *) particle)->modelFrame;
                temp <<= 3;
                tempvtx = &model->vertices[temp];
                material_set(dList, model->texture, renderFlags, particle->textureFrame << 8);
                gSPVertexDKR((*dList)++, OS_K0_TO_PHYSICAL(tempvtx), model->vertexCount, 0);
                gSPPolygon((*dList)++, OS_K0_TO_PHYSICAL(model->triangles), model->triangleCount, TRIN_ENABLE_TEXTURE);
                if (particle->brightness != 255) {
                    gDPSetPrimColor((*dList)++, 0, 0, 255, 255, 255, 255);
                }
            }
        } else if (particle->kind == PARTICLE_KIND_LINE) {
            gDPSetPrimColor((*dList)++, 0, 0, particle->brightness, particle->brightness, particle->brightness, alpha);
            if (particle->lineCreationPhase >= 2) {
                model = particle->model;
                material_set(dList, model->texture, renderFlags, particle->textureFrame << 8);
                gSPVertexDKR((*dList)++, OS_K0_TO_PHYSICAL(model->vertices), model->vertexCount, 0);
                gSPPolygon((*dList)++, OS_K0_TO_PHYSICAL(model->triangles), model->triangleCount, TRIN_ENABLE_TEXTURE);
            } else if (particle->lineCreationPhase > 0) {
                model = particle->model;
                material_set(dList, model->texture, renderFlags, particle->textureFrame << 8);
                gSPVertexDKR((*dList)++, OS_K0_TO_PHYSICAL(model->vertices), 4, 0);
                gSPPolygon((*dList)++, OS_K0_TO_PHYSICAL(&model->triangles[model->triangleCount]), 1,
                           TRIN_ENABLE_TEXTURE);
            }
            if (alpha != 255 || particle->brightness != 255) {
                gDPSetPrimColor((*dList)++, 0, 0, 255, 255, 255, 255);
            }
        }
    }
}

/**
 * Write one point's ring of four vertices: right, up, left, down around its position,
 * in its own rotation, with its colour and opacity.
 */
static void point_particle_write_ring(PointParticle *particle, Vertex *verts) {
    Vec3f vec_up;
    Vec3f vec_right;
    ObjectTransform *trans;
    ColourRGBA *colour;
    u8 alpha;

    trans = &particle->base.trans;
    colour = &particle->base.colour;
    alpha = particle->base.opacity >> 8;

    vec_right.x = trans->scale;
    vec_right.y = 0.0f;
    vec_right.z = 0.0f;
    vec3f_rotate(&trans->rotation, &vec_right);
    vec_up.x = 0.0f;
    vec_up.y = trans->scale;
    vec_up.z = 0.0f;
    vec3f_rotate(&trans->rotation, &vec_up);

    vertex_set(&verts[0], trans->x_position + vec_right.f[0], trans->y_position + vec_right.f[1],
               trans->z_position + vec_right.f[2], colour->r, colour->g, colour->b, alpha);
    vertex_set(&verts[1], trans->x_position + vec_up.f[0], trans->y_position + vec_up.f[1],
               trans->z_position + vec_up.f[2], colour->r, colour->g, colour->b, alpha);
    vertex_set(&verts[2], trans->x_position - vec_right.f[0], trans->y_position - vec_right.f[1],
               trans->z_position - vec_right.f[2], colour->r, colour->g, colour->b, alpha);
    vertex_set(&verts[3], trans->x_position - vec_up.f[0], trans->y_position - vec_up.f[1],
               trans->z_position - vec_up.f[2], colour->r, colour->g, colour->b, alpha);
}

/**
 * Sets the position and colours of all vertices for all particles spawned by a single emitter.
 * Together, they form the shape of a continuous pipe-like structure.
 *
 * Each point owns eight vertices per model frame: its own ring in the second four, and a
 * copy of the newer neighbour's ring in the first four so consecutive points share a seam.
 * The newest point joins to itself.
 */
void regenerate_point_particles_mesh(PointParticle *obj) {
    s32 index;
    s32 i;
    PointParticle *particle;
    ParticleEmitter *emitter;
    Vertex *prevRing;
    Vertex *verts;

    emitter = obj->pointEmitter;
    if (emitter == NULL || emitter->refPoints == NULL) {
        return;
    }
    prevRing = NULL;
    for (index = emitter->pointCount - 1; index >= 0; index--) {
        particle = emitter->refPoints[index];
        if (particle->base.destroyTimer == 0) {
            break;
        }
        verts = &particle->base.model->vertices[particle->modelFrame << 3];
        point_particle_write_ring(particle, &verts[4]);
        if (prevRing == NULL) {
            prevRing = &verts[4];
        }
        for (i = 0; i < 4; i++) {
            vertex_copy(&verts[i], &prevRing[i]);
        }
        prevRing = &verts[4];
        particle->meshRegenerated = -1;
    }
}

/**
 * Return a specific particle asset table from the main table.
 */
UNUSED ParticleDescriptor *get_particle_asset_table(s32 idx) {
    if (idx < gParticlesAssetTableCount) {
        return gParticlesAssetTable[idx];
    }
    return gParticlesAssetTable[gParticlesAssetTableCount - 1];
}

/**
 * Return the next particle table after the index.
 * Make sure the index is in range by wrapping it.
 */
UNUSED ParticleDescriptor *get_next_particle_table(s32 *idx) {
    *idx = *idx + 1;
    while (*idx >= gParticlesAssetTableCount) {
        *idx = *idx - gParticlesAssetTableCount;
    }
    return gParticlesAssetTable[*idx];
}

/**
 * Return the previous particle table before the index.
 * Make sure the index is in range by wrapping it.
 */
UNUSED ParticleDescriptor *get_previous_particle_table(s32 *idx) {
    *idx = *idx - 1;
    while (*idx < 0) {
        *idx += gParticlesAssetTableCount;
    }
    return gParticlesAssetTable[*idx];
}

/**
 * Return the particle behaviour ID from the behaviour table.
 */
UNUSED ParticleBehaviour *get_particle_behaviour(s32 idx) {
    if (idx < gParticleBehavioursAssetTableCount) {
        return gParticleBehavioursAssetTable[idx];
    }
    return gParticleBehavioursAssetTable[gParticleBehavioursAssetTableCount - 1];
}

/**
 * Return the next particle behaviour ID from the behaviour table.
 * Make sure the index is in range by wrapping it.
 */
UNUSED ParticleBehaviour *get_next_particle_behaviour(s32 *idx) {
    *idx += 1;
    while (*idx >= gParticleBehavioursAssetTableCount) {
        *idx -= gParticleBehavioursAssetTableCount;
    }
    return gParticleBehavioursAssetTable[*idx];
}

/**
 * Return the previous particle behaviour ID from the behaviour table.
 * Make sure the index is in range by wrapping it.
 */
UNUSED ParticleBehaviour *get_previous_particle_behaviour(s32 *idx) {
    *idx -= 1;
    while (*idx < 0) {
        *idx += gParticleBehavioursAssetTableCount;
    }
    return gParticleBehavioursAssetTable[*idx];
}

/**
 * Increases the opacity of all point particles spawned by the emitter.
 */
void increase_emitter_opacity(Object *obj, s32 idx, s32 delta, s32 maxOpacity) {
    s32 newOpacity;

    maxOpacity <<= 8;
    newOpacity = (obj->particleEmitter[idx].point_opacity & 0xFFFF) + delta;
    if (maxOpacity < newOpacity) {
        obj->particleEmitter[idx].point_opacity = maxOpacity;
    } else {
        obj->particleEmitter[idx].point_opacity = newOpacity;
    }
    obj->particleEmitter[idx].flags |= PARTICLE_OVERRIDE_OPACITY_FROM_DESCRIPTOR;
}

/**
 * Decreases the opacity of all point particles spawned by the emitter.
 */
void decrease_emitter_opacity(Object *obj, s32 idx, s32 delta, s32 maxOpacity) {
    s32 newOpacity;

    maxOpacity <<= 8;
    newOpacity = (obj->particleEmitter[idx].point_opacity & 0xFFFF) - delta;
    if (newOpacity < maxOpacity) {
        obj->particleEmitter[idx].point_opacity = maxOpacity;
    } else {
        obj->particleEmitter[idx].point_opacity = newOpacity;
    }
    obj->particleEmitter[idx].flags |= PARTICLE_OVERRIDE_OPACITY_FROM_DESCRIPTOR;
}
