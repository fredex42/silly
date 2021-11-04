/* test an entrypoint for a C function */
#include "cfuncs.h"

void test_c_entrypoint(){
	char buf[12];
	long number = 1234567;

	kputs("Hello from C\r\n");
	longToString(number, &buf);
	kputs(buf);

}
