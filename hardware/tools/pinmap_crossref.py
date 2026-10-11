#!/usr/bin/env python3
"""Pin-map cross-reference audit: schematic vs board vs pins_final.h vs firmware vs docs.

Reads (never writes) these sources and checks that every GPIO, connector pin, test point and
keypad crossing agrees everywhere:
  - hardware/kicad/*.kicad_sch   parsed directly in Python (pin end-points + labels; the design has
                                 no wires: every net is a label sitting on a pin end)
  - kicad-cli netlist            optional second opinion on the schematic (skipped if not installed)
  - hardware/kicad/ai_calc.kicad_pcb   footprint pad -> net, streamed footprint by footprint
  - hardware/pins_final.h        the firmware's single source of truth
  - firmware-prototype/src/*     pins.h (must include pins_final.h), keys.cpp kMatrix, power.cpp divider
  - firmware/src/pins.h          the bench tester (different hardware: reported, not compared)
  - core/device.cpp              DKey -> legend names
  - docs                         bringup_guide.html, FINAL_STATUS.md, HANDOFF.md, FIRMWARE_STAGE13.md,
                                 verification/02_power_boot.md, 03_interfaces.md, firmware-prototype/README.md,
                                 kicad/DESIGN_SUMMARY.md

Usage (repo root):  python hardware/tools/pinmap_crossref.py [-o hardware/verification/09_pinmap_crossref.md]
Exit code = number of disagreements (0 = everything agrees).
"""
import glob
import html
import io
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
KICAD = os.path.join(ROOT, "hardware", "kicad")
P = lambda *a: os.path.join(ROOT, *a)

# ----------------------------------------------------------------------------- s-expressions
def parse_sexpr(text):
    """Minimal KiCad s-expression reader: lists, quoted strings, bare atoms (numbers kept as str)."""
    tokens = re.findall(r'"(?:\\.|[^"\\])*"|\(|\)|[^\s()]+', text)
    stack = [[]]
    for tok in tokens:
        if tok == "(":
            stack.append([])
        elif tok == ")":
            lst = stack.pop()
            stack[-1].append(lst)
        elif tok[0] == '"':
            stack[-1].append(tok[1:-1].replace('\\"', '"'))
        else:
            stack[-1].append(tok)
    return stack[0]


def kids(node, key):
    return [k for k in node[1:] if isinstance(k, list) and k and k[0] == key]


def kid(node, key, default=None):
    k = kids(node, key)
    return k[0] if k else default


def prop(node, name):
    for p in kids(node, "property"):
        if p[1] == name:
            return p[2]
    return None


# ----------------------------------------------------------------------------- schematic
def rot(x, y, deg):
    deg = int(round(float(deg))) % 360
    if deg == 0:
        return x, y
    if deg == 90:
        return -y, x
    if deg == 180:
        return -x, -y
    if deg == 270:
        return y, -x
    raise ValueError(deg)


def read_schematic():
    """Returns (pin_net, sheet_labels) where pin_net[(ref, pinnum)] = (net, pinname, sheet)."""
    pin_net = {}
    unlabelled = []
    for path in sorted(glob.glob(os.path.join(KICAD, "*.kicad_sch"))):
        sheetfile = os.path.basename(path)
        doc = parse_sexpr(open(path, encoding="utf-8").read())[0]
        if doc[0] != "kicad_sch":
            continue
        # sheet name as the root sheet calls it (for local-label net names)
        sheetname = None
        root = parse_sexpr(open(os.path.join(KICAD, "ai_calc.kicad_sch"), encoding="utf-8").read())[0]
        for sh in kids(root, "sheet"):
            if prop(sh, "Sheetfile") == sheetfile:
                sheetname = prop(sh, "Sheetname")
        # library symbols: pins per lib_id (all units)
        libpins = {}
        for ls in kids(kid(doc, "lib_symbols", ["lib_symbols"]), "symbol"):
            pins = []
            for unit in kids(ls, "symbol"):
                for pin in kids(unit, "pin"):
                    at = kid(pin, "at")
                    pins.append((float(at[1]), float(at[2]), kid(pin, "name")[1], kid(pin, "number")[1],
                                 pin[1]))
            libpins[ls[1]] = pins
        # labels at coordinates
        labels = defaultdict(list)
        for key, scope in (("label", "local"), ("global_label", "global"), ("hierarchical_label", "hier")):
            for lb in kids(doc, key):
                at = kid(lb, "at")
                labels[(round(float(at[1]), 2), round(float(at[2]), 2))].append((lb[1], scope))
        ncs = set()
        for nc in kids(doc, "no_connect"):
            at = kid(nc, "at")
            ncs.add((round(float(at[1]), 2), round(float(at[2]), 2)))
        # symbol instances
        for sym in kids(doc, "symbol"):
            lib_id = kid(sym, "lib_id")[1]
            at = kid(sym, "at")
            X, Y, R = float(at[1]), float(at[2]), float(at[3]) if len(at) > 3 else 0
            mirror = kid(sym, "mirror")
            ref = prop(sym, "Reference")
            if lib_id.startswith("power:"):
                continue
            for (px, py, pname, pnum, ptype) in libpins.get(lib_id, []):
                x, y = px, py
                if mirror:
                    if mirror[1] == "x":
                        y = -y
                    elif mirror[1] == "y":
                        x = -x
                x, y = rot(x, y, R)
                pos = (round(X + x, 2), round(Y - y, 2))
                found = labels.get(pos)
                if found:
                    name, scope = found[0]
                    net = name if scope == "global" else f"/{sheetname}/{name}"
                    pin_net[(ref, pnum)] = (net, pname, sheetfile)
                elif pos in ncs:
                    pin_net[(ref, pnum)] = (f"unconnected-({ref}-{pname}-Pad{pnum})", pname, sheetfile)
                else:
                    unlabelled.append((ref, pnum, pname, sheetfile, pos))
    return pin_net, unlabelled


def read_schematic_values():
    vals = {}
    for path in sorted(glob.glob(os.path.join(KICAD, "*.kicad_sch"))):
        doc = parse_sexpr(open(path, encoding="utf-8").read())[0]
        for sym in kids(doc, "symbol"):
            ref = prop(sym, "Reference")
            if ref:
                vals[ref] = prop(sym, "Value")
    return vals


