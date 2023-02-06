#include <types.h>

#ifndef __API_PROCESS_OPS_H
#define __API_PROCESS_OPS_H

void api_terminate_current_process();
void api_sleep_current_process();
pid_t api_create_process();

#endif
