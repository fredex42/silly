CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables

.PHONY: vfat

all: vfat fs.o

fs.o: fs.c fs.h
	gcc -c ${CFLAGS} fs.c

vfat:
	make -C vfat

clean:
	make -C vfat clean
