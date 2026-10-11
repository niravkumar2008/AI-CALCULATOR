"""Fit-check outputs: a 1:1 grind map (SVG, normal + mirrored), a clearance table and a dummy-board STL.
Run with KiCad's python:  python tools/make_fitcheck.py   (writes fitcheck/)

Stage 12: every F-side part height now comes from its 3D model (the STL export), not a hand table,
and each part is checked against the back-cover ribs/rings/solar box in fitcheck/backcover_fx115es.json
for three values of the free height above the F side (pessimistic / design / optimistic, see the json).
"""
import pcbnew, pathlib, json, math, re, subprocess, sys
HW = pathlib.Path(__file__).resolve().parent.parent
PCB = HW / 'kicad' / 'ai_calc.kicad_pcb'
b = pcbnew.LoadBoard(str(PCB)); mm = pcbnew.ToMM
T = 0.8  # board thickness; STL has x = KiCad x, y = -KiCad y, board bottom at z = 0

# ---- 1. STL with every 3D model -> real part heights
stl = HW / 'fitcheck' / 'dummy_board.stl'
cli = pathlib.Path(sys.executable).with_name('kicad-cli.exe')
subprocess.run([str(cli), 'pcb', 'export', 'stl', '--subst-models', '-f', '-o', str(stl), str(PCB)],
               check=True, capture_output=True)
stl_text = stl.read_text()
V = [(float(a), -float(b_), float(c)) for a, b_, c in re.findall(r'vertex\s+(\S+)\s+(\S+)\s+(\S+)', stl_text)]

def cy_box(f):
    c = f.GetCourtyard(pcbnew.F_CrtYd)
    q = c.BBox() if c.OutlineCount() else f.GetBoundingBox(False)
    return mm(q.GetX()), mm(q.GetY()), mm(q.GetRight()), mm(q.GetBottom())

parts = []
for f in b.GetFootprints():
    r = f.GetReference()
    if f.GetLayer() != pcbnew.F_Cu or r.startswith(('H', 'FID')):
        continue
    x0, y0, x1, y1 = cy_box(f)
    # only vertices inside this part's pad/body box (shrunk 0.1 so neighbours don't leak in)
    zs = [z for x, y, z in V if x0 + 0.1 <= x <= x1 - 0.1 and y0 + 0.1 <= y <= y1 - 0.1 and z > T + 0.01]
    h = round(max(zs) - T, 2) if zs else 0.05
    fp = str(f.GetFPID().GetLibItemName())
    for size, hmax in (('0402', 0.6), ('0603', 0.95), ('0805', 1.45), ('SOT-23', 1.6)):
        if size in fp: h = min(h, hmax)   # a neighbour's model can spill into a tiny passive's box
    parts.append([r, x0, y0, x1, y1, h])
# off-board parts that sit over / beside the board: (label, x0, y0, x1, y1, height)
EXTRA = [('camera 5.4 (8.5 x 8.5 module, 7 mm window)', 145.75, 90.85, 154.25, 99.35, 5.4),
         # rev E/F: Adafruit #1317 (26.02 x 19.75 x 3.8) flat on the back-cover floor, KiCad x 152.59-178.61, y 60.36-80.11
         # (battery_upgrade.md section 8). It lies UNDER the board level (Z 1.0-4.8 from the back), 2.2 mm below the board.
         ('LiPo #1317 150 mAh 3.8 thick (flat on the back-cover floor, 2.2 under the board)', 152.59, 60.36, 178.61, 80.11, 3.8),
         ('magnet body (Yiwei 21 x 7 face, beside the board edge)', 117.0, 57.3, 138.0, 61.3, 4.8)]
# notes for the extra rows in the clearance table
EXTRA_NOTE = {'camera': 'not soldered; module sits on the board (or taped to the cover), lens under the drilled window',
              'LiPo': 'rev E: Adafruit #1317 flat on the back-cover floor under the solar-box zone (ground flat), 2.2 mm under the board, 0.3 mm under the LR44 cup (kept); its margin row counts the floor-to-board space (6.0) only',
              'magnet': 'stage 13b: body in front of the board edge (y 57.3-61.3), centred 1.3 mm above the F side, spanning 2.2 below to 4.8 above it; legs straightened into J3 (right-angle header)'}
