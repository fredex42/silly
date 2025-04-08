CFLAGS=-fPIC -fno-stack-protector -m32 -Iinclude -march=i486 -fno-asynchronous-unwind-tables 
LDFLAGS=-Ttext 0x7E00 -static -z nostartfiles -z nodefaultlib -nostdlib -z nolibc -m elf_i386 --oformat elf32-i386

.PHONY: mmgr utils drivers 8259pic

all: andy.sys kernel.map

clean:
	rm -f andy.sys *.o
	make -C mmgr clean
	make -C utils clean
	make -C drivers clean
	make -C 8259pic clean

mmgr:
	make -C mmgr

utils:
	make -C utils

drivers:
	make -C drivers

8259pic:
	make -C 8259pic

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
memops.o: memops.asm include/memops.h
	nasm -f elf32 memops.asm

main.o: main.c include/sys/idt.h
	gcc -c ${CFLAGS} main.c

andy.sys: cfuncs.o cpuid.o exceptions.o basic_console.o main.o c_exceptions.o stdio_funcs.o memops.o mmgr utils drivers 8259pic
	ld  ${LDFLAGS} -b elf32-i386 main.o memops.o cfuncs.o cpuid.o exceptions.o basic_console.o stdio_funcs.o c_exceptions.o  \
	-Lmmgr -lmmgr \
	-Lutils -lutils \
	-Ldrivers/early_serial -learly_serial \
	-L8259pic -l8259pic \
	-o andy.sys

kernel.map: andy.sys
	./gen_bochs_map.sh