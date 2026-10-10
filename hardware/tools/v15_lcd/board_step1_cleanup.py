# Stage 15 (v15-LCD) board step 1: remove the e-paper socket + SSD1680 booster, the 1 x 14 mm ribbon slot
# and every track that only served them; rename the surviving SPI nets to the LCD names; move the
# slot (1.0 x 20 mm) to x 177.9 for the 30-pin LCD FPC; update the user-layer markers; silk/title.
# Run with KiCad's python:  "C:/Program Files/KiCad/10.0/bin/python.exe" board_step1_cleanup.py
import pcbnew, re
from pcbnew import VECTOR2I, FromMM as mm, ToMM as tm

B = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\kicad_v15_lcd\ai_calc_v15_lcd.kicad_pcb"
b = pcbnew.LoadBoard(B)

# 1. footprints that go (schematic lcd.kicad_sch no longer has them)
GONE = {"J2", "L1", "Q3", "R11", "R12", "D3", "D4", "D5", "C19", "C20", "C21", "C22", "C23", "C24", "C25", "C26", "C27", "C28", "C29", "C30"}
# C19 is re-placed later (it keeps its reference but moves to LCD_VDD), so it is removed here too.
for f in list(b.GetFootprints()):
    if f.GetReference() in GONE:
        b.Delete(f)
print("removed footprints:", sorted(GONE))

# 2. tracks/vias of the booster-only nets, and the e-paper end of the SPI/3V3/BUSY routes
BOOST = {"EPD_GDR", "EPD_RESE", "EPD_VGL", "EPD_VGH", "EPD_SW", "EPD_PUMP", "EPD_VDD", "EPD_VPP", "EPD_VSH",
         "EPD_PREVGH", "EPD_VSL", "EPD_PREVGL", "EPD_VCOM"}
SPI = {"EPD_CLK", "EPD_DIN", "EPD_CS", "EPD_DC", "EPD_RST"}
n = 0
for t in list(b.GetTracks()):
    net = str(t.GetNetname())
    s, e = t.GetStart(), t.GetEnd()
    xs = (tm(s.x), tm(e.x)); ys = (tm(s.y), tm(e.y))
    kill = False
    if net in BOOST:
        kill = True
    elif net in SPI and min(xs) >= 161.5:              # the risers at x 166.4..182.3 and the J2 stubs
        kill = True
    elif net == "EPD_BUSY" and min(xs) >= 139.0:        # the whole B.Cu diagonal to the old J2 (re-routed as LCD_BL_EN)
        kill = True
    elif net == "+3V3" and min(xs) >= 150.0 and max(ys) <= 115 and min(ys) >= 78:   # the e-paper 3V3 feed east of the camera
        kill = True
    if kill:
        b.Delete(t); n += 1
# stale GND stitching vias in the old e-paper area (the new parts/lanes go there; the pour is refilled later)
for t in list(b.GetTracks()):
    if t.GetClass() == "PCB_VIA" and str(t.GetNetname()) == "GND":
        x, y = tm(t.GetStart().x), tm(t.GetStart().y)
        if 156.0 <= x <= 184.0 and 80.0 <= y <= 110.0:
            b.Delete(t); n += 1
print("removed tracks/vias:", n)

# 3. old slot (Edge.Cuts items inside x 172.3..173.5, y 85.7..100) and the matching rule area
for d in list(b.GetDrawings()):
    if d.GetLayerName() == "Edge.Cuts":
        bb = d.GetBoundingBox()
        if tm(bb.GetLeft()) >= 172.3 and tm(bb.GetRight()) <= 173.5 and tm(bb.GetTop()) >= 85.7 and tm(bb.GetBottom()) <= 100.0:
            b.Delete(d)
            print("removed old slot edge", tm(bb.GetLeft()), tm(bb.GetTop()))
for z in list(b.Zones()):
    if z.GetIsRuleArea() and z.GetZoneName() == "edge keep-out":
        bb = z.GetBoundingBox()
        if 172.0 <= tm(bb.GetLeft()) <= 172.2:
            b.Delete(z); print("removed old slot keep-out")

