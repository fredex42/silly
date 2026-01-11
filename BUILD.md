# Building the OS and Userland

## Kernel

## Userland

### 1. Build the cross-compiler

### 2. Configure and build newlib (the C library)

TODO: add crosscompiler flags and output path

```bash
mkdir newlib-i686-silly
cd newlib-i686-silly
../newlib/configure --disable-multilib   --disable-newlib-supplied-syscalls --srcdir ../newlib-4.5.0.20241231
make
../newlib/newlib/configure --disable-multilib   --disable-newlib-supplied-syscalls --srcdir ../newlib-4.5.0.20241231/newlib
make
```

### 3. Test cross-compiler with newlib

### 4. Configure Rust toolchain