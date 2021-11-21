#include <acpi/sdt_header.h>

/**
performs checksum validation on the given SDT header.
Returns 1 if valid, 0 if not valid.
*/
uint8_t validate_sdt_header(ACPISDTHeader *h)
{
  char *buf=(char *)h;
  size_t buf_len = sizeof(ACPISDTHeader);
  uint32_t sum=0;

  for(register uint32_t i=0;i<buf_len;i++) {
    sum += buf[i];
  }

  if((uint8_t)sum==0) return 1;
  return 0;
}
