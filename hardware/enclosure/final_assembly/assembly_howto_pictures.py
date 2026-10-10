"""Extra pictures for the v14 Calculator Assembly Guide (assembly_guide.html). Pillow only, no Fusion.

    python assembly_howto_pictures.py

Writes
  renders/howto/*.png     drawn how-to diagrams (FPC latch, battery tape L, magnet meter check, J4 polarity,
                          H6 filing, "cover won't close" check map, stiffener)
  renders/gallery/*.jpg   small thumbnails for the parts gallery and the glossary (crops of existing renders/photos)
  renders/photos/*_ann.jpg  two of Nirav's photos with labels (back cover battery corner, key mat contact side)
Numbers come from battery_upgrade.md section 8, assembly_report.md and verification/01, 03 (see the guide's footer).
Label style is the one in ../annotate_guides.py (imported).
"""
import math, os, sys
from PIL import Image, ImageDraw, ImageOps

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import annotate_guides as AG  # noqa: E402

R = os.path.join(HERE, "renders")
HOW = os.path.join(R, "howto"); GAL = os.path.join(R, "gallery"); PH = os.path.join(R, "photos")
UP = os.path.join(os.path.expanduser("~"), ".claude", "uploads")
for d in (HOW, GAL):
    os.makedirs(d, exist_ok=True)
font, COL, arrow = AG.font, AG.COL, AG.arrow
INK = (25, 25, 35); GREY = (120, 125, 135)
BOARD = (54, 92, 70); BODY = (222, 214, 196); LATCH = (45, 45, 50); RIB = (190, 120, 40); GOLD = (225, 180, 50)
STIFF = (150, 95, 30); NAVY = (52, 58, 82)


