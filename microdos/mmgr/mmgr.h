#include <types.h>
#include <errors.h>

//system memory map structure, from https://www.ctyme.com/intr/rb-1741.htm
struct MemoryMapEntry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_extended_attributes;  //this is probably blank.
} __attribute__((packed));

/** our own physical memory map, unconstrained by hardware requirements
*/
struct PhysMapEntry {
  int present:1;
  int in_use:1;
} __attribute__((packed));

#define MMAP_TYPE_USABLE            1
#define MMAP_TYPE_RESERVED          2
#define MMAP_TYPE_ACPI_RECLAIMABLE  3
#define MMAP_TYPE_ACPI_NONVOLATILE  4
#define MMAP_TYPE_BAD               5

//page-protection flags
#define MP_PRESENT    1 << 0  //if this is 0 then accessing the page raises a page fault for swap-in
#define MP_READWRITE  1 << 1  //if this is 0 then page is read-only, if it's 1 then read-write
#define MP_USER       1 << 2  //if this is 0 then supervisor access only, if it's 1 then any ring
#define MP_PWT        1 << 3  //if this is 0 then use write-back caching, if not then use write-through
#define MP_PCD        1 << 4  //if this is 0 then the page will not be cached
#define MP_ACCESSED   1 << 5  //if this is 0 then the page has not been accessed, if 1 then it has
#define MP_DIRTY      1 << 6  //if this is 0 then the page has not been written, if 1 then it has
#define MP_PAGESIZE   1 << 7  //if this is 0 then the address refers to a page table, otherwise to a 4Mib block
#define MP_GLOBAL     1 << 8  //don't invalidate when CR3 changes if set

#define MPC_PAGINGDIR    1 << 9  //custom attribute - if this is 1 then the page is a paging directory, i.e. not present but can be mapped in
#define MP_PAGEATTRIBUTE 1 << 12 //if Page Attribute Table is supported, forms a 3-bit index value with MP_PWT and MP_PCD

#define MP_OSBITS_MASK 0xF00  //bitmask for the 3 os-dependent bits
#define MP_ADDRESS_MASK 0xFFFFF000

//memory page sizes
#define PAGE_SIZE         0x1000  //4k pages
#define PAGE_SIZE_DWORDS  0x400

void initialise_mmgr();
void allocate_paging_directories(size_t highest_value, size_t physical_map_start, uint32_t **root_paging_dir_out, size_t *pages_used_out);
void allocate_physical_map(size_t highest_value, size_t *area_start_out, size_t *map_length_in_pages_out);
size_t highest_free_memory();
void activate_paging(uint32_t *root_paging_dir);
void debug_dump_used_memory();
void setup_physical_map(uint32_t first_page_of_pagetables, size_t pages_used_for_paging);
err_t relocate_kernel(uint32_t new_base_addr);
err_t find_next_free_pages(uint32_t optional_start_addr, uint32_t optional_end_addr, uint32_t page_count, uint8_t should_reverse, uint32_t *out_block_start);

//return the classic paging directory for a 32-bit address
#define CLASSIC_PAGEDIR(addr) (uint32_t)addr >> 22
//return the classic paging table for a 32-bit address
#define CLASSIC_PAGETABLE(addr) ((uint32_t)addr >> 12) & 0x3FF
//turn a page and table into a 32-bit address
#define CLASSIC_ADDRESS(page, table) (void*)((page << 22) | (table << 12))
//get the full index of a page from its address
#define CLASSIC_PAGE_INDEX(addr) (uint32_t)addr >> 12
#define CLASSIC_ADDRESS_FROM_INDEX(idx) (uint32_t)idx << 12