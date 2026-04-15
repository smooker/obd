#!/bin/bash
# usb_detach.sh — detach USB device from running QEMU VM
# Usage: bash usb_detach.sh [vendor:product]
# Default: 0403:d6da (DS150E / Autocom CDP+)

VID_PID="${1:-0403:d6da}"
VID="${VID_PID%%:*}"
PID="${VID_PID##*:}"
DEV_ID="usb_${VID}_${PID}"

QMP="/tmp/qemu-qmp.sock"
MON="/tmp/qemu-monitor.sock"

if [ -S "$MON" ]; then
    echo "device_del ${DEV_ID}" | socat - UNIX-CONNECT:"$MON"
    echo "[+] Detached ${DEV_ID}"
elif [ -S "$QMP" ]; then
    (
        echo '{"execute":"qmp_capabilities"}'
        sleep 0.3
        echo "{\"execute\":\"device_del\",\"arguments\":{\"id\":\"${DEV_ID}\"}}"
    ) | socat - UNIX-CONNECT:"$QMP"
    echo "[+] Detached ${DEV_ID}"
else
    echo "[!] No QEMU socket found"
fi
