[BITS 32]

global pic_send_eoi
global pic_get_isr
global pic_get_irr
global pic_get_irq_reg
global mask_irq
global unmask_irq
global disable_lapic

;Purpose: Sends the end-of-interrupt message to the PIC
;Arguments: irq number (uint8_t)
;Returns: nothing
pic_send_eoi:
  push ebp
  mov ebp, esp
  push eax
  xor eax, eax
  cmp byte [ebp+8], 8
  jl _eoi_pic_master

  mov al, 0x20          ;end-of-interrupt commend number
  out 0xA0, al          ;send EOI to slave pic command port

  _eoi_pic_master:
  mov al, 0x20
  out 0x20, al          ;send EOI to the master pic command port

  pop eax
  pop ebp
  ret

;Purpose: Returns a value from both PICS as a uint16_t. The upper 8 bits are the
;value from PIC1 and the lower 8 bits are the value from PIC2.
;Arguments: cmd - the command word to send (uint8_t). 0x0a for IRR, 0x0b for
;ISR
;Returns: the result in eax.
pic_get_irq_reg:
  push ebp
  mov ebp, esp

  xor eax, eax
  mov al, byte [ebp+8]
  out 0x20, al  ;write the provided command byte
  out 0xA0, al

  in al, 0x20
  shl ax,8
  in al, 0xA0
  pop ebp
  ret

;Purpose: retrieve the contents of both PICs irq request register
pic_get_irr:
  xor eax, eax
  mov al, 0x0A
  push eax
  call pic_get_irq_reg
  add esp, 4
  ret

;Purpose: retrieve the contents of both PICs in-service register
pic_get_isr:
  xor eax, eax
  mov al, 0x0B
  push eax
  call pic_get_irq_reg
  add esp, 4
  ret

;Purpose: use model-specific register to disable the local APIC.
;This will cause an Undefined Opcode exception if the msr functions are not supported.
;Use the cpuid functions to detect whether MSRs are actually supported before calling this.
disable_lapic:
  push eax
  push ecx
  push edx

  mov ecx, 0x1B   ;IA32_APIC_BASE_MSR see https://sandpile.org/x86/msr.htm
  rdmsr           ;sets edx to the upper 32 bits and eax to the lower 32 bits
  and eax, 0xFFFFF7FF ;clear bit 11
  wrmsr           ;write the value back again.

  pop edx
  pop ecx
  pop eax
  ret

;Purpose: temporarily disable (mask) a given IRQ line.
;Arguments: (uint8_t) IRQ line to mask
;Returns: The updated mask value
mask_irq:
  push ebp
  mov ebp, esp

  push ebx
  push ecx

  xor eax, eax
  xor ebx, ebx
  xor ecx, ecx

  mov cl, byte [ebp+8]
  cmp cl, 0x8
  jl _mask_secondary_pic

  mov bx, 1
  shl bx, cl    ;create the mask by shifting to the left cl times (1 in the bit place of interrupt nr)

  in al, 0x21   ;PIC1 data port
  or al, bl
  out 0x21, al  ;Output the updated mask

  _mask_secondary_pic:  ;mask the secondary pic, subtract 8 from the irq number and send to port 0xA1
  sub cl, 0x8
  mov bx, 1
  shl bx, cl    ;create the mask by shifting to the left cl times (1 in the bit place of interrupt nr)

  in al, 0xA1   ;PIC2 data port
  or al, bl
  out 0xA1, al  ;Output the updated mask

  pop ecx
  pop ebx
  pop ebp
  ret

;Purpose: re-enable an IRQ that was previously disabled by mask_irq
;Arguments: (uint8_t) IRQ line to mask
;Returns: The updated mask value
unmask_irq:
  push ebp
  mov ebp, esp

  push ebx
  push ecx

  xor eax, eax
  xor ebx, ebx
  xor ecx, ecx

  mov cl, byte [ebp+8]
  cmp cl, 0x8
  jl _unmask_secondary_pic

  mov bx, 1
  shl bx, cl    ;create the mask by shifting to the left cl times (1 in the bit place of interrupt nr)
  xor bx, 0xFF  ;invert the mask by XORING with all 1s

  in al, 0x21   ;PIC1 data port
  and al, bl
  out 0x21, al  ;Output the updated mask

  _unmask_secondary_pic:  ;mask the secondary pic, subtract 8 from the irq number and send to port 0xA1
  sub cl, 0x8
  mov bx, 1
  shl bx, cl    ;create the mask by shifting to the left cl times (1 in the bit place of interrupt nr)
  xor bx, 0xFF  ;invert the mask by XORING with all 1s

  in al, 0xA1   ;PIC2 data port
  and al, bl
  out 0xA1, al  ;Output the updated mask

  pop ecx
  pop ebx
  pop ebp
  ret
