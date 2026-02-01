#include <types.h>
#include <stdio.h>
#include <process.h>
#include <x86/idt.h>
#include "process_ops.h"
#include <x86/segmentation.h>

// This is defined in native_api.asm
extern native_api_landing_pad();

/**
 * Initializes the native API by creating an IDT entry for the native API interrupt on 0x60.
 */
void init_native_api()
{
    create_idt_entry(0x60, native_api_landing_pad, IDT_SELECTOR_CODE, 3);   
}

void api_terminate_current_process()
{
    pid_t current_pid = get_active_pid();
    struct ProcessTableEntry *entry = get_process(current_pid);
    if(entry==NULL) {
        kprintf("\r\nERROR No process entry found for %d\r\n", current_pid);
        return;
    }

    kprintf("\r\nDEBUG api_terminate PID %d is at 0x%x\r\n", current_pid, entry);
    entry->status = PROCESS_TERMINATING;
    schedule_cleanup_task(current_pid);
}

void api_sleep_current_process()
{

}

pid_t api_create_process()
{
  
}
