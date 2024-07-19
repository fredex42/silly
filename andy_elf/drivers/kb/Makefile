CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables

all: libkb.a

keyboard.o: keyboard.c ../../include/drivers/kb_buffer.h
	gcc ${CFLAGS} -c keyboard.c

pending_operation_list.o: pending_operation_list.c ../../include/drivers/kb_buffer.h
	gcc ${CFLAGS} -c pending_operation_list.c

libkb.a: keyboard.o pending_operation_list.o
	ar rcs libkb.a keyboard.o pending_operation_list.o

clean:
	rm -rf *.a *.o