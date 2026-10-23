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
 * Audio object init function.
 * Sets up a sound point for the audio, then frees the audio object.
 */
void obj_init_audio(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_Audio *entry = &levelEntry->audio;
    Object_Audio *audio;

    audio = obj->audio;
    audio->soundId = entry->soundId;
    audio->range = entry->range;
    audio->fastFalloff = entry->fastFalloff;
    audio->pitch = entry->pitch;
    audio->volume = entry->volume;
    audio->minVolume = entry->minVolume;
    audio->priority = entry->priority;
    audio->soundMask = NULL;
    if (sound_is_looped(audio->soundId)) {
        audspat_point_create(audio->soundId, entry->common.x, entry->common.y, entry->common.z, 9, audio->minVolume,
                             audio->volume, audio->range, audio->fastFalloff, audio->pitch, audio->priority,
                             &audio->soundMask);
    } else {
        audspat_point_create(audio->soundId, entry->common.x, entry->common.y, entry->common.z, 10, audio->minVolume,
                             audio->volume, audio->range, audio->fastFalloff, audio->pitch, audio->priority,
                             &audio->soundMask);
    }
    free_object(obj);
}

/**
 * Audio Line init function.
 * Sets up a polyline in the audio spatial system, then frees the audio object.
 * Official name: audioLineInit
 */
void obj_init_audioline(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_AudioLine *entry = &levelEntry->audioLine;
    Object_AudioLine *audLine;

    audLine = obj->audio_line;
    audLine->unk0 = entry->unk8;
    audLine->soundID = entry->soundID;
    audLine->lineID = entry->lineID;
    audLine->unkD = entry->unkD;
    audLine->unk_union.unk8_word = 0;
    audLine->unk4 = entry->unkE;
    audLine->unk11 = entry->unk12;
    audLine->unk10 = entry->unk11;
    audLine->unkE = entry->unk9;
    audLine->unkF = entry->unk10;
    audLine->unk12 = entry->unk13;
    audspat_line_add_vertex(audLine->unk0, audLine->soundID, entry->common.x, entry->common.y, entry->common.z,
                            audLine->unkF, audLine->unkE, audLine->unk10, audLine->unk12, audLine->unk4, audLine->unk11,
                            audLine->lineID, audLine->unkD);
    free_object(obj);
}

/**
 * Audio Reverb init function.
 * Sets up a reverb point in the audio spatial system, then frees the audio object.
 */
void obj_init_audioreverb(Object *obj, LevelObjectEntry *levelEntry, UNUSED s32 param) {
    LevelObjectEntry_AudioReverb *entry = &levelEntry->audioReverb;
    s32 lineID;
    Object_AudioReverb *reverb = obj->audio_reverb;
    reverb->magnitude = entry->magnitude;
    lineID = entry->lineID;
    reverb->lineID = lineID & 0xFF;
    reverb->vertexIndex = entry->vertexIndex;
    audspat_reverb_add_vertex(entry->common.x, entry->common.y, entry->common.z, reverb->magnitude, reverb->lineID,
                              reverb->vertexIndex);
    free_object(obj);
}
