// Asset overrides: a folder of replacement files laid over the packed asset
// image at start-up, so a mod needs no rebuild.
//
// The folder has the layout tools/assets.py writes for assets/tree/:
//     <section>/<NNNN>.bin    one entry of a section that has an offset table
//     <section>.bin           a section kept whole
// Only the files present are replaced; a section's offset table is rebuilt
// around the new entry sizes. Entries past the last one the image has are
// appended, as long as their numbers run on without a gap.
//
// Overrides apply only to an image packed by tools/assets.py (one whose
// entries are stored inflated), because that is the form the files are in.
//
// The folder is $DKR_ASSET_OVERRIDES, or assets/overrides next to the working
// directory or the executable.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL3/SDL.h>

#include "types.h"

#define SECTION_COUNT 50
#define LUT_TRAILER_SIZE 16
#define IMAGE_ALIGN 16

enum { KIND_BLOB, KIND_ENTRIES, KIND_TABLE };

typedef struct {
    const char *name;
    u8 kind;
    s8 table; // KIND_ENTRIES: index of the section's offset table
    u8 unit;  // KIND_ENTRIES: bytes per offset-table step
} PcAssetSection;

// Mirrors SECTIONS in tools/assets.py, in the order of include/asset_enums.h.
static const PcAssetSection sSections[SECTION_COUNT] = {
    { "ai_behaviour", KIND_ENTRIES, 1, 1 },
    { "ai_behaviour_table", KIND_TABLE, -1, 1 },
    { "textures_3d", KIND_ENTRIES, 3, 1 },
    { "textures_3d_table", KIND_TABLE, -1, 1 },
    { "textures_2d", KIND_ENTRIES, 5, 1 },
    { "textures_2d_table", KIND_TABLE, -1, 1 },
    { "game_text", KIND_BLOB, -1, 1 },
    { "game_text_table", KIND_BLOB, -1, 1 },
    { "menu_text", KIND_BLOB, -1, 1 },
    { "menu_text_table", KIND_BLOB, -1, 1 },
    { "screens", KIND_BLOB, -1, 1 },
    { "screens_table", KIND_BLOB, -1, 1 },
    { "sprites", KIND_ENTRIES, 13, 1 },
    { "sprites_table", KIND_TABLE, -1, 1 },
    { "empty_14", KIND_BLOB, -1, 1 },
    { "misc", KIND_ENTRIES, 16, 4 },
    { "misc_table", KIND_TABLE, -1, 1 },
    { "hud_element_ids", KIND_BLOB, -1, 1 },
    { "menu_element_ids", KIND_BLOB, -1, 1 },
    { "weather_particles", KIND_BLOB, -1, 1 },
    { "level_object_maps_table", KIND_TABLE, -1, 1 },
    { "level_object_maps", KIND_ENTRIES, 20, 1 },
    { "level_headers_table", KIND_TABLE, -1, 1 },
    { "level_headers", KIND_ENTRIES, 22, 1 },
    { "level_names_table", KIND_TABLE, -1, 1 },
    { "level_names", KIND_ENTRIES, 24, 1 },
    { "level_models_table", KIND_TABLE, -1, 1 },
    { "level_models", KIND_ENTRIES, 26, 1 },
    { "object_models_table", KIND_TABLE, -1, 1 },
    { "object_models", KIND_ENTRIES, 28, 1 },
    { "animation_ids", KIND_BLOB, -1, 1 },
    { "object_animations_table", KIND_TABLE, -1, 1 },
    { "object_animations", KIND_ENTRIES, 31, 1 },
    { "object_headers_table", KIND_TABLE, -1, 1 },
    { "objects", KIND_ENTRIES, 33, 1 },
    { "level_object_translation_table", KIND_BLOB, -1, 1 },
    { "empty_37_table", KIND_BLOB, -1, 1 },
    { "empty_37", KIND_BLOB, -1, 1 },
    { "audio_table", KIND_BLOB, -1, 1 },
    { "audio", KIND_BLOB, -1, 1 },
    { "particles_table", KIND_TABLE, -1, 1 },
    { "particles", KIND_ENTRIES, 40, 1 },
    { "particle_behaviors_table", KIND_TABLE, -1, 1 },
    { "particle_behaviors", KIND_ENTRIES, 42, 1 },
    { "fonts", KIND_BLOB, -1, 1 },
    { "japanese_fonts_table", KIND_BLOB, -1, 1 },
    { "japanese_fonts", KIND_BLOB, -1, 1 },
    { "dummy_particle_ids", KIND_BLOB, -1, 1 },
    { "ttghosts_table", KIND_BLOB, -1, 1 },
    { "ttghosts", KIND_BLOB, -1, 1 },
};

typedef struct {
    u8 *data;
    u32 size;
    s32 owned; // malloc'd here (replaced), else points into the original image
} PcSectionBuf;