_BCJ = json.loads((HW / 'fitcheck' / 'backcover_fx115es.json').read_text())
FRONT = {k: v for k, v in _BCJ.get('_front_shell', {}).items() if not k.startswith('_')}
KEEP = set(_BCJ.get('_front_shell', {}).get('_keep', []))          # front-shell features drawn but NOT ground (rev D/F)
ZONE = _BCJ.get('_zone', {})                                        # grinding-guide zone number per feature
DRILL = _BCJ.get('_drill', {})                                      # holes to drill in the back cover (zone 3)
ANT = _BCJ.get('_antenna_tab')                                      # ESP32 antenna tab box (paper dry fit)

# ---- 2. back-cover features and clearance check
BC = json.loads((HW / 'fitcheck' / 'backcover_fx115es.json').read_text())
RH = BC.get('_heights', {}); AG = BC.get('_after_grind', {}); GAP = BC.get('_gap_above_F', {'pessimistic': 4.0, 'design': 4.5, 'optimistic': 5.5})
U = 1.0   # photo position uncertainty of the ribs (mm)
W = 0.5   # half rib thickness

def near(feat, x0, y0, x1, y1):
    kind = feat[0]
    if kind == 'box':
        (bx0, by0), (bx1, by1) = feat[1], feat[2]
        return not (x1 < bx0 or x0 > bx1 or y1 < by0 or y0 > by1)
    pts = ([(feat[1][0] + feat[2] * math.cos(math.radians(a)), feat[1][1] + feat[2] * math.sin(math.radians(a))) for a in range(0, 360, 2)]
           if kind == 'ring' else
           [(feat[1][0] + t / 40 * (feat[2][0] - feat[1][0]), feat[1][1] + t / 40 * (feat[2][1] - feat[1][1])) for t in range(41)])
    return min(math.hypot(max(x0 - px, 0, px - x1), max(y0 - py, 0, py - y1)) for px, py in pts) <= W + U

rows, grind = [], set()
for r, x0, y0, x1, y1, h in parts:
    hits = [k for k, v in BC.items() if not k.startswith('_') and near(v, x0, y0, x1, y1)]
    rib = max([RH.get(k, RH.get('default rib', 1.0)) for k in hits], default=0.0)
    m = {k: round(g - rib - h, 2) for k, g in GAP.items() if not k.startswith('_')}
    if m.get('design', 9) < 0.3:
        grind.update(k for k in hits if RH.get(k, 1.0) > 0)
    rib_g = max([AG.get(k, RH.get(k, RH.get('default rib', 1.0))) for k in hits], default=0.0)
    m['after grind @ design'] = round(GAP['design'] - rib_g - h, 2)
    rows.append((r, h, ', '.join(hits) or '-', rib, m))
for lab, x0, y0, x1, y1, h in EXTRA:
    hits = [k for k, v in BC.items() if not k.startswith('_') and near(v, x0, y0, x1, y1)]
    if lab.startswith('LiPo'):
        hits = [k for k in hits if k == 'solar box']
    rib = max([RH.get(k, RH.get('default rib', 1.0)) for k in hits], default=0.0)
    m = {k: round(g - rib - h, 2) for k, g in GAP.items() if not k.startswith('_')}
    if lab.startswith('LiPo'):   # rev E: the LiPo lies on the back-cover floor, under the board's level: room = free height only
        m = {k: round(g - rib - h, 2) for k, g in GAP.items() if not k.startswith('_')}
    if m.get('design', 9) < 0.3 and not lab.startswith('magnet'):
        grind.update(k for k in hits if RH.get(k, 1.0) > 0)
    rib_g = max([AG.get(k, RH.get(k, RH.get('default rib', 1.0))) for k in hits], default=0.0)
    m['after grind @ design'] = round(GAP['design'] - rib_g - h, 2)
    rows.append((lab.split(' (')[0] + ' *', h, ', '.join(hits) or '-', rib, m))
