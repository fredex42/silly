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
  kprintf("ata_service_interrupt master_driver_state=0x%x bus_nr=%d\r\n", master_driver_state, bus_nr);
  
  // Check if master_driver_state is accessible
  if(!master_driver_state) {
    kprintf("ERROR master_driver_state is NULL\r\n");
    return;
  }
  
  // Check if bus_nr is valid
  if(bus_nr >= 2) {  // Assuming max 2 ATA buses
    kprintf("ERROR invalid bus_nr %d\r\n", bus_nr);
    return;
  }
  
  kprintf("About to access pending_disk_operation[%d]\r\n", bus_nr);
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
