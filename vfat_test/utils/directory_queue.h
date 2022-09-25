#include "generic_queue.h"
#include "../vfat.h"
#ifndef __VFAT_DIRECTORY_QUEUE_H
#define __VFAT_DIRECTORY_QUEUE_H

typedef struct GenericQueue DIRECTORY_QUEUE;

DIRECTORY_QUEUE* new_directory_queue(size_t capacity);
void free_directory_queue(DIRECTORY_QUEUE* q);
uint8_t directory_queue_empty(DIRECTORY_QUEUE *q);
uint8_t directory_queue_push(DIRECTORY_QUEUE *q, DirectoryEntry *dir);
uint8_t directory_queue_pop(DIRECTORY_QUEUE *q, DirectoryEntry *dir);

void directory_queue_ref(DIRECTORY_QUEUE *q);
void directory_queue_unref(DIRECTORY_QUEUE *q);
#endif
