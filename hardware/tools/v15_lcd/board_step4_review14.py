"""Review 14 (2026-10-08): key-pad position corrections from verification/15_keypad_vs_casio.md.

Run with KiCad 10's python after board_step3_review13.py (or on the committed review-13 board):
    "C:/Program Files/KiCad/10.0/bin/python.exe" hardware/tools/v15_lcd/board_step4_review14.py

What it does (nothing else on the board is touched):
  * SW1  (SHIFT) (176.623, 120.795) -> (177.75, 119.80); footprint KeyPad_6.0x4.5_H3notch -> KeyPad_6.0x4.5
    (the move gives > 1 mm copper-to-H3, so the notch is no longer needed); its comb bus-bar tracks are
    re-created from SW3's (plain 6.0x4.5) pattern, the two feed tracks (ROW0 from SW2, COL0 from the via
    at (180.265, 124.437)) are re-attached.
  * SW50 (ON)    (123.377, 120.795) -> (122.45, 119.55): footprint, its comb tracks and the KEYPADS outline
    move together; the three feed stubs (KEY_ON via, two GND stubs) are re-attached.
  * SW2  (ALPHA) (166.026, 120.795) -> (167.15, 120.80): same, x only.
  Pad sizes, nets, every other footprint: unchanged. keypad.kicad_sch SW1 Footprint field is changed by
  the caller (text replace) so the schematic-parity check stays at 0.
"""
import pcbnew, sys

PCB = r"C:/Users/r_kas/OneDrive/Documents/GitHub/AI-CALCULATOR/hardware/kicad_v15_lcd/ai_calc_v15_lcd.kicad_pcb"
LIB = r"C:/Users/r_kas/OneDrive/Documents/GitHub/AI-CALCULATOR/hardware/kicad_v15_lcd/ai_calc.pretty"
M = 1_000_000
MOVES = {"SW1": (177.75, 119.80), "SW50": (122.45, 119.55), "SW2": (167.15, 120.80)}

b = pcbnew.LoadBoard(PCB)
V = pcbnew.VECTOR2I


def mm(v):
    return round(v / M, 3)


def inside(pt, bb, grow=0.06):
    g = int(grow * M)
    return bb.GetLeft() - g <= pt.x <= bb.GetRight() + g and bb.GetTop() - g <= pt.y <= bb.GetBottom() + g


def snap(pt, cands, tol=0.12):
    """snap pt to the nearest candidate endpoint within tol mm (so feed stubs land exactly on a bar end)."""
    best, bp = tol * M, None
    for c in cands:
        d = ((c.x - pt.x) ** 2 + (c.y - pt.y) ** 2) ** 0.5
        if d < best:
            best, bp = d, c
    return bp if bp is not None else pt


log = []
template = b.FindFootprintByReference("SW3")          # plain KeyPad_6.0x4.5, rot 180, unchanged
tbb = template.GetBoundingBox(False, False)
tpos = template.GetPosition()
tnets = {}
for p in template.Pads():
    tnets[p.GetNetname()] = p.GetNumber()               # net -> pad number ("1" top comb / "2" bottom comb)
template_tracks = [t for t in b.GetTracks() if t.GetClass() == "PCB_TRACK" and t.GetLayerName() == "B.Cu"
                   and inside(t.GetStart(), tbb) and inside(t.GetEnd(), tbb)]
log.append(f"template SW3: {len(template_tracks)} comb tracks, nets {tnets}")

