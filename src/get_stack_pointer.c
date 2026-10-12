#include "memory.h"

#ifdef __sgi
/**
 * Uses IDO specific -dollar compiler flag to get the current stack pointer.
 * Official Name: diCpuTraceCurrentStack
 */
StackInfo *stack_pointer(void) {
    return (StackInfo *) __$sp;
}
#else
#ifdef TARGET_PC
/**
 * Uses a GCC builtin to get the current stack pointer, portable to any target.
 * Official Name: diCpuTraceCurrentStack
 */
#else
/**
 * Uses GCC specific code to get the current stack pointer.
 * Official Name: diCpuTraceCurrentStack
 */
#endif
StackInfo *stack_pointer(void) {
#ifdef TARGET_PC
    return (StackInfo *) __builtin_frame_address(0);
#else
    register StackInfo *sp;
    asm volatile("move %0, $sp\n" : "=r"(sp));
    return sp;
#endif
}
#endif
