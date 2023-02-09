#include <acpi/rsdp.h>
#include <acpi/rsdt.h>
#include <acpi/fadt.h>
#include <stdio.h>
#include <sys/mmgr.h>
#include "search_mem.h"

static struct AcpiTableShortcut *acpi_shortcuts;
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

/**
 * Returns the IAPC_BOOT_ARCH value from the Fixed Acpi Description Table. See the BA_* constants in fadt.h to interpret the result
*/
uint16_t get_boot_arch_flags()
{
  struct AcpiTableShortcut *sc = acpi_shortcut_find(acpi_shortcuts, "FACP");  //yes, the signature for the FADT is "FACP". Go figure.
  if(!sc) {
    kprintf("WARNING There is no ACPI FADT present. Either the BIOS is very old or this has been called before ACPI initialisation\r\n");
    return 0;
  }
  struct FADT *fadt = (struct FADT *)sc->ptr;
  kprintf("DEBUG FADT is at memory address 0x%x\r\n", fadt);
  if((fadt->iaPCBootArch & BA_8042_PRESENT)!=0) kprintf("DEBUG Detected 8042 ps/2 controller\r\n");
  return fadt->iaPCBootArch;
}

void acpi_setup_shortcuts(const struct RSDT *rsdt)
{
  uint8_t valid;
  char *printable_sig = (char *)malloc(8);
  char *printable_oem = (char *)malloc(8);
  memset(printable_sig, 0, 8);
  memset(printable_oem, 0, 8);

  int entries = (rsdt->h.Length - sizeof(struct ACPISDTHeader)) / 4;
  kprintf("RSDT has %d entries with total length of %d bytes.\r\n", entries, rsdt->h.Length);
  ShortcutTableLength = (uint8_t) entries;

  acpi_shortcuts = acpi_shortcut_new(NULL, "RSDT", rsdt);

  for(register int i=0; i<entries; i++) {
    //passing NULL as the base_directory uses the static kernel one
    k_map_page_identity(NULL, rsdt->PointerToOtherSDT[i], MP_PRESENT);
    struct ACPISDTHeader *h = (struct ACPISDTHeader *) rsdt->PointerToOtherSDT[i];
    valid = validate_sdt_header(h);
    memcpy(printable_sig, h->Signature, 4);
    memcpy(printable_oem, h->OEMID, 6);

    kprintf("Table %d: Signature %s, OEM ID %s, Length %d, Valid %d.\r\n", i, h->Signature, h->OEMID, h->Length, (uint32_t)valid);

    //if(valid) {
      acpi_shortcut_new(acpi_shortcuts, h->Signature, h);
    //  ++ShortcutTableLength;
    //}
    // if(h->Signature[0]=='A' && h->Signature[1]=='P' && h->Signature[2]=='I' && h->Signature[3]=='C') {
    //   read_madt_info((char *)h);
    // }
    if(i>10) break;
  }

  free(printable_sig);
  free(printable_oem);
}

void acpi_load_data() {
  acpi_shortcuts = NULL;
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

    //ACPI tables should be at the end of RAM, so identity-map it to keep it seperate from our other stuff
    k_map_page_identity(NULL, rsdp->RsdtAddress, MP_PRESENT);
    const struct RSDT* rsdt = (const struct RSDT *)rsdp->RsdtAddress;
    kprintf("DEBUG mapped location of 0x%x is 0x%x\r\n", rsdp->RsdtAddress, rsdt);

    acpi_setup_shortcuts(rsdt);
    kprintf("ACPI shortcut table at 0x%x has %d entries\r\n",acpi_shortcuts, ShortcutTableLength);
    uint16_t baf = get_boot_arch_flags();
    kprintf("DEBUG boot arch flags value is 0x%x\r\n", baf);
  }
}
