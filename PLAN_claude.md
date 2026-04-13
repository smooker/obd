# PLAN — Claude (техническо изпълнение)

> Конкретни решения, файлове, размери, ред на операциите.
> Огледало на `PLAN_smooker.md` от инженерна страна.

## A. sniff.c — rewrite, pure C

| Аспект | Решение |
|---|---|
| Source | `obd/sniff.c` (rewrite) |
| Dependencies | gcc + libc only. Drop libpcap. |
| USB layer | `/dev/usbmonN` direct read, mon_bin_hdr_v1 (64 байта) |
| Auto-discovery | `/sys/bus/usb/devices/*/idVendor`+`idProduct` (без `popen("lsusb")`) |
| Hotplug wait | `-w` flag → polling `/sys/bus/usb/devices/*` на 200ms |
| Output dir | `realpath(/proc/self/exe)` → `dirname` → `captures/` |
| Bin file | `open(O_WRONLY\|O_CREAT)`, `write(2)` директно (no userspace buffer, kill-resilient) |
| Text file | `setvbuf(_IOLBF)` line-buffered |
| Filter | `device_address == target` + `busnum == target_bus` + `xfer_type == 3 (BULK)` |
| FTDI quirk | Skip 2 byte modem-status prefix на IN payloads |
| Signal | SIGINT → close files cleanly → exit |
| CLI | `sniff [-w] [-o name]`. Default: capture сега. `-w` = wait for hotplug. |

Размер: ~180 реда C, audit-able.

### Phase 0 — hotplug wait (опционално с `-w`)

```c
if (wait_mode) {
    while (!find_dongle(&bus, &dev)) usleep(200 * 1000);
}
```

### Phase 1 — open + record

```c
open_usbmon(bus);
open_outputs();              // captures/<ts>.bin (write(2)), .txt (line-buf)
install_sigint();
loop {
    read 64-byte mon_bin_hdr_v1
    read len_cap bytes payload
    write_raw_bin_unbuffered(hdr+payload)
    if (busnum != target_bus) continue
    if (devnum != target_dev) continue
    if (transfer_type != 3) continue
    direction = ep & 0x80 ? IN : OUT
    if (IN && len >= 2) skip first 2 (FTDI modem-status)
    print_arrow_line(txt, ts, dir, payload)
}
```

## B. obd-go.sh — orchestrator (bash thin wrapper)

| Аспект | Решение |
|---|---|
| Source | `obd/obd-go.sh` (нов; стария `go.sh` archive като `go.sh.legacy`) |
| Зависимости | bash, tmux, virsh (libvirt), gcc (build на sniff ако липсва) |
| Workflow | sanity → tmux session → sniff -w → poll за hotplug → virsh attach-device → wait |
| VM domain | `OBD_VM` env var, default tbd (нужно потвърждение от smooker) |
| USB XML | inline heredoc с `<vendor id='0x0403'/><product id='0xd6da'/>` |
| Idempotent | tmux has-session check → re-attach existing |
| Detach | Ctrl-B D, връщане `tmux a -t obd-capture` |

Размер: ~80 реда bash.

## B-bis. Auto-attach UI question

Open: immediate fire `virsh attach-device` при hotplug, или
`read -p "attach? [y/N]"`?

- **Immediate** = zero-touch, но риск от грешен dongle / непреднамерено
- **Confirm** = безопасно, но връща PTP-то на колата

→ default: **immediate** (security през VID/PID match), с `OBD_CONFIRM=1`
env var override за confirm mode

## C. SD card investigation track (parallel research)

Преди да напишем код за SD dump → **research first**.

1. **Hardware inspection:** smooker отвaря дongle-а или ползва datasheet
2. **Protocol search:** forum threads, GitHub `*ds150*`, `*cdp*`,
   `*delphi*` за SD-related commands
3. **Static analysis на Autocom 2021 binary:** `strings`, `objdump`,
   `ghidra` ако е необходимо, за `*` команди свързани с storage
4. **Existing capture pattern:** scan за команди от вида `*70x` / `*80x`
   които може да са storage-related (не сме виждали в 2026-04-06 capture-а,
   но той е mid-session, не cold init)
5. **Кандидатна команда → пробен active send** (Stage 1 passive принципът
   ще се ревизира за тази проба)

## D. Internet research track (parallel)

