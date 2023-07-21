[BITS 32]

%include "utils_32bit.asm"

;Offsets in the ELF file header table
%define KERNEL_BINARY_BASE  0x110000
%define ELF_IDENT       0x00    ;dword
%define ELF_BITS        0x04    ;byte.  1=>32-bit, 2=>64-bit. Lower offsets only valid for 32-bit.
%define ELF_ENDIAN      0x05    ;byte.  1=>LE, 2=>BE
%define ELF_VERSION     0x06    ;byte. Expect to be 1.
%define ELF_ABI         0x07    ;byte. Ignore as we are our own OS!
%define ELF_OBJECT_TYPE 0x10    ;word. Expect to be 0x02 (executable).
%define ELF_ISA         0x12    ;word. Instruction set architecture. Expect to be 0x03 (x86)
%define ELF_ENTRY_POINT 0x18    ;dword
%define ELF_PH_OFF      0x1C    ;dword. Offset to the program header table.
%define ELF_SH_OFF      0x20    ;dword. Offset to the section header table.
%define ELF_FILEHEADER_SIZE 0x28    ;word
%define ELF_PH_ENT_SIZE 0x2A    ;word. Size of a program header table entry.
%define ELF_PH_ENT_NUM  0x2C    ;word. Count of entries in the program header table.
%define ELF_SH_ENT_SIZE 0x2E    ;word. Size of a section header table entry
%define ELF_SH_ENT_NUM  0x30    ;word. Count of entries in the section header table.
%define ELF_SH_NAMES_IDX    0x32    ;word. Index of the section header table entry that contains section names.

;Offsets in the ELF section header tables. Non-useful ones left out.
%define SH_NAME         0x00    ;dword. Offset to string in .shstrtab section
%define SH_TYPE         0x04    ;dword. Refer to https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
%define SH_FLAGS        0x08    ;dword. Refer to https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
%define SH_ADDR         0x0C    ;dword. Virtual address of the section in memory, if it is to be loaded.
%define SH_OFFSET       0x10    ;dword. Offset in the file image where the section starts
%define SH_SIZE         0x14    ;dword. Size in bytes of the section in the file image. May be 0.

;Flag values for SH_FLAGS
%define SHF_WRITE       0x01
%define SHF_ALLOC       0x02
%define SHF_EXEC        0x04
%define SHF_MERGE       0x10
%define SHF_STRINGS     0x20
%define SHF_INFO_LINK   0x40
%define SHF_LINK_ORDER  0x80

;The prologue jumps us to here when we are in protected mode.
_pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov ax, 0x18
    mov fs, ax
    mov gs, ax
    mov esp, 0x200000   ;move our stack up to the top of the first Mb

    ;Whoohoo. Now we are in PM, the conventional RAM area is full of junk but the kernel itself is loaded from 0x10000 (and the size is a 32-bit integer at 0x1700)
    ;What we now want to do is clear out all the crud and just load our kernel into this area.
    ;So, what we are first going to do is to copy the entire conventional memory area into the 1Mb range. We are then going to jump ourselves into that copy
    ;of our code, parse the ELF headers, figure out which sections go where, re-write the conventional memory area and then jump back into the kernel.

    push dx

    ;Now, let's copy everything from 0x0 -> 0x7FFFF to 0x100000 -> 107FFFF (above that is reserved)
    mov ecx, 0x20000    ;0x80000 bytes = 0x20000 dwords
    mov esi, 0x0
    mov edi, 0x100000
    rep movsd

    ;When we enter PM, the prologue set dh to the current cursor row and dl to the current cursor column
    ;Put them somewhere our simple display driver can find them.
    pop dx
    mov byte [CursorRowPtr], DH
    mov byte [CursorColPtr], dl

    mov esi, _bl_relocation
    add esi, 0x100000
    jmp esi

