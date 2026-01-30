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
void kputs(char *str) {
  puts(str);
}

void k_panic() {
  puts("Error, kernel would have panicked\n");
}

void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    (void)vprintf(fmt, ap);
}
