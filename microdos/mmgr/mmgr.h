#include <types.h>

//system memory map structure, from https://www.ctyme.com/intr/rb-1741.htm
struct MemoryMapEntry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
};

#define MEM_TYPE_AVAILABLE          0x01
#define MEM_TYPE_RESERVED           0x02
#define MEM_TYPE ACPI_RECLAIMABLE   0x03
#define MEM_TYPE_ACPI_NVS           0x04

void initialise_mmgr();
