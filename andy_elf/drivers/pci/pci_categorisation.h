#include <types.h>

//Data taken from https://wiki.osdev.org/PCI#Enumerating_PCI_Buses
//Class Codes
#define PCI_CC_UNCLASSIFIED     0x00
#define PCI_CC_MASS_STORAGE     0x01
#define PCI_CC_NETWORK          0x02
#define PCI_CC_DISPLAY          0x03
#define PCI_CC_MULTIMEDIA       0x04
#define PCI_CC_MEMORY           0x05
#define PCI_CC_BRIDGE           0x06

//Unclassified class
#define PCI_SUBC_NON_VGA_UNCLASSIFIED           0x00
#define PCI_SUBC_VGA_COMPATIBLE_UNCLASSIFIED    0x01

//Mass storage class
#define PCI_SUBC_MASS_SCSI      0x00
#define PCI_SUBC_MASS_IDE       0x01
#define PCI_SUBC_MASS_FLOPPY    0x03
#define PCI_SUBC_MASS_IPI       0x04
#define PCI_SUBC_MASS_RAID      0x05
#define PCI_SUBC_MASS_ATA       0x06
#define PCI_SUBC_MASS_SAS       0x07
#define PCI_SUBC_MASS_NVM       0x08
#define PCI_SUBC_MASS_OTHER     0x80

//Network controller class
#define PCI_SUBC_NET_ETHER      0x00
#define PCI_SUBC_NET_TOKEN      0x01
#define PCI_SUBC_NET_FDDI       0x02
#define PCI_SUBC_NET_ATM        0x03
#define PCI_SUBC_NET_ISDN       0x04
#define PCI_SUBC_NET_WORLDFIP   0x05
#define PCI_SUBC_NET_PICMG      0x06
#define PCI_SUBC_NET_INFINIBAND 0x07
#define PCI_SUBC_NET_FABRIC     0x08
#define PCI_SUBC_NET_OTHER      0x80

//Display controller class
#define PCI_SUBC_DISPLAY_VGA    0x00
#define PCI_SUBC_DISPLAY_XGA    0x01
#define PCI_SUBC_DISPLAY_3DCTRL 0x02
#define PCI_SUBC_DISPLAY_OTHER  0x80

//Multimedia class - fill me in!
//Memory class - fill me in!

//Bridge class
#define PCI_SUBC_BRIDGE_HOST    0x00
#define PCI_SUBC_BRIDGE_ISA     0x01
#define PCI_SUBC_BRIDGE_EISA    0x02
#define PCI_SUBC_BRIDGE_MCA     0x03
#define PCI_SUBC_BRIDGE_PCI     0x04
#define PCI_SUBC_BRIDGE_PCMCIA  0x05
#define PCI_SUBC_BRIDGE_NUBUS   0x06
#define PCI_SUBC_BRIDGE_CARDBUS 0x07
#define PCI_SUBC_BRIDGE_RACEWAY 0x08
#define PCI_SUBC_BRIDGE_PCI2    0x09
#define PCI_SUBC_BRIDGE_INFINIBAND  0x0a
#define PCI_SUBC_BRIDGE_OTHER   0x80

const char* pci_description_string(uint8_t pci_class, uint8_t subclass);
