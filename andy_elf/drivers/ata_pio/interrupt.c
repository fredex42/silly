#include <types.h>
#include <stdio.h>
#include <cfuncs.h>
#include <scheduler/scheduler.h>
#include <sys/ioports.h>
#include <panic.h>
#include <memops.h>
#include "ata_pio.h"

extern ATADriverState *master_driver_state;

void ata_complete_read_lowerhalf(SchedulerTask *t);
void ata_complete_write_lowerhalf(SchedulerTask *t);

/*
This is called from the (assembly language) IPrimaryATA / ISecondaryATA
handlers to actually work on the event that occurred. This is still a part of
the interrupt handler so care should be taken not to block un-necessarily
*/
void ata_service_interrupt(uint8_t bus_nr)
{
  SchedulerTask *t;
  ATAPendingOperation *op = master_driver_state->pending_disk_operation[bus_nr];
  kprintf("DEBUG received ATA service interrupt for %d\r\n", (uint16_t) bus_nr);

  if(op==NULL || op->type==ATA_OP_NONE) {
    kprintf("ERROR Received interrupt notification for IDE bus %d without a pending operation\r\n", (uint16_t)bus_nr);
    return;
  }

  switch(op->type) {
    case ATA_OP_READ:
        //set up a lower-half to complete the read operation
        t = new_scheduler_task(TASK_ASAP, &ata_complete_read_lowerhalf, op);
        if(t==NULL) {
          k_panic("ERROR Could not create schedule task to implement lower-half of ata interrupt\r\n");
          return;
        }
        schedule_task(t);
        break;
    case ATA_OP_WRITE:
      //set up a lower-half to complete the write operation
      t = new_scheduler_task(TASK_ASAP, &ata_complete_write_lowerhalf, op);
      if(t==NULL) {
        k_panic("ERROR Could not create schedule task to implement lower-half of ata interrupt\r\n");
        return;
      }
      schedule_task(t);
      break;
    case ATA_OP_IGNORE:
      kputs("DEBUG Ignoring ATA operation as requested\r\n");
      op->type = ATA_OP_NONE;
      return;
  }
}

void ata_dump_errors(uint8_t err)
{
  kprintf("ERROR ATA error code %d\r\n", (uint16_t)err);
}

/*
This is called by the scheduler from the kernel context
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

void ata_continue_write(ATAPendingOperation *op)
{
  vaddr old_pd = switch_paging_directory_if_required((vaddr)op->paging_directory);

  //need to make sure interrupts are disabled, otherwise we trigger the next data packet
  //before we stored the last word of this one, meaning that we miss data.
  cli();

  kprintf("DEBUG sectors written %d sectors total to write %d\r\n", op->sectors_read, op->sector_count);

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