# 4. new slot: 1.0 x 20.0 mm, centre (177.9, 93.1): two lines + two 180-degree arcs (stadium)
SX, SY, SW, SL = 177.9, 93.1, 1.0, 20.0
x0, x1 = SX - SW / 2, SX + SW / 2
ya, yb = SY - SL / 2 + SW / 2, SY + SL / 2 - SW / 2     # arc centres
def seg(xa, ya_, xb, yb_, layer=pcbnew.Edge_Cuts, w=0.1):
    s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_SEGMENT); s.SetLayer(layer)
    s.SetStart(VECTOR2I(mm(xa), mm(ya_))); s.SetEnd(VECTOR2I(mm(xb), mm(yb_))); s.SetWidth(mm(w)); b.Add(s); return s
def arc(cx, cy, r, a_start_deg, a_end_deg, layer=pcbnew.Edge_Cuts, w=0.1):
    """arc from angle a_start to a_end (degrees, KiCad y-down screen angles) CCW on screen"""
    import math
    s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_ARC); s.SetLayer(layer); s.SetWidth(mm(w))
    p0 = VECTOR2I(mm(cx + r * math.cos(math.radians(a_start_deg))), mm(cy + r * math.sin(math.radians(a_start_deg))))
    pm = VECTOR2I(mm(cx + r * math.cos(math.radians((a_start_deg + a_end_deg) / 2))), mm(cy + r * math.sin(math.radians((a_start_deg + a_end_deg) / 2))))
    p1 = VECTOR2I(mm(cx + r * math.cos(math.radians(a_end_deg))), mm(cy + r * math.sin(math.radians(a_end_deg))))
    s.SetArcGeometry(p0, pm, p1); b.Add(s); return s
seg(x0, ya, x0, yb); seg(x1, ya, x1, yb)
arc(SX, ya, SW / 2, 180, 360)   # top cap (y smaller)
arc(SX, yb, SW / 2, 0, 180)     # bottom cap
# keep-out around the slot (copper 0.3 mm away, both layers), same style as before
z = pcbnew.ZONE(b)
z.SetIsRuleArea(True); (z.SetDoNotAllowZoneFills if hasattr(z, 'SetDoNotAllowZoneFills') else z.SetDoNotAllowCopperPour)(True); z.SetDoNotAllowTracks(True); z.SetDoNotAllowVias(True)
z.SetDoNotAllowPads(False); z.SetDoNotAllowFootprints(False)
ls = pcbnew.LSET(); ls.addLayer(pcbnew.F_Cu); ls.addLayer(pcbnew.B_Cu); z.SetLayerSet(ls)
z.SetZoneName("edge keep-out")
pts = [(x0 - 0.35, SY - SL / 2 - 0.35), (x1 + 0.35, SY - SL / 2 - 0.35), (x1 + 0.35, SY + SL / 2 + 0.35), (x0 - 0.35, SY + SL / 2 + 0.35)]
ol = pcbnew.SHAPE_POLY_SET(); ol.NewOutline()
for px, py in pts: ol.Append(mm(px), mm(py))
z.SetOutline(ol); b.Add(z)
print("new slot at", SX, SY)

# 5. user-layer (KEEPOUTS) markers: e-paper outline + active area -> LCD panel outline + active area; camera window 6 -> 7 mm
for d in list(b.GetDrawings()):
    if d.GetLayerName() == "KEEPOUTS":
        bb = d.GetBoundingBox(); L, R, T, Bo = tm(bb.GetLeft()), tm(bb.GetRight()), tm(bb.GetTop()), tm(bb.GetBottom())
        if (abs(L - 120.42) < 0.1 and abs(R - 179.52) < 0.1) or (abs(L - 123.12) < 0.1 and abs(R - 171.77) < 0.1):
            b.Delete(d); print("removed e-paper marker", L, R)
        elif abs(L - 146.93) < 0.1 and abs(R - 153.07) < 0.1:     # camera window circle (was drawn 6.0)
            b.Delete(d); print("removed 6 mm window marker")
def rect(l, t, r, bo, layer, w=0.1):
    s = pcbnew.PCB_SHAPE(b); s.SetShape(pcbnew.SHAPE_T_RECTANGLE); s.SetLayer(layer); s.SetWidth(mm(w))
    s.SetStart(VECTOR2I(mm(l), mm(t))); s.SetEnd(VECTOR2I(mm(r), mm(bo))); b.Add(s); return s
