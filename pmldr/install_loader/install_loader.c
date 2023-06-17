/*
This is a Linux C program which is designed to install our loader and kernel onto a device
*/
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "vfat.h"
#include "fs.h"
#include "copier.h"

#define BLOCKSIZE   32

int copy_bootsect(int source_fd, int dest_fd, size_t offset, size_t len) {
    char buf[BLOCKSIZE];
    long remaining;
    size_t count;
    int n=0;

    printf("INFO Writing bootloader at an offset of 0x%lx\n", offset);

    lseek(dest_fd, offset, SEEK_SET);

    for(remaining=len;remaining>0;remaining-=BLOCKSIZE) {
        if(remaining>BLOCKSIZE) {
            count = BLOCKSIZE;
        } else {
            count = remaining;
        }
        n=read(source_fd, buf, count);
        if(n==-1) {
            fprintf(stderr, "ERROR Could not read from source file\n");
            return -1;
        }
        n=write(dest_fd, buf, count);
        if(n==-1) {
            fprintf(stderr, "ERROR Could not write to destination device or file\n");
            return -1;
        }
    }

    return len;
}


size_t find_destination_offset(BIOSParameterBlock *bpb) {
    if(bpb->logical_sectors_per_fat==0) {   //we have FAT32
        return sizeof(BootSectorStart) + sizeof(BIOSParameterBlock) + sizeof(ExtendedBiosParameterBlock) + sizeof(FAT32ExtendedBiosParameterBlock);
    } else {
        return sizeof(BootSectorStart) + sizeof(BIOSParameterBlock) + sizeof(ExtendedBiosParameterBlock) ;
    }
}

int main(int argc, char **argv)
{
    char *device = argv[1];
    char oemname[9];
    BIOSParameterBlock *bpb;
    char *jump_bytes;

    if(device==NULL || strlen(device)==0) {
        fputs("ERROR You must specify the device or image to install onto as the first argument\n", stderr);
        exit(1);
    }

    int raw_device_fd = open(device, O_RDWR);
    if(raw_device_fd==-1) {
        fputs("ERROR Could not open device or file\n", stderr);
        exit(1);
    }

    void *bootsector = load_bootsector(raw_device_fd);
    if(!bootsector) {
        fputs("ERROR Could not load bootsector\n", stderr);
        exit(1);
    }

    get_oem_name(bootsector, oemname);
    bpb = get_bpb(bootsector);
    jump_bytes = get_jump_bytes(bootsector);

    printf("OEM name on device '%s' is '%s'\n", device, oemname);

    printf("Jump bytes on device %s are 0x%02x 0x%02x 0x%02x\n", device, jump_bytes[0] & 0xFF, jump_bytes[1] & 0xFF, jump_bytes[2] & 0xFF);

    if(boot_sig_byte(bootsector, 0)==0x55 && boot_sig_byte(bootsector, 1)==0xAA) {
        fprintf(stderr, "ERROR The volume at %s already has boot code present, not over-writing\n", device);
        exit(2);
    }
    // ---- copy over pmldr (the second-stage bootloader) as a regular file

    if(bpb->total_logical_sectors==0 && bpb->bytes_per_logical_sector!=0) {
        FAT32ExtendedBiosParameterBlock *f32bpb = get_f32bpb(bootsector);
        f32_copy_file("pmldr/pmldr", raw_device_fd, bpb, f32bpb);
    } else if( bpb->total_logical_sectors!=0) {
        f16_copy_file("pmldr/pmldr", raw_device_fd, bpb);
    } else {
        fprintf(stderr, "ERROR Could not determine if we are FAT16 or FAT32. Total sectors: %d, bytes per logical sector: %d", bpb->total_logical_sectors, bpb->bytes_per_logical_sector);
        exit(4);
    }
     
    // ---- install the bootsector
    int source_fd = open("bootsect/bootsect.bin", O_RDONLY);
    if(source_fd==-1) {
        fputs("ERROR Could not find bootsect executable. Ensure that it is in a subdirectory called bootsect below the current working directory.\n", stderr);
        exit(3);
    }

    size_t size = get_file_size("bootsect/bootsect.bin");
    if(size==0) {
        fputs("ERROR Either bootsect not found or it is zero-length\n", stderr);
        exit(3);
    }

    char buf[16];
    
    printf("INFO Copying bootsect.bin with a size of %ld bytes\n", size);
    size_t offset = find_destination_offset(bpb);
    copy_bootsect(source_fd, raw_device_fd, offset, size);
    //Now write the jump bytes
    lseek(raw_device_fd, 0, SEEK_SET);

    buf[0] = 0xEB;  //"JMP rel8"
    buf[1] = (uint8_t) offset;
    write(raw_device_fd, buf, 2);

    //Now write boot signature bytes
    lseek(raw_device_fd, 0x1FE, SEEK_SET);
    buf[0] = 0x55;
    buf[1] = 0xAA;
    write(raw_device_fd, buf, 2);

    close(source_fd);

    close(raw_device_fd);
}