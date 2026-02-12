#include <types.h>
#include <native_api/errors.h>
#include <native_api/apicodes.h>
#include <scheduler/scheduler.h>
#include <stdio.h>
#include "../drivers/cmos/rtc.h"
#include "dispatcher.h"
#include "process_ops.h"
#include "stream_ops.h"

uint32_t native_api_dispatcher(uint32_t call_id, uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi) {
    kprintf("trace: napi call_id=0x%x ebx=0x%x ecx=0x%x edx=0x%x esi=0x%x edi=0x%x\r\n", call_id, ebx, ecx, edx, esi, edi);
    switch(call_id) {
        case API_EXIT:
        //TODO - need to return to idle loop not process
            set_needs_reschedule(1);
            api_terminate_current_process(ebx);
            return 0;
        case API_SLEEP:
        //TODO - need to return to idle loop not process
            set_needs_reschedule(1);
            api_sleep_current_process(ebx);
            return 0;
        case API_GET_TIME:
            return rtc_get_epoch_time();
        case API_CREATE_PROCESS:
        //TODO - need to return to idle loop not process
            set_needs_reschedule(1);
            return api_create_process((const char*)ebx, (const char*)ecx);
        case API_CLOSE:
            return api_close(ebx);
        case API_OPEN:
        // Note: edi = filename, esi = extension, edx = mode_flags. TODO - unified extension and filename, drop esi
        //TODO - need to return to idle loop not process
            set_needs_reschedule(1);
            return api_open((const char *)edi, (const char *)esi, (uint16_t)edx);
        case API_READ:
        //TODO - need to return to idle loop not process if api_read blocks.  Set this up in the api_read function
            set_needs_reschedule(1);    //TEMPORARY
            return api_read(ebx, (char *)esi, (size_t)ecx);
        case API_WRITE:
        //TODO - need to return to idle loop not process if api_write blocks.  Set this up in the api_write function
            //set_needs_reschedule(1);    //TEMPORARY
            return api_write(ebx, (const char *)esi, (size_t)ecx);
        default:
            return API_ERR_NOTFOUND;
    }
}