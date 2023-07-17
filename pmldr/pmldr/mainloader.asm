[BITS 32]


;The prologue jumps us to here when we are in protected mode.
_pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ax, 0x18
    mov fs, ax
    mov gs, ax

    
    hlt
    jmp $