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

// Butterfly states.
enum ButterflyState {
    BUTTERFLY_REST,          // Sitting at its home position.
    BUTTERFLY_RETURN_SEARCH, // Flying home, but looking for something to chase.
    BUTTERFLY_RETURN,        // Flying home.
    BUTTERFLY_CHASE,         // Flying towards its target.
    BUTTERFLY_CIRCLE         // Circling its target.
};

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
        butterfly->isAscending = 0;
        butterfly->targetObj = 0;
        butterfly->retargetTimer = 0;
        butterfly->state = BUTTERFLY_REST;
        butterfly->turnSpeed = 0x180;
        butterfly->forwardSpeed = 0.0f;
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
        butterfly->vertexBufferIndex = 1;
    }
    butterflyObj->trans.scale = butterflyEntry->scale * 0.01f;
    if (butterflyEntry->textureIndex < butterflyObj->header->numberOfModelIds) {
        butterfly->texture = butterflyObj->textures[butterflyEntry->textureIndex];
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

/**
 * Resting at home: waits until a racer, or failing that a butterfly node, comes within the home radius,
 * then chases it. Butterflies of the fast type hold their wings still while resting.
 */
static void butterfly_rest(Object *butterflyObj, Object_Butterfly *butterfly,
                           LevelObjectEntry_Butterfly *butterflyEntry, s32 isFastType, s32 updateRate) {
    f32 searchRadius;
    s16 racersFound;
    Object *nearbyRacers[8];

    butterfly->forwardSpeed = 0.0f;
    searchRadius = butterflyEntry->homeRadius;
    racersFound =
        obj_dist_racer(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, searchRadius, 1, nearbyRacers);
    if (racersFound > 0) {
        butterfly->targetObj = nearbyRacers[0];
        butterfly->retargetTimer = 0xF0;
        butterfly->state = BUTTERFLY_CHASE;
    } else {
        butterfly->targetObj =
            obj_butterfly_node(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, searchRadius, 1);
        if (butterfly->targetObj != 0) {
            butterfly->retargetTimer = 0xF0;
            butterfly->state = BUTTERFLY_CHASE;
        }
    }
    butterflyObj->animFrame = (butterflyObj->animFrame + updateRate) & 0xFF;
    if (isFastType) {
        butterflyObj->animFrame = 0x78;
    }
}

/**
 * Flying home: turns and climbs towards the home position. While searching, a racer or butterfly node in
 * the home radius sets it chasing again. Within 4 units of home it snaps there and rests.
 */
static void butterfly_return_home(Object *butterflyObj, Object_Butterfly *butterfly,
                                  LevelObjectEntry_Butterfly *butterflyEntry, s16 turnStep, f32 climbRate,
                                  s32 updateRate) {
    f32 xDiff;
    f32 yDiff;
    f32 zDiff;
    f32 heightDiff;
    f32 searchRadius;
    s16 angleDiff;
    s16 racersFound;
    Object *nearbyRacers[8];

    if (butterfly->turnSpeed < 0x480) {
        butterfly->turnSpeed += (updateRate * 0x10);
        if (butterfly->turnSpeed >= 0x481) {
            butterfly->turnSpeed = 0x480;
        }
    }
    xDiff = butterflyObj->trans.x_position - butterflyEntry->common.x;
    zDiff = butterflyObj->trans.z_position - butterflyEntry->common.z;
    angleDiff = arctan2_f(xDiff, zDiff);
    angleDiff -= butterflyObj->trans.rotation.y_rotation & 0xffff & 0xffff;
    if (angleDiff < 0) {
        if (angleDiff > -turnStep) {
            butterflyObj->trans.rotation.y_rotation += angleDiff;
        } else {
            butterflyObj->trans.rotation.y_rotation -= turnStep;
        }
    } else if ((angleDiff > 0)) {
        if (angleDiff < turnStep) {
            butterflyObj->trans.rotation.y_rotation += angleDiff;
        } else {
            butterflyObj->trans.rotation.y_rotation += turnStep;
        }
    }
    heightDiff = butterflyEntry->common.y - butterflyObj->trans.y_position;
    if (heightDiff < 0.0f) {
        butterflyObj->y_velocity -= climbRate * updateRate;
        if (butterflyObj->y_velocity < heightDiff) {
            butterflyObj->y_velocity = heightDiff;
        }
    } else if ((heightDiff > 0.0f)) {
        butterflyObj->y_velocity += climbRate * updateRate;
        if (heightDiff < butterflyObj->y_velocity) {
            butterflyObj->y_velocity = heightDiff;
        }
    } else {
        butterflyObj->y_velocity = 0.0f;
    }
    if (butterfly->state == BUTTERFLY_RETURN_SEARCH) {
        searchRadius = butterflyEntry->homeRadius;
        racersFound =
            obj_dist_racer(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, searchRadius, 1, nearbyRacers);
        if (racersFound > 0) {
            butterfly->targetObj = nearbyRacers[0];
            butterfly->retargetTimer = 0x78;
            butterfly->state = BUTTERFLY_CHASE;
        } else {
            butterfly->targetObj =
                obj_butterfly_node(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, searchRadius, 1);
            if (butterfly->targetObj != NULL) {
                butterfly->retargetTimer = 0xF0;
                butterfly->state = BUTTERFLY_CHASE;
            }
        }
    }
    if (butterfly->state != BUTTERFLY_CHASE) {
        yDiff = butterflyObj->trans.y_position - butterflyEntry->common.y;
        if (sqrtf((xDiff * xDiff) + (yDiff * yDiff) + (zDiff * zDiff)) < 4.0f) {
            ignore_bounds_check();
            move_object(butterflyObj, -xDiff, -yDiff, -zDiff);
            butterflyObj->y_velocity = 0.0f;
            butterfly->state = BUTTERFLY_REST;
            butterfly->forwardSpeed = 0.0f;
        }
    }
    butterflyObj->animFrame = (butterflyObj->animFrame + (updateRate * 0x10)) & 0xFF;
}

/**
 * Chasing a target: if the target strays over twice the home radius from home, looks for a new one.
 * Turns and climbs towards a point 16 units above the target, and starts circling it once within 16 units.
 * With no target left, flies home.
 */
static void butterfly_chase(Object *butterflyObj, Object_Butterfly *butterfly,
                            LevelObjectEntry_Butterfly *butterflyEntry, s16 turnStep, f32 climbRate, s32 updateRate) {
    f32 xDiff;
    f32 zDiff;
    f32 radius;
    f32 heightDiff;
    f32 searchRadius;
    s16 angleDiff;
    s16 racersFound;
    Object *nearbyRacers[8];

    if (butterfly->turnSpeed < 0x480) {
        butterfly->turnSpeed += (updateRate * 0x10);
        if (butterfly->turnSpeed >= 0x481) {
            butterfly->turnSpeed = 0x480;
        }
    }
    xDiff = butterfly->targetObj->trans.x_position - butterflyEntry->common.x;
    zDiff = butterfly->targetObj->trans.z_position - butterflyEntry->common.z;
    radius = (butterflyEntry->homeRadius * 2);
    radius *= radius;
    if (radius < ((xDiff * xDiff) + (zDiff * zDiff))) {
        butterfly->targetObj = 0;
        searchRadius = butterflyEntry->homeRadius;
        racersFound =
            obj_dist_racer(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, searchRadius, 1, nearbyRacers);
        if (racersFound > 0) {
            butterfly->targetObj = nearbyRacers[0];
            butterfly->retargetTimer = 0xF0;
        } else {
            butterfly->targetObj =
                obj_butterfly_node(butterflyEntry->common.x, 0.0f, butterflyEntry->common.z, searchRadius, 1);
            if (butterfly->targetObj != 0) {
                butterfly->retargetTimer = 0xF0;
            }
        }
    }
    if (butterfly->targetObj != 0) {
        xDiff = butterflyObj->trans.x_position - butterfly->targetObj->trans.x_position;
        zDiff = butterflyObj->trans.z_position - butterfly->targetObj->trans.z_position;
        angleDiff = arctan2_f(xDiff, zDiff);
        angleDiff -= butterflyObj->trans.rotation.y_rotation & 0xffff & 0xffff;
        if (angleDiff < 0) {
            if (-turnStep < angleDiff) {
                butterflyObj->trans.rotation.y_rotation += angleDiff;
            } else {
                butterflyObj->trans.rotation.y_rotation -= turnStep;
            }
        } else if ((angleDiff > 0)) {
            if (angleDiff < turnStep) {
                butterflyObj->trans.rotation.y_rotation += angleDiff;
            } else {
                butterflyObj->trans.rotation.y_rotation += turnStep;
            }
        }
        heightDiff = (butterfly->targetObj->trans.y_position + 16.0f) - butterflyObj->trans.y_position;
        if (heightDiff < 0.0f) {
            butterflyObj->y_velocity -= climbRate * updateRate;
            if (butterflyObj->y_velocity < heightDiff) {
                butterflyObj->y_velocity = heightDiff;
            }
        } else if ((heightDiff > 0.0f)) {
            butterflyObj->y_velocity += climbRate * updateRate;
            if (heightDiff < butterflyObj->y_velocity) {
                butterflyObj->y_velocity = heightDiff;
            }
        }
        if (((xDiff * xDiff) + (zDiff * zDiff)) < 256.0f) {
            butterfly->state = BUTTERFLY_CIRCLE;
        }
    } else {
        butterfly->state = BUTTERFLY_RETURN_SEARCH;
    }
    butterflyObj->animFrame = (butterflyObj->animFrame + (updateRate * 0x10)) & 0xFF;
}

/**
 * Circling a target: heads home once it strays over twice the home radius from home. Every 240 frames it picks a
 * random racer within 150 units to follow instead (going home if there are none). Otherwise it circles,
 * now and then reversing direction, and bobs between 8 and 64 units above the target. A target
 * more than 150 units away is chased again.
 */
static void butterfly_circle(Object *butterflyObj, Object_Butterfly *butterfly,
                             LevelObjectEntry_Butterfly *butterflyEntry, f32 climbRate, s32 updateRate) {
    f32 xDiff;
    f32 zDiff;
    f32 radius;
    s16 racersFound;
    Object *nearbyRacers[8];

    if (butterfly->turnSpeed >= 0x181) {
        butterfly->turnSpeed -= (updateRate * 0x10);
        if (butterfly->turnSpeed < 0x180) {
            butterfly->turnSpeed = 0x180;
        }
    }
    xDiff = butterflyObj->trans.x_position - butterflyEntry->common.x;
    zDiff = butterflyObj->trans.z_position - butterflyEntry->common.z;
    radius = (butterflyEntry->homeRadius * 2);
    radius *= radius;
    if (radius < ((xDiff * xDiff) + (zDiff * zDiff))) {
        butterfly->state = BUTTERFLY_RETURN;
    } else {
        if (updateRate < butterfly->retargetTimer) {
            butterfly->retargetTimer -= updateRate;
        } else {
            butterfly->retargetTimer = 0;
            racersFound = obj_dist_racer(butterflyObj->trans.x_position, 0.0f, butterflyObj->trans.z_position, 150.0f,
                                         1, nearbyRacers);
            if (racersFound == 0) {
                butterfly->targetObj = 0;
                butterfly->state = BUTTERFLY_RETURN_SEARCH;
            } else {
                if (racersFound >= 2) {
                    racersFound = rand_range(1, racersFound);
                }
                racersFound--;
                butterfly->targetObj = nearbyRacers[racersFound];
                butterfly->retargetTimer = 0xF0;
            }
        }
        if (butterfly->state == BUTTERFLY_CIRCLE) {
            xDiff = butterflyObj->trans.x_position - butterfly->targetObj->trans.x_position;
            zDiff = butterflyObj->trans.z_position - butterfly->targetObj->trans.z_position;
            if (((xDiff * xDiff) + (zDiff * zDiff)) > 22500.0f) {
                butterfly->state = BUTTERFLY_CHASE;
            } else {
                if (rand_range(0, 0x64) >= 0x63) {
                    butterfly->turnSpeed = -butterfly->turnSpeed;
                }
                butterflyObj->trans.rotation.y_rotation += updateRate * butterfly->turnSpeed;
                if ((butterfly->targetObj->trans.y_position + 64.0f) < butterflyObj->trans.y_position) {
                    butterfly->isAscending = 0;
                } else if (butterflyObj->trans.y_position < (butterfly->targetObj->trans.y_position + 8.0f)) {
                    butterfly->isAscending = 1;
                }
                if (butterfly->isAscending != 0) {
                    butterflyObj->y_velocity += (climbRate * updateRate);
                } else {
                    butterflyObj->y_velocity -= (climbRate * updateRate);
                }
                butterflyObj->trans.rotation.y_rotation += butterfly->turnSpeed;
            }
        }
    }
    butterflyObj->animFrame = (butterflyObj->animFrame + (updateRate * 0x10)) & 0xFF;
}

/**
 * Speeds up towards the top forward speed while flying, clamps the climb speed, then moves forwards.
 */
static void butterfly_move(Object *butterflyObj, Object_Butterfly *butterfly, s32 isFastType, f32 maxForwardSpeed,
                           s32 updateRate) {
    f32 maxClimbSpeed;

    if (butterfly->state != BUTTERFLY_REST) {
        if (butterfly->forwardSpeed < maxForwardSpeed) {
            butterfly->forwardSpeed += (0.25f * updateRate);
            if (maxForwardSpeed < butterfly->forwardSpeed) {
                butterfly->forwardSpeed = maxForwardSpeed;
            }
        }
    }
    if (butterfly->forwardSpeed != 0.0f) {
        if (butterfly->state != BUTTERFLY_CIRCLE) {
            maxClimbSpeed = 1.2f;
        } else {
            maxClimbSpeed = 1.6f;
        }
        if (isFastType) {
            maxClimbSpeed *= 1.6f;
        }
        if (butterflyObj->y_velocity < -maxClimbSpeed) {
            butterflyObj->y_velocity = -maxClimbSpeed;
        }
        if (maxClimbSpeed < butterflyObj->y_velocity) {
            butterflyObj->y_velocity = maxClimbSpeed;
        }
        if (butterfly->forwardSpeed != 0.0f) {
            butterflyObj->x_velocity = sins_f(butterflyObj->trans.rotation.y_rotation) * -butterfly->forwardSpeed;
            butterflyObj->z_velocity = coss_f(butterflyObj->trans.rotation.y_rotation) * -butterfly->forwardSpeed;
            move_object(butterflyObj, butterflyObj->x_velocity, butterflyObj->y_velocity, butterflyObj->z_velocity);
        }
    }
}

/**
 * Flaps the wings: swaps to the other vertex buffer and sets the wing tip vertices from the animation frame.
 * The fast type flaps twice as fast and its wings can dip below the body.
 */
static void butterfly_flap_wings(Object *butterflyObj, Object_Butterfly *butterfly, s32 isFastType) {
    s16 wingShift;
    s32 wingX;
    s32 wingY;
    Vertex *vertices;

    butterfly->vertexBufferIndex = 1 - butterfly->vertexBufferIndex;
    vertices = &butterfly->vertices[butterfly->vertexBufferIndex * 6];
    wingShift = isFastType == 0 ? 7 : 8;
    wingX = sins_s16((butterflyObj->animFrame << wingShift)) >> 10;
    wingY = coss_s16((butterflyObj->animFrame << wingShift)) >> 10;
    if (wingX < 0) {
        wingX = -wingX;
    }
    if ((!isFastType) && (wingY < 0)) {
        wingY = -wingY;
    }
    vertices[0].x = -wingX;
    vertices[0].y = wingY;
    vertices[1].x = -wingX;
    vertices[1].y = wingY;
    vertices[4].x = wingX;
    vertices[4].y = wingY;
    vertices[5].x = wingX;
    vertices[5].y = wingY;
}

void obj_loop_butterfly(Object *butterflyObj, s32 updateRate) {
    f32 climbRate;
    f32 maxForwardSpeed;
    s16 turnStep;
    s32 isFastType;
    Object_Butterfly *butterfly;
    LevelObjectEntry_Butterfly *butterflyEntry;

    butterflyEntry = &butterflyObj->level_entry->butterfly;
    butterfly = butterflyObj->butterfly;
    // Butterflies using the third texture fly, climb and flap faster.
    isFastType = butterflyEntry->textureIndex == 2;
    turnStep = butterfly->turnSpeed;
    if (turnStep < 0) {
        turnStep = -turnStep;
    }
    climbRate = 0.1f;
    turnStep *= updateRate;
    if (isFastType) {
        climbRate *= 1.6f;
    }
    maxForwardSpeed = 3.0f;
    if (isFastType) {
        maxForwardSpeed *= 1.6f;
    }
    switch (butterfly->state) {
        case BUTTERFLY_REST:
            butterfly_rest(butterflyObj, butterfly, butterflyEntry, isFastType, updateRate);
            break;
        case BUTTERFLY_RETURN_SEARCH:
        case BUTTERFLY_RETURN:
            butterfly_return_home(butterflyObj, butterfly, butterflyEntry, turnStep, climbRate, updateRate);
            break;
        case BUTTERFLY_CHASE:
            butterfly_chase(butterflyObj, butterfly, butterflyEntry, turnStep, climbRate, updateRate);
            break;
        case BUTTERFLY_CIRCLE:
            butterfly_circle(butterflyObj, butterfly, butterflyEntry, climbRate, updateRate);
            break;
    }
    butterfly_move(butterflyObj, butterfly, isFastType, maxForwardSpeed, updateRate);
    butterfly_flap_wings(butterflyObj, butterfly, isFastType);
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
            if (obj == butterfly->targetObj) {
                butterfly->targetObj = 0;
                butterfly->state = BUTTERFLY_RETURN_SEARCH;
            }
        }
    }
}

void obj_render_butterfly(Object *obj) {
    Object_Butterfly *butterfly = obj->butterfly;
    render_misc_model(obj, &butterfly->vertices[butterfly->vertexBufferIndex * 6], 6, butterfly->triangles, 8,
                      butterfly->texture, RENDER_Z_COMPARE | RENDER_FOG_ACTIVE, 0, 1.0f);
}
