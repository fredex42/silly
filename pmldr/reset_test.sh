#!/bin/bash

echo -----------------------------
echo Resetting disk image
echo -----------------------------
rm -f test.fat
dd if=/dev/zero of=test.fat bs=1M count=128
mkfs.vfat -F 32 test.fat

echo -----------------------------
echo Installing boot loader
echo -----------------------------
./install_loader/install_loader test.fat

echo -----------------------------
echo Copying kernel
echo -----------------------------
mcopy -i test.fat ../andy_elf/test.elf ::/andy.sys

echo -----------------------------
echo Done
echo -----------------------------
