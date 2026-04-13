#!/bin/bash
# go.sh — Autocom CDP+ capture launcher (run as root on host, не от chroot)
#
# Usage:
#   sudo ./go.sh                  # interactive menu
#   sudo ./go.sh cold_init        # named session (skip menu)
#   sudo ./go.sh injectors
#
# Sessions:
#   cold_init   — пуска ПРЕДИ да attach-неш USB-то във Win VM (cold handshake)
#   vin         — connect → read VIN → disconnect
#   idle        — connect → 30s idle → disconnect (TesterPresent observation)
#   injectors   — нормалната работа по дюзите
#   dtc         — read + clear DTCs
#   live        — live data window 15s
#   custom      — питай за име
#
# Output: ~/obd_session_YYYYMMDD/<NN>_<name>.{pcap,txt}
# Винаги чете автоматично lsusb за 0403:d6da.

set -e

VID=0403
PID=d6da
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SNIFF="$SCRIPT_DIR/sniff"

# === safety: must be root ===
if [[ $EUID -ne 0 ]]; then
    echo "ERR: трябва да си root (sudo $0)"
    exit 1
fi

# === safety: must NOT be in chroot ===
if [[ ! -d /sys/kernel/debug ]] || ! mountpoint -q /sys/kernel/debug 2>/dev/null; then
    echo "INFO: debugfs не е mount-нат, опитвам..."
    mount -t debugfs none /sys/kernel/debug 2>/dev/null || {
        echo "ERR: не мога да mount-на debugfs. Извън chroot ли си?"
        exit 1
    }
fi

# === ensure usbmon module ===
if ! lsmod 2>/dev/null | grep -q '^usbmon'; then
    echo "INFO: зареждам usbmon kernel module..."
    modprobe usbmon || { echo "ERR: modprobe usbmon failed"; exit 1; }
fi

# === verify usbmon debugfs visible ===
if [[ ! -d /sys/kernel/debug/usb/usbmon ]]; then
    echo "ERR: /sys/kernel/debug/usb/usbmon липсва (kernel build issue?)"
    exit 1
fi

# === verify sniff binary ===
if [[ ! -x "$SNIFF" ]]; then
    echo "INFO: $SNIFF липсва, build-вам..."
    if ! command -v gcc >/dev/null; then
        echo "ERR: няма gcc на host-а. Build от chroot и копирай."
        exit 1
    fi
    gcc -O2 -Wall -o "$SNIFF" "$SCRIPT_DIR/sniff.c" -lpcap || {
        echo "ERR: build failed (липсва libpcap?)"
        exit 1
    }
fi

# === find dongle ===
echo "Търся Autocom $VID:$PID..."
LSUSB_LINE=$(lsusb | grep -i "$VID:$PID" || true)
if [[ -z "$LSUSB_LINE" ]]; then
    echo
    echo "⚠ Autocom $VID:$PID НЕ Е НАМЕРЕН в lsusb."
    echo
    echo "Това е нормално АКО искаш да правиш cold_init capture —"
    echo "стартирай sniff СЕГА, после attach USB във Win VM."
    echo
    read -p "Продължи без device филтър (cold init mode)? [y/N] " ans
    [[ "$ans" != "y" && "$ans" != "Y" ]] && exit 0
    DEV_INFO="cold-init mode (no device filter, listen on usbmon1)"
else
    BUS=$(echo "$LSUSB_LINE" | awk '{print $2}' | sed 's/^0*//')
    DEV=$(echo "$LSUSB_LINE" | awk '{print $4}' | tr -d ':' | sed 's/^0*//')
    DEV_INFO="bus=$BUS device=$DEV  ($LSUSB_LINE)"
fi
echo "→ $DEV_INFO"
echo

# === pick session name ===
SESSION_NAME="${1:-}"
if [[ -z "$SESSION_NAME" ]]; then
    echo "Избери сценарий:"
    echo "  1) cold_init    — sniff PREDI USB attach във Win VM"
    echo "  2) vin          — connect → read VIN → disconnect"
    echo "  3) idle         — connect → 30s idle (TesterPresent)"
    echo "  4) injectors    — нормалната работа по дюзите"
    echo "  5) dtc          — read + clear DTC"
    echo "  6) live         — live data window 15s"
    echo "  7) custom       — задай си име"
    echo
    read -p "Избор [1-7]: " choice
    case "$choice" in
        1) SESSION_NAME=cold_init ;;
        2) SESSION_NAME=vin ;;
        3) SESSION_NAME=idle ;;
        4) SESSION_NAME=injectors ;;
        5) SESSION_NAME=dtc ;;
        6) SESSION_NAME=live ;;
        7) read -p "Име на сесията: " SESSION_NAME ;;
        *) echo "невалиден избор"; exit 1 ;;
    esac
fi
[[ -z "$SESSION_NAME" ]] && { echo "няма име"; exit 1; }

# === setup session dir ===
SESSION_DIR=~/obd_session_$(date +%Y%m%d)
mkdir -p "$SESSION_DIR"

# auto-number: брой съществуващи + 1
NEXT=$(printf "%02d" "$(ls "$SESSION_DIR"/*.pcap 2>/dev/null | wc -l | awk '{print $1+1}')")
PCAP="$SESSION_DIR/${NEXT}_${SESSION_NAME}.pcap"
LOG="$SESSION_DIR/${NEXT}_${SESSION_NAME}.txt"

echo
echo "================================================================"
echo "  Session: ${NEXT}_${SESSION_NAME}"
echo "  Device:  $DEV_INFO"
echo "  pcap:    $PCAP"
echo "  log:     $LOG"
echo "================================================================"
echo

# === session-specific instructions ===
case "$SESSION_NAME" in
    cold_init)
        echo "1. (sniff ще се пусне след секунда)"
        echo "2. SEGA attach USB-то във Win VM"
        echo "3. Отвори Autocom софтуера"
        echo "4. НЕ свързвай към кола — стой 10-15s"
        echo "5. Ctrl-C тук"
        ;;
    vin)
        echo "1. (sniff пуска)"
        echo "2. В Autocom: Connect to vehicle → Read VIN → Disconnect"
        echo "3. Ctrl-C"
        ;;
    idle)
        echo "1. (sniff пуска)"
        echo "2. Connect → стой 30 секунди в главния прозорец → Disconnect"
        echo "3. Ctrl-C"
        ;;
    injectors)
        echo "1. (sniff пуска)"
        echo "2. Изпълни нормалната работа по дюзите"
        echo "3. Ctrl-C след като приключи"
        ;;
    dtc)
        echo "1. (sniff пуска)"
        echo "2. Read DTCs → Clear DTCs"
        echo "3. Ctrl-C"
        ;;
    live)
        echo "1. (sniff пуска)"
        echo "2. Отвори live data window (RPM/speed/temps) ~15s"
        echo "3. Ctrl-C"
        ;;
esac
echo
read -p "[Enter] за старт..." _

# === run sniff ===
# tee към log + pcap наред в цялостния pcap output
"$SNIFF" -w "$PCAP" 2>&1 | tee "$LOG" || true

echo
echo "Capture спрян."
ls -la "$PCAP" "$LOG" 2>/dev/null
echo
echo "Готов за следваща сесия:  sudo $0 <name>"
