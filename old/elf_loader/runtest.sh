#!/bin/bash -e

make

rm -f test.fat
qemu-img create test.fat 16M
mkfs.vfat ./test.fat
sudo mount test.fat /mnt
sudo cp ../andy_elf/test.elf /mnt/andy.sys
sudo cp ../shell.app/shell.app /mnt/shell.app
sudo umount /mnt
make inject

bochs
