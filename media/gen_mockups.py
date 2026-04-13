#!/usr/bin/python3
"""gen_mockups.py — генерира futuristic OBD interface mockup PNG-та.

Стил: dark cyberpunk, neon cyan/green/amber акценти, монospace font, ASCII
art frames, terminal-like layout. Размер по подразбиране 1280x720 (16:9).

Output: media/mockup_<NN>_<name>.png
"""
import os
from PIL import Image, ImageDraw, ImageFont

DIR = os.path.dirname(os.path.abspath(__file__))

# Палитра — dark cyberpunk
BG       = (12, 14, 20)
PANEL    = (20, 24, 32)
PANEL_HI = (28, 34, 46)
CYAN     = (0, 240, 255)
CYAN_DIM = (0, 120, 140)
GREEN    = (80, 255, 140)
GREEN_DIM= (40, 130, 70)
AMBER    = (255, 180, 40)
RED      = (255, 80, 80)
GREY     = (120, 130, 150)
WHITE    = (220, 230, 240)
BORDER   = (60, 80, 110)

W, H = 1920, 1080

# --- PCF bitmap text rendering via pcftext (FreeType, native pixels) ---
import subprocess, tempfile, struct
PCFDIR = "/home/claude-agent/work/csrecombing/csvision/resources/fonts"
PCFTEXT = os.path.join(DIR, "pcftext")
PCF_NATIVE = [12, 14, 16, 18, 20, 22, 24]  # available ter-x{N}{n,b}.pcf.gz

class PCFFont:
    __slots__ = ("size", "bold")
    def __init__(self, size, bold):
        self.size = size
        self.bold = bold

def font(sz, bold=False):
    # snap to nearest available PCF strike
    if sz not in PCF_NATIVE:
        sz = min(PCF_NATIVE, key=lambda x: abs(x - sz))
    return PCFFont(sz, bold)

_glyph_cache = {}

def _render_text(text, f):
    key = (text, f.size, f.bold)
    if key in _glyph_cache:
        return _glyph_cache[key]
    pcf = f"{PCFDIR}/ter-x{f.size}{'b' if f.bold else 'n'}.pcf.gz"
    with tempfile.NamedTemporaryFile(suffix=".pbm", delete=False) as tf:
        tmp = tf.name
    try:
        out = subprocess.check_output(
            [PCFTEXT, pcf, str(f.size), text, tmp]).decode().strip()
        w, h, base = map(int, out.split())
        with open(tmp, "rb") as fh:
            line = fh.readline()           # P4
            line = fh.readline()           # w h (skip comments)
            while line.startswith(b"#"): line = fh.readline()
            data = fh.read()
        # PBM P4: 1 = ink. Build a mask (L) from bits.
        mask = Image.frombytes("1", (w, h), data).convert("L")
        _glyph_cache[key] = (mask, w, h, base)
        return _glyph_cache[key]
    finally:
        os.unlink(tmp)

class TextDraw:
    """Wraps ImageDraw — overrides .text()/.textlength() to use pcftext."""
    def __init__(self, img):
        self._img = img
        self._d = ImageDraw.Draw(img)
    def __getattr__(self, name):
        return getattr(self._d, name)
    def textlength(self, text, font=None, **kw):
        if isinstance(font, PCFFont) and text:
            _, w, _, _ = _render_text(text, font)
            return w
        return self._d.textlength(text, font=font, **kw)
    def textbbox(self, xy, text, font=None, **kw):
        if isinstance(font, PCFFont) and text:
            _, w, h, _ = _render_text(text, font)
            x, y = xy
            return (x, y, x + w, y + h)
        return self._d.textbbox(xy, text, font=font, **kw)
    def text(self, xy, text, font=None, fill=(255,255,255), **kw):
        if not isinstance(font, PCFFont) or not text:
            return self._d.text(xy, text, font=font, fill=fill, **kw)
        mask, w, h, base = _render_text(text, font)
        # PIL d.text uses xy as top-left; same here.
        x, y = int(xy[0]), int(xy[1])
        if isinstance(fill, str):
            fill = (255,255,255)
        tile = Image.new("RGB", (w, h), fill)
        self._img.paste(tile, (x, y), mask)

def new_canvas():
    img = Image.new("RGB", (W, H), BG)
    d = TextDraw(img)
    return img, d

