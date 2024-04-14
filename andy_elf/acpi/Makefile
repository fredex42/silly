
CFLAGS=-fno-pie -m32 -I../include -march=i486  

all: libacpi.a

clean:
	rm -f *.o *.a

search_mem.o: search_mem.c
	gcc -c ${CFLAGS} search_mem.c

rsdp.o: rsdp.c ../include/sys/mmgr.h
	gcc -c ${CFLAGS} rsdp.c

sdt_header.o: sdt_header.c
	gcc -c ${CFLAGS} sdt_header.c

apic.o: apic.h
	gcc -c ${CFLAGS} apic.c

libacpi.a: apic.o sdt_header.o rsdp.o search_mem.o
	ar rcs libacpi.a apic.o sdt_header.o rsdp.o search_mem.o
