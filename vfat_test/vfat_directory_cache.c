#ifdef __BUILDING_TESTS
#include <stdlib.h> //for malloc(), free
#include <string.h> //for memset()
#include <stdio.h>  //for printf()
#endif

#include "fat_fs.h"
#include "vfat_directory_cache.h"

void scan_directory(FATFS *fs_ptr, uint32_t starting_cluster, VFAT_DIRECTORY_CACHE_DATA *data, void (*callback)(FATFS *fs_ptr, uint8_t status,VFAT_DIRECTORY_CACHE_DATA *data))
{

}

void initialise_directory_cache(FATFS *fs_ptr,
   void (*callback)(FATFS *fs_ptr, uint8_t status, VFAT_DIRECTORY_CACHE_NODE *root))
{

}
