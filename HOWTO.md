# HOWTO — Autocom CDP+ on Linux

A practical guide for sniffing, decoding and (eventually) talking to the
Autocom CDP+ / Delphi DS150E diagnostic dongle from a Linux host.

> Status: living document. Reflects what we've actually run and verified,
> not what the marketing brochure claims.

---

## 0. What you need

### Hardware
- An **Autocom CDP+** (or Delphi DS150E, or one of the many clones).
  USB ID `0403:d6da` — FTDI front-end, STM32 inside.
- An OBD-II vehicle to talk to. Anything with a 16-pin connector.
- Optional: a bench 12 V supply with current limit, for safe poking
  outside the car.

### Software
- Linux kernel with `usbmon` (`CONFIG_USB_MON=y` or as a module).
- `libpcap` built **with USB support** (Gentoo: `USE="usb"`).
- `gcc`, `make`, `python3` ≥ 3.10.
- Optional but useful: `wireshark`, `dev-python/freetype-py`,
  `dev-libs/freetype` (for the visualization tools).

### Privileges
- Read access to `/sys/kernel/debug/usb/usbmon/*` — by default root only.
  Either run sniffers as root, or `chmod +r` the relevant `Nu` node.
- For raw USB control transfers later: a udev rule giving your user
  group write access to `/dev/bus/usb/<bus>/<dev>`.

---

## 1. Find the dongle

```sh
lsusb | grep 0403:d6da
# Bus 001 Device 025: ID 0403:d6da Future Technology Devices International, Ltd
```

Note **bus** (`001`) and **device** (`025`). Bus number maps to
`/sys/kernel/debug/usb/usbmon/1u` (text) or `usbmon1` (binary). Device
number is what you'll filter on.

The kernel will bind `ftdi_sio` automatically and create
`/dev/ttyUSB<N>`. **You don't want that** — the Autocom protocol is not
a serial terminal stream, it's a custom command/response protocol over
the FTDI bulk pipes. Either:

- leave `ftdi_sio` attached and ignore `ttyUSB`, opening the device via
  bulk transfers anyway, or
- `modprobe -r ftdi_sio` and talk straight to the FTDI bulk endpoints.

For passive sniffing this doesn't matter — usbmon sees everything either
way.

---

## 2. Sniffing — three flavours

### 2.1 Quick & dirty: usbmon text mode

```sh
cat /sys/kernel/debug/usb/usbmon/1u > capture.txt
```

Filter to your device:

```sh
grep ' 1:025:' capture.txt > asx.txt    # bus 1, device 25
```

**Catch**: usbmon text mode truncates payloads at **32 bytes**
(`#define DATA_MAX 32` in `drivers/usb/mon/mon_text.c`). Anything longer
is silently cut off — and a lot of Autocom responses are longer.
Useful only for quick "is it talking?" checks.

`autocom_sniff.sh` wraps this and is fine for first-look.

### 2.2 Full payload: libpcap (binary usbmon)

`sniff.c` is ~200 lines of C against libpcap. It opens `usbmon1` in
binary mode (no truncation), filters by device address, dumps hex
payloads, and optionally writes a `.pcap` for Wireshark.

```sh
gcc -O2 -Wall -o sniff sniff.c -lpcap
sudo ./sniff                    # stdout only
sudo ./sniff -w asx.pcap        # + pcap file for Wireshark
```

The sniffer auto-discovers the Autocom dongle via `lsusb` (looks for
`0403:d6da`) and filters by its device address. If the dongle is not
present at start, it falls back to listening on `usbmon1` without a
device filter — useful for catching the cold-init enumerate.

Open `asx.pcap` in Wireshark — set `Decode As → USB → FTDI`.

### 2.3 No libpcap: pure Python binary usbmon

`sniff_full.py` opens `/dev/usbmon1` directly and parses the binary
struct. Same data, no native deps. Slower, but useful when you can't
build C.

```sh
sudo /usr/bin/python3 sniff_full.py 1 25 > asx.bin
```

---

## 3. The protocol, in one paragraph

The host sends ASCII commands; the dongle answers with ASCII responses:

```
*<cmd>[_<arg>[_<arg>…]]\r          host  → dongle
*<resp>[ <data>…]\r                dongle → host
```

- Start byte: `*` (0x2A)
- Terminator: `\r` (0x0D)
- Field separator: `_` (0x5F)
- Everything is plain ASCII hex, lowercase. Yes, even the CAN frames —
  they're wrapped inside this text envelope.

That's the entire framing. No checksums on the outer envelope; some
inner payloads (e.g. `*606` periodic CAN slots) carry their own 2-hex
checksum at the end.

A captured exchange looks like:

```
→ *200\r
← *1 100251\r       # serial number
→ *201\r
← *1 1622\r         # firmware version
→ *203\r
← *1 1366\r         # battery = 13.66 V
```

See `ANALYSE.md` for the full command list and the ECU parameter map
extracted from a Mitsubishi ASX 1.8 DiD capture.

---

## 4. Decoding a capture

`decode.py` turns a raw usbmon dump into a readable `→/←` transcript:

```sh
/usr/bin/python3 decode.py asx.txt > asx.decoded
```

It strips FTDI's 2-byte modem-status prefix on every IN bulk packet,
re-frames on `*…\r` boundaries, and prints direction arrows.

For binary captures:

```sh
/usr/bin/python3 decode.py --binary asx.bin > asx.decoded
```

---

## 5. Talking back — the dangerous part

> ⚠️ **Read this twice.** The CDP+ has internal relays that switch
> automotive bus pins between protocols and, in some modes, **route
> +12 V onto pins that should be signal**. Sending a wrong init
> sequence can damage the dongle, the OBD connector, or the ECU. The
> safe rule for now: **only replay byte-perfect captures from a known-
> good Windows session**. Don't synthesize commands until you've read
> the full init sequence and understand the relay state machine.

We're not there yet in the public code — the replay tool is on the
roadmap. When it lands it will:

1. Take a recorded init sequence from a clean cold-boot capture.
2. Open the FTDI bulk endpoints (libusb).
3. Send the host frames in order, wait for the matching `*…\r`
   responses, abort hard on any deviation.

Until then: capture, decode, document. Don't transmit.

---

## 6. Tooling roadmap (and where this HOWTO grows)

- [ ] Cold-boot init capture (Win VM USB attach happens too late in our
      current dump — we miss the first ~50 frames).
- [ ] `replay.c` — minimal libusb replayer with strict response check.
- [ ] `cdp.c` — stand-alone Linux client. Goals: read VIN, read DTCs,
      clear DTCs, live PIDs, battery voltage. No GUI yet.
- [ ] K-line / ISO 9141 support for older cars (the dongle has the
      hardware, the protocol bytes are different).
- [ ] Mockups → real Qt5 frontend, sharing the same Terminus PCF font
      stack we're prototyping in `media/`.
- [ ] PDF report generator (`gen_report.py` already does a first pass
      via pslib).

---

## 7. Legal

Reverse engineering for **interoperability and educational purposes**,
EU Software Directive 2009/24/EC Art. 6. Not affiliated with Autocom,
Delphi, or any vehicle OEM. No proprietary code is redistributed —
only protocol observations, which are facts and not copyrightable.
