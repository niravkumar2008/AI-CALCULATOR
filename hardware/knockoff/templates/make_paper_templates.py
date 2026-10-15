"""1:1 paper versions of the knockoff marking templates (for when there is no 3D printer).

    python hardware/knockoff/templates/make_paper_templates.py

Writes paper_templates.html (US Letter, exact millimetres) and prints it to paper_templates.pdf with Microsoft Edge.
Numbers come from the same places as the printed template (build_templates.py) and the knockoff model
(cad/assembly/geometry.json), so paper and plastic mark the same spots.

Page 1: the back cover, seen from the BACK (as it lies in front of you with the keys facing the table): cut along the
        outline, lay it on the back, prick the camera centre. Also marks where the notch is on the top edge.
Page 2: the top-edge strip with the magnet-notch window, and the ruler-only fallback numbers.
"""
import json, os, pathlib, subprocess, math

HERE = pathlib.Path(__file__).parent
G = json.loads((HERE.parent / "cad" / "assembly" / "geometry.json").read_text())
CAM = (0.0, 42.0)
CAM_D = 7.0
NOTCH_X = (11.75, 33.25)          # front-view X of the U-notch (21.5 wide, centred 22.5)
SEAM, NOTCH_DEPTH = 1.5, 7.95     # parting line above the back's outside; notch depth from the parting line
T_TOP = 11.8
TOP = max(p[1] for p in G["Vb"])
XR = max(p[0] for p in G["Vf"])   # right outer edge (front view)
EDGE = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"


def path(P, fx, fy):
    return "M" + " L".join("%.2f,%.2f" % (fx(x), fy(y)) for x, y in P) + " Z"


def page1():
    xs = [p[0] for p in G["Vb"]]; ys = [p[1] for p in G["Vb"]]
    w, h = 190.0, 200.0                     # drawing area in mm
    cx = w / 2
    fx = lambda x: cx - x                   # seen from the back: front-view +X is on the LEFT
    fy = lambda y: 18 + (TOP - y)           # top edge 18 mm below the area's top
    o = []
    o.append('<path d="%s" class="cut"/>' % path(G["Vb_s"], fx, fy))
    # camera
    x, y = fx(CAM[0]), fy(CAM[1])
    o.append('<circle cx="%.2f" cy="%.2f" r="%.2f" class="cam"/>' % (x, y, CAM_D / 2))
    o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="cross"/>' % (x - 9, y, x + 9, y))
    o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="cross"/>' % (x, y - 9, x, y + 9))
    o.append('<circle cx="%.2f" cy="%.2f" r="0.6" class="dot"/>' % (x, y))
    o.append('<text x="%.2f" y="%.2f" class="lbl" text-anchor="middle">CAMERA: prick the red dot</text>' % (x, y + 14))
    o.append('<text x="%.2f" y="%.2f" class="sm" text-anchor="middle">drill 2 mm, then 7 mm · centred, %.1f mm below the top edge</text>' % (x, y + 18, TOP - CAM[1]))
    # dimension: top edge to camera centre
    o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="dim"/>' % (x - 14, fy(TOP), x - 14, y))
    o.append('<text x="%.2f" y="%.2f" class="sm" transform="rotate(-90 %.2f %.2f)">%.1f mm</text>' % (x - 15.5, (fy(TOP) + y) / 2 + 6, x - 15.5, (fy(TOP) + y) / 2 + 6, TOP - CAM[1]))
    # magnet-notch tick marks on the top edge (seen from the back: mirrored)
    for X in NOTCH_X:
        ty = fy(TOP)
        o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="tick"/>' % (fx(X), ty - 6, fx(X), ty + 4))
    a, b = fx(NOTCH_X[1]), fx(NOTCH_X[0])
    o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="5" class="notch"/>' % (a, fy(TOP) - 11, b - a))
    o.append('<text x="%.2f" y="%.2f" class="sm" text-anchor="middle">magnet notch (on the top wall)</text>' % ((a + b) / 2, fy(TOP) - 12.5))
    # side labels
    o.append('<text x="%.2f" y="%.2f" class="lbl" text-anchor="middle">TOP EDGE (screen end)</text>' % (cx + 22, fy(TOP) - 3))
    o.append('<text x="%.2f" y="%.2f" class="sm" text-anchor="middle">bottom edge</text>' % (cx, fy(min(ys)) + 6))
    o.append('<text x="%.2f" y="%.2f" class="sm" text-anchor="end">← solar-strip side</text>' % (fx(XR) - 2 + 0, fy(60)))
    # 50 mm scale check
    sx, sy = 6, h - 6
    o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="scale"/>' % (sx, sy, sx + 50, sy))
    for k in range(0, 51, 10):
        o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="scale"/>' % (sx + k, sy - (3 if k % 50 == 0 else 1.6), sx + k, sy))
    o.append('<text x="%.2f" y="%.2f" class="sm">50 mm: measure it. If it is not 50, print at "Actual size" / 100 %%.</text>' % (sx + 53, sy + 1))
    return '<svg width="%gmm" height="%gmm" viewBox="0 0 %g %g">%s</svg>' % (w, h, w, h, "".join(o))


