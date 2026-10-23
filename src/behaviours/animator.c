#include "behaviours.h"

#include "audio.h"
#include "audio_spatial.h"
#include "audio_vehicle.h"
#include "audiosfx.h"
#include "camera.h"
#include "collision.h"
#include "common.h"
#include "fade_transition.h"
#include "font.h"
#include "game.h"
#include "game_text.h"
#include "game_ui.h"
#include "host.h"
#include "joypad.h"
#include "lights.h"
#include "macros.h"
#include "math_util.h"
#include "menu.h"
#include "object_models.h"
#include "objects.h"
#include "particles.h"
#include "PR/os_cont.h"
#include "PRinternal/viint.h"
#include "printf.h"
#include "racer.h"
#include "structs.h"
#include "textures_sprites.h"
#include "thread3_main.h"
#include "tracks.h"
#include "types.h"
#include "video.h"
#include "waves.h"
#include "weather.h"

/**
 * Object animator init function.
 * Sets up the initial properties of the animator, then calls the loop function once.
 * Official name: animInit
 */
void obj_init_animator(Object *obj, LevelObjectEntry *levelEntry, s32 param) {
    LevelObjectEntry_Animator *entry = &levelEntry->animator;
    Object_Animator *animator;
    LevelModel *levelModel;
    s16 segmentBatchCount;

    animator = obj->animator;
    levelModel = get_current_level_model();
    animator->batchId = entry->batchID;
    animator->speedFactorX = entry->speedFactorX;
    animator->speedFactorY = entry->speedfactorY;
    animator->segmentId =
        get_level_segment_index_from_position(obj->trans.x_position, obj->trans.y_position, obj->trans.z_position);
    // Always true.
    if (param == 0) {
        animator->xSpeed = 0;
        animator->ySpeed = 0;
    }
    if (animator->segmentId != -1) {
        if (animator->batchId < 0) {
            animator->batchId = 0;
        }
        segmentBatchCount = levelModel->segments[animator->segmentId].numberOfBatches;
        if (animator->batchId >= segmentBatchCount) {
            animator->batchId = segmentBatchCount - 1;
        }
    }
    obj_loop_animator(obj, 0x20000);
}

void obj_loop_animator(Object *obj, s32 updateRate) {
    s32 pad[2];
    Object_Animator *animator;
    LevelModel *levelModel;
    s32 trisStart;
    s32 trisEnd;
    s32 textureIndex;
    s32 tri;
    s32 shift; // a1
    s32 t0;
    s32 t1;
    Triangle *curTriangle;
    TriangleBatchInfo *curBatch;
    LevelModelSegment *curBlock;
    s32 shift2;
    TextureHeader *tex;

    animator = obj->animator;

    t0 = animator->speedFactorX;
    t1 = animator->speedFactorY;

    t0 *= updateRate;
    t1 *= updateRate;

    t0 <<= 4;
    t1 <<= 4;

    animator->xSpeed += t0;
    animator->ySpeed += t1;

    trisStart = animator->xSpeed;
    t0 = trisStart;
    trisEnd = animator->ySpeed;
    t1 = trisEnd;

    animator->xSpeed &= 0xF;
    animator->ySpeed &= 0xF;

    t0 >>= 4;
    t1 >>= 4;

    if (animator->segmentId == -1) {
        return;
    }

    levelModel = get_current_level_model();
    curBlock = &levelModel->segments[animator->segmentId];
    curBatch = &curBlock->batches[animator->batchId];
    textureIndex = curBatch->textureIndex;
    trisStart = curBatch->facesOffset;
    trisEnd = (curBatch + 1)->facesOffset;

    if (textureIndex != TEX_INDEX_NO_TEXTURE) {
        if (!curBatch) {} // Fake
        tex = levelModel->textures[textureIndex].texture;
        shift2 = tex->width << 7;
        shift = tex->width << 7;
        for (tri = trisStart; tri < trisEnd; tri++) {
            curTriangle = &curBlock->triangles[tri];
            if (!(curTriangle->flags & TRI_FLAG_80)) {
                if ((shift << 1) < curTriangle->uv0.v) {
                    curTriangle->uv0.v -= shift;
                    curTriangle->uv1.v -= shift;
                    curTriangle->uv2.v -= shift;
                }
                if (curTriangle->uv0.v < 0) {
                    curTriangle->uv0.v += shift;
                    curTriangle->uv1.v += shift;
                    curTriangle->uv2.v += shift;
                }
                if ((shift2 << 1) < curTriangle->uv0.u) {
                    curTriangle->uv0.u -= shift2;
                    curTriangle->uv1.u -= shift2;
                    curTriangle->uv2.u -= shift2;
                }
                if (curTriangle->uv0.u < 0) {
                    curTriangle->uv0.u += shift2;
                    curTriangle->uv1.u += shift2;
                    curTriangle->uv2.u += shift2;
                }
                curTriangle->uv0.v += t1;
                curTriangle->uv1.v += t1;
                curTriangle->uv2.v += t1;
                curTriangle->uv0.u += t0;
                curTriangle->uv1.u += t0;
                curTriangle->uv2.u += t0;
            }
        }
    }
}
