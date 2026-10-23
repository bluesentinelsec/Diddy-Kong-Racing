#include "asset_loading.h"
#include "pc_asset.h"

#include "common.h"
#include "gzip.h"
#include "host.h"
#include "macros.h"
#include "memory.h"

/************ .bss ************/

u32 *gAssetsLookupTable;

/*******************************/

#include "pc_rom.h" // the two ROM offsets the asset image is addressed from, as constants

/**
 * Load the asset lookup table: the offset of every section of the asset image.
 * Official Name: piInit
 */
void pi_init(void) {
    u32 assetTableSize;

    assetTableSize = pc_asset_lut_size();
    gAssetsLookupTable = (u32 *) mempool_alloc_safe(assetTableSize, COLOUR_TAG_GREY);
    mempool_locked_set((u8 *) gAssetsLookupTable);
    dmacopy((u32) (uintptr_t) __ASSETS_LUT_START, (uintptr_t) gAssetsLookupTable, (s32) assetTableSize);
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
    dmacopy((u32) (uintptr_t) (start + __ASSETS_LUT_END), (uintptr_t) out, size);
    return out;
}

/**
 * Loads part of an asset section to a specific memory address.
 * Returns the size argument.
 * Official name: piRomLoadSection
 */
s32 asset_load(u32 assetIndex, uintptr_t address, s32 assetOffset, s32 size) {
    u32 *index;
    s32 start;

    if (size == 0 || gAssetsLookupTable[0] < assetIndex) {
        return NULL;
    }
    assetIndex++;
    index = assetIndex + gAssetsLookupTable;
    start = *index + assetOffset;
    dmacopy((u32) (uintptr_t) (start + __ASSETS_LUT_END), address, size);
    return size;
}

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

/**
 * Returns a rom offset of an asset given its asset section and a local offset.
 * Official name: piRomGetSectionPtr
 */
u8 *asset_rom_offset(u32 assetIndex, u32 assetOffset) {
    u32 *index;
    u32 start;

    if (gAssetsLookupTable[0] < assetIndex) {
        return NULL;
    }
    assetIndex++;
    index = assetIndex + gAssetsLookupTable;
    start = *index + assetOffset;
    return start + __ASSETS_LUT_END;
}

/**
 * TRUE when the asset image was packed by tools/assets.py and so stores the
 * entries the ROM keeps deflated (object models, animations, level models and
 * level object maps) already inflated. Those loaders then read them as they are.
 */
s32 asset_image_inflated(void) {
    return pc_asset_image_inflated();
}

/**
 * Returns the size of an asset section.
 * Official name: piRomGetFileSize
 */
s32 asset_table_size(u32 assetIndex) {
    u32 *index;

    if (gAssetsLookupTable[0] < assetIndex) {
        return NULL;
    }
    assetIndex++;
    index = assetIndex + gAssetsLookupTable;
    return *(index + 1) - *index;
}

/**
 * Copies data from the game cartridge to a ram address.
 * Official name: romCopy
 */
void dmacopy(u32 romOffset, uintptr_t ramAddress, s32 numBytes) {
    pc_dmacopy(romOffset, ramAddress, numBytes);
}
