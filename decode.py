#!/usr/bin/python3
"""decode.py — декодира usbmon text dump на Autocom CDP+ в четим формат.
Извлича Bulk OUT (host→dongle) и Bulk IN (dongle→host) данни и ги печата
като ASCII (протоколът е текстов: *XXX...\r)."""
import sys, re

f = sys.argv[1] if len(sys.argv) > 1 else '/home/claude-agent/work/autocom/autocom_20260406_172130.txt'

# usbmon line: tag ts type addr status len = hex...
# we want S Bo (host→) and C Bi (←dongle) with hex data
def parse_hex(parts):
    """Join hex words into ascii, replacing non-printables with '.'"""
    raw = ''.join(parts).replace(' ', '')
    out = bytearray()
    for i in range(0, len(raw), 2):
        try: out.append(int(raw[i:i+2], 16))
        except: pass
    return out

ftdi_status_seen = False
for line in open(f):
    p = line.split()
    if len(p) < 6: continue
    typ, addr = p[2], p[3]
    if '=' not in line: continue
    eq = p.index('=')
    hexparts = p[eq+1:]
    data = parse_hex(hexparts)

    if typ == 'S' and addr.startswith('Bo:'):
        # host → dongle (raw серийни данни)
        ts = int(p[1])
        try: txt = data.decode('ascii', errors='replace')
        except: txt = repr(data)
        print(f"{ts} → {txt!r}")
    elif typ == 'C' and addr.startswith('Bi:'):
        # dongle → host. ПЪРВИТЕ 2 байта са FTDI modem status — skip.
        if len(data) <= 2: continue
        payload = data[2:]
        ts = int(p[1])
        try: txt = payload.decode('ascii', errors='replace')
        except: txt = repr(payload)
        print(f"{ts}        ← {txt!r}")
