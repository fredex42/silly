[BITS 32]
section .text
global _start
global idle_loop			;NOTE this is NOT a function but a label, a place that gets returned to.

; remove this definition to disable serial console (also remove in basic_console.s)
%define SERIAL_CONSOLE 1

extern early_serial_lowlevel_init

global SimpleTSS	;we need to set the current ESP in TSS when we move to ring3

%include "basic_console.inc"
%include "memlayout.asm"
%include "exceptions.inc"

; We start off using the GDT which was configured by PMLDR, then later on the GDT is shifted to "FullGDT" which
; includes ring3 code/data selectors and a Task Segment Selector as well.

;assuming that DH is still the cursor row and DL is still the cursor position

_start:
cli             ;ensure that interrupts are disabled

; When loading from multiboot2, eax should contain the magic number and ebx the address of the multiboot2 info structure.
; The push order matters, these are not popped but are passed as parameters to a C function later on.
push ebx
push eax

;set up serial debugging output. Vital if SERIAL_CONSOLE is defined in basic_console.inc
call early_serial_lowlevel_init


; We first need to set up the GDT, so we can reliably access segment registers.  We might as well sort out the TSS and ring3 selectors at the same time.
;Set up a TSS. We'll need this in order to jump from ring 3 to ring 0 for interrupts etc.
mov edi, FullTSS
mov eax, 0					;link to previous TSS, not used
stosd
mov eax, 0x7FFF0		;our stack pointer. This needs to be over-written every time we switch to ring3 with the current SP position
stosd
mov eax, 0x10				;our stack segment
stosd
;everything else is unused
xor eax, eax
mov ecx, 24
rep stosd

;Now set up a proper GDT.
cld							;clear direction flag, we want to write forwards
mov edi, FullGDT			;set up for writing GDT

mov dword [edi], 0
mov dword [edi+4], 0
add edi, 8
;entry 1 (segment 0x08): kernel CS
mov word [edi], 0xffff	;limit bits 0-15
mov word [edi+2], 0x0000	;base bits 0-15
mov byte [edi+4], 0x00	;base bits 16-23
mov byte [edi+5], 0x9A		;access byte. Set Pr, Privl=0, S=1, Ex=1, DC=0, RW=1, Ac=0
mov byte [edi+6], 0xCF		;limit bits 16-19 [lower], flags [higher]. Set Gr=1 [page addressing], Sz=1 [32-bit sector]
mov byte [edi+7], 0x00		;base bits 24-31
add edi, 8
;entry 2 (segment 0x10): kernel DS. Allow this to span the whole addressable space.
mov word [edi], 0xffff	;limit bits 0-15
mov word [edi+2], 0x0000	;base bits 0-15
mov word [edi+4], 0x00		;base bits 16-23. Start from 0meg.
mov word [edi+5], 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
mov word [edi+6], 0xCf		;limit bits 16-19 [lower], flags [higher]. Set Gr=1 [page addressing], Sz=1 [32-bit sector]
mov word [edi+7], 0x00		;base bits 24-31
add edi, 8
;entry 3 (segment 0x18): video RAM. See https://wiki.osdev.org/Memory_Map_(x86).
mov word [edi], 0xFFFF	; limit bits 0-15.
mov word [edi+2], 0x0000	; base bits 0-15
mov word [edi+4], 0x0A		; base bits 16-23. Start from 0x0A0000
mov word [edi+5], 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
mov word [edi+6], 0x41		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
mov word [edi+7], 0x00		;base bits 24-31
add edi, 8

;edi should now be pointing to FullGDT+0x20
;entry 4 (segment 0x20): TSS. See https://wiki.osdev.org/Task_State_Segment.
mov word [edi], 0x68				;limit (TSS size) bits 0-15. 104 bytes = 0x68 (note last 2 fields are word not dword)
mov word [edi+2], FullTSS		;base (TSS location) bits 0-15.
mov byte [edi+4], 0					;base (TSS location) bits 0-15.
mov byte [edi+5], 0x89			;access byte. Set Pr, Privl=0, S=0, Ex=0, DC=0, RW=1,Ac=1
mov byte [edi+6], 0x40			;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector], available=1
mov byte [edi+7], 0x00			;base bits 24-31
;entry 5 (segment 0x28): User-mode CS
mov word [edi+8], 0xFFFF    ;limit bits 0-15
mov word [edi+10], 0x0000	;base bits 0-15
mov byte [edi+12], 0x0000	;base bits 16-23
mov byte [edi+13], 0xFA		;access byte. Set Pr, Privl=3, S=1, Ex=1, DC=0, RW=1, Ac=0
mov byte [edi+14], 0xCF		;limit bits 16-19 [lower], flags [higher]. Set Gr=1 [page addressing], Sz=1 [32-bit sector]
mov byte [edi+15], 0x00		;base bits 24-31
;entry 6 (segment 0x30): User-mode DS
mov word [edi+16], 0xffff	;limit bits 0-15
mov word [edi+18],  0x0000	;base bits 0-15
mov byte [edi+20],  0x00		;base bits 16-23. Start from 0meg.
mov byte [edi+21],  0xF2		;access byte. Set Pr, Privl=3, S=1, Ex=0, DC=0, RW=1, Ac=0
mov byte [edi+22], 0xCf		;limit bits 16-19 [lower], flags [higher]. Set Gr=1 [page addressing], Sz=1 [32-bit sector]
mov byte [edi+23], 0x00		;base bits 24-31

