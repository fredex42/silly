#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

extern int errno;
int main(int argc, char **argv) {
	char buf[512];

	if(argc!=3) {
		puts("Usage: ./inject {disk-image} {bootsector-bin}");
		exit(1);
	}

	printf("Disk image: %s, bootsector-bin: %s\n", argv[1], argv[2]);
	
	FILE *fpboot = fopen(argv[2],"r");
	if(fpboot==NULL) {
		fprintf(stderr, "could not open boot sector binary %s: %d\n", argv[2], errno);
		exit(2);
	}

	fread(buf, 512, 1, fpboot);
	fclose(fpboot);

	int fp = open(argv[1], O_WRONLY);
	if(fp<0) {
		fprintf(stderr,"Could not open disk image %s: %d\n", argv[1], errno);
		exit(2);
	}
	lseek(fp, 0, SEEK_SET);

	write(fp, buf, 512);
	close(fp);
	exit(0);
}
