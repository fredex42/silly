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

static char *days[] = {"Sat", "Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

/**
 * Ensures that time values are decoded from BCD, if necessary, and normalised to 24 hour.
*/
void rtc_normalise_time(struct RealTimeClockRawData *raw)
{
    if(!(raw->status_b & RTC_STATUSB_FORMAT_BINARY)) {
        //we must decode BCD times
        raw->year = ( (raw->year & 0xF0) >> 1) + ( (raw->year & 0xF0) >> 3) + (raw->year & 0xf);
        raw->month = ( (raw->month & 0xF0) >> 1) + ( (raw->month & 0xF0) >> 3) + (raw->month & 0xf);
        raw->monthday = ( (raw->monthday & 0xF0) >> 1) + ( (raw->monthday & 0xF0) >> 3) + (raw->monthday & 0xf);
        raw->hours = ( (raw->hours & 0xF0) >> 1) + ( (raw->hours & 0xF0) >> 3) + (raw->hours & 0xf);
        raw->minutes = ( (raw->minutes & 0xF0) >> 1) + ( (raw->minutes & 0xF0) >> 3) + (raw->minutes & 0xf);
        raw->seconds = ( (raw->seconds & 0xF0) >> 1) + ( (raw->seconds & 0xF0) >> 3) + (raw->seconds & 0xf);
    }
}

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
Decodes the CMOS RTC values into an epoch time (i.e. seconds since midnight, jan 1st 2000)
Note that this modifies the struct pointed to in its argument - the values are normalised (converted from BCD and 12H if necessary)
*/
uint32_t rtc_raw_data_to_epoch(struct RealTimeClockRawData *raw)
{
    rtc_normalise_time(raw);
    //rtc_print_data(raw);

    //Algorithm courtesy of https://stackoverflow.com/questions/7960318/math-to-convert-seconds-since-1970-into-date-and-vice-versa?noredirect=1&lq=1 !
    // Cumulative days for each previous month of the year
    int mdays[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    int year = (int)raw->year;

    // Compensation of the non-leap years
    int minusYear = 0;
    // Detect potential lead day (February 29th) in this year?
    if ( raw->month >= 3 )
    {
      // Then add this year into "sum of leap days" computation
      year++;
      // Compute one year less in the non-leap years sum
      minusYear = 1;
    }

    int mindex = (raw->month - 1);
    return
      // + Seconds from computed minutes
      60 * (
        // + Minutes from computed hours
        60 * (
          // + Hours from computed days
          24L * (
            // + Day (zero index)
            raw->monthday - 1
            // + days in previous months (leap day not included)
            + mdays[mindex]
            // + days for each year divisible by 4 (starting from 1973)
            + ( ( year + 1 ) / 4 )
            // - days after year 2000
            - ( ( year > 130 ) ? 1 : 0 )
            // + days for each year (as all are non-leap years) from 1970 (minus this year if potential leap day taken into account)
            + ( 5 * 73 ) * ( year - minusYear )
          // + Hours
          ) + raw->hours
        // + Minutes
        ) + raw->minutes
      // + Seconds
      ) + raw->seconds;
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

        if(rtc_raw_data_equal(&reads[0], &reads[1])) {
            kprintf("DEBUG Determined stable time after %d attempts\r\n", i);
            uint32_t epochtime = rtc_raw_data_to_epoch(&reads[0]);
            kprintf("INFO Epoch time calculated as %l\r\n", epochtime);
            return epochtime;
        }
    }
    k_panic("Could not reliably determine RTC time after maximum attempts\r\n");
}

uint32_t rtc_get_epoch_time()
{
    uint32_t ticks = rtc_get_ticks();
    return (uint32_t)((float)ticks / 512.0) + rtc_get_boot_time();
}

uint32_t rtc_get_unix_time()
{
    return (uint32_t)rtc_get_epoch_time() + 946684800;
}