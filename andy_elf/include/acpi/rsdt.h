#include <acpi/sdt_header.h>

#ifndef __ACPI_RSDT_H
#define __ACPI_RSDT_H

struct RSDT {
  struct ACPISDTHeader h;
  uint32_t PointerToOtherSDT[255];  //need to set a static value here, should be <255.
};

struct AcpiTableShortcut {
  char sig[4];
  void *ptr;
};

#endif
