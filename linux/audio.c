// Host audio output: SDL3 audio stream.
//
// The game's audio manager (src/audiomgr.c, TARGET_PC path) mixes one video
// frame of stereo s16 PCM at a time and hands it to the "AI" through the shims
// below. SDL3's push-model audio stream replaces the ring buffer of the SDL2
// backend: we push each buffer, ask how much is still queued, and pace the
// manager against that.

#include <SDL3/SDL.h>
#include "trace.h"
#include <stdio.h>
#include <string.h>

typedef signed int s32;
typedef unsigned int u32;

// The audio manager's per-frame tick (src/audiomgr.c, TARGET_PC path).
extern void am_audio_frame_pc(void);

// src/audiomgr.c: samples of audio the manager wants to produce per tick. Derived
// from outputRate * 2 / refreshRate = 735 at NTSC — i.e. 1/30s of audio, because
// the audio thread ticks once per game frame (two video fields), not per field.
extern unsigned int frameSize;

#define PC_AUDIO_CHANNELS 2
#define PC_AUDIO_BYTES_PER_SAMPLE 4 // stereo s16

static SDL_AudioStream *sStream;
static s32 sAudioReady;
static s32 sAudioRate;
static u32 sUnderruns;

// ---------------------------------------------------------------------------
// AI shims (replacing libultra/src/io/ai.c, aigetlen.c, aisetfreq.c,
// aisetnextbuf.c — all dropped from the PC build, they only poke MMIO)
// ---------------------------------------------------------------------------

// Called once from amCreateAudioMgr with OUTPUT_RATE. Returns the rate actually
// achieved; the manager derives its whole frame-size schedule from the value we
// return here, so it must be the truth.
s32 osAiSetFrequency(u32 frequency) {
    SDL_AudioSpec spec;

    if (sAudioReady) {
        return sAudioRate;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        fprintf(stderr, "AUDIO: SDL_InitSubSystem failed: %s — running silent\n", SDL_GetError());
        return (s32) frequency;
    }

    memset(&spec, 0, sizeof(spec));
    spec.format = SDL_AUDIO_S16;
    spec.channels = PC_AUDIO_CHANNELS;
    spec.freq = (int) frequency;

    // The stream converts from our spec to whatever the device wants.
    sStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (sStream == NULL) {
        fprintf(stderr, "AUDIO: SDL_OpenAudioDeviceStream failed: %s — running silent\n", SDL_GetError());
        return (s32) frequency;
    }
    SDL_ResumeAudioStreamDevice(sStream);

    sAudioReady = 1;
    sAudioRate = (s32) frequency;
    printf("AUDIO: %d Hz, %d ch (SDL3 stream)\n", spec.freq, spec.channels);
    return sAudioRate;
}

// The game hands us the PCM the mixer produced for the *previous* frame.
void osAiSetNextBuffer(void *buf, u32 size) {
    if (buf != NULL && size != 0) {
        pc_trace_audio(buf, size);
        pc_trace_mark("audio");
    }
    if (!sAudioReady || buf == NULL || size == 0) {
        return;
    }
    if (!SDL_PutAudioStreamData(sStream, buf, (int) size)) {
        fprintf(stderr, "AUDIO: SDL_PutAudioStreamData failed: %s\n", SDL_GetError());
    }
}

// Bytes queued and not yet played, in our (input) format.
static u32 pc_audio_queued(void) {
    int queued;

    if (!sAudioReady) {
        return 0;
    }
    queued = SDL_GetAudioStreamQueued(sStream);
    if (queued == 0) {
        sUnderruns++;
    }
    return queued < 0 ? 0 : (u32) queued;
}

// Bytes still to play. This is the feedback signal __amHandleFrameMsg uses to size
// the next chunk:
//
//     frameSamples = (16 + (frameSize - osAiGetLength()/4 + 96)) & ~0xf
//
// On N64 the AI holds at most two buffers — one playing, one pending — and
// osAiGetLength() returns what is left of the *playing* one, so it is never more
// than a single frame. Our queue is deeper than that, and reporting the whole
// backlog makes (frameSize - samplesLeft) go negative, which the game's unsigned
// clamp lets through as a huge length. So: saturate at one frame, exactly like the
// hardware. Total latency is governed by the pump's target below, not by this.
u32 osAiGetLength(void) {
    u32 used = pc_audio_queued();
    u32 oneFrame;

    if (pc_trace_active()) {
        return 0; // the mixer then produces the same sample count every tick
    }
    if (frameSize == 0) {
        return used; // before amCreateAudioMgr has run
    }
    oneFrame = frameSize * PC_AUDIO_BYTES_PER_SAMPLE;
    return (used > oneFrame) ? oneFrame : used;
}

// ---------------------------------------------------------------------------
// Frame pump
// ---------------------------------------------------------------------------

// Called once per video frame from linux/main.c, standing in for the audio
// thread's OS_SC_RETRACE_MSG wakeup — but NOT one tick per call.
//
// The manager produces 1/30s of audio per tick, so it needs ticking at 30Hz. The
// host frame rate is whatever the host frame rate is (60, 144, hitching, vsync
// off), so pace against the queue instead of against the frame: top it up to a
// target depth and stop. This is self-correcting and independent of host fps.
#define PC_AUDIO_TARGET_FRAMES 3 // ~100ms of buffered audio
#define PC_AUDIO_MAX_TICKS 4     // don't spin forever if something goes wrong

void pc_audio_frame(void) {
    u32 target;
    s32 ticks = 0;

    if (frameSize == 0) {
        return;
    }
    if (pc_trace_active()) {
        am_audio_frame_pc(); // exactly one tick per frame, device or no device
        return;
    }
    if (!sAudioReady) {
        return;
    }
    target = frameSize * PC_AUDIO_BYTES_PER_SAMPLE * PC_AUDIO_TARGET_FRAMES;

    while (pc_audio_queued() < target && ticks < PC_AUDIO_MAX_TICKS) {
        am_audio_frame_pc();
        ticks++;
    }
}

// Diagnostics: an empty queue when the pump looks means the DAC ran dry.
void pc_audio_report(void) {
    static u32 lastUnderruns;

    if (!sAudioReady) {
        return;
    }
    if (sUnderruns != lastUnderruns) {
        printf("AUDIO: %u underruns\n", sUnderruns);
        lastUnderruns = sUnderruns;
    }
}
