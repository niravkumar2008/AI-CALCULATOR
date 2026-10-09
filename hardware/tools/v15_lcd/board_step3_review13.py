# Stage 15 (v15-LCD) board step 3, written by verification review 13 (2026-10-08). Run with KiCad 10's python
# AFTER board_step2_route.py. It applies the CAD requests from enclosure/final_assembly_v15_lcd/REPORT.md
# and the pin-1 finding of hardware/verification/13_v15_lcd_review.md:
#   1. ribbon slot moved outwards: x 177.4-178.4 -> 180.5-181.5 (centre 181.0), y unchanged (83.1-103.1)
#   2. J5 moved 6.4 mm west: (165.0, 93.1) -> (158.6, 93.1), rot 90, body x 156.73-160.53 (east of 156.5 = camera
#      module keep-out), mouth at x 160.53 facing the slot; route length slot->J5 grows by ~9.5 mm
#   3. J5 footprint = FPC_30P_P0.5mm_DualContact_C2919501_LcdReversed (pad N at the position of contact 31-N):
#      with the vendor drawing's finger numbering (30..1 left-to-right on the display face) finger 1 arrives at
#      the LARGE-y end of the socket after the tail folds through the slot, so pad 1 must be at the bottom.
#      Because the five B.Cu SPI lanes keep their west->east order (IO5, IO6, IO8, IO41, IO42) and now meet the
#      pads in the opposite order, the GPIO ROLES are swapped in the schematic (mcu.kicad_sch labels):
#      IO5 = LCD_MOSI, IO6 = LCD_DC, IO8 = LCD_SCK (unchanged), IO41 = LCD_CS, IO42 = LCD_RST. The copper from
#      U1 to the bus junctions is unchanged; only its net names change.
#   4. Q4/Q5/R21-R23/C19/C36 re-placed in two bands north (backlight: Q5, R22, R23) and south (power: Q4, R21,
#      C19, C36) of the socket, everything re-routed, the strip from the mouth to the slot free of parts.
#   5. J5 3D model installed (ai_calc.3dshapes/FPC_30P_HDGC_0.5K-HX-30PWB.step from easyeda2kicad C2919501).
#   6. a track/via keep-out ring around the new slot (as v14 had around its slot).
# Coordinates in mm, board frame. Idempotent only in the sense that it rebuilds the whole LCD cluster.
import os, sys, math
import pcbnew
from pcbnew import VECTOR2I, FromMM as mm, ToMM as tm

B = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\kicad_v15_lcd\ai_calc_v15_lcd.kicad_pcb"
PROJ = os.path.dirname(B)
KLIB = r"C:\Program Files\KiCad\10.0\share\kicad\footprints"
b = pcbnew.LoadBoard(B)
F, Bk = pcbnew.F_Cu, pcbnew.B_Cu
FAB = pcbnew.F_Fab
CLR = 0.15  # Default netclass clearance; the 0.5 mm pitch escapes need it


def net(name):
    n = b.FindNet(name)
    if n is None:
        n = pcbnew.NETINFO_ITEM(b, name); b.Add(n)
    return n


def place(ref, libnick, libdir, name, value, x, y, rot, lcsc, nets, descr="", mpn="", fpdescr=""):
    fp = pcbnew.FootprintLoad(libdir, name)
    fp.SetFPID(pcbnew.LIB_ID(libnick, name))
    fp.SetReference(ref); fp.SetValue(value)
    fp.SetPosition(VECTOR2I(mm(x), mm(y))); fp.SetOrientationDegrees(rot); fp.SetLayer(F)
    fp.SetField("LCSC", lcsc)
    if descr:
        fp.SetField("Description", descr)
    if mpn:
        fp.SetField("MPN", mpn)
    if fpdescr:
        fp.SetLibDescription(fpdescr)
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


