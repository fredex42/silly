;What is the purpose of PMLDR? Well, it's a second-stage bootloader.  This extra space (beyond the <512byte limit allowed on the bootsector)
;allows us to jump into protected mode and to then use the system's extended memory to load the kernel so that we don't have to keep hopping
;and bopping around 16-bit conventional RAM and hitting arbitary limits as we do so.  Loading this way should be simpler, and also cleaner,
;as the kernel code won't need a 16-bit prologue part.

%include "prologue.asm"
%include "mainloader.asm"

%include "ata.asm"