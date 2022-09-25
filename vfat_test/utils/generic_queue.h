#include <stddef.h>
#include <stdint.h>

#ifndef __GENERIC_QUEUE_H
#define __GENERIC_QUEUE_H

typedef struct GenericQueue {
  size_t entry_size;
  size_t entry_count;
  size_t capacity;
  size_t queue_head_idx;
  size_t queue_tail_idx;

  uint32_t refcount;

  void *buffer;
} GENERIC_QUEUE;

//return codes for uint8_t types
#define QUEUE_OK        0
#define QUEUE_EMPTY     1
#define QUEUE_OVERFLOW  2

GENERIC_QUEUE* new_generic_queue(size_t entry_size, size_t capacity);
void free_generic_queue(GENERIC_QUEUE* q);
uint8_t generic_queue_empty(GENERIC_QUEUE *q);
uint8_t generic_queue_push(GENERIC_QUEUE *q, void *p);
uint8_t generic_queue_pop(GENERIC_QUEUE *q, void *p);

void generic_queue_ref(GENERIC_QUEUE *q);
void generic_queue_unref(GENERIC_QUEUE *q);
#endif
