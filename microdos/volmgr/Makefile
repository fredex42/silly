CFLAGS=-fPIC -fno-stack-protector -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables 

all: libvolmgr.a

clean:
	rm -f *.o *.a
	
volmgr.o: volmgr.c volmgr.h
	gcc ${CFLAGS} -c volmgr.c

libvolmgr.a: volmgr.o
	ar rcs libvolmgr.a volmgr.o