# Builds the annotated bring-up pictures for both First Power-Up guides.
# Input: kicad-cli 3D renders (raw_{14,15}_{top,bottom}.png, 2384x4792) + footprint json (v14.json / v15.json).
import json, math, os, random
from PIL import Image, ImageDraw, ImageFont, ImageFilter

SCR = os.path.dirname(os.path.abspath(__file__))
REPO = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR"
OUT = os.path.join(REPO, "hardware", "renders_bringup")
BG = (243, 245, 241)
WF = r"C:\Windows\Fonts"

def F(size, bold=False):
    return ImageFont.truetype(os.path.join(WF, "segoeuib.ttf" if bold else "segoeui.ttf"), size)
def M(size, bold=False):
    return ImageFont.truetype(os.path.join(WF, "consolab.ttf" if bold else "consola.ttf"), size)

# board mm -> raw render px (fitted on the seven yellow test pads, residual < 2 px)
AX, BX = 31.143, -3481.24
AY, BY = 31.228, -1860.14
def mm2raw(X, Y, side="top"):
    px, py = AX * X + BX, AY * Y + BY
    if side == "bottom":
        px = 2382.6 - px
    return px, py

FP = {v: json.load(open(os.path.join(SCR, f"v{v}.json")))["fp"] for v in ("14", "15")}

def load(v, side):
    im = Image.open(os.path.join(SCR, f"raw_{v}_{side}.png")).convert("RGBA")
    a = im.split()[3]
    if side == "bottom":  # drop the grey ghosts of top-side parts
        a = a.point(lambda x: 255 if x >= 250 else 0)
    bg = Image.new("RGB", im.size, BG)
    bg.paste(im.convert("RGB"), mask=a)
    return bg

RAW = {}
def raw(v, side):
    k = (v, side)
    if k not in RAW:
        RAW[k] = load(v, side)
    return RAW[k]

class View:
    """A crop of a render, scaled; maps board mm to image px."""
    def __init__(self, v, side, x0, y0, x1, y1, out_w):
        self.v, self.side = v, side
        r = raw(v, side)
        ax0, ay0 = mm2raw(x0, y0, side)
        ax1, ay1 = mm2raw(x1, y1, side)
        L, R = sorted((ax0, ax1)); T, B = sorted((ay0, ay1))
        self.L, self.T = L, T
        self.s = out_w / (R - L)
        im = r.crop((int(L), int(T), int(R), int(B)))
        self.im = im.resize((out_w, int((B - T) * self.s)), Image.LANCZOS)
        self.d = ImageDraw.Draw(self.im, "RGBA")
    def p(self, X, Y):
        px, py = mm2raw(X, Y, self.side)
        return ((px - self.L) * self.s, (py - self.T) * self.s)
    def pad(self, ref, num):
        x, y, _ = FP[self.v][ref]["pads"][num]
        return self.p(x, y)
    def fp(self, ref):
        f = FP[self.v][ref]
        return self.p(f["x"], f["y"])

KIND = {
    "tp":    ((250, 204, 21), (20, 20, 20)),     # test pad: yellow tag, dark text
    "conn":  ((37, 99, 235), (255, 255, 255)),   # connector: blue
    "chip":  ((22, 101, 52), (255, 255, 255)),   # chip: dark green
    "part":  ((71, 85, 105), (255, 255, 255)),   # small parts: slate
    "warn":  ((185, 28, 28), (255, 255, 255)),   # careful: red
    "note":  ((255, 255, 255), (20, 20, 20)),
}

def text_size(d, txt, font):
    b = d.multiline_textbbox((0, 0), txt, font=font, spacing=2)
    return b[2] - b[0], b[3] - b[1]

def tag(d, xy, txt, kind="part", size=20, anchor="mm", bold=True):
    """Rounded label. anchor: m=centre, l=left edge at x, r=right edge at x; second letter t/m/b."""
    fill, fg = KIND[kind]
    font = F(size, bold)
    w, h = text_size(d, txt, font)
    padx, pady = int(size * .45), int(size * .3)
    W, H = w + 2 * padx, h + 2 * pady + 4
    x, y = xy
    x0 = {"l": x, "m": x - W / 2, "r": x - W}[anchor[0]]
    y0 = {"t": y, "m": y - H / 2, "b": y - H}[anchor[1]]
    d.rounded_rectangle((x0 + 2, y0 + 3, x0 + W + 2, y0 + H + 3), radius=int(size * .4), fill=(0, 0, 0, 70))
    d.rounded_rectangle((x0, y0, x0 + W, y0 + H), radius=int(size * .4), fill=fill, outline=(255, 255, 255) if kind != "note" else (40, 40, 40), width=2)
    d.multiline_text((x0 + padx, y0 + pady - 1), txt, font=font, fill=fg, spacing=2)
    return (x0, y0, x0 + W, y0 + H)

