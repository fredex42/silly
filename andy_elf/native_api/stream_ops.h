#include <types.h>

#ifndef __API_STREAM_OPS_H
#define __API_STREAM_OPS_H

uint32_t api_open(char *filename, char *xtn, uint16_t mode_flags);
void api_close(uint32_t fd);
size_t api_read(uint32_t fd, char *buf, size_t len);
size_t api_write(uint32_t fd, char *buf, size_t len);

#endif
