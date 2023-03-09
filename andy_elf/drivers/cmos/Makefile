CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables  

all: libcmos.a

lowlevel.o: lowlevel.asm
	nasm -f elf32 lowlevel.asm

rtc.o: rtc.c rtc.h lowlevel.h
	gcc -c ${CFLAGS} rtc.c

libcmos.a: lowlevel.o rtc.o
	ar rcs libcmos.a rtc.o lowlevel.o

clean:
	rm -f *.a *.o
