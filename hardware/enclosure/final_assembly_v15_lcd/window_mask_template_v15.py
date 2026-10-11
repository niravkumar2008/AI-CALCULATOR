"""v15-LCD COPY. Window mask cut template (1:1 SVG for printing + DXF for a laser cutter) from the final assembly's numbers.

    python window_mask_template_v15.py    -> window_mask_template_v15.svg, window_mask_template_v15.dxf

Numbers come from placement.json ("mask", written by build_final_assembly_v15.py stage mask):
lens outline (= mask outline), opening over the 1.9" IPS LCD active area (42.72 x 22.70, centre (150.0, 93.1) KiCad) PLUS 0.5 mm on every side
(the panel's own black border hides the edge, so no pixel is ever covered). Drawn as seen from the FRONT of the calculator
(through the lens). Units: mm.
"""
import json, math, os

HERE = os.path.dirname(os.path.abspath(__file__))
M = json.load(open(os.path.join(HERE, "placement.json")))["mask"]
WIN = (60.65, 24.3)          # display window through-opening (caliper C12), centred on the lens
WIN_R = 1.0

# page frame: lens centre at (cx, cy) on an A4 page (210 x 297), y down in SVG
PW, PH = 210.0, 297.0
cx, cy = PW / 2, 95.0
ox, oy = M["open_c"][0] - M["lens_c"][0], M["open_c"][1] - M["lens_c"][1]   # opening offset (front view, +x right, +y up)

def rrect_path(x, y, w, h, r):
    """SVG path of a rounded rectangle centred at (x, y)."""
    l, t, rr = x - w / 2, y - h / 2, min(r, w / 2, h / 2)
    return ("M %.3f %.3f H %.3f A %.3f %.3f 0 0 1 %.3f %.3f V %.3f A %.3f %.3f 0 0 1 %.3f %.3f H %.3f "
            "A %.3f %.3f 0 0 1 %.3f %.3f V %.3f A %.3f %.3f 0 0 1 %.3f %.3f Z") % (
        l + rr, t, l + w - rr, rr, rr, l + w, t + rr, t + h - rr, rr, rr, l + w - rr, t + h, l + rr,
        rr, rr, l, t + h - rr, t + rr, rr, rr, l + rr, t)

