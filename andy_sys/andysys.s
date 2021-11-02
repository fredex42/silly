%include "memlayout.inc"
[BITS 16]
;Source code of andy.sys. This file is called by the bootloader.
;Some parts are split into other files which are %include'd into here

mov si, HelloString
call BiosPrintString

;Try to enter protected mode - https://wiki.osdev.org/Protected_Mode
;Disable interrupts
cli
;Disable NMI - https://wiki.osdev.org/Non_Maskable_Interrupt
in AL, 0x70
or al, 0x80
out 0x70, al
in al, 0x71
mov si, TestA20
call BiosPrintString
;Enable the a20 line if needed
call check_a20
test ax,1
jz a20_enabled

in al, 0x92	;a20 not enabled, do it now
or al, 2
out 0x92, al
a20_enabled:
mov si, A20En
call BiosPrintString

;Now set up Global Descriptor Table
mov ax,cs
mov ds,ax
mov es,ax
xor ax, ax
mov ax, SimpleGDT		;get the location of the GDT
add ax, 0x7E00			;add our base address at the start of conventional RAM
mov [SimpleGDTPtr+2], ax	;set the GDT location into the pointer structure
lgdt [SimpleGDTPtr]

mov si, GdtDone
call BiosPrintString

;Get the current cursor position
mov ax, 0x0300
mov bx, 0
int 0x10
;DH is row and DL is column

;now we are ready!
mov eax, cr0
or al, 1
mov cr0, eax	;hello protected mode
jmp 0x08:PModeMain+0x7E00

; Function: check_a20
;
; Purpose: to check the status of the a20 line in a completely self-contained state-preserving way.
;          The function can be modified as necessary by removing push's at the beginning and their
;          respective pop's at the end if complete self-containment is not required.
;
; Returns: 0 in ax if the a20 line is disabled (memory wraps around)
;          1 in ax if the a20 line is enabled (memory does not wrap around)
 
check_a20:
    xor ax, ax ; ax = 0
    mov es, ax
 
    not ax ; ax = 0xFFFF
    mov ds, ax
 
    mov di, 0x0500
    mov si, 0x0510
 
    mov al, byte [es:di]
    push ax
 
    mov al, byte [ds:si]
    push ax
 
    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
 
    cmp byte [es:di], 0xFF
 
    pop ax
    mov byte [ds:si], al
 
    pop ax
    mov byte [es:di], al
 
    mov ax, 0
    je check_a20__exit
    mov ax, 1
 
check_a20__exit:
    ret

; uses BIOS int10 to output the character in AL to the screen
BiosPrintCharacter:
	mov ah, 0x0E
	mov bh, 0x00
	mov bl, 0x07
	int 0x10
	ret

; repeatedly calls PrintCharacter to output the ASCIIZ string pointed to by the SI register
BiosPrintString:
	next_char:
	mov al, [cs:si]
	inc si
	or al, al	;sets zero flag if al is 0 => end of string
	jz exit_function
	call BiosPrintCharacter
	jmp next_char
	exit_function:
	ret

;Data
StringTableStart:
HelloString db 'ANDY.SYS v1', 0x0a,0x0d,0
TestA20 db 'TestingA20', 0x0a,0x0d,0
A20En db 'A20 enabled.', 0x0a,0x0d,0
GdtDone db 'GDT ready.', 0x0a, 0x0d, 0
HelloTwo db 'Hello Protected Mode!', 0
;all these errors are fatal if the occur in the kernel. see https://wiki.osdev.org/Exceptions
DivZeroMsg db 'FATAL: Divide by zero', 0
DebugMsg db 'FATAL: Debug trap', 0
NMIMsg db 'FATAL: Non-maskable interrupt occurred', 0
BreakPointMsg db 'FATAL: Hardware breakpoint', 0
OverflowMsg db 'FATAL: An overflow error occurred', 0
BoundRangeMsg db 'FATAL: Bound range exceeded', 0
InvalidOpcodeMsg db 'FATAL: Invalid opcode', 0
DevNotAvailMsg db 'FATAL: Device not availalbe', 0
DoubleFaultMsg db 'FATAL: Double fault', 0	;leaves error code on top of stack
InvalidTSSMsg db 'FATAL: Invalid TSS', 0	;leaves error code
SegmentNotPresentMsg db 'FATAL: Segment not present', 0	;leaves error code
StackSegmentFaultMsg db 'FATAL: Stack segment fault', 0	;leaves error code
GPFMsg db 'FATAL: General protection fault', 0		;leaves error code
PageFaultMsg db 'FATAL: Page fault', 0			;leaves error code
FPExceptMsg db 'FATAL: Floating-point Exception', 0
AlignCheckMsg db 'FATAL: Alignment check failed', 0	;leaves error code
MachineCheckMsg db 'FATAL: Machine check failed', 0
SIMFPExceptMsg db 'FATAL: SIMD floating-point exception', 0
VirtExceptMsg db 'FATAL: Virtualization exception', 0
SecExceptMsg db 'FATAL: Security exception', 0		;leaves error code
StringTableEnd:

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
db 0x4f		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
db 0x00		;base bits 24-31
;entry 2 (segment 0x10): kernel DS
dw 0xff00	;limit bits 0-15
dw 0x0000	;base bits 0-15
db 0x10		;base bits 16-23. Start from 1meg.
db 0x92		;access byte. Set Pr, Privl=0, S=1, Ex=0, DC=0, RW=1, Ac=0
db 0x4f		;limit bits 16-19 [lower], flags [higher]. Set Gr=0 [byte addressing], Sz=1 [32-bit sector]
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



