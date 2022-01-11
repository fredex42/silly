#include <types.h>

#ifndef __ATA_PIO_H
#define __ATA_PIO_H

#define ATA_PRIMARY_BASE    0x1F0
#define ATA_SECONDARY_BASE  0x170
#define ATA_TERTIARY_BASE   0x1E8
#define ATA_QUATERNARY_BASE 0x168

#define ATA_DATA_REG(base_addr) base_addr+0
#define ATA_ERROR_REG(base_addr) base_addr+1      //only when reading
#define ATA_FEATURES_REG(base_addr) base_addr+1   //only when writing
#define ATA_SECTOR_COUNT(base_addr) base_addr+2
#define ATA_LBA_LOW(base_addr) base_addr+3
#define ATA_LBA_MID(base_addr) base_addr+4
#define ATA_LBA_HI(base_addr) base_addr+5
#define ATA_DRIVE_HEAD(base_addr) base_addr+6
#define ATA_STATUS(base_addr) base_addr+7         //only when reading
#define ATA_COMMAND(base_addr) base_addr+7        //only when writing

#define ATA_CMD_IDENTIFY    0xEC
#define ATA_SELECT_MASTER   0xA0
#define ATA_SELECT_SLAVE    0xB0

typedef struct ata_driver_state {
  uint8_t active_bus_mask;  //bitfield with LSB representing primary and bit 4 representing quaternary

  uint8_t active_drive_count;
  uint16_t *disk_identity[8]; //8 pointers to IDENTITY structures (256 bytes each), PRI MAS, PRI SLV, SEC MAS, SEC SLV etc. Non-present disks are NULL here.


} ATADriverState;

void print_drive_info(uint8_t drive_nr);
#endif
