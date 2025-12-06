#include <sys/ioports.h>
#include <types.h>
#include "8259pic.h"
#include <cpuid.h>
#include "picroutines.h"

/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
void legacy_pic_remap(int offset1, int offset2)
{
  kputs("    Remapping legacy PIC...");
	unsigned char a1, a2;

	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA);

	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();

	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	outb(PIC1_DATA, 0);   // enable all.
	io_wait();
	outb(PIC2_DATA, 0);
  kputs("done.\r\n");
}

/*
disable the legacy 8259 pic. See https://wiki.osdev.org/PIC
*/
void disable_legacy_pic()
{
asm volatile ("mov $0xff, %%al\n\t"
  "out %%al, $0xa1\n\t"
  "out %%al, $0x21\n\t"
 : : : "%eax");
}

void setup_pic()
{
  uint32_t cpuflags_dx = cpuid_edx_features();
  if( (cpuflags_dx & CPUID_FEAT_EDX_APIC) && (cpuflags_dx & CPUID_FEAT_EDX_MSR)) {
    kputs("Detected APIC, disabling...");
    disable_lapic();
    kputs("done.\r\n");

    // kputs("Configuring IOAPIC...");
    // configure_ioapic(NULL);
    // kputs("done.\r\n");
  }
  legacy_pic_remap(0x20, 0x28);
  configure_pic_interrupts();

}
