#include <types.h>
#include <stdio.h>

/*
Pagefault error code bitfield
*/
#define PAGEFAULT_ERR_PRESENT 1<< 0 // 	When set, the page fault was caused by a page-protection violation. When not set, it was caused by a non-present page.
#define PAGEFAULT_ERR_WRITE   1<< 1 //When set, the page fault was caused by a write access. When not set, it was caused by a read access.
#define PAGEFAULT_ERR_USER    1<< 2 // When set, the page fault was caused while CPL = 3. This does not necessarily mean that the page fault was a privilege violation.
#define PAGEFAULT_ERR_RESERVEDWRITE 1<< 3 // 	When set, one or more page directory entries contain reserved bits which are set to 1. This only applies when the PSE or PAE flags in CR4 are set to 1.
#define PAGEFAULT_ERR_INSFETCH 1<< 4 //When set, the page fault was caused by an instruction fetch. This only applies when the No-Execute bit is supported and enabled.
#define PAGEFAULT_ERR_PKEY 1<< 5    // 	When set, the page fault was caused by a protection-key violation. The PKRU register (for user-mode accesses) or PKRS MSR (for supervisor-mode accesses) specifies the protection key rights.
#define PAGEFAULT_ERR_SS 1<< 6      // When set, the page fault was caused by a shadow stack access.
#define PAGEFAULT_ERR_SGX 1<< 15    //When set, the fault was due to an SGX violaton. The fault is unrelated to ordinary paging.
#define PAGEFAULT_ERR_PRESENT 1<< 0

/**
higher-level exception handlers
*/

/**
On a divide by zero error, identify where the fault occurred.
*/
void c_except_div0(uint32_t faulting_addr, uint32_t faulting_codeseg, uint32_t eflags)
{
  kprintf("ERROR: Divide by zero, occurring at 0x%x:0x%x\r\n", faulting_codeseg, faulting_addr);
}

/**
On a GPF, identify where the fault occurs
*/
void c_except_gpf(uint32_t error_code, uint32_t faulting_addr, uint32_t faulting_codeseg, uint32_t eflags)
{
  kprintf("ERROR: General protection fault, occurring at 0x%x:0x%x. Error code was 0x%x\r\n", faulting_codeseg, faulting_addr, error_code);
}

void c_except_invalidop(uint32_t faulting_addr, uint32_t faulting_codeseg, uint32_t eflags)
{
  kprintf("ERROR: Invalid opcode, occurring at 0x%x:0x%x\r\n", faulting_codeseg, faulting_addr);
}

void c_except_pagefault(uint32_t pf_load_addr, uint32_t error_code, uint32_t faulting_addr, uint32_t faulting_codeseg, uint32_t eflags)
{
  kprintf("ERROR: Page fault, occurring at 0x%x:0x%x\r\n", faulting_codeseg, faulting_addr);
  kprintf("Page fault loading address was 0x%x\r\n", pf_load_addr);
  if(error_code&PAGEFAULT_ERR_PRESENT) { kputs("Page-protection violation."); } else { kputs("Page was not present,."); }
  if(error_code&PAGEFAULT_ERR_USER) { kputs("Page fault caused by process."); } else { kputs("Page fault caused by kernel."); }
  if(error_code&PAGEFAULT_ERR_INSFETCH) { kputs("Page fault caused by instruction fetch."); }
  if(error_code&PAGEFAULT_ERR_WRITE) { kputs("Page fault caused by write access"); } else { kputs("Page fault caused by read access"); }


}
