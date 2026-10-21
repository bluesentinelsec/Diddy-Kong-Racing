#include "PR/os_internal.h"
#include "PR/R4300.h"
#include "PRinternal/osint.h"

uintptr_t osVirtualToPhysical(void* addr) {
    return (uintptr_t) addr;
}
