"""Bench / power-up pictures of the bare boards (no Fusion): kicad-cli 3D renders + Pillow labels.

    python make_bringup.py            render (if missing) + annotate, v14 and v15
    python make_bringup.py render     force new kicad-cli renders

Boards are only READ: hardware/kicad/ai_calc.kicad_pcb (v14 e-paper) and hardware/kicad_v15_lcd/ai_calc_v15_lcd.kicad_pcb
(v15 LCD). Raw renders (3200 x 2400, transparent) go to raw/; the pictures are 1600 x 1200 on white. Pixel positions come from
the board file: the drilled mounting holes (H1, H3, H4, H6, H8, H10) are found in the render and a scale + offset is fitted to
them, so every label sits on the pad named in the board file.
TOP = F.Cu = component side (faces the back cover: test pads, J1-J5). BOTTOM = B.Cu = key side (key pads, screen).
"""
import json, math, os, re, subprocess, sys
from PIL import Image, ImageDraw, ImageFont, ImageChops

HERE = os.path.dirname(os.path.abspath(__file__))
HW = os.path.dirname(HERE)
RAW = os.path.join(HERE, "raw")
KICAD_CLI = r"C:\Program Files\KiCad\10.0\bin\kicad-cli.exe"
BOARDS = {"v14": os.path.join(HW, "kicad", "ai_calc.kicad_pcb"),
          "v15": os.path.join(HW, "kicad_v15_lcd", "ai_calc_v15_lcd.kicad_pcb")}
RW, RH = 3200, 2400                     # raw render size; pictures are half that
FONT = r"C:\Windows\Fonts\segoeuib.ttf"
if not os.path.exists(FONT):
    FONT = r"C:\Windows\Fonts\arialbd.ttf"
COL = {"r": (200, 20, 60), "b": (0, 80, 200), "g": (0, 120, 60), "o": (200, 90, 0), "k": (30, 30, 40), "p": (120, 40, 170)}
KEEP_OUT = []                            # rectangles labels must avoid (set per picture: the title box)
HOLES = ("H1", "H3", "H4", "H6", "H8", "H10")

# ── board file ──
def footprints(path):
    s = open(path, encoding="utf8").read()
    idx = [m.start() for m in re.finditer(r"\n\t\(footprint ", s)] + [len(s)]
    out = {}
    for a, b in zip(idx, idx[1:]):
        blk = s[a:b]
        ref = re.search(r'"Reference" "([^"]+)"', blk).group(1)
        at = re.search(r"\(at ([\d\.\-]+) ([\d\.\-]+)(?: ([\d\.\-]+))?\)", blk).groups()
        x, y, r = float(at[0]), float(at[1]), float(at[2] or 0)
        side = re.search(r'\(layer "([^"]+)"\)', blk).group(1)
        val = re.search(r'"Value" "([^"]+)"', blk).group(1)
        pads = {}
        for m in re.finditer(r'\(pad "([^"]*)" \w+ \w+\s*\(at ([\d\.\-]+) ([\d\.\-]+)', blk):
            if m.group(1) in pads:
                continue
            px, py = float(m.group(2)), float(m.group(3))
            t = math.radians(r)
            pads[m.group(1)] = (x + px * math.cos(t) + py * math.sin(t), y - px * math.sin(t) + py * math.cos(t))
        out[ref] = dict(at=(x, y), rot=r, side=side, value=val, pads=pads)
    return out

def edge_bbox(path):
    s = open(path, encoding="utf8").read()
    xs, ys = [], []
    for m in re.finditer(r'\(gr_(?:line|arc)\s*\(start ([\d\.\-]+) ([\d\.\-]+)\)(?:\s*\(mid ([\d\.\-]+) ([\d\.\-]+)\))?\s*\(end ([\d\.\-]+) ([\d\.\-]+)\)(.*?)\(layer "Edge.Cuts"\)', s, re.S):
        g = [v for v in m.groups()[:6] if v is not None]
        xs += [float(v) for v in g[0::2]]; ys += [float(v) for v in g[1::2]]
    return min(xs), max(xs), min(ys), max(ys)

