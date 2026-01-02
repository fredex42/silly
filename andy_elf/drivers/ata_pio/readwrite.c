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
int8_t ata_pio_start_read(uint8_t drive_nr, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata))
{
  uint16_t base_addr;
  uint8_t selector;

  // Input parameter validation
  if(buffer == NULL) {
    kprintf("ERROR: NULL buffer pointer passed to ata_pio_start_read\r\n");
    return E_PARAMS;
  }
  if(callback == NULL) {
    kprintf("ERROR: NULL callback function passed to ata_pio_start_read\r\n");
    return E_PARAMS;
  }
  if(sector_count == 0) {
    kprintf("ERROR: Zero sector count passed to ata_pio_start_read\r\n");
    return E_PARAMS;
  }
  if(lba_address > 0xFFFFFFF) {  // LBA28 maximum
    kprintf("ERROR: LBA address 0x%x exceeds LBA28 limit\r\n", (uint32_t)lba_address);
    return E_PARAMS;
  }

  uint8_t bus_nr = (uint8_t) ((drive_nr & 0xF) >> 1);
  
  // Critical bounds check: pending_disk_operation array only has 4 elements
  if(bus_nr >= 4) {
    kprintf("ERROR Invalid drive number %d (bus_nr %d >= 4)\r\n", (uint16_t)drive_nr, (uint16_t)bus_nr);
    return E_PARAMS;
  }
  
  if(master_driver_state==NULL || master_driver_state->pending_disk_operation[bus_nr]==NULL) {
    k_panic("ERROR Requested disk read before ATA subsystem was initialised\r\n");
    return E_BUSY;
  }

  //if we have an operation already pending the caller must wait for that to finish
  if(master_driver_state->pending_disk_operation[bus_nr]->type != ATA_OP_NONE) {
    return E_BUSY;
  }

  uint8_t rv = ports_for_drive_nr(drive_nr, &base_addr, &selector);
  if(rv != E_OK) return rv;

  //ATA28 sector count is 8-bit, so max 255 sectors per command (0 means 256)
  //For larger reads, we need to limit to 255 sectors and let the completion handler continue
  uint8_t current_sector_count = (sector_count > 255) ? 255 : (uint8_t)sector_count;

  //We will receive an interrupt when the drive has the data ready for us
  //so store the fact we are waiting for an operation so it can be picked up later.
  //we should get an interrupt for every sector.
  ATAPendingOperation *op = master_driver_state->pending_disk_operation[bus_nr];
  op->type = ATA_OP_READ;
  op->sector_count = sector_count;  // Total sectors requested
  op->device = drive_nr & 0x1;  //just take the leftmost bit, 0=>master, 1=>slave
  op->buffer = buffer;
  op->extradata = extradata;
  op->paging_directory = get_current_paging_directory();
  op->buffer_loc = 0;
  op->base_addr = base_addr;
  op->sectors_read = 0;
  op->start_lba = lba_address;
  op->current_lba = lba_address;
  op->continuation_pending = 0;  // Initialize continuation flag
  op->completed_func = callback;

  //request an LBA28 read
  outb(ATA_DRIVE_HEAD(base_addr), selector | LBA28_HI(lba_address));
  outb(ATA_SECTOR_COUNT(base_addr), current_sector_count);
  outb(ATA_LBA_LOW(base_addr), LBA28_LO(lba_address));
  outb(ATA_LBA_MID(base_addr), LBA28_LMID(lba_address));
  outb(ATA_LBA_HI(base_addr), LBA28_HMID(lba_address));
  outb(ATA_COMMAND(base_addr), ATA_CMD_READ_SECTORS);
  return E_OK;
}

