#include <memops.h>
#include <scheduler/scheduler.h>
#include <sys/mmgr.h>
#include <stdio.h>
#include <sys/ioports.h>
#include "scheduler_task_internals.h"
#include <cfuncs.h>
#include "../drivers/cmos/rtc.h"
#include "../8259pic/picroutines.h"
#include "../mmgr/process.h"
#include "lowlevel.h"

static SchedulerState *global_scheduler_state;
static pid_t last_run_pid;
static pid_t active_process;

void initialise_scheduler()
{
  global_scheduler_state = (SchedulerState *)vm_alloc_pages(NULL,1, MP_READWRITE);
  memset((void *)global_scheduler_state, 0, PAGE_SIZE);

  for(int i=0;i<BUFFER_COUNT;i++) {
    global_scheduler_state->buffers[i] = new_scheduler_task_buffer();
  }
  last_run_pid = 0;
  active_process = 0;
}


/*
scheduler_tick is expected to be run from IRQ0, i.e. every 55ms or so.
*/
void scheduler_tick()
{
  SchedulerTask *to_run;

  cli();
  //FIXME: this is just for debugging!!
  uint32_t current_ticks = rtc_get_ticks();
  uint32_t epoch_time = rtc_get_unix_time();
  uint32_t boot_time = rtc_get_boot_time();

  //kprintf("DEBUG: There have been %l elapsed 512Hz ticks.\r\n", current_ticks);
  //kprintf("System startup was at %l, Current unix time is %l\r\n", boot_time, epoch_time);
  ++global_scheduler_state->ticks_elapsed;
  //FIXME: run deadline tasks first
  //FIXME: run after-time tasks second

  if(global_scheduler_state->tasks_in_progress>0) {
    //we have not finished processing the last run yet!
    return;
  }

  //run scheduler state tasks
  if(global_scheduler_state->task_asap_list!=NULL) {
    to_run = global_scheduler_state->task_asap_list;
    global_scheduler_state->task_asap_list = global_scheduler_state->task_asap_list->next;

    ++global_scheduler_state->tasks_in_progress;
    (to_run->task_proc)(to_run);  //call the task_proc to do its thang
    --global_scheduler_state->tasks_in_progress;
  }
  sti();
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

//finds the next runnable process (in a round-robin fashion) and attempts to enter it
void enter_next_process()
{
  pid_t i, temp;
  struct ProcessTableEntry* process = NULL;

  cli();
  for(i=last_run_pid+1;i<PID_MAX;i++) {
    temp = i;
    process = get_process(i);
    if(process->status==PROCESS_READY) break;
  }
  if(process->status!=PROCESS_READY) { //still not found one? Re-start the list. Ignore process 0 as that's us (the kernel)
    for(i=1;i<=last_run_pid;i++) {
      temp = i;
      process = get_process(i);
      if(process->status==PROCESS_READY) break;
    }
  }
  if(process->status!=PROCESS_READY){
    sti();
    return; //OK, nothing doing. Go back to the kernel idle loop.
  }

  //Right, we have something.  We must switch process.
  last_run_pid = temp;
  active_process = temp;
  kprintf("DEBUG Exiting into process 0x%x\r\n", process);
  exit_to_process(process);
}

pid_t get_active_pid()
{
  return active_process;
}
