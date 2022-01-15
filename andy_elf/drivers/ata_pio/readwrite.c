#include <types.h>
#include <scheduler/scheduler.h>
#include <cfuncs.h>
#include <sys/ioports.h>
// #include <sys/mmgr.h>
#include <memops.h>
#include <errors.h>
#include <panic.h>
#include "ata_pio.h"
#include "ata_readwrite.h"

extern ATADriverState *master_driver_state;

uint8_t ports_for_drive_nr(uint8_t drive_nr, uint16_t *base_addr, uint8_t *selector)
{
  switch(drive_nr) {
    case 0:
      *base_addr = ATA_PRIMARY_BASE;
      *selector = ATA_SELECT_MASTER_READ;
      break;
    case 1:
      *base_addr = ATA_PRIMARY_BASE;
      *selector = ATA_SELECT_SLAVE_READ;
      break;
    case 2:
      *base_addr = ATA_SECONDARY_BASE;
      *selector = ATA_SELECT_MASTER_READ;
      break;
    case 3:
      *base_addr = ATA_SECONDARY_BASE;
      *selector = ATA_SELECT_SLAVE_READ;
      break;
    case 4:
      *base_addr = ATA_TERTIARY_BASE;
      *selector = ATA_SELECT_MASTER_READ;
      break;
    case 5:
      *base_addr = ATA_TERTIARY_BASE;
      *selector = ATA_SELECT_SLAVE_READ;
      break;
    case 6:
      *base_addr = ATA_QUATERNARY_BASE;
      *selector = ATA_SELECT_MASTER_READ;
      break;
    case 7:
      *base_addr = ATA_QUATERNARY_BASE;
      *selector = ATA_SELECT_SLAVE_READ;
      break;
    default:
      kprintf("ERROR attempt to read from invalid drive number %d\r\n", (uint16_t) drive_nr);
      return E_PARAMS;
  }

  return E_OK;
}

/*
Initialises a read operation on the given drive.
This should be called with interrupts disabled.
It will return E_BUSY if another operation is already pending, for this reason should be considered
"internal" as it needs queueing in front of it.
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

  //kprintf("DEBUG bus_nr is %d\r\n", (uint16_t) bus_nr);

  //if we have an operation already pending the caller must wait for that to finish
  if(master_driver_state->pending_disk_operation[bus_nr]->type != ATA_OP_NONE) {
    kprintf("DEBUG pending operation is type %d\r\n", (uint16_t)master_driver_state->pending_disk_operation[bus_nr]->type  );
    return E_BUSY;
  }

  uint8_t rv = ports_for_drive_nr(drive_nr, &base_addr, &selector);
  if(rv != E_OK) return rv;

  //We will receive an interrupt when the drive has the data ready for us
  //so store the fact we are waiting for an operation so it can be picked up later.
  //we should get an interrupt for every sector.
  ATAPendingOperation *op = master_driver_state->pending_disk_operation[drive_nr >> 1];
  op->type = ATA_OP_READ;
  op->sector_count = sector_count;
  op->device = drive_nr & 0x1;  //just take the leftmost bit, 0=>master, 1=>slave
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

int8_t ata_pio_start_write(uint8_t drive_nr, uint64_t lba_address, uint8_t sector_count, void *buffer, void (*callback)(uint8_t status, void *buffer))
{
  uint16_t base_addr;
  uint8_t selector;

  uint8_t bus_nr = (uint8_t) ((drive_nr & 0xF) >> 1);
  if(master_driver_state==NULL || master_driver_state->pending_disk_operation[drive_nr >> 1]==NULL) {
    k_panic("ERROR Requested disk read before ATA subsystem was initialised\r\n");
    return E_BUSY;
  }

  //kprintf("DEBUG bus_nr is %d\r\n", (uint16_t) bus_nr);

  //if we have an operation already pending the caller must wait for that to finish
  if(master_driver_state->pending_disk_operation[bus_nr]->type != ATA_OP_NONE) {
    kprintf("DEBUG pending operation is type %d\r\n", (uint16_t)master_driver_state->pending_disk_operation[bus_nr]->type  );
    return E_BUSY;
  }

  uint8_t rv = ports_for_drive_nr(drive_nr, &base_addr, &selector);
  if(rv != E_OK) return rv;

  //We will receive an interrupt when the drive has the data ready for us
  //so store the fact we are waiting for an operation so it can be picked up later.
  //we should get an interrupt for every sector.
  ATAPendingOperation *op = master_driver_state->pending_disk_operation[drive_nr >> 1];
  op->type = ATA_OP_WRITE;
  op->sector_count = sector_count;
  op->device = drive_nr & 0x1;  //just take the leftmost bit, 0=>master, 1=>slave
  op->buffer = buffer;
  op->paging_directory = get_current_paging_directory();
  op->buffer_loc = 0;
  op->base_addr = base_addr;
  op->sectors_read = 0;
  op->completed_func = callback;

  //request an LBA28 write
  outb(ATA_DRIVE_HEAD(base_addr), selector | LBA28_HI(lba_address));
  outb(ATA_SECTOR_COUNT(base_addr), sector_count);
  outb(ATA_LBA_LOW(base_addr), LBA28_LO(lba_address));
  outb(ATA_LBA_MID(base_addr), LBA28_LMID(lba_address));
  outb(ATA_LBA_HI(base_addr), LBA28_HMID(lba_address));
  outb(ATA_COMMAND(base_addr), ATA_CMD_WRITE_SECTORS);

  ata_continue_write(op); //write the first sector. Second one will get set by the interrupt.
}

/*
This is a "lower half", i.e. a routine that is run by the scheduler from the root
kernel event loop. It's requested by the interrupt handler and does the actual work
of reading bytes from the disk and transferring them into memory.
Arguments: t - pointer to the SchedulerTask that was used to queue the event. The
      "data" field can be assumed to be a pointer to ATAPendingOperation which
      gives details of the event in progress
Returns: Nothing
*/
void ata_complete_read_lowerhalf(SchedulerTask *t)
{
  ATAPendingOperation *op = (ATAPendingOperation *)t->data;

  kprintf("DEBUG in disk read lower-half. Requested paging dir is 0x%x\r\n", op->paging_directory);

  vaddr old_pd = switch_paging_directory_if_required((vaddr)op->paging_directory);

  //need to make sure interrupts are disabled, otherwise we trigger the next data packet
  //before we stored the last word of this one, meaning that we miss data.
  cli();
  //each sector is 512 bytes (or 256 words)
  uint16_t *buf = (uint16_t *)op->buffer;
  for(register size_t i=op->buffer_loc; i<op->buffer_loc+256;i++) {
    if(i==op->buffer_loc+255) inb(ATA_STATUS(op->base_addr)); //read status byte to reset interrupt flag. We don't actually care about the values right now.
    buf[i] = inw(ATA_DATA_REG(op->base_addr));
  }

  op->buffer_loc += 256;
  ++op->sectors_read;
  if(op->sectors_read>=op->sector_count) {
    //reset the "pending operation" block for the next operation
    //memset((void *)op, 0, sizeof(ATAPendingOperation));
    op->type = ATA_OP_NONE;
    //the operation is now completed
    op->completed_func(ATA_STATUS_OK, op->buffer);
  }

  if(old_pd!=NULL) switch_paging_directory_if_required(old_pd);

  sti();

}

