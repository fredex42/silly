.PHONY: 8259pic acpi drivers scheduler mmgr fs process utils native_api

all: test.elf kernel.map

tests: test_mmgr

CFLAGS=-fno-pie -m32 -Iinclude -march=i486 -fno-asynchronous-unwind-tables 

clean:
	rm -f *.o test.elf test_mmgr
	make -C acpi clean
	make -C 8259pic clean
	make -C drivers clean
	make -C scheduler clean
	make -C mmgr clean
	make -C process clean
	make -C utils clean
	make -C native_api clean

kickoff.o: kickoff.s
	nasm -f elf32 kickoff.s

basic_console.o: basic_console.s
	nasm -f elf32 basic_console.s

exceptions.o: exceptions.s
	nasm -f elf32 exceptions.s

cfuncs.o: cfuncs.s
	nasm -f elf32 cfuncs.s


stdio_funcs.o: stdio_funcs.c
	gcc -c ${CFLAGS} stdio_funcs.c

c_exceptions.o: c_exceptions.c
	gcc -c ${CFLAGS} c_exceptions.c

panic.o: panic.c
	gcc -c ${CFLAGS} panic.c

inkernel_memory_tests.o: inkernel_memory_tests.c include/sys/mmgr.h include/types.h
	gcc -c ${CFLAGS} inkernel_memory_tests.c

memops.o: memops.asm
	nasm -f elf32 memops.asm

ioports.o: ioports.asm
	nasm -f elf32 ioports.asm

cpuid.o: cpuid.asm
	nasm -f elf32 cpuid.asm

acpi:
	make -C acpi

8259pic:
	make -C 8259pic

drivers:
	make -C drivers

scheduler:
	make -C scheduler
mmgr:
	make -C mmgr

fs:
	make -C fs

process:
	make -C process

utils:
	make -C utils

native_api:
	make -C native_api

test.elf: kickoff.o exceptions.o basic_console.o cfuncs.o stdio_funcs.o \
		c_exceptions.o panic.o inkernel_memory_tests.o memops.o ioports.o \
		cpuid.o mmgr 8259pic acpi drivers scheduler fs process utils native_api
	ld -b elf32-i386 kickoff.o exceptions.o basic_console.o cfuncs.o stdio_funcs.o \
		c_exceptions.o panic.o inkernel_memory_tests.o memops.o ioports.o cpuid.o fs/fs.o \
		-L mmgr/ -lmmgr \
		-L 8259pic/ -l8259pic -Lacpi/ -lacpi -Ldrivers/ata_pio/ -lata_pio -Lscheduler/ -l scheduler \
		-Ldrivers/early_serial -learly_serial -Lnative_api -lnative_api -Lfs/vfat -lfs_vfat -L process -lprocess -L utils -lutils \
		-Ldrivers/cmos -lcmos -Ldrivers/pci -lpci \
		-Ttext 0x7E00 \
		-static -nostartfiles -nodefaultlibs -nostdlib -nolibc -m elf_i386 --oformat elf32-i386 -o test.elf

kernel.map: test.elf
	./gen_bochs_map.sh

test_mmgr: test_mmgr.c test_stubs.c mmgr.c include/sys/mmgr.h
	gcc -o test_mmgr -g test_mmgr.c test_stubs.c mmgr.c -D__BUILDING_TESTS -m32
