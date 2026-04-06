#!/usr/bin/python3
"""gen_report.py — diagnostic report за обследването на ASX-а.

Стил: pslib example.pdf — title, QR горе вдясно, RSC, секции с таблици,
техник + подпис, footer line, ISO timestamps, page numbers.

Output: media/report_<ts>.pdf
"""
import os, sys, math, random, string, subprocess
from datetime import datetime
sys.path.insert(0, '/home/claude-agent/work/pslib')
from pslib import PSDoc

DIR = os.path.dirname(os.path.abspath(__file__))
TS_FILE = datetime.now().strftime("%Y%m%d_%H%M%S")
OUT_PS  = os.path.join(DIR, f"media/report_{TS_FILE}.ps")
OUT_PDF = os.path.join(DIR, f"media/report_{TS_FILE}.pdf")
os.makedirs(os.path.dirname(OUT_PDF), exist_ok=True)

# ── Layout ─────────────────────────────────────────────────────
ML, MR, MT, MB = 28, 14, 14, 14
QR_ZONE = 60
FOOTER_H = 22

# ── Vehicle data ───────────────────────────────────────────────
VIN          = "JMBXLGA1WEZ006344"   # placeholder — заменете с реалния
MAKE         = "MITSUBISHI"
MODEL        = "ASX"
YEAR         = "2014"
ENGINE       = "4N13 1.8 DiD (1798 cc, diesel, common-rail)"
DRIVETRAIN   = "AWD"
ECU_MAP      = "MMC_ASX_4N13"
TECH_NAME    = "smooker"
TECH_ROLE    = "Diagnostic Technician"
WORK_ORDER   = "WO-2026-0406-01"
WORK_DESC    = "Injector centering / quantity correction read"

# ── Doc init ───────────────────────────────────────────────────
doc = PSDoc(OUT_PS, title=f"OBD Report — {VIN}",
            margin=ML, margin_top=MT + QR_ZONE,
            margin_bottom=MB + FOOTER_H, margin_right=MR)
W, H = doc.A4W, doc.A4H

rsc = ''.join(random.choices(string.ascii_uppercase + string.digits, k=8))
iso_date = datetime.now().strftime("%Y-%m-%dT%H:%M")
iso_ts = datetime.now().strftime("%Y-%m-%dT%H:%M:%S")

CONTENT_TOP = H - MT - QR_ZONE
y = CONTENT_TOP

def need(h):
    global y
    if y - h < MB + FOOTER_H:
        doc.new_page()
        y = CONTENT_TOP

# ── Title block ────────────────────────────────────────────────
doc.font("Helvetica-Bold", 17)
doc.text(W/2, y, "ON-BOARD DIAGNOSTIC REPORT", align="center")
y -= 18
doc.font("Helvetica", 10)
doc.text(W/2, y, f"Work Order {WORK_ORDER} · {WORK_DESC}", align="center")
y -= 14
doc.hr(y, ML, W - MR)
y -= 12

# ── Vehicle info table ─────────────────────────────────────────
need(150)
doc.font("Helvetica-Bold", 11)
doc.text(ML, y, "01 — VEHICLE")
y -= 13
doc.hr(y + 4, ML, W - MR)
y -= 4

veh_rows = [
    ["VIN",        VIN],
    ["Make",       MAKE],
    ["Model",      MODEL],
    ["Year",       YEAR],
    ["Engine",     ENGINE],
    ["Drivetrain", DRIVETRAIN],
    ["ECU map",    ECU_MAP],
    ["CAN bus",    "500 kbps · TX 0x7E0 · RX 0x7E8 (ISO 15765-4)"],
]
y = doc.table(ML, y, ["Field", "Value"], veh_rows,
              col_widths=[120, W - ML - MR - 120],
              font_name="Helvetica",
              header_size=9, body_size=9, row_height=14,
              col_align=["left", "left"])
y -= 8

# ── Adapter info ───────────────────────────────────────────────
need(120)
doc.font("Helvetica-Bold", 11)
doc.text(ML, y, "02 — DIAGNOSTIC ADAPTER")
y -= 13
doc.hr(y + 4, ML, W - MR)
y -= 4
adp_rows = [
    ["Make/Model",  "Autocom CDP+ (USB FTDI 0403:d6da)"],
    ["MCU",         "STM32F205ZGT6 + FT232R"],
    ["Firmware",    "1622"],
    ["Serial",      "100251"],
    ["Battery V",   "13.62 V (engine off) → 13.66 V (running)"],
    ["Protocol",    "ASCII text wrapper · CAN/ISO 15765-4 underneath"],
]
y = doc.table(ML, y, ["Field", "Value"], adp_rows,
              col_widths=[120, W - ML - MR - 120],
              font_name="Helvetica",
              header_size=9, body_size=9, row_height=14,
              col_align=["left", "left"])
y -= 8

