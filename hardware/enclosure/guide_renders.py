"""Picture library for the build guides (render + camera + anchor code only: no model geometry is changed or saved).

    python guide_renders.py ver=v15 set=grind [only=z1,z3]     grinding close-ups  -> final_assembly/renders/grind/
    python guide_renders.py ver=v14 set=detail [only=d05]      v14 assembly close-ups -> final_assembly/renders/detail/
    python guide_renders.py ver=v15 set=detail15               v15 assembly close-ups -> final_assembly_v15_lcd/renders/detail/
    (d22_battery_tw302030 in both detail sets needs the TW302030 candidate in the document first:
     final_assembly/variant_battery_tw302030/fit_tw302030.py ver=.. case=nominal shift=1, then case=off after the shot)

Fusion must be open with the FusionMCPBridge add-in and the matching final-assembly document loaded (v14: root occurrences
"Final - ...", v15: "V15 - ..."; File > Open the exported .f3d is enough). The build script of that version is loaded as a
module (its stages do not run) and its helpers are reused: place(), section(), show/hide, the same camera maths as stage
"steps". Grind features are only suppressed / unsuppressed (restored to all-ground at the end); zone 6c, which the model does
not have, is shown with a temporary cut that is deleted again right after its picture. Nothing is saved or exported.

Each picture is <outdir>/<name>.png (white background, 1600 x 1200 or 1600 x 1000); model points named in a shot's
`anchors` / `polys` are written to <outdir>/anchors.json in image pixels, for annotate_guides.py (Pillow, offline).
"""
import json, math, os, sys, time, importlib.util

REPO = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR"
ENC = os.path.join(REPO, "hardware", "enclosure")
SRC = {"v14": os.path.join(ENC, "build_final_assembly.py"),
       "v15": os.path.join(ENC, "final_assembly_v15_lcd", "build_final_assembly_v15.py")}
LOG = os.path.join(ENC, "guide_renders.log")
CY = 138.94
F = lambda x, y: (150.0 - x, CY - y)          # KiCad -> model front frame

def log(msg):
    with open(LOG, "a", encoding="utf8") as f:
        print(time.strftime("%H:%M:%S"), msg, file=f)

# ═════════════════════════════════════════════════════════════════════════════
# Shot lists. Component names without the "Final - " / "V15 - " prefix.
#   show, hide_b (body-name substrings), move {comp: (dx,dy,dz[,flip])}, target, d (view direction, eye = target + d*300)
#   or eye, up, ext (view extent, mm), w/h, ortho, section (plane, offset, flip), grind_off (GRIND name prefixes suppressed;
#   default: none = all ground), temp_cut (zone 6c), anchors {k: (x,y,z)}, polys {k: [(x,y,z), ...]}
# ═════════════════════════════════════════════════════════════════════════════
PCB = "PCB (ai_calc_board.step)"
BACK = ["Back cover", "Battery lid"]
FRONT = ["Front shell"]
UPY = (0, 1, 0)

def box_pts(x0, x1, y0, y1, z):
    return [(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)]

def box8(x0, x1, y0, y1, z0, z1):
    return box_pts(x0, x1, y0, y1, z0) + box_pts(x0, x1, y0, y1, z1)

# ── A: grinding (shared shell; rendered from the v15 document, which carries the rev G shell with every grind) ──
TOP = 82.1                                   # front shell / back cover top outer edge (front Y), geometry "top"
RIM = 1.5                                    # parting line (SEAM_Z)
Z67 = RIM + 7.0                              # "7.0 mm below the rim"
XM, MAGY = 22.5, 81.667                      # magnet notch centre X, top-wall outer face Y
NOTCH = (XM - 10.75, XM + 10.75, RIM, RIM + 7.95)
CAMY = CY - 95.1                             # 43.84
RIBX = (34.95, 35.95)                        # inner rib (solar / antenna side, front +X), Y 24-66, from Z 6.0
ZONE6C = (34.6, 36.3, 23.9, 26.6, RIM - 0.01, Z67)   # last ~2.7 mm of the rib's lower end, down to 7.0 below the rim

GRIND_SHOTS = []
def gpair(name, zone_key, **kw):
    """before (this zone suppressed, the others ground) + after (all ground), same camera."""
    GRIND_SHOTS.append(dict(kw, name=name + "_before", grind_off=[zone_key]))
    GRIND_SHOTS.append(dict(kw, name=name + "_after", grind_off=[]))

gpair("z1_solar_box", "solar_box", show=BACK, target=(12.5, 68.9, 3.0), d=(0.35, -0.75, 1.0), up=UPY, ext=52,
      anchors=dict(c=(12.5, 68.9, 4), x0=(-4.0, 62.9, 1.0), x1=(29.0, 62.9, 1.0), y1=(29.0, 74.9, 1.0),
                   ztop=(29.0, 62.9, 6.5), zbot=(29.0, 62.9, 1.0), post=(33.5, 76.5, 4), floor=(8, 58.5, 1.0)))
