"""Fit-check outputs: a 1:1 grind map (SVG, normal + mirrored) of where the tall parts sit.
Run with KiCad's python:  python tools/make_fitcheck.py   (writes fitcheck/)"""
import pcbnew, pathlib
HW = pathlib.Path(__file__).resolve().parent.parent
b = pcbnew.LoadBoard(str(HW / 'kicad' / 'ai_calc.kicad_pcb')); mm = pcbnew.ToMM

# height above the board's F side, mm (datasheets / stage 2-4 notes). ponytail: hand table, update if parts change
H = {'J4': 5.5, 'J3': 5.0, 'U1': 2.5, 'J1': 2.0, 'J2': 2.0, 'L1': 2.0}
EXTRA = [  # off-board parts that sit over / beside the board: (label, x, y, w, h, height)
    ('camera 5.4', 144.0, 89.1, 12.0, 12.0, 5.4),
    ('battery 3.8', 147.9, 63.5, 31.0, 11.5, 3.8),
    ('magnet ~6', 125.25, 60.0, 21.5, 7.5, 6.0),
]
def col(h): return '#d7263d' if h >= 5 else '#f49d37' if h >= 3 else '#3f88c5'

ol = pcbnew.SHAPE_POLY_SET(); b.GetBoardPolygonOutlines(ol, False)
bb = b.GetBoardEdgesBoundingBox()
x0, y0, W, Ht = mm(bb.GetX()) - 5, mm(bb.GetY()) - 12, mm(bb.GetWidth()) + 10, mm(bb.GetHeight()) + 30

def ring(c): return 'M' + ' L'.join(f'{mm(c.CPoint(i).x):.2f},{mm(c.CPoint(i).y):.2f}' for i in range(c.PointCount())) + ' Z'
paths = [ring(ol.Outline(i)) for i in range(ol.OutlineCount())] + \
        [ring(ol.Hole(i, j)) for i in range(ol.OutlineCount()) for j in range(ol.HoleCount(i))]
els = [f'<path d="{" ".join(paths)}" fill="#eef3ee" fill-rule="evenodd" stroke="#000" stroke-width="0.25"/>']
for f in b.GetFootprints():
    for p in f.Pads():
        if p.GetDrillSizeX() > 0:
            c = p.GetPosition(); els.append(f'<circle cx="{mm(c.x):.2f}" cy="{mm(c.y):.2f}" r="{mm(p.GetDrillSizeX())/2:.2f}" fill="#fff" stroke="#000" stroke-width="0.2"/>')
    r = f.GetReference()
    if r in H:
        cy = f.GetCourtyard(pcbnew.F_CrtYd); q = cy.BBox() if cy.OutlineCount() else f.GetBoundingBox(False)
        EXTRA.append((f'{r} {H[r]}', mm(q.GetX()), mm(q.GetY()), mm(q.GetWidth()), mm(q.GetHeight()), H[r]))
for lab, x, y, w, h, z in EXTRA:
    els.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{w:.2f}" height="{h:.2f}" fill="{col(z)}" fill-opacity=".45" stroke="{col(z)}" stroke-width="0.3"/>')
    els.append(f'<text class="t" x="{x+w/2:.2f}" y="{y+h/2+0.8:.2f}">{lab}</text>')

# back-cover rings/ribs (fx-115ES, from photos, ~1 mm): drawn as dark dashed lines, back-cover map only
import json
BC = json.loads((HW / 'fitcheck' / 'backcover_fx115es.json').read_text())
bc = []
for k, v in BC.items():
    if k.startswith('_'): continue
    if v[0] == 'ring':
        bc.append(f'<circle cx="{v[1][0]:.2f}" cy="{v[1][1]:.2f}" r="{v[2]:.2f}" fill="none" stroke="#222" stroke-width="0.8" stroke-dasharray="1.5 0.8"/>')
    else:
        bc.append(f'<line x1="{v[1][0]:.2f}" y1="{v[1][1]:.2f}" x2="{v[2][0]:.2f}" y2="{v[2][1]:.2f}" stroke="#222" stroke-width="0.8" stroke-dasharray="1.5 0.8"/>')

