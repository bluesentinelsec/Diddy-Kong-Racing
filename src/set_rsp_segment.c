#include "set_rsp_segment.h"
#include "PR/gbi.h"
#include "PR/mbi.h"
#include <PR/ultratypes.h>

/**
 * Set the offset for the given address, so the RSP can get the correct physical address.
 * This is used for the colour and depth buffers.
 * Official name: segSetBase
 */
// The segment base carries the KSEG0 bit the display-list interpreter expects.
#define K0BASE 0x80000000

void rsp_segment(Gfx **dList, s32 segment, ptrint_t base) {
    gSPSegment((*dList)++, segment, base + K0BASE)
}
