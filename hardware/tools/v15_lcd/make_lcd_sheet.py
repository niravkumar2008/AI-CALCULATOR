#!/usr/bin/env python3
"""Stage 15 (v15-LCD): write hardware/kicad_v15_lcd/lcd.kicad_sch (the sheet that replaces the
e-paper driver sheet) and add the two new symbols to ai_calc.kicad_sym.

Everything is generated from scratch so the sheet is reproducible; lib symbols for Device:C,
Device:R and Transistor_FET:AO3401A are copied from the sibling sheets (same KiCad version).
Net names are global labels on the pin ends, the same style the other sheets use.

usage: python make_lcd_sheet.py <hardware/kicad_v15_lcd dir>
"""
import re
import sys
import uuid
from pathlib import Path

ROOT_UUID = "c7949ef3-e92b-45dc-b599-4b69606d677f"      # root sheet (ai_calc_v15_lcd.kicad_sch)
SHEET_UUID = "fdaba283-ba88-4d88-a06e-1b44c8a2f0d2"     # the (old e-paper, now LCD) sub-sheet instance
PROJECT = "ai_calc_v15_lcd"


def block(text, start_token):
    """Return the balanced s-expression block starting at start_token."""
    i = text.find(start_token)
    if i < 0:
        raise SystemExit(f"not found: {start_token}")
    d = 0
    for k in range(i, len(text)):
        c = text[k]
        if c == "(":
            d += 1
        elif c == ")":
            d -= 1
            if d == 0:
                return text[i:k + 1]
    raise SystemExit("unbalanced")


def pins_of(lib_block):
    out = {}
    for m in re.finditer(r'\(pin (\w+) \w+\s*\(at ([-\d.]+) ([-\d.]+) (\d+)\)\s*\(length ([\d.]+)\)\s*\(name "([^"]*)"[\s\S]*?\(number "([^"]*)"', lib_block):
        out[m.group(7)] = (float(m.group(2)), float(m.group(3)), int(m.group(4)), m.group(6))
    return out


def sym_pin(num, name, x, y, ang, etype="passive", length=2.54):
    return (f'\t\t\t\t(pin {etype} line\n\t\t\t\t\t(at {x:g} {y:g} {ang})\n\t\t\t\t\t(length {length:g})\n'
            f'\t\t\t\t\t(name "{name}"\n\t\t\t\t\t\t(effects\n\t\t\t\t\t\t\t(font\n\t\t\t\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t\t\t\t)\n\t\t\t\t\t\t)\n\t\t\t\t\t)\n'
            f'\t\t\t\t\t(number "{num}"\n\t\t\t\t\t\t(effects\n\t\t\t\t\t\t\t(font\n\t\t\t\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t\t\t\t)\n\t\t\t\t\t\t)\n\t\t\t\t\t)\n\t\t\t\t)\n')


def prop(name, value, x, y, hide=False, justify=None):
    j = f"\n\t\t\t\t\t(justify {justify})" if justify else ""
    h = "\n\t\t\t(hide yes)" if hide else ""
    return (f'\t\t\t(property "{name}" "{value}"\n\t\t\t\t(at {x:g} {y:g} 0){h}\n\t\t\t\t(show_name no)\n\t\t\t\t(do_not_autoplace no)\n'
            f'\t\t\t\t(effects\n\t\t\t\t\t(font\n\t\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t\t){j}\n\t\t\t\t)\n\t\t\t)\n')


