# Stage 15 (v15-LCD) board step 2: place J5 (30-pin LCD FPC socket), Q4 (LCD power switch), Q5 (backlight
# switch), R21..R23, C19, C36; route everything; refill zones. Coordinates in mm, board frame.
# Run with KiCad's python after board_step1_cleanup.py.
#
# GPIO decisions taken here (mirrored in lcd.kicad_sch / mcu.kicad_sch / pins_v15_lcd.h):
#   IO4  (U1 pad 8)  = LCD_BL_EN   (was KEYPAD_INT; its F.Cu escape can reach the LCD area)
#   IO3  (U1 pad 7)  = KEYPAD_INT  (was free; RTC pin, so it still wakes the chip; it is boxed in by the U1 fan-out)
#   IO33 (U1 pad 28) = LCD_PWR_N   (was EPD_BUSY; its existing via at (138.5, 94.65) feeds a B.Cu run along y 81)
#   IO5/6/8/41/42   = LCD_RST/CS/SCK/DC/MOSI (the five e-paper SPI lines, re-named so the bus order matches the socket)
import os, sys
import pcbnew
from pcbnew import VECTOR2I, FromMM as mm, ToMM as tm

B = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\kicad_v15_lcd\ai_calc_v15_lcd.kicad_pcb"
PROJ = os.path.dirname(B)
KLIB = r"C:\Program Files\KiCad\10.0\share\kicad\footprints"
b = pcbnew.LoadBoard(B)
F, Bk = pcbnew.F_Cu, pcbnew.B_Cu
FAB = pcbnew.F_Fab


def net(name):
    n = b.FindNet(name)
    if n is None:
        n = pcbnew.NETINFO_ITEM(b, name); b.Add(n)
    return n


def place(ref, libnick, libdir, name, value, x, y, rot, lcsc, nets, descr="", mpn=""):
    fp = pcbnew.FootprintLoad(libdir, name)
    fp.SetFPID(pcbnew.LIB_ID(libnick, name))
    fp.SetReference(ref); fp.SetValue(value)
    fp.SetPosition(VECTOR2I(mm(x), mm(y))); fp.SetOrientationDegrees(rot); fp.SetLayer(F)
    fp.SetField("LCSC", lcsc)
    if descr:
        fp.SetField("Description", descr)
    if mpn:
        fp.SetField("MPN", mpn)
    # extra fields hidden on F.Fab (a new field is created visible on the silkscreen!); references hidden like the rest of the board
    for fld in fp.GetFields():
        if fld.GetName() not in ("Reference", "Value"):
            fld.SetVisible(False); fld.SetLayer(FAB)
    fp.Reference().SetVisible(False)
    fp.Value().SetVisible(False)
    b.Add(fp)
    for p in fp.Pads():
        n = nets.get(p.GetNumber())
        if n:
            p.SetNet(net(n))
    return fp


def pads(fp):
    return {p.GetNumber(): (round(tm(p.GetPosition().x), 3), round(tm(p.GetPosition().y), 3)) for p in fp.Pads()}


def track(x1, y1, x2, y2, netname, layer=F, w=0.2):
    t = pcbnew.PCB_TRACK(b); t.SetStart(VECTOR2I(mm(x1), mm(y1))); t.SetEnd(VECTOR2I(mm(x2), mm(y2)))
    t.SetWidth(mm(w)); t.SetLayer(layer); t.SetNet(net(netname)); b.Add(t); return t


def route(points, netname, layer=F, w=0.2):
    for (x1, y1), (x2, y2) in zip(points, points[1:]):
        track(x1, y1, x2, y2, netname, layer, w)


def via(x, y, netname, d=0.5, drill=0.3):
    v = pcbnew.PCB_VIA(b); v.SetPosition(VECTOR2I(mm(x), mm(y))); v.SetWidth(mm(d)); v.SetDrill(mm(drill))
    v.SetViaType(pcbnew.VIATYPE_THROUGH); v.SetLayerPair(F, Bk); v.SetNet(net(netname)); b.Add(v); return v


def silk_line(x1, y1, x2, y2, w=0.15):
    s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_SEGMENT); s.SetLayer(pcbnew.F_SilkS)
    s.SetStart(VECTOR2I(mm(x1), mm(y1))); s.SetEnd(VECTOR2I(mm(x2), mm(y2))); s.SetWidth(mm(w)); b.Add(s)


