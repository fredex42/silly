CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables

all: libfs_vfat.a

clean:
	rm -f *.o *.a

vfat.o: vfat.c vfat.h
	gcc -c ${CFLAGS} vfat.c

fatbuffer.o: fatbuffer.c
	gcc -c ${CFLAGS} fatbuffer.c


libfs_vfat.a: vfat.o
	ar rcs libfs_vfat.a vfat.o
