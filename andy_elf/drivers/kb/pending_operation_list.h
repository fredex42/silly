#ifndef __KB_PENDING_OPERATION_LIST_H
#define __KB_PENDING_OPERATION_LIST_H
#include <drivers/kb_buffer.h>

void init_kb_pending_ops();
PendingOperationList* kb_push_pending_operation(PendingOperationList **start, struct ProcessTableEntry *p, struct FilePointer *fp);
PendingOperationList* kb_cancel_pending_ops_for_process(PendingOperationList *start, struct ProcessTableEntry *process);
PendingOperationList* kb_cancel_pending_ops_for_file(PendingOperationList *start, struct FilePointer *fp);

#endif //__KB_PENDING_OPERATION_LIST_H