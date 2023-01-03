#ifndef __SYS_GDT_H
#define __SYS_GDT_H

//NOTE! This must be kept in-sync with the GDT definition in kickoff.s
//When using the selector values, you must OR the bits with the ring to enter -
// 0 for kernel or 3 for user
#define GDT_KERNEL_CS 0x08
#define GDT_KERNEL_DS 0x10
#define GDT_VIDEO_RAM 0x18
#define GDT_TSS       0x20
#define GDT_USER_CS   0x28
#define GDT_USER_DS   0x30

#endif
