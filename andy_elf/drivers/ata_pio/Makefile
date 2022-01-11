CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables

all: libata_pio.a

clean:
	rm -f *.o *.a

detect.o: detect.c ata_pio.h
	gcc -c ${CFLAGS} detect.c

basic_routines.o: basic_routines.asm
	nasm -f elf32 basic_routines.asm

libata_pio.a: detect.o basic_routines.o
	ar rcs libata_pio.a detect.o basic_routines.o
