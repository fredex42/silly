#include <process.h>

#ifndef __KB_BUFFER_H
#define __KB_BUFFER_H
//These functions are implemented in drivers/ps2_controller at present
char ps2_get_buffer();  //gets the next char in the keyboard buffer, or NULL if none is present.

//kb/keyboard.c
uint8_t kb_read_to_file(struct ProcessTableEntry *process, struct FilePointer *fp);
void init_keyboard();

/**
 * Called by the low-level driver to notify us of activity (possibly in an interrupt context, careful)
 */
void kb_notify_activity();

typedef struct PendingOperationList {
    struct PendingOperationList *next;

    struct ProcessTableEntry *process;
    struct FilePointer *fp;
} PendingOperationList;

/**
 * Removes any pending operations for the given process (as part of the process shutdown)
 * Returns a new start-pointer if the first item in the list changes
 */
PendingOperationList* kb_cancel_pending_ops_for_process(PendingOperationList *start, struct ProcessTableEntry *process);
/**
 * Removes any pending operations for the given file-pointer (as part of the file close)
 * Returns a new start-pointer if the first item in the list changes
 */
PendingOperationList* kb_cancel_pending_ops_for_file(PendingOperationList *start, struct FilePointer *fp);
/**
 * Return the first pending operation, if there is one
 */
PendingOperationList *kb_pop_pending_operation(PendingOperationList **start);

#endif