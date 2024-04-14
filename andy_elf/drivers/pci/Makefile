CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables  

all: libpci.a

libpci.a: pci_ops.o
	ar rcs libpci.a pci_ops.o

pci_ops.o: pci_ops.c pci_ops.h
	gcc ${CFLAGS} -c pci_ops.c

clean:
	rm -f *.a *.o
