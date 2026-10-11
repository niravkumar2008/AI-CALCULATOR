"""Annotated pictures for assembly_guide_v15_lcd.html (v15-LCD copy of final_assembly/annotate_guide.py; Pillow only).

The v14 photos are not re-made here: their annotated copies were copied from final_assembly/renders/photos/.

    python annotate_guide.py            all step renders + photos
    python annotate_guide.py s07 photo  only names starting with these

Step renders: renders/steps/<name>.png -> renders/steps/<name>_ann.png. Label targets come from
renders/steps/anchors.json (written by `build_final_assembly_v15.py steps`), so a re-render keeps the arrows on
the right parts. Photos (Nirav's own, private): resized to 1500 px wide, saved as renders/photos/<name>.jpg
(quality 85) and <name>_ann.jpg. Photo label points are in that 1500 x 2000 frame.
Label = (text, target, label position). target = anchor name or (x, y); position = (dx, dy) from the target.
"""
import json, os, sys
from PIL import Image, ImageDraw, ImageFont, ImageOps

HERE = os.path.dirname(os.path.abspath(__file__))
STEPS = os.path.join(HERE, "renders", "steps")
PHOTOS = os.path.join(HERE, "renders", "photos")
UPLOADS = r"C:\Users\r_kas\.claude\uploads\0abf7810-ba1d-425f-a86b-93460f8cb6c8"
FONT = r"C:\Windows\Fonts\segoeuib.ttf"
if not os.path.exists(FONT):
    FONT = r"C:\Windows\Fonts\arialbd.ttf"
FS = 54                                   # label font size in px (images are 1500-1600 wide)
COL = {"r": (200, 20, 60), "b": (0, 80, 200), "g": (0, 120, 60), "o": (200, 90, 0), "k": (30, 30, 40), "p": (120, 40, 170)}

# ── step renders ──
R = {
    "s01_open_shell": [("1 Key mat (keep)", "mat", (-420, 150), "b"), ("2 Front shell", "shell", (-330, 200), "k"),
                       ("3 Back cover", "cover", (-80, 330), "k"), ("Display window", "window", (-420, -230), "g")],
    "s04_parts": [("1 Board (v15-LCD)", "board", (-420, -330), "k"), ("2 LCD panel (ER-TFT019-1)", "panel", (-150, -230), "b"),
                  ("3 Camera", "camera", (240, -40), "g"), ("4 Battery 150 mAh #1317", "battery", (200, -85), "o"),
                  ("5 Magnet piece", "magnet", (220, 140), "r")],
    "s05a_lcd_tail": [("LCD panel", "panel", (250, -330), "b"), ("0.1 mm tape", "tape", (380, -130), "p"),
                      ("J5", "j5", (-150, -300), "g"), ("Stiff end: straight into J5", "stiff", (-150, 260), "k"),
                      ("Board", "board", (-250, -150), "k"), ("One gentle bow, no crease", "bow", (100, 280), "r"),
                      ("Slot (1 × 20 mm)", "slot", (0, 300), "o")],
    "s05b_lcd_j5": [("J5 socket", "j5", (380, -250), "g"), ("Lid (flip up)", "latch", (-200, -150), "b"),
                    ("Tail comes down the slot", "slot", (-300, -330), "r"), ("Bow", "bow", (300, -80), "r"),
                    ("Finger 1 end (silk tick)", "pin1", (260, 300), "o"), ("LED fingers 20–24: top half", "led", (230, -370), "o")],
    "s05c_lcd_on_board": [("LCD on the KEY side", "panel", (-360, -330), "b"), ("Tail end", "tail", (-100, 260), "r"),
                          ("Key pads: keep clear", "keypads", (-330, 330), "k")],
    "s06a_camera": [("Camera, lens facing you", "camera", (180, -260), "g"), ("Ribbon flat, no twist", "ribbon", (200, 60), "r"),
                    ("J1", "j1", (220, 200), "b")],
    "s06b_camera_j1": [("J1 socket", "j1", (300, 330), "b"), ("Ribbon straight in, all the way", "ribbon", (-620, -260), "r")],
    "s07_magnet_j3": [("Pin 1 = N = VBUS (+)", "pin1", (-560, 210), "r"), ("Pin 4 = GND (−)", "pin4", (120, 260), "k"),
                      ("VBUS leg (pen mark)", "leg1", (-600, -230), "r"), ("J3", "j3", (330, -120), "b"),
                      ("Push in ≈ 4.5 mm", "face", (330, -300), "o")],
    "s07b_magnet_in": [("Pin 1 = N = VBUS", "pin1", (-520, 200), "r"), ("J3: legs fully in", "j3", (350, -90), "b"),
                       ("Magnet face", "face", (260, -330), "k")],
    "s08a_keymat": [("Key mat, domes up", "mat", (260, -220), "b"), ("Half-round notches at the bottom posts", "notch", (250, 170), "o"),
                    ("Every post through its hole", "posts", (-480, 120), "k")],
    "s08b_board_in": [("Board, key side down", "board", (220, -260), "g"), ("H6: tightest hole", "h6", (300, 140), "r"),
                      ("H6's post", "h6post", (-460, 230), "r"), ("Magnet rides along", "magnet", (260, -100), "k"),
                      ("U-notch", "notch", (-380, 80), "o")],
    "s08c_magnet_flush": [("Face flush with the wall", "face", (-180, 330), "r"), ("U-notch in top wall", "notch", (-120, -330), "o")],
    "s09_battery": [("Battery 150 mAh #1317 on the back floor", "battery", (-80, -330), "o"), ("J4", "j4", (330, -240), "b"),
                    ("Plug: push by its body", "plug", (380, -60), "b"), ("TP7 keep clear", "tp7", (-560, 80), "r"),
                    ("TP1 / TP4 keep clear", "tp1", (-150, 120), "r")],
    "s10a_cover_lowering": [("Back cover straight down", "cover", (330, 60), "b"), ("7 mm window over the camera", "window", (220, -260), "g"),
                            ("6 screws: snug, not tight", "screws", (-150, 330), "k")],
    "s10b_closed_back": [("7 mm camera window", "window", (260, -180), "g")],
    "s11_finished_front": [("LCD screen", "screen", (-200, -330), "b"), ("ON", "on", (260, -240), "r")],
}

