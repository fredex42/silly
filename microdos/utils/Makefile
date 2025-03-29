CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables 

all: libutils.a

clean:
	rm -f *.o *.a

gdt32.o: gdt32.c gdt32.h tss32.h
	gcc -c ${CFLAGS} gdt32.c

v86.o: v86.c v86.h
	gcc -c ${CFLAGS} v86.c 

tss32.o: tss32.c tss32.h
	gcc -c ${CFLAGS} tss32.c

libutils.a: v86.o gdt32.o tss32.o
	ar rcs libutils.a v86.o gdt32.o tss32.o