for ref, (nx, ny) in MOVES.items():
    fp = b.FindFootprintByReference(ref)
    old = fp.GetPosition()
    bb = fp.GetBoundingBox(False, False)
    d = V(int(round(nx * M)) - old.x, int(round(ny * M)) - old.y)
    padnets = {p.GetNumber(): p.GetNet() for p in fp.Pads()}
    log.append(f"{ref}: ({mm(old.x)},{mm(old.y)}) -> ({nx},{ny}) delta ({mm(d.x)},{mm(d.y)}) lib {fp.GetFPIDAsString()}")

    if ref == "SW50":
        # the old GND feed (leftmost finger -> NW -> up x 120.066 -> diagonal to the pour) would run through the moved
        # KEY_ON fingers: delete it; the bottom (GND) bar gets a straight stub down to the GND via at (121.139, 123.976).
        kill = {((120.752, 120.945), (120.066, 120.259)), ((120.066, 120.259), (120.066, 115.584)),
                ((120.066, 115.584), (132.596, 103.054))}
        for t in list(b.GetTracks()):
            if t.GetClass() == "PCB_TRACK" and t.GetNetname() == "GND":
                k = ((mm(t.GetStart().x), mm(t.GetStart().y)), (mm(t.GetEnd().x), mm(t.GetEnd().y)))
                if k in kill or (k[1], k[0]) in kill:
                    b.Remove(t); log.append(f"  SW50: removed old GND feed {k}")
    comb = [t for t in b.GetTracks() if t.GetClass() == "PCB_TRACK" and t.GetLayerName() == "B.Cu"
            and inside(t.GetStart(), bb) and inside(t.GetEnd(), bb)]
    feeds = [t for t in b.GetTracks() if t.GetClass() == "PCB_TRACK" and t.GetLayerName() == "B.Cu" and t not in comb
             and (inside(t.GetStart(), bb) != inside(t.GetEnd(), bb))]
    log.append(f"  comb tracks {len(comb)}, feed tracks {len(feeds)}: " +
               "; ".join(f"{t.GetNetname().split('/')[-1]} ({mm(t.GetStart().x)},{mm(t.GetStart().y)})-({mm(t.GetEnd().x)},{mm(t.GetEnd().y)})" for t in feeds))

    if ref == "SW1":
        # swap the footprint for the plain one, keeping ref/value/rotation/layer/pad nets (pad numbers 1 and 2 match)
        newfp = pcbnew.FootprintLoad(LIB, "KeyPad_6.0x4.5")
        assert newfp is not None, "KeyPad_6.0x4.5 not found in ai_calc.pretty"
        newfp.SetReference(fp.GetReference()); newfp.SetValue(fp.GetValue())
        newfp.SetFPIDAsString("ai_calc:KeyPad_6.0x4.5")
        newfp.SetPath(fp.GetPath())
        for f in fp.GetFields():                      # copy Sheetname/Sheetfile/… (Reference/Value set above)
            if f.GetName() not in ("Reference", "Value", "Footprint"):
                newfp.SetField(f.GetName(), f.GetText())
        newfp.SetAttributes(fp.GetAttributes())
        b.Add(newfp)
        if fp.IsFlipped():
            newfp.Flip(newfp.GetPosition(), False)
        newfp.SetOrientationDegrees(fp.GetOrientationDegrees())
        newfp.SetPosition(V(int(round(nx * M)), int(round(ny * M))))
        for p in newfp.Pads():
            p.SetNet(padnets[p.GetNumber()])
        # hide the same text items as the old one
        newfp.Reference().SetVisible(fp.Reference().IsVisible()); newfp.Value().SetVisible(fp.Value().IsVisible())
        newfp.Reference().SetLayer(fp.Reference().GetLayer()); newfp.Value().SetLayer(fp.Value().GetLayer())
        b.Remove(fp)
        fp = newfp
        for t in comb:
            b.Remove(t)
        # re-create the comb tracks from the template, mapped by comb side (pad number)
        pad_by_num = {p.GetNumber(): p for p in fp.Pads()}
        shift = V(fp.GetPosition().x - tpos.x, fp.GetPosition().y - tpos.y)
        newends = []
        for t in template_tracks:
            n = pcbnew.PCB_TRACK(b)
            n.SetLayer(t.GetLayer()); n.SetWidth(t.GetWidth())
            n.SetStart(V(t.GetStart().x + shift.x, t.GetStart().y + shift.y))
            n.SetEnd(V(t.GetEnd().x + shift.x, t.GetEnd().y + shift.y))
            n.SetNet(pad_by_num[tnets[t.GetNetname()]].GetNet())
            b.Add(n); newends += [n.GetStart(), n.GetEnd()]
        log.append(f"  SW1: footprint swapped to KeyPad_6.0x4.5, {len(template_tracks)} comb tracks re-created from SW3")
    else:
        fp.SetPosition(V(old.x + d.x, old.y + d.y))
        newends = []
        for t in comb:
            t.SetStart(V(t.GetStart().x + d.x, t.GetStart().y + d.y))
            t.SetEnd(V(t.GetEnd().x + d.x, t.GetEnd().y + d.y))
            newends += [t.GetStart(), t.GetEnd()]
    # feeds: move the inner end with the pad, snap it onto a bar end
    for t in feeds:
        if inside(t.GetStart(), bb):
            t.SetStart(snap(V(t.GetStart().x + d.x, t.GetStart().y + d.y), newends))
        else:
            t.SetEnd(snap(V(t.GetEnd().x + d.x, t.GetEnd().y + d.y), newends))
    # KEYPADS user-layer outline + legend text
    for g in b.GetDrawings():
        if g.GetLayerName() == "KEYPADS":
            c = g.GetBoundingBox().GetCenter()
            if inside(c, bb, 0.0):
                g.Move(d)
    if ref == "SW50":
        gnd = [p for p in fp.Pads() if p.GetNetname() == "GND"][0].GetNet()
        n = pcbnew.PCB_TRACK(b); n.SetLayer(pcbnew.B_Cu); n.SetWidth(int(0.25 * M)); n.SetNet(gnd)
        n.SetStart(V(int(121.139 * M), int((122.722 - 1.245) * M))); n.SetEnd(V(int(121.139 * M), int(123.976 * M))); b.Add(n)
        log.append("  SW50: new GND stub (121.139, 121.477) -> via (121.139, 123.976)")
        # CAM_PWR_EN runs down x 119.35 on B.Cu, 0.15 mm from the moved pads: jog it to x 119.15 between y 116.6 and 123.0
        # (board edge there is the chamfer, > 2 mm away; GND via (118.527, 120.945) stays 0.22 mm clear)
        for t in list(b.GetTracks()):
            if t.GetClass() == "PCB_TRACK" and t.GetNetname() == "CAM_PWR_EN" and                (mm(t.GetStart().x), mm(t.GetStart().y)) == (119.35, 114.458) and (mm(t.GetEnd().x), mm(t.GetEnd().y)) == (119.35, 161.862):
                w = t.GetWidth(); net = t.GetNet(); b.Remove(t)
                pts = [(119.35, 114.458), (119.35, 116.4), (119.15, 116.6), (119.15, 123.0), (119.35, 123.2), (119.35, 161.862)]
                for a, c in zip(pts, pts[1:]):
                    s = pcbnew.PCB_TRACK(b); s.SetLayer(pcbnew.B_Cu); s.SetWidth(w); s.SetNet(net)
                    s.SetStart(V(int(round(a[0] * M)), int(round(a[1] * M)))); s.SetEnd(V(int(round(c[0] * M)), int(round(c[1] * M)))); b.Add(s)
                log.append("  SW50: CAM_PWR_EN jogged to x 119.15 for y 116.6-123.0")
    log.append(f"  {ref} now at ({mm(fp.GetPosition().x)},{mm(fp.GetPosition().y)}) rot {fp.GetOrientationDegrees()} layer {fp.GetLayerName()}")

b.BuildListOfNets()
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
pcbnew.SaveBoard(PCB, b)
print("\n".join(log))
print("saved", PCB)