gpair("z2_rib_b", "rib_b", show=BACK, target=(0, 43.9, 1.5), d=(0.3, -0.75, 1.0), up=UPY, ext=34,
      anchors=dict(c=(0, 43.9, 2.0), x0=(-7.0, 43.4, 2.0), x1=(7.0, 43.4, 2.0), ztop=(7.0, 43.4, 2.0), zbot=(7.0, 43.4, 1.0),
                   rib=(-12, 43.8, 2.0), floor=(0, 38, 1.0)))
gpair("z3_camera_window_inside", "camera_window", show=BACK, target=(0, CAMY, 1.0), d=(0.25, -0.6, 1.0), up=UPY, ext=30,
      anchors=dict(c=(0, CAMY, 1.0), e0=(-3.5, CAMY, 1.0), e1=(3.5, CAMY, 1.0), rib=(-10, 43.8, 2.0)))
gpair("z3_camera_window_outside", "camera_window", show=BACK, target=(0, CAMY, 0.0), d=(0.25, -0.6, -1.0), up=UPY, ext=30,
      anchors=dict(c=(0, CAMY, 0.0), e0=(-3.5, CAMY, 0.0), e1=(3.5, CAMY, 0.0)))
gpair("z5_magnet_notch", "magnet_slot (front", show=FRONT, target=(XM, MAGY, 5.0), d=(0.3, 1.0, 0.45), up=(0, 0, 1), ext=42,
      anchors=dict(c=(XM, MAGY, 5.0), l=(NOTCH[0], MAGY, RIM), r=(NOTCH[1], MAGY, RIM), lt=(NOTCH[0], MAGY, NOTCH[3]),
                   rt=(NOTCH[1], MAGY, NOTCH[3]), rim=(XM - 18, MAGY, RIM)))
gpair("z5b_lip", "magnet_slot (back", show=BACK, target=(XM, 79.5, 1.8), d=(-0.2, -0.65, 1.0), up=UPY, ext=40,
      anchors=dict(c=(XM, 79.5, 2.0), l=(NOTCH[0], 79.8, 2.3), r=(NOTCH[1], 79.8, 2.3)))
gpair("z6_antenna_relief", "antenna_relief", show=FRONT, target=(34.0, 50.0, 6.0), d=(-0.75, -0.35, -1.0), up=UPY, ext=46,
      anchors=dict(c=(34.0, 50.0, 6.0), y15=(34.95, TOP - 15, RIM), y50=(34.95, TOP - 50, RIM),
                   zr=(34.95, 40, RIM), z7=(34.95, 40, Z67), rib=(34.95, 60, 7.0)))
gpair("z6b_pins_hooks", "pins_hooks", show=FRONT, target=(33.5, 45.0, 6.5), d=(-0.75, -0.35, -1.0), up=UPY, ext=50,
      anchors=dict(c=(33.5, 45.0, 6.5), p1=(34.6, 56.0, 7.5), p2=(34.6, 47.5, 7.5), p3=(34.6, 37.3, 7.5),
                   h1=(34.4, 64.5, 7.0), h2=(34.4, 59.0, 7.0), h3=(34.4, 24.2, 7.0)))
gpair("z6b_pins_hooks_both_walls", "pins_hooks", show=FRONT, target=(0, 45.0, 6.0), d=(0.0, -0.45, -1.0), up=UPY, ext=88,
      anchors=dict(c=(0, 45, 6), lw=(-34.6, 47.5, 7.5), rw=(34.6, 47.5, 7.5)))
GRIND_SHOTS.append(dict(name="z6c_rib_ends_before", grind_off=[], show=FRONT, target=(34.0, 26.0, 6.0), d=(-0.75, -0.6, -1.0),
                        up=UPY, ext=24, anchors=dict(c=(34.0, 26.0, 6.0), y56=(35.0, TOP - 56, RIM), y58=(35.0, 24.0, RIM),
                                                     z7=(34.95, 25.0, Z67), zr=(34.95, 25.0, RIM)),
                        polys=dict(zone=box8(*ZONE6C))))
GRIND_SHOTS.append(dict(GRIND_SHOTS[-1], name="z6c_rib_ends_after", temp_cut=ZONE6C))
# overviews (orthographic, straight on): every zone, before -> highlight = difference to all-ground
for st, off in (("before", ["solar_box", "rib_b", "camera_window", "magnet_slot", "antenna_relief", "pins_hooks"]), ("after", [])):
    GRIND_SHOTS.append(dict(name="overview_back_cover_" + st, grind_off=off, show=BACK, target=(0, 2.0, 1.0), eye=(0, 2.0, 300),
                            up=(-1, 0, 0), ext=108, w=1600, h=1000, ortho=True,
                            anchors=dict(z1=(12.5, 68.9, 2), z2=(0, 43.9, 2), z3=(0, CAMY, 1), z5b=(XM, 80.5, 2), top=(0, TOP, 1))))
    GRIND_SHOTS.append(dict(name="overview_faceplate_" + st, grind_off=off, show=FRONT, target=(0, 2.0, 6.0), eye=(0, 2.0, -300),
                            up=(1, 0, 0), ext=108, w=1600, h=1000, ortho=True, temp_cut=ZONE6C if st == "after" else None,
                            anchors=dict(z5=(XM, 81.0, 4), z6=(34.9, 50, 4), z6b_l=(-34.6, 47.5, 7.5), z6b_r=(34.6, 56, 7.5),
                                         z6c=(35.0, 25.0, 6), top=(0, TOP, 4)),
                            polys=dict(z6c=box8(*ZONE6C))))
