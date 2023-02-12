all: libps2_controller

lowlevel.o: lowlevel.asm
	nasm -f elf32 lowlevel.asm

libps2_controller: lowlevel.o
	ar rcs libps2_controller.a lowlevel.o

clean:
	rm -rf *.a *.o