[BITS 32]
;we jump here once the switch to protected mode is complete, and we can use 32-bit code.
PModeMain:
;assuming that DH is still the cursor row and DL is still the cursor position

mov ax, 0x10
mov ds, ax	;set up segments to point to 32-bit data seg
mov es, ax
mov ss, ax
mov esp, 0x00fff00	;stack at the end of RAM data segment

mov byte [CursorColPtr], dl
mov byte [CursorRowPtr], dh

;Now we want to copy our strings from the code segment into the data segment
push ds
cld
mov ax, cs
mov ds, ax	;set ds=cs. es is already set up
mov eax, StringTableEnd
sub eax, StringTableStart
xor edx, edx
mov ecx, 0x04				;for clock-efficiency move 32bits at once. Therefore divide the byte count by 4.
div ecx
mov ecx, eax
mov esi, StringTableStart+0x7E00	;we are loaded at an offset of 0x7E00 by the bootloader
mov edi, StringTableOffset
rep movsd	;move from ds:esi to es:edi, cx times
pop ds

mov eax, HelloTwo
sub eax, StringTableStart
add eax, StringTableOffset	;we are actually accessing the string in the data-segment copy. So we need to adjust the offset here.
mov esi, eax
call PMPrintString
call PMScrollConsole
;call PMScrollConsole

;set up TSS

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
mov word [IDTPtr],  0x800	;IDT length
mov dword [IDTPtr+2], IDTOffset+0x100000	;IDT offset
lidt [IDTPtr]

pop es

;configure the PIC to send IRQs to IDT 0x20+
;using legacy 8259 PIC for time being
%define PIC1	0x10	;IO base address for master PIC
%define PIC2 	0xA0	;IO base address
%define PIC1_CMD	PIC1	;command port for PIC1
%define PIC1_DATA	PIC1+1	;data port for PIC1
%define PIC2_CMD	PIC2
%define PIC2_DATA	PIC2+1
%define ICW1_ICW4	0x01	;ICW4 not needed
%define ICW1_SINGLE	0x02	;Single mode (i.e. cascade)
%define ICW1_INTERVAL4	0x04	;Call address interval 4
%define ICW1_LEVEL	0x08	;Level triggered (edge) mode
%define ICW1_INIT	0x10	;initialization command - required
%define ICW4_8086	0x01	;8086/88 mode
%define ICW4_AUTO	0x02	;Auto end-of-interrupt
%define ICW4_BUF_SLAVE	0x08	;buffered mode slave
%define ICW4_BUF_MASTER	0x0c	;buffered mode master
%define ICW4_SFNM	0x10	;'special fully nested'
%define PIC1_OFFSET 	0x20	;software int offset for PIC1
%define PIC2_OFFSET	0x28	;software int offset for PIC2

%define IRQ_SYSTEM_TIMER	PIC1_OFFSET+0
%define IRQ_KEYBOARD		PIC1_OFFSET+1
%define IRQ_CASCADE		PIC1_OFFSET+2
%define IRQ_SERIAL2		PIC1_OFFSET+3
%define IRQ_SERIAL1		PIC1_OFFSET+4
%define IRQ_5			PIC1_OFFSET+5
%define IRQ_FLOPPY		PIC1_OFFSET+6
%define IRQ_PARALLEL1		PIC1_OFFSET+7
%define IRQ_RTC			PIC2_OFFSET+0
%define IRQ_ACPI_MAYBE		PIC2_OFFSET+1
%define IRQ_MOUSE		PIC2_OFFSET+4
%define IRQ_ATA1		PIC2_OFFSET+6
%define IRQ_ATA2		PIC2_OFFSET+7

;save existing mask state to stack as one word (PIC1 MSB, PIC2 LSB)
xor ax, ax
in al, PIC1_DATA
shl ax, 8
in al, PIC2_DATA
push ax

;start 3-byte initialization sequence
xor ax,ax
mov al, ICW1_INIT | ICW1_ICW4
out PIC1_CMD, al
call io_wait
mov al, ICW1_INIT | ICW1_ICW4
out PIC2_CMD, al
call io_wait

mov al, PIC1_OFFSET
out PIC1_DATA, al
call io_wait
mov al, PIC2_OFFSET
out PIC2_DATA, al
call io_wait

