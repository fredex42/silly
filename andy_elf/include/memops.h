#include <types.h>

void *memset(void *s, uint8_t c, size_t n);
void *memset_dw(void *s, uint32_t c, size_t n_dwords);
void *memcpy(void *dest, const void *src, size_t n);
void *memcpy_dw(void *dest, const void *src, size_t n_dwords);

void mb();  //memory barrier

void *get_current_paging_directory();
vaddr switch_paging_directory_if_required(vaddr directory_ptr);
