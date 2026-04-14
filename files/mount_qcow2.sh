#!/bin/bash
# mount_qcow2.sh — mount autocom qcow2 and extract Autocom software
# Run as root on st: bash /chroot/claude/home/claude/work/obd/files/mount_qcow2.sh
#
# Actions: mount / umount / extract

DIR="$(cd "$(dirname "$0")" && pwd)"
QCOW="$DIR/win7_pro_autocom.qcow2"
MNT="/mnt/qcow2"
EXTRACT="$DIR/autocom_sw"
NBD="/dev/nbd0"

case "${1:-extract}" in

mount)
    echo "[+] Loading nbd module..."
    modprobe nbd max_part=8

    echo "[+] Connecting $QCOW..."
    qemu-nbd -r -c "$NBD" "$QCOW"
    sleep 2

    echo "[+] Partitions:"
    fdisk -l "$NBD"

    mkdir -p "$MNT"
    echo "[+] Trying to mount partitions..."
    # Win7 typically has p1=System Reserved, p2=C:
    for p in "${NBD}p2" "${NBD}p1" "${NBD}p3"; do
        if [ -b "$p" ]; then
            mount -o ro "$p" "$MNT" 2>/dev/null && echo "[+] Mounted $p on $MNT" && break
        fi
    done

    if mountpoint -q "$MNT"; then
        echo "[+] Contents:"
        ls "$MNT/"
    else
        echo "[!] Could not mount any partition. Try manually:"
        echo "    mount -o ro ${NBD}pN $MNT"
    fi
    ;;

umount)
    echo "[+] Unmounting..."
    umount "$MNT" 2>/dev/null
    qemu-nbd -d "$NBD" 2>/dev/null
    rmdir "$MNT" 2>/dev/null
    echo "[+] Done"
    ;;

extract)
    echo "=== Extract Autocom software from qcow2 ==="

    # mount first
    modprobe nbd max_part=8
    qemu-nbd -r -c "$NBD" "$QCOW"
    sleep 2

    mkdir -p "$MNT"
    # try p2 first (typical C:), then p1
    for p in "${NBD}p2" "${NBD}p1" "${NBD}p3"; do
        if [ -b "$p" ]; then
            mount -o ro "$p" "$MNT" 2>/dev/null && echo "[+] Mounted $p" && break
        fi
    done

    if ! mountpoint -q "$MNT"; then
        echo "[!] Mount failed. Partitions:"
        fdisk -l "$NBD"
        qemu-nbd -d "$NBD"
        exit 1
    fi

    echo "[+] Searching for Autocom/Delphi/CDP software..."
    mkdir -p "$EXTRACT"

    # search common locations
    for d in \
        "$MNT/Program Files/Autocom"* \
        "$MNT/Program Files (x86)/Autocom"* \
        "$MNT/Program Files/Delphi"* \
        "$MNT/Program Files (x86)/Delphi"* \
        "$MNT/Program Files/CDP"* \
        "$MNT/Program Files (x86)/CDP"* \
        "$MNT/Program Files/TDB"* \
        "$MNT/Program Files (x86)/TDB"* \
        "$MNT/Users/*/Desktop/Autocom"* \
        "$MNT/Users/*/Desktop/CDP"* \
        "$MNT/Users/*/Documents/Autocom"* \
    ; do
        if [ -d "$d" ]; then
            echo "  [+] Found: $d"
            cp -r "$d" "$EXTRACT/"
        fi
    done

    # also grab any relevant exe/dll from Program Files
    echo "[+] Searching for related executables..."
    find "$MNT/Program Files" "$MNT/Program Files (x86)" \
        -iname '*autocom*' -o -iname '*delphi*' -o -iname '*cdp*' -o -iname '*ds150*' \
        2>/dev/null | tee "$EXTRACT/found_files.txt"

    echo ""
    echo "[+] Extracted to: $EXTRACT/"
    ls -la "$EXTRACT/"

    # cleanup
    umount "$MNT"
    qemu-nbd -d "$NBD"
    rmdir "$MNT" 2>/dev/null
    echo "[+] Done"
    ;;

*)
    echo "Usage: $0 [mount|umount|extract]"
    ;;
esac
