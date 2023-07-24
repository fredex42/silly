#include <stdio.h>
#include "vfat.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "fs.h"

#define BOOT_SECTOR_SIZE 512

/**
 * Loads the disk bootsector into the given buffer
*/
void *load_bootsector(int fd)
{
    lseek(fd, 0, SEEK_SET);
    void* buf = malloc(BOOT_SECTOR_SIZE);
    if(!buf) return NULL;

    ssize_t bytes_read = read(fd, buf, BOOT_SECTOR_SIZE);
    if(bytes_read!=BOOT_SECTOR_SIZE) {
        fprintf(stderr, "ERROR Could only read %ld bytes for bootsector, expected 512\n", bytes_read);
        free(buf);
        return NULL;
    }
    return buf;
}

void get_oem_name(void *bootsect, char *buf)
{
    memcpy(buf, bootsect+0x03, OEM_NAME_LENGTH);
    buf[8] = 0;
}

/**
 * Calculate the number of clusters that we must allocate
*/
uint16_t file_size_to_clusters(size_t file_size, BIOSParameterBlock *bpb) 
{
    double sector_count = (double)file_size / bpb->bytes_per_logical_sector;
    double cluster_count = sector_count / bpb->logical_sectors_per_cluster;
    uint16_t clusters = (uint16_t) ceil(cluster_count);

    fprintf(stdout, "INFO File size is %ld bytes which is %f sectors and %f clusters. Allocating %d clusters\n", file_size, sector_count, cluster_count, clusters);
    return clusters;
}

/**
 * Returns an array of the fat16 root directory. This array should have bpb->max_root_dir_entries entries, the un-used ones being filled with 0.
*/
DirectoryEntry* f16_get_root_dir(int raw_device_fd, BIOSParameterBlock *bpb)
{
    if(bpb->max_root_dir_entries==0) return NULL;  //if max_root_dir_entries is 0 then we are fat32 and the wrong function was called.

    size_t buffer_size = bpb->max_root_dir_entries * sizeof(DirectoryEntry);
    DirectoryEntry* buffer = (DirectoryEntry *)malloc(buffer_size);
    if(!buffer) {
        fprintf(stderr, "ERROR Not enough memory for %d root dir entries\n", bpb->max_root_dir_entries);
        return NULL;
    }

    memset(buffer, 0, buffer_size);

    size_t root_dir_offset = f16_get_root_dir_location(bpb);
    lseek(raw_device_fd, root_dir_offset, SEEK_SET);
    size_t bytes_read = read(raw_device_fd, buffer, buffer_size);

    if(bytes_read < buffer_size) {
        fprintf(stderr, "ERROR Could not read in FAT16 root directory, expected %ld bytes got %ld\n", buffer_size, bytes_read);
        free(buffer);
        return NULL;
    }
    return buffer;
}

/**
 * Writes an entire root directory block back to the disk. The DirectoryEntry pointer _must_ point to an array of bpb->max_root_dir_entries
 * directory entries.
 * Returns 0 on success or -1 on error
*/
int f16_write_root_dir(int raw_device_fd, BIOSParameterBlock *bpb, DirectoryEntry *buffer)
{
    size_t buffer_size = bpb->max_root_dir_entries * sizeof(DirectoryEntry);
    size_t root_dir_offset = f16_get_root_dir_location(bpb);
    lseek(raw_device_fd, root_dir_offset, SEEK_SET);
    fprintf(stdout, "INFO Writing %ld bytes for root directory\n", buffer_size);
    size_t bytes_written = write(raw_device_fd, buffer, buffer_size);
    if(bytes_written < buffer_size) {
        fprintf(stderr, "ERROR Could not write out FAT16 root directory, expected %ld bytes got %ld\n", buffer_size, bytes_written);
        return -1;
    }
    return 0;
}

/**
 * Returns the byte offset of the root directory location for a FAT16 FS
*/
size_t f16_get_root_dir_location(BIOSParameterBlock *bpb) 
{
    size_t sector_location = bpb->reserved_logical_sectors + (bpb->fat_count * bpb->logical_sectors_per_fat);
    return sector_location * bpb->bytes_per_logical_sector;
}

/**
 * Returns the first 4k of the root directory on FAT32
*/
DirectoryEntry *f32_get_root_dir(int raw_device_fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb)
{
    size_t root_dir_offset = f32bpb->root_directory_entry_cluster * bpb->logical_sectors_per_cluster * bpb->bytes_per_logical_sector;

    size_t buffer_size = bpb->logical_sectors_per_cluster * bpb->bytes_per_logical_sector;
    DirectoryEntry *buffer = (DirectoryEntry *)malloc(buffer_size);
    if(!buffer) {
        fprintf(stderr, "ERROR Not enough memory\n");
        return NULL;
    }

    size_t reserved_sectors = (size_t)bpb->reserved_logical_sectors; // + (bpb->fat_count * f32bpb->logical_sectors_per_fat);
    root_dir_offset += reserved_sectors * bpb->bytes_per_logical_sector;    //FIXME - why are we off by 2 sectors?

    memset(buffer, 0, buffer_size);
    lseek(raw_device_fd, root_dir_offset, SEEK_SET);
    printf("DEBUG root dir is at cluster %d, root dir byte offset is %ld\n", f32bpb->root_directory_entry_cluster, root_dir_offset);
    size_t bytes_read = read(raw_device_fd, buffer, buffer_size);
    if(bytes_read < buffer_size) {
        fprintf(stderr, "ERROR Could not read in FAT32 root directory, expected %ld bytes got %ld\n", buffer_size, bytes_read);
        free(buffer);
        return NULL;
    }
    return buffer;
}

