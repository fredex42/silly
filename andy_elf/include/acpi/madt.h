#include <types.h>
#include <acpi/sdt_header.h>

/**
This header contains defintions of the Multiple Apic Definition Table or MADT.
See https://wiki.osdev.org/MADT for details.
*/
#ifndef __ACPI_MADT_H
#define __ACPI_MADT_H

typedef struct AcpiMADT {
    ACPISDTHeader h;

    uint32_t local_apic_address;  //byte 0x24
    uint32_t local_apic_flags;
} AcpiMADT;

//Values for local_apic_flags
#define MADT_LOCAL_FLAGS_DUAL8259 1<<0  //if this is set you should mask the 8259 PIC's interrupts

typedef struct AcpiMADTProcessorLocalAPIC {
  uint8_t entry_type; //must be 0
  uint8_t length;
  uint8_t processor_id;
  uint8_t apic_id;
  uint32_t flags;
} AcpiMADTProcessorLocalAPIC;

//Values for AcpiMADTProcessorLocalAPIC flags
#define MADT_PROCESSOR_ENABLED        1<<0  //if this is 1 then enabled already
#define MADT_PROCESSOR_ONLINE_CAPABLE 1<<1  //if this is 0 don't try to enable

typedef struct AcpiMADTIOAPIC {
  uint8_t entry_type; //must be 1
  uint8_t length;
  uint8_t ioapic_id;
  uint8_t reserved;
  uint32_t ioapic_address;
  uint32_t global_system_interrupt_base; //The global system interrupt base is the first interrupt number that this I/O APIC handles.
} AcpiMADTIOAPIC;
#endif
