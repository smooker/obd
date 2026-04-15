# DS150E / Autocom CDP+ — Firmware Analysis

Анализ на firmware-а от `Autocom 2021.11 Software/Firmware/` без изпращане на
команди към устройството. Чист статичен анализ на Intel HEX файлове.

---

## Хардуерни варианти

### CDP (по-стар) — FIRMCDP.H86

| Поле | Стойност |
|------|----------|
| Файл | `FIRMCDP.H86` (675KB source, 234KB binary) |
| MCU | **НЕ е ARM** — pattern `FA 03 00 01` с `FA`/`EA` opcodes |
| Архитектура | Вероятно Infineon C166/XC167 (16-bit automotive) или custom packed |
| Адресен диапазон | 0x00000000 — 0x00060000 |
| Firmware версия | 1621 (`firmware_version_vci=1621` от Firm.info) |

### VCI+ (по-нов, нашият) — OBD_VCI+.hex

| Поле | Стойност |
|------|----------|
| Файл | `OBD_VCI+.hex` (767KB source, 511KB binary) |
| MCU | **STM32** ARM Cortex-M |
| Initial SP | 0x20017100 (92KB SRAM) |
| Reset vector | 0x0803BA4D |
| Flash | 511KB (0x08000000 — 0x0807FF04) |
| SRAM | 92KB → STM32F103 high-density или STM32F2xx |
| Build дата | Jun 21 2016, 08:39:52 |
| Build тип | Release |
| Firmware версия | 1622 (`firmware_version_vci_plus=1622`) |
| Интерфейси | USB (FTDI FT232R), Bluetooth, SD-card, CAN, KWP2000, J1587/J1939 |

### Помощни файлове

| Файл | Размер | Base addr | Описание |
|------|--------|-----------|----------|
| `FLASH.H86` | 8KB (2928B binary) | 0x100000 | RAM-loaded flash programming stub |
| `1lev.hex` | 257B | ? | Read Protection Level 1 settings |
| `2lev.hex` | 257B | ? | Read Protection Level 2 settings |
| `eOBDCDP.H86` | 110KB | ? | eOBD вариант firmware |
| `eOBD_VCI+.hex` | 192KB | ? | eOBD VCI+ вариант |
| `Firm.info` | 198B | — | Версии на всички варианти |

---

## Firmware версии (от Firm.info)

```
firmware_version_vci=1621
firmware_version_vci_plus=1622
firmware_version_weasy_plus=1622
firmware_version_eobd_vci=221
firmware_version_eobd_vci_plus=121
firmware_version_eobd_weasy_plus=121
```

---

## Извлечени протоколни команди (VCI+ firmware string dump)

### Идентификация на устройството

| Стринг | Контекст |
|--------|----------|
| `*CDP+` | Име на устройството |
| `*USB` | Тип връзка — USB |
| `*Bluetooth` | Тип връзка — Bluetooth |
| `*Unknown` | Тип връзка — неизвестна |
| `*Main app` | Основно приложение |
| `Build target: (Release)` | Build конфигурация |
| `Jun 21 2016` | Дата на компилация |
| `08:39:52` | Час на компилация |

### Self-test команди (`*918x`)

Вътрешна диагностика на DS150E, не изисква свързана кола.

| Команда | Отговори |
|---------|----------|
| `*918A` | `*918A_OK` / `*918A_FAIL` |
| `*918B` | `*918B_OK` / `*918B_FAIL` |
| `*918C` | `*918C_OK` / `*918C_FAIL` |
| `*918D` | `*918D_OK` |
| `*918E` | `*918E_OK` |
| `*918F` | `*918F_OK` / `*918F_FAIL` |

918D и 918E нямат FAIL стринг — или винаги минават, или грешката е различна.

### Bluetooth контрол

| Стринг | Описание |
|--------|----------|
| `*Wireless com enabled` | BT включен |
| `*Wireless com disabled` | BT изключен |
| `*No response from module` | BT модул не отговаря |
| `BT MAC` | MAC адрес на BT |
| `Entering AT mode` | BT модул AT command mode |
| `Invalid response from module` | Невалиден отговор от BT |
| `No response from module` | BT модул timeout |

### Misc firmware команди

