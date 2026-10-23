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

/* Official name: texscrollInit */
void obj_init_texscroll(Object *obj, LevelObjectEntry *levelEntry, s32 arg2) {
    LevelObjectEntry_TexScroll *entry = &levelEntry->texScroll;
    Object_TexScroll *texscroll;
    LevelModel *levelModel;
    s16 numberOfTexturesInLevel;

    texscroll = obj->tex_scroll;
    levelModel = get_current_level_model();
    texscroll->textureIndex = entry->textureIndex;
    if (texscroll->textureIndex < 0) {
        texscroll->textureIndex = 0;
    }
    numberOfTexturesInLevel = levelModel->numberOfTextures;
    if (texscroll->textureIndex >= numberOfTexturesInLevel) {
        texscroll->textureIndex = numberOfTexturesInLevel - 1;
    }
    texscroll->scrollSpeedU = entry->scrollSpeedU;
    texscroll->scrollSpeedV = entry->scrollSpeedV;
    if (arg2 == 0) {
        texscroll->scrollAccumU = 0;
        texscroll->scrollAccumV = 0;
    }
}

void obj_loop_texscroll(Object *obj, s32 updateRate) {
    s32 pad[2];
    LevelModel *levelModel;
    LevelModelSegment *curBlock;
    Object_TexScroll *texScroll;
    Triangle *curTriangle;
    TriangleBatchInfo *curBatch;
    s32 prevUnk8;
    s32 prevUnkA;
    s32 tri;
    s32 uShift;
    s32 vShift;
    s32 block;
    s32 batch;
    TextureInfo *texture;
    TextureHeader *tex;
    s32 texIndex;
    s32 temp;
    s32 temp2;
    u8 temp3;
    s32 t0;
    s32 t1;
    s32 i;
    s32 j;

    texScroll = obj->tex_scroll;
    levelModel = get_current_level_model();

    t0 = texScroll->scrollSpeedU;
    t1 = texScroll->scrollSpeedV;

    tex = levelModel->textures[texScroll->textureIndex].texture;

    uShift = tex->width;
    vShift = tex->height;
    uShift <<= 8;
    vShift <<= 8;

    t0 *= updateRate;
    t1 *= updateRate;

    texScroll->scrollAccumU += t0;
    texScroll->scrollAccumV += t1;

    t0 = texScroll->scrollAccumU;
    t1 = texScroll->scrollAccumV;

    texScroll->scrollAccumU &= 3;
    texScroll->scrollAccumV &= 3;

    t0 = t0 >> 2;
    t1 = ((t1 >> 2) & 0xFFFFFFFFFFFFFFFF); // fake

    curBlock = levelModel->segments;
    for (i = 0; i < levelModel->numberOfSegments; i++) {
        curBatch = curBlock[i].batches;
        for (j = 0; j < curBlock[i].numberOfBatches; j++) {
            if (curBatch[j].textureIndex == texScroll->textureIndex) {
                for (tri = curBatch[j].facesOffset; tri < curBatch[j + 1].facesOffset; tri++) {
                    curTriangle = &curBlock[i].triangles[tri];
                    if (!(curTriangle->flags & TRI_FLAG_80)) {
                        if (vShift < curTriangle->uv0.v) {
                            curTriangle->uv0.v -= vShift;
                            curTriangle->uv1.v -= vShift;
                            curTriangle->uv2.v -= vShift;
                        }
                        if (curTriangle->uv0.v < 0) {
                            curTriangle->uv0.v += vShift;
                            curTriangle->uv1.v += vShift;
                            curTriangle->uv2.v += vShift;
                        }
                        if (uShift < curTriangle->uv0.u) {
                            curTriangle->uv0.u -= uShift;
                            curTriangle->uv1.u -= uShift;
                            curTriangle->uv2.u -= uShift;
                        }
                        if (curTriangle->uv0.u < 0) {
                            curTriangle->uv0.u += uShift;
                            curTriangle->uv1.u += uShift;
                            curTriangle->uv2.u += uShift;
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
    }
}
