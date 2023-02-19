#include <types.h>

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

#endif