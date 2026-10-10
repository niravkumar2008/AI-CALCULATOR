"""Derive the AI Calculator board geometry from the teardown scan.

Input : hardware/scan/9C102_A_extracted.png  (pdfimages -png 9C102_A.pdf, 1632 x 2126 px)
Output: board_outline.dxf, features.json, keys.csv, overlay_front.png, overlay_back.png
Run   : python derive_geometry.py   (needs numpy, opencv-python, scipy, shapely, ezdxf)

All pixel numbers below were measured on the extracted image (see
hardware/geometry_assumptions.md for how and how sure). Everything is in
"scan view": the front shell seen from inside = the board seen from its
component side (KiCad top view). Left/right are mirrored vs. the calculator front.
"""
import csv
import json
import math
from pathlib import Path

import cv2
import ezdxf
import numpy as np
from scipy import ndimage as ndi
from shapely.geometry import Point, Polygon, box, mapping
from shapely.ops import unary_union

HERE = Path(__file__).resolve().parent
IMG = HERE.parent / "scan" / "9C102_A_extracted.png"

# ---------------------------------------------------------------- scale
S_F = 7.55          # px per mm for front-shell features (see assumptions doc, section 2)
CX = 621.0          # front-shell centreline, px (post columns + shell edges agree to +-1 px)
TOP = 469.0         # front-shell outer top edge at the centreline, px
KX, KY = 150.0, 60.0  # where (CX, TOP) lands in KiCad, mm
BACK_RATIO = 1.013  # front px per back-cover px for the same physical distance
BACK_CX, BACK_REF_Y, FRONT_REF_Y = 1226.0, 980.0, 974.0  # mid-screw row in each shell


def mm(x, y):
    """scan px -> KiCad mm (y down)."""
    return (round(KX + (x - CX) / S_F, 3), round(KY + (y - TOP) / S_F, 3))


def to_back(x, y):
    """front-shell px -> back-cover px (mirror about the centreline)."""
    return BACK_CX - (x - CX) / BACK_RATIO, BACK_REF_Y + (y - FRONT_REF_Y) / BACK_RATIO


# ---------------------------------------------------------------- silhouette
def front_silhouette(img):
    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV).astype(int)
    paper = (hsv[..., 1] < 45) & (hsv[..., 2] > 150)
    m = np.zeros(paper.shape, bool)
    m[350:1800, 335:925] = ~paper[350:1800, 335:925]
    m = ndi.binary_opening(m, np.ones((5, 5)))
    lab, n = ndi.label(m)
    k = np.argmax(ndi.sum(m, lab, range(1, n + 1))) + 1
    f = ndi.binary_fill_holes(lab == k)
    f = ndi.binary_opening(f, np.ones((9, 9)))
    rows = []
    for y in range(470, 1661):
        xs = np.where(f[y])[0]
        if len(xs) == 0:
            continue
        left, right = xs.min(), xs.max()
        # the ruler touches the left wall: its tick marks read as "shell" at x<=335.
        # Use the clean right edge mirrored, and keep the narrower side (safer).
        hl = CX - left if left > 336 else 1e9
        hr = right - CX
        rows.append((y, min(hl, hr)))
    ys = np.array([r[0] for r in rows], float)
    hw = ndi.median_filter(np.array([r[1] for r in rows], float), 9)
    pts = [(CX - h, y) for y, h in zip(ys, hw)] + [(CX + h, y) for y, h in zip(ys[::-1], hw[::-1])]
    return Polygon(pts).buffer(0)


# ---------------------------------------------------------------- features (px)
CASIO_BOARD = (377.0, 865.0, 1609.0)      # left, right, bottom of Casio's own board
CASIO_INSET = 0.25 * S_F                  # we stay 0.25 mm inside Casio's edge (JLC outline tol. +-0.2 mm)
WALL_PLUS_CLEAR = 14.0                    # 1.35 mm wall + 0.5 mm clearance, px
WAIST_JOIN = 1075.0                       # below this, Casio's board edge is the reference
CORNER_R = 5.0 * S_F                      # 5 mm convex corners

