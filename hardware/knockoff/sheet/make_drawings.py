"""Draw the to-scale overview pictures for the knockoff measurement sheet from the CAD geometry.

    python hardware/knockoff/sheet/make_drawings.py

Reads ../cad/geometry.json (written by build_knockoff_shell.py), fills the <!--FRONT--> and <!--BACK-->
markers in template.html and writes knockoff_measurement_sheet.html next to it.
"""
import json, math, pathlib

HERE = pathlib.Path(__file__).parent
G = json.loads((HERE.parent / "cad" / "geometry.json").read_text())
S = 4.0                    # px per mm
TOP = G["top"]             # Y of the case top edge
PAD = 58                   # px margin round the outline (room for the call-outs)
KO_CUPS = ((-12.4, 69.94), (-24.8, 69.94))       # same as build_knockoff_shell.py
SOLAR_C, SOLAR = (12.0, 67.34), (34.0, 11.0)
WIN = (60.65, 24.3)
CAM = (0.0, 138.94 - 95.1)                       # camera window, KiCad (150, 95.1)
DOOR_C, DOOR, BATT_HOLE_D = (-18.3, 69.94), (16.0, 15.0), 12.0


def make(mirror):
    xs = [p[0] for p in G["Vb_s"]]
    half = max(abs(min(xs)), abs(max(xs)))
    ox = PAD + half * S

    def X(x):
        return ox + (-x if mirror else x) * S

    def Y(y):
        return PAD + (TOP - y) * S

    def path(P):
        return "M" + " L".join("%.1f,%.1f" % (X(x), Y(y)) for x, y in P) + " Z"

    def pt(p):
        return X(p[0]), Y(p[1])

    w = 2 * PAD + 2 * half * S
    ys = [p[1] for p in G["Vb_s"]]
    h = 2 * PAD + (TOP - min(ys)) * S
    return X, Y, path, pt, w, h


def rrect(cx, cy, w, h, r, cls):
    return '<rect class="%s" x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="%.1f"/>' % (cls, cx - w / 2, cy - h / 2, w, h, r)


def circle(c, r, cls):
    return '<circle class="%s" cx="%.1f" cy="%.1f" r="%.1f"/>' % (cls, c[0], c[1], r)


def co(id_, at, label_at, text=None):
    """Numbered call-out: dot on the part, line to a pill with the reading's number."""
    (x, y), (lx, ly) = at, label_at
    t = text or id_.replace("K-", "")
    wpx = 12 + 7.2 * len(t)
    return ('<g class="co" data-id="%s"><line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/><circle class="dot" cx="%.1f" cy="%.1f" r="3"/>'
            '<rect x="%.1f" y="%.1f" width="%.1f" height="20" rx="10"/><text x="%.1f" y="%.1f">%s</text></g>'
            % (id_, x, y, lx, ly, x, y, lx - wpx / 2, ly - 10, wpx, lx, ly + 4.2, t))


