#include <stdint.h>
#include <stdarg.h>
/* Forward declare libc functions to avoid include path conflicts */
int puts(const char *);
int vprintf(const char *, va_list);

/**
This file contains stub implementations of logging functions that call through to the
glibc counterparts. This is to enable compilation of tests that can run as regular
linux apps.
*/
void kputs(const char *str) {
  puts(str);
}

void kputlen(const char *str, uint32_t len) {
  /* naive: print as a separate buffer */
  for(uint32_t i=0;i<len && str[i]!=0;i++) {
    putchar(str[i]);
  }
}

void k_panic() {
  puts("Error, kernel would have panicked\n");
}

void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    (void)vprintf(fmt, ap);
}
