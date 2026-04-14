# Injector Coding — Mitsubishi 4N13

## Overview

Each DENSO common rail injector has unique manufacturing tolerances. The ECU
stores per-cylinder correction data to compensate for these variations,
improving injection accuracy and reducing emissions/noise.

Injector coding must be performed when:
- Replacing any injector
- Replacing the ECU
- After ECU flash/reprogram that clears EEPROM

## Injector Identification Code

### Format
- **30 alphanumeric characters** printed on the injector
- Located on the **injector connector** (solenoid body) and **upper plane**
- Code is divided into **8 parts** for input into MUT-III

### What the Code Contains
- Individual injector flow rate correction data
- Nozzle spray pattern compensation
- Response time compensation
- The code adjusts **injection timing** for each cylinder

### Injector Part Numbers (4N13)
| Part Number | Notes |
|-------------|-------|
| 295050-0120 | DENSO common rail injector for 4N13 |
| 1465A323 | Mitsubishi OE part number |
| 1465A306 | Alternative Mitsubishi part number |

### QR Code
Injectors manufactured from September 2008 onwards have:
- Different nozzle specifications vs earlier versions
- **QR code** printed on the injector body
- QR code encodes the same 30-character correction data
- Can be scanned with a QR reader instead of manual entry

## Coding Procedure — Injector Replacement

### Using MUT-III SE

1. Connect M.U.T.-III SE to diagnosis connector
2. Ignition ON (engine OFF)
3. Select engine system → **"Inj.ID Writing (Exchanging INJ.)"**
4. Select the specific cylinder being replaced (Cyl 1/2/3/4)
5. Input the 30-character identification code:
   - Code is divided into 8 fields
   - Enter each section in the designated input field
   - **One digit wrong = code rejected** (extremely sensitive)
6. Confirm and write to ECU
7. Execute **"Small injection quantity learning"** (pilot quantity relearn)
8. Clear any DTCs
9. Start engine, verify:
   - Engine warning lamp is OFF
   - No diagnosis codes set
   - Engine runs smoothly at idle

### Using MUT-III SE — ECU Replacement

1. **Before removing old ECU**:
   - Connect MUT-III SE
   - Select **"Inj.ID Writing (Exchanging ECU)"**
   - Select **"Injector ID Read & Save"**
   - This saves all 4 injector codes from the old ECU

2. **After installing new ECU**:
   - Connect MUT-III SE
   - Select **"Saved Injector ID Writing"**
   - Tool transfers saved codes to new ECU
   - Execute pilot quantity relearn

### Alternative: Manual Code Entry on New ECU
If the old ECU is dead and codes were not saved:
- Read the 30-character code from each physical injector
- Enter codes for all 4 cylinders manually
- Cylinder order: #1 = timing belt end

## Pilot Quantity Learning (Relearn)

After any injector coding change:
1. MUT-III SE → Special Function → **"Small injection quantity learning"**
2. Conditions: engine at idle, coolant temp > 70°C
3. Process runs automatically (~30 seconds)
4. ECU adjusts pilot injection quantities based on new correction data

## Third-Party Tool Support

| Tool | Injector Coding Support |
|------|------------------------|
| MUT-III SE | Full support (OEM tool) |
| Launch X-431 | Supported on some versions |
| Autel MaxiSys | Supported (Mitsubishi special function) |
| G-Scan / G-Scan2 | Supported |
| CGSULIT SC530 | Mitsubishi-specific, supports coding |
| Generic ELM327 | **NOT supported** — requires special function, not standard OBD |

## Notes

- The injector coding procedure does NOT require removing the injectors
  from the engine — only the codes need to be entered
- If codes are lost and injectors are installed, they must be physically
  removed to read the codes from the solenoid body
- Running the engine without correct injector codes will cause:
  - Rough idle
  - Increased emissions
  - Potential DTC P1250 or similar
  - Knock/combustion noise