def svg(mirror, name, title):
    g = f'<g transform="translate({2*x0+W:.2f},0) scale(-1,1)">' if mirror else '<g>'
    body = '\n'.join(e for e in els if not e.startswith('<text')) + ('\n'.join(bc) if mirror else '')
    txt = '\n'.join(e for e in els if e.startswith('<text'))
    if mirror:  # mirror the shapes, keep labels readable
        import re
        txt = re.sub(r'x="([\d.]+)"', lambda m: f'x="{2*x0+W-float(m.group(1)):.2f}"', txt)
    sb = f'<g><line x1="{x0+3}" y1="{y0+Ht-6}" x2="{x0+53}" y2="{y0+Ht-6}" stroke="#000" stroke-width="0.4"/>' \
         f'<text class="s" x="{x0+3}" y="{y0+Ht-2}" style="text-anchor:start">50 mm: check this with a ruler (print at 100 %)</text></g>'
    head = f'<text class="s" x="{x0+3}" y="{y0+5}" style="text-anchor:start">{title}</text>' \
           f'<text class="s" x="{x0+3}" y="{y0+9}" style="text-anchor:start">height above board: red 5+ mm, orange 3-5, blue under 3' \
           f'{"; dashed = back-cover rings/ribs (grind where they cross colour)" if mirror else ""}</text>'
    (HW / 'fitcheck' / name).write_text(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{W:.2f}mm" height="{Ht:.2f}mm" viewBox="{x0:.2f} {y0:.2f} {W:.2f} {Ht:.2f}">'
        '<style>.t{font:1.6px sans-serif;text-anchor:middle}.s{font:2.2px sans-serif}</style>'
        f'{g}{body}</g>{txt}{head}{sb}</svg>', encoding='utf-8')

svg(False, 'grind_map_front_shell.svg', 'GRIND MAP - lay in the FRONT shell (keys down), printed side up')
svg(True, 'grind_map_back_cover.svg', 'GRIND MAP - lay in the BACK cover (inside up): mirrored')
print('ok', len(EXTRA), 'zones')

# ---- dummy board STL: the full board with every part, plus camera and battery blocks
import subprocess, sys
stl = HW / 'fitcheck' / 'dummy_board.stl'
cli = pathlib.Path(sys.executable).with_name('kicad-cli.exe')
subprocess.run([str(cli), 'pcb', 'export', 'stl', '--subst-models', '-f', '-o', str(stl),
                str(HW / 'kicad' / 'ai_calc.kicad_pcb')], check=True, capture_output=True)
T = 0.8  # board thickness; STL has x = KiCad x, y = -KiCad y, board bottom at z = 0


def box(x, y, w, h, z0, z1):
    """12 triangles of an axis-aligned box in KiCad x/y (y flipped for the STL)."""
    X, Y = (x, x + w), (-y - h, -y)
    v = lambda i, j, k: (X[i], Y[j], (z0, z1)[k])
    quads = [((0,0,0),(0,1,0),(1,1,0),(1,0,0)), ((0,0,1),(1,0,1),(1,1,1),(0,1,1)),
             ((0,0,0),(1,0,0),(1,0,1),(0,0,1)), ((0,1,0),(0,1,1),(1,1,1),(1,1,0)),
             ((0,0,0),(0,0,1),(0,1,1),(0,1,0)), ((1,0,0),(1,1,0),(1,1,1),(1,0,1))]
    return [(v(*q[0]), v(*q[a]), v(*q[a + 1])) for q in quads for a in (1, 2)]


tris = box(144.0, 89.1, 12.0, 12.0, T, T + 5.4)      # camera module on the board, 8.5 mm part in a 12 mm keepout
tris += box(147.9, 63.5, 31.0, 11.5, 0.0, 3.8)       # LiPo in the battery bay (separate piece)
t = stl.read_text()  # KiCad writes ASCII STL
cut = t.rstrip().rfind('endsolid')
facets = ''.join('facet normal 0 0 0\n outer loop\n' + ''.join(f'  vertex {p[0]:.4f} {p[1]:.4f} {p[2]:.4f}\n' for p in tri)
                 + ' endloop\nendfacet\n' for tri in tris)
stl.write_text(t[:cut] + facets + t[cut:])
print('dummy board STL: +', len(tris), 'block triangles')
