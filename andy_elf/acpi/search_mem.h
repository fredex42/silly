#include <acpi/rsdp.h>

const struct RSDPDescriptor* scan_memory_for_acpi();
uint8_t *map_bios_area();
void unmap_bios_area(uint8_t *buffer);