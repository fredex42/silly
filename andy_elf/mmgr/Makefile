CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables

all: libmmgr.a

mmgr.o: mmgr.c
	gcc -c ${CFLAGS} mmgr.c

heap.o: heap.c heap.h
	gcc -c ${CFLAGS} heap.c

libmmgr.a: mmgr.o heap.o
	ar rcs libmmgr.a mmgr.o heap.o
