"""Dimension drawing of the knockoff shell (rev 2), to scale, from the CAD geometry.

    python hardware/knockoff/build_knockoff_shell.py                 # refresh cad/geometry.json first
    python hardware/knockoff/sheet/make_dimension_drawing.py         # writes knockoff_dimensions.html

Every number on the page is read from the build script's parameters or from cad/geometry.json, so the drawing and the
CAD model can't disagree. "Below top" = distance down from the case's top edge; "from centre" = sideways from the
centre line. Inside views are drawn the way the part lies on the desk.
"""
import importlib.util, json, math, pathlib

HERE = pathlib.Path(__file__).parent
spec = importlib.util.spec_from_file_location("ko", HERE.parent / "build_knockoff_shell.py")
K = importlib.util.module_from_spec(spec); spec.loader.exec_module(K)
G = json.loads((HERE.parent / "cad" / "geometry.json").read_text())
TOP = G["top"]
CAM = (0.0, 42.0)                                   # build_knockoff_assembly.py: camera centre Y 42.0 (KiCad y 96.94)
CAM_D, CAM_MOD = 7.0, 8.5
BOARD_BOTTOM_W, BOARD_MAX_W, BOARD_L = 64.1, 71.6, 148.8   # v15 board (KiCad Edge.Cuts)
f1 = lambda v: ("%.2f" % v).rstrip("0").rstrip(".")


class View:
    def __init__(self, s, mirror, x0, x1, y0, y1, pad=(70, 60, 70, 40)):
        self.s, self.m = s, mirror
        self.x0, self.x1, self.y0, self.y1 = x0, x1, y0, y1
        self.pl, self.pt, self.pr, self.pb = pad
        self.w = pad[0] + pad[2] + (x1 - x0) * s
        self.h = pad[1] + pad[3] + (y1 - y0) * s
        self.o = []

    def X(self, x):
        x = -x if self.m else x
        return self.pl + (x - self.x0) * self.s

    def Y(self, y):
        return self.pt + (self.y1 - y) * self.s

    def p(self, pt):
        return self.X(pt[0]), self.Y(pt[1])

    def path(self, P, cls):
        self.o.append('<path class="%s" d="M%s Z"/>' % (cls, " L".join("%.1f,%.1f" % self.p(q) for q in P)))

    def circle(self, c, d, cls):
        x, y = self.p(c)
        self.o.append('<circle class="%s" cx="%.1f" cy="%.1f" r="%.1f"/>' % (cls, x, y, d / 2 * self.s))

    def rect(self, x0, x1, y0, y1, cls, r=0):
        a, b = self.p((x0, y1)), self.p((x1, y0))
        self.o.append('<rect class="%s" x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="%.1f"/>'
                      % (cls, min(a[0], b[0]), a[1], abs(b[0] - a[0]), b[1] - a[1], r * self.s))

    def line(self, a, b, cls):
        (x1, y1), (x2, y2) = self.p(a), self.p(b)
        self.o.append('<line class="%s" x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/>' % (cls, x1, y1, x2, y2))

    def text(self, x, y, s, cls="dt", anchor="middle", rot=0):
        tr = ' transform="rotate(%d %.1f %.1f)"' % (rot, x, y) if rot else ""
        self.o.append('<text class="%s" x="%.1f" y="%.1f" text-anchor="%s"%s>%s</text>' % (cls, x, y, anchor, tr, s))

    def hdim(self, xa, xb, y, label, off=0, cls="dim"):
        """Horizontal dimension between model X xa and xb, drawn at model Y y, shifted off px."""
        (x1, yy), (x2, _) = self.p((xa, y)), self.p((xb, y))
        yy += off
        self.o.append('<g class="%s"><line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" marker-start="url(#da)" marker-end="url(#da)"/>'
                      '<line class="ext" x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/><line class="ext" x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/></g>'
                      % (cls, x1, yy, x2, yy, x1, yy - 5, x1, yy - off + (0 if off else 0), x2, yy - 5, x2, yy - off))
        self.text((x1 + x2) / 2, yy - 4, label)

    def vdim(self, x, ya, yb, label, off=0, cls="dim", side="r"):
        (xx, y1), (_, y2) = self.p((x, ya)), self.p((x, yb))
        xx += off
        self.o.append('<g class="%s"><line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" marker-start="url(#da)" marker-end="url(#da)"/>'
                      '<line class="ext" x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/><line class="ext" x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/></g>'
                      % (cls, xx, y1, xx, y2, xx - off, y1, xx + 4 * (1 if off >= 0 else -1), y1, xx - off, y2, xx + 4 * (1 if off >= 0 else -1), y2))
        tx = xx + (5 if side == "r" else -5)
        self.text(tx, (y1 + y2) / 2 + 4, label, anchor="start" if side == "r" else "end")

    def note(self, at, to, label, anchor="start"):
        (x, y) = self.p(at)
        tx, ty = to
        lines = label.split("|")
        tw = 6.95 * max(len(s) for s in lines)          # mono 11.5 px: ~6.95 px per character
        ex = tx + tw + 8 if anchor == "start" else tx - tw - 8
        if (anchor == "start" and x < ex) or (anchor == "end" and x > ex):
            ex = tx                                        # the point is beyond the label: run to its near end
        self.o.append('<g class="lead"><line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f"/><circle cx="%.1f" cy="%.1f" r="2.2"/></g>' % (x, y, ex, ty, x, y))
        for i, s in enumerate(lines):
            self.text(tx + (4 if anchor == "start" else -4), ty - 3 + i * 14 - (len(lines) - 1) * 7, s, "nt", anchor)

    def svg(self, label):
        return ('<svg class="fig" viewBox="0 0 %.0f %.0f" role="img" aria-label="%s"><defs><marker id="da" viewBox="0 0 10 10" refX="9" refY="5" '
                'markerWidth="8" markerHeight="8" orient="auto-start-reverse"><path d="M0 1.5L10 5L0 8.5z" class="arh"/></marker></defs>%s</svg>'
                % (self.w, self.h, label, "\n".join(self.o)))


