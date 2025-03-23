#include <acpi/sdt_header.h>

#ifndef __ACPI_RSDT_H
#define __ACPI_RSDT_H

struct RSDT {
  struct ACPISDTHeader h;
  uint32_t PointerToOtherSDT[255];  //need to set a static value here, should be <255.
}  __attribute__ ((packed));

struct AcpiTableShortcut {
  struct AcpiTableShortcut *next;
  char sig[4];
  void *ptr;
};

struct AcpiTableShortcut *acpi_shortcut_find(struct AcpiTableShortcut *list, char *sig);
struct AcpiTableShortcut *acpi_shortcut_new(struct AcpiTableShortcut *list, char *sig, void *ptr);
struct AcpiTableShortcut *acpi_shortcut_list_end(struct AcpiTableShortcut *list) ;

//Pointer to the Fixed Acpi Description Table - see https://wiki.osdev.org/FADT
#define GetFADTPointer(list) acpi_shortcut_find(list, "FACP");

/**
 * Returns a READ-ONLY pointer to the Multi APIC Description Table
*/
void* acpi_get_madt();

#endif
