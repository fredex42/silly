#!/usr/bin/env bash
set -euo pipefail

# ============================================================
# CONFIGURATION
# ============================================================
cp ../andy_elf/test.elf ./mykernel.elf
IMAGE=disk.img
IMAGE_SIZE=200M            # Size of disk image
KERNEL=./mykernel.elf      # Path to your kernel file
MOUNTDIR=./mnt

# ============================================================
# PREP
# ============================================================

if [ ! -f "$KERNEL" ]; then
    echo "Kernel file not found: $KERNEL"
    exit 1
fi

rm -f "$IMAGE"
qemu-img create -f raw "$IMAGE" "$IMAGE_SIZE"

# ============================================================
# PARTITION DISK
# ============================================================

# Create:
#  - MBR partition table
#  - 1 bootable primary partition taking the whole disk

echo "Creating partition table..."
parted -s "$IMAGE" mklabel msdos
parted -s "$IMAGE" mkpart primary fat32 1MiB 100%
parted -s "$IMAGE" set 1 boot on

# ============================================================
# MAP LOOP DEVICE + PARTITIONS
# ============================================================

echo "Mapping partitions..."
LOOPDEV=$(sudo losetup --find --show "$IMAGE")

# Create /dev/mapper/loopXpY mappings
sudo kpartx -av "$LOOPDEV"

PART_DEV="/dev/mapper/$(basename $LOOPDEV)p1"

# Wait for the device to appear
sleep 1

# ============================================================
# CREATE FILESYSTEM
# ============================================================

echo "Creating ext2 filesystem..."
sudo mkfs.vfat -F32 "$PART_DEV"

sudo mkdir -p "$MOUNTDIR"
sudo mount "$PART_DEV" "$MOUNTDIR"

# ============================================================
# INSTALL GRUB
# ============================================================

echo "Installing GRUB..."
sudo grub-install \
    --target=i386-pc \
    --boot-directory="$MOUNTDIR/boot" \
    --modules="fat part_msdos multiboot" \
    "$LOOPDEV"

# ============================================================
# COPY KERNEL + GRUB CONFIG
# ============================================================

echo "Setting up /boot contents..."
sudo cp "$KERNEL" "$MOUNTDIR/boot/"

# Create grub.cfg
sudo mkdir -p "$MOUNTDIR/boot/grub"
sudo tee "$MOUNTDIR/boot/grub/grub.cfg" >/dev/null <<EOF
set timeout=0
set default=0

menuentry "My Kernel" {
    multiboot2 /boot/$(basename "$KERNEL")
    boot
}
EOF

# ============================================================
# CLEANUP
# ============================================================

echo "Cleaning up..."
sudo umount "$MOUNTDIR"
sudo kpartx -dv "$LOOPDEV"
sudo losetup -d "$LOOPDEV"

echo "Done. Created disk image: $IMAGE"
echo
echo "You can test it with:"
echo "  qemu-system-x86_64 -hda $IMAGE"
