#!/bin/bash
# baud_scan.sh — find DS150E baud rate
# Usage: bash baud_scan.sh [/dev/ttyUSBx]
# Tries all common baud rates with *20A (get device name)
# Safe: read-only, no relays, no CAN

DEV="${1:-/dev/ttyUSB0}"

if [ ! -c "$DEV" ]; then
    echo "[!] $DEV not found"
    echo "    Load ftdi_sio first:"
    echo "    modprobe ftdi_sio"
    echo "    echo 0403 d6da > /sys/bus/usb-serial/drivers/ftdi_sio/new_id"
    exit 1
fi

echo "=== DS150E Baud Rate Scan ==="
echo "Device: $DEV"
echo

for baud in 9600 19200 38400 57600 115200 230400 460800; do
    # Configure port
    stty -F "$DEV" "$baud" raw -echo -echoe -echok cs8 -parenb -cstopb clocal cread \
        -crtscts -ixon -ixoff min 0 time 20 2>/dev/null

    # Flush
    dd if="$DEV" of=/dev/null bs=1024 count=1 iflag=nonblock 2>/dev/null

    # Send *20A (get device name) - safest command
    printf '*20A\r' > "$DEV"

    # Read response (2 sec timeout via VTIME=20)
    resp=$(dd if="$DEV" bs=256 count=1 2>/dev/null | tr -d '\r\n' | strings)

    if [ -n "$resp" ]; then
        echo "[+] $baud: RESPONSE: '$resp'"
        if echo "$resp" | grep -q 'CDP'; then
            echo
            echo "*** FOUND! Baud rate = $baud ***"
            echo "    Device: $resp"
            echo
            echo "Quick test:"
            echo "    stty -F $DEV $baud raw -echo cs8 clocal cread"
            echo "    echo -ne '*20A\r' > $DEV && cat $DEV"
            echo
            echo "Or run:"
            echo "    ./ds150e $DEV info"
            exit 0
        fi
    else
        echo "[ ] $baud: no response"
    fi
done

echo
echo "[!] No response at any baud rate."
echo "    Check:"
echo "    - DS150E powered? (USB LED on?)"
echo "    - lsusb | grep 0403"
echo "    - ls $DEV"
echo "    - Is another program holding the port? (fuser $DEV)"
