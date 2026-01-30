# Building andy_elf with Meson

## Prerequisites
- GCC with 32-bit support (-m32) and binutils
- nasm assembler
- Meson (>= 0.60) and Ninja

## Build
```sh
meson setup build
meson compile -C build
```
Artifacts:
- build/test.elf — linked with linker.ld
- build/kernel.map — generated via gen_bochs_map.sh
- build/test_mmgr — standalone mmgr unit test

## Notes
- The build mirrors the original Makefiles: NASM sources are assembled via `nasm -f elf32`, C sources use `-m32` and `-fno-pie`.
- If your toolchain lacks 32-bit support, install gcc-multilib (distribution-specific) or an i386 cross toolchain.