def front():
    X, Y, path, pt, w, h = make(mirror=True)
    o = ['<svg class="fig" viewBox="0 0 %.0f %.0f" role="img" aria-label="Knockoff front shell seen from the inside, with numbered measuring spots">' % (w, h)]
    o.append('<path class="shell" d="%s"/>' % path(G["Vf_s"]))
    o.append('<path class="cavity" d="%s"/>' % path(G["C"]))
    for r in G["row_ribs"]:
        (x1, y1), (x2, y2) = pt(r[0]), pt(r[1])
        o.append('<line class="rowrib" x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/>' % (x1, y1, x2, y2))
    for k in G["openings"]:
        c = pt(k["c"])
        if k.get("oval"):
            o.append('<ellipse class="keyo" cx="%.1f" cy="%.1f" rx="%.1f" ry="%.1f"/>' % (c[0], c[1], k["w"] * S / 2, k["h"] * S / 2))
        else:
            o.append(rrect(c[0], c[1], k["w"] * S, k["h"] * S, 1.2 * S, "keyo"))
    o.append(circle(pt(G["replay"]), 15.4 / 2 * S, "keyo"))
    wc = pt(G["win_c"])
    o.append(rrect(wc[0], wc[1], WIN[0] * S, WIN[1] * S, 4, "win"))
    o.append('<text class="lab" x="%.1f" y="%.1f" text-anchor="middle">screen window</text>' % (wc[0], wc[1] + 4))
    for q in G["inner_ribs"]:
        o.append('<path class="rib" d="%s"/>' % path(q))
    for q in G["pins"]:
        o.append('<path class="pinp" d="%s"/>' % path(q))
    sc = pt(SOLAR_C)
    o.append(rrect(sc[0], sc[1], (SOLAR[0] + 2) * S, (SOLAR[1] + 2) * S, 4, "solarframe"))
    o.append(rrect(sc[0], sc[1], SOLAR[0] * S, SOLAR[1] * S, 3, "solar"))
    o.append('<text class="lab inv" x="%.1f" y="%.1f" text-anchor="middle">solar cell</text>' % (sc[0], sc[1] + 4))
    for c in KO_CUPS:
        o.append(circle(pt(c), 7.0 * S, "pocket"))
    for c in KO_CUPS:
        o.append(circle(pt(c), 6.2 * S, "pocketin"))
    pc = pt(((KO_CUPS[0][0] + KO_CUPS[1][0]) / 2, KO_CUPS[0][1]))
    o.append('<text class="lab" x="%.1f" y="%.1f" text-anchor="middle">2 round pockets</text>' % (pc[0], pc[1] + 7.0 * S + 16))
    for n, c in G["screws"]:
        o.append(circle(pt(c), 1.925 * S + 1.5, "spost"))
        o.append(circle(pt(c), 0.85 * S, "phole"))
    for n, c in G["locs"]:
        o.append(circle(pt(c), 1.45 * S + 1, "lpost"))
    # key mat: same outline as the original board (the knockoff mat lines up with the real Casio one)
    bc, BW, BH = G["board_c"], 65.05, 97.9            # C6: the original board, which the mat covers
    mt = bc[1] + BH / 2
    mcx, mcy = pt(bc)
    o.append(rrect(mcx, mcy, BW * S, BH * S, 3 * S, "mat"))
    mc = pt((0.0, mt - 20))
    o.append('<text class="lab" x="%.1f" y="%.1f" text-anchor="middle">key mat (dashed)</text>' % (mcx, Y(bc[1] - BH / 2) - 8))
    # call-outs
    scr = dict((n, c) for n, c in G["screws"])
    mid = pt(scr["mid L"])
    o.append(co("K-M2", mid, (mid[0] + 58, mid[1] - 26)))
    o.append(co("K-M10", pt(scr["bottom L"]), (pt(scr["bottom L"])[0] + 56, pt(scr["bottom L"])[1] - 22)))
    o.append(co("K-M3", pt(KO_CUPS[1]), (pt(KO_CUPS[1])[0] + 40, Y(TOP) - 30)))
    o.append(co("K-M9", (sc[0] - SOLAR[0] * S / 2 - 4, sc[1]), (sc[0] - SOLAR[0] * S / 2 - 40, Y(TOP) - 30)))
    ir = G["inner_ribs"][1]
    rp = pt((ir[0][0], 50.0))
    o.append(co("K-M8", rp, (rp[0] + 46, rp[1] + 30)))
    ir = G["inner_ribs"][0]
    rp = pt((ir[0][0], 50.0))
    o.append(co("K-M8", rp, (rp[0] - 46, rp[1] + 30)))
    cl, cr = pt(scr["corner L"]), pt(scr["corner R"])
    o.append('<g class="dim"><line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" marker-start="url(#arF)" marker-end="url(#arF)"/></g>'
             % (cl[0], cl[1] - 16, cr[0], cr[1] - 16))
    o.append(co("K-M11", ((cl[0] + cr[0]) / 2, cl[1] - 16), ((cl[0] + cr[0]) / 2, Y(TOP) - 30)))
    o.append(co("K-M13", pt((8.0, mt - 46)), (pt((8.0, mt - 46))[0] - 70, pt((8.0, mt - 46))[1] + 26)))
    wb = G["win_c"][1] - WIN[1] / 2
    ga, gb = pt((-14.0, wb)), pt((-14.0, mt))
    o.append('<g class="dim"><line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" marker-start="url(#arF)" marker-end="url(#arF)"/></g>' % (ga[0], ga[1], gb[0], gb[1]))
    o.append(co("K-M17", ((ga[0] + gb[0]) / 2, (ga[1] + gb[1]) / 2), ((ga[0] + gb[0]) / 2 - 150, (ga[1] + gb[1]) / 2)))
    o.append(co("K-M6", (wc[0] + WIN[0] * S / 2 - 30, wc[1] + WIN[1] * S / 2), (wc[0] + WIN[0] * S / 2 - 30, wc[1] + WIN[1] * S / 2 + 22)))
    o.append('<defs><marker id="arF" viewBox="0 0 10 10" refX="5" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
             '<path d="M0 1L9 5L0 9z" class="arh"/></marker></defs>')
    o.append("</svg>")
    return "\n".join(o)


