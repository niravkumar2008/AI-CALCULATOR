#!/usr/bin/env python3
"""Post-route cleanup: merge near-duplicate vias, move silkscreen references to free spots, refill.
usage: cleanup.py <pcb>"""
import sys, math
import pcbnew
from shapely.geometry import box, Point

pcb = sys.argv[1]
b = pcbnew.LoadBoard(pcb)
MM = pcbnew.VECTOR2I_MM
mm = pcbnew.ToMM

# ---- 1. near-duplicate vias (same net, holes closer than 0.55 mm edge-to-edge)
vias = [t for t in b.GetTracks() if t.GetClass() == 'PCB_VIA']
ends = {}
for t in b.GetTracks():
    if t.GetClass() != 'PCB_VIA':
        for p in (t.GetStart(), t.GetEnd()):
            ends[(p.x, p.y)] = ends.get((p.x, p.y), 0) + 1
removed = set()
for i, a in enumerate(vias):
    if id(a) in removed:
        continue
    for c in vias[i + 1:]:
        if id(c) in removed or a.GetNetCode() != c.GetNetCode():
            continue
        pa, pc = a.GetPosition(), c.GetPosition()
        d = math.hypot(mm(pa.x - pc.x), mm(pa.y - pc.y))
        if d < mm(a.GetDrill()) + 0.25 + 0.05:
            # keep the one more tracks end on; move those ends onto it
            keep, drop = (a, c) if ends.get((pa.x, pa.y), 0) >= ends.get((pc.x, pc.y), 0) else (c, a)
            kp, dp = keep.GetPosition(), drop.GetPosition()
            for t in b.GetTracks():
                if t.GetClass() == 'PCB_VIA' or t.GetNetCode() != keep.GetNetCode():
                    continue
                if t.GetStart() == dp:
                    t.SetStart(kp)
                if t.GetEnd() == dp:
                    t.SetEnd(kp)
            b.Remove(drop); removed.add(id(drop))
print('merged vias', len(removed))

# ---- 2. silkscreen references: pick a free spot around each visible reference
blockers = []
for fp in b.GetFootprints():
    for pad in fp.Pads():
        bb = pad.GetBoundingBox()
        blockers.append(box(mm(bb.GetLeft()) - 0.15, mm(bb.GetTop()) - 0.15, mm(bb.GetRight()) + 0.15, mm(bb.GetBottom()) + 0.15))
    for it in fp.GraphicalItems():
        if it.GetLayer() == pcbnew.F_SilkS and it.GetClass() != 'PCB_TEXT':
            bb = it.GetBoundingBox()
            blockers.append(box(mm(bb.GetLeft()) - 0.1, mm(bb.GetTop()) - 0.1, mm(bb.GetRight()) + 0.1, mm(bb.GetBottom()) + 0.1))
import json
from shapely.geometry import Polygon as SP
F = json.load(open('/home/claude/pcb/out/hardware/geometry/features.json'))
inside = SP(F['board_outline_mm']).buffer(-0.4)
moved = 0
for fp in b.GetFootprints():
    r = fp.Reference()
    if not r.IsVisible() or fp.GetLayer() != pcbnew.F_Cu:
        continue
    r.SetTextAngleDegrees(0)
    w = 0.8 * 0.7 * len(fp.GetReference()) + 0.2; h = 1.0
    fb = fp.GetBoundingBox(False)
    x0, y0, x1, y1 = mm(fb.GetLeft()), mm(fb.GetTop()), mm(fb.GetRight()), mm(fb.GetBottom())
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    cands = [(cx, y0 - 0.7), (cx, y1 + 0.7), (x0 - w / 2 - 0.3, cy), (x1 + w / 2 + 0.3, cy),
             (x0, y0 - 0.7), (x1, y0 - 0.7), (x0, y1 + 0.7), (x1, y1 + 0.7)]
    for (x, y) in cands:
        g = box(x - w / 2, y - h / 2, x + w / 2, y + h / 2)
        if inside.contains(g) and not any(g.intersects(o) for o in blockers):
            r.SetPosition(MM(x, y)); blockers.append(g); moved += 1
            break
    else:
        r.SetVisible(False)
        print('no silk room for', fp.GetReference(), '(hidden; still on the fab drawing)')
print('refs placed', moved)

pcbnew.ZONE_FILLER(b).Fill(b.Zones())
b.Save(pcb)
