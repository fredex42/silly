#include "panic.h"
#include <cfuncs.h>

void k_panic(char *msg) {
  kputs("KERNEL PANIC: ");
  kputs(msg);
  kputs("\r\n");

  asm volatile("cli\n\thlt\n\tjmp -1\n\t");
}
