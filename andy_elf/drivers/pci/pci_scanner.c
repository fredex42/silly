#include "pci_scanner.h"
#include "pci_ops.h"
#include "pci_categorisation.h"

//Mostly taken from https://wiki.osdev.org/PCI#Enumerating_PCI_Buses !
const char* pci_description_string(uint8_t pci_class, uint8_t subclass) {
    switch(pci_class) {
        case PCI_CC_UNCLASSIFIED:
            switch(subclass) {
                case PCI_SUBC_NON_VGA_UNCLASSIFIED:
                    return "Unclassified non-VGA-compatible device";
                case PCI_SUBC_VGA_COMPATIBLE_UNCLASSIFIED:
                    return "Unclassified VGA-compatible device";
                default:
                    return "Unknown unclassified device";
            }
            break;
        case PCI_CC_MASS_STORAGE:
            switch(subclass) {
                case PCI_SUBC_MASS_SCSI:
                    return "SCSI Mass Storage Controller";
                case PCI_SUBC_MASS_IDE:
                    return "IDE Mass Storage Controller";
                case PCI_SUBC_MASS_FLOPPY:
                    return "Floppy drive controller";
                case PCI_SUBC_MASS_IPI:
                    return "IPI Mass storage Controller";
                case PCI_SUBC_MASS_RAID:
                    return "RAID Controller";
                case PCI_SUBC_MASS_ATA:
                    return "ATA Mass Storage Controller";
                case PCI_SUBC_MASS_SAS:
                    return "Serial-attached SCSI Controller";
                case PCI_SUBC_MASS_NVM:
                    return "Non-volatile memory Mass Storage";
                default:
                    return "Unknown Mass Storage controller";
            }
            break;
        case PCI_CC_NETWORK:
            switch(subclass) {
                case PCI_SUBC_NET_ETHER:
                    return "Ethernet Controller";
                case PCI_SUBC_NET_TOKEN:
                    return "Token-ring Controller";
                case PCI_SUBC_NET_FDDI:
                    return "FDDI Fibre Controller";
                case PCI_SUBC_NET_ATM:
                    return "ATM Network Controller";
                case PCI_SUBC_NET_ISDN:
                    return "ISDN Controller";
                case PCI_SUBC_NET_WORLDFIP:
                    return "WorldFIP Controller";
                case PCI_SUBC_NET_INFINIBAND:
                    return "Infiniband Network Controller";
                case PCI_SUBC_NET_FABRIC:
                    return "Fabric Controller";
                default:
                    return "Unknown Network Controller";
            }
        case PCI_CC_DISPLAY:
            switch(subclass) {
                case PCI_SUBC_DISPLAY_VGA:
                    return "VGA-Compatible Display Controller";
                case PCI_SUBC_DISPLAY_XGA:
                    return "XGA-Compatible Display Controller";
                case PCI_SUBC_DISPLAY_3DCTRL:
                    return "Non-VGA Compatible 3d Display Controller";
                default:
                    return "Unknown Display Controller";
            }
        case PCI_CC_BRIDGE:
            switch (subclass) {
            case PCI_SUBC_BRIDGE_HOST:
                return "PCI Host Bridge";
            case PCI_SUBC_BRIDGE_ISA:
                return "PCI-ISA Bridge";
            case PCI_SUBC_BRIDGE_EISA:
                return "PCI-Extended ISA Bridge";
            case PCI_SUBC_BRIDGE_MCA:
                return "PCI-MCA Bridge";
            case PCI_SUBC_BRIDGE_PCI:
                return "PCI-PCI Bridge";
            case PCI_SUBC_BRIDGE_PCMCIA:
                return "PCI-PCMCIA Bridge";
            case PCI_SUBC_BRIDGE_NUBUS:
                return "PCI-NuBus Bridge";
            case PCI_SUBC_BRIDGE_CARDBUS:
                return "PCI-CardBus Bridge";
            case PCI_SUBC_BRIDGE_RACEWAY:
                return "Raceway Bridge";
            case PCI_SUBC_BRIDGE_PCI2:
                return "PCI-PCI Bridge (#2)";
            case PCI_SUBC_BRIDGE_INFINIBAND:
                return "PCI-Infiniband Bridge";
            default:
                return "Unknown PCI Bridge";
            }
        //fixme: more to come in here!

        case PCI_CC_SERIALBUS:
            switch(subclass) {
                case PCI_SUBC_SBUS_FIREWIRE:
                    return "Fibrechannel controller";
                case PCI_SUBC_SBUS_ACCESS:
                    return "ACCESS Bus Controller";
                case PCI_SUBC_SBUS_SSA:
                    return "SSA Controller";
                case PCI_SUBC_SBUS_USB:
                    return "USB Host Controller";
                case PCI_SUBC_SBUS_FIBRECHANNEL:
                    return "FibreChannel Controller";
                case PCI_SUBC_SBUS_SMBUS:
                    return "SMBUS Controller";
                case PCI_SUBC_SBUS_INFINIBAND:
                    return "Infiniband Controller";
                case PCI_SUBC_SBUS_IPMI:
                    return "IPMI Controller";
                case PCI_SUBC_SBUS_SERCOS:
                    return "SERCOS Controller";
                case PCI_SUBC_SBUS_CANBUS:
                    return "CANBUS Controller";
                default:
                    return "Unknown serial bus";
            }
        default:
            return "Not recognised yet!";
    }
}

