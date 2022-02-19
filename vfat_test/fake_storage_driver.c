#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#include "fake_storage_driver.h"

/*
Simulates a driver read on a disk-image file
*/
int8_t fake_driver_start_read(uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata))
{
  uint64_t byte_offset = lba_address * BYTES_PER_LOGICAL_SECTOR;
  uint64_t byte_length = sector_count * BYTES_PER_LOGICAL_SECTOR;

  lseek(drive_nr, byte_offset, SEEK_SET);

  ssize_t bytes_read = 0;
  while(bytes_read < byte_length) {
    ssize_t this_read = read(drive_nr, &((char *)buffer)[bytes_read], byte_length);
    if(this_read==-1) { //read failed
      printf("ERROR Disk read failed\n");
      return -1;
    } else if(this_read==0) { //no data
      printf("ERROR No data to read\n");
      return -1;
    } else {
      bytes_read += this_read;
    }
  }
  callback(0, buffer, extradata);
  return 0;
}

int8_t fake_driver_start_write(uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata))
{
  puts("ERROR fake_driver_start_write not implemented\n");
  return -1;
}

void fake_driver_print_drive_info(uint8_t drive_nr)
{

}

void dispose_fake_storage_driver(FakeStorageDriver *drv)
{
  if(drv->fd>0) close(drv->fd);
  free(drv);
  return;
}

FakeStorageDriver *new_fake_storage_driver(char *filename)
{
  int fd = open(filename, O_RDONLY);
  if(fd==-1) {
    printf("ERROR Could not open %s for read.\n", filename);
    return NULL;
  }

  FakeStorageDriver *drv = (FakeStorageDriver *)malloc(sizeof(FakeStorageDriver));
  drv->fd = fd;
  drv->driver_start_read = &fake_driver_start_read;
  drv->driver_start_write = &fake_driver_start_write;
  drv->driver_print_drive_info = &fake_driver_print_drive_info;
  return drv;
}
