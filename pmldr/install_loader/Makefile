all: install_loader

fs.o: fs.c vfat.h fs.h
	gcc -c fs.c

install_loader.o: install_loader.c fs.h
	gcc -c install_loader.c

copier.o: copier.c copier.h
	gcc -c copier.c

install_loader: install_loader.o fs.o copier.o
	gcc -o install_loader fs.o install_loader.o copier.o -lm