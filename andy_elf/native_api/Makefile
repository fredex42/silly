CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables

all: libnative_api.a

clean:
	rm -f *.o *.a

console.o: console.c console.h
	gcc -c ${CFLAGS} console.c

native_api.o: native_api.asm apicodes.inc
	nasm -f elf32 native_api.asm

process_ops.o: process_ops.c process_ops.h
	gcc -c ${CFLAGS} process_ops.c

stream_ops.o: stream_ops.c stream_ops.h
	gcc -c ${CFLAGS} stream_ops.c

libnative_api.a: process_ops.o stream_ops.o native_api.o console.o
	ar rcs libnative_api.a native_api.o process_ops.o stream_ops.o console.o