# camera hole drill position (outside, straight on) and the magnet U-notch, straight on, orthographic
GRIND_SHOTS.append(dict(name="camera_hole_position", grind_off=[], show=BACK, target=(0, 50.0, 0.0), eye=(0, 50.0, -300), up=UPY,
                        ext=78, ortho=True, measure="cam",
                        anchors=dict(c=(0, CAMY, 0), e0=(-3.5, CAMY, 0), e1=(3.5, CAMY, 0), top=(0, TOP, 0))))
GRIND_SHOTS.append(dict(name="magnet_notch_dims", grind_off=[], show=FRONT, target=(XM + 5, MAGY, 5.0), eye=(XM + 5, 300, 5.0),
                        up=(0, 0, -1), ext=40, ortho=True, measure="notch",
                        anchors=dict(l=(NOTCH[0], MAGY, RIM), r=(NOTCH[1], MAGY, RIM), lt=(NOTCH[0], MAGY, NOTCH[3]),
                                     rt=(NOTCH[1], MAGY, NOTCH[3]))))

# ── B: v14 assembly close-ups ──
EPD_C = F(149.969, 92.986)                   # (0.031, 45.954)
CAMC = (0.0, CAMY, 1.6)
J1_PIN1 = (5.75, -23.64, 5.0)
J4_P1, J4_P2 = (8.4, 67.27, 2.0), (6.4, 67.27, 2.0)
J3_PIN1, J3_PIN4 = (150 - 123.69, CY - 72.0, 6.0), (150 - 131.31, CY - 72.0, 6.0)
H6 = (150 - 130.6, CY - 186.99, 7.0)
D_BACK, D_FRONT, D_SIDE = (0.28, -0.42, -1.0), (0.28, -0.42, 1.0), (1.0, -0.55, -0.8)
SHELL = ["Front shell", "Keymat", "Keycaps", "Window lens", "Window mask", "Solar cell (dummy)"]
BSET14 = [PCB, "E-paper panel", "Camera module", "Magnet connector", "Ribbons and wires"]
COVER = ["Back cover", "Battery lid", "Screws"]
LIPO_BOX = (-28.61, -2.59, 58.83, 78.58)
LID_OPEN = (-24.2, -12.4, 64.0, 77.1)        # battery-lid opening in the floor (front X, Y), measured at Z 0.95: round + a small tab at the top
LID_C, LID_R = (-18.3, 70.0), 5.95
def lid_ring(z, n=36):
    return [(LID_C[0] + LID_R * math.cos(2 * math.pi * k / n), LID_C[1] + LID_R * math.sin(2 * math.pi * k / n), z) for k in range(n)]
# Taiwoo TW302030 candidate (final_assembly/variant_battery_tw302030): nominal 30 x 20 x 3.0 placed 1 mm further left than
# the #1317 (left edge X -29.61, top edge Y 78.58), worst case 32 x 20.5 x 3.3 outline; the fit script must have left
# the nominal cell in the document ("TW302030 candidate") before the d22 shot is rendered.
TW = (-29.61, 0.39, 58.58, 78.58)
TW_WORST = (-29.61, 2.39, 58.08, 78.58)
J4_BOX = (4.45, 12.35, 62.69, 71.29)
def d22_shot(extra_hide):
    zt = 1.1 + 3.0 + 0.02
    return dict(name="d22_battery_tw302030", show=BACK + ["LiPo battery", "Ribbons and wires", "TW302030 candidate"],
                hide_b=["LiPo pouch", "LiPo lead", "Camera FPC"] + extra_hide, target=(-12, 66, 2.0), d=(0.3, -0.6, 1.0), up=UPY, ext=62,
                polys=dict(worst=box_pts(TW_WORST[0], TW_WORST[1], TW_WORST[2], TW_WORST[3], 4.42),
                           tape1=box_pts(TW[1] - 7.0, TW[1] - 1.0, TW[2] + 1.0, TW[2] + 19.0, zt),
                           tape2=box_pts(TW[0] + 1.0, TW[0] + 17.0, TW[2] + 1.0, TW[2] + 5.0, zt),
                           lid=lid_ring(zt), j4=box_pts(*J4_BOX, 1.6)),
                anchors=dict(cell=(-14.6, 70.5, 4.1), post=(-34.05, 74.05, 4.0), rib=(-14.0, 57.3, 2.0), lip=(-14.0, 79.8, 2.0),
                             leads=(0.99, 57.5, 3.5), plug=(8.4, 60.6, 4.2), leadend=(0.39, 66.0, 2.6)))