| Команда/Стринг | Описание |
|----------------|----------|
| `*9999` | Вероятно bootloader entry (виж по-долу) |
| `*F1F5` / `*F0F5` | ? (mode switch, може BT on/off) |
| `*F1F` / `*F9F/` | ? |
| `*911` / `*912` | ? |
| `*No parameter selected` | Няма избран ECU параметър |
| `*PID list erased!` | PID списък изчистен |
| `*Enabled` / `*Disabled` | Toggle отговори |
| `*Not implemented!` | Неимплементирана функция |
| `*Not yet supported by firmware` | Непoддържано от тази версия |
| `*Invalid argument` | Невалиден аргумент |
| `*Invalid command!` | Непозната команда |
| `*ArbId error` | CAN Arbitration ID грешка |
| `*Wrong cable!` | Грешен OBD кабел |
| `*Unsupported voltage` | Напрежение извън обхват |
| `Test relay:` | Тест на релетата |

### CAN frame шаблони (хардкоднати)

```
608h31 B8 00 00#
608h31 B8 01 03#
608h31 BA 01 03#
608h31 B9 01 03 02#
608h31 B9 01 03 00 00#
608h31 BB 01 03 00 00 00 06 46 22 00 BA#
608h32 B8 01 03#
```

Формат: `<cmd>h<byte1> <byte2> ...#` — вътрешно представяне на CAN frames.

### `*609` разширен формат (пример от firmware)

```
*609MH_41_81_9B&4A_&4_&5_&6_&7+2C_&#02
```

Семантика: `&` = field reference, `+` = конкатенация, `#` = терминатор. Това е
шаблон за конструиране на CAN frame от multiple ECU responses.

### KWP2000 / ISO-9141 грешки

| Стринг | Описание |
|--------|----------|
| `*KW1 & KW2 error` | KWP2000 keyword bytes mismatch |
| `*Inverse KW2 error` | Инверсен KW2 не съвпада |
| `*No answer to init!` | ECU не отговаря на init |
| `*No response to ECU init` | ECU init timeout |
| `*No respons to 5baud!` | ISO 5-baud init провал |
| `*No answer to 5baud init!` | ISO 5-baud timeout |
| `*Timeout waiting for KW1&KW2` | KWP2000 keyword timeout |
| `*No response to IC init` | IC init timeout |
| `*Keep alive message error` | Tester Present провал |
| `*Error sending keepalive!` | Keepalive изпращане неуспешно |
| `*No/wrong answer to init!` | Init грешен/липсващ отговор |
| `*No answer to login!` | Login timeout |
| `*! Reinit failed!` | Повторна инициализация неуспешна |
| `*! Larm i main` | Аларма в main loop (шведски!) |
| `*! Erroneous block! (PID 192-253)` | Грешен PID блок |
| `*! Erroneous block! (PID 254)` | Грешен PID 254 |

Забележка: `*! Larm i main` е на **шведски** ("Alarm in main") — Autocom е
шведска компания (Trollhättan).

### J1587/J1939 протокол

| Стринг | Описание |
|--------|----------|
| `*1939 73` | J1939 reference |
| `send1587ProprietaryReq` | J1587 proprietary request |
| `searchSpecificProprietary:` | Търсене на proprietary PID |
| `repeatRequest greater_than maxNoOfRepeatRequest` | Retry limit |

### SD-card

| Стринг | Описание |
|--------|----------|
| `SD-card inserted` | Карта засечена |
| `SD-card not inserted` | Няма карта |
| `CardBlockSize=` | Block size |
| `RdBlockLen=` | Read block length |
| `MaxWrBlockLen=` | Max write block length |

### USB Host

| Стринг | Описание |
|--------|----------|
| `USBH device not connected` | USB Host — няма устройство |
| `USB:d=8,o=5,b=160:16a,16g,16a,16a#,c6,c6` | USB мелодия/buzzer нотация |

### Звукови сигнали (buzzer)

Firmware-ът съдържа музикални нотации за buzzer:

```
USB:d=8,o=5,b=160:16a,16g,16a,16a#,c6,c6          — USB свързан
FrecStart:d=8,o=6,b=160:c,d,a#5,d,2f               — Стартиране
FrecFailed:d=8,o=6,b=160:c,d,a#5,d,p,c4,...         — Грешка
Watchdog:d=16,o=5,b=160:a,p,g,p,a,p,e,p,c,p,e,p,8a4 — Watchdog
```

