#!/bin/bash

if [ ! -f disk.img ]; then
    ./makegrub_hdd.sh
fi

if [ ! -f disk.img ]; then
    echo "Disk image not found and could not be created."
    exit 1
fi

qemu-system-i386 -hda disk.img -m 64 -cpu athlon -serial stdio