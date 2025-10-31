#ifdef __BUILDING_TESTS
#include <sys/types.h>
#else
#include <types.h>
#endif

#ifndef __FS_VFAT_H
#define __FS_VFAT_H

struct fat_fs* fs_vfat_new(uint8_t drive_nr, void *extradata, void (*did_mount_cb)(struct fat_fs *fs_ptr, uint8_t status, void *extradata));

/*
See https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
*/

#define MEDIA_8INCH_FLOPPY      0xE5
#define MEDIA_525_INCH_720K     0xED
#define MEDIA_RESERVED          0xEE
#define MEDIA_35INCH_144M       0xF0
#define MEDIA_DOUBLE_DENSITY    0xF4
#define MEDIA_ALTOS_FIXED195    0xF5
#define MEDIA_FIXED_DISK        0xF8
#define MEDIA_35_720_OR_525_12M 0xF9
#define MEDIA_FLOPPY_320K       0xFA
#define MEDIA_FLOPPY_640K       0xFB
#define MEDIA_525_180K          0xFC
#define MEDIA_525_360K          0xFD
#define MEDIA_525_160K          0xFE
#define MEDIA_525_320K          0xFF

//base BPB
typedef struct __attribute__((packed)) bios_parameter_block { //__attribute__((packed)) tells the compiler not to mess with padding and give us literal layout
  uint16_t bytes_per_logical_sector;  //Bytes per logical sector; the most common value is 512.
                                      // logical sector size is often identical to a disk's physical sector size, but can be larger or smaller in some scenarios.
  uint8_t logical_sectors_per_cluster;  //Allowed values are 1, 2, 4, 8, 16, 32, 64, and 128
  uint16_t reserved_logical_sectors;    //The number of logical sectors before the first FAT in the file system image.
  uint8_t fat_count;                    //Number of File Allocation Tables. Almost always 2; RAM disks might use 1. Most versions of MS-DOS/PC DOS do not support more than 2 FATs.
  uint16_t max_root_dir_entries;        //Maximum number of FAT12 or FAT16 root directory entries. 0 for FAT32, where the root directory is stored in ordinary data clusters; see offset 0x02C in FAT32 EBPBs.
  uint16_t total_logical_sectors;       //0 for FAT32. (If zero, use 4 byte value at offset 0x020)
  uint8_t media_descriptor;             //see defines above
  uint16_t logical_sectors_per_fat;     //FAT32 sets this to 0 and uses the 32-bit value at offset 0x024 instead.
} BIOSParameterBlock;

//DOS3.0 extensions. Goes after BIOSParameterBlock
typedef struct __attribute__((packed)) dos3_bios_parameter_block {
    uint16_t reserved;
    uint16_t chs_heads;
    uint16_t hidden_sectors_preceding_partition;
} DOS3BiosParameterBlock;

//DOS3.3 extensions. Overwrites DOS3.0 extensions, goes after BIOSParameterBlock
typedef struct __attribute__((packed)) dos32_bios_parameter_block {
  uint16_t reserved;
  uint16_t chs_heads;
  /*
  If both of these entries are 0 on volumes using a FAT32 EBPB with signature 0x29,
   values exceeding the 4,294,967,295 (232−1) limit (f.e. some DR-DOS volumes
    with 32-bit cluster entries) can use a 64-bit entry at offset 0x052 instead.
  */
  uint32_t hidden_sectors_preceding_partition;
  uint32_t total_logical_sectors; // Officially, it must be used only if the logical sectors entry at offset 0x013 is zero, but some operating systems (some old versions of DR DOS) use this entry also for smaller disks.
                          // Officially, it must be used only if the logical sectors entry at offset 0x013 is zero, but some operating systems (some old versions of DR DOS) use this entry also for smaller disks.
} DOS32BiosParameterBlock;

typedef struct __attribute__((packed)) extended_bios_parameter_block {
  uint8_t physical_drive_number;  //Allowed values for possible physical drives depending on BIOS are 0x00-0x7E and 0x80-0xFE
  uint8_t reserved;
  uint8_t extended_boot_signature;  //Should be 0x29 to indicate that an EBPB with the following 3 entries exists
  uint32_t volume_id;
  char partition_volume_label[11];
  char printable_fs_type[8];        //This entry is meant for display purposes onl
} ExtendedBiosParameterBlock;

//FAT32 extensions. This follows the DOS 3.31 BPB
typedef struct __attribute__((packed)) fat32_extended_bios_parameter_block {
  uint32_t logical_sectors_per_fat;
  uint16_t descrip_mirroring_flags; //bits 3-0: zero-based number of active FAT, if bit 7 set.[10] If bit 7 is clear, all FATs are mirrored as usual. Other bits reserved and should be 0.
  uint8_t version[2]; //low byte first, high byte second. Defined as LOW.HIGH.
  uint32_t root_directory_entry_cluster;  //Cluster number of root directory start, typically 2 (first cluster[37]) if it contains no bad sector
  uint16_t fs_information_sector; //Logical sector number of FS Information Sector, typically 1, i.e., the second of the three FAT32 boot sectors. if 0xFFFF or 0x0000 then there isn't one.
  uint16_t boot_sectors_copy_start; //First logical sector number of a copy of the three FAT32 boot sectors, typically 6
  uint8_t reserved[12];
  uint8_t drive_physical_number;
  uint8_t verious;  //Cf. 0x025 for FAT12/FAT16
  uint8_t extended_boot_signature;
  uint32_t volume_id;
  char volume_label[11];
  char printable_fs_type[8];
} FAT32ExtendedBiosParameterBlock;

