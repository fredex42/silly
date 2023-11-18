#include <types.h>
#include <stdio.h>
#include <sys/mmgr.h>
#include <sys/pagefault.h>

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

uint32_t c_except_pagefault(uint32_t pf_load_addr, uint32_t error_code, uint32_t faulting_addr, uint32_t faulting_codeseg, uint32_t eflags)
{
  uint8_t rc = handle_allocation_fault(pf_load_addr, error_code, faulting_addr, faulting_codeseg, eflags);
  if(rc==0) return 0; //page fault was handled!
  
  kprintf("ERROR: Page fault, occurring at 0x%x:0x%x\r\n", faulting_codeseg, faulting_addr);
  kprintf("Page fault loading address was 0x%x\r\n", pf_load_addr);
  if(error_code&PAGEFAULT_ERR_PRESENT) { kputs("Page-protection violation."); } else { kputs("Page was not present,."); }
  if(error_code&PAGEFAULT_ERR_USER) { kputs("Page fault caused by process."); } else { kputs("Page fault caused by kernel."); }
  if(error_code&PAGEFAULT_ERR_INSFETCH) { kputs("Page fault caused by instruction fetch."); }
  if(error_code&PAGEFAULT_ERR_WRITE) { kputs("Page fault caused by write access"); } else { kputs("Page fault caused by read access"); }

  return 1;
}
