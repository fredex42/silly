#include "rtc.h"
#include "lowlevel.h"
#include <panic.h>
#include <stdio.h>

/*
Returns 1 if the time represented by the two structs are equal, 0 otherwise.
*/
uint8_t rtc_raw_data_equal(struct RealTimeClockRawData *a, struct RealTimeClockRawData *b)
{
    if(
        a->seconds == b->seconds &&
        a->minutes == b->minutes &&
        a->hours == b->hours &&
        a->weekday == b->weekday &&
        a->monthday == b->monthday &&
        a->month == b->month &&
        a->year == b->year &&
        a->century == b->century
    ) {
        return 1;
    } else {
        return 0;
    }
}

static char *days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

/*
Dumps the RTC time onto the screen
*/
void rtc_print_data(struct RealTimeClockRawData *raw)
{
    if(raw->status_b & RTC_STATUSB_FORMAT_24H) kputs("INFO System time in 24 hour\r\n"); else kputs("INFO System time in 12 hour");
    if(raw->status_b & RTC_STATUSB_FORMAT_BINARY) kputs("INFO System time in regular encoding\r\n"); else kputs("INFO System time in BCD encoding\r\n");

    kprintf("INFO Raw system time is %s %d/%d/%d at %d:%d:%d\r\n", days[raw->weekday], raw->monthday, raw->month, raw->year, raw->hours, raw->minutes, raw->seconds);
}

/*
Decodes the CMOS RTC values into an epoch time
*/
uint32_t rtc_raw_data_to_epoch(struct RealTimeClockRawData *raw)
{
    rtc_print_data(raw);

}

uint32_t cmos_get_epoch_time()
{
/* 
    See https://wiki.osdev.org/CMOS#RTC_Update_In_Progress.
    In a nutshell, any RTC read can be inconsistent if an update occurs.
    To avoid this, we must:
    - check that the update flag is clear
    - read the values
    - check that the update flag is clear again
    - read the values again
    - check if the values match.
    If the values match then we are good, if they don't match then we must retry.
*/
    struct RealTimeClockRawData reads[2];
    
    for(size_t i=0; i<MAX_RETRIES;i++) {
        while(cmos_get_update_in_progress()) { }    //wait until the update in progress is 0
        cmos_read_rtc_raw(&reads[0]);
        while(cmos_get_update_in_progress()) { }    //wait until the update in progress is 0
        cmos_read_rtc_raw(&reads[1]);

        if(rtc_raw_data_equal(&reads[0], &reads[1])) return rtc_raw_data_to_epoch(&reads[0]);
    }
    k_panic("Could not reliably determine RTC time after maximum attempts\r\n");
}

