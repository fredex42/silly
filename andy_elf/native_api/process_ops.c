#include <types.h>
#include <stdio.h>
#include <process.h>

void api_terminate_current_process()
{
    pid_t current_pid = get_active_pid();
    struct ProcessTableEntry *entry = get_process(current_pid);
    if(entry==NULL) {
        kprintf("ERROR No process entry found for %d\r\n", current_pid);
        return;
    }

    kprintf("DEBUG api_terminate PID %d is at 0x%x\r\n", current_pid, entry);
    entry->status = PROCESS_TERMINATING;
}

void api_sleep_current_process()
{

}

pid_t api_create_process()
{
  
}