def save(im, path, jpg=False):
    if jpg:
        im.convert("RGB").save(path, quality=84, optimize=True)
    else:
        im = im.convert("RGB").quantize(256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
        im.save(path, optimize=True)
    print("%-60s %4d KB" % (os.path.relpath(path, HERE), os.path.getsize(path) // 1024))


def blank(w, h):
    im = Image.new("RGB", (w, h), "white")
    return im, ImageDraw.Draw(im)


def text(d, xy, s, fs=32, c=INK, bold=False, anchor="la"):
    y = xy[1]
    for ln in s.split("\n"):
        d.text((xy[0], y), ln, font=font(fs, bold), fill=c, anchor=anchor)
        y += fs + 8


def tag(d, xy, s, c="k", fs=30):
    """Small solid label (no arrow)."""
    f = font(fs)
    lines = s.split("\n")
    tw = max(d.textbbox((0, 0), l, font=f)[2] for l in lines)
    h = len(lines) * (fs + 8) + 14
    x, y = xy
    d.rounded_rectangle([x - 3, y - 3, x + tw + 27, y + h + 3], 12, fill="white")
    d.rounded_rectangle([x, y, x + tw + 24, y + h], 10, fill=COL[c])
    yy = y + 7
    for l in lines:
        d.text((x + 12, yy), l, font=f, fill="white"); yy += fs + 8
    return (x, y, x + tw + 24, y + h)


def title(d, s, sub=None, w=1600):
    d.rectangle([0, 0, w, 74], fill=COL["b"])
    d.text((24, 37), s, font=font(40), fill="white", anchor="lm")
    if sub:
        text(d, (24, 88), sub, 28, GREY)


def panel_frame(d, box, n, head, ok=None):
    x0, y0, x1, y1 = box
    col = COL["g"] if ok is True else COL["r"] if ok is False else (200, 205, 215)
    d.rounded_rectangle(box, 16, outline=col, width=4 if ok is not None else 3)
    d.ellipse([x0 + 14, y0 + 14, x0 + 70, y0 + 70], fill=COL["k"] if ok is None else col)
    d.text((x0 + 42, y0 + 42), str(n), font=font(34 if len(str(n)) < 2 else 22), fill="white", anchor="mm")
    text(d, (x0 + 84, y0 + 18), head, 31, INK, True)


# ───────────────────────── A. FPC latch, side view ─────────────────────────
def fpc_side(d, ox, oy, state):
    """state: closed_empty | open_empty | open_in | closed_in.  Board line at oy, socket mouth on the left."""
    d.rectangle([ox, oy, ox + 620, oy + 26], fill=BOARD)
    text(d, (ox + 540, oy + 30), "board", 24, GREY)
    bx0, bx1, bt = ox + 230, ox + 430, oy - 70
    d.rectangle([bx0, bt, bx1, oy], fill=BODY, outline=(150, 140, 120), width=3)    # socket body
    d.rectangle([bx0 - 10, oy - 30, bx0 + 4, oy - 10], fill=(150, 140, 120))           # mouth lip
    if state.endswith("empty"):
        text(d, (bx0 - 150, oy - 50), "opening", 24, GREY)
        arrow(d, (bx0 - 60, oy - 30), (bx0 - 12, oy - 20), GREY, w=4)
    hx, hy = bx1, bt                                                                 # latch hinge (far side)
    if state.startswith("closed"):
        d.rectangle([bx1 - 120, bt - 18, bx1 + 14, bt], fill=LATCH)
    else:
        d.rectangle([hx - 4, hy - 140, hx + 14, hy], fill=LATCH)
        if state == "open_empty":
            d.arc([hx - 130, hy - 130, hx + 10, hy + 10], 200, 265, fill=COL["b"], width=5)
    if state.endswith("_in"):
        tip = bx0 + 85                                                               # 2-3 mm inside
        d.rectangle([ox - 10, oy - 52, tip, oy - 38], fill=RIB)
        d.rectangle([bx0 - 110, oy - 54, bx0 + 10, oy - 36], fill=STIFF)              # stiffener
        d.rectangle([tip - 75, oy - 38, tip, oy - 33], fill=GOLD)                     # fingers (underside)
        return tip
    return None


def fpc_latch():
    im, d = blank(1600, 1720)
    title(d, "Ribbon sockets J1 and J2: open, insert, close", "Seen from the side. Both sockets work the same way: the dark bar (latch) is on the side AWAY from the opening.")
    boxes = [(30, 150, 785, 720), (815, 150, 1570, 720), (30, 750, 785, 1320), (815, 750, 1570, 1320)]
    heads = ["Closed and empty (as it comes)", "Open: flip the dark bar up ~90°", "Slide the ribbon in until it stops", "Press the bar flat: locked"]
    states = ["closed_empty", "open_empty", "open_in", "closed_in"]
    for i, (b, h, st) in enumerate(zip(boxes, heads, states)):
        panel_frame(d, b, i + 1, h)
        tip = fpc_side(d, b[0] + 70, b[1] + 360, st)
        x0, y0 = b[0], b[1]
        if st == "closed_empty":
            text(d, (x0 + 30, y0 + 440), "Never push a ribbon into a CLOSED socket:\nit jams under the contacts and bends them.", 27)
        if st == "open_empty":
            arrow(d, (x0 + 640, y0 + 330), (x0 + 520, y0 + 200), COL["b"], w=7)
            text(d, (x0 + 30, y0 + 440), "Fingernail or plastic pick under the bar's\nMIDDLE. Lift; it hinges, it doesn't come off.\nStop at ~90°. Never pry the white body.", 27)
        if st == "open_in":
            arrow(d, (x0 + 30, y0 + 210), (x0 + 200, y0 + 210), COL["r"], w=7)
            d.line([b[0] + 300, b[1] + 260, b[0] + 300, b[1] + 300], fill=COL["o"], width=4)
            d.line([tip, b[1] + 260, tip, b[1] + 300], fill=COL["o"], width=4)
            d.line([b[0] + 300, b[1] + 280, tip, b[1] + 280], fill=COL["o"], width=4)
            text(d, (b[0] + 300, b[1] + 220), "2–3 mm in", 26, COL["o"], True)
            text(d, (x0 + 30, y0 + 440), "Square, no force, until it stops. The gold\nfingers disappear completely; the stiff\nend's edge sits right at the mouth.", 27)
        if st == "closed_in":
            arrow(d, (x0 + 500, y0 + 140), (x0 + 450, y0 + 285), COL["g"], w=7)
            text(d, (x0 + 30, y0 + 440), "Press down at the middle, both ends level.\nThen a very light tug on the ribbon:\nit must not move.", 27)
    # bottom strip: top view square vs crooked
    for k, (ok, x) in enumerate(((True, 30), (False, 815))):
        b = (x, 1350, x + 755, 1700)
        panel_frame(d, b, "OK" if ok else "NO", "Seen from above: SQUARE" if ok else "Seen from above: CROOKED", ok)
        sx, sy = x + 220, 1500
        d.rectangle([sx, sy, sx + 330, sy + 70], fill=BODY, outline=(150, 140, 120), width=3)
        d.rectangle([sx, sy + 70, sx + 330, sy + 92], fill=LATCH)
        if ok:
            d.rectangle([sx + 40, sy - 80, sx + 290, sy + 20], fill=RIB)
            text(d, (x + 30, 1610), "Same depth at both ends. No gold showing.", 26)
        else:
            d.polygon([(sx + 20, sy - 60), (sx + 260, sy - 90), (sx + 300, sy + 20), (sx + 60, sy + 40)], fill=RIB)
            d.rectangle([sx + 40, sy + 0, sx + 100, sy + 10], fill=GOLD)
            text(d, (x + 30, 1610), "Gold visible at one end = not home. Open the bar,\npull it out, try again. Crooked = wrong pins.", 26)
    save(im, os.path.join(HOW, "fpc_latch_steps.png"))


# ───────────────────────── B. stiffener close-up (glossary) ─────────────────────────
def stiffener():
    im, d = blank(900, 520)
    d.rectangle([60, 220, 840, 300], fill=RIB)
    d.rectangle([600, 214, 840, 306], fill=STIFF)
    for k in range(12):
        d.rectangle([700 + k * 11, 228, 706 + k * 11, 292], fill=GOLD)
    tag(d, (60, 60), "Thin, bendy ribbon (FPC)", "o")
    tag(d, (470, 360), "Stiffener: the thick, stiff end", "k")
    tag(d, (540, 60), "Gold fingers (contacts)", "b")
    arrow(d, (380, 120), (300, 240), COL["o"], w=6)
    arrow(d, (740, 360), (700, 300), COL["k"], w=6)
    arrow(d, (760, 130), (760, 228), COL["b"], w=6)
    text(d, (60, 450), "Only the stiff end goes in. Bend the ribbon, never the stiff end.", 26, GREY)
    save(im, os.path.join(GAL, "g_stiffener.png"))


# ───────────────────────── C. battery tape L (to scale) ─────────────────────────
def battery_tape():
    S = 30.0                                           # px per mm
    im, d = blank(1600, 1200)
    title(d, "Battery on the back-cover floor: where the two tape strips go",
          "Inside of the back cover, top end AWAY from you (left/right as on the calculator's front). Drawn to scale: 30 px = 1 mm.")
    X0, Y0 = 330, 300                                  # cell top-left
    mm = lambda x, y: (X0 + x * S, Y0 + y * S)         # x right, y down from the cell's top-left
    # surroundings
    d.rectangle([60, Y0 - 0.8 * S - 26, 1560, Y0 - 0.8 * S], fill=(205, 208, 216))
    text(d, (420, Y0 - 0.8 * S - 66), "top wall lip: cell's top edge 0.8 mm from it (almost touching)", 26, GREY)
    bx, by = X0 - (2.6 + 2.75) * S, Y0 + 3.5 * S
    d.ellipse([bx - 2.75 * S, by - 2.75 * S, bx + 2.75 * S, by + 2.75 * S], fill=(205, 208, 216), outline=GREY, width=3)
    d.ellipse([bx - 0.9 * S, by - 0.9 * S, bx + 0.9 * S, by + 0.9 * S], fill="white")
    text(d, (40, by + 3 * S), "corner screw\npost", 26, GREY)
    d.line([X0 - 2.6 * S, by, X0, by], fill=COL["o"], width=4)
    text(d, (X0 - 2.6 * S - 10, by - 44), "2.6", 26, COL["o"], True)
    ry = Y0 + (19.75 + 1.0) * S
    d.rectangle([60, ry, 1560, ry + 1.0 * S], fill=(205, 208, 216))
    text(d, (70, ry + 36), "rib A (1 mm tall): shows as a 1 mm sliver below the cell", 26, GREY)
    # lid opening (cell bridges it)
    lo = [mm(4.6, 1.6), mm(26 - 8.9, 19.75 - 5.2)]
    d.rectangle([lo[0][0], lo[0][1], lo[1][0], lo[1][1]], fill=(255, 236, 236), outline=COL["r"], width=4)
    text(d, (lo[0][0] + 14, lo[0][1] + 14), "old battery-lid\nopening:\nNO TAPE here", 28, COL["r"], True)
    # cell outline
    c0, c1 = mm(0, 0), mm(26.02, 19.75)
    for k in range(0, int(c1[0] - c0[0]), 26):
        d.line([c0[0] + k, c0[1], min(c0[0] + k + 13, c1[0]), c0[1]], fill=INK, width=4)
        d.line([c0[0] + k, c1[1], min(c0[0] + k + 13, c1[0]), c1[1]], fill=INK, width=4)
    for k in range(0, int(c1[1] - c0[1]), 26):
        d.line([c0[0], c0[1] + k, c0[0], min(c0[1] + k + 13, c1[1])], fill=INK, width=4)
        d.line([c1[0], c0[1] + k, c1[0], min(c0[1] + k + 13, c1[1])], fill=INK, width=4)
    # strips
    s1 = [mm(26.02 - 1 - 6, 19.75 - 1 - 18), mm(26.02 - 1, 19.75 - 1)]
    s2 = [mm(1, 19.75 - 1 - 4), mm(17, 19.75 - 1)]
    for a, b in (s1, s2):
        d.rectangle([a[0], a[1], b[0], b[1]], fill=(70, 120, 220), outline=COL["b"], width=4)
    text(d, (s1[0][0] + 14, s1[0][1] + 200), "strip 1\n6 × 18", 28, "white", True)
    text(d, (s2[0][0] + 150, s2[0][1] + 2), "strip 2   4 × 16", 28, "white", True)
    # protection board end + leads
    d.rectangle([c1[0] - 0.9 * S, c0[1] + 2 * S, c1[0], c1[1] - 2 * S], fill=(240, 210, 80))
    lx = c1[0] + 0.6 * S
    for dx, colr in ((0, (200, 30, 30)), (14, (20, 20, 20))):
        y_turn = Y0 + (19.75 + 3.3) * S + dx
        pts = [(c1[0], c0[1] + 3 * S + dx), (lx + dx, c0[1] + 3 * S + dx), (lx + dx, y_turn), (X0 + (26 + 11.0) * S - dx, y_turn),
               (X0 + (26 + 11.0) * S - dx, Y0 + (19.75 - 5.5 + 2.3) * S)]
        d.line(pts, fill=colr, width=8, joint="curve")
    px, py = X0 + (26 + 11.0) * S, Y0 + (19.75 - 5.5) * S
    d.rectangle([px - 3.2 * S, py - 2.3 * S, px + 3.2 * S, py + 2.3 * S], fill=(240, 238, 230), outline=INK, width=3)
    text(d, (px - 3.0 * S, py - 2.3 * S - 40), "J4 plug", 28, INK, True)
    text(d, (1165, 690), "plug mouth\nfaces down:\nleads go in\nfrom below", 24, GREY)
    # S-fold of spare lead
    sx0, sy0 = 1185, 330
    for k in range(3):
        y = sy0 + k * 1.6 * S
        d.line([sx0, y, sx0 + 5.5 * S, y], fill=(90, 90, 90), width=10)
        if k < 2:
            xe = sx0 + 5.5 * S if k % 2 == 0 else sx0
            d.arc([xe - 0.8 * S, y, xe + 0.8 * S, y + 1.6 * S], 270 if k % 2 == 0 else 90, 90 if k % 2 == 0 else 270, fill=(90, 90, 90), width=10)
    tag(d, (1150, 460), "spare lead: flat S of 3 runs,\nunder 4 mm tall, one\n10 × 15 mm Kapton strip", "k", 26)
    tag(d, (60, 1120), "Strips go on the CELL's underside first, then lower the cell in. Press 10 s on the strips only.", "b", 28)
    tag(d, (60, 1040), "Nothing on top of the cell: only about 0.2 mm to the coin-cell holder above it", "r", 28)
    save(im, os.path.join(HOW, "battery_tape_layout.png"))


# ───────────────────────── D. magnet meter check ─────────────────────────
def magnet_check():
    im, d = blank(1600, 1180)
    title(d, "Magnet piece: find the +5 V leg BEFORE it goes into J3",
          "Piece snapped onto the real cable, cable's USB-A end in a phone charger, legs in the air. Meter on DC volts (20 V).")
    # cable head + piece
    d.rounded_rectangle([420, 170, 1180, 250], 30, fill=(60, 60, 66))
    text(d, (440, 186), "cable's magnet head (snaps on only one way)", 28, "white", True)
    d.line([1180, 210, 1450, 210], fill=(60, 60, 66), width=18)
    d.rounded_rectangle([1450, 175, 1560, 250], 10, fill=(235, 235, 235), outline=INK, width=3)
    text(d, (1420, 262), "phone\ncharger", 26, GREY)
    d.rectangle([480, 252, 1120, 330], fill=(30, 30, 34))
    text(d, (500, 268), "N", 40, "white", True)
    legs = [560, 700, 900, 1040]
    for k, x in enumerate(legs):
        d.rectangle([x - 9, 330, x + 9, 520], fill=(160, 160, 165), outline=INK, width=2)
    d.rectangle([548, 470, 572, 500], fill=COL["r"])
    text(d, (300, 540), "outer leg at\nthe N end", 26, INK, True)
    text(d, (1070, 540), "other outer leg", 26, INK, True)
    # meter
    mx, my = 640, 700
    d.rounded_rectangle([mx, my, mx + 330, my + 300], 24, fill=(250, 200, 40), outline=INK, width=4)
    d.rectangle([mx + 30, my + 30, mx + 300, my + 120], fill=(200, 215, 190), outline=INK, width=3)
    d.text((mx + 165, my + 75), "+5.0 V", font=font(56), fill=INK, anchor="mm")
    text(d, (mx + 40, my + 150), "DC volts", 30, INK, True)
    d.line([mx + 80, my + 300, mx + 80, my + 330], fill=INK, width=6)
    d.line([(mx + 90, my + 2), (560, 520)], fill=COL["r"], width=8)
    d.line([(mx + 250, my + 2), (1040, 520)], fill=(20, 20, 20), width=8)
    text(d, (mx - 230, my + 40), "RED probe", 30, COL["r"], True)
    text(d, (mx + 360, my + 60), "BLACK probe", 30, INK, True)
    # result boxes
    tag(d, (40, 1010), "+5 V shown: the RED-probe leg is VBUS. Mark it with a pen. It goes into J3 pin 1 (silk N / +).", "g", 28)
    tag(d, (40, 1090), "Inner legs to the black leg: 0–0.6 V.  5 V on an inner leg, or anything odd: STOP, don't fit it, ask.", "r", 28)
    tag(d, (1110, 830), "−5 V?  Swap the probes\nand read again. Don't\nturn the piece over.", "o", 28)
    save(im, os.path.join(HOW, "magnet_meter_check.png"))


# ───────────────────────── E. J4 polarity ─────────────────────────
def j4_polarity():
    im, d = blank(1600, 1130)
    title(d, "Battery plug and J4: check the polarity with the meter first",
          "J4 seen from the component side with its opening towards you: pin 1 GND (−) on the LEFT, pin 2 + on the RIGHT.")
    # panel 1: battery plug
    panel_frame(d, (30, 150, 785, 940), 1, "Battery plug (cell NOT plugged in)")
    px, py = 230, 420
    d.rounded_rectangle([px, py, px + 340, py + 170], 14, fill=(240, 238, 230), outline=INK, width=4)
    for k, (x, lab) in enumerate(((px + 70, "−"), (px + 230, "+"))):
        d.rectangle([x, py + 40, x + 40, py + 120], fill=(180, 160, 90), outline=INK, width=2)
        d.text((x + 20, py + 150), lab, font=font(36), fill=INK, anchor="mm")
    d.line([px + 90, py + 170, px + 90, py + 330], fill=(20, 20, 20), width=12)
    d.line([px + 250, py + 170, px + 250, py + 330], fill=(200, 30, 30), width=12)
    text(d, (px + 20, py + 340), "black wire", 26, INK, True); text(d, (px + 220, py + 340), "red wire", 26, COL["r"], True)
    d.line([(px + 250, py + 80), (650, 300)], fill=COL["r"], width=7)
    d.line([(px + 90, py + 80), (560, 330)], fill=(20, 20, 20), width=7)
    d.rounded_rectangle([520, 220, 760, 330], 16, fill=(250, 200, 40), outline=INK, width=3)
    d.text((640, 275), "+3.9 V", font=font(44), fill=INK, anchor="mm")
    text(d, (60, 820), "Hold the plug the way it will go into J4. Red probe on\nthe contact that meets J4 '+', black on the other:\npositive 3.0–4.2 V and it's the red wire = good.", 26)
    # panel 2: board-side beep
    panel_frame(d, (815, 150, 1570, 940), 2, "Board side, nothing plugged in: beep test")
    jx, jy = 1050, 330
    d.rounded_rectangle([jx, jy, jx + 300, jy + 150], 12, fill=(240, 238, 230), outline=INK, width=4)
    d.rectangle([jx + 60, jy + 40, jx + 100, jy + 150], fill=(30, 30, 30)); d.rectangle([jx + 200, jy + 40, jx + 240, jy + 150], fill=(30, 30, 30))
    text(d, (jx + 40, jy - 50), "pin 1 −", 28, INK, True); text(d, (jx + 190, jy - 50), "pin 2 +", 28, COL["r"], True)
    text(d, (jx + 70, jy + 160), "opening towards you", 24, GREY)
    d.ellipse([880, 640, 940, 700], fill=(235, 200, 60), outline=INK, width=3)
    text(d, (860, 710), "TP4 (GND)", 26, INK, True)
    d.line([(910, 640), (jx + 80, jy + 120)], fill=(20, 20, 20), width=6)
    tag(d, (1010, 560), "pin 1 ↔ TP4: BEEP", "g", 30)
    tag(d, (1010, 640), "pin 2 ↔ TP4: silent", "k", 30)
    text(d, (845, 800), "Meter on beep, battery unplugged, no cable. This proves\nwhich J4 pin is ground on YOUR board.", 26)
    tag(d, (40, 965), "Reading negative, or the + contact is on the black wire: lift the two crimp tabs with a needle, swap the pins,\nclick them back, meter again. Never 'try it and see': backwards, nothing works or charges.", "r", 28)
    save(im, os.path.join(HOW, "j4_polarity_check.png"))


# ───────────────────────── F. H6 filing ─────────────────────────
def h6_file():
    im, d = blank(1600, 900)
    title(d, "Board sticks on post H6? File the HOLE, never the post", "Seen from above, looking down on hole H6 with its post inside. Not to scale.")
    cx, cy, S = 520, 480, 60
    d.rectangle([150, 180, 900, 820], fill=BOARD)
    d.ellipse([cx - 2.1 * S, cy - 2.1 * S, cx + 2.1 * S, cy + 2.1 * S], fill="white", outline=INK, width=3)
    d.ellipse([cx - 1.45 * S, cy - 1.45 * S + 0.55 * S, cx + 1.45 * S, cy + 1.45 * S + 0.55 * S], fill=(205, 208, 216), outline=INK, width=4)
    text(d, (cx - 60, cy + 40), "post", 28, INK, True)
    d.pieslice([cx - 2.1 * S - 20, cy - 2.1 * S - 20, cx + 2.1 * S + 20, cy + 2.1 * S + 20], 200, 340, fill=(240, 190, 120))
    d.ellipse([cx - 2.1 * S, cy - 2.1 * S, cx + 2.1 * S, cy + 2.1 * S], outline=INK, width=3)
    arrow(d, (cx, 120), (cx, cy - 2.1 * S - 20), COL["o"], w=7)
    tag(d, (960, 200), "Board's TOP end this way (↑)", "k", 30)
    tag(d, (960, 290), "File here: a few strokes with a round\nneedle file, towards the top only", "o", 30)
    tag(d, (960, 420), "Only 0.10 mm of play at H6\n(post Ø2.9 in a Ø4.2 hole,\n0.55 mm off-centre)", "b", 30)
    tag(d, (960, 580), "Never file, grind or bend the post:\nit locates the keys under the board", "r", 30)
    text(d, (160, 840), "Try again after every 2–3 strokes. Blow the dust off: no dust on the key pads.", 28, GREY)
    save(im, os.path.join(HOW, "h6_file_hole.png"))


# ───────────────────────── G. cover won't close: where to look ─────────────────────────
def cover_check():
    cv = AG.Canvas(Image.open(os.path.join(R, "back_cover_off.png")))
    cv.panel("Back cover won't sit flat? Check these", ["Open it. Check 1 to 6 in order", "Marker on rib tops, close gently,", "open: the mark shows what hits", "Never force it with the screws"], corner="tl", c="r")
    cv.label("1 J4 + plug: 0.41 mm to the floor\n(stub left in grind zone 1?)", (630, 850), (330, 210), "r")
    cv.label("2 Battery: tape under it only,\nleads flat, nothing on top", (470, 800), (-180, 250), "o")
    cv.label("3 Camera: thin tape, not foam;\nrib B ground flat", (655, 745), (560, 60), "b")
    cv.label("4 Magnet body: lip 5b\nunder the notch?", (650, 940), (520, 150), "p")
    cv.label("5 Camera ribbon: flat,\nno loop over the mid rib", (790, 590), (430, -180), "k")
    cv.label("6 E-paper ribbon at J2:\nnot pinched", (520, 655), (-150, -300), "g")
    save(cv.finish(), os.path.join(HOW, "cover_not_flat_checks.png"))


# ───────────────────────── H. photos ─────────────────────────
def photo_backcover():
    src = os.path.join(UP, "60646b63-acfc-4429-8268-71edaab5a3bd", "050df9b7-image.jpg")
    im = ImageOps.exif_transpose(Image.open(src)).convert("RGB")
    im = im.crop((0, int(im.height * 0.27), im.width, im.height))         # drop the empty table
    s = 1500 / im.width
    im = im.resize((1500, int(im.height * s)), Image.LANCZOS)
    k = 1932 / 1500 * s                                                    # displayed-1500 px -> this image
    Y = lambda y: (y * 1932 / 1500 - 2576 * 0.27) * s
    X = lambda x: x * 1932 / 1500 * s
    cv = AG.Canvas(im)
    box = [(X(495), Y(655)), (X(760), Y(655)), (X(770), Y(850)), (X(490), Y(850))]
    cv.poly(box, (240, 140, 30), alpha=0.30, outline=(230, 110, 0), w=6, dash=True)
    cv.label("Battery goes here (rough):\nflat on the floor, bridging\nthe old lid opening", (X(630), Y(760)), (60, 330), "o", fs=34)
    cv.label("Corner screw post:\n2.5 mm gap to the cell", (X(447), Y(686)), (-40, -260), "k", fs=34)
    cv.label("Solar box: grind flat (zone 1);\nJ4 and its plug end up above here", (X(940), Y(780)), (240, -300), "r", fs=34)
    cv.label("Rib A: 1 mm below the cell", (X(380), Y(820)), (-60, 330), "b", fs=34)
    cv.panel("Your back cover, inside up", ["Top end away from you", "Left/right as on the calculator's front", "Positions are rough: use the", "numbers in step 9"], corner="bl", c="b", fs=30)
    save(cv.finish(), os.path.join(PH, "backcover_battery_spot_ann.jpg"), jpg=True)


def photo_keymat():
    src = os.path.join(UP, "60646b63-acfc-4429-8268-71edaab5a3bd", "6996fcbd-image.jpg")
    im = ImageOps.exif_transpose(Image.open(src)).convert("RGB")
    im = im.crop((int(im.width * 0.12), int(im.height * 0.16), int(im.width * 0.88), int(im.height * 0.80)))
    s = 1200 / im.width
    im = im.resize((1200, int(im.height * s)), Image.LANCZOS)
    cv = AG.Canvas(im)
    cv.label("Black dots = key contacts:\ndon't touch them", (583, 293), (330, 330), "r", fs=34)
    cv.label("Small holes: the locating\nposts poke through", (343, 218), (-40, 280), "b", fs=34)
    cv.label("Half-round notches", (350, 82), (480, -30), "o", fs=34)
    cv.panel("Key mat: the side that faces the BOARD", ["Face-down shell: this side faces UP at you", "The domes bulge the other way, to the keys"], corner="bl", c="k", fs=30)
    save(cv.finish(), os.path.join(PH, "keymat_contacts_ann.jpg"), jpg=True)


# ───────────────────────── I. gallery / glossary thumbnails ─────────────────────────
def thumb(src, box, name, size=(600, 450), bg="white"):
    im = ImageOps.exif_transpose(Image.open(src)).convert("RGB")
    if box:
        im = im.crop(box)
    im.thumbnail(size, Image.LANCZOS)
    out = Image.new("RGB", size, bg)
    out.paste(im, ((size[0] - im.width) // 2, (size[1] - im.height) // 2))
    save(out, os.path.join(GAL, name), jpg=True)


def gallery():
    D = os.path.join(R, "detail"); S = os.path.join(R, "steps")
    thumb(os.path.join(S, "s04_parts.png"), (320, 90, 800, 1050), "p_board.jpg")
    thumb(os.path.join(S, "s04_parts.png"), (840, 240, 1250, 490), "p_epaper.jpg")
    thumb(os.path.join(D, "d06_camera_ribbon_path.png"), (560, 40, 1120, 460), "p_camera.jpg")
    thumb(os.path.join(D, "d11_battery_placed.png"), (340, 290, 1240, 820), "p_battery.jpg")
    thumb(os.path.join(D, "d09_magnet_j3.png"), (380, 0, 1220, 440), "p_magnet.jpg")
    thumb(os.path.join(UP, "60646b63-acfc-4429-8268-71edaab5a3bd", "88e03c27-image.jpg"), None, "p_casio.jpg")
    thumb(os.path.join(UP, "60646b63-acfc-4429-8268-71edaab5a3bd", "6996fcbd-image.jpg"), (230, 450, 1690, 2100), "p_keymat.jpg")
    # glossary
    thumb(os.path.join(D, "d06_camera_ribbon_path.png"), (640, 180, 1000, 1000), "g_fpc.jpg")
    thumb(os.path.join(D, "d07_j1_latch_open.png"), (430, 400, 1180, 800), "g_latch.jpg")
    thumb(os.path.join(D, "d05_camera_tape_ann.png"), (540, 350, 1060, 860), "g_kapton.jpg")
    thumb(os.path.join(D, "d12_j4_plug.png"), (500, 380, 980, 1000), "g_jst.jpg")
    thumb(os.path.join(os.path.dirname(os.path.dirname(HERE)), "renders_bringup", "v14_tp_power_corner_ann.png"), None, "g_testpad.jpg")
    # JPEG copies of two bring-up maps, so the guide only links files under renders/
    for n in ("v14_top_ann", "v14_j3_j4_polarity_ann"):
        src = os.path.join(os.path.dirname(os.path.dirname(HERE)), "renders_bringup", n + ".png")
        save(Image.open(src), os.path.join(HOW, n + ".jpg"), jpg=True)
    # d21 is over 400 KB as PNG
    save(Image.open(os.path.join(D, "d21_window_mask_on_ann.png")), os.path.join(D, "d21_window_mask_on_ann.jpg"), jpg=True)


if __name__ == "__main__":
    fpc_latch(); stiffener(); battery_tape(); magnet_check(); j4_polarity(); h6_file(); cover_check()
    photo_backcover(); photo_keymat(); gallery()
