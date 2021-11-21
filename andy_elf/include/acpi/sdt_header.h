#include <types.h>

#ifndef __SDT_HEADER_H
#define __SDT_HEADER_H

typedef struct ACPISDTHeader {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
} ACPISDTHeader;

typedef struct GenericAddressStructure
{
  uint8_t AddressSpace;
  uint8_t BitWidth;
  uint8_t BitOffset;
  uint8_t AccessSize;
  uint64_t Address;
} GenericAddressStructure;

/**
performs checksum validation on the given SDT header.
Returns 1 if valid, 0 if not valid.
*/
uint8_t validate_sdt_header(ACPISDTHeader *h);

#endif
