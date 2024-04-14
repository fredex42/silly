#include <types.h>
#include <process.h>
#include <scheduler/scheduler.h>
#include <malloc.h>
#include <memops.h>
#include <panic.h>
#include <sys/mmgr.h>

/**
 * Routine to actually cleanup the process.  This is called from the scheduler by schedule_cleanup_task
*/
void cleanup_process(SchedulerTask *t)
{
    kprintf("DEBUG entered cleanup_process\r\n");
    pid_t pid = (pid_t)t->data;
    struct ProcessTableEntry *process = get_process(pid);

    kprintf("DEBUG pid %d is at 0x%d\r\n", pid, process);
    if(process->magic!=PROCESS_TABLE_MAGIC_NUMBER) {
        k_panic("ERROR process table corruption detected");
        return;
    }

    if(process->status!=PROCESS_TERMINATING) {
        kprintf("WARNING process not in terminating state\r\n");
        return;
    }

    //get hold of the process's memory table
        validate_kernel_memory_allocations();
    kprintf("DEBUG cleanup_process mapping\r\n");
    uint32_t *pagingdir = map_app_pagingdir(process->root_paging_directory_phys, APP_PAGEDIRS_BASE);
        validate_kernel_memory_allocations();
    kprintf("DEBUG cleanup_process deallocating\r\n");
    free_app_memory(pagingdir, process->root_paging_directory_phys);
        validate_kernel_memory_allocations();
    kprintf("DEBUG cleanup_process unmapping\r\n");
    unmap_app_pagingdir(pagingdir);
        validate_kernel_memory_allocations();
    kprintf("INFO cleanup_process done\r\n");
    validate_kernel_memory_allocations();
}

/**
 * We need to clean up a terminated process, but do this off the critical path.
 * Schedule a callback task for the scheduler to call when idle.
*/
void schedule_cleanup_task(pid_t pid)
{
    SchedulerTask *t = new_scheduler_task(TASK_ASAP, &cleanup_process, (void *)pid);

    schedule_task(t);
}

