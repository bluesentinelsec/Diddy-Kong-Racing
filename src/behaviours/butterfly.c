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

// Butterfly Object Related
Triangle sButterflyTriangles[8] = {
    { { { 0, 0, 3, 1 } }, { { { 0, 0 } } }, { { { -1, -1 } } }, { { { 0, -1 } } } },
    { { { 0, 0, 2, 3 } }, { { { 0, 0 } } }, { { { -1, 0 } } }, { { { -1, -1 } } } },
    { { { 0, 2, 5, 3 } }, { { { -1, 0 } } }, { { { 0, -1 } } }, { { { -1, -1 } } } },
    { { { 0, 2, 4, 5 } }, { { { -1, 0 } } }, { { { 0, 0 } } }, { { { 0, -1 } } } },
    { { { 0, 0, 1, 3 } }, { { { 0, 0 } } }, { { { 0, -1 } } }, { { { -1, -1 } } } },
    { { { 0, 0, 3, 2 } }, { { { 0, 0 } } }, { { { -1, -1 } } }, { { { -1, 0 } } } },
    { { { 0, 2, 3, 5 } }, { { { -1, 0 } } }, { { { -1, -1 } } }, { { { 0, -1 } } } },
    { { { 0, 2, 5, 4 } }, { { { -1, 0 } } }, { { { 0, -1 } } }, { { { 0, 0 } } } },
};

VertexPosition sButterflyVertices[6] = {
    { -64, 0, 32 }, { -64, 0, -32 }, { 0, 0, 32 }, { 0, 0, -32 }, { 64, 0, 32 }, { 64, 0, -32 },
};

void obj_init_butterfly(Object *butterflyObj, LevelObjectEntry *levelEntry, s32 param) {
    LevelObjectEntry_Butterfly *butterflyEntry = &levelEntry->butterfly;
    Object_Butterfly *butterfly;
    s32 uMask;
    s32 vMask;
    s32 i;

    butterfly = butterflyObj->butterfly;
    if (param == 0) {
        butterflyObj->y_velocity = 0.0f;
        butterfly->unkFE = 0;
        butterfly->unk100 = 0;
        butterfly->unk104 = 0;
        butterfly->unkFD = 0;
        butterfly->unk106 = 0x180;
        butterfly->unk108 = 0.0f;
        for (i = 0; i < 8; i++) {
            butterfly->triangles[i].flags = sButterflyTriangles[i].flags;
            butterfly->triangles[i].vi0 = sButterflyTriangles[i].vi0;
            butterfly->triangles[i].vi1 = sButterflyTriangles[i].vi1;
            butterfly->triangles[i].vi2 = sButterflyTriangles[i].vi2;
        }
        uMask = 0;
        for (i = 0; i < 6; i++) {
            butterfly->vertices[uMask].x = sButterflyVertices[uMask].x;
            butterfly->vertices[uMask].y = sButterflyVertices[uMask].y;
            butterfly->vertices[uMask].z = sButterflyVertices[uMask].z;
            butterfly->vertices[uMask].r = 255;
            butterfly->vertices[uMask].g = 255;
            butterfly->vertices[uMask].b = 255;
            butterfly->vertices[uMask].a = 255;
            butterfly->vertices[uMask + 6].x = sButterflyVertices[uMask].x;
            butterfly->vertices[uMask + 6].y = sButterflyVertices[uMask].y;
            butterfly->vertices[uMask + 6].z = sButterflyVertices[uMask].z;
            butterfly->vertices[uMask + 6].r = 255;
            butterfly->vertices[uMask + 6].g = 255;
            butterfly->vertices[uMask + 6].b = 255;
            butterfly->vertices[uMask + 6].a = 255;
            uMask++;
        }
        butterfly->unkFC = 1;
    }
    butterflyObj->trans.scale = butterflyEntry->unkB * 0.01f;
    if (butterflyEntry->unkA < butterflyObj->header->numberOfModelIds) {
        butterfly->texture = butterflyObj->textures[butterflyEntry->unkA];
    } else {
        butterfly->texture = butterflyObj->textures[0];
    }
    if (butterfly->texture != NULL) {
        uMask = (butterfly->texture->width - 1) << 5;
        vMask = (butterfly->texture->height - 1) << 5;
    } else {
        uMask = 0;
        vMask = 0;
    }
    for (i = 0; i < 8; i++) {
        butterfly->triangles[i].uv0.u = sButterflyTriangles[i].uv0.u & uMask;
        butterfly->triangles[i].uv0.v = sButterflyTriangles[i].uv0.v & vMask;
        butterfly->triangles[i].uv1.u = sButterflyTriangles[i].uv1.u & uMask;
        butterfly->triangles[i].uv1.v = sButterflyTriangles[i].uv1.v & vMask;
        butterfly->triangles[i].uv2.u = sButterflyTriangles[i].uv2.u & uMask;
        butterfly->triangles[i].uv2.v = sButterflyTriangles[i].uv2.v & vMask;
    }
}

