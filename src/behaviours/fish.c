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

VertexPosition sFishVertices[6] = {
    { 0, 64, -64 }, { 0, -64, -64 }, { 0, 64, 32 }, { 0, -64, 32 }, { 0, 64, 64 }, { 0, -64, 64 },
};

// Fish Object Related
Triangle sFishTriangles[8] = {
    { { { 0, 0, 3, 1 } }, { { { 4, 0 } } }, { { { 1, 4 } } }, { { { 4, 4 } } } },
    { { { 0, 0, 2, 3 } }, { { { 4, 0 } } }, { { { 1, 0 } } }, { { { 1, 4 } } } },
    { { { 0, 2, 5, 3 } }, { { { 1, 0 } } }, { { { 0, 4 } } }, { { { 1, 4 } } } },
    { { { 0, 2, 4, 5 } }, { { { 1, 0 } } }, { { { 0, 0 } } }, { { { 0, 4 } } } },
    { { { 0, 0, 1, 3 } }, { { { 4, 0 } } }, { { { 4, 4 } } }, { { { 1, 4 } } } },
    { { { 0, 0, 3, 2 } }, { { { 4, 0 } } }, { { { 1, 4 } } }, { { { 1, 0 } } } },
    { { { 0, 2, 3, 5 } }, { { { 1, 0 } } }, { { { 1, 4 } } }, { { { 0, 4 } } } },
    { { { 0, 2, 5, 4 } }, { { { 1, 0 } } }, { { { 0, 4 } } }, { { { 0, 0 } } } },
};

void obj_init_fish(Object *fishObj, LevelObjectEntry *levelEntry, s32 param) {
    LevelObjectEntry_Fish *fishEntry = &levelEntry->fish;
    Object_Fish *fish;
    s32 pad0[2];
    f32 xPos;
    f32 zPos;
    f32 sins104;
    s32 uMask;
    s32 vMask;
    f32 coss104;
    s32 i;
    f32 tempxPos;

    fish = fishObj->fish;
    fish->unk100 = fishEntry->unkE << 4;
    fish->unk104 = fishEntry->unkF << 8;
    fish->unk114 = fishEntry->unk8;
    fish->unk118 = fishEntry->unkA;
    fish->unk108 = fishEntry->common.x;
    fish->unk10C = fishEntry->common.y;
    fish->unk110 = fishEntry->common.z;
    if (param == 0) {
        fish->unkFE = rand_range(0, 0x10000);
        fish->unk102 = rand_range(0, 0x10000);
        fish->unk106 = rand_range(0, 0x10000);
        fish->unkFD = 0;
    } else {
        fish->unkFE = 0x4000;
        fishObj->trans.rotation.y_rotation = fish->unk104;
    }

    xPos = sins_f(fish->unkFE * 2) * fish->unk114;
    zPos = coss_f(fish->unkFE) * fish->unk114;
    sins104 = sins_f(fish->unk104);
    coss104 = coss_f(fish->unk104);

    tempxPos = xPos;

    xPos = (tempxPos * coss104) + (zPos * sins104);
    zPos = (zPos * coss104) - (tempxPos * sins104);

    fishObj->trans.x_position = fish->unk108;
    fishObj->trans.z_position = fish->unk110;

    ignore_bounds_check();
    move_object(fishObj, xPos, 0.0f, zPos);
    if (param == 0) {
        for (i = 0; i < 8; i++) {
            fish->triangles[i].flags = sFishTriangles[i].flags;
            fish->triangles[i].vi0 = sFishTriangles[i].vi0;
            fish->triangles[i].vi1 = sFishTriangles[i].vi1;
            fish->triangles[i].vi2 = sFishTriangles[i].vi2;
        }
        uMask = 0;
        for (i = 0; i < 6; i++) {
            fish->vertices[uMask].x = sFishVertices[uMask].x;
            fish->vertices[uMask].y = sFishVertices[uMask].y;
            fish->vertices[uMask].z = sFishVertices[uMask].z;
            fish->vertices[uMask].r = 255;
            fish->vertices[uMask].g = 255;
            fish->vertices[uMask].b = 255;
            fish->vertices[uMask].a = 255;
            fish->vertices[uMask + 6].x = sFishVertices[uMask].x;
            fish->vertices[uMask + 6].y = sFishVertices[uMask].y;
            fish->vertices[uMask + 6].z = sFishVertices[uMask].z;
            fish->vertices[uMask + 6].r = 255;
            fish->vertices[uMask + 6].g = 255;
            fish->vertices[uMask + 6].b = 255;
            fish->vertices[uMask + 6].a = 255;
            uMask++;
        }
        fish->unkFC = 1;
    }
    fishObj->trans.scale = fishEntry->unkC * 0.01f;
    if (fishEntry->unkB < fishObj->header->numberOfModelIds) {
        fish->texture = fishObj->textures[fishEntry->unkB];
    } else {
        fish->texture = fishObj->textures[0];
    }
    if (fish->texture != NULL) {
        uMask = (fish->texture->width - 1) << 5;
        vMask = (fish->texture->height - 1) << 5;
    } else {
        uMask = 0;
        vMask = 0;
    }
    for (i = 0; i < 8; i++) {
        fish->triangles[i].uv0.u = sFishTriangles[i].uv0.u * uMask >> 2;
        fish->triangles[i].uv0.v = sFishTriangles[i].uv0.v * vMask >> 2;
        fish->triangles[i].uv1.u = sFishTriangles[i].uv1.u * uMask >> 2;
        fish->triangles[i].uv1.v = sFishTriangles[i].uv1.v * vMask >> 2;
        fish->triangles[i].uv2.u = sFishTriangles[i].uv2.u * uMask >> 2;
        fish->triangles[i].uv2.v = sFishTriangles[i].uv2.v * vMask >> 2;
    }
}

