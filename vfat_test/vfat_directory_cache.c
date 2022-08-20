#include "vfat_directory_cache.h"

void initialise_directory_cache(FATFS *fs_ptr, uint8_t drive_nr, void (*callback)(FATFS *fs_ptr, uint8_t status, VFAT_DIRECTORY_CACHE_NODE *root));
