#include <types.h>
#include <malloc.h>
#include "volmgr_internal.h"
#include "../drivers/ata_pio/ata_pio.h"
#include "../drivers/ata_pio/ata_readwrite.h"

struct VolMgr_GlobalState *volmgr_state = NULL;

void volmgr_init() {
    volmgr_state = (struct VolMgr_GlobalState *)malloc(sizeof(struct VolMgr_GlobalState));
    volmgr_state->disk_list = NULL;
    volmgr_state->disk_count = 0;
}

void volmgr_boot_sector_loaded(uint8_t status, void *buffer, void *extradata) {
    struct VolMgr_Disk *disk = (struct VolMgr_Disk *)extradata;
    kputs("volmgr: Boot sector loaded callback for 0x%x\r\n", disk);
    //Parse partition table from buffer
    uint8_t *boot_sector = (uint8_t *)buffer;
    if(boot_sector[510] != 0x55 || boot_sector[511] != 0xAA) {
        kputs("volmgr: Invalid boot sector signature\r\n");
        free(buffer);
        return;
    }
    disk->optional_signature = *((uint32_t *)&boot_sector[0x1B8]);
    kprintf("volmgr: Disk signature: 0x%X\r\n", disk->optional_signature);
    
    for(int i=0; i<4; i++) {
        uint8_t *part_entry = &boot_sector[0x1BE + (i * 16)];
        uint8_t part_type = part_entry[4];
        if(part_type == PARTTYPE_EMPTY)
            continue;
        uint32_t start_sector = *((uint32_t *)&part_entry[8]);
        uint32_t sector_count = *((uint32_t *)&part_entry[12]);
        kprintf("volmgr: Found partition %d: Type 0x%X, Start Sector %d, Sector Count %d\r\n", i+1, part_type, start_sector, sector_count);
        volmgr_add_volume(disk, start_sector, sector_count);
        disk->partition_count++;
    }
    free(buffer);
}

uint8_t volmgr_intialise_disk(struct VolMgr_Disk *disk) {
    switch(disk->type) {
        case DISK_TYPE_ISA_IDE:
            //Initialise ISA IDE disk
            kputs("volmgr: Initialising ISA IDE disk at 0x%x\r\n", disk);
            void * buffer = malloc(512); //Temporary buffer for boot sector
            int disk_num = 0;
            if(disk->base_addr==0x1F0 && (disk->flags & DF_IDE_MASTER))
                disk_num = 0;
            else if(disk->base_addr==0x1F0 && (disk->flags & DF_IDE_SLAVE))
                disk_num = 1;
            else if(disk->base_addr==0x170 && (disk->flags & DF_IDE_MASTER))
                disk_num = 2;
            else if(disk->base_addr==0x170 && (disk->flags & DF_IDE_SLAVE))
                disk_num = 3;
            else {
                kprintf("volmgr: Unknown IDE disk base address 0x%X\r\n", disk->base_addr);
                return 0xFF;
            }
            ata_pio_start_read(disk_num, 0, 1, buffer, (void *)disk, &volmgr_boot_sector_loaded);
            break;
        case DISK_TYPE_PCI_IDE:
            //Initialise PCI IDE disk
            kputs("volmgr: PCI IDE disk initialisation not yet implemented\r\n");
            return 0xFF;
        default:
            kprintf("volmgr: Unknown disk type %d\r\n", disk->type);
            return 0xFF;
    }
}
void volmgr_add_disk(enum disk_type type, uint32_t base_addr, uint32_t flags) {
    struct VolMgr_Disk *new_disk = (struct VolMgr_Disk *)malloc(sizeof(struct VolMgr_Disk));
    new_disk->type = type;
    new_disk->base_addr = base_addr;
    new_disk->flags = flags;
    new_disk->volume_count = 0;
    new_disk->partition_count = 0;
    new_disk->optional_signature = 0;
    new_disk->volumes = NULL;
    new_disk->next = volmgr_state->disk_list;
    volmgr_state->disk_list = new_disk;
    volmgr_state->disk_count++;
}