def ring(d, xy, r=14, color=(255, 255, 255), width=4):
    x, y = xy
    d.ellipse((x - r - 2, y - r - 2, x + r + 2, y + r + 2), outline=(0, 0, 0, 160), width=width + 2)
    d.ellipse((x - r, y - r, x + r, y + r), outline=color, width=width)

def edge_point(box, target):
    x0, y0, x1, y1 = box
    tx, ty = target
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    # nearest point on box edge towards target
    px = min(max(tx, x0), x1); py = min(max(ty, y0), y1)
    if x0 < tx < x1 and y0 < ty < y1:
        return cx, cy
    return px, py

def callout(d, target, tagxy, txt, kind="part", size=20, anchor="mm", r=12, ringcol=None):
    fill = KIND[kind][0]
    box = tag_box(d, tagxy, txt, size, anchor)
    ep = edge_point(box, target)
    d.line([ep, target], fill=(0, 0, 0, 150), width=6)
    d.line([ep, target], fill=fill if kind != "note" else (255, 255, 255), width=3)
    if r:
        ring(d, target, r, ringcol or (fill if kind not in ("tp", "note") else (255, 255, 255)))
    tag(d, tagxy, txt, kind, size, anchor)

def tag_box(d, xy, txt, size, anchor):
    font = F(size, True)
    w, h = text_size(d, txt, font)
    padx, pady = int(size * .45), int(size * .3)
    W, H = w + 2 * padx, h + 2 * pady + 4
    x, y = xy
    x0 = {"l": x, "m": x - W / 2, "r": x - W}[anchor[0]]
    y0 = {"t": y, "m": y - H / 2, "b": y - H}[anchor[1]]
    return (x0, y0, x0 + W, y0 + H)

def save(im, v, name, q=84):
    d = os.path.join(OUT, f"v{v}")
    os.makedirs(d, exist_ok=True)
    path = os.path.join(d, name)
    if name.endswith(".jpg"):
        im.convert("RGB").save(path, quality=q, optimize=True, progressive=True)
    else:
        im.save(path, optimize=True)
    kb = os.path.getsize(path) / 1024
    print(f"  {name:34s} {im.size[0]}x{im.size[1]}  {kb:.0f} KB")
    MANIFEST.append({"file": f"hardware/renders_bringup/v{v}/{name}", "w": im.size[0], "h": im.size[1], "kb": round(kb)})
    return path

MANIFEST = []

# ---------------------------------------------------------------- probes
def probe(d, tip, color, angle, length=330, label=None):
    """Meter probe: needle at `tip`, body going out at `angle` degrees (0 = right, 90 = down)."""
    a = math.radians(angle)
    ux, uy = math.cos(a), math.sin(a)
    tx, ty = tip
    n1 = (tx + ux * 34, ty + uy * 34)
    end = (tx + ux * length, ty + uy * length)
    body = (255, 40, 40) if color == "red" else (25, 25, 25)
    edge = (255, 255, 255)
    # needle
    d.line([tip, n1], fill=(0, 0, 0, 170), width=9)
    d.line([tip, n1], fill=(205, 210, 215), width=6)
    # finger guard + handle
    def seg(p0, p1, w, col):
        d.line([p0, p1], fill=col, width=w)
        for p in (p0, p1):
            d.ellipse((p[0] - w / 2, p[1] - w / 2, p[0] + w / 2, p[1] + w / 2), fill=col)
    g0 = (tx + ux * 40, ty + uy * 40); g1 = (tx + ux * 52, ty + uy * 52)
    seg(n1, end, 26, edge)
    seg(n1, end, 21, body)
    # guard ring
    px, py = -uy, ux
    for w, col in ((34, edge), (29, body)):
        d.line([(g0[0] + px * w / 2 + ux * 4, g0[1] + py * w / 2 + uy * 4), (g0[0] - px * w / 2 + ux * 4, g0[1] - py * w / 2 + uy * 4)], fill=col, width=10)
    ring(d, tip, 13, (255, 255, 255), 3)
    if label:
        mid = (tx + ux * (length * .62), ty + uy * (length * .62))
        font = F(18, True)
        w, h = text_size(d, label, font)
        d.rounded_rectangle((mid[0] - w / 2 - 7, mid[1] - h / 2 - 6, mid[0] + w / 2 + 7, mid[1] + h / 2 + 8), radius=6, fill=body, outline=edge, width=2)
        d.text((mid[0] - w / 2, mid[1] - h / 2 - 3), label, font=font, fill=(255, 255, 255))

