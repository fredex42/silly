[BITS 32]

global pic_send_eoi

;Purpose: Sends the end-of-interrupt message to the PIC
;Arguments: irq number (uint8_t)
;Returns: nothing
pic_send_eoi:
  push ebp
  mov ebp, esp
  push eax
  xor eax, eax
  cmp byte [ebp+4], 8
  jl _eoi_pic_master

  mov al, 0x20          ;end-of-interrupt commend number
  out 0xA0, al          ;send EOI to slave pic command port
  _eoi_pic_master:
  mov al, 0x20
  out 0x20, al          ;send EOI to the master pic command port

  pop eax
  pop ebp
  ret
