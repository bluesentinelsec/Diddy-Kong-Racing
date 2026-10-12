#ifndef DKR_TRACE_H
#define DKR_TRACE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Golden-frame tracing (`--trace FILE`, or DKR_TRACE=FILE in the environment).
 *
 * While a trace is active the game runs deterministically - a frame-counted
 * clock instead of the wall clock, no controller input, an erased save image,
 * one audio tick per video frame - and every frame writes one line:
 *
 *     frame <n> gfx <hash> audio <hash>
 *
 * `gfx` hashes everything the renderer is handed for that frame: every state
 * change and every vertex, with textures represented by a hash of their pixels
 * rather than by handle. `audio` hashes the PCM the mixer produced. Neither
 * depends on the GPU or the audio device, so two builds whose game logic agrees
 * produce identical files whatever the host - which is what tests/golden.sh
 * checks against the committed trace. The process exits once the requested
 * number of frames has been written.
 */

// Command-line / environment configuration; call before pc_trace_init().
void pc_trace_configure(const char *path, long frames);
void pc_trace_init(void);

int pc_trace_active(void);

// Feed data into the running hashes for the current frame.
void pc_trace_gfx(const void *data, size_t size);
void pc_trace_gfx_u32(uint32_t value);
void pc_trace_gfx_f32(float value);
void pc_trace_audio(const void *data, size_t size);

// With DKR_TRACE_DETAIL=1, logs the running hashes after an event, to find the first
// divergence inside a frame.
void pc_trace_mark(const char *what);

// Texture pixels are hashed once at creation and recalled by handle on bind.
void pc_trace_texture_create(unsigned int handle, const void *rgba, int width, int height, int cmS, int cmT);
void pc_trace_texture_bind(unsigned int handle);

// Called by the renderer after presenting each frame.
void pc_trace_frame_end(void);

// The N64 Count register as the trace sees it (46.875 MHz, advancing per frame).
uint32_t pc_trace_count(void);

// Frames completed so far (the number the next "frame" line will carry, minus one).
long pc_trace_frame(void);

// DKR_TRACE_PCM=<frame>: append that frame's mixed PCM to dkr_pcm_<frame>.raw.
void pc_trace_pcm(const void *data, size_t size);

#endif
