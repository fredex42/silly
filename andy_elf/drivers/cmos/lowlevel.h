#include <types.h>

void cmos_read_rtc_raw(struct RealTimeClockRawData* dest);
uint8_t cmos_get_update_in_progress();
uint8_t cmos_read(uint8_t reg);
void cmos_write(uint8_t reg, uint8_t value);

#define RTC_STATUSB_FORMAT_24H      0x02        //bit 1 of status b set => 24 hour. clear => 12 hour
#define RTC_STATUSB_FORMAT_BINARY   0x04        //bit 2 of status b set => binary (i.e. sensible encoding). clear => BCD