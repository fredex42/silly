all: test.elf

clean:
	rm -f kickoff.o exceptions.o basic_console.o test.elf
	
kickoff.o: kickoff.s
	nasm -f elf32 kickoff.s

basic_console.o: basic_console.s
	nasm -f elf32 basic_console.s

exceptions.o: exceptions.s
	nasm -f elf32 exceptions.s

test.elf: kickoff.o exceptions.o basic_console.o
	ld -b elf32-i386 kickoff.o exceptions.o basic_console.o -Ttext 0x7E00 -Tdata 0xA000 -static -nostartfiles -nodefaultlibs -nostdlib -nolibc -m elf_i386 --oformat elf32-i386 -o test.elf
