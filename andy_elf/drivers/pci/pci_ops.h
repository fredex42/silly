#include <types.h>

uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_preinit(void **pci_entrypoint);

//See https://wiki.osdev.org/PCI
enum PCI_MODE {
    ISA_ONLY = 1,   //PCI not supported
    LEGACY2,        //PCI1.0 only supported ("Configuration space access mechanism #2")
    LEGACY1,        //PCI2.0 is the maximum supported ("Configuration space access mechanism #1")
    PCI_EX,         //PCI-Express memory-mapped access supported 
};