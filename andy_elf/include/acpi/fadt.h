#include <types.h>
#include <acpi/sdt_header.h>

#ifndef __ACPI_FADT_H
#define __ACPI_FADT_H

struct FADT
{
    struct   ACPISDTHeader h;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t  Reserved;

    uint8_t  PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t  AcpiEnable;
    uint8_t  AcpiDisable;
    uint8_t  S4BIOS_REQ;
    uint8_t  PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t  PM1EventLength;
    uint8_t  PM1ControlLength;
    uint8_t  PM2ControlLength;
    uint8_t  PMTimerLength;
    uint8_t  GPE0Length;
    uint8_t  GPE1Length;
    uint8_t  GPE1Base;
    uint8_t  CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t  DutyOffset;
    uint8_t  DutyWidth;
    uint8_t  DayAlarm;
    uint8_t  MonthAlarm;
    uint8_t  Century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t iaPCBootArch;  //see IAPC_BOOT_ARCH in documentation e.g. https://uefi.org/sites/default/files/resources/ACPI_Spec_6_3_A_Oct_6_2020.pdf

    uint8_t  Reserved2;
    uint32_t Flags;

    // 12 byte structure; see below for details
    GenericAddressStructure ResetReg;

    uint8_t  ResetValue;
    uint8_t  Reserved3[3];

    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_FirmwareControl;
    uint64_t                X_Dsdt;

    GenericAddressStructure X_PM1aEventBlock;
    GenericAddressStructure X_PM1bEventBlock;
    GenericAddressStructure X_PM1aControlBlock;
    GenericAddressStructure X_PM1bControlBlock;
    GenericAddressStructure X_PM2ControlBlock;
    GenericAddressStructure X_PMTimerBlock;
    GenericAddressStructure X_GPE0Block;
    GenericAddressStructure X_GPE1Block;
};

//iaPCBootArch flags for x86 (not ARM) - see https://uefi.org/sites/default/files/resources/ACPI_Spec_6_3_A_Oct_6_2020.pdf p. 156
#define BA_LEGACY_DEVICES       1<<0    // If set, indicates that the motherboard supports user-visible devices on the LPC or ISA bus. If clear, the OS may assume that all devices
                                    //in the system can be detected exclusively via indus-try standard device enumeration mechanisms
#define BA_8042_PRESENT         1<<1    // Indicates the presence of a PS/2 keyboard/mouse controller
#define BA_VGA_NOT_PRESENT      1<<2    // If set, do not probe the VGA hardware
#define BA_MSI_NOT_SUPPORTED    1<<3    //do not enable message-signalled interrupts
#define BA_PCIE_ASPM            1<<4    //do not enable OSPM / ASPM control
#define CMOS_RTC_NOT_PRESENT    1<<5    //if set, there is no CMOS RTC on legacy addresses, must use ACPI instead
//other bits are reserved and must be 0.
#endif