- WebFetch към:
  - `delphi-tech.com`, `autocomcdp.com` (официални docs ако ги има)
  - GitHub: `0403 d6da`, `cdp+ linux`, `delphi ds150e protocol`
  - Forums: Snap-on, MHH Auto, Digital Kaos (RE communities)
- Ако намерим съществуваща протоколна документация → 80% RE спестен
- Ако намерим работещ Linux client (open source) → 100% спестен;
  reference implementation

## E. libpcap visibility fix (side quest)

- `emerge --info net-libs/libpcap` → виж дали `usb` е в use на binary-то
- Ако не: `emerge -1 net-libs/libpcap` (rebuild)
- Verification: `tcpdump -D | grep usbmon`
- **Не блокира main execution**

## F. Файлове + структура (target state след Stage 1)

```
obd/
├── CLAUDE.md
├── PLAN_smooker.md
├── PLAN_claude.md          ← този файл
├── sniff.c                 (rewrite — pure C)
├── obd-go.sh               (нов — tmux + virsh launcher)
├── go.sh.legacy            (стария преименуван — за reference)
├── HOWTO.md                (update — нов workflow)
├── README.md               (update — link към плановете)
├── ANALYSE.md              (без промяна сега)
├── .gitignore              (update — captures/, sniff)
├── captures/
│   └── .gitkeep
└── (legacy: sniff_full.py, decode.py, gen_report.py)
```

## C — Сечение / synthesis

### C.1 Кое е критично за днес/утре

1. **Капче infrastructure (A + B)** — без него нищо друго не работи.
   Една сесия с Mitsubishi-то ни дава 80% от data-та.
2. **Cold init capture** — приоритет 0. Без него replay е невъзможен.
   Hotplug detect (A phase 0) е техническото решение.

### C.2 Кое е "good to have" но не блокира

- **PLAN_smooker §3 SD card hack** — потенциално революционно, но
  изисква research. Stage 1 базира се на passive sniff. SD идеята →
  паралелна track (C по-горе), не блокира главния deliverable.
- **PLAN_smooker §4 internet research** — може да премахне 80% от RE
  работата. Стартира паралелно на капчещата infrastructure.
- **PLAN_smooker §5 libpcap fix** — pure side quest. Не блокира.

### C.3 Order of operations

| # | Задача | Кой | Блокира? |
|---|---|---|---|
| 1 | sniff.c rewrite | claude | Да (всичко зависи) |
| 2 | obd-go.sh | claude | Зависи от 1 |
| 3 | Smoke test (unplug/replug на st bench) | smooker | Зависи от 1+2 |
| 4 | Cold init capture на колата | smooker | Зависи от 3 |
| 5 | Internet research (паралелно от 1) | claude (WebFetch) | Не |
| 6 | Autocom 2021 binary extract от Win VM | smooker | Не блокира 1-4 |
| 7 | SD card hardware check | smooker | Не блокира 1-4 |
| 8 | libpcap rebuild | smooker (1 команда) | Не |
| 9 | Decoder offline (от capture-ите) | claude | След 4 |
| 10 | `*606`/`*608` checksum brute force | claude | След 4 |

### C.4 Ready-state criteria — преди тръгване към колата

Smooker отива **подготвен** ако всичко по-долу е ✓:

- [ ] `sniff.c` compile-ва без warnings
- [ ] `obd-go.sh` минава bash syntax check
- [ ] Smoke test: armed sniff → unplug/replug → capture файл > 0 byte
- [ ] tmux session оцелява detach/reattach
- [ ] kill -9 test: capture файл оцелява (unbuffered write)
- [ ] virsh dominfo на target Win VM работи
- [ ] virsh attach-device dry-run (с тестов USB device) минава

### C.5 Разширения за Stage 2 (после този session)

- Decoder C tool (`decode.c`)
- Replay tool (`replay.c`) — байт-точно възпроизвеждане на cold init
- Минимален Linux client (read VIN, read DTC)
- SD card dump tool (ако хипотезата §3 се потвърди)

## D. Открити въпроси (нужни преди start на implementation)

1. **Libvirt domain име** на Win7 VM-та с Autocom?
2. **Auto-attach mode** — immediate или confirm? (по B-bis: default immediate)
3. **libpcap rebuild** scope — задължителна стъпка или TODO?
4. **CDP+ SD card** — потвърдено ли е че hardware-ът има SD slot?
5. **Autocom 2021 source** — формат, размер, кога ще го извадиш?
