all: shell.app

clean:
	rm -f *.o *.app
	
start.o: start.asm
	nasm -f elf32 start.asm

shell.app: start.o
	ld -m elf_i386 start.o -static --nostdlib -o shell.app
