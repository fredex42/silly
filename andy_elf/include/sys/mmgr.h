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

void * k_map_page(void * phys_addr, uint16_t pagedir_idx, uint16_t pageent_idx, uint32_t flags);
void k_unmap_page(uint16_t pagedir_idx, uint16_t pageent_idx);
void* k_map_if_required(void *phys_addr, uint32_t flags);