rows.sort(key=lambda t: -t[1])
with open(HW / 'fitcheck' / 'clearance_table.md', 'w', encoding='utf-8') as fo:
    fo.write('# F-side clearance table (generated by tools/make_fitcheck.py)\n\n')
    fo.write('Height = tallest point of the 3D model above the board F side. Rib = tallest back-cover feature within '
             f'{W + U:.1f} mm of the part (photo positions are +-{U:.0f} mm). Margin = free height - rib - part, for the '
             'free-height cases in backcover_fx115es.json (design 6.0 = D4 measured to the inside face of the back cover). Below 0.3 = needs a fix or a grind.\n\n')
    keys = [k for k in GAP if not k.startswith('_')] + ['after grind @ design']
    fo.write('Last column: after the grind plan (' + ', '.join(f'{k} -> {v:g} mm' for k, v in AG.items() if not k.startswith('_')) + ').\n\n')
    fo.write('| Part | Height | Back-cover feature over it | Feature height | '
             + ' | '.join(f'Margin @ {GAP[k]} ({k})' if k in GAP else 'Margin after grind (design)' for k in keys) + ' |\n')
    fo.write('|---|---|---|---|' + '---|' * len(keys) + '\n')
    for r, h, hits, rib, m in rows:
        fo.write(f'| {r} | {h:.2f} | {hits} | {rib:g} | ' + ' | '.join(('**%.2f**' if m[k] < 0.3 else '%.2f') % m[k] for k in keys) + ' |\n')
    fo.write('\n\\* = not a board part. ' + ' '.join(f'**{k}:** {v}.' for k, v in EXTRA_NOTE.items()) +
             ' The LiPo row is the floor-to-board space (6.0) minus the cell (3.8): the real limit is the LR44 cup in the faceplate (0.3 mm, assembly_report.md section 9).\n')
    fo.write('\nFront shell: ' + '; '.join(f'{k} at {v[1]}-{v[2]}' if v[0] == 'box' else f'{k} (centre {v[1][0]}, {v[1][1]}, outer dia {2 * v[2]:g})' for k, v in FRONT.items()) +
             '. See the grind list on grind_map_front_shell.svg.\n')
print('clearance table:', len(rows), 'parts; grind:', sorted(grind))

# ---- 3. grind maps (SVG 1:1)
def col(h): return '#d7263d' if h >= 4 else '#f49d37' if h >= 2 else '#3f88c5'
ol = pcbnew.SHAPE_POLY_SET(); b.GetBoardPolygonOutlines(ol, False)
bb = b.GetBoardEdgesBoundingBox()
X0, Y0, Wd, Ht = mm(bb.GetX()) - 5, mm(bb.GetY()) - 36, mm(bb.GetWidth()) + 10, mm(bb.GetHeight()) + 54
def ring_path(c): return 'M' + ' L'.join(f'{mm(c.CPoint(i).x):.2f},{mm(c.CPoint(i).y):.2f}' for i in range(c.PointCount())) + ' Z'
paths = [ring_path(ol.Outline(i)) for i in range(ol.OutlineCount())] + \
        [ring_path(ol.Hole(i, j)) for i in range(ol.OutlineCount()) for j in range(ol.HoleCount(i))]
els = [f'<path d="{" ".join(paths)}" fill="#eef3ee" fill-rule="evenodd" stroke="#000" stroke-width="0.25"/>']
for f in b.GetFootprints():
    for p in f.Pads():
        if p.GetDrillSizeX() > 0 and not p.IsOnLayer(pcbnew.F_Cu):
            c = p.GetPosition(); els.append(f'<circle cx="{mm(c.x):.2f}" cy="{mm(c.y):.2f}" r="{mm(p.GetDrillSizeX())/2:.2f}" fill="#fff" stroke="#000" stroke-width="0.2"/>')
