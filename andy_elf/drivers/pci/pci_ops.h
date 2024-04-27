#include <types.h>

uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_preinit(void **pci_entrypoint);

//See https://wiki.osdev.org/PCI
enum PCI_MODE {
    ISA_ONLY = 1,   //PCI not supported
    LEGACY2,        //PCI1.0 only supported ("Configuration space access mechanism #2")
    LEGACY1,        //PCI2.0 is the maximum supported ("Configuration space access mechanism #1")
    PCI_EX,         //PCI-Express memory-mapped access supported 
};

//Returns a logical '1' if the given BAR is in the IO space
#define BAR_IS_IOSPACE(bar) (bar & 0x1)
//Returns the 'type' flag of a memory BAR
#define BAR_MEM_TYPE(bar) ((bar >> 1) & 0x03)
//Returns the 'prefetch' flag of a memory BAR
#define BAR_MEM_PREFETCH(bar) ((bar >> 3) & 0x01)

//Returns the IO port of the given BAR, if it's in the IO space
#define BAR_DECODE_IOSPACE(bar) (uint32_t)(bar & 0xFFFFFFFc)
#define BAR_DECODE_MEM16(bar) (uint16_t)(bar & 0xFFF0)
#define BAR_DECODE_MEM32(bar) (uint32_t)(bar & 0xFFFFFFF0)

#define BAR_TYPE_32BIT  0x00
#define BAR_TYPE_RESERVED   0x01
#define BAR_TYPE_64BIT  0x02