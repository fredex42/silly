#include <types.h>
#include "ioapic.h"
#include "interrupts.h"
#include <sys/mmgr.h>

void write_ioapic_register(const vaddr apic_base, const uint8_t offset, const uint32_t val)
{
    /* tell IOREGSEL where we want to write to */
    *(volatile uint32_t*)(apic_base) = offset;
    mb();
    /* write the value to IOWIN */
    *(volatile uint32_t*)(apic_base + 0x10) = val;
}

uint32_t read_ioapic_register(const vaddr apic_base, const uint8_t offset)
{
    /* tell IOREGSEL where we want to read from */
    *(volatile uint32_t*)(apic_base) = offset;
    mb();
    /* return the data from IOWIN */
    return *(volatile uint32_t*)(apic_base + 0x10);
}

void configure_ioapic()
{
  uint32_t value = 0;
  size_t dir_idx = (size_t)DEFAULT_IOAPIC_BASE >> 22;
  size_t dir_off = (size_t)DEFAULT_IOAPIC_BASE >> 12 & 0x03FF;

  k_map_page(NULL, DEFAULT_IOAPIC_BASE, dir_idx, dir_off, MP_READWRITE);
  //The only interesting field is in bits 24 - 27: the APIC ID for this device
  uint32_t apic_id = (read_ioapic_register(DEFAULT_IOAPIC_BASE, IOAPICID) >> 24 & 0xF);

  uint32_t apic_ver_reg = read_ioapic_register(DEFAULT_IOAPIC_BASE, IOAPICVER);
  uint8_t apic_version = apic_ver_reg & 0xFF;
  uint8_t max_redir = (apic_ver_reg >> 16) & 0xFF;

  kprintf("IOAPIC id %l with version %d handling %d interrupts\r\n", apic_id, (uint16_t)apic_version, (uint16_t)max_redir);

  //IRQ0, timer
  //set vector 0x20 with delivery Fixed, destination Physical, delivery 0, pin active high, remote IRR unset, trigger mode
  //edge, unmasked
  write_ioapic_register(DEFAULT_IOAPIC_BASE, IOREDTBL_L(0), 0x20);
  write_ioapic_register(DEFAULT_IOAPIC_BASE, IOREDTBL_H(0), 0); //destination is ACPI ID 0

}
