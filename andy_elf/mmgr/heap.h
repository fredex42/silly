#include <types.h>
#include <sys/mmgr.h>

#ifndef __MMGR_HEAP_H
#define __MMGR_HEAP_H

#define HEAP_ZONE_SIG 0x4f5a454e
#define HEAP_PTR_SIG  0x54505220

#define MIN_ZONE_SIZE_PAGES 50
#define MIN_ZONE_SIZE_BYTES MIN_ZONE_SIZE_PAGES*PAGE_SIZE

typedef struct HeapZoneStart {
  uint32_t magic; //must be 0x4f5a454e = "ZONE"
  size_t zone_length; //zone length DOES include all headers
  size_t allocated;
  uint8_t dirty : 1;
  struct HeapZoneStart* next_zone;
  struct PointerHeader* first_ptr;
} HeapZoneStart;

typedef struct PointerHeader {
  uint32_t magic; //must be 0x54505220 = "PTR "
  size_t block_length;  //block length does NOT include this header.
  uint8_t in_use : 1;
  struct PointerHeader* next_ptr;
} PointerHeader;

struct HeapZoneStart* initialise_heap(size_t initial_pages);

//allocates onto the heap of the given process, which must exist.
void* malloc_for_process(uint16_t pid, size_t bytes);
//allocates onto the kernel heap; calls malloc_for_process with pid=0
void* malloc(size_t bytes);

//frees from the heap of the given process, which must exist.
void free_for_process(uint16_t pid, void *ptr);
//frees from the kernel heap; calls free_for_process with pid=0
void free(void* ptr);

#endif
