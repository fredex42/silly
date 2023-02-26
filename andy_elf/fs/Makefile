CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables  -target i386-pc-none-elf

.PHONY: vfat

all: vfat fs.o

fs.o: fs.c ../include/fs.h
	gcc -c ${CFLAGS} fs.c

vfat:
	make -C vfat

clean:
	make -C vfat clean
