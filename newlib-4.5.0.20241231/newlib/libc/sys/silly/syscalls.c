#include <unistd.h>
#include <sys/types.h>
#include "syscalls.h"

/**
 * Private function to perform a syscall with up to six arguments.
 * Handles interrupt version only at the moment
 */
static inline uint32_t __syscall(uint32_t num, uint32_t ebx, uint32_t ecx,
                          uint32_t edx, uint32_t esi, uint32_t edi) {
  uint32_t ret;

  __asm__ volatile(
    "int $0x60"
      : "=a"(ret)
      : "a"(num), "b"(ebx), "c"(ecx), "d"(edx), "S"(esi), "D"(edi)
      : "memory", "cc");
  return ret;
}

/**
 * Writes up to count bytes from the buffer starting at buf to the file
 * descriptor fd.
 */
int write(int fd, const void *buf, size_t count) {
    return __syscall(0x0B, fd, count, 0, (uint32_t)buf, 0);
}

/**
 * Terminates the calling process with the given exit status.
 */
void _exit(int status) {
    __syscall(0x01, status, 0, 0, 0, 0);
    while (1) { } // perma-loop just in case it does return
}