//A disk should always start with this.
//That is followed by the BiosParameterBlock, then maybe one or more extensions
typedef struct __attribute__((packed)) bootsector_start {
  char boot_jump[3];    //jmp instruction to bootloader code
  char oem_name[8];     //OEM Name (padded with spaces 0x20)
} BootSectorStart;

//Introduced in FAT32 for speeding up some operations
/*
The sector's data may be outdated and not reflect the current media contents,
because not all operating systems update or use this sector, and even if they do,
the contents is not valid when the medium has been ejected without properly
unmounting the volume or after a power-failure. Therefore, operating systems
should first inspect a volume's optional shutdown status bitflags residing
in the FAT entry of cluster 1 or the FAT32 EBPB at offset 0x041 and ignore
the data stored in the FS information sector, if these bitflags indicate
that the volume was not properly unmounted before. This does not cause any
problems other than a possible speed penalty for the first free space
query or data cluster allocation; see fragmentation.
*/
typedef struct __attribute__((packed)) fs_info_sector {
  char sig[4];  //FS information sector signature (0x52 0x52 0x61 0x41 = "RRaA")
  char reserved[480];
  char sig2[4]; //FS information sector signature (0x72 0x72 0x41 0x61 = "rrAa")
  uint32_t last_known_free_cluster_count; //0xFFFFFFFF if unknown (set to this when formatted)
  uint32_t most_recently_known_allocated; // Before using this value, the operating system should sanity check this value to be a valid cluster number on the volume. ]
  char reserved2[12];
  char sig3[4]; // 	FS information sector signature (0x00 0x00 0x55 0xAA)
} FSInformationSector;

#define VFAT_ATTR_READONLY  1<<0
#define VFAT_ATTR_HIDDEN    1<<1
#define VFAT_ATTR_SYSTEM    1<<2
#define VFAT_ATTR_VOLLABEL  1<<3
#define VFAT_ATTR_SUBDIR    1<<4
#define VFAT_ATTR_ARCHIVE   1<<5
#define VFAT_ATTR_DEVICE    1<<6  //Device (internally set for character device names found in filespecs, never found on disk), must not be changed by disk tools.
//Bit 0x7 is Reserved, must not be changed by disk tools.
#define VFAT_ATTR_LFNCHUNK  0X0F  //equivalent to volume label, system, hidden and readonly. Invalid combination used to signify a long file name

typedef struct __attribute__((packed)) directory_entry {
  char short_name[8];
  char short_xtn[3];  //padded with spaces
  uint8_t attributes;
  uint8_t user_attributes;
  uint8_t first_char_of_deleted;
  uint16_t creation_time; //only since DOS 7
  uint16_t creation_date; //only since DOS 7. Year 0=>1980
  uint16_t last_access_date;  //since DOS 7
  uint16_t f32_high_cluster_num;  //in FAT32, high 2 bytes of 4-byte first cluster number
  uint16_t last_mod_time;
  uint16_t last_mod_date;
  uint16_t low_cluster_num;       //in FAT12/16, first cluster number. In FAT32, low 2 bytes of 4-byte cluster Number
  uint32_t file_size;
} DirectoryEntry;

#define FAT32_CLUSTER_NUMBER(directory_entry) ((uint32_t)(directory_entry->f32_high_cluster_num) << 16 ) + (uint32_t)directory_entry->low_cluster_num

typedef struct __attribute__((packed)) long_file_name {
  uint8_t sequence;
  char chars_one[10];
  uint8_t attributes; //always 0X0F
  uint8_t type;       //always 0x0 for VFAT LFN
  uint8_t checksum;   //checksum byte for DOS 8.3 filename
  char chars_two[12];
  uint16_t first_cluster; //always 0x00
  char chars_three[4];
} LongFileName;

typedef struct directory_contents_list {
  struct directory_contents_list *next;
  union {
    DirectoryEntry *entry;
    LongFileName *lfn;
  };
  char short_name[12];  //zero-terminated, unpadded short name
} DirectoryContentsList;

/**
buffer structure that we use to load and interrogate the FAT
*/
#define FAT_TYPE_INVALID  0
#define FAT12_TYPE        1
#define FAT16_TYPE        2
#define FAT32_TYPE        3

typedef struct fat_buffer {
  uint8_t fat_type;
  char *buffer;
  uint32_t buffer_length_bytes;     //the amount of bytes actually used in the buffer
  uint32_t initial_length_bytes;    //the amount of RAM originally allocated
  uint32_t buffer_length_sectors;   //how many sectors to write back to disk
} FATBuffer;

void vfat_mount(FATFS *new_fs, uint8_t drive_nr, uint32_t sector_offset, void *extradata, void (*callback)(struct fat_fs *fs_ptr, uint8_t status, void *extradata));

#endif
