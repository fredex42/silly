all: pmldr

pmldr: pmldr.asm prologue.asm mainloader.asm fs.asm
	nasm -f bin pmldr.asm
	./convertmap.sh

clean:
	rm -f pmldr *.map