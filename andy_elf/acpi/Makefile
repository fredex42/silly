
CFLAGS=-fno-pie -m32 -I../include -march=i486

all: search_mem.o rsdp.o sdt_header.o apic.o

clean:
	rm -f *.o

search_mem.o: search_mem.asm
	nasm -f elf32 search_mem.asm

rsdp.o: rsdp.c ../include/sys/mmgr.h
	gcc -c ${CFLAGS} rsdp.c

sdt_header.o: sdt_header.c
	gcc -c ${CFLAGS} sdt_header.c

apic.o: apic.h
	gcc -c ${CFLAGS} apic.c
