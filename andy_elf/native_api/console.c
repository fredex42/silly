#include <types.h>
#include "console.h"

//this function does not follow C calling conventions
void PMPrintStringLen();

size_t console_write(char *buf, size_t len)
{
  //arguments are reversed compared to nasm!!!!!
  asm volatile(
    "mov %0, %%esi\n\t"
    "mov %1, %%ecx\n\t"
    "call %P2" : : "m" (buf), "m" (len), "i"(PMPrintStringLen) : "edi", "ecx"
  );
}
