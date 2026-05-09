#include <types.h>
#include <interrupts.h>
#include "ata_bus.h"
#include <spinlock.h>
#include <stdio.h>
#include "ata_pio.h"
#include <errors.h>
#include <scheduler/scheduler.h>

spinlock_t ata_bus_list_lock = 0;
struct AtaBus *ata_bus_list = NULL;

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
  kprintf("INFO identify_drive drive_nr is %d\r\n", (uint16_t)drive_nr);
  uint8_t bus_nr = (drive_nr & 0xF) >> 1;
  if(bus_nr>4) {
    kprintf("ERROR identify_drive bus_nr %d is out of range\r\n", (uint16_t) bus_nr);
    return NULL;
  }

  outb(ATA_DRIVE_HEAD(base_addr), drive_nr);
  outb(ATA_COMMAND(base_addr), ATA_CMD_IDENTIFY);

  while(1) {
    st = inb(ATA_STATUS(base_addr));
    if(st==0){
      return NULL;  //zero status => nothing here, move on
    }

    if(! (st & 0x80) ) break; //poll the status until BSY bit clears (bit 7)
  }

  //Still here? Good. We've got a drive, now poll for success or failure.

  //create a new buffer to store the drive information. If we get a failure this is freed otherwise it
  //is linked back into the ATA state
  uint16_t *buffer = (uint16_t *)malloc(256*sizeof(int16_t)); //we pull 256 words.
  if(buffer==NULL) {
    kputs("ERROR No memory to allocate buffer\r\n");
    return NULL;
  }

  while(1) {
    st = inb(ATA_STATUS(base_addr));
    if((st & 0x8)) { //DRQ, bit 3 was set => success
      //dump out the data
      for(register int i=0;i<256;i++) {
          buffer[i] = inw(ATA_DATA_REG(base_addr));
      }
      return buffer;
    }
    if((st & 0x1)) {  //ERR, bit 1 was set => error
      kprintf("\tWARNING Disk error trying to identify drive 0x%x on 0x%x, ignoring\r\n", (uint32_t)drive_nr, (uint32_t)base_addr);
      free(buffer);
      return NULL;
    }
  }
}

void initialise_bus_disks(struct AtaBus *bus) {
  bus->refcount += 1; // Increment reference count for the bus
  uint16_t *identity = identify_drive(bus->io_base, bus->device_number==0 ? ATA_SELECT_MASTER : ATA_SELECT_SLAVE);
  if(identity) {
      kprintf("INFO Master drive found on bus %s\r\n", bus->bus_name);
      bus->connected_disk_count+=1;
      uint32_t lba28_sectors = ((uint32_t)identity[60]) | ((uint32_t)identity[61] << 16);
      if(lba28_sectors>0) {
          bus->flags |= (bus->device_number==0) ? ATA_MASTER_LBA28 : ATA_SLAVE_LBA28;
          kprintf("\tSupports LBA28 with %d sectors\r\n", lba28_sectors);
      }
      uint64_t lba48_sectors = ((uint64_t)identity[100]) | ((uint64_t)identity[101] << 16) | ((uint64_t)identity[102] << 32) | ((uint64_t)identity[103] << 48);
      if(lba48_sectors>0) {
          bus->flags |= (bus->device_number==0) ? ATA_MASTER_LBA48 : ATA_SLAVE_LBA48;
          kprintf("\tSupports LBA48 with %d sectors\r\n", lba48_sectors);
      }
  }
  bus->refcount -= 1; // Decrement reference count after initialization
}

void register_ata_bus(char *name, uint8_t bus_number, uint8_t device_number, uint16_t io_base, uint16_t control_base, uint16_t bus_master_base, uint32_t irq_num) {
  struct AtaBus *new_bus = (struct AtaBus *)malloc(sizeof(struct AtaBus));
  if(!new_bus) {
    k_panic("Could not allocate memory for new ATA bus\r\n");
    return;
  }
  memset(new_bus, 0, sizeof(struct AtaBus));
  new_bus->refcount = 1; // Initial reference count
  new_bus->sig[0] = 'A';
  new_bus->sig[1] = 'T';
  new_bus->sig[2] = 'A';
  new_bus->sig[3] = 'B';

  new_bus->state = ATA_STATE_INITIALISING;
  new_bus->bus_name = name;
  new_bus->bus_number = bus_number;
  new_bus->device_number = device_number;
  new_bus->io_base = io_base;
  new_bus->control_base = control_base;
  new_bus->bus_master_base = bus_master_base;
  new_bus->irq_num = irq_num;
  // Add to the linked list of buses
  spinlock_acquire(&ata_bus_list_lock);
  new_bus->next = ata_bus_list;
  ata_bus_list = new_bus;
  spinlock_release(&ata_bus_list_lock);

  kprintf("Registered ATA bus: %s (bus %d, device %d)\r\n", name, bus_number, device_number);
  
  initialise_bus_disks(new_bus);
  new_bus->device_number = 1; // Now check the slave device
  initialise_bus_disks(new_bus);
  new_bus->device_number = 0; // Reset to master for future operations
  new_bus->state = ATA_STATE_IDLE; // Set state to idle after initialization
}

