CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables

all: libmmgr.a

mmgr.o: mmgr.c 
	gcc -c ${CFLAGS} mmgr.c

libmmgr.a: mmgr.o
	ar rcs libmmgr.a mmgr.o
