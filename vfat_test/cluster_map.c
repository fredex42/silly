#include <string.h>
#include <sys/types.h>
#include <unistd.h>


#include "cluster_map.h"

/**
frees the memory associated with the given cluster map
*/
void vfat_dispose_cluster_map(VFatClusterMap *m)
{
  free(m->buffer);
  free(m);
}

/**
Allocates and initialises a new cluster map object
*/
VFatClusterMap *vfat_load_cluster_map(int fd, struct bios_parameter_block *bpb, struct fat32_extended_bios_parameter_block *f32bpb)
{
  VFatClusterMap *m = (VFatClusterMap *)malloc(sizeof(VFatClusterMap));
  size_t bytes_offset, bytes_to_load;

  bytes_offset = (size_t) bpb->reserved_logical_sectors * (size_t) bpb->bytes_per_logical_sector;
  if(f32bpb) {
    bytes_to_load = (size_t) f32bpb->logical_sectors_per_fat * (size_t)bpb->bytes_per_logical_sector;
  } else {
    bytes_to_load = (size_t) bpb->logical_sectors_per_fat * (size_t)bpb->bytes_per_logical_sector;
  }

  m->buffer_size = bytes_to_load;
  m->buffer = (uint8_t *)malloc(bytes_to_load);

  lseek(fd, bytes_offset, SEEK_SET);
  read(fd, m->buffer, bytes_to_load);

  uint16_t discriminator = (uint16_t) m->buffer[3]<<8 | (uint16_t)m->buffer[2];

  if(discriminator==0xFFFF) {
    m->bitsize=16;
  } else if(discriminator>=0x0FFF) {
    m->bitsize=32;
  } else {
    m->bitsize=12;
  }
}

/**
Queries the next cluster in the chain following on from the given cluster.
Returns 0x0FFFFFFF when end-of-file is reached
*/
uint32_t vfat_cluster_map_next_cluster(VFatClusterMap *m, uint32_t current_cluster_num)
{
  size_t offset;

  switch(m->bitsize) {
    case 12:
      puts("ERROR fat12 not supported yet\n");
      return -1;
    case 16:
      offset = current_cluster_num*2;
      uint16_t wvalue = ((uint16_t *)m->buffer)[current_cluster_num];
      if(wvalue=0xFFFF) {
        return 0x0FFFFFFF;
      } else {
        return (uint32_t) wvalue;
      }
    case 32:
      offset = current_cluster_num*4;
      uint32_t dvalue = ((uint32_t *)m->buffer)[current_cluster_num] & 0x0FFFFFFF; //upper 4 bits are reserved and must be masked off, apparently
      if(dvalue>=0x0FFFFFF8) {
        return 0x0FFFFFFF;
      } else {
        return dvalue;
      }
  }
}