DETAIL14 = [
    dict(name="d01_epaper_tape", show=[PCB], target=(0, 42, 8), d=D_FRONT, ext=88,
         polys=dict(panel=box_pts(EPD_C[0] - 29.5, EPD_C[0] + 29.5, EPD_C[1] - 14.6, EPD_C[1] + 14.6, 7.8),
                    tape1=box_pts(-23.0, 27.5, 33.5, 37.5, 7.8), tape2=box_pts(-23.0, 27.5, 54.4, 58.4, 7.8)),
         anchors=dict(slot=(-22.9, 46, 7.8), ledge=(-27, 46, 7.8), keys=(0, 8, 7.8))),
    dict(name="d02_epaper_on_board", show=[PCB, "E-paper panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo"],
         target=(0, 42, 8), d=D_FRONT, ext=88, anchors=dict(panel=(0, 46, 9), ribbon=(-29.5, 46, 8.9), slot=(-22.9, 37, 7.8))),
    dict(name="d03_j2_latch_open", show=[PCB, "E-paper panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo", "E-paper FPC"],
         target=(-25.5, 46, 6), d=(0.9, -0.3, -1.0), ext=22, anchors=dict(j2=(-25.5, 46, 5), latch=(-27.9, 46, 5), slot=(-22.9, 40, 7.0))),
    dict(name="d04_j2_latch_closed", show=[PCB, "E-paper panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo"],
         target=(-25.5, 46, 6), d=(0.9, -0.3, -1.0), ext=22, anchors=dict(j2=(-25.5, 46, 5), latch=(-27.9, 46, 5), ribbon=(-22.9, 40, 6.5))),
    dict(name="d05_camera_tape", show=[PCB], target=(0, CAMY, 6), d=D_BACK, ext=36,
         polys=dict(kapton=box_pts(-6.5, 6.5, CAMY - 6.5, CAMY + 6.5, 6.98), tape=box_pts(-4.0, 4.0, CAMY - 4.0, CAMY + 4.0, 6.97)),
         anchors=dict(c=(0, CAMY, 7.0), j1dir=(0, CAMY - 12, 7.0))),
    dict(name="d06_camera_ribbon_path", show=[PCB, "Camera module", "Ribbons and wires"], hide_b=["E-paper FPC", "LiPo"],
         target=(0, 12, 5), d=D_BACK, ext=82, h=1200,
         anchors=dict(camera=CAMC, ribbon=(0, 15, 5.5), j1=(0, -19.6, 5.5), lens=(0, CAMY, 1.0))),
    dict(name="d07_j1_latch_open", show=[PCB, "Camera module", "Ribbons and wires"], hide_b=["E-paper FPC", "LiPo", "Camera FPC"],
         target=(0, -19.5, 5.5), d=D_BACK, ext=28, anchors=dict(j1=(0, -19.6, 5.0), latch=(0, -22.3, 5.0), pin1=J1_PIN1)),
    dict(name="d08_j1_latch_closed", show=[PCB, "Camera module", "Ribbons and wires"], hide_b=["E-paper FPC", "LiPo"],
         target=(0, -19.5, 5.5), d=D_BACK, ext=28, anchors=dict(j1=(0, -19.6, 5.0), latch=(0, -22.3, 5.0), ribbon=(0, -12, 5.5))),
    dict(name="d09_magnet_j3", show=[PCB, "Magnet connector"], move={"Magnet connector": (0, 7.5, 0)},
         target=(22.5, 74, 5.5), d=D_BACK, ext=32,
         anchors=dict(pin1=J3_PIN1, pin4=J3_PIN4, nleg=(26.25, 80.5, 5.7), face=(22.5, 88.5, 5.7), nface=(26.5, 89.2, 5.7))),
    # tape L (battery_upgrade.md section 8): strip 1 6 x 18 under the lead end (outer edge 1 mm in from the lead edge,
    # from 1 mm above the bottom edge), strip 2 4 x 16 along the bottom edge (1 mm in from the left edge); both on solid
    # floor only. LID_OPEN = the battery-lid opening in the floor, measured in the model at Z 0.95 (fit_tw302030.py).
    dict(name="d10_battery_tape", show=BACK, target=(-14, 66, 1.0), d=(0.3, -0.6, 1.0), up=UPY, ext=52,
         polys=dict(outline=box_pts(*LIPO_BOX, 1.02), tape1=box_pts(LIPO_BOX[1] - 7.0, LIPO_BOX[1] - 1.0, LIPO_BOX[2] + 1.0, LIPO_BOX[2] + 19.0, 1.03),
                    tape2=box_pts(LIPO_BOX[0] + 1.0, LIPO_BOX[0] + 17.0, LIPO_BOX[2] + 1.0, LIPO_BOX[2] + 5.0, 1.03),
                    lid=lid_ring(1.02)),
         anchors=dict(c=(-15.6, 68.7, 1.0), j4end=(-2.6, 68.7, 1.0), post=(-34.05, 74.05, 3.0), rib=(-14.0, 57.3, 2.0))),
    dict(name="d11_battery_placed", show=BACK + ["LiPo battery", "Ribbons and wires"], hide_b=["E-paper FPC", "Camera FPC"],
         target=(-9, 66, 2.0), d=(0.3, -0.6, 1.0), up=UPY, ext=56,
         anchors=dict(battery=(-15.6, 68.7, 4.8), leads=(-1.5, 70.5, 3.0), plug=(8.4, 63, 3.0))),
    dict(name="d12_j4_plug", show=[PCB, "LiPo battery", "Ribbons and wires"], hide_b=["E-paper FPC", "Camera FPC"],
         target=(7.4, 66, 3.0), d=D_BACK, ext=26, anchors=dict(p1=J4_P1, p2=J4_P2, plug=(7.4, 62.5, 2.5), j4=(8.4, 69.5, 2.5))),
    dict(name="d13_keymat_lowering", show=SHELL, move={"Keymat": (0, 0, -22)}, target=(0, -5, -2), d=D_SIDE, ext=190,
         anchors=dict(mat=(0, -20, -14), posts=(30, -60, 9), notch=(0, -75, -14))),
    dict(name="d14_keymat_seated", show=SHELL, target=(0, -5, 8), d=D_BACK, ext=185, h=1200,
         anchors=dict(mat=(0, -20, 7.5), posts=(30, -60, 9))),
    dict(name="d15_board_drop", show=SHELL + BSET14, hide_b=["LiPo"], move={k: (0, 0, -24) for k in BSET14},
         target=(0, -10, -4), d=D_SIDE, ext=190,
         anchors=dict(board=(0, 0, -17), h6=(H6[0], H6[1], -17), h6post=(H6[0], H6[1], 7.0))),
] + [dict(name="d%02d_cover_close_%d" % (16 + i, i + 1), show=SHELL + BSET14 + ["LiPo battery"] + COVER,
          move={"Back cover": (0, 0, dz), "Battery lid": (0, 0, dz), "Screws": (0, 0, dz + ds)},
          target=(0, 0, -10), d=D_SIDE, ext=215,
          anchors=dict(cover=(0, -40, dz), window=(0, CAMY, dz), screws=(30, -60, dz + ds), magnet=(22.5, 81.7, 5.7)))
     for i, (dz, ds) in enumerate(((-62, -30), (-38, -26), (-14, -20), (0, 0)))] + [
    dict(name="d20_window_mask_off", show=["Front shell", "Window lens", "Window mask"], move={"Window mask": (0, 0, -18)},
         target=(0, 46, 10), d=(0.25, -0.35, -1.0), up=UPY, ext=82, anchors=dict(mask=(0, 46, -6), lens=(0, 58, 14))),
    dict(name="d21_window_mask_on", show=["Front shell", "Window lens", "Window mask"],
         target=(0, 46, 10), d=(0.25, -0.35, -1.0), up=UPY, ext=82, anchors=dict(mask=(0, 46, 12.5), edge=(-26, 46, 12.5))),
]

