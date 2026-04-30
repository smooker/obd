# obd — Project Rules

> **Stage 1** — sniff & analyse phase. Принципите по-долу важат за този
> етап. Когато минем към replay/active client (Stage 2), някои ще се
> ревизират (особено #6 passive-only).

## Език и зависимости

**САМО C.** Никакъв Python, никакъв shell за основната работа.

- Sniffer, decoder, replay, client — **C**
- `sniff_full.py`, `decode.py`, `gen_report.py` — legacy/scratch, не са source of truth
- `go.sh` — допустим само като thin launcher (root checks, kernel module load)

## Зависимости

- **БЕЗ libpcap.** Чете `/dev/usbmonN` директно (binary mon format).
  - Защо: libpcap често е build-нат без USB support → invisible bug,
    rebuild е скъпо. Direct read елиминира dependency.
- **БЕЗ tcpdump/Wireshark.** Не ни трябва GUI на този етап.
- gcc, libc — толкова.

## Output формат

- **Binary** (`/tmp/autocom_<ts>.bin`): суров mon stream + payload
- **Text** (`/tmp/autocom_<ts>.txt`): ASCII transcript с `→` / `←` стрелки,
  `'*cmd\r'` формат
- Auto-numbering и sub-sessions — в launcher (по-късно)

## tmux е задължителен

Всеки capture се пуска **в tmux session**, не директно от shell-а.

- `go.sh` създава/attach-ва tmux session и стартира sniff вътре
- root shell-ът на st има auto-logout → дълги интерактивни capture-и
  умират по средата
- tmux оцелява logout, мрежови disconnect, случайно затваряне на терминал
- detach с `Ctrl-B D`, връщане с `tmux a -t obd`
- НЕ разчитаме на буфериране/flush hack-ове за оцеляване — tmux е
  правилното решение

## Kernel изисквания

- `CONFIG_USB_MON=y` (built-in, не module — ползваме директно `/dev/usbmonN`)
- debugfs може да не е mount-нат — `/dev/usbmonN` е независим
- root за read

## Capture философия

- **Само passive sniff.** Никога не send-ваме байтове от наш код преди да
  имаме чист cold-init capture **и** разбран checksum алгоритъм.
- **Един capture за цялата сесия.** Не комбинираме сценарии = не значи
  отделни файлове! smooker не може да кара кола И да пише по клавиатурата —
  пуска capture веднъж, прави цялата работа, спира накрая. Сегментирането
  по сценарии става **офлайн при анализа**, по timestamps. По-малко PTP
  на колата.
- Cold init = sniff пуска **преди** USB attach във Win VM.

## Структура на kompromiti (известни от 2026-04-06)

- `*668` CAN config — труncated, нужни 29 байта от ISO-15765 timing
- `*606` periodic slot — нужен пълен payload с checksum trailer
- 8 ECU param индекса труncated: `01, 11, 4E, 51, A2, A5, A6, BF`
- Cold init — никога capture-нат

## Запомнено вчера / днес

- libpcap на st е build-нат без USE=usb (USE флаг добавен после, не
  rebuild-нат). `tcpdump -D | grep usb` → нула. **Не разчитаме на libpcap.**
  - **TODO investigation:** имаше нещо въртеливо в libpcap dep дървото —
    USB support беше зад някакъв друг trigger. Да се разнищи защо
    `equery u libpcap` показва `+ + usb` но build-нат binary няма
    `pcap_create("usbmon1")` функционалност. Пробата с rebuild
    (`emerge -1 libpcap`) и дали `tcpdump -D` ще покаже usb интерфейси —
    отделна сесия, не блокира Stage 1.
- usbmon е built-in в kernel-а на st (не module). debugfs mount-нат,
  `/dev/usbmon0..6` съществуват с major 243.
- Auto-logout на root shell-а губи дълги интерактивни capture-и → ползваме
  tmux/nohup, или sniff пише unbuffered за да оцелее kill.
- Buffer-ите по подразбиране на `open(..., "wb")` губят данни на kill —
  всеки writer трябва да е unbuffered или периодично flush-нат.

## Workflow + paths

1. Аз чета/пиша код вътре в chroot: `/home/claude-agent/work/obd/`
2. **Когато давам shell команди на smooker** — ВИНАГИ host prefix:
   `/chroot/claude/home/claude-agent/work/obd/`. Без изключения.
   (Виж memory: `feedback_chroot_paths_to_user.md`)
3. **Output винаги в локални папки на проекта**, никога в `/tmp`.
   Capture файловете отиват в `obd/captures/` директно (а не в `/tmp/`
   с post-cp). `/tmp` е tmpfs → 2-часов capture = язък ако батерията
   умре. Скриптовете пишат с relative path спрямо проекта.
   (Виж memory: `feedback_relative_paths.md`)

## Git

На Stage 1 push-овете не са приоритет — нямаме нищо съществено за
push-ване още. Repo координати са налични за момента когато ще ни
трябват:

- Repo: `ntr-git:/repos/obd.git` (main)
- GitHub mirror: `git@github-obd:smooker/obd.git` (НЕ push без потвърждение)
- Git user: `smooker <smooker@smooker.org>`

## Stage 2 — Active client (2026-04-14)

Преминахме на Stage 2. Директна комуникация с DS150E без Windows VM.

### FTDI serial setup
- Device: `0403:d6da` (Autocom CDP+ USB), FTDI FT232R
- Baud: **115200** 8N1, DTR+RTS high, no flow control (потвърдено от usbmon capture)
- Kernel: `ftdi_sio` модул, custom ID: `echo 0403 d6da > /sys/bus/usb-serial/drivers/ftdi_sio/new_id`
- STM32 се захранва от OBD конектор (12V), не от USB! Без кола = FTDI жив, STM32 мъртъв.
- BT MAC кандидат: `21:61:5C:AA:AC:65` (LE advertising дори от USB захранване, изчезва при disconnect)

### Tools
- `tools/ds150e.c` — C client за директна serial комуникация
- `tools/go.sh` — launcher (ftdi_sio load, custom ID, compile, run)
- `tools/compile.sh` — build script
- `tools/acz_decrypt.py` — .acz/.sdf analysis tool

### Autocom 2021.11 Software reverse engineering
- Source: Win7 qcow2 image → `files/Autocom 2021.11 Software/`
- Obfuscation: .NET Reactor + CryptoObfuscator
- **Passwords found:**
  - `Autocom186` — SQL Server password (VehicleSelection.dll)
  - `kwH2eae7Rpshm7S` — SQL CE password (Text.dll)
- `.acz` files: AES encrypted + GZip (Core.Crypto.Lib.dll, PBKDF2 key derivation)
- `.sdf` files: SQL Server Compact, `encryption mode=ppc2003 compatibility`
- `Scan.mdb`: Access DB (unencrypted) — vehicle profiles, Mitsubishi = `9mit_engine_4n14`

### Firmware
- `firmware/` directory for analysis
- `FIRMCDP.H86` — NOT ARM! Pattern `FA`/`EA` → possibly C166/XC167 (Infineon 16-bit automotive MCU) or custom packed format
- `OBD_VCI+.hex` — **STM32** (ARM Cortex-M). SP=0x20017100 (~92KB SRAM), reset=0x0803BA4D. Likely STM32F1xx high-density or STM32F2xx.
- `Firm.info`: firmware_version_vci=1621, firmware_version_vci_plus=1622

### Mitsubishi ASX / Outlander
- Кола: Mitsubishi ASX 2014, 1.8 DiD (4N13), AWD
- Софтуерът ползва `9mit_engine_4n14` profile (4N13 и 4N14 споделят платформа)
- Outlander profiles покриват: ABS, AC, gearbox, meter, SRS, ETACS, AWC, KOS/IMMO

### ECU — DENSO 275700-4772 / 1860C481
- Mitsubishi PN от ECU отговор: **1860C481** (потвърдено от capture 2026-04-14)
- MCU: Renesas V850ES/Fx3 (R4F70580SV), 1MB flash, 93C86 EEPROM
- CAN: 500 kbps, standard 11-bit IDs, 7E0/7E8 (engine ECU), UDS (ISO 14229)
- Injector: DENSO 295050-0120 / 1465A323, QR code 30 chars
- Подробна документация: `ecu/` директория (PIDs, DPF, flash, pinout, DTC)

### DPF система
- Диференциален pressure sensor: 1865A210 / 1865A184
- EGT сензори: pre-catalyst, pre-DPF, post-DPF
- Soot thresholds: 45% → active regen, 75% → forced regen warning, 85% → replace
- **Active regen** (от ECU на движение): ~600°C, ~10 мин при натоварване
- **Forced (service) regen** (на място, празен ход, през MUT-III/DS150E): ~600°C, ~25 мин
  - UDS service $31 routineControl, exact routine ID — proprietary (TODO: снемане от capture)
- Стандартен OBD2 PID $7C (service $01) — DPF differential pressure
- TODO: първи тест на колата — чети диф. налягане и soot level

## Какво НЕ е принцип

- **Изкуствени лимити по дължина** (напр. "C source < 200 реда"). Не са
  принцип. Кодът да е толкова дълъг колкото му трябва — не повече, не
  по-малко. Дори на Stage 1 такова ограничение е вредно: води до
  сплитване в множество файлове за нищо или до съкращаване което
  жертва четимост.

## DS150E hardware notes (findings 2026-04-16)

**Vendor: Autocom / Delphi -- шведска фирма** (Autocom от Trollhättan). Обяснява професионалния engineering -- over-engineered в хубавия смисъл, Volvo mentality за automotive tool. Използва се и от професионални сервизи с десетилетия.

### Dual operation modes

**1. Live USB sniff** -- real-time diagnostic сесия
- Командите и отговорите текат през USB
- Interactive: инжектираш ECU заявки, четеш отговорите
- Това е което досега анализираме

**2. REC mode (SD card)** -- on-the-fly data logging
- Бутон **REC** на самото устройство
- Оставяш DS150E в колата, той записва сам
- Анализ after the fact -- кога какво е станало
- Sensor values, fault conditions, environment over time
- USB sniff **не** хваща тези записи (защото са локални)

### SD card access

Картата си стои на топло вътре -- **не трябва да я вадим**. Четем remotely:
- **USB commands** -- file listing, read by offset, streaming dump
- **Bluetooth commands** -- същото през BLE (виж по-долу)

Протоколът трябва да има:
- `LS` equivalent за listing на recordings
- `READ` by offset за partial/resumable fetch
- Timestamp query за selective download

### Built-in battery -- множество предназначения

DS150E има вградена батерия, не е просто OBD cable:

1. **RTC backup** -- timestamp-ите на records трябва да се keep-ват
   - Всеки record е timestamped чрез internal RTC
   - Батерията пази часовника когато няма OBD захранване
   - Но RTC може да drift/reset -- **нужни са SET_TIME / GET_TIME команди** за периодичен sync с host

2. **Safe SD writes** -- защита срещу corrupt при power loss
   - Automotive environment: cranking voltage drops, ignition cycles
   - Battery-backed write circuit → graceful flush на last record
   - Finalization на FAT/exFAT metadata
   - Safe unmount на filesystem

3. **Bluetooth radio** -- **работи дори без OBD захранване**!
   - BT stack винаги on (pairing, advertising, listening)
   - Може да се pair-ва/конфигурира без да е в колата
   - Някой на 10m може да attempt pairing (security consideration)
   - За нас -- достъп до SD без колата да е paleна

### Use case: remote recording analysis

1. Оставяш DS150E в колата с REC натиснат
2. Караш, устройството записва on SD
3. Сядаш с laptop/phone близо до колата (парлинг)
4. BT connect → download recordings
5. Анализираш offline

Дори от магистрален паркинг -- кола спряна, стоиш 10m далече, четеш записи през BT.

## System setup

### ftdi_sio built-in handling (2026-04-16)

Kernel-ът има `CONFIG_USB_SERIAL_FTDI_SIO=y` (built-in, не модул). При enumerate на DS150E:

```
usb 1-1: New USB device found, idVendor=0403, idProduct=d6da
usb 1-1: Manufacturer: FTDI
usb 1-1: Product: Autocom CDP+ USB
```

...нещо се опитва да `modprobe ftdi_sio` и получава `Error: Driver 'ftdi_sio' is already registered, aborting... usbcore: error -16`. Auto-attach понякога не довежда до `/dev/ttyUSB0` при първи plug-in.

**Fix -- blacklist-ваме modprobe zaarediuvane:**

```
echo "blacklist ftdi_sio" > /etc/modprobe.d/ftdi-builtin.conf
```

Това не махa built-in драйвъра (който си работи), а спира modprobe да се опитва да го зарежда като модул. Auto-attach на kernel-a minava чисто.

### udev rules за DS150E

За да се създаде `/dev/autocom` symlink и user access без root:

```
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="d6da", MODE="0666", GROUP="dialout"' > /etc/udev/rules.d/99-autocom.rules
echo 'KERNEL=="ttyUSB[0-9]*", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="d6da", SYMLINK+="autocom", MODE="0666", GROUP="dialout"' >> /etc/udev/rules.d/99-autocom.rules
udevadm control --reload-rules
udevadm trigger
```

**Резултат:**
- `/dev/autocom` → symlink към `ttyUSB0` (stable име, не зависи от U-Sbus order)
- mode 0666 + group dialout
- smooker е в dialout → няма нужда от sudo за `cat/minicom/screen /dev/autocom`

### TODO

- **Идентифицирай командите:**
  - `GET_TIME` / `SET_TIME` (RTC management)
  - `LS` за SD listing
  - `READ` за file fetch
  - `DELETE` или overwrite за cleanup
  
- **BT discovery:**
  - Намери BT device name / MAC на DS150E
  - Hunt със `hcitool`, `bluetoothctl`, `gatttool`
  - GATT services/characteristics mapping
  
- **SD format reverse engineering:**
  - FAT/exFAT или custom?
  - Record format -- header, timestamp, ECU ID, payload
  - Mapping към live USB traffic (overlap в протокола?)
  
- **RTC sync:**
  - Първо SET_TIME на известна стойност
  - После GET_TIME → calibrate drift
  - Периодичен sync ако е нужно

- **Security consideration:**
  - BT always-on → exposure
  - Паролa/pairing защита?
  - Може ли някой друг да четe нашите записи?
