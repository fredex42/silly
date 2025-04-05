#!/bin/bash
objdump -t andy.sys | perl -e 'while(<>) { chomp $_; /^([0-9a-f]+).*?([^\s]+)$/g; print "$1 $2\n" }' > kernel.map
cat kernel.map | perl -e 'while(<>) { chomp $_; /^([0-9a-f]+)\s([^\s]+)/g; my $addr=hex($1) + 0xff000000; printf("%x $2\n"
, $addr); }' > kernel_hi.map