int f32_write_root_dir(int raw_device_fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb, DirectoryEntry *buffer)
{
    if(!bpb->max_root_dir_entries==0) return -1;  //if max_root_dir_entries not 0 then we are fat12/16 and the wrong function was called.

    size_t root_dir_offset = f32bpb->root_directory_entry_cluster * bpb->logical_sectors_per_cluster * bpb->bytes_per_logical_sector;
    size_t buffer_size = bpb->logical_sectors_per_cluster * bpb->bytes_per_logical_sector;

    size_t reserved_sectors = (size_t)bpb->reserved_logical_sectors + (bpb->fat_count * f32bpb->logical_sectors_per_fat);
    root_dir_offset += (reserved_sectors-2) * bpb->bytes_per_logical_sector;
    
    lseek(raw_device_fd, root_dir_offset, SEEK_SET);
    fprintf(stdout, "INFO Writing %ld bytes for root directory at %ld\n", buffer_size, root_dir_offset);
    size_t bytes_written = write(raw_device_fd, buffer, buffer_size);
    if(bytes_written < buffer_size) {
        fprintf(stderr, "ERROR Could not write out FAT32 root directory, expected %ld bytes got %ld\n", buffer_size, bytes_written);
        return -1;
    }
    return 0;
}

void *get_allocation_table(int raw_device_fd, size_t logical_sectors_per_fat, BIOSParameterBlock *bpb)
{
    size_t offset = bpb->reserved_logical_sectors * bpb->bytes_per_logical_sector;
    size_t buffer_size = logical_sectors_per_fat * bpb->bytes_per_logical_sector;

    fprintf(stdout, "DEBUG allocation table offset is %ld and size is %ld\n", offset, buffer_size);
    
    void *buf = malloc(buffer_size);
    if(!buf) {
        fprintf(stderr, "ERROR Not enough memory for file allocation table\n");
        return NULL;
    }
    lseek(raw_device_fd, offset, SEEK_SET);
    size_t bytes_read = read(raw_device_fd, buf, buffer_size);
    if(bytes_read < buffer_size) {
        fprintf(stderr, "ERROR Could not read in entire file allocation table, expected %ld bytes got %ld\n", buffer_size, bytes_read);
        free(buf);
        return NULL;
    }
    return buf;
}

int write_allocation_table(int raw_device_fd, BIOSParameterBlock *bpb, size_t logical_sectors_per_fat, void *buffer)
{
    size_t offset = bpb->reserved_logical_sectors * bpb->bytes_per_logical_sector;
    size_t buffer_size = logical_sectors_per_fat * bpb->bytes_per_logical_sector;

    fprintf(stderr, "DEBUG allocation table size is %ld bytes and offset is %ld\n", buffer_size, offset);

    lseek(raw_device_fd, offset, SEEK_SET);
    size_t bytes_written = write(raw_device_fd, buffer, buffer_size);
    if(bytes_written < buffer_size) {
        fprintf(stderr, "ERROR Could not write out entire file allocation table, expected %ld bytes got %ld\n", buffer_size, bytes_written);
        return -1;
    }
    return 0;
}

int f32_update_cluster_count(int raw_device_fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *ebpb, size_t clusters_used, size_t last_allocated)
{
    char sector_buffer[0x200];

    size_t offset = ebpb->fs_information_sector * bpb->bytes_per_logical_sector;
    lseek(raw_device_fd, offset, SEEK_SET);
    int n = read(raw_device_fd, sector_buffer, 0x200);
    if(n==-1) {
        fprintf(stderr, "ERROR Could not read FAT32 FS info sector\n");
        return -1;
    }

    if(sector_buffer[0]!=0x52 && sector_buffer[1]!=0x52 && sector_buffer[2]!=0x61 && sector_buffer[3]!=0x41) {
        fprintf(stderr, "ERROR FAT32 FS info sector is not valid\n");
        return -2;
    }

    size_t updated_cluster_count = 0;
    memcpy(&updated_cluster_count, &sector_buffer[0x1E8], 4);
    fprintf(stderr, "DEBUG Previous cluster count was 0x%lx (%ld)\n", updated_cluster_count, updated_cluster_count);
    updated_cluster_count -= clusters_used;
    fprintf(stderr, "DEBUG Updated cluster count is 0x%lx (%ld)\n", updated_cluster_count, updated_cluster_count);

    memcpy(&sector_buffer[0x1E8], &updated_cluster_count, 4);
    if(last_allocated>0) memcpy(&sector_buffer[0x1EC], &last_allocated, 4);

    lseek(raw_device_fd, offset, SEEK_SET);
    write(raw_device_fd, sector_buffer, 0x200);
    return 0;
}