# ------------------------------------------------------------------ leftovers from step 1, GPIO re-assignment at U1
for t in list(b.GetTracks()):
    n = str(t.GetNetname()); s_, e_ = t.GetStart(), t.GetEnd()
    xs = (tm(s_.x), tm(e_.x)); ys = (tm(s_.y), tm(e_.y))
    if n == "LCD_BL_EN" and max(xs) > 160:
        b.Delete(t); continue
    if n == "GND" and t.GetClass() == "PCB_TRACK" and min(xs) >= 155.0 and max(xs) <= 184.0 and min(ys) >= 79.0 and max(ys) <= 111.0:
        b.Delete(t); continue
    if max(xs) >= 177.0 and min(xs) <= 178.8 and max(ys) >= 82.7 and min(ys) <= 103.5:
        b.Delete(t); continue
    if n == "LCD_BL_EN":
        t.SetNet(net("LCD_PWR_N"))        # U1 pad 28 stub + via (138.5, 94.65)
U1 = b.FindFootprintByReference("U1")
for p_ in U1.Pads():
    if p_.GetNumber() == "7":
        p_.SetNet(net("KEYPAD_INT"))
    elif p_.GetNumber() == "8":
        p_.SetNet(net("LCD_BL_EN"))
    elif p_.GetNumber() == "28":
        p_.SetNet(net("LCD_PWR_N"))
for t in list(b.GetTracks()):
    if str(t.GetNetname()) == "KEYPAD_INT" and t.GetClass() == "PCB_TRACK":
        s_, e_ = t.GetStart(), t.GetEnd()
        if abs(tm(s_.x) - 127.55) < 0.01 and abs(tm(e_.x) - 127.55) < 0.01 and min(tm(s_.y), tm(e_.y)) < 104.1:
            top = VECTOR2I(mm(127.55), mm(105.5))
            if tm(s_.y) < tm(e_.y):
                t.SetStart(top)
            else:
                t.SetEnd(top)
            print("KEYPAD_INT vertical now starts at y 105.5")
route([(126.7, 104.0), (126.7, 104.55), (127.55, 105.4), (127.55, 105.5)], "KEYPAD_INT", F, 0.15)

# ------------------------------------------------------------------ placement
PRJ = os.path.join(PROJ, "ai_calc.pretty")
j5nets = {"1": "GND", "2": "LCD_VDD", "3": "LCD_VDD", "4": "LCD_VDD", "5": "LCD_RST", "6": "LCD_CS", "7": "LCD_SCK", "8": "LCD_DC",
          "9": "GND", "10": "LCD_MOSI", "20": "LCD_LEDA", "21": "LCD_LEDK", "22": "LCD_LEDK", "23": "LCD_LEDK", "24": "LCD_LEDK",
          "25": "GND", "30": "GND", "MP": "GND",
          "19": "unconnected-(J5-SDO{slash}NC-Pad19)", "26": "unconnected-(J5-TP_RESET(T)-Pad26)",
          "27": "unconnected-(J5-TP_SCL(NC)-Pad27)", "28": "unconnected-(J5-TP_SDA(NC)-Pad28)", "29": "unconnected-(J5-TP_INT(NC)-Pad29)"}
for n in range(11, 19):
    j5nets[str(n)] = "GND"
J5 = place("J5", "ai_calc", PRJ, "FPC_30P_P0.5mm_DualContact_C2919501",
           "LCD 1.9in 170x320 IPS 30P FPC (190-1732TBWPG01 / T-Display-S3 panel)", 165.0, 93.1, 90, "C2919501", j5nets,
           "30-pin 0.5 mm dual-contact FPC socket, 1.0 mm high, front insert / rear flip", "HDGC 0.5K-HX-30PWB")
P = pads(J5)
assert abs(P["1"][0] - 163.8) < 0.05 and abs(P["1"][1] - 85.85) < 0.05, P["1"]
assert abs(P["MP"][0] - 166.2) < 0.05, P["MP"]

SOT = os.path.join(KLIB, "Package_TO_SOT_SMD.pretty")
R04 = os.path.join(KLIB, "Resistor_SMD.pretty"); C04 = os.path.join(KLIB, "Capacitor_SMD.pretty")
Q4 = place("Q4", "Package_TO_SOT_SMD", SOT, "SOT-23", "AO3401A (LCD power switch)", 160.4, 84.1, 0, "C15127",
           {"1": "LCD_PWR_N", "2": "+3V3", "3": "LCD_VDD"}, "P-MOSFET high-side switch: +3V3 -> LCD_VDD when LCD_PWR_N (IO33) is low", "AO3401A")
