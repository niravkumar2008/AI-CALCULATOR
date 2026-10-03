#!/usr/bin/env python3
"""GND copper pours on both layers + vias that tie GND pads and the two pours together.

usage: add_gnd.py <pcb> [unconnected_report]
 - (re)creates the two GND zones and fills them
 - adds a via beside every GND pad listed as unconnected in the DRC report (or every SMD GND pad
   if no report given), at the first spot with full clearance to other nets
 - adds a coarse grid of stitching vias where both pours exist
"""
import sys, re, math, json
import pcbnew
from shapely.geometry import Point, LineString, Polygon as SP, box
from shapely.ops import unary_union
from shapely.strtree import STRtree

pcb = sys.argv[1]
rpt = sys.argv[2] if len(sys.argv) > 2 else None
b = pcbnew.LoadBoard(pcb)
MM = pcbnew.VECTOR2I_MM
F = json.load(open('/home/claude/pcb/out/hardware/geometry/features.json'))
outline = SP(F['board_outline_mm']).buffer(0)
gnd = b.FindNet('GND')
VIA_D, VIA_DRILL, CLR = 0.6, 0.3, 0.2

# ---- zones
have = {z.GetLayer() for z in b.Zones() if not z.GetIsRuleArea() and z.GetNetname() == 'GND'}
for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
    if layer in have:
        continue
    z = pcbnew.ZONE(b)
    z.SetLayer(layer); z.SetNet(gnd); z.SetZoneName('GND ' + b.GetLayerName(layer))
    z.SetLocalClearance(pcbnew.FromMM(0.25)); z.SetMinThickness(pcbnew.FromMM(0.25))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(pcbnew.FromMM(0.3)); z.SetThermalReliefSpokeWidth(pcbnew.FromMM(0.35))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    ol = z.Outline(); ol.NewOutline()
    xs, ys = outline.exterior.xy
    for x, y in list(zip(xs, ys))[:-1]:
        ol.Append(MM(x, y))
    b.Add(z)


def obstacles():
    """Copper of every net except GND, plus holes and keep-outs, as shapely geometry (mm)."""
    geoms = []
    for t in b.GetTracks():
        if t.GetNetname() == 'GND':
            continue
        w = pcbnew.ToMM(t.GetWidth())
        if t.GetClass() == 'PCB_VIA':
            p = t.GetPosition(); geoms.append(Point(pcbnew.ToMM(p.x), pcbnew.ToMM(p.y)).buffer(w / 2))
        else:
            s, e = t.GetStart(), t.GetEnd()
            geoms.append(LineString([(pcbnew.ToMM(s.x), pcbnew.ToMM(s.y)), (pcbnew.ToMM(e.x), pcbnew.ToMM(e.y))]).buffer(w / 2))
    for fp in b.GetFootprints():
        for pad in fp.Pads():
            if pad.GetNetname() == 'GND' and pad.GetAttribute() != pcbnew.PAD_ATTRIB_NPTH:
                continue
            for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
                if not pad.IsOnLayer(layer) and pad.GetAttribute() != pcbnew.PAD_ATTRIB_NPTH:
                    continue
                ps = pad.GetEffectivePolygon(layer)
                if ps.OutlineCount():
                    o = ps.Outline(0)
                    geoms.append(SP([(pcbnew.ToMM(o.CPoint(i).x), pcbnew.ToMM(o.CPoint(i).y)) for i in range(o.PointCount())]).buffer(0))
            if pad.GetDrillSizeX() > 0:
                p = pad.GetPosition()
                geoms.append(Point(pcbnew.ToMM(p.x), pcbnew.ToMM(p.y)).buffer(pcbnew.ToMM(pad.GetDrillSizeX()) / 2 + 0.05))
    zones = list(b.Zones()) + [z for fp in b.GetFootprints() for z in fp.Zones()]
    for z in zones:
        if z.GetIsRuleArea() and (z.GetDoNotAllowVias() or z.GetDoNotAllowTracks()):
            ol = z.Outline()
            for k in range(ol.OutlineCount()):
                o = ol.Outline(k)
                pts = [(pcbnew.ToMM(o.CPoint(i).x), pcbnew.ToMM(o.CPoint(i).y)) for i in range(o.PointCount())]
                holes = []
                for h in range(ol.HoleCount(k)):
                    hh = ol.Hole(k, h)
                    holes.append([(pcbnew.ToMM(hh.CPoint(i).x), pcbnew.ToMM(hh.CPoint(i).y)) for i in range(hh.PointCount())])
                geoms.append(SP(pts, holes).buffer(0))
    return geoms


obs = obstacles()
# no vias inside (or within 0.4 mm of) a bare key pad
for fp in b.GetFootprints():
    if fp.GetReference().startswith('SW'):
        xs, ys = [], []
        for p in fp.Pads():
            bb = p.GetBoundingBox()
            xs += [pcbnew.ToMM(bb.GetLeft()), pcbnew.ToMM(bb.GetRight())]
            ys += [pcbnew.ToMM(bb.GetTop()), pcbnew.ToMM(bb.GetBottom())]
        obs.append(box(min(xs) - 0.4, min(ys) - 0.4, max(xs) + 0.4, max(ys) + 0.4))
