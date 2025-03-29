#define GDT_BASE_ADDRESS 0x500  //this is a small memory area on the first page, below the bootloader etc. Perfect place for this to exist.
#define GDT_ENTRY_COUNT 0x6

#define MEMORY_MAP_STATICPTR 0x600  //put this above the GDT
#define MEMORY_MAP_LIMIT     0x100