mov al, 0x04
out PIC1_DATA, al	;ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
call io_wait
mov al, 0x02
out PIC2_DATA, al	;ICW3: tell Slave PIC its cascade identity (0000 0010)
call io_wait

mov al, ICW4_8086
out PIC1_DATA, al
call io_wait
mov al, ICW4_8086
out PIC2_DATA, al
call io_wait

;restore previous mask state
pop ax
out PIC2_DATA, al
shr ax, 8
and al, 0xFD	;clear bit 2 => enable keyboard interrupt
out PIC1_DATA, al

;PIC setup complete

;trigger the divide-by-zero interrupt to make sure everything is working
mov ax, 5
mov dx, 0
div dx

jmp $

;Wait a very small amount of time (1 to 4 microseconds generally)
;Provided by https://wiki.osdev.org/Inline_Assembly/Examples#I.2FO_access
io_wait:
	mov al, 0
	out 0x80, al
	ret

;send end-of-interrupt to the master PIC ONLY
PIC_Send_EOI:
	push ax
	mov al, 0x20	;end-of-interrupt command code
	out PIC1_CMD, al
	pop ax
	ret

;Print a string in protected mode.
;Expects the string to print in ds:esi
PMPrintString:
	push es
	push edi
	push eax
	push ecx
	push edx
	pushf

	;set up es:edi to point to where we want to write the characters
	mov ax, DisplayMemorySeg
	mov es, ax
	mov edi, TextConsoleOffset

	reset_offset:
	;calculate the correct display offset based on the cursor position
	xor eax, eax
	mov al, byte [CursorRowPtr]	;assume 80 columns per row = 0x50. Double this because each char is 2 bytes (char, attribute)
	mov cx, 0x00a0
	mul cx
	xor dx, dx
	mov dl, byte [CursorColPtr]
	add ax, dx			;AX is the offset
	add ax, dx			;add again to double the offset
	add edi, eax

	pm_next_char:
	lodsb
	test al, al
	jz pm_string_done	;if the next char is 0, then exit
	
	stosb
	mov al, DefaultTextAttribute
	stosb	;only use default attribute at the moment

	jmp pm_next_char

	pm_string_done:
	popf
	pop edx
	pop ecx
	pop eax
	pop edi
	pop es
	ret

;Print a character in protected mode.
;Expects the character to print in bl.
PMPrintChar:
	push es
	push eax
	push dx
	mov ax, DisplayMemorySeg
	mov es, ax
	mov edi, TextConsoleOffset

	xor eax,eax
	mov al, byte [CursorRowPtr]
	mov cx, 0x00a0
	mul cx
	xor dx, dx
	mov dl, byte [CursorColPtr]
	add ax, dx
	add ax, dx
	add edi, eax

	mov al, bl
	stosb
	mov al, DefaultTextAttribute
	stosb

	inc byte [CursorColPtr]
	pop dx
	pop eax
	pop es

;Scrolls the console down by 1 line.
;Destroys AX, CX
PMScrollConsole:
	push es
	push esi
	push ds
	push edi

	mov ax, DisplayMemorySeg
	mov es, ax
	mov ds, ax

	;we want to copy all of the data _up_ one line, and fill the last with 0x20/0x00 (blank space) characters
	;es:edi is destination and ds:esi is source
	mov edi, TextConsoleOffset
	mov esi, TextConsoleOffset
	add esi, 0xA0		;80 characters * 2 bytes = 1 line
	mov ecx, 0x3C0		;80 characters * 24 lines * 2 bytes per char / 4 bytes per dword (into doublewords)
	rep movsd

	mov eax, 0x20
	mov ecx, 0x50		;1 line
	rep stosw

	pop edi
	pop ds
	pop esi
	pop es
	ret

;Create an IDT (Interrupt Descriptor Table) entry
;The entry is created at ds:esi. esi is incremented to point to the next entry
;destroys values in bl, cx, edi
;edi: offset of handler
;es : GDT selector containing the code
;bl : gate type. 0x05 32-bit task gate, 0x0E 32-bit interrupt gate, 0x0F 32-bit trap gate
;cl : ring level required to call. Only 2 bits are used.
CreateIA32IDTEntry:
	add edi, 0x7E00		;add in our own code offset
	mov word [ds:esi], di	;lower 4 bytes of offset
	mov word [ds:esi+2], es	;selector
	mov byte [ds:esi+4], 0x00	;reserved
	;next byte is (LSB to MSB) gate type x3, reserved 0 x1, DPL x2, present 1 x1
	or bl, 0x80	;set present bit
	clc
	xor ch, ch
	shl cl, 5	;DPL value, shift this to the right position then add it on
	or bl, cl
	mov byte [ds:esi+5], bl
	shr edi, 16
	mov word [ds:esi+6], di
	add esi,8
	ret

%include "exceptions.s"
%include "irqs.s"
