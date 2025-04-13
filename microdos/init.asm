[BITS 32]

extern cmain
global _start

_start:
    ; save the boot device location (given to us by the bootloader)
    mov byte [0x500], bl
    mov word [0x501], dx

    ; now go to the real app
    jmp cmain