void obj_loop_fish(Object *fishObj, s32 updateRate) {
    f32 zThing;
    f32 yThing;
    f32 xThing;
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    f32 dist2D;
    f32 dist3D;
    f32 sinUnk104;
    Object_Fish *fish;
    f32 cosUnk104;
    s16 temp_v0_2;
    s32 temp_s0_2;
    s32 temp_t1;
    s32 temp_t9;
    f32 prevXThing, prevZThing;
    Vertex *verts;
    s32 randNumber;

    if (cam_get_viewport_layout() > 0) {
        free_object(fishObj);
        return;
    }
    fish = fishObj->fish;
    yThing = fishObj->trans.y_position;
    if (fish->unkFD != 0) {
        if (fish->unkFD >= updateRate) {
            yThing += fishObj->y_velocity * updateRate;
            fish->unkFD -= updateRate;
        } else {
            yThing += fishObj->y_velocity * fish->unkFD;
            fish->unkFD = 0;
        }
    } else {
        randNumber = rand_range(0, 7);
        if (randNumber == 0 && fish->unk118 >= 1.0f) {
            fish->unkFD = rand_range(160, 255);
            randNumber = rand_range(0, fish->unk118);
            fishObj->y_velocity = fish->unk10C - fishObj->trans.y_position;
            fishObj->y_velocity = (fishObj->trans.y_position < fish->unk10C) ? (fishObj->y_velocity + randNumber)
                                                                             : (fishObj->y_velocity - randNumber);
            fishObj->y_velocity /= fish->unkFD;
        }
    }
    fish->unkFE += updateRate * fish->unk100;
    zThing = sins_f(fish->unkFE * 2) * fish->unk114;
    xThing = coss_f(fish->unkFE) * fish->unk114;
    sinUnk104 = sins_f(fish->unk104);
    cosUnk104 = coss_f(fish->unk104);
    dist3D += 0; // Fakematch
    prevXThing = zThing;
    prevZThing = xThing;
    zThing = prevXThing; // Fakematch?
    zThing = (zThing * cosUnk104) + (prevZThing * sinUnk104);
    xThing = (prevZThing * cosUnk104) - (prevXThing * sinUnk104);
    zThing += fish->unk108;
    xThing += fish->unk110;
    xDiff = fishObj->trans.x_position - zThing;
    yDiff = fishObj->trans.y_position - yThing;
    zDiff = fishObj->trans.z_position - xThing;
    fishObj->trans.x_position = 0.0f;
    fishObj->trans.y_position = 0.0f;
    fishObj->trans.z_position = 0.0f;
    ignore_bounds_check();
    move_object(fishObj, zThing, yThing, xThing);
    dist2D = (xDiff * xDiff) + (zDiff * zDiff);
    dist3D = sqrtf((yDiff * yDiff) + dist2D);
    dist2D = sqrtf(dist2D);
    fish->unk106 += (s32) (dist3D * 0x600);
    fishObj->trans.rotation.y_rotation = arctan2_f(xDiff, zDiff);
    temp_v0_2 = arctan2_f(-yDiff, dist2D);
    temp_t9 = (temp_v0_2 - fishObj->trans.rotation.x_rotation) & 0xFFFF;
    temp_t1 = (fishObj->trans.rotation.x_rotation - temp_v0_2) & 0xFFFF;
    if (temp_t9 < temp_t1) {
        fishObj->trans.rotation.x_rotation += ((temp_t9 * updateRate) >> 3);
    } else {
        fishObj->trans.rotation.x_rotation -= ((temp_t1 * updateRate) >> 3);
    }
    fish->unkFC = 1 - fish->unkFC;
    verts = &fish->vertices[fish->unkFC * 6];
    randNumber = sins_s16(fish->unk106) >> 3;
    temp_s0_2 = (s32) (coss_f(randNumber) * 32);
    temp_t1 = (s32) (sins_f(randNumber) * 32);
    verts[4].x = (verts[2].x + temp_t1);
    verts[4].z = (verts[2].z + temp_s0_2);
    verts[5].x = (verts[3].x + temp_t1);
    verts[5].z = (verts[3].z + temp_s0_2);
}

void obj_render_fish(Object *obj) {
    f32 scaleY;
    Object_Fish *fish;

    fish = obj->fish;
    scaleY = obj->level_entry->fish.unkD;
    scaleY *= 0.01f;
    render_misc_model(obj, &fish->vertices[fish->unkFC * 6], 6, fish->triangles, 8, fish->texture,
                      RENDER_Z_COMPARE | RENDER_FOG_ACTIVE | RENDER_CUTOUT, 0, scaleY);
}
