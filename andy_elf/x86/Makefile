CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables -Wno-builtin-declaration-mismatch  

all: libx86.a

clean:
	rm -f *.o *.a

idt.o: idt.c ../include/x86/idt.h ../include/types.h
	gcc -c ${CFLAGS} idt.c

setup_idt.o: setup_idt.c ../include/x86/idt.h ../include/types.h ../include/x86/segmentation.h setup_idt.h
	gcc -c ${CFLAGS} setup_idt.c

libx86.a: idt.o setup_idt.o
	ar rcs libx86.a idt.o setup_idt.o