#ifndef __COPIER_H
#define __COPIER_H

/**
 * Returns the number of clusters required to write the given file onto the disk, based on the filesystem information
 * in the provided BPB
*/
size_t clusters_for_file(char *filepath, BIOSParameterBlock *bpb)

#endif