KEEP = b.GetLayerID("KEEPOUTS")
# LCD 190-1732TBWPG01 in landscape: backlight outline 49.72 x 25.8 (glass 48.52 x 24.8), active area 42.72 x 22.70.
# Active area centred on the window centre (150.0, 93.1); the FPC end (+x) has the 2.6 mm driver ledge.
AX0, AY0 = 150.0 - 42.72 / 2, 93.1 - 22.70 / 2
rect(AX0, AY0, AX0 + 42.72, AY0 + 22.70, KEEP)
PX0 = AX0 - 1.6 - 0.6; PX1 = AX0 + 42.72 + 1.6 + 2.6 + 0.6
rect(PX0, 93.1 - 25.8 / 2, PX1, 93.1 + 25.8 / 2, KEEP)
c = pcbnew.PCB_SHAPE(b); c.SetShape(pcbnew.SHAPE_T_CIRCLE); c.SetLayer(KEEP); c.SetWidth(mm(0.1))
c.SetStart(VECTOR2I(mm(150.0), mm(95.1))); c.SetEnd(VECTOR2I(mm(153.5), mm(95.1))); b.Add(c)
def utext(s, x, y, layer, size=1.0):
    t = pcbnew.PCB_TEXT(b); t.SetText(s); t.SetLayer(layer); t.SetPosition(VECTOR2I(mm(x), mm(y)))
    t.SetTextSize(VECTOR2I(mm(size), mm(size))); t.SetTextThickness(mm(0.15)); b.Add(t); return t
utext("LCD 1.9in 170x320 A.A. 42.72x22.70 (B side, faces the window)", 150.0, 79.6, KEEP, 0.8)
utext("LCD outline 49.72x25.8; FPC tail (15.5 wide, 36.6 long) exits +x, through the slot at x 177.9 to J5", 150.0, 107.0, KEEP, 0.8)
utext("camera window 7 mm", 150.0, 99.4, KEEP, 0.8)

# 6. silk + title block
for d in b.GetDrawings():
    if d.GetClass() == "PCB_TEXT" and "AI CALC v14" in d.GetText():
        d.SetText("AI CALC v15-LCD 2026-10-08"); print("silk updated")
tb = b.GetTitleBlock()
tb.SetTitle("AI Calculator main board v15-LCD (fx-115ES shell, 1.9in IPS)")
tb.SetDate("2026-10-08"); tb.SetRevision("v15-LCD")
tb.SetComment(0, "Stage 15: 2.13in e-paper (J2 + SSD1680 booster) replaced by a 1.9in 170x320 IPS LCD (J5, AW9364 backlight, Q4 power switch). See hardware/stage15_lcd.md")
b.SetTitleBlock(tb)

# 7. join the two antenna keep-outs (06 M2): extend each to cover y 85..109 and drop one
ants = [z for z in b.Zones() if z.GetIsRuleArea() and z.GetZoneName().startswith("antenna clearance")]
if len(ants) == 2:
    keep, drop = ants
    ol = pcbnew.SHAPE_POLY_SET(); ol.NewOutline()
    for px, py in [(118.4, 85.0), (119.85, 85.0), (119.85, 109.0), (118.4, 109.0)]:
        ol.Append(mm(px), mm(py))
    keep.SetOutline(ol); keep.SetZoneName("antenna clearance (joined, v15)")
    b.Delete(drop); print("antenna keep-outs joined")

pcbnew.SaveBoard(B, b)

# 8. net renames (text level: unique quoted strings)
s = open(B, encoding="utf-8").read()
REN = {"EPD_CLK": "LCD_RST", "EPD_DIN": "LCD_CS", "EPD_CS": "LCD_SCK", "EPD_DC": "LCD_DC", "EPD_RST": "LCD_MOSI", "EPD_BUSY": "LCD_BL_EN"}
for a, c in REN.items():
    s = s.replace(f'"{a}"', f'"@@{c}"')
s = s.replace('"@@', '"')
open(B, "w", encoding="utf-8").write(s)
print("nets renamed; saved")
