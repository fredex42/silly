#define KERNEL_RELOCATION_BASE 0xff000000

//These addresses are on page 0 and don't get relocated.
#define GDT_BASE_ADDRESS 0x500  //this is a small memory area on the first page, below the bootloader etc. Perfect place for this to exist.
#define GDT_ENTRY_COUNT 0x6

#define IDTR_BASE_ADDRESS 0x700
#define IDT_BASE_ADDRESS 0x708
#define IDT_ENTRY_COUNT 0xFF

#define MEMORY_MAP_STATICPTR 0x600  //put this above the GDT
#define MEMORY_MAP_LIMIT     0x100