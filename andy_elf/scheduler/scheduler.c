#include <memops.h>
#include <scheduler/scheduler.h>
#include <sys/mmgr.h>
#include <stdio.h>
#include <sys/ioports.h>
#include "scheduler_task_internals.h"
#include <cfuncs.h>
#include "../8259pic/picroutines.h"

static SchedulerState *global_scheduler_state;

void initialise_scheduler()
{
  global_scheduler_state = (SchedulerState *)vm_alloc_pages(NULL,1, MP_READWRITE);
  memset((void *)global_scheduler_state, 0, PAGE_SIZE);

  for(int i=0;i<BUFFER_COUNT;i++) {
    global_scheduler_state->buffers[i] = new_scheduler_task_buffer();
  }
}

void scheduler_tick()
{
  SchedulerTask *to_run;

  // //we don't want the scheduler to tick again while we are processing
  // cli();
  // mask_irq(0);
  // sti();

  ++global_scheduler_state->ticks_elapsed;
  //FIXME: run deadline tasks first
  //FIXME: run after-time tasks second

  //run scheduler state tasks
  if(global_scheduler_state->task_asap_list!=NULL) {
    to_run = global_scheduler_state->task_asap_list;
    global_scheduler_state->task_asap_list = global_scheduler_state->task_asap_list->next;

    (to_run->task_proc)(to_run);  //call the task_proc to do its thang
  }
  // cli();
  // unmask_irq(0);
  // sti();

}

uint64_t get_scheduler_ticks()
{
  return global_scheduler_state->ticks_elapsed;
}

/**
Creates a new SchedulerTask object in our buffer and returns it
*/
SchedulerTask *new_scheduler_task(uint8_t task_type, void (*task_proc)(struct scheduler_task *t), void *data)
{
  SchedulerTask *task_ptr;
  unsigned int iterations = 0;

  SchedulerTaskBuffer *current_buffer = global_scheduler_state->buffers[global_scheduler_state->current_buffer];

  kputs("creating new scheduler task\r\n");
  while(current_buffer->buffer_idx + sizeof(SchedulerTaskBuffer) > PAGE_SIZE*TASK_BUFFER_SIZE_IN_PAGES) {
    ++current_buffer->buffer_idx;
    if(current_buffer->buffer_idx > BUFFER_COUNT) {
      if(iterations>0) {  //if we have already looped back to the beginning once in order to check for space there, and not found any, we ran out :(
        k_panic("Ran out of mem for scheduling tasks\r\n");
        return NULL;
      } else {
        ++iterations;
        current_buffer->buffer_idx=0;
      }
    }

    current_buffer = global_scheduler_state->buffers[global_scheduler_state->current_buffer];
  }

  task_ptr = (SchedulerTask *)((vaddr)current_buffer->buffer_ptr + current_buffer->buffer_idx);
  current_buffer->buffer_idx += sizeof(SchedulerTask);
  memset(task_ptr, 0, sizeof(SchedulerTask));

  task_ptr->task_type = task_type;
  task_ptr->task_proc = task_proc;
  task_ptr->data = data;

  return task_ptr;
}

/**
Adds the schedule task into one of our lists and increments the relevant buffer
*/
void schedule_task(SchedulerTask *t)
{
  SchedulerTask *task_list;

  switch(t->task_type) {
    case TASK_NONE:
      kputs("ERROR Tried to schedule an invalid task with type TASK_NONE\r\n");
      return;
    case TASK_ASAP:
      //cli();
      kputs("scheduling asap task\r\n");
      if(global_scheduler_state->task_asap_list==NULL) {
        global_scheduler_state->task_asap_list = t;
      } else {
        //find the end of the linked list
        //for(task_list=global_scheduler_state->task_asap_list;task_list!=NULL;task_list=task_list->next);
        task_list = global_scheduler_state->task_asap_list;
        while(task_list->next!=NULL) {
          task_list = task_list->next;
        }
        //task_list now points to the last item on the linked list
        task_list->next = t;
      }
      kputs("done.\r\n");
      //sti();
      return;
    case TASK_DEADLINE:
      kputs("ERROR Deadline tasks not implemented yet\r\n");
      return;
    case TASK_AFTERTIME:
      kputs("ERROR after-time tasks not implemented yet\r\n");
      return;
    default:
      kprintf("ERROR tried to schedule an invalid task with type %d\r\n", (uint16_t) t->task_type);
      return;
  }
}
