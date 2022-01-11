#include <types.h>

#ifndef __SYS_IOPORTS_H
#define __SYS_IOPORTS_H

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t ind(uint16_t port);

void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
void outd(uint16_t port, uint32_t value);

void io_wait();

#endif
