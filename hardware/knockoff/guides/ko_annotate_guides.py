"""KNOCKOFF labels (copy of hardware/enclosure/annotate_guides.py, main checkout, adapted 2026-10-15) for the pictures made
by ko_guide_renders.py (Pillow only, no Fusion).

    python ko_annotate_guides.py              every set
    python ko_annotate_guides.py grind ko_z1  only names starting with these (set names: grind detail)

Writes <name>_ann.png next to each render in renders/grind and renders/detail. Grinding: the material to remove is found
by comparing the "before" and "after" renders (same camera), shown in semi-transparent red on "before" and as a green
outline on "after"; dimension lines between model points (anchors.json), a key-numbers panel and, on straight-on views,
a 10 mm scale bar.
"""
import json, math, os, re, sys
from PIL import Image, ImageDraw, ImageFont, ImageChops, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
DIRS = {"grind": os.path.join(HERE, "renders", "grind"), "detail": os.path.join(HERE, "renders", "detail")}
FONT = r"C:\Windows\Fonts\segoeuib.ttf"
if not os.path.exists(FONT):
    FONT = r"C:\Windows\Fonts\arialbd.ttf"
FONT_R = r"C:\Windows\Fonts\segoeui.ttf" if os.path.exists(r"C:\Windows\Fonts\segoeui.ttf") else FONT
COL = {"r": (200, 20, 60), "b": (0, 80, 200), "g": (0, 120, 60), "o": (200, 90, 0), "k": (30, 30, 40), "p": (120, 40, 170)}
RED_FILL = (235, 30, 45)
FS = 40

def font(sz=FS, bold=True):
    return ImageFont.truetype(FONT if bold else FONT_R, sz)

# ── primitives ──
def arrow(d, p0, p1, col, w=9, head=True):
    ang = math.atan2(p1[1] - p0[1], p1[0] - p0[0])
    L = 38 if head else 0
    back = (p1[0] - L * math.cos(ang), p1[1] - L * math.sin(ang))
    for ww, cc in ((w + 7, "white"), (w, col)):
        d.line([p0, back], fill=cc, width=ww)
    if head:
        left = (back[0] + 19 * math.sin(ang), back[1] - 19 * math.cos(ang))
        right = (back[0] - 19 * math.sin(ang), back[1] + 19 * math.cos(ang))
        d.polygon([p1, left, right], fill=col, outline="white", width=3)

def text_size(d, lines, f):
    ws = [d.textbbox((0, 0), ln, font=f) for ln in lines]
    return max(b[2] - b[0] for b in ws), sum(f.size + 8 for _ in lines) - 8

