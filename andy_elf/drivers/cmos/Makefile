CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables  -target i386-pc-none-elf

all: libcmos.a

lowlevel.o: lowlevel.asm
	nasm -f elf32 lowlevel.asm

rtc.o: rtc.c rtc.h lowlevel.h
	gcc -c ${CFLAGS} rtc.c

libcmos.a: lowlevel.o rtc.o
	ar rcs libcmos.a lowlevel.o rtc.o

clean:
	rm -f *.a *.o
