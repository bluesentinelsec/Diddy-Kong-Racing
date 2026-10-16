// Legacy BSD byte-string functions that libultra and the decompiled game call.
// glibc and macOS still provide them, but Android's bionic removed them, so this
// file supplies them on Android only. It is empty everywhere else.
//
// Only <string.h> is included (never <strings.h>) so bionic's deprecated
// declarations of these names cannot clash with the definitions below.
#ifdef __ANDROID__

#include <stddef.h>
#include <string.h>

void bzero(void *s, size_t n) {
    memset(s, 0, n);
}

void bcopy(const void *src, void *dst, size_t n) {
    memmove(dst, src, n);
}

int bcmp(const void *a, const void *b, size_t n) {
    return memcmp(a, b, n);
}

#endif