Q5 = place("Q5", "Package_TO_SOT_SMD", SOT, "SOT-23", "AO3400A (backlight switch)", 158.4, 97.5, 0, "C20917",
           {"1": "LCD_BL_EN", "2": "GND", "3": "LCD_LEDK"}, "N-MOSFET low-side switch for the 4 backlight LEDs, PWM on LCD_BL_EN (IO4)", "AO3400A")
R21 = place("R21", "Resistor_SMD", R04, "R_0402_1005Metric", "100k", 157.6, 84.6, 90, "C25741", {},
            "Gate pull-up: LCD off in deep sleep / before the firmware runs")
R22 = place("R22", "Resistor_SMD", R04, "R_0402_1005Metric", "100k", 158.6, 94.6, 90, "C25741", {},
            "Gate pull-down: backlight off while the ESP32 sleeps or resets")
R23 = place("R23", "Resistor_SMD", R04, "R_0603_1608Metric", "15R", 160.9, 93.9, 0, "C22810", {"1": "+3V3", "2": "LCD_LEDA"},
            "Backlight series resistor: 4 LEDs in parallel, (3.3 V - ~2.8 V) / 22 R = ~20-25 mA total at 100 % PWM")
C36 = place("C36", "Capacitor_SMD", C04, "C_0402_1005Metric", "100nF", 161.4, 86.6, 180, "C1525", {"1": "LCD_VDD", "2": "GND"},
            "LCD VDD decoupling at the FPC socket")
C19 = place("C19", "Capacitor_SMD", C04, "C_0603_1608Metric", "4.7uF", 161.0, 81.4, 180, "C19666", {"1": "LCD_VDD", "2": "GND"},
            "LCD VDD bulk (was the e-paper 3V3 cap)")
for fp, top, bot in ((R21, "LCD_PWR_N", "+3V3"), (R22, "GND", "LCD_BL_EN")):
    ps = sorted(fp.Pads(), key=lambda p: p.GetPosition().y)
    ps[0].SetNet(net(top)); ps[1].SetNet(net(bot))
PQ4, PQ5, PR21, PR22, PR23, PC36, PC19 = pads(Q4), pads(Q5), pads(R21), pads(R22), pads(R23), pads(C36), pads(C19)
assert PC36["1"][0] > PC36["2"][0] and PC19["1"][0] > PC19["2"][0], (PC36, PC19)
r21_top = min(PR21.values(), key=lambda v: v[1]); r21_bot = max(PR21.values(), key=lambda v: v[1])
r22_top = min(PR22.values(), key=lambda v: v[1]); r22_bot = max(PR22.values(), key=lambda v: v[1])
for fp, (rx, ry) in ((Q4, (160.4, 86.0)), (Q5, (158.4, 99.7)), (R21, (156.6, 84.6)), (R22, (157.5, 94.6)), (R23, (160.9, 92.9)),
                     (C36, (161.4, 87.6)), (C19, (161.0, 80.4)), (J5, (165.0, 103.0))):
    fp.Reference().SetPosition(VECTOR2I(mm(rx), mm(ry)))
silk_line(162.2, 85.3, 162.9, 85.3)           # J5 pin 1 (top end of the row)
silk_line(158.4, 82.45, 158.9, 82.45)         # Q4 pin 1 side
silk_line(156.4, 95.75, 156.9, 95.75)         # Q5 pin 1 side

# ------------------------------------------------------------------ bus: shorten the five B.Cu lines at the lane junctions
LANE = {"LCD_RST": 158.1, "LCD_CS": 158.45, "LCD_SCK": 158.8, "LCD_DC": 159.15, "LCD_MOSI": 159.5}
junction = {}
for t in list(b.GetTracks()):
    n = str(t.GetNetname())
    if n in LANE and t.GetClass() == "PCB_TRACK" and t.GetLayer() == Bk:
        x0, y0, x1, y1 = tm(t.GetStart().x), tm(t.GetStart().y), tm(t.GetEnd().x), tm(t.GetEnd().y)
        if abs(x1 - 138.5) < 0.01 and x0 > 160:
            x0, y0, x1, y1 = x1, y1, x0, y0
            t.SetStart(VECTOR2I(mm(x0), mm(y0))); t.SetEnd(VECTOR2I(mm(x1), mm(y1)))
        if abs(x0 - 138.5) < 0.01 and x1 > 160:
            xl = LANE[n]
            yl = y0 + (y1 - y0) * (xl - x0) / (x1 - x0)
            t.SetEnd(VECTOR2I(mm(xl), mm(yl))); junction[n] = (xl, round(yl, 3))
