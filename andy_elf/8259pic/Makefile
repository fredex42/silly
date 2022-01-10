CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables

all: lib8259pic.a

clean:
	rm -f *.o *.a

8259pic.o: 8259pic.c 8259pic.h
	gcc -c ${CFLAGS} 8259pic.c

picroutines.o: picroutines.asm
	nasm -f elf32 picroutines.asm

interrupts.o: interrupts.asm
	nasm -f elf32 interrupts.asm

ioapic.o: ioapic.c ioapic.h interrupts.h
	gcc -c ${CFLAGS} ioapic.c

lib8259pic.a: 8259pic.o picroutines.o interrupts.o ioapic.o
	ar rcs lib8259pic.a 8259pic.o picroutines.o interrupts.o ioapic.o
