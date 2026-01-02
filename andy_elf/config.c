#include <kernel_config.h>
#include <memops.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <cfuncs.h>

struct KernelConfig *new_kernel_config() {
    struct KernelConfig *cfg = (struct KernelConfig *)malloc(sizeof(struct KernelConfig));
    if(cfg) {
        cfg->cmdline = NULL;
        cfg->bios_boot_device = 0;
        cfg->bios_partition = 0;
        cfg->bios_subpartition = 0;
    }
    return cfg;
}

struct ParsedCommandLine *cmdline_extract_param(const char *cmdline, size_t start, size_t split, size_t end) {
    if(cmdline==NULL || start>=end || split<=start || split>=end) return NULL;

    size_t key_len = split - start;
    size_t val_len = end - split - 1;

    struct ParsedCommandLine *param = (struct ParsedCommandLine *)malloc(sizeof(struct ParsedCommandLine));
    if(param==NULL) {
        kputs("WARNING Could not allocate memory for command line parameter\r\n");
        return NULL;
    }

    param->key = (char *)malloc(key_len + 1);
    param->value = (char *)malloc(val_len + 1);
    if(param->key==NULL || param->value==NULL) {
        kputs("WARNING Could not allocate memory for command line parameter key/value\r\n");
        if(param->key) free(param->key);
        if(param->value) free(param->value);
        free(param);
        return NULL;
    }

    memcpy(param->key, &cmdline[start], key_len);
    param->key[key_len] = '\0';
    memcpy(param->value, &cmdline[split + 1], val_len);
    param->value[val_len] = '\0';
    param->next = NULL;

    return param;
}

struct ParsedCommandLine *parse_command_line(const char *cmdline) {
    if(cmdline==NULL) return NULL;
    //Assume that the commandline is a series of key=value pairs separated by spaces in ASCII format and is null-terminated

    size_t len = strlen(cmdline);
    if(len==0) {
        kputs("INFO Empty command line\r\n");
        return NULL;
    }

    if(len>2048) {
        kprintf("WARNING Command line length %d is invalid\r\n", len);
        return NULL;
    }

    struct ParsedCommandLine *head = NULL;
    struct ParsedCommandLine *tail = NULL;

    size_t param_start = 0;
    size_t param_split = 0;

    for(register size_t i=0;i<len;i++) {
        if(cmdline[i]=='=') {
            param_split=i;
        } else if(cmdline[i]==' ' || i==len-1) {
            //we reached the end of a parameter
            if(param_split==param_start) {
                //no '=' found, ignore this parameter
                param_start = i+1;
                param_split = param_start;
                continue;
            }
            struct ParsedCommandLine *next = cmdline_extract_param(cmdline, param_start, param_split, i);
            if(next==NULL) {
                kputs("WARNING Could not extract command line parameter: out of memory or invalid params\r\n");
                continue;
            }
            if(head==NULL) {
                head = next;
                tail = next;
            } else if(tail) {
                tail->next = next;
                tail = next;
            }
            param_start = i+1;
            param_split = param_start;
        }
    }
    return head;
}

void free_command_line(struct ParsedCommandLine *cmdline) {
    do {
        struct ParsedCommandLine *next = cmdline->next;
        if(cmdline->key) free(cmdline->key);
        if(cmdline->value) free(cmdline->value);
        free(cmdline);
        cmdline = next;
    } while(cmdline);
}

/**
 * Returns the value of the given command line parameter key, or NULL if not found.
 * The returned pointer is owned by the KernelConfig structure and should not be freed.
 */
const char* config_commandline_param(struct KernelConfig* cfg, const char *key) {
    if(cfg==NULL || key==NULL || cfg->cmdline==NULL) return NULL;

    struct ParsedCommandLine *current = cfg->cmdline;
    while(current) {
        if(strncmp(current->key, key, 0xFF)==0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Uses the kernel configuration to try to obtain a boot device string.
 * The returned string is dynamically allocated and should be freed by the caller.
 * 
 * - If a root= param is supplied in the kernel commandline, this is used without validation
 * - Otherwise, if a fixed-disk is determined by the bios, we use ide{n} where {n} is the bios boot disk.  This is not guaranteed to work
 * - If a non-fixed disk is determined by the bios, we use fd{n} where {n} is the bios boot disk.  This is not guaranteed to work
 * - If no boot device is specified, we default to ide0p0.
 */
const char* config_root_device(struct KernelConfig* cfg) {
    if(cfg==NULL) return NULL;
    char temp[32];
    char *buf = (char *)malloc(32);
    if(buf==NULL) {
        kputs("WARNING Could not allocate memory for root device string\r\n");
        return NULL;
    }
    memset(buf, 0, 32);

    const char *boot_device = config_commandline_param(cfg, "root");
    if(boot_device) {
        strncpy(buf, boot_device, 31);
        buf[31] = '\0'; // Ensure null-termination
        kprintf("Using root device from command line: %s\r\n", buf);
        return buf;
    }

    //If no root device is specified, use the BIOS boot device
    if(cfg->bios_boot_device!=0) {
        if(cfg->bios_boot_device & 0x80) {  //fixed disk
            strncpy(buf, "$ide", 5);
            longToString((int32_t)cfg->bios_boot_device & 0x7F, temp, 10);
            strncpy(&buf[4], temp, 3);
            buf[5] = 'p';
            buf[6] = '\0';
            longToString((int32_t)cfg->bios_partition, temp, 10);
            strncpy(&buf[6], temp, 3);
            buf[7] = '\0';
            kprintf("Using root device %s (BIOS boot device %d, partition %d)\r\n", buf, cfg->bios_boot_device, cfg->bios_partition);
        } else { //assume floppy
            strncpy(buf, "$fd", 4);
            longToString((int32_t)cfg->bios_boot_device & 0x7F, temp, 10);
            strncpy(&buf[3], temp, 3);
            kprintf("Using root device %s (BIOS boot device %d)\r\n", buf, cfg->bios_boot_device);
        }
        return buf;
    } else {
        kprintf("Unable to determine root device, falling back to ide0p0\r\n");
        strncpy(buf, "$ide0p0", 8); // Default to ide0p0 if no boot device is specified
    }
    return buf;
}