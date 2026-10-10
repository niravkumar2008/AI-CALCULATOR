"""Inline-SVG diagrams for the Shell Grinding Guide (grinding_manual.html).

Each diagram is written between the markers <!--dg:NAME--> and <!--/dg:NAME--> in the HTML, so the page stays one
self-contained file and the drawings can be regenerated after a number changes:

    python grind_diagrams.py

The drawings are schematic (not to scale unless they say so). Colours come from the page's CSS tokens
(.dg classes), so they follow light/dark mode. Numbers come from the guide / verification 07 + 12 / v15-LCD REPORT /
variant_camera_dcxyx/results.json.
"""
import math
import os
import re
import textwrap

HTML = os.path.join(os.path.dirname(os.path.abspath(__file__)), "grinding_manual.html")


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


class D:
    def __init__(self, w, h, title):
        self.w, self.h, self.title, self.e = w, h, title, []

    def add(self, s):
        self.e.append(s)

    def rect(self, x, y, w, h, cls="pl", rx=0):
        self.add(f'<rect x="{x:g}" y="{y:g}" width="{w:g}" height="{h:g}" rx="{rx:g}" class="{cls}"/>')

    def line(self, x1, y1, x2, y2, cls="ln", end=None, start=None):
        m = (f' marker-end="url(#{end})"' if end else "") + (f' marker-start="url(#{start})"' if start else "")
        self.add(f'<line x1="{x1:g}" y1="{y1:g}" x2="{x2:g}" y2="{y2:g}" class="{cls}"{m}/>')

    def path(self, d, cls="pl"):
        self.add(f'<path d="{d}" class="{cls}"/>')

    def poly(self, pts, cls="pl"):
        self.path("M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in pts) + " Z", cls)

    def circle(self, cx, cy, r, cls="pl"):
        self.add(f'<circle cx="{cx:g}" cy="{cy:g}" r="{r:g}" class="{cls}"/>')

    def text(self, x, y, s, cls="", anchor="start"):
        lines = s.split("\n")
        if len(lines) == 1:
            self.add(f'<text x="{x:g}" y="{y:g}" text-anchor="{anchor}" class="{cls}">{esc(s)}</text>')
            return
        spans = "".join(f'<tspan x="{x:g}" dy="{0 if i == 0 else 1.2:g}em">{esc(t)}</tspan>' for i, t in enumerate(lines))
        self.add(f'<text x="{x:g}" y="{y:g}" text-anchor="{anchor}" class="{cls}">{spans}</text>')

    def note(self, y, s, width=66):
        """Centred footnote, wrapped to fit the 420-wide frame."""
        self.text(self.w / 2, y, "\n".join(textwrap.wrap(s, width)), "ts", "middle")

    def dimh(self, x1, x2, y, label, ext_from=None, up=True):
        if ext_from is not None:
            self.line(x1, ext_from, x1, y + (-4 if up else 4), "ext")
            self.line(x2, ext_from, x2, y + (-4 if up else 4), "ext")
        self.line(x1, y, x2, y, "dim", end="ad", start="ad")
        self.text((x1 + x2) / 2, y - 5 if up else y + 14, label, "tm", "middle")

    def dimv(self, y1, y2, x, label, side="r", ext_from=None):
        if ext_from is not None:
            self.line(ext_from, y1, x + (4 if side == "r" else -4), y1, "ext")
            self.line(ext_from, y2, x + (4 if side == "r" else -4), y2, "ext")
        self.line(x, y1, x, y2, "dim", end="ad", start="ad")
        if side == "r":
            self.text(x + 6, (y1 + y2) / 2 + 4, label, "tm")
        else:
            self.text(x - 6, (y1 + y2) / 2 + 4, label, "tm", "end")

    def lead(self, x1, y1, x2, y2, red=False):
        self.line(x1, y1, x2, y2, "lr" if red else "lead", end="ar" if red else "al")

    def svg(self):
        body = "\n  ".join(self.e)
        return (f'<svg class="dg" viewBox="0 0 {self.w} {self.h}" role="img" aria-label="{esc(self.title)}">'
                f"<title>{esc(self.title)}</title>\n  {body}\n</svg>")


# ---------------------------------------------------------------- measuring ------------------------------------------

def rim_method():
    d = D(420, 262, "Ruler across the rim: compare the depth-rod reading over the floor (R) with the reading over the feature (F)")
    d.path("M30,70 H44 V190 H376 V70 H390 V206 H30 Z", "nv")
    d.rect(232, 130, 22, 60, "cut")
    d.text(262, 160, "wall to grind", "ts")
    d.rect(10, 58, 400, 12, "tool")
    d.text(400, 52, "steel ruler on the rim", "ts", "end")
    d.dimv(58, 70, 20, "t", "l")
    d.rect(100, 22, 30, 36, "tool")
    d.line(115, 70, 115, 190, "rod")
    d.text(115, 16, "R", "tb", "middle")
    d.text(124, 110, "R = reading over\nthe bare floor", "ts")
    d.rect(228, 22, 30, 36, "tool")
    d.line(243, 70, 243, 130, "rod")
    d.text(243, 16, "F", "tb", "middle")
    d.text(270, 95, "F = reading over\nthe part you grind", "ts")
    d.note(226, "F smaller than R: plastic still standing. F = R: flush, stop. F bigger than R: you are into the floor. (Not to scale; the rod passes just beside the ruler's edge.)", 70)
    return d.svg()


def depth_rod():
    d = D(420, 236, "How to take a depth-rod reading with digital calipers")
    d.path("M40,160 H60 V196 H360 V160 H380 V212 H40 Z", "pl")  # part below, in section
    d.rect(30, 150, 360, 10, "tool")                             # ruler on the rims
    d.rect(170, 20, 26, 130, "tool")                             # beam: end face rests on the ruler
    d.rect(196, 40, 92, 52, "tool", 4)
    d.rect(204, 50, 76, 22, "disp", 2)
    d.text(242, 67, "6.95", "tdisp", "middle")
    d.line(183, 160, 183, 196, "rod")
    d.circle(183, 196, 2.5, "dot")
    d.text(24, 34, "1  Close the jaws,\n    press ZERO.", "ts")
    d.text(24, 96, "2  End face flat on\n    the ruler, upright.", "ts")
    d.text(298, 112, "3  Rod down until\n    it just touches.", "ts")
    d.note(230, "4  Read it. Take 3 readings a few mm apart; trust the middle one.", 70)
    return d.svg()


def good_bad():
    d = D(420, 200, "Good and bad grinds in cross-section")
    titles = (("GOOD: flush", "tg"), ("BAD: stub left", "tr"), ("BAD: gouge", "tr"))
    for i, x0 in enumerate((10, 150, 290)):
        d.text(x0 + 60, 22, titles[i][0], titles[i][1] + " tb", "middle")
        if i == 0:
            d.path(f"M{x0},120 H{x0 + 120} V140 H{x0} Z", "nv")
            d.text(x0 + 60, 112, "F = R", "tm", "middle")
            d.text(x0 + 60, 162, "floor 1.0 mm\nleft as it was", "ts", "middle")
        elif i == 1:
            d.path(f"M{x0},120 H{x0 + 45} V112 H{x0 + 75} V120 H{x0 + 120} V140 H{x0} Z", "nv")
            d.rect(x0 + 35, 50, 50, 56, "part")
            d.text(x0 + 60, 82, "J4", "tb", "middle")
            d.text(x0 + 60, 162, "0.5 mm stub: J4 has\nonly 0.41 mm. It hits.", "ts", "middle")
        else:
            d.path(f"M{x0},120 H{x0 + 35} Q{x0 + 60},134 {x0 + 85},120 H{x0 + 120} V140 H{x0} Z", "nv")
            d.text(x0 + 60, 112, "F > R", "tm", "middle")
            d.text(x0 + 60, 162, "floor under 0.8 mm:\nweak, may crack", "ts", "middle")
    return d.svg()


def tool_angle():
    d = D(420, 232, "How to hold the rotary tool: burr at an angle, sanding drum flat, never the tip straight down")

    def tool(cx, cy, ang, drum=False):
        a = math.radians(ang)
        ux, uy = math.cos(a), -math.sin(a)
        px, py = -uy, ux
        L, W = 95, 22
        bx, by = cx + ux * 16, cy + uy * 16
        d.poly([(bx + px * W / 2, by + py * W / 2), (bx + px * W / 2 + ux * L, by + py * W / 2 + uy * L),
                (bx - px * W / 2 + ux * L, by - py * W / 2 + uy * L), (bx - px * W / 2, by - py * W / 2)], "tool")
        r = 7 if drum else 4
        d.poly([(cx + px * r - ux * 4, cy + py * r - uy * 4), (cx + px * r + ux * 16, cy + py * r + uy * 16),
                (cx - px * r + ux * 16, cy - py * r + uy * 16), (cx - px * r - ux * 4, cy - py * r - uy * 4)], "bit")

    d.rect(10, 140, 125, 14, "nv")
    tool(66, 134, 35)
    d.text(72, 30, "Burr: 30-45 deg,\nside of the burr cuts", "ts", "middle")
    d.text(72, 176, "move it side to side,\nlight touch", "ts", "middle")
    d.rect(148, 140, 125, 14, "nv")
    tool(200, 133, 0, True)
    d.text(210, 30, "Sanding drum: axis\nflat to the surface", "ts", "middle")
    d.text(210, 176, "it rides along like\na rolling pin", "ts", "middle")
    d.rect(286, 140, 125, 14, "nv")
    tool(348, 136, 90)
    d.line(320, 40, 376, 96, "lr")
    d.line(376, 40, 320, 96, "lr")
    d.text(348, 176, "never the tip straight\ndown into the floor", "tr ts", "middle")
    d.note(214, "Speed 5,000-10,000 RPM (low end of the dial). Shiny, stringy or soft plastic = too fast: stop, cool, slow down.", 70)
    return d.svg()


# ---------------------------------------------------------------- zones ----------------------------------------------

def z1_top():
    d = D(420, 236, "Zone 1 top view: the solar box, about 33 by 12 mm, all of it removed")
    s, x0, y0 = 8.5, 50, 56
    d.rect(20, 30, 390, 170, "nvl")
    d.rect(x0, y0, 33 * s, 12 * s, "cut")
    for k in range(1, 5):
        d.line(x0 + k * 33 * s / 5, y0, x0 + k * 33 * s / 5, y0 + 12 * s, "lcut")
    d.circle(372, 76, 13, "keep")
    d.circle(372, 76, 4, "hole")
    d.text(372, 106, "screw post:\nleave it", "tg ts", "middle")
    d.dimh(x0, x0 + 33 * s, 46, "~ 33 mm", y0)
    d.dimv(y0, y0 + 12 * s, 38, "12", "l")
    d.text(x0 + 33 * s / 2, y0 + 12 * s + 22, "remove ALL of it: frame + grid", "tr", "middle")
    d.note(218, "Model: frame 7.2-19.2 mm in from the cover's top outer edge, near end 10.7 mm from the side edge.", 70)
    return d.svg()


def z1_sec():
    d = D(420, 236, "Zone 1 cross-section: 5 to 5.5 mm tall walls on a 1.0 mm floor; J4 needs them gone with no stub")
    fy, k = 170, 14
    d.rect(10, fy, 400, 14, "nv")
    for x in (110, 165, 215, 255, 300, 333):
        d.rect(x, fy - 5.5 * k, 7, 5.5 * k, "cut")
    d.rect(0, 20, 420, 12, "board")
    d.text(410, 15, "board", "ts", "end")
    d.rect(150, 32, 120, fy - 0.41 * k - 32, "partd")
    d.text(210, 52, "J4 (battery socket)", "tb", "middle")
    d.text(210, 68, "5.5 mm tall", "ts", "middle")
    d.dimv(fy - 5.5 * k, fy, 360, "5-5.5", "r")
    d.text(12, 128, "after zone 1:\n0.41 mm gap\nunder J4", "ts")
    d.lead(70, 156, 148, fy - 3)
    d.text(12, fy + 30, "floor 1.0 mm (keep at least 0.8)", "ts")
    d.note(222, "Red = remove. Without zone 1, J4 runs 4.6 mm into these walls.", 70)
    return d.svg()


def z2_top():
    d = D(420, 240, "Zone 2 top view: only the middle 14 mm of rib B comes off, centred on the camera spot")
    s, cx, ry = 6, 236, 120
    d.rect(10, 30, 400, 170, "nvl")
    d.rect(24, ry - 3, 380, 6, "rib")
    d.rect(cx - 7 * s, ry - 3, 14 * s, 6, "cut")
    d.rect(cx - 7 * s - 15 * s, ry - 3, 15 * s, 6, "opt")
    d.line(cx, 34, cx, 196, "cl")
    d.text(cx + 4, 46, "width centre line", "ts")
    d.circle(cx, ry, 3.5 * s, "hole2")
    d.rect(cx - 4.25 * s, ry - 4.25 * s, 8.5 * s, 8.5 * s, "ghost")
    d.dimh(cx - 7 * s, cx + 7 * s, 82, "14 mm", ry - 4)
    d.dimh(cx - 7 * s, cx, 164, "7", ry + 4, up=False)
    d.dimh(cx, cx + 7 * s, 164, "7", ry + 4, up=False)
    d.text(92, 62, "v15-LCD only, if the tail\nbow is squeezed: ~15 mm more", "to ts", "middle")
    d.lead(110, 88, cx - 7 * s - 7.5 * s, ry - 6)
    d.text(cx + 50, ry + 30, "camera 8.5 x 8.5\nand the 7 mm window", "ts")
    d.note(222, "Camera spot on rib B: 38.0 mm down from the back cover's own top outer edge, on the centre line (model). Check it on the template.", 70)
    return d.svg()


def z2_sec():
    d = D(420, 236, "Zone 2 cross-section along rib B: 1 mm of rib under the camera lens comes off")
    fy, k, s, cx = 170, 14, 7, 210
    d.rect(10, fy, 400, 14, "nv")
    d.rect(10, fy - k, 400, k, "rib")
    d.rect(cx - 7 * s, fy - k, 14 * s, k, "cut")
    d.rect(cx - 4.25 * s, 52, 8.5 * s, 66, "partd")
    d.rect(cx - 2.6 * s, 118, 5.2 * s, fy - 0.71 * k - 118, "lens")
    d.text(cx, 88, "camera", "tb", "middle")
    d.text(cx + 26, 140, "lens", "ts")
    d.dimh(cx - 7 * s, cx + 7 * s, 40, "14 mm", 46)
    d.dimv(fy - k, fy, 400, "1.0", "l")
    d.text(14, fy - k - 6, "rib B", "ts")
    d.text(14, fy + 30, "floor 1.0 mm under the rib", "ts")
    d.note(222, "Before: the lens hits the rib by ~0.5 mm. After: 0.71 mm clear.", 70)
    return d.svg()


def z3_top():
    d = D(420, 236, "Zone 3, seen from outside: one 7 mm hole on the width centre line, over the middle of the ground rib-B gap")
    s, cx, cy = 10, 200, 100
    d.rect(10, 20, 400, 170, "nvl")
    d.rect(cx - 70, cy - 55, 140, 110, "tape")
    d.text(cx + 76, cy - 40, "masking tape on\nthe outside: less\nchipping", "ts")
    d.circle(cx, cy, 3.5 * s, "cutc")
    d.circle(cx, cy, 1.0 * s, "pilot")
    d.line(cx - 50, cy, cx + 50, cy, "cl")
    d.line(cx, cy - 50, cx, cy + 50, "cl")
    d.dimh(cx - 35, cx + 35, cy + 66, "7.0 (9/32 in = 7.1 OK)", cy + 38, up=False)
    d.text(24, 46, "pilot 2 mm first", "ts")
    d.lead(80, 52, cx - 11, cy - 4)
    d.note(218, "Drill only after zone 2, and only once the camera route is decided (v15 DCXYX: see the v15 section).", 70)
    return d.svg()


def z3_sec():
    d = D(420, 230, "Zone 3 drilling in three steps: pilot, open to 7 mm, deburr")
    for i, (x0, title) in enumerate(((10, "1  Pilot 2 mm"), (150, "2  Open to 7 mm"), (290, "3  Deburr"))):
        d.text(x0 + 60, 20, title, "tb", "middle")
        d.rect(x0, 150, 120, 26, "wood")
        d.text(x0 + 60, 168, "scrap wood", "ts", "middle")
        if i == 0:
            d.path(f"M{x0},136 H{x0 + 55} V150 H{x0} Z M{x0 + 65},136 H{x0 + 120} V150 H{x0 + 65} Z", "nv")
            d.rect(x0 + 56, 40, 8, 106, "bit")
            d.text(x0 + 60, 196, "straight up\nand down", "ts", "middle")
        elif i == 1:
            d.path(f"M{x0},136 H{x0 + 25} V150 H{x0} Z M{x0 + 95},136 H{x0 + 120} V150 H{x0 + 95} Z", "nv")
            d.path(f"M{x0 + 30},40 H{x0 + 90} V122 L{x0 + 60},140 L{x0 + 30},122 Z", "bit")
            d.text(x0 + 60, 196, "slow; ease off\nat the end", "ts", "middle")
        else:
            d.path(f"M{x0},136 H{x0 + 25} V150 H{x0} Z M{x0 + 95},136 H{x0 + 120} V150 H{x0 + 95} Z", "nv")
            d.poly([(x0 + 20, 132), (x0 + 30, 126), (x0 + 72, 56), (x0 + 62, 50)], "tool")
            d.text(x0 + 60, 196, "file or sandpaper;\ndon't widen it", "ts", "middle")
    return d.svg()


def z5_top():
    d = D(420, 236, "Zone 5: the U-notch seen from outside the top wall, shell face down, rim up")
    s = 4.6
    x1 = 400
    x0 = x1 - 80 * s
    rim = 90
    d.path(f"M{x0},{rim} H{x1 - 27.9 * s} V{rim + 7.95 * s} H{x1 - 6.4 * s} V{rim} H{x1} V{rim + 11 * s} H{x0} Z", "pl")
    d.rect(x1 - 27.9 * s, rim, 21.5 * s, 7.95 * s, "cut")
    d.rect(x1 - 27.65 * s, rim + 0.45 * s, 21 * s, 7 * s, "ghost")
    d.text(x1 - 17.15 * s, rim + 22, "magnet face", "ts", "middle")
    d.dimh(x1 - 27.9 * s, x1 - 6.4 * s, rim - 12, "21.5", rim)
    d.dimh(x1 - 6.4 * s, x1, rim - 12, "6.4", rim)
    d.dimh(x1 - 27.9 * s, x1, rim - 44, "27.9", rim - 16)
    d.dimv(rim, rim + 7.95 * s, x1 - 27.9 * s - 14, "7.95", "l")
    d.text(x1, rim - 66, "measured from this outer edge (solar-window side)", "ts", "end")
    d.text(x0 + 4, rim - 6, "rim (parting line)", "ts")
    d.text(x0 + 4, rim + 11 * s + 18, "display face down on the bench", "ts")
    d.note(200, "Between the notch and the centre are the comb snap tabs: leave them. The corner screw post sits just behind the notch's outer end.", 70)
    return d.svg()


def z5_sec():
    d = D(420, 230, "Zone 5 cross-section: the notch goes right through the 1.2 mm wall; the magnet face ends flush with the outside")
    k, rim = 12, 40
    d.rect(0, 18, 420, 22, "nvg")
    d.text(410, 14, "back cover (on top when face down)", "ts", "end")
    d.path(f"M230,{rim} H244.4 V{rim + 12 * k} H230 Z", "pl")
    d.rect(230, rim, 14.4, 7.95 * k, "cut")
    d.rect(150, rim + 0.45 * k, 94.4, 7 * k, "partd")
    d.text(192, rim + 4 * k, "magnet", "tb", "middle")
    d.line(160, rim + 7.45 * k, 160, rim + 10 * k, "ln")
    d.line(175, rim + 7.45 * k, 175, rim + 10 * k, "ln")
    d.text(168, rim + 11.3 * k, "legs to J3", "ts", "middle")
    d.dimv(rim, rim + 7.95 * k, 300, "7.95 notch", "r", 246)
    d.dimv(rim + 0.45 * k, rim + 7.45 * k, 120, "face 0.45-7.45\nbelow the rim", "l", 148)
    d.dimh(230, 244.4, rim + 12 * k + 14, "1.2 wall", None, up=False)
    d.text(320, rim + 11 * k, "outside", "ts")
    d.text(40, rim + 11 * k, "inside", "ts")
    return d.svg()


def z5b():
    d = D(420, 236, "Zone 5b: if the back cover has a locating lip at the top end, cut 21.5 mm of it to the rim, in line with the notch")
    k, rim = 12, 70
    d.rect(0, rim - 30, 420, 30, "nvg")
    d.text(10, rim - 36, "back cover rim (seen from the side)", "ts")
    d.path(f"M230,{rim} H244.4 V{rim + 11 * k} H230 Z", "pl")
    d.rect(150, rim + 0.45 * k, 94.4, 7 * k, "partd")
    d.rect(206, rim, 14, 1.5 * k, "cut")
    d.text(180, rim + 5.5 * k, "magnet", "tb", "middle")
    d.text(60, rim + 16, "lip? (model\nguess 1.5 mm)", "to ts", "middle")
    d.lead(96, rim + 12, 204, rim + 8)
    d.text(256, rim + 20, "the magnet's top edge is\n0.45 below the rim: a 1.5\nlip hits it by ~0.5 mm", "ts")
    d.note(214, "No lip at the top end of your cover? Skip 5b. Lip there? Burr or snip it off, 21.5 mm wide, level with the rim.", 70)
    return d.svg()


def z6_top():
    d = D(420, 262, "Zone 6 top view along the solar-side wall: the rib, its pins and hooks, and what reaches into it")
    s, x0 = 5.5, 34
    X = lambda mm: x0 + mm * s
    d.rect(X(0), 40, 65 * s, 10, "pl")
    d.text(X(0), 34, "outer skin (don't touch)", "ts")
    d.rect(X(0), 50, 65 * s, 30, "chan")
    d.text(X(1), 69, "wire channel", "ts")
    d.rect(X(0), 80, 58.4 * s, 7, "rib")
    d.rect(X(15), 80, 35 * s, 7, "cut")
    for mm in (26, 35, 45):
        d.rect(X(mm) - 4, 87, 8, 9, "cut")
    for mm in (17, 22):
        d.path(f"M{X(mm) - 6},87 v10 h12 v-10", "lcut")
    d.path(f"M{X(56)},87 v10 h{3 * s} v-10", "lcut")
    d.path(f"M{X(15)},88 H{X(27)} V128 H{X(15)}", "bo")
    d.rect(X(31), 86, 19 * s, 22, "tab")
    d.text(X(21), 110, "board\ncorner", "ts", "middle")
    d.text(X(40.5), 122, "antenna tab", "ts", "middle")
    d.text(X(40.5), 135, "(0.8 mm bare PCB)", "ts", "middle")
    d.text(X(61), 116, "rib end\n(6c)", "to ts", "middle")
    for mm in range(0, 66, 5):
        d.line(X(mm), 176, X(mm), 182 if mm % 10 else 186, "ln")
        if mm % 10 == 0:
            d.text(X(mm), 198, str(mm), "tm", "middle")
    d.line(X(0), 176, X(65), 176, "ln")
    d.text(X(65), 212, "mm from the top outer edge", "ts", "end")
    d.dimh(X(15), X(50), 162, "zone 6: 15 to 50 mm", 96)
    d.note(232, "Red: rib top filed down (zone 6) and the pins + hooks snipped (6b). Board corner 15-27 mm, antenna tab 31-50 mm.", 70)
    return d.svg()


def z6_sec():
    d = D(420, 336, "Zone 6 cross-section across the wall: rib top 4.5 to 5.0 below the rim, board 5.5 to 6.3, file the rib to 7.0")
    k, rim = 24, 40
    Y = lambda mm: rim + mm * k
    d.rect(0, rim - 10, 420, 10, "tool")
    d.text(410, rim - 14, "ruler across the rim", "ts", "end")
    d.path(f"M20,{rim} H36 V{Y(9.4)} H20 Z", "pl")
    d.rect(36, Y(9.0), 130, 0.4 * k, "pl")
    d.rect(150, Y(4.5), 16, Y(9.0) - Y(4.5), "pl")
    d.rect(150, Y(4.5), 16, Y(7.0) - Y(4.5), "cut")
    d.rect(166, Y(5.1), 16, 0.35 * k, "cut")
    d.rect(155, Y(5.5), 135, 0.8 * k, "board")
    d.text(225, Y(5.5) + 14, "board", "tb", "middle")
    d.line(158, rim, 158, Y(7.0), "rod")
    d.line(140, Y(7.0), 176, Y(7.0), "lr")
    sx = 300
    d.line(sx, rim, sx, Y(7.4), "ln")
    for mm, lab in ((4.75, "4.5-5.0 rib now"), (5.5, "5.5 board"), (6.3, "6.3 board"), (7.0, "7.0 target")):
        d.line(sx - 4, Y(mm), sx + 4, Y(mm), "ln")
        d.text(sx + 7, Y(mm) + 4, lab, "tm" + (" tr" if mm == 7.0 else ""))
    for mm, x in ((4.5, 166), (6.3, 290), (7.0, 176)):
        d.line(x, Y(mm), sx - 4, Y(mm), "ext")
    d.text(46, 92, "pin (6b) on the\nrib's inner face", "ts")
    d.lead(110, 116, 172, Y(5.1) + 2)
    d.text(46, 160, "depth rod: 7.0\nwhen you're done", "ts")
    d.lead(100, 182, 156, Y(7.0) - 4)
    d.text(44, Y(9.0) - 6, "channel floor", "ts")
    d.text(14, Y(9.4) + 18, "outer skin", "ts")
    d.text(140, Y(9.4) + 18, "inner rib (1 mm)", "ts")
    d.note(Y(9.4) + 40, "6.5 is the bare minimum (the board's far face is at 6.3); 7.0 leaves 0.7 mm of air.", 70)
    return d.svg()


def z6b():
    d = D(420, 236, "Zone 6b: flush cutters flat against the rib; good cut versus a stub")
    d.text(100, 20, "Flat side of the cutters\nagainst the rib", "tb", "middle")
    d.rect(30, 50, 14, 140, "pl")
    d.rect(44, 108, 34, 12, "cut")
    d.text(88, 118, "pin", "ts")
    d.path("M46,96 L150,70 L156,84 L52,106 Z", "tool")
    d.path("M46,132 L150,158 L156,144 L52,122 Z", "tool")
    d.text(100, 210, "cut at the root", "ts", "middle")
    d.text(250, 32, "GOOD", "tg tb", "middle")
    d.rect(243, 50, 14, 140, "pl")
    d.text(250, 210, "flush: a fingernail\ndoesn't catch", "ts", "middle")
    d.text(360, 32, "BAD", "tr tb", "middle")
    d.rect(353, 50, 14, 140, "pl")
    d.rect(367, 108, 8, 12, "cut")
    d.text(380, 104, "stub", "tr ts")
    d.text(360, 210, "file it off", "ts", "middle")
    return d.svg()


def z6c():
    d = D(420, 262, "Zone 6c top view: the board widens 59.5 to 63 mm from the top; if the rib's lower end reaches there, file it back 2 to 3 mm")
    s, y0 = 9, 30
    Y = lambda mm: y0 + (mm - 50) * s
    d.rect(40, y0, 14, Y(68) - y0, "pl")
    d.rect(54, y0, 9, Y(58.4) - y0, "rib")
    d.rect(54, Y(58.4), 9, Y(61.5) - Y(58.4), "cut")
    d.path(f"M120,{y0} V{Y(59.5)} L60,{Y(63)} V{Y(68)}", "bo")
    d.circle(63, Y(62.0), 6, "warnc")
    for mm in (50, 55, 60, 65):
        d.line(20, Y(mm), 34, Y(mm), "ln")
        d.text(16, Y(mm) + 4, str(mm), "tm", "end")
    d.text(134, 44, "board edge beside the screen", "ts")
    d.text(150, Y(56.2), "the rib may run to 60-62 mm:\nfile this end back 2-3 mm", "to ts")
    d.lead(146, Y(57.5), 66, Y(59.8))
    d.text(150, Y(63.6), "the diagonal (board widens)\ncrosses the rib line about\n61.4-62.1 mm from the top", "ts")
    d.lead(146, Y(64.2), 70, Y(62.3))
    d.note(230, "Only if the paper's (or the dummy board's) wide corner rides up after 6b. Depth: 7.0 below the rim, like zone 6.", 70)
    return d.svg()


def v15_bow():
    d = D(420, 276, "v15-LCD: the LCD tail's U-bow passes 0.25 mm over the un-ground part of rib B")
    k, fy = 20, 215
    Z = lambda z: fy - (z - 1.0) * k
    d.rect(0, fy, 420, 14, "nv")
    d.rect(150, Z(2.0), 270, Z(1.0) - Z(2.0), "rib")
    d.text(410, Z(1.0) - 6, "rib B (not ground here)", "ts", "end")
    d.rect(0, Z(7.8), 420, Z(7.0) - Z(7.8), "board")
    d.text(410, Z(7.8) - 6, "board", "ts", "end")
    r = 1.2 * k
    xL, xR, zb = 110, 320, Z(2.25)
    d.path(f"M{xL},{Z(7.0)} V{zb - r} A{r},{r} 0 0 0 {xL + r},{zb} H{xR - r} A{r},{r} 0 0 0 {xR},{zb - r} V{Z(6.42)}", "fpc")
    d.text(215, 130, "LCD tail: one U-bow\n(corners r 1.2 mm)", "ts", "middle")
    d.text(250, 172, "0.25 mm gap", "tm", "middle")
    d.lead(250, 177, 250, Z(2.0) - 2)
    d.text(10, 22, "Tail 36.3 / 36.6 / 36.9 / 37.2 mm", "tm")
    d.text(10, 40, "gap  0.40 / 0.25 / 0.10 / touches", "tm")
    d.note(250, "Tail over ~37 mm, or the bow squeezed? Grind zone 2 about 15 mm further toward the slot side.", 70)
    return d.svg()


def v15_sfold():
    d = D(420, 262, "v15 DCXYX camera: the ribbon's S-fold lies in the bay between rib C and the small ring, 0.44 mm over the floor")
    k, fy = 20, 190
    Z = lambda z: fy - (z - 1.0) * k
    d.rect(0, fy, 420, 14, "nv")
    d.rect(0, Z(7.8), 420, Z(7.0) - Z(7.8), "board")
    d.text(410, Z(7.8) - 6, "board", "ts", "end")
    d.rect(56, Z(2.0), 10, fy - Z(2.0), "rib")
    d.text(61, Z(2.0) - 6, "rib C", "ts", "middle")
    d.rect(350, Z(5.0), 10, fy - Z(5.0), "rib")
    d.text(372, Z(3.0), "small\nring", "ts")
    r = 1.2 * k
    zt, zm, zbm = Z(6.24), Z(3.84), Z(1.44)
    d.path(f"M10,{zt} H280 A{r},{r} 0 0 1 280,{zm} H110 A{r},{r} 0 0 0 110,{zbm} H300 "
           f"C330,{zbm} 320,{Z(5.53)} 350,{Z(5.53)} H415", "fpc")
    d.line(76, fy, 330, fy, "keepl")
    d.text(200, Z(2.55), "0.44 mm over the floor", "tm", "middle")
    d.lead(200, Z(2.35), 200, zbm - 2)
    d.text(10, 22, "camera ribbon: one S-fold, bends r 1.2 mm", "ts")
    d.text(203, fy + 32, "keep this floor bare and flat", "tg", "middle")
    d.note(240, "No stubs, epoxy blobs, tape or debris here: KiCad y 108-116, x 147-153 (within ~3 mm of the centre line).", 70)
    return d.svg()


ALL = dict(rim_method=rim_method, depth_rod=depth_rod, good_bad=good_bad, tool_angle=tool_angle,
           z1_top=z1_top, z1_sec=z1_sec, z2_top=z2_top, z2_sec=z2_sec, z3_top=z3_top, z3_sec=z3_sec,
           z5_top=z5_top, z5_sec=z5_sec, z5b=z5b, z6_top=z6_top, z6_sec=z6_sec, z6b=z6b, z6c=z6c,
           v15_bow=v15_bow, v15_sfold=v15_sfold)


def main():
    html = open(HTML, encoding="utf-8").read()
    n = 0
    for name, fn in ALL.items():
        pat = re.compile(r"(<!--dg:%s-->).*?(<!--/dg:%s-->)" % (name, name), re.S)
        if not pat.search(html):
            print("no marker for", name)
            continue
        svg = fn()
        html = pat.sub(lambda m: m.group(1) + svg + m.group(2), html)
        n += 1
    open(HTML, "w", encoding="utf-8", newline="\n").write(html)
    print(n, "diagrams written")


if __name__ == "__main__":
    main()
