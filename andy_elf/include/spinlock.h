#include <types.h>

//use an int64 for the spinlock. needs to occupy a whole cache line; but the lock itself is the LSB only.
typedef uint64_t *spinlock_t;
