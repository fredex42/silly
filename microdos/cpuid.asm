;Purpose: Calls the "get features" function of cpuid and returns
;(as a uint32_t) the extended "ecx" features. See https://wiki.osdev.org/CPUID
global cpuid_ecx_features
cpuid_ecx_features:
  push ebx
  push ecx
  push edx

  mov eax, 0x01 ;"get features"
  cpuid
  mov eax, ecx
  pop edx
  pop ecx
  pop ebx
  ret

;Purpose: Calls the "get features" function of cpuid and returns
;(as a uint32_t) the standard "edx" features. See https://wiki.osdev.org/CPUID
global cpuid_edx_features
cpuid_edx_features:
  push ebx
  push ecx
  push edx

  mov eax, 0x01 ;"get features"
  cpuid
  mov eax, edx
  pop edx
  pop ecx
  pop ebx
  ret

;Purpose: Returns the CPUID vendor string in the provided buffer.
;The buffer must have enough space allocated to accomodate 12 ASCII characters and
;a newline.
global cpuid_get_vendorid
cpuid_get_vendorid:
  push ebp
  mov ebp, esp

  push eax
  push ebx
  push ecx
  push edx

  mov eax, 0x00 ;"get id string"
  cpuid

  mov edi, dword [ebp+4]  ;string pointer to write to
  mov dword [edi], ebx    ;first 4 chars
  mov dword [edi+4], edx  ;next 4 chars
  mov dword [edi+8], ecx  ;last 4 chars
  mov byte [edi+12], 0    ;terminating null

  pop edx
  pop ecx
  pop ebx
  pop eax
  pop ebp
  ret