for r, x0, y0, x1, y1, h in parts:
    if h < 1.0:
        continue  # small passives: not worth drawing
    els.append(f'<rect x="{x0:.2f}" y="{y0:.2f}" width="{x1-x0:.2f}" height="{y1-y0:.2f}" fill="{col(h)}" fill-opacity=".45" stroke="{col(h)}" stroke-width="0.3"/>')
    els.append(f'<text class="t" x="{(x0+x1)/2:.2f}" y="{(y0+y1)/2+0.6:.2f}">{r} {h:.1f}</text>')
for lab, x0, y0, x1, y1, h in EXTRA:
    els.append(f'<rect x="{x0:.2f}" y="{y0:.2f}" width="{x1-x0:.2f}" height="{y1-y0:.2f}" fill="none" stroke="{col(h)}" stroke-width="0.4" stroke-dasharray="1 0.6"/>')
    els.append(f'<text class="t" x="{(x0+x1)/2:.2f}" y="{y1-0.8:.2f}">{lab}</text>')

bc = []
for k, v in BC.items():
    if k.startswith('_'):
        continue
    hgt = RH.get(k, RH.get('default rib', 1.0)); g = k in grind
    stroke = '#d00' if g else '#222'; wdt = 1.2 if g else 0.8
    lab = (f'{ZONE[k]}: ' if k in ZONE else '') + f'{k} {hgt:g} mm' + (' - GRIND' if g else '')
    if v[0] == 'ring':
        bc.append(f'<circle cx="{v[1][0]:.2f}" cy="{v[1][1]:.2f}" r="{v[2]:.2f}" fill="none" stroke="{stroke}" stroke-width="{wdt}" stroke-dasharray="1.5 0.8"/>')
        bc.append(f'<text class="t" x="{v[1][0]:.2f}" y="{v[1][1]+v[2]+2.2:.2f}">{lab}</text>')
    elif v[0] == 'box':
        (bx0, by0), (bx1, by1) = v[1], v[2]
        bc.append(f'<rect x="{bx0}" y="{by0}" width="{bx1-bx0}" height="{by1-by0}" fill="#d00" fill-opacity=".12" stroke="{stroke}" stroke-width="{wdt}" stroke-dasharray="1.5 0.8"/>')
        bc.append(f'<text class="t" x="{(bx0+bx1)/2:.2f}" y="{by0+1.8:.2f}">{lab} FLAT</text>')
    else:
        bc.append(f'<line x1="{v[1][0]:.2f}" y1="{v[1][1]:.2f}" x2="{v[2][0]:.2f}" y2="{v[2][1]:.2f}" stroke="{stroke}" stroke-width="{wdt}" stroke-dasharray="1.5 0.8"/>')
        if g:
            L = math.hypot(v[2][0] - v[1][0], v[2][1] - v[1][1])
            bc.append(f'<text class="t" x="{(v[1][0]+v[2][0])/2:.2f}" y="{(v[1][1]+v[2][1])/2-1.2:.2f}">{k}: grind {hgt:g} mm down to the floor over {L:.0f} mm</text>')

# grind list (printed on both maps): every zone with its depth; never grind into the floor (keep >= 0.8 mm plastic)
GL_BACK = [f'{ZONE.get(k, "")} {k}: remove {RH.get(k, RH.get("default rib", 1.0)):g} mm (down to the floor)'.strip() for k in sorted(grind)]
GL_BACK += [f'{v[3]}' for k, v in DRILL.items()]
FN = _BCJ.get('_front_shell', {}).get('_notes', {})
GL_FRONT = [f'{ZONE.get(k, "")} {k} (front shell): {FN.get(k, "remove down to the floor")}'.strip() for k in FRONT if k not in KEEP]
GL_FRONT += [f'{ZONE.get(k, "")} {k} (front shell): {FN.get(k, "keep")}'.strip() for k in FRONT if k in KEEP]
GL_FRONT += ['ANTENNA TAB (yellow): ' + ANT['note']] if ANT else []
# drill holes on the back-cover map (zone 3) and the antenna tab on both maps
for k, v in DRILL.items():
    bc.append(f'<circle cx="{v[1][0]:.2f}" cy="{v[1][1]:.2f}" r="{v[2]:.2f}" fill="none" stroke="#d00" stroke-width="0.5"/>')
    bc.append(f'<line x1="{v[1][0]-v[2]-1.5:.2f}" y1="{v[1][1]:.2f}" x2="{v[1][0]+v[2]+1.5:.2f}" y2="{v[1][1]:.2f}" stroke="#d00" stroke-width="0.2"/>')
    bc.append(f'<line x1="{v[1][0]:.2f}" y1="{v[1][1]-v[2]-1.5:.2f}" x2="{v[1][0]:.2f}" y2="{v[1][1]+v[2]+1.5:.2f}" stroke="#d00" stroke-width="0.2"/>')
    bc.append(f'<text class="t" x="{v[1][0]:.2f}" y="{v[1][1]+v[2]+3.0:.2f}">{ZONE.get(k, k)}: drill {2*v[2]:g} mm here</text>')