MID_BOSSES = [(457.6, 972.9), (781.2, 969.9)]                 # screw bosses through the board
POSTS = [(459.5, 1050.7), (783.0, 1048.8), (780.0, 1118.0), (460.0, 1190.0),
         (475.1, 1263.3), (766.9, 1260.8), (474.0, 1348.0), (768.0, 1346.0),
         (474.2, 1432.9), (766.9, 1430.6)]
BOTTOM_BOSSES = [(474.0, 1603.0), (768.0, 1601.0)]            # half-circle notches
TOP_BOSSES = [(370.0, 537.0), (862.0, 580.0)]                 # hidden; from back-cover screw holes
COIN_POCKET = (602.1, 607.0)              # cut x >= 147.5 mm, y <= 78.3 mm: old LR44 corner, now the battery bay
BATTERY = (147.9, 63.5, 31.0, 11.5)       # mm, x, y, w, h: Adafruit #1570 LiPo (31 x 11.5 x 3.8, 100 mAh) in the bay
MAGNET_X = 136.0                          # mm, magnet connector centred behind the old solar cell (layout v2)
SHIELD = (366.0, 874.0, 612.0, 858.0)     # Casio LCD module shield bbox
WINDOW_C = (620.0, 745.0)                 # photo ratios put it 735-760; inside the LCD shield
WINDOW = (57.0, 21.0)                     # from Nirav's front photo vs the key grid (was 64 x 25)
BACK_PINS = [(414, 658), (617, 662), (820, 662), (432, 865), (807, 865)]  # back-cover pins (mirrored)
CAMERA_C = (621.0, 734.0)                 # 37 mm below back-cover top, centred

D_MID, D_POST, D_NOTCH, D_TOP = 6.0, 4.4, 8.0, 6.0   # mm
MAGNET_ZONE = (21.5, 7.5)                 # mm, top-wall slot + connector body/legs zone
# The case has exactly 6 screws (Nirav, 2026-10-02): top pair = TOP_BOSSES (the scan-right one,
# beside the LR44 opening, also holds the battery door), mid pair = MID_BOSSES, bottom = BOTTOM_BOSSES.


def board_polygon(outer):
    top = outer.buffer(-WALL_PLUS_CLEAR, join_style=1).intersection(box(0, 0, 2000, WAIST_JOIN + 5))
    l, r, b = CASIO_BOARD
    l, r, b = l + CASIO_INSET, r - CASIO_INSET, b - CASIO_INSET
    rc = 36.0  # Casio's bottom-corner radius, px
    key = box(l, WAIST_JOIN - 15, r, b - rc).union(box(l + rc, b - 2 * rc, r - rc, b))
    key = key.union(Point(l + rc, b - rc).buffer(rc)).union(Point(r - rc, b - rc).buffer(rc))
    p = unary_union([top, key])
    p = p.buffer(10, join_style=1).buffer(-10, join_style=1)             # fill the waist step
    p = p.buffer(-CORNER_R, join_style=1).buffer(CORNER_R, join_style=1)  # 5 mm corners
    cuts = [Point(x, y).buffer(D_NOTCH / 2 * S_F) for x, y in BOTTOM_BOSSES]
    tx, ty = TOP_BOSSES[0]
    rr = D_TOP / 2 * S_F
    cuts.append(Point(tx, ty).buffer(rr).union(box(200, ty - rr, tx, ty + rr)))
    cx, cy = COIN_POCKET
    cuts.append(box(cx + 15, -100, 1100, cy - 15).buffer(15))
    return p.difference(unary_union(cuts)).simplify(0.6)


