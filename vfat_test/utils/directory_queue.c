#include "directory_queue.h"
#include "../vfat.h"

DIRECTORY_QUEUE* new_directory_queue(size_t capacity)
{
  return (DIRECTORY_QUEUE *)new_generic_queue(sizeof(DirectoryEntry), capacity);
}

void free_directory_queue(DIRECTORY_QUEUE* q)
{
  return free_generic_queue((GENERIC_QUEUE *)q);
}

uint8_t directory_queue_empty(DIRECTORY_QUEUE *q)
{
  return generic_queue_empty((GENERIC_QUEUE *)q);
}

/**
Note that `dir` is copied, so stack-local pointers are allowed
*/
uint8_t directory_queue_push(DIRECTORY_QUEUE *q, DirectoryEntry *dir)
{
  return generic_queue_push((GENERIC_QUEUE *)q, (void *)dir);
}

/**
The next item is _copied_ to the data pointed to by `dir`
*/
uint8_t directory_queue_pop(DIRECTORY_QUEUE *q, DirectoryEntry *dir)
{
  return generic_queue_pop((GENERIC_QUEUE *)q, (void *)dir);
}

void directory_queue_ref(DIRECTORY_QUEUE *q) {
  generic_queue_ref((GENERIC_QUEUE *)q);
}

void directory_queue_unref(DIRECTORY_QUEUE *q)
{
  generic_queue_unref((GENERIC_QUEUE *)q);
}
