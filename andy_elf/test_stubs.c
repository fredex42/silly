#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

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

int kprintf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    return vprintf(fmt, ap);
}
