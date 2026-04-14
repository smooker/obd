# Diagnostic Trouble Codes — Mitsubishi 4N13 Diesel

## Fuel System

| DTC | Description |
|-----|-------------|
| P0087 | Fuel rail/system pressure — too low |
| P0088 | Fuel rail/system pressure — too high |
| P0089 | Fuel pressure regulator performance / SCV stuck |
| P0091 | Fuel pressure regulator control circuit low |
| P0092 | Fuel pressure regulator control circuit high |
| P0093 | Fuel system leak detected — large leak |
| P0094 | Fuel system leak detected — small leak |
| P0190 | Fuel rail pressure sensor circuit malfunction |
| P0191 | Fuel rail pressure sensor range/performance |
| P0192 | Fuel rail pressure sensor circuit low |
| P0193 | Fuel rail pressure sensor circuit high |
| P0200 | Injector circuit malfunction |
| P0201 | Injector circuit — cylinder 1 |
| P0202 | Injector circuit — cylinder 2 |
| P0203 | Injector circuit — cylinder 3 |
| P0204 | Injector circuit — cylinder 4 |
| P0251 | Injection pump fuel metering control A — malfunction |
| P0253 | Injection pump fuel metering control A — low |
| P0254 | Injection pump fuel metering control A — high |

## Turbocharger / Boost

| DTC | Description |
|-----|-------------|
| P0234 | Turbocharger/supercharger overboost condition |
| P0235 | Turbocharger boost sensor A circuit malfunction |
| P0236 | Turbocharger boost sensor A range/performance |
| P0237 | Turbocharger boost sensor A circuit low |
| P0238 | Turbocharger boost sensor A circuit high |
| P0243 | Turbocharger wastegate solenoid A malfunction |
| P0244 | Turbocharger wastegate solenoid A range/performance |
| P0299 | Turbocharger/supercharger underboost condition |
| P006A | MAP — barometric pressure correlation |

## EGR System

| DTC | Description |
|-----|-------------|
| P0400 | EGR flow malfunction |
| P0401 | EGR flow insufficient detected |
| P0402 | EGR flow excessive detected |
| P0403 | EGR control circuit malfunction |
| P0404 | EGR control circuit range/performance |
| P0405 | EGR sensor A circuit low |
| P0406 | EGR sensor A circuit high |
| P0407 | EGR sensor B circuit low |
| P0408 | EGR sensor B circuit high |
| P0489 | EGR control circuit low |
| P0490 | EGR control circuit high |

## DPF (Diesel Particulate Filter)

| DTC | Description |
|-----|-------------|
| P1498 | DPF system — PM accumulation excessive |
| P2002 | DPF efficiency below threshold (Bank 1) |
| P2003 | DPF efficiency below threshold (Bank 2) |
| P2458 | DPF regeneration duration |
| P2459 | DPF regeneration frequency |
| P2463 | DPF restriction — soot accumulation |
| P244A | DPF differential pressure too low (sensor/piping) |
| P244B | DPF differential pressure too high |
| P2452 | DPF pressure sensor A circuit |
| P2453 | DPF pressure sensor A range/performance |
| P2454 | DPF pressure sensor A circuit low |
| P2455 | DPF pressure sensor A circuit high |

## Exhaust Gas Temperature

| DTC | Description |
|-----|-------------|
| P0544 | Exhaust gas temperature sensor 1 circuit (pre-cat) |
| P0545 | Exhaust gas temp sensor 1 circuit low |
| P0546 | Exhaust gas temp sensor 1 circuit high |
| P2031 | Exhaust gas temp sensor 2 circuit (pre-DPF) |
| P2032 | Exhaust gas temp sensor 2 circuit low |
| P2033 | Exhaust gas temp sensor 2 circuit high |
| P2080 | Exhaust gas temp sensor 1 range/performance (Bank 1) |
| P2084 | Exhaust gas temp sensor 2 range/performance (Bank 1) |

## Air Intake / MAF

| DTC | Description |
|-----|-------------|
| P0100 | MAF sensor circuit malfunction |
| P0101 | MAF sensor range/performance |
| P0102 | MAF sensor circuit low |
| P0103 | MAF sensor circuit high |
| P0110 | Intake air temperature sensor 1 circuit |
| P0112 | Intake air temperature sensor 1 circuit low |
| P0113 | Intake air temperature sensor 1 circuit high |

## Coolant / Oil Temperature

| DTC | Description |
|-----|-------------|
| P0115 | Engine coolant temperature sensor circuit |
| P0116 | Engine coolant temperature sensor range/performance |
| P0117 | Engine coolant temperature sensor circuit low |
| P0118 | Engine coolant temperature sensor circuit high |
| P0196 | Engine oil temperature sensor range/performance |
| P0197 | Engine oil temperature sensor circuit low |
| P0198 | Engine oil temperature sensor circuit high |

## Engine Position / Speed

| DTC | Description |
|-----|-------------|
| P0335 | Crankshaft position sensor A circuit malfunction |
| P0336 | Crankshaft position sensor A range/performance |
| P0340 | Camshaft position sensor A circuit (Bank 1) |
| P0341 | Camshaft position sensor A range/performance |

## Throttle / Pedal

| DTC | Description |
|-----|-------------|
| P2122 | Accelerator pedal position sensor 1 circuit low |
| P2123 | Accelerator pedal position sensor 1 circuit high |
| P2127 | Accelerator pedal position sensor 2 circuit low |
| P2128 | Accelerator pedal position sensor 2 circuit high |
| P2138 | Accelerator pedal position sensor 1/2 correlation |

## Glow Plug System

| DTC | Description |
|-----|-------------|
| P0380 | Glow plug/heater circuit A malfunction |
| P0381 | Glow plug/heater indicator circuit malfunction |
| P0670 | Glow plug module control circuit |
| P0671 | Glow plug cylinder 1 circuit |
| P0672 | Glow plug cylinder 2 circuit |
| P0673 | Glow plug cylinder 3 circuit |
| P0674 | Glow plug cylinder 4 circuit |

## Engine Oil Dilution

| DTC | Description |
|-----|-------------|
| P252F | Engine oil dilution level (fuel in oil from DPF regen) |

## Communication

| DTC | Description |
|-----|-------------|
| U0100 | Lost communication with ENGINE ECU/PCM |
| U0073 | Control module communication bus A off |
| U0101 | Lost communication with TCM |
| U0122 | Lost communication with ASC (stability control) |

## Mitsubishi-Specific (Manufacturer)

| DTC | Description |
|-----|-------------|
| P1250 | Injector correction data error |
| P1498 | DPF PM accumulation excessive (Mitsubishi-specific) |
| P1605 | ECU internal malfunction |

## Notes

- DTCs are read via OBD2 Mode 03 (service $03) or UDS service $19
- Mitsubishi uses the standard ISO 15031-6 format (P0xxx = generic, P1xxx = manufacturer)
- MUT-III can read both standard and enhanced (manufacturer-specific) codes
- Some codes have sub-codes (e.g., P2122-00) indicating specific failure modes
- Freeze frame data is available via Mode 02 (service $02)
