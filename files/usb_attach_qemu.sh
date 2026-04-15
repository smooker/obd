#!/bin/bash
# usb_attach.sh — attach USB device to running QEMU VM via QMP
# Usage: bash usb_attach.sh [vendor:product]
# Default: 0403:d6da (DS150E / Autocom CDP+)
#
# QEMU must be started with:
#   -qmp unix:/tmp/qemu-qmp.sock,server,nowait
#   -device qemu-xhci,id=xhci
# or:
#   -monitor unix:/tmp/qemu-monitor.sock,server,nowait

VID_PID="${1:-0403:d6da}"
VID="${VID_PID%%:*}"
PID="${VID_PID##*:}"

QMP="/tmp/qemu-qmp.sock"
MON="/tmp/qemu-monitor.sock"

if [ -S "$MON" ]; then
    echo "[+] Using QEMU monitor socket"
    echo "device_add usb-host,vendorid=0x${VID},productid=0x${PID},id=usb_${VID}_${PID}" | socat - UNIX-CONNECT:"$MON"
    echo "[+] Attached ${VID}:${PID}"
elif [ -S "$QMP" ]; then
    echo "[+] Using QMP socket"
    (
        echo '{"execute":"qmp_capabilities"}'
        sleep 0.3
        echo "{\"execute\":\"device_add\",\"arguments\":{\"driver\":\"usb-host\",\"vendorid\":$((16#$VID)),\"productid\":$((16#$PID)),\"id\":\"usb_${VID}_${PID}\"}}"
    ) | socat - UNIX-CONNECT:"$QMP"
    echo "[+] Attached ${VID}:${PID}"
else
    echo "[!] No QEMU socket found. Fallback: add to QEMU command line:"
    echo "    -device usb-host,vendorid=0x${VID},productid=0x${PID}"
fi
