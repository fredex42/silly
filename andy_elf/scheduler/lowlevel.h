#include "../mmgr/process.h"
#ifndef __SCHEDULER_LOWLEVEL_H
#define __SCHEDULER_LOWLEVEL_H

/**
;Purpose: Executes an IRET to exit kernel mode into user-mode on the given process.
;Expects the process stack frame to be configured for the return already; this is either
;done by the call into kernel-mode or by the loader.
;Arguments: 1. Pointer to the struct ProcessTableEntry describing the process.
;Returns when the kernel is re-entered
*/
void exit_to_process(struct ProcessTableEntry *entry);

#endif
