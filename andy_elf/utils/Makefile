CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables -DDEBUG_SPINLOCK

all: libutils.a

clean:
	rm -f *.a *.o

ucs_conv.o: ucs_conv.c ucs_conv.h
	gcc -c ${CFLAGS} ucs_conv.c

string.o: string.asm
	nasm -f elf32 string.asm

spinlock.o: spinlock.c ../include/spinlock.h
	gcc -c ${CFLAGS} spinlock.c

ringbuffer.o: ringbuffer.c ../include/utils/ringbuffer.h
	gcc -c ${CFLAGS} ringbuffer.c

debug.o: debug.c debug.h
	gcc -c ${CFLAGS} debug.c

libutils.a: ucs_conv.o string.o spinlock.o ringbuffer.o debug.o
	ar rcs libutils.a ucs_conv.o string.o spinlock.o ringbuffer.o debug.o