;OK, that's set up, now tell the processor
mov edi, FullGDTPtr
mov word [edi], 0x38				;length in bytes
mov dword [edi+2], FullGDT	;memory location
lgdt [edi]

; now we must tell it to reload the segment registers. We can do this with a far jump
jmp 0x08:.flush2

.flush2:
mov ax, 0x10
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax

mov byte [CursorColPtr], dl
mov byte [CursorRowPtr], dh

mov esi, HelloString
call PMPrintString


;And tell the processor the correct TSS to use
mov ax, TSS_Selector
ltr ax

;set up IDT for standard intel exceptions
push es
mov esi, IDTOffset
xor eax, eax
mov ax, cs
mov es, ax
mov edi, IDivZero
mov bl, 0x0F	;trap gate (Present flag is set by CreateIA32IDTEntry)
xor cx, cx
call CreateIA32IDTEntry
mov edi, IDebug
xor cx, cx
call CreateIA32IDTEntry
mov edi, INMI
mov bl, 0x0E	;interrupt gate
xor cx, cx
call CreateIA32IDTEntry
mov edi, IBreakPoint
mov bl, 0x0F	;trap gate
xor cx, cx
call CreateIA32IDTEntry
mov edi, IOverflow
mov bl, 0x0F	;trap gate
xor cx, cx
call CreateIA32IDTEntry
mov edi, IBoundRange
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IOpcodeInval
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IDevNotAvail
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IDoubleFault
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IReserved
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IInvalidTSS
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, ISegNotPresent
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IStackSegFault
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IGPF
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IPageFault
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IReserved
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IFloatingPointExcept
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IAlignmentCheck
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IMachineCheck
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, ISIMDFPExcept
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IVirtExcept
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
;next 8 interrupts are all reserved
mov dx, 9
_idt_reserv_loop:
mov edi, IReserved
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
dec dx
test dx, dx	;got to 0 yet?
jnz _idt_reserv_loop
mov edi, ISecurityExcept
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
mov edi, IReserved
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
;That is the end of the system exceptions part. Hardware interrupts are configured in the 8259pic driver.
;Ensure that all other interrupts are initialised to point to the "ignored" handler
mov dx, 0xE0
_idt_reserv_loop_two:
mov edi, IReserved
mov bl, 0x0E
xor cx, cx
mov cl, 3			;allow ring3
call CreateIA32IDTEntry
dec dx
test dx, dx	;got to 0 yet?
jnz _idt_reserv_loop_two


.idt_setup:
;Configure IDT pointer
mov word [IDTPtr],  IDTSize	;IDT length
mov dword [IDTPtr+2], IDTOffset	;IDT offset
lidt [IDTPtr]

pop es

; Now check for multiboot2 information.  We already pushed the values of eax and ebx, so we can just pass the addresses to C
extern init_multiboot_system
call init_multiboot_system
add esp, 8	;remove the two parameters we pushed earlier
; if eax contains 1 then multiboot2 info was valid and processed, if 0 then not valid
; if it's been processed, then the memory manager has been initialised and acpi init started
test eax, eax
jnz .post_mbt

; no valid multiboot2 info, so we must set up the memory manager ourselves
extern pci_preinit
mov eax, TemporaryPciInfoLocation
push eax
call pci_preinit
add esp, 4


extern initialise_mmgr

mov eax, TemporaryMemInfoLocation  ;location of the memory map passed by the bootloader
push eax
call initialise_mmgr  ;initialise memory management
add esp, 4

extern load_acpi_data
call load_acpi_data

.post_mbt:
extern setup_pic
call setup_pic

extern initialise_scheduler
call initialise_scheduler

;need to reprogram the PIC before enabling interrupts or a double-fault happens
;specifically, the timer needs somewhere to go
sti

;set up the real-time clock tick counter
extern cmos_init_rtc_interrupt
call cmos_init_rtc_interrupt

extern ps2_initialise
call ps2_initialise

extern initialise_ata_driver
call initialise_ata_driver

extern init_native_api
call init_native_api

extern mount_root_device
call mount_root_device

;make sure that there are no left-over memory allocations
extern validate_kernel_memory_allocations
xor eax,eax
push ax
call validate_kernel_memory_allocations
pop ax

extern scheduler_tick
extern enter_next_process

idle_loop:

call scheduler_tick	;check if we have any work to do
call enter_next_process	;check if there is another process we need to go to
sti
hlt									;pause processor until an interrupt comes along. We will be regularly woken by the timer interrupt.
jmp idle_loop

section .rodata
HelloString: db 'Hello world', 0x0a, 0x0d, 0
