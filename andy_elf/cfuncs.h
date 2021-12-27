/**
Header file for the bridge routines coded in assembly in cfuncs.s
*/
#ifdef __BUILDING_TESTS
#include <stdint.h>
#else
#include <types.h>
#endif

void kputs(char *string);
void longToString(int32_t number, char *buf, int32_t base);
void kputlen(char *string, uint32_t len);