def badge(im, lines, where="tl", width=None):
    """Big readable info block in a corner: [(text, kind)] kinds: mode/expect/plain."""
    d = ImageDraw.Draw(im, "RGBA")
    fonts = {"mode": F(24, True), "expect": F(30, True), "plain": F(19), "small": F(17)}
    cols = {"mode": (255, 255, 255), "expect": (250, 204, 21), "plain": (230, 235, 232), "small": (190, 200, 195)}
    sizes = [text_size(d, t, fonts[k]) for t, k in lines]
    W = max(w for w, h in sizes) + 32 if width is None else width
    H = sum(h + 10 for w, h in sizes) + 22
    x0 = 14 if where[1] == "l" else im.width - W - 14
    y0 = 14 if where[0] == "t" else im.height - H - 14
    d.rounded_rectangle((x0, y0, x0 + W, y0 + H), radius=12, fill=(17, 22, 19, 225), outline=(255, 255, 255, 200), width=2)
    y = y0 + 12
    for (t, k), (w, h) in zip(lines, sizes):
        d.text((x0 + 16, y - 2), t, font=fonts[k], fill=cols[k])
        y += h + 10
    return (x0, y0, x0 + W, y0 + H)

# ================================================================ BOARD MAPS
def board_map(v):
    out_w = 1000
    vw = View(v, "top", 112.6, 60.4, 187.5, 212.3, out_w)
    d = vw.d
    fp = FP[v]
    P = vw.p
    s = vw.s / AX  # px per mm (approx)
    # test pads
    tps = [("TP1", "TP1 BOOT", (-14, 4), "rm"), ("TP4", "TP4 GND", (-14, 6), "rm"),
           ("TP5", "TP5 3V3", (-12, -3), "rm"), ("TP7", "TP7 EN", (6, -6), "lm"),
           ("TP6", "TP6 BAT", (7, 0), "lm"), ("TP2", "TP2 TX", (-1, 7), "rm"), ("TP3", "TP3 RX", (2, 7), "lm")]
    # draw connectors/chips first, TPs last (on top)
    def at(ref, dx, dy):
        x, y = P(fp[ref]["x"], fp[ref]["y"])
        return x + dx * s, y + dy * s
    # J3
    j3 = [vw.pad("J3", n) for n in "1234"]
    callout(d, j3[3], P(149, 63.4), "J3 magnet header\npin 1 = N end (VBUS) ... pin 4 = GND", "conn", 19, "lm", r=10)
    for n, nm in zip("1234", ("VBUS", "D-", "D+", "GND")):
        x, y = vw.pad("J3", n)
        tag(d, (x, y - 4.2 * s), f"{n}\n{nm}".replace("D-", "D\u2212"), "conn", 13, "mm")
    # J4
    callout(d, at("J4", 0, 1), P(149, 69.6), "J4 battery\npin 1 GND (left) \u00b7 pin 2 + (right)", "conn", 19, "lm", r=16)
    # chips in the power corner
    callout(d, at("U2", 0, 0), P(160, 75.0), "U2 charger", "chip", 18, "lm", r=12)
    callout(d, at("U3", 0, 0), P(112.9, 88.5), "U3 3.3 V\nregulator", "chip", 17, "lm", r=12)
    callout(d, at("U7", 0, 0), P(147, 92.0), "U7 USB protection", "chip", 18, "lm", r=11)
    callout(d, at("D7", 0, 0), P(112.9, 66.8), "D7 TVS\nband \u2192 right", "warn", 17, "lm", r=13)
    if 0: callout(d, at("D1", 0, 0), P(141, 63.0), "D1 (band = SYS)", "part", 17, "lm", r=11)
    if 0: callout(d, at("Q1", 0, 0), P(147, 84.0), "Q1 reverse-battery FET", "part", 17, "lm", r=10)
    # ESP32
    callout(d, at("U1", 3, 0), P(140, 100.5), "U1 ESP32-S3\n(the brain + Wi-Fi)", "chip", 20, "lm", r=0)
    # screen socket
    if v == "14":
        callout(d, at("J2", 0, 0), P(152, 116.5), "J2 e-paper socket\nopening faces left", "conn", 19, "mm", r=0)
        callout(d, at("L1", 0, 0), P(152, 125.0), "L1 + D3-D5: e-paper booster", "part", 16, "mm", r=12)
    else:
        callout(d, at("J5", 0, 0), P(150, 117.5), "J5 LCD socket (30 pins)\nopening faces the slot \u2192", "conn", 19, "mm", r=0)
        callout(d, at("Q5", 0, 0), P(168, 70.5) if False else P(165.5, 85.0), "Q5 backlight", "part", 15, "lm", r=11)
        callout(d, vw.fp("R23"), P(172, 77.0), "R23 15 \u03a9", "part", 16, "mm", r=10)
        callout(d, at("Q4", 0, 0), P(163.5, 109.5), "Q4 LCD power", "part", 15, "lm", r=11)
        callout(d, P(181, 100.0), P(180.5, 108.5), "tail slot", "note", 15, "mm", r=0)
    # camera
    callout(d, at("J1", 0, 0), P(150, 172.0), "J1 camera socket\nopening faces up", "conn", 19, "mm", r=0)
    callout(d, at("U4", 0, 1.8), P(125, 168.5), "U4 / U5 camera\nregulators (2.8 / 1.5 V)", "chip", 16, "mm", r=0)
    callout(d, at("U6", 0, 0), P(128, 135.0), "U6 keypad chip", "chip", 17, "mm", r=13)
    for ref, txt, off, anc in tps:
        x, y = vw.fp(ref)
        callout(d, (x, y), (x + off[0] * s, y + off[1] * s), txt, "tp", 17, anc, r=13)
    # legend
    tag(d, (14, vw.im.height - 14), "Component side \u00b7 top edge (magnet) away from you", "note", 18, "lb", bold=True)
    save(vw.im, v, "map_top.jpg", 80)

