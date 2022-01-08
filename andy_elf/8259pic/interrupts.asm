[BITS 32]

%include "../memlayout.inc"  ;for IDTPtr
;function exports
global IKeyboard
global ISpurious
global ITimer
global IDummy
global configure_pic_interrupts

;function imports
extern pic_send_eoi
extern pic_get_irr
extern pic_get_isr
extern PMPrintChar
extern CreateIA32IDTEntry

;Registers our interrupt handlers in the IDT
configure_pic_interrupts:
  push ebp
  mov ebp, esp
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push es

  mov ax, cs
  mov es, ax

  ;Create an IDT (Interrupt Descriptor Table) entry
  ;The entry is created at ds:esi. esi is incremented to point to the next entry
  ;destroys values in bl, cx, edi
  ;edi: offset of handler
  ;es : GDT selector containing the code
  ;bl : gate type. 0x05 32-bit task gate, 0x0E 32-bit interrupt gate, 0x0F 32-bit trap gate
  ;cl : ring level required to call. Only 2 bits are used.

  mov esi, IDTOffset  ;location of the IDT
  add esi, 0x100      ;start setting interrupts at 0x20, each entry is 8 bytes long.

  xor ebx, ebx
  mov bl, 0x0E
  mov edi, ITimer
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ0
  mov edi, IKeyboard
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ1
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ2
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ3
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ4
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ5
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ6
  mov edi, ISpurious
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ7
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ8
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ9
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ10
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ11
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ12
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ13
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ14
  mov edi, IDummy
  xor ecx, ecx
  call CreateIA32IDTEntry ;IRQ15

  pop es
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  pop ebp
  ret

ITimer:     ;timer interrupt handler
  pushf
  push bx
  xor bx, bx
  mov bl, '+'
  call PMPrintChar

  mov ebx, 0
  push ebx
	call pic_send_eoi
  add esp, 4

  pop bx
  popf
  iret

IKeyboard:	;keyboard interrupt handler
	pushf
	push bx
	xor bx, bx
	mov bl, '.'
	call PMPrintChar

  mov ebx, 1
  push ebx
	call pic_send_eoi
  add esp, 4

	pop bx
	popf
	iret

ISpurious:  ;spurious interrupt handler
  pushf
  push ax

  call pic_get_isr
  shr ax, 8       ;upper 8 bits are PIC1
  and ax, 0x40    ;check bit 7
  jz no_eoi_reqd  ;if result is 0 then this is a suprious IRQ and no EOI is required

  mov ax, 7
  push ax
  call pic_send_eoi
  add esp, 4

  no_eoi_reqd:
  pop ax
  popf
  iret

IDummy:   ;generic dummy interrupt handler
  iret
