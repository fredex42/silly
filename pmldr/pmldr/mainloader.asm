[BITS 32]

%include "utils_32bit.asm"

;Offsets in the ELF file header table
%define ELF_IDENT       0x00    ;dword
%define ELF_BITS        0x04    ;byte.  1=>32-bit, 2=>64-bit
%define ELF_ENDIAN      0x05    ;byte.  1=>LE, 2=>BE
%define ELF_VERSION     0x06    ;byte. Expect to be 1.
%define ELF_ABI         0x07    ;byte. Ignore as we are our own OS!
%define ELF_OBJECT_TYPE 0x10    ;word. Expect to be 0x02 (executable).
%define ELF_ISA         0x12    ;word. Instruction set architecture. Expect to be 0x03 (x86)

;The prologue jumps us to here when we are in protected mode.
_pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ax, 0x18
    mov fs, ax
    mov gs, ax

    ;Whoohoo. Now we are in PM, the conventional RAM area is full of junk but the kernel itself is loaded from 0x10000 (and the size is a 32-bit integer at 0x1700)
    ;What we now want to do is clear out all the crud and just load our kernel into this area.
    ;So, what we are first going to do is to copy the entire conventional memory area into the 1Mb range. We are then going to jump ourselves into that copy
    ;of our code, parse the ELF headers, figure out which sections go where, re-write the conventional memory area and then jump back into the kernel.

    ;When we enter PM, the prologue set dh to the current cursor row and dl to the current cursor column
    ;Put them somewhere our simple display driver can find them.
    mov byte [CursorRowPtr], DH
    mov byte [CursorColPtr], dl

    call CheckKernelFormat

    ;Now, let's copy everything from 0x0 -> 0x7FFFF to 0x100000 -> 107FFFF (above that is reserved)
    mov ecx, 0x20000    ;0x80000 bytes = 0x20000 dwords
    mov esi, 0x0
    mov edi, 0x100000
    rep movsd

    mov esi, _bl_relocation
    add esi, 0x100000
    jmp esi

;This label is a jump-target, when we get here we have jumped up out of conventional memory.
_bl_relocation:
    
    hlt
    jmp $

;Check that the file we loaded has an ELF header, and is 32-bit LE.
CheckKernelFormat:
    push ebp
    mov ebp, esp

    ;check magic number
    mov esi, ElfHeaderBytes
    mov edi, 0x10000        ;start of the loaded binary image
    mov ecx, 4
    call strncmp_32
    test ax, ax
    jnz _kf_inv_file        ;ax !=0 => this is not valid

    ;check architecture flags
    mov edi, 0x10000
    mov al, byte [edi+ELF_BITS]
    cmp al, 1
    jnz _kf_inv_arch
    mov al, byte [edi+ELF_ENDIAN]
    cmp al, 1
    jnz _kf_inv_arch
    mov ax, word [edi+ELF_ISA]
    cmp ax, 0x03
    jnz _kf_inv_arch

    pop ebp
    ret

    _kf_inv_file:
    mov esi, InvalidFile
    call PMPrintString
    hlt
    jmp $

    _kf_inv_arch:
    mov esi, InvalidElf
    call PMPrintString
    hlt
    jmp $

;Data
ElfHeaderBytes  db  0x7F, 0x45, 0x4c, 0x46      ;the "magic number" header at the start of a valid ELF file

InvalidFile     db 'The provided kernel is not a valid ELF executable', 0x0d, 0x0a, 0
InvalidElf      db 'The provided kernel is not compatible with this architecture', 0x0d, 0x0a, 0