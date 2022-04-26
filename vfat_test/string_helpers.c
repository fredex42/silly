#include "string_helpers.h"

/**
Return the index of the first occurrence of the character c in the string buf.
Returns -1 if the character is not present.
*/
size_t find_in_string(const char *buf, char c)
{
  for(size_t i=0;i<strlen(buf);i++) {
    if(buf[i]==c) return i;
  }
  return -1;
}
