CFLAGS= -O0 -g -D__BUILDING_TESTS
all: vfat_test

clean:
	rm -f *.o vfat_test
	make -C utils/ clean

utils/libutils.a:
	make -C utils/

mounter.o: mounter.c vfat.h ucs_conv.h cluster_map.h generic_storage.h fat_fs.h
	gcc -c mounter.c ${CFLAGS}

file.o: file.c file.h vfat.h fat_fs.h
	gcc -c file.c ${CFLAGS}

dir.o: dir.c dir.h vfat.h fat_fs.h
	gcc -c dir.c ${CFLAGS}

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

string_helpers.o: string_helpers.c string_helpers.h
	gcc -c string_helpers.c ${CFLAGS}

vfat_test: mounter.o file.o dir.o fake_storage_driver.o fat_fs.o ucs_conv.o cluster_map.o vfat_test.o string_helpers.o utils/libutils.a
	gcc -o vfat_test vfat_test.o ucs_conv.o cluster_map.o dir.o fat_fs.o fake_storage_driver.o string_helpers.o mounter.o  file.o -Lutils -lutils
