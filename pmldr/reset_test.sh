#!/bin/bash

rm -f test.fat
dd if=/dev/zero of=test.fat bs=1M count=128
mkfs.vfat -F 32 test.fat

