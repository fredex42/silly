#ifndef __FS_H
#define __FS_H

void *load_bootsector(int fd);
void get_oem_name(void *bootsect, char *buf);

#define get_bpb(bs_ptr) (BIOSParameterBlock *)((char *)bs_ptr)[0x0B]
#define get_jump_bytes(bs_ptr) (char *)bs_ptr;  //first 3 bytes
#define OEM_NAME_LENGTH 8

#define boot_sig_byte(bs_ptr,off) ((char *)bs_ptr)[0x1FE + off]
#endif