// Host-side asset unpacking. See include/pc_asset.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_asset.h"

// Byteswap a buffer of big-endian u32s in place (asset offset tables etc.).
void pc_swap32_buf(void *buf, u32 numBytes) {
    u8 *p = buf;
    u32 i;

    for (i = 0; i + 3 < numBytes; i += 4) {
        u8 t0 = p[i + 0], t1 = p[i + 1];
        p[i + 0] = p[i + 3];
        p[i + 1] = p[i + 2];
        p[i + 2] = t1;
        p[i + 3] = t0;
    }
}

// Same, for buffers of big-endian u16s.
void pc_swap16_buf(void *buf, u32 numBytes) {
    u8 *p = buf;
    u32 i;

    for (i = 0; i + 1 < numBytes; i += 2) {
        u8 t = p[i];
        p[i] = p[i + 1];
        p[i + 1] = t;
    }
}

void pc_memcpy(void *dst, const void *src, size_t numBytes) {
    memcpy(dst, src, numBytes);
}

void pc_memmove(void *dst, const void *src, size_t numBytes) {
    memmove(dst, src, numBytes);
}

void *pc_alloc(size_t numBytes) {
    void *p = calloc(1, numBytes ? numBytes : 1);
    if (p == NULL) {
        fprintf(stderr, "pc_alloc: out of host memory (%zu bytes)\n", numBytes);
        abort();
    }
    return p;
}

void pc_free(void *p) {
    free(p);
}

#define PC_UNPACK_TEMP_MAX 4096

static void pc_unpack_fields(u8 *dst, const u8 *tmp, const PcField *f) {
    for (; f->kind != 0xFFFF; f++) {
        u16 i;
        switch (f->kind) {
            case PCU_BYTES:
                memcpy(dst + f->hostOff, tmp + f->diskOff, f->count);
                break;
            case PCU_U16:
                for (i = 0; i < f->count; i++) {
                    u16 v = pc_be16(tmp, f->diskOff + i * 2u);
                    memcpy(dst + f->hostOff + i * 2u, &v, 2);
                }
                break;
            case PCU_U32:
                for (i = 0; i < f->count; i++) {
                    u32 v = pc_be32(tmp, f->diskOff + i * 4u);
                    memcpy(dst + f->hostOff + i * 4u, &v, 4);
                }
                break;
            case PCU_SPTR:
                for (i = 0; i < f->count; i++) {
                    uintptr_t v = (uintptr_t) (intptr_t) (s32) pc_be32(tmp, f->diskOff + i * 4u);
                    memcpy(dst + f->hostOff + i * sizeof(uintptr_t), &v, sizeof(uintptr_t));
                }
                break;
            case PCU_PTR:
                for (i = 0; i < f->count; i++) {
                    uintptr_t v = pc_be32(tmp, f->diskOff + i * 4u);
                    memcpy(dst + f->hostOff + i * sizeof(uintptr_t), &v, sizeof(uintptr_t));
                }
                break;
            default:
                fprintf(stderr, "pc_unpack: bad field kind %u\n", f->kind);
                abort();
        }
    }
}

void pc_unpack(void *dst, size_t dstSize, const void *src, size_t srcSize, const PcField *fields) {
    u8 tmp[PC_UNPACK_TEMP_MAX];

    if (srcSize > sizeof(tmp)) {
        fprintf(stderr, "pc_unpack: record of %zu bytes exceeds the %d-byte scratch buffer\n", srcSize,
                PC_UNPACK_TEMP_MAX);
        abort();
    }
    // Copy first: dst and src usually alias (the record was loaded in place).
    memcpy(tmp, src, srcSize);
    memset(dst, 0, dstSize);
    pc_unpack_fields((u8 *) dst, tmp, fields);
}

void pc_unpack_array(void *dst, size_t dstStride, const void *src, size_t srcStride, s32 n, const PcField *fields) {
    s32 i;

    // Back to front: when dst == src and dstStride > srcStride, record i's host
    // image spills into record i+1's disk bytes, which must already be consumed.
    for (i = n - 1; i >= 0; i--) {
        pc_unpack((u8 *) dst + (size_t) i * dstStride, dstStride, (const u8 *) src + (size_t) i * srcStride, srcStride,
                  fields);
    }
}
