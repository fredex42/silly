CFLAGS= -O0 -g -D__BUILDING_TESTS

all: libutils.a

clean:
	rm -f *.o *.a

generic_queue.o: generic_queue.c generic_queue.h
	gcc -c ${CFLAGS} generic_queue.c

directory_queue.o: directory_queue.c directory_queue.h generic_queue.h
	gcc -c ${CFLAGS} directory_queue.c

libutils.a: generic_queue.o directory_queue.o
	ar rcs libutils.a generic_queue.o directory_queue.o