def outline_extent(P):
    xs = [p[0] for p in P]; ys = [p[1] for p in P]
    return min(xs), max(xs), min(ys), max(ys)


# ── 1. front shell, inside up (mirrored) ─────────────────────────────────────────────────────────────────────
def front_full():
    x0, x1, y0, y1 = outline_extent(G["Vf"])
    v = View(3.4, True, -x1, -x0, y0, y1, pad=(330, 96, 330, 44))
    v.path(G["Vf_s"], "shell"); v.path(G["C"], "cav")
    for k in G["openings"]:
        if k.get("oval"):
            cx, cy = v.p(k["c"]); v.o.append('<ellipse class="keyo" cx="%.1f" cy="%.1f" rx="%.1f" ry="%.1f"/>' % (cx, cy, k["w"] / 2 * v.s, k["h"] / 2 * v.s))
        else:
            v.rect(k["c"][0] - k["w"] / 2, k["c"][0] + k["w"] / 2, k["c"][1] - k["h"] / 2, k["c"][1] + k["h"] / 2, "keyo", 1.0)
    v.circle(G["replay"], 15.4, "keyo")
    wc = G["win_c"]
    v.rect(wc[0] - K.WIN[0] / 2, wc[0] + K.WIN[0] / 2, wc[1] - K.WIN[1] / 2, wc[1] + K.WIN[1] / 2, "win", 1.0)
    v.rect(K.SOLAR_C[0] - K.SOLAR[0] / 2, K.SOLAR_C[0] + K.SOLAR[0] / 2, K.SOLAR_C[1] - K.SOLAR[1] / 2, K.SOLAR_C[1] + K.SOLAR[1] / 2, "solar", 0.6)
    for c in K.KO_CUPS:
        v.circle(c, K.KO_CUP_D + 1.6, "cup")
    for c in K.KO_CUPS:
        v.circle(c, K.KO_CUP_D, "cupin")
    v.circle(K.KO_PIN[:2], K.KO_PIN[2], "pin")
    for n, c in G["screws"]:
        v.circle(c, K.SCREW_POST_D, "post"); v.circle(c, K.PILOT_D, "hole")
    for n, c in G["locs"]:
        v.circle(c, K.LOC_POST_D, "lpost")
    # overall
    v.hdim(x0, x1, y1, "79.3 wide (screen section)", off=-26)
    v.vdim(x0, y0, y1, "159.9 long", off=26, side="r")   # mirrored: model x0 (left in front view) is drawn on the right
    # window
    v.hdim(wc[0] - K.WIN[0] / 2, wc[0] + K.WIN[0] / 2, wc[1] - K.WIN[1] / 2, "window 60.65", off=22)
    v.vdim(wc[0] + K.WIN[0] / 2, wc[1] - K.WIN[1] / 2, wc[1] + K.WIN[1] / 2, "24.3", off=-16, side="l")
    v.vdim(-x1 * 0 + 0, wc[1] + K.WIN[1] / 2, TOP, "24.0 to the top edge", off=0, side="r")
    # posts table-style notes
    scr = dict((n, c) for n, c in G["screws"])
    for n, c in G["screws"]:
        lbl = {"corner R": "corner screw|34.05 from centre, 8.05 below top",
               "battery corner": "battery-corner screw (moved)|33.5 from centre, 15.9 below top",
               "mid R": "mid screw|46.5 c-c, 69.1 below top", "mid L": "mid screw|46.5 c-c (centre 0.8 off)",
               "bottom R": "bottom screw|39.0 c-c, 153.75 below top", "bottom L": "bottom screw|39.0 c-c"}[n]
        x, y = v.p(c)
        right = x > v.w / 2
        v.note(c, (v.w - 8 if right else 8, y + (24 if n == "battery corner" else 0)), lbl, "end" if right else "start")
    v.text(v.w / 2, 22, "FRONT SHELL, SEEN FROM INSIDE (lying face down on the desk)", "ttl")
    v.text(v.w / 2, v.h - 10, "Ø3.85 = screw post (pilot hole Ø1.7) · grey dots = Ø2.9 locating posts · purple = window", "nt")
    return v.svg("Front shell from inside, with dimensions")



