# Sources & References

## ECU Identification & Calibration

- [ECU Backup — Mitsubishi ASX 1.8 1860C107 275700-2962 4N13 H16VRA6](https://www.ecubackup.com/mitsubishi/mitsubishi-asx/ecufile-mitsubishi-asx-1-8-1860c107-275700-2962-4n13-h16vra6-t1u1hdu1dm01-1/)
  - Confirmed: part number 1860C107, DENSO 275700-2962, calibration H16VRA6, hardware T1U1HDU1DM01

- [Dyno-ChiptuningFiles — Denso 4N13 H15VRA6 Original File](https://www.dyno-chiptuningfiles.com/original-ecu-files-database/denso-4n13-h15vra6/)
  - Calibration ID H15VRA6 for earlier 4N13 variant

- [ZipTuning — Denso H16YRA6 T1R2HDR2DF03](https://www.ziptuning.com/ecu-tuning-file/denso-undefined-h16yra6-t1r2hdr2df03-jmbmncj19yu0972091860c12705-ecu-tuning-files/)
  - Calibration H16YRA6, Mitsubishi PN 1860C127

- [OldSkullTuning — Calibration Numbers Identification](https://oldskulltuning.shop/pages/calibration-numbers-identification)
  - General DENSO calibration ID format explanation

## Flash Read/Write Tools & Methods

- [MHH AUTO — Mitsubishi ASX 1.8 2014 DENSO 4N13 ECU (R4F70580SV) how to read?](https://mhhauto.com/Thread-Mitsubishi-ASX-1-8-2014-DENSO-4N13-ECU-R4F70580SV-how-to-read)
  - Forum thread discussing read methods for exact ECU model

- [Digital Kaos — Denso 4N13 Renesas 7058 Mitsubishi ASX](https://www.digital-kaos.co.uk/forums/showthread.php/500303-Denso-4N13-Renesas-7058-Mitsubishi-ASX)
  - Forum discussion on 4N13 ECU architecture and reading

- [Digital Kaos — Mitsubishi ASX 1.8 2014 DENSO 4N13 ECU KTAG 2.25 CAN READ?](https://www.digital-kaos.co.uk/forums/showthread.php/856892-Mitsubishi-ASX-1-8-2014-DENSO-4N13-ECU-KTAG-2-25-CAN-READ)
  - KTAG compatibility discussion for this ECU

- [Digital Kaos — Need read Denso RA6](https://www.digital-kaos.co.uk/forums/showthread.php/702054-Need-read-Denso-RA6)
  - DENSO RA6 reading tools comparison

- [ecuedit.com — Denso RA6 Read](https://www.ecuedit.com/denso-ra6-read-t22371)
  - Tool recommendations for DENSO RA6

- [MHH AUTO — Mitsubishi ASX Denso RA6](https://mhhauto.com/Thread-Mitsubishi-ASX-Denso-RA6)
  - KESS V2 and OBD reading confirmed

- [AutoTuner — Denso 4N15 (SH7058)](https://www.autotuner.com/pages/ecu/denso-4n15-sh7058)
  - AutoTuner support for Denso SH7058 platform (read/write via OBD)

- [TuningTools — PCMFlash Module 42 Denso SH705X Bootloader](https://www.tuningtools.com/module-42-denso-sh705x-bootloader-for-pcm-flash)
  - PCMFlash support for SH7058 read/write/checksum

- [I/O Terminal — DENSO ECU Tool](https://ioterminal.com/?page_id=577)
  - I/O Terminal support: 64F7055/7058/7059, full R/W, K-LINE/CAN, Mitsubishi L200/Pajero/Outlander

- [Microtronik — RA6 Bench Mode with Hexprog II](https://www.microtronik.com/technical-info/hexprog/precision-cloning-exploring-ra6-bench-mode-with-hexprog-ii)
  - HexProg II bench mode for RA6 ECU cloning

- [ChiptuningShop — BitBox Denso SH7058/SH7059 CAN Module](https://chiptuningshop.com/product/bitbox-denso-sh7058-sh7059-can-module/)
  - BitBox OBD/Bench module for SH7058, 1MB flash size confirmed

## Tuning / Map Data

- [ChiptuningShop — BitEdit Mitsubishi Denso Diesel SH7058](https://chiptuningshop.com/product/bitedit-mitsubishi-denso-diesel-sh7058-module/)
  - Complete list of tuning maps: airmass, boost, injection timing, rail pressure, torque

- [The Automotive Clinic — Mitsubishi WinOLS Map Packs](https://theautomotiveclinic.com.au/mitsubishi-winols-map-packs/)
  - WinOLS map packs for Mitsubishi (Denso/Transtron SH7058/SH7059)

- [ecuedit.com — Denso: Damos, Mappacks & A2L](https://www.ecuedit.com/denso-t17150)
  - Forum for DENSO A2L/DAMOS files

- [ZipTuning — Mitsubishi Damos/A2L files](https://www.ziptuning.com/damos-files/mitsubishi/)
  - Mitsubishi damos files (availability varies)

- [ECULinks — WinOLS MapPacks](https://eculinks.com/winols-mappacks)
  - General WinOLS mappack resources

## Engine & Common Rail System

- [Service-engine.com.ua — 4N13, 4N14 Common Rail System PDF](https://www.service-engine.com.ua/webroot/pdf/MITSUBISHI%204N13,%204N14%20ENGINES.pdf)
  - DENSO CRS documentation: injector specs, supply pump, rail pressure 200 MPa, SV3 SCV, sensor specs, ECU connector terminal layouts

- [Scribd — Mitsubishi 4N13, 4N14 Engines PDF](https://www.scribd.com/document/333689799/MITSUBISHI-4N13-4N14-ENGINES-pdf)
  - Same document hosted on Scribd

- [Wikipedia — Mitsubishi 4N1 Engine](https://en.wikipedia.org/wiki/Mitsubishi_4N1_engine)
  - Engine specs: 83mm bore, 83.1mm stroke, 14.9:1 CR, DOHC 16v MIVEC

- [AutoManiac — Mitsubishi 1.8 4N13 116hp](https://www.automaniac.org/engine/mitsubishi/1182/mitsubishi-1798cc-diesel-1.8-4n13-16v-116hp)
  - Engine specifications

## DPF System

- [Mitsubishi Workshop Manual — Forcible DPF Regeneration (ASX 2013)](http://mitsubishi.automotive-manuals.com/asx-2013/M117500680019100ENG.HTML)
  - OEM procedure for forced DPF regeneration

- [MMC-Manuals — DPF Regeneration (L200 2020)](http://mmc-manuals.ru/manuals/l200_v/online/Service_Manual_v2/2020/M1/html/M11750068A005700ENG.html)
  - Detailed forced regen procedure: 600°C, 25 min, Item 118 < 115°C precondition

- [MHH AUTO — Mitsubishi ASX Denso 4N13 DPF problem](https://mhhauto.com/Thread-Mitsubishi-ASX-Denso-4N13-DPF-problem)
  - Forum discussion on 4N13 DPF issues

- [MHH AUTO — Need DPF OFF help in Mitsubishi ASX](https://mhhauto.com/Thread-Need-DPF-OFF-help-in-Mitsubishi-ASX)
  - DPF removal/disable discussion

- [Digital Kaos — DPF off Mitsubishi 1.8 DID Denso 4N13](https://www.digital-kaos.co.uk/forums/archive/index.php/t-939613.html)
  - DPF disable for 4N13

- [Mitsubishi Forums — DPF figures](https://www.mitsubishi-forums.com/threads/can-anybody-make-sense-of-these-dpf-figures.263329/)
  - User DPF data readings

- [Mitsubishi Forums — DPF regen every few miles](https://www.mitsubishi-forums.com/threads/asx-1-8-diesel-dpf-regen-every-few-miles.263322/)

- [MitsHelp — DPF Owner's Manual](https://www.mitshelp.com/micont-1412.html)
  - DPF regeneration intervals: 500-600 km normal

- [ManualsLib — Mitsubishi ASX DPF Manual Page](https://www.manualslib.com/manual/1141878/Mitsubishi-Asx.html?page=167)
  - Owner's manual DPF section

## Injector Coding

- [MMC-Manuals — Injector ID Registration (Eclipse Cross)](http://mmc-manuals.ru/manuals/eclipse_cross/online/Service_Manual/M1/html/M10010126A000300ENG.html)
  - 30-character code, 8-part input, MUT-III SE procedure

- [Denco Diesel — Coding Common Rail Injectors](https://www.dencodiesel.com/pages/coding-common-rail-injectors-and-pilot-relearn)
  - Injector coding overview: code on solenoid, sensitivity, pilot quantity learning

- [MHH AUTO — Mitsubishi ASX Injector Code](https://mhhauto.com/Thread-mitsubishi-ASX-injector-code)
  - ASX-specific injector coding discussion

- [NewTriton.net — Fuel Injector Calibration Codes](https://www.newtriton.net/phpbb/viewtopic.php?t=22793)
  - Injector calibration code format discussion

- [Mitsubishi Forums — Recode Injectors](https://www.mitsubishi-forums.com/threads/recode-injectors-after-replacing-them.262363/)

## MCU / Microcontroller

- [Renesas — V850ES/Fx3 Series User Manual](https://www.manualslib.com/manual/1949545/Renesas-V850es-Fx3-Series.html)
  - MCU architecture, flash self-programming, FLMD0 boot mode

- [Renesas — Flash Self-Programming Application Note](https://www.renesas.com/en/document/apn/flash-self-programming-v850es-microcontrollers-application-note)
  - V850ES flash programming library and procedures

- [Renesas Community — V850ES/Fx3-L Programming](https://community.renesas.com/mcu/legacy-mcu/v850/f/v850---forum/29786/v850es-fx3-l-programming)
  - FLMD0 HIGH + FLMD1 LOW for UART boot mode entry

- [GitHub — gregjhogan/renesas-bootmode](https://github.com/gregjhogan/renesas-bootmode)
  - Open source serial interface for Renesas boot mode (V850E2, SH-2A)

- [CarProTool — NEC/Renesas V850 Programmer](https://carprotool.com/forum/forum/cpt/nec-renesas-v850-core-programmer/137-nec-renesas-v850-series-programmer)
  - V850 direct programming tool

- [Octopart — R4F70580SV](https://octopart.com/r4f70580sv-renesas-84689797)
  - Component distributor listing, QFP-256 package

## Wiring & Pinout

- [ecu.design — Pinout Denso SH72453 RA6 4N14 DSM Mitsubishi](https://ecu.design/ecu-pinout/pinout-denso-sh72453-ra6-4n14-dsm-mitsubishi/)
  - DENSO RA6 4N14 pinout (behind paywall at time of access)

- [Scribd — Diagrama Eletrico Motor ASX](https://www.scribd.com/doc/316198124/Diagrama-Eletrico-Motor-Asx)
  - ASX engine wiring diagram (Scribd, requires subscription)

- [Scribd — I/O Terminal ECU Wiring](https://www.scribd.com/document/454981834/iot-ecuwiring-pdf)
  - I/O Terminal wiring connections for various ECUs including DENSO

- [MHH AUTO — 2010 Mitsubishi ASX 4N13 wiring diagram](https://mhhauto.com/Thread-2010-mitsubishi-asx-4n13-wiring-diagram)
  - Forum request for ASX wiring diagram

- [PinoutGuide — Mitsubishi OBD-II diagnostic connector](https://pinoutguide.com/CarElectronics/mitsubishi_obd2_daig_pinout.shtml)
  - Standard Mitsubishi OBD-II port pinout

- [Tuning Technology — Denso 76-Pin ECU Connector](https://www.tuningtechnology.net/ecu-headers/denso-76-pin-ecu-connector)
  - Physical connector header for DENSO 76-pin

## DTC Codes

- [TroubleCodes.net — Mitsubishi OBD/OBD2 Codes](https://www.troublecodes.net/mitsu/)
  - General Mitsubishi DTC reference

- [30MinuteDPFClean — Mitsubishi Fault Code List](https://30minutedpfclean.com/mitsubishi-fault-code-list/)
  - Mitsubishi fault code database

- [Scribd — Mitsubishi DTC Fault Codes Guide](https://www.scribd.com/document/575970035/Mitsubishi-Trouble-Code-Chart)

- [Scribd — Mitsubishi DTC Codes Overview](https://www.scribd.com/doc/211616306/Mitsubishi-Dtc-Code)

## OBD2 PIDs

- [Mitsubishi Outlander PHEV Forum — List of OBDII parameters](https://www.myoutlanderphev.com/threads/list-of-obdii-parameters.1655/)

- [MirageForum — Custom OBD PIDs](https://mirageforum.com/forum/showthread.php/6449-Custom-OBD-PIDs)
  - Mitsubishi Mode $21 PIDs (gasoline, limited diesel applicability)

- [Pajero Forum — OBDII and Mitsubishi specific PIDs](https://www.pajeroforum.com.au/forum/vehicles/challenger/pb-pc-challenger-2009-2014/31808-obdii-and-mitsubishi-specific-pids)

- [MUT for Torque Plugin](https://xkaixrezza-mut-ii.andro.io/)
  - Torque Pro plugin for MUT protocol (ASX listed as incompatible)

## Workshop Manuals

- [MMC-Manuals — ASX Service Manual 2018](https://mmc-manuals.ru/manuals/asx/online/Service_Manual/2018/index_M1.htm)
  - Online service manual index

- [ManualsLib — Mitsubishi ASX Manual](https://www.manualslib.com/manual/1141878/Mitsubishi-Asx.html)
  - Owner's manual reference

- [WorkshopManuals.org — Mitsubishi ASX 2013-2015](https://workshopmanuals.org/product/mitsubishi-asx-workshop-manual-download-2013-2015/)
  - Workshop manual purchase

- [EPC Depo — Mitsubishi ASX 2010-2015 Service Manual](https://epcdepo.com/mitsubishi-asx-service.html)

## Sensor Part Numbers

- [Amazon — DPF Differential Exhaust Pressure Sensor 4N13 1865A184](https://www.amazon.com/Mitsubishi-Differential-Pressure-1-8Diesel-1865A184/dp/B0B3XVZLSY)
- [SensorGal — Exhaust Pressure Sensor ASX XA/XB/XC 1865A210](https://www.sensorgal.com/exhaust-pressure-sensor-for-mitsubishi-asx-xa-xb-x)
- [Baileys Diesel — Denso Injector 4N13/4N14](https://www.baileysdiesel.com/store/Denso-Injector-to-suit-Mitsubishi-4N13-4N14-p440200579)

## Tuning Performance References

- [BlueSpark — Mitsubishi ASX 1.8 DID 115hp ECU Remap](https://bluesparkautomotive.com/mitsubishi-asx-1798cc-115ps-300nm-diesel-ecu-remap)
- [TC Performance — Mitsubishi ASX 1.8 DID 115hp](https://www.tc-performance.com/catalog/mitsubishi/asx/all/18-did-115hp)
- [MyChiptuningFiles — Mitsubishi ASX 1.8 DID 150hp](https://mychiptuningfiles.com/en/chiptuning-files/mitsubishi/mitsubishi-asx/mitsubishi-asx-1-8-did-150hp)
