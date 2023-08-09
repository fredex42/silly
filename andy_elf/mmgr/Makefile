CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables -DMMGR_PARANOID -DMMGR_VERBOSE

all: libmmgr.a

clean:
	rm -f *.o *.a

mmgr.o: mmgr.c
	gcc -c ${CFLAGS} mmgr.c

heap.o: heap.c heap.h process.h
	gcc -c ${CFLAGS} heap.c

process.o: process.c process.h
	gcc -c ${CFLAGS} process.c

libmmgr.a: mmgr.o heap.o process.o
	ar rcs libmmgr.a mmgr.o heap.o process.o
