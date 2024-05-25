CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables

all: libps2_controller

controller.o: controller.c controller.h lowlevel.h
	gcc -c ${CFLAGS} controller.c

keymap.o: keymap.c keymap.h builtin_keymap.h
	gcc -c ${CFLAGS} keymap.c

lowlevel.o: lowlevel.asm
	nasm -f elf32 lowlevel.asm

libps2_controller: lowlevel.o controller.o keymap.o
	ar rcs libps2_controller.a lowlevel.o controller.o keymap.o

clean:
	rm -rf *.a *.o