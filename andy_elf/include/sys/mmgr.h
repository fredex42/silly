#ifdef __BUILDING_TESTS
#include <stdint.h>
#include <sys/types.h>
#else
#include <types.h>
#endif

#ifndef __SYS_MMGR_H
#define __SYS_MMGR_H

/** gets the page directory index for the given virtual address  */
#define ADDR_TO_PAGEDIR_IDX(addr) (size_t)addr >> 22;
/** gets the page index within the page directory for the given address */
#define ADDR_TO_PAGEDIR_OFFSET(addr) ((size_t)addr >> 12) & 0x03FF

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

#define PAGE_SIZE         0x1000  //4k pages
#define PAGE_SIZE_DWORDS  0x400


//We only create mapped app pagedirs in this region.
#define APP_PAGEDIRS_BASE (vaddr)0xC0000000

/* external facing functions */
/**
initialise the memory manager, applying protections as per the BiosMemoryMap pointed to
*/
void initialise_mmgr(struct BiosMemoryMap *ptr);
/**
allocates the given number of pages of physical RAM and maps them into the memory space of the given page directory.
*/
void *vm_alloc_pages(uint32_t *root_page_dir, size_t page_count, uint32_t flags);
/**
unmaps the physical ram pages pointed to but the vmem_ptr and marks them as "free" in the physical
ram map
*/
void vm_deallocate_physical_pages(uint32_t *root_page_dir, void *vmem_ptr, size_t page_count);

/**
if the given physical page is not mapped, then map it into the directory and return
a virtual memory pointer relative to that root directory.
If it is mapped in the given directory then return the pointer to the existing mapping.
*/
void* k_map_if_required(uint32_t *base_directory, void *phys_addr, uint32_t flags);

/**
directly map a physical page into the memory space of the given page directory
*/
void * k_map_page(uint32_t *root_page_dir, void * phys_addr, uint16_t pagedir_idx, uint16_t pageent_idx, uint32_t flags);
/*
remove the given page mapping
*/
void k_unmap_page(uint32_t *root_page_dir, uint16_t pagedir_idx, uint16_t pageent_idx);
void k_unmap_page_ptr(uint32_t *root_page_dir, void *vptr);

/**
Maps the given physical address(es) into the next (contigous block of) free page of the given root page directory
Arguments:
- root_page_dir - pointer to the root page directory to use. Null will cause a crash.
- flags - MP_* flags to apply to the allocated Memory
- phys_addr - a pointer to an array of physical RAM pointers. These must be 4k aligned, and available.
- pages - the length of the phys_addr array.
Returns:
- a virtual memory pointer to the mapped region, or NULL if there were not enough pages of
virtual memory to complete or `pages` was 0.
*/
void * vm_map_next_unallocated_pages(uint32_t *root_page_dir, uint32_t flags, void **phys_addr, size_t pages);

/**
 * Maps the contents of the given paging dir into memory, on a 4mb boundary
*/
uint32_t *map_app_pagingdir(vaddr paging_dir_phys, vaddr starting_from);

/**
 * Unmaps and invalidates the given paging dir, the argument must have previously been obtained from map_app_pagingdir
*/
void unmap_app_pagingdir(uint32_t *mapped_pd);

/**
 * Check if the given page is mapped into the page-directory area
*/
uint8_t vm_is_address_present(uint32_t *mapped_pagedirs, void *ptr);

/**
 * allocates a new page of physical RAM and maps it to the given dest_vaddr
*/
void *vm_alloc_specific_page(uint32_t root_page_dir, void *dest_vaddr, uint32_t flags);

/**
 * frees all the physical RAM associated with the given app
*/
void free_app_memory(uint32_t *mapped_pd, void *root_pd_phys);

/**
 * ensures that all pages mapped into kernel-space are also backed by allocated ram.
 * if they are not, it raises a kernel panic if should_panic is non-zero; otherwise it will unmap the offending page.
*/
void validate_kernel_memory_allocations(uint8_t should_panic);

/* internal functions */
void setup_paging();
uint8_t find_next_unallocated_page(uint32_t *base_directory, int16_t *dir, int16_t *off);
/**
internal function to resolve a virtual memory pointer into a directory and page
location.
Arguments:
- vmem_ptr - pointer to resolve
- dir - pointer to 16-bit integer to receive the directory index
- off - pointer to a 16-bit integer to receive the offset within the given directory
*/
uint8_t _resolve_vptr(void *vmem_ptr, uint16_t *dir, uint16_t *off);

uint32_t *initialise_app_pagingdir(void **phys_ptr_list, size_t phys_ptr_count);

/** called from the page-fault handler for JIT allocation*/
uint8_t handle_allocation_fault(uint32_t pf_load_addr, uint32_t error_code, uint32_t faulting_addr, uint32_t faulting_codeseg, uint32_t eflags);
#endif
