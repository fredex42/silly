#include <unistd.h>

int write(int fd, const void *buf, size_t count) {
	int ret;
	//TODO: check actual register assignments to call
	asm volatile(
			"movl %[fd], %%ebx\n\t"
			"movl %[buf], %%edi\n\t"
			"movl %[count], %%ecx\n\t"
			"movl $0x01, %%eax\n\t"	//function id
			"int $0x60\n\t"
			"movl %%eax, %[ret]\n\t"
			: [ret] "=r"(ret)
			: [fd] "rm"(fd), [buf] "rm"(buf), [count] "rm"(count)
			: "eax", "ebx", "edi", "ecx"
	);
	return ret;
}

void _exit(int status) {
	asm volatile(
			"movl $0x01, %%eax\n\t"	//function id
			"int $0x60"			//does not return
			: : : "eax"
	);
	while(1) { }	//perma-loop just in case it does return
}
