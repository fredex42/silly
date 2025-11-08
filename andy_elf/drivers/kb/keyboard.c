#include <types.h>
#include <drivers/kb_buffer.h>
#include <process.h>
#include "../ps2_controller/controller.h"
#include "pending_operation_list.h"

PendingOperationList *kb_pending_ops;

void init_keyboard() {
    kb_pending_ops = NULL;
    init_kb_pending_ops();
}

/**
 * Read from the keyboard device into the given filepointer
 */
uint8_t kb_read_to_file(struct ProcessTableEntry *process, struct FilePointer *fp) {
    char ch = 0;
    // kputs("DEBUG kb_read_to_file\r\n");
    // kprintf("DEBUG fp read buffer 0x%x\r\n", fp->read_buffer);

    ring_buffer_ref(fp->read_buffer);
    if(ring_buffer_empty(fp->read_buffer)) {
        //Nothing in the buffer, go to wait state
        //kputs("DEBUG kb buffer empty, going to wait\r\n");
        process->status = PROCESS_IOWAIT;
        fp->status = FP_STATUS_WAIT;
        kb_push_pending_operation(&kb_pending_ops, process, fp);
    } else {
        //We have something
        //kputs("DEBUG kb buffer has data, going to return\r\n");
        ch = ring_buffer_pop(fp->read_buffer);
        if(process->status==PROCESS_IOWAIT) process->status = PROCESS_READY;
    }
    ring_buffer_unref(fp->read_buffer);
    return ch;
}

/**
 * Called by the low-level driver to notify us of activity (possibly in an interrupt context, careful)
 */
void kb_notify_activity() {
    PendingOperationList *next_op = kb_pop_pending_operation(&kb_pending_ops);
    if(next_op && next_op->fp && next_op->fp->read_buffer) {
        char ch = ps2_get_buffer();
        ring_buffer_ref(next_op->fp->read_buffer);
        ring_buffer_push(next_op->fp->read_buffer, ch);
        ring_buffer_unref(next_op->fp->read_buffer);
        if(next_op->process->status == PROCESS_IOWAIT) next_op->process->status = PROCESS_READY;
        next_op->fp->status = FP_STATUS_READY;
    }
}

/**
 * Clean up any pending keyboard operations for a process that is being terminated
 */
void kb_cleanup_process_operations(struct ProcessTableEntry *process) {
    kprintf("DEBUG kb_cleanup_process_operations for process 0x%x\r\n", process);
    kb_pending_ops = kb_cancel_pending_ops_for_process(kb_pending_ops, process);
}