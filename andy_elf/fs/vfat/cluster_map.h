#include <types.h>

#ifndef __VFAT_CLUSTER_MAP_H
#define __VFAT_CLUSTER_MAP_H

#define CLUSTER_MAP_EOF_MARKER  0x0FFFFFFF

typedef struct vfat_cluster_map {
    uint8_t bitsize;  //either 12, 16 or 32
    uint32_t buffer_size;
    uint8_t *buffer;  //buffer that holds the entire cluster map

    struct fat_fs *parent_fs;

} VFatClusterMap;

uint32_t vfat_cluster_map_next_cluster(VFatClusterMap *m, uint32_t current_cluster_num);
#endif
