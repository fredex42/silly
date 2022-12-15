CFLAGS=-fno-pie -m32 -I../../include -march=i486 -fno-asynchronous-unwind-tables -Wno-builtin-declaration-mismatch

all: libfs_vfat.a

clean:
	rm -f *.o *.a

vfat.o: vfat.c ../../include/fs/vfat.h cluster_map.h
	gcc -c ${CFLAGS} vfat.c

fileops.o: fileops.c ../../include/fs/vfat.h ../../include/fs/fat_fileops.h cluster_map.h
	gcc -c ${CFLAGS} fileops.c

dirops.o: dirops.c ../../include/fs/vfat.h ../../include/fs/fat_fileops.h ../../include/fs/fat_dirops.h
	gcc -c ${CFLAGS} dirops.c

cluster_map.o: cluster_map.c cluster_map.h
	gcc -c ${CFLAGS} cluster_map.c

libfs_vfat.a: vfat.o fileops.o dirops.o cluster_map.o
	ar rcs libfs_vfat.a vfat.o fileops.o dirops.o cluster_map.o
