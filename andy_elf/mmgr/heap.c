#include "heap.h"
#include <stdio.h>
#include <sys/mmgr.h>
#include <panic.h>
#include "process.h"

/**
Note the following compilation flags to aid in mmgr debugging:
MMGR_VERBOSE  - output extra debugging information
MMGR_VALIDATE_PRE - perform a full validation on the given heap zone before an allocate or free is done
MMGR_VALIDATE_POST - perform a full validation on the given heap zone after an allocate or free is done
MMGR_PARANOID - combines MMGR_VALIDATE_PRE and MMGR_VALIDATE_POST to validate before and after validation.
*/
#ifdef MMGR_PARANOID
#define MMGR_VALIDATE_PRE
#define MMGR_VALIDATE_POST
#define MMGR_VERBOSE
#endif

struct HeapZoneStart * initialise_heap(struct ProcessTableEntry *process, size_t initial_pages)
{
  kprintf("Initialising heap at %l pages....\r\n", initial_pages);

  //NULL => kernel's paging dir. Note need MP_USER if this is not for kernel
  uint32_t flags = MP_PRESENT|MP_READWRITE;
  if(process->root_paging_directory_kmem!=NULL) flags |= MP_USER; //if not allocating for kernel then allow user-write
  void *slab = vm_alloc_pages(process->root_paging_directory_kmem, initial_pages, flags);
  if(!slab) k_panic("Unable to allocate heap!\r\n");

  kprintf("Heap start at 0x%x\r\n", slab);

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

