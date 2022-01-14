CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables

all: libata_pio.a

clean:
	rm -f *.o *.a

detect.o: detect.c ata_pio.h
	gcc -c ${CFLAGS} detect.c

basic_routines.o: basic_routines.asm
	nasm -f elf32 basic_routines.asm

readwrite.o: readwrite.c ata_pio.h
	gcc -c ${CFLAGS} readwrite.c

interrupt.o: interrupt.c ata_pio.h
	gcc -c ${CFLAGS} interrupt.c

libata_pio.a: detect.o basic_routines.o interrupt.o readwrite.o
	ar rcs libata_pio.a detect.o basic_routines.o readwrite.o interrupt.o