def board_map_bottom(v):
    vw = View(v, "bottom", 112.6, 60.4, 187.5, 212.3, 700)
    d = vw.d
    s = vw.s / AX
    tag(d, vw.p(150, 175), "Key side: 50 gold key pads\n(keep them clean, no fingerprints)", "note", 17, "mm")
    if v == "15":
        callout(d, vw.p(181, 93.1), vw.p(160, 100), "LCD tail slot\n(seen from this side\nit is on the LEFT)", "note", 15, "mm", r=0)
    tag(d, (12, vw.im.height - 12), "Other side (keys), flipped left-to-right", "note", 15, "lb")
    save(vw.im, v, "map_bottom.jpg", 78)

def zoom_power(v):
    vw = View(v, "top", 114, 61.6, 148, 90, 900)
    d = vw.d; s = vw.s / AX
    fp = FP[v]
    for n, nm in zip("1234", ("VBUS", "D\u2212", "D+", "GND")):
        x, y = vw.pad("J3", n)
        tag(d, (x, vw.p(0, 65.2)[1]), f"pin {n}\n{nm}", "conn", 17, "mm")
        ring(d, (x, y), 11, KIND["conn"][0], 3)
    tag(d, (vw.p(0, 0)[0] + 0, vw.p(0, 0)[1]), "", "note", 1) if False else None
    if 0: callout(d, vw.p(119.0, 67.8), vw.p(114.6, 63.1), "N end", "conn", 17, "lm", r=0)
    callout(d, vw.fp("D7"), vw.p(114.4, 80.0), "D7: band on the RIGHT\n(towards J3)", "warn", 17, "lm", r=26)
    callout(d, vw.fp("U3"), vw.p(114.4, 86.5), "U3 3.3 V regulator\ndot top-left", "chip", 16, "lm", r=24)
    callout(d, vw.pad("D1", "1"), vw.p(127.5, 62.9), "D1 band end = SYS\n(red probe here)", "part", 16, "mm", r=12)
    callout(d, vw.fp("U2"), vw.p(141.5, 62.6), "U2 charger, dot top-left", "chip", 16, "mm", r=26)
    x1, y1 = vw.pad("J4", "1"); x2, y2 = vw.pad("J4", "2")
    tag(d, (x1 - 4, y1 + 3.8 * s), "1\nGND", "conn", 15, "mm")
    tag(d, (x2 + 6, y2 + 3.8 * s), "2\n+", "warn", 15, "mm")
    callout(d, vw.p(142.6, 78.5), vw.p(141.5, 88.8), "J4 battery (opening down)", "conn", 16, "mm", r=0)
    callout(d, vw.fp("Q1"), vw.p(146.7, 83.2), "Q1", "part", 16, "rm", r=20)
    callout(d, vw.fp("Q2"), vw.p(133.0, 64.2), "Q2", "part", 16, "mm", r=18)
    callout(d, vw.fp("U7"), vw.p(127.5, 89.0), "U7 USB protection", "chip", 16, "mm", r=26)
    for ref, txt in (("TP5", "TP5 3V3"), ("TP6", "TP6 BAT"), ("TP7", "TP7 EN"), ("TP2", "TP2 TX"), ("TP3", "TP3 RX")):
        ring(d, vw.fp(ref), 22, (250, 204, 21), 4)
    tag(d, (14, 14), "Zoom A \u00b7 power corner", "note", 20, "lt")
    save(vw.im, v, "zoom_power.jpg", 84)

