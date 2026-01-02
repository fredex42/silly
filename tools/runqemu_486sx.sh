#!/bin/bash

qemu-system-i386 -hda disk.img -m 4 -M isapc -cpu 486 -serial stdio \
    -device sb16 -vga cirrus -boot c -rtc base=localtime 
    #-device isa-fdc,drive=floppy0 -drive id=floppy0,index=1,format=raw,media=disk  \
