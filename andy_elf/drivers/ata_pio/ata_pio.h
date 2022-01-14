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

#define ATA_ALTSTATUS(base_addr) base_addr+0x206

#define ATA_CMD_IDENTIFY      0xEC
#define ATA_CMD_READ_SECTORS  0x20

#define ATA_SELECT_MASTER   0xA0
#define ATA_SELECT_SLAVE    0xB0

#define ATA_SELECT_MASTER_READ  0xE0
#define ATA_SELECT_SLAVE_READ   0xF0

/*
Macros to disassamble an LBA28 address from a uint32_t or uint64_t
*/
#define LBA28_HI(value) (uint8_t) ( (value >> 24) & 0xF) //uppermost 4 bits of the lba28 address
#define LBA28_HMID(value) (uint8_t) (value>>16)
#define LBA28_LMID(value) (uint8_t) (value>>8 )
#define LBA28_LO(value) (uint8_t) (value)


typedef struct ata_driver_state {
  uint8_t active_bus_mask;  //bitfield with LSB representing primary and bit 4 representing quaternary

  uint8_t active_drive_count;
  uint16_t *disk_identity[8]; //8 pointers to IDENTITY structures (256 bytes each), PRI MAS, PRI SLV, SEC MAS, SEC SLV etc. Non-present disks are NULL here.

  struct ata_pending_operation *pending_disk_operation[4]; //4 pointers to ATAPendingOperation structures representing pending operations on each _bus_
} ATADriverState;

#define ATA_OP_NONE       0
#define ATA_OP_READ       1
#define ATA_OP_WRITE      2
#define ATA_OP_IGNORE     3

#define ATA_STATUS_OK     0
#define ATA_STATUS_ERR    1

typedef struct ata_pending_operation {
  uint8_t type;

  void *buffer;   //data buffer that is being sent or retrieved
  void *paging_directory; //paging directory for the vmem pointer `buffer`
  size_t buffer_loc;
  uint8_t sector_count;
  uint8_t sectors_read;
  uint16_t base_addr;     //base IO port to do the read from/write to

  void (*completed_func)(uint8_t status, void *buffer);
} ATAPendingOperation;

void print_drive_info(uint8_t drive_nr);
int8_t ata_pio_start_read(uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void (*callback)(uint8_t status, void *buffer));

//ATA error codes
#define ATA_E_AMNF    1<<0  //address mark not found
#define ATA_E_TKZNF   1<<1  //trak 0 not found
#define ATA_E_ABRT    1<<2  //aborted
#define ATA_E_MCR     1<<3  //media change requested
#define ATA_E_IDNF    1<<4  //ID not found
#define ATA_E_MC      1<<5  //media changed
#define ATA_E_UNC     1<<6  //Uncorrectable data error
#define ATA_E_BBK     1<<7  //Bad block detected

//ATA status codes
#define ATA_STATUS_ERR  1<<0
#define ATA_STATUS_DRQ  1<<3
#define ATA_STATUS_SRV  1<<4
#define ATA_STATUS_DF   1<<5  //Drive Fault Error (does not set ERR).
#define ATA_STATUS_RDY  1<<6  //Bit is clear when drive is spun down, or after an error. Set otherwise.
#define ATA_STATUS_BSY  1<<7  //Indicates the drive is preparing to send/receive data (wait for it to clear). In case of 'hang' (it never clears), do a software reset.
#endif