def svg():
    lw, lh, lr = M["lens_w"], M["lens_h"], M["lens_r"]
    ow, oh, orr = M["open_w"], M["open_h"], M["open_r"]
    px, py = cx + ox, cy - oy                       # opening centre on the page
    o = []
    a = o.append
    a('<svg xmlns="http://www.w3.org/2000/svg" width="%gmm" height="%gmm" viewBox="0 0 %g %g" font-family="Arial, Helvetica, sans-serif">' % (PW, PH, PW, PH))
    a('<rect x="0" y="0" width="%g" height="%g" fill="white"/>' % (PW, PH))
    a('<text x="15" y="16" font-size="6" font-weight="bold">AI calculator v15-LCD: window mask cut template (1:1)</text>')
    a('<text x="15" y="23" font-size="3.6">PRINT AT 100 % (“Actual size”, no “fit to page”). Check the 50 mm bar with a ruler before cutting.</text>')
    a('<text x="15" y="28" font-size="3.6">Drawn as seen from the FRONT of the calculator, through the lens. Black = keep. White = cut out.</text>')
    # the mask: black ring between the lens outline and the opening (evenodd)
    a('<path d="%s %s" fill="black" fill-rule="evenodd" stroke="none"/>' % (rrect_path(cx, cy, lw, lh, lr), rrect_path(px, py, ow, oh, orr)))
    # cut lines (thin red over the black edges, so they show on any printer)
    a('<path d="%s" fill="none" stroke="#d00" stroke-width="0.15"/>' % rrect_path(cx, cy, lw, lh, lr))
    a('<path d="%s" fill="none" stroke="#d00" stroke-width="0.15"/>' % rrect_path(px, py, ow, oh, orr))
    # optional smaller outline: fits through the window opening from inside (lens stays glued)
    a('<path d="%s" fill="none" stroke="#08f" stroke-width="0.2" stroke-dasharray="1.2 0.8"/>' % rrect_path(cx, cy, WIN[0] - 0.3, WIN[1] - 0.3, WIN_R))
    # alignment marks: centre lines of the LENS, extended 6 mm outside the outline (line them up with the lens edges' midpoints)
    for x1, y1, x2, y2 in ((cx, cy - lh / 2 - 7, cx, cy - lh / 2 - 1), (cx, cy + lh / 2 + 1, cx, cy + lh / 2 + 7),
                           (cx - lw / 2 - 7, cy, cx - lw / 2 - 1, cy), (cx + lw / 2 + 1, cy, cx + lw / 2 + 7, cy)):
        a('<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" stroke="black" stroke-width="0.25"/>' % (x1, y1, x2, y2))
    # opening centre tick (white on black)
    a('<line x1="%.3f" y1="%.3f" x2="%.3f" y2="%.3f" stroke="#d00" stroke-width="0.2"/>' % (px, cy - lh / 2 - 13.5, px, cy - lh / 2 - 12))
    # labels
    a('<text x="%.2f" y="%.2f" font-size="3" text-anchor="middle">TOP (solar-cell / display-top edge)</text>' % (cx, cy - lh / 2 - 9))
    a('<text x="%.2f" y="%.2f" font-size="3" text-anchor="middle">BOTTOM (towards the keys)</text>' % (cx, cy + lh / 2 + 11))
    a('<text x="%.2f" y="%.2f" font-size="3" text-anchor="end">LEFT = LCD ribbon end</text>' % (cx - lw / 2 - 8, cy - 2))
    a('<text x="%.2f" y="%.2f" font-size="3" text-anchor="end">wide band %.1f</text>' % (cx - lw / 2 - 8, cy + 2.5, (px - ow / 2) - (cx - lw / 2)))
    a('<text x="%.2f" y="%.2f" font-size="3">RIGHT</text>' % (cx + lw / 2 + 8, cy - 2))
    a('<text x="%.2f" y="%.2f" font-size="3">band %.1f</text>' % (cx + lw / 2 + 8, cy + 2.5, (cx + lw / 2) - (px + ow / 2)))
    a('<text x="%.2f" y="%.2f" font-size="2.6" fill="#d00" text-anchor="middle">opening centre (%.2f right of the lens centre)</text>' % (px, cy - lh / 2 - 14.5, ox))
    # dimensions table
    rows = [("Outer edge (= clear lens)", "%.2f x %.2f mm, corner R %.2f" % (lw, lh, lr)),
            ("Opening (LCD active area + %.1f each side)" % ((ow - 42.72) / 2), "%.2f x %.2f mm, corner R %.1f" % (ow, oh, orr)),
            ("Opening position", "%.2f mm right of the lens centre, %.2f up (front view)" % (ox, oy)),
            ("Bands: left / right / top / bottom", "%.2f / %.2f / %.2f / %.2f mm" % ((px - ow / 2) - (cx - lw / 2), (cx + lw / 2) - (px + ow / 2),
                                                                                  (py - oh / 2) - (cy - lh / 2), (cy + lh / 2) - (py + oh / 2))),
            ("Blue dashed (option)", "%.2f x %.2f: fits through the window from inside, lens stays in" % (WIN[0] - 0.3, WIN[1] - 0.3)),
            ("Material", "matte black vinyl or sticker paper, 0.08-0.15 mm")]
    y = cy + lh / 2 + 22
    for k, v in rows:
        a('<text x="15" y="%.2f" font-size="3.3" font-weight="bold">%s</text>' % (y, k))
        a('<text x="95" y="%.2f" font-size="3.3">%s</text>' % (y, v))
        y += 5.5
    # 50 mm scale bar with 10 mm ticks
    y += 6
    a('<line x1="15" y1="%.2f" x2="65" y2="%.2f" stroke="black" stroke-width="0.4"/>' % (y, y))
    for k in range(6):
        a('<line x1="%g" y1="%.2f" x2="%g" y2="%.2f" stroke="black" stroke-width="0.3"/>' % (15 + 10 * k, y - (3 if k in (0, 5) else 1.8), 15 + 10 * k, y))
    a('<text x="70" y="%.2f" font-size="3.6">50 mm: measure this bar. If it is not exactly 50 mm, reprint at 100 %%.</text>' % (y + 1))
    y += 9
    a('<rect x="15" y="%.2f" width="30" height="30" fill="none" stroke="black" stroke-width="0.3"/>' % y)
    a('<text x="50" y="%.2f" font-size="3.3">30 x 30 mm check square</text>' % (y + 16))
    y += 40
    for line in ("Steps: as ../final_assembly/window_mask.md (same lens, same method). Cut the opening first (straight edge + new craft-knife blade), then the outer edge.",
                 "The sticky side goes on the INSIDE face of the lens. The opening is centred left-right (sticker bands 10.8 / 10.8 mm; 8.5 mm of black shows each side in the window).",
                 "Generated by window_mask_template_v15.py from placement.json (v15-LCD final assembly, 2026-10-08)."):
        a('<text x="15" y="%.2f" font-size="3">%s</text>' % (y, line)); y += 5
    a('</svg>')
    return "\n".join(o)

