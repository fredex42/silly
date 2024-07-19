#include <types.h>
#include <drivers/kb_buffer.h>
#include <process.h>
#include "../ps2_controller/controller.h"

PendingOperationList *kb_pending_ops = NULL;

/**
 * Read from the keyboard device into the given filepointer
 */
uint8_t kb_read_to_file(struct ProcessTableEntry *process, struct FilePointer *fp) {
    char ch = 0;
    // uint8_t ch = ps2_get_buffer();
    // ch = ps2_get_buffer();

    // kprintf("\r\nDEBUG kb_read_to_file next char is 0x%x (%c)\r\n", (uint32_t)ch, ch);
    // if(ch==0) {
    //     process->status = PROCESS_IOWAIT; //this tells the scheduler not to schedule the process until IO is ready
    //     fp->status = FP_STATUS_WAIT;
    //     return 0;   //returning 0 from this function tells the landing pad to return to kernel idle loop instead of the process
    // } else {
    //     if(process->status==PROCESS_IOWAIT) process->status = PROCESS_READY;
    //     fp->status = FP_STATUS_READY;
    //     ring_buffer_push(fp->read_buffer, ch);
    //     return ch;
    // }
    ring_buffer_ref(fp->read_buffer);
    if(ring_buffer_empty(fp->read_buffer)) {
        //Nothing in the buffer, go to wait state
        process->status = PROCESS_IOWAIT;
        fp->status = FP_STATUS_WAIT;
        kb_push_pending_operation(&kb_pending_ops, process, fp);
    } else {
        //We have something
        ch = ring_buffer_pop(fp->read_buffer);
        ring_buffer_unref(fp);
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