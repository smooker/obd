#!/bin/bash
# obd-go.sh — orchestrator: sniff + hotplug → libvirt USB passthrough
#
# Run as root, on the host (NOT inside chroot):
#   sudo /chroot/claude/home/claude-agent/work/obd/obd-go.sh
#
# Behaviour:
#   1. Sanity (root, usbmon, /dev/usbmonN, tmux, virsh, target VM up)
#   2. Build sniff if missing
#   3. Create captures/ if missing
#   4. Start tmux session "obd-capture" with sniff -w (waits for hotplug)
#   5. Watch for 0403:d6da appearance, then virsh attach-device → VM
#   6. Attach to tmux session
#
# Detach: Ctrl-B D    Re-attach: tmux a -t obd-capture
# Stop: Ctrl-C inside the session.
#
# Env overrides:
#   OBD_VM=name        target libvirt domain (default: win7_pro)
#   OBD_CONFIRM=1      ask y/N before virsh attach-device (default: immediate)

set -e

VID=0403
PID=d6da
VM_NAME="${OBD_VM:-win7_pro}"
CONFIRM="${OBD_CONFIRM:-0}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SNIFF="$SCRIPT_DIR/sniff"
SESSION=obd-capture

# === Sanity ===
[[ $EUID -ne 0 ]] && { echo "ERR: трябва root"; exit 1; }

if ! grep -q usbmon /proc/modules 2>/dev/null && [[ ! -e /dev/usbmon0 ]]; then
    modprobe usbmon 2>/dev/null || true
fi
[[ ! -e /dev/usbmon1 ]] && { echo "ERR: /dev/usbmon* липсват"; exit 1; }

command -v tmux  >/dev/null || { echo "ERR: tmux не е инсталиран"; exit 1; }
command -v virsh >/dev/null || { echo "ERR: virsh не е инсталиран"; exit 1; }
command -v gcc   >/dev/null || { echo "ERR: gcc не е инсталиран"; exit 1; }

if ! virsh dominfo "$VM_NAME" >/dev/null 2>&1; then
    echo "ERR: VM '$VM_NAME' не съществува (virsh dominfo)"
    echo "     override: OBD_VM=name $0"
    exit 1
fi

VM_STATE=$(virsh domstate "$VM_NAME" 2>/dev/null)
echo "VM '$VM_NAME': $VM_STATE"

# === Build sniff if needed ===
if [[ ! -x "$SNIFF" ]] || [[ "$SCRIPT_DIR/sniff.c" -nt "$SNIFF" ]]; then
    echo "Build sniff..."
    gcc -O2 -Wall -o "$SNIFF" "$SCRIPT_DIR/sniff.c" || exit 1
fi

mkdir -p "$SCRIPT_DIR/captures"

# === Existing tmux session? ===
if tmux has-session -t "$SESSION" 2>/dev/null; then
    echo "Има активна сесия '$SESSION', attach-вам..."
    exec tmux attach -t "$SESSION"
fi

# === Orchestration helper that runs inside tmux ===
ORCH=$(mktemp /tmp/obd-orch.XXXXXX.sh)
trap 'rm -f "$ORCH"' EXIT

cat >"$ORCH" <<ORCHEOF
#!/bin/bash
set +e
SNIFF="$SNIFF"
VM_NAME="$VM_NAME"
VID=$VID
PID=$PID
CONFIRM=$CONFIRM

echo "=========================================="
echo "  obd capture orchestrator"
echo "=========================================="
echo "VM: \$VM_NAME"
echo
PRE=\$(lsusb | grep -i "\$VID:\$PID" || echo 'NOT plugged')
echo "Pre-state: \$PRE"
echo

# 1. Start sniff in wait mode (background)
"\$SNIFF" -w &
SNIFF_PID=\$!
echo "sniff PID: \$SNIFF_PID"
echo

# 2. Wait for hotplug via udev event hook (no polling)
echo "→ plug-ни dongle сега; passthrough ще се активира автоматично"

# Бърза проверка ако dongle вече е там (avoid waiting for next event)
if lsusb | grep -qi "\$VID:\$PID"; then
    echo "✓ already plugged"
else
    # Block on udev kernel event matching VID/PID
    udevadm monitor --udev --property --subsystem-match=usb 2>/dev/null |
    awk -v VID="\$VID" -v PID="\$PID" '
        /^UDEV.*add/  { in_add=1; vid=0; pid=0; next }
        /^UDEV.*remove/ { in_add=0; next }
        in_add && /^ID_VENDOR_ID=/ { split(\$0,a,"="); if (a[2]==VID) vid=1 }
        in_add && /^ID_MODEL_ID=/  { split(\$0,a,"="); if (a[2]==PID) pid=1 }
        /^\$/ { if (in_add && vid && pid) { print "MATCH"; exit 0 }; in_add=0 }
    '
    if ! kill -0 \$SNIFF_PID 2>/dev/null; then
        echo "WARN: sniff exited преди hotplug match"
        exit 1
    fi
fi

DETECTED=\$(lsusb | grep -i "\$VID:\$PID" || echo "?")
echo
echo "✓ detected: \$DETECTED"
echo

# 3. Optional confirm
if [[ "\$CONFIRM" == "1" ]]; then
    read -p "attach to VM '\$VM_NAME'? [y/N] " ans
    [[ "\$ans" != "y" && "\$ans" != "Y" ]] && {
        echo "skipped attach. sniff остава активен."
        wait \$SNIFF_PID
        exit 0
    }
fi

# 4. Generate USB hostdev XML and attach
USB_XML=\$(mktemp /tmp/obd-usb.XXXXXX.xml)
cat >"\$USB_XML" <<XMLEOF
<hostdev mode='subsystem' type='usb' managed='yes'>
  <source>
    <vendor id='0x\$VID'/>
    <product id='0x\$PID'/>
  </source>
</hostdev>
XMLEOF

echo "→ virsh attach-device \$VM_NAME --live ..."
if virsh attach-device "\$VM_NAME" "\$USB_XML" --live 2>&1; then
    echo "✓ attach OK"
else
    echo "✗ attach FAILED — провери VM state, dongle ownership"
fi
rm -f "\$USB_XML"

echo
echo "→ capture в ход. Ctrl-C за стоп."
wait \$SNIFF_PID
ORCHEOF
chmod +x "$ORCH"

# === Launch in tmux ===
tmux new-session -d -s "$SESSION" "$ORCH; rm -f $ORCH; echo; echo 'session done. press any key.'; read -n1"
echo "Стартирано в tmux session '$SESSION'."
echo "Attach: tmux a -t $SESSION    Detach: Ctrl-B D"
echo
exec tmux attach -t "$SESSION"
