#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>   //for ceil()
#include "fs.h"
#include "copier.h"

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

int f16_copy_file(char *source_file_name, int raw_device_fd, BIOSParameterBlock *bpb, struct copied_file_info *dest_info)
{
    uint16_t *fat = (uint16_t *)get_allocation_table(raw_device_fd, bpb->logical_sectors_per_fat, bpb);
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

/**
 * Overwrites the DirectoryEntry pointer to by the given pointer with an entry for PMLDR
*/
void apply_pmldr_entry(DirectoryEntry *entry, uint32_t cluster_address, uint32_t file_size_bytes) {
    time_t unixtime = time(NULL);
    struct tm nowtime;
    localtime_r(&unixtime, &nowtime);

    strcpy(entry->short_name, "PMLDR   ");
    strcpy(entry->short_xtn, "   ");
    entry->attributes = 0x01 | 0x04;    //read_only | system
    entry->user_attributes = 0x00;
    entry->first_char_of_deleted = 0x00;
    entry->creation_time = (uint8_t)nowtime.tm_hour << 11 | (uint8_t)nowtime.tm_min << 5 | (uint8_t)(nowtime.tm_sec / 2);
    entry->creation_date = (nowtime.tm_year - 80) << 9 | (uint8_t)nowtime.tm_mon << 5 | (uint8_t)nowtime.tm_mday;
    entry->last_access_date = entry->creation_date;
    entry->f32_high_cluster_num = cluster_address >> 16;
    entry->last_mod_time = (uint8_t)nowtime.tm_hour << 11 | (uint8_t)nowtime.tm_min << 5 | (uint8_t)(nowtime.tm_sec / 2);
    entry->last_mod_date = (nowtime.tm_year - 80) << 9 | (uint8_t)nowtime.tm_mon << 5 | (uint8_t)nowtime.tm_mday;
    entry->low_cluster_num = (uint16_t) cluster_address;
    entry->file_size = file_size_bytes;
}

/**
 * Finds an empty entry in the given root directory chain, and over-writes it with the PMLDR entry
 * Arguments:
 *   root_dir_start - DirectoryEntry pointer to the beginning of the root dir
 *   cluster_address - cluster number that the PMLDR data starts with
 *   file_size_bytes - file size in bytes
 *   root_dir_count - number of entries in the root_dir.
*/
int add_pmldr_entry(DirectoryEntry *root_dir_start, uint32_t cluster_address, size_t file_size_bytes, size_t root_dir_count)
{
    //First, find the first empty (or deleted) root dir entry
    for(size_t i=0; i<root_dir_count; i++) {
        DirectoryEntry entry = root_dir_start[i];
        if(entry.short_name[0]==0x00 || entry.short_name[0]==0xE5) {  //0x00 => free, 0xE5 => deleted
            fprintf(stderr, "INFO Found free directory entry slot at %ld\n", i);
            apply_pmldr_entry(&root_dir_start[i], cluster_address, file_size_bytes);
            return 0;
        }
    }
    fprintf(stderr, "ERROR Could not find any free entries in the root directory :(");
    return -1;
}

int f32_copy_file(char *source_file_name, int raw_device_fd, BIOSParameterBlock *bpb, FAT32ExtendedBiosParameterBlock *f32bpb, struct copied_file_info *dest_info)
{
    uint32_t *fat = (uint32_t *)get_allocation_table(raw_device_fd, f32bpb->logical_sectors_per_fat, bpb);
    if(!fat) {
        fprintf(stderr, "ERROR Could not get fat32 allocation table\n");
        return -1;
    }
    size_t fat_size = (size_t) ceil((double)(f32bpb->logical_sectors_per_fat * bpb->bytes_per_logical_sector) / (double) 4.0); //divide by 4 because 4 bytes per entry

    size_t source_file_size = get_file_size(source_file_name);
    if(source_file_size==0) {
        fprintf(stderr, "ERROR Source file for copy either missing or zero-length\n");
        return -1;
    }
    uint16_t cluster_count = file_size_to_clusters(source_file_size, bpb);
    fprintf(stdout, "INFO Copying %s which is %ld bytes or %d clusters\n", source_file_name, source_file_size, cluster_count);
    
    uint32_t start_cluster_index = f32_find_free_clusters((uint32_t)cluster_count, fat, fat_size);
    if(start_cluster_index==0) {
        fprintf(stderr, "ERROR Could not find enough free clusters for 2nd stage bootloader\n");
        return -1;
    }
    fprintf(stdout, "INFO Starting cluster is %d\n", start_cluster_index);

    //now, set the cluster map for what we are about to copy
    for(uint32_t i=0; i<cluster_count; i++) {
        if(i==cluster_count-1) {
            fat[start_cluster_index+i] = 0x0FFFFFFF; //end-of-file marker
        } else {
            fat[start_cluster_index+i] = start_cluster_index+i+1;   //next cluster to load. We have ensured that they are contigous.
        }
    }

    //create a new directory entry in memory
    DirectoryEntry *root_dir_start = f32_get_root_dir(raw_device_fd, bpb, f32bpb);
    if(root_dir_start==NULL) {
        fprintf(stderr, "ERROR Could not find FAT32 root directory");
        free(fat);
        return -1;
    }
    add_pmldr_entry(root_dir_start, start_cluster_index, source_file_size, 16384 / sizeof(DirectoryEntry));

    //copy the file over, in blocks corresponding to each cluster
    int source_fd = open(source_file_name, O_RDONLY);
    if(source_fd==-1) {
        fprintf(stderr, "ERROR Could not open %s\n", source_file_name);
        free(fat);
        return -1;
    }

    //FIXME - why is this calculation out by 2 sectors?
    size_t reserved_sectors_total = (size_t)bpb->reserved_logical_sectors-2 + ((bpb->fat_count * (size_t)f32bpb->logical_sectors_per_fat));
    
    int rv = recursive_copy_file(source_fd, raw_device_fd,
     start_cluster_index, cluster_count, reserved_sectors_total * (size_t)bpb->bytes_per_logical_sector,
     (size_t)bpb->logical_sectors_per_cluster * (size_t)bpb->bytes_per_logical_sector);

    if(dest_info) {
        dest_info->start_sector_location = (start_cluster_index * bpb->logical_sectors_per_cluster) + reserved_sectors_total;
        dest_info->length_in_bytes = source_file_size;
        dest_info->length_in_sectors = (uint16_t) ( ceil((double)source_file_size / (double)bpb->bytes_per_logical_sector ));
    }

    if(rv!=0) {
        fprintf(stderr, "ERROR Could not copy PMLDR\n");
        free(fat);
        return -1;
    }
    close(source_fd);
    rv = write_allocation_table(raw_device_fd, bpb, f32bpb->logical_sectors_per_fat, (void *)fat);
    if(rv!=0) {
        fprintf(stderr, "ERROR Could not write updated allocation table\n");
        free(fat);
        return -1;
    }

    rv = f32_write_root_dir(raw_device_fd, bpb, f32bpb, root_dir_start);
    if(rv != 0) {
        fprintf(stderr, "ERROR Could not write updated root directory\n");
        free(fat);
        return -1;
    }
    fprintf(stdout, "INFO Copied over PMLDR\n");

    free(fat);

    rv = f32_update_cluster_count(raw_device_fd, bpb, f32bpb, (size_t)cluster_count, (size_t)start_cluster_index+(size_t)cluster_count);
    if(rv !=0) {
        fprintf(stderr, "WARNING Could not update FS Information Sector");
    }
    
    return 0;
}

/**
 * finds a free cluster chain of the given length. Returns the index of the first cluster of the chain, or 0 if nothing was found.
 * table_size is specified in _entries_ not _bytes_.
*/
uint32_t f32_find_free_clusters(uint32_t required_length, uint32_t *allocation_table, size_t table_size)
{
    fprintf(stdout, "DEBUG looking for %d clusters in table of size %ld\n", required_length, table_size);
    char counting;
    uint32_t found_blocks = 0;
    uint32_t starting_block = 0;

    for(uint32_t i=2; i<table_size;i++) {   //cluster 0 and 1 are reserved so start at 2
        uint32_t entry = allocation_table[i] & 0x0FFFFFFF;  //FAT32 uses 28 bits for cluster numbers. The remaining 4 bits in the 32-bit FAT entry are usually zero, but are reserved and should be left untouched.
        if(entry == 0x00) {   //free cluster
            if(!counting) {
                counting = 1;
                starting_block = i;
                found_blocks = 1;
            } else {
                ++found_blocks;
            }
        } else {    //cluster is occupied, bad, or reserved
            if(counting) {
                counting = 0;
                found_blocks = 0;
            }
        }
        if(found_blocks==required_length) { //yes, we found enough blocks so return
            return starting_block;
        }
    }
    //if we got here then we never found enough
    fprintf(stderr, "ERROR Not enough space on the disk for %d clusters\n", required_length);
    return 0;
}

int recursive_copy_file(int source_fd, int raw_device_fd, size_t dest_block_offset, size_t total_blocks, size_t reserved_bytes, size_t block_size)
{
    void *buffer = malloc(block_size);
    if(buffer==NULL) {
        fprintf(stderr, "ERROR Not enough memory for a %ld byte block\n", block_size);
        return -1;
    }

    int rv = copy_next_block(source_fd, 0, total_blocks, raw_device_fd, dest_block_offset, reserved_bytes, block_size, buffer);
    free(buffer);
    return rv;
}

int copy_next_block(int source_fd, size_t blocks_copied, size_t total_blocks, int raw_device_fd, size_t dest_block, size_t reserved_bytes, size_t block_size, void *buffer)
{

    fprintf(stderr, "DEBUG copy_next_block copied %ld out of %ld blocks from %d to %d\n", blocks_copied, total_blocks, source_fd, raw_device_fd);

    if(blocks_copied==total_blocks) return 0;   //we are done

    size_t target_offset = dest_block * block_size + reserved_bytes;
    fprintf(stdout, "INFO Copying block %ld to block %ld\n", blocks_copied, target_offset);

    memset(buffer, 0, block_size);
    size_t bytes_read = read(source_fd, buffer, block_size);    //this read may be short if we get to the end of the file
    fprintf(stdout, "INFO Read in %ld bytes\n", bytes_read);

    if(bytes_read==0) {
        fprintf(stderr, "ERROR Not enough blocks in the source file\n");
        return -1;
    }

    //write the whole block down
    fprintf(stdout, "INFO Byte offset is %ld\n", target_offset);
    lseek(raw_device_fd, target_offset, SEEK_SET);
    size_t bytes_written = write(raw_device_fd, buffer, block_size);
    fprintf(stdout, "INFO Wrote out %ld bytes\n", bytes_written);

    return copy_next_block(source_fd, blocks_copied+1, total_blocks, raw_device_fd, dest_block+1, reserved_bytes, block_size, buffer);
}