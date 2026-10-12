#ifndef PC_ASSET_H
#define PC_ASSET_H

/*
 * Host-side asset unpacking (TARGET_PC).
 *
 * Every asset the game loads is a big-endian N64 image whose structs contain
 * 32-bit pointer slots. The N64 (and a 32-bit host) can overlay the C struct
 * straight onto those bytes; a 64-bit host cannot, because every pointer field
 * is now 8 bytes and shifts everything after it. So on PC each record is
 * *unpacked*: the on-disk bytes are read field by field, by their documented
 * disk offsets, into a host-layout struct. Pointer slots are copied as their
 * raw 32-bit value (an offset or an asset index) stored in the pointer field,
 * which keeps the game's existing "offset -> pointer" fixups working unchanged.
 *
 * This works identically on 32-bit hosts (where disk and host layouts happen
 * to coincide), so there is one code path for the Pi, Linux, macOS and
 * Windows. It replaces the previous in-place byteswap helpers.
 */

#include <stddef.h>
#include <stdint.h>
#include "types.h"

// Big-endian scalar readers over a byte image.
static inline u16 pc_be16(const void *base, size_t off) {
    const u8 *p = (const u8 *) base + off;
    return (u16) ((p[0] << 8) | p[1]);
}

static inline u32 pc_be32(const void *base, size_t off) {
    const u8 *p = (const u8 *) base + off;
    return ((u32) p[0] << 24) | ((u32) p[1] << 16) | ((u32) p[2] << 8) | (u32) p[3];
}

static inline void pc_wr16(void *base, size_t off, u16 v) {
    u8 *p = (u8 *) base + off;
    p[0] = (u8) (v >> 8);
    p[1] = (u8) v;
}

static inline void pc_wr32(void *base, size_t off, u32 v) {
    u8 *p = (u8 *) base + off;
    p[0] = (u8) (v >> 24);
    p[1] = (u8) (v >> 16);
    p[2] = (u8) (v >> 8);
    p[3] = (u8) v;
}

// Field kinds for table-driven unpacking.
enum PcFieldKind {
    PCU_BYTES = 0, // `count` raw bytes, copied as-is (u8/s8/char arrays, padding)
    PCU_U16,       // `count` big-endian 16-bit values
    PCU_U32,       // `count` big-endian 32-bit values (s32/u32/f32)
    PCU_PTR,       // `count` 32-bit pointer slots: raw value stored into a host pointer
    PCU_SPTR,      // same, but sign-extended (an s32 index such as -1 stored in a pointer-sized field)
};

typedef struct PcField {
    u16 hostOff; // offsetof() in the host struct
    u16 diskOff; // byte offset in the N64 image
    u16 count;   // number of elements of this kind
    u16 kind;    // enum PcFieldKind
} PcField;

#define PCU_FIELD(T, field, diskOff, count, kind) { (u16) offsetof(T, field), (u16) (diskOff), (u16) (count), (u16) (kind) }
#define PCU_END { 0, 0, 0, 0xFFFF }

// Memory moves usable from game code, which sees the N64 libc stubs rather than
// the host string.h.
void pc_memcpy(void *dst, const void *src, size_t numBytes);
void pc_memmove(void *dst, const void *src, size_t numBytes);

// Host heap allocation for scratch and host-only structures (never from the game pool).
void *pc_alloc(size_t numBytes);
void pc_free(void *p);

// In-place byteswap helpers for flat arrays of scalars (tables of offsets, ids).
void pc_swap16_buf(void *buf, u32 numBytes);
void pc_swap32_buf(void *buf, u32 numBytes);

// Swap a run of scalar fields in place, addressed from the struct base (so the
// span may legitimately cover several adjacent fields).
#define PC_SWAP16_AT(p, field, nbytes) pc_swap16_buf((u8 *) (p) + offsetof(typeof(*(p)), field), (nbytes))
#define PC_SWAP32_AT(p, field, nbytes) pc_swap32_buf((u8 *) (p) + offsetof(typeof(*(p)), field), (nbytes))

// Unpack one record. `src` is `srcSize` bytes of N64 image; `dst` is a host
// struct of `dstSize` bytes. The two may overlap (the common case: the record
// was loaded in place). Fields not named in the table are zeroed.
void pc_unpack(void *dst, size_t dstSize, const void *src, size_t srcSize, const PcField *fields);

// Unpack `n` consecutive records of `srcStride` bytes into consecutive host
// records of `dstStride` bytes. dst may equal src (the array grows in place;
// the caller guarantees room for n * dstStride bytes at dst).
void pc_unpack_array(void *dst, size_t dstStride, const void *src, size_t srcStride, s32 n, const PcField *fields);

// Byteswap-only variant for records whose host layout equals the disk layout
// (no pointer fields): identical to pc_unpack with dst == src and equal sizes.
#define pc_swap_record(ptr, size, fields) pc_unpack((ptr), (size), (ptr), (size), (fields))

// Pointer-sized integer casts for address arithmetic that the N64 source does
// through s32/u32.
#define PC_PTR2INT(p) ((uintptr_t) (p))
#define PC_INT2PTR(T, i) ((T) (uintptr_t) (i))

#endif
