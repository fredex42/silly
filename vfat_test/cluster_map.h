#include <stddef.h>
#include <stdint.h>
#include "vfat.h"

#ifndef __CLUSTER_MAP_H
#define __CLUSTER_MAP_H

typedef struct vfat_cluster_map {
    uint8_t bitsize;  //either 12, 16 or 32
    uint32_t buffer_size;
    uint8_t *buffer;  //buffer that holds the entire cluster map
} VFatClusterMap;

/**
frees the memory associated with the given cluster map
*/
void vfat_dispose_cluster_map(VFatClusterMap *m);

/**
Allocates and initialises a new cluster map object
*/
VFatClusterMap *vfat_load_cluster_map(int fd, struct bios_parameter_block *bpb, struct fat32_extended_bios_parameter_block *f32bpb);

/**
Queries the next cluster in the chain following on from the given cluster.
Returns 0xFFFFFFFF when end-of-file is reached
*/
uint32_t vfat_cluster_map_next_cluster(VFatClusterMap *m, uint32_t current_cluster_num);
#endif
