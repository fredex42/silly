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

void configure_ioapic(vaddr ioapic_base)
{
  if(ioapic_base==NULL) ioapic_base = DEFAULT_IOAPIC_BASE;
  uint32_t value = 0;
  size_t dir_idx = (size_t)ioapic_base >> 22;
  size_t dir_off = (size_t)ioapic_base >> 12 & 0x03FF;

  //identity-map the register location for the ioapic
  k_map_page(NULL, ioapic_base, dir_idx, dir_off, MP_READWRITE);
  //The only interesting field is in bits 24 - 27: the APIC ID for this device
  uint32_t apic_id = (read_ioapic_register(ioapic_base, IOAPICID) >> 24 & 0xF);

  uint32_t apic_ver_reg = read_ioapic_register(ioapic_base, IOAPICVER);
  uint8_t apic_version = apic_ver_reg & 0xFF;
  uint8_t max_redir = (apic_ver_reg >> 16) & 0xFF;

  kprintf("IOAPIC id %l with version %d handling %d interrupts\r\n", apic_id, (uint16_t)apic_version, (uint16_t)max_redir);

  //IRQ0, timer
  //set vector 0x20 with delivery Fixed, destination Physical, delivery 0, pin active high, remote IRR unset, trigger mode
  //edge, unmasked
  write_ioapic_register(ioapic_base, IOREDTBL_L(0), 0x20);
  write_ioapic_register(ioapic_base, IOREDTBL_H(0), 0); //destination is ACPI ID 0

}
