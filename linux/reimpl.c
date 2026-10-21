// OS reimplementation stubs for the PC build.

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "input.h"
#include "trace.h"
#include <SDL3/SDL.h>

typedef signed char s8;
typedef signed int s32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// ---------------------------------------------------------------------------
// RSP microcode blobs (task-submission code references these; replaced by the
// HLE renderer / audio interpreter later)
// ---------------------------------------------------------------------------
u64 rspbootTextStart[1] = { 0 };
u64 rspbootTextEnd[1] = { 0 };
u64 aspMainTextStart[1] = { 0 };
u64 aspMainDataStart[1] = { 0 };
u64 rspF3DDKRXbusStart[1] = { 0 };
u64 rspF3DDKRDataXbusStart[1] = { 0 };
u64 rspF3DDKRFifoStart[1] = { 0 };
u64 rspF3DDKRDataFifoStart[1] = { 0 };

// ---------------------------------------------------------------------------
// Linker-script symbols (placed by the N64 linker script; static here)
// ---------------------------------------------------------------------------
// Becomes the real asset LUT/data location once the PC asset loader exists.
#include "pc_rom.h"
u8 *__ROM_END; // cheat-menu checksum bound
u8 *main_BSS_START[1] = { 0 };

// N64: the main heap is [end of BSS .. RAM_END], carved out by the linker.
// PC: a static pool. memory.c's "ramEnd - (s32)&gMainMemoryPool" sizing math
// still needs TARGET_PC surgery to use this pool's real size instead.
// Storage lives in memory.c, which has MemoryPoolSlot in scope; its size
// comes from -DPC_MAIN_POOL_BYTES in the platform makefile.

// ---------------------------------------------------------------------------
// Asset "DMA" — the DKR equivalent of the OoT port's DmaMgr_DmaRomToRam.
// On N64 the asset LUT and asset data sit in cart ROM right after the code,
// bracketed by __ASSETS_LUT_START/__ASSETS_LUT_END. On PC the same bytes live
// in the files the N64 build already produces (assets/assets.lut.bin and
// assets/assets.bin); dmacopy addresses are translated back to file offsets
// relative to the two stub symbols above.
// The LUT is an array of big-endian u32s — byteswapped once at load. Asset
// *contents* are left big-endian; each parse site gets fixed as it comes up.
// ---------------------------------------------------------------------------
static u8 *sAssetLut = NULL;
static u32 sAssetLutSize = 0;
static u8 *sAssetsBin = NULL;
static u32 sAssetsBinSize = 0;

// Opens `path` relative to the working directory, or failing that relative to
// the directory holding the executable, so a build can be launched by double-
// clicking it with assets/ sitting next to it.
#ifndef __ANDROID__
static FILE *pc_open_asset(const char *path) {
    FILE *f = fopen(path, "rb");
    const char *base;
    char full[2048];

    if (f != NULL) {
        return f;
    }
    base = SDL_GetBasePath();
    if (base == NULL) {
        return NULL;
    }
    snprintf(full, sizeof(full), "%s%s", base, path);
    return fopen(full, "rb");
}
#endif

#ifdef DKR_EMBED_ASSETS
// The two files, embedded by linux/assets_embed.S. Copied out because the LUT
// is byte-swapped in place and the executable's data is read-only.
extern const u8 dkr_embedded_assets_lut[], dkr_embedded_assets_lut_end[];
extern const u8 dkr_embedded_assets_bin[], dkr_embedded_assets_bin_end[];

static u8 *pc_embedded_file(const char *path, u32 *sizeOut) {
    const u8 *start, *end;
    u8 *buf;

    if (strcmp(path, "assets/assets.lut.bin") == 0) {
        start = dkr_embedded_assets_lut;
        end = dkr_embedded_assets_lut_end;
    } else if (strcmp(path, "assets/assets.bin") == 0) {
        start = dkr_embedded_assets_bin;
        end = dkr_embedded_assets_bin_end;
    } else {
        return NULL;
    }
    *sizeOut = (u32) (end - start);
    buf = malloc(*sizeOut);
    memcpy(buf, start, *sizeOut);
    return buf;
}
#endif

