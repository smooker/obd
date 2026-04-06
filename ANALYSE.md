# Autocom CDP+ — анализ на USB протокола

**Hardware**: Autocom CDP+ (`0403:d6da`) — FTDI чип + STM32 микроконтролер
вътре, който говори custom **ASCII текстов** протокол върху bulk pipe-а на FTDI.

**Capture среда**: Linux host, Win7 VM с оригиналния Autocom софтуер,
USB passthrough → host snif-ва между VM и dongle-а през `usbmon`.

---

## Какво открихме (от лога 2026-04-06 17:21)

Capture-нат върху работеща кола, ~119k реда usbmon text dump. Не съдържа
чист init (capture стартиран по средата на работа). Първото взаимодействие
е query на dongle-а:

```
*203\r          ← '*0 1362\r'        battery voltage = 13.62 V
*20A\r          ← '*CDP+\r'          device name
*200\r          ← '*100251\r'        serial number?
*201\r          ← '*1622\r'          firmware version
*60b\r          ← '*121\r'           ?
*60bc\r         ← '*121\r'           ?
*668_0_500_7E0_7E8_000_01C_02_3E…    CAN config (TRUNCATED — виж по-долу)
```

## Структура на ASCII протокола

```
*<cmd>[_<arg>[_<arg>...]]\r
```

- Стартов символ: `*` (0x2A)
- Терминатор: `\r` (0x0D)
- Разделител на полета: `_` (0x5F)
- Първите 3 hex/дeц цифри след `*` са код на командата

### Открити команди (host → dongle)

| Код | Брой | Описание |
|-----|-----:|----------|
| `*200` | 2 | get serial number → `*100251` |
| `*201` | 2 | get firmware version → `*1622` |
| `*203` | 33 | get battery voltage → `*<flag> <mV/10>` |
| `*20A` | 1 | get device name → `*CDP+` |
| `*60b` | 2 | init handshake → `*121` |
| `*60bc` | 2 | init handshake → `*121` |
| `*668_<bus>_<rate>_<txid>_<rxid>_…` | 1 | configure CAN bus (500 kbps, OBD-II IDs) |
| `*606_…` | 1 | send CAN frame, e.g. tester present `02 3E 02 …` |
| `*608_18_00_FF_00` | — | ECU init? → `*88 0` |
| `*608_21_<XX>` | 7901 | **read ECU parameter at index XX** → `*97 XX <6 bytes>` |
| `*608_10_<XX>` | — | start session / write? |
| `*609_<bus>_<txid>_<rxid>_<dlc>_<XX>` | 2 | send raw CAN frame |

### Отговори (dongle → host)

| Префикс | Значение |
|---------|----------|
| `*0 NNNN\r` | OK + value (voltage poll) |
| `*1 NNNN\r` | OK + value (variant) |
| `*97 XX <bytes>\r` | response to `*608_21_XX` (param read) |
| `*88 0\r` | ack за init? |
| `*80 NNN\r` | ack за `*608` команди |
| `*121\r` | ack за `*60b/60bc` |
| `*255\r` | ? (често след `*608`) |

## Какво стана с диюзите

Логът е capture-нат докато потребителят е центровал дюзи. След init phase
има стотици `*608_21_<XX>` reads с различни XX. Отговорите често съдържат
ASCII фрагменти като `/6Ruk`, `/6Rv.` — типично за **IMA/IMI калибрационни
кодове на дюзи** (16-символни alphanumeric). Pattern-ът от циклично четене
на `_46`, `_02`, `_04`, `_46`, `_02` и т.н. предполага итерация по 4
цилиндъра.

## ⚠️ TRUNCATION — критичен проблем за replay

Скриптът `autocom_sniff.sh` чете `/sys/kernel/debug/usb/usbmon/1u`, който
е **text формат**. Kernel модулът `drivers/usb/mon/mon_text.c` има hardcoded
лимит:

```c
#define DATA_MAX 32   /* за "u" формат */
#define DATA_MAX  4   /* за "t" формат — още по-зле */
```

→ всеки USB packet с payload > 32 байта ни идва **отрязан**, без означение.
Виждаме го по `length=61 cap=32` в lcap полето.

**Конкретни жертви** в текущия лог:
- `*668_0_500_7E0_7E8_000_01C_02_3E` — реалните 61 байта станаха 32, не
  знаем последните 29 байта от CAN config-а.
- Всяка `*606`/`*608` команда с дълъг payload (> ~28 байта content)
  е потенциално отрязана.

**Решение**: binary usbmon (`/dev/usbmonN` или MON_IOCX_GET ioctl-и), който
чете от `mon_bin.c` ring buffer без този лимит. Виж `sniff_full.py`.

## Защо не tshark/libpcap

`libpcap` на Gentoo с USE `usb` би трябвало да поддържа usbmon, но `tshark`
казва "There is no device named usbmon1". Вероятна причина: dumpcap
permissions или wireshark пакета без USB capture support. Не сме копали
там — binary usbmon през Python е по-простия път.

---

## Следваща капитулация (TODO)

1. **Чист init capture**: пуснем `sniff_full.py` ПРЕДИ да attach-нем USB-то
   в Win7 VM-та. Тогава виждаме целия cold init: USB enumerate → FTDI setup
   → Autocom handshake → "connect to vehicle" → "read VIN" → disconnect.
2. **Replay test**: на bench (без кола) изпратим записания init дословно
   през libusb или ftdi_sio към dongle-а. Очакваме същите релета да щракат,
   същите отговори да идват. Битов сравняване с capture.
3. **Заместване на VM**: като знаем целия handshake, пишем малък Linux
   client (Python + libusb) който прави read VIN, read DTC, read live data.
4. **Опасни команди**: `*668` (CAN config), `*606..608` setup команди —
   тези променят състоянието на релетата. НЕ пускаме случайни байтове.
   Само replay на познати, byte-for-byte capture-нати последователности.

## Файлове в директорията

| Файл | Описание |
|------|----------|
| `autocom_sniff.sh` | text-mode usbmon sniffer (truncated, използвай само за бърз поглед) |
| `sniff_full.py` | binary usbmon sniffer (пълни payload-и) |
| `decode.py` | парсва text dump в четим формат `→ '*cmd\r'` / `← '*resp\r'` |
| `autocom_20260406_172130.txt` | първи capture (TRUNCATED, без чист init) |
| `ANALYSE.md` | този файл |
