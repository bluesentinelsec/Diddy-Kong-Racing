#include "PR/os_internal.h"
#include "PR/R4300.h"
#include "PRinternal/osint.h"

#ifdef TARGET_PC
uintptr_t osVirtualToPhysical(void* addr) {
    return (uintptr_t) addr;
}
#else
u32 osVirtualToPhysical(void* addr) {
    if (IS_KSEG0(addr)) {
        return K0_TO_PHYS(addr);
    } else if (IS_KSEG1(addr)) {
        return K1_TO_PHYS(addr);
    } else {
        return __osProbeTLB(addr);
    }
}
#endif
