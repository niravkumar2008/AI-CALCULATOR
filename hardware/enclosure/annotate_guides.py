"""Labels for the guide picture library made by guide_renders.py (Pillow only, no Fusion).

    python annotate_guides.py            every set
    python annotate_guides.py grind z1   only names starting with these (set names: grind detail detail15 dcxyx)

Writes <name>_ann.png next to each render. Same look as final_assembly/annotate_guide.py: bold white-on-colour labels,
arrows with a white edge, numbered circles. Extra here:
  * grinding: the material to remove is found by comparing the "before" and "after" renders (same camera), shown in
    semi-transparent red on "before" and as a green outline on "after";
  * dimension lines between model points (anchors.json), a key-numbers panel and, on straight-on (orthographic) views, a
    10 mm scale bar.
"""
import json, math, os, re, sys
from PIL import Image, ImageDraw, ImageFont, ImageChops, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
DIRS = {"grind": os.path.join(HERE, "final_assembly", "renders", "grind"),
        "detail": os.path.join(HERE, "final_assembly", "renders", "detail"),
        "detail15": os.path.join(HERE, "final_assembly_v15_lcd", "renders", "detail"),
        "dcxyx": os.path.join(HERE, "final_assembly_v15_lcd", "renders", "detail")}
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
# A  GRINDING
# ═════════════════════════════════════════════════════════════════════════════
FLOOR = "!Keep >= 0.8 mm of floor (it is about 1.0)"
G = {
    "z1_solar_box": dict(
        panel=("Zone 1 - back cover: solar box", ["Remove the walled grid box", "Area about 33 x 12 mm", "Height to remove 5-5.5 mm",
                                                   "Flush with the floor, no stub", FLOOR, "Carbide burr, then sanding drum, low speed"]),
        before=[("Grind this box flat\nto the floor", "c", (-420, -330), "r"), ("Corner screw post:\ntape it, keep clear", "post", (60, -250), "k")],
        after=[("Flat floor, no ridge:\nJ4 has only 0.41 mm here", "c", (-420, -300), "g")],
        dims=[("x0", "x1", "about 33 mm", "b", 70), ("x1", "y1", "about 12 mm", "b", 70), ("zbot", "ztop", "5-5.5 mm tall", "o", -90)]),
    "z2_rib_b": dict(
        panel=("Zone 2 - back cover: rib B", ["Only the middle 14 mm of rib B", "7 mm each side of the centre line", "Height to remove 1 mm",
                                              "Flush with the floor", FLOOR, "Sanding drum (burr if slow), low speed"]),
        before=[("Rib B crosses the\ncamera spot", "c", (-120, -330), "r"), ("Rest of rib B stays", "rib", (-60, 220), "k")],
        after=[("14 mm gap: the camera\nsits here", "c", (-120, -330), "g"), ("Leave your two marker lines", "x1", (240, -200), "k")],
        dims=[("x0", "x1", "14 mm", "b", 80), ("zbot", "ztop", "1 mm", "o", -70)]),
    "z3_camera_window_inside": dict(
        panel=("Zone 3 - back cover: camera window", ["Drill after zone 2", "Centre = middle of the 14 mm gap", "Pilot first, then 7 mm (9/32 in OK)",
                                                      "Slow, light pressure at break-through", "Board ref: KiCad (150, 95.1)"]),
        before=[("Drill here: centre of\nthe ground rib B", "c", (-60, -330), "r")],
        after=[("One round 7 mm hole", "c", (-60, -330), "g")],
        dims=[("e0", "e1", "7 mm", "b", 140)]),
    "z3_camera_window_outside": dict(
        panel=("Zone 3 - outside of the back cover", ["Masking tape here before drilling", "(less chipping)", "Hole is 7 mm, round and clean",
                                                      "Don't make it bigger"]),
        before=[("The drill comes out here", "c", (-60, -330), "r")],
        after=[("7 mm camera window", "c", (-60, -330), "g")],
        dims=[("e0", "e1", "7 mm", "b", 140)]),
    "z5_magnet_notch": dict(
        panel=("Zone 5 - front shell: magnet U-notch", ["Width 21.5 mm", "Depth 7.95 mm from the rim", "Through the 1.2 mm top wall",
                                                        "6.4 -> 27.9 mm from the right edge", "Stay 0.5 mm inside, then file", "Burr then file, low speed"]),
        before=[("Top wall: cut a U-notch\nopen at the rim", "c", (-60, -340), "r"), ("Rim (parting line)", "rim", (60, 160), "k")],
        after=[("Magnet face sits flush\nin this notch", "c", (-60, -340), "g")],
        dims=[("l", "r", "21.5 mm", "b", -60), ("r", "rt", "7.95 mm", "o", -80)]),
    "z5b_lip": dict(
        panel=("Zone 5b - back cover lip (only if there)", ["No raised lip at the top end? Skip it", "If there is one: mark it through", "the notch, both sides",
                                                            "Take it level with the rim", "21.5 mm wide, outer wall untouched"]),
        before=[("Lip under the magnet notch", "c", (-80, -330), "r")],
        after=[("Level with the rim", "c", (-80, -330), "g")],
        dims=[("l", "r", "21.5 mm", "b", -110)]),
    "z6_antenna_relief": dict(
        panel=("Zone 6 - front shell: side-wall relief", ["Solar-window side wall, inner rib only", "14 -> 49 mm from the top outer edge (at this wall)",
                                                          "File down to 7.0 mm below the rim", "(6.5 is the minimum)", "Do zone 6b (pins, hooks) first",
                                                          "Don't touch the outer skin"]),
        before=[("Thin inner rib (1 mm):\nnotch it between the marks", "rib", (-430, 40), "r")],
        after=[("Rib notched to 7.0 mm\nbelow the rim", "rib", (-430, 40), "g")],
        dims=[("y15", "y50", "14 -> 49 mm from the top", "b", 110), ("zr", "z7", "7.0 mm", "o", -60)]),
    "z6b_pins_hooks": dict(
        panel=("Zone 6b - front shell: pins and hooks", ["3 round pins + 3 wire hooks", "on BOTH side walls", "Flush cutters flat on the rib",
                                                         "Needle-file the stubs", "Leave the rib, posts and snap tabs"]),
        before=[("Round pin", "p1", (-330, -40), "r"), ("Round pin", "p2", (-330, 0), "r"), ("Round pin", "p3", (-330, 0), "r"),
                ("Wire hook", "h1", (330, 0), "r"), ("Wire hook", "h2", (330, 30), "r"), ("Wire hook (wide)", "h3", (330, -40), "r")],
        after=[("Smooth rib: a fingernail\nshould not catch", "p2", (-380, 0), "g")],
        dims=[]),
    "z6b_pins_hooks_both_walls": dict(
        panel=("Zone 6b - both side walls", ["Snip all pins and hooks", "on the left AND the right wall", "6 pins + 6 hooks in total"]),
        before=[("This wall", "lw", (-140, -300), "r"), ("And this wall", "rw", (160, -300), "r")],
        after=[("Both ribs smooth", "rw", (160, -300), "g")],
        dims=[]),
    "z6c_rib_ends": dict(
        panel=("Zone 6c - rib lower ends (only if needed)", ["Decide after 6b with the paper template:", "wide corners lie flat -> skip",
                                                             "one rides up -> file that side", "Last 2-3 mm of the rib (55-62 mm from top)",
                                                             "Down to 7.0 mm below the rim", "(not in the 3D model: shown as a guide)"]),
        before=[("Lower end of the inner rib", "y58", (-330, 180), "r")],
        after=[("Filed to 7.0 mm\nbelow the rim", "y58", (-330, 180), "g")],
        dims=[("zr", "z7", "7.0 mm", "o", -80)]),
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
            pts = A[name]["points"]
            cv = Canvas(Image.open(os.path.join(D, name + ".png")))
            if state == "before":
                cv.tint(mask)
            else:
                cv.outline_mask(mask)
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
    for part, zones in (("back_cover", [("1", "z1", "Solar box: flat", (120, -120)), ("2", "z2", "Rib B: 14 mm", (-80, -170)),
                                         ("3", "z3", "Camera window 7 mm", (-80, 170)), ("5b", "z5b", "Lip (only if there)", (-200, 160))]),
                        ("faceplate", [("5", "z5", "Magnet U-notch", (-170, 160)), ("6", "z6", "Side-wall relief", (-120, -110)),
                                       ("6b", "z6b_r", "Pins + hooks (both walls)", (60, -120)), ("6b", "z6b_l", "Pins + hooks", (40, 120)),
                                       ("6c", "z6c", "Rib ends (if needed)", (-120, 150))])):
        if not want("grind", "overview"):
            continue
        b, a = "overview_%s_before" % part, "overview_%s_after" % part
        mask = diff_mask(os.path.join(D, b + ".png"), os.path.join(D, a + ".png"))
        for st in ("before", "after"):
            nm = "overview_%s_%s" % (part, st)
            pts = A[nm]["points"]
            cv = Canvas(Image.open(os.path.join(D, nm + ".png")))
            cv.tint(mask) if st == "before" else cv.outline_mask(mask)
            title = ("Back cover, inside" if part == "back_cover" else "Front shell (faceplate), inside") + (": the grind zones" if st == "before" else ": all zones done")
            cv.panel(title, ["Top end of the calculator on the RIGHT", "Red = material to remove" if st == "before" else "Green outline = ground away",
                             FLOOR] + (["Zone 4 (LR44 cup) stays: don't grind"] if part == "faceplate" else []), corner="tl", c="r" if st == "before" else "g")
            cv.scalebar(A[nm]["px_per_mm"], 10, "br")
            for n, k, txt, off in zones:
                cv.label(n + " " + txt, pts[k], off, "r" if st == "before" else "g", fs=32, num=n)
            out.append(save(cv.finish(), os.path.join(D, nm + "_ann.png")))
    # camera hole position
    if want("grind", "camera_hole"):
        nm = "camera_hole_position"; R_ = A[nm]; pts = R_["points"]; m = R_["measure"]
        cv = Canvas(Image.open(os.path.join(D, nm + ".png")))
        top_mm = m["top_Y"] - 43.84; side_mm = -m["left_X"]
        cv.dim(pts["top"], pts["c"], "%.1f mm from the top edge" % top_mm, "b", 300, tpos=0.45)
        cv.dim(pts["left"], pts["c"], "%.1f mm from the side edge\n(= half the %.1f mm width)" % (side_mm, m["right_X"] - m["left_X"]), "o", -150)
        cv.dim(pts["e1"], pts["e0"], "7 mm", "r", 90)
        cv.label("Drill centre", pts["c"], (330, -260), "r")
        cv.panel("Camera window: where to drill (outside view)", ["On the width centre line", "%.1f mm down from the top outer edge" % top_mm,
                                                                   "(cover's own top edge; 38.3 from the faceplate top)", "Pilot hole first, then 7 mm",
                                                                   "Check from inside: middle of the 14 mm gap"], corner="bl", c="b")
        cv.scalebar(R_["px_per_mm"], 10, "br")
        out.append(save(cv.finish(), os.path.join(D, nm + "_ann.png")))
    if want("grind", "magnet_notch"):
        nm = "magnet_notch_dims"; R_ = A[nm]; pts = R_["points"]
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
# B / C  ASSEMBLY CLOSE-UPS
# label = (text, anchor | (x, y), offset, colour); polys = {poly: (rgb, alpha, outline colour, dashed)}
# ═════════════════════════════════════════════════════════════════════════════
BLUE_T, ORANGE_T, YELLOW_T, GREEN_T, GREY_T = (40, 110, 230), (240, 140, 20), (250, 210, 40), (20, 170, 90), (90, 90, 100)
# Taiwoo TW302030 candidate (final_assembly/variant_battery_tw302030/results.json), same picture in the v14 and v15 sets
D22 = dict(polys={"worst": ((255, 255, 255), 0.0, (200, 90, 0), True), "lid": ((235, 30, 45), 0.0, (200, 20, 60), True),
                  "tape1": (BLUE_T, 0.35, (0, 60, 180), True), "tape2": (BLUE_T, 0.35, (0, 60, 180), True), "j4": ((255, 255, 255), 0.0, (30, 30, 40), True)},
           labels=[("TW302030 (30 x 20 x 3.0)\n1 mm further left than the #1317", "cell", (-60, -360), "k"),
                   ("Worst case 32 x 20.5 (dashed):\nstill 2.1 mm short of J4", ("poly", "worst", 2), (330, -290), "o"),
                   ("Tape L underneath\n(same strips, same rules)", ("poly", "tape2", 2), (120, 330), "b"),
                   ("Lid opening below:\nno tape there", ("poly", "lid", 18), (-330, 280), "r"),
                   ("Leads: down past the\nlead end, over rib A", "leads", (330, 280), "o"), ("J4 (on the board,\ncover closed)", ("poly", "j4", 1), (380, -80), "k"),
                   ("Corner post: 1.7 mm", "post", (-60, -250), "k")],
           panel=("Candidate bulk cell: Taiwoo TW302030", ["Rib A 0.8 (0.3 worst)  lip 0.8  LR44 holder 1.0 (0.7)",
                                                         "Leads to J4 2.9 (0.9 worst)  0 overlaps", "Leads 50-80 mm: ~34 mm used"], "br", ),
           fs_panel=26)
D14 = {
    "d01_epaper_tape": dict(polys={"panel": ((255, 255, 255), 0.0, (30, 30, 40), True), "tape1": (BLUE_T, 0.55, (0, 60, 180), False),
                                   "tape2": (BLUE_T, 0.55, (0, 60, 180), False)},
        labels=[("Panel outline (dashed)", ("poly", "panel", 1), (220, -150), "k"), ("Thin double-sided tape:\n2 strips, 0.15 mm", ("poly", "tape2", 2), (240, -140), "b"),
                ("Ribbon slot", "slot", (-200, -260), "o"), ("Ribbon end: no tape\n(the ribbon folds here)", "ledge", (-120, 260), "r")],
        panel=("E-paper on the KEY side", ["Tape strips along the long edges", "Nothing under the ribbon end", "Panel edges on the outline"])),
    "d02_epaper_on_board": dict(labels=[("E-paper panel, face up", "panel", (180, -300), "b"), ("Ribbon goes round the\nboard edge into the slot", "ribbon", (-200, 300), "r"),
                                        ("Slot (1 x 14 mm)", "slot", (60, 260), "o")]),
    "d03_j2_latch_open": dict(labels=[("J2: flip the dark latch UP\n(open) before the ribbon", "latch", (300, -300), "b"), ("Ribbon comes up the slot", "slot", (-360, 120), "r")],
                              panel=("J2 latch OPEN", ["Lift the bar with a fingernail", "Never pull on the socket body"])),
    "d04_j2_latch_closed": dict(labels=[("Ribbon all the way in,\ncontacts toward the board", "ribbon", (-380, 60), "r"), ("Press the latch DOWN\n(closed)", "latch", (300, -300), "g")],
                                panel=("J2 latch CLOSED", ["Ribbon square in the socket", "Latch flat = locked"])),
    "d05_camera_tape": dict(polys={"kapton": (ORANGE_T, 0.55, (180, 90, 0), False), "tape": (BLUE_T, 0.65, (0, 60, 180), False)},
        labels=[("Kapton square on the board\n(about 13 x 13 mm)", ("poly", "kapton", 1), (330, -260), "o"),
                ("Thin tape <= 0.1 mm under\nthe module (no foam)", ("poly", "tape", 3), (360, 200), "b"), ("Ribbon runs this way to J1", "j1dir", (330, -40), "k")],
        panel=("Camera spot (component side)", ["Centred under the 7 mm window", "Kapton first, then thin tape", "Lens faces the back cover"])),
    "d06_camera_ribbon_path": dict(labels=[("Camera, lens toward\nthe back cover", "camera", (330, 80), "g"), ("Ribbon flat, straight,\nno twist, no sag", "ribbon", (330, 0), "r"),
                                           ("Into J1", "j1", (300, -80), "b")]),
    "d07_j1_latch_open": dict(labels=[("J1 latch UP (open)", "latch", (300, 180), "b"), ("Pin 1 end", "pin1", (-260, 200), "r")],
                              panel=("J1 camera socket OPEN", ["Flip the latch up first", "Contacts on the ribbon face the board"])),
    "d08_j1_latch_closed": dict(labels=[("Ribbon straight in,\nall the way", "ribbon", (-360, 40), "r"), ("Latch DOWN (closed)", "latch", (300, 180), "g")],
                                panel=("J1 camera socket CLOSED", ["Ribbon square, nothing showing", "of the contacts"])),
    "d09_magnet_j3": dict(labels=[("N end = pin 1 = VBUS (+)", "pin1", (-380, 200), "r"), ("Pin 4 = GND (-)", "pin4", (330, 200), "k"),
                                  ("Pen-mark this leg", "nleg", (-380, -60), "r"), ("Magnet face: N mark here", "nface", (-380, 60), "r")],
                          panel=("Magnet piece into J3", ["N end to pin 1 (VBUS)", "Straightened legs pushed fully in", "Face goes into the U-notch"])),
    "d10_battery_tape": dict(polys={"outline": ((255, 255, 255), 0.0, (240, 240, 240), True), "lid": ((235, 30, 45), 0.18, (200, 20, 60), True),
                                    "tape1": (BLUE_T, 0.7, (0, 60, 180), False), "tape2": (BLUE_T, 0.7, (0, 60, 180), False)},
        labels=[("Battery outline 26 x 19.75 mm", ("poly", "outline", 1), (250, 170), "k"),
                ("Strip 1: 6 x 18 mm, under the lead end\n(1 mm in from the right edge)", ("poly", "tape1", 1), (330, -250), "b"),
                ("Strip 2: 4 x 16 mm, along the bottom edge\n(1 mm in from the left edge)", ("poly", "tape2", 3), (-60, 300), "b"),
                ("Battery-lid opening:\nNO tape here", ("poly", "lid", 9), (-60, -330), "r"),
                ("Lead end toward J4", "j4end", (330, 140), "o"), ("Corner post", "post", (-200, 220), "k")],
        panel=("Battery #1317 (rev E): back-cover floor", ["Top-left corner (inside view, top end away)", "Tape L: 2 strips of 0.1 mm tape, solid floor only",
                                                           "Strips on the cell first, then lower it in", "Nothing on the lid, nothing on top"], "br")),
    "d22_battery_tw302030": D22,
    "d11_battery_placed": dict(labels=[("Battery 150 mAh (#1317)\nflat on the floor", "battery", (-300, -280), "o"), ("Leads: short loop,\nat J4 height", "leads", (60, -300), "k"),
                                       ("Plug into J4", "plug", (200, 230), "b")]),
    "d12_j4_plug": dict(labels=[("Pin 1 = GND (black wire)", "p1", (-420, -160), "k"), ("Pin 2 = + (red wire)", "p2", (360, -180), "r"),
                                ("Push by the plug body,\nnot the wires", "plug", (330, 200), "b")],
                        panel=("Battery plug into J4", ["Check: red wire on pin 2 (+)", "Wrong way = damage: check twice"])),
    "d13_keymat_lowering": dict(labels=[("Key mat, domes toward the keys", "mat", (380, -220), "b"), ("Every post through its hole", "posts", (-380, 100), "k"),
                                        ("Half-round notches at the\nbottom posts", "notch", (330, 80), "o")]),
    "d14_keymat_seated": dict(labels=[("Key mat seated flat", "mat", (380, -260), "b"), ("Posts poke through", "posts", (-380, 100), "k")]),
    "d15_board_drop": dict(labels=[("Board, key side down", "board", (330, -280), "g"), ("H6: tightest hole", "h6", (380, 120), "r"),
                                   ("H6's post", "h6post", (-380, 140), "r")],
                           panel=("Board onto the posts", ["Line up H6 first, then lower flat", "Binds on H6? file the board hole", "toward the top, not the post"]),
                           circles=[("h6", 34, "r"), ("h6post", 30, "r")]),
    "d16_cover_close_1": dict(labels=[("1  Back cover straight down", "cover", (330, 40), "b"), ("7 mm window over the camera", "window", (330, -120), "g")],
                              panel=("Closing the back: 1 of 4", ["Battery flat, leads tucked", "Cover parallel to the shell"])),
    "d17_cover_close_2": dict(labels=[("2  Keep it level", "cover", (330, 40), "b"), ("Magnet face in its notch", "magnet", (-300, 120), "k")],
                              panel=("Closing the back: 2 of 4", ["Nothing pinched at the edges", "Ribbons inside the outline"])),
    "d18_cover_close_3": dict(labels=[("3  Seat it: no force", "cover", (330, 40), "b"), ("Screws ready", "screws", (330, 160), "k")],
                              panel=("Closing the back: 3 of 4", ["Gap all round? stop and check", "the battery and the leads"])),
    "d19_cover_close_4": dict(labels=[("4  6 screws: snug, not tight", "screws", (330, 120), "k"), ("Closed", "cover", (330, -60), "g")],
                              panel=("Closing the back: 4 of 4", ["Corner screws first, then the rest", "Plastic threads: stop when snug"])),
    "d20_window_mask_off": dict(labels=[("Black window mask:\nsticker side toward the lens", "mask", (380, 260), "k"), ("Clear lens (inside of the shell)", "lens", (-380, -300), "b")],
                                panel=("Window mask", ["Clean the lens first", "Opening = screen's active area", "Template: window_mask_template.svg"])),
    "d21_window_mask_on": dict(labels=[("Mask stuck on the lens,\nopening centred", "mask", (-380, 300), "k"), ("No white border visible", "edge", (230, 280), "g")]),
}

D15 = {
    "e01_lcd_jig": dict(polys={"paper": ((255, 255, 255), 0.55, (120, 120, 120), True), "window": ((40, 40, 50), 0.0, (200, 20, 60), False),
                               "slot": ((40, 40, 50), 0.0, (200, 90, 0), False)},
        labels=[("Paper jig (lcd_position_jig_v15.svg)\nprinted at 100 %, edges on the board edges", ("poly", "paper", 3), (330, 220), "k"),
                ("Cut-out = LCD backlight outline:\ndrop the panel in here", "win", (60, -300), "r"), ("Cut-out slot: tail goes down here", "slot", (-200, 260), "o")],
        panel=("LCD position jig (key side)", ["Check the 50 mm bar first", "Panel into the window,", "tail toward the slot"])),
    "e02_lcd_on_board": dict(polys={"window": ((0, 0, 0), 0.0, (200, 20, 60), True)},
        labels=[("LCD on 0.1 mm tape, inside\nthe jig outline (dashed)", "panel", (300, -330), "b"), ("Tail toward the slot", "tail", (-240, -260), "o"),
                ("Slot", "slot", (-160, 220), "o")]),
    "e03_tail_section_side": dict(labels=[("Board", "board", (60, -260), "k"), ("LCD panel", "panel", (60, -230), "b"), ("Tail down the slot", "slot", (-150, 300), "o"),
                                          ("ONE gentle bow, no crease\n(corners r 1.2)", "bow", (0, 210), "r"), ("Into J5", "j5", (230, 260), "g"),
                                          ("Back-cover floor", "floor", (300, 120), "k")],
                                  dims=[("bow_l", "bow_r", "the spare length lives in the bow", "r", 60)],
                                  panel=("LCD tail, side section", ["Shortest path 30.2 mm, tail 36.6 mm:", "6.4 mm spare all in the bow", "Never fold it flat"], "tr"), bar=True),
    "e04_tail_3d": dict(labels=[("Tail through the slot", "slot", (-330, 120), "o"), ("One gentle bow\nunder the board", "bow", (-60, -330), "r"),
                                ("J5", "j5", (-260, -200), "g"), ("Panel edge (other side)", "panel_edge", (330, -260), "b")]),
    "e05_j5_finger1": dict(labels=[("Finger 1 (marked dot) at the\nJ5 silk tick (pin 1 end)", "pin1", (-420, 220), "r"), ("J5", "j5", (-300, -160), "g"),
                                   ("LCD tail", "tail", (300, -200), "o")], circles=[("pin1", 40, "r")]),
    "e06_j5_latch_open": dict(labels=[("J5 latch UP (open)", "latch", (-380, -260), "b"), ("Pin 1 end (silk tick)", "pin1", (-380, 160), "r")],
                              panel=("J5 LCD socket OPEN", ["Lift the latch first", "Finger 1 dot to the pin 1 end"])),
    "e07_j5_latch_closed": dict(labels=[("Tail fully in, square", "tail", (330, -240), "o"), ("Latch DOWN (closed)", "latch", (-380, -260), "g")],
                                panel=("J5 LCD socket CLOSED", ["No contacts visible", "Tail not twisted"])),
    "e08_bow_vs_rib_b": dict(labels=[("Bow (lowest point of the tail)", "bow", (-420, -200), "r"), ("Rib B (not ground here)", "rib", (-420, 200), "k"),
                                     ("Board", "board", (-300, 100), "k")],
                             dims=[("gap_lo", "gap_hi", "0.25 mm (tail 36.6)", "o", 90)],
                             panel=("Bow vs rib B: the tight spot", ["0.25 mm gap with the 36.6 mm tail", "0.10 mm with the longest tail (36.9)",
                                                                     "Touches from about 37.1 mm", "Squeezed? grind zone 2 15 mm further"], "tr"), bar=True),
    "e10_window_mask_off": dict(labels=[("v15 mask: opening 43.7 x 23.7 mm\n(0.5 mm OUTSIDE the active area)", "mask", (380, 260), "k"),
                                        ("Clear lens", "lens", (-380, -300), "b")],
                                panel=("v15 window mask", ["Template: window_mask_template_v15.svg", "Sticker side toward the lens", "The LCD's own black border hides the rest"])),
    "d22_battery_tw302030": D22,
    "e11_window_mask_on": dict(labels=[("Mask on the lens, opening centred", "mask", (-380, 300), "k"), ("Edge", "edge", (230, 280), "g")]),
    "e12_seeed_route_section": dict(labels=[("Camera", "camera", (-200, -300), "g"), ("Seeed ribbon: straight, flat", "ribbon", (0, -330), "r"),
                                            ("J1", "j1", (120, -300), "b"), ("Rib C", "rib_c", (60, 260), "k"), ("Back-cover floor", "floor", (-60, 300), "k")],
                                    panel=("For comparison: Seeed camera (v15 baseline)", ["Straight route at J1 height", "No fold needed"], "bl"), bar=True),
    "e13_seeed_route_back": dict(labels=[("Seeed camera", "camera", (330, 60), "g"), ("Ribbon straight to J1", "ribbon", (330, 0), "r"), ("J1", "j1", (330, -60), "b")],
                                 panel=("Seeed route (straight)", ["Back cover removed", "Compare with the DCXYX S-fold"])),
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
        for k, r, c in spec.get("circles", []):
            q = pts[k]
            cv.d.ellipse([q[0] - r - 3, q[1] - r - 3, q[0] + r + 3, q[1] + r + 3], outline="white", width=11)
            cv.d.ellipse([q[0] - r, q[1] - r, q[0] + r, q[1] + r], outline=COL[c], width=6)
        for a, b, txt, c, off in spec.get("dims", []):
            cv.dim(pts[a], pts[b], txt, c, off)
        for txt, k, off, c in spec.get("labels", []):
            cv.label(txt, pt(pts, polys, k), off, c)
        out.append(save(cv.finish(), os.path.join(D, nm + "_ann.png")))
    if setname == "detail15" and want(setname, "e09"):
        out.append(keys_vs_pills(D, A))
    return out

def keys_vs_pills(D, A):
    nm = "e09_keys_vs_pills"; R_ = A[nm]; pts, polys = R_["points"], R_["polys"]
    cv = Canvas(Image.open(os.path.join(D, nm + ".png")))
    for key in ("SHIFT", "ALPHA", "ON"):
        cv.poly(polys["old_" + key], GREY_T, 0.0, (110, 110, 120), 4, True)
        cv.poly(polys["pad_" + key], YELLOW_T, 0.75, (150, 110, 0), 4)
        cv.poly(polys["pill_" + key], (20, 20, 20), 0.85, (0, 150, 70), 6)
    offs = {"SHIFT": (60, -300), "ALPHA": (330, -120), "ON": (-60, -300)}
    for key, off in offs.items():
        cv.label("%s: pad moved under\nthe Casio pill" % key, pts["k_" + key], off, "b", fs=34)
    cv.panel("Moved keys vs the key-mat pills (key side)", ["Yellow = new v15 pad", "Black disc = Casio carbon pill (measured)",
                                                            "Grey dashed = old v14 pad position", "SHIFT 1.5 / ALPHA 1.1 / ON 1.6 mm moved",
                                                            "Pill now centred: 100 % on the pad"], corner="br", c="b", fs=28)
    cv.scalebar(R_["px_per_mm"], 10, "bl")
    return save(cv.finish(), os.path.join(D, nm + "_ann.png"))

# ═════════════════════════════════════════════════════════════════════════════
# DCXYX camera S-fold: the variant renders (variant_camera_dcxyx/zfold/renders, made by build_final_assembly_v15.py
# camrenders out=variant_camera_dcxyx/zfold ...) are copied into renders/detail as f*.png and labelled. Their cameras are
# known (stage camrenders: orthographic, target/eye/ext below), so model points are projected here directly.
# ═════════════════════════════════════════════════════════════════════════════
ZF = os.path.join(HERE, "final_assembly_v15_lcd", "variant_camera_dcxyx", "zfold", "renders")
CAMY = 138.94 - 95.1
TUCK_Y = 27.0                       # stage camrenders default tuck_y (bay KiCad y 108-116 = front Y 23-31)

def ortho(eye, tgt, up, ext, W, H):
    f = [t - e for t, e in zip(tgt, eye)]; n = math.sqrt(sum(v * v for v in f)); f = [v / n for v in f]
    r = [f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2], f[0] * up[1] - f[1] * up[0]]
    n = math.sqrt(sum(v * v for v in r)); r = [v / n for v in r]
    u = [r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0]]
    k = min(W, H) / ext
    return lambda p: (W / 2 + sum((p[i] - tgt[i]) * r[i] for i in range(3)) * k, H / 2 - sum((p[i] - tgt[i]) * u[i] for i in range(3)) * k), k