Формат: [RTTTL](https://en.wikipedia.org/wiki/Ring_Tone_Text_Transfer_Language)
(Ring Tone Text Transfer Language) — Nokia формат за мелодии!

---

## Bootloader анализ

### Bootloader entry (хипотеза, НЕ тествана)

От firmware strings на последователни адреси:

```
0x08011098: 'Usart1 <-> Usart3!'
0x080110AC: 'Entering Boot Loader.. '
0x080110C4: 'Bye bye!'
```

Веднага след `"Bye bye!"` на адрес `0x080110CC`:

```
EF BE AD DE   → 0xDEADBEEF (magic marker)
F0 FF 00 20   → 0x2000FFF0 (SRAM top, bootloader SP)
0C ED 00 E0   → 0xE000ED0C (SCB->AIRCR — System Reset register)
```

### Хипотеза за bootloader последователност

1. Host изпраща `*9999` по serial
2. Firmware отпечатва `"Entering Boot Loader.."`
3. `"Usart1 <-> Usart3!"` — FTDI serial (USART3) се bridge-ва към STM32 ROM bootloader (USART1)
4. STM32 ROM bootloader е достъпен през FTDI serial
5. `stm32flash -r dump.bin /dev/ttyUSB0` — четене на flash
6. При изход: `"Bye bye!"`

### FLASH.H86 — Flash programming stub

- 2928 байта, base address 0x100000
- Зарежда се в SRAM от bootloader-а
- Съдържа erase/program рутини за flash pages
- Ползва се при firmware update, не при четене

### Read Protection (RDP)

`1lev.hex` и `2lev.hex` съдържат RDP level настройки:

- **Level 0**: Flash четим, write/erase свободни
- **Level 1**: Flash нечетим през debug/bootloader, но firmware се изпълнява нормално. Erase = unlock (губи се целия firmware)
- **Level 2**: Перманентна защита, JTAG/SWD disabled, bootloader read disabled

Ако DS150E е с RDP Level 1+, `stm32flash` **не може** да чете flash-а.

### ⚠️ ВНИМАНИЕ

**НЕ изпращай `*9999` или други bootloader команди** без:
1. Потвърждение че RDP е Level 0
2. Backup на текущия firmware (чрез друг метод)
3. Разбиране на пълната bootloader sequence

Грешна команда може да:
- Заключи устройството в bootloader mode
- Trigger RDP Level 1 mass erase
- Направи устройството неработоспособно

---

## Извлечено от Autocom 2021.11 DLL-и (2026-04-15)

Анализ на `VCILinkNET.dll`, `VCI_HW_native.dll`, `wcl.dll`, `FTD2XX_NET.dll`.

### Baudrate detection

Autocom **не ползва фи��сиран baud rate**! Прави автоматично baudrate detection:

```
Send bauddetect
Response from second bauddetect
BootApp bauddetect OK
No response from bauddetect
FAILED to write bauddetect
```

Два режима на serial port:
- `Config port for normal application_cm3` — нормална работа
- `Config port for bootmode_cm3` — bootloader mode

Baudrate стойности намерени в DLL-ите:

| Baud | VCILinkNET | VCI_HW_native | Бележка |
|------|-----------|---------------|---------|
| 9600 | 1 | — | |
| **19200** | **13** | **10** | Най-много references — вероятен default |
| 38400 | 5 | 3 | |
| 57600 | 2 | 2 | |
| 115200 | 6 | 1 | |
| 460800 | 1 | 1 | Вероятно high-speed mode |

**Хипотеза**: Init на 19200 → bauddetect → switch на 115200 ил�� 460800.

FTDI комуникация:
```
FTDI port {0} opened
FTDI port {0} closed
FTDI Read 0 bytes (status{0})
FTDI Read Failed
COM{0} (USB)
VCP port {0} opened          — Virtual COM Port
Start bootmode by FTDI bitbang DONE
```

Ползва **FTDI D2XX** driver (директен USB), не COM port по default.
`FTDI bitbang` за bootloader entry — toggle DTR/RTS/CTS за hardware reset.

### Нови star команди (от DLL strings)

#### `*2xx` — Device info
| Команда | Описание |
|---------|----------|
| `*200` | Serial number |
| `*201` | Firmware version |
| `*203` | Voltage |
| `*204_` | С аргумент — ? |
| `*205` | ? |
| `*20A` | Device name |
| `*20E_` | С аргумент — ? |

#### `*4xx` — ? (нова група)
| Команда | Описание |
|---------|----------|
| `*400` | ? |
| `*405` | ? |
| `*406` | ? |
| `*407` | ? |
| `*408` | ? |
| `*409` | ? |
| `*40A` | ? |

#### `*5xx`
| Команда | Описание |
|---------|----------|
| `*599` | ? |

#### `*6xx` — CAN / bus commands
| Команда | Описание |
|---------|----------|
| `*605_` / `*605E` | ? |
| `*606C1_41` | Periodic msg, CAN ID 0x41 |
| `*606C1_62` | Periodic msg, CAN ID 0x62 |
| `*606LEOBD11` | LEOBD 11-bit CAN |
| `*606LEOBD29` | LEOBD 29-bit CAN (extended) |
| `*607` | ? |
| `*608_00_00_03` | ? variant |
| `*608_21_08_00_00_00` | Param read с extra args |
| `*608_31` | **UDS RoutineControl ($31)** |
| `*608_43_FF` | UDS ClearDTC? или response |
| `*60A` | ? |
| `*60C` | ? |
| `*650` / `*651` / `*652` | ? |
| `*65B_6E_250` | ? (250=baud? CAN timing?) |
| `*66A_0_250_7DF_000_000_0#` | CAN config 250kbps 11-bit broadcast |
| `*66A_0_500_7DF_000_000_0#` | CAN config 500kbps 11-bit broadcast |
| `*66A_1_250_18DB33F1_00000000_000_0#` | CAN config 250kbps **29-bit** |
| `*66A_1_500_18DB33F1_00000000_000_0#` | CAN config 500kbps **29-bit** |
| `*6DA` / `*6DB` / `*6DC` / `*6dd` | ? |

**`*66A` vs `*668`**: `*66A` е simplified CAN config (без payload, с `#` term).
`*668` е пълен CAN config с embedded payload + checksum.

#### `*9xx` — System / bootloader
| Команда | Описание |
|---------|----------|
| `*913` | ? |
| `*915` | Bootloader entry (confirmed: `SetBootMode by software command (*915)`) |
| `*91E_name_{0}` | Set device name |
| `*91E_num` | Set device number |
| `*923` | ? |
| `*928_{X}_{X}_{X}_{X}` | 4 hex args — firmware update block? |
| `*983_{0}` | ? |
| `*984` / `*986` / `*987` | ? |
| `*989_{0}{1}` | ? |
| `*999` | Reset |

### SecurityAccess ($27) — от DLL strings

```
#! In CMD_SEND_KEY, error in creating key process! Is seed correct?
Question to car with appended security key has not appeared!
*** (runSgwProtocol) The unlock procedure not executed
--- (openSgwChannel) Check status to verify that it's unlocked.
--- (openSgwChannel) Status is unlocked!!!
-----Security Access answer:
-----Security Access bnrG:
.writeUnlock->:
.writeUnlock<-:
<Seed>
</Seed>
```

- SGW = Security GateWay protocol
- Seed е в **XML** формат (`<Seed>...</Seed>`)
- `CMD_SEND_KEY` — функция за изчисляване на key от seed
- `bnrG` — вероятно identifier на seed-key алгоритъма
- `writeUnlock` — запис на unlock команда

**TODO**: decompile VCILinkNET.dll с mono/ilspycmd за пълния seed→key алгоритъм.

### Mitsubishi init sequences

```
MITSUBISHI1SEQ
MITSUBISHI2SEQ
MITSUBISHI3SEQ
MITSUBISHI4SEQ
MITSUBISHI5SEQ
```

5 различни init sequence-а за Mitsubishi — вероятно за различни протоколи:
1. CAN 500kbps 11-bit (нашият ASX 4N13)
2. CAN 250kbps 11-bit
3. CAN 29-bit (extended)
4. KWP2000 / ISO 14230
5. ISO 9141 / 5-baud init

### Bluetooth (от wcl.dll)

```
BTHENUM
\Device\BtPort
Connection exists.
Connection is active.
Connection was rejected by device.
Connection was terminated by user.
Device is not connected.
PTosBtHSPAPI.dll
HARDWARE\DEVICEMAP\SERIALCOMM
SYSTEM\CurrentControlSet\Enum\BTHENUM\{00001124-0000-1000-8000-00805f9b34fb}
Software\Microsoft\BluetoothAuthenticationAgent
```

BT ползва Windows Bluetooth stack (BTHENUM) и SPP (Serial Port Profile) — `\Device\BtPort`.
На Linux еквивалентът е rfcomm bind.

---

## Бъдещи стъпки

1. **Дизасемблиране** на `OBD_VCI_plus.bin` с `arm-none-eabi-objdump` — пълен control flow на command parser
2. **Идентификация на точния STM32** — по vector table размер и peripheral registers
3. **CAN driver анализ** — от firmware binary, намиране на bxCAN registers (0x40006400/0x40006800)
4. **Bluetooth модул** — идентификация (HC-05? RN42?) по AT command strings
5. **Cold boot capture** — с usbmon, виждаме пълния init от Autocom софтуера, включително bootloader detection
