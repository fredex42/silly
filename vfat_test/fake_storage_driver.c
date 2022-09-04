#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "fake_storage_driver.h"

extern int errno;

/*
Simulates a driver read on a disk-image file
*/
int8_t fake_driver_start_read(void *fs_ptr, uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *fs_ptr, void *extradata))
{
  uint64_t byte_offset = lba_address * BYTES_PER_LOGICAL_SECTOR;
  uint64_t byte_length = sector_count * BYTES_PER_LOGICAL_SECTOR;
  printf("DEBUG Disk reading 0x%x sectors from lba address 0x%x\n", (uint32_t)sector_count, lba_address);
  printf("DEBUG That's 0x%x bytes from an offset of 0x%x\n", byte_length, byte_offset);

  off_t seeked = lseek(drive_nr, byte_offset, SEEK_SET);
  switch(seeked) {
    case (off_t)-1:
      printf("ERROR Could not seek to offset: %d\n", errno);
      callback(errno, buffer,fs_ptr, extradata);
      return errno;
    default:
      printf("INFO Seeked to 0x%x\n", seeked);
      break;
  }
  ssize_t bytes_read = 0;
  while(bytes_read < byte_length) {
    ssize_t this_read = read(drive_nr, buffer + bytes_read, byte_length);
    if(this_read==-1) { //read failed
      printf("ERROR Disk read failed reading %d sectors from %ld on drive %d. errno was %d.\n", (uint32_t)sector_count, lba_address, drive_nr, errno);
      return -1;
    } else if(this_read==0) { //no data
      printf("ERROR No data to read\n");
      return -1;
    } else {
      bytes_read += this_read;
      printf("DEBUG Read 0x%x bytes\n", bytes_read);
    }
  }
  callback(0, buffer, fs_ptr, extradata);
  return 0;
}

int8_t fake_driver_start_write(void *fs_ptr, uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *fs_ptr, void *extradata))
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
