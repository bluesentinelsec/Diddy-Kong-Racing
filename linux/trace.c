// Golden-frame tracing. See trace.h for what the trace contains and why.

#include "trace.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET 0xcbf29ce484222325ull
#define FNV_PRIME 0x100000001b3ull

static const char *sPath;
static long sFrames = 1800;
static FILE *sFile;
static int sActive;
static uint64_t sGfxHash = FNV_OFFSET;
static uint64_t sAudioHash = FNV_OFFSET;
static uint64_t sGfxTotal = FNV_OFFSET;
static uint64_t sAudioTotal = FNV_OFFSET;
static long sFrame;
static int sDetail;
static uint32_t sCountReads; // osGetCount reads this frame // DKR_TRACE_DETAIL=1: also log the running hash after every
                             // renderer event

// Texture content hashes by GL handle. Handles are small integers; the table
// grows to fit the largest one seen.
static uint64_t *sTexHash;
static size_t sTexHashCount;

static uint64_t fnv(uint64_t h, const void *data, size_t size) {
    const unsigned char *p = data;
    size_t i;

    for (i = 0; i < size; i++) {
        h ^= p[i];
        h *= FNV_PRIME;
    }
    return h;
}

void pc_trace_configure(const char *path, long frames) {
    if (path != NULL) {
        sPath = path;
    }
    if (frames > 0) {
        sFrames = frames;
    }
}

void pc_trace_init(void) {
    const char *env = getenv("DKR_TRACE");
    const char *frames = getenv("DKR_TRACE_FRAMES");

    if (sPath == NULL && env != NULL && env[0] != '\0') {
        sPath = env;
    }
    if (frames != NULL && atol(frames) > 0) {
        sFrames = atol(frames);
    }
    if (sPath == NULL) {
        return;
    }
    sFile = fopen(sPath, "w");
    if (sFile == NULL) {
        fprintf(stderr, "TRACE: cannot write %s\n", sPath);
        exit(1);
    }
    sActive = 1;
    sDetail = getenv("DKR_TRACE_DETAIL") != NULL && atoi(getenv("DKR_TRACE_DETAIL")) != 0;
    if (sDetail) {
        setvbuf(sFile, NULL, _IOLBF, 0); // a hung run still leaves its last events on disk
    }
    fprintf(sFile, "# dkracing trace v1 frames=%ld\n", sFrames);
    printf("TRACE: writing %ld frames to %s\n", sFrames, sPath);
}

// DKR_TRACE_LEVEL / DKR_TRACE_PLAYERS: a scene that starts in a level the menus
// cannot reach by script (a boss race, a hub world). The request fires once the
// title menu has been running for a while, so the boot sequence is untouched.
#define TRACE_LEVEL_FRAME 120

int pc_trace_level_request(unsigned int frame) {
    static int sDone = 0;
    const char *env = getenv("DKR_TRACE_LEVEL");

    if (sDone || env == NULL || env[0] == '\0' || frame < TRACE_LEVEL_FRAME) {
        return -1;
    }
    sDone = 1;
    return atoi(env);
}

int pc_trace_players(void) {
    const char *env = getenv("DKR_TRACE_PLAYERS");
    int n = env != NULL ? atoi(env) : 1;

    return n < 1 ? 1 : (n > 4 ? 4 : n);
}

// "# adventure" in a scene script (DKR_TRACE_ADVENTURE): the level request starts the level in
// adventure mode instead of tracks mode, so a race ends the way it does from a hub door.
int pc_trace_adventure(void) {
    const char *env = getenv("DKR_TRACE_ADVENTURE");

    return env != NULL && env[0] != '\0' && env[0] != '0';
}

int pc_trace_active(void) {
    return sActive;
}

void pc_trace_gfx(const void *data, size_t size) {
    if (sActive) {
        sGfxHash = fnv(sGfxHash, data, size);
    }
}

void pc_trace_gfx_u32(uint32_t value) {
    pc_trace_gfx(&value, sizeof(value));
}

