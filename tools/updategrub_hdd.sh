#!/bin/bash -e

if [ ! -f disk.img ]; then
    echo "disk.img not found!"
    exit 1
fi

if [ ! -f ../andy_elf/build/test.elf ]; then
    echo "Kernel ELF not found!"
    exit 1
fi

if [ ! -f ../shell.app/shell.app ]; then
    echo "Shell application not found!"
    exit 1
fi

sudo kpartx -av disk.img
sudo mount /dev/mapper/loop0p1 /mnt/temp
sudo cp ../andy_elf/build/test.elf /mnt/temp/boot/mykernel.elf
sudo cp ../userland/test/shell.app /mnt/temp
sudo umount /mnt/temp
sudo kpartx -dv disk.img