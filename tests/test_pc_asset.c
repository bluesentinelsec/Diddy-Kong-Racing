// Unit tests for the host asset unpacker (linux/pc_asset.c).
//
// The unpacker is the piece of the port that decides whether a 64-bit host
// reads the N64 asset images correctly, so its contract is pinned down here:
// big-endian readers, field-table unpacking into host layouts (including the
// in-place/overlapping case and the sign-extended pointer slots), and the
// back-to-front array unpack that lets a record array grow in place.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pc_asset.h"
#include "test.h"

// A host struct standing in for the game's: scalar fields around pointer slots.
typedef struct {
    u8 tag[2];
    u16 count;
    u32 size;
    void *ptr;   // 32-bit offset on disk
    void *index; // s32 asset index on disk, may be -1
    s16 coords[3];
} Record;

#define RECORD_DISK_SIZE 22 // 0x10: three s16 coords end at 0x16

static const PcField sRecordFields[] = {
    PCU_FIELD(Record, tag, 0x00, 2, PCU_BYTES),
    PCU_FIELD(Record, count, 0x02, 1, PCU_U16),
    PCU_FIELD(Record, size, 0x04, 1, PCU_U32),
    PCU_FIELD(Record, ptr, 0x08, 1, PCU_PTR),
    PCU_FIELD(Record, index, 0x0C, 1, PCU_SPTR),
    PCU_FIELD(Record, coords, 0x10, 3, PCU_U16),
    PCU_END,
};

static void make_disk_record(u8 *d, u8 tag0, u16 count, u32 size, u32 ptr, s32 index, s16 x, s16 y, s16 z) {
    d[0] = tag0;
    d[1] = 0xEE;
    pc_wr16(d, 2, count);
    pc_wr32(d, 4, size);
    pc_wr32(d, 8, ptr);
    pc_wr32(d, 12, (u32) index);
    pc_wr16(d, 16, (u16) x);
    pc_wr16(d, 18, (u16) y);
    pc_wr16(d, 20, (u16) z);
}

static void test_readers(void) {
    u8 buf[8] = { 0x12, 0x34, 0x56, 0x78, 0xFF, 0xFE, 0x00, 0x01 };

    CHECK_EQ(pc_be16(buf, 0), 0x1234u);
    CHECK_EQ(pc_be16(buf, 4), 0xFFFEu);
    CHECK_EQ(pc_be32(buf, 0), 0x12345678u);
    CHECK_EQ(pc_be32(buf, 4), 0xFFFE0001u);

    pc_wr32(buf, 0, 0xA1B2C3D4u);
    CHECK_EQ(buf[0], 0xA1u);
    CHECK_EQ(buf[3], 0xD4u);
    pc_wr16(buf, 4, 0x0102);
    CHECK_EQ(buf[4], 0x01u);
    CHECK_EQ(buf[5], 0x02u);
}

static void test_swap_buffers(void) {
    u8 w[8] = { 0, 0, 0, 1, 0, 0, 1, 0 };
    u8 h[4] = { 0, 1, 0, 2 };
    u32 words[2];
    u16 halves[2];

    pc_swap32_buf(w, 8);
    memcpy(words, w, 8);
    CHECK_EQ(words[0], 1u);
    CHECK_EQ(words[1], 0x100u);

    pc_swap16_buf(h, 4);
    memcpy(halves, h, 4);
    CHECK_EQ(halves[0], 1u);
    CHECK_EQ(halves[1], 2u);
}

