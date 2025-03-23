void _start() {
    kputs("Hello world!");
    while(1) {
        __asm__ volatile("nop");
    }
}