log = []
for d in list(b.GetDrawings()):
    if d.GetLayer() == pcbnew.F_SilkS and d.GetClass() == "PCB_SHAPE":
        x, y = tm(d.GetStart().x), tm(d.GetStart().y)
        if 155.5 <= x <= 163.5 and 81.0 <= y <= 96.5:
            b.Delete(d)
# ------------------------------------------------------------------ 0. remove the old cluster
for ref in ("J5", "Q4", "Q5", "R21", "R22", "R23", "C19", "C36"):
    fp = b.FindFootprintByReference(ref)
    assert fp is not None, ref
    b.Delete(fp)
log.append("removed footprints J5 Q4 Q5 R21 R22 R23 C19 C36 (re-placed below)")

SPI = ("LCD_RST", "LCD_CS", "LCD_SCK", "LCD_DC", "LCD_MOSI")
ndel = 0
for t in list(b.GetTracks()):
    n = str(t.GetNetname()); s_, e_ = t.GetStart(), t.GetEnd()
    xs = (tm(s_.x), tm(e_.x)); ys = (tm(s_.y), tm(e_.y))
    isvia = t.GetClass() == "PCB_VIA"
    kill = False
    if n in SPI and min(xs) > 150.0 and min(ys) < 108.9:
        kill = True                                   # F.Cu escapes, vias, B.Cu lanes (the bus diagonals start at x 138.5)
    elif n == "GND" and min(xs) >= 155.0 and max(xs) <= 184.0 and min(ys) >= 79.0 and max(ys) <= 111.0:
        kill = True                                   # socket GND bars, cluster GND vias, strip stitch vias (re-added)
    elif n in ("LCD_VDD", "LCD_LEDA", "LCD_LEDK"):
        kill = True
    elif n == "+3V3" and min(xs) >= 155.0 and max(ys) <= 100.0:
        kill = True                                   # the pieces east of the diagonal
    elif n == "LCD_PWR_N" and (max(xs) > 139.0 and not (isvia and abs(xs[0] - 138.5) < 0.01)) and max(xs) < 170:
        kill = True                                   # B.Cu run along y 81 and the Q4 end; the U1 stub + via (138.5, 94.65) stay
    elif n == "LCD_BL_EN" and max(xs) > 150.0:
        kill = True                                   # B.Cu y 113.5 run east of x 150 is re-made (keeps the via 140.5,113.5)
    if kill:
        b.Delete(t); ndel += 1
log.append("deleted %d tracks/vias of the old cluster" % ndel)
# the +3V3 diagonal (143.31,84.13)-(155.60,95.73) is shortened to end at (153.48, 94.30), still on its line y = x - 59.18
for t in b.GetTracks():
    if str(t.GetNetname()) == "+3V3" and t.GetClass() == "PCB_TRACK":
        s_, e_ = t.GetStart(), t.GetEnd()
        if abs(tm(e_.x) - 155.6) < 0.01 and abs(tm(e_.y) - 95.73) < 0.01:
            t.SetEnd(VECTOR2I(mm(153.48), mm(94.30))); log.append("+3V3 diagonal shortened to (153.48, 94.30)")
        elif abs(tm(s_.x) - 155.6) < 0.01 and abs(tm(s_.y) - 95.73) < 0.01:
            t.SetStart(VECTOR2I(mm(153.48), mm(94.30))); log.append("+3V3 diagonal shortened to (153.48, 94.30)")
# the LCD_BL_EN B.Cu run (140.5,113.5)->(160,113.5) was deleted above only if it reached x>150: it did. Re-made below.

# ------------------------------------------------------------------ 1. slot: x +3.1
EDGE = pcbnew.Edge_Cuts
nmoved = 0
for d in b.GetDrawings():
    if d.GetLayer() == EDGE and d.GetClass() == "PCB_SHAPE":
        bb = d.GetBoundingBox(); x0, x1 = tm(bb.GetLeft()), tm(bb.GetRight())
        if 177.2 <= x0 and x1 <= 178.6 and tm(bb.GetBottom()) < 110:
            d.Move(VECTOR2I(mm(3.1), 0)); nmoved += 1
