#include "pci_ide.h"
#include "pci_ops.h"
#include <stdio.h>

void pci_ide_init(uint8_t bus, uint8_t slot, uint8_t func, uint8_t prog_if)
{
    switch(prog_if & 0xF) {
        case 0x00:
            kputs("      ISA Compatibility-mode only\r\n");
            break;
        case 0x05:
            kputs("      PCI-native mode only\r\n");
            break;
        case 0x0A:
            kputs("      ISA Compatibility Mode, supports PCI native\r\n");
            break;
        case 0x0F:
            kputs("      PCI-native mode controller, supports ISA compatible\r\n");
            break;
        default:
            kprintf("      Unrecognised PROG_IF 0x%x\r\n", (uint32_t)prog_if);
            break;
    }
    if(prog_if & 0x80) {
        kputs("    Supports bus-mastering\r\n");
    }
}