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

void acpi_setup_shortcuts(const struct RSDT *rsdt, uint32_t rsdt_phys)
{
  uint8_t valid;
  void *phys_ptr;
  char sig_str[8];
  char id_str[8];
  uint8_t pci_init_done = 0;

  kprintf("DEBUG RSDT virt 0x%x phys 0x%x\r\n", rsdt, rsdt_phys);
  memset(sig_str, 0, 8);
  strncpy(sig_str, rsdt->h.Signature, 5);
  kprintf("DEBUG RSDT sig '%s'\r\n", sig_str);
  memset(id_str, 0, 8);
  strncpy(id_str, rsdt->h.OEMID, 7);
  kprintf("DEBUG RSDT oemid '%s'\r\n", id_str);

  int entries = (rsdt->h.Length - sizeof(struct ACPISDTHeader)) / 4;
  kprintf("RSDT has %d entries with total length of %d bytes.\r\n", entries, rsdt->h.Length);
  ShortcutTableLength = (uint8_t) entries;

  acpi_shortcuts = acpi_shortcut_new(NULL, "RSDT", rsdt);

  for(register int i=0; i<entries; i++) {
    kprintf("DEBUG RSDT entry %d at 0x%x\r\n", i, rsdt->PointerToOtherSDT[i]);

    //It's important to note that the relative_ptr can be _negative_ e.g. in qemu the RSDT is on the _end_ of the tables not the _start_.
    //So it's a signed int32_t, then the comparison
    int32_t relative_ptr = (int32_t) rsdt->PointerToOtherSDT[i] - rsdt_phys;
    kprintf("DEBUG relative ptr is 0x%x\r\n", relative_ptr);
    
    //At present, we assume that all the tables are on the same page of RAM and just show a warning if we find one that isn't. 
    // This avoids the need for fiddly and expensive page-mappings here.
    if( (relative_ptr >0 && relative_ptr < 0x1000) || (relative_ptr < 0 && relative_ptr > -0x1000)) { //we are on the same page :D
      ACPISDTHeader *h = (ACPISDTHeader *)((uint32_t)rsdt + relative_ptr);
      kprintf("DEBUG virtual address of header is 0x%x\r\n", h);
      valid = validate_sdt_header(h);
      memset(sig_str, 0, 8);
      strncpy(sig_str, h->Signature, 5);
      memset(id_str, 0, 8);
      strncpy(id_str, h->OEMID, 7);

      kprintf("Table %d: Signature %s, OEM ID %s, Length %d, Valid %d.\r\n", i, sig_str, id_str, h->Length, (uint32_t)valid);
      if(h->Signature[0]=='A' && h->Signature[1]=='P' && h->Signature[2]=='I' && h->Signature[3]=='C') {
        read_madt_info((char *)h);
      } else if (h->Signature[0]=='M' && h->Signature[1]=='C' && h->Signature[2]=='F' && h->Signature[3]=='G') {
        pci_init((void *)h);
        pci_init_done = 1;
      }
    } else {
      kprintf("WARNING unable to access entry %d as it is on another RAM page\r\n", i);
    }
    // if(h->Signature[0]=='A' && h->Signature[1]=='P' && h->Signature[2]=='I' && h->Signature[3]=='C') {
    //   read_madt_info((char *)h);
    // }
    if(i>10) break;
  }

  if(!pci_init_done) {
    pci_init(NULL);
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
  uint8_t *bios_area = map_bios_area();
  const struct RSDPDescriptor* rsdp;

  const struct RSDPDescriptor* rsdp_temp = scan_memory_for_acpi(bios_area);
  if(rsdp_temp==NULL) {
    kputs("Could not find ACPI information :(\r\n");
    unmap_bios_area(bios_area);
    return;
  } else {
    rsdp = (const struct RSDPDescriptor *)malloc(sizeof(struct RSDPDescriptor));
    if(!rsdp) {
      k_panic("Unable to allocate memory for RSDP descriptor");
    }
    memcpy(rsdp, rsdp_temp, sizeof(struct RSDPDescriptor)); //FIXME: meh, probably un-necessary
    unmap_bios_area(bios_area);

    kprintf("ACPI data version %d found at 0x%x\r\n", rsdp->Revision, (uint32_t) rsdp);
    kprintf("ACPI OEM is %s\r\n", rsdp->OEMID);
    
    kputs("INFO Validating ACPI descriptor table...");
    if(!validate_rsdp_checksum(rsdp)) {
      kputs("ERROR ACPI checksum did not validate\r\n");
      return;
    }
    kputs("Done.\r\n");

    //ACPI tables should be at the end of RAM, so identity-map it to keep it seperate from our other stuff
    k_map_page_identity(NULL, rsdp->RsdtAddress, MP_PRESENT);
    const struct RSDT* rsdt = (const struct RSDT *)rsdp->RsdtAddress;
    kprintf("DEBUG mapped location of 0x%x is 0x%x\r\n", rsdp->RsdtAddress, rsdt);

    acpi_setup_shortcuts(rsdt, rsdp->RsdtAddress);
    kprintf("ACPI shortcut table at 0x%x has %d entries\r\n",acpi_shortcuts, ShortcutTableLength);
    uint16_t baf = get_boot_arch_flags();
    kprintf("DEBUG boot arch flags value is 0x%x\r\n", baf);
  }
}