assert nmoved == 4, nmoved
log.append("slot moved +3.1 mm: now x 180.5-181.5, y 83.1-103.1 (2.15 mm to the board edge at x 183.65)")
# keep-out ring 0.35 mm around the slot for tracks and vias (the copper-to-edge rule already enforces 0.30 mm)
try:
    z = pcbnew.ZONE(b)
    z.SetIsRuleArea(True); z.SetDoNotAllowTracks(True); z.SetDoNotAllowVias(True)
    z.SetDoNotAllowPads(False); z.SetDoNotAllowFootprints(False)
    (getattr(z, 'SetDoNotAllowZoneFills', None) or getattr(z, 'SetDoNotAllowCopperPour'))(False)
    ls = pcbnew.LSET(); ls.AddLayer(F); ls.AddLayer(Bk); z.SetLayerSet(ls)
    z.SetZoneName("slot keep-out (v15 review 13)")
    pts = []
    cx, r = 181.0, 0.85
    for i in range(17):  # bottom arc then top arc
        a = math.pi * i / 16; pts.append((cx + r * math.cos(a), 102.6 + r * math.sin(a)))
    for i in range(17):
        a = math.pi + math.pi * i / 16; pts.append((cx + r * math.cos(a), 83.6 + r * math.sin(a)))
    z.Outline().NewOutline()
    for (x, y) in pts:
        z.AppendCorner(VECTOR2I(mm(x), mm(y)), -1)
    b.Add(z); log.append("slot keep-out ring added (tracks/vias, F+B, 0.35 mm around the slot)")
except Exception as ex:
    log.append("slot keep-out ring NOT added (API: %s); the 0.3 mm copper-to-edge rule still applies" % ex)

# user-layer markers
KEEP = b.GetLayerID("KEEPOUTS")
for d in list(b.GetDrawings()):
    if d.GetLayer() != KEEP:
        continue
    if d.GetClass() == "PCB_SHAPE" and d.GetShape() == pcbnew.SHAPE_T_RECTANGLE:
        bb = d.GetBoundingBox()
        if abs(tm(bb.GetLeft()) - 166.9) < 0.1 and abs(tm(bb.GetRight()) - 177.4) < 0.1:
            d.SetStart(VECTOR2I(mm(160.6), mm(85.0))); d.SetEnd(VECTOR2I(mm(180.5), mm(101.2)))
            log.append("KEEPOUTS strip rectangle now x 160.6-180.5 (mouth to slot)")
    if d.GetClass() == "PCB_TEXT":
        s = d.GetText()
        if "FPC path on F side" in s:
            d.SetPosition(VECTOR2I(mm(170.5), mm(93.1)))
        if "slot at x 177.9" in s:
            d.SetText(s.replace("slot at x 177.9 to J5", "slot at x 181.0, folds back west to J5 at x 158.6 (pad 1 at the bottom, y 100.35)"))
            log.append("KEEPOUTS text updated")

# ------------------------------------------------------------------ 2. J5 (mirrored numbering) at (158.6, 93.1)
PRJ = os.path.join(PROJ, "ai_calc.pretty")
j5nets = {"1": "GND", "2": "LCD_VDD", "3": "LCD_VDD", "4": "LCD_VDD", "5": "LCD_RST", "6": "LCD_CS", "7": "LCD_SCK", "8": "LCD_DC",
          "9": "GND", "10": "LCD_MOSI", "20": "LCD_LEDA", "21": "LCD_LEDK", "22": "LCD_LEDK", "23": "LCD_LEDK", "24": "LCD_LEDK",
          "25": "GND", "30": "GND", "MP": "GND",
          "19": "unconnected-(J5-SDO{slash}NC-Pad19)", "26": "unconnected-(J5-TP_RESET(T)-Pad26)",
          "27": "unconnected-(J5-TP_SCL(NC)-Pad27)", "28": "unconnected-(J5-TP_SDA(NC)-Pad28)", "29": "unconnected-(J5-TP_INT(NC)-Pad29)"}
