#include <types.h>
#ifndef __FS_H
#define __FS_H

#define MAX_DEVICE 4  //for the time being!

//const FATFS* fs_for_device(uint8_t device_no);

typedef enum PartitionType {
    PARTTYPE_EMPTY              = 0x00, // Unused entry
    PARTTYPE_FAT12              = 0x01, // FAT12 (≤32 MB)
    PARTTYPE_FAT16_SMALL        = 0x04, // FAT16 (≤32 MB)
    PARTTYPE_EXTENDED           = 0x05, // Extended partition
    PARTTYPE_FAT16              = 0x06, // FAT16 (≥32 MB)
    PARTTYPE_NTFS               = 0x07, // NTFS / exFAT / HPFS
    PARTTYPE_FAT32              = 0x0B, // FAT32 (CHS)
    PARTTYPE_FAT32_LBA          = 0x0C, // FAT32 (LBA)
    PARTTYPE_FAT16_LBA          = 0x0E, // FAT16 (LBA)
    PARTTYPE_EXTENDED_LBA       = 0x0F, // Extended (LBA)
    PARTTYPE_LINUX              = 0x83, // Linux native (ext2/ext3/ext4)
    PARTTYPE_LINUX_SWAP         = 0x82, // Linux swap
    PARTTYPE_LINUX_LVM          = 0x8E, // Linux LVM
    PARTTYPE_LINUX_RAID         = 0xFD, // Linux RAID autodetect
    PARTTYPE_FREEBSD            = 0xA5, // FreeBSD
    PARTTYPE_OPENBSD            = 0xA6, // OpenBSD
    PARTTYPE_NETBSD             = 0xA9, // NetBSD
    PARTTYPE_SOLARIS_X86        = 0xBF, // Solaris x86
    PARTTYPE_GPT_PROTECTIVE     = 0xEE, // GPT protective MBR
    PARTTYPE_EFI_SYSTEM         = 0xEF, // EFI system partition (for UEFI)
    PARTTYPE_WINDOWS_RECOVERY   = 0x27, // Windows recovery environment
    PARTTYPE_HIDDEN_NTFS        = 0x17, // Hidden NTFS
    PARTTYPE_HIDDEN_FAT32       = 0x1B, // Hidden FAT32 (CHS)
    PARTTYPE_HIDDEN_FAT32_LBA   = 0x1C, // Hidden FAT32 (LBA)
    PARTTYPE_HIDDEN_FAT16_LBA   = 0x1E, // Hidden FAT16 (LBA)
    PARTTYPE_DELL_UTILITY       = 0xDE, // Dell diagnostic/utility
    PARTTYPE_BEOS_FS            = 0xEB, // BeOS BFS
} PartitionType;
#endif