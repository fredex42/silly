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

  // Validate parameters
  if(initial_pages == 0) {
    kprintf("ERROR Cannot initialize heap with 0 pages\r\n");
    return NULL;
  }
  
  // Check for integer overflow in page calculation
  if(initial_pages > (size_t)-1/PAGE_SIZE) {
    kprintf("ERROR Initial pages %l would cause integer overflow\r\n", initial_pages);
    return NULL;
  }
  
  size_t zone_size = initial_pages * PAGE_SIZE;
  if(zone_size < initial_pages || zone_size < PAGE_SIZE) {
    kprintf("ERROR Integer overflow in zone size calculation\r\n");
    return NULL;
  }
  
  // Ensure we have enough space for headers
  if(zone_size < sizeof(struct HeapZoneStart) + sizeof(struct PointerHeader)) {
    kprintf("ERROR Zone too small for required headers\r\n");
    return NULL;
  }

  //NULL => kernel's paging dir. Note need MP_USER if this is not for kernel
  uint32_t flags = MP_PRESENT|MP_READWRITE;
  if(process->root_paging_directory_kmem!=NULL) flags |= MP_USER; //if not allocating for kernel then allow user-write
  void *slab = vm_alloc_pages(process->root_paging_directory_kmem, initial_pages, flags);
  if(!slab) k_panic("Unable to allocate heap!\r\n");

  kprintf("INFO Heap start at 0x%x\r\n", slab);

  struct HeapZoneStart* zone = (struct HeapZoneStart *)slab;
  zone->magic = HEAP_ZONE_SIG;
  zone->zone_length = zone_size;
  zone->allocated = 0;
  zone->dirty = 0;
  zone->next_zone=NULL;

  struct PointerHeader* ptr = (struct PointerHeader *)((char *)slab + sizeof(struct HeapZoneStart));
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
  #ifdef MMGR_VERBOSE
  kprintf("DEBUG validate_pointer checking 0x%x\r\n", ptr);
  #endif
  
  if(ptr==NULL) {
    if(panic) {
      k_panic("Attempt to check a null pointer from the heap");
    } else {
      return 0;
    }
  }
  struct PointerHeader *hdr = (struct PointerHeader *)((char *)ptr - sizeof(struct PointerHeader));
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
  if(zone->magic != HEAP_ZONE_SIG) {
    kprintf("!!!!! ZONE MAGIC NUMBER INVALID: expected 0x%x, got 0x%x\r\n", HEAP_ZONE_SIG, zone->magic);
    k_panic("Heap corruption detected.\r\n");
  }
  
  struct PointerHeader *ptr = zone->first_ptr;
  struct PointerHeader *prev_ptr = NULL;
  
  // Validate that first_ptr is within zone bounds
  if((void*)ptr < (void*)zone || (void*)ptr >= (void*)((char*)zone + zone->zone_length)) {
    kprintf("!!!!! FIRST POINTER 0x%x OUT OF ZONE BOUNDS\r\n", ptr);
    k_panic("Heap corruption detected.\r\n");
  }

  size_t i=0;
  size_t total_accounted = sizeof(struct HeapZoneStart);
  #ifdef MMGR_VERBOSE
  kprintf("> START OF ZONE 0x%x length 0x%x\r\n", zone, zone->zone_length);
  #endif
  while(ptr!=NULL) {
    // Validate pointer magic number
    if(ptr->magic != HEAP_PTR_SIG) {
      kprintf("!!!!! POINTER MAGIC NUMBER INVALID at 0x%x: expected 0x%x, got 0x%x\r\n", ptr, HEAP_PTR_SIG, ptr->magic);
      k_panic("Heap corruption detected.\r\n");
    }
    
    // Validate pointer is within zone bounds
    if((void*)ptr < (void*)zone || (void*)ptr >= (void*)((char*)zone + zone->zone_length)) {
      kprintf("!!!!! POINTER 0x%x OUT OF ZONE BOUNDS\r\n", ptr);
      k_panic("Heap corruption detected.\r\n");
    }
    
    #ifdef MMGR_VERBOSE
    kprintf(">    Zone 0x%x block index %l at 0x%x block length is 0x%x in_use %d\r\n", zone, i, ptr, ptr->block_length, (uint16_t)ptr->in_use);
    #endif
    
    if(prev_ptr!=NULL) {
      // Calculate end of previous block including its header and data
      size_t prev_block_end = (size_t)prev_ptr + sizeof(struct PointerHeader) + prev_ptr->block_length;
      // Check for integer overflow
      if(prev_block_end < (size_t)prev_ptr || prev_block_end < prev_ptr->block_length) {
        kprintf("!!!!! INTEGER OVERFLOW in previous block end calculation\r\n");
        k_panic("Heap corruption detected.\r\n");
      }
      // Check that blocks don't overlap and are properly aligned
      if(prev_block_end > (size_t)ptr) {
        kprintf("!!!!! OVERLAPPING BLOCK DETECTED: prev ends at 0x%x, current starts at 0x%x\r\n", prev_block_end, ptr);
        k_panic("Heap corruption detected.\r\n");
      }
    }
    
    // Validate block length is reasonable
    if(ptr->block_length == 0) {
      kprintf("!!!!! ZERO-SIZED BLOCK DETECTED at 0x%x\r\n", ptr);
      k_panic("Heap corruption detected.\r\n");
    }
    
    if(ptr->block_length > zone->zone_length) {
      kprintf("!!!   OVERLARGE BLOCK DETECTED, block 0x%x zone length 0x%x\r\n", ptr->block_length, zone->zone_length);
      k_panic("Heap corruption detected.\r\n");
    }
    
    // Validate that block + header doesn't extend beyond zone
    size_t block_end = (size_t)ptr + sizeof(struct PointerHeader) + ptr->block_length;
    if(block_end < (size_t)ptr || block_end < ptr->block_length) {
      kprintf("!!!!! INTEGER OVERFLOW in block end calculation\r\n");
      k_panic("Heap corruption detected.\r\n");
    }
    if(block_end > (size_t)zone + zone->zone_length) {
      kprintf("!!!!! BLOCK EXTENDS BEYOND ZONE: block ends at 0x%x, zone ends at 0x%x\r\n", 
              block_end, (size_t)zone + zone->zone_length);
      k_panic("Heap corruption detected.\r\n");
    }
    
    // Keep track of total accounted space
    total_accounted += sizeof(struct PointerHeader) + ptr->block_length;
    
    i++;
    prev_ptr = ptr;
    ptr=ptr->next_ptr;
  }
  
  // Validate that we've accounted for all space in the zone
  if(total_accounted > zone->zone_length) {
    kprintf("!!!!! ACCOUNTING ERROR: total accounted 0x%x > zone length 0x%x\r\n", 
            total_accounted, zone->zone_length);
    k_panic("Heap corruption detected.\r\n");
  }
  
  #ifdef MMGR_VERBOSE
  kprintf("> END OF ZONE, validated %l blocks, accounted 0x%x / 0x%x bytes\r\n", i, total_accounted, zone->zone_length);
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

  // Check for integer overflow in page calculations
  if(page_count > (size_t)-1/PAGE_SIZE - 1) {
    kprintf("ERROR Page count %l would cause integer overflow\r\n", page_count);
    return NULL;
  }
  
  size_t expansion_size = (page_count+1)*PAGE_SIZE;
  if(expansion_size < page_count || expansion_size < PAGE_SIZE) {
    kprintf("ERROR Integer overflow in expansion size calculation\r\n");
    return NULL;
  }

  //Add 1 to the page count, because if we allocate a new zone then we need space for the header information too!
  void *slab = vm_alloc_pages(NULL, page_count+1, MP_PRESENT|MP_READWRITE);
  if(!slab) return NULL;

  if((char*)zone + zone->zone_length == (char *)slab) {
    // Check for overflow when expanding zone length
    if(zone->zone_length > (size_t)-1 - expansion_size) {
      kprintf("ERROR Zone length expansion would overflow\r\n");
      return NULL;
    }
    zone->zone_length += expansion_size;
    zone->dirty=1;
    struct PointerHeader* ptr = _last_pointer_of_zone(zone);
    if(ptr->in_use) { 
      // End of the zone is in use, we should add another PointerHeader after it.
      // Calculate where the new header would go
      struct PointerHeader* new_ptr = (struct PointerHeader *)((char*)ptr + sizeof(struct PointerHeader) + ptr->block_length);
      
      // Validate the new header location
      if((char*)new_ptr + sizeof(struct PointerHeader) > (char*)zone + zone->zone_length) {
        kprintf("ERROR New header would extend beyond zone boundary\r\n");
        return NULL;
      }
      
      new_ptr->magic = HEAP_PTR_SIG;
      new_ptr->block_length = expansion_size - sizeof(struct PointerHeader);
      new_ptr->in_use = 0;
      new_ptr->next_ptr = NULL;
      ptr->next_ptr = new_ptr;
      return zone;
    } else { //end of the zone is not in use, we should expand the free pointer region
      #ifdef MMGR_VERBOSE
      kprintf("DEBUG _expand_zone last block not in use, expanding");
      #endif
      // Check for overflow when expanding block length
      if(ptr->block_length > (size_t)-1 - expansion_size) {
        kprintf("ERROR Block length expansion would overflow\r\n");
        return NULL;
      }
      ptr->block_length += expansion_size;
      return zone;
    }
  } else {
    #ifdef MMGR_VERBOSE
    kputs("DEBUG _expand_zone memory blocks are not continous, creating a new zone\r\n");
    #endif
    struct HeapZoneStart *new_zone = (struct HeapZoneStart*)slab;
    new_zone->magic = HEAP_ZONE_SIG;
    new_zone->zone_length = expansion_size;
    new_zone->allocated = 0;
    new_zone->dirty = 0;
    new_zone->next_zone=NULL;
    zone->next_zone = new_zone;

    struct PointerHeader* ptr = (struct PointerHeader *)((char *)slab + sizeof(struct HeapZoneStart));
    ptr->magic = HEAP_PTR_SIG;
    
    // Validate that we have space for both the zone header and pointer header
    if(new_zone->zone_length < sizeof(struct HeapZoneStart) + sizeof(struct PointerHeader)) {
      kprintf("ERROR New zone too small for headers\r\n");
      return NULL;
    }
    
    ptr->block_length = new_zone->zone_length - sizeof(struct HeapZoneStart) - sizeof(struct PointerHeader);
    ptr->in_use=0;
    ptr->next_ptr = NULL;

    new_zone->first_ptr = ptr;
    kprintf("DEBUG _expand_zone new zone is at 0x%x and size is 0x%x\r\n", new_zone, new_zone->zone_length);
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

  #ifdef MMGR_VERBOSE
  kprintf("DEBUG _zone_alloc request for %l (0x%x) bytes in zone 0x%x\r\n", bytes, bytes, zone);
  #endif

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
      new_ptr = (struct PointerHeader *)((size_t)p + sizeof(struct PointerHeader) + p->block_length);
      if((size_t)new_ptr < (size_t)p) { //check for integer overflow
        kprintf("ERROR calculated new pointer 0x%x is before the existing pointer 0x%x\r\n", new_ptr, p);
        k_panic("Heap corruption detected\r\n");
      }
      if((size_t)new_ptr + sizeof(struct PointerHeader) > (size_t)zone + zone->zone_length) {
        kprintf("ERROR calculated new pointer 0x%x + header size would extend outside zone 0x%x + 0x%x\r\n", new_ptr, zone, zone->zone_length);
        k_panic("Heap corruption detected\r\n");
      }
      new_ptr->magic = HEAP_PTR_SIG;
      //the existing remaining_length does NOT include the existing header, so we should subtract 2* the header size from it
      if(remaining_length < 2*sizeof(struct PointerHeader)) {
        kprintf("ERROR insufficient remaining space 0x%x for two headers 0x%x\r\n", remaining_length, 2*sizeof(struct PointerHeader));
        k_panic("Heap corruption detected\r\n");
      }
      new_ptr->block_length = remaining_length - 2*sizeof(struct PointerHeader);
      if(new_ptr->block_length == 0 || new_ptr->block_length > remaining_length) { //check for underflow or overflow
        kprintf("ERROR calculated new pointer 0x%x has invalid block length 0x%x - integer over/underflow\r\n", new_ptr, new_ptr->block_length);
        k_panic("Heap corruption detected\r\n");
      }
      new_ptr->in_use = 0;
      new_ptr->next_ptr = p->next_ptr;
      p->next_ptr = new_ptr;
    }

    //return pointer to the data zone
    void *ptr= (void *)((size_t)p + sizeof(struct PointerHeader));
    #ifdef MMGR_VERBOSE
    kprintf("DEBUG Success, block start is 0x%x and ptr is 0x%x\r\n", p, ptr);
    #endif
    
    // Extra validation for large allocations
    // if(bytes > 0x100000) { // 1MB+
    //   kprintf("DEBUG Large allocation: ptr=0x%x, size=0x%x, zone=0x%x\r\n", ptr, bytes, zone);
    //   kprintf("DEBUG Block header at 0x%x: magic=0x%x, block_length=0x%x, in_use=%d\r\n", 
    //           p, p->magic, p->block_length, p->in_use);
    //   if(new_ptr) {
    //     kprintf("DEBUG Next block header at 0x%x: magic=0x%x, block_length=0x%x, in_use=%d\r\n",
    //             new_ptr, new_ptr->magic, new_ptr->block_length, new_ptr->in_use);
    //   }
    // }

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
  // Validate that the pointer is within the zone boundaries
  if(ptr <= (void*)zone || ptr >= (void*)((char*)zone + zone->zone_length)) {
    kprintf("ERROR Pointer 0x%x is not within zone boundaries 0x%x to 0x%x\r\n", ptr, zone, (char*)zone + zone->zone_length);
    k_panic("Heap corruption detected\r\n");
  }
  
  struct PointerHeader* ptr_info = (struct PointerHeader*)((char*) ptr - sizeof(struct PointerHeader));
  
  // Ensure the header is also within zone boundaries
  if((void*)ptr_info < (void*)zone || (void*)ptr_info >= (void*)((char*)zone + zone->zone_length)) {
    kprintf("ERROR Pointer header 0x%x is not within zone boundaries\r\n", ptr_info);
    k_panic("Heap corruption detected\r\n");
  }
  
  if(ptr_info->magic != HEAP_PTR_SIG) {
    kprintf("ERROR Could not free pointer at 0x%x in zone 0x%x, heap may be corrupted\r\n", ptr, zone);
    k_panic("Heap corruption detected\r\n");
  }
  
  if(ptr_info->in_use == 0) {
    kprintf("ERROR Double-free detected: pointer 0x%x is already free\r\n", ptr);
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
    
    // Check for overflow when adding block lengths
    size_t total_block_length = ptr_info->block_length + ptr_to_remove->block_length + sizeof(struct PointerHeader);
    if(total_block_length < ptr_info->block_length || total_block_length < ptr_to_remove->block_length) {
      kprintf("ERROR Integer overflow when coalescing blocks 0x%x + 0x%x + 0x%x\r\n", 
              ptr_info->block_length, ptr_to_remove->block_length, sizeof(struct PointerHeader));
      k_panic("Heap corruption detected\r\n");
    }
    
    ptr_info->next_ptr = ptr_to_remove->next_ptr;
    ptr_info->block_length = total_block_length;
    memset(ptr_to_remove, 0, sizeof(struct PointerHeader));
  }
}