for n in range(11, 19):
    j5nets[str(n)] = "GND"
J5 = place("J5", "ai_calc", PRJ, "FPC_30P_P0.5mm_DualContact_C2919501_LcdReversed",
           "LCD 1.9in 170x320 IPS 30P FPC (190-1732TBWPG01 / T-Display-S3 panel)", 158.6, 93.1, 90, "C2919501", j5nets,
           "30-pin 0.5 mm dual-contact FPC socket, 1.0 mm high, front insert / rear flip",
           "HDGC 0.5K-HX-30PWB",
           "HDGC 0.5K-HX-30PWB (LCSC C2919501) EasyEDA footprint, pad numbers mirrored (pad N = socket contact 31-N): review 13, 2026-10-08. Same geometry as FPC_30P_P0.5mm_DualContact_C2919501.")
P = pads(J5)
assert abs(P["1"][0] - 157.4) < 0.05 and abs(P["1"][1] - 100.35) < 0.05, P["1"]
assert abs(P["30"][1] - 85.85) < 0.05 and abs(P["10"][1] - 95.85) < 0.05, (P["30"], P["10"])
MPs = sorted([(tm(p.GetPosition().x), tm(p.GetPosition().y)) for p in J5.Pads() if p.GetNumber() == "MP"], key=lambda v: v[1])
assert abs(MPs[0][0] - 159.8) < 0.05 and abs(MPs[0][1] - 84.85) < 0.05, MPs
log.append("J5 placed at (158.6, 93.1) rot 90 with FPC_30P_P0.5mm_DualContact_C2919501_LcdReversed: pad 1 (157.4, 100.35), pad 30 (157.4, 85.85), tabs (159.8, 84.85/101.35), body x 156.73-160.53")

SOT = os.path.join(KLIB, "Package_TO_SOT_SMD.pretty")
R04 = os.path.join(KLIB, "Resistor_SMD.pretty"); C04 = os.path.join(KLIB, "Capacitor_SMD.pretty")
# south band (power): Q4 rot 90 (D at top centre), C36 / C19 rot 270 (pad 1 = LCD_VDD on top), R21 rot 180 (pad 1 = +3V3 east)
Q4 = place("Q4", "Package_TO_SOT_SMD", SOT, "SOT-23", "AO3401A (LCD power switch)", 159.5, 103.9, 90, "C15127",
           {"1": "LCD_PWR_N", "2": "+3V3", "3": "LCD_VDD"}, "P-MOSFET high-side switch: +3V3 -> LCD_VDD when LCD_PWR_N (IO33) is low", "AO3401A")
C36 = place("C36", "Capacitor_SMD", C04, "C_0402_1005Metric", "100nF", 157.3, 102.8, 270, "C1525", {"1": "LCD_VDD", "2": "GND"},
            "LCD VDD decoupling at the FPC socket")
C19 = place("C19", "Capacitor_SMD", C04, "C_0603_1608Metric", "4.7uF", 155.6, 103.6, 270, "C19666", {"1": "LCD_VDD", "2": "GND"},
            "LCD VDD bulk (was the e-paper 3V3 cap)")
R21 = place("R21", "Resistor_SMD", R04, "R_0402_1005Metric", "100k", 159.5, 106.3, 180, "C25741", {"1": "+3V3", "2": "LCD_PWR_N"},
            "Gate pull-up: LCD off in deep sleep / before the firmware runs")
# north band (backlight): Q5 rot 270 (D at bottom centre), R22 rot 180 (pad 1 = LCD_BL_EN east), R23 rot 180 (pad 1 = +3V3 east)
Q5 = place("Q5", "Package_TO_SOT_SMD", SOT, "SOT-23", "AO3400A (backlight switch)", 157.3, 82.3, 270, "C20917",
           {"1": "LCD_BL_EN", "2": "GND", "3": "LCD_LEDK"}, "N-MOSFET low-side switch for the 4 backlight LEDs, PWM on LCD_BL_EN (IO4)", "AO3400A")
