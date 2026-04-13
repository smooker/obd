# PLAN — Smooker (визия)

> Идеи, посоки, защо. Какво искаме да постигнем — в неговия глас.

## 1. Capture infrastructure

- Sniff в **C**, без libpcap (libpcap е счупен на st, махаме го от path-а)
- **tmux задължителен** (auto-logout е реален враг — доказа го днес)
- **Един capture за цялата сесия** (PTP-намаление на колата)
- Output **локално** в `obd/captures/`, не `/tmp` (батерията може да падне)
- **Cold init е свещен** — sniff пуска **преди** USB attach във VM

## 2. Hotplug + auto-passthrough (zero-touch)

- Капчето да започне в момента на plug-in, без human action
- Detect на `0403:d6da` появата → auto attach към libvirt Win VM
  чрез `virsh attach-device`
- Целта: plug-ваш и забравяш за keyboard-а

## 3. SD card hack — потенциално най-важният breakthrough

> **Update 2026-04-07:** SD slot **е потвърден реален**. В момента няма
> карта вътре (smooker я е извадил за rpi501). На обяд ще постави 64GB
> SanDisk → SD hack track се активира след обяд.
>
> **Хипотеза:** CDP+ записва running values + параметри на собствената
> си SD card. Ако намерим командата която го enable-ва → след сесията
> на колата dump-ваме SD-то през USB и получаваме целия лог **без да
> капваме USB трафика въобще**.

- Премахва нуждата от passive sniff в реално време
- Капчето става "drive normally, dump after" вместо "sniff during"
- Stage 1 проверки:
  - Има ли изобщо SD slot на CDP+ board-а? (hardware inspection)
  - Коя команда я enable-ва, коя я чете
- Ако да: променя цялата стратегия
- Ако не: пада, връщаме се към passive sniff

## 4. Знание = research

- Internet search за публични документи / forums за CDP / Autocom protocol
- **Autocom 2021 source — gold mine** (smooker го вади сега от Win VM-та).
  Освен CDP протокол съдържа и vehicle-specific бази (ECU pinouts,
  parameter maps, calibration tables на конкретни коли — Mitsubishi ASX,
  Volvo, Renault, Mercedes...). Това е валидна static analysis target
  не само за нашия dongle, а за RE на протоколи на десетки производители.
- Crowdsourced RE: forum threads, GitHub repos на similar dongles
  (DS150E, Delphi, NXP-clone-ове)
- "Работим като бели хора и AI" — research, не sliding в тъмнина

## 5. libpcap visibility fix (страничен)

- Дори ако ние не ползваме libpcap, искаме `tcpdump -D` да показва
  `usbmonN` за third-party tooling и diagnostic
- Investigate защо `equery u libpcap` показва `+ + usb` но binary няма
  capture support → най-вероятно USE добавен post-build, fix:
  `emerge -1 net-libs/libpcap`
- **Не блокира main execution**

## 6. Готовност преди тръгване към колата

- Smooker отива **подготвен** само след passed pre-flight test:
  bench unplug/replug цикъл на st, всичко работи
- Никакви "ой, открих че..." на колата — всичко тества се на bench
- Ready-state checklist: виж `PLAN_claude.md` секция C.4
