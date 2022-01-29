all: vfat_test

clean:
	rm -f *.o vfat_test

vfat_test.o: vfat_test.c vfat.h ucs_conv.h cluster_map.h
	gcc -c vfat_test.c -O0 -g -D__BUILDING_TESTS

ucs_conv.o: ucs_conv.c ucs_conv.h
	gcc -c ucs_conv.c -O0 -g

cluster_map.o: cluster_map.c cluster_map.h
	gcc -c cluster_map.c -O0 -g -D__BUILDING_TESTS

vfat_test: vfat_test.o ucs_conv.o cluster_map.o
	gcc -o vfat_test vfat_test.o ucs_conv.o cluster_map.o