/*
 * Checks to see if there is a device at the given address on the given bus
*/
void pci_scanner_check_device(uint8_t bus, uint8_t device) {
    uint8_t function = 0;

    //get the vendor ID
    uint16_t vendor_id = pci_config_read_word(bus, device, function, 0);
    if(vendor_id == 0xFFFF) {
        //kprintf("    DEBUG No device at 0x%x:0x%x\r\n", (uint32_t)bus, (uint32_t) device);
        return;
    }
    kprintf("    DEBUG PCI device with vendor ID 0x%x at 0x%x:0x%x\r\n", (uint32_t)vendor_id, (uint32_t)bus, (uint32_t)device);
    pci_scanner_check_function(bus, device, function);

    uint16_t headerType_bist = pci_config_read_word(bus, device, function, 0x0E);
    uint8_t headerType = (uint8_t)(headerType_bist & 0xFF);

    if( (headerType & 0x80) != 0) {
        kputs("    DEBUG PCI device is multi-function\r\n");
        //This is a multi-function device, check remaining functions (up to 8)
        for(function = 1; function < 8; function++) {
            uint16_t subvendor_id = pci_config_read_word(bus, device, function, 0);
            if(subvendor_id != 0xFFFF) {
                pci_scanner_check_function(bus, device, function);
            }
        }
    }
}

/**
 * Checks whether the given function of the given device is a PCI-PCI bridge. If so, scans the bus
 * that the bridge points to.
*/
void pci_scanner_check_function(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t class_values = pci_config_read_word(bus, device, function, 0x0A);
    uint8_t base_class = (uint8_t)(class_values >> 8);
    uint8_t sub_class  = (uint8_t)(class_values & 0xFF);
    kprintf("    DEBUG 0x%x:0x%x:0x%x is class 0x%x:0x%x\r\n", (uint32_t)bus, (uint32_t)device, (uint32_t)function, (uint32_t)base_class, (uint32_t)sub_class);
    kprintf("    DEBUG Found '%s'\r\n", pci_description_string(base_class, sub_class));
    if(base_class==0x06 && sub_class==0x04) {
        uint16_t bus_values = pci_config_read_word(bus, device, function, 0x18);    //Primary & secondary bus values
        uint8_t secondary_bus = (uint8_t)(bus_values >> 8);
        kprintf("    DEBUG Secondary bus ID is 0x%x\r\n", secondary_bus);
        pci_scanner_check_bus(secondary_bus);
    }
}

void pci_scanner_check_bus(uint8_t bus) {
    kprintf("  DEBUG Scanning PCI bus 0x%x\r\n", (uint32_t)bus);

    for(uint8_t device = 0; device < 32; device++) {
        pci_scanner_check_device(bus, device);
    }
}

void pci_recursive_scan() {
    pci_scanner_check_bus(0);
}