def lib_symbol_custom(name, ref_prefix, value, footprint, descr, keywords, pins, body, extra_props=()):
    """A library symbol (lib_symbols entry) with a rectangle body and the given pins.
    pins: list of (number, name, x, y, angle, etype). body: (x1, y1, x2, y2)."""
    s = (f'\t\t(symbol "{name}"\n\t\t\t(pin_names\n\t\t\t\t(offset 1.016)\n\t\t\t)\n\t\t\t(exclude_from_sim no)\n\t\t\t(in_bom yes)\n\t\t\t(on_board yes)\n'
         f'\t\t\t(in_pos_files yes)\n\t\t\t(duplicate_pin_numbers_are_jumpers no)\n')
    s += prop("Reference", ref_prefix, 0, body[3] + 2.54)
    s += prop("Value", value, 0, body[1] - 2.54)
    s += prop("Footprint", footprint, 0, body[1] - 5.08, hide=True)
    s += prop("Datasheet", "", 0, body[1] - 7.62, hide=True)
    s += prop("Description", descr, 0, body[1] - 10.16, hide=True)
    for k, v in extra_props:
        s += prop(k, v, 0, body[1] - 12.7, hide=True)
    s += prop("ki_keywords", keywords, 0, 0, hide=True)
    short = name.split(":")[-1]
    s += f'\t\t\t(symbol "{short}_0_1"\n'
    s += (f'\t\t\t\t(rectangle\n\t\t\t\t\t(start {body[0]:g} {body[1]:g})\n\t\t\t\t\t(end {body[2]:g} {body[3]:g})\n'
          f'\t\t\t\t\t(stroke\n\t\t\t\t\t\t(width 0.254)\n\t\t\t\t\t\t(type default)\n\t\t\t\t\t)\n\t\t\t\t\t(fill\n\t\t\t\t\t\t(type background)\n\t\t\t\t\t)\n\t\t\t\t)\n')
    s += '\t\t\t)\n'
    s += f'\t\t\t(symbol "{short}_1_1"\n'
    for num, pname, x, y, ang, et in pins:
        s += sym_pin(num, pname, x, y, ang, et)
    s += '\t\t\t)\n\t\t\t(embedded_fonts no)\n\t\t)\n'
    return s


def label(net, x, y, pin_angle):
    rot = {0: 180, 180: 0, 270: 90, 90: 270}[pin_angle]
    just = {0: "right", 180: "left", 270: "left", 90: "right"}[pin_angle]
    return (f'\t(global_label "{net}"\n\t\t(at {x:g} {y:g} {rot})\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n'
            f'\t\t\t(justify {just})\n\t\t)\n\t\t(uuid "{uuid.uuid4()}")\n\t)\n')


def no_connect(x, y):
    return f'\t(no_connect\n\t\t(at {x:g} {y:g})\n\t\t(uuid "{uuid.uuid4()}")\n\t)\n'


def text(s, x, y):
    s = s.replace('"', '\\"')
    return (f'\t(text "{s}"\n\t\t(exclude_from_sim no)\n\t\t(at {x:g} {y:g} 0)\n\t\t(effects\n\t\t\t(font\n\t\t\t\t(size 1.27 1.27)\n\t\t\t)\n'
            f'\t\t\t(justify left top)\n\t\t)\n\t\t(uuid "{uuid.uuid4()}")\n\t)\n')


def instance(lib_id, ref, value, footprint, x, y, pins, lcsc="", mpn="", descr="", value_just="left", ref_dx=2.54, val_dx=2.54):
    """Symbol instance at (x, y), rotation 0. pins: list of pin numbers (strings)."""
    u = uuid.uuid4()
    s = (f'\t(symbol\n\t\t(lib_id "{lib_id}")\n\t\t(at {x:g} {y:g} 0)\n\t\t(unit 1)\n\t\t(body_style 1)\n\t\t(exclude_from_sim no)\n'
         f'\t\t(in_bom yes)\n\t\t(on_board yes)\n\t\t(in_pos_files yes)\n\t\t(dnp no)\n\t\t(uuid "{u}")\n')
    def p(name, val, px, py, hide=False, justify=None):
        j = f"\n\t\t\t\t(justify {justify})" if justify else ""
        h = "\n\t\t\t(hide yes)" if hide else ""
        return (f'\t\t(property "{name}" "{val}"\n\t\t\t(at {px:g} {py:g} 0){h}\n\t\t\t(show_name no)\n\t\t\t(do_not_autoplace no)\n'
                f'\t\t\t(effects\n\t\t\t\t(font\n\t\t\t\t\t(size 1.27 1.27)\n\t\t\t\t){j}\n\t\t\t)\n\t\t)\n')
    s += p("Reference", ref, x + ref_dx, y - 1.27, justify="left")
    s += p("Value", value, x + val_dx, y + 1.27, justify=value_just)
    s += p("Footprint", footprint, x, y, hide=True)
    s += p("Datasheet", "", x, y, hide=True)
    s += p("Description", descr, x, y, hide=True)
    if lcsc:
        s += p("LCSC", lcsc, x, y, hide=True)
    if mpn:
        s += p("MPN", mpn, x, y, hide=True)
    for pn in pins:
        s += f'\t\t(pin "{pn}"\n\t\t\t(uuid "{uuid.uuid4()}")\n\t\t)\n'
    s += (f'\t\t(instances\n\t\t\t(project "{PROJECT}"\n\t\t\t\t(path "/{ROOT_UUID}/{SHEET_UUID}"\n\t\t\t\t\t(reference "{ref}")\n\t\t\t\t\t(unit 1)\n'
          f'\t\t\t\t)\n\t\t\t)\n\t\t)\n\t)\n')
    return s


