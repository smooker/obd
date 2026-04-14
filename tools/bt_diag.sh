#!/bin/bash
# bt_diag.sh — Bluetooth diagnostics for st
# Run as root: bash /chroot/claude/home/claude/work/obd/tools/bt_diag.sh

echo "=== BT Diagnostics — $(date) ==="

echo ""
echo "--- hciconfig ---"
hciconfig -a 2>&1

echo ""
echo "--- rfkill ---"
rfkill list 2>&1

echo ""
echo "--- bluetoothd process ---"
ps aux | grep bluetoothd | grep -v grep

echo ""
echo "--- bluetoothctl list ---"
timeout 3 bluetoothctl list 2>&1

echo ""
echo "--- bluetoothctl show ---"
timeout 3 bluetoothctl show 2>&1

echo ""
echo "--- btmgmt info ---"
timeout 3 btmgmt info 2>&1

echo ""
echo "--- btmgmt index-info 0 ---"
timeout 3 btmgmt --index 0 info 2>&1

echo ""
echo "--- /sys/class/bluetooth ---"
ls -la /sys/class/bluetooth/ 2>&1

echo ""
echo "--- /sys/class/bluetooth/hci0 ---"
ls -la /sys/class/bluetooth/hci0/ 2>/dev/null

echo ""
echo "--- hci0 sysfs info ---"
for f in address type uevent; do
    [ -f /sys/class/bluetooth/hci0/$f ] && echo "  $f: $(cat /sys/class/bluetooth/hci0/$f)"
done 2>/dev/null

echo ""
echo "--- USB BT device ---"
lsusb | grep -i 8087
lsusb -v -d 8087:0029 2>/dev/null | grep -E 'idVendor|idProduct|bcdDevice|iProduct|iManufacturer|iSerial' | head -10

echo ""
echo "--- kernel modules ---"
lsmod | grep -iE 'bt|bluetooth' 2>/dev/null || echo "(all built-in)"
zgrep -E '^CONFIG_BT' /proc/config.gz 2>/dev/null | grep '=y' | head -20

echo ""
echo "--- dbus bluetooth ---"
dbus-send --system --dest=org.bluez --print-reply / org.freedesktop.DBus.ObjectManager.GetManagedObjects 2>&1 | head -30

echo ""
echo "--- dmesg bluetooth (last 20) ---"
dmesg | grep -i bluetooth | tail -20

echo ""
echo "=== DONE ==="
