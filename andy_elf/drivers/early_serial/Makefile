all: libearly_serial.a

serial_lowlevel.o: serial_lowlevel.asm
	nasm -f elf32 serial_lowlevel.asm

libearly_serial.a: serial_lowlevel.o
	ar rcs libearly_serial.a serial_lowlevel.o