tree = STRtree(obs)
placed = []


def free(x, y):
    c = Point(x, y).buffer(VIA_D / 2 + CLR)
    if not outline.buffer(-0.5).contains(c):
        return False
    for i in tree.query(c):
        if obs[i].intersects(c):
            return False
    for p in placed:
        if p.distance(Point(x, y)) < VIA_D + 0.3:
            return False
    return True


def add_via(x, y):
    v = pcbnew.PCB_VIA(b)
    v.SetPosition(MM(x, y)); v.SetWidth(pcbnew.FromMM(VIA_D)); v.SetDrill(pcbnew.FromMM(VIA_DRILL))
    v.SetNet(gnd); b.Add(v); placed.append(Point(x, y))
    return v


def via_near_pad(pad):
    p = pad.GetPosition(); px, py = pcbnew.ToMM(p.x), pcbnew.ToMM(p.y)
    sx, sy = pcbnew.ToMM(pad.GetBoundingBox().GetWidth()) / 2, pcbnew.ToMM(pad.GetBoundingBox().GetHeight()) / 2
    for extra in (0.55, 0.8, 1.1, 1.5, 2.0):
        for ang in range(0, 360, 30):
            a = math.radians(ang)
            x = px + math.cos(a) * (sx + extra); y = py + math.sin(a) * (sy + extra)
            if free(x, y):
                v = add_via(x, y)
                # short GND track from pad to via (so it's connected even if the pour can't reach)
                t = pcbnew.PCB_TRACK(b); t.SetStart(p); t.SetEnd(MM(x, y)); t.SetWidth(pcbnew.FromMM(0.3))
                t.SetLayer(pcbnew.F_Cu if pad.IsOnLayer(pcbnew.F_Cu) else pcbnew.B_Cu); t.SetNet(gnd)
                tc = LineString([(px, py), (x, y)]).buffer(0.15 + CLR)
                if any(obs[i].intersects(tc) for i in tree.query(tc)):
                    continue_track = False
                else:
                    b.Add(t)
                return True
    return False


targets = set()
if rpt:
    txt = open(rpt).read()
    for blk in re.split(r'\n(?=\[)', txt):
        if blk.startswith('[unconnected') and '[GND]' in blk:
            for m in re.finditer(r'Pad (\S+) \[GND\] of (\S+) on', blk):
                targets.add((m.group(2), m.group(1)))
n_ok = n_fail = 0
for fp in b.GetFootprints():
    for pad in fp.Pads():
        if pad.GetNetname() != 'GND' or pad.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH:
            continue
        if rpt and (fp.GetReference(), pad.GetNumber()) not in targets:
            continue
        if not rpt and pad.GetAttribute() != pcbnew.PAD_ATTRIB_SMD:
            continue
        if via_near_pad(pad):
            n_ok += 1
        else:
            n_fail += 1
            print('no via spot for', fp.GetReference(), pad.GetNumber())

# stitching grid (only where nothing else is)
n_st = 0
if not rpt:
    for x in range(116, 186, 6):
        for y in range(64, 210, 6):
            if free(x, y):
                add_via(x, y); n_st += 1

filler = pcbnew.ZONE_FILLER(b)
filler.Fill(b.Zones())

# every pour island gets at least two vias (one via = a long, thin return path)
n_isl = 0
gvias = [Point(pcbnew.ToMM(t.GetPosition().x), pcbnew.ToMM(t.GetPosition().y))
         for t in b.GetTracks() if t.GetClass() == 'PCB_VIA' and t.GetNetname() == 'GND']
for z in b.Zones():
    if z.GetIsRuleArea() or z.GetNetname() != 'GND':
        continue
    fp_ = z.GetFilledPolysList(z.GetLayer())
    for k in range(fp_.OutlineCount()):
        o = fp_.Outline(k)
        poly = SP([(pcbnew.ToMM(o.CPoint(i).x), pcbnew.ToMM(o.CPoint(i).y)) for i in range(o.PointCount())]).buffer(0)
        if poly.area < 3:
            continue
        have = sum(1 for v in gvias if poly.contains(v))
        if have >= 2:
            continue
        inner = poly.buffer(-0.6)
        if inner.is_empty:
            continue
        x0, y0, x1, y1 = inner.bounds
        need = 2 - have
        yy = y0
        while yy <= y1 and need > 0:
            xx = x0
            while xx <= x1 and need > 0:
                if inner.contains(Point(xx, yy)) and free(xx, yy):
                    add_via(xx, yy); gvias.append(Point(xx, yy)); need -= 1; n_isl += 1
                xx += 0.8
            yy += 0.8
if n_isl:
    filler.Fill(b.Zones())
print('island vias', n_isl)
b.Save(pcb)
print(f'pad vias {n_ok}, failed {n_fail}, stitching {n_st}')
