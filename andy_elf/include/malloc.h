#include <types.h>
#ifndef __MALLOC_H
#define __MALLOC_H

//These alloc functions are defined in heap.c
void* malloc(size_t bytes);
void* malloc_for_process(uint16_t pid, size_t bytes);
void* heap_alloc(struct HeapZoneStart *heap, size_t bytes);

#endif