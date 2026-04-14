# Flash Read/Write — DENSO RA6 (4N13, R4F70580SV)

## ECU Platform Summary

| Property | Value |
|----------|-------|
| ECU family | DENSO RA6 |
| MCU | Renesas V850ES/Fx3 (R4F70580SV) |
| Flash size | 1 MB (1,048,576 bytes) |
| External EEPROM | 93C86 (SPI, 2Kx8 or 1Kx16) |
| Package | QFP-256 |
| Operating temp | -40°C to +105°C |
| CAN bus | 500 kbps |

## Method 1: OBD Read/Write

### Supported Tools
| Tool | Protocol | Notes |
|------|----------|-------|
| **KESS V2 / KESS3** | CAN OBD | Confirmed working. Use "Mitsubishi Pajero" protocol if ASX is not listed |
| **AutoTuner** | CAN OBD | Read + Write + DTC clear. Denso 4N15 (SH7058) protocol |
| **BitBox (BitSoftware)** | CAN OBD | Denso SH7058/SH7059 CAN module |
| **PCMFlash Module 42** | CAN OBD | Denso SH705X bootloader. Read/Write/Checksum |
| **I/O Terminal** | CAN/K-LINE | Full R/W flash + EEPROM. 64F7055/7058/7059 |
| **Piasini** | CAN OBD | Reported working for DENSO RA6 |
| **MMC Flasher** | CAN OBD | Can write Mitsubishi Diesel DENSO via OBD2 |

### OBD Procedure (Generic)
1. Connect tool to OBD-II port (pin 6=CAN-H, pin 14=CAN-L)
2. Ignition ON, engine OFF
3. Select appropriate protocol:
   - KESS: "Denso SH7058" or "Mitsubishi Diesel"
   - AutoTuner: "Denso 4N15 (SH7058)"
   - PCMFlash: Module 42 "Denso SH705X Bootloader"
4. Read takes ~2-5 minutes (1MB)
5. Always verify read (read twice, compare checksums)
6. Write back modified file — tool handles checksum correction

### Important Notes
- The RA6 DENSO ECU can be read via OBD with most mainstream tools
- The "SH7058" protocol label in tools is a DENSO platform designation
  (the actual MCU is V850ES, but the flash protocol is compatible)
- KESS V2 works but ensure K-Suite version >= 2.80
- If KESS does not list ASX directly, use Mitsubishi Pajero 4D56/4M41 protocol

---

## Method 2: Bench Read/Write

### When to Use Bench
- OBD port damaged or inaccessible
- ECU removed from vehicle for repair
- Cloning ECU to replacement unit
- If OBD read fails or produces corrupt dumps

### Wiring (Generic DENSO RA6 Bench)
Connection is through the ECU harness connector. The specific wiring diagram
is embedded in each tool's software (BitBox, KESS, etc).

**General connections required:**
| Signal | Description |
|--------|-------------|
| CAN-H | CAN bus high (from ECU connector) |
| CAN-L | CAN bus low (from ECU connector) |
| +12V | Battery positive to ECU power pins |
| GND | Battery ground to ECU ground pins |
| IGN | Ignition signal (some tools simulate via +12V) |

### Bench Tools
| Tool | Notes |
|------|-------|
| **HexProg II** | RA6 bench mode confirmed. Read/Write/Clone. Wiring in software. |
| **BitBox** | Bench + OBD. Requires Denso SH7058/SH7059 CAN module license |
| **KESS3** | Bench adapter available for Denso SH705x |
| **I/O Terminal** | Bench via K-LINE or CAN. Full flash + EEPROM |

### KESS Bench Notes
- Two different flat cables needed: one for READ, one for WRITE
- A **2.2 kOhm resistor** must be installed and pads linked for reading
- KTAG: use 14AM00T10M adapter with 144300T102 cable (no soldering)

---

## Method 3: Boot Mode (MCU-Level)

### V850ES/Fx3 Boot Mode Entry

The Renesas V850ES/Fx3 MCU supports serial programming via boot mode:

#### FLMD0 Pin
- **FLMD0 = HIGH** at reset release → enters boot/programming mode
- **FLMD0 = LOW** at reset release → normal execution mode
- **FLMD1 = LOW** (tied to GND for UART boot mode)

#### Boot Mode Sequence
1. Assert FLMD0 = HIGH (VCC level)
2. Assert FLMD1 = LOW (GND)
3. Release RESET (rising edge)
4. Some V850ES variants require 1-3 pulses on FLMD0 after reset
   release and before the first low pulse on RxD
5. MCU enters UART boot mode on serial pins

#### UART Interface
- Uses dedicated UART pins (TxD/RxD on the MCU)
- Pull-up resistors required on data lines
- Boot mode communicates at configurable baud rate
- Supports: flash erase, write, verify, blank check

#### V850ES Flash Self-Programming
- FLMD0 pin must be held at 0V during normal operation
- For self-programming: apply VCC to FLMD0 via port control before
  memory rewrite, return to 0V after completion
- The flash self-programming library handles sector erase/write

### Boot Mode Tools
| Tool | Notes |
|------|-------|
| **Renesas Flash Programmer** | Official Renesas tool. Requires direct MCU pin access |
| **CarProTool** | NEC/Renesas V850 programmer. Direct CPU access |
| **Custom FTDI adapter** | FTDI FT232RL + level shifter. See renesas-bootmode on GitHub |

### Practical Notes
- Boot mode requires direct soldering to MCU pins on the ECU PCB
- This is the last-resort method (bricked ECU recovery)
- OBD or bench methods are strongly preferred
- EEPROM (93C86) can be read separately via SPI if needed

---

## Checksum

The DENSO RA6 uses proprietary checksums in the flash image.
Most tuning tools (KESS, AutoTuner, PCMFlash, BitEdit) handle checksum
correction automatically during write.

For manual checksum calculation:
- The flash image has a checksum block typically at a fixed offset
- WinOLS with the correct .kp plugin can verify/correct checksums
- Incorrect checksum = ECU will not start

---

## EEPROM (93C86)

| Property | Value |
|----------|-------|
| Type | Microchip 93C86 or equivalent |
| Interface | 3-wire SPI (CS, CLK, DI, DO) |
| Size | 2048 bytes (2Kx8) or 1024x16-bit |
| Contents | Immobilizer data, mileage, VIN, adaptation values |

- Can be read/written separately with EEPROM programmers
- I/O Terminal and KESS can read EEPROM via bench connection
- **Critical**: backup EEPROM before any flash operations

---

## File Format

Standard dump file is **1,048,576 bytes** (1MB) raw binary.
- Used by WinOLS, BitEdit, ECM Titanium for map editing
- Calibration data is in the upper portion of the flash
- Boot code and core firmware in lower addresses
