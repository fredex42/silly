#ifdef __BUILDING_TESTS
#include <stdlib.h> //for malloc(), free
#include <string.h> //for memset()
#include <stdio.h>  //for printf()
#endif

#include "generic_storage.h"
#include "fat_fs.h"
#include "cluster_map.h"
#include "ucs_conv.h"
#include "string_helpers.h"



void fat_fs_unmount(struct fat_fs *fs_ptr)
{

}





/**
Initialises a new FATFS struct.
Returns NULL if unsuccessful (could not allocate) or the initialised FATFS if successul.
You should call the `mount` function on the returned FATFS
structure to access it.
*/
FATFS* new_fat_fs(uint8_t drive_nr)
{
  FATFS *newstruct = (FATFS *)malloc(sizeof(FATFS));
  if(newstruct==NULL) return NULL;
  memset(newstruct, 0, sizeof(FATFS));

  newstruct->drive_nr = drive_nr;
  // newstruct->mount = &fat_fs_mount;
  // newstruct->unmount = &fat_fs_unmount;
  // newstruct->find_file = &fat_fs_find_file;
  newstruct->busy = 0;
  return newstruct;
}