print("junctions", junction)
assert len(junction) == 5, junction
for t in list(b.GetTracks()):
    if str(t.GetNetname()) == "+3V3" and t.GetClass() == "PCB_TRACK":
        s_, e_ = t.GetStart(), t.GetEnd()
        if abs(tm(e_.x) - 157.8) < 0.01 and abs(tm(e_.y) - 97.81) < 0.01:
            t.SetEnd(VECTOR2I(mm(155.6), mm(95.73))); print("+3V3 diagonal shortened")
        elif abs(tm(s_.x) - 157.8) < 0.01 and abs(tm(s_.y) - 97.81) < 0.01:
            t.SetStart(VECTOR2I(mm(155.6), mm(95.73))); print("+3V3 diagonal shortened (start)")

# ------------------------------------------------------------------ SPI: pad -> via -> B.Cu lane -> bus
VIA_COL = {"5": 161.7, "6": 160.95, "7": 160.2, "8": 161.7, "10": 160.95}
PIN_NET = {"5": "LCD_RST", "6": "LCD_CS", "7": "LCD_SCK", "8": "LCD_DC", "10": "LCD_MOSI"}
for pin, n in PIN_NET.items():
    px, py = P[pin]; xv = VIA_COL[pin]; xl = LANE[n]
    track(px, py, xv, py, n, F, 0.15)
    via(xv, py, n)
    track(xv, py, xl, py, n, Bk, 0.15)
    xj, yj = junction[n]
    track(xl, py, xl, yj, n, Bk, 0.15)

# ------------------------------------------------------------------ J5 GND pads: bars + vias (the pour cannot reach 0.3 mm pads)
via(162.45, P["9"][1], "GND"); track(P["9"][0], P["9"][1], 162.45, P["9"][1], "GND", F, 0.15)
track(163.0, P["11"][1], 163.0, P["18"][1], "GND", F, 0.15)
for pin in range(11, 19):
    track(P[str(pin)][0], P[str(pin)][1], 163.0, P[str(pin)][1], "GND", F, 0.15)
via(162.45, 92.6, "GND"); track(163.0, 92.6, 162.45, 92.6, "GND", F, 0.15)
track(163.0, P["25"][1], 163.0, P["30"][1], "GND", F, 0.15)
track(P["25"][0], P["25"][1], 163.0, P["25"][1], "GND", F, 0.15); track(P["30"][0], P["30"][1], 163.0, P["30"][1], "GND", F, 0.15)
via(162.45, 99.1, "GND"); track(163.0, 99.1, 162.45, 99.1, "GND", F, 0.15)
for p_ in J5.Pads():
    if p_.GetNumber() == "MP":
        mx, my = tm(p_.GetPosition().x), tm(p_.GetPosition().y)
        via(167.4, my, "GND"); track(mx, my, 167.4, my, "GND", F, 0.3)
        if my < 93:
            route([P["1"], (164.3, P["1"][1]), (mx, my + 0.3), (mx, my)], "GND", F, 0.15)
        else:
            route([P["30"], (164.3, P["30"][1]), (mx, my - 0.3), (mx, my)], "GND", F, 0.15)

# ------------------------------------------------------------------ LCD_VDD: C19 - Q4.D - C36 - J5 pins 2,3,4 (spine at x 162.6)
route([PC19["1"], (162.6, PC19["1"][1]), (162.6, 87.35)], "LCD_VDD", F, 0.25)
track(PQ4["3"][0], PQ4["3"][1], 162.6, PQ4["3"][1], "LCD_VDD", F, 0.25)
track(PC36["1"][0], PC36["1"][1], 162.6, PC36["1"][1], "LCD_VDD", F, 0.25)
for pin in ("2", "3", "4"):
    track(162.6, P[pin][1], P[pin][0], P[pin][1], "LCD_VDD", F, 0.15)
via(159.3, PC19["2"][1], "GND"); track(PC19["2"][0], PC19["2"][1], 159.3, PC19["2"][1], "GND", F, 0.25)
via(160.3, PC36["2"][1], "GND"); track(PC36["2"][0], PC36["2"][1], 160.3, PC36["2"][1], "GND", F, 0.25)

