all: andy.sys

clean:
	rm -f andy.sys

andy.sys: andysys.s exceptions.s memlayout.inc
	nasm andysys.s -o andy.sys
