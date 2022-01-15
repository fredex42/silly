#include <types.h>

#define TASK_BUFFER_SIZE_IN_PAGES   1

/*
the task buffer manages a memory page which contains a number of task defintitions.
once used, a memory space is not re-used; once the page has been vacated then it's
disposed and a new one created
*/
typedef struct scheduler_task_buffer {
  void *buffer_ptr; //points to the buffer space following this struct
  uint32_t waiting_tasks; //count of tasks that have not been run yet
  uint32_t buffer_idx;  //where to put the next task in the buffer
  uint32_t task_limit;  //maximum number of tasks we can put in
} SchedulerTaskBuffer;

SchedulerTaskBuffer *new_scheduler_task_buffer();
uint8_t should_vacate_task_buffer(SchedulerTaskBuffer *b);
void scheduler_vacate_task_buffer(SchedulerTaskBuffer *b);