# ---------------------------------------------------------------- keys
F6 = [822, 742, 661, 581, 500, 420]     # function columns, front-left -> front-right
N5 = [815, 718, 621, 524, 427]          # number columns
KEY_ROWS = [  # (row name, y px, [(DKey, label, x px)], pad w, pad h, confidence)
    ("R1", 928, [("Shift", "SHIFT", F6[0]), ("Alpha", "ALPHA", F6[1]), ("Mode", "MODE", F6[4]),
                 ("On", "ON", F6[5])], 6.0, 4.5, "med"),
    # R2 per Nirav's fx-300ES PLUS photo: Abs, x^3 | x^-1, log_a(b). (firmware DKey still says
    # Inv, NCr, Pol, Cube: nCr/Pol are SHIFT functions of the divide/plus keys, so it needs Abs + LogBase.)
    ("R2", 1016, [("Abs", "Abs", F6[0]), ("Cube", "x^3", F6[1]), ("Inv", "x^-1", F6[4]),
                  ("LogBase", "log_a", F6[5])], 6.0, 4.5, "med"),
    ("R3", 1083, list(zip(["Frac", "Sqrt", "Sq", "Pow", "Log", "Ln"],
                          ["a/b", "sqrt", "x^2", "x^n", "log", "ln"], F6)), 7.0, 5.5, "med"),
    ("R4", 1154, list(zip(["Neg", "Dms", "Hyp", "Sin", "Cos", "Tan"],
                          ["(-)", "o'\"", "hyp", "sin", "cos", "tan"], F6)), 7.0, 5.5, "med"),
    ("R5", 1226, list(zip(["Rcl", "Eng", "Open", "Close", "SD", "MPlus"],
                          ["RCL", "ENG", "(", ")", "S<>D", "M+"], F6)), 7.0, 5.5, "med"),
    ("N1", 1305, list(zip(["D7", "D8", "D9", "Del", "AC"], ["7", "8", "9", "DEL", "AC"], N5)), 9.0, 7.0, "high"),
    ("N2", 1389, list(zip(["D4", "D5", "D6", "Mul", "Div"], ["4", "5", "6", "x", "/"], N5)), 9.0, 7.0, "high"),
    ("N3", 1473, list(zip(["D1", "D2", "D3", "Add", "Sub"], ["1", "2", "3", "+", "-"], N5)), 9.0, 7.0, "high"),
    ("N4", 1557, list(zip(["D0", "Dot", "Exp10", "Ans", "Eq"], ["0", ".", "x10^x", "Ans", "="], N5)), 9.0, 7.0, "med"),
]
REPLAY_C = (621.0, 964.0)  # photo: pad centre 4.7 mm below the SHIFT row
REPLAY = [("Up", "UP", 0, -40), ("Down", "DOWN", 0, 40), ("Left", "LEFT", 50, 0), ("Right", "RIGHT", -50, 0)]


def keys():
    out = []
    for row, y, ks, w, h, conf in KEY_ROWS:
        for i, (dk, lab, x) in enumerate(ks):
            out.append(dict(key=dk, label=lab, row=row, x_px=x, y_px=y, w=w, h=h, conf=conf))
    for dk, lab, dx, dy in REPLAY:
        out.append(dict(key=dk, label=lab, row="REPLAY", x_px=REPLAY_C[0] + dx, y_px=REPLAY_C[1] + dy,
                        w=5.0, h=4.0, conf="low"))
    for k in out:
        k["x_mm"], k["y_mm"] = mm(k["x_px"], k["y_px"])
    return out


# ---------------------------------------------------------------- outputs
def poly_mm(p):
    return [mm(x, y) for x, y in p.exterior.coords]


