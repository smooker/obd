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

## ECU param map — Mitsubishi ASX 2014 4N13 AWD (MMC_ASX_4N13)

Кола: Mitsubishi Motor Company ASX 2014, 1.8 DiD diesel (4N13), AWD.
Capture от един "read all parameters" pull в Autocom UI-ят (5-6 секунди,
итерация през всички известни индекси) — **56 уникални param индекса**,
от които виждаме отговор за повечето.

Колоната "raw" е байтовете без префикса `*97 <idx>` (т.е. полезните данни).
Колоната "guess" е първоначална хипотеза — **трябва потвърждение от
оператора на колата**.

### Група 01-19 — Core ECU sensors / status (предполагам)

| idx | raw bytes | guess |
|-----|-----------|-------|
| 02 | `54 7F 49 61 C0` | ? |
| 03 | `00 05 CC 86` | ? (5*256+204=1484, 134=?) |
| 04 | `D5 00 33 19 26` | ? (variable — last byte changes 38→39) |
| 08 | `00 00 D0 00 7F` | ? |
| 09 | `B3 0F 00 00` | ? |
| 10 | `41 40 41 1B A0` | ? |
| 12 | `42 42 00 00 00` | ? (`42 42` = "BB"?) |
| 13 | `05 CC 86 00 00` | същия pattern като 03 |
| 14 | `FF FA` | малко bytes |
| 15 | `00 00 00 00 28 00` | `28` = 40 dec |
| 16 | `3C BA 78 28 35 00` | ? |
| 17 | `EA 64 64 00 00` | `64 64` = 100,100 (proc?) |
| 18 | `43 01 00 00 00` | ? |
| 19 | `00 00 00 00 00` | празно |

### Група 24-28

| idx | raw bytes | guess |
|-----|-----------|-------|
| 24 | `6D 3F 37 73 2E 00 EB` | ASCII частично: `m?7s.` |
| 26 | `80 1A` | ? |
| 27 | `00 00 00 00 00 00` | празно |
| 28 | `00 01 28 28 28 28` | `28`=40 (повтарящ се) |

### Група 46-5F — частично ASCII (вероятно injector codes)

| idx | raw bytes | guess |
|-----|-----------|-------|
| 46 | `2F 36 52 75 6B` | **`/6Ruk`** ASCII (injector code part) |
| 47 | `00 00 00 00` | празно |
| 49 | `00 00 00 00 5F` | `_` |
| 4A | `0E 38 3D 10` | ? |
| 4B | `75 14` | `u` + 0x14 |
| 4C | `23 23 48 88 02` | `##H` |
| 4F | `37 37 2F 48` | **`77/H`** |
| 58 | `00 00 00 00 00` | празно |
| 59 | `00 31 00` | `\0 1 \0` |
| 5B | `41 41 41 36 00` | **`AAA6\0`** |
| 5D | `05 CC 86 00 00` | същи bytes като 03 и 13 |
| 5F | `00 00 00 00` | празно |

### Група 74

| idx | raw bytes | guess |
|-----|-----------|-------|
| 74 | `F3 CC 00 00 FF 60 30` | ? |

### Група A0-A8 — calibration lookup table (X→Y pairs)

Pattern: `<X1> 121 <X2> 121` — две (X, 121) двойки на индекс. **Y винаги 121**
(константа — може би температура °C, може би idle setpoint). X-овете растат
монотонно — типично за **calibration map** (RPM/position/load axis).

| idx | pair 1 | pair 2 |
|-----|--------|--------|
| A0 | (10, 121) | (20, 121) |
| A1 | (30, 121) | (44, 121) |
| A3 | (70, 121) | (80, 121) |
| A4 | (90, 121) | (100, 121) |
| A7 | (150, 121) | (160, 121) |
| A8 | `00 00` | празно/край |

(A2, A5, A6 truncated в текущия лог)

### Група B0-B9 — **injector quantity correction (10 work points × 2 values)**

Това е **най-уверената** ни хипотеза — pattern-ът е прекалено типичен за
дизелов injector calibration, и е capture-нат точно по време на центровка.

Format: 4 байта = 2× big-endian uint16 = `(value1, value2)`. Стойностите са
в диапазона ~440-491, което е типично за injector qty correction units.

| idx | (val1, val2) | возможно work point |
|-----|--------------|---------------------|
| B0 | (491, 482) | idle / leerlauf |
| B1 | (479, 469) | … |
| B2 | (474, 474) | … |
| B3 | (465, 462) | … |
| B4 | (464, 458) | … |
| B5 | (467, 456) | … |
| B6 | (458, 459) | … |
| B7 | (468, 462) | … |
| B8 | (440, 452) | … |
| B9 | (457, 450) | full load? |