def zoom_esp(v):
    vw = View(v, "top", 114, 80, 142, 118, 700)
    d = vw.d
    callout(d, vw.p(121.0, 103.3), vw.p(128.5, 108.5), "U1 ESP32-S3 module\npin-1 dot here", "chip", 17, "mm", r=12)
    for ref, txt, xy, anc in (("TP1", "TP1 BOOT", (124.5, 110.4), "lm"), ("TP4", "TP4 GND (black probe)", (124.0, 115.2), "lm"),
                              ("TP7", "TP7 EN", (131.0, 84.6), "lm"), ("TP5", "TP5 3V3", (118.5, 87.0), "mm")):
        callout(d, vw.fp(ref), vw.p(*xy), txt, "tp", 18, anc, r=20)
    tag(d, (14, 14), "Zoom B \u00b7 ESP32 and the recovery pads", "note", 19, "lt")
    save(vw.im, v, "zoom_esp32.jpg", 84)

def zoom_camera(v):
    vw = View(v, "top", 131, 148, 164, 167, 900)
    d = vw.d
    callout(d, vw.fp("J1"), vw.p(150, 150.0), "J1 camera socket \u00b7 the ribbon goes in here (opening faces up)", "conn", 17, "mm", r=0)
    callout(d, vw.fp("U4"), vw.p(133.0, 152.5), "U4 2.8 V", "chip", 16, "mm", r=22)
    callout(d, vw.fp("U5"), vw.p(133.0, 164.5), "U5 1.5 V", "chip", 16, "mm", r=22)
    callout(d, vw.fp("D2"), vw.p(161, 151.0), "D2", "part", 16, "mm", r=22)
    tag(d, (14, vw.im.height - 14), "Zoom C \u00b7 camera socket", "note", 19, "lb")
    save(vw.im, v, "zoom_camera.jpg", 84)

def zoom_screen(v):
    if v == "14":
        vw = View(v, "top", 152, 80, 186, 112, 800)
        d = vw.d
        callout(d, vw.fp("J2"), vw.p(168.5, 82.0), "J2 e-paper socket\nopening faces LEFT", "conn", 18, "mm", r=0)
        callout(d, vw.pad("J2", "1"), vw.p(184.0, 82.5), "pad 1", "conn", 15, "mm", r=8)
        callout(d, vw.fp("L1"), vw.p(160, 110.0), "L1 booster coil", "part", 16, "mm", r=34)
        callout(d, vw.fp("Q3"), vw.p(156.0, 94.0), "Q3", "part", 16, "mm", r=20)
        tag(d, (14, vw.im.height - 14), "Zoom D \u00b7 e-paper socket", "note", 19, "lb")
    else:
        vw = View(v, "top", 149, 77, 186, 109, 800)
        d = vw.d
        callout(d, vw.fp("J5"), vw.p(170, 95.0), "J5 LCD socket\nlid on the left,\nopening faces the slot \u2192", "conn", 17, "mm", r=0)
        callout(d, vw.pad("J5", "1"), vw.p(150.5, 106.5), "pad 1 end (silk tick)\nfinger 1 lands here", "warn", 15, "lm", r=10)
        callout(d, vw.fp("Q5"), vw.p(150.5, 78.6), "Q5 backlight", "part", 15, "lm", r=20)
        callout(d, vw.fp("R23"), vw.p(170.5, 79.0), "R23 15 \u03a9 (backlight resistor)", "warn", 15, "lm", r=18)
        callout(d, vw.fp("Q4"), vw.p(168.0, 106.0), "Q4 LCD power", "part", 15, "lm", r=20)
        callout(d, vw.fp("C19"), vw.p(150.5, 101.0), "C19", "part", 15, "lm", r=13)
        callout(d, vw.p(181.0, 87.0), vw.p(176.5, 83.5), "tail slot", "note", 15, "mm", r=0)
        tag(d, (vw.im.width - 14, vw.im.height - 14), "Zoom D \u00b7 LCD socket", "note", 19, "rb")
    save(vw.im, v, "zoom_screen.jpg", 84)

# ================================================================ MEASUREMENT PICTURES
REGION_UL = (114, 61.6, 146, 117)  # power corner down to TP4

def meas(v, name, red, black, mode, expect, extra=None, region=REGION_UL, out_w=620, red_ang=-20, black_ang=200, red_len=300, black_len=300, where="br"):
    vw = View(v, "top", *region, out_w)
    d = vw.d
    rp = red if isinstance(red, tuple) and len(red) == 2 and isinstance(red[0], float) else red
    def resolve(t):
        if t is None:
            return None
        kind, a, b = t
        if kind == "fp": return vw.fp(a)
        if kind == "pad": return vw.pad(a, b)
        if kind == "mm": return vw.p(a, b)
    r, k = resolve(red), resolve(black)
    if k: probe(d, k, "black", black_ang, black_len, "COM")
    if r: probe(d, r, "red", red_ang, red_len, "+")
    lines = [(mode, "mode"), (expect, "expect")]
    if extra: lines += [(e, "small") for e in extra]
    badge(vw.im, lines, where)
    save(vw.im, v, name, 82)

