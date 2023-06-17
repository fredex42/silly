#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fs.h"

size_t get_file_size(char *source_file_name) 
{
    struct stat statbuf;
    int rv = stat(source_file_name, &statbuf);
    if(rv==-1) {
        fprintf(stderr, "ERROR Could not stat %s\n", source_file_name);
        return 0;
    }
    return statbuf.st_size;
}

int f16_copy_file(char *source_file_name, int raw_device_fd, BIOSParameterBlock *bpb)
{
    uint16_t *fat = (uint16_t *)get_allocation_table(raw_device_fd, bpb);
    if(!fat) {
        fprintf(stderr, "ERROR Could not get fat16 allocation table\n");
        return -1;
    }

    size_t source_file_size = get_file_size(source_file_name);
    if(source_file_size==0) {
        fprintf(stderr, "ERROR Source file for copy either missing or zero-length\n");
        return -1;
    }

    uint16_t cluster_count = file_size_to_clusters(source_file_size, bpb);
    fprintf(stdout, "INFO Copying %s which is %ld bytes or %d clusters\n", source_file_name, source_file_size, cluster_count);

}

int f32_copy_file(char *source_file_name, int raw_device_fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb)
{
    uint32_t *fat = (uint32_t *)get_allocation_table(raw_device_fd, bpb);
    if(!fat) {
        fprintf(stderr, "ERROR Could not get fat32 allocation table\n");
        return -1;
    }

    size_t source_file_size = get_file_size(source_file_name);
    if(source_file_size==0) {
        fprintf(stderr, "ERROR Source file for copy either missing or zero-length\n");
        return -1;
    }
    uint16_t cluster_count = file_size_to_clusters(source_file_size, bpb);
    fprintf(stdout, "INFO Copying %s which is %ld bytes or %d clusters\n", source_file_name, source_file_size, cluster_count);
}