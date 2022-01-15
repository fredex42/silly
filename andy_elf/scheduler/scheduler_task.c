#include <sys/mmgr.h>
#include <stdio.h>
#include <types.h>
#include <scheduler/scheduler.h>
#include "scheduler_task_internals.h"

/**
Initialise a new SchedulerTaskBuffer instance
*/
SchedulerTaskBuffer *new_scheduler_task_buffer()
{
  SchedulerTaskBuffer *b = (SchedulerTaskBuffer *)vm_alloc_pages(NULL, TASK_BUFFER_SIZE_IN_PAGES, MP_READWRITE);
  if(b==NULL) {
    k_panic("Unable to allocate scheduler task buffer\r\n");
    return NULL;
  }

  //make sure the page is zeroed
  memset((void *)b, 0, TASK_BUFFER_SIZE_IN_PAGES*PAGE_SIZE);

  b->buffer_ptr = (void *)((vaddr)b + sizeof(SchedulerTaskBuffer));
  b->task_limit = (uint32_t)(TASK_BUFFER_SIZE_IN_PAGES*PAGE_SIZE) / sizeof(SchedulerTask);
  return b;
}

/**
Frees the memory associated with this task buffer.
The pointer should not be re-used after calling this function
*/
void scheduler_vacate_task_buffer(SchedulerTaskBuffer *b)
{
  vm_deallocate_physical_pages(NULL, (void *)b, TASK_BUFFER_SIZE_IN_PAGES);
}
