#!/usr/bin/python3
"""sniff_full.py — пълен USB capture за Autocom CDP+ (0403:d6da)
   през binary usbmon (/dev/usbmon<N>). Без libpcap, без truncation.

Output: /tmp/autocom_full_<ts>.bin (raw binary usbmon stream) +
        /tmp/autocom_full_<ts>.txt (decoded text view).

Usage: sudo /usr/bin/python3 sniff_full.py
       Ctrl-C спира.

ВНИМАНИЕ: за да хванем INIT, пусни този скрипт ПРЕДИ да си attach-нал
USB-то в Win7 VM-та. После в Win7: connect to vehicle → една операция
→ disconnect. Тогава Ctrl-C тук."""

import os, sys, struct, fcntl, ctypes, time, subprocess, re

VID, PID = 0x0403, 0xd6da

def find_device():
    out = subprocess.check_output(["lsusb"]).decode()
    for line in out.splitlines():
        m = re.search(r"Bus (\d+) Device (\d+): ID ([0-9a-f]+):([0-9a-f]+)", line)
        if m and int(m.group(3),16)==VID and int(m.group(4),16)==PID:
            return int(m.group(1)), int(m.group(2))
    return None, None

def main():
    if os.geteuid() != 0:
        sys.exit("трябва root")
    bus, dev = find_device()
    if bus is None:
        sys.exit(f"Autocom {VID:04x}:{PID:04x} не е намерен. lsusb не го вижда.\n"
                 "Това може да е добре — пусни скрипта ПРЕДИ да attach-неш USB-то в VM-та.\n"
                 "Тогава махни тази проверка или re-стартирай след attach.")
    print(f"Намерен Autocom: bus={bus} device={dev}")

    # Опитваме binary usbmon device. Ако /dev/usbmonN липсва — пишем грешка.
    devpath = f"/dev/usbmon{bus}"
    if not os.path.exists(devpath):
        print(f"{devpath} не съществува. Опитваме debugfs binary път...")
        # Като fallback — четем text от debugfs БЕЗ truncation през 't' format
        # (1t format има пълни данни, 1u само 32 байта)
        sys.exit(f"Решение: modprobe usbmon → ls /dev/usbmon*. Ако пак няма,\n"
                 "kernel може да не е build-нат с CONFIG_USB_MON_BIN.")

    ts = time.strftime("%Y%m%d_%H%M%S")
    binfile = f"/tmp/autocom_full_{ts}.bin"
    txtfile = f"/tmp/autocom_full_{ts}.txt"
    print(f"Binary → {binfile}")
    print(f"Text   → {txtfile}")
    print("Cyk-ай в Win7 (connect → operation → disconnect), после Ctrl-C тук.\n")

    # MON_IOCQ_RING_SIZE = 0x9205, MON_IOCT_RING_SIZE = 0x4004920a, MON_IOCX_MFETCH = 0xc0109207
    MON_IOCT_RING_SIZE = 0x40049204  # set ring size to allow large packets
    fd = os.open(devpath, os.O_RDONLY)
    # Try to set ring size to ~4MB (allows packets up to ~64KB each)
    try:
        fcntl.ioctl(fd, MON_IOCT_RING_SIZE, 4*1024*1024)
    except Exception as e:
        print(f"warn: ring size ioctl: {e}")

    # bf unbuffered: на kill (auto-logout, OOM, panic) нищо не се губи
    bf = open(binfile, "wb", buffering=0)
    tf = open(txtfile, "w", buffering=1)  # line-buffered

    # Binary mon header is 48 bytes; data follows. Format:
    #  u64 id, u8 type, u8 xfer_type, u8 epnum, u8 devnum, u16 busnum, s8 flag_setup, s8 flag_data,
    #  s64 ts_sec, s32 ts_usec, s32 status, u32 length, u32 len_cap,
    #  u8 setup[8] (or s/iso descriptor), s32 interval, s32 start_frame, u32 xfer_flags, u32 ndesc
    HDR = struct.Struct("<Q B B B B H b b q i i I I 8s i i I I")
    assert HDR.size == 64, HDR.size

    XFER = {0: "ISO", 1: "INT", 2: "CTL", 3: "BLK"}
    TYPE = {ord('S'): "S", ord('C'): "C", ord('E'): "E"}

    n = 0
    try:
        while True:
            hdr = os.read(fd, 64)
            if len(hdr) < 64: break
            (uid, typ, xfer, ep, dn, bn, fs, fd_, sec, usec,
             stat, length, lcap, setup, intv, sfr, xflg, nd) = HDR.unpack(hdr)
            data = os.read(fd, lcap) if lcap else b""
            bf.write(hdr); bf.write(data)
            if dn != dev or bn != bus:
                continue
            xt = XFER.get(xfer & 3, "?")
            tt = TYPE.get(typ, "?")
            dirn = "IN" if (ep & 0x80) else "OUT"
            epn = ep & 0x7f
            ts_us = sec*1_000_000 + usec
            if xfer == 3:  # bulk
                # FTDI IN payload: skip first 2 modem status bytes
                show = data[2:] if (dirn == "IN" and len(data) >= 2) else data
                ascii_txt = show.decode('ascii', errors='replace')
                arrow = "→" if dirn == "OUT" else "←"
                line = f"{ts_us} {tt} BLK {arrow} ep{epn} len={length} cap={lcap} {ascii_txt!r}\n"
            elif xfer == 2:  # control
                line = f"{ts_us} {tt} CTL setup={setup.hex()} len={length} data={data.hex()}\n"
            else:
                line = f"{ts_us} {tt} {xt} ep{epn} len={length} {data.hex()}\n"
            tf.write(line)
            tf.flush()
            n += 1
            if n % 100 == 0:
                print(f"\r{n} packets", end="", flush=True)
    except KeyboardInterrupt:
        print(f"\nспрян. {n} пакета.")
    finally:
        bf.close(); tf.close(); os.close(fd)

if __name__ == "__main__":
    main()
