#include <types.h>
#include <process.h>
#include <scheduler/scheduler.h>
#include <malloc.h>
#include <memops.h>

/**
 * Routine to actually cleanup the process.  This is called from the scheduler by schedule_cleanup_task
*/
void cleanup_process(void *data)
{
    kprintf("DEBUG entered cleanup_process");
    pid_t pid = (pid_t)data;

}

/**
 * We need to clean up a terminated process, but do this off the critical path.
 * Schedule a callback task for the scheduler to call when idle.
*/
void schedule_cleanup_task(pid_t pid)
{
    SchedulerTask *t = (SchedulerTask *)malloc(sizeof(SchedulerTask));
    memset(t, 0, sizeof(SchedulerTask));

    t->task_type = TASK_ASAP;
    t->task_proc = cleanup_process;
    t->data = (void *)pid;
    t->process_id = 0;

    schedule_task(t);
}

