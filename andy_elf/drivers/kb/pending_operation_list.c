#include <types.h>
#include <spinlock.h>
#include <drivers/kb_buffer.h>
#include <malloc.h>
#include <panic.h>
#include <memops.h>

static spinlock_t kb_pending_operation_lock;

void init_kb_pending_ops() {
    kprintf("DEBUG kb_pending_operation_lock at 0x%x\r\n", &kb_pending_operation_lock);
    kb_pending_operation_lock = 0;
    mb();
}

/**
 * Removes any pending operations for the given process (as part of the process shutdown)
 * Returns a new start-pointer if the first item in the list changes
 */
PendingOperationList* kb_cancel_pending_ops_for_process(PendingOperationList *start, struct ProcessTableEntry *process)
{
    if(start==NULL) return; //no list => nothing to do

    acquire_spinlock(&kb_pending_operation_lock);
    PendingOperationList *prev=NULL,*op=NULL,*new_start=NULL,*temp=NULL;
    uint8_t last_iteration=0;

    new_start = start;
    op = new_start;
    while(op!=NULL) {
        if(op->process==process) {
            //remove this matching record
            if(prev) {
                //there is a previous record
                prev->next = op->next;
                temp = op;
                op = op->next;
                free(temp);
            } else {
                //there is no previous record, we must be at the start of the list
                new_start = op->next;
                temp = op;
                op = op->next;
                free(temp);
            }
        } else {
            //no need to remove the record
            prev = op;
            op = op->next;
        }
    }

    release_spinlock(&kb_pending_operation_lock);
}

/**
 * Removes any pending operations for the given file-pointer (as part of the file close)
 * Returns a new start-pointer if the first item in the list changes
 */
PendingOperationList* kb_cancel_pending_ops_for_file(PendingOperationList *start, struct FilePointer *fp)
{
    if(start==NULL) return; //no list => nothing to do

    acquire_spinlock(&kb_pending_operation_lock);
    PendingOperationList *prev=NULL,*op=NULL,*new_start=NULL,*temp=NULL;
    uint8_t last_iteration=0;

    new_start = start;
    op = new_start;
    while(op!=NULL) {
        if(op->fp==fp) {
            //remove this matching record
            if(prev) {
                //there is a previous record
                prev->next = op->next;
                temp = op;
                op = op->next;
                free(temp);
            } else {
                //there is no previous record, we must be at the start of the list
                new_start = op->next;
                temp = op;
                op = op->next;
                free(temp);
            }
        } else {
            //no need to remove the record
            prev = op;
            op = op->next;
        }
    }

    release_spinlock(&kb_pending_operation_lock);
}

/**
 * Return the first pending operation, if there is one
 */
PendingOperationList *kb_pop_pending_operation(PendingOperationList **start)
{
    if(start==NULL) return NULL;

    acquire_spinlock(&kb_pending_operation_lock);
    PendingOperationList *head = *start;
    start = (*start)->next;
    release_spinlock(&kb_pending_operation_lock);
    return head;
}

/**
 * Queue a pending operation to read from the keyboard
 */
PendingOperationList *kb_push_pending_operation(PendingOperationList **start, struct ProcessTableEntry *p, struct FilePointer *fp) 
{
    PendingOperationList *last = *start;
    kprintf("DEBUG POL start at 0x%x\r\n", start);

    kputs("DEBUG kb_push_pending_operation malloc\r\n");
    PendingOperationList *new_entry = (PendingOperationList *)malloc(sizeof(PendingOperationList));
    if(!new_entry) {
        k_panic("Not enough memory to allocate a pending operation!\r\n");
    }
    memset((void *)new_entry, 0, sizeof(PendingOperationList));

    kprintf("DEBUG kb_push_pending_operation entry is at 0x%x\r\n", new_entry);

    new_entry->process = p;
    new_entry->fp = fp;

    acquire_spinlock(&kb_pending_operation_lock);
    //find the end of the list
    while(last && last->next) {
        kprintf("DEBUG walk POL at 0x%x\r\n", last);
        last=last->next;
    }

    if(last) {
        //There is an existing list there
        last->next = new_entry;
    } else {
        //There is no existing list, start a new one
        *start = new_entry;
    }

    release_spinlock(&kb_pending_operation_lock);
}