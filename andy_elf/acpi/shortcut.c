#include <types.h>
#include <acpi/rsdt.h>
#include <acpi/fadt.h>
#include <memops.h>
#include <memory.h>

struct AcpiTableShortcut *acpi_shortcut_list_end(struct AcpiTableShortcut *list) 
{
    if(!list) return NULL;

    struct AcpiTableShortcut *ptr = list;
    while(ptr->next != NULL) ptr = ptr->next;
    return ptr;
}

/**
 * Adds a new entry into the given AcpiTableshortcut list and returns a pointer to the new entry.
 * `sig` must be 4 chars long, less than this will cause invalid data.
*/
struct AcpiTableShortcut *acpi_shortcut_new(struct AcpiTableShortcut *list, char *sig, void *ptr)
{
    struct AcpiTableShortcut *new_sc = (struct AcpiTableShortcut *)malloc(sizeof(struct AcpiTableShortcut));
    if(!new_sc) {
        kputs("ERROR Could not allocate memory for shortcuts table\r\n");
        return NULL;
    }
    memset(new_sc, 0, sizeof(struct AcpiTableShortcut));

    memcpy(new_sc->sig, sig, 4);
    new_sc->ptr = ptr;

    if(list != NULL) {
        struct AcpiTableShortcut *list_end = acpi_shortcut_list_end(list);
        list_end->next = new_sc;
    }

    return new_sc;
}

/**
 * Tries to find the given ACPI table in the shortcuts. Returns NULL if not found, or a pointer if it is.
*/
struct AcpiTableShortcut *acpi_shortcut_find(struct AcpiTableShortcut *list, char *sig)
{
    struct AcpiTableShortcut *ptr;
    for(ptr=list; ptr!=NULL; ptr=ptr->next) {
        if(memcmp(sig, ptr->sig, 4)==0) return ptr;
    }
    return NULL;
}