def all_meas(v):
    TP = lambda t: ("fp", t, None)
    # before power: resistance / beep
    meas(v, "m_short_3v3.jpg", TP("TP5"), TP("TP4"), "\u03a9 ohms (or beep)", "> 1 k\u03a9 after 5 s", ["no steady beep"], red_ang=-10, black_ang=200, where="tr")
    meas(v, "m_short_bat.jpg", TP("TP6"), TP("TP4"), "\u03a9 ohms", "> 10 k\u03a9", ["not a short"], red_ang=10, black_ang=200, where="tr")
    meas(v, "m_beep_j3gnd.jpg", ("pad", "J3", "4"), TP("TP4"), "beep (continuity)", "BEEPS", ["pin 4 = GND"], red_ang=-40, black_ang=200, red_len=220, where="br")
    meas(v, "m_beep_j3vbus.jpg", ("pad", "J3", "1"), TP("TP4"), "beep (continuity)", "NO beep", ["pin 1 = VBUS"], red_ang=-150, black_ang=200, red_len=150, where="br")
    # first power: DC volts
    meas(v, "m_vbus.jpg", ("pad", "J3", "1"), TP("TP4"), "DC volts (20 V)", "4.75 \u2013 5.25 V", ["J3 pin 1 leg"], red_ang=-150, black_ang=200, red_len=150, where="br")
    meas(v, "m_sys.jpg", ("pad", "D1", "1"), TP("TP4"), "DC volts (20 V)", "4.55 \u2013 4.7 V", ["D1 band end (SYS)"], red_ang=-35, black_ang=200, red_len=240, where="br")
    meas(v, "m_3v3.jpg", TP("TP5"), TP("TP4"), "DC volts (20 V)", "3.25 \u2013 3.35 V", ["TP5 3V3"], red_ang=-10, black_ang=200, where="br")
    meas(v, "m_en.jpg", TP("TP7"), TP("TP4"), "DC volts (20 V)", "about 3.3 V", ["TP7 EN (high = running)"], red_ang=-15, black_ang=200, where="br")
    meas(v, "m_bat.jpg", TP("TP6"), TP("TP4"), "DC volts (20 V)", "0 V or 4.1 \u2013 4.25 V", ["TP6 BAT, no cell fitted"], red_ang=5, black_ang=200, where="br")
    if v == "15":
        reg = (147, 76, 170, 112)
        meas(v, "m_r23.jpg", ("pad", "R23", "1"), ("pad", "R23", "2"), "DC volts (2 V / mV)", "240 \u2013 560 mV", ["\u00f7 15 = 16 \u2013 37 mA", "red on the right (3V3) end"], region=(150, 74, 168, 92), out_w=620, red_ang=-30, black_ang=210, red_len=260, black_len=260, where="bl")
        meas(v, "m_c19.jpg", ("pad", "C19", "1"), None, "DC volts (20 V)", "about 3.3 V", ["C19 top end, screen on", "black still on TP4"], region=(148, 94, 166, 110), out_w=620, red_ang=200, red_len=260, where="tr")

# ---------------------------------------------------------------- recovery 3 panels
def wire(d, p0, p1, col, sag=40, width=9):
    (x0, y0), (x1, y1) = p0, p1
    pts = []
    for i in range(41):
        t = i / 40
        x = x0 + (x1 - x0) * t
        y = y0 + (y1 - y0) * t
        nx, ny = -(y1 - y0), (x1 - x0)
        n = math.hypot(nx, ny) or 1
        bow = sag * math.sin(math.pi * t)
        pts.append((x + nx / n * bow, y + ny / n * bow))
    d.line(pts, fill=(0, 0, 0, 160), width=width + 4, joint="curve")
    d.line(pts, fill=col, width=width, joint="curve")
    for p in (p0, p1):
        d.ellipse((p[0] - 8, p[1] - 8, p[0] + 8, p[1] + 8), fill=(200, 200, 205), outline=(0, 0, 0), width=2)

