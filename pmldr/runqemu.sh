#!/bin/bash

qemu-system-i386 -hda test.fat -m 64 -cpu athlon -serial stdio
