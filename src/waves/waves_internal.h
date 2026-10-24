#ifndef DKR_WAVES_INTERNAL_H
#define DKR_WAVES_INTERNAL_H

/*
 * Shared between the files of src/waves/: the includes they all use, the wave state they share
 * (defined in waves.c) and the functions one file calls in another.
 */
#include "camera.h"
#include "host.h"
#include "macros.h"
#include "math_util.h"
#include "memory.h"
#include "objects.h"
#include "pc_gfx.h"
#include "printf.h"
#include "racer.h"
#include "textures_sprites.h"
#include "tracks.h"
#include "types.h"
#include "waves.h"
#include <ultra64.h>

#define gWaveVisibleBlocksBody (gWaveVisibleBlocks + 2)
// Entries of gWaveVisibleBlocksBody (gWaveVisibleBlocks after its first two).
#define WAVE_VISIBLE_BLOCKS_BODY_COUNT 24

// gWaveDetailLevels holds the water depth under each wave vertex. Below this depth the swell is damped
// towards gWaveController.minDetailScale; at or above it the waves have their full height.
#define WAVE_FULL_SWELL_DEPTH 0x7F

// Each column of the wave grid lists up to WAVE_GEN_SLOTS wave generators (swells) that reach it, as indices
// into gWaveGenList; unused slots hold WAVE_GEN_SLOT_EMPTY and come after the used ones.
#define WAVE_GEN_SLOTS 8
#define WAVE_GEN_SLOT_EMPTY 0xFF
// Wave generators a level can have at once (entries of gWaveGenObjs).
#define WAVE_GEN_MAX 32
// wavegen_register flags: the swell travels along one axis instead of spreading out in rings.
#define WAVEGEN_LINEAR_X 1
#define WAVEGEN_LINEAR_Z 2

// Starting (and fully faded) value of LevelModel_Alternate.viewportFade.
#define WAVE_FADE_MAX 0x80

// gWaveController.waveViewDist: wave cells drawn in full around the camera, as a square this many cells
// across. Two-player games always use the near distance.
#define WAVE_VIEW_DIST_NEAR 3
#define WAVE_VIEW_DIST_FAR 5
// Most wave cells that can be visible at once (WAVE_VIEW_DIST_FAR squared); each vertex buffer holds a grid
// for every one.
#define WAVE_MAX_VISIBLE_CELLS 25
// Two-player games always use this many wave subdivisions per segment, whatever the level header says.
#define WAVE_SUBDIVISIONS_2P 4

extern f32 *gWaveHeightTable;
extern Vec2s *gWaveHeightIndices;
extern TexCoords *gWaveUVTable;
extern f32 *gWaveHeightBuffers[9];
extern Vertex *gWaveVertices[4];
extern Triangle *gWaveTriangles[4];
extern Triangle gWaveQuadTriangles[4];
extern TextureHeader *gWaveTextureHeader;
extern s32 *gWaveBlockCells;
extern LevelModel_Alternate *gWaveModel;
extern s32 gVisibleWaveTiles;
extern s16 *gWaveCellLookup;
extern s16 *gWaveCellIndexLookup;
extern u8 *gWaveDetailLevels;
extern s32 gWaveTileTotal;
extern ColourCycle *gWavePulseLight;
extern WaveGenTileSlots *gWaveGenSlots;
extern s32 gWaveGenCount;
extern s32 gWaveTileGridCount;
extern WaveGen *gWaveGenList;
extern Object **gWaveGenObjs;
extern Object *gWaveGeneratorObj;
extern Gfx *gWaveDL;
extern Mtx *gWaveMtx;
extern WaveControl gWaveController;
extern s32 gWaveVertexFlip;
extern f32 gWaveLowerY;
extern f32 gWaveUpperY;
extern Vertex gWaveCornerVertices[2][4];
extern s32 gWavePlayerCount;
extern TriangleBatchInfo *gWaveBatch;
extern TextureHeader *gWaveTexture;
extern s32 gWaveUVBaseX;
extern s32 gWaveUVBaseY;
extern s32 gWaveUVStepX;
extern s32 gWaveUVStepY;
extern s32 gWaveTexUVMaskX;
extern s32 gWaveTexUVMaskY;
extern s32 gWaveTexAnimFrame;
extern f32 gWaveBoundingBoxW;
extern f32 gWaveBoundingBoxH;
extern s32 gWaveBoundingBoxDiffX;
extern s32 gWaveBoundingBoxDiffZ;
extern s32 gWaveBoundingBoxX1;
extern s32 gWaveBoundingBoxZ1;
extern f32 gWaveVtxStepX;
extern f32 gWaveVtxStepZ;
extern s32 gWaveBlockBoundsX1;
extern s32 gWaveBlockBoundsZ1;
extern s32 gWaveBlockBoundsX2;
extern s32 gWaveBlockBoundsZ2;
extern s32 gWaveBlockPosX;
extern s32 gWaveBlockPosZ;
extern s32 gWaveTileCountX;
extern s32 gWaveTileCountZ;
extern s32 gNumberOfLevelSegments;
extern s32 gWaveVisibilityGrid[64];
extern s16 gWaveBlockIDs[512];
extern WaveVisibleBlock gWaveVisibleBlocks[26];
extern f32 gWavePowerBase;
extern f32 gWaveMagnitude;
extern s32 gWavePowerDivisor;

void wave_block_build_vertices(s32 blockID, s32 viewportID);
void wave_block_build_vertices_depth_shaded(s32 blockID, s32 viewportID);
void waves_update(s32 updateRate);
void waves_render(Gfx **dList, Mtx **mtx, s32 viewportID);
void waves_init_level(LevelModel *level, LevelHeader *header);
void waves_build_sine_table(void);
void waves_init_collision(LevelModel *model);
f32 waves_get_y(s32 blockID, s32 cellX, s32 cellZ);
void wavegen_destroy(Object *obj);
WaveGen *wavegen_register(Object *obj, f32 xPos, f32 zPos, f32 waveSize, s32 phase, f32 waveSpeed, f32 wavelength,
                          f32 amplitude, s32 flags);
void wave_gens_update(s32 updateRate);

#endif