R22 = place("R22", "Resistor_SMD", R04, "R_0402_1005Metric", "100k", 156.9, 79.6, 180, "C25741", {"1": "LCD_BL_EN", "2": "GND"},
            "Gate pull-down: backlight off while the ESP32 sleeps or resets")
R23 = place("R23", "Resistor_SMD", R04, "R_0603_1608Metric", "15R", 161.0, 80.6, 180, "C22810", {"1": "+3V3", "2": "LCD_LEDA"},
            "Backlight series resistor: 4 LEDs in parallel, (3.3 V - ~2.8 V) / 22 R = ~20-25 mA total at 100 % PWM")
PQ4, PQ5, PR21, PR22, PR23, PC36, PC19 = pads(Q4), pads(Q5), pads(R21), pads(R22), pads(R23), pads(C36), pads(C19)
assert abs(PQ4["3"][0] - 159.5) < 0.05 and abs(PQ4["3"][1] - 102.96) < 0.05, PQ4
assert abs(PQ4["1"][0] - 158.55) < 0.05 and abs(PQ4["2"][0] - 160.45) < 0.05, PQ4
assert abs(PQ5["3"][0] - 157.3) < 0.05 and abs(PQ5["3"][1] - 83.24) < 0.05 and abs(PQ5["1"][0] - 158.25) < 0.05, PQ5
assert PC36["1"][1] < PC36["2"][1] and PC19["1"][1] < PC19["2"][1], (PC36, PC19)       # pad 1 (LCD_VDD) on top
assert PR21["1"][0] > PR21["2"][0] and PR22["1"][0] > PR22["2"][0] and PR23["1"][0] > PR23["2"][0], (PR21, PR22, PR23)
for fp, (rx, ry) in ((Q4, (159.5, 101.9)), (Q5, (157.3, 84.2)), (R21, (159.5, 107.3)), (R22, (156.9, 78.9)), (R23, (161.0, 81.7)),
                     (C36, (157.3, 104.0)), (C19, (155.6, 105.6)), (J5, (158.6, 103.0))):
    fp.Reference().SetPosition(VECTOR2I(mm(rx), mm(ry)))
log.append("placed Q4 (159.5,103.9) r90, C36 (157.3,102.8) r270, C19 (155.6,103.6) r270, R21 (159.5,106.3) r180 [south band]; "
           "Q5 (157.3,82.3) r270, R22 (156.9,79.6) r180, R23 (161.0,80.6) r180 [north band]")
silk_line(155.5, 100.35, 156.1, 100.35)       # J5 pin 1 = bottom end of the row
silk_line(157.5, 105.95, 157.9, 105.95)       # Q4 pin 1 (G, bottom-left) side
silk_line(158.6, 80.6, 159.0, 80.6)           # Q5 pin 1 (G, top-right) side

# ------------------------------------------------------------------ 3. SPI: west escapes, vias, B.Cu lanes to the bus
# lanes west->east = U1 IO5, IO6, IO8, IO41, IO42 (bus order unchanged); nets after the schematic swap:
LANE = {"LCD_MOSI": 153.0, "LCD_DC": 153.35, "LCD_SCK": 153.7, "LCD_CS": 154.05, "LCD_RST": 154.4}
PIN_NET = {"10": "LCD_MOSI", "8": "LCD_DC", "7": "LCD_SCK", "6": "LCD_CS", "5": "LCD_RST"}
VIA_COL = {"10": 154.9, "8": 155.6, "7": 156.3, "6": 154.9, "5": 155.6}
# rename the bus copper (U1 -> junction) first: old LCD_RST(IO5)->LCD_MOSI, LCD_CS(IO6)->LCD_DC, LCD_DC(IO41)->LCD_CS, LCD_MOSI(IO42)->LCD_RST
SWAP = {"LCD_RST": "LCD_MOSI", "LCD_CS": "LCD_DC", "LCD_DC": "LCD_CS", "LCD_MOSI": "LCD_RST"}
items = [t for t in b.GetTracks() if str(t.GetNetname()) in SWAP]
for t in items:
    t.SetNet(net(SWAP[str(t.GetNetname())]))