/** 
 * Look up an ATA bus by name. Increments the reference count for the bus if found.
 * NOTE: Should be called with interrupts disabled.  The returned value should be unreff'd, not freed.
 * Returns: Pointer to the AtaBus structure if found, or NULL if not found.
 */
struct AtaBus *get_ata_bus(char *name) {
  spinlock_acquire(&ata_bus_list_lock);
  struct AtaBus *current = ata_bus_list;
  while(current) {
    if(strcmp(current->bus_name, name) == 0) {
      current->refcount += 1; // Increment reference count for the bus
      spinlock_release(&ata_bus_list_lock);
      return current;
    }
    current = current->next;
  }
  spinlock_release(&ata_bus_list_lock);
  return NULL; // Bus not found
}

/**
 * Drop a reference to an ATA bus. If the reference count reaches zero, the bus is unregistered and its memory is freed.
 * NOTE: Should be called with interrupts disabled.  The bus should have been obtained via get_ata_bus, not directly allocated or freed.
 */
void ata_bus_unref(struct AtaBus *bus) {
  if(bus->refcount > 0) {
    bus->refcount -= 1; // Decrement reference count for the bus
    if(bus->refcount == 0) {
      spinlock_acquire(&ata_bus_list_lock);
      kprintf("Unregistering ATA bus: %s\r\n", bus->bus_name);
      // Remove from the linked list
      if(ata_bus_list == bus) {
        ata_bus_list = bus->next;
      } else {
        struct AtaBus *current = ata_bus_list;
        while(current && current->next != bus) {
          current = current->next;
        }
        if(current) {
          current->next = bus->next;
        }
      }
      spinlock_release(&ata_bus_list_lock);
      free(bus);
    }
  }
}

void ata_bus_send_software_reset(struct AtaBus *bus) {
  outb(bus->control_base, 0x4); // Set SRST bit to send software reset
  io_wait(); // Wait for a short period to allow the reset to take effect
  io_wait();
  io_wait();
  io_wait();
  outb(bus->control_base, 0x0); // Clear SRST bit to complete the reset
  io_wait(); // Wait for the drive to recover from the reset
  io_wait();
  io_wait();
  io_wait();
}

/**
 * Start the ATA bus sending interrupts when it has data ready.  
 * This is used when starting a read or write operation, to allow the bus to signal when the disk has
 *  data ready or has finished processing the current data packet.
 */
void ata_bus_set_interrupts(struct AtaBus *bus) {
  uint8_t dcr = inb(bus->control_base);
  dcr &= ~0x2; // Clear bit 1 to enable interrupts
  outb(bus->control_base, dcr);
}

/**
 * Stops the ATA bus sending interrupts.  This is used between the IRQ handler acknoledging the interrupt and
 * the lower-half draining the disk buffer, to prevent the next interrupt from triggering before the lower 
 * half has finished processing the current one.
 */
void ata_bus_clear_interrupts(struct AtaBus *bus) {
  uint8_t dcr = inb(bus->control_base);
  dcr |= 0x2; // Set bit 1 to disable interrupts
  outb(bus->control_base, dcr);
}