10 work points × 2 values = вероятно **current vs target** correction за
всеки work point, или **тест-импулс 1 vs тест-импулс 2**.

### Група BE-BF

| idx | raw bytes | guess |
|-----|-----------|-------|
| BE | `02 DC 00 00 00` | `02 DC`=732 |
| BF | `94 EA EC F1 F7 18` | truncated, още байтове следват |

### Truncated в текущия лог (нужен нов capture)

`01`, `11`, `4E`, `51`, `A2`, `A5`, `A6`, `BF` — отговорите им са били
> 32 байта и са рязани от usbmon text формат.

---

## Какво стана с диюзите

Логът е capture-нат докато потребителят е центровал дюзи. След init phase
има стотици `*608_21_<XX>` reads с различни XX. Отговорите често съдържат
ASCII фрагменти като `/6Ruk`, `/6Rv.` — типично за **IMA/IMI калибрационни
кодове на дюзи** (16-символни alphanumeric). Pattern-ът от циклично четене
на `_46`, `_02`, `_04`, `_46`, `_02` и т.н. предполага итерация по 4
цилиндъра.

## Декодиране на `*606` — periodic CAN message slot

В лога имаме само **един** `*606`, и той е truncated (37 → 32 байта):

```
*606B001_7DF_02_3E_02_00_00_00_0[??]
```

Реконструкция:

| Поле | Стойност | Значение |
|------|----------|----------|
| `*606` | команда | configure **periodic message slot** |
| `B001` | slot ID | вътрешен идентификатор (вероятно slot index в hex) |
| `7DF` | CAN ID | OBD-II **broadcast** (хваща всеки ECU) |
| `02 3E 02 00 00 00 00 00` | CAN payload (8 bytes) | UDS Tester Present (виж долу) |

Payload-ът `02 3E 02 00 00 00 00 00` е **учебникарски UDS Tester Present
keepalive** (ISO 14229):

- `02` — ISO-TP single frame header, length=2
- `3E` — UDS service 0x3E = `TesterPresent`
- `02` — sub-function `suppressPosRspMsgIndicationBit` (no reply expected)
- `00 00 00 00 00` — padding до 8 байта (CAN frame изисква DLC=8)

Това е стандартния "I'm still here" пинг, който dongle-ът трябва да изпраща
на всеки ~2 секунди, за да не timeout-не диагностичната сесия в ECU-то.
След като host-ът го конфигурира веднъж с `*606`, dongle-ът сам поема
тази периодика — host-ът не повтаря. Това обяснява защо в нашия лог има
само **един** `*606` въпреки многочасовата сесия.

### Реконструиран format

```
*606<slot4>_<canid3>_<b1>_<b2>_<b3>_<b4>_<b5>_<b6>_<b7>_<b8>_<chk2>\r
```

Дължинна сметка:

```
*  6 0 6  B 0 0 1  _  7 D F  _  X X  _ ... (8 bytes × 3 chars + 7 underscores) ... _  N N  \r
1   3      4         1   3   1   2  _   2   _   2   _   2   _   2   _   2   _   2   1  2   1
                                       (2+1)*8 = 24 chars data + underscores
```

Total: `4 (cmd) + 4 (slot) + 1 (_) + 3 (canid) + 1 (_) + 23 (8 bytes
underscored) + 1 (_) + 2 (chk) + 1 (\r) = ~37 chars` ✓

Това потвърждава 2-знаков hex **checksum trailer**.

Липсващите ни 5 байта от truncation-а са: последния byte (`00`) + `_` +
2 hex digits checksum + `\r`.

### Какъв е checksum-ът?

Не можем да го извадим от един пример. Със следващия (нетruncated) capture
ще съберем 5-6 различни `*606`/`*608` команди и ще пробваме:

| Кандидат | Тест |
|----------|------|
| Sum mod 256 | `sum(bytes) & 0xFF` |
| XOR | `reduce(operator.xor, bytes, 0)` |
| CRC-8 / Maxim | стандартен `0x31` polynomial |
| CRC-8 / J1850 | OBD-II CAN стандарт |
| 2's complement | `(-sum) & 0xFF` |
| Включва ли `*` и `\r`? | проверка със / без |

Brute force върху 5 примера ще го реши за минути.

## Аналогична хипотеза за `*668` (CAN bus config)

`*668_0_500_7E0_7E8_000_01C_02_3E…` (length 61, cap 32) изглежда
конфигурира CAN bus със следните полета:

