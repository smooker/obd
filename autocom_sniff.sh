#!/bin/bash
# autocom_sniff.sh — sniff на Autocom CDP+ (0403:d6da) през usbmon debugfs.
# Без tshark/libpcap. Output: text dump във формат на usbmon.
#
# Usage: sudo ./autocom_sniff.sh
#        Ctrl-C спира.
#
# Файлове в /tmp/:
#   autocom_<ts>.txt   — пълен usbmon text лог (само нашето device)
#   autocom_<ts>.raw   — суров binary payload (само BULK IN/OUT данни)

set -e

VID=0403
PID=d6da

[[ $EUID -ne 0 ]] && { echo "трябва root"; exit 1; }

modprobe usbmon 2>/dev/null || true
[[ ! -d /sys/kernel/debug/usb/usbmon ]] && {
    mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
}

LINE=$(lsusb | grep -i "$VID:$PID" || true)
[[ -z "$LINE" ]] && { echo "Autocom $VID:$PID не е намерен"; lsusb; exit 1; }

BUS=$(echo "$LINE" | awk '{print $2}' | sed 's/^0*//')
DEV=$(echo "$LINE" | awk '{print $4}' | tr -d ':' | sed 's/^0*//')
echo "Намерен: $LINE"
echo "Bus=$BUS Device=$DEV"

USBMON=/sys/kernel/debug/usb/usbmon/${BUS}u
[[ ! -r "$USBMON" ]] && { echo "няма $USBMON"; exit 1; }

TS=$(date +%Y%m%d_%H%M%S)
LOG=/tmp/autocom_${TS}.txt
echo "Лог: $LOG"
echo "Филтър: device :${DEV}: на bus $BUS"
echo "Cyk-ай в Win7 и натисни Ctrl-C когато си готов."
echo

# usbmon text формат (1u/2u): "URBtag timestamp event type address ..."
# address колоната е "URB_type:bus:device:endpoint", напр. "Bo:1:025:2"
# Филтрираме на ":025:" — внимание ако се смени device address при reset.
DEV_PAD=$(printf "%03d" "$DEV")
exec stdbuf -oL grep ":${DEV_PAD}:" "$USBMON" | tee "$LOG"
