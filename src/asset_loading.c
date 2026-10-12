#include "asset_loading.h"
#ifdef TARGET_PC
#include "pc_asset.h"
#endif

#include "common.h"
#include "gzip.h"
#include "macros.h"
#include "memory.h"
#include "ultra64.h"

/************ .bss ************/

OSIoMesg gAssetsDmaIoMesg;
OSMesg gDmaMesg;
OSMesgQueue gDmaMesgQueue;
OSMesg gPIMesgBuf[16];
OSMesgQueue gPIMesgQueue;
#if VERSION >= VERSION_79
OSMesg gDmaMutexBuf;
OSMesgQueue gDmaMutex;
#define dmacopy_internal dmacopy_v1
#else
#define dmacopy_internal dmacopy
#endif
u32 *gAssetsLookupTable;

/*******************************/

// These are both defined in the generated dkr.ld file.
#ifdef TARGET_PC
#include "pc_rom.h" // the same two ROM offsets, as constants
#else
extern u8 __ASSETS_LUT_START[], __ASSETS_LUT_END[];
#endif

#ifdef TARGET_PC
#ifdef TARGET_PC
// linux/reimpl.c: assets served from assets/assets{,.lut}.bin on disk.
extern u32 pc_asset_lut_size(void);
extern void pc_dmacopy(u32 romOffset, uintptr_t ramAddress, s32 numBytes);
#endif

#endif
/**
 * Set up the peripheral interface message queues and scheduling.
 * This will send messages when DMA reads are finished.
 * After, allocate space and load the asset table into RAM.
 * Official Name: piInit
 */
void pi_init(void) {
    u32 assetTableSize;
    osCreateMesgQueue(&gPIMesgQueue, gPIMesgBuf, ARRAY_COUNT(gPIMesgBuf));
    osCreateMesgQueue(&gDmaMesgQueue, &gDmaMesg, 1);
    osCreatePiManager((OSPri) 150, &gPIMesgQueue, gPIMesgBuf, ARRAY_COUNT(gPIMesgBuf));

#if VERSION >= VERSION_79
    osCreateMesgQueue(&gDmaMutex, &gDmaMutexBuf, 1);
    osSendMesg(&gDmaMutex, (OSMesg) 1, OS_MESG_NOBLOCK);
#endif

#ifdef TARGET_PC
    assetTableSize = pc_asset_lut_size();
#else
    assetTableSize = __ASSETS_LUT_END - __ASSETS_LUT_START;
#endif
    gAssetsLookupTable = (u32 *) mempool_alloc_safe(assetTableSize, COLOUR_TAG_GREY);
    mempool_locked_set((u8 *) gAssetsLookupTable);
#ifdef TARGET_PC
    dmacopy_internal((u32) (uintptr_t) __ASSETS_LUT_START, (uintptr_t) gAssetsLookupTable, (s32) assetTableSize);
#else
    dmacopy_internal((u32) __ASSETS_LUT_START, (u32) gAssetsLookupTable, (s32) assetTableSize);
#endif
}

/**
 * Returns the memory address containing an asset section loaded from ROM.
 * Official Name: piRomLoad
 */
u32 *asset_table_load(u32 assetIndex) {
    u32 *index;
    u32 *out;
    s32 size;
    u32 start;

#if VERSION >= VERSION_79
    OSMesg msg = NULL;
    osRecvMesg(&gDmaMutex, &msg, OS_MESG_BLOCK);
#endif

    if (gAssetsLookupTable[0] < assetIndex) {
        return NULL;
    }
    assetIndex++;
    index = assetIndex + gAssetsLookupTable;
    start = *index;
    size = *(index + 1) - start;
    out = (u32 *) mempool_alloc_safe(size, COLOUR_TAG_GREY);
    if (out == 0) {
        return NULL;
    }

#ifdef TARGET_PC
    dmacopy_internal((u32) (uintptr_t) (start + __ASSETS_LUT_END), (uintptr_t) out, size);
#else
    dmacopy_internal((u32) (start + __ASSETS_LUT_END), (u32) out, size);
#endif

#if VERSION >= VERSION_79
    osSendMesg(&gDmaMutex, (OSMesg) 1, OS_MESG_NOBLOCK);
#endif

    return out;
}

/**
 * Loads a gzip compressed asset from the ROM file.
 * Returns a pointer to the decompressed data.
 * Official name: piRomLoadCompressed
 */
