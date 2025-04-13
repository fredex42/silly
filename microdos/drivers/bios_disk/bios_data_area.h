#include <types.h>

#define BIOS_DATA_AREA_START 0xff00000400
#define BIOS_FIXED_DISK_COUNT_PTR (uint8_t *)(BIOS_DATA_AREA_START + 0x75)
