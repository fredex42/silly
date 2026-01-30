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
    return __syscall(API_WRITE, fd, count, 0, (uint32_t)buf, 0);
}

/**
 * Reads up to count bytes from the file descriptor fd into the buffer
 * starting at buf.  Blocks if no data is available.
 */
int read(int fd, void *buf, size_t count) {
    return __syscall(API_READ, fd, count, 0, (uint32_t)buf, 0);
}

/**
 * Closes the file descriptor fd.
 */
void close(int fd) {
    __syscall(API_CLOSE, fd, 0, 0, 0, 0);
}

/**
 * Duplicates the file descriptor oldfd, returning a new file descriptor
 * that refers to the same open file description. Not yet implemented in kernel.
 */
int dup(int oldfd) {
    return __syscall(API_DUP, oldfd, 0, 0, 0, 0);
}

/**
 * Performs the ioctl operation on the file descriptor fd. Not yet implemented in kernel.
 */
int ioctl(int fd, unsigned long request, void *argp) {
    return __syscall(API_IOCTL, fd, request, 0, (uint32_t)argp, 0);
}

/**
 * Returns the current time as the number of seconds since Jan 1, 2000.
 * If tloc is non-null, also stores this value at the location pointed to by tloc.
 */
long time(long *tloc) {
    uint32_t seconds = __syscall(API_GET_TIME, 0, 0, 0, 0, 0);
    if (tloc) {
        *tloc = (long)seconds;
    }
    return (long)seconds;
}

/**
 * Terminates the calling process with the given exit status.
 */
void _exit(int status) {
    __syscall(API_EXIT, status, 0, 0, 0, 0);
    while (1) { } // perma-loop just in case it does return
}