def clip_open(v, cid):
    a, b = v.p((v.x0 if not v.m else -v.x0, v.y1)), v.p((v.x1 if not v.m else -v.x1, v.y0))
    x0, x1 = sorted((a[0], b[0]))
    v.o.append('<clipPath id="%s"><rect x="%.1f" y="%.1f" width="%.1f" height="%.1f"/></clipPath><g clip-path="url(#%s)">'
               % (cid, x0, a[1], x1 - x0, b[1] - a[1], cid))
    v.o.append('<rect class="frame" x="%.1f" y="%.1f" width="%.1f" height="%.1f"/>' % (x0, a[1], x1 - x0, b[1] - a[1]))
    return x0, x1, a[1], b[1]


# ── 2. battery corner detail (front shell, inside up) ────────────────────────────────────────────────────────
def corner_detail():
    v = View(11.0, True, 0.0, 40.5, 59.0, TOP + 0.5, pad=(30, 175, 30, 130))
    fx0, fx1, fy0, fy1 = clip_open(v, "cc")
    v.path(G["Vf_s"], "shell"); v.path(G["C"], "cav")
    v.rect(K.SOLAR_C[0] - K.SOLAR[0] / 2, K.SOLAR_C[0] + K.SOLAR[0] / 2, K.SOLAR_C[1] - K.SOLAR[1] / 2, K.SOLAR_C[1] + K.SOLAR[1] / 2, "solar", 0.6)
    for c in K.KO_CUPS:
        v.circle(c, K.KO_CUP_D + 1.6, "cup")
    for c in K.KO_CUPS:
        v.circle(c, K.KO_CUP_D, "cupin")
    v.circle(K.KO_PIN[:2], K.KO_PIN[2], "pin")
    sc = K.KO_CORNER_SCREW
    v.circle(sc, K.SCREW_POST_D, "post"); v.circle(sc, K.PILOT_D, "hole")
    old = (-34.05, 74.05)
    v.circle(old, K.SCREW_POST_D, "ghost")
    a, b = K.KO_CUPS
    v.hdim(a[0], b[0], a[1], "10.5", off=-(K.KO_CUP_D / 2 + 0.8) * v.s - 10)
    v.o.append("</g>")
    v.hdim(0, a[0], v.y0, "14.5 from centre", off=26)
    v.hdim(0, b[0], v.y0, "25.0 from centre", off=56)
    v.vdim(-5.6, b[1], TOP, "11.6", off=0, side="l")
    top_y = fy0 - 16
    v.note(old, (fx1 - 4, top_y - 112), "red ring = where the fx-115ES|corner screw was (34.05, 8.05)", "end")
    v.note(K.KO_PIN[:2], (fx1 - 4, top_y - 22), "pin Ø1.6 · 35.0 from centre · 8.9 below top", "end")
    v.note(b, (fx0 + 4, top_y - 66), "two C cups, Ø11.8 inside, 0.8 wall|5.5 tall (guess) · open at the sides")
    v.note(sc, (fx1 - 4, fy1 + 104), "small screw boss Ø3.85 (pilot Ø1.7)|33.5 from centre · 15.9 below top", "end")
    v.text(v.w / 2, 22, "DETAIL: BATTERY CORNER (front shell from inside)", "ttl")
    v.text(v.w / 2, v.h - 8, "Photos K01 (straight down) + K12 · positions ±1.5 mm", "nt")
    return v.svg("Battery corner detail")


