#!/bin/bash

qemu-system-i386 -s -S -serial stdio -hda test.fat -m 64 -cpu athlon
