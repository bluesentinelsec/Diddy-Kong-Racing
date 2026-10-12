// Golden-frame tracing. See trace.h for what the trace contains and why.

#include "trace.h"

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
static int sDetail; // DKR_TRACE_DETAIL=1: also log the running hash after every renderer event

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
    fprintf(sFile, "# dkracing trace v1 frames=%ld\n", sFrames);
    printf("TRACE: writing %ld frames to %s\n", sFrames, sPath);
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

uint32_t pc_trace_count(void) {
    // 46,875,000 ticks per second at 60 frames per second.
    return (uint32_t) ((uint64_t) sFrame * 781250u);
}