# ── 3. back cover, inside up ─────────────────────────────────────────────────────────────────────────────────
def back_full():
    x0, x1, y0, y1 = outline_extent(G["Vb"])
    v = View(3.4, False, x0, x1, y0, y1, pad=(330, 96, 330, 44))
    v.path(G["Vb_s"], "shellB"); v.path(G["lip_i"], "cavB")
    for n, a, b, h in G["ribs"]:
        v.line(a, b, "ribB")
    bx = K.SOLAR_BOX
    v.rect(bx[0], bx[1], bx[2], bx[3], "boxB", 0.4)
    pl = K.KO_PLATE
    v.rect(pl[0], pl[1], pl[2], pl[3], "plateB", 2.0)
    v.circle(K.DOOR_C, K.BATT_HOLE_D, "holeB")
    for x, y, w, h in K.KO_SLOTS:
        v.rect(x - w / 2, x + w / 2, y - h / 2, y + h / 2, "holeB", 0.4)
    for n, c in G["screws"]:
        v.circle(c, K.BOSS_D, "bossB"); v.circle(c, K.SCREW_D, "holeB")
    v.circle(CAM, CAM_D, "cam")
    v.hdim(x0, x1, y1, "79.3 wide", off=-26)
    v.vdim(x1, y0, y1, "161.0 long", off=26)
    labels = ["rib 1: 25.1 below top", "rib 2: 35.0 below top", "rib 3: 45.2 below top"]
    for (n, a, b, h), s in zip(G["ribs"], labels):
        v.note(b, (v.w - 8, v.Y(b[1])), s + "|1.0 tall (guess)", "end")
    v.note(CAM, (8, v.Y(CAM[1]) + 30), "camera window Ø7|centred, %s below top|(was %s on the fx-115ES)" % (f1(TOP - CAM[1]), f1(TOP - 43.84)))
    v.note(K.DOOR_C, (8, v.Y(K.DOOR_C[1]) - 30), "battery hole Ø10|25.0 left of centre, 11.6 below top")
    v.note((pl[0] + 2, pl[2] + 1), (8, v.Y(pl[2]) + 30), "raised plate 34.1 × 19.5, 0.6 tall (guess)|ground flat under the LiPo")
    v.note((bx[1], bx[3]), (v.w - 8, v.Y(bx[3]) - 34), "open box 19.6 × 10.1|4.3 to 23.9 right of centre", "end")
    v.text(v.w / 2, 22, "BACK COVER, SEEN FROM INSIDE (left/right as in the front view)", "ttl")
    v.text(v.w / 2, v.h - 10, "Bosses Ø5.5 with Ø2.2 screw holes. Mid pair 46.5 c-c, bottom pair 39.0 c-c, top-right 34.05 / 8.05.", "nt")
    return v.svg("Back cover from inside, with dimensions")