def page2():
    w, h = 190.0, 120.0
    L = 2 * XR                                   # top edge, left to right as seen from the front (keys facing you)
    x0, y0 = (w - L) / 2, 30.0                    # strip origin (left end, back-cover side)
    fx = lambda X: x0 + (X + XR)
    o = []
    # strip: X across, Z down (back side on top, face side at the bottom), plus 6 mm tabs to fold round the corners
    o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" class="cut"/>' % (x0 - 6, y0, L + 12, T_TOP))
    o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="fold"/>' % (x0, y0, x0, y0 + T_TOP))
    o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="fold"/>' % (x0 + L, y0, x0 + L, y0 + T_TOP))
    o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="seam"/>' % (x0, y0 + SEAM, x0 + L, y0 + SEAM))
    a, b = fx(NOTCH_X[0]), fx(NOTCH_X[1])
    o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" class="win"/>' % (a, y0 + SEAM, b - a, NOTCH_DEPTH))
    o.append('<text x="%.2f" y="%.2f" class="sm" text-anchor="middle">CUT OUT: magnet notch 21.5 × 7.95</text>' % ((a + b) / 2, y0 + SEAM + NOTCH_DEPTH / 2 + 1.2))
    o.append('<text x="%.2f" y="%.2f" class="sm">BACK-COVER SIDE ↑</text>' % (x0, y0 - 2))
    o.append('<text x="%.2f" y="%.2f" class="sm">KEYS / FACE SIDE ↓</text>' % (x0, y0 + T_TOP + 4.5))
    o.append('<text x="%.2f" y="%.2f" class="sm" text-anchor="end">seam (where the two halves meet) - - -</text>' % (x0 + L, y0 - 2))
    o.append('<text x="%.2f" y="%.2f" class="lbl" text-anchor="end">RIGHT end (keys facing you)</text>' % (x0 + L + 6, y0 + T_TOP + 9))
    o.append('<text x="%.2f" y="%.2f" class="lbl">LEFT end</text>' % (x0 - 6, y0 + T_TOP + 9))
    # dims from the right end
    for X, t in ((NOTCH_X[1], "%.1f" % (XR - NOTCH_X[1])), (NOTCH_X[0], "%.1f" % (XR - NOTCH_X[0]))):
        o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="dim"/>' % (fx(X), y0 - 10, fx(XR), y0 - 10))
        o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="tick"/>' % (fx(X), y0 - 12, fx(X), y0))
    o.append('<text x="%.2f" y="%.2f" class="sm" text-anchor="middle">6.4 mm and 27.9 mm from the right end</text>' % ((fx(NOTCH_X[0]) + fx(XR)) / 2, y0 - 12))
    # scale check
    sx, sy = 6, h - 6
    o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="scale"/>' % (sx, sy, sx + 50, sy))
    for k in range(0, 51, 10):
        o.append('<line x1="%.2f" y1="%.2f" x2="%.2f" y2="%.2f" class="scale"/>' % (sx + k, sy - (3 if k % 50 == 0 else 1.6), sx + k, sy))
    o.append('<text x="%.2f" y="%.2f" class="sm">50 mm check</text>' % (sx + 53, sy + 1))
    return '<svg width="%gmm" height="%gmm" viewBox="0 0 %g %g">%s</svg>' % (w, h, w, h, "".join(o))


