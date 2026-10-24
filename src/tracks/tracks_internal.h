#ifndef DKR_TRACKS_INTERNAL_H
#define DKR_TRACKS_INTERNAL_H

/*
 * Shared between the files of src/tracks/: the includes they all use, the level geometry
 * state they share (defined in tracks.c) and the functions one file calls in another.
 */
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
#include "libc/string.h"
#include "macros.h"
#include "math_util.h"
#include "memory.h"
#include "menu.h"
#include "objects.h"
#include "particles.h"
#include "pc_swap.h"
#include "PR/gu.h"
#include "racer.h"
#include "structs.h"
#include "textures_sprites.h"
#include "thread3_main.h"
#include "tracks.h"
#include "types.h"
#include "video.h"
#include "waves.h"
#include "weather.h"

// Maximum size for a level model is 522.5 KiB
#define LEVEL_MODEL_MAX_SIZE 0xA0000 // host layout adds a header delta plus segment/texture tables
#define LEVEL_SEGMENT_MAX 128
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
#define PC_REBASE(obj, field, T) (obj)->field = (T) ((uintptr_t) (obj)->field + LEVEL_MODEL_PC_DELTA)

// gShadowBucketVertices holds one bucket of this many clip vertices per edge of the shadow rectangle.
#define SHADOW_BUCKET_SIZE 32
// Set on a triangle's edge plane index when the plane faces the other way for it (shared edges).
#define EDGE_PLANE_FLIPPED 0x8000
#define EDGE_PLANE_INDEX_MASK 0x7FFF

extern LevelModel *gCurrentLevelModel;
extern LevelHeader *gCurrentLevelHeader2;
extern u8 *gVoidData;
extern Gfx *gTrackDL;
extern Mtx *gTrackMtxPtr;
extern Vertex *gTrackVtxPtr;
extern Triangle *gTrackTriPtr;
extern Camera *gSceneActiveCamera;
extern Object *gSkydomeSegment;
extern s32 gIsNearCurrBBox;
extern s32 gDisableShadows;
extern s32 gShadowHeapFlip;
extern s32 gShadowIndex;
extern s32 gWaterEffectIndex;
extern s32 gSceneStartSegment;
extern s32 gSceneRenderSkyDome;
extern s8 gDrawLevelSegments;
extern u8 gVoidColourR;
extern u8 gVoidColourG;
extern u8 gVoidColourB;
extern f32 gCollisionNormalX;
extern f32 gCollisionNormalY;
extern f32 gCollisionNormalZ;
extern s32 gHitWall;
extern s32 gCollisionMode;
extern s32 gLevelIsCutscene;
extern s32 gAntiAliasing;
extern s32 gTrackTexAnimOffset;
extern s32 gShadowVertexCount;
extern ShadowVertex gShadowVertices[32];
extern s32 gShadowBucketCounts[4];
extern ShadowClipVertex gShadowBucketVertices[120];
extern s32 gShadowTriCount;
extern ShadowPolygon gShadowTris[32];
extern ShadowWaveTri gShadowWaveCells[64];
extern ShadowPlane gShadowWavePlanes[128];
extern s32 gShadowWavePlaneCount;
extern ShadowPlane *gShadowCurrentPlane;
extern TextureHeader *gNewShadowTexture;
extern Object *gNewShadowObj;
extern f32 gShadowYOffset;
extern s16 gNewShadowY1;
extern s16 gNewShadowY2;
extern s16 gShadowMinY;
extern f32 gShadowOpacity;
extern f32 gNewShadowScale;
extern f32 gNewShadowWidth;
extern f32 gNewShadowLength;
extern f32 gShadowArea;
extern s32 gShadowCentreTri;
extern s32 gShadowCentreShared;
extern f32 gShadowFadeStart;
extern f32 gShadowFadeRange;
extern Vec4f gFrustumPlanes[3];
extern WaterProperties gWaterSurfaceHits[20];
extern WaterProperties *gTrackWaves[20];
extern s8 gWaterSurfaceHitCount;
extern LevelModel *gTrackModelHeap;
extern s32 *gLevelModelTable;
extern f32 gPrevCameraX;
extern f32 gPrevCameraY;
extern f32 gPrevCameraZ;
extern Triangle *gShadowHeapTris[2 + 2];
extern Triangle *gCurrShadowTris;
extern Vertex *gShadowHeapVerts[2 + 2];
extern Vertex *gCurrShadowVerts;
extern ShadowHeapProperties *gShadowHeapData[2 + 2];
extern ShadowHeapProperties *gCurrShadowHeapData;
extern s32 gShadowTail;
extern s32 gNewShadowTriCount;
extern s32 gNewShadowVtxCount;
extern ptrint_t *gCollisionCandidates;
extern s8 *gCollisionSurfaces;
extern s32 gNumCollisionCandidates;
extern s32 gScenePlayerViewports;
extern f32 gCurrBBoxDistanceToCamera;
extern u32 gWaveBlockCount;
extern FogData gFogData[4];
extern Vec3i gScenePerspectivePos;
extern VoidMesh *gVoidMesh;
extern VoidCheckPoint *gVoidCheckPoints;
extern s8 *gVoidCheckPairs;
extern Vertex *gVoidVerts[2];
extern Vertex *gVoidCurrVerts;
extern Triangle *gVoidTris[2];
extern Triangle *gVoidCurrTris;
extern s16 gVoidCheckPairIndex;
extern s16 gVoidCheckPointCount;
extern f32 gVoidCheckSidePlaneX;
extern f32 gVoidCheckSidePlaneZ;
extern f32 gVoidCheckSidePlaneD;
extern f32 gVoidCheckAheadX;
extern f32 gVoidCheckAheadZ;
extern s8 gVoidVertexFlip;
extern s16 gVoidVertCount;
extern s16 gVoidPrimCount;
extern s16 gVoidCheckMaxPoints;
extern s16 gVoidPrimLimit;

void void_init(s32 viewportCount);
void void_free(void);
void void_check(u8 *segmentIds, s32 numberOfSegments, s32 viewportIndex);
s32 is_camera_behind_wall(void);
void track_tex_anim(s32 updateRate);
void skydome_spawn(s32 objectID);
void trackbg_render_flashy(void);
void trackbg_render_gradient(void);
void skydome_render(void);
void initialise_player_viewport_vars(s32 updateRate);
s32 get_level_segment_index_from_position(f32 xPos, f32 yPos, f32 zPos);
s32 get_inside_segment_count_xz(s32 x, s32 z, s32 *segmentsOut);
s32 get_inside_segment_count_xyz(s32 *segmentsOut, s16 xPos1, s16 yPos1, s16 zPos1, s16 xPos2, s16 yPos2, s16 zPos2);
void compute_frustum_planes(void);
void generate_track(s32 modelId);
void segment_compute_triangle_grid_masks(LevelModelSegment *segment, LevelModelSegmentBoundingBox *bbox,
                                         s32 segmentIndex);
s32 track_init_collision(LevelModelSegment *block);
void shadow_render(Object *obj, ShadowData *shadow);
void watereffect_render(Object *obj, WaterEffect *effect);
void shadow_update(s32 group, s32 waterGroup, s32 updateRate);
void update_fog(s32 viewportCount, s32 updateRate);
void apply_fog(s32 playerID);

#endif
