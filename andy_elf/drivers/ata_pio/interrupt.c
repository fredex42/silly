#include <types.h>
#include <stdio.h>
#include <cfuncs.h>
#include <scheduler/scheduler.h>
#include <sys/ioports.h>
#include <panic.h>
#include <memops.h>
#include "ata_pio.h"
#include "ata_readwrite.h"

extern ATADriverState *master_driver_state;

/*
This is called from the (assembly language) IPrimaryATA / ISecondaryATA
handlers to actually work on the event that occurred. This is still a part of
the interrupt handler so care should be taken not to block un-necessarily
*/
void ata_service_interrupt(uint8_t bus_nr)
{
  SchedulerTask *t;
  ATAPendingOperation *op = master_driver_state->pending_disk_operation[bus_nr];
  
  if(op==NULL || op->type==ATA_OP_NONE) {
    kprintf("ERROR Received unexpected notification for IDE bus %d\r\n", (uint16_t)bus_nr);
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
