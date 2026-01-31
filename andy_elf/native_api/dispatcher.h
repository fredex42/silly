#ifndef NATIVE_API_DISPATCHER_H
#define NATIVE_API_DISPATCHER_H
#include <types.h>

uint32_t native_api_dispatcher(uint32_t call_id, uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi);

#endif // NATIVE_API_DISPATCHER_H
