CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables  

all: libpci.a

libpci.a: pci_ops.o pci_scanner.o pci_ide.o
	ar rcs libpci.a pci_ops.o pci_scanner.o pci_ide.o

pci_ops.o: pci_ops.c pci_ops.h
	gcc ${CFLAGS} -c pci_ops.c

pci_scanner.o: pci_scanner.c pci_scanner.h pci_ops.h pci_categorisation.h
	gcc ${CFLAGS} -c pci_scanner.c

pci_ide.o:  pci_ide.c pci_ide.h pci_ops.h
	gcc ${CFLAGS} -c pci_ide.c

clean:
	rm -f *.a *.o
