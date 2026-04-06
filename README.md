# obd — Reverse engineering на Autocom CDP+ под Linux

Малък toolkit за snif-ване и анализ на USB трафика между оригиналния
Autocom (Delphi/DS150) Windows софтуер и **Autocom CDP+** dongle-а
(`USB ID 0403:d6da` — FTDI чип + STM32 вътре).

Целта: разчитане на ASCII текстовия протокол който STM32-ката говори,
така че да можем да правим OBD-II диагностика от Linux **без** Windows VM.

**Първи capture**: Mitsubishi ASX 1.8 DiD (4N13 diesel), по време на
центровка на дюзи (injector calibration codes).

🌐 **[obd.smooker.org](https://obd.smooker.org)** *(идва скоро)*

---

## Какво има тук

| Файл | Какво прави |
|------|-------------|
| [`sniff.c`](sniff.c) | C/libpcap USB sniffer — пълни payload-и, без truncation, optional pcap output за Wireshark |
| [`sniff_full.py`](sniff_full.py) | Python alternative — binary usbmon без libpcap dependency |
| [`autocom_sniff.sh`](autocom_sniff.sh) | Bash quick-look — text-mode usbmon (⚠️ truncated до 32 байта/packet) |
| [`decode.py`](decode.py) | Парсва usbmon text dump в четим `→/←` формат |
| [`ANALYSE.md`](ANALYSE.md) | Анализ на ASCII протокола: команди, отговори, формат |

## Как се прави capture

```bash
# 1. Build sniffer
gcc -O2 -Wall -o sniff sniff.c -lpcap

# 2. Стартирай ПРЕДИ да attach-неш USB-то в Win7 VM-та (за да хванеш init)
sudo ./sniff -w /tmp/init.pcap

# 3. В Win7: connect to vehicle → една чиста операция → disconnect

# 4. Ctrl-C тук
```

В реално време виждаш ASCII протокола:
```
17:21:12.034735 → '*203\r'
17:21:12.038111        ← '*0 1362\r'   (battery = 13.62 V)
17:21:12.051758 → '*20A\r'
17:21:12.056192        ← '*CDP+\r'
```

## Структура на протокола (накратко)

```
*<cmd>[_<arg>[_<arg>...]]\r          host → dongle
*<resp>[ <data>...]\r                dongle → host
```

Открити команди (виж [`ANALYSE.md`](ANALYSE.md) за пълен списък):

| Команда | Описание |
|---------|----------|
| `*200` / `*201` / `*20A` | get serial / firmware / device name |
| `*203` | battery voltage (mV/10) |
| `*60b` / `*60bc` | init handshake |
| `*668_<bus>_<rate>_<txid>_<rxid>_…` | configure CAN bus (напр. 500 kbps OBD-II) |
| `*608_21_<XX>` | read ECU parameter at index XX |
| `*608_18_…` | ECU init / session start |
| `*609_<bus>_<txid>_<rxid>_<dlc>_<data>` | send raw CAN frame |
| `*606_…` | send periodic / tester-present frame |

## Зависимости

- Linux kernel с `CONFIG_USB_MON=m` (стандартно навсякъде)
- `libpcap` с `usb` USE flag (Gentoo: `USE="usb" emerge libpcap`)
- Python 3.8+ (за helper-ите)

## Защо не tshark/Wireshark директно

Wireshark **може** да чете usbmon, но изисква `libpcap` build-нат с USB
support и dumpcap permissions. На много дистрибуции едното или другото
липсва по подразбиране. Нашият sniffer е standalone — 200 реда C, един
системен модул, без UI overhead.

## ⚠️ Disclaimer

> Reverse engineering за **interoperability и образователни цели**.
> Това не е affiliated с Autocom, Delphi, или който и да е OEM.
>
> CDP+ dongle-ът има релета които комутират различни автомобилни
> протоколи и захранване. Грешна команда може да pull-не пин на 12V
> където не трябва и да повреди dongle, OBD конектор или ECU. Replay
> само на byte-perfect capture-нати последователности. **Use at your
> own risk.**

## Roadmap

- [x] Sniff пълен USB трафик без truncation
- [x] Decode на ASCII протокола
- [x] Документация на познатите команди
- [ ] Чист init capture от cold start (без работеща кола)
- [ ] Bench replay tool — reproduce-ва capture-нат init
- [ ] Минимален Linux client: read VIN, read DTC, clear DTC
- [ ] Live data (PID 010C RPM, 010D speed, etc.)
- [ ] Поддръжка на K-line protocol-и (по-стари коли)

## Свързани проекти

- **MCP2515 CAN bridge** — 2× MCP2515 на RPi, MITM между ECU и KESS
  програматор за hardware-level CAN sniffing. Различен подход —
  вместо USB MITM, sit-ваме директно на физическия CAN bus.

## License

[GPL-3.0](LICENSE) — copyleft. Подобренията остават отворени.
