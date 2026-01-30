#include <types.h>
#include "console.h"

//this function does not follow C calling conventions
void PMPrintStringLen();

size_t console_write(char *buf, size_t len)
{
  // Load args into ESI/ECX and call the routine address in a register.
  __asm__ __volatile__(
    "call *%2\n\t"
    :
    : "S" (buf), "c" (len), "r" (PMPrintStringLen)
    : "edi", "memory"
  );
  return len;
}