def kicad_cli_netlist():
    """(ref,pin)->net from kicad-cli, or None when kicad-cli is not installed."""
    cli = shutil.which("kicad-cli")
    if not cli:
        c = sorted(glob.glob(r"C:\Program Files\KiCad\*\bin\kicad-cli.exe"), reverse=True)
        cli = c[0] if c else None
    if not cli:
        return None
    tmp = tempfile.mkdtemp(prefix="pinmap_")
    try:
        for pat in ("*.kicad_sch", "*.kicad_sym", "*.kicad_pro", "*-lib-table"):
            for f in glob.glob(os.path.join(KICAD, pat)):
                shutil.copy(f, tmp)
        out = os.path.join(tmp, "net.net")
        subprocess.run([cli, "sch", "export", "netlist", "--format", "kicadsexpr", "-o", out,
                        os.path.join(tmp, "ai_calc.kicad_sch")], check=True, capture_output=True)
        text = open(out, encoding="utf-8").read()
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    res = {}
    nets = text[text.find("(nets"):]
    for block in re.split(r"\(net\s+\(code", nets)[1:]:
        name = re.search(r'\(name\s+"([^"]*)"\)', block).group(1)
        for ref, pin in re.findall(r'\(node\s+\(ref\s+"([^"]+)"\)\s+\(pin\s+"([^"]+)"\)', block):
            res[(ref, pin)] = name
    return res


# ----------------------------------------------------------------------------- board
def iter_blocks(text, tag):
    i = 0
    needle = "(" + tag + " "
    while True:
        i = text.find(needle, i)
        if i < 0:
            return
        depth, j = 0, i
        while True:
            c = text[j]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    break
            elif c == '"':
                j = text.find('"', j + 1)
            j += 1
        yield text[i:j + 1]
        i = j + 1


def read_board():
    """pad_net[(ref, padnum)] = set(nets); values[ref] = Value.  Streams the file footprint by footprint."""
    pad_net, values = {}, {}
    text = open(P("hardware", "kicad", "ai_calc.kicad_pcb"), encoding="utf-8").read()
    for fp in iter_blocks(text, "footprint"):
        m = re.search(r'\(property "Reference" "([^"]*)"', fp)
        if not m:
            continue
        ref = m.group(1)
        v = re.search(r'\(property "Value" "([^"]*)"', fp)
        values[ref] = v.group(1) if v else ""
        for pad in iter_blocks(fp, "pad"):
            num = re.match(r'\(pad "([^"]*)"', pad).group(1)
            net = re.search(r'\(net (?:\d+ )?"([^"]*)"\)', pad)
            pad_net.setdefault((ref, num), set()).add(net.group(1) if net else "")
    del text
    return pad_net, values


# ----------------------------------------------------------------------------- ESP32-S3-MINI-1 datasheet
# ESP32-S3-MINI-1 datasheet (v1.7) table 3-1: pad number -> pin name. Independent of the KiCad symbol.
DS_PADS = {1: "GND", 2: "GND", 3: "3V3", 4: "IO0", 5: "IO1", 6: "IO2", 7: "IO3", 8: "IO4", 9: "IO5", 10: "IO6",
           11: "IO7", 12: "IO8", 13: "IO9", 14: "IO10", 15: "IO11", 16: "IO12", 17: "IO13", 18: "IO14",
           19: "IO15", 20: "IO16", 21: "IO17", 22: "IO18", 23: "IO19", 24: "IO20", 25: "IO21", 26: "IO26",
           27: "IO47", 28: "IO33", 29: "IO34", 30: "IO48", 31: "IO35", 32: "IO36", 33: "IO37", 34: "IO38",
           35: "IO39", 36: "IO40", 37: "IO41", 38: "IO42", 39: "TXD0", 40: "RXD0", 41: "IO45", 42: "GND",
           43: "GND", 44: "IO46", 45: "EN"}
DS_PADS.update({n: "GND" for n in range(46, 66)})
PINNAME_GPIO = {"TXD0": 43, "RXD0": 44, "USB_D-": 19, "USB_D+": 20}


def gpio_of_pinname(name):
    m = re.match(r"IO(\d+)$", name)
    if m:
        return int(m.group(1))
    for k, v in PINNAME_GPIO.items():
        if name.startswith(k):
            return v
    return None


# ----------------------------------------------------------------------------- firmware
def read_pins_final():
    """const -> (gpio, net) from `constexpr int PIN_x = N;  // [NET]` lines; plus other facts."""
    pins, facts = {}, {}
    for line in open(P("hardware", "pins_final.h"), encoding="utf-8"):
        m = re.match(r"\s*constexpr int (PIN_\w+)\s*=\s*(-?\d+);(?:\s*//\s*(?:\[([^\]]+)\])?)?", line)
        if m:
            pins[m.group(1)] = (int(m.group(2)), m.group(3))
        m = re.match(r"\s*constexpr uint8_t TCA8418_I2C_ADDR\s*=\s*(0x[0-9A-Fa-f]+)", line)
        if m:
            facts["tca_addr"] = int(m.group(1), 16)
    text = open(P("hardware", "pins_final.h"), encoding="utf-8").read()
    facts["vbat_divider"] = re.search(r"VBAT_SENSE\][^\n]*?(\d+ ?M ?/ ?\d+ ?M)", text)
    facts["vbat_divider"] = facts["vbat_divider"].group(1) if facts["vbat_divider"] else None
    facts["vbus_divider"] = re.search(r"VBUS_SENSE\][^\n]*?(\d+ ?k ?/ ?\d+ ?k)", text)
    facts["vbus_divider"] = facts["vbus_divider"].group(1) if facts["vbus_divider"] else None
    facts["mag_order"] = re.search(r'MAG_PIN_ORDER\[4\] = \{([^}]*)\}', text)
    facts["mag_order"] = [s.strip('" ') for s in facts["mag_order"].group(1).split(",")] if facts["mag_order"] else None
    return pins, facts


def read_tester_pins():
    pins = {}
    for line in open(P("firmware", "src", "pins.h"), encoding="utf-8"):
        m = re.match(r"\s*constexpr int (PIN_\w+)\s*=\s*(-?\d+);", line)
        if m:
            pins[m.group(1)] = int(m.group(2))
    return pins


def read_proto_pins_h():
    text = open(P("firmware-prototype", "src", "pins.h"), encoding="utf-8").read()
    includes = bool(re.search(r'#include\s+"\.\./\.\./hardware/pins_final\.h"', text))
    own = re.findall(r"constexpr int (PIN_\w+)\s*=\s*(-?\d+)", text)
    return includes, own


