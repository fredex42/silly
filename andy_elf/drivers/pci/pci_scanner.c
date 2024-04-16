#include "pci_scanner.h"
#include "pci_ops.h"

//Mostly taken from https://wiki.osdev.org/PCI#Enumerating_PCI_Buses !

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
            if(vendor_id != 0xFFFF) {
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