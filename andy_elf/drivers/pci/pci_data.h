#include <types.h>

typedef struct pci_common_header {
    uint16_t vendor_id;
    uint16_t device_id;     //register 0
    uint16_t command;
    uint16_t status;        //register 1
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;     
    uint8_t cache_line_size;//register 2
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
} __attribute__((packed)) PCI_COMMON_HEADER;