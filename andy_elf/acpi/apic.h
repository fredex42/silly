#include <types.h>

//useful information from the Multiple Apic Description Table or MADT
//https://wiki.osdev.org/MADT

typedef struct {
  uint8_t type; //type is the number of the struct to decode to.
  uint8_t length; //size in bytes
} MADTEntryHeader;

typedef struct {
  uint32_t local_apic_address;
  uint32_t legacy_flags;
} APICGeneralInformation;

//values for legacy_flags
#define APIC_GENERAL_LEGACY_PIC_PRESENT         1<<0

typedef struct {
  uint8_t acpi_processor_id;
  uint8_t apic_id;
  uint32_t apic_flags;
} ProcessorLocalAPIC; //type 0

//values for apic_flags
#define PROCESSOR_LOCAL_APIC_PROCESSOR_ENABLED  1<<0
#define PROCESSOR_LOCAL_APIC_CAN_ONLINE         1<<1

typedef struct {
  uint8_t io_apic_id;
  uint8_t _reserved;
  uint32_t io_apic_phys_address;  //this is provided by acpi
  uint32_t global_system_interrupt_base;  // the first interrupt number that this I/O APIC handles
} IOAPIC; //type 1

typedef struct {
  uint8_t bus_source;
  uint8_t irq_source;
  uint32_t global_system_interrupt;
  uint16_t flags;
} IOAPICInterruptSourceOverride;  //type 2

typedef struct {
  uint8_t nmi_source;
  uint8_t _reserved;
  uint16_t flags;
  uint32_t global_system_interrupt;
} IOAPICNonMaskableInterruptSource; //type 3

typedef struct {
  uint8_t acpi_processor_id;  //0xFF means "all processors"
  uint16_t flags;
  uint8_t lint_number;  //0 or 1
} LocalAPICNonMaskableInterrupt;  //type 4

typedef struct {
  uint16_t _reserved;
  uint64_t io_apic_phys_address;
} LocalAPICAddressOverride; //type 5

typedef struct {
  uint16_t _reserved;
  uint32_t x2_apic_id;
  uint32_t apic_flags;
  uint32_t acpi_id;
} ProcessorLocalX2Apic; //type 6

//values for interrupt_flags
#define INTERRUPT_ACTIVE_LOW    1<<1
#define INTERRUPT_LEVEL_TRIGGER 1<<3

//values for Loval Vector Table registers (LAPIC timer, LINT0, LINT1)
#define LVT_VECTOR_MASK 0xff          //mask for vector number
#define LVT_INTERRUPT_PENDING   1<<12
#define LVT_INTERRUPT_POLARITY  1<<13 //reserved for timer. "set"=>low trigger "unset"=>high trigger
#define LVT_REMOTE_IRR          1<<14 //reserved for timer
#define LVT_INTERRUPT_TRIGGER   1<<15 //"set"=>level "unset"=>edge
#define LVT_INTERRUPT_MASK      1<<16 //"set"=>masked

//values for Spurious Interrupt Vector register
#define SPV_VECTOR_MASK         0xff
#define SPV_APIC_ENABLED        1<<8  //set this to enable the whole apic
#define SPV_DISABLE_EOI_BCAST   1<<12 //if set, end-of-interrupt won't be broadcast

#define LAPIC_TIMER_CHOSEN_VECTOR 0x20

#define PLAPIC_DEFAULT_ADDRESS  0xFEE00000