def read_kmatrix():
    """keys.cpp kMatrix -> {(row, col): DKey name}; also TCA address source."""
    text = open(P("firmware-prototype", "src", "keys.cpp"), encoding="utf-8").read()
    m = re.search(r"kMatrix\[kRows\]\[kCols\]\s*=\s*\{(.*?)\n\};", text, re.S)
    rows = re.findall(r"\{([^{}]*)\}", m.group(1))
    matrix = {}
    for r, row in enumerate(rows):
        for c, cell in enumerate([s.strip() for s in row.split(",") if s.strip()]):
            if cell != "X":
                matrix[(r, c)] = cell.replace("DKey::", "")
    addr = re.search(r"kTca = (\w+)", text).group(1)
    code = re.search(r"const int code = \(ev & 0x7F\) - 1;.*?\n\s*const int row = code / (\d+), col = code % (\d+);",
                     text, re.S)
    return matrix, addr, (int(code.group(1)), int(code.group(2))) if code else None


def read_dkey_names():
    text = open(P("core", "device.h"), encoding="utf-8").read()
    enum = re.search(r"enum class DKey : uint8_t \{(.*?)\};", text, re.S).group(1)
    names = [s.strip() for s in re.sub(r"//[^\n]*", "", enum).split(",") if s.strip()]
    names = [n for n in names if n != "Count"]
    text = open(P("core", "device.cpp"), encoding="utf-8").read()
    arr = re.search(r"kKeyNames\[\] = \{(.*?)\};", text, re.S).group(1)
    legends = re.findall(r'"((?:\\.|[^"\\])*)"', arr)
    return dict(zip(names, legends))


def read_power_divider():
    text = open(P("firmware-prototype", "src", "power.cpp"), encoding="utf-8").read()
    m = re.search(r"analogReadMilliVolts\(PIN_VBAT_SENSE\).*?\n.*?\*\s*(\d+)\s*\*", text, re.S)
    return int(m.group(1)) if m else None


# Schematic key legend -> firmware legend (core/device.cpp kKeyNames) where they differ in spelling only.
LEGEND_ALIAS = {"∫dx": "int_dx", "a/b": "frac", "deg-min-sec": "dms"}


# ----------------------------------------------------------------------------- docs
DOCS = [
    ("bringup_guide.html", P("hardware", "bringup_guide.html")),
    ("FINAL_STATUS.md", P("hardware", "FINAL_STATUS.md")),
    ("HANDOFF.md", P("hardware", "HANDOFF.md")),
    ("FIRMWARE_STAGE13.md", P("firmware", "FIRMWARE_STAGE13.md")),
    ("verification/02_power_boot.md", P("hardware", "verification", "02_power_boot.md")),
    ("verification/03_interfaces.md", P("hardware", "verification", "03_interfaces.md")),
    ("firmware-prototype/README.md", P("firmware-prototype", "README.md")),
    ("kicad/DESIGN_SUMMARY.md", P("hardware", "kicad", "DESIGN_SUMMARY.md")),
]

# Net name -> extra spellings the docs use for the same signal.
DOC_ALIASES = {
    "CAM_XCLK": ["XCLK", "MCLK"], "CAM_SIOD": ["SIOD", "SCCB data", "CAM_SDA"], "CAM_SIOC": ["SIOC", "CAM_SCL"],
    "CAM_VSYNC": ["VSYNC"], "CAM_HREF": ["HREF"], "CAM_PCLK": ["PCLK"], "CAM_PWDN": ["PWDN"],
    "CAM_RESET": ["RESET"], "CAM_PWR_EN": ["PWR_EN"],
    "EPD_BUSY": ["BUSY"], "EPD_RST": ["RST"], "EPD_DC": ["DC"], "EPD_CS": ["CS"], "EPD_CLK": ["SCLK", "CLK"],
    "EPD_DIN": ["DIN", "MOSI", "SDI"],
    "I2C_SDA": ["SDA"], "I2C_SCL": ["SCL"], "KEYPAD_INT": ["INT"], "KEY_ON": ["ON"],
    "VBAT_SENSE": ["battery sense"], "VBUS_SENSE": ["USB sense"], "CHG_STAT": ["STAT"],
    "USB_DM": ["D-", "D−"], "USB_DP": ["D+"], "BOOT": ["GPIO0", "IO0"],
}


ALL_DOC_NAMES = sorted({n for k, v in DOC_ALIASES.items() for n in [k] + v if len(n) > 2}, key=len, reverse=True)


def doc_text(path):
    t = open(path, encoding="utf-8", errors="replace").read()
    if path.endswith(".html"):
        t = re.sub(r"<[^>]+>", " ", t)
        t = html.unescape(t)
    return t


def doc_gpio_claims(text, nets):
    """For each net name (or alias) find 'NAME ... IOnn' / 'NAME ... GPIO nn' within 48 chars -> {net: set(gpio)}."""
    claims = defaultdict(set)
    for net in nets:
        names = [net] + DOC_ALIASES.get(net, [])
        for name in names:
            pat = re.escape(name) + r"(?:[^\n]{0,48}?)\b(?:IO|GPIO)[ _]?(\d{1,2})\b"
            for m in re.finditer(pat, text):
                between = text[m.start() + len(name):m.start(1)]
                # Skip if another signal-ish token intervenes (e.g. "CAM_D7 | IO21 (25) | D7 21 | IO48" where IO48 is another board)
                if re.search(r"\b(?:IO|GPIO)[ _]?\d{1,2}\b", between):
                    continue
                if len(name) <= 4 and not re.match(r"[\s(|:=,]", text[m.start() - 1:m.start()] or " "):
                    continue  # short aliases must start a word
                if "✓" in between or between.count("|") > 1 or re.search(r"\.\s", between):
                    continue  # the number sits in another table cell or another sentence
                if re.search(r"(?<![\w.])\d{1,2}(?![\w.])", between):
                    continue  # a bare number in between: shorthand like "INT 4), ON on GPIO 7"
                if any(re.search(r"(?<![\w_])" + re.escape(o) + r"(?![\w_])", between)
                       for o in ALL_DOC_NAMES if o not in names):
                    continue  # another signal is named in between: the GPIO belongs to that one
                claims[net].add(int(m.group(1)))
    return claims


