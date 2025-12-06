#include <types.h>

//use an int64 for the spinlock. needs to occupy a whole cache line; but the lock itself is the LSB only.
typedef uint64_t __attribute__ ((aligned (64))) *spinlock_t;


/** 
 * Atomically acquire the given lock if it's free, or loop until it is available
*/
void acquire_spinlock(volatile spinlock_t *lock);

/**
 * Release a previouisly acquired lock
*/
void release_spinlock(volatile spinlock_t *lock);