void pc_trace_mark(const char *what) {
    if (sActive && sDetail) {
        fprintf(sFile, "  %s %016llx %016llx\n", what, (unsigned long long) sGfxHash, (unsigned long long) sAudioHash);
    }
}

void pc_trace_gfx_f32(float value) {
    pc_trace_gfx(&value, sizeof(value));
}

void pc_trace_audio(const void *data, size_t size) {
    if (sActive) {
        sAudioHash = fnv(sAudioHash, data, size);
    }
}

void pc_trace_texture_create(unsigned int handle, const void *rgba, int width, int height, int cmS, int cmT) {
    uint64_t h;
    int dims[4];

    if (!sActive || handle == 0) {
        return;
    }
    dims[0] = width;
    dims[1] = height;
    dims[2] = cmS;
    dims[3] = cmT;
    h = fnv(FNV_OFFSET, dims, sizeof(dims));
    h = fnv(h, rgba, (size_t) width * (size_t) height * 4);
    if (handle >= sTexHashCount) {
        size_t n = sTexHashCount ? sTexHashCount : 256;
        uint64_t *grown;

        while (n <= handle) {
            n *= 2;
        }
        grown = realloc(sTexHash, n * sizeof(*grown));
        if (grown == NULL) {
            return;
        }
        memset(grown + sTexHashCount, 0, (n - sTexHashCount) * sizeof(*grown));
        sTexHash = grown;
        sTexHashCount = n;
    }
    sTexHash[handle] = h;
}

void pc_trace_texture_bind(unsigned int handle) {
    uint64_t h = 0;

    if (!sActive) {
        return;
    }
    if (handle != 0 && handle < sTexHashCount) {
        h = sTexHash[handle];
    }
    pc_trace_gfx(&h, sizeof(h));
}

void pc_trace_frame_end(void) {
    if (!sActive) {
        return;
    }
    sFrame++;
    fprintf(sFile, "frame %ld gfx %016llx audio %016llx\n", sFrame, (unsigned long long) sGfxHash,
            (unsigned long long) sAudioHash);
    sGfxTotal = fnv(sGfxTotal, &sGfxHash, sizeof(sGfxHash));
    sAudioTotal = fnv(sAudioTotal, &sAudioHash, sizeof(sAudioHash));
    sGfxHash = FNV_OFFSET;
    sAudioHash = FNV_OFFSET;
    sCountReads = 0;
    if (sFrame >= sFrames) {
        fprintf(sFile, "end frames=%ld gfx=%016llx audio=%016llx\n", sFrame, (unsigned long long) sGfxTotal,
                (unsigned long long) sAudioTotal);
        fclose(sFile);
        printf("TRACE: %ld frames written to %s\n", sFrame, sPath);
        exit(0);
    }
}

long pc_trace_frame(void) {
    return sFrame;
}

void pc_trace_pcm(const void *data, size_t size) {
    const char *want = getenv("DKR_TRACE_PCM");
    char name[64];
    FILE *f;

    if (!sActive || want == NULL || atol(want) != sFrame + 1) {
        return;
    }
    snprintf(name, sizeof(name), "dkr_pcm_%ld.raw", sFrame + 1);
    f = fopen(name, "ab");
    if (f != NULL) {
        fwrite(data, 1, size, f);
        fclose(f);
    }
}

// 46,875,000 ticks per second; a traced frame is one 30 Hz logic step (two
// retrace periods), and every read within a frame advances a little so that no
// two reads agree, as on the real Count register.
uint32_t pc_trace_count(void) {
    sCountReads++;
    return (uint32_t) ((uint64_t) sFrame * 1562500u + (uint64_t) sCountReads * 128u);
}

// Ad-hoc diagnostics from game-side TARGET_PC regions (no host headers there).
int pc_trace_debug(const char *env) {
    return sActive && getenv(env) != NULL;
}

void pc_trace_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void pc_trace_event(const char *fmt, ...) {
    va_list args;

    if (!sActive || !sDetail) {
        return;
    }
    fputs("  ev ", sFile);
    va_start(args, fmt);
    vfprintf(sFile, fmt, args);
    va_end(args);
    fputc('\n', sFile);
}
