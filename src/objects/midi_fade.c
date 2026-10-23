#include "objects_internal.h"

/**
 * Fade channels by the player's distance from a midi fade point: full inside the inner
 * radius, gone at the outer one. Only acts while its sequence is playing.
 */
static void midi_fade_point_apply(Object *racerObj, Object *obj) {
    Object_MidiFadePoint *point;
    f32 distance;
    f32 range;
    s32 fade;
    s32 i;

    distance = sqrtf(
        ((racerObj->trans.x_position - obj->trans.x_position) * (racerObj->trans.x_position - obj->trans.x_position)) +
        ((racerObj->trans.y_position - obj->trans.y_position) * (racerObj->trans.y_position - obj->trans.y_position)) +
        ((racerObj->trans.z_position - obj->trans.z_position) * (racerObj->trans.z_position - obj->trans.z_position)));
    point = obj->midi_fade_point;
    if (distance >= point->outerRadius || point->sequenceID != music_current_sequence()) {
        return;
    }
    if (distance <= point->innerRadius) {
        fade = 0;
    } else {
        distance -= point->innerRadius;
        range = (point->outerRadius - point->innerRadius);
        fade = (127.0f * distance) / range;
    }
    for (i = 0; i < 16; i++) {
        switch (point->channelModes[i]) {
            case 1:
                if (fade >= 0x7B) {
                    music_channel_off(i);
                } else {
                    music_channel_fade_set(i, (0x7F - fade));
                    music_channel_on(i);
                }
                break;
            case 2:
                if ((music_channel_fade(i) > 0) && (music_channel_active(i) == 0)) {
                    music_channel_fade_set(i, fade);
                }
                break;
            default:
                break;
        }
    }
}

/**
 * Start a midi fade when the player's movement this frame crosses its plane inside its
 * box. The crossing direction picks which channel modes apply.
 */
static void midi_fade_check_crossing(Object *racerObj, Object *obj, f32 xPos, f32 yPos, f32 zPos) {
    Object_MidiFade *midiFade;
    f32 prevSide;
    f32 currSide;
    f32 moveX;
    f32 moveY;
    f32 moveZ;
    f32 t;
    f32 hitX;
    s8 direction;

    midiFade = obj->midi_fade;
    prevSide =
        (midiFade->planeX * xPos) + (midiFade->planeY * yPos) + (midiFade->planeZ * zPos) + midiFade->planeOffset;
    currSide = (midiFade->planeX * racerObj->trans.x_position) + (midiFade->planeY * racerObj->trans.y_position) +
               (midiFade->planeZ * racerObj->trans.z_position) + midiFade->planeOffset;
    if (prevSide > 0.0f && currSide <= 0.0f) {
        direction = 1;
    } else if (currSide > 0.0f && prevSide <= 0.0f) {
        direction = -1;
    } else {
        return;
    }
    moveX = racerObj->trans.x_position - xPos;
    moveY = racerObj->trans.y_position - yPos;
    // Where along the movement the plane was crossed.
    t = (-midiFade->planeX * xPos - midiFade->planeY * yPos - midiFade->planeZ * zPos - midiFade->planeOffset) /
        (midiFade->planeX * moveX + midiFade->planeY * moveY + midiFade->planeZ * (racerObj->trans.z_position - zPos));
    hitX = t * moveX;
    if (!((midiFade->minX <= hitX + xPos) && (hitX + xPos <= midiFade->maxX))) {
        return;
    }
    moveZ = racerObj->trans.z_position - zPos;
    if (!((midiFade->minY <= (t * moveY) + yPos) && ((t * moveY) + yPos <= midiFade->maxY))) {
        return;
    }
    if (!((midiFade->minZ <= (t * (moveZ)) + zPos) && ((t * (moveZ)) + zPos <= midiFade->maxZ))) {
        return;
    }
    midiFade->direction = direction;
    midiFade->progress = 0;
    midiFade->timer = 0;
    gCurrentMidiFade = midiFade;
}

/**
 * Switch the music's channel mask when the player comes within range of a channel set.
 */