def dcxyx():
    D = DIRS["dcxyx"]
    out = []
    if not want("dcxyx", "f"):
        return out
    yj = -19.0
    ym, ext_ = (CAMY + 5 + yj) / 2, CAMY + 5 - yj + 4
    shots = {
        "f01_dcxyx_sfold_route": ("zfold_section_route.png", (300, ym, 4), (0, ym, 4), (0, 0, 1), ext_ * 0.3),
        "f02_dcxyx_sfold_closeup": ("zfold_section_tuck.png", (300, TUCK_Y, 4), (0, TUCK_Y, 4), (0, 0, 1), 20),
        "f03_dcxyx_back_open": ("zfold_back_open.png", (-60, TUCK_Y - 70, -110), (0, 14, 5), (0, 0, 1), 80),
    }
    for nm, (src, eye, tgt, up, ext) in shots.items():
        im = Image.open(os.path.join(ZF, src)).convert("RGB")
        W, H = im.size
        if W > 1600:                                   # the route section is 2400 x 560: pad to 1600 x 1000 for the guide
            im = im.resize((1600, round(H * 1600 / W)), Image.LANCZOS)
            sc = 1600 / W
            c = Image.new("RGB", (1600, 1000), "white"); oy = (1000 - im.size[1]) // 2 + 120
            c.paste(im, (0, oy))
            P0, k = ortho(eye, tgt, up, ext, W, H)
            P = lambda p, P0=P0, sc=sc, oy=oy: (P0(p)[0] * sc, P0(p)[1] * sc + oy)
            im, k = c, k * sc
        else:
            P, k = ortho(eye, tgt, up, ext, W, H)
        im.save(os.path.join(D, nm + ".png"), optimize=True)
        cv = Canvas(im)
        if nm == "f01_dcxyx_sfold_route":
            cv.label("Camera (unchanged)", P((0, CAMY, 4.0)), (-60, 300), "g")
            cv.label("S-fold here: KiCad y 108-116\n(between rib C and the small ring)", P((0, TUCK_Y, 3.5)), (60, 330), "r")
            cv.label("Straight at J1 height", P((0, 5, 6.5)), (-140, -300), "k")
            cv.label("J1 (tip 2 mm in)", P((0, -17.5, 6.0)), (140, -300), "b")
            cv.panel("DCXYX OV5640 (70 mm ribbon): recommended route", ["One S-fold, r 1.2 mm, in the tuck bay", "Rest straight at J1 height",
                                                                       "No shell or board change"], corner="tl", c="b", fs=28)
        elif nm == "f02_dcxyx_sfold_closeup":
            cv.label("S-fold: pre-bend each fold round a\n2.4 mm rod (drill shank) = r 1.2", P((0, TUCK_Y + 0.4, 3.6)), (420, -230), "r")
            cv.label("Kapton strip on the floor\nunder the fold (0.44 mm gap)", P((0, TUCK_Y - 0.5, 1.05)), (420, 150), "o")
            cv.label("To the camera", P((0, TUCK_Y + 8, 6.6)), (0, -220), "g")
            cv.label("To J1", P((0, TUCK_Y - 8, 6.4)), (60, -240), "b")
            cv.panel("S-fold close-up (section at KiCad x 150)", ["Floor 0.44  small-ring wall 0.53", "board 0.54  rib C 2.58 mm",
                                                                 "Fold the ribbon BEFORE it goes in;", "never crease it flat"], corner="bl", c="b", fs=28)
            cv.scalebar(k, 2, "br")
        else:
            cv.label("Camera", P((0, CAMY, 1.6)), (330, -120), "g")
            cv.label("S-fold in the tuck bay", P((0, TUCK_Y, 3.0)), (360, 80), "r")
            cv.label("Into J1", P((0, -19.6, 5.5)), (300, 120), "b")
            cv.panel("DCXYX route seen from the back", ["Back cover removed", "Fold sits between rib C and the small ring", "Kapton on the floor under it"],
                     corner="tl", c="b", fs=28)
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
    res = grind() + details("detail", D14) + details("detail15", D15) + dcxyx()
    for p in res:
        print("%-75s %4d KB" % (os.path.relpath(p, HERE), os.path.getsize(p) // 1024))