PH = {}   # v14 photos: copied as finished _ann.jpg files, nothing to redo

def font(sz=FS):
    return ImageFont.truetype(FONT, sz)

def arrow(d, p0, p1, col, w=11):
    import math
    ang = math.atan2(p1[1] - p0[1], p1[0] - p0[0])
    L = 44
    tip = p1
    back = (tip[0] - L * math.cos(ang), tip[1] - L * math.sin(ang))
    left = (back[0] + 22 * math.sin(ang), back[1] - 22 * math.cos(ang))
    right = (back[0] - 22 * math.sin(ang), back[1] + 22 * math.cos(ang))
    for ww, cc in ((w + 8, "white"), (w, col)):
        d.line([p0, back], fill=cc, width=ww)
    d.polygon([tip, left, right], fill=col, outline="white", width=4)

def label_box(d, text, pos, col, f):
    """Rounded box centred on pos; returns its rectangle."""
    l, t, r, b = d.textbbox((0, 0), text, font=f)
    w, h = r - l + 36, b - t + 24
    x0, y0 = pos[0] - w / 2, pos[1] - h / 2
    rect = [x0, y0, x0 + w, y0 + h]
    return rect, (x0 + 18 - l, y0 + 12 - t)

def draw(im, labels, pts=None, number=True):
    im = im.convert("RGB")
    W, H = im.size
    d = ImageDraw.Draw(im)
    f, fn = font(), font(46)
    placed = []
    for i, (text, tgt, off, c) in enumerate(labels, 1):
        p = pts[tgt] if isinstance(tgt, str) else tgt
        col = COL[c]
        import re
        num = text.split(" ", 1)[0] if re.match(r"^\d+ [A-Z]", text) else ""
        if num:
            text = text.split(" ", 1)[1]
        pos = [p[0] + off[0], p[1] + off[1]]
        rect, tp = label_box(d, text, pos, col, f)
        # keep the box inside the picture
        dx = max(0, (100 if num else 12) - rect[0]) - max(0, rect[2] - (W - 12))
        dy = max(0, 12 - rect[1]) - max(0, rect[3] - (H - 12))
        rect = [rect[0] + dx, rect[1] + dy, rect[2] + dx, rect[3] + dy]; tp = (tp[0] + dx, tp[1] + dy)
        cx, cy = (rect[0] + rect[2]) / 2, (rect[1] + rect[3]) / 2
        # arrow from the box edge to the target
        ex = min(max(p[0], rect[0]), rect[2]); ey = min(max(p[1], rect[1]), rect[3])
        if abs(ex - p[0]) + abs(ey - p[1]) > 40:
            arrow(d, (ex, ey), p, col)
        else:
            d.ellipse([p[0] - 14, p[1] - 14, p[0] + 14, p[1] + 14], fill=col, outline="white", width=4)
        placed.append((rect, tp, text, col, num))
    for rect, tp, text, col, num in placed:     # boxes on top of every arrow
        d.rounded_rectangle([rect[0] - 4, rect[1] - 4, rect[2] + 4, rect[3] + 4], 16, fill="white")
        d.rounded_rectangle(rect, 14, fill=col)
        d.text(tp, text, font=f, fill="white")
        if num:                                  # numbered circle hanging off the box's left end
            r = 40; cx, cy = rect[0] - r + 6, (rect[1] + rect[3]) / 2
            d.ellipse([cx - r - 5, cy - r - 5, cx + r + 5, cy + r + 5], fill="white")
            d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(20, 20, 30))
            d.text((cx, cy), num, font=fn, fill="white", anchor="mm")
    return im

