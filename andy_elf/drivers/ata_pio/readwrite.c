#include <types.h>
#include <scheduler/scheduler.h>
#include <cfuncs.h>
#include <sys/ioports.h>
// #include <sys/mmgr.h>
#include <memops.h>
#include <errors.h>
#include <panic.h>
#include "ata_pio.h"

extern ATADriverState *master_driver_state;

/*
Initialises a read operation on the given drive.
This should be called with interrupts disabled.
*/
int8_t ata_pio_start_read(uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void (*callback)(uint8_t status, void *buffer))
{
  uint16_t base_addr;
  uint8_t selector;

  uint8_t bus_nr = (uint8_t) ((drive_nr & 0xF) >> 1);
  if(master_driver_state==NULL || master_driver_state->pending_disk_operation[drive_nr >> 1]==NULL) {
    k_panic("ERROR Requested disk read before ATA subsystem was initialised\r\n");
    return E_BUSY;
  }

  kprintf("DEBUG bus_nr is %d\r\n", (uint16_t) bus_nr);

  //if we have an operation already pending the caller must wait for that to finish
  if(master_driver_state->pending_disk_operation[bus_nr]->type != ATA_OP_NONE) {
    kprintf("DEBUG pending operation is type %d\r\n", (uint16_t)master_driver_state->pending_disk_operation[bus_nr]->type  );
    return E_BUSY;
  }

  switch(drive_nr) {
    case 0:
      base_addr = ATA_PRIMARY_BASE;
      selector = ATA_SELECT_MASTER_READ;
      break;
    case 1:
      base_addr = ATA_PRIMARY_BASE;
      selector = ATA_SELECT_MASTER_READ;
      break;
    case 2:
      base_addr = ATA_PRIMARY_BASE;
      selector = ATA_SELECT_MASTER_READ;
      break;
    case 3:
      base_addr = ATA_PRIMARY_BASE;
      selector = ATA_SELECT_MASTER_READ;
      break;
    case 4:
      base_addr = ATA_PRIMARY_BASE;
      selector = ATA_SELECT_MASTER_READ;
      break;
    case 5:
      base_addr = ATA_PRIMARY_BASE;
      selector = ATA_SELECT_MASTER_READ;
      break;
    case 6:
      base_addr = ATA_PRIMARY_BASE;
      selector = ATA_SELECT_MASTER_READ;
      break;
    case 7:
      base_addr = ATA_PRIMARY_BASE;
      selector = ATA_SELECT_MASTER_READ;
      break;
    case 8:
      base_addr = ATA_PRIMARY_BASE;
      selector = ATA_SELECT_MASTER_READ;
      break;
    default:
      kprintf("ERROR attempt to read from invalid drive number %d\r\n", (uint16_t) drive_nr);
      return E_PARAMS;
  }

  //We will receive an interrupt when the drive has the data ready for us
  //so store the fact we are waiting for an operation so it can be picked up later.
  //we should get an interrupt for every sector.
  ATAPendingOperation *op = master_driver_state->pending_disk_operation[drive_nr >> 1];
  op->type = ATA_OP_READ;
  op->sector_count = sector_count;
  op->buffer = buffer;
  op->paging_directory = get_current_paging_directory();
  op->buffer_loc = 0;
  op->base_addr = base_addr;
  op->sectors_read = 0;
  op->completed_func = callback;

  //request an LBA28 read
  outb(ATA_DRIVE_HEAD(base_addr), selector | LBA28_HI(lba_address));
  outb(ATA_SECTOR_COUNT(base_addr), sector_count);
  outb(ATA_LBA_LOW(base_addr), LBA28_LO(lba_address));
  outb(ATA_LBA_MID(base_addr), LBA28_LMID(lba_address));
  outb(ATA_LBA_HI(base_addr), LBA28_HMID(lba_address));
  outb(ATA_COMMAND(base_addr), ATA_CMD_READ_SECTORS);

}