  process->heap_start = zone;
  process->heap_allocated = zone->zone_length;

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

uint8_t validate_pointer(void *ptr, uint8_t panic)
{
  struct PointerHeader *hdr = ptr - sizeof(struct PointerHeader);
  if(hdr->magic != HEAP_PTR_SIG) {
    kprintf("ERROR pointer 0x%x is not valid, magicnumber not present\r\n", ptr);
    if(panic) {
      k_panic("Heap corruption detected");
    } else {
      return 0;
    }
  }

  if(hdr->in_use==0) {
    kprintf("ERROR pointer 0x%x is not valid, marked as not in use\r\n", ptr);
    if(panic) {
      k_panic("Heap corruption detected");
    } else {
      return 0;
    }
  }
  #ifdef MMGR_VERBOSE
  kprintf("DEBUG pointer 0x%x is valid\r\n", ptr);
  #endif
  return 1;
}

void validate_zone(struct HeapZoneStart* zone)
{
  struct PointerHeader *ptr = zone->first_ptr;
  struct PointerHeader *prev_ptr = NULL;

  size_t i=0;
  #ifdef MMGR_VERBOSE
  kprintf("> START OF ZONE\r\n");
  #endif
  while(ptr!=NULL) {
    #ifdef MMGR_VERBOSE
    kprintf(">    Zone 0x%x block index %l at 0x%x block length is 0x%x in_use %d\r\n", zone, i, ptr, ptr->block_length, (uint16_t)ptr->in_use);
    #endif
    if(prev_ptr!=NULL) {
      if((void *)prev_ptr + prev_ptr->block_length > (void*) ptr) {
        kprintf("!!!!! OVERLAPPING BLOCK DETECTED\r\n");
        k_panic("Heap corruption detected.\r\n");
      }
    }
    if(ptr->block_length > zone->zone_length) {
      kprintf("!!!   OVERLARGE BLOCK DETECTED, block 0x%x zone 0x%x\r\n", ptr->block_length, zone->allocated);
      k_panic("Heap corruption detected.\r\n");
    }
    i++;
    prev_ptr = ptr;
    ptr=ptr->next_ptr;
  }
  #ifdef MMGR_VERBOSE
  kprintf("> END OF ZONE\r\n");
  #endif
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
    zone->zone_length+=page_count*PAGE_SIZE;
    zone->dirty=1;
    struct PointerHeader* ptr = _last_pointer_of_zone(zone);
    if(ptr->in_use) { //end of the zone is in use, we should add another PointerHeader after it.

    } else { //end of the zone is not in use, we should expand the free pointer region
      #ifdef MMGR_VERBOSE
      kprintf("DEBUG _expand_zone last block not in use, expanding");
      #endif
      ptr->block_length += page_count*PAGE_SIZE;
      return zone;
    }
  } else {
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

  struct PointerHeader *p=zone->first_ptr;
  while(1) {
    if(p->magic!=HEAP_PTR_SIG) k_panic("Kernel heap corrupted\r\n");

    if(p->in_use) {
      p=p->next_ptr;
      if(p==NULL) break; else continue;
    }

    #ifdef MMGR_VERBOSE
    kprintf("DEBUG Block at 0x%x of zone 0x%x not in use.\r\n", p, zone);
    kprintf("DEBUG Free block length is 0x%x\r\n", p->block_length);
    #endif

    //ok, this block is not in use, can we fit this alloc into it?
    if(p->block_length<bytes){
      p = p->next_ptr;
      if(p==NULL) break; else continue;
    }

    //great, this alloc fits.
    //remaining_length takes us right up to the next block so we must make sure we have enough space for the buffer and the new header to go with it.

    remaining_length = p->block_length - bytes;
    p->block_length = bytes;
    p->in_use = 1;
    zone->allocated += bytes;
    zone->dirty = 1;

    if(remaining_length > 2*sizeof(struct PointerHeader)) {
      //we can fit another block in the remaining space.
      new_ptr = (struct PointerHeader *)((void *)p + sizeof(struct PointerHeader) + p->block_length);

      new_ptr->magic = HEAP_PTR_SIG;
      //the existing remaining_length does NOT include the existing header, so we should subtract 2* the header size from it
      new_ptr->block_length = remaining_length - 2*sizeof(struct PointerHeader);
      new_ptr->in_use = 0;
      new_ptr->next_ptr = p->next_ptr;
      p->next_ptr = new_ptr;
    }

    //return pointer to the data zone
    void *ptr= (void *)p + sizeof(struct PointerHeader);
    #ifdef MMGR_VERBOSE
    kprintf("DEBUG Success, block start is 0x%x and ptr is 0x%x\r\n", p, ptr);
    #endif

    return ptr;
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
    #ifdef MMGR_VERBOSE
    kprintf("DEBUG Pointer 0x%x following 0x%x is free too, coalescing\r\n", ptr_info->next_ptr, ptr_info);
    #endif
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
  #ifdef MMGR_VERBOSE
  kprintf("INFO Freed block 0x%x\r\n", ptr - sizeof(struct PointerHeader));
  #endif
  #ifdef MMGR_VALIDATE_POST
  validate_zone(zone);
  #endif
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
    #ifdef MMGR_VERBOSE
    kprintf("DEBUG heap_alloc space is %l / %l for zone 0x%x\r\n", z->allocated, z->zone_length, z);
    #endif

    if(z->zone_length - z->allocated > bytes) {
      #ifdef MMGR_VALIDATE_PRE
      kprintf("DEBUG !! Pre-alloc\r\n");
      validate_zone(z);
      #endif
      void* result = _zone_alloc(z, bytes);
      #ifdef MMGR_VALIDATE_POST
      kprintf("DEBUG !! Post-alloc\r\n");
      validate_zone(z);
      #endif
      #ifdef MMGR_VERBOSE
      kprintf("INFO heap_alloc used space in zone 0x%x is now 0x%x (%l) out of 0x%x\r\n", z, z->allocated, z->allocated, z->zone_length);
      #endif
      return result;
    }
    if(z->next_zone==NULL) break;
    z=z->next_zone;
  }

  #ifdef MMGR_VERBOSE
  kputs("DEBUG No existing heap zones had space, expanding last zone\r\n");
  #endif
  //if we get here, then no zones had available space. z should be set to the last zone.
  size_t pages_required = bytes / PAGE_SIZE;
  //always allocate at least MIN_ZONE_SIZE_PAGES
  size_t pages_to_alloc = pages_required > MIN_ZONE_SIZE_PAGES ? pages_required : MIN_ZONE_SIZE_PAGES;
  z = _expand_zone(z, pages_to_alloc);
  //at this point, z might be the previous zone; it might be a new one; or it might be NULL if something went wrong.
  if(!z) return NULL;
  void* result = _zone_alloc(z, bytes);
  #ifdef MMGR_VERBOSE
  kprintf("INFO Allocated block 0x%x\r\n", result - sizeof(struct PointerHeader));
  #endif
  #ifdef MMGR_VALIDATE_POST
  validate_zone(z);
  #endif
  #ifdef MMGR_VERBOSE
  kprintf("INFO heap_alloc used space in zone 0x%x is now 0x%x (%l) out of 0x%x\r\n", z, z->allocated, z->allocated, z->zone_length);
  #endif
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
