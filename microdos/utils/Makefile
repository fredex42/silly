CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables 

all: libutils.a

clean:
	rm -f *.o *.a

v86.o: v86.c v86.h
	gcc -c ${CFLAGS} v86.c 

libutils.a: v86.o
	ar rcs libutils.a v86.o