U1 = b.FindFootprintByReference("U1")
for p_ in U1.Pads():
    if str(p_.GetNetname()) in SWAP and p_.GetNumber() in ("9", "10", "37", "38"):
        p_.SetNet(net(SWAP[str(p_.GetNetname())]))
log.append("renamed %d bus tracks/vias and U1 pads 9/10/37/38: IO5=LCD_MOSI IO6=LCD_DC IO41=LCD_CS IO42=LCD_RST" % len(items))
junction = {}
for t in list(b.GetTracks()):
    n = str(t.GetNetname())
    if n in LANE and t.GetClass() == "PCB_TRACK" and t.GetLayer() == Bk:
        x0, y0, x1, y1 = tm(t.GetStart().x), tm(t.GetStart().y), tm(t.GetEnd().x), tm(t.GetEnd().y)
        if abs(x1 - 138.5) < 0.01 and x0 > 150:
            x0, y0, x1, y1 = x1, y1, x0, y0
            t.SetStart(VECTOR2I(mm(x0), mm(y0))); t.SetEnd(VECTOR2I(mm(x1), mm(y1)))
        if abs(x0 - 138.5) < 0.01 and x1 > 150:
            xl = LANE[n]
            yl = y0 + (y1 - y0) * (xl - x0) / (x1 - x0)
            t.SetEnd(VECTOR2I(mm(xl), mm(yl))); junction[n] = (xl, round(yl, 3))
assert len(junction) == 5, junction
log.append("bus cut at the new junctions: %s" % junction)
for pin, n in PIN_NET.items():
    px, py = P[pin]; xv = VIA_COL[pin]; xl = LANE[n]
    track(px, py, xv, py, n, F, 0.15)
    via(xv, py, n)
    track(xv, py, xl, py, n, Bk, 0.15)
    xj, yj = junction[n]
    track(xl, py, xl, yj, n, Bk, 0.15)

# ------------------------------------------------------------------ 4. GND at the socket
# pads 11-18: bar at x 158.0 under the housing, via at (160.1, 93.6); pad 9 own via east; pad 1 own via east;
# pads 25 and 30: own vias west (pads 26-29 between them are open); tabs: vias east at x 161.0
track(158.0, P["11"][1], 158.0, P["18"][1], "GND", F, 0.15)
for pin in range(11, 19):
    track(P[str(pin)][0], P[str(pin)][1], 158.0, P[str(pin)][1], "GND", F, 0.15)
via(160.1, 93.6, "GND"); track(158.0, 93.6, 160.1, 93.6, "GND", F, 0.15)
via(158.5, P["9"][1], "GND"); track(P["9"][0], P["9"][1], 158.5, P["9"][1], "GND", F, 0.15)
via(158.0, P["1"][1], "GND"); track(P["1"][0], P["1"][1], 158.0, P["1"][1], "GND", F, 0.15)
for pin in ("25", "30"):
    via(156.5, P[pin][1], "GND"); track(P[pin][0], P[pin][1], 156.5, P[pin][1], "GND", F, 0.15)
for (mx, my) in MPs:
    via(161.0, my, "GND"); track(mx, my, 161.0, my, "GND", F, 0.3)

# ------------------------------------------------------------------ 5. LCD_VDD: pads 2-4 -> column x 159.0 -> C36 -> Q4.D ; C19
for pin in ("2", "3", "4"):
    track(P[pin][0], P[pin][1], 159.0, P[pin][1], "LCD_VDD", F, 0.15)
