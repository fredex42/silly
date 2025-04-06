CFLAGS=-fPIC -fno-stack-protector -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables -DMMGR_PARANOID

all: libmmgr.a

clean:
	rm -f *.o *.a

mmgr.o: mmgr.c mmgr.h
	gcc -c ${CFLAGS} mmgr.c 

heap.o: heap.c heap.h
	gcc -c ${CFLAGS} heap.c

libmmgr.a: mmgr.o heap.o
	ar rcs libmmgr.a mmgr.o heap.o