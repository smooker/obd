# Mitsubishi ASX GA 1.8 DiD — DENSO ECU Documentation

## Vehicle
- **Vehicle**: Mitsubishi ASX (GA) / RVR
- **Year**: 2014
- **Engine**: 4N13 — 1.8L (1,798 cc) DOHC 16v turbodiesel
- **Emission standard**: Euro 5

## Engine Specifications (4N13)
| Parameter | Value |
|-----------|-------|
| Displacement | 1,798 cc |
| Bore x Stroke | 83.0 mm x 83.1 mm |
| Compression ratio | 14.9:1 |
| Power (low variant) | 85 kW / 116 PS @ 4000 rpm |
| Power (high variant) | 110 kW / 150 PS @ 4000 rpm |
| Torque | 300 Nm @ 2000-3000 rpm |
| Valvetrain | DOHC 16v, MIVEC (intake) |
| Fuel system | Common rail, solenoid injectors, 200 MPa max |
| Turbo | Variable geometry (VGT) with intercooler |
| Fuel injection | DENSO G3 piezo/solenoid injectors |
| Supply pump | DENSO HP3/HP4, SV3-type SCV |

## ECU Identification
| Field | Value |
|-------|-------|
| **DENSO part number** | 275700-4772 |
| **Mitsubishi part number** | 1860C480 |
| **MCU** | Renesas V850ES/Fx3 (R4F70580SV) |
| **Package** | QFP-256 |
| **Flash size** | 1 MB (confirmed by BitEdit SH7058 module) |
| **EEPROM** | 93C86 (external SPI) |
| **ECU family** | DENSO RA6 |

## Calibration & Software IDs
Multiple calibration versions exist for 4N13 ECUs. Known IDs:

| Calibration ID | Hardware ID | Mitsubishi PN | DENSO PN |
|----------------|-------------|---------------|----------|
| H15VRA6 | — | 1860B550 | — |
| H16VRA6 | T1U1HDU1DM01 | 1860C107 | 275700-2962 |
| H16YRA6 | T1R2HDR2DF03 | 1860C127 | — |

The ECU in question (1860C480 / 275700-4772) is a later revision. Calibration ID
is likely H16VRA6 or a successor (H17VRA6 etc). The calibration ID can be read via
OBD2 Mode 09 (service $09) PID 04 (CALID) and PID 06 (CVN).

## ECU Architecture Notes
- The ECU is commonly referred to as "DENSO RA6" in the tuning community
- Despite forum references to "SH7058", the actual MCU is Renesas V850ES/Fx3
  (R4F70580SV). The "SH7058" label in tuning tools refers to a DENSO platform
  classification, not the actual silicon. Some RA6 variants use SH72453.
- The 4N14 (2.2L) variant uses the same ECU platform (DENSO RA6) with
  SH72453 or SH7059 MCU, making pinout/protocol data largely interchangeable.

## Also Used In
- Mitsubishi Lancer (from 2010)
- Peugeot 4008 (rebadged ASX)
- Citroen C4 Aircross (rebadged ASX)