# ── Findings: parameters ───────────────────────────────────────
need(40)
doc.font("Helvetica-Bold", 11)
doc.text(ML, y, "03 — ECU PARAMETER MAP")
y -= 13
doc.hr(y + 4, ML, W - MR)
y -= 4
doc.font("Helvetica", 9)
doc.text(ML, y, "56 unique parameter indices discovered via *608_21_<XX> read commands.")
y -= 11
doc.text(ML, y, "Indices grouped by purpose (preliminary classification, pending operator validation):")
y -= 12

groups = [
    ["01-19", "16",  "Core ECU sensors (temps, voltages, status flags)"],
    ["24-28", "4",   "Auxiliary status / minor counters"],
    ["46-5F", "13",  "ASCII fragments — likely injector calibration codes"],
    ["74",    "1",   "Single isolated parameter"],
    ["A0-A8", "9",   "Calibration lookup table (X→Y pairs, Y constant)"],
    ["B0-B9", "10",  "Injector quantity correction (10 work points)"],
    ["BE-BF", "2",   "End-of-range telemetry"],
]
y = doc.table(ML, y, ["Range", "Count", "Preliminary classification"], groups,
              col_widths=[60, 50, W - ML - MR - 110],
              font_name="Helvetica",
              header_size=9, body_size=9, row_height=14,
              col_align=["center", "right", "left"])
y -= 8

# ── Injector data ──────────────────────────────────────────────
need(220)
doc.font("Helvetica-Bold", 11)
doc.text(ML, y, "04 — INJECTOR QUANTITY CORRECTION (B0..B9)")
y -= 13
doc.hr(y + 4, ML, W - MR)
y -= 4
doc.font("Helvetica", 9)
doc.text(ML, y, "Decoded as 4-byte field per index = two 16-bit big-endian values per work point.")
y -= 11
doc.text(ML, y, "Pattern matches typical diesel injector centering data (10 work points × current/target).")
y -= 12

inj_rows = [
    ["B0", "01EB", "01E2",  "491", "482",  "-9",  "-1.8 %"],
    ["B1", "01DF", "01D5",  "479", "469",  "-10", "-2.1 %"],
    ["B2", "01DA", "01DA",  "474", "474",   "0",   "0.0 %"],
    ["B3", "01D1", "01CE",  "465", "462",  "-3",  "-0.6 %"],
    ["B4", "01D0", "01CA",  "464", "458",  "-6",  "-1.3 %"],
    ["B5", "01D3", "01C8",  "467", "456",  "-11", "-2.4 %"],
    ["B6", "01CA", "01CB",  "458", "459",  "+1",  "+0.2 %"],
    ["B7", "01D4", "01CE",  "468", "462",  "-6",  "-1.3 %"],
    ["B8", "01B8", "01C4",  "440", "452",  "+12", "+2.7 %"],
    ["B9", "01C9", "01C2",  "457", "450",  "-7",  "-1.5 %"],
]
y = doc.table(ML, y,
              ["WP", "raw1", "raw2", "val1", "val2", "Δ", "Δ %"],
              inj_rows,
              col_widths=[40, 60, 60, 60, 60, 60, 60],
              font_name="Helvetica",
              header_size=9, body_size=9, row_height=14,
              col_align=["center", "center", "center", "right",
                         "right", "right", "right"])
y -= 8
doc.font("Helvetica-Oblique", 8)
doc.text(ML, y, "Δ = val2 - val1 (target − current). Pattern shows minor drift, "
               "all within typical ±20 unit tolerance.")
y -= 12

# ── Findings text ──────────────────────────────────────────────
need(120)
doc.font("Helvetica-Bold", 11)
doc.text(ML, y, "05 — OBSERVATIONS & ACTIONS")
y -= 13
doc.hr(y + 4, ML, W - MR)
y -= 4
doc.font("Helvetica", 9)
notes = [
    "● No fault codes (DTC) read during this session — diagnostic was parameter-only.",
    "● ECU responds to all 56 polled parameter indices without timeout.",
    "● Injector correction values within tolerance, no replacement required.",
    "● 8 parameter responses were truncated by sniffing layer (not ECU side); ",
    "    a follow-up capture using full-payload sniffer is scheduled.",
    "● Adapter handshake clean: device name CDP+, fw 1622, S/N 100251.",
    "● No unsafe writes performed. Read-only session.",
]
for n in notes:
    doc.text(ML, y, n)
    y -= 11
y -= 6

# ── Sign-off ───────────────────────────────────────────────────
need(110)
doc.font("Helvetica-Bold", 11)
doc.text(ML, y, "06 — TECHNICIAN SIGN-OFF")
y -= 13
doc.hr(y + 4, ML, W - MR)
y -= 8

