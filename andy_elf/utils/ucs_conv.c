//quick and dirty UTF-16 -> ASCII conversion https://stackoverflow.com/questions/5364977/how-to-convert-utf-16-to-ascii
#include <string.h>
#include <types.h>

/**
Get the length of a zero-terminated UTF-16 buffer
*/
size_t w_strlen(const char *unicode_buffer) {
  size_t i;
  while(1) {
    uint16_t codepoint=(uint16_t)unicode_buffer[i*2];
    if(codepoint==0 || codepoint==0xFFFF) return i;
  }
}

#define VALID_ASCII_MASK  0xFF80  //for a UTF-16 character to be representable in ASCII these bits must be 0

char *unicode_to_ascii(const char *unicode_buffer, size_t length)
{
  char *buf = (char *) malloc(length*sizeof(char));
  memset(buf, 0, length);

  for(size_t i=0;i<length;i++) {
    uint16_t codepoint = (uint16_t)unicode_buffer[i*2];
    if(codepoint==0 || codepoint==0xFFFF) {
      return buf;
    } else if( (codepoint&VALID_ASCII_MASK)==0) {  //we have ASCII
      buf[i] = (uint8_t) codepoint;
    } else {
      return buf; //bail out on invalid character
    }
  }
}
