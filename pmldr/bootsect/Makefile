all: bootsect.bin

bootsect.bin: bootsect.asm
	nasm -f bin bootsect.asm -o bootsect.bin

clean:
	rm -f bootsect.bin