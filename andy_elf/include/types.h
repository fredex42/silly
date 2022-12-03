/* shamelessly borrowed from linux bits/types.h */
#include "wordsize.h"
#ifndef _BITS_TYPES_H
#define _BITS_TYPES_H

typedef unsigned char u_char;
typedef unsigned short int u_short;
typedef unsigned int u_int;
typedef unsigned long int u_long;

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int int16_t;
typedef unsigned short int uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;

#if __WORDSIZE == 64
typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;
typedef unsigned long int size_t;
typedef uint64_t vaddr;
#else
__extension__ typedef signed long long int __int64_t;
__extension__ typedef unsigned long long int __uint64_t;
typedef uint32_t size_t;
typedef uint32_t vaddr;
#endif

typedef __uint64_t uint64_t;
typedef __int64_t __int64_t;
#define NULL (void *)0
#endif

// typedef size_t pid_t; //temporary
