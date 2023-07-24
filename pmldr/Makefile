.PHONY: bootsect pmldr install_loader

all: bootsect pmldr install_loader

bootsect:
	make -C bootsect

pmldr:
	make -C pmldr

install_loader:
	make -C install_loader

clean:
	make -C bootsect clean
	make -C pmldr clean
	make -C install_loader clean
	