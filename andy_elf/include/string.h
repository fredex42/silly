/*functions in utils/string.asm */
#include <types.h>

#ifndef __STRING_H
#define __STRING_H

size_t strncpy(char *dest, const char *src, size_t len);
size_t strncmp(const char *a, const char *b, size_t max);
const char *strchr(const char *s, char c);
size_t strlen(const char *str);
#endif
