all: install_loader

fs.o: fs.c vfat.h fs.h
	gcc -c fs.c

install_loader.o: install_loader.c fs.h
	gcc -c install_loader.c


install_loader: install_loader.o fs.o
	gcc -o install_loader fs.o install_loader.o -lm

clean:
	rm -f *.o install_loader