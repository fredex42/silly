#include <stdio.h>
#include "vfat.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
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
        fprintf(stderr, "ERROR Could only read %d bytes for bootsector, expected 512\n");
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