CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables  

all: libprocess.a

clean:
	rm -f *.o *.a
	
elfloader.o: elfloader.c elfloader.h
	gcc -c ${CFLAGS} elfloader.c

cleanup.o: cleanup.c
	gcc -c ${CFLAGS} cleanup.c

libprocess.a: elfloader.o cleanup.o
	ar rcs libprocess.a elfloader.o cleanup.o