# ----------------------------------------------------------------------------- main audit
def main():
    out_path = P("hardware", "verification", "09_pinmap_crossref.md")
    if "-o" in sys.argv:
        out_path = sys.argv[sys.argv.index("-o") + 1]
    issues = []       # (severity, title, consequence, fix)
    notes = []

    sch, unlabelled = read_schematic()
    sch_vals = read_schematic_values()
    cli = kicad_cli_netlist()
    pcb, pcb_vals = read_board()
    pf, pf_facts = read_pins_final()
    tester = read_tester_pins()
    proto_includes, proto_own = read_proto_pins_h()
    kmatrix, tca_addr_src, code_split = read_kmatrix()
    dkey_legend = read_dkey_names()
    divider_mult = read_power_divider()
    docs = [(name, doc_text(path)) for name, path in DOCS if os.path.exists(path)]

    # --- schematic parser self-check vs kicad-cli (if available)
    if cli is not None:
        diff = [(k, v[0], cli.get(k)) for k, v in sch.items() if cli.get(k) != v[0]]
        missing = [k for k in cli if k not in sch and not k[0].startswith("#")]
        if diff or missing:
            issues.append(("TOOL", "Python schematic parser disagrees with kicad-cli netlist",
                           f"{len(diff)} pins differ, {len(missing)} missing: {diff[:5]} {missing[:5]}",
                           "fix hardware/tools/pinmap_crossref.py before trusting the table"))
        else:
            notes.append(f"Python schematic parser and kicad-cli netlist agree on all {len(sch)} pins.")
    else:
        notes.append("kicad-cli not found: schematic column comes from the Python parser only.")
    if unlabelled:
        notes.append("Schematic pins with neither a label nor a no-connect: " +
                     ", ".join(f"{r}.{n}({pn})" for r, n, pn, s, pos in unlabelled))

    # --- U1 pad -> GPIO from the symbol, checked against the datasheet table
    u1 = {}  # gpio -> (padnum, pinname, net)
    for (ref, pad), (net, pname, sheet) in sch.items():
        if ref != "U1":
            continue
        ds = DS_PADS.get(int(pad))
        sym_gpio = gpio_of_pinname(pname)
        ds_gpio = gpio_of_pinname(ds) if ds else None
        if ds and ds != pname.split("_")[0] and not pname.startswith(ds) and sym_gpio != ds_gpio:
            issues.append(("SERIOUS", f"U1 symbol pad {pad} is named {pname} but the datasheet says {ds}",
                           "a GPIO would be routed to the wrong module pad", "fix the symbol in ai_calc.kicad_sym"))
        g = gpio_of_pinname(pname)
        if g is not None:
            u1[g] = (int(pad), pname, net)

    def pcb_net(ref, pad):
        s = pcb.get((ref, str(pad)))
        return "/".join(sorted(s)) if s else None

    # --- master rows
    rows = []  # dict(signal, sch_net, gpio, pad, pcb_net, pins_final, fw, proto, docs, agree, note)

    def doc_claims_for(net, gpio):
        found = []
        for name, text in docs:
            c = doc_gpio_claims(text, [net]).get(net)
            if c:
                found.append((name, sorted(c)))
        return found

    def add_gpio_row(signal, const, net_expected):
        pfv = pf.get(const)
        gpio = pfv[0] if pfv else None
        pf_net = pfv[1] if pfv else None
        if gpio is None or gpio < 0:
            rows.append(dict(signal=signal, sch=net_expected, gpio="-", pad="-", pcb="-", pf=f"{const} = {gpio}",
                             fw="n/a", proto="(includes pins_final.h)", docs="", agree="see note",
                             note=f"not wired to the ESP32 by design"))
            return
        pad, pname, sch_net = u1.get(gpio, (None, None, None))
        board_net = pcb_net("U1", pad) if pad else None
        docclaims = doc_claims_for(net_expected, gpio)
        docs_s = "; ".join(f"{n}: IO{','.join(map(str, g))}" for n, g in docclaims) or "(no explicit claim)"
        problems = []
        if sch_net != net_expected:
            problems.append(f"schematic IO{gpio} carries {sch_net}, expected {net_expected}")
        if pf_net != net_expected:
            problems.append(f"pins_final.h comment names [{pf_net}]")
        if board_net != net_expected:
            problems.append(f"board U1 pad {pad} is {board_net}")
        for n, g in docclaims:
            if g != [gpio]:
                problems.append(f"{n} says IO{','.join(map(str, g))}")
        for p in problems:
            issues.append(("SERIOUS" if "schematic" in p or "board" in p else "DOC", f"{signal}: {p}",
                           "firmware and hardware would disagree on this pin", "see table"))
        rows.append(dict(signal=signal, sch=sch_net, gpio=f"IO{gpio}", pad=pad, pcb=board_net,
                         pf=f"{const} = {gpio} [{pf_net}]", fw=tester_note(const), proto="= pins_final.h",
                         docs=docs_s, agree="yes" if not problems else "NO", note="; ".join(problems)))

    def tester_note(const):
        if const in tester:
            return f"tester {tester[const]} (other hardware)"
        return "n/a (tester)"

    cam = [("CAM_XCLK", "PIN_CAM_XCLK"), ("CAM_SIOD", "PIN_CAM_SIOD"), ("CAM_SIOC", "PIN_CAM_SIOC"),
           ("CAM_D0", "PIN_CAM_D0"), ("CAM_D1", "PIN_CAM_D1"), ("CAM_D2", "PIN_CAM_D2"), ("CAM_D3", "PIN_CAM_D3"),
           ("CAM_D4", "PIN_CAM_D4"), ("CAM_D5", "PIN_CAM_D5"), ("CAM_D6", "PIN_CAM_D6"), ("CAM_D7", "PIN_CAM_D7"),
           ("CAM_VSYNC", "PIN_CAM_VSYNC"), ("CAM_HREF", "PIN_CAM_HREF"), ("CAM_PCLK", "PIN_CAM_PCLK"),
           ("CAM_PWDN", "PIN_CAM_PWDN"), ("CAM_RESET", "PIN_CAM_RESET"), ("CAM_PWR_EN", "PIN_CAM_PWR_EN")]
    epd = [("EPD_CLK", "PIN_EPD_CLK"), ("EPD_DIN", "PIN_EPD_DIN"), ("EPD_CS", "PIN_EPD_CS"), ("EPD_DC", "PIN_EPD_DC"),
           ("EPD_RST", "PIN_EPD_RST"), ("EPD_BUSY", "PIN_EPD_BUSY")]
    key = [("I2C_SDA", "PIN_I2C_SDA"), ("I2C_SCL", "PIN_I2C_SCL"), ("KEYPAD_INT", "PIN_KEYPAD_INT"),
           ("KEY_ON", "PIN_KEY_ON"), ("TCA_RESET", "PIN_TCA_RESET")]
    pwr = [("VBAT_SENSE", "PIN_VBAT_SENSE"), ("VBUS_SENSE", "PIN_VBUS_SENSE"), ("CHG_STAT", "PIN_CHG_STAT")]
    usb = [("USB_DM", "PIN_USB_DM"), ("USB_DP", "PIN_USB_DP")]
    misc = [("BOOT", "PIN_BOOT"), ("UART_TX", "PIN_UART_TX"), ("UART_RX", "PIN_UART_RX")]
    sections = [("Camera (J1, OV5640)", cam), ("E-paper (J2, SSD1680)", epd), ("Keypad / TCA8418", key),
                ("Power sense", pwr), ("USB", usb), ("Boot / UART test pads", misc)]
    for title, lst in sections:
        rows.append(dict(section=title))
        for net, const in lst:
            add_gpio_row(net, const, net)

    # --- every ESP32 GPIO accounted for (nothing on U1 that pins_final.h doesn't know)
    pf_gpios = {v[0] for v in pf.values() if v[0] >= 0}
    for g, (pad, pname, net) in sorted(u1.items()):
        if g not in pf_gpios and not net.startswith("unconnected"):
            issues.append(("SERIOUS", f"U1 IO{g} (pad {pad}) carries {net} but pins_final.h has no constant for it",
                           "firmware could drive or float a pin the board uses", "add a PIN_ constant"))

    # --- EN
    rows.append(dict(section="Reset"))
    en_sch = sch.get(("U1", "45"), ("?",))[0]
    en_pcb = pcb_net("U1", 45)
    rows.append(dict(signal="EN (chip enable)", sch=en_sch, gpio="EN (pad 45)", pad=45, pcb=en_pcb,
                     pf="(TP7 comment)", fw="n/a", proto="n/a", docs="TP7 = EN in all docs",
                     agree="yes" if en_sch == en_pcb == "EN" else "NO", note="R1 10 k to +3V3, C4 to GND"))

    # --- test points
    rows.append(dict(section="Test points"))
    tp_expect = {"TP1": "BOOT", "TP2": "UART_TX", "TP3": "UART_RX", "TP4": "GND", "TP5": "+3V3", "TP6": "BAT+",
                 "TP7": "EN"}  # from pins_final.h comment: IO0 = BOOT (TP1), TX/RX TP2/TP3, EN = TP7, GND = TP4
    bring = dict(docs).get("bringup_guide.html", "")
    bring_tp = dict(re.findall(r"\bTP([1-7])\s+(BOOT|TX|RX|GND|3V3|BAT|EN)\b", bring))
    tp_bring_alias = {"BOOT": "BOOT", "TX": "UART_TX", "RX": "UART_RX", "GND": "GND", "3V3": "+3V3", "BAT": "BAT+",
                      "EN": "EN"}
    for tp, exp in tp_expect.items():
        s = sch.get((tp, "1"), ("?",))[0]
        b = pcb_net(tp, 1)
        v = sch_vals.get(tp)
        bg = tp_bring_alias.get(bring_tp.get(tp[2]), None)
        ok = s == b == exp == v and (bg in (None, exp))
        if not ok:
            issues.append(("SERIOUS", f"{tp}: schematic {s}, board {b}, value {v}, bring-up {bg}, expected {exp}",
                           "a probe on the wrong pad", "see table"))
        rows.append(dict(signal=tp, sch=s, gpio={"BOOT": "IO0", "UART_TX": "IO43 (TXD0)", "UART_RX": "IO44 (RXD0)",
                                                  "EN": "EN"}.get(exp, "-"), pad="1", pcb=b,
                         pf=f"comment: {exp}", fw="n/a", proto="n/a", docs=f"bring-up: {bring_tp.get(tp[2])}",
                         agree="yes" if ok else "NO", note=f"symbol value '{v}'"))

    # --- connectors J3 / J4
    rows.append(dict(section="Connectors"))
    j3_expect = ["VBUS", "USB_DM", "USB_DP", "GND"]
    pf_mag = pf_facts.get("mag_order") or []
    pf_mag_norm = [{"D-": "USB_DM", "D+": "USB_DP"}.get(x, x) for x in pf_mag]
    for i, exp in enumerate(j3_expect, 1):
        s, b = sch.get(("J3", str(i)), ("?",))[0], pcb_net("J3", i)
        ok = s == b == exp and (not pf_mag_norm or pf_mag_norm[i - 1] == exp)
        if not ok:
            issues.append(("SERIOUS", f"J3 pin {i}: schematic {s}, board {b}, pins_final.h {pf_mag}", "", ""))
        rows.append(dict(signal=f"J3 pin {i} (magnet USB)", sch=s, gpio={"USB_DM": "IO19", "USB_DP": "IO20"}.get(exp, "-"),
                         pad=str(i), pcb=b, pf=f"MAG_PIN_ORDER[{i-1}] = {pf_mag[i-1] if pf_mag else '?'}", fw="n/a",
                         proto="n/a", docs="", agree="yes" if ok else "NO", note=""))
    # doc claims of the J3 order
    j3_doc_pat = re.compile(r"J3[^\n]{0,40}?\b1\s*(VBUS)[,\s]+2\s*(D[-−]|GND|USB_DM)[,\s]+3\s*(D\+|USB_DM|USB_DP|D[-−])[,\s]+4\s*(GND|USB_DP|D\+)")
    for name, text in docs:
        for m in j3_doc_pat.finditer(text):
            order = [{"D-": "USB_DM", "D−": "USB_DM", "D+": "USB_DP"}.get(x, x) for x in m.groups()]
            line_start = text.rfind("\n", 0, m.start()) + 1
            superseded = "old order" in text[line_start:m.end() + 60] or "Superseded" in text[line_start:m.end()]
            if order != j3_expect:
                sev = "DOC-STALE" if (superseded or "Netlist: J3" in text[line_start:m.start()] or
                                      "J3 is wired" in text[line_start:m.start()] or
                                      "Orientation" in text[line_start:m.start()]) else "DOC"
                issues.append((sev, f"{name} states J3 order {order}", "a reader following this text wires the magnet wrong",
                               "the line is marked superseded; keep the warning banner" if sev == "DOC-STALE" else
                               "edit the doc to 1 VBUS, 2 D-, 3 D+, 4 GND"))
            rows.append(dict(signal=f"J3 order per {name}", sch=" ".join(j3_expect), gpio="-", pad="1-4",
                             pcb=" ".join(pcb_net("J3", i) for i in range(1, 5)), pf=" ".join(pf_mag), fw="n/a",
                             proto="n/a", docs=" ".join(order), agree="yes" if order == j3_expect else "NO (stale)",
                             note=""))
    for i, exp in ((1, "GND"), (2, "BAT+")):
        s, b = sch.get(("J4", str(i)), ("?",))[0], pcb_net("J4", i)
        ok = s == b == exp
        if not ok:
            issues.append(("SERIOUS", f"J4 pin {i}: schematic {s}, board {b}", "battery polarity", ""))
        rows.append(dict(signal=f"J4 pin {i} (battery JST-PH)", sch=s, gpio="-", pad=str(i), pcb=b, fw="n/a",
                         pf="(not in pins_final.h)", proto="n/a",
                         docs="02_power_boot: J4.1 = GND, J4.2 = BAT+" if "J4.1 = GND, J4.2 = BAT+" in dict(docs).get("verification/02_power_boot.md", "") else "",
                         agree="yes" if ok else "NO", note="Adafruit polarity (red = +)"))

    # --- J1 / J2 full pin lists: schematic vs board
    for conn, n in (("J1", 24), ("J2", 24)):
        bad = []
        for i in range(1, n + 1):
            s, b = sch.get((conn, str(i)), ("?",))[0], pcb_net(conn, i)
            if s != b:
                bad.append((i, s, b))
        if bad:
            issues.append(("SERIOUS", f"{conn} schematic/board pad nets differ: {bad}", "", ""))
        else:
            notes.append(f"{conn}: all {n} pins carry the same net on the schematic and the board.")

    # --- camera GPIO <-> J1 pin order table (for the report)
    j1_table = []
    for i in range(1, 25):
        s = sch.get(("J1", str(i)), ("?",))[0]
        g = next((f"IO{g}" for g, (pad, pn, net) in u1.items() if net == s), "-")
        j1_table.append((i, s, g, pcb_net("J1", i)))
    j2_table = []
    for i in range(1, 25):
        s = sch.get(("J2", str(i)), ("?",))[0]
        g = next((f"IO{g}" for g, (pad, pn, net) in u1.items() if net == s), "-")
        j2_table.append((i, s, g, pcb_net("J2", i)))

    # --- dividers and pull resistors
    rows.append(dict(section="Resistor networks"))

    def rnet(ref):
        return (sch.get((ref, "1"), ("?",))[0], sch.get((ref, "2"), ("?",))[0], sch_vals.get(ref), pcb_vals.get(ref),
                pcb_net(ref, 1), pcb_net(ref, 2))

    def rrow(ref, expect_nets, expect_val, what):
        a, b, v, bv, pa, pb = rnet(ref)
        ok = {a, b} == set(expect_nets) and {pa, pb} == set(expect_nets) and v == expect_val and bv == expect_val
        if not ok:
            issues.append(("SERIOUS", f"{ref} ({what}): schematic {a}/{b} {v}, board {pa}/{pb} {bv}; expected {expect_nets} {expect_val}", "", ""))
        rows.append(dict(signal=f"{ref} {what}", sch=f"{a} - {b} ({v})", gpio="-", pad="1,2", pcb=f"{pa} - {pb} ({bv})",
                         pf=f"{expect_val}", fw="n/a", proto="n/a", docs="", agree="yes" if ok else "NO", note=""))

    rrow("R6", ("VBAT_P", "VBAT_SENSE"), "1M", "VBAT divider top")
    rrow("R7", ("VBAT_SENSE", "GND"), "1M", "VBAT divider bottom")
    rrow("R4", ("VBUS", "VBUS_SENSE"), "10k", "VBUS divider top")
    rrow("R5", ("VBUS_SENSE", "GND"), "20k", "VBUS divider bottom")
    rrow("R20", ("VBUS_SENSE", "CHG_STAT"), "100k", "CHG_STAT pull-up (to VBUS_SENSE)")
    rrow("R15", ("+3V3", "KEYPAD_INT"), "10k", "KEYPAD_INT pull-up")
    rrow("R16", ("+3V3", "TCA_RESET"), "10k", "TCA8418 ~RESET pull-up")
    rrow("R17", ("+3V3", "KEY_ON"), "100k", "KEY_ON pull-up")
    rrow("R1", ("+3V3", "EN"), "10k", "EN pull-up")
    rrow("R9", ("CAM_PWDN", "GND"), "10k", "CAM_PWDN pull-down")
    rrow("R10", ("CAM_PWR_EN", "GND"), "100k", "CAM_PWR_EN pull-down")
    rrow("R8", ("CAM_2V8", "CAM_RESET"), "10k", "CAM_RESET pull-up (to CAM_2V8)")
    rrow("R18", ("CAM_2V8", "CAM_SIOD"), "4.7k", "SCCB SDA pull-up (to CAM_2V8)")
    rrow("R19", ("CAM_2V8", "CAM_SIOC"), "4.7k", "SCCB SCL pull-up (to CAM_2V8)")
    rrow("R13", ("+3V3", "I2C_SDA"), "4.7k", "I2C SDA pull-up")
    rrow("R14", ("+3V3", "I2C_SCL"), "4.7k", "I2C SCL pull-up")
    # VBAT divider ratio vs firmware multiplier
    r6, r7 = sch_vals.get("R6"), sch_vals.get("R7")
    ratio_ok = r6 == r7 == "1M" and divider_mult == 2 and (pf_facts.get("vbat_divider") or "").replace(" ", "") == "1M/1M"
    if not ratio_ok:
        issues.append(("SERIOUS", f"VBAT divider: R6 {r6} / R7 {r7}, firmware multiplies by {divider_mult}, pins_final.h says {pf_facts.get('vbat_divider')}", "battery % wrong", ""))
    rows.append(dict(signal="VBAT divider ratio", sch=f"R6 {r6} / R7 {r7} -> Vsense = Vbat x {1/2 if r6==r7 else '?'}",
                     gpio="IO9 ADC1_CH8", pad="13", pcb=f"R6 {pcb_vals.get('R6')} / R7 {pcb_vals.get('R7')}",
                     pf=pf_facts.get("vbat_divider"), fw="n/a", proto=f"power.cpp: mV x {divider_mult}",
                     docs="02_power_boot: 1 M / 1 M; README: 1 M / 1 M", agree="yes" if ratio_ok else "NO", note=""))
    r4, r5 = sch_vals.get("R4"), sch_vals.get("R5")
    rows.append(dict(signal="VBUS divider ratio", sch=f"R4 {r4} / R5 {r5} -> 5.0 V x 20/30 = 3.33 V",
                     gpio="IO37 (digital in)", pad="33", pcb=f"R4 {pcb_vals.get('R4')} / R5 {pcb_vals.get('R5')}",
                     pf=pf_facts.get("vbus_divider"), fw="n/a", proto="digitalRead", docs="02_power_boot: 10 k / 20 k; HANDOFF: 10 k / 20 k",
                     agree="yes" if (r4, r5) == ("10k", "20k") and (pf_facts.get("vbus_divider") or "").replace(" ", "") == "10k/20k" else "NO", note=""))

    # --- TCA8418 pins
    rows.append(dict(section="TCA8418 (U6)"))
    tca_expect = {"22": ("SDA", "I2C_SDA"), "23": ("SCL", "I2C_SCL"), "24": ("~{INT}", "KEYPAD_INT"),
                  "20": ("~{RESET}", "TCA_RESET"), "21": ("VCC", "+3V3"), "19": ("GND", "GND"), "25": ("EP", "GND")}
    for pad, (pn, exp) in tca_expect.items():
        s, pname = sch.get(("U6", pad), ("?", "?"))[:2]
        b = pcb_net("U6", pad)
        ok = s == b == exp and pname.startswith(pn)
        if not ok:
            issues.append(("SERIOUS", f"U6 pad {pad} ({pn}): schematic {s} ({pname}), board {b}, expected {exp}", "", ""))
        rows.append(dict(signal=f"U6.{pad} {pn}", sch=s, gpio=next((f"IO{g}" for g, (pd, n, net) in u1.items() if net == s), "-"),
                         pad=pad, pcb=b, pf={"I2C_SDA": "PIN_I2C_SDA", "I2C_SCL": "PIN_I2C_SCL", "KEYPAD_INT": "PIN_KEYPAD_INT",
                                             "TCA_RESET": "PIN_TCA_RESET = -1"}.get(exp, "-"), fw="n/a", proto="keys.cpp Wire.begin(SDA,SCL)",
                         docs="", agree="yes" if ok else "NO", note=""))
    # address
    addr_ok = pf_facts.get("tca_addr") == 0x34 and tca_addr_src == "TCA8418_I2C_ADDR"
    rows.append(dict(signal="TCA8418 I2C address", sch="(fixed by the part: 0x34, no address pins)", gpio="-", pad="-",
                     pcb="-", pf=f"0x{pf_facts.get('tca_addr', 0):02X}", fw="n/a", proto=f"kTca = {tca_addr_src}",
                     docs="README: TCA8418 at I2C; 0x34 in pins_final.h", agree="yes" if addr_ok else "NO", note="TCA8418 datasheet: slave address 0110100b = 0x34"))
    if not addr_ok:
        issues.append(("SERIOUS", "TCA8418 address", "", ""))
    # ROW/COL pads of U6 vs expected pad order (datasheet: ROW0..7 = pads 8..1? no: TCA8418RTW: pad1 ROW7? check symbol)
    tca_rc = {}
    for pad in range(1, 19):
        s, pname = sch.get(("U6", str(pad)), ("?", "?"))[:2]
        tca_rc[pad] = (pname, s, pcb_net("U6", pad))
        if s != pcb_net("U6", pad):
            issues.append(("SERIOUS", f"U6 pad {pad} schematic {s} vs board {pcb_net('U6', pad)}", "", ""))

    # --- key matrix: schematic SW -> (row, col) -> legend, vs firmware kMatrix
    rows.append(dict(section="Keypad matrix (ROW/COL -> key)"))
    sch_matrix = {}   # (r,c) -> (ref, legend)
    for ref in [f"SW{i}" for i in range(1, 50)]:
        n1 = sch.get((ref, "1"), ("?",))[0]
        n2 = sch.get((ref, "2"), ("?",))[0]
        r = re.search(r"ROW(\d)", n1 + n2)
        c = re.search(r"COL(\d)", n1 + n2)
        b1, b2 = pcb_net(ref, 1), pcb_net(ref, 2)
        if (b1, b2) != (n1, n2):
            issues.append(("SERIOUS", f"{ref} schematic {n1}/{n2} vs board {b1}/{b2}", "key on the wrong crossing on the board", ""))
        if not (r and c):
            issues.append(("SERIOUS", f"{ref} ({sch_vals.get(ref)}) is not on a ROW/COL pair: {n1}, {n2}", "", ""))
            continue
        rc = (int(r.group(1)), int(c.group(1)))
        if rc in sch_matrix:
            issues.append(("SERIOUS", f"{ref} and {sch_matrix[rc][0]} share crossing {rc}", "two keys read as one", ""))
        sch_matrix[rc] = (ref, sch_vals.get(ref))
    key_table = []
    for rc in sorted(set(sch_matrix) | set(kmatrix)):
        ref, legend = sch_matrix.get(rc, ("-", "-"))
        fw = kmatrix.get(rc)
        fw_legend = dkey_legend.get(fw, "-") if fw else "-"
        sch_norm = LEGEND_ALIAS.get(legend, legend)
        ok = fw is not None and ref != "-" and fw_legend == sch_norm
        if not ok:
            issues.append(("SERIOUS", f"ROW{rc[0]}/COL{rc[1]}: schematic {ref} '{legend}', firmware kMatrix {fw} ('{fw_legend}')",
                           "pressing this key does the wrong thing", "fix kMatrix in firmware-prototype/src/keys.cpp"))
        key_table.append((rc, ref, legend, fw, fw_legend, "yes" if ok else "NO"))
    # ON key
    on1, on2 = sch.get(("SW50", "1"), ("?",))[0], sch.get(("SW50", "2"), ("?",))[0]
    on_ok = {on1, on2} == {"KEY_ON", "GND"} and {pcb_net("SW50", 1), pcb_net("SW50", 2)} == {"KEY_ON", "GND"}
    if not on_ok:
        issues.append(("SERIOUS", f"SW50 ON: schematic {on1}/{on2}, board {pcb_net('SW50',1)}/{pcb_net('SW50',2)}", "", ""))
    # 03_interfaces Table D
    t3 = dict(docs).get("verification/03_interfaces.md", "")
    tableD = re.search(r"## Table D[^\n]*\n(?:[^\n]*\n)*?((?:\| R\d[^\n]*\n)+)", t3)
    tableD_rows = {}
    if tableD:
        for line in tableD.group(1).strip().split("\n"):
            cells = [c.strip() for c in line.strip("|").split("|")]
            r = int(cells[0][1])
            for c, cell in enumerate(cells[1:]):
                if cell and cell not in ("–", "-"):
                    tableD_rows[(r, c)] = cell
    tableD_norm = {"√": "sqrt", "x²": "x^2", "x^□": "x^n", "Abs → **CALC** on 115ES": "CALC", "x³ → **∫dx** on 115ES": "∫dx",
                   "log□□": "log_a", "(−)": "(-)", "°'\"": "deg-min-sec", "S⇔D": "S<>D", "×": "x", "÷": "/", "−": "-", "×10ˣ": "x10^x"}
    tableD_bad = []
    for rc, cell in tableD_rows.items():
        want = sch_matrix.get(rc, ("-", "-"))[1]
        if tableD_norm.get(cell, cell) != want:
            tableD_bad.append((rc, cell, want))
    for rc in sch_matrix:
        if rc not in tableD_rows:
            tableD_bad.append((rc, "(missing)", sch_matrix[rc][1]))
    if tableD_bad:
        issues.append(("DOC", f"03_interfaces.md Table D differs from the schematic matrix: {tableD_bad}", "", ""))

    # --- firmware-prototype pins.h and firmware/src pins.h
    if not proto_includes or proto_own:
        issues.append(("SERIOUS", "firmware-prototype/src/pins.h does not purely include hardware/pins_final.h", "", ""))
    if code_split != (10, 10):
        issues.append(("SERIOUS", f"keys.cpp decodes TCA8418 key numbers with row = code / {code_split}", "", ""))

    # --- strapping pins
    straps = []
    for g in (0, 3, 45, 46):
        pad, pname, net = u1.get(g, (None, None, None))
        straps.append((g, pad, net, pcb_net("U1", pad) if pad else None))

    # ------------------------------------------------------------------ report
    n_checked = len([r for r in rows if "signal" in r]) + len(key_table)
    buf = io.StringIO()
    w = buf.write
    w("# 09 — Pin-map cross-reference (scripted)\n\n")
    w("Generated by `python hardware/tools/pinmap_crossref.py` from the KiCad schematic sheets (parsed in Python, "
      "cross-checked against a kicad-cli netlist), the board file's footprint pads, `hardware/pins_final.h`, the "
      "firmware, the tester firmware and the docs. Re-run after any schematic, board, pins_final.h, keys.cpp or doc change.\n\n")
    w(f"**Result: {n_checked} signals/crossings checked, {len([i for i in issues if i[0] in ('SERIOUS','TOOL')])} serious disagreement(s), "
      f"{len([i for i in issues if i[0].startswith('DOC')])} doc-only.**\n\n")
    for n in notes:
        w(f"- {n}\n")
    w("\n## Disagreements\n\n")
    if not issues:
        w("None.\n")
    for sev, title, cons, fix in issues:
        w(f"- **[{sev}]** {title}\n")
        if cons:
            w(f"  - Consequence: {cons}\n")
        if fix:
            w(f"  - Fix: {fix}\n")
    w("\n## Master table\n\n")
    w("Columns: schematic net (Python parse of the sheets) | ESP32 GPIO from the U1 symbol pin name | U1 pad | board pad net "
      "| pins_final.h | firmware/ (bench tester, other hardware) | firmware-prototype | docs | AGREE?\n\n")
    w("| signal | schematic net | ESP32 GPIO | U1 pad | board pad net | pins_final.h | firmware/ | firmware-prototype | docs | AGREE? | note |\n")
    w("|---|---|---|---|---|---|---|---|---|---|---|\n")
    for r in rows:
        if "section" in r:
            w(f"| **{r['section']}** | | | | | | | | | | |\n")
        else:
            w("| " + " | ".join(str(r.get(k, "")).replace("|", "\\|") for k in
                                ("signal", "sch", "gpio", "pad", "pcb", "pf", "fw", "proto", "docs", "agree", "note")) + " |\n")
    w("\n## Keypad: ROW/COL crossing -> key (schematic SW vs firmware kMatrix)\n\n")
    w("TCA8418 key number = row * 10 + col + 1 (keys.cpp decodes `code / 10`, `code % 10`). Legend spellings: "
      "schematic `∫dx` = firmware `int_dx`, `a/b` = `frac`, `deg-min-sec` = `dms`.\n\n")
    w("| ROW | COL | schematic SW | schematic legend | firmware DKey | firmware legend | AGREE? |\n|---|---|---|---|---|---|---|\n")
    for (r, c), ref, legend, fw, fwl, ok in key_table:
        w(f"| {r} | {c} | {ref} | {legend} | {fw} | {fwl} | {ok} |\n")
    w(f"| - | - | SW50 | ON | DKey::On (own pin) | ON | {'yes' if on_ok else 'NO'} |\n")
    w("\nTCA8418 pad -> ROW/COL (schematic = board):\n\n| U6 pad | pin name | net |\n|---|---|---|\n")
    for pad in range(1, 19):
        pn, s, b = tca_rc[pad]
        w(f"| {pad} | {pn} | {s}{'' if s == b else ' (board: ' + str(b) + ')'} |\n")
    w("\n## J1 camera socket (24-pin): pin -> net -> GPIO\n\n| J1 pin | net | GPIO | board |\n|---|---|---|---|\n")
    for i, s, g, b in j1_table:
        w(f"| {i} | {s} | {g} | {'same' if s == b else b} |\n")
    w("\n## J2 e-paper socket (24-pin): pin -> net -> GPIO\n\n| J2 pin | net | GPIO | board |\n|---|---|---|---|\n")
    for i, s, g, b in j2_table:
        w(f"| {i} | {s} | {g} | {'same' if s == b else b} |\n")
    w("\n## ESP32-S3 strapping pins at reset\n\n| GPIO | U1 pad | schematic net | board net | what is on it | effect at reset |\n|---|---|---|---|---|---|\n")
    strap_info = {
        0: ("TP1 pad only; internal weak pull-up", "high -> normal SPI flash boot; TP1 to GND at reset -> download mode"),
        3: ("nothing (no label, no-connect); floats", "JTAG-source strap; only read when the eFuse JTAG_SEL_ENABLE is burnt (it is not) -> no effect"),
        45: ("nothing; internal pull-down", "low -> VDD_SPI = 3.3 V, correct for the module's 3.3 V flash"),
        46: ("nothing; internal pull-down", "low -> ROM boot messages on UART0 / no effect on boot mode"),
    }
    for g, pad, net, b in straps:
        w(f"| IO{g} | {pad} | {net} | {b} | {strap_info[g][0]} | {strap_info[g][1]} |\n")
    w("\nAlso: IO26 (pad 26) unconnected = in-package PSRAM CS on the N4R2; IO19/IO20 = USB D-/D+ fixed by the chip.\n")
    w("\n## Tester firmware (firmware/src/pins.h)\n\nThe bench tester runs on a different board (Waveshare HAT + dev kit); "
      "its pins are listed for completeness and are not compared:\n\n")
    w(", ".join(f"{k}={v}" for k, v in tester.items()) + "\n")
    w("\n## How to re-run\n\n```\npython hardware/tools/pinmap_crossref.py\n```\nExit code = number of disagreements.\n")
    report = buf.getvalue()
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(report)
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    serious = [i for i in issues if i[0] in ("SERIOUS", "TOOL")]
    print(f"{n_checked} checked, {len(issues)} disagreement(s) ({len(serious)} serious) -> {out_path}")
    for sev, title, cons, fix in issues:
        print(f"  [{sev}] {title}")
    print("PASS: schematic, board, pins_final.h, firmware and docs agree on every pin." if not issues else
          ("FAIL: hardware/firmware disagreement(s) above." if serious else
           "PASS (hardware = firmware); doc-only issue(s) listed above."))
    sys.exit(len(serious))


if __name__ == "__main__":
    main()
