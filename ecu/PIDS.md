# PIDs / DIDs — Mitsubishi ASX 4N13 DENSO ECU

## 1. Standard OBD2 PIDs (Service $01)

The 4N13 ECU supports standard EOBD/OBD2 PIDs via CAN (ISO 15765-4, 500 kbps).
ECU address: **0x7E0** (response 0x7E8).

### Confirmed Supported Standard PIDs (Diesel — Euro 5)

| PID (hex) | Description | Unit | Formula |
|-----------|-------------|------|---------|
| 0x00 | Supported PIDs [01-20] | bitmap | — |
| 0x01 | Monitor status since DTC cleared | bitmap | — |
| 0x03 | Fuel system status | enum | — |
| 0x04 | Calculated engine load | % | A*100/255 |
| 0x05 | Engine coolant temperature | °C | A-40 |
| 0x06 | Short-term fuel trim Bank 1 | % | (A-128)*100/128 |
| 0x0B | Intake manifold absolute pressure | kPa | A |
| 0x0C | Engine RPM | rpm | (256*A+B)/4 |
| 0x0D | Vehicle speed | km/h | A |
| 0x0F | Intake air temperature | °C | A-40 |
| 0x10 | MAF air flow rate | g/s | (256*A+B)/100 |
| 0x11 | Throttle position | % | A*100/255 |
| 0x1C | OBD standards supported | enum | — |
| 0x1F | Run time since engine start | s | 256*A+B |
| 0x20 | Supported PIDs [21-40] | bitmap | — |
| 0x21 | Distance traveled with MIL on | km | 256*A+B |
| 0x23 | Fuel rail gauge pressure (diesel) | kPa | (256*A+B)*10 |
| 0x2C | Commanded EGR | % | A*100/255 |
| 0x2D | EGR error | % | (A-128)*100/128 |
| 0x2E | Commanded evaporative purge | % | A*100/255 |
| 0x2F | Fuel tank level input | % | A*100/255 |
| 0x30 | Warm-ups since codes cleared | count | A |
| 0x31 | Distance since codes cleared | km | 256*A+B |
| 0x33 | Barometric pressure | kPa | A |
| 0x3C | Catalyst temp B1S1 | °C | ((256*A+B)/10)-40 |
| 0x3E | Catalyst temp B1S2 | °C | ((256*A+B)/10)-40 |
| 0x40 | Supported PIDs [41-60] | bitmap | — |
| 0x42 | Control module voltage | V | (256*A+B)/1000 |
| 0x44 | Commanded air-fuel ratio | ratio | (256*A+B)*2/65536 |
| 0x46 | Ambient air temperature | °C | A-40 |
| 0x49 | Accelerator pedal position D | % | A*100/255 |
| 0x4A | Accelerator pedal position E | % | A*100/255 |
| 0x4C | Commanded throttle actuator | % | A*100/255 |
| 0x60 | Supported PIDs [61-80] | bitmap | — |

### Diesel-Specific Standard PIDs (likely supported)

| PID (hex) | Description | Unit |
|-----------|-------------|------|
| 0x5A | Relative accelerator pedal position | % |
| 0x5B | Engine oil temperature | °C |
| 0x5C | Engine oil temperature | °C |
| 0x5E | Engine fuel rate | L/h |
| 0x61 | Driver's demand torque | % |
| 0x62 | Actual engine torque | % |
| 0x63 | Engine reference torque | Nm |
| 0x6B | Exhaust gas temperature Bank 1 Sensor 1 | °C |
| 0x6C | Exhaust gas temperature Bank 1 Sensor 2 | °C |
| 0x7C | DPF temperature | °C |
| 0x7E | DPF pressure | Pa |
| 0x7F | DPF inverse pressure | Pa |

**Note**: PIDs 0x7C-0x7F are defined in SAE J1979 for DPF monitoring. Support on
this specific ECU is unconfirmed — scan PID $00/$20/$40/$60/$80 to determine
which are actually supported.

---

## 2. Service $09 — Vehicle Information

| PID | Description | Expected Value |
|-----|-------------|----------------|
| 0x02 | VIN | Vehicle Identification Number |
| 0x04 | Calibration ID (CALID) | e.g. "H16VRA6" (8-16 chars) |
| 0x06 | Calibration Verification Number (CVN) | 4-byte hash |
| 0x0A | ECU name | e.g. "4N13" or "DCU" |

---

## 3. Mitsubishi-Specific PIDs — Service $21 (MUT-II Legacy)

Mitsubishi uses Service $21 for extended diagnostic data (MUT-II protocol, ISO 9141-2).
These may not be accessible on CAN-based 4N13 diesel ECUs (which primarily use
ISO 15765 / UDS). The following are known from gasoline Mitsubishi ECUs and may
partially apply:

| PID | Bytes | Description (known from Mirage/Evo) |
|-----|-------|--------------------------------------|
| 0x01 | 5 | Speed, RPM, A/C status |
| 0x02 | 2 | Temperature (B-40 = °C) |
| 0x03 | 3 | Engine load |
| 0x0F | 1 | Full range 00-FF (unknown parameter) |
| 0x1F | 2 | Engine running status (00=off, 16=idle) |

**Warning**: The MUT for Torque app lists ASX as "incompatible" with MUT-II PIDs.
The 4N13 diesel ECU likely uses UDS (service $22) rather than legacy MUT-II ($21).

---

## 4. UDS DIDs — Service $22 (ReadDataByIdentifier)

### Standard UDS DIDs (ISO 14229)