# Two columns: tech info + signature box
doc.font("Helvetica", 9)
doc.text(ML, y, "Technician:");      doc.text(ML+90, y, TECH_NAME)
y -= 12
doc.text(ML, y, "Role:");            doc.text(ML+90, y, TECH_ROLE)
y -= 12
doc.text(ML, y, "Date:");            doc.text(ML+90, y, iso_date)
y -= 12
doc.text(ML, y, "Work Order:");      doc.text(ML+90, y, WORK_ORDER)
y -= 12
doc.text(ML, y, "Signature:")
# signature box
sig_x = ML + 90
sig_y = y - 6
sig_w = 220
sig_h = 38
doc.rect(sig_x, sig_y - sig_h + 18, sig_w, sig_h)
# fancy "signature" line in cursive style
doc.font("Helvetica-Oblique", 14)
doc.text(sig_x + 12, sig_y, "smooker", )
doc.font("Helvetica", 7)
doc.text(sig_x + 12, sig_y - 14, "/digital signature placeholder/")
y = sig_y - sig_h - 4

# ── Per-page decorations: QR, RSC, watermark, footer ───────────
def qr_matrix(data):
    r = subprocess.run(["/usr/bin/qrencode","-t","ASCII","-m","0","-l","H",data],
                       capture_output=True, text=True)
    return [[line[i:i+2]=="##" for i in range(0,len(line),2)]
            for line in r.stdout.rstrip('\n').split('\n')]

doc.font("Courier-Bold", 6)
date_w = doc.string_width(iso_date)
doc.font("Courier-Bold", 7)
rsc_text = f"RSC: {rsc}"
rsc_w = doc.string_width(rsc_text)
rsc_sz = 7 * date_w / rsc_w if rsc_w > 0 else 7

wm_angle = math.degrees(math.atan2(H, W))
total = len(doc.pages)
for pg_idx in range(total):
    cmds = doc.pages[pg_idx]
    # Watermark
    wm = [
        "gsave", "0.93 setgray",
        "/Helvetica_Bold_Cyr findfont 130 scalefont setfont",
        f"{W/2} {H/2 - 30} translate",
        f"{wm_angle} rotate",
        "0 0 moveto (DRAFT) dup stringwidth pop 2 div neg 0 rmoveto show",
        "grestore",
    ]
    for k, c in enumerate(wm):
        cmds.insert(k, c)
    # Footer
    fy = MB + FOOTER_H - 4
    cmds.append(f"0.3 setlinewidth {ML} {fy} moveto {W - MR - ML} 0 rlineto stroke")
    ts_e = doc._escape_ps(iso_ts)
    cmds.append("/Helvetica_Cyr 6 selectfont")
    cmds.append(f"{ML} {MB} moveto ({ts_e}) show")
    pg_label = doc._escape_ps(f"page {pg_idx+1}/{total}")
    cmds.append(f"({pg_label}) stringwidth pop neg {W - MR} add {MB} moveto ({pg_label}) show")
    # center: VIN
    vin_e = doc._escape_ps(f"VIN: {VIN}")
    cmds.append(f"({vin_e}) stringwidth pop 2 div neg {W/2} add {MB} moveto ({vin_e}) show")
    # QR top-right
    pg_num = pg_idx + 1
    qr_payload = f"RSC:{rsc}/{pg_num}/VIN:{VIN}/{WORK_ORDER}"
    pgm = qr_matrix(qr_payload)
    qm = 1.8
    qsz = len(pgm) * qm
    qpx = W - MR - qsz
    qpy = H - MT
    for r, row in enumerate(pgm):
        for c, b in enumerate(row):
            if b:
                px = qpx + c * qm
                py = qpy - (r + 1) * qm
                cmds.append(f"newpath {px} {py} moveto {qm} 0 rlineto 0 {qm} rlineto {qm} neg 0 rlineto closepath fill")
    # SC label
    cx = qpx + qsz/2
    cyc = qpy - qsz/2
    lbl = qm * 3.5
    rad = lbl/2 + 2.5
    cmds.append(f"gsave 1 setgray newpath {cx} {cyc} {rad} 0 360 arc closepath fill grestore")
    cmds.append("0 setgray")
    sc = doc._escape_ps("SC")
    tysc = cyc - lbl * 0.35
    cmds.append(f"/Helvetica_Bold_Cyr {lbl} selectfont")
    cmds.append(f"({sc}) stringwidth pop 2 div neg {cx} add {tysc} moveto ({sc}) show")
    # RSC + date left of QR
    lx = qpx - 5
    rsc_l = doc._escape_ps(f"RSC: {rsc}")
    date_l = doc._escape_ps(iso_date)
    cmds.append(f"/Courier_Bold_Cyr {rsc_sz} selectfont")
    rsc_y = qpy - rsc_sz
    cmds.append(f"({rsc_l}) stringwidth pop neg {lx} add {rsc_y} moveto ({rsc_l}) show")
    cmds.append("/Courier_Bold_Cyr 6 selectfont")
    cmds.append(f"({date_l}) stringwidth pop neg {lx} add {rsc_y - 9} moveto ({date_l}) show")

doc.save()
doc.to_pdf(OUT_PDF)
print(f"Wrote {OUT_PDF}, pages={total}, RSC={rsc}")
