#include <types.h>

#ifndef __ACPI_RSDP_H
#define __ACPI_RSDP_H
/**
Root System Description Pointer definitions
https://wiki.osdev.org/RSDP
*/

struct RSDPDescriptor {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;
} __attribute__ ((packed));

struct RSDPDescriptor20 {
 struct RSDPDescriptor firstPart;

 uint32_t Length;
 uint64_t XsdtAddress;
 uint8_t ExtendedChecksum;
 uint8_t reserved[3];
} __attribute__ ((packed));

#endif
