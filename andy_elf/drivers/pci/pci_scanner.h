#include <types.h>

void pci_scanner_check_device(uint8_t bus, uint8_t device);
void pci_scanner_check_function(uint8_t bus, uint8_t device, uint8_t function);

void pci_recursive_scan();
void pci_scanner_check_bus(uint8_t bus);