static u8 *pc_load_file(const char *path, u32 *sizeOut) {
#ifdef DKR_EMBED_ASSETS
    u8 *embedded = pc_embedded_file(path, sizeOut);

    if (embedded != NULL) {
        return embedded;
    }
#endif
#ifdef __ANDROID__
    // Assets live inside the APK, which is not a real directory, so fopen cannot
    // reach them. SDL's IO maps a relative path onto the Android asset manager.
    // These buffers are loaded once and kept for the process's lifetime.
    size_t sz = 0;
    void *data = SDL_LoadFile(path, &sz);

    if (data == NULL) {
        fprintf(stderr, "ASSETS: cannot open %s from the APK (%s). Was `make assets` packaged?\n", path,
                SDL_GetError());
        exit(1);
    }
    *sizeOut = (u32) sz;
    return (u8 *) data;
#else
    FILE *f = pc_open_asset(path);
    long size;
    u8 *buf;

    if (f == NULL) {
        fprintf(stderr,
                "ASSETS: cannot open %s. Put the assets/ folder next to the executable (or run from the "
                "directory that holds it); `make assets` produces it from your ROM.\n",
                path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(size);
    if (fread(buf, 1, size, f) != (size_t) size) {
        fprintf(stderr, "ASSETS: short read on %s\n", path);
        exit(1);
    }
    fclose(f);
    *sizeOut = (u32) size;
    return buf;
#endif
}

static void pc_assets_init(void) {
    u32 i;

    if (sAssetLut != NULL) {
        return;
    }
    sAssetLut = pc_load_file("assets/assets.lut.bin", &sAssetLutSize);
    sAssetsBin = pc_load_file("assets/assets.bin", &sAssetsBinSize);

    // LUT: entry count followed by offsets, all big-endian u32 — swap in place.
    for (i = 0; i + 3 < sAssetLutSize; i += 4) {
        u8 *p = &sAssetLut[i];
        u8 t0 = p[0], t1 = p[1];
        p[0] = p[3];
        p[1] = p[2];
        p[2] = t1;
        p[3] = t0;
    }
    printf("ASSETS: lut %u bytes, data %u bytes\n", sAssetLutSize, sAssetsBinSize);
}

u32 pc_asset_lut_size(void) {
    pc_assets_init();
    return sAssetLutSize;
}

// Byteswap helpers (pc_swap16_buf / pc_swap32_buf) live in linux/pc_asset.c.
extern void pc_swap16_buf(void *buf, u32 numBytes);

// Byteswap a region of the loaded asset image in place, addressed the way the
// game addresses ROM. Audio's RAW16 sample data is big-endian s16 and is DMA'd
// straight out of this image on demand by __amDMA — which has no idea what type
// of wave it is serving — so the swap has to happen once, here, at the source.
// (ADPCM waves are a byte stream and must NOT come through this. See bnkf.c.)
void pc_asset_swap16_region(u32 romOffset, u32 numBytes) {
    u32 offset;

    pc_assets_init();

    // ROM addresses are host addresses of the __ASSETS_LUT_* stubs plus a file
    // offset, truncated to 32 bits by the game. Modular subtraction recovers the
    // file offset whatever the (possibly 64-bit) stub address was.
    offset = romOffset - (u32) (uintptr_t) __ASSETS_LUT_END;
    if (offset >= sAssetsBinSize || offset + numBytes > sAssetsBinSize) {
        fprintf(stderr, "AUDIO: raw16 swap outside the asset image (0x%X + 0x%X) — skipped\n", romOffset, numBytes);
        return;
    }
    pc_swap16_buf(sAssetsBin + offset, numBytes);
}

void pc_dmacopy(u32 romOffset, uintptr_t ramAddress, s32 numBytes) {
    u32 offset;

    pc_assets_init();

    if (romOffset == (u32) (uintptr_t) __ASSETS_LUT_START) {
        if ((u32) numBytes > sAssetLutSize) {
            numBytes = sAssetLutSize;
        }
        memcpy((void *) ramAddress, sAssetLut, numBytes);
        return;
    }

    offset = romOffset - (u32) (uintptr_t) __ASSETS_LUT_END;
    if (offset < sAssetsBinSize) {
        if (offset + numBytes > sAssetsBinSize) {
            fprintf(stderr, "ASSETS: read past end (offset 0x%X + 0x%X > 0x%X), clamped\n", offset, numBytes,
                    sAssetsBinSize);
            numBytes = sAssetsBinSize - offset;
        }
        memcpy((void *) ramAddress, sAssetsBin + offset, numBytes);
        return;
    }

    fprintf(stderr, "ASSETS: dmacopy from unknown ROM address 0x%X (%d bytes) — zero-filled\n", romOffset, numBytes);
    memset((void *) ramAddress, 0, numBytes);
}

// ---------------------------------------------------------------------------
// Boot globals (normally set up by the PIF/boot code)
// ---------------------------------------------------------------------------
void *osRomBase = (void *) 0xB0000000;
s32 osTvType = 1;    // 0 = PAL, 1 = NTSC, 2 = MPAL
s32 osResetType = 0; // cold boot
s32 osAppNMIBuffer[16] = { 0 };

// Anti-piracy check: camera.c reads this raw cart-domain address and expects
// the low half to be 0x8965; anything else triggers the piracy path.
s32 D_B0000578 = 0x8965;

// ---------------------------------------------------------------------------
// Math library data (was libultra/src/gu/libm_vals.s)
// ---------------------------------------------------------------------------
float __libm_qnan_f = __builtin_nanf("");

// ---------------------------------------------------------------------------
// SI bus — controllers, EEPROM saves, controller paks, rumble.
// The N64 talks to all of these through PIF-RAM DMA over the Serial Interface
// (the libultra cont*/pfs*/eeprom*/motor files, dropped from the PC build).
// Constructive lies instead: one controller in port 0 with neutral input, an
// erased EEPROM (game falls back to creating default saves), and no
// controller paks / rumble paks plugged in anywhere.
// TODO: real host input (this is where it plugs in) and EEPROM persisted to a
// save file on disk.
// ---------------------------------------------------------------------------
#define PC_CONT_TYPE_NORMAL 0x0005
#define PC_CONT_NO_RESPONSE 0x8
#define PC_PFS_ERR_NOPACK 1
#define PC_MAXCONTROLLERS 4

typedef struct {
    u16 type;
    u8 status;
    u8 error;
} PCContStatus;

typedef struct {
    u16 button;
    s8 stick_x;
    s8 stick_y;
    u8 error;
} PCContPad;

extern s32 osSendMesg(void *mq, void *msg, s32 flags);

s32 osContInit(void *mq, u8 *bitpattern, PCContStatus *status) {
    s32 i;

    *bitpattern = 1; // controller in port 0 only
    for (i = 0; i < PC_MAXCONTROLLERS; i++) {
        status[i].type = (i == 0) ? PC_CONT_TYPE_NORMAL : 0;
        status[i].status = 0;
        status[i].error = (i == 0) ? 0 : PC_CONT_NO_RESPONSE;
    }
    return 0;
}

s32 osContStartReadData(void *mq) {
    // Poll "completes" instantly: post the done-message the game waits for.
    osSendMesg(mq, NULL, 0 /* OS_MESG_NOBLOCK */);
    return 0;
}

void osContGetReadData(PCContPad *pads) {
    s32 i;

    for (i = 0; i < PC_MAXCONTROLLERS; i++) {
        pads[i].button = 0;
        pads[i].stick_x = 0;
        pads[i].stick_y = 0;
        pads[i].error = (i == 0) ? 0 : PC_CONT_NO_RESPONSE;
    }

    input_host_read(&pads[0].button, &pads[0].stick_x, &pads[0].stick_y);
}

// EEPROM_TYPE_4K is 512 bytes / 64 eight-byte blocks. Persisted as
// eeprom.bin in the per-user preference directory SDL provides (on macOS
// ~/Library/Application Support/..., on Linux ~/.local/share/..., on Windows
// %APPDATA%\...). Every write goes straight to disk, since the game writes a
// few blocks at a time and only on save. Build with -DEEPROM_PRESET_100 to seed
// a fresh file from a 100%-completion image instead of erased 0xFF.
#define PC_EEPROM_BLOCKS 64
#define PC_EEPROM_BYTES (PC_EEPROM_BLOCKS * 8)

#ifdef EEPROM_PRESET_100
#include "eeprom_preset.h"
#endif

static u8 gEeprom[PC_EEPROM_BYTES];
static s32 gEepromReady = 0;
static char gEepromPath[1024];

#ifdef __EMSCRIPTEN__
// In a browser the file system is in memory and gone on reload, so the save
// lives on an IndexedDB-backed mount instead: pulled in once before the first
// read, pushed out after every write. The pull has to finish before the game
// reads the save, and ASYNCIFY lets this await it.
#include <emscripten.h>
EM_ASYNC_JS(void, web_save_mount, (), {
    FS.mkdir('/save');
    FS.mount(IDBFS, {}, '/save');
    await new Promise(function(resolve) { FS.syncfs(true, function() { resolve(); }); });
});
EM_JS(void, web_save_sync, (), { FS.syncfs(false, function(){}); });
#endif

static void eeprom_flush(void) {
    char tmp[sizeof(gEepromPath) + 8];
    FILE *f;

    if (gEepromPath[0] == '\0') {
        return;
    }
    // Write to a temp file and rename, so a crash mid-write cannot truncate the save.
    snprintf(tmp, sizeof(tmp), "%s.tmp", gEepromPath);
    f = fopen(tmp, "wb");
    if (f == NULL) {
        fprintf(stderr, "SAVE: cannot write %s\n", tmp);
        return;
    }
    if (fwrite(gEeprom, 1, PC_EEPROM_BYTES, f) != PC_EEPROM_BYTES) {
        fprintf(stderr, "SAVE: short write to %s\n", tmp);
    }
    fclose(f);
    if (rename(tmp, gEepromPath) != 0) {
        // Windows refuses to rename over an existing file.
        remove(gEepromPath);
        if (rename(tmp, gEepromPath) != 0) {
            fprintf(stderr, "SAVE: cannot replace %s\n", gEepromPath);
        }
    }
#ifdef __EMSCRIPTEN__
    web_save_sync();
#endif
}

static void eeprom_init(void) {
    char *pref;
    FILE *f;

    if (gEepromReady) {
        return;
    }
    gEepromReady = 1;

#ifdef EEPROM_PRESET_100
    memcpy(gEeprom, gEepromPreset, PC_EEPROM_BYTES);
#else
    memset(gEeprom, 0xFF, PC_EEPROM_BYTES); // erased
#endif

    if (pc_trace_active()) {
        return; // a trace starts from an erased save and keeps nothing
    }
#ifdef __EMSCRIPTEN__
    web_save_mount();
    snprintf(gEepromPath, sizeof(gEepromPath), "/save/eeprom.bin");
#else
    pref = SDL_GetPrefPath("bluesentinelsec", "DiddyKongRacing");
    if (pref == NULL) {
        fprintf(stderr, "SAVE: no preference directory (%s); progress will not be kept\n", SDL_GetError());
        return;
    }
    snprintf(gEepromPath, sizeof(gEepromPath), "%seeprom.bin", pref);
    SDL_free(pref);
#endif

    f = fopen(gEepromPath, "rb");
    if (f != NULL) {
        u8 image[PC_EEPROM_BYTES];
        size_t n = fread(image, 1, PC_EEPROM_BYTES, f);
        fclose(f);
        if (n == PC_EEPROM_BYTES) {
            memcpy(gEeprom, image, PC_EEPROM_BYTES);
            printf("SAVE: loaded %s\n", gEepromPath);
            return;
        }
        fprintf(stderr, "SAVE: %s is not a %d-byte EEPROM image; starting fresh\n", gEepromPath, PC_EEPROM_BYTES);
    }
    printf("SAVE: new game data at %s\n", gEepromPath);
    eeprom_flush();
}

s32 osEepromProbe(void *mq) {
    eeprom_init();
    return 1; // EEPROM_TYPE_4K — present
}

s32 osEepromRead(void *mq, u8 address, u8 *buffer) {
    eeprom_init();
    if (address >= PC_EEPROM_BLOCKS) {
        memset(buffer, 0xFF, 8);
        return 0;
    }
    memcpy(buffer, &gEeprom[address * 8], 8);
    return 0;
}

s32 osEepromWrite(void *mq, u8 address, u8 *buffer) {
    eeprom_init();
    if (address < PC_EEPROM_BLOCKS) {
        if (memcmp(&gEeprom[address * 8], buffer, 8) != 0) {
            memcpy(&gEeprom[address * 8], buffer, 8);
            eeprom_flush();
        }
    }
    return 0;
}

s32 osPfsInit(void *mq, void *pfs, s32 channel) {
    return PC_PFS_ERR_NOPACK;
}

s32 osPfsIsPlug(void *mq, u8 *pattern) {
    *pattern = 0;
    return 0;
}

s32 osPfsFileState(void *pfs, s32 fileNo, void *state) {
    return PC_PFS_ERR_NOPACK;
}

s32 osPfsReadWriteFile(void *pfs, s32 fileNo, u8 flag, s32 offset, s32 size, u8 *data) {
    return PC_PFS_ERR_NOPACK;
}

s32 osPfsNumFiles(void *pfs, s32 *maxFiles, s32 *filesUsed) {
    return PC_PFS_ERR_NOPACK;
}

s32 osPfsFreeBlocks(void *pfs, s32 *bytesNotUsed) {
    return PC_PFS_ERR_NOPACK;
}

s32 osPfsReFormat(void *pfs, void *mq, s32 channel) {
    return PC_PFS_ERR_NOPACK;
}

s32 osPfsFindFile(void *pfs, u16 companyCode, u32 gameCode, u8 *gameName, u8 *extName, s32 *fileNo) {
    return PC_PFS_ERR_NOPACK;
}

s32 osPfsDeleteFile(void *pfs, u16 companyCode, u32 gameCode, u8 *gameName, u8 *extName) {
    return PC_PFS_ERR_NOPACK;
}

s32 osPfsChecker(void *pfs) {
    return PC_PFS_ERR_NOPACK;
}

s32 osPfsAllocateFile(void *pfs, u16 companyCode, u32 gameCode, u8 *gameName, u8 *extName, s32 size, s32 *fileNo) {
    return PC_PFS_ERR_NOPACK;
}

s32 osMotorInit(void *mq, void *pfs, s32 channel) {
    return PC_PFS_ERR_NOPACK; // no rumble pak
}

s32 osMotorStart(void *pfs) {
    return 0;
}

s32 osMotorStop(void *pfs) {
    return 0;
}

// ---------------------------------------------------------------------------
// Debug printing (was src/isv_print.c writing to the IS-Viewer MMIO device at
// 0xB3FF0000 — the game's stubbed_printf channel; goes to stdout here)
// ---------------------------------------------------------------------------
void isv_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    // Flush every line: this is the port's only debug channel, and on a crash or
    // an ASan abort a buffered stdout swallows exactly the lines that say why.
    fflush(stdout);
}

// ---------------------------------------------------------------------------
// Cache management (no caches to manage on the host)
// ---------------------------------------------------------------------------
void osWritebackDCache(void *vaddr, s32 size) {
}
void osWritebackDCacheAll(void) {
}
void osInvalDCache(void *vaddr, s32 size) {
}
void osInvalICache(void *vaddr, s32 size) {
}

// ---------------------------------------------------------------------------
// Interrupt management (single-threaded host: nothing to mask)
// ---------------------------------------------------------------------------
s32 __osDisableInt(void) {
    return 0;
}
void __osRestoreInt(s32 mask) {
}
u32 osSetIntMask(u32 mask) {
    return 0;
}

// ---------------------------------------------------------------------------
// Timer / CP0 / FPU registers
// ---------------------------------------------------------------------------
// The N64 Count register ticks at 46.875 MHz (CPU clock / 2).
// 46875000 ticks/sec over 1e9 ns/sec = exactly 3/64 ticks per nanosecond.
// The VR4300 Count register: 46.875 MHz, and two consecutive reads never
// return the same value. The game relies on the latter - music_animation_fraction
// (src/audio.c) treats an unchanged count as a wrap and adds 2^32 ticks, which
// scrambled the character-select dance on hosts whose clock has microsecond
// granularity (macOS CLOCK_MONOTONIC). So read the finest clock available and
// force each read past the previous one.
u32 osGetCount(void) {
    static u32 sLast;
    struct timespec ts;
    u64 ns;
    u32 count;

    if (pc_trace_active()) {
        return pc_trace_count();
    }
#ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    ns = (u64) ts.tv_sec * 1000000000ull + (u64) ts.tv_nsec;
    count = (u32) (ns * 3 / 64);
    if (count == sLast) {
        count++;
    }
    sLast = count;
    return count;
}
void __osSetCompare(u32 value) {
}
u32 __osGetSR(void) {
    return 0;
}
void __osSetSR(u32 value) {
}
u32 __osSetFpcCsr(u32 value) {
    return 0;
}

// ---------------------------------------------------------------------------
// TLB (host has a real MMU; virtual==physical as far as the game cares)
// ---------------------------------------------------------------------------
u32 __osProbeTLB(void *vaddr) {
    return 0;
}
void osMapTLBRdb(void) {
}

// ---------------------------------------------------------------------------
// Exception / hardware-interrupt plumbing (was exceptasm.s data)
// ---------------------------------------------------------------------------
typedef struct {
    unsigned int inst1;
    unsigned int inst2;
    unsigned int inst3;
    unsigned int inst4;
} __osExceptionVector;
__osExceptionVector __osExceptionPreamble[1] = { { 0, 0, 0, 0 } };

// Pre-2.0J layout (both PC targets build VERSION_G): a plain array of
// handlers, with no stack frame.
s32 (*__osHwIntTable[8])(void) = { 0 };

// ---------------------------------------------------------------------------
// Thread context switching (was exceptasm.s code).
// Empty stubs for now, same as the OoT port's bring-up state — the real
// scheduler replacement is its own milestone.
// ---------------------------------------------------------------------------
void __osEnqueueThread(void **queue, void *thread) {
}
void __osEnqueueAndYield(void **queue) {
}
void *__osPopThread(void **queue) {
    return 0;
}
void __osDispatchThread(void) {
}
void __osCleanupThread(void) {
}
