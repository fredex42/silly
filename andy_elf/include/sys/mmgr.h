#ifdef __BUILDING_TESTS
#include <stdint.h>
#include <sys/types.h>
#else
#include <types.h>
#endif

#define MP_PRESENT    1 << 0  //if this is 0 then accessing the page raises a page fault for swap-in
#define MP_READWRITE  1 << 1  //if this is 0 then page is read-only, if it's 1 then read-write
#define MP_USER       1 << 2  //if this is 0 then supervisor access only, if it's 1 then any ring
#define MP_PWT        1 << 3  //if this is 0 then use write-back caching, if not then use write-through
#define MP_PCD        1 << 4  //if this is 0 then the page will not be cached
#define MP_ACCESSED   1 << 5  //if this is 0 then the page has not been accessed, if 1 then it has
#define MP_DIRTY      1 << 6  //if this is 0 then the page has not been written, if 1 then it has
#define MP_PAGESIZE   1 << 7  //if this is 0 then the address refers to a page table, otherwise to a 4Mib block
#define MP_GLOBAL     1 << 8  //don't invalidate when CR3 changes if set
#define MP_PAGEATTRIBUTE 1 << 12 //1 if Page Attribute Table is supported. Keep to 0

#define MP_OSBITS_MASK 0xF00  //bitmask for the 3 os-dependent bits
#define MP_ADDRESS_MASK 0xFFFFF000

#define PAGE_SIZE     0x1000  //4k pages

void * k_map_page(uint32_t *root_page_dir, void * phys_addr, uint16_t pagedir_idx, uint16_t pageent_idx, uint32_t flags);
void k_unmap_page(uint16_t pagedir_idx, uint16_t pageent_idx);
void* k_map_if_required(uint32_t *base_directory, void *phys_addr, uint32_t flags);
uint8_t find_next_unallocated_page(uint32_t *base_directory, int16_t *dir, int16_t *off);
void setup_paging();

/**
data structure for a memory region obtained from INT 0x15, EAX = 0xE820
See https://wiki.osdev.org/Detecting_Memory_(x86)#Getting_an_E820_Memory_Map
*/
struct MemoryMapEntry {
  uint64_t base_addr;
  uint64_t length;      //if this is 0 then ignore
  uint32_t type;        //see type defines below
  uint32_t acpi_extended_attributes;  //this is probably blank.
};

#define MMAP_TYPE_USABLE            1
#define MMAP_TYPE_RESERVED          2
#define MMAP_TYPE_ACPI_RECLAIMABLE  3
#define MMAP_TYPE_ACPI_NONVOLATILE  4
#define MMAP_TYPE_BAD               5

struct BiosMemoryMap {
  uint8_t entries;
  //this is followed by `entries` x MemoryMapEntry structures
};

/** our own physical memory map, unconstrained by hardware requirements
*/
struct PhysMapEntry {
  int present:1;
  int in_use:1;
} __attribute__((packed));
