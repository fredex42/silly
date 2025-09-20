#ifndef _DEBUG_H
#define _DEBUG_H


void dump_stack_32();

#if __WORDSIZE == 32
#define dump_stack() dump_stack_32()
#else
#error "No dump_stack implementation for this architecture"
#endif

#endif