#include <types.h>

#ifndef __SYS_IOPORTS_H
#define __SYS_IOPORTS_H

//These are emitted as functions if optimization is off, otherwise they should be inlined (-O2 or higher)

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    //`"Nd`" implies that the port is either a constant 8-bit immediate (N) or in DX (d); thus the compiler
    //can choose to use either in port, al or in dx, al depending on what is more efficient.
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}
static inline uint32_t ind(uint16_t port) {
    uint32_t ret;
    asm volatile ( "ind %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ( "outb %0, %1" : : "a"(value), "Nd"(port) );
}
static inline void outw(uint16_t port, uint16_t value) {
    asm volatile ( "outw %0, %1" : : "a"(value), "Nd"(port) );
}
static inline void outd(uint16_t port, uint32_t value) {
    asm volatile ( "outd %0, %1" : : "a"(value), "Nd"(port) );
}

/*
Purpose: Wait a very small amount of time (1 to 4 microseconds, generally).
 Useful for implementing a small delay for PIC remapping on old hardware or
 generally as a simple but imprecise wait.
 You can do an IO operation on any unused port: the Linux kernel by default
 uses port 0x80, which is often used during POST to log information on the
 motherboard's hex display but almost always unused after boot.
 https://wiki.osdev.org/Inline_Assembly/Examples#I.2FO_access
Returns: 0
*/
static inline void io_wait() {
    asm volatile ( "outb %%al, $0x80" : : "a"(0) );
}

static inline void cli() {
    asm volatile ( "cli" : : : "memory" );
}
static inline void sti() {
    asm volatile ( "sti" : : : "memory" );
}

#endif
