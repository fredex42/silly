CFLAGS=-g 

all: install_loader

fs.o: fs.c vfat.h fs.h
	gcc $(CFLAGS) -c fs.c

install_loader.o: install_loader.c fs.h
	gcc $(CFLAGS) -c install_loader.c

copier.o: copier.c fs.h vfat.h
	gcc $(CFLAGS) -c copier.c

install_loader: install_loader.o fs.o copier.o
	gcc $(CFLAGS) -o install_loader fs.o install_loader.o copier.o -lm

clean:
	rm -f *.o install_loader