def panel(d, x, y, w, h, title=None, color=CYAN):
    """Draw a bordered panel with optional title bar."""
    # outer border
    d.rectangle([x, y, x+w, y+h], outline=color, width=1)
    d.rectangle([x+2, y+2, x+w-2, y+h-2], fill=PANEL)
    # corner ticks
    L = 8
    for cx, cy in [(x, y), (x+w, y), (x, y+h), (x+w, y+h)]:
        dx = -1 if cx == x+w else 1
        dy = -1 if cy == y+h else 1
        d.line([cx, cy, cx+dx*L, cy], fill=color, width=2)
        d.line([cx, cy, cx, cy+dy*L], fill=color, width=2)
    if title:
        f = font(11, bold=True)
        tw = d.textlength(title, font=f)
        d.rectangle([x+10, y-7, x+10+tw+12, y+7], fill=BG)
        d.text((x+16, y-6), title, font=f, fill=color)

def header_bar(d):
    f = font(13, bold=True)
    f2 = font(11)
    # Top bar
    d.rectangle([0, 0, W, 32], fill=PANEL_HI)
    d.line([0, 32, W, 32], fill=CYAN, width=1)
    d.text((16, 9), "■ obd.smooker.org", font=f, fill=CYAN)
    d.text((W-340, 11), "AUTOCOM CDP+ │ FW 1622 │ S/N 100251", font=f2, fill=GREEN)
    d.text((W-90, 11), "● ONLINE", font=f2, fill=GREEN)
    # status dot
    d.ellipse([W-110, 14, W-100, 24], fill=GREEN)

def footer_bar(d, msg=""):
    f = font(10)
    d.rectangle([0, H-22, W, H], fill=PANEL_HI)
    d.line([0, H-22, W, H-22], fill=CYAN_DIM, width=1)
    d.text((16, H-16), msg or "[F1] Help  [F2] Connect  [F5] Refresh  [F8] Live  [F10] Quit", font=f, fill=GREY)
    d.text((W-160, H-16), "lat: 12ms │ tx: 142B/s", font=f, fill=GREEN_DIM)

def grid_bg(d):
    """Subtle grid background"""
    for x in range(0, W, 40):
        d.line([x, 32, x, H-22], fill=(20, 24, 34), width=1)
    for y in range(40, H-22, 40):
        d.line([0, y, W, y], fill=(20, 24, 34), width=1)

