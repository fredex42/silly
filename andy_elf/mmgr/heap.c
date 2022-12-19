#include "heap.h"
#include <stdio.h>
#include <sys/mmgr.h>
#include <panic.h>
#include "process.h"

struct HeapZoneStart * initialise_heap(size_t initial_pages)
{
  kprintf("Initialising kernel heap at %l pages....\r\n", initial_pages);

  //NULL => kernel's paging dir. Note need MP_USER if this is not for kernel
  void *slab = vm_alloc_pages(NULL, initial_pages, MP_PRESENT|MP_READWRITE);
  if(!slab) k_panic("Unable to allocate kernel heap!\r\n");

  kprintf("Kernel heap start at 0x%x\r\n", slab);

  struct HeapZoneStart* zone = (struct HeapZoneStart *)slab;
  zone->magic = HEAP_ZONE_SIG;
  zone->zone_length = initial_pages*PAGE_SIZE;
  zone->allocated = 0;
  zone->dirty = 0;
  zone->next_zone=NULL;

  struct PointerHeader* ptr = (struct PointerHeader *)((char *)(slab + sizeof(struct HeapZoneStart)));
  ptr->magic = HEAP_PTR_SIG;
  ptr->block_length = zone->zone_length - sizeof(struct HeapZoneStart) - sizeof(struct PointerHeader);
  ptr->in_use=0;
  ptr->next_ptr = NULL;

  zone->first_ptr = ptr;

  struct ProcessTableEntry *kernel_process = get_process(0);
  kernel_process->heap_start = zone;
  kernel_process->heap_allocated = zone->zone_length;

  return zone;
}

struct PointerHeader* _last_pointer_of_zone(struct HeapZoneStart* zone)
{
  if(zone->magic != HEAP_ZONE_SIG) k_panic("Kernel heap is corrupted\r\n");

  struct PointerHeader *ptr = zone->first_ptr;
  while(ptr->next_ptr!=NULL) {
    if(ptr->magic != HEAP_PTR_SIG) k_panic("Kernel heap is corrupted\r\n");
    ptr=ptr->next_ptr;
  }
  return ptr;
}

/**
Allocates page_count more RAM pages. If they are continuous with the given heap zone,
then they are added onto it.  Otherwise, a new heap zone is created.
Whichever zone the new memory is in is then returned.
Can return NULL if allocation fails.
*/
struct HeapZoneStart* _expand_zone(struct HeapZoneStart* zone, size_t page_count)
{
  if(zone->magic!=HEAP_ZONE_SIG) k_panic("Kernel memory heap is corrupted\r\n");

  void *slab = vm_alloc_pages(NULL, page_count, MP_PRESENT|MP_READWRITE);
  if(!slab) return NULL;

  if((char*)zone + zone->zone_length == (char *)slab) {
    //kprintf("DEBUG _expand_zone zone is continous\r\n");
    zone->zone_length+=page_count*PAGE_SIZE;
    zone->dirty=1;
    struct PointerHeader* ptr = _last_pointer_of_zone(zone);
    if(ptr->in_use) { //end of the zone is in use, we should add another PointerHeader after it.

    } else { //end of the zone is not in use, we should expand the free pointer region
      kprintf("DEBUG _expand_zone last block not in use, expanding");
      ptr->block_length += page_count*PAGE_SIZE;
      return zone;
    }
  } else {
    //kprintf("DEBUG _expand_zone new memory is discontinous, making a new zone\r\n");
    struct HeapZoneStart *new_zone = (struct HeapZoneStart*)slab;
    new_zone->magic = HEAP_ZONE_SIG;
    new_zone->zone_length = page_count*PAGE_SIZE;
    new_zone->allocated = 0;
    new_zone->dirty = 0;
    new_zone->next_zone=NULL;
    zone->next_zone = new_zone;

    struct PointerHeader* ptr = (struct PointerHeader *)((char *)(slab + sizeof(struct HeapZoneStart)));
    ptr->magic = HEAP_PTR_SIG;
    ptr->block_length = zone->zone_length - sizeof(struct HeapZoneStart);
    ptr->in_use=0;
    ptr->next_ptr = NULL;

    new_zone->first_ptr = ptr;
    return new_zone;
  }
}
/**
Allocate a new pointer into the given zone.
This assumes that there is sufficient space in the given zone.
*/
void* _zone_alloc(struct HeapZoneStart *zone, size_t bytes)
{
  size_t remaining_length = 0;
  struct PointerHeader* new_ptr=NULL;

  //kprintf("DEBUG allocating %l bytes into zone 0x%x...\r\n", bytes, zone);
  //if(bytes > 1024*1024*1024) k_panic("allocation attempt over 1Gb, this must be a bug\r\n");

  struct PointerHeader *p=zone->first_ptr;
  while(1) {
    if(p->magic!=HEAP_PTR_SIG) k_panic("Kernel heap corrupted\r\n");

    if(p->in_use) {
      p=p->next_ptr;
      if(p==NULL) break; else continue;
    }
    //ok, this block is not in use, can we fit this alloc into it?
    if(p->block_length<bytes){
      p = p->next_ptr;
      if(p==NULL) break; else continue;
    }

    //kprintf("DEBUG proceeding with alloc\r\n");
    //great, this alloc fits.
    remaining_length = p->block_length - bytes;
    //kprintf("DEBUG remaining length is 0x%x (%l)\r\n", remaining_length, remaining_length);
    p->block_length = bytes + sizeof(struct PointerHeader);
    p->in_use = 1;
    zone->allocated += bytes;
    zone->dirty = 1;

    if(remaining_length>sizeof(struct PointerHeader)) {
      //we can fit another block in the remaining space.
      new_ptr = (struct PointerHeader *)((void *)p + sizeof(struct PointerHeader) + p->block_length);
      new_ptr->magic = HEAP_PTR_SIG;
      new_ptr->block_length = remaining_length + sizeof(struct PointerHeader);
      new_ptr->in_use = 0;
      new_ptr->next_ptr = p->next_ptr;
      p->next_ptr = new_ptr;
    } else {
      //kputs("INFO not enough remaining length to allocate a new block\r\n");
    }
    //return pointer to the data zone
    return (void *)p + sizeof(struct PointerHeader);
  }
  kputs("ERROR ran out of space in the zone, this should not happen\r\n");
  return NULL;
}