# ── rendering ──
def render(ver, side, force=False):
    os.makedirs(RAW, exist_ok=True)
    out = os.path.join(RAW, "%s_%s.png" % (ver, side))
    if force or not os.path.exists(out):
        subprocess.run([KICAD_CLI, "pcb", "render", "-o", out, "-w", str(RW), "-h", str(RH), "--side", side,
                        "--background", "transparent", "--quality", "high", BOARDS[ver]], check=True, capture_output=True)
    return out

def holes_px(img):
    """Centroids of enclosed transparent blobs (drilled holes) in a raw render."""
    a = img.split()[-1].point(lambda v: 255 if v < 40 else 0)
    W, H = a.size
    px = a.load()
    seen = bytearray(W * H)
    blobs = []
    bb = img.split()[-1].point(lambda v: 255 if v > 40 else 0).getbbox()
    for y0 in range(bb[1], bb[3], 3):
        for x0 in range(bb[0], bb[2], 3):
            if px[x0, y0] and not seen[y0 * W + x0]:
                st = [(x0, y0)]; seen[y0 * W + x0] = 1; pts = []; edge = False
                while st:
                    x, y = st.pop(); pts.append((x, y))
                    if x <= bb[0] + 1 or x >= bb[2] - 2 or y <= bb[1] + 1 or y >= bb[3] - 2:
                        edge = True
                    for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                        if 0 <= nx < W and 0 <= ny < H and px[nx, ny] and not seen[ny * W + nx]:
                            seen[ny * W + nx] = 1; st.append((nx, ny))
                    if len(pts) > 60000:
                        edge = True; break
                if not edge and 300 < len(pts) < 60000:
                    xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
                    w, h = max(xs) - min(xs), max(ys) - min(ys)
                    if 0.75 < (w + 1) / (h + 1) < 1.33:
                        blobs.append((sum(xs) / len(xs), sum(ys) / len(ys), len(pts)))
    return blobs

def fit(ver, side, img, fps):
    """Least-squares x_px = sx*x + tx, y_px = s*y + ty (sx = +-s: the bottom view is mirrored) from the mounting holes."""
    x0, x1, y0, y1 = edge_bbox(BOARDS[ver])
    W, H = img.size
    bb = img.split()[-1].point(lambda v: 255 if v > 40 else 0).getbbox()
    s0 = (bb[3] - bb[1]) / (y1 - y0)
    mir = -1 if side == "bottom" else 1
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    guess = lambda x, y: (W / 2 + mir * (x - cx) * s0, H / 2 + (y - cy) * s0)
    blobs = holes_px(img)
    pairs = []
    for h in HOLES:
        if h not in fps:
            continue
        gx, gy = guess(*fps[h]["at"])
        best = min(blobs, key=lambda b: (b[0] - gx) ** 2 + (b[1] - gy) ** 2) if blobs else None
        if best and math.hypot(best[0] - gx, best[1] - gy) < 0.08 * W:
            pairs.append((fps[h]["at"], best[:2]))
    if len(pairs) < 3:
        raise RuntimeError("%s %s: only %d holes matched" % (ver, side, len(pairs)))
    # solve for s, tx, ty
    n = len(pairs)
    mx = sum(mir * p[0][0] for p in pairs) / n; my = sum(p[0][1] for p in pairs) / n
    ux = sum(p[1][0] for p in pairs) / n; uy = sum(p[1][1] for p in pairs) / n
    num = sum((mir * p[0][0] - mx) * (p[1][0] - ux) + (p[0][1] - my) * (p[1][1] - uy) for p in pairs)
    den = sum((mir * p[0][0] - mx) ** 2 + (p[0][1] - my) ** 2 for p in pairs)
    s = num / den
    tx, ty = ux - s * mx, uy - s * my
    res = max(math.hypot(s * mir * a[0] + tx - b[0], s * a[1] + ty - b[1]) for a, b in pairs)
    return (lambda x, y: (s * mir * x + tx, s * y + ty)), s, res, len(pairs)

# ── drawing (same style as enclosure/final_assembly/annotate_guide.py) ──
def font(sz):
    return ImageFont.truetype(FONT, sz)

def arrow(d, p0, p1, col, w=9):
    ang = math.atan2(p1[1] - p0[1], p1[0] - p0[0])
    L = 36
    back = (p1[0] - L * math.cos(ang), p1[1] - L * math.sin(ang))
    left = (back[0] + 18 * math.sin(ang), back[1] - 18 * math.cos(ang))
    right = (back[0] - 18 * math.sin(ang), back[1] + 18 * math.cos(ang))
    for ww, cc in ((w + 7, "white"), (w, col)):
        d.line([p0, back], fill=cc, width=ww)
    d.polygon([p1, left, right], fill=col, outline="white", width=3)

