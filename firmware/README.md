# DS150E / Autocom CDP+ Firmware Analysis

## Files

| File | MCU | Size | Description |
|------|-----|------|-------------|
| `OBD_VCI_plus.bin` | STM32 (ARM Cortex-M) | 511KB | VCI+ firmware, converted from Intel HEX |
| Source: `../files/Autocom 2021.11 Software/Firmware/OBD_VCI+.hex` |

## VCI+ Hardware

- **MCU:** STM32 (92KB SRAM, 511KB flash) — likely STM32F103 high-density or STM32F2xx
- **SP:** 0x20017100, **Reset:** 0x0803BA4D
- **Build:** Jun 21 2016, Release
- **Firmware version:** 1622
- **Interfaces:** USB (FTDI FT232R), Bluetooth, SD-card, CAN, KWP2000, J1587/J1939

## FIRMCDP.H86 (older CDP variant)

- **NOT ARM** — first bytes `FA 03 00 01` with `FA`/`EA` pattern
- Possibly Infineon C166/XC167 (16-bit automotive MCU) or custom packed format
- Address range: 0x00000000 - 0x00060000, 234KB
- Needs further investigation

## Protocol commands found in VCI+ firmware

### Device info
| Command | Response | Description |
|---------|----------|-------------|
| `*CDP+` | - | Device name identifier |
| `*USB` | - | USB connection type |
| `*Bluetooth` | - | BT connection type |
| `*Main app` | - | Application identifier |

### Self-test (`*918x`)
| Command | Response | Description |
|---------|----------|-------------|
| `*918A` | `*918A_OK` / `*918A_FAIL` | Self-test A |
| `*918B` | `*918B_OK` / `*918B_FAIL` | Self-test B |
| `*918C` | `*918C_OK` / `*918C_FAIL` | Self-test C |
| `*918D` | `*918D_OK` | Self-test D |
| `*918E` | `*918E_OK` | Self-test E |
| `*918F` | `*918F_OK` / `*918F_FAIL` | Self-test F |

### CAN frame templates (hardcoded)
```
608h31 B8 00 00#
608h31 B8 01 03#
608h31 BA 01 03#
608h31 B9 01 03 02#
608h31 B9 01 03 00 00#
608h31 BB 01 03 00 00 00 06 46 22 00 BA#
608h32 B8 01 03#
```

### Misc commands
| Command | Description |
|---------|-------------|
| `*9999` | ? (test/reset?) |
| `*F1F5` / `*F0F5` | ? (mode switch?) |
| `*F1F` / `*F9F/` | ? |
| `*Wireless com enabled` | BT on response |
| `*Wireless com disabled` | BT off response |
| `*No response from module` | BT module not responding |
| `*No parameter selected` | No ECU param selected |
| `*PID list erased!` | PID list cleared |
| `*Enabled` / `*Disabled` | Feature toggle response |

### `*609` format example
```
*609MH_41_81_9B&4A_&4_&5_&6_&7+2C_&#02
```
Format uses `&` for field reference and `+` for concatenation, `#` for terminator.

### Bootloader entry (from firmware analysis, NOT tested)

**Hypothesis:**
1. Send `*9999` over serial → DS150E enters bootloader
2. Firmware prints `"Entering Boot Loader.."`
3. `"Usart1 <-> Usart3!"` — FTDI serial bridged to STM32 ROM bootloader
4. Standard STM32 UART bootloader protocol accessible via `stm32flash`
5. `stm32flash -r dump.bin /dev/ttyUSB0` — read flash backup
6. On exit: `"Bye bye!"`

**DEADBEEF marker at 0x080110CC** — bootloader jump setup, SRAM top 0x2000FFF0.

**FLASH.H86** (2928 bytes, base 0x100000) — RAM-loaded flash programming stub, used by bootloader for erase/program.

**1lev.hex / 2lev.hex** — Read Protection Level settings. If RDP Level 1+, flash read is locked.

**WARNING:** Do NOT send `*9999` without understanding the consequences. If RDP is set, entering bootloader and failing could brick the device.

### Error responses
- `*KW1 & KW2 error` — KWP2000 keyword mismatch
- `*Inverse KW2 error` — KWP2000 inverse keyword error
- `*No answer to init!` — ECU init timeout
- `*No response to ECU init` — ECU not responding
- `*No respons to 5baud!` — ISO 5-baud init failed
- `*Timeout waiting for KW1&KW2` — KWP2000 timeout
- `*Keep alive message error` — Tester Present failed
- `*ArbId error` — CAN arbitration ID error
- `*Wrong cable!` — wrong OBD cable
- `*! Reinit failed!` — re-initialization failed
- `*Unsupported voltage` — voltage out of range
