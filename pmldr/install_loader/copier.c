#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#include "copier.h"
#include "vfat.h"

/**
 * Returns the number of clusters required to write the given file onto the disk, based on the filesystem information
 * in the provided BPB
*/
size_t clusters_for_file(char *filepath, BIOSParameterBlock *bpb)
{
    struct stat fileinfo;

    int r = stat(filepath, &fileinfo);
    if(r!=0) {
        fprintf(stderr, "ERROR Could not read local file '%s'\n", filepath);
        return 0;
    }

    size_t bytes_per_cluster = bpb->bytes_per_logical_sector * bpb->logical_sectors_per_cluster;

    double partial_cluster_count = (double)fileinfo.st_size / (double)bytes_per_cluster;
    return (size_t)ceil(partial_cluster_count);
}


