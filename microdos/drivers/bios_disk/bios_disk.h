#include <types.h>

//This is the Int13 Extensions drive parameters, https://www.ctyme.com/intr/rb-0715.htm
struct BiosDriveParameters {
    uint16_t buffer_size;
    uint16_t information_flags;
    uint32_t cylinder_count;
    uint32_t head_count;
    uint32_t sector_count;
    uint64_t total_sector_count;
    uint16_t bytes_per_sector;
    //only for v2.0+, i.e. length > 0x1a
    uint32_t edd_configuration_ptr; //this is in 16-bit segment:offset format!
    //only for v3.0+, i.e. length > 0x1e
    uint16_t v3_sig;    //0xBEDD to indicate that this is present
    uint8_t path_length;
    char reserved[3];
    char host_bus[4];   //ASCIZ
    char interface_type[8]; //ASCIZ
    char interface_path[8]; //different interpretations depending on interface type
    char device_path[8];
    uint8_t reserved2;
    uint8_t checksum;   //makes the 8-bit sum of all bytes in the v3.0+ section sum to 0
} __attribute__((packed));

//Bitfield for information_flags
#define BIOS_DRIVE_INFO_TDMA    1<<0    //handles DMA boundary errors transparently
#define BIOS_DRIVE_INFO_CHS     1<<1    //CHS information is valid
#define BIOS_DRIVE_INFO_REMOVABLE   1<<2    //removable media
#define BIOS_DRIVE_INFO_SUPPORT_VERIFY  1<<3    //verify is supported
#define BIOS_DRIVE_INFO_CHANGELINE  1<<4    //changeline is supported (for removables)
#define BIOS_DRIVE_INFO_LOCKABLE    1<<5    //drive can be locked (for removables)
#define BIOS_DROVE_INFO_CHS_MAXED   1<<6    //CHS set to max supported valies, not current media (for removables)

void biosdisk_scan(struct VolMgr *volmgr);