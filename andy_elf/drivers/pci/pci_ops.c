#include "pci_ops.h"
#include <stdio.h>

static enum PCI_MODE mode = ISA_ONLY;   //i.e., PCI disabled initially. This value is set in `pci_init`
vaddr pci_entrypoint_phys;
uint8_t last_pci_bus_num;

// Read a word from the PCI configuration space.  See https://wiki.osdev.org/PCI for details
// This does not use PCI-express enhancements, which should be used if available.
uint16_t pci_config_legacy1_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) 
{
    uint32_t address;
    uint32_t lbus = (uint32_t) bus;
    uint32_t lslot = (uint32_t) slot;
    uint32_t lfunc = (uint32_t) func;
    uint32_t value = 0;

    //Create the configuration address
    //Bit  31    - enable bit
    //Bits 30-24 - reserved
    //Bits 23-16 - bus number
    //Bits 15-11 - device number
    //Bits 10-8  - Function number
    //Bits 7 -0  - Register offset
    //Note: Register Offset has to point to consecutive DWORDs, ie. bits 1:0 are always 0b00 (they are still part of the Register Offset). 
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) |
                ((uint32_t) 0x80000000));
    
    // Write the address to the address port 0xCF8
    asm ("movl %0, %%eax\n\tmov $0xCF8, %%edx\n\toutl %%eax, %%dx\n\t" : : "m"(address) : "edx", "eax");

    //Read in the data from the data port 0xCFC
    asm ("movl $0xCFC, %%edx\n\tinl %%dx, %%eax\n\tmovl %%eax, %0\n\t" : "=m"(value) : : "edx", "eax");

    //shift the result by the offset and return lower 16 bits
    return (uint16_t)((value >> ((offset & 2) * 8)) & 0xFFFF);
}

uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    switch(mode) {
        case ISA_ONLY:
            return 0xFFFF;  //same as PCI "not present"
        case LEGACY1:
            return pci_config_legacy1_read_word(bus,slot,func,offset);
        case LEGACY2:
            kputs("ERROR PCI access mechanism #2 not implemented\r\n");
            return 0xFFFF;
        case PCI_EX:
            kputs("ERROR PCI-Express not implemented\r\n");
            return 0xFFFF;
    }
}

/**
 * This must be called _BEFORE_ memory manager initialisation, as the PCI BIOS expects direct physical RAM access
*/
void pci_preinit(void **pci_entrypoint)
{
    uint8_t status;
    uint8_t hardware_characteristics;
    uint8_t interface_major_bcd;
    uint8_t interface_minor_bcd;
    uint8_t last_pci_bus;

    kprintf("PCI pre-initialisation...\r\n");
    kprintf("   DEBUG pci_preinit 32-bit entrypoint ptr is at 0x%x\r\n", pci_entrypoint);
    pci_entrypoint_phys = (vaddr)*pci_entrypoint;
    kprintf("   DEBUG pci_preinit 32-bit entrypoint is at 0x%x\r\n", pci_entrypoint_phys);
    if(pci_entrypoint_phys==0) {
        kprintf("WARNING PCI not detected, staying in ISA-only mode");
        mode = ISA_ONLY;
    } else {
        kprintf("   DEBUG pci_preinit checking access method...\r\n");

        asm("mov $0xB101, %%eax\n\t"
            "mov $0, %%edi\n\t"
            "push %%cs\n\t"     //required to complete FAR return at end of function. this is an indirect jump to the pointer given to us by BIOS
            "call *%5\n\t"      //this requires unmapped memory as far as i can tell http://www.ctyme.com/intr/rb-2371.htm
            "mov %%al, %0\n\t"
            "mov %%bh, %1\n\t"
            "mov %%bl, %2\n\t"
            "mov %%bl, %3\n\t"
            "mov %%ah, %4\n\t"
            : "=m"(hardware_characteristics),
              "=m"(interface_major_bcd),
              "=m"(interface_minor_bcd),
              "=m"(last_pci_bus),
              "=m"(status)
            : "m"(pci_entrypoint_phys) : "eax", "ebx", "edi", "ecx", "edx"
        );

        switch(status) {
            case 0:
                kprintf("   DEBUG pci_preinit hardware_characteristics 0x%x\r\n", hardware_characteristics);
                kprintf("   DEBUG pci_preinit version 0x%x.0x%x\r\n", interface_major_bcd, interface_minor_bcd);
                if((hardware_characteristics & (1<<0))) {
                    kprintf("  Configuration space access mechanism #1 supported\r\n");
                    mode = LEGACY1;
                } else if((hardware_characteristics & (1<<1))) { //if we have access #1 we don't need #2
                    kprintf("  Configuration space access mechanism #2 supported\r\n");
                    mode = LEGACY2;
                } else {
                    kprintf("  Error, no configuration space access mechanism given\r\n");
                    mode = ISA_ONLY;
                }
                kprintf("   DEBUG pci_preinit bus count is %d\r\n", last_pci_bus);
                last_pci_bus_num = last_pci_bus;
                break;
            case 0x81:
                kprintf("   ERROR pci_preinit installation check not supported\r\n");
                mode = ISA_ONLY;
                break;
            default:
                kprintf("ERROR pci_preinit installation check invalid status 0x%x\r\n", status);
                mode = ISA_ONLY;
                break;
        }
    }

    kputs("PCI scanning devices...\r\n");
    pci_recursive_scan();
    kputs("PCI pre-initialisation done.\r\n");
}

/**
 * Initialise the PCI subsystem. pci_preinit should have been previously called, or nothing will happen.
 * Expects a pointer to the MCFG ACPI table which will allow memory-mapped access; if this is NULL then it's assumed that 
 * PCI-Express is not supported and we will fall back to IO port based access
*/
void pci_init(void *mcfg_table)
{
    if(mcfg_table==NULL) {
        kprintf("INFO pci_init PCI-express not detected.\r\n");
        switch(mode) {
            case ISA_ONLY:
                kputs("INFO No PCI detected :(. Old-skool!\r\n");
                return;
            case LEGACY1:
                kputs("INFO Using PCI 2.x access\r\n");
                break;
            case LEGACY2: 
                kputs("INFO Using PCI 1.x access\r\n");
                break;
            default:
                kprintf("ERROR Invalid PCI mode value %d\r\n", (uint32_t)mode);
                break;
        }
    } else {
        kputs("INFO pci_init ACPI informs us that PCI-Express is available\r\n");
    }
}