static void test_unpack_separate(void) {
    u8 disk[RECORD_DISK_SIZE];
    Record r;

    memset(disk, 0, sizeof(disk));
    make_disk_record(disk, 0x7A, 300, 0x12345, 0x80, -1, -5, 7, 9);
    memset(&r, 0xAA, sizeof(r));
    pc_unpack(&r, sizeof(r), disk, RECORD_DISK_SIZE, sRecordFields);

    CHECK_EQ(r.tag[0], 0x7Au);
    CHECK_EQ(r.tag[1], 0xEEu);
    CHECK_EQ(r.count, 300u);
    CHECK_EQ(r.size, 0x12345u);
    CHECK_EQ((uintptr_t) r.ptr, 0x80u);
    // PCU_SPTR sign-extends: -1 compares equal to (void *) -1 on any pointer width.
    CHECK(r.index == (void *) (intptr_t) -1);
    CHECK_EQ((unsigned) (u16) r.coords[0], (unsigned) (u16) -5);
    CHECK_EQ(r.coords[1], 7);
    CHECK_EQ(r.coords[2], 9);
}

static void test_unpack_in_place(void) {
    // The record is loaded at the front of a buffer big enough for the host
    // struct; unpacking over itself must read every field before clobbering it.
    u8 buf[sizeof(Record) + 32];
    Record *r = (Record *) buf;

    memset(buf, 0, sizeof(buf));
    make_disk_record(buf, 0x01, 2, 0xDEADBEEF, 0x40, 12, 1, 2, 3);
    pc_unpack(r, sizeof(Record), buf, RECORD_DISK_SIZE, sRecordFields);

    CHECK_EQ(r->tag[0], 0x01u);
    CHECK_EQ(r->count, 2u);
    CHECK_EQ(r->size, 0xDEADBEEFu);
    CHECK_EQ((uintptr_t) r->ptr, 0x40u);
    CHECK_EQ((uintptr_t) r->index, 12u);
    CHECK_EQ(r->coords[2], 3);
}

static void test_unpack_array_grows_in_place(void) {
    // Three disk records packed at 22 bytes each, unpacked in place into host
    // records (larger stride). Only the back-to-front order survives this.
    enum { N = 3, DISK = RECORD_DISK_SIZE };
    u8 buf[N * sizeof(Record) + 64];
    Record *out = (Record *) buf;
    int i;

    memset(buf, 0, sizeof(buf));
    for (i = 0; i < N; i++) {
        make_disk_record(buf + i * DISK, (u8) (0x10 + i), (u16) (100 + i), (u32) i, (u32) (i * 8), i - 1, i, i, i);
    }
    pc_unpack_array(out, sizeof(Record), buf, DISK, N, sRecordFields);

    for (i = 0; i < N; i++) {
        CHECK_EQ(out[i].tag[0], (unsigned) (0x10 + i));
        CHECK_EQ(out[i].count, (unsigned) (100 + i));
        CHECK_EQ(out[i].size, (unsigned) i);
        CHECK_EQ((uintptr_t) out[i].ptr, (uintptr_t) (i * 8));
        CHECK(out[i].index == (void *) (intptr_t) (i - 1));
        CHECK_EQ(out[i].coords[1], i);
    }
}

static void test_unpack_zeroes_unlisted_fields(void) {
    typedef struct {
        u32 a;
        void *runtimeOnly;
        u32 b;
    } Mixed;
    static const PcField fields[] = {
        PCU_FIELD(Mixed, a, 0, 1, PCU_U32),
        PCU_FIELD(Mixed, b, 4, 1, PCU_U32),
        PCU_END,
    };
    u8 disk[8] = { 0, 0, 0, 5, 0, 0, 0, 6 };
    Mixed m;

    memset(&m, 0xFF, sizeof(m));
    pc_unpack(&m, sizeof(m), disk, sizeof(disk), fields);
    CHECK_EQ(m.a, 5u);
    CHECK_EQ(m.b, 6u);
    CHECK(m.runtimeOnly == NULL);
}

int main(void) {
    RUN(test_readers);
    RUN(test_swap_buffers);
    RUN(test_unpack_separate);
    RUN(test_unpack_in_place);
    RUN(test_unpack_array_grows_in_place);
    RUN(test_unpack_zeroes_unlisted_fields);
    return test_summary();
}
