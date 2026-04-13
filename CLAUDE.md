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

## Какво НЕ е принцип

- **Изкуствени лимити по дължина** (напр. "C source < 200 реда"). Не са
  принцип. Кодът да е толкова дълъг колкото му трябва — не повече, не
  по-малко. Дори на Stage 1 такова ограничение е вредно: води до
  сплитване в множество файлове за нищо или до съкращаване което
  жертва четимост.