/*
This is an internal routine that writes a block of bytes to the disk from a buffer.
It's called from `ata_pio_start_write` to write the first sector requested and then
from `ata_complete_write_lowerhalf` for each subsequent write.
Arguments: op - pointer to an ATAPendingOperation giving details of the operation in progress
Returns: Nothing
*/
void ata_continue_write(ATAPendingOperation *op)
{
  vaddr old_pd = switch_paging_directory_if_required((vaddr)op->paging_directory);

  //need to make sure interrupts are disabled, otherwise we trigger the next data packet
  //before we stored the last word of this one, meaning that we miss data.
  cli();

  //kprintf("DEBUG sectors written %d sectors total to write %d\r\n", op->sectors_read, op->sector_count);

  //each sector is 512 bytes (or 256 words)
  uint16_t *buf = (uint16_t *)op->buffer;
  for(register size_t i=op->buffer_loc; i<op->buffer_loc+256;i++) {
    if(i==op->buffer_loc+255) inb(ATA_STATUS(op->base_addr)); //read status byte to reset interrupt flag. We don't actually care about the values right now.
    outw(ATA_DATA_REG(op->base_addr), buf[i]);
    io_wait();  //in ioports.asm
  }

  outb(ATA_COMMAND(op->base_addr), ATA_CMD_CACHE_FLUSH);  //make sure to flush the cache once the write has completed

  op->buffer_loc += 256;
  ++op->sectors_read;


  if(old_pd!=NULL) switch_paging_directory_if_required(old_pd);

  sti();
}


/*
This is a "lower half", i.e. a routine that is run by the scheduler from the root
kernel event loop. It's requested by the interrupt handler and does the actual work
of taking bytes from memory and writing them to disk.
Arguments: t - pointer to the SchedulerTask that was used to queue the event. The
      "data" field can be assumed to be a pointer to ATAPendingOperation which
      gives details of the event in progress
Returns: Nothing
*/
void ata_complete_write_lowerhalf(SchedulerTask *t)
{
  ATAPendingOperation *op = (ATAPendingOperation *)t->data;

  kprintf("DEBUG in disk write lower-half. Requested paging dir is 0x%x\r\n", op->paging_directory);

  if(op->sectors_read>=op->sector_count) {
    //the operation is now completed
    op->completed_func(ATA_STATUS_OK, op->buffer);
    //reset the "pending operation" block for the next operation
    op->type = ATA_OP_NONE;
  } else {
      ata_continue_write(op);
  }
}