| DID (hex) | Description |
|-----------|-------------|
| 0xF186 | Active diagnostic session |
| 0xF187 | Vehicle manufacturer spare part number |
| 0xF188 | Vehicle manufacturer ECU software version |
| 0xF189 | Vehicle manufacturer ECU software version date |
| 0xF18A | System supplier identifier |
| 0xF18B | ECU manufacturing date |
| 0xF18C | ECU serial number |
| 0xF190 | VIN (Vehicle Identification Number) |
| 0xF191 | Vehicle manufacturer ECU hardware version |
| 0xF192 | Vehicle manufacturer ECU software version |
| 0xF193 | Vehicle manufacturer ECU software version |
| 0xF194 | System supplier ECU software version |
| 0xF195 | System supplier ECU software date |
| 0xF197 | System name or ECU name |

### Mitsubishi/DENSO Manufacturer-Specific DIDs (Estimated)

Based on DENSO diesel ECU patterns and MUT-III service data items, these DIDs
are expected to be available via service $22. Header: **7E0/7E8** (engine ECU).

**NOTE**: Exact DID addresses are proprietary to Mitsubishi/DENSO. The addresses
below are educated estimates based on cross-referencing MUT-III item numbers with
known DENSO diesel patterns. They require verification on the actual ECU.

#### Engine Parameters
| DID (est.) | MUT-III Item | Description | Unit |
|------------|-------------|-------------|------|
| 0x0100-01FF | — | Engine speed | rpm |
| 0x0200-02FF | — | Vehicle speed | km/h |
| — | Item 5 | Engine coolant temperature | °C |
| — | Item 7 | Intake air temperature | °C |
| — | Item 12 | Battery voltage | V |
| — | Item 15 | Target fuel rail pressure | MPa |
| — | Item 16 | Actual fuel rail pressure | MPa |
| — | Item 20 | Accelerator pedal position | % |
| — | Item 25 | Boost pressure (actual) | kPa |
| — | Item 26 | Boost pressure (target) | kPa |
| — | Item 30 | MAF sensor output | g/s |
| — | Item 35 | Injection quantity (main) | mm³/st |
| — | Item 36 | Injection timing (main) | °BTDC |

#### Turbo / EGR
| DID (est.) | MUT-III Item | Description | Unit |
|------------|-------------|-------------|------|
| — | Item 40 | VGT actuator position (target) | % |
| — | Item 41 | VGT actuator position (actual) | % |
| — | Item 45 | EGR valve position (target) | % |
| — | Item 46 | EGR valve position (actual) | % |
| — | Item 50 | EGR cooler outlet temperature | °C |

#### DPF Parameters
| DID (est.) | MUT-III Item | Description | Unit |
|------------|-------------|-------------|------|
| — | Item 110 | DPF differential pressure | kPa |
| — | Item 112 | Exhaust gas temp - pre-catalyst (B1S1) | °C |
| — | Item 114 | Exhaust gas temp - pre-DPF (B1S2) | °C |
| — | Item 116 | Exhaust gas temp - post-DPF | °C |
| — | Item 118 | Exhaust gas temp sensor #2 | °C |
| — | Item 120 | DPF soot accumulation amount | g |
| — | Item 122 | DPF soot level | % |
| — | Item 124 | Distance since last DPF regen | km |
| — | Item 126 | DPF regeneration count | count |
| — | Item 328 | DPF regeneration request status | ON/OFF |

#### Injector Data
| DID (est.) | MUT-III Item | Description | Unit |
|------------|-------------|-------------|------|
| — | Item 60 | Injector correction cyl 1 | μs |
| — | Item 61 | Injector correction cyl 2 | μs |
| — | Item 62 | Injector correction cyl 3 | μs |
| — | Item 63 | Injector correction cyl 4 | μs |
| — | Item 70 | Pilot injection quantity | mm³/st |
| — | Item 71 | Pilot injection timing | °BTDC |

---

## 5. Torque Pro Extended PID Plugins

### Advanced EX for Mitsubishi
Reported to work on ASX 2.2 (4N14) diesel. Provides:
- Boost pressure
- Extended sensor data

### MUT for Torque
- Uses ISO 9141-2 (K-line) protocol
- ASX is listed as **incompatible** (CAN-only vehicle)
- Provides: Knocksum, fuel trims, ECU load, airflow Hz, EGR temperatures

### Recommended Approach
For the CAN-based 4N13, use a scan tool that supports:
1. Standard OBD2 PIDs (service $01) for basic data
2. UDS service $22 for extended DENSO parameters
3. MUT-III protocol for full parameter access (proprietary)

---

## 6. WinOLS / BitEdit Tuning Maps (SH7058 1MB)

The BitEdit Mitsubishi Denso Diesel SH7058 module exposes these calibration
maps (relevant to understanding what parameters the ECU handles):

### Airmass / Airflow
- Desired airmass
- Desired airmass (EGR disabled)

### Boost Control
- Boost pressure limits (vs intake air temp)
- Base boost pressure limitation
- Target boost pressure
- Boost actuator duty cycle

### Fuel Injection
- Dynamic base injection timing
- Injection duration (from rail pressure)

### Injection Volume
- Torque-to-injection volume conversion (multiple modes)
- Volume limitation maps
- System error volume limiters
- Conditional injection volume restrictions

### Rail Pressure
- Target rail pressure
- Rail pressure under fault conditions
- Rail pressure ceiling limits

### Injection Timing
- Primary injection timing baseline
- Corrected injection timing values

### Torque Management
- Injection volume to torque conversion
- Requested torque
- Maximum allowable torque
- Torque limitation thresholds
- System error torque limiting
- Inverse pedal mapping (torque/RPM correlation)

### DTC Mask
- DTC mask editor (enable/disable specific fault codes)
