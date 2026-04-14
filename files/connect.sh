#!/bin/bash
# connect.sh — mount autocom qcow2 read-only + bind into chroot
# Run as root on st: bash /chroot/claude/home/claude/work/obd/files/connect.sh

DIR="$(cd "$(dirname "$0")" && pwd)"
QCOW="$DIR/win7_pro_autocom.qcow2"
MNT="/mnt/qcow2"
CHROOT_MNT="/chroot/claude/home/claude/work/obd/files/mnt"
NBD="/dev/nbd0"

if mountpoint -q "$MNT" 2>/dev/null; then
    echo "[*] Already mounted on $MNT"
    exit 0
fi

echo "[+] Loading nbd..."
modprobe nbd max_part=8

echo "[+] Connecting qcow2..."
qemu-nbd -r -c "$NBD" "$QCOW"
sleep 2

echo "[+] Partitions:"
fdisk -l "$NBD" 2>/dev/null | grep "$NBD"

mkdir -p "$MNT"

for p in "${NBD}p2" "${NBD}p1" "${NBD}p3"; do
    if [ -b "$p" ]; then
        if mount -o ro,noexec "$p" "$MNT" 2>/dev/null; then
            echo "[+] Mounted $p → $MNT"
            break
        fi
    fi
done

if ! mountpoint -q "$MNT"; then
    echo "[!] Could not mount. Try: mount -o ro ${NBD}pN $MNT"
    qemu-nbd -d "$NBD"
    exit 1
fi

echo "[+] C: drive contents:"
ls "$MNT/"

# bind mount into chroot so claude can see it
mkdir -p "$CHROOT_MNT"
mount --bind "$MNT" "$CHROOT_MNT"
echo "[+] Bind mounted → $CHROOT_MNT"
echo "[+] From chroot: ls /home/claude/work/obd/files/mnt/"
