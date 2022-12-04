#include "heap.h"
#include <stdio.h>
#include <sys/mmgr.h>
#include <panic.h>
#include "process.h"

struct HeapZoneStart * initialise_heap(size_t initial_pages)
{
  kprintf("Initialising kernel heap at %d pages....\r\n", initial_pages);

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
    kprintf("DEBUG _expand_zone zone is continous\r\n");
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
    kprintf("DEBUG _expand_zone new memory is discontinous, making a new zone\r\n");
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

  kprintf("DEBUG allocating %d bytes into zone 0x%x...\r\n", bytes, zone);
  kprintf("DEBUG first pointer in zone at 0x%x\r\n", zone->first_ptr);

  for(struct PointerHeader *p=zone->first_ptr; p->next_ptr==NULL; p=p->next_ptr) {
    kprintf("DEBUG checking pointer block at 0x%x\r\n", p);
    if(p->magic!=HEAP_PTR_SIG) k_panic("Kernel heap corrupted\r\n");

    if(p->in_use) continue;
    //ok, this block is not in use, can we fit this alloc into it?
    if(p->block_length<bytes) continue;
    kprintf("DEBUG proceeding with alloc\r\n");
    //great, this alloc fits.
    remaining_length = p->block_length - bytes;
    kprintf("DEBUG remaining length is 0x%x (%d)\r\n", remaining_length, remaining_length);
    p->block_length = bytes+sizeof(struct PointerHeader);
    p->in_use = 1;
    zone->allocated += bytes;
    zone->dirty = 1;

    if(remaining_length>sizeof(struct PointerHeader)) {
      kprintf("DEBUG placing new block at boundary\r\n");
      //we can fit another block in the remaining space.
      kprintf("DEBUG existing block start is 0x%x. Sizeof pointer header is 0x%x and remaining length is 0x%x\r\n", p, sizeof(struct PointerHeader), remaining_length);
      new_ptr = (struct PointerHeader *)((void *)p + sizeof(struct PointerHeader) + p->block_length);
      kprintf("DEBUG new block start is at 0x%x\r\n", new_ptr);
      new_ptr->magic = HEAP_PTR_SIG;
      new_ptr->block_length = remaining_length ;
      new_ptr->in_use = 0;
      new_ptr->next_ptr = p->next_ptr;
      p->next_ptr = new_ptr;
    } else {
      kputs("INFO not enough remaining length to allocate a new block\r\n");
    }
    //return pointer to the data zone
    return (void *)p + sizeof(struct PointerHeader);
  }
  kputs("ERROR ran out of space in the zone, this should not happen\r\n");
  return NULL;
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
      return _zone_alloc(z, bytes);
    }
    if(z->next_zone==NULL) break;
    z=z->next_zone;
  }

  //if we get here, then no zones had available space. z should be set to the last zone.
  size_t pages_required = bytes / PAGE_SIZE;
  //always allocate at least MIN_ZONE_SIZE_PAGES
  size_t pages_to_alloc = pages_required > MIN_ZONE_SIZE_PAGES ? pages_required : MIN_ZONE_SIZE_PAGES;
  z = _expand_zone(z, pages_to_alloc);
  //at this point, z might be the previous zone; it might be a new one; or it might be NULL if something went wrong.
  if(!z) return NULL;
  return _zone_alloc(z, bytes);
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
