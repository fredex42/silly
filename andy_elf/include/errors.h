#ifndef __ERRORS_H
#define __ERRORS_H

#define E_OK        0
#define E_BUSY      1
#define E_PARAMS    2
#define E_NOMEM     3

#define E_VFAT_NOT_RECOGNIZED 0x11
#define E_INVALID_DEVICE      0x12
#define E_INVALID_FILE        0x13

#define E_BAD_EXECUTABLE      0x21
#define E_NOT_ELF             0x22
#define E_UNSUPPORTED_ELF     0x23
#define E_MALFORMED_ELF       0x24
// #define E_ELF_LOAD_FAILED     0x25
// #define E_ELF_NOLOADABLES     0x26
// #define E_ELF_SEGMENT_FAILED  0x27
// #define E_ELF_NO_ENTRYPOINT   0x28
// #define E_ELF_RELOC_FAILED    0x29
// #define E_ELF_SYMBOL_NOT_FOUND 0x2A

#endif
