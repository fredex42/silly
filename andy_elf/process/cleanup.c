#include <types.h>
#include <process.h>
#include <scheduler/scheduler.h>
#include <malloc.h>
#include <memops.h>
#include <panic.h>
#include <sys/mmgr.h>
#include <drivers/kb_buffer.h>

/**
 * Routine to actually cleanup the process.  This is called from the scheduler by schedule_cleanup_task
*/
void cleanup_process(SchedulerTask *t)
{
    kputs("DEBUG entered cleanup_process\r\n");
    pid_t pid = (pid_t)t->data;
    struct ProcessTableEntry *process = get_process(pid);

    kprintf("DEBUG pid %d is at 0x%x\r\n", pid, process);
    if(process->magic!=PROCESS_TABLE_MAGIC_NUMBER) {
        k_panic("ERROR process table corruption detected");
        return;
    }

    if(process->status!=PROCESS_TERMINATING) {
        kprintf("WARNING process not in terminating state\r\n");
        return;
    }

    // Clean up any pending keyboard operations for this process
    kputs("DEBUG cleanup_process: cleaning keyboard operations\r\n");
    kb_cleanup_process_operations(process);

    //get hold of the process's memory table
    uint32_t *pagingdir = map_app_pagingdir(process->root_paging_directory_phys, APP_PAGEDIRS_BASE);
    free_app_memory(pagingdir, process->root_paging_directory_phys);
    unmap_app_pagingdir(pagingdir);

    /*
      don't panic if there are left-over maps in the kernel, just remove them.
      this is because the allocation process leaves on page (the upper-level paging dir) mapped in kernel space AND in
      process-space; so we need to make sure it gets removed here to prevent a memory leak.
    */
    validate_kernel_memory_allocations(0);
    
    // Reset the process table entry to PROCESS_NONE to indicate it's available
    // Clear any stale data that might cause issues
    process->status = PROCESS_NONE;
    process->root_paging_directory_phys = NULL;
    process->root_paging_directory_kmem = NULL;
    process->heap_start = NULL;
    process->heap_allocated = 0;
    process->heap_used = 0;
    process->stack_phys_ptr = NULL;
    process->stack_kmem_ptr = NULL;
    
    kprintf("INFO cleanup_process done\r\n");
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

