#!/bin/bash

rm -rf iso

mkdir -p iso/boot/grub
cp ../andy_elf/test.elf iso/boot/kernel.elf
cat > iso/boot/grub/grub.cfg << EOF
set timeout=10
set default=0
menuentry "AndyELF" {
    multiboot2 /boot/kernel.elf
    boot
}
EOF

grub-mkrescue -o silly.iso iso