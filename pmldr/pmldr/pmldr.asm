;What is the purpose of PMLDR? Well, it's a second-stage bootloader.  This extra space (beyond the <512byte limit allowed on the bootsector)
;allows us to jump into protected mode and to then use the system's extended memory to load the kernel so that we don't have to keep hopping
;and bopping around 16-bit conventional RAM and hitting arbitary limits as we do so.  Loading this way should be simpler, and also cleaner,
;as the kernel code won't need a 16-bit prologue part.

[map symbols pmldr.map]
;Offsets of BPB parameters in the boot sector
%define BytesPerSector 		0x0B	;word
%define SectorsPerCluster 	0x0D	;byte
%define ReservedSectors 	0x0E	;word
%define FATCount			0x10	;byte
%define FAT16RootDirEnt		0x11	;word
%define FAT16TotalSectors	0x13	;word
%define MediaDescriptor		0x15	;byte
%define FAT16SectorsPerFAT	0x16	;word
%define PartitionHiddenSectors	0x1c	;dword. Count of hidden sectors preceding the partition that contains this FAT volume. This field should always be zero on media that are not partitioned
%define TotalSectors		0x20	;dword. If total is < 64k then use FAT16 value.
%define FAT32SectorsPerFAT	0x24	;dword
%define FAT32RootDirStart	0x2C	;dword. This is a cluster address so needs to be converted to sectors for loading
%define FAT32FSInfoSector	0x30	;word
%define CustomDiskBlocksPerCluster  0x32    ;word
%define CurrentlyLoadingCluster     0x34    ;dword

%define BootSectorBase	0x500
%define BootSectorMemSectr 0x50
%define FATBase			0x700
%define FATSector       0x70

%define KernelSizeInBytesPtr    0x9F00  ;dword
%define CursorColPtr            0x9F04  ;byte
%define CursorRowPtr            0x9F05  ;byte

;We implement the following layout:	(512byte sectors = 0x200)
;At present, partitioned media is not supported.
; 0x500		0x700	Boot sector image
; 0x700		0xF00	Cluster 1 of FAT
; 0xF00		0x1700	Cluster 1 of root directory
; 0x1700	0x2500	Free
; 0x2500	0x2700	Read buffer (overwritten by memory info after loading)
; 0x3000	0x3100	disk address packet buffer
; 0x7c00    0x9000  our code
; 0x9F00    0x10000 Our values storage (see above)
; 0x10000   0x7F000 kernel image loading buffer (458k)
; 0x7F000   0x80000 (stack)

%include "prologue.asm"
%include "fs.asm"

%include "mainloader.asm"
