#include <types.h>

struct BiosMemoryMap {
    uint8_t entries;
    //this is followed by `entries` x MemoryMapEntry structures
  };

//system memory map structure, from https://www.ctyme.com/intr/rb-1741.htm
struct MemoryMapEntry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_extended_attributes;  //this is probably blank.
} __attribute__((packed));

#define MMAP_TYPE_USABLE            1
#define MMAP_TYPE_RESERVED          2
#define MMAP_TYPE_ACPI_RECLAIMABLE  3
#define MMAP_TYPE_ACPI_NONVOLATILE  4
#define MMAP_TYPE_BAD               5

void initialise_mmgr();
