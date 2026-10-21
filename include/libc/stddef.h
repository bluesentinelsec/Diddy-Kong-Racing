#ifndef STDDEF_H
#define STDDEF_H

#include "PR/ultratypes.h"

/* Host builds get the real stddef.h (size_t, offsetof). The host header
 * redefines NULL as ((void *) 0) on every inclusion; the game assigns NULL to
 * integers, so restore the N64 spelling after it. */
#include_next <stddef.h>
#undef NULL
#define NULL 0

#ifndef offsetof
#define offsetof(st, m) ((size_t)&(((st *)0)->m))
#endif

#endif