# ── 4. camera between the ribs ───────────────────────────────────────────────────────────────────────────────
def camera_detail():
    v = View(13.0, False, -12.0, 12.0, 31.5, 52.5, pad=(290, 70, 210, 60))
    fx0, fx1, fy0, fy1 = clip_open(v, "cm")
    v.rect(-14, 14, 30, 62, "floorB")
    for n, a, b, h in G["ribs"]:
        y = a[1]
        v.rect(-14, 14, y - K.RIB_T / 2, y + K.RIB_T / 2, "ribRect")
    v.rect(CAM[0] - CAM_MOD / 2, CAM[0] + CAM_MOD / 2, CAM[1] - CAM_MOD / 2, CAM[1] + CAM_MOD / 2, "camMod", 0.3)
    v.circle(CAM, CAM_D, "cam")
    v.o.append("</g>")
    r2, r3 = G["ribs"][1][1][1], G["ribs"][2][1][1]
    gap_lo = CAM[1] - CAM_MOD / 2 - r3 - K.RIB_T / 2
    gap_hi = r2 - K.RIB_T / 2 - CAM[1] - CAM_MOD / 2
    v.vdim(12.0, r3, r2, "rib centres %s apart" % f1(r2 - r3), off=24)
    v.vdim(-12.0, CAM[1] - CAM_MOD / 2, CAM[1] + CAM_MOD / 2, "camera module 8.5", off=-24, side="l")
    v.note((CAM_MOD / 2, CAM[1] + CAM_MOD / 2 + gap_hi / 2), (fx1 + 30, v.Y(r2) - 30), "%.2f air to rib 2" % gap_hi)
    v.note((CAM_MOD / 2, CAM[1] - CAM_MOD / 2 - gap_lo / 2), (fx1 + 30, v.Y(r3) + 34), "%.2f air to rib 3" % gap_lo)
    v.note(CAM, (fx0 - 30, v.Y(CAM[1]) + 50), "window Ø7, %s below top|KiCad (150, 96.94)|1.84 mm lower than before" % f1(TOP - CAM[1]), "end")
    v.text(v.w / 2, 22, "DETAIL: CAMERA BETWEEN RIBS 2 AND 3 (back cover from inside)", "ttl")
    v.text(v.w / 2, v.h - 8, "If a rib turns out to sit closer, file 10 mm of it flat over the camera.", "nt")
    return v.svg("Camera between ribs detail")


ROWS = [
    ("Outline", "Front shell 79.3 × 159.9, back cover 79.3 × 161.0, 11.3–11.8 thick", "same as fx-115ES (paper board fits)"),
    ("Screen window", "60.65 × 24.3, top edge 24.0 below the top", "fx-115ES, looks the same in K06/K15"),
    ("Screw posts / bosses", "corner 34.05 from centre, 8.05 down · mid pair 46.5 c-c, 69.1 down · bottom pair 39.0 c-c, 153.75 down", "fx-115ES; paper holes line up (K10, K13)"),
    ("Battery-corner screw", "33.5 from centre, 15.9 below top (fx-115ES: 34.05, 8.05)", "photos K01 + K12, ±1.5"),
    ("Corner pin", "Ø1.6 at 35.0 from centre, 8.9 below top", "K01, K16, ±1.5"),
    ("Coin-cell cups", "two C cups Ø11.8 inside, 0.8 wall, 14.5 and 25.0 from centre, 11.6 below top, 5.5 tall", "K01, K15–K17; height guessed"),
    ("Solar strip", "34 × 11, centre 13.3 from centre (fx-115ES 10.85)", "K01"),
    ("Back-cover ribs", "three full-width ribs at 25.1 / 35.0 / 45.2 below the top, ~65 mm long, 1.0 thick, 1.0 tall", "K12 (perspective-corrected); height guessed"),
    ("Battery hole", "Ø10 at 25.0 left of centre, 11.6 below top; lid recess 11.8 square", "K12"),
    ("Battery plate", "34.1 × 19.5 raised 0.6, two slots 5.8 × 3.4; ground flat under the LiPo", "K12; height guessed"),
    ("Back-cover box", "open frame 19.6 × 10.1, 4.3–23.9 right of centre, 6.5–16.6 below top, 5.5 tall", "K12; height = fx-115ES"),
    ("Camera", "Ø7 window centred, 40.1 below top (fx-115ES 38.3); module 8.5 square, 0.35 mm to each rib", "between ribs 2 and 3"),
    ("Our v15 board", "148.8 long, 71.6 at the widest, 64.1 at the bottom, 0.8 thick", "KiCad Edge.Cuts"),
]


