all: test.elf

clean:
	rm -f kickoff.o test.elf
	
kickoff.o: kickoff.s
	nasm -f elf32 kickoff.s

test.elf: kickoff.o
	ld -b elf32-i386 kickoff.o -Ttext 0x7E00 -Tdata 0xA000 -static -nostartfiles -nodefaultlibs -nostdlib -nolibc -m elf_i386 --oformat elf32-i386 -o test.elf
