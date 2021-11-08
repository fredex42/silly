/* test an entrypoint for a C function */
#include "cfuncs.h"
#include <stdio.h>

void test_c_entrypoint(){
	char buf[12];
	long number = 1234567;

	kputs("Hello from C\r\n");
	longToString(number, &buf, 10);
	kputs(buf);

	kputlen("ABCDEFG",7);

	kprintf("Hello again\r\n");

	kprintf("Hello %x with %d and %l.    ", 0x42a, 162, 8463524L);
}
