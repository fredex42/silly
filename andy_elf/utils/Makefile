all: libutils.a

ucs_conv.o: ucs_conv.c ucs_conv.h
	gcc -c ${CFLAGS} ucs_conv.c

libutils.a: ucs_conv.o
	ar rcs libutils.a ucs_conv.o
	