UNUSED u8 *asset_table_load_zipped(u32 assetIndex, s32 extraMemory) {
    s32 size;
    s32 start;
    s32 totalSpace;
    u8 *gzipHeaderRamPos;
    u8 *out;

#if VERSION >= VERSION_79
    OSMesg msg = NULL;
    osRecvMesg(&gDmaMutex, &msg, OS_MESG_BLOCK);
#endif

    if (gAssetsLookupTable[0] < assetIndex) {
        return NULL;
    }
    assetIndex++;
    out = (u8 *) (assetIndex + gAssetsLookupTable);
    start = ((s32 *) out)[0];
    size = ((s32 *) out)[1] - start;
    gzipHeaderRamPos = (u8 *) mempool_alloc_safe(8, COLOUR_TAG_WHITE);

    dmacopy_internal((u32) (start + __ASSETS_LUT_END), (u32) gzipHeaderRamPos, 8);

    totalSpace = byteswap32(gzipHeaderRamPos) + extraMemory;
    mempool_free(gzipHeaderRamPos);
    out = (u8 *) mempool_alloc_safe(totalSpace + extraMemory, COLOUR_TAG_GREY);
    if (out == NULL) {
        return NULL;
    }
    gzipHeaderRamPos = (out + totalSpace) - size;
    if (1) {} // Fakematch
    dmacopy_internal((u32) (start + __ASSETS_LUT_END), (u32) gzipHeaderRamPos, size);
    gzip_inflate(gzipHeaderRamPos, out);

#if VERSION >= VERSION_79
    osSendMesg(&gDmaMutex, (OSMesg) 1, OS_MESG_NOBLOCK);
#endif

    return out;
}

/**
 * Loads an asset section to a specific memory address.
 * Returns the size of asset section.
 */
