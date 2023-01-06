all: shell.app

clean:
	rm -f *.o *.app
	
start.o: start.asm
	nasm -f elf32 start.asm

shell.app: start.o
	ld -b elf32-i386 start.o \
		-static -nostartfiles -nodefaultlibs -nostdlib -nolibc -m elf_i386 --oformat elf32-i386 -o shell.app