def px_points(name):
    A = json.load(open(os.path.join(STEPS, "anchors.json")))[name]
    W, H = A["size"]; c = A["vp_points"]["_c"]
    s = H / (2 * c[1])                       # saved image keeps the viewport's vertical extent
    return {k: (W / 2 + (q[0] - c[0]) * s, H / 2 + (q[1] - c[1]) * s) for k, q in A["vp_points"].items()}

def trim(im, keep):
    """Crop white margins (wide portrait renders), keeping every label box. Returns image, offset."""
    from PIL import ImageChops
    bg = Image.new("RGB", im.size, im.getpixel((2, 2)))
    g = ImageChops.difference(im.convert("RGB"), bg).convert("L").point(lambda v: 255 if v > 18 else 0)
    bb = g.getbbox() or (0, 0) + im.size
    x0, x1 = bb[0], bb[2]
    for x, _ in keep:
        x0, x1 = min(x0, x - 60), max(x1, x + 60)
    x0, x1 = max(0, x0 - 50), min(im.size[0], x1 + 50)
    return im.crop((x0, 0, x1, im.size[1])), x0

def run(only):
    want = lambda n: not only or any(n.startswith(o) for o in only)
    os.makedirs(PHOTOS, exist_ok=True)
    out = []
    for name, labels in R.items():
        if not want(name):
            continue
        im = Image.open(os.path.join(STEPS, name + ".png")).convert("RGB")
        pts = px_points(name)
        a = draw(im, labels, pts)
        if a.size[1] >= a.size[0]:            # portrait: trim side margins so it shows bigger on a phone
            a, _ = trim(a, [])
        a = a.quantize(256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE)
        p = os.path.join(STEPS, name + "_ann.png"); a.save(p, optimize=True); out.append(p)
    for name, (src, labels) in PH.items():
        if not (want(name) or want("photo")):
            continue
        im = ImageOps.exif_transpose(Image.open(os.path.join(UPLOADS, src))).convert("RGB")
        im = im.resize((1500, round(im.size[1] * 1500 / im.size[0])), Image.LANCZOS)
        im.save(os.path.join(PHOTOS, name + ".jpg"), quality=85, optimize=True)
        a = draw(im, labels)
        p = os.path.join(PHOTOS, name + "_ann.jpg"); a.save(p, quality=85, optimize=True); out.append(p)
    for p in out:
        print("%-60s %4d KB" % (os.path.relpath(p, HERE), os.path.getsize(p) // 1024))

if __name__ == "__main__":
    run(sys.argv[1:])