def labels(im, items, fs=40, nudge=True):
    """items: (text, target_xy, offset_xy, colour). Boxes are drawn after every arrow."""
    d = ImageDraw.Draw(im)
    f = font(fs)
    W, H = im.size
    boxes = []
    for text, p, off, c in items:
        col = COL[c]
        lines = text.split("\n")
        tw = max(d.textbbox((0, 0), ln, font=f)[2] for ln in lines)
        th = sum(d.textbbox((0, 0), ln, font=f)[3] - d.textbbox((0, 0), ln, font=f)[1] for ln in lines) + 8 * (len(lines) - 1)
        w, h = tw + 30, th + 22
        cx, cy = p[0] + off[0], p[1] + off[1]
        x0 = min(max(cx - w / 2, 8), W - w - 8); y0 = min(max(cy - h / 2, 8), H - h - 8)
        rect = [x0, y0, x0 + w, y0 + h]
        for _ in range(40 if nudge else 0):       # nudge away from boxes already placed (and from the title)
            hit = [r for r in [b[0] for b in boxes] + list(KEEP_OUT) if not (rect[2] + 8 < r[0] or rect[0] > r[2] + 8 or rect[3] + 8 < r[1] or rect[1] > r[3] + 8)]
            if not hit:
                break
            r = hit[0]
            dy = (r[3] + 10 - rect[1]) if (rect[1] + rect[3]) >= (r[1] + r[3]) or rect[3] + (r[3] - rect[1]) > H - 8 else -(rect[3] + 10 - r[1])
            if rect[3] + dy > H - 8 or rect[1] + dy < 8:
                dy = -dy if 8 < rect[1] - dy and rect[3] - dy < H - 8 else dy
            rect = [rect[0], rect[1] + dy, rect[2], rect[3] + dy]
        ex = min(max(p[0], rect[0]), rect[2]); ey = min(max(p[1], rect[1]), rect[3])
        if math.hypot(ex - p[0], ey - p[1]) > 30:
            arrow(d, (ex, ey), p, col)
        boxes.append((rect, lines, col))
    for rect, lines, col in boxes:
        d.rounded_rectangle([rect[0] - 3, rect[1] - 3, rect[2] + 3, rect[3] + 3], 14, fill="white")
        d.rounded_rectangle(rect, 12, fill=col)
        y = rect[1] + 9
        for ln in lines:
            d.text((rect[0] + 15, y - d.textbbox((0, 0), ln, font=f)[1]), ln, font=f, fill="white")
            y += d.textbbox((0, 0), ln, font=f)[3] - d.textbbox((0, 0), ln, font=f)[1] + 8
    return im

def column_labels(im, items, fs=30, xl=230, xr=1370, y_min=150):
    """Full-board pictures: labels stacked in a left and a right column (no overlaps), arrows to the targets.
    items: (text, target, side 'L'/'R', colour)."""
    d = ImageDraw.Draw(im)
    f = font(fs)
    W, H = im.size
    out = []
    for side, x in (("L", xl), ("R", xr)):
        col = sorted([it for it in items if it[2] == side], key=lambda it: it[1][1])
        sizes = []
        for text, p, _, c in col:
            lines = text.split("\n")
            h = len(lines) * (fs + 10) + 14
            sizes.append(h)
        ys, y = [], y_min
        for (text, p, _, c), h in zip(col, sizes):
            y = max(y, p[1] - h / 2)
            ys.append(y); y += h + 14
        over = y - (H - 10)
        if over > 0:                                   # push the stack back up
            ys = [max(y_min, v - over) for v in ys]
            for k in range(1, len(ys)):
                ys[k] = max(ys[k], ys[k - 1] + sizes[k - 1] + 14)
        for (text, p, _, c), y0, h in zip(col, ys, sizes):
            out.append((text, p, (x - p[0], y0 + h / 2 - p[1]), c))
    return labels(im, out, fs, nudge=False)

def ring(im, p, r, col, w=6):
    d = ImageDraw.Draw(im)
    d.ellipse([p[0] - r - 3, p[1] - r - 3, p[0] + r + 3, p[1] + r + 3], outline="white", width=w + 4)
    d.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], outline=COL[col], width=w)