if ANT:
    (ax0, ay0), (ax1, ay1) = ANT['box']
    els.append(f'<rect x="{ax0}" y="{ay0}" width="{ax1-ax0:.2f}" height="{ay1-ay0:.2f}" fill="#ffd54f" fill-opacity=".55" stroke="#000" stroke-width="0.25"/>')
    els.append(f'<text class="t" x="{(ax0+ax1)/2:.2f}" y="{ay0-1.0:.2f}">ANTENNA TAB {ax1-ax0:.1f} x {ay1-ay0:.1f}: cut out WITH the board</text>')

fr = []
for k, v in FRONT.items():
    keep = k in KEEP
    col_ = '#1a7f37' if keep else '#d00'
    z = ZONE.get(k, '')
    if v[0] == 'box':
        (bx0, by0), (bx1, by1) = v[1], v[2]
        cond = 'only if needed' in z
        dash = ' stroke-dasharray="1 0.6"' if cond else ''
        fr.append(f'<rect x="{bx0}" y="{by0}" width="{bx1-bx0}" height="{by1-by0}" fill="{col_}" fill-opacity=".35" stroke="{col_}" stroke-width="0.4"{dash}/>')
        if bx1 - bx0 < 3:   # narrow (the inner rib): label beside it, rotated
            cx_, cy_ = bx1 + 1.4, (by0 + by1) / 2
            fr.append(f'<text class="t" x="{cx_:.2f}" y="{cy_:.2f}" transform="rotate(-90 {cx_:.2f},{cy_:.2f})">{z}: {k} - file the 1 mm rib away, only if it reaches the board plane</text>')
        else:
            fr.append(f'<text class="t" x="{(bx0+bx1)/2:.2f}" y="{by0-1.0:.2f}">{z}: {k}: CUT {bx1-bx0:.1f} wide</text>')
    else:
        what = 'KEEP, do not grind' if keep else 'TRIM FLAT'
        fr.append(f'<circle cx="{v[1][0]:.2f}" cy="{v[1][1]:.2f}" r="{v[2]:.2f}" fill="{col_}" fill-opacity=".15" stroke="{col_}" stroke-width="1.2" stroke-dasharray="1.5 0.8"/>')
        fr.append(f'<text class="t" x="{v[1][0]:.2f}" y="{v[1][1]+v[2]+2.2:.2f}">{z}: {k} (front shell) - {what}</text>')

