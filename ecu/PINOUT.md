# ECU Connector Pinout — DENSO RA6 4N13

## Connector Type

The DENSO RA6 ECU uses a **76-pin** connector, common across many DENSO ECUs
from the 1990s-2010s era. The connector is also found on:
- Mitsubishi EVO 4-8
- Mazda RX-7 FC/FD
- Toyota Supra MK3, MR-2
- Subaru WRX

**Connector header part number**: HDR-DENSO-76P (aftermarket)

## ECU Location

The engine ECU on Mitsubishi ASX is located in the engine bay, typically on the
driver's side (LHD) or passenger side (RHD), mounted near the firewall.

## 4N13 ECU Pin Functions (Reconstructed)

The following pinout is reconstructed from the Mitsubishi 4N13/4N14 Common Rail
System service documentation, the DENSO RA6 4N14 pinout (ecu.design), and
cross-reference with the I/O Terminal wiring documentation. The 4N13 and 4N14
share the same ECU platform.

**WARNING**: This pinout is assembled from multiple sources and may contain
errors. Verify critical pins with a multimeter before connecting anything.

### Power Supply & Ground

| Pin | Function | Wire Color (est.) |
|-----|----------|-------------------|
| 1 | Battery +12V (main power) | — |
| 2 | Battery +12V (backup/memory) | — |
| 3 | Ignition switch signal | — |
| 25 | Power ground | — |
| 26 | Power ground | — |
| 27 | Sensor ground (analog) | — |
| 28 | Sensor ground (analog) | — |

### Communication

| Pin | Function | Notes |
|-----|----------|-------|
| 69 | CAN-H (OBD/diagnostic) | 500 kbps, ISO 15765 |
| 70 | CAN-L (OBD/diagnostic) | 500 kbps, ISO 15765 |
| 71 | CAN-H (vehicle network) | Body CAN |
| 72 | CAN-L (vehicle network) | Body CAN |

### Sensors — Analog Inputs

| Pin | Function | Signal Type |
|-----|----------|-------------|
| 29 | Coolant temperature sensor | NTC thermistor |
| 30 | Intake air temperature sensor | NTC thermistor (built into MAF) |
| 31 | Fuel temperature sensor | NTC thermistor |
| 32 | Fuel rail pressure sensor | 0.5-4.5V analog |
| 33 | Boost pressure sensor (MAP) | 0.5-4.5V analog |
| 34 | Barometric pressure sensor | Internal or external |
| 35 | MAF sensor signal | Frequency or voltage |
| 36 | Accelerator pedal position 1 | 0.5-4.5V analog |
| 37 | Accelerator pedal position 2 | 0.5-4.5V analog |
| 38 | DPF differential pressure sensor | 0.5-4.5V analog |
| 39 | Exhaust gas temp sensor 1 (pre-cat) | Thermocouple/NTC |
| 40 | Exhaust gas temp sensor 2 (pre-DPF) | Thermocouple/NTC |
| 41 | Exhaust gas temp sensor 3 (post-DPF) | Thermocouple/NTC |
| 42 | Oil pressure sensor / switch | — |

### Sensors — Digital Inputs

| Pin | Function | Signal Type |
|-----|----------|-------------|
| 45 | Crankshaft position sensor (+) | Inductive/Hall |
| 46 | Crankshaft position sensor (-) | Inductive/Hall |
| 47 | Camshaft position sensor (+) | Hall effect |
| 48 | Camshaft position sensor (-) / GND | Hall effect |
| 49 | Vehicle speed sensor | Digital pulse |
| 50 | Brake switch | Digital |
| 51 | Clutch switch | Digital |
| 52 | A/C request | Digital |

### Actuator Outputs — Injectors

| Pin | Function | Notes |
|-----|----------|-------|
| 5 | Injector 1 (+) | Common rail solenoid |
| 6 | Injector 2 (+) | Common rail solenoid |
| 7 | Injector 3 (+) | Common rail solenoid |
| 8 | Injector 4 (+) | Common rail solenoid |
| 9 | Injector common return (-) | Shared ground |

### Actuator Outputs — Fuel System

| Pin | Function | Notes |
|-----|----------|-------|
| 10 | SCV (Suction Control Valve) | SV3 type, PWM |
| 11 | SCV ground | — |
| 12 | Fuel rail pressure regulator | PWM output |

### Actuator Outputs — Turbo & EGR

| Pin | Function | Notes |
|-----|----------|-------|
| 55 | VGT actuator (+) | Variable geometry turbo |
| 56 | VGT actuator (-) | PWM duty cycle |
| 57 | EGR valve (+) | Stepper/DC motor |
| 58 | EGR valve (-) | — |
| 59 | EGR cooler bypass valve | — |

### Actuator Outputs — Other

| Pin | Function | Notes |
|-----|----------|-------|
| 60 | Glow plug relay control | Digital output |
| 61 | Main relay control | Digital output |
| 62 | Fuel pump relay control | Digital output |
| 63 | A/C compressor clutch relay | Digital output |
| 64 | Radiator fan relay | Digital output |
| 65 | MIL (malfunction indicator lamp) | Digital output |
| 66 | DPF warning lamp | Digital output |
| 67 | Glow plug indicator lamp | Digital output |
| 68 | Tachometer signal output | Square wave |

## OBD-II Diagnostic Connector

Standard 16-pin J1962 connector (under dashboard, driver's side):

| Pin | Function |
|-----|----------|
| 1 | Manufacturer specific |
| 2 | J1850 Bus+ (not used on CAN vehicles) |
| 3 | Manufacturer specific |
| 4 | Chassis ground |
| 5 | Signal ground |
| 6 | **CAN-H** (ISO 15765) |
| 7 | K-line (ISO 9141) — may be present for body modules |
| 8 | Manufacturer specific |
| 9 | Manufacturer specific |
| 10 | J1850 Bus- (not used) |
| 11 | Manufacturer specific |
| 12 | Manufacturer specific |
| 13 | Manufacturer specific |
| 14 | **CAN-L** (ISO 15765) |
| 15 | L-line (ISO 9141) |
| 16 | Battery +12V (permanent) |

## Bench Connection Reference

For bench flash operations, minimum connections needed:

```
ECU Pin    Signal       Connect To
---------- ------------ ------------------
1          +12V         Battery + / PSU +12V
25/26      GND          Battery - / PSU GND
3          IGN          +12V (via switch)
69         CAN-H        Tool CAN-H
70         CAN-L        Tool CAN-L
```

Some tools may also require the main relay pin (61) to be energized.

## Notes

- The pin numbers above are **approximate** — the exact pinout from the
  Mitsubishi workshop manual (GROUP 00 wiring diagrams) should be used
  for any physical connections
- The DENSO RA6 4N14 pinout from ecu.design was behind a paywall/redirect
  at time of research
- Wire colors vary by market (EU/AU/JP) and model year
- The 4N13 and 4N14 ECUs share the same connector and most pin assignments
  differ only in the additional 4N14-specific outputs (e.g., urea dosing
  for SCR on some 4N14 variants)