def probe(im, p, colour, text, off):
    """Meter probe: a coloured tip on the pad plus a label (red = +, black = COM)."""
    d = ImageDraw.Draw(im)
    c = (210, 20, 30) if colour == "red" else (20, 20, 20)
    tip = p
    end = (p[0] + off[0] * 0.55, p[1] + off[1] * 0.55)
    d.line([end, tip], fill="white", width=26); d.line([end, tip], fill=c, width=18)
    d.ellipse([tip[0] - 14, tip[1] - 14, tip[0] + 14, tip[1] + 14], fill=c, outline="white", width=4)
    labels(im, [(text, end, (off[0] * 0.45, off[1] * 0.45), "r" if colour == "red" else "k")])

def title(im, text, sub=None, dry=False):
    d = ImageDraw.Draw(im)
    f, fs = font(46), font(30)
    w = d.textbbox((0, 0), text, font=f)[2] + 40
    if sub:
        w = max(w, d.textbbox((0, 0), sub, font=fs)[2] + 40)
    h = 70 + (44 if sub else 0)
    KEEP_OUT[:] = [[14, 14, 14 + w, 14 + h]]
    if dry:
        return
    d.rounded_rectangle([14, 14, 14 + w, 14 + h], 14, fill=(255, 255, 255), outline=(30, 30, 40), width=3)
    d.text((34, 24), text, font=f, fill=(30, 30, 40))
    if sub:
        d.text((34, 82), sub, font=fs, fill=(80, 80, 90))

def on_white(img):
    bg = Image.new("RGBA", img.size, (255, 255, 255, 255))
    bg.alpha_composite(img)
    return bg.convert("RGB")

# ── per-version content ──
TPS = {"TP1": ("TP1 BOOT\n3.3 V (0 V = boot mode)", "b"), "TP2": ("TP2 UART TX\n3.3 V idle", "p"), "TP3": ("TP3 UART RX\n3.3 V idle", "p"),
       "TP4": ("TP4 GND\n0 V (black probe)", "k"), "TP5": ("TP5 3V3\n3.25-3.35 V", "r"), "TP6": ("TP6 BAT+\n3.0-4.2 V", "o"),
       "TP7": ("TP7 EN (reset)\n3.3 V", "g")}

