
CFLAGS=-fno-pie -m32 -I../include -march=i486

all: search_mem.o rsdp.o

clean:
	rm -f *.o

search_mem.o: search_mem.asm
	nasm -f elf32 search_mem.asm

rsdp.o: rsdp.c
	gcc -c ${CFLAGS} rsdp.c
