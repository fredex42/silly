.PHONY: andy_sys

all: andy_sys

andy_sys:
	make -C andy_sys/

clean:
	make clean -C andy_sys
