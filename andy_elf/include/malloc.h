#include <types.h>
#ifndef __MALLOC_H
#define __MALLOC_H

//These alloc functions are defined in heap.c
void* malloc(size_t bytes);
void* malloc_for_process(uint16_t pid, size_t bytes);
void* heap_alloc(struct HeapZoneStart *heap, size_t bytes);
//frees from the heap of the given process, which must exist.
void free_for_process(uint16_t pid, void *ptr);
//frees from the kernel heap; calls free_for_process with pid=0
void free(void* ptr);
void heap_free(struct HeapZoneStart *heap, void *ptr);
#endif