#ifdef TARGET_PC
UNUSED s32 asset_table_load_addr(u32 assetIndex, uintptr_t address) {
#else
UNUSED s32 asset_table_load_addr(u32 assetIndex, u32 address) {
#endif
    u32 start;
    s32 size;
    u32 *index;

#if VERSION >= VERSION_79
    OSMesg msg = NULL;
    osRecvMesg(&gDmaMutex, &msg, OS_MESG_BLOCK);
#endif

    if (gAssetsLookupTable[0] < assetIndex) {
        return NULL;
    }
    assetIndex++;
    index = assetIndex + gAssetsLookupTable;
    start = *index;
    size = *(index + 1) - start;

#ifdef TARGET_PC
    dmacopy_internal((u32) (uintptr_t) (start + __ASSETS_LUT_END), address, size);
#else
    dmacopy_internal((u32) (start + __ASSETS_LUT_END), address, size);
#endif

#if VERSION >= VERSION_79
    osSendMesg(&gDmaMutex, (OSMesg) 1, OS_MESG_NOBLOCK);
#endif

    return size;
}

/**
 * Loads part of an asset section to a specific memory address.
 * Returns the size argument.
 * Official name: piRomLoadSection
 */
#ifdef TARGET_PC
s32 asset_load(u32 assetIndex, uintptr_t address, s32 assetOffset, s32 size) {
#else
s32 asset_load(u32 assetIndex, u32 address, s32 assetOffset, s32 size) {
#endif
    u32 *index;
    s32 start;

#if VERSION >= VERSION_79
    OSMesg msg = NULL;
    osRecvMesg(&gDmaMutex, &msg, OS_MESG_BLOCK);
#endif

    if (size == 0 || gAssetsLookupTable[0] < assetIndex) {
        return NULL;
    }

    assetIndex++;
    index = assetIndex + gAssetsLookupTable;
    start = *index + assetOffset;

#ifdef TARGET_PC
    dmacopy_internal((u32) (uintptr_t) (start + __ASSETS_LUT_END), address, size);
#else
    dmacopy_internal((u32) (start + __ASSETS_LUT_END), address, size);
#endif

#if VERSION >= VERSION_79
    osSendMesg(&gDmaMutex, (OSMesg) 1, OS_MESG_NOBLOCK);
#endif

    return size;
}
#ifdef TARGET_PC

#ifdef TARGET_PC
/**
 * PC: some offset tables are reinterpreted by the game as arrays of pointers in
 * place, which only works where a pointer is 4 bytes. Load such a table as a
 * pointer-sized array holding the raw (zero-extended) big-endian entries; the
 * game's "(s32) entry != -1" terminators and offset->pointer fixups then work.
 */
void **asset_table_load_pointers(u32 assetIndex) {
    s32 numBytes = asset_table_size(assetIndex);
    u32 *raw = asset_table_load(assetIndex);
    s32 n = numBytes / 4;
    s32 i;
    void **out;

    if (raw == NULL) {
        return NULL;
    }
    out = mempool_alloc_safe((n > 0 ? n : 1) * sizeof(void *), COLOUR_TAG_GREY);
    for (i = 0; i < n; i++) {
        out[i] = (void *) (uintptr_t) (intptr_t) (s32) pc_be32(raw, i * 4); // sign-extend: -1 terminators
    }
    mempool_free(raw);
    return out;
}
#endif
#endif

/**
 * Returns a rom offset of an asset given its asset section and a local offset.
 * Official name: piRomGetSectionPtr
 */
u8 *asset_rom_offset(u32 assetIndex, u32 assetOffset) {
    u32 *index;
    u32 start;

#if VERSION >= VERSION_79
    OSMesg msg = NULL;
    osRecvMesg(&gDmaMutex, &msg, OS_MESG_BLOCK);
#endif

    if (gAssetsLookupTable[0] < assetIndex) {
        return NULL;
    }

    assetIndex++;
    index = assetIndex + gAssetsLookupTable;
    start = *index + assetOffset;

#if VERSION >= VERSION_79
    osSendMesg(&gDmaMutex, (OSMesg) 1, OS_MESG_NOBLOCK);
#endif

    return start + __ASSETS_LUT_END;
}

/**
 * Returns the size of an asset section.
 * Official name: piRomGetFileSize
 */
s32 asset_table_size(u32 assetIndex) {
    u32 *index;

#if VERSION >= VERSION_79
    OSMesg msg = NULL;
    osRecvMesg(&gDmaMutex, &msg, OS_MESG_BLOCK);
#endif

    if (gAssetsLookupTable[0] < assetIndex) {
        return NULL;
    }

    assetIndex++;
    index = assetIndex + gAssetsLookupTable;

#if VERSION >= VERSION_79
    osSendMesg(&gDmaMutex, (OSMesg) 1, OS_MESG_NOBLOCK);
#endif

    return *(index + 1) - *index;
}

#define MAX_TRANSFER_SIZE 0x5000

/**
 * Copies data from the game cartridge to a ram address.
 * Official name: romCopy
 */
#ifdef TARGET_PC
void dmacopy(u32 romOffset, uintptr_t ramAddress, s32 numBytes) {
#else
void dmacopy(u32 romOffset, u32 ramAddress, s32 numBytes) {
#endif
#if VERSION >= VERSION_79
    OSMesg msg = NULL;
    osRecvMesg(&gDmaMutex, &msg, OS_MESG_BLOCK);
    dmacopy_internal(romOffset, ramAddress, numBytes);
    osSendMesg(&gDmaMutex, (OSMesg) 1, OS_MESG_NOBLOCK);
}

// Looks like v2 ROMs made an alternate version of this function, and this is the original.
#ifdef TARGET_PC
void dmacopy_internal(u32 romOffset, uintptr_t ramAddress, s32 numBytes) {
#else
void dmacopy_internal(u32 romOffset, u32 ramAddress, s32 numBytes) {
#endif
#endif
#ifdef TARGET_PC
    pc_dmacopy(romOffset, ramAddress, numBytes);
#else
    OSMesg dmaMesg;
    s32 numBytesToDMA;

    osInvalDCache((u32 *) ramAddress, numBytes);
    numBytesToDMA = MAX_TRANSFER_SIZE;
    while (numBytes > 0) {
        if (numBytes < numBytesToDMA) {
            numBytesToDMA = numBytes;
        }
        osPiStartDma(&gAssetsDmaIoMesg, OS_MESG_PRI_NORMAL, OS_READ, romOffset, (u32 *) ramAddress, numBytesToDMA,
                     &gDmaMesgQueue);
        osRecvMesg(&gDmaMesgQueue, &dmaMesg, OS_MESG_BLOCK);
        numBytes -= numBytesToDMA;
        romOffset += numBytesToDMA;
        ramAddress += numBytesToDMA;
    }
#endif
}
