#include <types.h>

/**
 * Initialise the volume manager and return a global reference.
 */
struct VolMgr *volmgr_init(uint8_t bios_disk_number);