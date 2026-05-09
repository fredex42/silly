#include <types.h>
#include <stdio.h>
#include <cfuncs.h>
#include <scheduler/scheduler.h>
#include <sys/ioports.h>
#include <panic.h>
#include <memops.h>
// #include "ata_pio.h"
// #include "ata_readwrite.h"
#include "ata_bus.h"

//extern ATADriverState *master_driver_state;

/*
This is called from the (assembly language) IPrimaryATA / ISecondaryATA
handlers to actually work on the event that occurred. This is still a part of
the interrupt handler so care should be taken not to block un-necessarily
*/
void ata_service_interrupt(uint32_t irq_num)
{
  SchedulerTask *t = NULL;

  kprintf("DEBUG Received ATA interrupt on IRQ %d\r\n", irq_num);
  struct AtaBus *bus = ata_bus_get_by_irq(irq_num);
  if(bus == NULL) {
    kprintf("ERROR Received ATA interrupt for unknown IRQ %d\r\n", irq_num);
    return;
  }

  switch(bus->state) {
    case ATA_STATE_PENDING_READ:
      bus->state = ATA_STATE_PENDING_READ_DRAIN;
      ata_bus_clear_interrupts(bus); // Prevent further interrupts until the lower half has drained the disk buffer
      t = new_scheduler_task(TASK_ASAP, &ata_bus_complete_read, bus);
      break;
    case ATA_STATE_PENDING_WRITE:
      bus->state = ATA_STATE_PENDING_WRITE_DRAIN;
      ata_bus_clear_interrupts(bus); // Prevent further interrupts until the lower half has drained the disk buffer
      t = new_scheduler_task(TASK_ASAP, &ata_complete_write_lowerhalf, bus);
      break;
    default:
      kprintf("ERROR Received ATA interrupt for bus %s in unexpected state %d\r\n", bus->bus_name, bus->state);
      return;
  }

  if(t==NULL) {
    k_panic("ERROR Could not create schedule task to implement lower-half of ata interrupt\r\n");
    return;
  }
  kprintf("DEBUG scheduled drain task at 0x%x\r\n", (uint32_t)t);
  schedule_task(t);
}

void ata_dump_errors(uint8_t err)
{
  kprintf("ERROR ATA error code %d\r\n", (uint16_t)err);
}
