#ifndef __IOAPIC_H
#define __IOAPIC_H

#define DEFAULT_IOAPIC_BASE 0xfec00000

//IOAPIC register definitions
#define IOAPICID      0
#define IOAPICVER     1
#define IOAPICARB     2

//Macro to calculare the register values for the given irq
//The first is the value for the "low" register bits 0-31
//The second is for the "high" register which is bits 32-63
#define IOREDTBL_L(irq_num) 0x10 + irq_num*2
#define IOREDTBL_H(irq_num) 0x10 + irq_num*2 + 1

#endif
