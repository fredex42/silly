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

#define BLOCKSIZE   32

int copy_pmldr(int source_fd, int dest_fd, size_t offset, size_t len) {
    char buf[BLOCKSIZE];
    long remaining;
    size_t count;
    int n=0;

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

size_t get_file_size(char *filepath) {
    struct stat fileinfo;

    int r = stat(filepath, &fileinfo);
    if(r!=0) {
        fprintf(stderr, "ERROR Could not read local file '%s'\n", filepath);
        return 0;
    }
    return fileinfo.st_size;
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
    
    int source_fd = open("pmldr/pmldr", O_RDONLY);
    if(source_fd==-1) {
        fputs("ERROR Could not find pmldr executable. Ensure that it is in a subdirectory called pmldr below the current working directory.\n", stderr);
        exit(3);
    }

    size_t size = get_file_size("pmldr/pmldr");
    if(size==0) {
        fputs("ERROR Either pmldr not found or it is zero-length\n", stderr);
        exit(3);
    }
    copy_pmldr(source_fd, raw_device_fd, find_destination_offset(bpb), size);
}