static u32 be32(const u8 *p) {
    return ((u32) p[0] << 24) | ((u32) p[1] << 16) | ((u32) p[2] << 8) | p[3];
}

static void put32(u8 *p, u32 v) {
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v;
}

static u32 align_up(u32 n) {
    return (n + IMAGE_ALIGN - 1) & ~(u32) (IMAGE_ALIGN - 1);
}

static u8 *read_file(const char *path, u32 *sizeOut) {
    FILE *f = fopen(path, "rb");
    long size;
    u8 *buf;

    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(size > 0 ? size : 1);
    if (fread(buf, 1, size, f) != (size_t) size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *sizeOut = (u32) size;
    return buf;
}

static s32 dir_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && (st.st_mode & S_IFDIR);
}

// An offset table: k non-decreasing offsets from 0, the end offset, then a tail
// (the all-ones terminator and padding). Returns the number of entries k; the
// word after entry k is the end, and *tailWords counts the words after that.
static u32 parse_table(const u8 *table, u32 tableSize, u32 *tailWords) {
    u32 words = tableSize / 4;
    u32 m = 0;

    while (m + 1 < words && be32(table + (m + 1) * 4) != 0xFFFFFFFFu &&
           be32(table + (m + 1) * 4) >= be32(table + m * 4)) {
        m++;
    }
    *tailWords = words > m + 1 ? words - (m + 1) : 0;
    return m;
}

// Replace the entries of section `si` that have files in `folder`, rebuilding
// the section and its offset table. Returns the number of entries replaced.
static s32 override_entries(PcSectionBuf *secs, s32 si, const char *folder) {
    const PcAssetSection *sec = &sSections[si];
    PcSectionBuf *data = &secs[si];
    PcSectionBuf *table = &secs[sec->table];
    u32 tailWords;
    u32 count = parse_table(table->data, table->size, &tailWords);
    u32 capacity = count + 64;
    u8 **pieces = calloc(capacity, sizeof(u8 *));
    u32 *sizes = calloc(capacity, sizeof(u32));
    s32 *owned = calloc(capacity, sizeof(s32));
    u32 total = 0;
    u32 n = 0;
    s32 replaced = 0;
    u32 i;
    char path[2048];

    // Each entry: the override file if there is one, else the image's bytes.
    for (i = 0;; i++) {
        u32 fileSize;
        u8 *file;

        snprintf(path, sizeof(path), "%s/%04u.bin", folder, i);
        file = read_file(path, &fileSize);
        if (i >= count && file == NULL) {
            break;
        }
        if (n == capacity) {
            capacity *= 2;
            pieces = realloc(pieces, capacity * sizeof(u8 *));
            sizes = realloc(sizes, capacity * sizeof(u32));
            owned = realloc(owned, capacity * sizeof(s32));
        }
        if (file != NULL) {
            if (fileSize % sec->unit != 0) {
                fprintf(stderr, "ASSETS: override %s must be a multiple of %u bytes, ignored\n", path, sec->unit);
                free(file);
                file = NULL;
            } else {
                replaced++;
            }
        }
        if (file != NULL) {
            pieces[n] = file;
            sizes[n] = fileSize;
            owned[n] = 1;
        } else {
            u32 start = be32(table->data + i * 4) * sec->unit;
            u32 end = be32(table->data + (i + 1) * 4) * sec->unit;
            pieces[n] = data->data + start;
            sizes[n] = end - start;
            owned[n] = 0;
        }
        total += sizes[n];
        n++;
    }

    if (replaced == 0) {
        for (i = 0; i < n; i++) {
            if (owned[i]) {
                free(pieces[i]);
            }
        }
    } else {
        // New section bytes and a new table: offsets, end, the original tail,
        // padded to at least the original word count.
        u32 tableWords = n + 1 + tailWords;
        u32 origWords = table->size / 4;
        u8 *newData = malloc(total > 0 ? total : 1);
        u8 *newTable;
        u32 off = 0;

        if (tableWords < origWords) {
            tableWords = origWords;
        }
        newTable = calloc(tableWords, 4);
        for (i = 0; i < n; i++) {
            put32(newTable + i * 4, off / sec->unit);
            memcpy(newData + off, pieces[i], sizes[i]);
            off += sizes[i];
            if (owned[i]) {
                free(pieces[i]);
            }
        }
        put32(newTable + n * 4, off / sec->unit);
        memcpy(newTable + (n + 1) * 4, table->data + (count + 1) * 4, tailWords * 4);
        if (data->owned) {
            free(data->data);
        }
        if (table->owned) {
            free(table->data);
        }
        data->data = newData;
        data->size = total;
        data->owned = 1;
        table->data = newTable;
        table->size = tableWords * 4;
        table->owned = 1;
    }
    free(pieces);
    free(sizes);
    free(owned);
    return replaced;
}

