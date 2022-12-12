CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables

all: libfs_vfat.a

clean:
	rm -f *.o *.a

vfat.o: vfat.c ../../include/fs/vfat.h cluster_map.h
	gcc -c ${CFLAGS} vfat.c

fileops.o: fileops.c ../../include/fs/vfat.h ../../include/fs/fat_fileops.h cluster_map.h
	gcc -c ${CFLAGS} fileops.c

cluster_map.o: cluster_map.c cluster_map.h
	gcc -c ${CFLAGS} cluster_map.c

libfs_vfat.a: vfat.o fileops.o cluster_map.o
	ar rcs libfs_vfat.a vfat.o fileops.o cluster_map.o