def main():
    img = cv2.imread(str(IMG))
    outer = front_silhouette(img)
    board = board_polygon(outer)
    ks = keys()

    wx, wy = WINDOW_C
    window = WINDOW
    feats = {
        "scale_px_per_mm": S_F,
        "kicad_origin_note": f"scan px ({CX},{TOP}) = KiCad ({KX},{KY}) mm = front shell top-centre",
        "board_outline_mm": poly_mm(board),
        "board_bbox_mm": [round(v, 2) for v in (np.array(board.bounds) - [CX, TOP, CX, TOP]) / S_F],
        "outer_shell_mm": poly_mm(outer.simplify(1.0)),
        "holes": [dict(kind="screw boss", d=D_MID, at=mm(*p), conf="high") for p in MID_BOSSES]
        + [dict(kind="post", d=D_POST, at=mm(*p), conf="high") for p in POSTS],
        "notches": [dict(kind="bottom screw boss", d=D_NOTCH, at=mm(*p), conf="high") for p in BOTTOM_BOSSES]
        + [dict(kind="top screw boss U-slot", d=D_TOP, at=mm(*TOP_BOSSES[0]), conf="med")],
        "coin_pocket_cut": dict(x_from=mm(*COIN_POCKET)[0], y_to=mm(*COIN_POCKET)[1],
                                top_right_boss=mm(*TOP_BOSSES[1]), conf="med",
                                note="battery bay: old LR44 corner widened to x>=148 mm"),
        "window": dict(center=mm(wx, wy), size=window, conf="low"),
        "lcd_shield_bbox": [mm(SHIELD[0], SHIELD[2]), mm(SHIELD[1], SHIELD[3])],
        "epaper": dict(center=mm(wx, wy), outline=(59.2, 29.2), active=(48.55, 23.7), conf="med"),
        "camera": dict(center=mm(*CAMERA_C), hole_d=6.0, module_keepout=(12.0, 12.0), conf="low"),
        # Adafruit 5358 = MG04254FRA1S1N: face 21 x 7 mm, 4 pins at 2.50 mm, right angle, 2 A / 36 V.
        # Slot 21.5 x 7.5 in the top wall; zone runs 7.5 mm in from the outer edge (wall + body + legs).
        "magnet_slot": dict(center=(MAGNET_X, 63.75), size=MAGNET_ZONE, face=(21.0, 7.0), slot=(21.5, 7.5),
                            header_pins_mm=[(MAGNET_X + (i - 1.5) * 2.5, 67.0) for i in range(4)],
                            pitch=2.5, conf="low"),
        "battery": dict(rect=BATTERY, part="Adafruit #1570: 3.7 V 100 mAh LiPo, 31 x 11.5 x 3.8 mm, protection PCB, JST-PH 2-pin (2.0 mm)",
                        conf="low"),
        "back_cover_pins": [dict(at=mm(*p), d=5.0) for p in BACK_PINS],
        "solar_box_zone": [mm(394, 483), mm(637, 569)],
        "keys": ks,
    }
    (HERE / "features.json").write_text(json.dumps(feats, indent=1))
    with open(HERE / "keys.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(ks[0].keys()))
        w.writeheader()
        w.writerows(ks)

    # ---- DXF (mm, KiCad coordinates with Y flipped so the DXF reads upright)
    doc = ezdxf.new("R2010", setup=True)
    doc.header["$INSUNITS"] = 4
    msp = doc.modelspace()
    for name, col in [("EDGE_CUTS", 7), ("HOLES", 2), ("OUTER_SHELL", 8), ("WINDOW", 6), ("EPAPER", 30),
                      ("CAMERA", 1), ("MAGNET", 3), ("BATTERY", 140), ("KEYPADS", 4), ("BACK_PINS", 5), ("VERIFY", 1)]:
        doc.layers.add(name, color=col)
    fl = lambda pts: [(x, -y) for x, y in pts]
    msp.add_lwpolyline(fl(poly_mm(board)), close=True, dxfattribs={"layer": "EDGE_CUTS"})
    msp.add_lwpolyline(fl(poly_mm(outer.simplify(1.0))), close=True, dxfattribs={"layer": "OUTER_SHELL"})
    for h in feats["holes"]:
        msp.add_circle((h["at"][0], -h["at"][1]), h["d"] / 2, dxfattribs={"layer": "HOLES"})
    def rect(c, s, layer):
        x, y = c
        a, b = s[0] / 2, s[1] / 2
        msp.add_lwpolyline([(x - a, -(y - b)), (x + a, -(y - b)), (x + a, -(y + b)), (x - a, -(y + b))],
                           close=True, dxfattribs={"layer": layer})
    rect(feats["window"]["center"], window, "WINDOW")
    rect(feats["epaper"]["center"], (59.2, 29.2), "EPAPER")
    rect(feats["epaper"]["center"], (48.55, 23.7), "EPAPER")
    c = feats["camera"]["center"]
    msp.add_circle((c[0], -c[1]), 3.0, dxfattribs={"layer": "CAMERA"})
    rect(c, (12.0, 12.0), "CAMERA")
    rect(feats["magnet_slot"]["center"], MAGNET_ZONE, "MAGNET")
    bx, by, bw, bh = BATTERY
    rect((bx + bw / 2, by + bh / 2), (bw, bh), "BATTERY")
    for p in feats["back_cover_pins"]:
        msp.add_circle((p["at"][0], -p["at"][1]), 2.5, dxfattribs={"layer": "BACK_PINS"})
    for k in ks:
        rect((k["x_mm"], k["y_mm"]), (k["w"], k["h"]), "KEYPADS")
        msp.add_text(k["label"], height=1.2, dxfattribs={"layer": "KEYPADS"}).set_placement(
            (k["x_mm"], -k["y_mm"]), align=ezdxf.enums.TextEntityAlignment.MIDDLE_CENTER)
    doc.saveas(HERE / "board_outline.dxf")

    overlays(img, outer, board, ks)
    print("board bbox mm (rel. to shell top-centre):", feats["board_bbox_mm"])
    print("board area mm2:", round(board.area / S_F ** 2, 1), " keys:", len(ks))


def overlays(img, outer, board, ks):
    ov = img.copy()
    big = lambda p: np.int32(np.array(p.exterior.coords) * 4)
    # draw at 4x sub-pixel precision
    sh = dict(lineType=cv2.LINE_AA, shift=2)
    cv2.polylines(ov, [big(outer)], True, (160, 160, 160), 1, **sh)
    cv2.polylines(ov, [big(board)], True, (255, 255, 0), 2, **sh)
    for x, y in MID_BOSSES:
        cv2.circle(ov, (int(x * 4), int(y * 4)), int(D_MID / 2 * S_F * 4), (0, 255, 255), 2, **sh)
    for x, y in POSTS:
        cv2.circle(ov, (int(x * 4), int(y * 4)), int(D_POST / 2 * S_F * 4), (0, 255, 255), 2, **sh)
    for x, y in TOP_BOSSES + BOTTOM_BOSSES:
        cv2.drawMarker(ov, (int(x), int(y)), (0, 140, 255), cv2.MARKER_CROSS, 18, 2)
    wx, wy = WINDOW_C
    for (w, h), col in [(WINDOW, (255, 0, 255)), ((59.2, 29.2), (0, 165, 255)), ((48.55, 23.7), (0, 120, 255))]:
        cv2.rectangle(ov, (int(wx - w / 2 * S_F), int(wy - h / 2 * S_F)),
                      (int(wx + w / 2 * S_F), int(wy + h / 2 * S_F)), col, 1, cv2.LINE_AA)
    cx, cy = CAMERA_C
    cv2.circle(ov, (int(cx), int(cy)), int(3 * S_F), (0, 0, 255), 2, cv2.LINE_AA)
    cv2.rectangle(ov, (int(cx - 6 * S_F), int(cy - 6 * S_F)), (int(cx + 6 * S_F), int(cy + 6 * S_F)), (0, 0, 255), 1)
    mxp = CX + (MAGNET_X - KX) * S_F
    cv2.rectangle(ov, (int(mxp - MAGNET_ZONE[0] / 2 * S_F), int(TOP)),
                  (int(mxp + MAGNET_ZONE[0] / 2 * S_F), int(TOP + MAGNET_ZONE[1] * S_F)), (0, 200, 0), 2)
    bx, by, bw, bh = BATTERY
    p0 = (int(CX + (bx - KX) * S_F), int(TOP + (by - KY) * S_F))
    p1 = (int(CX + (bx + bw - KX) * S_F), int(TOP + (by + bh - KY) * S_F))
    cv2.rectangle(ov, p0, p1, (255, 120, 60), 2)
    for x, y in BACK_PINS:
        cv2.drawMarker(ov, (x, y), (200, 0, 160), cv2.MARKER_TILTED_CROSS, 16, 2)
    for k in ks:
        a, b = k["w"] / 2 * S_F, k["h"] / 2 * S_F
        col = {"high": (255, 255, 255), "med": (200, 255, 200), "low": (120, 120, 255)}[k["conf"]]
        cv2.rectangle(ov, (int(k["x_px"] - a), int(k["y_px"] - b)), (int(k["x_px"] + a), int(k["y_px"] + b)),
                      col, 1, cv2.LINE_AA)
        cv2.putText(ov, k["label"], (int(k["x_px"] - a + 2), int(k["y_px"] + 4)), cv2.FONT_HERSHEY_SIMPLEX,
                    0.33, col, 1, cv2.LINE_AA)
    cv2.putText(ov, "SCAN VIEW = KiCad top (component side). Keys are mirrored vs the front.",
                (20, 2080), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 0), 2, cv2.LINE_AA)
    legend = [("board edge (Edge.Cuts)", (255, 255, 0)), ("holes for posts / screw bosses", (0, 255, 255)),
              ("notch / hidden screw boss", (0, 140, 255)), ("window 57x21 (photo)", (255, 0, 255)),
              ("e-paper outline / active", (0, 165, 255)), ("camera 6 mm hole + 12 mm keepout", (0, 0, 255)),
              ("magnet slot 21.5x7.5 (5358)", (0, 200, 0)), ("battery 401230 in bay", (255, 120, 60)), ("back-cover pins (press here)", (200, 0, 160)),
              ("key pad: high / med / low", (255, 255, 255))]
    for i, (t, col) in enumerate(legend):
        cv2.rectangle(ov, (1050, 1760 + i * 30), (1072, 1782 + i * 30), col, -1)
        cv2.rectangle(ov, (1050, 1760 + i * 30), (1072, 1782 + i * 30), (0, 0, 0), 1)
        cv2.putText(ov, t, (1082, 1778 + i * 30), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 0, 0), 1, cv2.LINE_AA)
    # back cover: where the board features land, incl. the camera drill point
    for x, y in MID_BOSSES + BOTTOM_BOSSES + TOP_BOSSES:
        bx, by = to_back(x, y)
        cv2.circle(ov, (int(bx), int(by)), 12, (0, 140, 255), 2, cv2.LINE_AA)
    bx, by = to_back(*CAMERA_C)
    cv2.circle(ov, (int(bx), int(by)), int(3 * S_F / BACK_RATIO), (0, 0, 255), 2, cv2.LINE_AA)
    cv2.drawMarker(ov, (int(bx), int(by)), (0, 0, 255), cv2.MARKER_CROSS, 30, 1)
    cv2.putText(ov, "DRILL 6 mm (camera)", (int(bx) + 26, int(by) + 5), cv2.FONT_HERSHEY_SIMPLEX, 0.55,
                (0, 0, 255), 2, cv2.LINE_AA)
    bx, by = to_back(*CX_COIN)
    cv2.drawMarker(ov, (int(bx), int(by)), (0, 200, 0), cv2.MARKER_DIAMOND, 20, 2)
    cv2.imwrite(str(HERE / "overlay_full.png"), ov)
    cv2.imwrite(str(HERE / "overlay_front.png"), ov[440:1700, 300:940])
    cv2.imwrite(str(HERE / "overlay_back.png"), ov[440:1700, 900:1560])


CX_COIN = (771.5, 544.5)  # LR44 centre: the back cover's D-hole maps onto it (mirror check)

if __name__ == "__main__":
    main()