/**
Find the zone that this ptr belongs to.
*/
struct HeapZoneStart* _zone_for_ptr(struct HeapZoneStart *heap,void* ptr)
{
  // Protect against NULL pointer
  if(!heap || !ptr) return NULL;
  
  if(heap->magic != HEAP_ZONE_SIG) k_panic("Kernel heap corrupted\r\n");
  void* zone_start = (void*)heap;
  void* zone_end = (char*)heap + heap->zone_length;
  
  // Use inclusive boundaries for proper zone checking
  if(ptr >= zone_start && ptr < zone_end) return heap; //this is the zone that contains the pointer
  
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
  kprintf("DEBUG No existing heap zones had space to allocate %l (0x%x) bytes, expanding last zone\r\n", bytes, bytes);
  #endif
  //if we get here, then no zones had available space. z should be set to the last zone.
  size_t pages_required = (bytes / PAGE_SIZE) +1; //if bytes is an exact page size, we'll need more space for zone and block headers.
  //always allocate at least MIN_ZONE_SIZE_PAGES
  size_t pages_to_alloc = pages_required > MIN_ZONE_SIZE_PAGES ? pages_required : MIN_ZONE_SIZE_PAGES;
  z = _expand_zone(z, pages_to_alloc);
  //at this point, z might be the previous zone; it might be a new one; or it might be NULL if something went wrong.
  if(!z) return NULL;
  void* result = _zone_alloc(z, bytes);
  #ifdef MMGR_VERBOSE
  if(result) {
    kprintf("INFO Allocated block 0x%x\r\n", (char*)result - sizeof(struct PointerHeader));
  } else {
    kprintf("INFO Allocation failed\r\n");
  }
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
  #ifdef MMGR_VERBOSE
  kprintf("DEBUG malloc_for_process 0x%x 0x%x\r\n", (uint32_t)pid, (uint32_t)bytes);
  #endif

