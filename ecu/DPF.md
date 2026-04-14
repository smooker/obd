# DPF System — Mitsubishi ASX 4N13

## System Overview

The 4N13 uses a wall-flow ceramic DPF to capture particulate matter (PM) from
the diesel exhaust. The system includes:

- **DPF differential pressure sensor** (PN: 1865A210 or 1865A184)
  - Measures pressure drop across DPF to estimate soot loading
  - Sensor is welded into exhaust — not a removable bolt-in part
- **Exhaust gas temperature sensor — pre-catalyst** (upstream of DOC)
- **Exhaust gas temperature sensor — pre-DPF** (between DOC and DPF)
- **Exhaust gas temperature sensor — post-DPF** (downstream)

## Regeneration Types

### 1. Passive Regeneration
- Occurs continuously during normal highway driving
- Exhaust gas temps above ~350°C oxidize soot naturally via NO2
- No special ECU intervention required

### 2. Active (Automatic) Regeneration
- ECU-initiated when soot accumulation reaches ~45% threshold
- Post-injection raises exhaust temp to ~600°C to burn soot
- Conditions: engine at operating temp, vehicle speed > 40 km/h
- Duration: ~10-15 minutes
- Normal interval: every 500-600 km
- Warning indicator: DPF lamp illuminates during active regen

### 3. Forcible (Service) Regeneration — via MUT-III

Required when soot level exceeds ~75% or DTC P2463/P1498 is set.

#### Preconditions
- Engine coolant temperature at normal operating level
- DPF not in critical overfill state (>85% requires replacement)
- No active DTCs preventing regeneration
- Well-ventilated area (exhaust temps reach 600°C+)

#### Procedure (from Mitsubishi Workshop Manual — L200/ASX)

1. Set vehicle to pre-inspection condition
2. Perform ammonia purge (if equipped with SCR — 4N14 only)
3. Wait until exhaust gas temperature sensor #2 falls below **115°C**
   (MUT-III Data List **Item 118**) — approx 38 minutes cooling
4. Connect M.U.T.-III via diagnosis connector, ignition LOCK (OFF)
5. Start engine, hold at idle
6. Select **"MPI/GDI/DIESEL"** on M.U.T.-III
7. Navigate to **"Special Function"**
8. Select **"DPF regeneration"**
9. Execute **Item 34: DPF Regeneration**
10. Monitor regeneration progress:
    - Engine speed: **1,500-2,000 RPM** (automatically controlled)
    - DPF temperature: maintained at approximately **600°C**
    - Duration: approximately **25 minutes** (automatic stop)
11. When DPF warning mark extinguishes:
    - Let engine idle for **15 minutes**
    - Wait until diesel preheat indicator lamp goes out
    - Turn ignition to LOCK (OFF)
12. Verify: confirm engine warning lamp is off, no DTCs set

#### Post-Regeneration
- Oil level may have risen due to post-injection fuel dilution
- Check and replace engine oil if level is above MAX mark

## DPF-Related MUT-III Data Items

| Item # | Description | Unit | Threshold/Note |
|--------|-------------|------|----------------|
| 34 | DPF regeneration (execute) | command | Special Function |
| 110 | DPF differential pressure | kPa | — |
| 112 | Exhaust gas temp pre-catalyst | °C | — |
| 114 | Exhaust gas temp pre-DPF | °C | — |
| 116 | Exhaust gas temp post-DPF | °C | — |
| 118 | Exhaust gas temp sensor #2 | °C | Must be < 115°C before regen |
| 120 | DPF soot accumulation | g | — |
| 122 | DPF soot level | % | 45%=active, 75%=forced, 85%=replace |
| 124 | Distance since last regen | km | Normal: 500-600 km |
| 126 | DPF regen count | count | — |
| 328 | DPF regen request status | ON/OFF | — |

## DPF-Related OBD2 Standard PIDs

| PID (hex) | Mode | Description |
|-----------|------|-------------|
| 0x7C | 01 | DPF temperature |
| 0x7E | 01 | DPF differential pressure |
| 0x3C | 01 | Catalyst temp B1S1 (pre-cat) |
| 0x3E | 01 | Catalyst temp B1S2 (post-cat/pre-DPF) |
| 0x6B | 01 | EGT Bank 1 Sensor 1 |
| 0x6C | 01 | EGT Bank 1 Sensor 2 |

## DPF-Related DTCs

| DTC | Description |
|-----|-------------|
| P1498 | DPF system — PM accumulation excessive |
| P2002 | DPF efficiency below threshold (Bank 1) |
| P2463 | DPF restriction — soot accumulation |
| P244A | DPF differential pressure too low |
| P244B | DPF differential pressure too high |
| P2458 | DPF regeneration duration |
| P2459 | DPF regeneration frequency |
| P252F | Engine oil dilution level |

## UDS DPF Regeneration (Estimated)

The MUT-III tool uses a proprietary protocol over CAN for the DPF forced
regeneration function. This likely maps to:

- **UDS Service $31** (RoutineControl)
  - Sub-function: **0x01** (startRoutine)
  - Routine Identifier: **Proprietary** (unknown exact value)

The exact Routine ID is embedded in MUT-III software and is not publicly
documented. Common patterns for DENSO diesel ECUs suggest it may be in the
range 0xFF00-0xFFFF (manufacturer-specific routines) or 0x0200-0x02FF.

## DPF Driving Regeneration (Alternative to MUT-III)

If MUT-III is not available, and soot is below the critical threshold:
1. Ensure engine is fully warm (coolant temp stabilized, mid-gauge)
2. Drive at sustained motorway speed (60+ mph / 100+ km/h)
3. Maintain moderate load for 15-30 minutes
4. Exhaust temp needs to reach ~600°C for oxidation
5. DPF lamp should extinguish when complete
