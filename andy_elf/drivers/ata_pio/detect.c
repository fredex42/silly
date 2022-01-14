#include <types.h>
#include <cfuncs.h>
#include <sys/ioports.h>
#include <sys/mmgr.h>
#include <memops.h>
#include <stdio.h>
#include <panic.h>
#include <errors.h>
#include "ata_pio.h"

ATADriverState *master_driver_state = NULL;

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

  kprintf("INFO identify_drive drive_nr is %d\r\n", (uint16_t)drive_nr);
  uint8_t bus_nr = (drive_nr & 0xF) >> 1;
  if(bus_nr>4) {
    kprintf("ERROR identify_drive bus_nr %d is out of range\r\n", (uint16_t) bus_nr);
    return NULL;
  }

  //tell the interrupt handler to ignore, as we are polling
  master_driver_state->pending_disk_operation[bus_nr]->type = ATA_OP_IGNORE;
  while(1) {
    st = inb(ATA_STATUS(base_addr));
    if(st==0) return NULL;  //zero status => nothing here, move on

    if(! (st & 0x80) ) break; //poll the status until BSY bit clears (bit 7)
  }

  //Check LBAmid and LBAhi ports. If they are not 0 then this is not an ATA drive and we should stop.
  st = inb(ATA_LBA_MID(base_addr));
  if(st!=0) {
    kprintf("\tWARNING Drive 0x%x on 0x%x is not ATA compliant, ignoring\r\n", (uint32_t)drive_nr, (uint32_t)base_addr);
    master_driver_state->pending_disk_operation[bus_nr]->type = ATA_OP_NONE;
    return NULL;
  }
  st = inb(ATA_LBA_HI(base_addr));
  if(st!=0) {
    kprintf("\tWARNING Drive 0x%x on 0x%x is not ATA compliant, ignoring\r\n", (uint32_t)drive_nr, (uint32_t)base_addr);
    master_driver_state->pending_disk_operation[bus_nr]->type = ATA_OP_NONE;
    return NULL;
  }

  //Still here? Good. We've got a drive, now poll for success or failure.

  //kprintf("DEBUG sizeof(ATADriverState) is %l, active drive count is %d\r\n", sizeof(ATADriverState), (uint16_t) master_driver_state->active_drive_count);

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
      master_driver_state->pending_disk_operation[bus_nr]->type = ATA_OP_NONE;
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
  for(register int i=0;i<4; i++) {
    //these area already initialised to 0
    master_driver_state->pending_disk_operation[i] = (ATAPendingOperation *)( (vaddr)master_driver_state + sizeof(ATADriverState) + i*sizeof(ATAPendingOperation));
  }

  ata_detect_active_buses();
  kputs("\tDetecting drives...\r\n");
  if(master_driver_state->active_bus_mask & 0x1) {
    info = identify_drive(ATA_PRIMARY_BASE, ATA_SELECT_MASTER);
    if(info!=NULL) {
      kprintf("\tFound ATA Primary Master, data at 0x%x\r\n", info);
      master_driver_state->disk_identity[0] = info;
      //print_drive_info(0);
    }
    info = identify_drive(ATA_PRIMARY_BASE, ATA_SELECT_SLAVE);
    if(info!=NULL) {
      kprintf("\tFound ATA Primary Slave, data at 0x%x\r\n", info);
      master_driver_state->disk_identity[1] = info;
      //print_drive_info(1);
    }
  }
  if(master_driver_state->active_bus_mask & 0x2) {
    info = identify_drive(ATA_SECONDARY_BASE, ATA_SELECT_MASTER);
    if(info!=NULL) {
      kputs("\tFound ATA Secondary Master\r\n");
      master_driver_state->disk_identity[2] = info;
      //print_drive_info(2);
    }
    info = identify_drive(ATA_SECONDARY_BASE, ATA_SELECT_SLAVE);
    if(info!=NULL) {
      kputs("\tFound ATA Secondary Slave\r\n");
      master_driver_state->disk_identity[3] = info;
      //print_drive_info(3);
    }
  }

  if(master_driver_state->active_drive_count==0) {
    kputs("\tWARNING - did not detect any hard disks!\r\n");
  }

  for(register int i=0;i<4; i++) {
    master_driver_state->pending_disk_operation[i]->type = ATA_OP_NONE;
  }
  kputs("done.\r\n");
}

