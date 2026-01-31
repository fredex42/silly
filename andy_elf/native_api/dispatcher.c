#include <types.h>
#include <native_api/apicodes.h>
#include <drivers/cmos/rtc.h>
#include "dispatcher.h"
#include "process_ops.h"
#include "stream_ops.h"

uint32_t native_api_dispatcher(uint32_t call_id, uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi) {
    switch(call_id) {
        case API_EXIT:
        //TODO - need to return to idle loop not process
            api_terminate_current_process(ebx);
            return 0;
        case API_SLEEP:
        //TODO - need to return to idle loop not process
            api_sleep_current_process(ebx);
            return 0;
        case API_GET_TIME:
            return rtc_get_epoch_time();
        case API_CREATE_PROCESS:
        //TODO - need to return to idle loop not process
            return api_create_process((const char*)ebx, (const char*)ecx);
        case API_CLOSE:
            return api_close(ebx);
        case API_OPEN:
        // Note: edi = filename, esi = extension, edx = mode_flags. TODO - unified extension and filename, drop esi
        //TODO - need to return to idle loop not process
            return api_open((const char *)edi, (const char *)esi, (uint16_t)edx);
        case API_READ:
        //TODO - need to return to idle loop not process if api_read blocks.  Set this up in the api_read function
            return api_read(ebx, (char *)esi, (size_t)ecx);
        case API_WRITE:
        //TODO - need to return to idle loop not process if api_write blocks.  Set this up in the api_write function
            return api_write(ebx, (const char *)esi, (size_t)ecx);
        default:
            return API_ERR_NOTFOUND;
    }
}