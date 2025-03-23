CFLAGS=-fno-pie -m32 -Iinclude -march=i486 -fno-asynchronous-unwind-tables 
LDFLAGS=-Ttext 0x7E00 -static -z nostartfiles -z nodefaultlib -nostdlib -z nolibc -m elf_i386 --oformat elf32-i386

all: andy.sys kernel.map

clean:
	rm -f andy.sys *.o

cfuncs.o: cfuncs.s basic_console.inc
	nasm -f elf32 cfuncs.s 
cpuid.o: cpuid.asm
	nasm -f elf32 cpuid.asm
exceptions.o: exceptions.s exceptions.inc
	nasm -f elf32 exceptions.s
basic_console.o: basic_console.s basic_console.inc memlayout.asm
	nasm -f elf32 basic_console.s
c_exceptions.o: c_exceptions.c
	gcc -c ${CFLAGS} c_exceptions.c
stdio_funcs.o: stdio_funcs.c include/cfuncs.h
	gcc -c ${CFLAGS} stdio_funcs.c

main.o: main.c
	gcc -c ${CFLAGS} main.c

andy.sys: cfuncs.o cpuid.o exceptions.o basic_console.o main.o c_exceptions.o stdio_funcs.o
	ld  ${LDFLAGS} -b elf32-i386 main.o cfuncs.o cpuid.o exceptions.o basic_console.o stdio_funcs.o c_exceptions.o  -o andy.sys

kernel.map: andy.sys
	./gen_bochs_map.sh