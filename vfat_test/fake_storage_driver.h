#include "generic_storage.h"

#define BYTES_PER_LOGICAL_SECTOR  512

typedef struct fake_storage_driver {
  int8_t (*driver_start_read)(uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata));
  int8_t (*driver_start_write)(uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata));
  void (*driver_print_drive_info)(uint8_t drive_nr);

  int fd;
} FakeStorageDriver;

FakeStorageDriver *new_fake_storage_driver(char *filename);
void dispose_fake_storage_driver(FakeStorageDriver *drv);
