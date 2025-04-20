[BITS 32]

extern cmain
global _start

_start:
    ; save the boot device location (given to us by the bootloader)
    mov byte [0x500], bl
    mov word [0x501], dx

    ; fix up the stack before we go to C
    mov esp, 0x80000    ;just below BIOS area. This will get relocated.
    
    ; now go to the real app
    jmp cmain