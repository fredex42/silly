#ifndef PARTITION_TYPES_H
#define PARTITION_TYPES_H

// Standard partition types
#define PARTITION_EMPTY              0x00  // Unused entry
#define PARTITION_FAT12              0x01  // DOS FAT12
#define PARTITION_XENIX_ROOT         0x02  // XENIX root
#define PARTITION_XENIX_USR          0x03  // XENIX /usr
#define PARTITION_FAT16_LT32M        0x04  // DOS FAT16 <32MB
#define PARTITION_EXTENDED           0x05  // DOS Extended (CHS)
#define PARTITION_FAT16              0x06  // DOS FAT16B
#define PARTITION_NTFS               0x07  // NTFS / exFAT / OS/2 HPFS
#define PARTITION_AIX                0x08  // AIX boot
#define PARTITION_AIX_DATA           0x09  // AIX data
#define PARTITION_OS2_BOOT_MANAGER   0x0A  // OS/2 Boot Manager
#define PARTITION_FAT32              0x0B  // FAT32 (CHS)
#define PARTITION_FAT32_LBA          0x0C  // FAT32 (LBA)
#define PARTITION_FAT16_LBA          0x0E  // FAT16B (LBA)
#define PARTITION_EXTENDED_LBA       0x0F  // Extended (LBA)

// Hidden partitions
#define PARTITION_HIDDEN_FAT12       0x11  // Hidden FAT12
#define PARTITION_HIDDEN_FAT16       0x14  // Hidden FAT16 <32MB
#define PARTITION_HIDDEN_FAT16B      0x16  // Hidden FAT16B
#define PARTITION_HIDDEN_NTFS        0x17  // Hidden NTFS / HPFS
#define PARTITION_HIDDEN_FAT32       0x1B  // Hidden FAT32
#define PARTITION_HIDDEN_FAT32_LBA   0x1C  // Hidden FAT32 (LBA)
#define PARTITION_HIDDEN_FAT16_LBA   0x1E  // Hidden FAT16B (LBA)

// OS-specific and vendor-specific types
#define PARTITION_LINUX_SWAP         0x82  // Linux swap
#define PARTITION_LINUX              0x83  // Linux native
#define PARTITION_LINUX_EXTENDED     0x85  // Linux extended
#define PARTITION_LINUX_LVM          0x8E  // Linux LVM
#define PARTITION_FREEBSD            0xA5  // FreeBSD
#define PARTITION_OPENBSD            0xA6  // OpenBSD
#define PARTITION_NETBSD             0xA9  // NetBSD
#define PARTITION_MACOSX             0xA8  // Apple UFS
#define PARTITION_MACOSX_BOOT        0xAB  // Apple boot
#define PARTITION_MACOSX_RAID        0xAC  // Apple RAID
#define PARTITION_HFS                0xAF  // Apple HFS / HFS+
#define PARTITION_QNX                0x4D  // QNX4.x
#define PARTITION_QNX2               0x4F  // QNX2.x
#define PARTITION_VMWARE_VMFS        0xFB  // VMware VMFS
#define PARTITION_VMWARE_SWAP        0xFC  // VMware swap
#define PARTITION_EFI_GPT_PROTECTIVE 0xEE  // EFI GPT protective
#define PARTITION_EFI_SYSTEM         0xEF  // EFI System Partition

// Secured and special partitions
#define PARTITION_DRDOS_SECURE_FAT16 0xF2  // DR-DOS secured FAT16
#define PARTITION_LINUX_RAID         0xFD  // Linux RAID auto-detect
#define PARTITION_UNKNOWN            0xFF  // XENIX bad block table

#endif // PARTITION_TYPES_H
