[BITS 32]

%include "../memlayout.asm"  ;for IDTPtr
;function exports
global ISpurious
global ITimer
global IDummy
; global configure_pic_interrupts

;function imports
extern pic_send_eoi
extern pic_get_irr
extern pic_get_isr
extern PMPrintChar
extern IKeyboard
; extern CreateIA32IDTEntry
extern ICmosRTC

; ;Registers our interrupt handlers in the IDT
; configure_pic_interrupts:
;   push ebp
;   mov ebp, esp
;   push eax
;   push ebx
;   push ecx
;   push edx
;   push esi
;   push edi
;   push es

;   mov ax, cs
;   mov es, ax

;   ;Create an IDT (Interrupt Descriptor Table) entry
;   ;The entry is created at ds:esi. esi is incremented to point to the next entry
;   ;destroys values in bl, cx, edi
;   ;edi: offset of handler
;   ;es : GDT selector containing the code
;   ;bl : gate type. 0x05 32-bit task gate, 0x0E 32-bit interrupt gate, 0x0F 32-bit trap gate
;   ;cl : ring level required to call. Only 2 bits are used.

;   mov esi, IDTOffset  ;location of the IDT
;   add esi, 0x100      ;start setting interrupts at 0x20, each entry is 8 bytes long.

;   xor ebx, ebx
;   mov bl, 0x0E
;   mov edi, ITimer
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ0
;   mov edi, IKeyboard
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ1
;   mov edi, IDummy
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ2 - this is the cascade and never raised, so kept as Dummy
;   mov edi, ISerial2
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ3
;   mov edi, ISerial1
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ4
;   mov edi, IParallel2
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ5
;   mov edi, IFloppy1
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ6
;   mov edi, ISpurious
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ7
;   mov edi, ICmosRTC
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ8
;   mov edi, IRQ9
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ9
;   mov edi, IRQ10
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ10
;   mov edi, IRQ11
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ11
;   mov edi, IMouse
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ12
;   mov edi, IFPU
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ13
;   mov edi, IPrimaryATA
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ14
;   mov edi, ISecondaryATA
;   xor ecx, ecx
;   call CreateIA32IDTEntry ;IRQ15

;   pop es
;   pop edi
;   pop esi
;   pop edx
;   pop ecx
;   pop ebx
;   pop eax
;   pop ebp
;   ret

ITimer:     ;timer interrupt handler
  pushf
  push bx
  xor bx, bx

  mov ebx, 0
  push ebx
	call pic_send_eoi
  add esp, 4

  pop bx
  popf
  iret

ISerial2:             ;IRQ3 serial port 2 interrupt handler. Just sends EOI at present
  pushf
  push bx
  mov ebx, 3
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

ISerial1:             ;IRQ4 serial port 1 interrupt handler. Just sends EOI at present
  pushf
  push bx
  mov ebx, 4
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IParallel2:             ;IRQ5 parallel port 2 interrupt handler. Just sends EOI at present
  pushf
  push bx
  mov ebx, 5
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IFloppy1:             ;IRQ6 floppy disk 1 interrupt handler. Just sends EOI at present
  pushf
  push bx
  mov ebx, 6
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

ISpurious:  ;IRQ7 spurious interrupt handler (also parallel port 1)
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

IRQ9:             ;IRQ9
  pushf
  push bx
  mov ebx, 9
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IRQ10:             ;IRQ10
  pushf
  push bx
  mov ebx, 10
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IRQ11:             ;IRQ11
  pushf
  push bx
  mov ebx, 11
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IMouse:             ;IRQ12 PS/2 mouse
  pushf
  push bx
  mov ebx, 12
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

IFPU:             ;IRQ13 FPU/Coprocessor/inter-processor
  pushf
  push bx
  mov ebx, 13
  push ebx
  call pic_send_eoi
  add esp, 4
  pop bx
  popf
  iret

extern ata_service_interrupt  ;this is defined in the ata_pio driver for servicing IDE interrupts

IPrimaryATA:             ;IRQ14 Primary IDE
  pushf
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ebp
  
  mov ebx, 0
  push bx
  ;call ata_service_interrupt  ;tell the ATA driver that this cane from bus 0 (primary)
  add esp, 2

  mov ebx, 14
  push ebx
  call pic_send_eoi
  add esp, 4

  pop ebp
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  popf
  iret

ISecondaryATA:             ;IRQ15 Secondary IDE
  pushf
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ebp

  mov ebx, 1
  push bx
  ;call ata_service_interrupt ;tell the ATA driver that this cane from bus 1 (secondary)
  add esp, 2

  mov ebx, 15
  push ebx
  call pic_send_eoi
  add esp, 4

  pop ebp
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  pop eax
  popf
  iret

IDummy:   ;generic dummy interrupt handler
  mov ebx, 2
  push ebx
  call pic_send_eoi
  add esp, 4
  iret
