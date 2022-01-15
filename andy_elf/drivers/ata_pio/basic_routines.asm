; simple low-level routines for talking to the ATA controllers

;Purpose: polls the status port until BSY clears
;Arguments: base address of the controller to target (uint16_t)
ata_poll_status:
  push ebp
  mov ebp, esp

  push edx

  xor edx,edx
  mov dx, word [ebp+8] ;first argument - base address of the controller
  add dx, 07           ; status port is (base_port + 7)
  _get_ata_status:
  in al, dx

  and al, 0x80          ;test for bit 7 set
  jnz _get_ata_status   ;if not zero (bit 7 still set) then loop

  pop edx
  pop ebp
  ret
