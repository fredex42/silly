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
#define PCI_CC_SIMPLECOMMS      0x07
#define PCI_CC_BASE_SYSTEM      0x08
#define PCI_CC_INPUTDEVICE      0x09
#define PCI_CC_DOCKINGSTATION   0x0a
#define PCI_CC_PROCESSOR        0x0b
#define PCI_CC_SERIALBUS        0x0c
#define PCI_CC_WIRELESS         0x0d
#define PCI_CC_INTELLIGENT      0x0e
#define PCI_CC_SATELLITE        0x0f
#define PCI_CC_ENCRYPTION       0x10
#define PCI_CC_SIGNALPROCESSOR  0x11

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

//Simple communciation controller
#define PCI_SUBC_SIMPLE_SERIAL      0x00
#define PCI_SUBC_SIMPLE_PARALLEL    0x01
#define PCI_SUBC_SIMPLE_MULTIPORT   0x02
#define PCI_SUBC_SIMPLE_MODEM       0x03
#define PCI_SUBC_SIMPLE_IEEE488     0x04
#define PCI_SUBC_SIMPLE_SMARTCARD   0x05

//Base system peripheral
#define PCI_SUBC_BASE_PIC           0x00
#define PCI_SUBC_BASE_DMA           0x01
#define PCI_SUBC_BASE_TIMER         0x02
#define PCI_SUBC_BASE_RTC           0x03
#define PCI_SUBC_BASE_PCIHOTPLUG    0x04
#define PCI_SUBC_BASE_SDCONTROLLER  0x05
#define PCI_SUBC_BASE_IOMMU         0x06

//Input controller class - fill me in!
//Docking station class - fill me in!
//Processor class - fill me in!

//Serial bus controller
#define PCI_SUBC_SBUS_FIREWIRE      0x00
#define PCI_SUBC_SBUS_ACCESS        0x01
#define PCI_SUBC_SBUS_SSA           0x02
#define PCI_SUBC_SBUS_USB           0x03
#define PCI_SUBC_SBUS_FIBRECHANNEL  0x04
#define PCI_SUBC_SBUS_SMBUS         0x05
#define PCI_SUBC_SBUS_INFINIBAND    0x06
#define PCI_SUBC_SBUS_IPMI          0x07
#define PCI_SUBC_SBUS_SERCOS        0x08
#define PCI_SUBC_SBUS_CANBUS        0x09

const char* pci_description_string(uint8_t pci_class, uint8_t subclass);
