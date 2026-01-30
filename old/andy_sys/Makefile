all: andy.sys

clean:
	rm -f andy.sys

andy.sys: andysys.s exceptions.s memlayout.asm
	nasm andysys.s -o andy.sys
