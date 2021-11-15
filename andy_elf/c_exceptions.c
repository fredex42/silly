#include <types.h>
#include <stdio.h>

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
