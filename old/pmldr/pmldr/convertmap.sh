#!/bin/bash

cat pmldr.map | grep -P '^\s+\d' | awk '{print $2 " " $3}' > bochs.map
