.PHONY: acpi

all: test.elf

CFLAGS=-fno-pie -m32 -Iinclude -march=i486
clean:
	rm -f *.o test.elf
	make -C acpi clean

kickoff.o: kickoff.s
	nasm -f elf32 kickoff.s

basic_console.o: basic_console.s
	nasm -f elf32 basic_console.s

exceptions.o: exceptions.s
	nasm -f elf32 exceptions.s

cfuncs.o: cfuncs.s
	nasm -f elf32 cfuncs.s

test.o: test.c
	gcc -c ${CFLAGS} test.c

stdio_funcs.o: stdio_funcs.c
	gcc -c ${CFLAGS} stdio_funcs.c

c_exceptions.o: c_exceptions.c
	gcc -c ${CFLAGS} c_exceptions.c

mmgr.o: mmgr.c
	gcc -c ${CFLAGS} mmgr.c

acpi:
	make -C acpi/

test.elf: kickoff.o exceptions.o basic_console.o test.o cfuncs.o stdio_funcs.o c_exceptions.o mmgr.o acpi
	ld -b elf32-i386 kickoff.o exceptions.o basic_console.o cfuncs.o test.o stdio_funcs.o c_exceptions.o mmgr.o acpi/rsdp.o acpi/search_mem.o acpi/sdt_header.o \
		-Ttext 0x7E00 -Tdata 0xA000 -Trodata 0x9000 \
		-static -nostartfiles -nodefaultlibs -nostdlib -nolibc -m elf_i386 --oformat elf32-i386 -o test.elf