int8_t ata_pio_start_write(uint8_t drive_nr, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata))
{
  uint16_t base_addr;
  uint8_t selector;

  // Input parameter validation
  if(buffer == NULL) {
    kprintf("ERROR: NULL buffer pointer passed to ata_pio_start_write\r\n");
    return E_PARAMS;
  }
  if(callback == NULL) {
    kprintf("ERROR: NULL callback function passed to ata_pio_start_write\r\n");
    return E_PARAMS;
  }
  if(sector_count == 0) {
    kprintf("ERROR: Zero sector count passed to ata_pio_start_write\r\n");
    return E_PARAMS;
  }
  if(lba_address > 0xFFFFFFF) {  // LBA28 maximum
    kprintf("ERROR: LBA address 0x%x exceeds LBA28 limit\r\n", (uint32_t)lba_address);
    return E_PARAMS;
  }

  uint8_t bus_nr = (uint8_t) ((drive_nr & 0xF) >> 1);
  
  // Critical bounds check: pending_disk_operation array only has 4 elements
  if(bus_nr >= 4) {
    kprintf("ERROR Invalid drive number %d (bus_nr %d >= 4)\r\n", (uint16_t)drive_nr, (uint16_t)bus_nr);
    return E_PARAMS;
  }
  
  if(master_driver_state==NULL || master_driver_state->pending_disk_operation[bus_nr]==NULL) {
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

  //ATA28 sector count is 8-bit, so max 255 sectors per command (0 means 256)
  //For larger writes, we need to limit to 255 sectors and let the completion handler continue
  uint8_t current_sector_count = (sector_count > 255) ? 255 : (uint8_t)sector_count;

  //We will receive an interrupt when the drive has the data ready for us
  //so store the fact we are waiting for an operation so it can be picked up later.
  //we should get an interrupt for every sector.
  ATAPendingOperation *op = master_driver_state->pending_disk_operation[bus_nr];
  op->type = ATA_OP_WRITE;
  op->sector_count = sector_count;
  op->device = drive_nr & 0x1;  //just take the leftmost bit, 0=>master, 1=>slave
  op->buffer = buffer;
  op->paging_directory = get_current_paging_directory();
  op->buffer_loc = 0;
  op->base_addr = base_addr;
  op->extradata = extradata;
  op->sectors_read = 0;
  op->completed_func = callback;

  //request an LBA28 write
  outb(ATA_DRIVE_HEAD(base_addr), selector | LBA28_HI(lba_address));
  outb(ATA_SECTOR_COUNT(base_addr), current_sector_count);
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
  
  // Sanity check the operation structure
  if(op == NULL) {
    k_panic("NULL ATAPendingOperation pointer");
  }
  if(op->type != ATA_OP_READ) {
    kprintf("ERROR: Invalid operation type: %d\r\n", (int)op->type);
    k_panic("Invalid ATA operation type");
  }
  if(op->buffer == NULL) {
    k_panic("NULL buffer pointer in ATAPendingOperation");
  }

  vaddr old_pd = switch_paging_directory_if_required((vaddr)op->paging_directory);

  //need to make sure interrupts are disabled, otherwise we trigger the next data packet
  //before we stored the last word of this one, meaning that we miss data.
  cli();
  //each sector is 512 bytes (or 256 words)
  uint16_t *buf = (uint16_t *)op->buffer;
  
  
  // Quick sanity check
  size_t expected_buffer_loc = (size_t)op->sectors_read * 256;
  if(op->buffer_loc != expected_buffer_loc) {
    kprintf("ERROR: buffer_loc mismatch!\r\n");
    k_panic("buffer_loc corruption detected");
  }
  
  // Check for potential buffer overrun before reading
  size_t words_needed = op->buffer_loc + 256;
  size_t buffer_words = op->sector_count * 256;  // Total buffer capacity in words
  if(words_needed > buffer_words) {
    kprintf("ERROR: About to overrun buffer! words_needed=%d, buffer_words=%d\r\n", 
            (uint32_t)words_needed, (uint32_t)buffer_words);
    k_panic("Buffer overrun detected before sector read");
  }
  
  for(register size_t i=0; i<256; i++) {
    if(i==255) inb(ATA_STATUS(op->base_addr)); //read status byte to reset interrupt flag. We don't actually care about the values right now.
    buf[op->buffer_loc + i] = inw(ATA_DATA_REG(op->base_addr));
  }

  op->buffer_loc += 256;
  ++op->sectors_read;
  ++op->current_lba;
  
  if(op->sectors_read>=op->sector_count) {
    //All sectors completed - finish the operation
    op->type = ATA_OP_NONE;
    if(!op->completed_func) {
      kprintf("ERROR disk operation with NULL callback, this causes a memory leak\r\n");
    } else {
      op->completed_func(ATA_STATUS_OK, op->buffer, op->extradata);
    }
  } else {
    //Check if we need to issue another read command for remaining sectors
    uint16_t remaining_sectors = op->sector_count - op->sectors_read;
    
    // Prevent multiple continuation tasks for the same operation
    if(op->continuation_pending) {
      kprintf("WARNING: Continuation already pending for sectors_read=%d\r\n", (uint16_t)op->sectors_read);
      if(old_pd!=0) switch_paging_directory_if_required(old_pd);
      sti();
      return;
    }
    
    //Issue another hardware command for the remaining sectors
    uint8_t next_sector_count = (remaining_sectors > 255) ? 255 : (uint8_t)remaining_sectors;
    
    //kprintf("cont:s=%d\r\n", (uint16_t)op->sectors_read);
    
    //Update current LBA for the next chunk
    op->current_lba = op->start_lba + op->sectors_read;
    
    //Set flag to prevent duplicate continuation tasks
    op->continuation_pending = 1;
    
    //Schedule a task to issue the next hardware command
    //This avoids potential race conditions with interrupt handling
    SchedulerTask *continue_task = new_scheduler_task(TASK_ASAP, &ata_continue_read_chunk, op);
    if(continue_task == NULL) {
      k_panic("Could not create scheduler task for ATA chunk continuation");
    }
    schedule_task(continue_task);
  }

  if(old_pd!=0) switch_paging_directory_if_required(old_pd);

  sti();

}

/*
This function is called by the scheduler to issue the next chunk of an ATA read command.
It's used to avoid issuing hardware commands directly from interrupt context.
Arguments: t - scheduler task containing ATAPendingOperation as data
Returns: Nothing
*/
void ata_continue_read_chunk(SchedulerTask *t)
{
  ATAPendingOperation *op = (ATAPendingOperation *)t->data;
  
  if(op == NULL || op->type != ATA_OP_READ) {
    k_panic("Invalid operation in ata_continue_read_chunk");
  }
  
  // Switch to the correct paging directory for this operation
  vaddr old_pd = switch_paging_directory_if_required((vaddr)op->paging_directory);
  
  // Clear the continuation pending flag since we're now handling it
  op->continuation_pending = 0;
  
  //Calculate next chunk size
  uint16_t remaining_sectors = op->sector_count - op->sectors_read;
  uint8_t next_sector_count = (remaining_sectors > 255) ? 255 : (uint8_t)remaining_sectors;
  
  //kprintf("chunk:%d@0x%x\r\n", next_sector_count, (uint32_t)op->current_lba);
  
  //Issue the next read command
  uint8_t selector = 0xE0;  //LBA28 mode, master drive by default
  if(op->device) selector |= 0x10;  //set slave bit if needed
  
  outb(ATA_DRIVE_HEAD(op->base_addr), selector | LBA28_HI(op->current_lba));
  outb(ATA_SECTOR_COUNT(op->base_addr), next_sector_count);
  outb(ATA_LBA_LOW(op->base_addr), LBA28_LO(op->current_lba));
  outb(ATA_LBA_MID(op->base_addr), LBA28_LMID(op->current_lba));
  outb(ATA_LBA_HI(op->base_addr), LBA28_HMID(op->current_lba));
  outb(ATA_COMMAND(op->base_addr), ATA_CMD_READ_SECTORS);
  
  // Restore the original paging directory
  if(old_pd != 0) switch_paging_directory_if_required(old_pd);
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
  // Sanity check the operation structure
  if(op == NULL) {
    k_panic("NULL ATAPendingOperation pointer in ata_continue_write");
  }
  if(op->buffer == NULL) {
    k_panic("NULL buffer pointer in ATAPendingOperation");
  }

  vaddr old_pd = switch_paging_directory_if_required((vaddr)op->paging_directory);

  //need to make sure interrupts are disabled, otherwise we trigger the next data packet
  //before we stored the last word of this one, meaning that we miss data.
  cli();

  // Check for potential buffer overrun before writing
  size_t words_needed = op->buffer_loc + 256;
  size_t buffer_words = op->sector_count * 256;  // Total buffer capacity in words
  if(words_needed > buffer_words) {
    kprintf("ERROR: About to overrun buffer in write! words_needed=%d, buffer_words=%d\r\n", 
            (uint32_t)words_needed, (uint32_t)buffer_words);
    if(old_pd!=0) switch_paging_directory_if_required(old_pd);
    sti();
    k_panic("Buffer overrun detected before sector write");
  }

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


  if(old_pd!=0) switch_paging_directory_if_required(old_pd);

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

  if(op->sectors_read>=op->sector_count) {
    //the operation is now completed
    op->completed_func(ATA_STATUS_OK, op->buffer, op->extradata);
    //reset the "pending operation" block for the next operation
    op->type = ATA_OP_NONE;
  } else {
      ata_continue_write(op);
  }
}