  // Validate parameters
  if(bytes == 0) {
    kprintf("WARNING malloc called with 0 bytes\r\n");
    return NULL;
  }
  
  if(bytes > (size_t)-1 - sizeof(struct PointerHeader) - sizeof(struct HeapZoneStart)) {
    kprintf("ERROR malloc called with impossibly large size 0x%x\r\n", bytes);
    return NULL;
  }
  
  struct ProcessTableEntry *process = get_process(pid);
  if(!process) return NULL;
  if(process->status==PROCESS_NONE) return NULL;  //don't alloc a non-existent process
  if(!process->heap_start) return NULL; // No heap initialized
  
  return heap_alloc(process->heap_start, bytes);
}

//allocates onto the kernel heap
void* malloc(size_t bytes)
{
    return malloc_for_process(0, bytes);
}

void free_for_process(uint16_t pid, void *ptr)
{
  // Validate parameters
  if(!ptr) {
    kprintf("WARNING free called with NULL pointer\r\n");
    return;
  }
  
  struct ProcessTableEntry *process = get_process(pid);
  if(!process) return;
  if(process->status==PROCESS_NONE) return;  //don't free from a non-existent process
  if(!process->heap_start) return; // No heap initialized
  
  heap_free(process->heap_start, ptr);
}

void free(void* ptr)
{
  free_for_process(0, ptr);
}
