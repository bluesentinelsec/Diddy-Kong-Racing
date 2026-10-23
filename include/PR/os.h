#ifndef _OS_H_
#define _OS_H_

/*
 * What is left of the N64 OS for the audio library: the two calls it still makes, which the
 * host answers in linux/reimpl.c.
 */
#include <PR/ultratypes.h>
#include <stdint.h>

typedef u32 OSIntMask;
#define OS_IM_NONE 0x00000001

uintptr_t osVirtualToPhysical(void *addr);
OSIntMask osSetIntMask(OSIntMask mask);

#endif
