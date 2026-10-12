#ifndef _TYPES_H_
#define _TYPES_H_

#include <PR/ultratypes.h>

#ifdef TARGET_PC
#ifdef GBI_FLOAT_MTX
typedef float MtxF[4][4] __attribute__((aligned(8)));
#else
typedef float MtxF[4][4];
#endif
#else
typedef float MtxF[4][4];
#endif
typedef s32 MtxS[4][4];
typedef s16 VertexList;
typedef u8 TriangleList;
#ifdef TARGET_PC
// Platform code (dreamcast/, linux/) pulls the system headers, whose newlib
// (DC) already declares uintptr_t and sets this guard; redefining it is an
// error under C23. Game code compiles against the N64 libc stubs, which never
// declare it, so the typedef below still applies there.
#ifdef TARGET_PC
/* Host builds: pointers may be 64 bits wide, so take the real uintptr_t. */
#include <stdint.h>
/* Integer wide enough to hold an address: the game does address arithmetic
 * through s32, which only works where pointers are 32 bits. */
typedef uintptr_t ptrint_t;
#else
typedef s32 ptrint_t;
#ifndef _UINTPTR_T_DECLARED
typedef u32 uintptr_t;
#define _UINTPTR_T_DECLARED
#endif
#endif
#else
typedef u32 uintptr_t;
#endif

#ifdef TARGET_PC
#ifdef NON_MATCHING
void isv_printf(const char *fmt, ...);
#define stubbed_printf isv_printf
#else
#define stubbed_printf
#endif
#else
#if defined(__sgi)
#define stubbed_printf
#else
#define stubbed_printf(...)
#endif
#endif

#endif
