#!/bin/bash
# usb_attach_libvirt.sh — attach USB device to running libvirt VM
# Usage: bash usb_attach_libvirt.sh <vm-name> [vendor:product]
# Default: 0403:d6da (DS150E / Autocom CDP+)

VM="${1:?Usage: $0 <vm-name> [vendor:product]}"
VID_PID="${2:-0403:d6da}"
VID="${VID_PID%%:*}"
PID="${VID_PID##*:}"

virsh attach-device "$VM" /dev/stdin <<EOF
<hostdev mode='subsystem' type='usb' managed='yes'>
  <source>
    <vendor id='0x${VID}'/>
    <product id='0x${PID}'/>
  </source>
</hostdev>
EOF

echo "[+] Attached ${VID}:${PID} → ${VM}"