route([(159.0, P["4"][1]), (159.0, PC36["1"][1]), PQ4["3"]], "LCD_VDD", F, 0.25)
track(PC19["1"][0], PC19["1"][1], PC36["1"][0], PC36["1"][1], "LCD_VDD", F, 0.25)     # C19.1 -> C36.1
track(PC36["1"][0], PC36["1"][1], 159.0, PC36["1"][1], "LCD_VDD", F, 0.25)
via(155.4, 105.1, "GND"); track(PC19["2"][0], PC19["2"][1], 155.4, 105.1, "GND", F, 0.25)
via(156.9, 104.3, "GND"); track(PC36["2"][0], PC36["2"][1], 156.9, 104.3, "GND", F, 0.25)

# ------------------------------------------------------------------ 6. +3V3: diagonal end -> via -> B.Cu y 94.3 -> via -> F.Cu column x 161.6
via(153.48, 94.30, "+3V3")
track(153.48, 94.30, 161.6, 94.30, "+3V3", Bk, 0.25)
via(161.6, 94.30, "+3V3")
route([PR23["1"], (161.6, PR23["1"][1]), (161.6, PQ4["2"][1]), PQ4["2"]], "+3V3", F, 0.25)
route([PQ4["2"], PR21["1"]], "+3V3", F, 0.2)

# ------------------------------------------------------------------ 7. LCD_PWR_N: via (138.5,94.65) -> B.Cu y 94.75 east -> x 157.4 south -> via -> R21.2 / Q4.G
route([(138.5, 94.65), (139.0, 94.85), (157.4, 94.85), (157.4, 106.6)], "LCD_PWR_N", Bk, 0.15)
via(157.4, 106.6, "LCD_PWR_N")
track(157.4, 106.6, PR21["2"][0], PR21["2"][1], "LCD_PWR_N", F, 0.15)
route([PQ4["1"], PR21["2"]], "LCD_PWR_N", F, 0.15)

# ------------------------------------------------------------------ 8. LCD_BL_EN: via (140.5,113.5) -> B.Cu y 113.5 -> x 162.2 north -> y 79.5 west -> via -> Q5.G / R22.1
route([(140.5, 113.5), (162.2, 113.5), (162.2, 79.5), (158.3, 79.5)], "LCD_BL_EN", Bk, 0.15)
via(158.3, 79.5, "LCD_BL_EN")
track(158.3, 79.5, PQ5["1"][0], PQ5["1"][1], "LCD_BL_EN", F, 0.15)
track(158.3, 79.5, PR22["1"][0], PR22["1"][1], "LCD_BL_EN", F, 0.15)
route([PR22["2"], PQ5["2"]], "GND", F, 0.2)
via(155.6, PQ5["2"][1], "GND"); track(PQ5["2"][0], PQ5["2"][1], 155.6, PQ5["2"][1], "GND", F, 0.25)

# ------------------------------------------------------------------ 9. backlight: LEDA pad 20 -> x 159.0 north -> R23.2 ; LEDK pads 21-24 -> bar x 158.5 north -> Q5.D
route([P["20"], (159.0, P["20"][1]), (159.0, PR23["2"][1]), PR23["2"]], "LCD_LEDA", F, 0.15)
track(158.5, P["21"][1], 158.5, P["24"][1], "LCD_LEDK", F, 0.25)
for pin in ("21", "22", "23", "24"):
    track(P[pin][0], P[pin][1], 158.5, P[pin][1], "LCD_LEDK", F, 0.15)
route([(158.5, P["24"][1]), (158.5, 83.6), PQ5["3"]], "LCD_LEDK", F, 0.3)

# ------------------------------------------------------------------ 10. GND stitching in the strip / bay
for (x, y) in ((170.0, 82.0), (175.0, 82.0), (170.0, 104.5), (175.0, 104.5), (174.5, 93.1), (168.5, 93.1), (155.2, 107.4),
               (163.5, 82.0), (163.5, 103.5), (161.6, 107.5)):
    via(x, y, "GND")

filler = pcbnew.ZONE_FILLER(b); filler.Fill(b.Zones())
pcbnew.SaveBoard(B, b)
print("\n".join(log)); print("saved")