/**
De-allocate the pointer in the given zone.
*/
void _zone_dealloc(struct HeapZoneStart *zone, void* ptr)
{
  struct PointerHeader* ptr_info = (struct PointerHeader*)((void*) ptr - sizeof(struct PointerHeader));
  if(ptr_info->magic != HEAP_PTR_SIG) {
    kprintf("ERROR Could not free pointer at 0x%x in zone 0x%x, heap may be corrupted\r\n", ptr, zone);
    k_panic("Heap corruption detected\r\n");
  }

  ptr_info->in_use = 0;
  zone->allocated -= ptr_info->block_length;

  if(ptr_info->next_ptr && ptr_info->next_ptr->in_use==0) {
    //there is a following block. If this is also free, then we should coalesce them to fight fragmentation.
    //kprintf("DEBUG Pointer 0x%x following 0x%x is free too, coalescing\r\n", ptr_info->next_ptr, ptr_info);
    if(ptr_info->next_ptr->magic != HEAP_PTR_SIG) {
      kprintf("ERROR Pointer at 0x%x following 0x%x appears corrupted\r\n", ptr_info->next_ptr, ptr_info);
      return;
    }
    struct PointerHeader* ptr_to_remove = ptr_info->next_ptr;
    ptr_info->next_ptr = ptr_to_remove->next_ptr;
    ptr_info->block_length += ptr_to_remove->block_length;
    memset(ptr_to_remove, 0, sizeof(struct PointerHeader));
  }
}

/**
Find the zone that this ptr belongs to.
*/
struct HeapZoneStart* _zone_for_ptr(struct HeapZoneStart *heap,void* ptr)
{
  if(heap->magic != HEAP_ZONE_SIG) k_panic("Kernel heap corrupted\r\n");
  void* zone_start = (void*)heap;
  void* zone_end = (void*)heap + heap->zone_length;
  if(ptr>zone_start && ptr<zone_end) return heap; //this is the zone that contains the pointer
  if(heap->next_zone) return _zone_for_ptr(heap->next_zone, ptr); //recurse on to the next zone
  return NULL;  //we reached the end of the whole heap and did not find it.
}

/**
Frees the pointer from the given heap
*/
void heap_free(struct HeapZoneStart *heap, void* ptr)
{
  struct HeapZoneStart* zone = _zone_for_ptr(heap, ptr);
  if(!zone) {
    kprintf("ERROR Attempted to free pointer 0x%x from heap 0x%x which it does not belong to\r\n", ptr, heap);
    return;
  }
  _zone_dealloc(zone, ptr);
  //kprintf("INFO heap_free used space in zone 0x%x is now 0x%x (%l) out of 0x%x\r\n", zone, zone->allocated, zone->allocated, zone->zone_length);
}

/**
Allocate a new pointer in the given heap.
This will try to find a zone with enough space in it, and if not
will try to expand the heap.
*/
void* heap_alloc(struct HeapZoneStart *heap, size_t bytes)
{
  struct HeapZoneStart* z=heap;
  while(1) {
    if(z->magic!=HEAP_ZONE_SIG) k_panic("Kernel heap corrupted\r\n");
    if(z->zone_length - z->allocated > bytes) {
      void* result = _zone_alloc(z, bytes);
      //kprintf("INFO heap_alloc used space in zone 0x%x is now 0x%x (%l) out of 0x%x\r\n", z, z->allocated, z->allocated, z->zone_length);
      return result;
    }
    if(z->next_zone==NULL) break;
    z=z->next_zone;
  }

  kputs("DEBUG No existing heap zones had space, expanding last zone\r\n");
  //if we get here, then no zones had available space. z should be set to the last zone.
  size_t pages_required = bytes / PAGE_SIZE;
  //always allocate at least MIN_ZONE_SIZE_PAGES
  size_t pages_to_alloc = pages_required > MIN_ZONE_SIZE_PAGES ? pages_required : MIN_ZONE_SIZE_PAGES;
  z = _expand_zone(z, pages_to_alloc);
  //at this point, z might be the previous zone; it might be a new one; or it might be NULL if something went wrong.
  if(!z) return NULL;
  void* result = _zone_alloc(z, bytes);
  //kprintf("INFO heap_alloc used space in zone 0x%x is now 0x%x (%l) out of 0x%x\r\n", z, z->allocated, z->allocated, z->zone_length);
  return result;
}

void* malloc_for_process(uint16_t pid, size_t bytes)
{
  struct ProcessTableEntry *process = get_process(pid);
  if(!process) return NULL;
  if(process->status==PROCESS_NONE) return NULL;  //don't alloc a non-existent process
  return heap_alloc(process->heap_start, bytes);
}

//allocates onto the kernel heap
void* malloc(size_t bytes)
{
    return malloc_for_process(0, bytes);
}

void free_for_process(uint16_t pid, void *ptr)
{
  struct ProcessTableEntry *process = get_process(pid);
  if(!process) return;
  if(process->status==PROCESS_NONE) return;  //don't alloc a non-existent process
  return heap_free(process->heap_start, ptr);
}

void free(void* ptr)
{
  return free_for_process(0, ptr);
}
