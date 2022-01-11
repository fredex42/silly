#include <types.h>
#include "../../cfuncs.h"
#include <sys/ioports.h>
#include <sys/mmgr.h>
#include <memops.h>

#include "ata_pio.h"

static ATADriverState *master_driver_state = NULL;

/*
Returns 1 if the bus status is 0xFF, i.e. open-circuit (nothing connected to it) or 0 otherwise
*/
uint8_t ata_check_open_circuit(uint16_t base_addr)
{
  uint8_t b = ATA_STATUS(base_addr);
  if(b==0xFF) return 1;
  return 0;
}

/*
Queries the default IO port locations of the 4 standard ATA buses and checks
for open-circuit. If they are not open-circuit marks the bus as "present"
*/
void ata_detect_active_buses()
{
  static const uint16_t ports[] = {ATA_PRIMARY_BASE, ATA_SECONDARY_BASE, ATA_TERTIARY_BASE, ATA_QUATERNARY_BASE};
  kputs("\tChecking IDE buses...");

  for(register uint8_t i=0;i<4;i++) {
    if(ata_check_open_circuit(ports[i])==0) {
      //bus is present
      master_driver_state->active_bus_mask |= 1<<i;
    }
  }
  if(master_driver_state->active_bus_mask & 0x1) kputs("1 ");
  if(master_driver_state->active_bus_mask & 0x2) kputs("2 ");
  if(master_driver_state->active_bus_mask & 0x4) kputs("3 ");
  if(master_driver_state->active_bus_mask & 0x8) kputs("4 ");
  kputs("done.\r\n");
}

/*
Selects the given drive (ATA_SELECT_MASTER or ATA_SELECT_SLAVE) and sends it the IDENTIFY
command. Stores the resulting bytes in the free buffer space following ATADriverState (assumed to be
available after that structure) and returns the pointer to that data.
Returns: NULL if the drive does not exist, or a buffer of intentity data. Blocks until the drive has
returned the information.
*/
uint16_t * identify_drive(uint16_t base_addr, uint8_t drive_nr)
{
  register uint8_t st;
  outb(ATA_DRIVE_HEAD(base_addr), drive_nr);
  outb(ATA_COMMAND(base_addr), ATA_CMD_IDENTIFY);

  while(1) {
    st = inb(ATA_STATUS(base_addr));
    if(st==0) return NULL;  //zero status => nothing here, move on

    if(! (st & 0x80) ) break; //poll the status until BSY bit clears (bit 7)
  }

  //Check LBAmid and LBAhi ports. If they are not 0 then this is not an ATA drive and we should stop.
  st = inb(ATA_LBA_MID(base_addr));
  if(st!=0) {
    kprintf("\tWARNING Drive 0x%x on 0x%x is not ATA compliant, ignoring\r\n", (uint32_t)drive_nr, (uint32_t)base_addr);
    return NULL;
  }
  st = inb(ATA_LBA_HI(base_addr));
  if(st!=0) {
    kprintf("\tWARNING Drive 0x%x on 0x%x is not ATA compliant, ignoring\r\n", (uint32_t)drive_nr, (uint32_t)base_addr);
    return NULL;
  }

  //Still here? Good. We've got a drive, now poll for success or failure.

  kprintf("DEBUG sizeof(ATADriverState) is %l, active drive count is %d\r\n", sizeof(ATADriverState), (uint16_t) master_driver_state->active_drive_count);

  //on success, we want to save the data into the same ram page as the master_driver_state immediately following that structure
  uint16_t *buffer = (uint16_t *)((vaddr)master_driver_state + (vaddr)sizeof(ATADriverState) + ((vaddr)master_driver_state->active_drive_count * 256));

  while(1) {
    st = inb(ATA_STATUS(base_addr));
    if((st & 0x8)) { //DRQ, bit 3 was set => success
      //dump out the data
      for(register int i=0;i<256;i++) {
          buffer[i] = inw(ATA_DATA_REG(base_addr));
      }
      ++master_driver_state->active_drive_count;  //record the fact that we have the drive
      return buffer;
    }
    if((st & 0x1)) {  //ERR, bit 1 was set => error
      kprintf("\tWARNING Disk error trying to identify drive 0x%x on 0x%x, ignoring\r\n", (uint32_t)drive_nr, (uint32_t)base_addr);
      return NULL;
    }
  }
}

/*
Returns the status byte for the given controller
*/
uint8_t get_status(uint16_t base_addr)
{
  register uint16_t port_nr = ATA_STATUS(base_addr);

  return inb(port_nr);
}

void initialise_ata_driver()
{
  uint16_t *info;
  kputs("Initialising simple ATA driver...\r\n");
  master_driver_state = (ATADriverState *)vm_alloc_pages(NULL, 1, MP_READWRITE);
  if(!master_driver_state) {
    k_panic("Could not allocate memory for ATA driver state\r\n");
    return;
  }

  memset((void *)master_driver_state, 0, sizeof(master_driver_state));

  kprintf("\tDEBUG ATA driver state ptr is 0x%x\r\n", master_driver_state);
  ata_detect_active_buses();
  kputs("\tDetecting drives...\r\n");
  if(master_driver_state->active_bus_mask & 0x1) {
    info = identify_drive(ATA_PRIMARY_BASE, ATA_SELECT_MASTER);
    if(info!=NULL) {
      kprintf("\tFound ATA Primary Master, data at 0x%x\r\n", info);
      master_driver_state->disk_identity[0] = info;
    }
    info = identify_drive(ATA_PRIMARY_BASE, ATA_SELECT_SLAVE);
    if(info!=NULL) {
      kprintf("\tFound ATA Primary Slave, data at 0x%x\r\n", info);
      master_driver_state->disk_identity[1] = info;
    }
  }
  if(master_driver_state->active_bus_mask & 0x2) {
    info = identify_drive(ATA_SECONDARY_BASE, ATA_SELECT_MASTER);
    if(info!=NULL) {
      kputs("\tFound ATA Secondary Master\r\n");
      master_driver_state->disk_identity[0] = info;
    }
    info = identify_drive(ATA_SECONDARY_BASE, ATA_SELECT_SLAVE);
    if(info!=NULL) {
      kputs("\tFound ATA Secondary Slave\r\n");
      master_driver_state->disk_identity[1] = info;
    }
  }

  if(master_driver_state->active_drive_count==0) {
    kputs("\tWARNING - did not detect any hard disks!\r\n");
  }
  kputs("done.\r\n");
}
