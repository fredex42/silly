CFLAGS=-fno-pie -m32 -I../include -march=i486 -fno-asynchronous-unwind-tables

all: libscheduler.a

clean:
	rm -f *.o *.a

lowlevel.o:
	nasm -f elf32 lowlevel.asm

process.o: process.c process.h
	gcc -c ${CFLAGS} process.c

scheduler_task.o: scheduler_task.c scheduler_task_internals.h ../include/scheduler/scheduler.h
	gcc -c ${CFLAGS} scheduler_task.c

scheduler.o: scheduler.c scheduler_task_internals.h ../include/scheduler/scheduler.h
		gcc -c ${CFLAGS} scheduler.c

libscheduler.a: scheduler.o scheduler_task.o lowlevel.o
	ar rcs libscheduler.a scheduler.o scheduler_task.o lowlevel.o
