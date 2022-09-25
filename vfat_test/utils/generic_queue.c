#include "generic_queue.h"

#define PAGE_SIZE 4096

GENERIC_QUEUE* new_generic_queue(size_t entry_size, size_t capacity)
{
  size_t size_in_pages = ((entry_size * capacity) + sizeof(GENERIC_QUEUE)) / PAGE_SIZE;
  printf("DEBUG new_generic_queue allocating %d pages\n", size_in_pages);
  GENERIC_QUEUE *q = (GENERIC_QUEUE *)malloc(size_in_pages * PAGE_SIZE);
  if(q==NULL) return NULL;
  memset((void *)q, 0, size_in_pages*PAGE_SIZE);

  q->entry_size = entry_size;
  q->entry_count = 0;
  q->capacity = capacity;
  q->queue_head_idx = 0;
  q->queue_tail_idx = 0;
  q->buffer = ((char *)q + sizeof(GENERIC_QUEUE));
  q->refcount = 1;
  return q;
}

void free_generic_queue(GENERIC_QUEUE* q)
{
  printf("DEBUG free_generic_queue deleting queue at 0x%x\n", q);
  free(q);  //given that this is the slab pointer to the list as well, it's all we need to de-alloc.
}

uint8_t generic_queue_empty(GENERIC_QUEUE *q)
{
  return q->queue_head_idx==q->queue_tail_idx;
}

/**
push an item onto the queue. Note that `p` must be a _pointer_ to the item.
The content is copied into the queue so `p` is still free-able after this call.
So, `q->entry_size` bytes pointed to by `p` get copied into the queue buffer.
*/
uint8_t generic_queue_push(GENERIC_QUEUE *q, void *p)
{
  void *target_addr = (char *)q->buffer+(q->queue_head_idx*q->entry_size);
  printf("DEBUG queue base address is 0x%x, base struct size is 0x%x, buffer base address is 0x%x, queue_head_idx is 0x%x, entry_size is 0x%x\n", q, sizeof(q), q->buffer, q->queue_head_idx, q->entry_size);
  printf("DEBUG target address is 0x%x\n", target_addr);
  memcpy(target_addr, p, q->entry_size);
  ++q->queue_head_idx;
  ++q->entry_count;
  if(q->queue_head_idx > q->capacity) q->queue_head_idx = 0;
  return QUEUE_OK;
}

/**
pop an item from the front of the queue.
the content of the item is copied into the data pointed to by `p`,
which must be large enough to hold `q->entry_size` bytes.
*/
uint8_t generic_queue_pop(GENERIC_QUEUE *q, void *p)
{
  if(generic_queue_empty(q)) return QUEUE_EMPTY;
  memcpy(p, (void*)((char *)q->buffer+(q->queue_tail_idx*q->entry_size)), q->entry_size);
  ++q->queue_tail_idx;
  --q->entry_count;
  if(q->queue_tail_idx>q->capacity) q->queue_tail_idx = 0;
  return QUEUE_OK;
}

void generic_queue_ref(GENERIC_QUEUE *q)
{
  ++q->refcount;
  printf("DEBUG generic_queue_ref 0x%x refcount is now %d\n", q, q->refcount);
}

void generic_queue_unref(GENERIC_QUEUE *q)
{
  --q->refcount;
  printf("DEBUG generic_queue_ref 0x%x refcount is now %d\n", q, q->refcount);
  if(q->refcount==0) free_generic_queue(q);
}
