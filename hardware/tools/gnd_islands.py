"""Stage 11: give every GND pour island at least two stitching vias (KiCad python only, no shapely).

usage: python gnd_islands.py <pcb> [min_area_mm2=1.0] [--pads REF.PAD,...]
 - refills the GND zones, finds each island (per layer) with fewer than 2 GND vias,
   and drops vias inside it where the via fits (clearances, keep-outs, key areas respected),
   as far apart from each other as possible.
 - --pads: also put a via right beside these GND pads (short 0.25 mm stub) if none is within 1.2 mm.
"""
import sys, math, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pcbnew, pilroute as pr

mm = pcbnew.ToMM; MM = pcbnew.VECTOR2I_MM
P = sys.argv[1]
MIN_AREA = float(sys.argv[2]) if len(sys.argv) > 2 and not sys.argv[2].startswith('--') else 1.0
PADS = sys.argv[sys.argv.index('--pads') + 1].split(',') if '--pads' in sys.argv else []
b = pcbnew.LoadBoard(P)
gnd = b.FindNet('GND')

def gvias():
    return [t for t in b.GetTracks() if t.GetClass() == 'PCB_VIA' and t.GetNetCode() == gnd.GetNetCode()]

added = 0
# ---- 1. vias beside named GND pads
for spec in PADS:
    ref, num = spec.split('.')
    fp = b.FindFootprintByReference(ref)
    pad = [p for p in fp.Pads() if p.GetNumber() == num][0]
    q = pad.GetPosition(); px, py = mm(q.x), mm(q.y)
    if any(math.hypot(mm(v.GetPosition().x) - px, mm(v.GetPosition().y) - py) < 1.2 for v in gvias()):
        continue
    R = pr.Router(b, (px - 3, py - 3, px + 3, py + 3), G=0.05)
    blk, vb = R.masks(gnd.GetNetCode(), 0.25, 0.15)
    best = None
    for r in range(R.H):
        for c in range(R.W):
            if vb[r, c]: continue
            x, y = R.xy(r, c); d = math.hypot(x - px, y - py)
            if d < 0.6 or d > 1.6: continue
            if not R.free_seg(blk[0], (px, py), (x, y)): continue
            if best is None or d < best[0]: best = (d, x, y)
    if best:
        pr.polyline(b, 'GND', [(px, py), (best[1], best[2])], 0, 0.25, vias=[(best[1], best[2])]); added += 1
        print(f'  pad via {spec} at ({best[1]:.2f},{best[2]:.2f})')
    else:
        print(f'  pad via {spec}: no spot')

# ---- 2. islands with < 2 vias
for rnd in range(3):
    pcbnew.ZONE_FILLER(b).Fill(b.Zones())
    todo = []
    for z in b.Zones():
        if z.GetIsRuleArea() or z.GetNetCode() != gnd.GetNetCode(): continue
        for li, L in enumerate(pr.LAYERS):
            if not z.IsOnLayer(L): continue
            ps = z.GetFilledPolysList(L)
            for i in range(ps.OutlineCount()):
                one = pcbnew.SHAPE_POLY_SET(); one.AddOutline(ps.Outline(i))
                for h in range(ps.HoleCount(i)): one.AddHole(ps.Hole(i, h))
                if one.Area() / 1e12 < MIN_AREA: continue
                nv = [v for v in gvias() if one.Contains(v.GetPosition())]
                if len(nv) < 2: todo.append((li, one, nv))
    if not todo: break
    for li, one, nv in todo:
        bb = one.BBox()
        x0, y0, x1, y1 = mm(bb.GetLeft()), mm(bb.GetTop()), mm(bb.GetRight()), mm(bb.GetBottom())
        R = pr.Router(b, (x0 - 0.5, y0 - 0.5, x1 + 0.5, y1 + 0.5), G=0.1)
        blk, vb = R.masks(gnd.GetNetCode(), 0.25, 0.15)
        have = [(mm(v.GetPosition().x), mm(v.GetPosition().y)) for v in nv]
        cands = []
        for r in range(R.H):
            for c in range(R.W):
                if vb[r, c]: continue
                x, y = R.xy(r, c)
                if all(one.Contains(MM(x + 0.36 * math.cos(a), y + 0.36 * math.sin(a))) for a in [k * math.pi / 4 for k in range(8)]) and one.Contains(MM(x, y)):
                    cands.append((x, y))
        need = 2 - len(have)
        for _ in range(need):
            if not cands: break
            if have:
                x, y = max(cands, key=lambda p: min(math.hypot(p[0] - h[0], p[1] - h[1]) for h in have))
            else:   # first via: the candidate nearest the island's middle
                cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
                x, y = min(cands, key=lambda p: math.hypot(p[0] - cx, p[1] - cy))
            if have and min(math.hypot(x - h[0], y - h[1]) for h in have) < 1.0:
                break
            pr.polyline(b, 'GND', [], vias=[(x, y)]); have.append((x, y)); added += 1
            cands = [p for p in cands if math.hypot(p[0] - x, p[1] - y) > 1.0]
            print(f'  island via {pr.LAYERS[li] and "B" or "F"} ({x:.2f},{y:.2f})')
b.Save(P)
print('GND vias added', added)
