[BITS 16]
section .text
global _start
global idle_loop			;NOTE this is NOT a function but a label, a place that gets returned to.

; remove this definition to disable serial console (also remove in basic_console.s)
%define SERIAL_CONSOLE 1

extern early_serial_lowlevel_init

global SimpleTSS	;we need to set the current ESP in TSS when we move to ring3

%include "basic_console.inc"
%include "memlayout.inc"
%include "exceptions.inc"

jmp _start

; We want to keep the GDT where we can easily access them with 16-bit offsets
; to make life easier with the assembler / linker, but we need more functionality later.
; SimpleGDT is used to bootstrap protected mode, then later on the GDT is shifted to "FullGDT" which
; includes ring3 code/data selectors and a Task Segment Selector as well.

;basic GDT configuration. Each entry is 8 bytes long
SimpleGDT:
;entry 0: null entry
dd 0
dd 0
;entry 1 (segment 0x08): kernel CS
dw 0xffff	;limit bits 0-15
dw 0x0000	;base bits 0-15
db 0x0000	;base bits 16-23
db 0x9A		;access byte. Set Pr, Privl=0, S=1, Ex=1, DC=0, RW=1, Ac=0
db 0xCF		;limit bits 16-19 [lower], flags [higher]. Set Gr=1 [page addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;entry 2 (segment 0x10): kernel DS. Allow this to span the whole addressable space.
dw 0xffff	;limit bits 0-15
dw 0x0000	;base bits 0-15
db 0x00		;base bits 16-23. Start from 0meg.
db 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
db 0xCf		;limit bits 16-19 [lower], flags [higher]. Set Gr=1 [page addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;entry 3 (segment 0x18): video RAM. See https://wiki.osdev.org/Memory_Map_(x86).
dw 0xFFFF	; limit bits 0-15.
dw 0x0000	; base bits 0-15
db 0x0A		; base bits 16-23. Start from 0x0A0000
db 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
db 0x41		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31


SimpleGDTPtr:
dw 0x20		;length in bytes
dd 0x0		;offset, filled in by code

;assuming that DH is still the cursor row and DL is still the cursor position
_start:
;before we go to PM, retrieve the bios memory map
call detect_memory

;Try to enter protected mode - https://wiki.osdev.org/Protected_Mode
;Disable interrupts
cli

;Disable NMI - https://wiki.osdev.org/Non_Maskable_Interrupt
in AL, 0x70
or al, 0x80
out 0x70, al
in al, 0x71

in al, 0x92	;a20 not enabled, do it now
or al, 2
out 0x92, al
a20_enabled:

;Now set up Global Descriptor Table
mov ax, cs
mov ds, ax
xor eax, eax
mov eax, SimpleGDT		;get the location of the GDT, add the absolute location of our data segment
mov si, SimpleGDTPtr
sub si, 0x7E00
mov [si+2], eax	;set the GDT location into the pointer structure
lgdt [si]

;Get the current cursor position
mov ax, 0x0300
mov bx, 0
int 0x10
;DH is row and DL is column

;now we are ready!
mov eax, cr0
or al, 1
mov cr0, eax	;hello protected mode
jmp 0x08:_pm_start	;Here goes nothing!

;retrieve the BIOS memory map. This must be compiled and run in 16-bit mode, easiest way
;is to call it before the switch to PM.
detect_memory:
	mov ax, TemporaryMemInfoBufferSeg  ;target location is 0x250:000
	mov es, ax
	mov di, 2
	xor si, si	;use si as a counter
	xor ebx,ebx
	mem_det_loop:
	mov edx, 0x534d4150
	mov eax, 0xe820
  mov ecx, 24

	inc si
	int 0x15
	jc mem_det_done
	test ebx, ebx
	jz mem_det_done

	add di, 24
	jmp mem_det_loop

	mem_det_done:
	mov word [es:000], si
	ret


[BITS 32]
_pm_start:
;Now we need to re-set up the selectors as what we got from the bootloader was fairly minimal.
mov ax, 0x10
mov ds, ax
mov es, ax
mov ss, ax
mov esp, 0x7fff0	;set stack pointer to the end of conventional RAM

;set up serial debugging output. Vital if SERIAL_CONSOLE is defined in basic_console.inc
call early_serial_lowlevel_init

mov byte [CursorColPtr], dl
mov byte [CursorRowPtr], dh

mov esi, HelloString
call PMPrintString

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


;Configure IDT pointer
mov word [IDTPtr],  IDTSize	;IDT length
mov dword [IDTPtr+2], IDTOffset	;IDT offset
lidt [IDTPtr]

pop es

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

;Now set up a proper GDT.  We copy over the whole of SimpleGDT using movsd
;and then add in sections for TSS, user-mode CS and user-mode DS.
;This is done so we can protect the kernel code from tampering.
cld										;clear direction flag, we want to write forwards
mov edi, FullGDT			;movsd destination
mov esi, SimpleGDT		;movsd source
mov ecx, 8
rep movsd

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

;And tell the processor the correct TSS to use
mov ax, TSS_Selector
ltr ax

extern initialise_mmgr

mov eax, TemporaryMemInfoLocation  ;location of the memory map passed by the bootloader
push eax
call initialise_mmgr  ;initialise memory management
add esp, 4

;extern load_acpi_data
;call load_acpi_data

extern setup_pic
call setup_pic

extern initialise_scheduler
call initialise_scheduler

;need to reprogram the PIC before enabling interrupts or a double-fault happens
;specifically, the timer needs somewhere to go
sti

extern initialise_ata_driver
call initialise_ata_driver

extern init_native_api
call init_native_api

extern mount_root_device
call mount_root_device

extern scheduler_tick
extern enter_next_process

idle_loop:
sti
call scheduler_tick	;check if we have any work to do
call enter_next_process	;check if there is another process we need to go to
hlt									;pause processor until an interrupt comes along. We will be regularly woken by the timer interrupt.
jmp idle_loop

section .rodata
HelloString: db 'Hello world', 0x0a, 0x0d, 0
