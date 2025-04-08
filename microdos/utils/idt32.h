#include <types.h>
#include "memlayout.h"

extern void int_ff_trapvec();
void configure_interrupt(struct InterruptDescriptor32 *idt, uint8_t idx, void *ptr, uint8_t type, uint8_t with_relocation);
void setup_interrupts(uint8_t with_relocation);
void configure_pic_interrupts(uint8_t starting_vector);