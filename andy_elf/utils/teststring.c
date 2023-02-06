#include <stdio.h>
#include <stdint.h>

int main() {
  int32_t result = strncmp("HELLO","HELLO",5);
  printf("Compare HELLO with itself gave %d\n", result);
  result = strncmp("HELLO","YOU", 3);
  printf("Compare HELLO with YOU gave %d\n", result);
  result = strncmp("HELLO", "HALLO", 5);
  printf("Compare HELLO with HALLO gave %d\n", result);

  char test[16];
  strncpy(test, "hello", 4);
  printf("Copy 'hello' to test gave '%s'\n", test);
}