HTML = """<!doctype html><html><head><meta charset="utf-8"><title>Knockoff Marking Templates (paper)</title>
<style>
@page { size: Letter; margin: 10mm; }
* { box-sizing: border-box; }
body { margin: 0; font-family: Arial, Helvetica, sans-serif; color: #111; background: #fff; }
.page { width: 195mm; page-break-after: always; }
.page:last-child { page-break-after: auto; }
h1 { font-size: 15pt; margin: 0 0 1mm; } h2 { font-size: 11pt; margin: 3mm 0 1mm; }
p, li { font-size: 9pt; line-height: 1.35; margin: 0 0 1mm; }
ol { margin: 0 0 1mm; padding-left: 5mm; }
svg { display: block; }
svg text { font-family: Arial, Helvetica, sans-serif; }
.cut { fill: none; stroke: #000; stroke-width: 0.35; }
.fold { stroke: #000; stroke-width: 0.25; stroke-dasharray: 1.5 1; }
.seam { stroke: #c00; stroke-width: 0.3; stroke-dasharray: 2 1; }
.win { fill: #ffd84d; stroke: #000; stroke-width: 0.3; }
.notch { fill: #ffd84d; stroke: #000; stroke-width: 0.25; }
.cam { fill: none; stroke: #c00; stroke-width: 0.35; }
.cross { stroke: #c00; stroke-width: 0.2; }
.dot { fill: #c00; }
.dim { stroke: #555; stroke-width: 0.2; }
.tick { stroke: #000; stroke-width: 0.3; }
.scale { stroke: #000; stroke-width: 0.35; }
.lbl { font-size: 3.2px; font-weight: bold; fill: #000; }
.sm { font-size: 2.6px; fill: #222; }
.box { border: 0.3mm solid #000; padding: 2mm 3mm; margin-top: 2mm; }
</style></head><body>
<div class="page">
<h1>Proofline knockoff · paper template 1: back cover (camera spot)</h1>
<p><b>Print at 100 % / "Actual size"</b>, then measure the 50 mm bar at the bottom. Cut along the solid outline.</p>
<ol><li>Lay the calculator <b>keys down</b> on the table, top (screen) end away from you. Lay the cut-out on the back, printed side up, matching its outline to the back cover's edges.</li>
<li>Tape it down. Press a pin or the tip of a compass through the <b>red dot</b> to mark the camera centre. Remove the paper.</li>
<li>Drill a 2 mm pilot on the mark, then open it to 7 mm (step drill) from the outside. Deburr.</li></ol>
<!--P1-->
</div>
<div class="page">
<h1>Paper template 2: top edge (magnet notch)</h1>
<ol><li>Cut along the solid outline and cut the <b>yellow window</b> out.</li>
<li>Hold the closed calculator with the <b>keys facing you</b>, then tip the top (screen) end towards you so you look at the top edge. Fold the two end tabs on the dashed lines.</li>
<li>Wrap the strip on the top edge: the end marked <b>RIGHT</b> on the calculator's right-hand side (the solar-strip side), the <b>red seam line on the seam</b>, back-cover side up. Tape it.</li>
<li>Trace the window with a sharp pencil: that's the notch in the front shell. Its top edge sits on the seam; the back cover's lip behind it gets snipped to the same width.</li></ol>
<!--P2-->
<div class="box"><h2>No printer? Measure it instead (0.5 mm pencil, steel ruler, calipers)</h2>
<p><b>Camera:</b> on the back, find the middle between the two long sides and mark a line down the centre. Measure <b>%(cam)s mm</b> down from the top edge on that line. That point is the camera centre.</p>
<p><b>Magnet notch:</b> keys facing you, top edge towards you. From the <b>right-hand outer edge</b>, mark <b>6.4 mm</b> and <b>27.9 mm</b> along the top wall. Between those marks the notch goes from the <b>seam</b> towards the keys, <b>7.95 mm</b> deep.</p>
<p>These positions come from the CAD model built from your photos (good to about ±1.5 mm). Before drilling the first back cover, check the camera mark lands between back-cover ribs 2 and 3 on the inside (35.0 and 45.2 mm below the top edge).</p></div>
</div>
</body></html>"""


def main():
    html = HTML.replace("<!--P1-->", page1()).replace("<!--P2-->", page2()).replace("%(cam)s", "%.1f" % (TOP - CAM[1]))
    out = HERE / "paper_templates.html"
    out.write_text(html, encoding="utf8")
    pdf = HERE / "paper_templates.pdf"
    if os.path.exists(EDGE):
        subprocess.run([EDGE, "--headless=new", "--disable-gpu", "--no-pdf-header-footer",
                        "--print-to-pdf=%s" % pdf, out.as_uri()], check=False, timeout=120)
    print("wrote", out, "and", pdf if pdf.exists() else "(no PDF)")


if __name__ == "__main__":
    main()
