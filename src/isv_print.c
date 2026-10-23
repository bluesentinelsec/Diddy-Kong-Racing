#include "common.h"
#include "host.h"
#include <stdarg.h>

/**
 * Debug printing. On the N64 this wrote to the IS-Viewer cartridge; here it goes
 * to the host's log.
 */
void isv_printf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    pc_vlog(fmt, args);
    va_end(args);
}