def main(d):
    d = Path(d)
    power = (d / "power.kicad_sch").read_text(encoding="utf-8")
    old = (d.resolve().parent / "kicad" / "epaper.kicad_sch").read_text(encoding="utf-8")   # v14 sheet (read-only), used only as a template
    lib_c = block(old, '(symbol "Device:C"')
    lib_r = block(old, '(symbol "Device:R"')
    lib_q = block(power, '(symbol "Transistor_FET:AO3401A"')
    lib_qn = block(old, '(symbol "Transistor_FET:AO3400A"')
    sheet_uuid = re.search(r'\(uuid ([0-9a-f-]+)\)', old).group(1)

    # --- custom symbols -------------------------------------------------------------
    # 30-pin LCD FPC (Unvision/Newvisio 190-1732TBWPG01 family: same panel as LilyGO T-Display-S3 / Heltec HT-VMT190)
    lcd_pins = ["GND", "VDD", "IM2", "IM1", "RESET", "CS", "SCL(D/C)", "RS(WR)", "RD", "SDA", "DB0", "DB1", "DB2", "DB3",
                "DB4", "DB5", "DB6", "DB7", "SDO/NC", "LEDA", "LEDK1", "LEDK2", "LEDK3", "LEDK4", "GND", "TP_RESET(T)",
                "TP_SCL(NC)", "TP_SDA(NC)", "TP_INT(NC)", "GND"]
    pins = []
    for n, nm in enumerate(lcd_pins, 1):
        pins.append((str(n), nm, -12.7, 38.1 - n * 2.54, 0, "passive"))
    pins.append(("MP", "MP", 0, -43.18, 90, "passive"))
    lib_j = lib_symbol_custom("ai_calc:LCD_FPC_30P_HX", "J", "LCD_FPC_30P_HX", "ai_calc:FPC_30P_P0.5mm_DualContact_C2919501_LcdReversed",
                              "1.9in 170x320 IPS panel 190-1732TBWPG01 (ST7789V3), 30-pin 0.5 mm FPC, HDGC 0.5K-HX-30PWB dual-contact socket",
                              "LCD FPC ST7789", pins, (-10.16, -40.64, 10.16, 38.1))

    # --- instances ------------------------------------------------------------------
    qp = pins_of(lib_q); qnp = pins_of(lib_qn); cp = pins_of(lib_c); rp = pins_of(lib_r)
    body = []
    labels = []

    def place(lib_id, ref, value, fp, x, y, nets, pinmap, lcsc="", mpn="", descr="", **kw):
        """nets: {pin_number: net or 'NC'}; pinmap: {pin_number: (px, py, angle, name)} in symbol coords (y up)."""
        body.append(instance(lib_id, ref, value, fp, x, y, list(pinmap.keys()), lcsc, mpn, descr, **kw))
        for pn, (px, py, ang, _n) in pinmap.items():
            sx, sy = round(x + px, 2), round(y - py, 2)
            net = nets[pn]
            labels.append(no_connect(sx, sy) if net == "NC" else label(net, sx, sy, ang))

    jpins = {str(n): (-12.7, 38.1 - n * 2.54, 0, "") for n in range(1, 31)}
    jpins["MP"] = (0, -43.18, 90, "MP")
    jnets = {"1": "GND", "2": "LCD_VDD", "3": "LCD_VDD", "4": "LCD_VDD", "5": "LCD_RST", "6": "LCD_CS", "7": "LCD_SCK", "8": "LCD_DC",
             "9": "GND", "10": "LCD_MOSI", "19": "NC", "20": "LCD_LEDA", "21": "LCD_LEDK", "22": "LCD_LEDK", "23": "LCD_LEDK",
             "24": "LCD_LEDK", "25": "GND", "26": "NC", "27": "NC", "28": "NC", "29": "NC", "30": "GND", "MP": "GND"}
    for n in range(11, 19):
        jnets[str(n)] = "GND"
    place("ai_calc:LCD_FPC_30P_HX", "J5", "LCD 1.9in 170x320 IPS 30P FPC (190-1732TBWPG01 / T-Display-S3 panel)",
          "ai_calc:FPC_30P_P0.5mm_DualContact_C2919501_LcdReversed", 215.9, 101.6, jnets, jpins, "C2919501", "HDGC 0.5K-HX-30PWB",
          "30-pin 0.5 mm dual-contact FPC socket, 1.0 mm high, front insert / rear flip", ref_dx=-5.08, val_dx=-10.16)


    cmap = {k: (v[0], v[1], v[2], v[3]) for k, v in cp.items()}
    qnmap = {k: (v[0], v[1], v[2], v[3]) for k, v in qnp.items()}

    rmap = {k: (v[0], v[1], v[2], v[3]) for k, v in rp.items()}
    qmap = {k: (v[0], v[1], v[2], v[3]) for k, v in qp.items()}
    place("Transistor_FET:AO3400A", "Q5", "AO3400A (backlight switch)", "Package_TO_SOT_SMD:SOT-23", 120.65, 101.6,
          {"1": "LCD_BL_EN", "2": "GND", "3": "LCD_LEDK"}, qnmap, "C20917", "AO3400A",
          "N-MOSFET low-side switch for the 4 backlight LEDs, PWM on LCD_BL_EN (IO4)")
    place("Device:R", "R23", "15R", "Resistor_SMD:R_0603_1608Metric", 134.62, 88.9, {"1": "+3V3", "2": "LCD_LEDA"}, rmap, "C22810",
          descr="Backlight series resistor: 4 LEDs in parallel, (3.3 V - ~2.8 V) / 22 R = ~20-25 mA total at 100 % PWM")
    place("Device:R", "R22", "100k", "Resistor_SMD:R_0402_1005Metric", 101.6, 101.6, {"1": "LCD_BL_EN", "2": "GND"}, rmap, "C25741", descr="Gate pull-down: backlight off while the ESP32 sleeps or resets")
    place("Transistor_FET:AO3401A", "Q4", "AO3401A (LCD power switch)", "Package_TO_SOT_SMD:SOT-23", 63.5, 55.88,
          {"1": "LCD_PWR_N", "2": "+3V3", "3": "LCD_VDD"}, qmap, "C15127", "AO3401A",
          "P-MOSFET high-side switch: +3V3 -> LCD_VDD when LCD_PWR_N (IO33) is low")
    place("Device:R", "R21", "100k", "Resistor_SMD:R_0402_1005Metric", 48.26, 55.88, {"1": "+3V3", "2": "LCD_PWR_N"}, rmap, "C25741", descr="Gate pull-up: LCD off in deep sleep / before the firmware runs")
    place("Device:C", "C36", "100nF", "Capacitor_SMD:C_0402_1005Metric", 83.82, 55.88, {"1": "LCD_VDD", "2": "GND"}, cmap, "C1525", descr="LCD VDD decoupling at the FPC socket")
    place("Device:C", "C19", "4.7uF", "Capacitor_SMD:C_0603_1608Metric", 96.52, 55.88, {"1": "LCD_VDD", "2": "GND"}, cmap, "C19666", descr="LCD VDD bulk (was the e-paper 3V3 cap)")

    notes = [
        ("v15-LCD, 2026-10-08. 1.9in 170x320 IPS (ST7789V3) replaces the 2.13in e-paper. Panel = Unvision/Newvisio 190-1732TBWPG01 family (the LilyGO T-Display-S3 / Heltec HT-VMT190 panel), 30-pin 0.5 mm FPC, 4-wire SPI: IM1 = IM2 = VDD, RD and DB0..DB7 to GND (vendor 4-SPI circuit), SDO and TP pins open.", 20, 15),
        ("SPI on the old e-paper GPIOs, re-assigned so the board routes without crossings: MOSI IO5, D/C IO6, SCK IO8, CS IO41, RST IO42 (review 13, 2026-10-08: J5 footprint pad numbers are mirrored, pad N = socket contact 31-N, so that panel finger 1 lands on pad 1 after the tail folds through the slot; the five B.Cu lanes keep their order, so the GPIO roles swapped) (any GPIO works through the ESP32-S3 GPIO matrix). IO4 = LCD_BL_EN (PWM; KEYPAD_INT moved from IO4 to IO3, both RTC wake pins). IO33 (was EPD_BUSY) = LCD_PWR_N, active low, R21 pull-up: panel unpowered in deep sleep (ST7789 standby alone is ~20 uA).", 20, 20),
        ("Backlight: 4 white LEDs in parallel inside the panel (rated 60 mA, 3.0-3.4 V). LEDA <- R23 15R <- +3V3; the four cathodes are tied (LCD_LEDK) and switched by Q5 (AO3400A) with LEDC PWM on IO4. (3.3 V - Vf ~2.8 V) / 22 R = about 20-25 mA at 100 % duty (~1/3 of rated brightness, enough indoors); R22 keeps Q5 off in deep sleep. A constant-current driver (AW9364, LCSC C401007) is the upgrade if more brightness is wanted.", 20, 25),
        ("Rules: drive every LCD SPI pin low before LCD_PWR_N goes high, or the panel is back-powered through its ESD diodes. BL_EN low in sleep (Q5 off: no backlight current). No TE pin on this panel.", 20, 30),
        ("Removed vs v14: J2 (24P e-paper socket), the SSD1680 booster L1, Q3, R11, R12, D3, D4, D5, C20..C30 (17 parts). Added: J5, Q4, Q5, R21, R22, R23, C36 (C19 re-used on LCD_VDD).", 20, 33),
    ]

    hdr = ('(kicad_sch\n\t(version 20260101)\n\t(generator "eeschema")\n\t(generator_version "10.0")\n'
           f'\t(uuid {sheet_uuid})\n\t(paper "A4")\n\t(lib_symbols\n')
    out = hdr + lib_c + "\n" + lib_r + "\n" + lib_q + "\n" + lib_qn + "\n" + lib_j + "\t)\n"
    out += "".join(body) + "".join(labels)
    for s, x, y in notes:
        out += text(s, x, y)
    out += '\t(sheet_instances\n\t\t(path "/"\n\t\t\t(page "1")\n\t\t)\n\t)\n\t(embedded_fonts no)\n)\n'
    (d / "lcd.kicad_sch").write_text(out, encoding="utf-8")

    # --- project symbol library: add the two custom symbols -------------------------
    lib = (d / "ai_calc.kicad_sym").read_text(encoding="utf-8")
    for name, blk in (("LCD_FPC_30P_HX", lib_j),):
        if f'(symbol "{name}"' in lib:
            continue
        b = blk.replace(f'(symbol "ai_calc:{name}"', f'(symbol "{name}"', 1)
        # library files use one indent level less than lib_symbols entries
        b = "\n".join(line[1:] if line.startswith("\t") else line for line in b.splitlines()) + "\n"
        lib = lib.rstrip()
        assert lib.endswith(")")
        lib = lib[:-1] + b + ")\n"
    (d / "ai_calc.kicad_sym").write_text(lib, encoding="utf-8")
    print("wrote lcd.kicad_sch and ai_calc.kicad_sym")


if __name__ == "__main__":
    main(sys.argv[1])