uint8_t ata_bus_start_read_lba28(struct AtaBus *bus, uint64_t lba_address, uint16_t sector_count, void *buffer, void *extradata, void (*callback)(uint8_t status, void *buffer, void *extradata)) {
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
  if(sector_count > 255) {
    kprintf("ERROR: Sector count %d exceeds maximum of 256\r\n", sector_count);
    return E_PARAMS;
  }
  if(lba_address > 0xFFFFFFF) {  // LBA28 maximum
    kprintf("ERROR: LBA address 0x%x exceeds LBA28 limit\r\n", (uint32_t)lba_address);
    return E_PARAMS;
  }

  //FIXME - should be in a critical section.
  if(bus->state != ATA_STATE_IDLE) {
    kprintf("WARNING Attempt to start read on bus %s while state is not idle\r\n", bus->bus_name);
    return E_BUSY;
  }
  //check that the BSY bit is clear (if it isn't then the state machine is out of sync with the drive state)
  uint8_t status = ata_bus_get_status(bus, 0);
  if(status & ATA_STATUS_BSY) {
    kprintf("ERROR bus %s is out of sync (BSY bit set)\r\n", bus->bus_name);
    return E_BUSY;
  }
  bus->state = ATA_STATE_PENDING_READ;
  bus->active_completion_callback = callback; // Store the callback for later use when the read completes

  //request an LBA28 read
  uint8_t selector = 0xE0;  //LBA28 mode, master drive by default
  if(bus->device_number == 1) selector |= 0x10;  //set slave bit if needed
  
  outb(ATA_DRIVE_HEAD(bus->io_base), selector | LBA28_HI(lba_address));
  outb(ATA_SECTOR_COUNT(bus->io_base), sector_count);
  outb(ATA_LBA_LOW(bus->io_base), LBA28_LO(lba_address));
  outb(ATA_LBA_MID(bus->io_base), LBA28_LMID(lba_address));
  outb(ATA_LBA_HI(bus->io_base), LBA28_HMID(lba_address));
  outb(ATA_COMMAND(bus->io_base), ATA_CMD_READ_SECTORS);
  return E_OK;
}

void ata_bus_complete_read(SchedulerTask *t) {
  struct AtaBus *bus = (struct AtaBus *)t->data;

  uint8_t status = ata_bus_get_status(bus, 1);  // Read the status to acknowledge the interrupt and get the current status of the drive

  if(bus == NULL) {
    k_panic("ERROR NULL bus pointer in ata_bus_complete_read\r\n");
    return;
  }
  if(bus->active_completion_callback == NULL) {
    k_panic("ERROR NULL callback in ata_bus_complete_read\r\n");
    return;
  }

  if(bus->state != ATA_STATE_PENDING_READ_DRAIN) {
    kprintf("ERROR bus %s in unexpected state %d in ata_bus_complete_read\r\n", bus->bus_name, bus->state);
    return;
  }

  cli(); // Ensure interrupts are disabled while we complete the read operation and call the callback
  uint8_t return_status = ATA_STATUS_OK;

  // Status check: if BSY is still set, then something went wrong since we should have been signalled by the interrupt that the drive is ready.
  // If DRQ isn't set then we also have a problem since that means the drive doesn't have data ready for us to read.
  if((status & ATA_STATUS_ERR)) {
    //The disk signalled an error.  We can get more info about the error by reading the error register, but for now just log and call the callback with an error status.
    uint8_t error_code = inb(ATA_ERROR(bus->io_base));
    kprintf("ERROR Drive on bus %s signalled error after read interrupt. Status: 0x%x, error code: 0x%x\r\n", bus->bus_name, status, error_code);
    bus->state = ATA_STATE_FAULT;
    return_status = status;
  } else if((status & ATA_STATUS_BSY) || !(status & ATA_STATUS_DRQ)) {
    kprintf("ERROR Drive on bus %s signalled ready for read but status is 0x%x\r\n", bus->bus_name, status);
    bus->state = ATA_STATE_FAULT;
    return_status = status;
    return;
  } else {
    for(size_t i=0; i<256; i++) {
      bus->active_buffer[i] = inw(ATA_DATA_REG(bus->io_base));
    }
    return_status = ATA_STATUS_OK;
  }

  // Call the stored callback function to signal that the read operation has completed
  auto cb = bus->active_completion_callback; // Store the callback in a local variable before calling it
  bus->active_completion_callback = NULL; // Clear the callback after use
  bus->state = ATA_STATE_IDLE; // Set state back to idle after completion

  sti();

  cb(return_status, bus->active_buffer, (void *)bus); 
}

uint8_t ata_bus_get_status(struct AtaBus *bus, uint8_t clear_interrupt) {
  if(clear_interrupt) {
    return inb(ATA_STATUS(bus->io_base));
  } else {
    return inb(ATA_ALT_STATUS(bus->control_base));
  }
}

struct AtaBus *ata_bus_get_by_irq(uint32_t irq_num) {
  spinlock_acquire(&ata_bus_list_lock);
  struct AtaBus *current = ata_bus_list;
  while(current) {
    if(current->irq_num == irq_num) {
      spinlock_release(&ata_bus_list_lock);
      return current;
    }
    current = current->next;
  }
  spinlock_release(&ata_bus_list_lock);
  return NULL; // Bus not found
}