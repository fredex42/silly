; simple low-level routines for talking to the ATA controllers


%define ATA_PRIMARY_BASE    0x1F0
%define ATA_SECONDARY_BASE  0x170
%define ATA_TERTIARY_BASE   0x1E8
%define ATA_QUATERNARY_BASE 0x168

%define ATA_DRIVE_HEAD      0x05
%define ATA_SECTOR_COUNT    0x02
;%define ATA_DATA_REG(base_addr) base_addr+0
;%define ATA_ERROR_REG(base_addr) base_addr+1      //only when reading
;%define ATA_FEATURES_REG(base_addr) base_addr+1   //only when writing
;%define ATA_SECTOR_COUNT(base_addr) base_addr+2
;%define ATA_LBA_LOW(base_addr) base_addr+3
;%define ATA_LBA_MID(base_addr) base_addr+4
;%define ATA_LBA_HI(base_addr) base_addr+5
;%define ATA_DRIVE_HEAD(base_addr) base_addr+6
;%define ATA_STATUS(base_addr) base_addr+7         //only when reading
;%define ATA_COMMAND(base_addr) base_addr+7        //only when writing

%define ATA_ALTSTATUS(base_addr) base_addr+0x206

%define ATA_CMD_IDENTIFY      0xEC
%define ATA_CMD_READ_SECTORS  0x20
%define ATA_CMD_WRITE_SECTORS 0x30
%define ATA_CMD_CACHE_FLUSH   0xE7

%define ATA_SELECT_MASTER   0xA0
%define ATA_SELECT_SLAVE    0xB0

%define ATA_SELECT_MASTER_READ  0xE0
%define ATA_SELECT_SLAVE_READ   0xF0

; Macros to disassamble an LBA28 address from a uint32_t or uint64_t
%define LBA28_HI(value) (uint8_t) ( (value >> 24) & 0xF) ;uppermost 4 bits of the lba28 address
%define LBA28_HMID(value) (uint8_t) (value>>16)
%define LBA28_LMID(value) (uint8_t) (value>>8 )
%define LBA28_LO(value) (uint8_t) (value)

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

;Purpose: initiate a read from the ATA disk. You can use ata_poll_status to synchronously wait for the load to complete
; Call with these args on the stack (C-style):
;  1. Base port as above (primary, secondary, tertiaty, quaternary) (WORD)
;  2. 0xE0 for master, 0xF0 for slave (BYTE)
;  3. LBA28 address to start read at (DWORD)
;  4. Sector count to read (BYTE)
ata_start_read:
  push ebp
  mov ebp, esp

;   outb(ATA_DRIVE_HEAD(base_addr), selector | LBA28_HI(lba_address)); ;NOTE - `selector` is either ATA_SELECT_MASTER_READ or ATA_SELECT_SLAVE_READ depending on the drive we want
  mov dx, word [ebp+8]  ;ebp+8 is arg 1
  add dx, ATA_DRIVE_HEAD
  mov eax, dword [ebp+16] ;ebp+16 is arg 3
  shr eax, 24
  and eax, 0x0F
  or al, byte [ebp+12]  ;ebp+12 is arg 2
  out dx, al          ; all of the I/O ports are addressed as 8-bit

;   outb(ATA_SECTOR_COUNT(base_addr), sector_count);
  mov al, byte [ebp+20] ;ebp+20 is arg 4
  mov dx, word [ebp+8]
  add dx, ATA_SECTOR_COUNT
  out dx, al

;   outb(ATA_LBA_LOW(base_addr), LBA28_LO(lba_address));
  mov eax, dword [ebp+16] ;ebp+16 is arg 3
  and eax, 0x0F
  
  pop ebp
  ret

;   //request an LBA28 read


;   outb(ATA_LBA_MID(base_addr), LBA28_LMID(lba_address));
;   outb(ATA_LBA_HI(base_addr), LBA28_HMID(lba_address));
;   outb(ATA_COMMAND(base_addr), ATA_CMD_READ_SECTORS);