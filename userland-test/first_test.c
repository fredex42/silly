#include <unistd.h>
int main() {
	write(0, "Hello from userland", 20);
}