# ─────────────────────────────────────────────────────────────────
# Mockup 01: Dashboard / Connection screen
# ─────────────────────────────────────────────────────────────────
def mockup_01_dashboard():
    img, d = new_canvas()
    grid_bg(d)
    header_bar(d)

    # Big title
    f_big = font(32, bold=True)
    d.text((40, 56), "VEHICLE DIAGNOSTICS", font=f_big, fill=CYAN)
    d.text((40, 96), "ECU PROBE — Mitsubishi ASX 2014 1.8 DiD (4N13 AWD)", font=font(14), fill=GREEN_DIM)
    d.text((40, 116), f"session 76030.179s · 118,971 packets · 56 params decoded", font=font(12), fill=GREY)

    # Vehicle info panel — top left
    panel(d, 40, 150, 720, 290, "VEHICLE")
    f = font(14)
    fb = font(14, bold=True)
    rows = [
        ("VIN",         "JMBXLGA1WEZ006344",       GREEN),
        ("Make",        "MITSUBISHI",              WHITE),
        ("Model",       "ASX",                     WHITE),
        ("Year",        "2014",                    WHITE),
        ("Engine",      "4N13 1.8 DiD",            WHITE),
        ("Displacement","1798 cc · DOHC 16v",      WHITE),
        ("Fuel",        "Diesel · Common Rail",    WHITE),
        ("Drivetrain",  "AWD · 6-speed manual",    WHITE),
        ("ECU Map",     "MMC_ASX_4N13",            AMBER),
        ("CAN Speed",   "500 kbps · ISO 15765-4",  CYAN),
        ("CAN IDs",     "TX 0x7E0 · RX 0x7E8",     CYAN),
        ("Mileage",     "183,420 km",              WHITE),
    ]
    for i, (k, v, col) in enumerate(rows):
        y = 178 + i*20
        d.text((60, y), k.upper(), font=f, fill=GREY)
        d.text((220, y), v, font=fb, fill=col)

    # Live data panel — gauges
    panel(d, 780, 150, 1100, 290, "LIVE TELEMETRY")
    gauges = [
        ("BATTERY",   "13.66", "V",     GREEN),
        ("COOLANT",   "  87",  "°C",    GREEN),
        ("INTAKE T",  "  42",  "°C",    GREEN),
        ("RPM",       " 760",  "/min",  CYAN),
        ("MAF",       "12.4",  "g/s",   AMBER),
        ("BOOST",     " 0.02", "bar",   GREEN_DIM),
        ("FUEL T",    " 1.2",  "ms",    CYAN),
        ("RAIL P",    " 320",  "bar",   AMBER),
        ("OIL T",     "  91",  "°C",    GREEN),
        ("EGR",       "  18",  "%",     CYAN),
        ("DPF SOOT",  "  6.4", "g",     GREEN_DIM),
        ("VOLTAGE",   "13.66", "V",     GREEN),
    ]
    for i, (k, v, u, col) in enumerate(gauges):
        col_x = 800 + (i % 4) * 270
        col_y = 180 + (i // 4) * 90
        d.text((col_x, col_y), k, font=font(12), fill=GREY)
        d.text((col_x, col_y+18), v, font=font(32, bold=True), fill=col)
        d.text((col_x+140, col_y+38), u, font=font(14), fill=col)

    # ECU map panel — fancy hex grid (16x16) + selected param details strip
    panel(d, 40, 460, 1300, 530, "ECU PARAMETER MAP — *608_21_<XX> read commands · 56/256 indices")
    f_hex = font(14, bold=True)
    f_lbl = font(12)
    f_val = font(12)
    cols, rows = 16, 16
    cell_w, cell_h = 76, 28
    start_x, start_y = 90, 510
    # column header
    for c in range(cols):
        d.text((start_x + c*cell_w + 22, start_y - 22), f"{c:02X}", font=f_lbl, fill=CYAN)
    # row labels
    for r in range(rows):
        d.text((start_x - 36, start_y + r*cell_h + 6), f"{r:X}0", font=f_lbl, fill=CYAN)

    # Known params + first byte preview
    known_data = {
        0x02:"547F", 0x03:"0005", 0x04:"D500", 0x08:"0000", 0x09:"B30F",
        0x10:"4140", 0x11:"....", 0x12:"4242", 0x13:"05CC", 0x14:"FFFA",
        0x15:"0000", 0x16:"3CBA", 0x17:"EA64", 0x18:"4301", 0x19:"0000",
        0x24:"6D3F", 0x26:"801A", 0x27:"0000", 0x28:"0001",
        0x46:"2F36", 0x47:"0000", 0x49:"0000", 0x4A:"0E38", 0x4B:"7514",
        0x4C:"2323", 0x4E:"....", 0x4F:"3737", 0x51:"....",
        0x58:"0000", 0x59:"0031", 0x5B:"4141", 0x5D:"05CC", 0x5F:"0000",
        0x74:"F3CC",
        0xA0:"0A79", 0xA1:"1E79", 0xA2:"....", 0xA3:"4679", 0xA4:"5A79",
        0xA5:"....", 0xA6:"....", 0xA7:"9679", 0xA8:"0000",
        0xB0:"01EB", 0xB1:"01DF", 0xB2:"01DA", 0xB3:"01D1", 0xB4:"01D0",
        0xB5:"01D3", 0xB6:"01CA", 0xB7:"01D4", 0xB8:"01B8", 0xB9:"01C9",
        0xBE:"02DC", 0xBF:"....",
    }
    truncated = {0x01, 0x11, 0x4E, 0x51, 0xA2, 0xA5, 0xA6, 0xBF}
    selected = 0xB5
    for r in range(rows):
        for c in range(cols):
            idx = r * 16 + c
            x1 = start_x + c*cell_w
            y1 = start_y + r*cell_h
            label = f"{idx:02X}"
            cw = cell_w - 4
            ch = cell_h - 4
            if idx == selected:
                # selected — bright cyan inverted highlight
                d.rectangle([x1-1, y1-1, x1+cw+1, y1+ch+1], fill=CYAN, outline=CYAN)
                d.text((x1+5, y1+2), label, font=f_hex, fill=BG)
                d.text((x1+30, y1+8), known_data.get(idx, ""), font=f_val, fill=BG)
            elif idx in truncated:
                col = AMBER
                d.rectangle([x1, y1, x1+cw, y1+ch], outline=col, width=1)
                d.text((x1+5, y1+2), label, font=f_hex, fill=col)
                d.text((x1+30, y1+8), "···", font=font(12), fill=col)
            elif idx in known_data:
                col = GREEN
                d.rectangle([x1, y1, x1+cw, y1+ch], fill=(15, 40, 25), outline=col)
                d.text((x1+5, y1+2), label, font=f_hex, fill=col)
                d.text((x1+30, y1+8), known_data[idx], font=f_val, fill=WHITE)
            else:
                d.text((x1+5, y1+2), label, font=f_hex, fill=(40, 55, 75))

    # Legend below grid
    lg_y = 968
    d.rectangle([90, lg_y, 110, lg_y+14], fill=(15, 40, 25), outline=GREEN)
    d.text((118, lg_y), "decoded (56)", font=font(12), fill=GREEN)
    d.rectangle([300, lg_y, 320, lg_y+14], outline=AMBER, width=1)
    d.text((328, lg_y), "truncated (8)", font=font(12), fill=AMBER)
    d.rectangle([500, lg_y, 520, lg_y+14], fill=CYAN, outline=CYAN)
    d.text((528, lg_y), "selected", font=font(12), fill=CYAN)
    d.text((680, lg_y), "unknown — never read (192)", font=font(12), fill=GREY)
    d.text((1100, lg_y), "BUS: 500 kbps", font=font(12), fill=CYAN_DIM)

    # ── Right side: SELECTED PARAMETER DETAILS panel ──────────────
    panel(d, 1360, 460, 520, 530, f"SELECTED — *608_21_B5")
    sx = 1380
    sy = 500
    f = font(14)
    fb = font(14, bold=True)
    fH = font(16, bold=True)
    fXL = font(20, bold=True)
    rows_det = [
        ("INDEX",     "0xB5",                CYAN),
        ("GROUP",     "B0..B9 — injector qty correction", WHITE),
        ("WORK PT",   "WP5 of 10",           AMBER),
        ("CMD",       "*608_21_B5\\r",       CYAN),
        ("RESPONSE",  "*97 181 1 211 1 200", GREEN),
        ("LENGTH",    "5 bytes",             WHITE),
        ("FORMAT",    "uint16_be x2",        WHITE),
    ]
    for k, v, col in rows_det:
        d.text((sx, sy), k, font=f, fill=GREY)
        d.text((sx + 100, sy), v, font=fb, fill=col)
        sy += 22

    sy += 10
    d.line([sx, sy, sx + 480, sy], fill=BORDER, width=1); sy += 12
    d.text((sx, sy), "DECODED VALUES", font=fH, fill=CYAN); sy += 26

    # Two big numeric readouts
    d.text((sx, sy), "current", font=f, fill=GREY)
    d.text((sx + 250, sy), "target", font=f, fill=GREY); sy += 18
    d.text((sx, sy), "467", font=fXL, fill=CYAN)
    d.text((sx + 250, sy), "456", font=fXL, fill=GREEN); sy += 28
    d.text((sx, sy), "0x01D3", font=font(12), fill=CYAN_DIM)
    d.text((sx + 250, sy), "0x01C8", font=font(12), fill=GREEN_DIM); sy += 22
    d.line([sx, sy, sx + 480, sy], fill=BORDER, width=1); sy += 12

    d.text((sx, sy), "DELTA", font=f, fill=GREY)
    d.text((sx + 100, sy), "-11", font=fH, fill=AMBER)
    d.text((sx + 180, sy), "(-2.4 %)", font=f, fill=AMBER); sy += 22
    d.text((sx, sy), "STATUS", font=f, fill=GREY)
    d.text((sx + 100, sy), "WITHIN TOLERANCE ±20", font=fb, fill=GREEN); sy += 22
    d.text((sx, sy), "FRESHNESS", font=f, fill=GREY)
    d.text((sx + 110, sy), "live (1.2s ago)", font=fb, fill=GREEN); sy += 22
    d.text((sx, sy), "READS", font=f, fill=GREY)
    d.text((sx + 100, sy), "247 (in this session)", font=fb, fill=WHITE); sy += 22

    sy += 8
    d.line([sx, sy, sx + 480, sy], fill=BORDER, width=1); sy += 12
    d.text((sx, sy), "NOTES", font=f, fill=GREY); sy += 18
    d.text((sx, sy), "Pattern matches 4N13 / Bosch CR", font=fb, fill=WHITE); sy += 18
    d.text((sx, sy), "diesel injector calibration. Read", font=fb, fill=WHITE); sy += 18
    d.text((sx, sy), "during real centering procedure.", font=fb, fill=WHITE)

    footer_bar(d, "[F2] Connect  [F5] Refresh map  [F8] Live  [Enter] Inspect param  [Tab] Switch ECU  [/] Filter  [F10] Quit")
    img.save(os.path.join(DIR, "mockup_01_dashboard.png"))
    print("→ mockup_01_dashboard.png")

# ─────────────────────────────────────────────────────────────────
# Mockup 02: Injector calibration screen
# ─────────────────────────────────────────────────────────────────
def mockup_02_injectors():
    img, d = new_canvas()
    grid_bg(d)
    header_bar(d)

    f_big = font(32, bold=True)
    d.text((40, 56), "INJECTOR CALIBRATION", font=f_big, fill=AMBER)
    d.text((40, 96), "4N13 common-rail diesel · params B0..B9 (10 work points × 2 values)", font=font(14), fill=GREEN_DIM)
    d.text((40, 116), "raw decoded as 4 bytes / index = two 16-bit big-endian uint16 (current vs target)", font=font(12), fill=GREY)

    # Bar chart panel — left big
    panel(d, 40, 150, 1200, 720, "QUANTITY CORRECTION CHART")
    values = [
        (491, 482), (479, 469), (474, 474), (465, 462), (464, 458),
        (467, 456), (458, 459), (468, 462), (440, 452), (457, 450),
    ]
    chart_x, chart_y = 110, 220
    chart_w, chart_h = 1100, 580
    bar_w = 96
    gap = (chart_w - 10*bar_w) // 11
    vmin, vmax = 420, 510
    # axis lines
    d.line([chart_x, chart_y, chart_x, chart_y+chart_h], fill=CYAN_DIM, width=1)
    d.line([chart_x, chart_y+chart_h, chart_x+chart_w, chart_y+chart_h], fill=CYAN_DIM, width=1)
    # y ticks
    for v in range(vmin, vmax+1, 10):
        y = chart_y + chart_h - int((v-vmin)/(vmax-vmin) * chart_h)
        d.line([chart_x-6, y, chart_x, y], fill=CYAN_DIM)
        d.text((chart_x-50, y-8), f"{v}", font=font(12), fill=GREY)
        # horizontal grid line
        d.line([chart_x+1, y, chart_x+chart_w, y], fill=(20, 28, 40), width=1)
    # bars
    for i, (v1, v2) in enumerate(values):
        bx = chart_x + gap + i*(bar_w+gap)
        h1 = int((v1-vmin)/(vmax-vmin) * chart_h)
        h2 = int((v2-vmin)/(vmax-vmin) * chart_h)
        bar1_w = bar_w // 2 - 3
        # bar 1 (cyan = current)
        d.rectangle([bx, chart_y+chart_h-h1, bx+bar1_w, chart_y+chart_h], fill=(0, 60, 80), outline=CYAN, width=2)
        d.text((bx+4, chart_y+chart_h-h1-18), f"{v1}", font=font(14, bold=True), fill=CYAN)
        # bar 2 (green = target)
        d.rectangle([bx+bar1_w+6, chart_y+chart_h-h2, bx+bar_w, chart_y+chart_h], fill=(20, 60, 30), outline=GREEN, width=2)
        d.text((bx+bar1_w+10, chart_y+chart_h-h2-18), f"{v2}", font=font(14, bold=True), fill=GREEN)
        # label
        d.text((bx+bar_w//2-14, chart_y+chart_h+10), f"B{i:X}", font=font(16, bold=True), fill=AMBER)
        d.text((bx+bar_w//2-20, chart_y+chart_h+30), f"WP{i}", font=font(12), fill=GREY)
        # delta
        delta = v2 - v1
        col = GREEN if abs(delta) <= 10 else AMBER
        sign = "+" if delta > 0 else ""
        d.text((bx+bar_w//2-16, chart_y+chart_h+50), f"{sign}{delta}", font=font(12, bold=True), fill=col)

    # Legend
    d.rectangle([960, 232, 980, 248], fill=(0, 60, 80), outline=CYAN, width=2)
    d.text((988, 233), "current (val1)", font=font(14), fill=CYAN)
    d.rectangle([1100, 232, 1120, 248], fill=(20, 60, 30), outline=GREEN, width=2)
    d.text((1128, 233), "target (val2)", font=font(14), fill=GREEN)

    # Right side — data table panel
    panel(d, 1260, 150, 620, 720, "RAW DATA TABLE")
    f_t = font(14)
    fb_t = font(14, bold=True)
    # header
    th = ["WP", "raw1", "raw2", "v1", "v2", "Δ", "%"]
    cw = [60, 90, 90, 80, 80, 80, 80]
    tx = 1290
    ty = 200
    cx = tx
    for i, h in enumerate(th):
        d.text((cx + 4, ty), h, font=fb_t, fill=CYAN)
        cx += cw[i]
    d.line([tx, ty+22, tx+sum(cw), ty+22], fill=CYAN_DIM, width=1)
    ty += 30

    raw_pairs = [
        ("B0", "01EB", "01E2", 491, 482, -9,  -1.8),
        ("B1", "01DF", "01D5", 479, 469, -10, -2.1),
        ("B2", "01DA", "01DA", 474, 474,  0,   0.0),
        ("B3", "01D1", "01CE", 465, 462, -3,  -0.6),
        ("B4", "01D0", "01CA", 464, 458, -6,  -1.3),
        ("B5", "01D3", "01C8", 467, 456, -11, -2.4),
        ("B6", "01CA", "01CB", 458, 459, +1,  +0.2),
        ("B7", "01D4", "01CE", 468, 462, -6,  -1.3),
        ("B8", "01B8", "01C4", 440, 452, +12, +2.7),
        ("B9", "01C9", "01C2", 457, 450, -7,  -1.5),
    ]
    for row in raw_pairs:
        cx = tx
        cells = [str(row[0]), row[1], row[2], str(row[3]), str(row[4]),
                 f"{row[5]:+d}", f"{row[6]:+.1f}"]
        cols = [AMBER, GREY, GREY, CYAN, GREEN, GREEN if abs(row[5])<=10 else AMBER, GREEN if abs(row[5])<=10 else AMBER]
        for i, c in enumerate(cells):
            d.text((cx + 4, ty), c, font=fb_t if i==0 else f_t, fill=cols[i])
            cx += cw[i]
        ty += 28

    # Stats below table
    ty += 16
    d.line([tx, ty-6, tx+sum(cw), ty-6], fill=CYAN_DIM, width=1)
    d.text((tx, ty), "STATISTICS", font=font(14, bold=True), fill=CYAN); ty += 24
    stats = [
        ("Δ avg",      "-3.9 units",  GREEN),
        ("Δ avg %",    "-0.81 %",     GREEN),
        ("max  Δ",     "+12 (B8)",    AMBER),
        ("min  Δ",     "-11 (B5)",    AMBER),
        ("std dev",    " 6.8",        CYAN),
        ("tolerance",  "±20 units",   GREEN),
        ("verdict",    "WITHIN SPEC", GREEN),
    ]
    for k, v, col in stats:
        d.text((tx, ty), k.upper(), font=f_t, fill=GREY)
        d.text((tx + 200, ty), v, font=fb_t, fill=col)
        ty += 22

    ty += 16
    d.text((tx, ty), "STATUS", font=font(14, bold=True), fill=CYAN); ty += 24
    d.rectangle([tx, ty, tx+sum(cw), ty+44], fill=(20, 60, 30), outline=GREEN, width=2)
    d.text((tx + 30, ty + 13), "✓ READY FOR WRITE-BACK", font=font(18, bold=True), fill=GREEN)

    # Bottom info bar
    d.text((40, 920), "captured live during injector centering — Mitsubishi ASX 1.8 DiD · 2026-04-06 17:21",
           font=font(14), fill=GREY)
    d.text((40, 950), "session 113489426..114669042 µs · 7,901 *608_21_<XX> read commands · 5.6 sec window",
           font=font(12), fill=CYAN_DIM)

    footer_bar(d, "[F3] Read  [F4] Write  [F5] Reset adapt  [F6] Per-cyl breakdown  [F9] Export CSV  [Esc] Back")
    img.save(os.path.join(DIR, "mockup_02_injectors.png"))
    print("→ mockup_02_injectors.png")

# ─────────────────────────────────────────────────────────────────
# Mockup 03: Live protocol monitor / sniff view
# ─────────────────────────────────────────────────────────────────
def mockup_03_sniff():
    img, d = new_canvas()
    grid_bg(d)
    header_bar(d)

    f_big = font(32, bold=True)
    d.text((40, 56), "PROTOCOL MONITOR", font=f_big, fill=GREEN)
    d.text((40, 96), "live USB bulk OUT/IN — Autocom CDP+ ASCII protocol", font=font(14), fill=GREEN_DIM)
    d.text((40, 116), "session 76030.179s · all packets buffered to ring · 65535 byte snaplen", font=font(12), fill=GREY)

    # Stats bar — full width
    panel(d, 40, 150, 1840, 100)
    stats = [
        ("PACKETS",  "118,971", CYAN),
        ("OUT",      " 15,894", GREEN),
        ("IN",       "103,029", AMBER),
        ("UNIQ CMDS",       "9", CYAN),
        ("UNIQ PARAMS",    "56", AMBER),
        ("RATE",    " 142 B/s", GREEN),
        ("UPTIME",  "00:42:13", WHITE),
    ]
    for i, (k, v, col) in enumerate(stats):
        x = 70 + i*260
        d.text((x, 172), k, font=font(14), fill=GREY)
        d.text((x, 196), v, font=font(32, bold=True), fill=col)

    # Live feed — left big
    panel(d, 40, 270, 1180, 720, "LIVE FEED — Bulk OUT/IN ascii payload")
    feed = [
        ("76030.179", "→", "*203\\r",                                       CYAN),
        ("76034.735", "←", "*0 1362\\r",                                    GREEN),
        ("76051.758", "→", "*20A\\r",                                       CYAN),
        ("76056.192", "←", "*CDP+\\r",                                      GREEN),
        ("76079.908", "→", "*200\\r",                                       CYAN),
        ("76084.314", "←", "*100251\\r",                                    GREEN),
        ("76096.255", "→", "*201\\r",                                       CYAN),
        ("76101.058", "←", "*1622\\r",                                      GREEN),
        ("76126.677", "→", "*60b\\r",                                       CYAN),
        ("76131.215", "←", "*121\\r",                                       GREEN),
        ("76140.943", "→", "*60bc\\r",                                      CYAN),
        ("76145.444", "←", "*121\\r",                                       GREEN),
        ("76254.616", "→", "*668_0_500_7E0_7E8_000_01C_02_3E…",             AMBER),
        ("76714.463", "←", "*80 146 \\r",                                   GREEN),
        ("76725.585", "→", "*608_10_81\\r",                                 CYAN),
        ("76747.061", "←", "*80 129 \\r",                                   GREEN),
        ("78717.941", "→", "*606B001_7DF_02_3E_02_00_00_00_0…",             AMBER),
        ("78727.147", "←", "*1\\r",                                         GREEN),
        ("80969.629", "→", "*608_18_00_FF_00\\r",                           CYAN),
        ("80989.999", "←", "*88 0 \\r",                                     GREEN),
        ("113489.426","→", "*203\\r",                                       CYAN),
        ("113494.708","←", "*1 1366\\r",                                    GREEN),
        ("113615.567","→", "*608_21_4E\\r",                                 CYAN),
        ("113637.513","←", "*97 78 144 176 0 1 0 \\r",                      GREEN),
        ("113701.419","→", "*608_21_04\\r",                                 CYAN),
        ("113722.052","←", "*97 4 213 0 51 25 38 \\r",                      GREEN),
        ("113783.856","→", "*608_21_08\\r",                                 CYAN),
        ("113804.052","←", "*97 8 0 0 208 0 127 \\r",                       GREEN),
        ("114221.724","→", "*608_21_46\\r",                                 CYAN),
        ("114242.052","←", "*97 70 47 54 82 117 107 \\r  /6Ruk",            GREEN),
        ("114329.862","→", "*608_21_59\\r",                                 CYAN),
        ("114350.060","←", "*97 89 0 49 0 \\r",                             GREEN),
    ]
    f_mono = font(14)
    fb_mono = font(14, bold=True)
    for i, (ts, ar, payload, col) in enumerate(feed):
        y = 304 + i*21
        d.text((60, y), ts, font=f_mono, fill=GREY)
        d.text((180, y), ar, font=fb_mono, fill=col)
        d.text((204, y), payload, font=f_mono, fill=col)

    # Decoder panel — right
    panel(d, 1240, 270, 640, 720, "DECODER — *606")
    f = font(14)
    fb = font(14, bold=True)
    fH = font(16, bold=True)
    sx = 1260
    sy = 310
    d.text((sx, sy), "SELECTED:", font=f, fill=GREY); sy += 22
    d.text((sx, sy), "*606B001_7DF_02_3E_02_00_00_00_0…", font=fb, fill=AMBER); sy += 30

    fields = [
        ("CMD",       "*606", CYAN),
        ("TYPE",      "periodic CAN message slot", WHITE),
        ("SLOT",      "B001 (internal)", CYAN),
        ("CAN ID",    "0x7DF (OBD-II broadcast)", GREEN),
        ("DLC",       "8 bytes", WHITE),
    ]
    for k, v, col in fields:
        d.text((sx, sy), k, font=f, fill=GREY)
        d.text((sx + 130, sy), v, font=fb, fill=col)
        sy += 22

    sy += 8
    d.text((sx, sy), "PAYLOAD:", font=f, fill=GREY); sy += 22
    d.text((sx, sy), "02 3E 02 00 00 00 00 00", font=fH, fill=WHITE); sy += 28
    d.text((sx, sy), "byte 0: 02       ISO-TP SF len=2", font=f_mono, fill=GREEN_DIM); sy += 18
    d.text((sx, sy), "byte 1: 3E       UDS service ID", font=f_mono, fill=GREEN_DIM); sy += 18
    d.text((sx, sy), "                 = TesterPresent", font=fb, fill=AMBER); sy += 18
    d.text((sx, sy), "byte 2: 02       sub-function", font=f_mono, fill=GREEN_DIM); sy += 18
    d.text((sx, sy), "                 = suppressPosRsp", font=fb, fill=AMBER); sy += 18
    d.text((sx, sy), "bytes 3-7: 00x5  CAN frame padding", font=f_mono, fill=GREEN_DIM); sy += 22

    d.line([sx, sy, sx + 600, sy], fill=BORDER, width=1); sy += 12
    d.text((sx, sy), "PURPOSE:", font=f, fill=GREY); sy += 20
    d.text((sx, sy), "Keepalive ping. Prevents the ECU", font=fb, fill=WHITE); sy += 18
    d.text((sx, sy), "diagnostic session from timing out", font=fb, fill=WHITE); sy += 18
    d.text((sx, sy), "(~2s default). Once *606 sets up the", font=fb, fill=WHITE); sy += 18
    d.text((sx, sy), "slot, the dongle sends it autonomously", font=fb, fill=WHITE); sy += 18
    d.text((sx, sy), "— host doesn't have to repeat.", font=fb, fill=WHITE); sy += 24

    d.line([sx, sy, sx + 600, sy], fill=BORDER, width=1); sy += 12
    d.text((sx, sy), "⚠  TRUNCATED IN CAPTURE", font=fb, fill=AMBER); sy += 22
    d.text((sx, sy), "37 byte cmd captured as 32 (usbmon", font=f_mono, fill=AMBER); sy += 18
    d.text((sx, sy), "1u text format DATA_MAX=32 limit).", font=f_mono, fill=AMBER); sy += 18
    d.text((sx, sy), "Missing: last byte + _NN checksum + \\r", font=f_mono, fill=AMBER); sy += 24

    d.text((sx, sy), "[F6] Add to known cmds  [F7] Replay frame", font=f, fill=CYAN_DIM); sy += 18
    d.text((sx, sy), "[F8] Mark unsafe        [F9] Brute checksum", font=f, fill=CYAN_DIM)

    footer_bar(d, "[Space] Pause  [/] Filter  [s] Save pcap  [d] Decode  [r] Replay  [Esc] Back")
    img.save(os.path.join(DIR, "mockup_03_sniff.png"))
    print("→ mockup_03_sniff.png")

# ─────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    mockup_01_dashboard()
    mockup_02_injectors()
    mockup_03_sniff()
    print(f"\nGenerated in {DIR}")