# ── C: v15 LCD close-ups ──
BSET15 = [PCB, "LCD panel", "Camera module", "Magnet connector", "Ribbons and wires"]
BL = (150 - 176.16, 150 - 126.44, CY - 106.0, CY - 80.2)     # LCD backlight outline (front frame) = jig window
SLOT15 = (-31.5, -30.5, CY - 103.1, CY - 83.1)
J5_PIN1 = (-7.4, 38.59, 6.0)
TY = CY - 93.6                                               # tail centre line (front Y 45.34)
PILLS = [("SHIFT", (177.75, 119.8), (177.773, 119.815), (176.623, 120.795), 3.8),
         ("ALPHA", (167.15, 120.8), (167.126, 121.005), (166.026, 120.795), 3.9),
         ("ON", (122.45, 119.55), (122.467, 119.565), (123.377, 120.795), 3.5)]

def circle(c, r, z, n=28):
    return [(c[0] + r * math.cos(2 * math.pi * k / n), c[1] + r * math.sin(2 * math.pi * k / n), z) for k in range(n)]

_kp = {}
for nm, pad, casio, old, d_ in PILLS:
    pc, cc, oc = F(*pad), F(*casio), F(*old)
    _kp["pad_" + nm] = box_pts(pc[0] - 3.0, pc[0] + 3.0, pc[1] - 2.25, pc[1] + 2.25, 7.81)
    _kp["pill_" + nm] = circle(cc, d_ / 2, 7.82)
    _kp["old_" + nm] = box_pts(oc[0] - 3.0, oc[0] + 3.0, oc[1] - 2.25, oc[1] + 2.25, 7.81)