| Поле | Стойност | Значение |
|------|----------|----------|
| `0` | bus index | bus 0 (един от няколко) |
| `500` | bitrate | **500 kbps** (стандарт за модерни OBD-II коли) |
| `7E0` | TX ID | engine ECU request (physical addressing) |
| `7E8` | RX ID | engine ECU reply |
| `000` | filter? | може би addressing mode (functional/physical) |
| `01C` | ? | timing param? P2/P2* timeout? |
| `02 3E …` | ? | вероятно начало на initial session payload |

29-те липсващи байта могат да съдържат:
- ISO-15765 timing parameters (STmin, BS, P2, P2*)
- TX/RX timeout-и
- Допълнителни CAN ID-та за други ECU-та
- Padding byte (00 vs FF vs AA)
- Краен `_NN\r` checksum по аналогия с `*606`

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

## Firmware анализ (2026-04-14)

### Два варианта hardware

**FIRMCDP.H86 (CDP — по-старият):**
- НЕ е ARM — pattern `FA 03 00 01` с `FA`/`EA` opcodes
- Вероятно Infineon C166/XC167 (16-bit automotive MCU) или custom packed формат
- 234KB, адресен диапазон 0x00000000 - 0x00060000
- firmware_version_vci=1621

**OBD_VCI+.hex (VCI+ — по-новият, нашият):**
- **STM32** ARM Cortex-M
- SP=0x20017100 (92KB SRAM), Reset=0x0803BA4D
- 511KB flash, build Jun 21 2016, Release
- firmware_version_vci_plus=1622
- Интерфейси: USB (FTDI FT232R), Bluetooth, SD-card, CAN, KWP2000, J1587/J1939

### Нови команди от firmware string dump

**Self-test серия `*918x`** (вътрешна диагностика, без кола):

| Команда | Отговор | Описание |
|---------|---------|----------|
| `*918A` | `*918A_OK` / `*918A_FAIL` | Self-test A |
| `*918B` | `*918B_OK` / `*918B_FAIL` | Self-test B |
| `*918C` | `*918C_OK` / `*918C_FAIL` | Self-test C |
| `*918D` | `*918D_OK` | Self-test D |
| `*918E` | `*918E_OK` | Self-test E |
| `*918F` | `*918F_OK` / `*918F_FAIL` | Self-test F |

**Bluetooth контрол:**
- `*Wireless com enabled` — BT включен
- `*Wireless com disabled` — BT изключен

**CAN frame шаблони (хардкоднати в firmware):**
```
608h31 B8 00 00#
608h31 B8 01 03#
608h31 BA 01 03#
608h31 B9 01 03 02#
608h31 B9 01 03 00 00#
608h31 BB 01 03 00 00 00 06 46 22 00 BA#
608h32 B8 01 03#
```

**`*609` разширен формат:**
```
*609MH_41_81_9B&4A_&4_&5_&6_&7+2C_&#02
```
Използва `&` за field reference, `+` за конкатенация, `#` за терминатор.

**Други команди:**

| Команда | Описание |
|---------|----------|
| `*9999` | ? (test/reset?) |
| `*F1F5` / `*F0F5` | ? (mode switch?) |
| `*911` / `*912` | ? |
| `*No parameter selected` | няма избран ECU параметър |
| `*PID list erased!` | PID списък изтрит |
| `*Enabled` / `*Disabled` | toggle отговор |

**Грешки:**
- `*KW1 & KW2 error` — KWP2000 keyword mismatch
- `*No answer to init!` / `*No response to ECU init` — ECU timeout
- `*No respons to 5baud!` — ISO 5-baud init fail
- `*Timeout waiting for KW1&KW2` — KWP2000 timeout
- `*Keep alive message error` — Tester Present fail
- `*ArbId error` — CAN arbitration ID error
- `*Wrong cable!` — грешен OBD кабел
- `*Unsupported voltage` — напрежение извън обхват

### Autocom 2021.11 Software reverse engineering

**Пароли:**
- `Autocom186` — SQL Server password (VehicleSelection.dll)
- `kwH2eae7Rpshm7S` — SQL CE password (Text.dll)

**Файлови формати:**
- `.acz` — ZIP контейнер с AES-криптирани файлове + GZip компресия (Core.Crypto.Lib.dll)
- `.sdf` — SQL Server Compact, `encryption mode=ppc2003 compatibility`
- `Scan.mdb` — Access DB (некриптиран), vehicle profiles

**Vehicle profiles за Mitsubishi:**
- `9mit_engine_4n14_000a` — 4N14/4N13 двигател (ASX/Outlander)
- `9mitengine4n14_0008` — по-стара версия
- Outlander profiles: ABS, AC, gearbox, meter, SRS, ETACS, AWC, KOS/IMMO

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
