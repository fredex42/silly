CFLAGS= -O0 -g -D__BUILDING_TESTS
all: vfat_test

clean:
	rm -f *.o vfat_test

vfat_test.o: vfat_test.c vfat.h ucs_conv.h cluster_map.h generic_storage.h fat_fs.h
	gcc -c vfat_test.c ${CFLAGS}

ucs_conv.o: ucs_conv.c ucs_conv.h
	gcc -c ucs_conv.c ${CFLAGS}

cluster_map.o: cluster_map.c cluster_map.h fat_fs.h
	gcc -c cluster_map.c ${CFLAGS}

fake_storage_driver.o: fake_storage_driver.c fake_storage_driver.h generic_storage.h
	gcc -c fake_storage_driver.c ${CFLAGS}

fat_fs.o: fat_fs.c fat_fs.h
	gcc -c fat_fs.c ${CFLAGS}

vfat_test: fake_storage_driver.o fat_fs.o ucs_conv.o cluster_map.o vfat_test.o
	gcc -o vfat_test vfat_test.o ucs_conv.o cluster_map.o fat_fs.o fake_storage_driver.o