DETAIL15 = [
    dict(name="e01_lcd_jig", show=[PCB], target=(-3, 44, 8), d=D_FRONT, ext=92,
         polys=dict(paper=box_pts(-35.5, 35.5, 17.0, 76.9, 7.81), window=box_pts(*BL, 7.82), slot=box_pts(*SLOT15, 7.82)),
         anchors=dict(slot=(-31.0, 45.84, 7.8), bar=(30, 20, 7.8), win=(BL[0] + 4, BL[3] - 3, 7.8))),
    dict(name="e02_lcd_on_board", show=[PCB, "LCD panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo"],
         target=(-3, 44, 8), d=D_FRONT, ext=92, polys=dict(window=box_pts(*BL, 7.82)),
         anchors=dict(panel=(0, 45.84, 9.3), tail=(-30.6, 45.84, 8.7), slot=(-31.0, 37.5, 7.8))),
    dict(name="e03_tail_section_side", show=[PCB, "LCD panel", "Ribbons and wires", "Back cover"], hide_b=["Camera FPC", "LiPo"],
         section=("xz", TY, False), target=(-19.8, TY, 5.2), eye=(-19.8, TY + 300, 5.2), up=(0, 0, 1), ext=16.5, w=1600, h=1000, ortho=True,
         anchors=dict(slot=(-31.0, TY, 7.4), top_bend=(-30.7, TY, 8.7), bow=(-22.5, TY, 2.4), bow_l=(-29.8, TY, 2.4),
                      bow_r=(-15.4, TY, 2.4), j5=(-8.5, TY, 6.42), board=(-18, TY, 7.4), panel=(-24, TY, 9.1), floor=(-20, TY, 1.0),
                      riser=(-14.2, TY, 4.4))),
    dict(name="e04_tail_3d", show=[PCB, "LCD panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo"],
         target=(-21, 45.5, 5), d=(0.55, -0.5, -1.0), ext=40,
         anchors=dict(slot=(-31.0, 38, 7.0), bow=(-22.5, 45.5, 2.4), j5=(-8.6, 45.8, 6.4), panel_edge=(-31, 52, 8.8))),
    dict(name="e05_j5_finger1", show=[PCB, "LCD panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo"],
         target=(-8.6, 40.5, 6.2), d=D_BACK, ext=14, anchors=dict(pin1=J5_PIN1, j5=(-8.6, 44, 6.0), tail=(-12.5, 41.5, 6.4))),
    dict(name="e06_j5_latch_open", show=[PCB, "LCD panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo", "LCD FPC"],
         target=(-8.6, 45.8, 6.2), d=D_BACK, ext=26, anchors=dict(j5=(-8.6, 45.8, 6.0), latch=(-7.2, 45.8, 6.0), pin1=J5_PIN1)),
    dict(name="e07_j5_latch_closed", show=[PCB, "LCD panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo"],
         target=(-8.6, 45.8, 6.2), d=D_BACK, ext=26, anchors=dict(j5=(-8.6, 45.8, 6.0), latch=(-7.2, 45.8, 6.0), tail=(-13.5, 45.8, 6.4))),
    dict(name="e08_bow_vs_rib_b", show=[PCB, "LCD panel", "Ribbons and wires", "Back cover"], hide_b=["Camera FPC", "LiPo"],
         section=("xz", 44.0, False), target=(-25, 44.0, 3.5), eye=(-25, 344.0, 3.5), up=(0, 0, 1), ext=12, w=1600, h=1000, ortho=True,
         anchors=dict(bow=(-29.8, 44.0, 2.4), rib=(-29.8, 44.0, 2.0), gap_hi=(-29.8, 44.0, 2.25), gap_lo=(-29.8, 44.0, 2.0),
                      ground=(-5.0, 44.0, 1.0), floor=(-22, 44.0, 1.0), board=(-22, 44.0, 7.4))),
    dict(name="e09_keys_vs_pills", show=[PCB], target=(0, 19.0, 7.8), eye=(0, 19.0, 300), up=UPY, ext=42, w=1600, h=1000, ortho=True,
         polys=_kp, anchors={("k_" + nm): F(*pad) + (7.8,) for nm, pad, _, _, _ in PILLS}),
    dict(name="e10_window_mask_off", show=["Front shell", "Window lens", "Window mask"], move={"Window mask": (0, 0, -18)},
         target=(0, 46, 10), d=(0.25, -0.35, -1.0), up=UPY, ext=82, anchors=dict(mask=(0, 46, -6), lens=(0, 58, 14))),
    dict(name="e11_window_mask_on", show=["Front shell", "Window lens", "Window mask"],
         target=(0, 46, 10), d=(0.25, -0.35, -1.0), up=UPY, ext=82, anchors=dict(mask=(0, 46, 12.5), edge=(-23, 46, 12.5))),
    dict(name="e12_seeed_route_section", show_all=True, hide=["Magnetic cable plug (detached)"], section=("yz", 0.01, False),
         target=(0.0, 12.0, 4.0), eye=(300, 12.0, 4.0), up=(0, 0, 1), ext=44, w=1600, h=1000, ortho=True,
         anchors=dict(camera=(0, CAMY, 4.0), j1=(0, -19.6, 5.9), ribbon=(0, 15, 6.4), floor=(0, 20, 1.0), rib_c=(0, 31.8, 2.0))),
    dict(name="e13_seeed_route_back", show_all=True, hide=["Magnetic cable plug (detached)", "Back cover", "Screws", "Rubber feet",
                                                            "Battery lid", "LiPo battery"],
         target=(0, 14, 5), d=(-0.35, -0.6, -1.0), up=UPY, ext=82,
         anchors=dict(camera=CAMC, ribbon=(0, 15, 6.0), j1=(0, -19.6, 5.5))),
]

DETAIL14.append(d22_shot(["E-paper FPC"]))
DETAIL15.append(d22_shot(["LCD FPC"]))

SETS = {"grind": ("v15", os.path.join(ENC, "final_assembly", "renders", "grind"), GRIND_SHOTS),
        "detail": ("v14", os.path.join(ENC, "final_assembly", "renders", "detail"), DETAIL14),
        "detail15": ("v15", os.path.join(ENC, "final_assembly_v15_lcd", "renders", "detail"), DETAIL15)}

# ═════════════════════════════════════════════════════════════════════════════
# Fusion side
# ═════════════════════════════════════════════════════════════════════════════
def load_build(ver):
    spec = importlib.util.spec_from_file_location("build_" + ver, SRC[ver])
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)                 # no ARGS in its globals: definitions only, no stage runs
    m.app = app
    m.ARGS = {}
    m.design = m.find_design()
    m.wire_replica()
    return m

