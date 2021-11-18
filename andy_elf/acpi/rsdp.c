#include <acpi/rsdp.h>
#include <stdio.h>
#include "search_mem.h"


int8_t validate_rsdp_checksum(const struct RSDPDescriptor* rsdp)
{
  char *buf = (char *)rsdp; //cast the descriptor into an array of bytes
  int32_t sum = 0;

  for(int16_t i=0; i<sizeof(struct RSDPDescriptor);i++) {
    sum += buf[i];
  }
  if((char)sum==0) return 1;
  return 0;
}


void load_acpi_data() {
  const struct RSDPDescriptor* rsdp = scan_memory_for_acpi();
  if(rsdp==NULL) {
    kputs("Could not find ACPI information :(\r\n");
    return;
  } else {
    kprintf("ACPI data version %d found at 0x%x\r\n", rsdp->Revision, (uint32_t) rsdp);
    kputs("ACPI vendor is ");
    kputs(rsdp->OEMID);
    if(!validate_rsdp_checksum(rsdp)) {
      kputs("ERROR ACPI checksum did not validate\r\n");
      return;
    }
  }
}