# ------------------------------------------------------------------ +3V3: diagonal end -> R23.1, and up to Q4.S / R21
route([(155.6, 95.73), (155.6, 93.0), (159.6, 93.0), PR23["1"]], "+3V3", F, 0.25)
route([(156.5, 93.0), (156.5, PQ4["2"][1]), PQ4["2"]], "+3V3", F, 0.25)
track(r21_bot[0], r21_bot[1], r21_bot[0], PQ4["2"][1], "+3V3", F, 0.2)

# ------------------------------------------------------------------ LCD_PWR_N: U1 pad 28 (IO33) via (138.5,94.65) -> B.Cu north + east -> Q4.G / R21
route([(138.5, 94.65), (139.3, 93.85), (139.3, 81.0), (158.2, 81.0), (158.2, 83.15)], "LCD_PWR_N", Bk, 0.15)
via(158.2, 83.15, "LCD_PWR_N")
track(158.2, 83.15, PQ4["1"][0], PQ4["1"][1], "LCD_PWR_N", F, 0.15)
track(158.2, 83.15, r21_top[0], r21_top[1], "LCD_PWR_N", F, 0.15)

# ------------------------------------------------------------------ LCD_BL_EN: U1 pad 8 (IO4) -> F.Cu south, east below the bus comb, via -> B.Cu -> Q5.G / R22
route([(127.55, 104.0), (127.9, 104.5), (127.9, 113.5), (140.5, 113.5)], "LCD_BL_EN", F, 0.15)
via(140.5, 113.5, "LCD_BL_EN")
route([(140.5, 113.5), (160.0, 113.5), (160.0, 96.3)], "LCD_BL_EN", Bk, 0.15)      # below the bus, up east of the lane junctions
via(160.0, 96.3, "LCD_BL_EN")
track(160.0, 96.3, r22_bot[0], 96.3, "LCD_BL_EN", F, 0.15)                           # joins R22's vertical to Q5.G
route([r22_bot, (r22_bot[0], PQ5["1"][1]), PQ5["1"]], "LCD_BL_EN", F, 0.15)
via(157.3, r22_top[1], "GND"); track(r22_top[0], r22_top[1], 157.3, r22_top[1], "GND", F, 0.2)
via(PQ5["2"][0], 99.4, "GND"); track(PQ5["2"][0], PQ5["2"][1], PQ5["2"][0], 99.4, "GND", F, 0.3)

# ------------------------------------------------------------------ backlight: R23.2 -> pin 20 (LEDA); pins 21-24 -> bar -> Q5.D
route([PR23["2"], (162.5, PR23["2"][1]), (162.5, P["20"][1]), P["20"]], "LCD_LEDA", F, 0.15)
track(162.3, P["21"][1], 162.3, P["24"][1], "LCD_LEDK", F, 0.25)
for pin in ("21", "22", "23", "24"):
    track(P[pin][0], P[pin][1], 162.3, P[pin][1], "LCD_LEDK", F, 0.15)
route([(162.3, P["24"][1]), (160.3, P["24"][1]), PQ5["3"]], "LCD_LEDK", F, 0.3)

for (x, y) in ((170.0, 82.0), (175.0, 82.0), (170.0, 104.5), (175.0, 104.5), (174.5, 93.1), (168.5, 93.1)):
    via(x, y, "GND")

KEEP = b.GetLayerID("KEEPOUTS")
r = pcbnew.PCB_SHAPE(b); r.SetShape(pcbnew.SHAPE_T_RECTANGLE); r.SetLayer(KEEP); r.SetWidth(mm(0.1))
r.SetStart(VECTOR2I(mm(166.9), mm(85.0))); r.SetEnd(VECTOR2I(mm(177.4), mm(101.2))); b.Add(r)
t = pcbnew.PCB_TEXT(b); t.SetText("FPC path on F side: keep parts < 1 mm"); t.SetLayer(KEEP)
t.SetPosition(VECTOR2I(mm(172.1), mm(93.1))); t.SetTextSize(VECTOR2I(mm(0.7), mm(0.7))); t.SetTextThickness(mm(0.12)); b.Add(t)

filler = pcbnew.ZONE_FILLER(b); filler.Fill(b.Zones())
pcbnew.SaveBoard(B, b)
print("saved")
