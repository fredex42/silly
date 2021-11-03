all: test.elf

clean:
	rm -f kickoff.o exceptions.o basic_console.o test.o cfuncs.o test.elf
	
kickoff.o: kickoff.s
	nasm -f elf32 kickoff.s

basic_console.o: basic_console.s
	nasm -f elf32 basic_console.s

exceptions.o: exceptions.s
	nasm -f elf32 exceptions.s

cfuncs.o: cfuncs.s
	nasm -f elf32 cfuncs.s
	
test.o: test.c
	gcc -c -fno-pie -m32 test.c

test.elf: kickoff.o exceptions.o basic_console.o test.o cfuncs.o
	ld -b elf32-i386 kickoff.o exceptions.o basic_console.o cfuncs.o test.o \
		-Ttext 0x7E00 -Tdata 0xA000 -Trodata 0x9000 \
		-static -nostartfiles -nodefaultlibs -nostdlib -nolibc -m elf_i386 --oformat elf32-i386 -o test.elf
