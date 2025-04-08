#include <types.h>

#ifndef __SYS_IOPORTS_H
#define __SYS_IOPORTS_H

// uint8_t inb(uint16_t port);
// uint16_t inw(uint16_t port);
// uint32_t ind(uint16_t port);

// void outb(uint16_t port, uint8_t value);
// void outw(uint16_t port, uint16_t value);
// void outd(uint16_t port, uint32_t value);

// void io_wait();

// void cli();
// void sti();
/*Purpose: Wait a very small amount of time (1 to 4 microseconds, generally).
; Useful for implementing a small delay for PIC remapping on old hardware or
; generally as a simple but imprecise wait.
;Returns: 0
*/
#define io_wait() asm __volatile__ ("xor %%eax, %%eax\nout %%al,$0x80" : : : "eax");
/*
; You can do an IO operation on any unused port: the Linux kernel by default
; uses port 0x80, which is often used during POST to log information on the
; motherboard's hex display but almost always unused after boot.
; https://wiki.osdev.org/Inline_Assembly/Examples#I.2FO_access*/

//Read in a byte from the given IO port.
#define inb(port, out_value) asm __volatile__ ("in %%dx, %%al" : "=a"(out_value) : "d"(port))
//Read in a word (2 bytes) from the given IO port
#define inw(port, out_value) asm __volatile__ ("in %%dx, %%ax" : "=a"(out_value) : "d"(port))
//Read in a dword (4 bytes) from the given IO port
#define ind(port, out_value) asm __volatile__ ("in %%dx, %%eax" : "=a"(out_value) : "d"(port))

//Write
#define outb(port, value) asm __volatile__("out %%al, %%dx" : : "d"(port),"a"(value))

#endif