def recovery(v):
    reg = (114, 80, 137, 118)
    panels = []
    for i in range(3):
        vw = View(v, "top", *reg, 460)
        d = vw.d
        t1, t4, t7 = vw.fp("TP1"), vw.fp("TP4"), vw.fp("TP7")
        for t in (t1, t4, t7):
            ring(d, t, 17, (250, 204, 21), 3)
        if i in (0, 1):
            wire(d, t1, (t4[0] + 2, t4[1]), (37, 99, 235), sag=55)
            tag(d, (t1[0] + 90, t1[1] - 50), "wire A: TP1 \u2194 TP4\nHOLD it there", "conn", 16, "lm")
        if i == 1:
            wire(d, t7, (t4[0] - 3, t4[1] + 4), (234, 88, 12), sag=-70)
            tag(d, (t7[0] + 40, t7[1] + 140), "wire B: TP7 \u2194 TP4\nTAP 0.2 s, let go", "warn", 16, "lm")
            # motion marks
            for k in range(3):
                d.arc((t4[0] - 34 - k * 10, t4[1] - 34 - k * 10, t4[0] + 34 + k * 10, t4[1] + 34 + k * 10), 200, 250, fill=(234, 88, 12), width=3)
        if i == 2:
            tag(d, (vw.im.width / 2, vw.im.height * .42), "Both wires off.\nClick UPLOAD now.\nWhen it is done:\ntap TP7 \u2194 TP4 once", "note", 19, "mm")
        title = ["1 \u00b7 Hold BOOT low", "2 \u00b7 Tap EN (reset)", "3 \u00b7 Release, upload"][i]
        tag(d, (12, 12), title, "note", 21, "lt")
        for ref, t in (("TP1", t1), ("TP4", t4), ("TP7", t7)):
            tag(d, (t[0] - 26, t[1]), {"TP1": "TP1 BOOT", "TP4": "TP4 GND", "TP7": "TP7 EN"}[ref], "tp", 14, "rm")
        save(vw.im, v, f"recovery_{i+1}.jpg", 82)

# ================================================================ LCD SIMULATIONS (v15)
BARS = [(255, 255, 255), (255, 255, 0), (0, 255, 255), (0, 255, 0), (255, 0, 255), (255, 0, 0), (0, 0, 255), (0, 0, 0)]
def selftest_screen(w=320, h=170):
    im = Image.new("RGB", (w, h), (0, 0, 0))
    d = ImageDraw.Draw(im)
    top, bh = 22, 92
    bw = (w - 8) / 8
    for i, c in enumerate(BARS):
        d.rectangle((4 + i * bw, top, 4 + (i + 1) * bw - 1, top + bh), fill=c)
    for x in range(4, w - 4):
        g = int((x - 4) / (w - 9) * 255)
        d.line((x, top + bh + 6, x, top + bh + 30), fill=(g, g, g))
    d.rectangle((0, 0, w - 1, h - 1), outline=(255, 255, 255), width=2)
    d.text((8, 5), "SELF-TEST  calc-3f9a21", font=M(13, True), fill=(255, 255, 255))
    d.text((8, h - 20), "LCD 320x170  backlight sweep", font=M(12), fill=(200, 200, 200))
    return im

def bezel(screen, caption, kind="ok"):
    sw, sh = screen.size
    W, H = sw + 40, sh + 74
    im = Image.new("RGB", (W, H), (30, 33, 31))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((4, 4, W - 5, sh + 36), radius=14, fill=(12, 12, 14))
    im.paste(screen, (20, 20))
    col = {"ok": (95, 199, 150), "bad": (255, 138, 128), "info": (230, 200, 120)}[kind]
    d.text((20, sh + 42), caption, font=F(18, True), fill=col)
    return im

def lcd_sims():
    g = selftest_screen()
    import numpy as np
    a = np.array(g)
    sims = []
    sims.append(("lcd_good.png", g, "Correct: W Y C G M R B K", "ok"))
    sims.append(("lcd_rb_swap.png", Image.fromarray(a[:, :, ::-1].copy()), "Red/blue swapped: LCD_RGB_ORDER=1", "bad"))
    sims.append(("lcd_inverted.png", Image.fromarray(255 - a), "Negative: LCD_INVERT=0", "bad"))
    sims.append(("lcd_rotated.png", g.rotate(180), "Upside down: LCD_ROTATION=3", "bad"))
    # offset: picture shifted, noise strip on one edge
    off = Image.new("RGB", g.size, (0, 0, 0)); off.paste(g, (0, 35))
    rnd = random.Random(4)
    dd = ImageDraw.Draw(off)
    for y in range(0, 35):
        for x in range(0, 320, 2):
            c = rnd.choice(BARS[:7] + [(rnd.randrange(256), rnd.randrange(256), rnd.randrange(256))])
            dd.point((x, y), fill=c); dd.point((x + 1, y), fill=c)
    sims.append(("lcd_offset.png", off, "Noise strip: memory offset, ask", "info"))
    # tearing / sparkles
    t = g.copy(); ta = np.array(t)
    for y in range(40, 170, 23):
        ta[y:y + 6] = np.roll(ta[y:y + 6], rnd.randrange(-40, 40), axis=1)
    for _ in range(900):
        ta[rnd.randrange(170), rnd.randrange(320)] = [rnd.randrange(256) for _ in range(3)]
    sims.append(("lcd_sparkle.png", Image.fromarray(ta), "Sparkles/tearing: SPI too fast", "info"))
    glow = Image.new("RGB", (320, 170), (38, 40, 46))
    gd = ImageDraw.Draw(glow)
    for r in range(60, 0, -4):
        c = 38 + (60 - r) // 3
        gd.ellipse((160 - r * 3, 85 - r * 1.4, 160 + r * 3, 85 + r * 1.4), fill=(c, c + 2, c + 8))
    sims.append(("lcd_glow_only.png", glow, "Light, no picture: tail reversed/not in", "bad"))
    sims.append(("lcd_dark.png", Image.new("RGB", (320, 170), (0, 0, 0)), "Totally dark: tail out or backlight path", "bad"))
    for name, im, cap, kind in sims:
        save(bezel(im, cap, kind), "15", name)