static s32 override_blob(PcSectionBuf *sec, const char *path) {
    u32 size;
    u8 *file = read_file(path, &size);

    if (file == NULL) {
        return 0;
    }
    if (sec->owned) {
        free(sec->data);
    }
    sec->data = file;
    sec->size = size;
    sec->owned = 1;
    return 1;
}

// Lay the sections out as a fresh image and lookup table (same trailer).
static void rebuild_image(PcSectionBuf *secs, const u8 *oldLut, u32 oldLutSize, u8 **lutOut, u32 *lutSizeOut,
                          u8 **binOut, u32 *binSizeOut) {
    u32 lutWords = 1 + SECTION_COUNT + 1;
    u32 lutSize = align_up(lutWords * 4) + LUT_TRAILER_SIZE;
    u8 *lut = calloc(lutSize, 1);
    u32 total = 0;
    u8 *bin;
    u32 off = 0;
    s32 i;

    for (i = 0; i < SECTION_COUNT; i++) {
        total = align_up(total + secs[i].size);
    }
    bin = calloc(total > 0 ? total : 1, 1);
    put32(lut, SECTION_COUNT);
    for (i = 0; i < SECTION_COUNT; i++) {
        put32(lut + 4 + i * 4, off);
        memcpy(bin + off, secs[i].data, secs[i].size);
        off = align_up(off + secs[i].size);
    }
    put32(lut + 4 + SECTION_COUNT * 4, off);
    memcpy(lut + lutSize - LUT_TRAILER_SIZE, oldLut + oldLutSize - LUT_TRAILER_SIZE, LUT_TRAILER_SIZE);
    *lutOut = lut;
    *lutSizeOut = lutSize;
    *binOut = bin;
    *binSizeOut = total;
}

// Find the override folder: $DKR_ASSET_OVERRIDES, else assets/overrides in the
// working directory or next to the executable. Returns NULL when there is none.
static const char *find_override_dir(char *buf, size_t bufSize) {
    const char *env = getenv("DKR_ASSET_OVERRIDES");
    const char *base;

    if (env != NULL && env[0] != '\0') {
        snprintf(buf, bufSize, "%s", env);
        return dir_exists(buf) ? buf : NULL;
    }
    snprintf(buf, bufSize, "assets/overrides");
    if (dir_exists(buf)) {
        return buf;
    }
    base = SDL_GetBasePath();
    if (base != NULL) {
        snprintf(buf, bufSize, "%sassets/overrides", base);
        if (dir_exists(buf)) {
            return buf;
        }
    }
    return NULL;
}

/**
 * Apply the override folder, if any, to the loaded image (big-endian lookup
 * table, still unswapped). On a change the two buffers are replaced with new
 * malloc'd ones. Returns the number of files applied.
 */
s32 pc_asset_apply_overrides(u8 **lutIo, u32 *lutSizeIo, u8 **binIo, u32 *binSizeIo) {
    char dir[2048];
    char path[2304];
    PcSectionBuf secs[SECTION_COUNT];
    u8 *lut = *lutIo;
    u8 *bin = *binIo;
    s32 applied = 0;
    s32 i;

    if (find_override_dir(dir, sizeof(dir)) == NULL) {
        return 0;
    }
    if (be32(lut) != SECTION_COUNT || *lutSizeIo < 4 + (SECTION_COUNT + 1) * 4) {
        fprintf(stderr, "ASSETS: overrides in %s ignored: unexpected lookup table\n", dir);
        return 0;
    }
    for (i = 0; i < SECTION_COUNT; i++) {
        u32 start = be32(lut + 4 + i * 4);
        u32 end = be32(lut + 4 + (i + 1) * 4);

        secs[i].data = bin + start;
        secs[i].size = end - start;
        secs[i].owned = 0;
    }
    for (i = 0; i < SECTION_COUNT; i++) {
        s32 n = 0;

        if (sSections[i].kind == KIND_BLOB) {
            snprintf(path, sizeof(path), "%s/%s.bin", dir, sSections[i].name);
            n = override_blob(&secs[i], path);
        } else if (sSections[i].kind == KIND_ENTRIES) {
            snprintf(path, sizeof(path), "%s/%s", dir, sSections[i].name);
            if (dir_exists(path)) {
                n = override_entries(secs, i, path);
            }
        }
        if (n > 0) {
            printf("ASSETS: %d override%s for %s\n", n, n == 1 ? "" : "s", sSections[i].name);
            applied += n;
        }
    }
    if (applied > 0) {
        u8 *newLut;
        u8 *newBin;
        u32 newLutSize;
        u32 newBinSize;

        rebuild_image(secs, lut, *lutSizeIo, &newLut, &newLutSize, &newBin, &newBinSize);
        free(lut);
        free(bin);
        *lutIo = newLut;
        *lutSizeIo = newLutSize;
        *binIo = newBin;
        *binSizeIo = newBinSize;
    }
    for (i = 0; i < SECTION_COUNT; i++) {
        if (secs[i].owned) {
            free(secs[i].data);
        }
    }
    return applied;
}
