#include "include/stdarg.h"
#include "include/types.h"
#include <cfuncs.h>

void kprintf(const char *fmt, ...);
size_t strlen(const char *str);

/**
finds the next '%' token in the string. Ignore any escaped % signs (i.e. double-%)
*/
size_t find_next_token(const char *fmt, size_t start_at)
{
  for(size_t i=start_at; fmt[i]!=0; i++) {
    if(fmt[i]=='%' && fmt[i+1]!='%') return i;
  }
  return -1;
}

/**
very basic printf implementation
*/
void kprintf(const char *fmt, ...)
{
  va_list ap;
  char buf[64];

  size_t fmt_string_length = strlen(fmt);
  va_start(ap, fmt);
  size_t current_position=0;
  size_t next_position;
  int arg_ctr=0;
  size_t safety_limit=1024;

  do {
    if(current_position>=safety_limit) break;
    next_position = find_next_token(fmt, current_position);
    if(next_position==-1) { //we ran out of tokens
      kputs(&fmt[current_position]);
      break;
    } else {
      int32_t value;
      int16_t svalue;
      char cvalue;
      kputlen(&fmt[current_position], next_position-current_position+1);

      char format_specifier = fmt[next_position+1]; //the format should be the character after the %
      switch(format_specifier) {
        case 'd':
          svalue = (int16_t)va_arg(ap, int);  //varargs are upcast to `int` from `short`, according to gcc
          longToString((int32_t)svalue, buf, 10);
          kputs(buf);
          break;
        case 'x':
          value = (int32_t)va_arg(ap, int32_t);
          longToString(value, buf, 16);
          kputs(buf);
          break;
        case 'l':
          value = (int32_t)va_arg(ap, int32_t);
          longToString(value, buf, 10);
          kputs(buf);
          break;
        case 's':
          kputs((const char *)va_arg(ap, char *));
          break;
        case 'c':
          cvalue = (char)va_arg(ap, int);
          kputlen(&cvalue, 2);
          break;
        default:
          break;
      }
      current_position = next_position + 2;
      ++arg_ctr;
    }
  } while(1);

  va_end(ap);
}

/**
return the length of a null-terminated string
*/
size_t strlen(const char *str)
{
  size_t i=0;
  while(str[i]!=0) i++;
  return i;
}

void longToString(uint32_t value, char *buf, uint32_t base)
{
  char *numerals = "0123456789abcdef";
  char temp_buf[64];
  //memset(temp_buf, '0', 64);

  uint32_t idx = 0;

  if(base==0) {
    //can't divide by 0!
    return;
  }

  uint32_t current_value = value;
  while(current_value>=base) {
    uint32_t quotient = current_value / base;
    uint32_t remainder = current_value % base;
    temp_buf[idx] = numerals[remainder];
    current_value = quotient;
    ++idx;
  }
  temp_buf[idx] = numerals[current_value];
  for(uint32_t j=0; j<=idx; j++) {
    buf[j] = temp_buf[idx-j];
  }
  buf[idx+1] = 0;
}