void obj_loop_butterfly(Object *butterflyObj, s32 updateRate) {
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    f32 radius;
    f32 sp84;
    f32 var_f0;
    f32 sp7C;
    f32 sp78;
    s16 var_v1;
    s16 shiftAmount;
    s16 sp72;
    s16 temp;
    s32 xPos;
    s32 yPos;
    s32 sp64;
    Object *sp44[8];
    Vertex *vertices;
    Object_Butterfly *butterfly;
    LevelObjectEntry_Butterfly *butterflyEntry;

    butterflyEntry = &butterflyObj->level_entry->butterfly;
    butterfly = butterflyObj->butterfly;
    sp64 = butterflyEntry->unkA == 2;
    sp72 = butterfly->unk106;
    if (sp72 < 0) {
        sp72 = -sp72;
    }
    sp7C = 0.1f;
    sp72 *= updateRate;
    if (sp64) {
        sp7C *= 1.6f;
    }
    sp78 = 3.0f;
    if (sp64) {
        sp78 *= 1.6f;
    }
    switch (butterfly->unkFD) {
        case 0:
            butterfly->unk108 = 0.0f;
            sp84 = butterflyEntry->unk8;
            temp = obj_dist_racer(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, sp84, 1, sp44);
            if (temp > 0) {
                butterfly->unk100 = sp44[0];
                butterfly->unk104 = 0xF0;
                butterfly->unkFD = 3;
            } else {
                butterfly->unk100 =
                    obj_butterfly_node(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, sp84, 1);
                if (butterfly->unk100 != 0) {
                    butterfly->unk104 = 0xF0;
                    butterfly->unkFD = 3;
                }
            }
            butterflyObj->animFrame = (butterflyObj->animFrame + updateRate) & 0xFF;
            if (sp64) {
                butterflyObj->animFrame = 0x78;
            }
            break;
        case 1:
        case 2:
            if (butterfly->unk106 < 0x480) {
                butterfly->unk106 += (updateRate * 0x10);
                if (butterfly->unk106 >= 0x481) {
                    butterfly->unk106 = 0x480;
                }
            }
            xDiff = butterflyObj->trans.x_position - butterflyEntry->common.x;
            zDiff = butterflyObj->trans.z_position - butterflyEntry->common.z;
            temp = arctan2_f(xDiff, zDiff);
            temp -= butterflyObj->trans.rotation.y_rotation & 0xffff & 0xffff;
            if (temp < 0) {
                if (temp > -sp72) {
                    butterflyObj->trans.rotation.y_rotation += temp;
                } else {
                    butterflyObj->trans.rotation.y_rotation -= sp72;
                }
            } else if ((temp > 0)) {
                if (temp < sp72) {
                    butterflyObj->trans.rotation.y_rotation += temp;
                } else {
                    butterflyObj->trans.rotation.y_rotation += sp72;
                }
            }
            sp84 = butterflyEntry->common.y - butterflyObj->trans.y_position;
            if (sp84 < 0.0f) {
                butterflyObj->y_velocity -= sp7C * updateRate;
                if (butterflyObj->y_velocity < sp84) {
                    butterflyObj->y_velocity = sp84;
                }
            } else if ((sp84 > 0.0f)) {
                butterflyObj->y_velocity += sp7C * updateRate;
                if (sp84 < butterflyObj->y_velocity) {
                    butterflyObj->y_velocity = sp84;
                }
            } else {
                butterflyObj->y_velocity = 0.0f;
            }
            if (butterfly->unkFD == 1) {
                sp84 = butterflyEntry->unk8;
                temp = obj_dist_racer(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, sp84, 1, sp44);
                if (temp > 0) {
                    butterfly->unk100 = sp44[0];
                    butterfly->unk104 = 0x78;
                    butterfly->unkFD = 3;
                } else {
                    butterfly->unk100 =
                        obj_butterfly_node(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, sp84, 1);
                    if (butterfly->unk100 != NULL) {
                        butterfly->unk104 = 0xF0;
                        butterfly->unkFD = 3;
                    }
                }
            }
            if (butterfly->unkFD != 3) {
                yDiff = butterflyObj->trans.y_position - butterflyEntry->common.y;
                if (sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)) < 4.0f) {
                    ignore_bounds_check();
                    move_object(butterflyObj, -xDiff, -yDiff, -zDiff);
                    butterflyObj->y_velocity = 0.0f;
                    butterfly->unkFD = 0;
                    butterfly->unk108 = 0.0f;
                }
            }
            butterflyObj->animFrame = (butterflyObj->animFrame + (updateRate * 0x10)) & 0xFF;
            break;
        case 3:
            if (butterfly->unk106 < 0x480) {
                butterfly->unk106 += (updateRate * 0x10);
                if (butterfly->unk106 >= 0x481) {
                    butterfly->unk106 = 0x480;
                }
            }
            xDiff = butterfly->unk100->trans.x_position - butterflyEntry->common.x;
            zDiff = butterfly->unk100->trans.z_position - butterflyEntry->common.z;
            radius = (butterflyEntry->unk8 * 2);
            radius *= radius;
            if (radius < ((xDiff * xDiff) + (zDiff * zDiff))) {
                butterfly->unk100 = 0;
                sp84 = butterflyEntry->unk8;
                temp = obj_dist_racer(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, sp84, 1, sp44);
                if (temp > 0) {
                    butterfly->unk100 = sp44[0];
                    butterfly->unk104 = 0xF0;
                } else {
                    butterfly->unk100 =
                        obj_butterfly_node(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, sp84, 1);
                    if (butterfly->unk100 != 0) {
                        butterfly->unk104 = 0xF0;
                    }
                }
            }
            if (butterfly->unk100 != 0) {
                xDiff = butterflyObj->trans.x_position - butterfly->unk100->trans.x_position;
                zDiff = butterflyObj->trans.z_position - butterfly->unk100->trans.z_position;
                temp = arctan2_f(xDiff, zDiff);
                temp -= butterflyObj->trans.rotation.y_rotation & 0xffff & 0xffff;
                if (temp < 0) {
                    if (-sp72 < temp) {
                        butterflyObj->trans.rotation.y_rotation += temp;
                    } else {
                        butterflyObj->trans.rotation.y_rotation -= sp72;
                    }
                } else if ((temp > 0)) {
                    if (temp < sp72) {
                        butterflyObj->trans.rotation.y_rotation += temp;
                    } else {
                        butterflyObj->trans.rotation.y_rotation += sp72;
                    }
                }
                sp84 = (butterfly->unk100->trans.y_position + 16.0f) - butterflyObj->trans.y_position;
                if (sp84 < 0.0f) {
                    butterflyObj->y_velocity -= sp7C * updateRate;
                    if (butterflyObj->y_velocity < sp84) {
                        butterflyObj->y_velocity = sp84;
                    }
                } else if ((sp84 > 0.0f)) {
                    butterflyObj->y_velocity += sp7C * updateRate;
                    if (sp84 < butterflyObj->y_velocity) {
                        butterflyObj->y_velocity = sp84;
                    }
                }
                if (((xDiff * xDiff) + (zDiff * zDiff)) < 256.0f) {
                    butterfly->unkFD = 4;
                }
            } else {
                butterfly->unkFD = 1;
            }
            butterflyObj->animFrame = (butterflyObj->animFrame + (updateRate * 0x10)) & 0xFF;
            break;
        case 4:
            if (butterfly->unk106 >= 0x181) {
                butterfly->unk106 -= (updateRate * 0x10);
                if (butterfly->unk106 < 0x180) {
                    butterfly->unk106 = 0x180;
                }
            }
            xDiff = butterflyObj->trans.x_position - butterflyEntry->common.x;
            zDiff = butterflyObj->trans.z_position - butterflyEntry->common.z;
            radius = (butterflyEntry->unk8 * 2);
            radius *= radius;
            if (radius < ((xDiff * xDiff) + (zDiff * zDiff))) {
                butterfly->unkFD = 2;
            } else {
                if (updateRate < butterfly->unk104) {
                    butterfly->unk104 -= updateRate;
                } else {
                    butterfly->unk104 = 0;
                    var_v1 = obj_dist_racer(butterflyObj->trans.x_position, 0.0f, butterflyObj->trans.z_position,
                                            150.0f, 1, sp44);
                    if (var_v1 == 0) {
                        butterfly->unk100 = 0;
                        butterfly->unkFD = 1;
                    } else {
                        if (var_v1 >= 2) {
                            var_v1 = rand_range(1, var_v1);
                        }
                        var_v1--;
                        butterfly->unk100 = sp44[var_v1];
                        butterfly->unk104 = 0xF0;
                    }
                }
                if (butterfly->unkFD == 4) {
                    xDiff = butterflyObj->trans.x_position - butterfly->unk100->trans.x_position;
                    zDiff = butterflyObj->trans.z_position - butterfly->unk100->trans.z_position;
                    if (((xDiff * xDiff) + (zDiff * zDiff)) > 22500.0f) {
                        butterfly->unkFD = 3;
                    } else {
                        if (rand_range(0, 0x64) >= 0x63) {
                            butterfly->unk106 = -butterfly->unk106;
                        }
                        butterflyObj->trans.rotation.y_rotation += updateRate * butterfly->unk106;
                        if ((butterfly->unk100->trans.y_position + 64.0f) < butterflyObj->trans.y_position) {
                            butterfly->unkFE = 0;
                        } else if (butterflyObj->trans.y_position < (butterfly->unk100->trans.y_position + 8.0f)) {
                            butterfly->unkFE = 1;
                        }
                        if (butterfly->unkFE != 0) {
                            butterflyObj->y_velocity += (sp7C * updateRate);
                        } else {
                            butterflyObj->y_velocity -= (sp7C * updateRate);
                        }
                        butterflyObj->trans.rotation.y_rotation += butterfly->unk106;
                    }
                }
            }
            butterflyObj->animFrame = (butterflyObj->animFrame + (updateRate * 0x10)) & 0xFF;
            break;
    }
    if (butterfly->unkFD != 0) {
        if (butterfly->unk108 < sp78) {
            butterfly->unk108 += (0.25f * updateRate);
            if (sp78 < butterfly->unk108) {
                butterfly->unk108 = sp78;
            }
        }
    }
    if (butterfly->unk108 != 0.0f) {
        if (butterfly->unkFD != 4) {
            var_f0 = 1.2f;
        } else {
            var_f0 = 1.6f;
        }
        if (sp64) {
            var_f0 *= 1.6f;
        }
        if (butterflyObj->y_velocity < -var_f0) {
            butterflyObj->y_velocity = -var_f0;
        }
        if (var_f0 < butterflyObj->y_velocity) {
            butterflyObj->y_velocity = var_f0;
        }
        if (butterfly->unk108 != 0.0f) {
            butterflyObj->x_velocity = sins_f(butterflyObj->trans.rotation.y_rotation) * -butterfly->unk108;
            butterflyObj->z_velocity = coss_f(butterflyObj->trans.rotation.y_rotation) * -butterfly->unk108;
            move_object(butterflyObj, butterflyObj->x_velocity, butterflyObj->y_velocity, butterflyObj->z_velocity);
        }
    }
    butterfly->unkFC = 1 - butterfly->unkFC;
    vertices = &butterfly->vertices[butterfly->unkFC * 6];
    var_v1 = sp64 == 0 ? 7 : 8;
    xPos = sins_s16((butterflyObj->animFrame << var_v1)) >> 10;
    yPos = coss_s16((butterflyObj->animFrame << var_v1)) >> 10;
    if (xPos < 0) {
        xPos = -xPos;
    }
    if ((!sp64) && (yPos < 0)) {
        yPos = -yPos;
    }
    vertices[0].x = -xPos;
    vertices[0].y = yPos;
    vertices[1].x = -xPos;
    vertices[1].y = yPos;
    vertices[4].x = xPos;
    vertices[4].y = yPos;
    vertices[5].x = xPos;
    vertices[5].y = yPos;
}

/**
 * Butterflies that were following this object lose their target.
 */
void butterfly_forget_target(Object *obj) {
    Object *other;
    Object_Butterfly *butterfly;
    s32 i;

    for (i = 0; i < gObjectCount; i++) {
        other = gObjPtrList[i];
        if (other->behaviorId == BHV_BUTTERFLY) {
            butterfly = other->butterfly;
            if (obj == butterfly->unk100) {
                butterfly->unk100 = 0;
                butterfly->unkFD = 1;
            }
        }
    }
}

void obj_render_butterfly(Object *obj) {
    Object_Butterfly *butterfly = obj->butterfly;
    render_misc_model(obj, &butterfly->vertices[butterfly->unkFC * 6], 6, butterfly->triangles, 8, butterfly->texture,
                      RENDER_Z_COMPARE | RENDER_FOG_ACTIVE, 0, 1.0f);
}