;This label is a jump-target, when we get here we have jumped up out of conventional memory.
;Fortunately, 'call' still works as it uses a relative rather than an absolute address
_bl_relocation:
    ;Point to the relocated GDT
    mov esi, SimpleGDTPtr
    add esi, 0x100000
    mov eax, dword [esi+2]      ;first 2 bytes are length of the table, next 4 bytes are the location
    add eax, 0x100000
    mov dword [esi+2], eax
    lgdt [esi]

    ;Test; blank out the entire conventional memory area
    xor eax, eax
    mov edi, 0x500
    mov ecx, 0x1FEC0    ;range is 0x500 -> 0x80000 bytes, remember to divide by 4 for dwords!
    rep stosd

    ;Check if the file we loaded is actually an appropriate ELF binary
    call CheckKernelFormat

    ;Now load in all of the sections that should be loaded (i.e., which have SHF_ALLOC and size>0)
    mov edi, KERNEL_BINARY_BASE
    mov esi, [KERNEL_BINARY_BASE+ELF_SH_OFF]
    add esi, KERNEL_BINARY_BASE                ;esi offset relative to start of file
    mov ecx, 0
    _k_load_next_section:
    cmp cx, word [KERNEL_BINARY_BASE+ELF_SH_ENT_NUM]
    jz _k_all_sections_loaded
    call LoadInSection
    xor eax, eax
    mov ax, word [KERNEL_BINARY_BASE+ELF_SH_ENT_SIZE]  ;increment the pointer to the next entry
    add esi, eax
    inc cx
    jmp _k_load_next_section

    _k_all_sections_loaded:
    
    ;we now want to put back the memory information block, at 0x2500
    mov edi, TemporaryMemInfoLocation
    mov esi, TemporaryMemInfoLocation+0x100000
    xor eax, eax
    mov ax, word [esi]    ;the first 16 bytes are the count of entries
    mov ebx, 0x18
    mul ebx                                    ;we have reserved 24 bytes per entry, in detect_memory
    mov ecx, eax
    rep movsb

    ;tell the kernel where the cursor should be
    mov DH, byte [CursorRowPtr]
    mov dl, byte [CursorColPtr]

    ;now enter the kernel
    jmp 0x08:0x7e00

    ; mov esi, Temporary
    ; add esi, 0x100000
    ; call PMPrintString
    ; hlt
    ; jmp $

;Check that the file we loaded has an ELF header, and is 32-bit LE.
CheckKernelFormat:
    push ebp
    mov ebp, esp

    ;check magic number
    mov esi, ElfHeaderBytes
    add esi, 0x100000
    mov edi, KERNEL_BINARY_BASE        ;start of the loaded binary image
    mov ecx, 4
    call strncmp_32
    test ax, ax
    jnz _kf_inv_file        ;ax !=0 => this is not valid

    ;check architecture flags
    mov edi, KERNEL_BINARY_BASE
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
    add esi, 100000
    call PMPrintString
    hlt
    jmp $

    _kf_inv_arch:
    mov esi, InvalidElf
    add esi, 100000
    call PMPrintString
    hlt
    jmp $

;Loads the data for the given section into memory where it should be.
;Expects: edi -> ELF file header, esi -> section entry to load.
;Must preserve edi, esi & ecx.
LoadInSection:
    push ebp
    mov ebp, esp

    push ecx

    mov ax, [esi+SH_FLAGS]
    and ax, SHF_ALLOC
    test ax, ax                 ;if SHF_ALLOC is not set, then we do not need to allocate or load anything.
    jz _load_section_done

    mov ebx, [esi+SH_ADDR]      ;address to load in to
    test ebx, ebx
    jz _load_section_done       ;if no address, nothing to load in
    
    mov edx, [esi+SH_OFFSET]    ;address to load from

    mov ecx, [esi+SH_SIZE]      ;bytes to load
    test ecx, ecx
    jz _load_section_done

    push edi
    push esi

    mov esi, edx    ;copy from ds:si to es:di....
    add esi, KERNEL_BINARY_BASE
    mov edi, ebx
    rep movsb       ;we have already set ecx to the count of bytes to load.

    pop esi
    pop edi
    _load_section_done:
    pop ecx
    pop ebp
    ret
;Data
ElfHeaderBytes  db  0x7F, 0x45, 0x4c, 0x46      ;the "magic number" header at the start of a valid ELF file

InvalidFile     db 'The kernel is not a valid ELF executable', 0x0d, 0x0a, 0
InvalidElf      db 'The kernel is not compatible with this architecture', 0x0d, 0x0a, 0
Temporary       db 'Not implemented yet....', 0x0d, 0x0a, 0