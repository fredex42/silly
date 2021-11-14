#include <types.h>
#include <stdio.h>

/**
higher-level exception handlers
*/

/**
On a divide by zero error, identify where the fault occurred.
*/
void c_except_div0(int32_t faulting_addr, int32_t faulting_codeseg, int32_t eflags)
{
  kprintf("ERROR: Divide by zero, occurring at 0x%x:0x%x\r\n", faulting_codeseg, faulting_addr);
}