def timeline_items():
    tl = M.design.timeline
    for i in range(tl.count):
        it = tl.item(i)
        try:
            nm = it.entity.name
        except Exception:
            nm = it.name or ""
        yield it, nm or ""

def set_grinds(off):
    """GRIND features whose name (after 'GRIND ') starts with a key in `off` suppressed, all others active."""
    for it, nm in timeline_items():
        if nm.startswith("GRIND "):
            want = any(nm[6:].startswith(k) for k in off)
            if it.isSuppressed != want:
                it.isSuppressed = want

def temp_cut(box):
    """Zone 6c: a temporary cut of the inner ribs' lower ends (both walls), returned so it can be deleted."""
    fr = M.comp_named("Front shell")
    fb = M.R.body_named(fr, "Front shell")
    x0, x1, y0, y1, z0, z1 = box
    feats = []
    for s in (1, -1):
        xa, xb = sorted((s * x0, s * x1))
        feats.append(M.xy_prism(fr, [(xa, y0), (xb, y0), (xb, y1), (xa, y1)], z0, z1, "TEMP guide 6c", "cut", [fb]))
    return feats

def drop_temp():
    for it, nm in reversed(list(timeline_items())):
        if nm.startswith("TEMP guide"):
            try:
                it.entity.deleteMe()
            except Exception:
                pass
    for comp in M.design.allComponents:
        for sk in list(comp.sketches):
            if sk.name.startswith("TEMP guide"):
                sk.deleteMe()

def strip(nm):
    return nm.replace(M.PREFIX, "").replace("Replica - ", "")

