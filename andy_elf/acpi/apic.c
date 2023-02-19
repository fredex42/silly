#include <types.h>
#include <sys/mmgr.h>
#include <acpi/rsdp.h>
#include <acpi/sdt_header.h>
#include <cfuncs.h>
#include <stdio.h>
#include <memops.h>
#include "apic.h"

/*
works out how many pages we need to store the given number of bytes
*/
size_t bytes_to_pages(size_t bytes)
{
  size_t pages = bytes / PAGE_SIZE;
  size_t partial_pages = bytes % PAGE_SIZE;
  if(partial_pages>0) ++pages;
  return pages;
}

/*
Create an identity page mapping for the APIC io mapped memory range.
According to the specs, this should be on the memory page 0xFEE00xxx
*/
void* apic_io_identity_mapping(vaddr apic_base)
{
  uint16_t directory_number;
  uint16_t entry_number;
  _resolve_vptr((void *)apic_base, &directory_number, & entry_number);

  kprintf("      DEBUG pointer 0x%x is at 0x%x,0x%x\r\n", apic_base, directory_number, entry_number);

  //the memory pointer 0xFEC00000 should be on the page 0x0 of directory 0x3FB (1019)
  return k_map_page(NULL, (void *)apic_base, directory_number, entry_number, MP_READWRITE);
}

/*
Writes the given 32-bit value to the given memory-mapped register
*/
void _write_mmapped_reg(vaddr apic_base, uint16_t reg, uint32_t value)
{
  volatile uint32_t* regptr = (uint32_t *)(apic_base + reg);
  *regptr = value;
  mb(); //use a memory barrier to ensure that the write is ordered
}

/*
Read a 32-bit value from the given memory-mapped register and return it
*/
uint32_t _read_mmapped_reg(vaddr apic_base, uint16_t reg)
{
  volatile uint32_t* regptr = (uint32_t *)(apic_base + reg);
  register uint32_t v = *regptr;
  mb();
  return v;
}

void plapic_send_eoi()
{
  k_panic("plapic_send_eoi not implemented yet");
}
/*
Enables the processor-local apic on this processor.
Arguments:
- apic_base      - pointer to the APIC mapped base address to use for communication
- apic_id        - uint8_t APIC id
- interrupt_base - software interrupt number onto which we should map plapic #0
Returns:
- the number of interrupts mapped
*/
int16_t enable_plapic(vaddr apic_base, uint8_t apic_id, uint16_t interrupt_base)
{
  //set up lapic timer
  _write_mmapped_reg(apic_base, 0x320, (LAPIC_TIMER_CHOSEN_VECTOR & LVT_VECTOR_MASK));

  //set up spurious interrupt to 0xFF
  _write_mmapped_reg(apic_base, 0xF0, 0xFF | SPV_APIC_ENABLED);

  return 1;
}

/**
 * Enables the PLApic and IOApic, and disables the legacy 8259 APIC.
 * Returns 0 if successfully initialised or 1 if not supported.
*/
uint8_t apic_enable_modern_style()
{
  struct AcpiMADT* madt = (struct AcpiMADT*) acpi_get_madt();
  if(!madt) {
    kputs("ERROR No ACPI description for multi-processor APICs, will have to use legacy mode\r\n");
    return 1;
  }
  read_madt_info((char *)madt);
  return 0;
}

/*
Reads information in from the MADT to a form we can work with
*/
void read_madt_info(char *madt_ptr)
{
  register uint32_t ctr = 0;
  uint16_t i=0;
  ProcessorLocalAPIC *apic;
  IOAPIC *ioapic;
  IOAPICInterruptSourceOverride *ovr;
  uint32_t *temp;

  ACPISDTHeader *table_header = (ACPISDTHeader *) madt_ptr;
  size_t pages_needed = bytes_to_pages(table_header->Length);

  kprintf("MADT is %d bytes long, needs %d pages\r\n", table_header->Length, pages_needed);

  ctr += sizeof(ACPISDTHeader); //the main header

  APICGeneralInformation *apic_general = (APICGeneralInformation *)&madt_ptr[ctr];

  kprintf("  Local APIC physical address is 0x%x\r\n", apic_general->local_apic_address);
  return;

  if(apic_general->legacy_flags & APIC_GENERAL_LEGACY_PIC_PRESENT) {
    kputs("  Legacy APIC is present, disabling\r\n");
    legacy_pic_remap(0x20, 0x28);
    disable_legacy_pic();
  }
  ctr += sizeof(APICGeneralInformation);
  while(ctr<table_header->Length) {
    MADTEntryHeader *h = (MADTEntryHeader *)&madt_ptr[ctr];
    kprintf("  Entry %d is type %d length %d\r\n", (uint32_t)i, (uint32_t)h->type, (uint32_t)h->length);
    switch(h->type) {
      case 0:
        apic = (ProcessorLocalAPIC *)&madt_ptr[ctr+2];  //skip over the type and length fields
        kprintf("      Processor ID %d with apic ID %d flags 0x%x\r\n", apic->acpi_processor_id, apic->apic_id, apic->apic_flags);
        if(apic_io_identity_mapping(PLAPIC_DEFAULT_ADDRESS)==NULL){
          kputs("    ERROR unable to memory-map processor-local APIC\r\n");
        } else {
          enable_plapic(PLAPIC_DEFAULT_ADDRESS, apic->apic_id, 0x20);
        }; //processor-local APIC defaults to this range, always local to the prcoessor you're running on
        break;
      case 1:
        ioapic = (IOAPIC *)&madt_ptr[ctr+2]; //skip over the type and length fields
        kprintf("      0x%x: IOAPIC with ID %d at address 0x%x with interrupt base %d\r\n", (vaddr)ioapic, (uint32_t)ioapic->io_apic_id, ioapic->io_apic_phys_address, (uint32_t)ioapic->global_system_interrupt_base);

        temp = (uint32_t *)&madt_ptr[ctr+4];
        kprintf("    IOAPIC id %d address should be 0x%x\r\n", (uint32_t)ioapic->io_apic_id, *temp);

        //configure_ioapic((vaddr)temp);

        if(apic_io_identity_mapping(*temp)==NULL) {
          kputs("    ERROR unable to memory-map io apic\r\n");
        }

        break;
      case 2:
        ovr = (IOAPICInterruptSourceOverride *)&madt_ptr[ctr+2];
        kprintf("      IOAPIC Interrupt source override with bus source %d, irq source %d at global system interrupt %d with flags 0x%x\r\n",
          (uint32_t)ovr->bus_source,
          (uint32_t)ovr->irq_source,
          (uint32_t)ovr->global_system_interrupt,
          (uint32_t)ovr->flags);
        break;
      case 3:
        //ctr += sizeof(IOAPICNonMaskableInterruptSource);
        break;
      case 4:
        //ctr += sizeof(LocalAPICNonMaskableInterrupt);
        break;
      case 5:
        //ctr += sizeof(LocalAPICAddressOverride);
        break;
      case 6:
        //ctr += sizeof(ProcessorLocalX2Apic);
        break;
      default:
        kprintf("ERROR Unrecognised MADT entry type %d, this will break parsing\r\n", h->type);
        return;
    }
    ctr += h->length;
    i++;
  }
}
