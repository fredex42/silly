#include <types.h>

#ifndef __ATA_BUS_H
#define __ATA_BUS_H

enum AtaState {
    ATA_STATE_INITIALISING,
    ATA_STATE_DETECTING,
    ATA_STATE_IDLE,
    ATA_STATE_PENDING_READ,
    ATA_STATE_PENDING_WRITE,
    ATA_STATE_PENDING_READ_DRAIN,
    ATA_STATE_PENDING_WRITE_DRAIN,
    ATA_STATE_FAULT
};

#define ATA_MASTER_LBA28 0x01
#define ATA_MASTER_LBA48 0x02
#define ATA_SLAVE_LBA28 0x04
#define ATA_SLAVE_LBA48 0x08

struct AtaBus {
    char sig[4];
    uint32_t refcount;
    struct AtaBus *next;
    enum AtaState state;
    char *bus_name;
    uint8_t bus_number; // 0 for primary, 1 for secondary
    uint8_t device_number; // 0 for master, 1 for slave
    uint16_t io_base; // Base I/O port for the bus
    uint16_t control_base; // Control I/O port for the bus
    uint16_t bus_master_base; // Bus master I/O port for DMA
    uint32_t irq_num;   //IRQ number that the bus reports to
    uint16_t disk_count;
    uint8_t connected_disk_count;
    uint8_t master_udma_mode;
    uint8_t slave_udma_mode;
    uint16_t flags;
    void (*active_completion_callback)(uint8_t status, void *buffer, void *extradata); // Callback function for read/write completion
};

/**
 * Registers a new ATA bus with the given parameters. 
 * This function initializes the bus structure, adds it to the global list of ATA buses,
 *  and attempts to detect connected disks.  Intended to be called during ISA or PCI initialisation
 * NOTE: Should be called with interrupts disabled.
 */
void register_ata_bus(char *name, uint8_t bus_number, uint8_t device_number, uint16_t io_base, uint16_t control_base, uint16_t bus_master_base, uint32_t irq_num);

/**
 * Drop a reference to an ATA bus. If the reference count reaches zero, the bus is unregistered and its memory is freed.
 * NOTE: Should be called with interrupts disabled.  The bus should have been obtained via get_ata_bus, not directly allocated or freed.
 */
void ata_bus_unref(struct AtaBus *bus);
/** 
 * Look up an ATA bus by name. Increments the reference count for the bus if found.
 * NOTE: Should be called with interrupts disabled.  The returned value should be unreff'd, not freed.
 * Returns: Pointer to the AtaBus structure if found, or NULL if not found.
 */
struct AtaBus *get_ata_bus(char *name);

struct AtaBus *ata_bus_get_by_irq(uint32_t irq_num);
/**
 * Stops the ATA bus sending interrupts.  This is used between the IRQ handler acknoledging the interrupt and
 * the lower-half draining the disk buffer, to prevent the next interrupt from triggering before the lower 
 * half has finished processing the current one.
 */
void ata_bus_clear_interrupts(struct AtaBus *bus);

/**
 * Start the ATA bus sending interrupts when it has data ready.  
 * This is used when starting a read or write operation, to allow the bus to signal when the disk has
 *  data ready or has finished processing the current data packet.
 */
void ata_bus_set_interrupts(struct AtaBus *bus);

void ata_bus_complete_read(SchedulerTask *t);

#endif