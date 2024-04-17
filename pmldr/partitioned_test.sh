#!/bin/bash -e

echo -----------------------------
echo Resetting disk image
echo -----------------------------
rm -f test.img
dd if=/dev/zero of=test.fat bs=1M count=1024
cat << EOF | fdisk test.img
o
n
p


t
b
a
w
EOF

echo -----------------------------
echo Mounting partition. You might need to provide a sudo password.
echo -----------------------------
sudo kpartx -av test.img | tee partx.tmp
LOOPDEV=$(cat partx.tmp | awk '{print $3}')
if [ "${LOOPDEV}" == "" ]; then
    echo Unable to mount partition. Please check the script.
    exit 1
fi

echo Partition mounted at ${LOOPDEV}
mkfs.vfat /dev/mapper/${LOOPDEV}

