#include <types.h>
#include <fs/vfat.h>
#include <fs/fat_fs.h>
#include <stdio.h>
#include "cluster_map.h"

/**
Queries the next cluster in the chain following on from the given cluster.
Returns 0x0FFFFFFF when end-of-file is reached
*/
uint32_t vfat_cluster_map_next_cluster(VFatClusterMap *m, uint32_t current_cluster_num)
{
  size_t offset;
  #ifdef VFAT_VERBOSE
  kprintf("DEBUG vfat_cluster_map_next_cluster for 0x%x\r\n", current_cluster_num);
  #endif

  switch(m->bitsize) {
    case 12:
      kprintf("ERROR fat12 not supported yet\r\n");
      return -1;
    case 16:
      offset = current_cluster_num*2;
      uint16_t wvalue = ((uint16_t *)m->buffer)[current_cluster_num];
      #ifdef VFAT_VERBOSE
      kprintf("DEBUG vfat_cluster_map_next_cluster current_cluster_num is 0x%x (sector 0x%x) at offset 0x%x giving next cluster as 0x%x (sector 0x%x)\r\n", current_cluster_num, current_cluster_num*4, offset, (uint32_t) wvalue, ((uint32_t) wvalue)*4);
      #endif
      
      if(wvalue==0xFFFF || wvalue==0) {
        return CLUSTER_MAP_EOF_MARKER;
      } else {
        return (uint32_t) wvalue;
      }
    case 32:
      offset = current_cluster_num*4;
      uint32_t dvalue = ((uint32_t *)m->buffer)[current_cluster_num] & 0x0FFFFFFF; //upper 4 bits are reserved and must be masked off, apparently
      if(dvalue>=0x0FFFFFF8 || dvalue==0) {  //all of thses signify end-of-file
        return CLUSTER_MAP_EOF_MARKER;
      } else {
        return dvalue;
      }
  }
}
