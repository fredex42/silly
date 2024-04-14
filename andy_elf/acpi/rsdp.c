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

void acpi_setup_shortcuts(const struct RSDT *rsdt, uint32_t rsdt_phys)
{
  uint8_t valid;
  void *phys_ptr;
  char sig_str[8];
  char id_str[8];

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

  acpi_shortcuts[0].sig[0] = 'R';
  acpi_shortcuts[0].sig[1] = 'S';
  acpi_shortcuts[0].sig[2] = 'D';
  acpi_shortcuts[0].sig[3] = 'T';

  acpi_shortcuts[0].ptr = rsdt;

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
      }
    } else {
      kprintf("WARNING unable to access entry %d as it is on another RAM page\r\n", i);
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

    //this is probably outside our current mapped area (on bochs it is up around the end of provisioned ram)
    //so map it into somewhere we can access.  We want read-only for kernel so don't set MP_READWRITE or MP_USER.
    //choosing page 256 (=1Mbyte) because I know it has not yet been mapped.
    kprintf("DEBUG RSDT physical address is 0x%x\r\n", rsdp->RsdtAddress);
    void *phys_ptr = (void *)((vaddr)rsdp->RsdtAddress & (~0xFFF));
    kprintf("DEBUG RSDT physical page address is 0x%x\r\n", phys_ptr);
    vaddr offset = ((vaddr)rsdp->RsdtAddress - (vaddr)phys_ptr);

    void *rsdt_base = (const struct RSDT *)vm_map_next_unallocated_pages(NULL, MP_PRESENT, &phys_ptr, 1);
    kprintf("DEBUG mapped location of 0x%x is 0x%x\r\n", phys_ptr, rsdt_base);

    const struct RSDT* rsdt = (const struct RSDT *)((vaddr)rsdt_base + offset);
    acpi_setup_shortcuts(rsdt, rsdp->RsdtAddress);
    //kprintf("ACPI shortcut table at 0x%x has %d entries\r\n",acpi_shortcuts, ShortcutTableLength);
  }
}
