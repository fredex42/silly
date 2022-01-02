all: elf_boot

clean:
	rm -f elf_boot

elf_boot: elf_boot.asm elf_format.asm
	nasm -f bin elf_boot.asm -o elf_boot

inject: elf_boot
	../tools/inject test.fat elf_boot
