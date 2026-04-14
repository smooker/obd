#!/bin/bash
# disconnect.sh — unmount qcow2 and cleanup
# Run as root on st: bash /chroot/claude/home/claude/work/obd/files/disconnect.sh

MNT="/mnt/qcow2"
CHROOT_MNT="/chroot/claude/home/claude/work/obd/files/mnt"
NBD="/dev/nbd0"

echo "[+] Unmounting chroot bind..."
umount "$CHROOT_MNT" 2>/dev/null

echo "[+] Unmounting qcow2..."
umount "$MNT" 2>/dev/null

echo "[+] Disconnecting nbd..."
qemu-nbd -d "$NBD" 2>/dev/null

rmdir "$MNT" 2>/dev/null
rmdir "$CHROOT_MNT" 2>/dev/null

echo "[+] Done"
