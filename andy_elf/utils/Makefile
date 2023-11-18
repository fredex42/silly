CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables

all: libutils.a

clean:
	rm -f *.a *.o

ucs_conv.o: ucs_conv.c ucs_conv.h
	gcc -c ${CFLAGS} ucs_conv.c

string.o: string.asm
	nasm -f elf32 string.asm

spinlock.o: spinlock.c ../include/spinlock.h
	gcc -c ${CFLAGS} spinlock.c

libutils.a: ucs_conv.o string.o spinlock.o
	ar rcs libutils.a ucs_conv.o string.o spinlock.o
