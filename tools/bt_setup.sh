#!/bin/bash
# bt_setup.sh — DS150E Bluetooth setup/teardown
#
# scan/pair: ръчно от bluetoothctl (интерактивен mode)
# bind/unbind: този скрипт (root)
#
# Usage:
#   As smooker (interactive):
#     bluetoothctl → scan on → pair <MAC> → trust <MAC> → quit
#
#   As root:
#     bt_setup.sh bind <MAC>     — rfcomm bind + chroot device
#     bt_setup.sh unbind         — release rfcomm + remove chroot device
#     bt_setup.sh status         — show state
#     bt_setup.sh hci-up         — bring up hci0 (firmware load + bluetoothd restart)

CHROOT="/chroot/claude"
RFCOMM_DEV="/dev/rfcomm0"
RFCOMM_CHROOT="$CHROOT/dev/rfcomm0"
MAC_FILE="/tmp/ds150e_bt_mac"

case "${1:-status}" in

bind)
    MAC="${2}"
    if [ -z "$MAC" ]; then
        [ -f "$MAC_FILE" ] && MAC=$(cat "$MAC_FILE")
    fi
    if [ -z "$MAC" ]; then
        echo "Usage: $0 bind <MAC>"
        echo "  First pair from bluetoothctl:"
        echo "    bluetoothctl → scan on → pair <MAC> → trust <MAC>"
        exit 1
    fi

    echo "=== Binding rfcomm0 to $MAC ==="
    echo "$MAC" > "$MAC_FILE"

    # rfcomm bind
    if [ -e "$RFCOMM_DEV" ]; then
        echo "[+] $RFCOMM_DEV already exists"
    else
        rfcomm bind 0 "$MAC" 1
        sleep 1
    fi

    if [ ! -c "$RFCOMM_DEV" ]; then
        echo "[!] $RFCOMM_DEV not created"
        exit 1
    fi
    echo "[+] $RFCOMM_DEV ready"
    ls -la "$RFCOMM_DEV"

    # chroot device node
    if [ -e "$RFCOMM_CHROOT" ]; then
        echo "[+] $RFCOMM_CHROOT already exists"
    else
        MAJOR=$(stat -c '%t' "$RFCOMM_DEV")
        MINOR=$(stat -c '%T' "$RFCOMM_DEV")
        mknod "$RFCOMM_CHROOT" c $((16#$MAJOR)) $((16#$MINOR)) 2>/dev/null
        chmod 666 "$RFCOMM_CHROOT"
        echo "[+] Created $RFCOMM_CHROOT"
    fi

    echo ""
    echo "Test from chroot:"
    echo "  ds150e /dev/rfcomm0 info"
    ;;

unbind)
    echo "=== Unbinding rfcomm0 ==="
    rm -f "$RFCOMM_CHROOT" 2>/dev/null
    rfcomm release 0 2>/dev/null
    echo "[+] Done"
    ;;

hci-up)
    echo "=== Bringing up hci0 ==="

    # USB re-enumerate if needed
    if hciconfig hci0 2>/dev/null | grep -q "DOWN"; then
        echo "[+] USB re-enumerate..."
        echo 0 > /sys/bus/usb/devices/1-14/authorized 2>/dev/null
        sleep 1
        echo 1 > /sys/bus/usb/devices/1-14/authorized 2>/dev/null
        sleep 3
    fi

    # Bring up
    hciconfig hci0 up 2>/dev/null
    sleep 3

    # Restart bluetoothd
    rc-service bluetooth restart
    sleep 2

    # Check
    echo ""
    hciconfig hci0 | head -4
    echo ""
    echo "Now run: bluetoothctl → scan on"
    ;;

status)
    echo "=== DS150E Bluetooth Status ==="
    echo ""
    echo "--- hci0 ---"
    hciconfig hci0 2>/dev/null | head -6 || echo "not available"
    echo ""
    echo "--- rfcomm ---"
    rfcomm -a 2>/dev/null || echo "no bindings"
    echo ""
    echo "--- saved MAC ---"
    [ -f "$MAC_FILE" ] && cat "$MAC_FILE" || echo "none"
    echo ""
    echo "--- chroot device ---"
    ls -la "$RFCOMM_CHROOT" 2>/dev/null || echo "not present"
    ;;

*)
    echo "Usage: $0 <command>"
    echo ""
    echo "  hci-up          Bring up BT (firmware load + bluetoothd restart)"
    echo "  bind <MAC>      Bind rfcomm0 + create chroot device"
    echo "  unbind           Release rfcomm0"
    echo "  status           Show state"
    echo ""
    echo "Scan/pair is interactive — run as smooker:"
    echo "  bluetoothctl → scan on → pair <MAC> → trust <MAC> → quit"
    ;;

esac