/*
Checks if the given drive supports LBA48 mode.
Arguments: drive_nr is 0 for PRI MASTER, 1 for PRI SLAVE, 2 for SEC MASTER, 3 for SEC SLAVE etc.
Returns: 1 if the drive is present and supports LBA48 or 0 otherwise.

See: "Interesting information returned from IDENTITY", https://wiki.osdev.org/ATA_PIO_Mode
*/
uint8_t ata_drive_supports_lba48(uint8_t drive_nr)
{
  if(drive_nr>7) return 0;  //only 8 items in the array
  uint16_t *drive_info = master_driver_state->disk_identity[drive_nr];
  if(drive_info == NULL) return 0;

  if(drive_info[83] & 0x400) return 1; else return 0; //0x400=> bit 10 set
}

/*
Checks the highest supported UDMA mode for the drive
*/
uint8_t ata_max_udma_mode(uint8_t drive_nr)
{
  if(drive_nr>7) return 0;  //only 8 items in the array
  uint16_t *drive_info = master_driver_state->disk_identity[drive_nr];
  if(drive_info == NULL) return 0;

  return drive_info[88] & 0xFF; //low byte gives max the supported UDMA mode
}

/*
Checks the current UDMA mode for the drive
*/
uint8_t ata_current_udma_mode(uint8_t drive_nr)
{
  if(drive_nr>7) return 0;  //only 8 items in the array
  uint16_t *drive_info = master_driver_state->disk_identity[drive_nr];
  if(drive_info == NULL) return 0;

  return (drive_info[88] >> 8) & 0xFF; //high byte gives the active mode
}

/*
Returns the LBA sector limit for the given drive number.
If this returns 0, then LBA28 mode is not supported.
*/
uint32_t ata_lba28_sector_limit(uint8_t drive_nr)
{
  if(drive_nr>7) return 0;  //only 8 items in the array
  uint16_t *drive_info = master_driver_state->disk_identity[drive_nr];
  if(drive_info == NULL) return 0;

  return ((uint32_t)drive_info[61] <<16 ) | (uint32_t)drive_info[60]; //take these two words together to get the sector limit
}

/*
Returns the LBA48 sector limit for the given drive number.
If this returns 0, then LBA48 mode is not supported.
*/
uint64_t ata_lba48_sector_limit(uint8_t drive_nr)
{
  if(drive_nr>7) return 0;  //only 8 items in the array
  uint16_t *drive_info = master_driver_state->disk_identity[drive_nr];
  if(drive_info == NULL) return 0;

  return ((uint64_t)drive_info[103] <<48 ) | ((uint64_t)drive_info[102] <<32 ) | ((uint64_t)drive_info[101] <<16 ) | (uint64_t)drive_info[100]; //take these two words together to get the sector limit
}

void print_drive_info(uint8_t drive_nr)
{
  kprintf("Drive %d max UDMA %d current UDMA %d. LBA28 sector limit is %l, LBA48 sector limit low dword is %l.  ",
   (uint16_t)drive_nr,
    (uint16_t)ata_current_udma_mode(drive_nr),
    (uint16_t)ata_max_udma_mode(drive_nr),
    (uint32_t)ata_lba28_sector_limit(drive_nr),
    (uint32_t)ata_lba48_sector_limit(drive_nr));
  if(ata_drive_supports_lba48(drive_nr)) {
    kputs("LBA48 mode supported.\r\n");
  } else {
    kputs("LBA48 mode not supported.\r\n");
  }
}

void test_read_cb(uint8_t status, void *buffer)
{
  kprintf("Received data from test read with status %d at 0x%x\r\n", (uint16_t) status, buffer);
}


void test_read()
{
  void* buffer = vm_alloc_pages(NULL, 1, MP_READWRITE);

  kputs("Testing disk read from HDD0...\r\n");
  //read in 8 sectors (=4kb) from block 0 of the disk
  uint8_t err = ata_pio_start_read(0, 0, 2, buffer, &test_read_cb);
  switch(err) {
    case E_OK:
      break;
    case E_BUSY:
      kputs("Disk subsystem was busy\r\n");
      break;
    case E_PARAMS:
      kputs("Invalid parameters\r\n");
      break;
  }
}
