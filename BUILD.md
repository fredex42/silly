# Building the OS and Userland

## Kernel
You can build the kernel with your native toolchain, provided that it supports 32-bit x86 output.  The kernel is not yet ready for 64-bit.

```bash
cd andy_elf
make -j4
```

The final binary is called `test.elf`, this is intended to be loaded
in a VM via grub TODO

## Userland

**TL;DR** The fully configured toolchain is available on Docker Hub.  Simply:

```bash
docker run --rm -it andyg42/crosscompiler-silly-elf32:latest
```

to get the toolchain and not need to run any of the steps below.


### 1. Build the cross-compiler
Download binutils from here: https://sourceware.org/pub/binutils/releases/.
I initially used version 2.45

Download gcc from here: https://gcc.gnu.org/releases.html.
I initially used version 14.3.0.

I would **highly** recommend using Docker or another system that lets you
build these things in isolation.

```bash
docker run --rm -it -v $HOME/path/to/code:/usr/src ...any other paths... ubuntu:latest
apt update
apt install -y build-essential nasm vim file less libgmp-dev libmpc-dev libmpfr-dev
```

With this in place, you can decompress and compile binutils:

```bash
cd /path/to/binutils/in/container
tar xzf binutils-2.45
mkdir build-binutils
cd build-binutils
../binutils-2.45/configure --target=i686-elf --prefix=/opt --with-sysroot --disable-nls --disable-werror
make
make install
```

Once you've done this, `ls /opt/i686-elf/bin/` should show you `ar`, `as`, `ld`, etc.  `ls /opt/bin` should show you `i686-elf-ar`, `i686-elf-as`, `i686-elf-ld`, etc.

With binutils installed, next you can compile gcc:
```bash
cd /path/to/gcc/in/container
tar xzf gcc-14.3.0
mkdir build-gcc
cd build-gcc
../gcc-14.3.0/configure --target=i686-elf --prefix=/opt --with-sysroot=/opt/i686-elf --disable-nls --enable-languages=c --without-headers --disable-shared --disable-threads --disable-multilib
mkdir -p /opt/i686-elf/usr/include
make -j4 all-gcc #tune to your number of cores
make install-gcc
```

Once you've done this, `ls /opt/bin` should show you `i686-elf-gcc`, `i686-elf-cpp`, etc.

### 2. Configure and build newlib (the C library)

We're now going to use the cross-compiler to build libc.  This repo contains a version of newlib, customised
to contain syscalls for the OS.  **TODO** well, not actually customised yet ;-)

```bash
export PATH=/opt/bin:$PATH
export CC=/opt/bin/i686-elf-gcc
cd /usr/src/silly
cd newlib
mkdir i686-silly
cd i686-silly
../newlib/configure --disable-multilib --srcdir ../newlib --host i686-elf --prefix /opt
make -j4
make install
```

Once you've done this, `ls /opt/i686-elf/include` should be full of .h files, and `/opt/i686-elf/lib` should include `libc.a`, `libm.a`, etc.

Now build and install crt0.o:

```bash
cd userland-test
nasm -f elf32 crt0.asm -o crt0.o
cp crt0.o /opt/i686-elf/lib
```

### 3. Test cross-compiler with newlib
With these in place you should be able to smoke-test the crosscompiler:

```bash
i686-elf-gcc -nostdlib crt0.o first_test.c stubs.c -L/opt/i686-elf/lib -lc -I/opt/i686-elf/include
file a.out
# a.out: ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), statically linked, not stripped
i686-elf-objdump -x a.out

# a.out:     file format elf32-i386
# a.out
# architecture: i386, flags 0x00000112:
# EXEC_P, HAS_SYMS, D_PAGED
# start address 0x08048080

# Program Header:
#     LOAD off    0x00000000 vaddr 0x08048000 paddr 0x08048000 align 2**12
#          filesz 0x00000288 memsz 0x00000288 flags r-x
#     LOAD off    0x00000288 vaddr 0x08049288 paddr 0x08049288 align 2**12
#          filesz 0x00000014 memsz 0x00000034 flags rw-
# etc. etc.

i686-elf-objdump -d a.out #if you want to check the assembly code of the binary
```

Crucially:
- you want a 32-bit LSB executable, not a 64-bit one.
- you should see text, rodata, bss sections and two loadable chunks. Other sections may be present, but the OS loader doesn't care about them.
- you should see the `write` and `_exit` stubs as well as a `main` function

### 5. Rebuild the cross-compiler with libc

Now we have a working libc, we can rebuild the cross-compiler so it will work properly.

```bash
cd /Downloads
rm -rf build-gcc
mkdir build-gcc
cd build-gcc
unset CC # important - we want to rebuild gcc with the host compiler, not the cross-compiler
../gcc-14.3.0/configure --target=i686-elf --prefix=/opt --with-sysroot=/opt/i686-elf --disable-nls --enable-languages=c --disable-shared --disable-threads --disable-multilib #important - not using --without-headers as we were before
make all-gcc -j4
make install-gcc
make all-target-libgcc -j4
make install-target-libgcc
```

Once you have done this, `/opt/lib/gcc/i686-elf/14.3.0` should contain files like `crtbegin.o`, `crtend.o`, `libgcc.a` etc.

Now we can re-run the smoke test:

```bash
cd $REPO_ROOT/userland-test
rm a.out
i686-elf-gcc first_test.c stubs.c
file a.out 
# a.out: ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), statically linked, not stripped
objdump -x a.out
```

You'll now see more sections, things like `crtstuff.c`, `__CTOR_LIST__` and other symbols from libgcc.

### 6. Configure Rust toolchain