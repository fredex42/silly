/*functions in utils/string.asm */
#include <types.h>

#ifndef __STRING_H
#define __STRING_H

size_t strncpy(char *dest, char *src, size_t len);
size_t strncmp(char *a, char *b, size_t max);

#endif