def preview_mock():
    w, h = 320, 170
    im = Image.new("RGB", (w, h), (226, 222, 210))
    d = ImageDraw.Draw(im)
    # paper texture + printed problem
    rnd = random.Random(7)
    for _ in range(1500):
        x, y = rnd.randrange(w), rnd.randrange(h)
        c = rnd.randrange(205, 232)
        d.point((x, y), fill=(c, c - 3, c - 12))
    for y in range(36, 150, 18):
        d.line((0, y, w, y), fill=(190, 205, 225))
    d.text((70, 38), "3x\u00b2 + 5x \u2212 2 = 0", font=F(24, True), fill=(30, 30, 40))
    d.text((92, 104), "solve for x", font=F(15), fill=(50, 50, 60))
    d.rectangle((62, 34, 262, 126), outline=(245, 158, 11), width=2)
    # bands
    d.rectangle((0, 0, w, 20), fill=(0, 0, 0))
    d.text((6, 2), "FOCUSED", font=M(14, True), fill=(34, 197, 94))
    d.text((120, 3), "Normal", font=M(12), fill=(220, 220, 220))
    d.text((272, 3), "84%", font=M(12), fill=(220, 220, 220))
    d.rectangle((0, h - 20, w, h), fill=(0, 0, 0))
    d.text((6, h - 17), "= Scan  > Focus  ^v Effort  AC Back", font=M(11), fill=(220, 220, 220))
    d.text((276, h - 17), "15fps", font=M(11, True), fill=(250, 204, 21))
    # brackets
    for (x, y, sx, sy) in ((20, 28, 1, 1), (300, 28, -1, 1), (20, 142, 1, -1), (300, 142, -1, -1)):
        d.line((x, y, x + 18 * sx, y), fill=(255, 255, 255), width=3)
        d.line((x, y, x, y + 18 * sy), fill=(255, 255, 255), width=3)
    for x in range(40, 290, 12):
        d.line((x, 24, x + 6, 24), fill=(255, 255, 255), width=1)
        d.line((x, 146, x + 6, 146), fill=(255, 255, 255), width=1)
    d.line((154, 85, 166, 85), fill=(255, 255, 255), width=2); d.line((160, 79, 160, 91), fill=(255, 255, 255), width=2)
    big = im.resize((640, 340), Image.NEAREST)
    out = Image.new("RGB", (700, 486), (30, 33, 31))
    od = ImageDraw.Draw(out, "RGBA")
    out.paste(big, (30, 24))
    lab = [((30 + 6 * 2, 24 + 10 * 2), (30, 392), "top band: FOCUSING (amber) \u2192 FOCUSED (green)"),
           ((30 + 286 * 2, 24 + 160 * 2), (30, 420), "bottom right: frame rate, expect about 12\u201318 fps (estimate)"),
           ((30 + 160 * 2, 24 + 63 * 2), (30, 448), "middle: amber box = writing found, + = centre")]
    for i, (pt, tp, txt) in enumerate(lab):
        od.text(tp, f"{i+1}  {txt}", font=F(18, True), fill=(235, 240, 236))
    save(out, "15", "preview_mock.png")

if __name__ == "__main__":
    for v in ("14", "15"):
        print("v" + v)
        board_map(v); board_map_bottom(v); zoom_power(v); zoom_esp(v); zoom_camera(v); zoom_screen(v)
        all_meas(v); recovery(v)
    print("v15 LCD sims")
    lcd_sims(); preview_mock()
    json.dump(MANIFEST, open(os.path.join(SCR, "manifest_new.json"), "w"), indent=1)
