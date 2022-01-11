#include <types.h>

#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#define TASK_NONE       0 //invalid (null) task
#define TASK_ASAP       1 //deferred execution to be run as soon as we can
#define TASK_DEADLINE   2
#define TASK_AFTERTIME  3


#define BUFFER_COUNT    2

typedef struct scheduler_task {
  struct scheduler_task *next;

  uint8_t task_type;  //must be one of the TASK_ types listed above

  void (*task_proc)(struct scheduler_task *t);  //pointer to the service routine, in kernel CS
  void *data;                                   //generic data pointer for required data, in kernel DS

  uint32_t process_id;
  uint64_t time_val;  //only used for TASK_DEADLINE or TASK_AFTERTIME types
} SchedulerTask;

typedef struct scheduler_state {
  uint64_t ticks_elapsed;

  SchedulerTask *task_asap_list;  //tasks for running now
  SchedulerTask *task_deadline_list;  //ordered list of deadline tasks
  SchedulerTask *task_aftertime_list; //ordered list of "after time" tasks

  struct scheduler_task_buffer *buffers[BUFFER_COUNT];
  uint32_t current_buffer;
} SchedulerState;

void initialise_scheduler();
void scheduler_tick();

SchedulerTask *new_scheduler_task(uint8_t task_type, void (*task_proc)(struct scheduler_task *t), void *data);
void schedule_task(SchedulerTask *t); //pushes the task onto the relevant queue and takes ownership of the ptr

uint64_t get_scheduler_ticks();

#endif
