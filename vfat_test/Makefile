all: vfat_test

clean:
	rm -f *.o vfat_test

vfat_test.o: vfat_test.c vfat.h ucs_conv.h
	gcc -c vfat_test.c -O0 -g -D__BUILDING_TESTS

ucs_conv.o: ucs_conv.c ucs_conv.h
	gcc -c ucs_conv.c -O0 -g

vfat_test: vfat_test.o ucs_conv.o
	gcc -o vfat_test vfat_test.o ucs_conv.o
