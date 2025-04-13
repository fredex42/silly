CFLAGS=-fPIC -fno-stack-protector -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables

all: libbios_disk.a

clean:
	rm -f *.o *.a

bios_disk.o: bios_disk.c bios_disk.h bios_data_area.h
	gcc -c ${CFLAGS} bios_disk.c

libbios_disk.a: bios_disk.o
	ar rcs libbios_disk.a bios_disk.o