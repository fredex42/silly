#include <acpi/rsdp.h>
#include <acpi/rsdt.h>
#include <stdio.h>
#include <sys/mmgr.h>
#include "search_mem.h"

static struct AcpiTableShortcut acpi_shortcuts[128];
static uint8_t ShortcutTableLength=0;

/**
Validate that the bits of an RSDPDescriptor struct
checksum correctly. See https://wiki.osdev.org/RSDP.
*/
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

void acpi_setup_shortcuts(const struct RSDT *rsdt)
{
  uint8_t valid;
  int entries = (rsdt->h.Length - sizeof(struct ACPISDTHeader)) / 4;
  kprintf("RSDT has %d entries with total length of %d bytes.\r\n", entries, rsdt->h.Length);
  ShortcutTableLength = (uint8_t) entries;

  acpi_shortcuts[0].sig[0] = 'R';
  acpi_shortcuts[0].sig[1] = 'S';
  acpi_shortcuts[0].sig[2] = 'D';
  acpi_shortcuts[0].sig[3] = 'T';

  acpi_shortcuts[0].ptr = rsdt;

  for(register int i=0; i<entries; i++) {
    //passing NULL as the base_directory uses the static kernel one
    struct ACPISDTHeader *h = (struct ACPISDTHeader *) k_map_if_required(NULL, rsdt->PointerToOtherSDT[i], 0);
    valid = validate_sdt_header(h);
    kprintf("Table %d: Signature %s, OEM ID %s, Length %d, Valid %d.\r\n", i, h->Signature, h->OEMID, h->Length, (uint32_t)valid);
    if(h->Signature[0]=='A' && h->Signature[1]=='P' && h->Signature[2]=='I' && h->Signature[3]=='C') {
      read_madt_info((char *)h);
    }
    if(i>10) break;
  }
  // for(int i=1; i< entries+1; i++) {
  //   struct ACPISDTHeader *h = (struct ACPISDTHeader *) k_map_if_required(rsdt->PointerToOtherSDT[i], 0);
  //   acpi_shortcuts[i].sig[0] = h->Signature[0];
  //   acpi_shortcuts[i].sig[1] = h->Signature[1];
  //   acpi_shortcuts[i].sig[2] = h->Signature[2];
  //   acpi_shortcuts[i].sig[3] = h->Signature[3];
  //
  //   acpi_shortcuts[i].ptr = h;
  // }
}

void load_acpi_data() {
  const struct RSDPDescriptor* rsdp = scan_memory_for_acpi();
  if(rsdp==NULL) {
    kputs("Could not find ACPI information :(\r\n");
    return;
  } else {
    kprintf("ACPI data version %d found at 0x%x\r\n", rsdp->Revision, (uint32_t) rsdp);
    kputs("ACPI OEM is ");
    kputs(rsdp->OEMID);
    if(!validate_rsdp_checksum(rsdp)) {
      kputs("ERROR ACPI checksum did not validate\r\n");
      return;
    }

    //this is probably outside our current mapped area (on bochs it is up around the end of provisioned ram)
    //so map it into somewhere we can access.  We want read-only for kernel so don't set MP_READWRITE or MP_USER.
    //choosing page 256 (=1Mbyte) because I know it has not yet been mapped.
    const struct RSDT* rsdt = (const struct RSDT *)k_map_page(NULL, rsdp->RsdtAddress, 0, 256, MP_PRESENT);
    kprintf("DEBUG mapped location of 0x%x is 0x%x\r\n", rsdp->RsdtAddress, rsdt);

    acpi_setup_shortcuts(rsdt);
    //kprintf("ACPI shortcut table at 0x%x has %d entries\r\n",acpi_shortcuts, ShortcutTableLength);
  }
}
