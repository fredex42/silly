#include <types.h>

#ifndef __KERNEL_CONFIG_H
#define __KERNEL_CONFIG_H

struct ParsedCommandLine {
    char *key;
    char *value;
    struct ParsedCommandLine *next;
};

struct KernelConfig {
    struct ParsedCommandLine *cmdline;
    uint32_t bios_boot_device;
    uint32_t bios_partition;
    uint32_t bios_subpartition;
};

/* Functions in config.c */
struct KernelConfig *new_kernel_config();
struct ParsedCommandLine *parse_command_line(const char *cmdline);
void free_command_line(struct ParsedCommandLine *cmdline);
const char* config_commandline_param(struct KernelConfig* cfg, const char *key);
const char* config_root_device(struct KernelConfig* cfg);

/* Functions in multiboot.c */
/**
 * Returns a pointer to the kernel configuration structure, or NULL if not available.
 * The returned pointer is owned by the kernel and should not be freed nor modified.
 */
const struct KernelConfig *get_kernel_config();

#endif