def make(ver, force=False):
    fps = footprints(BOARDS[ver])
    out = []
    pics = {}
    for side in ("top", "bottom"):
        raw = Image.open(render(ver, side, force)).convert("RGBA")
        if side == "top":
            M, s, res, n = fit(ver, side, raw, fps)
            print("%s %s: %d holes, %.2f px/mm, worst hole %.1f px" % (ver, side, n, s, res))
        else:                       # the holes are not see-through from the key side: same centre and scale, mirrored in x
            Mt = pics["top"][1]
            M = (lambda Mt: lambda x, y: (raw.size[0] - Mt(x, y)[0], Mt(x, y)[1]))(Mt)
        pics[side] = (on_white(raw), M, s)
    img, M, s = pics["top"]
    P = lambda ref, pad=None: half(M(*(fps[ref]["pads"][pad] if pad else fps[ref]["at"])))
    KS, OY = 0.86, 150                         # full views: board shrunk and moved down, room for the title
    def framed(im_):
        c = Image.new("RGB", (1600, 1200), "white")
        sm = im_.resize((round(1600 * KS), round(1200 * KS)), Image.LANCZOS)
        c.paste(sm, ((1600 - sm.size[0]) // 2, OY))
        return c
    small = framed(img)
    half = lambda p: (p[0] / 2 * KS + (1600 - 1600 * KS) / 2, p[1] / 2 * KS + OY)
    tag = "v14 e-paper" if ver == "v14" else "v15 LCD"
    conn = "J2" if ver == "v14" else "J5"
    # 1 full top, every test pad + connectors
    a = small.copy()
    side = {"TP1": "L", "TP4": "L", "TP5": "L", "TP7": "L", "TP2": "R", "TP3": "R", "TP6": "R"}
    items = []
    for tp, (txt, c) in TPS.items():
        ring(a, P(tp), 13, c, 4)
        items.append((txt, P(tp), side[tp], c))
    items += [("J3 magnet USB\npin 1 = N = VBUS 5 V", P("J3", "1"), "L", "r"),
              ("J4 battery\npin 1 GND / pin 2 +", P("J4", "2"), "R", "o"),
              ("J1 camera socket\n(pin 1 end)", P("J1", "1"), "R", "b"),
              ("%s %s socket\n(pin 1 end)" % (conn, "e-paper" if ver == "v14" else "LCD"), P(conn, "1"), "R", "b")]
    title(a, "%s board, component side: test pads" % tag, "TP4 = GND for every check", dry=True)
    column_labels(a, items, 28, y_min=145)
    title(a, "%s board, component side: test pads" % tag, "TP4 = GND for every check")
    out.append(save(a, "%s_top_ann.png" % ver)); out.append(save(small, "%s_top.png" % ver))
    # 2 bottom (key side)
    b_img, Mb, sb = pics["bottom"]
    bs = framed(b_img)
    out.append(save(bs, "%s_bottom.png" % ver))
    Pb = lambda ref, pad=None: half(Mb(*(fps[ref]["pads"][pad] if pad else fps[ref]["at"])))
    bb = bs.copy()
    sw = "SW1"
    items = [("Key pads\n(key mat side)", Pb("SW28") if "SW28" in fps else Pb(sw), "R", "b"),
             ("SHIFT", Pb("SW1"), "L", "k"), ("ON", Pb("SW50") if "SW50" in fps else Pb("SW3"), "R", "k"),
             ("Screen goes here\n(" + ("e-paper" if ver == "v14" else "LCD") + ", key side)", Pb("U1"), "L", "g")]
    title(bb, "%s board, key side" % tag, "Nothing to probe here: all test pads are on the component side", dry=True)
    column_labels(bb, items, 30, y_min=150)
    title(bb, "%s board, key side" % tag, "Nothing to probe here: all test pads are on the component side")
    out.append(save(bb, "%s_bottom_ann.png" % ver))
    # zoomed crops from the 3200 x 2400 raw (labels drawn at full res, then cropped)
    def crop(name, centre_refs, items, probes=(), rings=(), heading=None, sub=None, span=48.0, up=0.0):
        pts = [M(*(fps[r]["pads"][p] if p else fps[r]["at"])) for r, p in centre_refs]
        cx = sum(p[0] for p in pts) / len(pts); cy = sum(p[1] for p in pts) / len(pts) - up * s
        h = span * s; w = h * 4 / 3
        box = [int(cx - w / 2), int(cy - h / 2), int(cx + w / 2), int(cy + h / 2)]
        pad = Image.new("RGB", (img.size[0] + 4000, img.size[1] + 4000), "white"); pad.paste(img, (2000, 2000))
        c = pad.crop([v + 2000 for v in box]).resize((1600, 1200), Image.LANCZOS)
        if heading:
            title(c, heading, sub, dry=True)
        else:
            KEEP_OUT[:] = []
        k = 1600 / (box[2] - box[0])
        Q = lambda ref, pad=None: (lambda q: ((q[0] - box[0]) * k, (q[1] - box[1]) * k))(M(*(fps[ref]["pads"][pad] if pad else fps[ref]["at"])))
        for ref, pad, col, r in rings:
            ring(c, Q(ref, pad), r, col, 7)
        for ref, pad, colour, text, off in probes:
            probe(c, Q(ref, pad), colour, text, off)
        labels(c, [(t, Q(*rp), off, col) for t, rp, off, col in items], 38)
        if heading:
            title(c, heading, sub)
        out.append(save(c, name))
    crop("%s_tp_power_corner_ann.png" % ver, [("TP5", None), ("TP6", None), ("J3", "1")],
         [("TP5 3V3", ("TP5",), (-260, -150), "r"), ("TP6 BAT+", ("TP6",), (260, -160), "o"), ("TP7 EN", ("TP7",), (-250, 160), "g"),
          ("TP2 TX", ("TP2",), (120, 200), "p"), ("TP3 RX", ("TP3",), (330, 160), "p"),
          ("J3 pin 1 = N = VBUS", ("J3", "1"), (-200, -230), "r"), ("J4 pin 1 = GND", ("J4", "1"), (-120, 260), "k"),
          ("J4 pin 2 = BAT +", ("J4", "2"), (330, 60), "o")],
         rings=[("TP5", None, "r", 30), ("TP6", None, "o", 30), ("TP7", None, "g", 30), ("TP2", None, "p", 22), ("TP3", None, "p", 22)],
         heading="%s: power corner, component side" % tag, span=40.0)
    crop("%s_boot_recovery_ann.png" % ver, [("TP1", None), ("TP4", None), ("TP7", None)],
         [("1  Short TP1 to TP4 and hold", ("TP1",), (-300, -200), "b"), ("TP4 GND", ("TP4",), (-280, 140), "k"),
          ("2  Tap TP7 (EN) to GND once\n3  Let go of TP1 -> boot mode", ("TP7",), (380, 190), "g")],
         rings=[("TP1", None, "b", 34), ("TP4", None, "k", 34), ("TP7", None, "g", 34)],
         heading="BOOT / EN recovery (%s)" % tag, sub="Board will not flash? Force the download mode", span=50.0, up=5.0)
    crop("%s_meter_3v3_ann.png" % ver, [("TP5", None), ("TP4", None)], [],
         probes=[("TP5", None, "red", "Red probe: TP5 (3V3)", (-420, -260)), ("TP4", None, "black", "Black probe: TP4 (GND)", (-420, 240))],
         heading="Check 1: 3V3 rail (%s)" % tag, sub="Meter on DC volts. Expect 3.25-3.35 V", span=44.0)
    crop("%s_meter_bat_ann.png" % ver, [("TP6", None), ("TP4", None)], [],
         probes=[("TP6", None, "red", "Red probe: TP6 (BAT+)", (420, -240)), ("TP4", None, "black", "Black probe: TP4 (GND)", (-420, 240))],
         heading="Check 2: battery (%s)" % tag, sub="Battery plugged into J4. Expect 3.0-4.2 V", span=56.0, up=4.0)
    crop("%s_meter_vbus_ann.png" % ver, [("J3", "1"), ("TP4", None)], [],
         probes=[("J3", "1", "red", "Red probe: J3 pin 1 (VBUS, N end)", (-300, -260)), ("TP4", None, "black", "Black probe: TP4 (GND)", (-420, 240))],
         heading="Check 3: USB VBUS (%s)" % tag, sub="Magnetic cable attached. Expect 4.8-5.2 V", span=64.0, up=6.0)
    crop("%s_j1_pin1_ann.png" % ver, [("J1", None)],
         [("Pin 1 (silk mark)", ("J1", "1"), (-320, -200), "r"), ("J1 camera socket", ("J1",), (250, 260), "b")],
         rings=[("J1", "1", "r", 22)], heading="J1 camera socket: pin 1 end (%s)" % tag, span=22.0)
    crop("%s_%s_pin1_ann.png" % (ver, conn.lower()), [(conn, None)],
         [("Pin 1 (silk mark)", (conn, "1"), (-320, -220), "r"),
          ("%s %s socket" % (conn, "e-paper" if ver == "v14" else "LCD"), (conn,), (300, 240), "b")],
         rings=[(conn, "1", "r", 22)], heading="%s socket: pin 1 end (%s)" % (conn, tag),
         sub="LCD finger 1 (marked dot) goes to this end" if ver == "v15" else None, span=26.0)
    crop("%s_j3_j4_polarity_ann.png" % ver, [("J3", None), ("J4", None)],
         [("J3 pin 1 = N = VBUS (+5 V)", ("J3", "1"), (-260, -230), "r"), ("J3 pin 4 = GND", ("J3", "4"), (-60, 250), "k"),
          ("J4 pin 1 = GND (black)", ("J4", "1"), (60, 280), "k"), ("J4 pin 2 = BAT + (red)", ("J4", "2"), (330, -200), "o")],
         rings=[("J3", "1", "r", 24), ("J4", "1", "k", 24), ("J4", "2", "o", 24)],
         heading="Polarity: magnet header J3 and battery socket J4 (%s)" % tag, span=34.0)
    return out

def save(im, name):
    p = os.path.join(HERE, name)
    im.save(p, optimize=True)
    return p

if __name__ == "__main__":
    force = "render" in sys.argv[1:]
    res = []
    for v in ("v14", "v15"):
        res += make(v, force)
    for p in res:
        print("%-45s %4d KB" % (os.path.relpath(p, HERE), os.path.getsize(p) // 1024))