static void midi_channel_set_apply(Object *racerObj, Object *obj) {
    Object_MidiChannelSet *channelSet;
    f32 distance;
    u8 radius;

    channelSet = obj->midi_channel_set;
    distance = sqrtf(
        ((racerObj->trans.x_position - obj->trans.x_position) * (racerObj->trans.x_position - obj->trans.x_position)) +
        ((racerObj->trans.y_position - obj->trans.y_position) * (racerObj->trans.y_position - obj->trans.y_position)) +
        ((racerObj->trans.z_position - obj->trans.z_position) * (racerObj->trans.z_position - obj->trans.z_position)));
    radius = channelSet->radius;
    if ((distance < (radius * 4)) && (channelSet->channelMask != music_channel_get_mask()) &&
        (channelSet->sequenceID == music_current_sequence())) {
        music_dynamic_set(channelSet->channelMask);
    }
}

/**
 * Step the running midi fade: progress runs 0..254 over its duration, and each channel
 * follows its mode for the crossing direction. The fade is dropped once complete.
 */
static void midi_fade_advance(s32 updateRate) {
    Object_MidiFade *midiFade;
    u16 durationFrames;
    s32 progress;
    s32 level;
    s8 mode;
    s32 i;

    midiFade = gCurrentMidiFade;
    if (midiFade->sequenceID != music_current_sequence()) {
        return;
    }
    midiFade->timer += updateRate;
    durationFrames = midiFade->duration * gVideoRefreshRate;
    if (durationFrames < midiFade->timer) {
        midiFade->timer = durationFrames;
    }
    progress = (midiFade->timer * 254.0f) / durationFrames;
    if (progress < 0xFE) {
        midiFade->progress = progress;
    } else {
        midiFade->progress = 0xFE;
    }
    for (i = 0; i < 16; i++) {
        mode = midiFade->channelModes[i];
        if (midiFade->direction == -1) {
            mode >>= 2;
        }
        mode &= 3;
        switch (mode) {
            case 1:
                music_channel_on(i);
                music_channel_fade_set(i, 0x7FU);
                break;
            case 0:
                music_channel_off(i);
                break;
            case 3:
                if (midiFade->progress >= 0x80) {
                    level = (midiFade->progress - 0x7F) & 0xFF;
                    music_channel_on(i);
                    if (music_channel_fade(i) < level) {
                        music_channel_fade_set(i, level);
                    }
                }
                break;
            case 2:
                if (midiFade->progress < 0x7F) {
                    level = (0x7F - midiFade->progress) & 0xFF;
                    if (level < music_channel_fade(i)) {
                        music_channel_fade_set(i, level);
                    }
                } else {
                    music_channel_off(i);
                }
                break;
            default:
                break;
        }
    }
    if (midiFade->progress == 0xFE) {
        gCurrentMidiFade = 0;
    }
}

/**
 * Fades music channels by player one's position relative to the level's midi fade
 * objects. (xPos, yPos, zPos) is where the racer was last frame.
 */
void update_midi_fade_points(Object *racerObj, f32 xPos, f32 yPos, f32 zPos, s32 updateRate) {
    Object *obj;
    s32 i;

    if (racerObj->racer->playerIndex != 0) {
        return;
    }
    if (cam_get_viewport_layout() != 0) {
        return;
    }
    for (i = gObjectListStart; i < gObjectCount; i++) {
        obj = gObjPtrList[i];
        if (obj->trans.flags & OBJ_FLAGS_PARTICLE) {
            continue;
        }
        if (obj->behaviorId == BHV_MIDI_FADE_POINT) {
            midi_fade_point_apply(racerObj, obj);
        } else if (obj->behaviorId == BHV_MIDI_FADE) {
            midi_fade_check_crossing(racerObj, obj, xPos, yPos, zPos);
        } else if (obj->behaviorId == BHV_MIDI_CHANNEL_SET) {
            midi_channel_set_apply(racerObj, obj);
        }
    }
    if (gCurrentMidiFade != 0) {
        midi_fade_advance(updateRate);
    }
}
