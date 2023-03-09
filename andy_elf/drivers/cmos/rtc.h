#include <types.h>
#include "wordsize.h"

#ifndef __CMOS_RTC_H
#define __CMOS_RTC_H

#define MAX_RETRIES 10
struct RealTimeClockRawData {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekday;
    uint8_t monthday;
    uint8_t month;
    uint8_t year;
    uint8_t century;    //not always century!
    uint8_t status_a;
    uint8_t status_b;
} __attribute__((packed));


/*
Gets the current time from the CMOS clock and converts it into an Epoch time since Jan 1st, 2000
NOTE - this is slow and should not be used by external callers. Use rtc_get_epoch_time instead.
*/
uint32_t cmos_get_epoch_time();

/*
Gets the current epoch time, since Jan 1st, 2000. This uses the recorded boot time and the
number of ticks that occurred since startup to calculate the time in a more performant manner
than directly reading it from CMOS
*/
uint32_t rtc_get_epoch_time();

/*
Same as rtc_get_epoch_time but returns it since the Unix epoch
*/
uint32_t rtc_get_unix_time();
#endif