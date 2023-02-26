CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables  -target i386-pc-none-elf

all: libprocess.a

clean:
	rm -f *.o *.a
	
elfloader.o: elfloader.c elfloader.h
	gcc -c ${CFLAGS} elfloader.c

libprocess.a: elfloader.o
	ar rcs libprocess.a elfloader.o
