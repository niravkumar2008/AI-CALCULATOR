#!/usr/bin/env python3
"""Placement check + picture.

Draws every F-side courtyard, the keep-outs that come from the case, and the outline.
Prints courtyard overlaps, parts outside the board and parts inside a keep-out.
Run: python3 placement_check.py <pcb> <out.png>
"""
import sys, json, math
import pcbnew
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle, Polygon
from shapely.geometry import Polygon as SPoly, Point, box
from shapely.ops import unary_union

pcb = sys.argv[1]; out = sys.argv[2]
F = json.load(open('/home/claude/pcb/out/hardware/geometry/features.json'))
b = pcbnew.LoadBoard(pcb)

outline = SPoly(F['board_outline_mm']).buffer(0)
# keep-outs on the F side (component side), each (name, shapely geom, hard?)
KO = []
for i, p in enumerate(F['back_cover_pins']):
    KO.append((f'back-cover pin {i+1}', Point(p['at']).buffer(p['d'] / 2 + 0.5), True))
cam = F['camera']['center']
KO.append(('camera module', box(cam[0] - 6, cam[1] - 6, cam[0] + 6, cam[1] + 6), True))
KO.append(('camera FPC corridor', box(cam[0] - 3.5, cam[1] + 4.25, cam[0] + 3.5, 157.4), True))
for h in F['holes']:
    KO.append((f"{h['kind']} {h['at']}", Point(h['at']).buffer(h['d'] / 2 + 0.6), True))
for n in F['notches']:
    KO.append((n['kind'], Point(n['at']).buffer(n['d'] / 2 + 0.6), True))
ms = F['magnet_slot']; mx, my = ms['center']; mw, mh = ms['size']
KO.append(('magnet connector body', box(mx - mw / 2, my - mh / 2, mx + mw / 2, my + mh / 2), True))
# antenna keep-out of the module itself is in the footprint; add the board-edge strip
KO_SOFT = [('back-cover ribs (keep F side flat)', unary_union([box(110, y - 1.0, 190, y + 1.0) for y in (172.5, 184.0, 195.5)]))]

fig, ax = plt.subplots(figsize=(9, 18))
ax.add_patch(Polygon(list(outline.exterior.coords), closed=True, fc='#1f5c45', ec='k', lw=1))
ep = F['epaper']; ex, ey = ep['center']; ew, eh = ep['outline']
ax.add_patch(Rectangle((ex - ew / 2, ey - eh / 2), ew, eh, fc='none', ec='#c9a227', ls='--', lw=1))
ax.text(ex, ey + eh / 2 + 1.5, 'e-paper panel (B side)', color='#c9a227', ha='center', fontsize=7)
bt = F['battery']['rect']
ax.add_patch(Rectangle((bt[0], bt[1]), bt[2], bt[3], fc='#5577aa', ec='k', alpha=.6))
for name, g, hard in KO:
    xs, ys = g.exterior.xy
    ax.fill(xs, ys, fc='#ff5555' if hard else '#ffaa55', alpha=.35, ec='#aa0000', lw=.5)
for name, g in KO_SOFT:
    for gg in getattr(g, 'geoms', [g]):
        xs, ys = gg.exterior.xy
        ax.fill(xs, ys, fc='#ff99ff', alpha=.25, ec='none')

problems = []
cy = {}
for fp in b.GetFootprints():
    ref = fp.GetReference()
    if ref.startswith('H'):
        continue
    layer = 'F' if fp.GetLayer() == pcbnew.F_Cu else 'B'
    poly = fp.GetCourtyard(pcbnew.F_CrtYd if layer == 'F' else pcbnew.B_CrtYd)
    pts = []
    if poly.OutlineCount():
        o = poly.Outline(0)
        pts = [(pcbnew.ToMM(o.CPoint(i).x), pcbnew.ToMM(o.CPoint(i).y)) for i in range(o.PointCount())]
    if len(pts) < 3:
        bb = fp.GetBoundingBox(False)
        x0, y0, x1, y1 = [pcbnew.ToMM(v) for v in (bb.GetLeft(), bb.GetTop(), bb.GetRight(), bb.GetBottom())]
        pts = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
    g = SPoly(pts).buffer(0)
    cy[ref] = (layer, g)
    col = '#dddddd' if layer == 'F' else '#c9a227'
    ax.add_patch(Polygon(pts, closed=True, fc=col if layer == 'F' else 'none', ec='k' if layer == 'F' else col,
                         lw=.4, alpha=.9 if layer == 'F' else .6))
    if layer == 'F':
        c = g.centroid
        ax.text(c.x, c.y, ref, fontsize=3.2, ha='center', va='center')
    if not outline.contains(g):
        problems.append(f'{ref}: courtyard outside the board edge ({g.difference(outline).area:.2f} mm2)')
    if layer == 'F':
        for name, k, hard in KO:
            if ref == 'J3' and name.startswith('magnet'):
                continue
            if g.intersects(k) and g.intersection(k).area > 0.01:
                problems.append(f'{ref}: in keep-out "{name}" ({g.intersection(k).area:.2f} mm2)')
        for name, k in KO_SOFT:
            if g.intersects(k) and g.intersection(k).area > 0.01:
                problems.append(f'{ref}: on {name}')
refs = sorted(cy)
for i, a in enumerate(refs):
    for bref in refs[i + 1:]:
        la, ga = cy[a]; lb, gb = cy[bref]
        if la == lb and ga.intersects(gb) and ga.intersection(gb).area > 0.02:
            problems.append(f'overlap {a} / {bref} ({ga.intersection(gb).area:.2f} mm2)')

ax.set_xlim(108, 192); ax.set_ylim(214, 56); ax.set_aspect('equal')
ax.set_title('F-side placement (seen from the back cover). Red = case keep-outs, pink = back-cover ribs,\n'
             'gold outlines = key pads on B side, dashed = e-paper panel on B side', fontsize=8)
ax.grid(alpha=.2)
fig.savefig(out, dpi=170, bbox_inches='tight')
print(f'{len(problems)} problems')
for p in problems:
    print(' -', p)
