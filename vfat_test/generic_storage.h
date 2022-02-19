#ifdef __BUILDING_TESTS
#include <stdint.h>
#else
#include <types.h>
#endif

#ifndef __DRIVER_GENERIC_STORAGE_H
#define __DRIVER_GENERIC_STORAGE_H

/*
This structure contains function pointers to low-level driver functions.
It allows us to abstract the low-level driver from the filesystem
*/
typedef struct generic_storage_driver {
    int8_t (*driver_start_read)(uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata));
    int8_t (*driver_start_write)(uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata));
    void (*driver_print_drive_info)(uint8_t drive_nr);
} GenericStorageDriver;

#define ATA_STATUS_OK     0
#define ATA_STATUS_ERR    1

#endif
