#!/bin/bash
objdump -t andy.sys | perl -e 'while(<>) { chomp $_; /^([0-9a-f]+).*?([^\s]+)$/g; print "$1 $2\n" }' > kernel.map