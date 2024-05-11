#!/bin/bash

qemu-system-i386 -serial stdio -hda test.fat -m 64 -cpu athlon