class Canvas:
    def __init__(self, im):
        self.im = im.convert("RGB")
        self.W, self.H = self.im.size
        self.d = ImageDraw.Draw(self.im)
        self.boxes = []                       # (rect, lines, colour, num) drawn last
        self.keep = []                        # rectangles labels avoid (panels, title)

    def _place(self, rect):
        W, H = self.W, self.H
        for _ in range(40):
            hit = [r for r in [b[0] for b in self.boxes] + self.keep
                   if not (rect[2] + 8 < r[0] or rect[0] > r[2] + 8 or rect[3] + 8 < r[1] or rect[1] > r[3] + 8)]
            if not hit:
                break
            r = hit[0]
            down = r[3] + 10 - rect[1]; up = rect[3] + 10 - r[1]
            dy = down if (rect[3] + down < H - 8 and (down <= up or rect[1] - up < 8)) else -up
            rect = [rect[0], rect[1] + dy, rect[2], rect[3] + dy]
        return rect

    def label(self, text, p, off, c="k", fs=FS, num=None, dot=False):
        col = COL[c]
        f = font(fs)
        lines = text.split("\n")
        tw, th = text_size(self.d, lines, f)
        w, h = tw + 34, th + 24
        cx, cy = p[0] + off[0], p[1] + off[1]
        x0 = min(max(cx - w / 2, (96 if num else 10)), self.W - w - 10)
        y0 = min(max(cy - h / 2, 10), self.H - h - 10)
        rect = self._place([x0, y0, x0 + w, y0 + h])
        ex = min(max(p[0], rect[0]), rect[2]); ey = min(max(p[1], rect[1]), rect[3])
        if math.hypot(ex - p[0], ey - p[1]) > 36:
            arrow(self.d, (ex, ey), p, col)
        if dot or math.hypot(ex - p[0], ey - p[1]) <= 36:
            self.d.ellipse([p[0] - 12, p[1] - 12, p[0] + 12, p[1] + 12], fill=col, outline="white", width=4)
        self.boxes.append((rect, lines, col, num, f))

    def numbered(self, n, p, c="k", r=34):
        f = font(38)
        self.d.ellipse([p[0] - r - 5, p[1] - r - 5, p[0] + r + 5, p[1] + r + 5], fill="white")
        self.d.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=COL[c])
        self.d.text(p, str(n), font=f, fill="white", anchor="mm")

    def dim(self, a, b, text, c="b", off=60, fs=34, ext=True, tpos=0.5):
        """Dimension line a-b shifted `off` px sideways (sign picks the side), extension lines, arrows both ends."""
        col = COL[c]
        dx, dy = b[0] - a[0], b[1] - a[1]
        L = math.hypot(dx, dy) or 1
        nx, ny = -dy / L * off, dx / L * off
        a2, b2 = (a[0] + nx, a[1] + ny), (b[0] + nx, b[1] + ny)
        if ext and off:
            for p, q in ((a, a2), (b, b2)):
                e = (q[0] + nx / abs(off) * 14, q[1] + ny / abs(off) * 14)
                self.d.line([p, e], fill="white", width=7); self.d.line([p, e], fill=col, width=3)
        for ww, cc in ((11, "white"), (5, col)):
            self.d.line([a2, b2], fill=cc, width=ww)
        for p, q in ((a2, b2), (b2, a2)):
            ang = math.atan2(p[1] - q[1], p[0] - q[0])
            back = (p[0] - 26 * math.cos(ang), p[1] - 26 * math.sin(ang))
            left = (back[0] + 12 * math.sin(ang), back[1] - 12 * math.cos(ang))
            right = (back[0] - 12 * math.sin(ang), back[1] + 12 * math.cos(ang))
            self.d.polygon([p, left, right], fill=col, outline="white", width=2)
        m = (a2[0] + (b2[0] - a2[0]) * tpos, a2[1] + (b2[1] - a2[1]) * tpos)
        f = font(fs)
        tw, th = text_size(self.d, text.split("\n"), f)
        rect = [m[0] - tw / 2 - 14, m[1] - th / 2 - 10, m[0] + tw / 2 + 14, m[1] + th / 2 + 10]
        sh = [0, 0]
        if rect[0] < 10: sh[0] = 10 - rect[0]
        if rect[2] > self.W - 10: sh[0] = self.W - 10 - rect[2]
        if rect[1] < 10: sh[1] = 10 - rect[1]
        if rect[3] > self.H - 10: sh[1] = self.H - 10 - rect[3]
        rect = [rect[0] + sh[0], rect[1] + sh[1], rect[2] + sh[0], rect[3] + sh[1]]
        self.boxes.append((rect, text.split("\n"), col, None, f))

    def panel(self, title, lines, corner="bl", c="k", fs=31):
        """Key-numbers panel (white box, coloured title bar)."""
        ft, fl = font(fs + 4), font(fs, bold=False)
        tw = max(self.d.textbbox((0, 0), title, font=ft)[2],
                 max(self.d.textbbox((0, 0), l.lstrip("!"), font=font(fs) if l.startswith("!") else fl)[2] for l in lines)) + 40
        h = (fs + 18) + len(lines) * (fs + 12) + 22
        x0 = 16 if corner[1] == "l" else self.W - tw - 16
        y0 = 16 if corner[0] == "t" else self.H - h - 16
        self.keep.append([x0, y0, x0 + tw, y0 + h])
        self._panels = getattr(self, "_panels", []) + [(x0, y0, tw, h, title, lines, ft, fl, fs, c)]

    def scalebar(self, px_per_mm, mm=10, corner="br"):
        L = px_per_mm * mm
        x1 = self.W - 40 if corner[1] == "r" else 40 + L
        y = self.H - 50 if corner[0] == "b" else 60
        x0 = x1 - L
        self.keep.append([x0 - 10, y - 50, x1 + 10, y + 20])
        self._bar = (x0, x1, y, mm)

    def outline_mask(self, mask, col=(0, 140, 70), w=6):
        edge = mask.filter(ImageFilter.MaxFilter(w * 2 + 1))
        ring = ImageChops.subtract(edge, mask)
        self.im.paste(Image.new("RGB", self.im.size, col), (0, 0), ring)

    def tint(self, mask, rgb=RED_FILL, alpha=0.55):
        a = mask.point(lambda v: int(v * alpha))
        self.im.paste(Image.new("RGB", self.im.size, rgb), (0, 0), a)
        e = ImageChops.subtract(mask.filter(ImageFilter.MaxFilter(5)), mask)
        self.im.paste(Image.new("RGB", self.im.size, (150, 0, 20)), (0, 0), e)

    def poly(self, pts, rgb, alpha=0.45, outline=None, w=5, dash=False):
        ov = Image.new("L", self.im.size, 0)
        ImageDraw.Draw(ov).polygon([tuple(p) for p in pts], fill=int(255 * alpha))
        if alpha:
            self.im.paste(Image.new("RGB", self.im.size, rgb), (0, 0), ov)
        if outline:
            q = [tuple(p) for p in pts] + [tuple(pts[0])]
            for i in range(len(q) - 1):
                if dash:
                    a, b = q[i], q[i + 1]; L = math.hypot(b[0] - a[0], b[1] - a[1]); n = max(1, int(L / 22))
                    for k in range(0, n, 2):
                        t0, t1 = k / n, min(1, (k + 1) / n)
                        self.d.line([(a[0] + (b[0] - a[0]) * t0, a[1] + (b[1] - a[1]) * t0), (a[0] + (b[0] - a[0]) * t1, a[1] + (b[1] - a[1]) * t1)], fill=outline, width=w)
                else:
                    self.d.line([q[i], q[i + 1]], fill="white", width=w + 4); self.d.line([q[i], q[i + 1]], fill=outline, width=w)

    def finish(self):
        for rect, lines, col, num, f in self.boxes:
            self.d.rounded_rectangle([rect[0] - 4, rect[1] - 4, rect[2] + 4, rect[3] + 4], 15, fill="white")
            self.d.rounded_rectangle(rect, 13, fill=col)
            y = rect[1] + 12
            for ln in lines:
                bb = self.d.textbbox((0, 0), ln, font=f)
                self.d.text((rect[0] + 17, y - bb[1]), ln, font=f, fill="white")
                y += f.size + 8
            if num:
                r = 38; cx, cy = rect[0] - r + 6, (rect[1] + rect[3]) / 2
                self.d.ellipse([cx - r - 5, cy - r - 5, cx + r + 5, cy + r + 5], fill="white")
                self.d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(20, 20, 30))
                self.d.text((cx, cy), num, font=font(40), fill="white", anchor="mm")
        for x0, y0, tw, h, title, lines, ft, fl, fs, c in getattr(self, "_panels", []):
            self.d.rounded_rectangle([x0, y0, x0 + tw, y0 + h], 14, fill="white", outline=COL[c], width=4)
            self.d.rounded_rectangle([x0, y0, x0 + tw, y0 + fs + 22], 14, fill=COL[c])
            self.d.rectangle([x0, y0 + fs + 8, x0 + tw, y0 + fs + 22], fill=COL[c])
            self.d.text((x0 + 20, y0 + 8), title, font=ft, fill="white")
            y = y0 + fs + 32
            for l in lines:
                bold = l.startswith("!")
                self.d.text((x0 + 20, y), l.lstrip("!"), font=font(fs, bold) if bold else fl, fill=COL["r"] if bold else (25, 25, 35))
                y += fs + 12
        if getattr(self, "_bar", None):
            x0, x1, y, mm = self._bar
            self.d.rectangle([x0 - 8, y - 46, x1 + 8, y + 16], fill="white")
            self.d.rectangle([x0, y, x1, y + 10], fill=(30, 30, 40))
            for k in range(mm + 1):
                x = x0 + (x1 - x0) * k / mm
                self.d.line([x, y - (14 if k in (0, mm, mm // 2) else 7), x, y], fill=(30, 30, 40), width=3)
            self.d.text(((x0 + x1) / 2, y - 18), "%d mm" % mm, font=font(28), fill=(30, 30, 40), anchor="mb")
        return self.im

def diff_mask(a_path, b_path, thr=9, clean=3):
    a = Image.open(a_path).convert("RGB"); b = Image.open(b_path).convert("RGB")
    m = ImageChops.difference(a, b).convert("L").point(lambda v: 255 if v > thr else 0)
    m = m.filter(ImageFilter.MinFilter(clean)).filter(ImageFilter.MaxFilter(clean + 4)).filter(ImageFilter.MinFilter(3))
    return m

def save(im, path):
    im.save(path, optimize=True)
    return path


# ═════════════════════════════════════════════════════════════════════════════
# A  GRINDING (knockoff zones)
# ═════════════════════════════════════════════════════════════════════════════
FLOOR = "!Keep >= 0.8 mm of floor (it is about 1.0)"
CAMY = 42.0                                  # knockoff camera centre (front Y); top edge 82.1 -> 40.1 below it
G = {
    "ko_z1_solar_box": dict(
        panel=("Zone 1 - back cover: solar box", ["Remove the open box (no grid inside)", "Inside about 19.6 x 10.1 mm (+ 1 mm wall)",
                                                   "Height to remove about 5.5 mm", "Flush with the floor, no stub", FLOOR,
                                                   "Carbide burr, then sanding drum, low speed"]),
        before=[("Grind this box flat\nto the floor", "c", (-420, -330), "r"), ("Corner screw boss:\nkeep clear", "post", (60, -250), "k")],
        after=[("Flat floor, no ridge:\nJ4 + battery plug sit here", "c", (-420, -300), "g")],
        dims=[("x0", "x1", "about 19.6 mm", "b", 70), ("x1", "y1", "about 10.1 mm", "b", 70), ("zbot", "ztop", "about 5.5 mm tall", "o", -90)]),
    "ko_z2_battery_plate": dict(
        panel=("Zone 2 - back cover: battery plate", ["Raised plate round the battery hole", "Flatten only under the LiPo:",
                                                       "about 26.5 x 19.5 mm (dashed)", "Plate is about 0.6 mm tall (guess)",
                                                       "Keep the battery hole and the", "corner screw hole clean", FLOOR,
                                                       "Sanding drum, light passes"]),
        polys={"zone": ((255, 255, 255), 0.0, (30, 30, 40), True)},
        before=[("Flatten the plate\ninside the dashed box", "c", (-60, -360), "r"), ("Battery hole (stays)", "hole", (-330, 230), "k"),
                ("Corner screw hole:\nkeep clear", "screw", (-260, -250), "k")],
        after=[("Flat: the LiPo lies\non the floor", "c", (-60, -360), "g")],
        dims=[("x0", "x1", "about 26.5 mm", "b", -60), ("x1", "y1", "about 19.5 mm", "b", -70)]),
    "ko_z3_camera_window_inside": dict(
        panel=("Zone 3 - back cover: camera window", ["Between ribs 2 and 3 (no rib to grind)", "Centre = middle of the gap,",
                                                      "40.1 mm below the top edge", "Pilot first, then 7 mm (9/32 in OK)",
                                                      "Slow, light pressure at break-through", "Board ref: KiCad (150, 96.94)"]),
        before=[("Drill here: midway\nbetween ribs 2 and 3", "c", (-60, -330), "r"), ("Rib 2", "rib2", (-240, -120), "k"), ("Rib 3", "rib3", (-240, 120), "k")],
        after=[("One round 7 mm hole", "c", (-60, -330), "g"), ("Ribs stay", "rib2", (-240, -120), "k")],
        dims=[("e0", "e1", "7 mm", "b", 140)]),
    "ko_z3_camera_window_outside": dict(
        panel=("Zone 3 - outside of the back cover", ["Masking tape here before drilling", "(less chipping)", "Hole is 7 mm, round and clean",
                                                      "Don't make it bigger"]),
        before=[("The drill comes out here", "c", (-60, -330), "r")],
        after=[("7 mm camera window", "c", (-60, -330), "g")],
        dims=[("e0", "e1", "7 mm", "b", 140)]),
    "ko_z5_magnet_notch": dict(
        panel=("Zone 5 - front shell: magnet U-notch", ["Width 21.5 mm", "Depth 7.95 mm from the rim", "Through the top wall",
                                                        "6.4 -> 27.9 mm from the right edge", "Stay 0.5 mm inside, then file", "Burr then file, low speed"]),
        before=[("Top wall: cut a U-notch\nopen at the rim", "c", (-60, -340), "r"), ("Rim (parting line)", "rim", (60, 160), "k")],
        after=[("Magnet face sits flush\nin this notch", "c", (-60, -340), "g")],
        dims=[("l", "r", "21.5 mm", "b", -60), ("r", "rt", "7.95 mm", "o", -80)]),
    "ko_z5b_lip": dict(
        panel=("Zone 5b - back cover lip (only if there)", ["No raised lip at the top end? Skip it", "If there is one: mark it through", "the notch, both sides",
                                                            "Take it level with the rim", "21.5 mm wide, outer wall untouched"]),
        before=[("Lip under the magnet notch", "c", (-80, -330), "r")],
        after=[("Level with the rim", "c", (-80, -330), "g")],
        dims=[("l", "r", "21.5 mm", "b", -110)]),
    "ko_z6_antenna_relief": dict(
        panel=("Zone 6 - front shell: side-wall relief", ["Solar-window side wall, inner rib only", "14 -> 49 mm from the top outer edge (at this wall)",
                                                          "File down to 7.0 mm below the rim", "(6.5 is the minimum)", "Do zone 6b (pins, hooks) first",
                                                          "Don't touch the outer skin"]),
        before=[("Thin inner rib (1 mm):\nnotch it between the marks", "rib", (-430, 40), "r")],
        after=[("Rib notched to 7.0 mm\nbelow the rim", "rib", (-430, 40), "g")],
        dims=[("y15", "y50", "14 -> 49 mm from the top", "b", 110), ("zr", "z7", "7.0 mm", "o", -60)]),
    "ko_z6b_pins_hooks": dict(
        panel=("Zone 6b - front shell: pins and hooks", ["3 round pins + 3 wire hooks", "on BOTH side walls", "Flush cutters flat on the rib",
                                                         "Needle-file the stubs", "Leave the rib, posts and snap tabs"]),
        before=[("Round pin", "p1", (-330, -40), "r"), ("Round pin", "p2", (-330, 0), "r"), ("Round pin", "p3", (-330, 0), "r"),
                ("Wire hook", "h1", (330, 0), "r"), ("Wire hook", "h2", (330, 30), "r"), ("Wire hook (wide)", "h3", (330, -40), "r")],
        after=[("Smooth rib: a fingernail\nshould not catch", "p2", (-380, 0), "g")],
        dims=[]),
    "ko_z6b_pins_hooks_both_walls": dict(
        panel=("Zone 6b - both side walls", ["Snip all pins and hooks", "on the left AND the right wall", "6 pins + 6 hooks in total"]),
        before=[("This wall", "lw", (-140, -300), "r"), ("And this wall", "rw", (160, -300), "r")],
        after=[("Both ribs smooth", "rw", (160, -300), "g")],
        dims=[]),
}

def grind():
    D = DIRS["grind"]
    A = json.load(open(os.path.join(D, "anchors.json")))
    out = []
    for z, spec in G.items():
        if not want("grind", z):
            continue
        bpath, apath = os.path.join(D, z + "_before.png"), os.path.join(D, z + "_after.png")
        mask = diff_mask(bpath, apath)
        for state in ("before", "after"):
            name = "%s_%s" % (z, state)
            pts, polys = A[name]["points"], A[name].get("polys", {})
            cv = Canvas(Image.open(os.path.join(D, name + ".png")))
            if state == "before":
                cv.tint(mask)
            else:
                cv.outline_mask(mask)
            for k, (rgb, alpha, oc, dash) in spec.get("polys", {}).items():
                cv.poly(polys[k], rgb, alpha, oc, 5, dash)
            t, lines = spec["panel"]
            cv.panel(t + (" - BEFORE" if state == "before" else " - AFTER"), lines + ([] if state == "before" else ["Green outline = where it was"]),
                     corner="bl", c="r" if state == "before" else "g")
            for a, b, txt, c, off in spec["dims"]:
                cv.dim(pts[a], pts[b], txt, c, off)
            for txt, k, off, c in spec[state]:
                cv.label(txt, pts[k], off, c)
            if state == "before":
                cv.label("Red = remove", (cv.W - 190, 60), (0, 0), "r", fs=34)
            out.append(save(cv.finish(), os.path.join(D, name + "_ann.png")))
    # overviews
    for part, zones in (("back_cover", [("1", "z1", "Solar box: flat", (120, -120)), ("2", "z2", "Battery plate: flat", (-80, -170)),
                                         ("3", "z3", "Camera window 7 mm", (-80, 170)), ("5b", "z5b", "Lip (only if there)", (-200, 160))]),
                        ("faceplate", [("5", "z5", "Magnet U-notch", (-170, 160)), ("6", "z6", "Side-wall relief", (-120, -110)),
                                       ("6b", "z6b_r", "Pins + hooks (both walls)", (60, -120)), ("6b", "z6b_l", "Pins + hooks", (40, 120))])):
        if not want("grind", "ko_overview"):
            continue
        b, a = "ko_overview_%s_before" % part, "ko_overview_%s_after" % part
        mask = diff_mask(os.path.join(D, b + ".png"), os.path.join(D, a + ".png"))
        for st in ("before", "after"):
            nm = "ko_overview_%s_%s" % (part, st)
            pts = A[nm]["points"]
            cv = Canvas(Image.open(os.path.join(D, nm + ".png")))
            cv.tint(mask) if st == "before" else cv.outline_mask(mask)
            title = ("Knockoff back cover, inside" if part == "back_cover" else "Knockoff front shell, inside") + (": the grind zones" if st == "before" else ": all zones done")
            cv.panel(title, ["Top end of the calculator on the RIGHT", "Red = material to remove" if st == "before" else "Green outline = ground away",
                             FLOOR] + (["Battery cups stay: don't grind"] if part == "faceplate" else []), corner="tl", c="r" if st == "before" else "g")
            cv.scalebar(A[nm]["px_per_mm"], 10, "br")
            for n, k, txt, off in zones:
                cv.label(n + " " + txt, pts[k], off, "r" if st == "before" else "g", fs=32, num=n)
            out.append(save(cv.finish(), os.path.join(D, nm + "_ann.png")))
    # camera hole position
    if want("grind", "ko_camera_hole"):
        nm = "ko_camera_hole_position"; R_ = A[nm]; pts = R_["points"]; m = R_["measure"]
        cv = Canvas(Image.open(os.path.join(D, nm + ".png")))
        TOP = 82.1                                # top outer edge (model max Y); the skin at X 0 measures m["top_Y"]
        top_mm = TOP - CAMY; side_mm = -m["left_X"]
        k_ = (pts["c"][1] - pts["top"][1]) / (m["top_Y"] - CAMY)          # px per mm from the measured skin edge
        pts["top"] = [pts["c"][0], pts["c"][1] - top_mm * k_]           # straight-on ortho view, Y up
        cv.dim(pts["top"], pts["c"], "%.1f mm from the top edge" % top_mm, "b", 300, tpos=0.45)
        cv.dim(pts["left"], pts["c"], "%.1f mm from the side edge\n(= half the %.1f mm width)" % (side_mm, m["right_X"] - m["left_X"]), "o", -150)
        cv.dim(pts["e1"], pts["e0"], "7 mm", "r", 90)
        cv.label("Drill centre", pts["c"], (330, -260), "r")
        cv.panel("Camera window: where to drill (outside view)", ["On the width centre line", "%.1f mm down from the top outer edge" % top_mm,
                                                                   "(model top edge Y 82.1; %.1f at the rounded skin)" % (m["top_Y"] - CAMY), "Pilot hole first, then 7 mm",
                                                                   "Check from inside: midway between ribs 2 and 3"], corner="bl", c="b")
        cv.scalebar(R_["px_per_mm"], 10, "br")
        out.append(save(cv.finish(), os.path.join(D, nm + "_ann.png")))
    if want("grind", "ko_magnet_notch"):
        nm = "ko_magnet_notch_dims"; R_ = A[nm]; pts = R_["points"]
        cv = Canvas(Image.open(os.path.join(D, nm + ".png")))
        l, r = pts["l"], pts["r"]
        k = ((r[0] - l[0]) / 21.5, (r[1] - l[1]) / 21.5)
        edge = (r[0] + k[0] * 6.4, r[1] + k[1] * 6.4)                 # right side edge = 6.4 mm beyond the notch (grinding manual)
        cv.dim(l, r, "21.5 mm wide", "b", -70)
        cv.dim(r, pts["rt"], "7.95 mm deep\nfrom the rim", "o", 130)
        cv.dim(r, edge, "6.4", "k", -70)
        cv.dim(l, edge, "27.9 mm from the right edge", "k", -170)
        cv.label("Rim (parting line):\nthe notch is open here", (l[0] - 120, l[1]), (-120, 120), "k")
        cv.panel("Magnet U-notch, seen from the top end", ["Display side DOWN in this view (rim on top)", "21.5 wide x 7.95 deep from the rim",
                                                           "Rounded inner corners r 1", "Measure from the RIGHT edge", "(display up, top end away)"],
                 corner="bl", c="b")
        cv.scalebar(R_["px_per_mm"], 10, "br")
        out.append(save(cv.finish(), os.path.join(D, nm + "_ann.png")))
    return out

# ═════════════════════════════════════════════════════════════════════════════
# B  KNOCKOFF ASSEMBLY CLOSE-UPS
# label = (text, anchor | (x, y), offset, colour); polys = {poly: (rgb, alpha, outline colour, dashed)}
# ═════════════════════════════════════════════════════════════════════════════
BLUE_T, ORANGE_T, YELLOW_T, GREEN_T, GREY_T = (40, 110, 230), (240, 140, 20), (250, 210, 40), (20, 170, 90), (90, 90, 100)
DKO = {
    "ko_d01_battery_corner_back": dict(
        polys={"j4": ((255, 255, 255), 0.0, (30, 30, 40), True), "cup1": ((255, 255, 255), 0.0, (120, 40, 170), True),
               "cup2": ((255, 255, 255), 0.0, (120, 40, 170), True)},
        labels=[("LiPo 150 mAh (#1317)\nflat on the ground plate", "battery", (-330, -330), "o"), ("Leads: short loop to J4", "leads", (330, -280), "k"),
                ("Plug into J4", "plug", (300, 220), "b"), ("J4 (on the board,\ncover closed)", ("poly", "j4", 2), (330, -60), "k"),
                ("Front-shell battery cups\n(other side of the board)", "cup2", (-360, -220), "p"), ("Battery-corner screw", "screw", (-300, 250), "k")],
        panel=("Knockoff battery corner (inside the back cover)", ["Cell over the ground battery plate", "Battery hole + lid under the cell",
                                                                  "Plug: red wire on pin 2 (+)"], "br")),
    "ko_d02_battery_corner_board": dict(
        labels=[("LiPo (back cover removed)", "battery", (-330, -300), "o"), ("JST plug in J4", "plug", (330, -250), "b"),
                ("Leads", "leads", (300, 230), "k")],
        panel=("Battery corner, back cover off", ["LiPo lies on the cover floor, not the board", "Leads go round to J4"], "bl")),
    "ko_d03_battery_corner_section": dict(
        labels=[("Battery cups (front shell)", "cup2", (-60, -330), "p"), ("Cups", "cup1", (60, -330), "p"), ("Board", "board", (260, -260), "k"),
                ("LiPo", "battery", (0, 260), "o"), ("J4 + plug", "j4", (-60, 300), "b"), ("Back-cover floor", "floor", (60, 300), "k")],
        panel=("Section at Y 70.5 (cup centre line)", ["Cups / board / LiPo / cover, top to bottom", "Look for any overlap"], "tr"), bar=True),
    "ko_d04_camera_between_ribs": dict(
        labels=[("Camera module", "camera", (330, -280), "g"), ("Rib 2", "rib2", (-300, -160), "k"), ("Rib 3", "rib3", (-300, 160), "k"),
                ("7 mm window under the lens", "window", (330, 240), "b")],
        panel=("Knockoff camera spot", ["Between back ribs 2 and 3", "40.1 mm below the top edge", "No rib is ground"], "bl")),
    "ko_d05_camera_rib_section": dict(
        labels=[("Camera module", "camera", (60, -300), "g"), ("Rib 2", "rib2", (260, 220), "k"), ("Rib 3", "rib3", (-260, 220), "k"),
                ("7 mm window", "window", (0, 300), "b"), ("Board", "board", (300, -250), "k")],
        dims=[("g3a", "g3b", "0.35", "o", -90), ("g2a", "g2b", "0.35", "o", -90)],
        panel=("Camera vs ribs 2 and 3 (section at X 0)", ["0.35 mm each side (model)", "Ribs 1.0 tall (guess)"], "tr"), bar=True),
    "ko_d06_window_mask_off": dict(
        labels=[("v15 mask: opening 43.7 x 23.7 mm\n(0.5 mm OUTSIDE the active area)", "mask", (380, 260), "k"), ("Clear lens", "lens", (-380, -300), "b")],
        panel=("Window mask (knockoff shell)", ["Template: window_mask_template_v15.svg", "Sticker side toward the lens", "The LCD's own black border hides the rest"])),
    "ko_d07_window_mask_on": dict(labels=[("Mask on the lens, opening centred", "mask", (-380, 300), "k"), ("Edge", "edge", (230, 280), "g")]),
}

def pt(pts, polys, k):
    if isinstance(k, tuple) and k and k[0] == "poly":
        return polys[k[1]][k[2]]
    return pts[k] if isinstance(k, str) else k

def details(setname, table):
    D = DIRS[setname]
    A = json.load(open(os.path.join(D, "anchors.json")))
    out = []
    for nm, spec in table.items():
        if not want(setname, nm) or nm not in A:
            continue
        R_ = A[nm]; pts, polys = R_["points"], R_["polys"]
        cv = Canvas(Image.open(os.path.join(D, nm + ".png")))
        for k, (rgb, alpha, oc, dash) in spec.get("polys", {}).items():
            cv.poly(polys[k], rgb, alpha, oc, 5, dash)
        if spec.get("panel"):
            p = spec["panel"]
            cv.panel(p[0], p[1], corner=p[2] if len(p) > 2 else "bl", c="b", fs=spec.get("fs_panel", 31))
        if spec.get("bar"):
            cv.scalebar(R_["px_per_mm"], 2 if R_["px_per_mm"] > 40 else 10, "br")
        for a, b, txt, c, off in spec.get("dims", []):
            cv.dim(pts[a], pts[b], txt, c, off)
        for txt, k, off, c in spec.get("labels", []):
            cv.label(txt, pt(pts, polys, k), off, c)
        out.append(save(cv.finish(), os.path.join(D, nm + "_ann.png")))
    return out

ONLY = []
def want(setname, name):
    if not ONLY:
        return True
    sets = [o for o in ONLY if o in DIRS]
    names = [o for o in ONLY if o not in DIRS]
    return (not sets or setname in sets) and (not names or any(name.startswith(n) for n in names))

if __name__ == "__main__":
    ONLY[:] = sys.argv[1:]
    res = (grind() if os.path.exists(os.path.join(DIRS["grind"], "anchors.json")) else [])
    res += (details("detail", DKO) if os.path.exists(os.path.join(DIRS["detail"], "anchors.json")) else [])
    for p in res:
        print("%-75s %4d KB" % (os.path.relpath(p, HERE), os.path.getsize(p) // 1024))