def measure(kind):
    """Read-only distances for the dimension pictures (temporary B-rep copies, nothing added to the design)."""
    import adsk.core, adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    def slab(body, x0, x1, y0, y1, z0, z1):
        a = tbm.copy(body)
        b = tbm.createBox(adsk.core.OrientedBoundingBox3D.create(
            M.P((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), adsk.core.Vector3D.create(1, 0, 0), adsk.core.Vector3D.create(0, 1, 0),
            (x1 - x0) / 10, (y1 - y0) / 10, (z1 - z0) / 10))
        tbm.booleanOperation(a, b, adsk.fusion.BooleanTypes.IntersectionBooleanType)
        bb = a.boundingBox
        return [bb.minPoint.x * 10, bb.maxPoint.x * 10, bb.minPoint.y * 10, bb.maxPoint.y * 10, bb.minPoint.z * 10, bb.maxPoint.z * 10]
    if kind == "cam":
        bb = M.R.body_named(M.comp_named("Back cover"), "Back cover")
        across = slab(bb, -60, 60, CAMY - 0.05, CAMY + 0.05, -0.5, 0.4)      # outer skin at the hole's height
        along = slab(bb, -0.05, 0.05, 0, 100, -0.5, 0.4)
        return dict(left_X=across[0], right_X=across[1], top_Y=along[3])
    if kind == "notch":
        fb = M.R.body_named(M.comp_named("Front shell"), "Front shell")
        wall = slab(fb, 0, 60, MAGY - 1.5, MAGY + 1.0, RIM + 1.0, RIM + 3.0)  # top wall just below the rim
        return dict(right_X=wall[1])
    return {}

def run_set(setname, only):
    import adsk.core
    ver, outdir, shots = SETS[setname]
    os.makedirs(outdir, exist_ok=True)
    apath = os.path.join(outdir, "anchors.json")
    anchors = json.load(open(apath)) if os.path.exists(apath) else {}
    I = M.load_info()
    grid_was = M.set_grid(False)
    vp = app.activeViewport
    global CAM_TYPE0
    CAM_TYPE0 = vp.camera.cameraType
    done = []
    try:
        for s in shots:
            if only and not any(s["name"].startswith(o) for o in only):
                continue
            t0 = time.time()
            if "grind_off" in s:
                set_grinds(s["grind_off"])
            drop_temp()
            if s.get("temp_cut"):
                temp_cut(s["temp_cut"])
            M.place({})
            show = s.get("show", [])
            for occ in M.design.rootComponent.occurrences:
                nm = strip(occ.component.name)
                occ.isLightBulbOn = (nm not in s.get("hide", []) and not nm.startswith("Slide case")) if s.get("show_all") else nm in show
            hb = s.get("hide_b", [])
            for occ in M.design.rootComponent.occurrences:
                for b in occ.component.bRepBodies:
                    b.isLightBulbOn = "cutter" not in b.name and not any(h in b.name for h in hb)
            M.hide_cutters()
            M.place(s.get("move", {}))
            W, H = s.get("w", 1600), s.get("h", 1200)
            sec = M.section(*s["section"]) if s.get("section") else None
            try:
                t = s["target"]
                if "eye" in s:
                    eye = s["eye"]
                else:
                    d = s["d"]; n = math.sqrt(sum(c * c for c in d))
                    eye = tuple(t[k] + d[k] / n * 300 for k in range(3))
                ctype = adsk.core.CameraTypes.OrthographicCameraType      # all shots orthographic, like the existing renders
                cam = vp.camera
                if cam.cameraType != ctype:
                    cam.cameraType = ctype
                    cam.isSmoothTransition = False
                    vp.camera = cam
                    adsk.doEvents()
                    cam = vp.camera
                cam.isFitView = False
                cam.target = M.P(*t); cam.eye = M.P(*eye)
                cam.upVector = adsk.core.Vector3D.create(*s.get("up", UPY))
                cam.viewExtents = M.cm(s["ext"] * min(W, H) / H)
                cam.isSmoothTransition = False
                vp.camera = cam
                adsk.doEvents()
                vp.refresh(); adsk.doEvents()
                vp.saveAsImageFile(os.path.join(outdir, s["name"] + ".png"), W, H)
                c = vp.modelToViewSpace(M.P(*t))
                # orthographic: viewExtents spans min(W, H) of the saved picture; view space = the viewport's physical
                # pixels with the target at its centre
                sc = min(W, H) / min(2 * c.x, 2 * c.y)
                px = lambda p: (lambda q: [round(W / 2 + (q.x - c.x) * sc, 1), round(H / 2 + (q.y - c.y) * sc, 1)])(vp.modelToViewSpace(M.P(*p)))
                rec = dict(size=[W, H], points={k: px(p) for k, p in s.get("anchors", {}).items()},
                           polys={k: [px(p) for p in pts] for k, pts in s.get("polys", {}).items()},
                           mm_per_px=None)
                # scale at the target (for rulers): pixels per mm along the screen's x axis
                e = vp.modelToViewSpace(M.P(t[0] + 1.0, t[1], t[2]))
                f = vp.modelToViewSpace(M.P(t[0], t[1] + 1.0, t[2]))
                g_ = vp.modelToViewSpace(M.P(t[0], t[1], t[2] + 1.0))
                rec["px_per_mm"] = round(sc * max(math.hypot(e.x - c.x, e.y - c.y), math.hypot(f.x - c.x, f.y - c.y), math.hypot(g_.x - c.x, g_.y - c.y)), 3)
                if s.get("measure"):
                    rec["measure"] = measure(s["measure"])
                    if s["measure"] == "cam":
                        m_ = rec["measure"]
                        rec["points"].update(left=px((m_["left_X"], CAMY, 0)), top=px((0, m_["top_Y"], 0)))
                    if s["measure"] == "notch":
                        m_ = rec["measure"]
                        rec["points"].update(edge=px((m_["right_X"], MAGY, RIM)), edge_t=px((m_["right_X"], MAGY, NOTCH[3])))
                anchors[s["name"]] = rec
                done.append(s["name"])
                log("%s %s %.0f s" % (setname, s["name"], time.time() - t0))
            finally:
                if sec:
                    sec.deleteMe()
                json.dump(anchors, open(apath, "w"), indent=1)
    finally:
        drop_temp()
        set_grinds([])
        M.place({})
        for occ in M.design.rootComponent.occurrences:
            occ.isLightBulbOn = not strip(occ.component.name).startswith("Slide case")
            for b in occ.component.bRepBodies:
                b.isLightBulbOn = "cutter" not in b.name
        M.hide_cutters()
        cam = vp.camera
        if cam.cameraType != CAM_TYPE0:
            cam.cameraType = CAM_TYPE0
            vp.camera = cam
        if grid_was is not None:
            M.set_grid(grid_was)
    return done

# ═════════════════════════════════════════════════════════════════════════════
def send(args):
    import urllib.request, urllib.error
    script = "ARGS = %r\n" % args + open(os.path.abspath(__file__), encoding="utf8").read()
    secret = open(os.path.join(os.path.expanduser("~"), ".fusion-mcp-secret")).read().strip()
    req = urllib.request.Request("http://127.0.0.1:7654/execute", data=json.dumps({"script": script}).encode(),
                                 headers={"Authorization": "Bearer " + secret, "Content-Type": "application/json"})
    start = os.path.getsize(LOG) if os.path.exists(LOG) else 0
    tag = "run %s %s" % (args.get("set"), args.get("only", ""))
    try:
        r = json.load(urllib.request.urlopen(req, timeout=60))
    except urllib.error.HTTPError as e:
        r = json.load(e)
    except Exception as e:
        r = {"error": "timeout: %s" % e}
    if "timeout" in str(r.get("error", "")).lower():
        tail = lambda: open(LOG, "rb").read()[start:].decode("utf8", "replace") if os.path.exists(LOG) else ""
        t0 = time.time()
        while tag + " done" not in tail() and "FAILED" not in tail() and time.time() - t0 < 5400:
            time.sleep(4)
        print(tail()[-4000:])
        return
    print(r.get("result") or r)

if "ARGS" in globals():
    import adsk.core, adsk.fusion, traceback
    tag = "run %s %s" % (ARGS.get("set"), ARGS.get("only", ""))
    log(tag)
    try:
        ver = SETS[ARGS["set"]][0]
        M = load_build(ver)
        done = run_set(ARGS["set"], [o for o in ARGS.get("only", "").split(",") if o])
        print("rendered:", done)
    except Exception:
        log(tag + " FAILED " + traceback.format_exc())
        raise
    log(tag + " done")
elif __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    send(dict(a.split("=", 1) for a in sys.argv[1:] if "=" in a))
