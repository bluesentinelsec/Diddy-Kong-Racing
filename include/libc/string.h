#ifndef STRING_H
#define STRING_H

#include "PR/ultratypes.h"

void *memcpy(void *dst, const void *src, size_t size);
void *memmove(void *dst, const void *src, size_t size);
void *memset(void *dst, int value, size_t size);
void bcopy(const void *src, void *dst, size_t size);
int bcmp(const void *a, const void *b, size_t size);
void bzero(void *dst, size_t size);
size_t strlen(const char *str);
char *strchr(const char *str, int ch);

#endif
