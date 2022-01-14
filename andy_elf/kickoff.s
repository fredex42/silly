[BITS 16]
section .text
global _start

%include "basic_console.inc"
%include "memlayout.inc"
%include "exceptions.inc"

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
jmp 0x08:_pm_start	;Here goes nothing! break 0x00007cb4

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
;That is the end of the system exceptions part. Next is what we will configure the PIC to write IRQs to
;IRQ0 - system timer
mov edi, IReserved
mov bl,0x0F
xor cx,cx
call CreateIA32IDTEntry
;IRQ1 - keyboard
mov edi, IKeyboard
mov bl,0x0F
xor cx,cx
call CreateIA32IDTEntry
mov dx, 14
_irq_reserv_loop:
mov edi, IReserved
mov bl, 0x0F
xor cx, cx
call CreateIA32IDTEntry
dec dx
test dx, dx	;got to 0 yet?
jnz _irq_reserv_loop


;Configure IDT pointer
mov word [IDTPtr],  IDTSize	;IDT length
mov dword [IDTPtr+2], IDTOffset	;IDT offset
lidt [IDTPtr]

pop es

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

; extern run_inkernel_memory_tests
; call run_inkernel_memory_tests

extern test_read
call test_read

extern scheduler_tick

idle_loop:
sti
call scheduler_tick	;check if we have any work to do
hlt									;pause processor until an interrupt comes along. We will be regularly woken by the timer interrupt.
jmp idle_loop

section .data
HelloString: db 'Hello world', 0x0a, 0x0d, 0

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
;entry 4 (segment 0x20): TSS. See https://wiki.osdev.org/Task_State_Segment.
dw 0x68		;limit bits 0-15
dw 0xff00	;base bits 0-15.
db 0x10		;base bits 16-23. Start from end of kernel DS
db 0x84		;access byte. Set Pr, Privl=0, S=0, Ex=0, DC=0, RW=1,Ac=0
db 0x40		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31

SimpleGDTPtr:
dw 0x28		;length in bytes
dd 0x0		;offset, filled in by code