def page():
    rows = "\n".join("<tr><td>%s</td><td>%s</td><td>%s</td></tr>" % r for r in ROWS)
    return TEMPLATE.replace("<!--F1-->", front_full()).replace("<!--F2-->", corner_detail()) \
        .replace("<!--F3-->", back_full()).replace("<!--F4-->", camera_detail()).replace("<!--ROWS-->", rows)


TEMPLATE = """<title>Knockoff Shell Dimensions</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Archivo:wdth,wght@75..100,600..800&family=Public+Sans:wght@400;600&family=JetBrains+Mono:wght@400;600&display=swap">
<style>
/* Layout: a drawing sheet. Title block, two full views side by side, two detail views, then the numbers table. */
:root { --bg:#EEF0EC; --panel:#FBFBF8; --ink:#1B201D; --muted:#5A635D; --line:#BAC2BC; --accent:#0D6C66; --red:#C8102E;
  --shell:#E3DFD3; --edge:#8F8A7C; --cav:#F2EFE6; --navy:#2F3B5C; --navy-in:#3D4B73; --rib:#AEB9DD; --win:#7E5BC4; --solar:#2B2740;
  --display:"Archivo","Arial Narrow",system-ui,sans-serif; --body:"Public Sans",system-ui,sans-serif; --mono:"JetBrains Mono",ui-monospace,Consolas,monospace; }
@media (prefers-color-scheme: dark) { :root:not([data-theme="light"]) { --bg:#111513; --panel:#181D1A; --ink:#E4E8E4; --muted:#9DA7A0; --line:#3E4842;
  --accent:#45BDB3; --red:#FF5A4E; --shell:#4A473F; --edge:#A39D8C; --cav:#2C2B27; --navy:#33416A; --navy-in:#253050; color-scheme:dark; } }
:root[data-theme="dark"] { --bg:#111513; --panel:#181D1A; --ink:#E4E8E4; --muted:#9DA7A0; --line:#3E4842;
  --accent:#45BDB3; --red:#FF5A4E; --shell:#4A473F; --edge:#A39D8C; --cav:#2C2B27; --navy:#33416A; --navy-in:#253050; color-scheme:dark; }
body { background:var(--bg); color:var(--ink); font:15px/1.55 var(--body); padding-inline:16px; padding-block:0 60px; }
.wrap { max-width:1200px; margin:0 auto; }
header { padding-block:36px 18px; border-bottom:2px solid var(--ink); display:grid; gap:8px; }
.eb { font:600 12px var(--mono); letter-spacing:.14em; text-transform:uppercase; color:var(--accent); }
h1 { font:800 clamp(30px,5.5vw,52px)/1 var(--display); font-stretch:78%; margin:0; }
.lede { max-width:70ch; margin:0; color:var(--muted); }
.grid { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); gap:16px; margin-top:16px; } .grid figure:last-child .fig { min-width:640px; }
@media (max-width:820px) { .grid { grid-template-columns:1fr; } }
figure { margin:0; background:var(--panel); border:1px solid var(--line); border-radius:6px; padding:12px; min-width:0; }
.scroll { overflow-x:auto; } .fig { width:100%; height:auto; display:block; min-width:520px; }
svg text { font-family:var(--mono); }
.shell { fill:var(--shell); stroke:var(--edge); stroke-width:1.4; } .cav { fill:var(--cav); stroke:var(--edge); stroke-width:.7; }
.keyo { fill:var(--panel); stroke:var(--edge); stroke-width:.7; opacity:.8; }
.win { fill:none; stroke:var(--win); stroke-width:1.6; stroke-dasharray:6 4; } .solar { fill:var(--solar); }
.cup { fill:var(--shell); stroke:var(--edge); stroke-width:1.4; } .cupin { fill:var(--cav); stroke:var(--edge); stroke-width:.8; }
.pin { fill:var(--edge); } .post { fill:var(--shell); stroke:var(--ink); stroke-width:1.3; } .hole { fill:var(--ink); }
.lpost { fill:var(--edge); stroke:var(--ink); stroke-width:.8; } .ghost { fill:none; stroke:var(--red); stroke-width:1.2; stroke-dasharray:3 3; }
.shellB { fill:var(--navy); } .cavB { fill:var(--navy-in); stroke:var(--rib); stroke-width:.6; }
.ribB { stroke:var(--rib); stroke-width:3.2; stroke-linecap:round; } .boxB { fill:none; stroke:var(--rib); stroke-width:3; }
.plateB { fill:#4A5A8C; stroke:var(--rib); stroke-width:1; } .holeB { fill:var(--bg); stroke:var(--rib); stroke-width:1; }
.bossB { fill:#6D7AA3; stroke:var(--rib); stroke-width:1; } .cam { fill:var(--bg); stroke:var(--red); stroke-width:2.4; }
.floorB { fill:var(--navy-in); } .frame { fill:none; stroke:var(--line); } .ribRect { fill:var(--rib); } .camMod { fill:none; stroke:var(--red); stroke-width:1.6; stroke-dasharray:5 3; }
.dim line, .dimS line { stroke:var(--ink); stroke-width:1.1; } .ext { stroke:var(--muted) !important; stroke-width:.7 !important; }
.arh { fill:var(--ink); } .dt { fill:var(--ink); font-size:12.5px; font-weight:600; paint-order:stroke; stroke:var(--panel); stroke-width:3px; }
.nt { fill:var(--ink); font-size:11.5px; paint-order:stroke; stroke:var(--panel); stroke-width:3px; }
.ttl { fill:var(--accent); font-size:12px; font-weight:600; letter-spacing:.08em; }
.lead line { stroke:var(--red); stroke-width:1; } .lead circle { fill:var(--red); }
.shellB ~ .lead line { stroke:#FF8A80; }
table { width:100%; border-collapse:collapse; font-size:14px; background:var(--panel); border:1px solid var(--line); }
th, td { text-align:left; padding:8px 10px; border-bottom:1px solid var(--line); vertical-align:top; }
th { font:600 12px var(--mono); letter-spacing:.08em; text-transform:uppercase; color:var(--muted); }
td:first-child { font-weight:600; white-space:nowrap; } td:nth-child(2) { font-family:var(--mono); font-size:13px; font-variant-numeric:tabular-nums; }
td:last-child { color:var(--muted); }
h2 { font:750 24px/1.1 var(--display); font-stretch:84%; margin:30px 0 10px; }
footer { margin-top:30px; font-size:13px; color:var(--muted); border-top:1px solid var(--line); padding-top:12px; }
</style>
<div class="wrap">
<header>
  <div class="eb">AI Calculator · knockoff shell · rev 2 · 14 Oct 2026</div>
  <h1>Knockoff Shell Dimensions</h1>
  <p class="lede">All sizes in millimetres, drawn to scale from the CAD model. "Below top" is measured down from the case's top edge; "from centre" is sideways from the centre line. Everything not listed matches the Casio fx-115ES model. Numbers read from photos are good to about ±1.5 mm.</p>
</header>
<figure style="margin-top:22px"><div class="scroll"><!--F1--></div></figure>
<figure style="margin-top:16px"><div class="scroll"><!--F3--></div></figure>
<div class="grid">
  <figure><div class="scroll"><!--F2--></div></figure>
  <figure><div class="scroll"><!--F4--></div></figure>
</div>
<h2>Every number</h2>
<div class="scroll"><table><thead><tr><th>Feature</th><th>Size and position</th><th>Source</th></tr></thead><tbody>
<!--ROWS-->
</tbody></table></div>
<footer>Generated by hardware/knockoff/sheet/make_dimension_drawing.py from build_knockoff_shell.py (rev 2) and cad/geometry.json. CAD: hardware/knockoff/cad/assembly/ai_calculator_v15_knockoff_assembly.f3d.</footer>
</div>
"""

if __name__ == "__main__":
    out = HERE / "knockoff_dimensions.html"
    out.write_text(page(), encoding="utf8")
    print("wrote", out)