def svg(mirror, name, title):
    g = f'<g transform="translate({2*X0+Wd:.2f},0) scale(-1,1)">' if mirror else '<g>'
    extra = bc if mirror else fr
    body = '\n'.join(e for e in els if not e.startswith('<text')) + '\n' + '\n'.join(e for e in extra if not e.startswith('<text'))
    txt = '\n'.join(e for e in els + extra if e.startswith('<text'))
    if mirror:  # mirror the shapes, keep labels readable
        txt = re.sub(r'x="([\d.]+)"', lambda m: f'x="{2*X0+Wd-float(m.group(1)):.2f}"', txt)
    sb = f'<g><line x1="{X0+3}" y1="{Y0+Ht-6}" x2="{X0+53}" y2="{Y0+Ht-6}" stroke="#000" stroke-width="0.4"/>' \
         f'<text class="s" x="{X0+3}" y="{Y0+Ht-2}" style="text-anchor:start">50 mm: check this with a ruler (print at 100 %)</text></g>'
    head = f'<text class="s" x="{X0+3}" y="{Y0+5}" style="text-anchor:start">{title}</text>' \
           f'<text class="s" x="{X0+3}" y="{Y0+9}" style="text-anchor:start">part height above board: red 4+ mm, orange 2-4, blue 1-2' \
           f'{"; dashed = back-cover ribs/rings (red = GRIND where it crosses a part); yellow = ESP32 antenna tab" if mirror else "; red = front-shell feature to cut, green = keep, yellow = ESP32 antenna tab (cut out with the board)"}</text>' \
           + ''.join(f'<text class="s" x="{X0+3}" y="{Y0+13+3*i}" style="text-anchor:start;fill:{"#1a7f37" if "KEEP" in t else "#000" if t.startswith("ANTENNA") or t.startswith("zone numbers") else "#d00"};font-size:{"1.5px" if len(t) > 140 else "2.2px"}">{t}</text>'
                     for i, t in enumerate((GL_BACK if mirror else GL_FRONT) + ['never grind into the floor: keep at least 0.8 mm of plastic'] +
                                           ['zone numbers = the Shell Grinding Guide (grinding_manual.html): 1 solar box, 2 rib B, 3 camera window, 4 LR44 cup (keep), 5 magnet notch (+5b lip), 6 antenna relief (only if needed)']))
    (HW / 'fitcheck' / name).write_text(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{Wd:.2f}mm" height="{Ht:.2f}mm" viewBox="{X0:.2f} {Y0:.2f} {Wd:.2f} {Ht:.2f}">'
        '<style>.t{font:1.4px sans-serif;text-anchor:middle}.s{font:2.2px sans-serif}</style>'
        f'{g}{body}</g>{txt}{head}{sb}</svg>', encoding='utf-8')

svg(False, 'grind_map_front_shell.svg', 'GRIND MAP - lay in the FRONT shell (keys down), printed side up')
svg(True, 'grind_map_back_cover.svg', 'GRIND MAP - lay in the BACK cover (inside up): mirrored')

# ---- 4. dummy board STL: add the camera and battery blocks
def box(x, y, w, h, z0, z1):
    """12 triangles of an axis-aligned box in KiCad x/y (y flipped for the STL)."""
    X, Y = (x, x + w), (-y - h, -y)
    v = lambda i, j, k: (X[i], Y[j], (z0, z1)[k])
    quads = [((0,0,0),(0,1,0),(1,1,0),(1,0,0)), ((0,0,1),(1,0,1),(1,1,1),(0,1,1)),
             ((0,0,0),(1,0,0),(1,0,1),(0,0,1)), ((0,1,0),(0,1,1),(1,1,1),(1,1,0)),
             ((0,0,0),(0,0,1),(0,1,1),(0,1,0)), ((1,0,0),(1,1,0),(1,1,1),(1,0,1))]
    return [(v(*q[0]), v(*q[a]), v(*q[a + 1])) for q in quads for a in (1, 2)]

tris = box(145.75, 90.85, 8.5, 8.5, T, T + 5.4)        # camera module 8.5 x 8.5 x 5.4 centred on (150, 95.1)
tris += box(152.59, 60.36, 26.02, 19.75, 0.0, 3.8)    # rev E: Adafruit #1317 at its back-cover-floor position (separate piece; the
                                                       # print lies loose: put it on the back-cover floor, not on the board)
# The ESP32 antenna tab (x 114.75-118.9, 0.8 thick) is already in the STL through U1's 3D model; nothing to add.
cut = stl_text.rstrip().rfind('endsolid')
facets = ''.join('facet normal 0 0 0\n outer loop\n' + ''.join(f'  vertex {p[0]:.4f} {p[1]:.4f} {p[2]:.4f}\n' for p in tri)
                 + ' endloop\nendfacet\n' for tri in tris)
stl.write_text(stl_text[:cut] + facets + stl_text[cut:])
print('fitcheck written: grind maps, clearance_table.md, dummy_board.stl (+', len(tris), 'block triangles)')
