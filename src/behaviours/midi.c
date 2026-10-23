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

void obj_init_midifade(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_MidiFade *entry = &levelEntry->midiFade;
    Object_MidiFade *midiFade;
    ModelInstance *modInst;
    ObjectTransform transform;
    f32 ox;
    f32 oy;
    f32 oz;
    ObjectModel *objModel;
    UNUSED s32 pad;
    MtxF mtx;
    f32 sinYRot;
    f32 tempF3;
    f32 minX;
    f32 minZ;
    f32 minY;
    f32 maxY;
    f32 maxX;
    f32 maxZ;
    f32 cosYRot;
    f32 scaleF;
    Vertex *vertex;
    f32 tempF2;
    s32 i;

    obj->trans.rotation.y_rotation = entry->angleY << 8 << 2; // Two shifts needed to skip a register.
    midiFade = obj->midi_fade;
    scaleF = (f32) (((s32) entry->scale) & 0xFFFF);
    if (scaleF < 1.0f) {
        scaleF = 1.0f;
    }
    scaleF /= 8;
    obj->trans.scale = obj->header->scale * (scaleF);
    transform.rotation.y_rotation = obj->trans.rotation.y_rotation;
    transform.rotation.x_rotation = obj->trans.rotation.x_rotation;
    transform.rotation.z_rotation = obj->trans.rotation.z_rotation;
    transform.scale = 1.0f;
    transform.x_position = 0.0f;
    transform.y_position = 0.0f;
    transform.z_position = 0.0f;
    mtxf_from_transform(&mtx, &transform);
    mtxf_transform_point(mtx, 0.0f, 0.0f, 1.0f, &ox, &oy, &oz);
    midiFade->planeX = ox;
    midiFade->planeY = oy;
    midiFade->planeZ = oz;
    midiFade->planeOffset =
        -((obj->trans.x_position * ox) + (obj->trans.y_position * oy) + (obj->trans.z_position * oz));
    midiFade->duration = entry->unk1A;
    midiFade->sequenceID = entry->unk1B;

    for (i = 0; i < ARRAY_COUNT(midiFade->channelModes) - 1; i++) {
        midiFade->channelModes[i] = entry->unkA[i];
    }

    modInst = obj->modelInstances[0];
    objModel = modInst->objModel;
    vertex = &objModel->vertices[1];
    maxX = vertex->x;
    maxY = vertex->y;
    maxZ = vertex->z;
    minX = maxX;
    minY = maxY;
    minZ = maxZ;
    for (i = 1; i < objModel->numberOfVertices; i++) {
        vertex = &objModel->vertices[i];
        if (vertex->x < minX) {
            minX = vertex->x;
        }
        if (maxX < vertex->x) {
            maxX = vertex->x;
        }
        if (vertex->y < minY) {
            minY = vertex->y;
        }
        if (maxY < vertex->y) {
            maxY = vertex->y;
        }
        if (vertex->z < minZ) {
            minZ = vertex->z;
        }
        if (maxZ < vertex->z) {
            maxZ = vertex->z;
        }
    }
    cosYRot = coss_f(obj->trans.rotation.y_rotation);
    sinYRot = sins_f(obj->trans.rotation.y_rotation);
    tempF3 = minX;
    minX = (minX * cosYRot) + (minZ * sinYRot);
    minZ = (minZ * cosYRot) - (tempF3 * sinYRot);
    tempF3 = maxX;
    maxX = (maxX * cosYRot) + (maxZ * sinYRot);
    maxZ = (maxZ * cosYRot) - (tempF3 * sinYRot);
    if (maxX < minX) {
        tempF2 = maxX;
        maxX = minX;
        minX = tempF2;
    }
    if (maxZ < minZ) {
        tempF2 = maxZ;
        maxZ = minZ;
        minZ = tempF2;
    }
    midiFade->minX = (obj->trans.scale * minX) + obj->trans.x_position;
    midiFade->minY = (obj->trans.scale * minY) + obj->trans.y_position;
    midiFade->minZ = (obj->trans.scale * minZ) + obj->trans.z_position;
    midiFade->maxX = (obj->trans.scale * maxX) + obj->trans.x_position;
    midiFade->maxY = (obj->trans.scale * maxY) + obj->trans.y_position;
    midiFade->maxZ = (obj->trans.scale * maxZ) + obj->trans.z_position;
    midiFade->progress = 0;
}

void obj_init_midifadepoint(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_MidiFadePoint *entry = &levelEntry->midiFadePoint;
    Object_MidiFadePoint *midi_fade_point;
    ModelInstance *modInst;
    ObjectModel *objModel;
    Vertex *vertex;
    s32 i;
    f32 x;
    f32 y;
    f32 z;
    f32 dist;

    midi_fade_point = obj->midi_fade_point;
    midi_fade_point->outerRadius = entry->unkA;
    midi_fade_point->innerRadius = entry->unk8;
    midi_fade_point->sequenceID = entry->unk1C;
    if (midi_fade_point->outerRadius < midi_fade_point->innerRadius) {
        midi_fade_point->outerRadius = midi_fade_point->innerRadius + 10;
    }
    obj->trans.rotation.z_rotation = 0;
    obj->trans.rotation.x_rotation = 0;
    obj->trans.rotation.y_rotation = 0;

    for (i = 0; i < 15; i++) {
        midi_fade_point->channelModes[i] = entry->unkC[i];
    }

    modInst = obj->modelInstances[0];
    objModel = modInst->objModel;
    vertex = objModel->vertices + 1;
    x = vertex->x;
    y = vertex->y;
    z = vertex->z;
    dist = sqrtf(((x * x) + (y * y)) + (z * z));
    midi_fade_point->innerScale = entry->unk8 / dist;
    midi_fade_point->outerScale = entry->unkA / dist;
    obj->trans.scale = midi_fade_point->outerScale;
}

void obj_init_midichset(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Midichset *entry = &levelEntry->midichset;
    Object_MidiChannelSet *temp = obj->midi_channel_set;
    temp->channelMask = entry->unk8;
    temp->radius = entry->unkA;
    temp->sequenceID = entry->unkB;
}
