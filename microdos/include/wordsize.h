/* shamelessly borrowed from linux bits/wordsize.h */
#ifndef __WORDSIZE_H
#define __WORDSIZE_H

#if defined __x86_64__ && !defined __ILP32__
#pragma GCC error "You should not be compiling in 64-bit mode"
# define __WORDSIZE     64
#else
# define __WORDSIZE     32
#define __WORDSIZE32_SIZE_ULONG         0
#define __WORDSIZE32_PTRDIFF_LONG       0
#endif

#endif