def dxf():
    """R12 DXF (mm): closed POLYLINEs with bulges. Layers CUT_OUTER, CUT_OPENING, OPTION_WINDOW, MARKS. Front view, lens centre at 0,0."""
    out = ["0", "SECTION", "2", "HEADER", "9", "$INSUNITS", "70", "4", "0", "ENDSEC", "0", "SECTION", "2", "ENTITIES"]
    def rr(layer, x, y, w, h, r):
        b = math.tan(math.pi / 8)                    # 90 deg CCW arc
        l, rgt, bt, tp = x - w / 2, x + w / 2, y - h / 2, y + h / 2
        pts = [(rgt - r, bt, b), (rgt, bt + r, 0), (rgt, tp - r, b), (rgt - r, tp, 0), (l + r, tp, b), (l, tp - r, 0),
               (l, bt + r, b), (l + r, bt, 0)]
        out.extend(["0", "POLYLINE", "8", layer, "66", "1", "70", "1"])
        for px, py, bu in pts:
            out.extend(["0", "VERTEX", "8", layer, "10", "%.4f" % px, "20", "%.4f" % py, "42", "%.6f" % bu])
        out.extend(["0", "SEQEND", "8", layer])
    def line(layer, x1, y1, x2, y2):
        out.extend(["0", "LINE", "8", layer, "10", "%.4f" % x1, "20", "%.4f" % y1, "11", "%.4f" % x2, "21", "%.4f" % y2])
    lw, lh, lr = M["lens_w"], M["lens_h"], M["lens_r"]
    rr("CUT_OUTER", 0, 0, lw, lh, lr)
    rr("CUT_OPENING", ox, oy, M["open_w"], M["open_h"], M["open_r"])
    rr("OPTION_WINDOW", 0, 0, WIN[0] - 0.3, WIN[1] - 0.3, WIN_R)
    for x1, y1, x2, y2 in ((0, lh / 2 + 1, 0, lh / 2 + 7), (0, -lh / 2 - 1, 0, -lh / 2 - 7), (-lw / 2 - 1, 0, -lw / 2 - 7, 0), (lw / 2 + 1, 0, lw / 2 + 7, 0)):
        line("MARKS", x1, y1, x2, y2)
    out.extend(["0", "ENDSEC", "0", "EOF"])
    return "\n".join(out) + "\n"

if __name__ == "__main__":
    open(os.path.join(HERE, "window_mask_template_v15.svg"), "w", encoding="utf8").write(svg())
    open(os.path.join(HERE, "window_mask_template_v15.dxf"), "w", encoding="ascii").write(dxf())
    print("opening %.2f x %.2f at +%.3f/+%.3f from the lens centre; lens %.2f x %.2f R%.2f" % (
        M["open_w"], M["open_h"], ox, oy, M["lens_w"], M["lens_h"], M["lens_r"]))
