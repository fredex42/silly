CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables -Wno-builtin-declaration-mismatch  

all: libvolmgr.a

clean:
	rm -f *.o *.a

volmgr.o: volmgr.c volmgr_internal.h ../include/volmgr.h ../include/fs/fat_fs.h
	gcc -c ${CFLAGS} volmgr.c

libvolmgr.a: volmgr.o
	ar rcs libvolmgr.a volmgr.o