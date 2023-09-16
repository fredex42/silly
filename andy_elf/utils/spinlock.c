#include <types.h>
#include <spinlock.h>

/** 
 * Atomically acquire the given lock if it's free, or loop until it is available
*/
void acquire_spinlock(spinlock_t *lock) {
    asm(
        ".acquire%=:\n\t"
        "lock bts $0, (%0)\n\t"        //bts is "bit switch". This will set bit 0 to 1 and return the previous value in the "carry" flag
        "jnc .wait_done%=\n\t"          //if the bit was 1 already, we are still locked, so wait
        ".spin_wait%=:\n\t"
        "pause\n\t"
        "testw $1, (%0)\n\t"      //is it clear yet?
        "jnz .spin_wait%=\n\t"
        "jmp .acquire%=\n\t"
        ".wait_done%=:"  : : "a"(lock) : "memory"   //once it's cleared, try to acquire again
    );
}

void release_spinlock(spinlock_t *lock) {
    asm(
        "lock btc $0, (%0)\n\t" : : "r"(lock) : "memory"
    );
}