def back():
    X, Y, path, pt, w, h = make(mirror=False)
    o = ['<svg class="fig" viewBox="0 0 %.0f %.0f" role="img" aria-label="Back cover seen from the inside, with numbered measuring spots">' % (w, h)]
    o.append('<path class="shellB" d="%s"/>' % path(G["Vb_s"]))
    o.append('<path class="cavityB" d="%s"/>' % path(G["lip_i"]))
    for n, a, b, t in G["ribs"]:
        (x1, y1), (x2, y2) = pt(a), pt(b)
        o.append('<line class="ribB" x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/>' % (x1, y1, x2, y2))
    for seg in G["box"] + G["grid"]:
        (x1, y1), (x2, y2) = pt(seg[0]), pt(seg[1])
        o.append('<line class="ribB" x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/>' % (x1, y1, x2, y2))
    for n, c, d in G["rings"]:
        o.append(circle(pt(c), d / 2 * S, "ringB"))
    dc = pt(DOOR_C)
    o.append(rrect(dc[0], dc[1], DOOR[0] * S, DOOR[1] * S, 6, "door"))
    o.append(circle(dc, BATT_HOLE_D / 2 * S, "hole"))
    for n, c in G["screws"]:
        o.append(circle(pt(c), 2.75 * S, "boss"))
        o.append(circle(pt(c), 0.85 * S, "phole"))
    cam = pt(CAM)
    o.append('<rect class="camzone" x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="4"/>' % (cam[0] - 4.25 * S, cam[1] - 4.25 * S, 8.5 * S, 8.5 * S))
    o.append(circle(cam, 3.5 * S, "camhole"))
    o.append('<text class="lab" x="%.1f" y="%.1f" text-anchor="middle">camera spot</text>' % (cam[0], cam[1] + 6.5 * S))
    o.append('<g class="dim"><line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" marker-start="url(#arB)" marker-end="url(#arB)"/></g>'
             % (cam[0] + 30, Y(TOP), cam[0] + 30, cam[1]))
    o.append('<text class="lab" x="%.1f" y="%.1f">%.0f mm</text>' % (cam[0] + 36, (Y(TOP) + cam[1]) / 2 + 4, TOP - CAM[1]))
    o.append(co("K-M4", cam, (cam[0] - 64, cam[1] - 18)))
    big = [r for r in G["rings"] if r[0] == "big ring"][0]
    bc = pt(big[1])
    o.append(co("K-M7", (bc[0] + big[2] / 2 * S, bc[1]), (bc[0] + big[2] / 2 * S + 52, bc[1] - 24)))
    o.append('<text class="note" x="%.1f" y="%.1f" text-anchor="middle">fx-115ES layout: your photo K-P1 replaces this</text>' % (w / 2, h - 8))
    o.append('<defs><marker id="arB" viewBox="0 0 10 10" refX="5" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
             '<path d="M0 1L9 5L0 9z" class="arh"/></marker></defs>')
    o.append("</svg>")
    return "\n".join(o)


if __name__ == "__main__":
    t = (HERE / "template.html").read_text(encoding="utf8")
    t = t.replace("<!--FRONT-->", front()).replace("<!--BACK-->", back())
    (HERE / "knockoff_measurement_sheet.html").write_text(t, encoding="utf8")
    print("wrote", HERE / "knockoff_measurement_sheet.html", len(t), "bytes")
