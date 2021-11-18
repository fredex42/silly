[BITS 32]
global scan_memory_for_acpi

;Purpose: attempt to find the ACPI RSDP structure in the main BIOS area
;Expects: Nothing
;Returns: A pointer to the RSDP structure or NULL if not found.
scan_memory_for_acpi:
  push esi

  ;start the search at 0xE0000 and run until 0xFFFFF
  mov esi, 0x7FFF0

  acpi_scan_next:
  cmp esi, 0xFFFF0
  jge acpi_scan_not_found

  add esi, 16
  cmp [esi], byte 'R'
  jnz acpi_scan_next
  cmp [esi+1], byte 'S'
  jnz acpi_scan_next
  cmp [esi+2], byte 'D'
  jnz acpi_scan_next
  jnz acpi_scan_next
  cmp [esi+3], byte ' '
  jnz acpi_scan_next
  cmp [esi+4], byte 'P'
  jnz acpi_scan_next
  cmp [esi+5], byte 'T'
  jnz acpi_scan_next
  cmp [esi+6], byte 'R'
  jnz acpi_scan_next
  cmp [esi+7], byte ' '
  jnz acpi_scan_next

  ;if we get here we found it!
  mov eax, esi  ;return the address we found
  pop esi
  ret

  acpi_scan_not_found:
  ;if we got here we ran out of places to look
  xor eax, eax
  pop esi
  ret
