#!/bin/bash
# go.sh — DS150E quick launcher
# Run as root on st: bash /chroot/claude/home/claude/work/obd/tools/go.sh
#
# Ensures ftdi_sio loaded, device present, runs ds150e

DIR="$(cd "$(dirname "$0")" && pwd)"
DEV="/dev/ttyUSB0"

echo "=== DS150E launcher ==="

# 1. ftdi_sio module
if ! lsmod | grep -q ftdi_sio; then
    echo "[+] Loading ftdi_sio..."
    modprobe ftdi_sio 2>/dev/null || true
fi

# 2. custom product ID
if lsusb | grep -q '0403:d6da'; then
    echo "[+] DS150E detected on USB"
    if [ ! -e "$DEV" ]; then
        echo "[+] Adding custom ID to ftdi_sio..."
        echo 0403 d6da > /sys/bus/usb-serial/drivers/ftdi_sio/new_id 2>/dev/null
        sleep 1
    fi
else
    echo "[!] DS150E not found on USB. Plugged in?"
    exit 1
fi

# 3. check ttyUSB
if [ ! -e "$DEV" ]; then
    # try other ttyUSB
    DEV=$(ls /dev/ttyUSB* 2>/dev/null | head -1)
    if [ -z "$DEV" ]; then
        echo "[!] No /dev/ttyUSB* found"
        exit 1
    fi
fi
echo "[+] Using $DEV"

# 4. compile if needed
BIN="$DIR/ds150e"
if [ ! -x "$BIN" ] || [ "$DIR/ds150e.c" -nt "$BIN" ]; then
    echo "[+] Compiling ds150e..."
    gcc -Wall -O2 -o "$BIN" "$DIR/ds150e.c" || exit 1
fi

# 5. run
echo ""
exec "$BIN" "$DEV" "${@:-info}"
