"""1:1 Fusion 360 model of the FINISHED AI calculator: our ESP32-S3 board and every off-board part
inside the ground (modified), unbranded fx-115ES shell replica.

    python build_final_assembly.py all              every stage in order (new Fusion document)
    python build_final_assembly.py pcb parts check  re-run single stages (after a board change: see README)
    python build_final_assembly.py renders          all pictures again
    python build_final_assembly.py                  offline: print the derived numbers only

Stages: new shell grind pcb parts looks check export renders  (= "all")
Fusion must be open with the FusionMCPBridge add-in (same as fusion_run.py). Log: final_assembly/build.log.

The shell is built by build_fx115es_replica.py (loaded as a module, its parameters and stages reused
unchanged), then the grind zones of hardware/stage13_heights.md section 6 are cut as separate,
named timeline features (suppress one to see the shell "before"). The board comes from
hardware/fab/ai_calc_board.step; off-board parts are modelled from vendor drawings (see README).

Frame (same as the replica): mm, front view. X right, Y up (towards the display), Z = 0 at the
outside of the back cover. KiCad points: X = 150 - x_kicad, Y = 138.94 - y_kicad.
stage13 section 7 heights ("z_rel", + towards the back cover, 0 = board component side) map to
Z = Z_F - z_rel, with Z_F = 7.0 (back floor 1.0 + D4 6.0).
Nothing here is branded.
"""
import json, math, os, sys, time, importlib.util

REPO = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR"
ENC = os.path.join(REPO, "hardware", "enclosure")
# ── v15-LCD COPY (2026-10-08) ── everything is written to final_assembly_v15_lcd/; the v14 model, the v14 board and the
# replica script are only READ. Board = the v15-LCD KiCad board (hardware/kicad_v15_lcd), STEP exported by kicad-cli into
# final_assembly_v15_lcd/board/ (J5 has no 3D model in the STEP: a marked proxy box is added in stage parts).
OUT = os.path.join(ENC, "final_assembly_v15_lcd")
REPLICA_PY = os.path.join(ENC, "build_fx115es_replica.py")          # read only (rev G shell)
STEP = os.path.join(OUT, "board", "ai_calc_v15_lcd_board.step")

def _opt(key, default=None):
    """Variant options (2026-10-09), from ARGS inside Fusion or key=value on the command line. Defaults = the baseline."""
    A = globals().get("ARGS")
    if A is not None:
        return A.get(key, default)
    for a in sys.argv[1:]:
        if a.startswith(key + "="):
            return a.split("=", 1)[1]
    return default
# Panel variants: out=<subfolder of final_assembly_v15_lcd> writes every output there (placement.json and board_snapshot.json are seeded
# from the baseline's on first use; the board STEP is still read from the baseline); lcd_dt=<mm> adds that much panel thickness
# in the backlight/frame (glass stack, ledge and tail exit move up by lcd_dt); lcd_tape=<mm> changes the tape under it.
# No options = the baseline exactly.
OUT_BASE = OUT
if _opt("out"):
    OUT = os.path.join(OUT_BASE, _opt("out"))
    os.makedirs(OUT, exist_ok=True)
    import shutil
    for _fn in ("placement.json", "board_snapshot.json"):          # board_snapshot.json: read by the replica (rev G mat)
        if not os.path.exists(os.path.join(OUT, _fn)) and os.path.exists(os.path.join(OUT_BASE, _fn)):
            shutil.copy(os.path.join(OUT_BASE, _fn), os.path.join(OUT, _fn))
PCB_V15 = os.path.join(REPO, "hardware", "kicad_v15_lcd", "ai_calc_v15_lcd.kicad_pcb")   # read only
DOC_NAME = "AI Calculator v15-LCD - Final Assembly 1:1"
PREFIX = "V15 - "
ROOT_TAG = PREFIX + "Front shell"
CX, CY = 150.0, 138.94

def load_replica():
    spec = importlib.util.spec_from_file_location("fx115es_replica", REPLICA_PY)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)           # module globals have no ARGS: nothing runs, only definitions
    m.FIT_STEP, m.FIT_PCB = STEP, PCB_V15  # v15 board for board_info()
    m.OUT = OUT                            # any replica-side file output goes to the v15 folder, never replica/
    return m

R = load_replica()

def F(x, y):
    return (CX - x, CY - y)

# ── Stack-up ──────────────────────────────────────────────────────────────────
Z_F = R.Z_BOARD_B          # 7.0: board component (F.Cu) side = back floor 1.0 + D4 6.0
BOARD_T = R.BOARD_T        # 0.8 (the STEP board may be thinner: it is placed by its F face)
Z_B = Z_F + BOARD_T        # 7.8: key side
zrel = lambda z: Z_F - z   # stage13 section 7 height -> model Z

# ── Grind zones (stage13_heights.md section 6) ───────────────────────────────
# (key, target body, label, how)  -- each is its own timeline feature named "GRIND <key>"
SOLAR_BOX_CUT = (R.SOLAR_BOX[0] - 0.8, R.SOLAR_BOX[1] + 0.8, R.SOLAR_BOX[2] - 0.8, R.SOLAR_BOX[3] + 0.8)   # frame + grid, rib 1.0 wide
RIB_B_CAM_K = (143.0, 157.0)        # KiCad x range of rib B ground flat over the camera (14 mm)
CAM_K = (150.0, 95.1)               # camera module centre (KiCad), window drilled here
CAM_WINDOW_D = 7.0                  # stage13 section 4 (9/32" bit = 7.1)
# (rev D, 2026-10-05: the LR44-cup grind is gone. Stage 14 put the LiPo on the back floor, so the cup stays: zone 4 = don't grind.)
MIN_FLOOR = 0.8                     # never grind into the floor below this
# Zone 6 (rev F, verification/07): on the KiCad-low-x side (the calculator's right-hand side in use) the faceplate's
# inner side-wall rib (inner face at x ~115.1) reaches the board plane if C4 4.7 was read on the rim. Two things on the
# board run into it: the ESP32-S3-MINI-1 antenna tab (bare 0.8 mm module PCB, x 114.75-118.9, y 89.3-104.7: 0.4 mm) and
# the board's own top-left corner strip (Edge.Cuts x 114.28-114.77, y 72.0-82.9: up to 0.85 mm; stage 12 narrowed the
# board to x 118.9 only from y 82.9 down). Relief: the rib (1.0 thick) is removed between these KiCad y values, from the
# rim down to STUB_Z0 (6.5 mm below the rim); the wire hooks and pins above the board plane stay. Only needed if the
# depth rod shows the rib at < 5.5 mm below the rim (pre-grind checklist); the paper dry fit tests the same thing.
ANT_RELIEF_K = (71.5, 106.5)        # KiCad y range of the relief: board corner (72.0-82.9) + antenna (89.3-104.7) + 0.5/1.8
ANT_RELIEF_EXTRA = 0.3              # cut this much beyond the rib on both faces (clean break-out)
# Zone 6b (rev G, verification/12, 2026-10-06): the depth rod put the rib top at 4.5-5.0 below the rim, the round pins at
# 5.1-5.5 (outer) / 6.0-6.1 (middle) and the U hooks at 5.5: every pin and hook crosses the board plane (Z 7.0-7.8), on both
# walls. The board edge passes 0.55 (J2 side) / the antenna tab 1.05 mm (solar side) inside the pin tips, the two upper hooks
# on the solar side are 1.1 mm inside the board's corner strip. Fix: snip all 3 pins + 3 hooks flush with the rib on BOTH
# walls (flush cutters; the hooks held Casio's wires, nothing of ours runs there). Modelled as a cut of the pin/hook prisms.
GRINDS = ["solar_box", "rib_b", "camera_window", "magnet_slot", "antenna_relief", "pins_hooks"]   # rev D: no "lr44_cup" (the cup stays)

# ── Magnet connector (Adafruit #5358 = Yiwei MG04254FRA1S1N, drawing in hardware/geometry) ──
# Option A of stage13 section 3 (stage 13b): body straddles a notch in the board's top edge, face glued
# into a slot in the shell's top wall, straightened legs pushed into a right-angle SMD header (C46061768).
MAG_XK = 127.5                      # KiCad x of the face centre (stage 13b: J3 = C46061768 at (127.5, 72.0))
MAG_ZREL = (-2.2, 4.8)              # body height range (z_rel), 7.0 face height
MAG_FACE = (21.0, 7.0, 1.0)         # face: obround length (X), height (Z), flange thickness (drawing)
# behind the face (stage 13b re-read of the drawing): 0.7 plate, 3.5-tall neck, 1.0 rear block 5.0 tall; 4.0 total.
# Widths along X are not on the drawing: estimates.
MAG_BODY = [("plate", "obround", 20.0, 6.4, 0.7), ("neck", "rect", 17.0, 3.5, 1.3), ("rear block", "rect", 17.0, 5.0, 1.0)]
MAG_PITCH, MAG_PIN_D = 2.5, 0.6     # 4 pins
MAG_PIN_L = 5.3                     # legs straightened (stage 13b), behind the rear block, into J3
MAG_PAD_D, MAG_MAGNET_X = 1.5, 7.0  # contact pads on the face, magnets at +-7 (drawing: 14 apart)
MAG_SLOT = (21.5, 7.95)             # U-notch in the top wall from the rim: width, depth below the rim (stage 13b grind #5)
MAG_FLUSH = 0.0                     # face this far inside the outer wall surface
PLUG_GAP = 12.0                     # the cable's magnetic plug is shown this far out (detached)
PLUG_BODY = (23.0, 9.0, 10.0)       # #5412 plug overmould: width, height, length (estimate, photos)

# ── Off-board parts ───────────────────────────────────────────────────────────
EPD = (59.0, 29.2, 1.05)            # Waveshare 2.13" V4 raw panel: 59.2 x 29.2 x 1.05 (Waveshare), P3 caliper 59.0
EPD_K = (149.969, 92.986)           # panel centre (KiCad keep-out)
EPD_AA = (48.55, 23.70)             # active area (Waveshare); centre from the KiCad keep-out
EPD_AA_K = (147.445, 92.986)
EPD_TAPE = 0.15                     # double-sided tape under the panel (also the room for the FPC fold)
EPD_LEDGE = 5.0                     # bottom-glass ledge at the FPC end (no top layer): estimate
EPD_TOP_T = 0.35                    # top layer (e-ink film + protective sheet) of the 1.05 total: estimate
EPD_FPC_W, FPC_T = 12.5, 0.12       # ribbon width (24 x 0.5 pitch + margins; board slot is 14), thickness
FPC_INSERT = 2.0                    # ribbon tip inside the FPC connector
EPD_FPC_LEN = 14.3                  # panel edge -> tip (stage 5 / V9); 13.5-15 works
SLOT_XK = 172.9                     # 1.0 x 14 board slot centre (KiCad x), slot x 172.4-173.4
SLOT_FPC_XK = 172.55                # the ribbon hugs the slot wall away from J2 (J2's mouth overhangs the slot to x 172.73)
CAM = (8.5, 8.5, 5.4)               # Seeed OV5640 AF module footprint and height (stage13)
CAM_TAPE = 0.1
CAM_LENS_D, CAM_LENS_H, CAM_APERTURE = 6.0, 1.0, 2.8   # lens barrel on top of the VCM (estimate)
CAM_FPC_W, CAM_FPC_W2, CAM_FPC_LEN = 8.0, 12.5, 70.5   # cable width at the module / at the plug end, length (Seeed)
LIPO = (26.02, 19.75, 3.8)          # rev E (2026-10-06): Adafruit #1317 150 mAh (was #1570 31 x 11.5 x 3.8 at (163.4, 69.25))
LIPO_K = (165.6, 70.24)             # battery_upgrade.md: centre front (-15.6, 68.7), long side along X, lead end (+X) towards J4
LIPO_ZREL = (2.2, 6.0)              # on the back-cover floor (Z 1.0-4.8; assembly_report fix). stage13 section 7 had (-3.4, 0.4): front plate
LIPO_WIRE_D = 1.1                   # 26-28 AWG leads
PH_PLUG = (6.0, 4.5, 4.0)           # JST PHR-2 housing: width, height, exposed length (the rest is inside J4)
SCREW = dict(d_core=1.6, d_head=3.8, head_h=1.1, length=8.0)   # assumed 2.0 x 8 self-tapping pan head (see README)

# ── v15: 1.9" 170 x 320 IPS LCD (ST7789V3, 190-1732TBWPG01 family = LilyGO T-Display-S3 panel), landscape, key side ──
# Outline numbers are the PCB agent's (hardware/tools/v15_lcd/board_step1_cleanup.py, KEEPOUTS text): backlight 49.72 x 25.8,
# glass 48.52 x 24.8, active area 42.72 x 22.70 centred on (150.0, 93.1), 2.6 mm driver ledge at the FPC end (+x KiCad),
# FPC tail 15.5 wide, 36.6 long, exits +x through the 1.0 x 20 board slot to J5 on the F side.
# Review 13 (2026-10-08) moved both: slot x 180.5-181.5 (centre 181.0, y 83.1-103.1), J5 (158.6, 93.1) rot 90 with the mirrored
# LcdReversed footprint, courtyard x 157.63-160.53, mouth at x 160.53 facing the slot. J5's 3D model is now in the board STEP.
# Thickness 1.43 total incl. backlight (hardware/stage15_lcd.md, vendor spec N190-1732TBWPG01-C30); the split is ASSUMED:
# backlight 0.65 + TFT glass 0.35 + CF glass 0.30 + polarizer 0.13. FPC 0.3 thick with a 4.5 mm stiffener at the tip.
# 2026-10-08 evening: FINAL panel = BuyDisplay ER-TFT019-1 (no touch), datasheet rev 2.0 p.6: BL 49.72 x 25.8, glass 48.52 x 24.8
# (CF glass 45.92 -> 2.6 ledge), V.A. 43.72 x 23.695, A.A. 42.72 x 22.695 (centred across, 1.5 / 1.0 from the glass edge),
# 1.43 +- 0.1 thick, tail 36.6 +- 0.3 x 15.5 x 0.3, stiffener 4.5, tail ~0.5 mm off-centre towards finger 1 (+y KiCad, report 14 M4).
LCD_AA = (42.72, 22.695)
LCD_AA_K = (150.0, 93.1)
LCD_BL_X = (126.44, 176.16)         # backlight (KiCad x); y = 93.1 +- 12.9
LCD_BL_H = 25.8
LCD_GL_X = (127.04, 175.56)         # TFT glass (KiCad x); y = 93.1 +- 12.4; CF glass stops at the 2.6 ledge
LCD_GL_H = 24.8
LCD_LEDGE = 2.6
LCD_T = dict(tape=0.10, bl=0.65, tft=0.35, cf=0.30, pol=0.13)   # tape = 0.1 mm double-sided tape under the backlight
LCD_T["bl"] += float(_opt("lcd_dt", 0.0))                         # variant: thicker panel, extra in the backlight/frame
LCD_T["tape"] = float(_opt("lcd_tape", LCD_T["tape"]))
LCD_FPC_W, LCD_FPC_LEN, LCD_FPC_T, LCD_STIFF = 15.5, 36.6, 0.30, 4.5
LCD_FPC_DY = 0.5                    # tail centre this far towards +y KiCad (finger 1) from the panel centre line (ER p.6, report 14 M4)
LCD_FPC_LEN_WORST = 36.9            # 36.6 + 0.3 tolerance: stage tailcase   # tail width, length from the glass edge, thickness, tip stiffener
LCD_BOND = 1.3                      # FPC bonded on the ledge over this length (from the glass edge inwards)
LCD_SLOT_XK, LCD_SLOT = 181.0, (1.0, 20.0)   # review 13: slot x 180.5-181.5 (was 177.9)
J5_BOX_K = (157.63, 160.53, 84.6, 101.6)   # J5 courtyard (footprint), F side; mouth at x 160.53 facing +x (front insert)
J5_H = 1.0                          # HDGC 0.5K-HX-30PWB: 1.0 mm high (BOM text); the STEP model's own height is used when found
FPC_BEND_R = 1.0                    # top bend (key side, slot entry): static FPC >= 6-10 x thickness = 1.8-3; limited by the 1.25 drop
FPC_BOW_R = 1.2                     # the bow's corners under the board (one U, no crease): 4 x thickness. 2.0 was tried first:
                                    # the bow then needs Z 1.73 and hits the back cover's cross rib (top Z 2.0, front Y 43.2-44.3)

# ── Window mask (2026-10-05): matte black sticker on the INSIDE of the clear lens, opening = LCD active area ──
MASK_T = 0.1                        # vinyl / sticker paper thickness (0.08-0.15); the lens is lifted by this much
MASK_MARGIN = -0.5                  # v15: opening 0.5 mm OUTSIDE the active area (the LCD's own black border hides the
                                    # edge; no pixel is ever covered by a slightly misplaced sticker)
MASK_OPEN = (43.72, 23.70)          # window_mask_template_v15: = ER V.A. 43.72 x 23.695 (rounded)
MASK_R = 0.3                        # opening corner radius (craft-knife friendly)

# ── Slide case mods (informational, "later, not needed for testing") ──
CASE_CAM_HOLE_D = 8.0               # case hole over the 7 mm camera window (case floor is 1.2 thick, 0.3 off the back)

LOOK = {"Front shell": R.SILVER, "Back cover": R.NAVY, "Keymat": "Rubber - Soft", "Window mask": "Plastic - Matte (Black)",
        "Slide case (in use)": R.NAVY, "Slide case (stored)": R.NAVY,
        "Window lens": "Glass (Grey)", "Solar cell (dummy)": "Paint - Metallic (Dark Grey)", "Battery lid": R.NAVY,
        "Rubber feet": "Rubber - Hard", "LCD panel": "Plastic - Matte (White)", "Camera module": "Plastic - Matte (Black)",
        "LiPo battery": "Aluminum - Anodized Glossy (Grey)", "Magnet connector": "Plastic - Matte (Black)",
        "Magnetic cable plug (detached)": "Plastic - Matte (Black)", "Screws": "Steel - Satin", "Ribbons and wires": "Polymide (Kapton)"}

SCREEN_LOOK = "Paint - Enamel Glossy (Dark Grey)"   # screen off; stage v15renders switches it to a chroma key for the UI image
CHROMA = "V15 chroma key"
R.CUSTOM_LOOKS[CHROMA] = ("Plastic - Matte (Green)", (0, 255, 0), ("opaque_albedo",))

def offline_numbers():
    g = R.geometry()
    yo = R.y_at(g["Vf"], 150 - MAG_XK, +1)
    return dict(mag_X=150 - MAG_XK, mag_face_Y=yo - MAG_FLUSH, mag_Z=(zrel(MAG_ZREL[1]), zrel(MAG_ZREL[0])),
                cam=F(*CAM_K), lipo=F(*LIPO_K), lipo_Z=(zrel(LIPO_ZREL[1]), zrel(LIPO_ZREL[0])), epd=F(*EPD_K),
                top=g["top"], inner_top=R.y_at(g["C"], 150 - MAG_XK, +1), lip_top=R.y_at(g["lip_o"], 150 - MAG_XK, +1))

# ═════════════════════════════════════════════════════════════════════════════
# Fusion helpers
# ═════════════════════════════════════════════════════════════════════════════
cm = R.cm
P = R.P

def log(msg):
    with open(os.path.join(OUT, "build.log"), "a", encoding="utf8") as f:
        print(time.strftime("%H:%M:%S"), msg, file=f)

def comp_named(name, create=False):
    import adsk.core
    root = design.rootComponent
    for occ in root.occurrences:
        if occ.component.name in (PREFIX + name, "Replica - " + name):
            return occ.component
    if create:
        occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        occ.component.name = PREFIX + name
        return occ.component
    raise KeyError(name)

def occ_named(name):
    for occ in design.rootComponent.occurrences:
        if occ.component.name == PREFIX + name:
            return occ
    raise KeyError(name)

def wire_replica():
    """Point the replica module's globals at this document and our naming."""
    R.app, R.design, R.log, R.comp_named = app, design, log, comp_named
    R.ARGS = ARGS

def find_design():
    import adsk.fusion
    for d in app.documents:
        p = adsk.fusion.Design.cast(d.products.itemByProductType("DesignProductType"))
        if p and any(o.component.name == ROOT_TAG for o in p.rootComponent.occurrences):
            if not d.isActive:
                d.activate()
            return p
    raise RuntimeError("final assembly document not open: run stage new first")

def new_sketch(comp, name, plane=None):
    sk = comp.sketches.add(plane or comp.xYConstructionPlane)
    sk.name = name
    sk.isComputeDeferred = True
    return sk

def poly_pts(sk, pts3):
    """Closed polygon from model-space 3D points (works on any sketch plane)."""
    L = sk.sketchCurves.sketchLines
    q = [sk.modelToSketchSpace(P(*p)) for p in pts3]
    n = len(q)
    first = prev = None
    for i in range(n):
        ln = L.addByTwoPoints(prev.endSketchPoint if prev else q[i], q[(i + 1) % n] if i < n - 1 else first.startSketchPoint)
        first = first or ln
        prev = ln

def obround_pts(cx, cy, length, width, n=10):
    r = width / 2
    a = length / 2 - r
    pts = []
    for k in range(n + 1):
        t = -math.pi / 2 + math.pi * k / n
        pts.append((cx + a + r * math.cos(t), cy + r * math.sin(t)))
    for k in range(n + 1):
        t = math.pi / 2 + math.pi * k / n
        pts.append((cx - a + r * math.cos(t), cy + r * math.sin(t)))
    return pts

def move_body(comp, bodies, dx=0.0, dy=0.0, dz=0.0):
    import adsk.core
    col = adsk.core.ObjectCollection.create()
    for b in bodies:
        col.add(b)
    mf = comp.features.moveFeatures
    inp = mf.createInput2(col)
    inp.defineAsTranslateXYZ(adsk.core.ValueInput.createByReal(cm(dx)), adsk.core.ValueInput.createByReal(cm(dy)),
                             adsk.core.ValueInput.createByReal(cm(dz)), True)
    return mf.add(inp)

def prism(comp, plane, pts3, a0, a1, name=None, op="new", bodies=None):
    """Polygon (3D points lying on the plane through the origin: 'xy', 'xz' or 'yz') extruded
    between a0 and a1 along the plane's normal axis (Z, Y or X)."""
    import adsk.core, adsk.fusion
    pl = {"xy": comp.xYConstructionPlane, "xz": comp.xZConstructionPlane, "yz": comp.yZConstructionPlane}[plane]
    sk = new_sketch(comp, name or "prism", pl)
    poly_pts(sk, pts3)
    sk.isComputeDeferred = False
    prof = adsk.core.ObjectCollection.create()
    for p in sk.profiles:
        prof.add(p)
    if prof.count == 0:
        raise RuntimeError("no profile for " + str(name))
    ops = {"new": adsk.fusion.FeatureOperations.NewBodyFeatureOperation,
           "cut": adsk.fusion.FeatureOperations.CutFeatureOperation,
           "join": adsk.fusion.FeatureOperations.JoinFeatureOperation}
    ex = comp.features.extrudeFeatures
    inp = ex.createInput(prof, ops[op])
    L = abs(a1 - a0)
    inp.setSymmetricExtent(adsk.core.ValueInput.createByReal(cm(L)), True)
    if bodies:
        inp.participantBodies = list(bodies)
    f = ex.add(inp)
    # the plane's normal may point either way: shift the result so it spans a0..a1
    ax = {"xy": 2, "xz": 1, "yz": 0}[plane]
    mid = (a0 + a1) / 2
    if abs(mid) > 1e-6:
        if op == "new":
            move_body(comp, [f.bodies.item(i) for i in range(f.bodies.count)], *[mid if k == ax else 0 for k in range(3)])
        else:
            raise RuntimeError("cut/join prisms must be centred on the plane")
    if name and op == "new":
        for i in range(f.bodies.count):
            f.bodies.item(i).name = name if f.bodies.count == 1 else "%s %d" % (name, i + 1)
    if op != "new" and name:
        f.name = name
    return f

def xy_prism(comp, pts2, z0, z1, name=None, op="new", bodies=None):
    """XY polygon extruded from z0 to z1 (any z: uses an offset start)."""
    import adsk.core
    sk = new_sketch(comp, name or "xy")
    R.poly(sk, pts2)
    f = R.extrude(comp, R.profiles(sk), z0, z1 - z0, op, bodies, name)
    if op != "new" and name:
        f.name = name
    return f

def xy_circle(comp, c, d, z0, z1, name=None, op="new", bodies=None):
    sk = new_sketch(comp, name or "circle")
    R.circle(sk, c, d)
    f = R.extrude(comp, R.profiles(sk), z0, z1 - z0, op, bodies, name)
    if op != "new" and name:
        f.name = name
    return f

def tbm_add(comp, tbodies, name):
    """Add temporary B-rep bodies (cylinders, spheres) to a component through a base feature."""
    import adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    acc = None
    for t in tbodies:
        if acc is None:
            acc = t
        else:
            tbm.booleanOperation(acc, t, adsk.fusion.BooleanTypes.UnionBooleanType)
    bf = comp.features.baseFeatures.add()
    bf.startEdit()
    try:
        b = comp.bRepBodies.add(acc, bf)
    finally:
        bf.finishEdit()
    b = comp.bRepBodies.item(comp.bRepBodies.count - 1)
    b.name = name
    bf.name = name
    return b

def tube(points, d):
    """Temporary round wire through 3D points (cylinders + spheres at the joints)."""
    import adsk.core, adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    out = []
    for a, b in zip(points, points[1:]):
        out.append(tbm.createCylinderOrCone(P(*a), cm(d / 2), P(*b), cm(d / 2)))
    for p in points[1:-1]:
        out.append(tbm.createSphere(P(*p), cm(d / 2)))
    return out

def round_path(path, r, seg=8):
    """Polyline with every inner corner replaced by an arc of radius r (2D points)."""
    out = [path[0]]
    for i in range(1, len(path) - 1):
        a, b, c = path[i - 1], path[i], path[i + 1]
        u = R._unit((a[0] - b[0], a[1] - b[1])); v = R._unit((c[0] - b[0], c[1] - b[1]))
        cosang = max(-1, min(1, u[0] * v[0] + u[1] * v[1]))
        ang = math.acos(cosang)
        if ang > math.pi - 1e-3:
            out.append(b); continue
        t = r / math.tan(ang / 2)
        t = min(t, 0.49 * math.dist(a, b), 0.49 * math.dist(b, c))
        p0 = (b[0] + u[0] * t, b[1] + u[1] * t); p1 = (b[0] + v[0] * t, b[1] + v[1] * t)
        bis = R._unit((u[0] + v[0], u[1] + v[1]))
        rr = t * math.tan(ang / 2)
        cc = (b[0] + bis[0] * rr / math.sin(ang / 2), b[1] + bis[1] * rr / math.sin(ang / 2))
        a0 = math.atan2(p0[1] - cc[1], p0[0] - cc[0]); a1 = math.atan2(p1[1] - cc[1], p1[0] - cc[0])
        da = (a1 - a0 + math.pi) % (2 * math.pi) - math.pi
        for k in range(seg + 1):
            th = a0 + da * k / seg
            out.append((cc[0] + rr * math.cos(th), cc[1] + rr * math.sin(th)))
    out.append(path[-1])
    return out

def round_path_r(path, radii, seg=8):
    """round_path with its own radius per inner corner (radii[i] for corner i+1; the last value repeats)."""
    out = [path[0]]
    for i in range(1, len(path) - 1):
        r = radii[min(i - 1, len(radii) - 1)]
        seg_ = round_path([path[i - 1], path[i], path[i + 1]], r, seg)
        out += seg_[1:-1]
    out.append(path[-1])
    return out

def band(path, t):
    """Closed polygon of thickness t around an open 2D path (ribbon cross-section)."""
    L, Rr = [], []
    n = len(path)
    for i in range(n):
        if i == 0:
            d = R._unit((path[1][0] - path[0][0], path[1][1] - path[0][1]))
        elif i == n - 1:
            d = R._unit((path[-1][0] - path[-2][0], path[-1][1] - path[-2][1]))
        else:
            d1 = R._unit((path[i][0] - path[i - 1][0], path[i][1] - path[i - 1][1]))
            d2 = R._unit((path[i + 1][0] - path[i][0], path[i + 1][1] - path[i][1]))
            d = R._unit((d1[0] + d2[0], d1[1] + d2[1]))
        nrm = (-d[1], d[0])
        L.append((path[i][0] + nrm[0] * t / 2, path[i][1] + nrm[1] * t / 2))
        Rr.append((path[i][0] - nrm[0] * t / 2, path[i][1] - nrm[1] * t / 2))
    return L + Rr[::-1]

def path_len(p):
    return sum(math.dist(a, b) for a, b in zip(p, p[1:]))

def appearance(body, name):
    return R.appearance(body, name)

def bodies_under(o):
    out = list(o.bRepBodies)
    for c in o.childOccurrences:
        out += bodies_under(c)
    return out

def leaves(o):
    """Occurrences under o that own bodies (STEP parts), as assembly-context proxies."""
    out = [o] if o.component.bRepBodies.count else []
    for c in o.childOccurrences:
        out += leaves(c)
    return out

def info_path():
    return os.path.join(OUT, "placement.json")

def save_info(**kw):
    p = info_path()
    d = json.load(open(p)) if os.path.exists(p) else {}
    d.update(kw)
    json.dump(d, open(p, "w"), indent=1)

def load_info():
    return json.load(open(info_path())) if os.path.exists(info_path()) else {}

# ═════════════════════════════════════════════════════════════════════════════
# Stages
# ═════════════════════════════════════════════════════════════════════════════
def stage_new(g):
    import adsk.core, adsk.fusion
    global design
    for d in list(app.documents):
        p = adsk.fusion.Design.cast(d.products.itemByProductType("DesignProductType"))
        if p and any(o.component.name == ROOT_TAG for o in p.rootComponent.occurrences):
            d.close(False)
    doc = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
    design = adsk.fusion.Design.cast(app.activeProduct)
    design.designType = adsk.fusion.DesignTypes.ParametricDesignType
    pass
    comp_named("Front shell", True)
    try:                                              # name the document (saved in the active Fusion project)
        doc.saveAs(DOC_NAME, app.data.activeProject.rootFolder, "AI calculator final assembly (generated)", "")
        log("saved as %s in project %s" % (DOC_NAME, app.data.activeProject.name))
    except Exception as e:
        log("saveAs skipped: %s" % str(e)[:120])
    if os.path.exists(info_path()):
        os.remove(info_path())
    print("new document")

def stage_shell(g):
    """The replica's front shell, back cover, keymat + caps, lens, solar cell, battery lid, feet."""
    wire_replica()
    R.stage_front(g)
    R.stage_back(g)
    R.stage_keys(g)
    R.stage_parts(g)
    for nm in ("Reference - Casio board (C6)", "Reference - LCD (dummy)", "Slide case (stored)"):   # case: stage "case"
        for occ in list(design.rootComponent.occurrences):
            if occ.component.name.endswith(nm):
                occ.deleteMe()
    print("shell ok")

def grind_cuts(g):
    """[(key, component, label, builder)] for every grind zone."""
    return GRINDS

def stage_grind(g):
    import adsk.core
    fr, bk = comp_named("Front shell"), comp_named("Back cover")
    fb, bb = R.body_named(fr, "Front shell"), R.body_named(bk, "Back cover")
    feats = {}
    # 1 solar box: frame and grid flat down to the floor (Z = BACK_PLATE: 1.0 of floor stays)
    x0, x1, y0, y1 = SOLAR_BOX_CUT
    f = xy_prism(bk, [(x0, y0), (x1, y0), (x1, y1), (x0, y1)], R.BACK_PLATE, Z_F, "GRIND solar_box", "cut", [bb])
    feats["solar_box"] = f
    # 2 rib B over the camera, 14 mm, flat to the floor
    X0, X1 = sorted((150 - RIB_B_CAM_K[0], 150 - RIB_B_CAM_K[1]))
    yb = [(a[1] + b[1]) / 2 for n, a, b, h in g["ribs"] if n.startswith("upper rib B")]
    yc = sum(yb) / len(yb)
    f = xy_prism(bk, [(X0, yc - 1.5), (X1, yc - 1.5), (X1, yc + 1.5), (X0, yc + 1.5)], R.BACK_PLATE, R.BACK_PLATE + 3,
                 "GRIND rib_b", "cut", [bb])
    feats["rib_b"] = f
    # (the LR44 cup in the front shell is NOT ground any more: rev D, stage 14)
    # 4 camera window, drilled through the back floor
    cam = F(*CAM_K)
    f = xy_circle(bk, cam, CAM_WINDOW_D, -1.0, R.BACK_PLATE + 0.5, "GRIND camera_window", "cut", [bb])
    feats["camera_window"] = f
    # 5 magnet slot through the top wall (front shell wall + the back cover's lip), obround 21.5 x 7.5
    Xm = 150 - MAG_XK
    zc = zrel(sum(MAG_ZREL) / 2)
    yo = R.y_at(g["Vf"], Xm, +1)
    w2, ztop, rr = MAG_SLOT[0] / 2, R.SEAM_Z + MAG_SLOT[1], 1.0     # U-notch from the rim, rounded inner corners
    q2 = [(Xm - w2, R.SEAM_Z - 0.01), (Xm + w2, R.SEAM_Z - 0.01)]
    q2 += [(Xm + w2 - rr + rr * math.cos(t * math.pi / 12), ztop - rr + rr * math.sin(t * math.pi / 12)) for t in range(0, 7)]
    q2 += [(Xm - w2 + rr + rr * math.cos(t * math.pi / 12), ztop - rr + rr * math.sin(t * math.pi / 12)) for t in range(6, 13)]
    q = [(x, 0.0, z) for x, z in q2]
    # xz prism must be centred on the plane: cut a symmetric band Y -(yo+2)..(yo+2) clipped by a keep box
    # -> instead build the cutter as a body, move it, then combine (cut) into both shells
    cutter = prism(fr, "xz", q, yo - 4.5, yo + 2.0, "magnet slot cutter")
    cb = fr.bRepBodies.item(fr.bRepBodies.count - 1)
    tb = adsk.fusion.TemporaryBRepManager.get()
    import adsk.fusion
    cf = fr.features.combineFeatures
    tools = adsk.core.ObjectCollection.create(); tools.add(cb)
    ci = cf.createInput(fb, tools); ci.operation = adsk.fusion.FeatureOperations.CutFeatureOperation; ci.isKeepToolBodies = True
    c1 = cf.add(ci); c1.name = "GRIND magnet_slot (front shell)"
    # the back cover lies in another component: copy the cutter there
    tcopy = tb.copy(cb)
    bf = bk.features.baseFeatures.add(); bf.startEdit()
    try:
        bk.bRepBodies.add(tcopy, bf)
    finally:
        bf.finishEdit()
    bf.name = "magnet slot cutter (copy)"
    cb2 = bk.bRepBodies.item(bk.bRepBodies.count - 1)
    cb2.name = "magnet slot cutter (copy)"
    tools2 = adsk.core.ObjectCollection.create(); tools2.add(cb2)
    ci2 = bk.features.combineFeatures.createInput(bb, tools2); ci2.operation = adsk.fusion.FeatureOperations.CutFeatureOperation
    ci2.isKeepToolBodies = False
    c2 = bk.features.combineFeatures.add(ci2); c2.name = "GRIND magnet_slot (back cover lip)"
    cb.isLightBulbOn = False
    cb.name = "magnet slot cutter (hidden)"
    feats["magnet_slot"] = c1
    # 6 antenna relief: the inner rib of the screen-section wall on the ESP32 side (front +X = KiCad low x), removed
    # between ANT_RELIEF_K from the rim down to the stubs' level (the stubs/hooks above the board plane are kept)
    ya, yb = sorted((CY - ANT_RELIEF_K[0], CY - ANT_RELIEF_K[1]))
    xs = [R.x_at(g["Vf"], y, +1) - R.WALL_SCREEN for y in (ya, yb)]          # inner face of the inner rib (front X)
    x_in, x_out = min(xs) - ANT_RELIEF_EXTRA, max(xs) + R.INNER_RIB_T + ANT_RELIEF_EXTRA
    f = xy_prism(fr, [(x_in, ya), (x_out, ya), (x_out, yb), (x_in, yb)], R.SEAM_Z - 0.01, R.STUB_Z0,
                 "GRIND antenna_relief", "cut", [fb])
    feats["antenna_relief"] = f
    # 6b pins + hooks snipped flush with the inner rib, both walls (rev G): one cut of all the pin/hook prisms
    import adsk.fusion as _af
    sk = new_sketch(fr, "GRIND pins_hooks")
    for q in g["pins"] + g["hooks"]:
        R.poly(sk, q)
    f = R.extrude(fr, R.profiles(sk), R.SEAM_Z, R.Z_PLATE_TOP - 0.05 - R.SEAM_Z, "cut", [fb], "GRIND pins_hooks")
    f.name = "GRIND pins_hooks"
    feats["pins_hooks"] = f
    # floor check: thinnest remaining floor in each back-cover zone
    save_info(grind={k: v.name for k, v in feats.items()}, magnet_slot_Y_outer=yo,
              floor_after=R.BACK_PLATE, min_floor=MIN_FLOOR)
    print("grind ok:", list(feats))

def stage_regrind6(g):
    """Replace the zone-6 cut (GRIND antenna_relief) in place after ANT_RELIEF_K changed."""
    import adsk.core, adsk.fusion
    fr = comp_named("Front shell")
    fb = R.body_named(fr, "Front shell")
    tl = design.timeline
    for i in range(tl.count - 1, -1, -1):
        it = tl.item(i)
        try:
            nm = it.entity.name
        except Exception:
            nm = it.name or ""
        if nm.startswith("GRIND antenna_relief"):
            it.entity.deleteMe()
    for skt in list(fr.sketches):
        if skt.name.startswith("GRIND antenna_relief"):
            skt.deleteMe()
    ya, yb = sorted((CY - ANT_RELIEF_K[0], CY - ANT_RELIEF_K[1]))
    xs = [R.x_at(g["Vf"], y, +1) - R.WALL_SCREEN for y in (ya, yb)]
    x_in, x_out = min(xs) - ANT_RELIEF_EXTRA, max(xs) + R.INNER_RIB_T + ANT_RELIEF_EXTRA
    f = xy_prism(fr, [(x_in, ya), (x_out, ya), (x_out, yb), (x_in, yb)], R.SEAM_Z - 0.01, R.STUB_Z0,
                 "GRIND antenna_relief", "cut", [fb])
    print("zone 6 re-cut: KiCad y %.1f-%.1f, front X %.2f-%.2f, Z %.2f-%.2f" % (ANT_RELIEF_K[0], ANT_RELIEF_K[1], x_in, x_out, R.SEAM_Z - 0.01, R.STUB_Z0))

def set_grind(keys, suppressed):
    """Suppress (before) / unsuppress (after) the grind features whose name contains a key."""
    n = 0
    tl = design.timeline
    for i in range(tl.count):
        it = tl.item(i)
        nm = it.name or ""
        try:
            nm = it.entity.name
        except Exception:
            pass
        if nm.startswith("GRIND ") and any(nm[6:].startswith(k) for k in keys):
            it.isSuppressed = suppressed
            n += 1
    return n

# ── PCB ──
PCB_NAME = "PCB (ai_calc_board.step)"

def stage_pcb(g):
    import adsk.core, adsk.fusion
    info = R.board_info()
    root = design.rootComponent
    for occ in list(root.occurrences):
        if occ.component.name == PREFIX + PCB_NAME:
            occ.deleteMe()
    occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
    occ.component.name = PREFIX + PCB_NAME
    t0 = time.time()
    im = app.importManager
    im.importToTarget(im.createSTEPImportOptions(STEP), occ.component)
    log("pcb: STEP imported in %.0f s" % (time.time() - t0))
    pcb = bodies_under(occ)
    span = lambda b: (b.boundingBox.maxPoint.x - b.boundingBox.minPoint.x) * (b.boundingBox.maxPoint.y - b.boundingBox.minPoint.y)
    board = max(pcb, key=span)
    bb = board.boundingBox
    xs0, xs1, ys0, ys1 = bb.minPoint.x * 10, bb.maxPoint.x * 10, bb.minPoint.y * 10, bb.maxPoint.y * 10
    zs0, zs1 = bb.minPoint.z * 10, bb.maxPoint.z * 10
    kx0, kx1, ky0, ky1 = info["bbox"]
    ox, oy = xs0 - kx0, ys1 + ky0
    size_err = max(abs((xs1 - xs0) - (kx1 - kx0)), abs((ys1 - ys0) - (ky1 - ky0)))
    if size_err > 0.05:
        ox, oy = 0.0, 0.0
        log("pcb: WARNING STEP board size differs from Edge.Cuts by %.2f: offset forced to 0" % size_err)
    m = adsk.core.Matrix3D.create()        # 180 deg about Y: F.Cu (STEP +z) faces the back cover, F face at Z_F
    m.setWithArray([-1, 0, 0, cm(CX + ox), 0, 1, 0, cm(CY - oy), 0, 0, -1, cm(Z_F + zs1), 0, 0, 0, 1])
    occ.transform2 = m
    try:
        design.snapshots.add()
    except Exception:
        pass
    adsk.doEvents()
    # name the STEP parts after their KiCad references (nearest footprint with the same model)
    stem = lambda f: os.path.splitext(f)[0].lower()
    refs = {}
    lv = [c for c in leaves(occ) if board not in list(c.bRepBodies)]
    pairs = []
    for c in lv:
        comp = c.component.name.split(" (")[0].lower()
        b = c.boundingBox
        X, Y = (b.minPoint.x + b.maxPoint.x) * 5, (b.minPoint.y + b.maxPoint.y) * 5
        for fp in info["fps"]:
            if not fp["models"]:
                continue
            d = math.dist((X, Y), F(fp["x"], fp["y"]))
            same = any(stem(mm) == comp or (len(comp) > 8 and stem(mm).startswith(comp)) for mm in fp["models"])
            if (same and d < 15) or d < 4:
                pairs.append((0 if same else 1, d, c.fullPathName, fp["ref"]))
    used_l, used_r = set(), set()
    for same, d, lk, ref in sorted(pairs):            # model-name matches first, then nearest
        if lk in used_l or ref in used_r:
            continue
        used_l.add(lk); used_r.add(ref); refs[lk] = ref
    parts = {}
    for c in lv:
        b = c.boundingBox
        parts[refs.get(c.fullPathName, c.component.name)] = [b.minPoint.x * 10, b.maxPoint.x * 10, b.minPoint.y * 10, b.maxPoint.y * 10,
                                           b.minPoint.z * 10, b.maxPoint.z * 10, c.name]
    save_info(pcb_transform=list(m.asArray()))
    save_info(step=dict(path=STEP, mtime=os.path.getmtime(STEP), mtime_s=time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(os.path.getmtime(STEP))),
                        thickness=zs1 - zs0, ox=ox, oy=oy, size_err=size_err, n_bodies=len(pcb)),
              parts=parts, refs=refs,
              fps={fp["ref"]: dict(x=fp["x"], y=fp["y"], rot=fp["rot"], side=fp["side"], name=fp["name"]) for fp in info["fps"]},
              slots=[s for s in info["segs"]], bbox=info["bbox"], keepouts=info["keepouts"])
    print("pcb ok: %d bodies, board %.2f thick, F face at Z %.2f" % (len(pcb), zs1 - zs0, Z_F))

# ── Off-board parts ──
def clear_comp(name):
    for occ in list(design.rootComponent.occurrences):
        if occ.component.name == PREFIX + name:
            occ.deleteMe()
    return comp_named(name, True)

def fit_bow(bow, j5_zmid, tail_len):
    """Bow bottom Z by bisection so that the modelled tail (glass edge -> tip in J5) = tail_len."""
    lo_, hi_ = 1.3, j5_zmid - 0.2
    for _ in range(50):
        zl = (lo_ + hi_) / 2
        if path_len(bow(zl)[0]) - LCD_BOND < tail_len:
            hi_ = zl                                    # too short: go deeper
        else:
            lo_ = zl
    path, r_up = bow(zl)
    return zl, path, r_up

def stage_parts(g):
    import adsk.core, adsk.fusion
    I = load_info()
    parts = I.get("parts", {})
    fps = I.get("fps", {})
    notes = []

    # ---------- v15: 1.9" IPS LCD (key side, landscape, display towards the window) ----------
    lp_ = clear_comp("LCD panel")
    T = LCD_T
    yc = CY - LCD_AA_K[1]
    rect = lambda x0k, x1k, h, inset=0.0: [(150 - x1k + inset, yc - h / 2 + inset), (150 - x0k - inset, yc - h / 2 + inset),
                                           (150 - x0k - inset, yc + h / 2 - inset), (150 - x1k + inset, yc + h / 2 - inset)]
    z0 = Z_B + T["tape"]
    z_bl, z_tft, z_cf, z_pol = z0 + T["bl"], z0 + T["bl"] + T["tft"], z0 + T["bl"] + T["tft"] + T["cf"], z0 + sum(T[k] for k in ("bl", "tft", "cf", "pol"))
    xy_prism(lp_, rect(*LCD_BL_X, LCD_BL_H), z0, z_bl, "LCD backlight (frame + LED light guide)")
    xy_prism(lp_, rect(*LCD_GL_X, LCD_GL_H), z_bl, z_tft, "LCD TFT glass (with driver ledge)")
    cf_x1 = LCD_GL_X[1] - LCD_LEDGE
    xy_prism(lp_, rect(LCD_GL_X[0], cf_x1, LCD_GL_H), z_tft, z_cf, "LCD CF glass")
    xy_prism(lp_, rect(LCD_GL_X[0] + 0.3, cf_x1 - 0.3, LCD_GL_H - 0.6), z_cf, z_pol, "LCD top polarizer")
    ax0, ax1 = LCD_AA_K[0] - LCD_AA[0] / 2, LCD_AA_K[0] + LCD_AA[0] / 2
    xy_prism(lp_, rect(ax0, ax1, LCD_AA[1]), z_pol, z_pol + 0.02, "LCD active area (marker)")
    xy_prism(lp_, rect(cf_x1 + 0.15, cf_x1 + 1.05, 16.0), z_tft, z_tft + 0.30, "LCD driver IC (COG) + sealant")
    # FPC: bonded on the ledge, out past the glass/backlight end on the key side, 90 deg down through the board slot,
    # ONE gentle bow under the board (towards the back cover) and up into J5's mouth (J5 on the component side, mouth facing
    # the slot). Review 13 route: slot x 181.0, J5 mouth x 160.53 -> ~29.8 mm used, ~7 mm spare taken up by the bow. No Z-fold.
    rb = clear_comp("Ribbons and wires")
    slot_X = 150 - LCD_SLOT_XK
    j5x0, j5x1, j5y0, j5y1 = J5_BOX_K
    j5_face = 150 - j5x1                                # mouth (front-view X)
    j5p = parts.get("J5")                               # J5's STEP model (front-view box) if the board STEP has it
    j5_zmid = (j5p[4] + j5p[5]) / 2 if j5p else Z_F - J5_H / 2
    zt = z_tft + LCD_FPC_T / 2
    xg = 150 - LCD_GL_X[1]                              # glass edge (front-view X)
    tip = j5_face + FPC_INSERT
    def bow(z_low):
        r_up = min(FPC_BOW_R, (j5_zmid - z_low) / 2.05)
        x_rise = tip - LCD_STIFF - r_up                 # the stiffener (4.5 mm) stays straight into J5
        pts = [(xg + LCD_BOND, zt), (slot_X, zt), (slot_X, z_low), (x_rise, z_low), (x_rise, j5_zmid), (tip, j5_zmid)]
        return round_path_r(pts, [FPC_BEND_R, FPC_BOW_R, r_up, r_up], 12), r_up
    tail_len = float(ARGS.get("tail", LCD_FPC_LEN))
    zl, path, r_up = fit_bow(bow, j5_zmid, tail_len)
    yt = yc - LCD_FPC_DY                                # front-view Y of the tail centre (KiCad y 93.1 + 0.5)
    prism(rb, "xz", [(x, 0.0, z) for x, z in band(path, LCD_FPC_T)], yt - LCD_FPC_W / 2, yt + LCD_FPC_W / 2, "LCD FPC")
    fpc_len = path_len(path) - LCD_BOND
    direct = [(xg, zt), (slot_X, zt), (slot_X, j5_zmid), (tip, j5_zmid)]
    direct_len = path_len(round_path(direct, 0.4, 10))
    notes.append("LCD FPC modelled as one bow: down the slot at front X %.2f, bottom at Z %.2f (outer face %.2f), up into J5 at Z %.2f; "
                 "bow corner radii %.1f / %.1f / %.1f (top bend %.1f); glass edge -> %.1f inside J5 = %.1f mm; shortest route %.1f; tail %.1f"
                 % (slot_X, zl, zl - LCD_FPC_T / 2, j5_zmid, FPC_BOW_R, r_up, r_up, FPC_BEND_R, FPC_INSERT, fpc_len, direct_len, tail_len))
    save_info(lcd=dict(z_bottom=z0, z_backlight_top=z_bl, z_tft_top=z_tft, z_top=z_pol, fpc_len=round(fpc_len, 2), fpc_direct=round(direct_len, 2),
                       fpc_tail=tail_len, fpc_dy=LCD_FPC_DY, bow_Z=round(zl, 2), bow_Z_outer=round(zl - LCD_FPC_T / 2, 2), bow_r=[FPC_BOW_R, round(r_up, 2), round(r_up, 2)],
                       top_bend_r=FPC_BEND_R, slot_X=slot_X, j5_face_X=j5_face, j5_zmid=round(j5_zmid, 3), j5_step=j5p,
                       fpc_path=[[round(a, 3), round(b, 3)] for a, b in path]))

    # ---------- camera (component side, lens towards the back-cover window) ----------
    cm_ = clear_comp("Camera module")
    cc = F(*CAM_K)
    hx, hy = CAM[0] / 2, CAM[1] / 2
    zt0 = Z_F - CAM_TAPE                                # module base on thin tape
    sq = [(cc[0] - hx, cc[1] - hy), (cc[0] + hx, cc[1] - hy), (cc[0] + hx, cc[1] + hy), (cc[0] - hx, cc[1] + hy)]
    xy_prism(cm_, sq, zt0 - 1.0, zt0, "Camera sensor board")
    sq2 = [(cc[0] - hx + 0.15, cc[1] - hy + 0.15), (cc[0] + hx - 0.15, cc[1] - hy + 0.15), (cc[0] + hx - 0.15, cc[1] + hy - 0.15), (cc[0] - hx + 0.15, cc[1] + hy - 0.15)]
    xy_prism(cm_, sq2, zt0 - CAM[2] + CAM_LENS_H, zt0 - 1.0, "Camera AF motor (VCM)")
    xy_circle(cm_, cc, CAM_LENS_D, zt0 - CAM[2], zt0 - CAM[2] + CAM_LENS_H, "Camera lens barrel")
    xy_circle(cm_, cc, CAM_APERTURE, zt0 - CAM[2] - 0.01, zt0 - CAM[2] + 0.3, "Camera lens glass")
    # ribbon to J1, lying on the tallest part in its corridor
    j1 = parts.get("J1")
    j1_face = j1[3] if j1 else F(150, 157.45)[1]        # J1 edge nearest the camera (front view Y max)
    j1_zmid = ((j1[4] + j1[5]) / 2) if j1 else Z_F - 1.0
    lo_z = Z_F
    for ref, b in parts.items():                        # parts under the ribbon corridor (component side only)
        if b[1] > cc[0] - CAM_FPC_W2 / 2 and b[0] < cc[0] + CAM_FPC_W2 / 2 and b[3] > j1_face + 1 and b[2] < cc[1] - hy - 0.5 \
                and b[5] <= Z_F + 0.05 and ref not in ("J1",):
            lo_z = min(lo_z, b[4])
    zr = lo_z - FPC_T / 2 - 0.05
    ys = cc[1] - hy
    path = [(ys - 0.02, zt0 - 0.5), (ys - 1.5, zt0 - 0.5), (ys - 3.5, zr), (j1_face + 4.0, zr), (j1_face + 1.5, j1_zmid), (j1_face - FPC_INSERT, j1_zmid)]
    path = round_path(path, 1.0)
    fpc_pts = band(path, FPC_T)
    prism(rb, "yz", [(0.0, y, z) for y, z in fpc_pts], cc[0] - CAM_FPC_W / 2, cc[0] + CAM_FPC_W / 2, "Camera FPC")
    cl = path_len(path)
    notes.append("camera FPC: path module -> into J1 %.1f mm vs %.1f cable: %.1f mm slack lies in a loop "
                 "(not modelled); ribbon modelled %.2f mm under the board (tallest part in its path)" % (cl, CAM_FPC_LEN, CAM_FPC_LEN - cl, Z_F - zr))

    # ---------- LiPo + leads + JST-PH plug in J4 ----------
    lp = clear_comp("LiPo battery")
    lc = F(*LIPO_K)
    lz0, lz1 = zrel(LIPO_ZREL[1]), zrel(LIPO_ZREL[0])
    sk = new_sketch(lp, "lipo")
    R.rrect(sk, lc, LIPO[0], LIPO[1], 1.0)
    R.extrude(lp, R.profiles(sk), lz0, lz1 - lz0, name="LiPo pouch (Adafruit #1317)")
    j4 = parts.get("J4")
    if j4:
        # the PH socket's opening faces the mounting-tab side (+y KiCad = -Y front view; checked on the STEP
        # model in a section): the plug sits in front of the body's -Y face, its leads leave towards -Y
        x0_, x1_, y0_, y1_, z0_, z1_ = j4[:6]
        xm, zm = (x0_ + x1_) / 2, (z0_ + z1_) / 2
        yface = y0_
        pw, ph, pl = PH_PLUG
        xy_prism(lp, [(xm - pw / 2, yface - pl), (xm + pw / 2, yface - pl), (xm + pw / 2, yface - 0.05), (xm - pw / 2, yface - 0.05)],
                 zm - ph / 2, zm + ph / 2, "JST-PH plug (in J4)")
        # leads: out of the plug (-Y), under the board to the bay, up past the board edge, into the LiPo end
        lend = lc[0] + LIPO[0] / 2                       # LiPo end nearest J4 (front-view X max)
        bay_X = lend + LIPO_WIRE_D / 2 + 0.05            # wire centre just in front of the LiPo end
        lzm = (lz0 + lz1) / 2
        for nm, dxw, dz in (("red", 1.0, 0.7), ("black", -1.0, -0.7)):
            yl = lc[1] + dxw * 1.6
            pts = [(xm + dxw, yface - pl - 0.03, zm + dz * 0.0), (xm + dxw, yface - pl - 2.0 - (dxw + 1), zm + dz),
                   (bay_X, yface - pl - 2.0 - (dxw + 1), zm + dz), (bay_X, yl, zm + dz), (bay_X, yl, lzm + 0.8 * dz),
                   (lend + 0.01, yl, lzm + 0.8 * dz)]
            tbm_add(rb, tube(pts, LIPO_WIRE_D), "LiPo lead %s" % nm)
        notes.append("LiPo leads: plug at front-view X %.1f, LiPo end at X %.1f" % (xm, lend))
    else:
        notes.append("J4 not found in the STEP: no plug modelled")

    # ---------- magnet connector (device half) in the top-wall slot ----------
    mg = clear_comp("Magnet connector")
    Xm = 150 - MAG_XK
    zc = zrel(sum(MAG_ZREL) / 2)
    yo = R.y_at(g["Vf"], Xm, +1) - MAG_FLUSH
    fl, fh, ft = MAG_FACE
    prism(mg, "xz", [(x, 0.0, z) for x, z in obround_pts(Xm, zc, fl, fh)], yo - ft, yo, "Magnet face flange")
    bl, bh, bd = MAG_BODY
    yb = yo - ft
    for nm, shape, bw, bh, bdp in MAG_BODY:
        q2 = obround_pts(Xm, zc, bw, bh) if shape == "obround" else             [(Xm - bw / 2, zc - bh / 2), (Xm + bw / 2, zc - bh / 2), (Xm + bw / 2, zc + bh / 2), (Xm - bw / 2, zc + bh / 2)]
        prism(mg, "xz", [(x, 0.0, z) for x, z in q2], yb - bdp, yb + 0.01, "Magnet " + nm)
        yb -= bdp
    bd = yo - ft - yb
    tbs = []
    import adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    for k in range(4):
        x = Xm + (k - 1.5) * MAG_PITCH
        tbs.append(tbm.createCylinderOrCone(P(x, yo - ft - bd + 0.01, zc), cm(MAG_PIN_D / 2), P(x, yo - ft - bd - MAG_PIN_L, zc), cm(MAG_PIN_D / 2)))
    tbm_add(mg, tbs, "Magnet pins (straightened)")
    pads = [tbm.createCylinderOrCone(P(Xm + (k - 1.5) * MAG_PITCH, yo - 0.05, zc), cm(MAG_PAD_D / 2), P(Xm + (k - 1.5) * MAG_PITCH, yo + 0.01, zc), cm(MAG_PAD_D / 2)) for k in range(4)]
    tbm_add(mg, pads, "Magnet contact pads")
    # ---------- the cable's magnetic plug (#5412), detached ----------
    cp = clear_comp("Magnetic cable plug (detached)")
    y1 = yo + PLUG_GAP
    prism(cp, "xz", [(x, 0.0, z) for x, z in obround_pts(Xm, zc, fl, fh)], y1, y1 + 1.0, "Plug mating face")
    pw, ph, pl = PLUG_BODY
    prism(cp, "xz", [(x, 0.0, z) for x, z in obround_pts(Xm, zc, pw, ph)], y1 + 1.0, y1 + 1.0 + pl, "Plug overmould")
    t = [tbm.createCylinderOrCone(P(Xm, y1 + 1.0 + pl - 0.1, zc), cm(2.5), P(Xm, y1 + pl + 7, zc), cm(1.9)),
         tbm.createCylinderOrCone(P(Xm, y1 + pl + 6.9, zc), cm(1.75), P(Xm, y1 + pl + 30, zc), cm(1.75))]
    tbm_add(cp, t, "Plug strain relief + cable")
    pg = [tbm.createCylinderOrCone(P(Xm + (k - 1.5) * MAG_PITCH, y1 - 0.6, zc), cm(0.5), P(Xm + (k - 1.5) * MAG_PITCH, y1 + 0.01, zc), cm(0.5)) for k in range(4)]
    tbm_add(cp, pg, "Plug pogo pins")
    header = parts.get("J3")
    if header:
        notes.append("magnet pins end at front-view Y %.2f; J3/header body Y %.2f..%.2f, Z %.2f..%.2f; pin axis Z %.2f"
                     % (yo - ft - bd - MAG_PIN_L, header[2], header[3], header[4], header[5], zc))

    # ---------- screws (6, from the back) ----------
    sc = clear_comp("Screws")
    tb_ = []
    for n, c in g["screws"]:
        z_head = R.CBORE_H - SCREW["head_h"]
        tb_.append(tbm.createCylinderOrCone(P(c[0], c[1], z_head), cm(SCREW["d_head"] / 2), P(c[0], c[1], R.CBORE_H), cm(SCREW["d_head"] / 2)))
        tb_.append(tbm.createCylinderOrCone(P(c[0], c[1], R.CBORE_H - 0.01), cm(SCREW["d_core"] / 2),
                                            P(c[0], c[1], R.CBORE_H + SCREW["length"] - 0.4), cm(SCREW["d_core"] / 2)))
        tb_.append(tbm.createCylinderOrCone(P(c[0], c[1], R.CBORE_H + SCREW["length"] - 0.41), cm(SCREW["d_core"] / 2),
                                            P(c[0], c[1], R.CBORE_H + SCREW["length"]), cm(0.4)))
    for i in range(0, len(tb_), 3):
        tbm_add(sc, tb_[i:i + 3], "Screw %s" % g["screws"][i // 3][0])
    save_info(part_notes=notes, magnet=dict(X=Xm, Zc=zc, face_Y=yo, pins_end_Y=yo - ft - bd - MAG_PIN_L))
    stage_looks(g)
    print("parts ok\n" + "\n".join(notes))

def stage_looks(g):
    wire_replica()
    R.stage_looks(g)
    for cname, look in LOOK.items():
        try:
            comp = comp_named(cname)
        except KeyError:
            continue
        for b in comp.bRepBodies:
            nm = b.name
            lk = look
            if "active area" in nm: lk = SCREEN_LOOK
            elif "backlight" in nm: lk = "Plastic - Matte (White)"
            elif "TFT glass" in nm or "CF glass" in nm: lk = "Glass - Light Color"
            elif "polarizer" in nm: lk = "Paint - Enamel Glossy (Black)"
            elif "PROXY" in nm: lk = "Plastic - Matte (Black)"
            elif "film" in nm: lk = "Plastic - Matte (White)"
            elif "bottom glass" in nm: lk = "Glass - Light Color"
            elif "driver" in nm: lk = "Plastic - Matte (Black)"
            elif "FPC" in nm: lk = "Polymide (Kapton)"
            elif "lead red" in nm: lk = "Plastic - Glossy (Red)"
            elif "lead black" in nm: lk = "Plastic - Glossy (Black)"
            elif "JST" in nm: lk = "Plastic - Matte (White)"
            elif "pins" in nm or "pads" in nm or "pogo" in nm: lk = "Gold - Polished"
            elif "lens glass" in nm: lk = "Glass - Heavy Color (Blue)"
            elif "sensor board" in nm: lk = "Plastic - Matte (Green)"
            elif "cutter" in nm: continue
            appearance(b, lk)

# ── Window mask (black sticker inside the lens) ──
def mask_geom(g):
    """Lens outline (= mask outline) and the opening over the LCD active area, front-view mm."""
    wc = tuple(g["win_c"])
    lw, lh = R.WIN[0] + 2 * R.BEZEL_MARGIN - 0.3, R.WIN[1] + 2 * R.BEZEL_MARGIN - 0.3
    lr = R.WIN_R + R.BEZEL_MARGIN - 0.15
    ac = F(*LCD_AA_K)
    ow, oh = MASK_OPEN
    return dict(lens_c=wc, lens_w=lw, lens_h=lh, lens_r=lr, open_c=ac, open_w=ow, open_h=oh, open_r=MASK_R,
                z0=R.T_BODY - R.BEZEL_DEPTH + 0.01, t=MASK_T)

def stage_mask(g):
    m = mask_geom(g)
    mk = clear_comp("Window mask")
    sk = new_sketch(mk, "mask")
    R.rrect(sk, m["lens_c"], m["lens_w"], m["lens_h"], m["lens_r"])
    R.rrect(sk, m["open_c"], m["open_w"], m["open_h"], m["open_r"])
    R.extrude(mk, R.profiles(sk, 2), m["z0"], MASK_T, name="Window mask (black vinyl, inside the lens)")
    ln = comp_named("Window lens")
    lb = list(ln.bRepBodies)
    zmin = min(b.boundingBox.minPoint.z for b in lb) * 10
    lift = m["z0"] + MASK_T + 0.005 - zmin
    if lift > 0.001:                                   # the sticker lifts the lens by its thickness
        move_body(ln, lb, dz=lift)
    # panel top (e-paper film) vs mask underside
    ep = comp_named("LCD panel")
    ztop = max(b.boundingBox.maxPoint.z for b in ep.bRepBodies) * 10
    save_info(mask=dict(m, lens_lift=round(max(lift, 0), 3), panel_top_Z=round(ztop, 3), gap_to_panel=round(m["z0"] - ztop, 3)))
    stage_looks(g)
    print("mask ok: opening %.2f x %.2f at (%.3f, %.3f); lens lifted %.3f; gap mask -> panel %.2f" % (
        m["open_w"], m["open_h"], m["open_c"][0], m["open_c"][1], max(lift, 0), m["z0"] - ztop))

# ── Slide case (separate navy part; informational for now) ──
CASE_COMPS = ("Slide case (in use)", "Slide case (stored)")

def stage_case(g):
    """Both case positions as separate components (in use = on the back, shown; stored = over the front, hidden)
    plus the camera hole (CASE MOD camera_hole) in the case body."""
    out = {}
    cam = F(*CAM_K)
    for cname, pose in (("Slide case (in use)", "in_use"), ("Slide case (stored)", "stored")):
        cc = clear_comp(cname)
        cb = R.build_slide_case(cc, g, pose, name="Slide case")
        z0 = -5.0 if pose == "in_use" else 10.0
        f = xy_circle(cc, cam, CASE_CAM_HOLE_D, z0, z0 + 10.0, "CASE MOD camera_hole", "cut", [cb])
        bb = cb.boundingBox
        out[pose] = dict(bbox=[round(v * 10, 2) for v in (bb.minPoint.x, bb.minPoint.y, bb.minPoint.z, bb.maxPoint.x, bb.maxPoint.y, bb.maxPoint.z)])
    occ_named("Slide case (stored)").isLightBulbOn = False
    stage_looks(g)
    # numbers for slide_case_mods.md (stored pose = as built; the case is symmetric in X)
    ytop = g["case_ytop"]
    ybot_in = min(y for x, y in g["case_in"]); ybot_out = min(y for x, y in g["case_out"])
    a_out = R.W / 2 + R.CASE_CLR + R.CASE_WALL
    out["camera_hole"] = dict(d=CASE_CAM_HOLE_D, X=cam[0], Y=cam[1], from_open_end=round(ytop - cam[1], 2),
                              from_closed_end_inside=round(cam[1] - ybot_in, 2), from_closed_end_outside=round(cam[1] - ybot_out, 2),
                              from_each_long_edge=round(a_out - abs(cam[0]), 2), case_outer=[round(2 * a_out, 2), round(ytop - ybot_out, 2)])
    save_info(case=out)
    print("case ok", json.dumps(out))

def _case_vs_all(case_comp):
    """Overlaps between one case body and every other visible body (cases hidden except this one)."""
    import adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    for c in CASE_COMPS:
        occ_named(c).isLightBulbOn = (c == case_comp)
    B = all_bodies()
    cases = [t for t in B if t[0] == case_comp]
    hits = []
    for _, nc, bc in cases:
        for c, n, b in B:
            if c == case_comp or not bc.boundingBox.intersects(b.boundingBox):
                continue
            a = tbm.copy(bc); b2 = tbm.copy(b)
            if b.assemblyContext and abs(b2.boundingBox.minPoint.z - b.boundingBox.minPoint.z) > 1e-4:
                tbm.transform(b2, b.assemblyContext.transform2)
            try:
                tbm.booleanOperation(a, b2, adsk.fusion.BooleanTypes.IntersectionBooleanType)
            except Exception as e:
                hits.append(dict(part=[c, n], error=str(e)[:60])); continue
            v = sum(a.lumps.item(k).volume for k in range(a.lumps.count)) * 1000 if a and a.lumps.count else 0
            if v > 0.005:
                lb = a.boundingBox
                hits.append(dict(part=[c, n], vol=round(v, 3), lo=[round(lb.minPoint.x * 10, 2), round(lb.minPoint.y * 10, 2), round(lb.minPoint.z * 10, 2)],
                                 hi=[round(lb.maxPoint.x * 10, 2), round(lb.maxPoint.y * 10, 2), round(lb.maxPoint.z * 10, 2)]))
    # clearances to the parts that matter
    mm = app.measureManager
    gaps = {}
    for c, n, b in B:
        if c in ("Back cover", "Front shell", "Magnet connector", "Camera module", "Keycaps", "Window lens", "Rubber feet", "Magnetic cable plug (detached)"):
            for _, nc, bc in cases:
                try:
                    d = mm.measureMinimumDistance(bc, b).value * 10
                except Exception:
                    continue
                k = c if c != "Keycaps" else "Keycaps (closest)"
                gaps[k] = round(min(d, gaps.get(k, 1e9)), 3)
    return hits, gaps

def stage_case_check(g):
    res = {}
    vis = {c: occ_named(c).isLightBulbOn for c in CASE_COMPS}
    try:
        for c in CASE_COMPS:
            hits, gaps = _case_vs_all(c)
            res[c] = dict(hits=hits, min_gaps=gaps)
            log("case check %s: %d hits" % (c, len(hits)))
    finally:
        for c, v in vis.items():
            occ_named(c).isLightBulbOn = v
    # does the case floor cover the camera window / the magnet face? (projection tests, plain numbers)
    I = load_info()
    cam = F(*CAM_K)
    a_in = R.W / 2 + R.CASE_CLR
    res["camera_covered_without_hole"] = abs(cam[0]) < a_in and min(y for x, y in g["case_in"]) < cam[1] < g["case_ytop"]
    mg = I.get("magnet", {})
    res["magnet_face_Y"] = mg.get("face_Y"); res["case_open_end_Y"] = g["case_ytop"]
    res["magnet_blocked"] = bool(mg) and mg.get("face_Y", 0) < g["case_ytop"] - 0.01 and False   # open end at the top: nothing in front of the face
    res["thickness"] = dict(body_top=R.T_TOP, body_keys=R.T_KEY, key_tops=R.T_KEY + R.KEY_PROUD, feet=R.FOOT_PROUD,
                            in_use_total=round(R.T_KEY + R.KEY_PROUD + R.FOOT_PROUD + R.CASE_FLOOR, 2),
                            in_use_top_part=round(R.T_TOP + R.FOOT_PROUD + R.CASE_FLOOR, 2),
                            stored_total=round(R.T_KEY + R.KEY_PROUD + R.CASE_KEY_CLR + R.CASE_FLOOR + R.FOOT_PROUD, 2))
    json.dump(res, open(os.path.join(OUT, "case_fit.json"), "w"), indent=1)
    print(json.dumps(res, indent=1)[:3000])

# ── Interference check ──
EXPECTED = [("Ribbons and wires", "LCD FPC", "PCB", "J5", "ribbon end inside J5 (STEP model)"),
            ("Ribbons and wires", "Camera FPC", "PCB", "J1", "ribbon end at J1's opening")]

def all_bodies():
    out = []
    for occ in design.rootComponent.occurrences:
        cname = occ.component.name.replace(PREFIX, "").replace("Replica - ", "")
        if not occ.isLightBulbOn:
            continue
        if cname == PCB_NAME:
            I = load_info(); refs = I.get("refs", {})
            for c in leaves(occ):
                for b in c.bRepBodies:
                    out.append(("PCB", refs.get(c.fullPathName, "board" if c.fullPathName not in refs and b.boundingBox.maxPoint.x - b.boundingBox.minPoint.x > 5 and b.boundingBox.maxPoint.y - b.boundingBox.minPoint.y > 5 else c.component.name), b))
        else:
            for b in occ.bRepBodies:
                if not b.isLightBulbOn or "cutter" in b.name:
                    continue
                out.append((cname, b.name, b))
    return out

def stage_check(g):
    import adsk.core, adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    before = ARGS.get("grind") == "off"                # check the shell as it is BEFORE grinding
    if before:
        set_grind(GRINDS, True)
    vis = {}
    for c in CASE_COMPS:                               # the main check is the calculator without the case (stage case_check does the case)
        try:
            o = occ_named(c); vis[c] = o.isLightBulbOn; o.isLightBulbOn = False
        except KeyError:
            pass
    try:
        _check(before)
    finally:
        for c, v in vis.items():
            occ_named(c).isLightBulbOn = v
        if before:
            set_grind(GRINDS, False)

def _check(before):
    import adsk.core, adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    B = all_bodies()
    log("check: %d bodies" % len(B))
    def tcopy(b):
        t = tbm.copy(b)
        tb_, bb_ = t.boundingBox, b.boundingBox
        if abs(tb_.minPoint.x - bb_.minPoint.x) > 1e-4 or abs(tb_.minPoint.y - bb_.minPoint.y) > 1e-4 or abs(tb_.minPoint.z - bb_.minPoint.z) > 1e-4:
            ctx = b.assemblyContext
            m = adsk.core.Matrix3D.create()
            while ctx:
                m2 = ctx.transform2.copy() if hasattr(ctx, "transform2") else ctx.transform.copy()
                m.transformBy(m2) if False else None
                break
            tbm.transform(t, b.assemblyContext.transform2)
        return t
    hits, pairs, errs = [], 0, []
    t0 = time.time()
    copies = {}
    for i in range(len(B)):
        ci, ni, bi = B[i]
        for j in range(i + 1, len(B)):
            cj, nj, bj = B[j]
            if ci == cj and ci in ("PCB", "Keycaps"):
                continue
            if ci == cj and ci not in ("Ribbons and wires",):
                continue                                   # bodies of one part touch by design
            if not bi.boundingBox.intersects(bj.boundingBox):
                continue
            pairs += 1
            try:
                a = tcopy(bi); b2 = tcopy(bj)
                tbm.booleanOperation(a, b2, adsk.fusion.BooleanTypes.IntersectionBooleanType)
            except Exception as e:
                errs.append("%s/%s vs %s/%s: %s" % (ci, ni, cj, nj, str(e)[:60])); continue
            for k in range(a.lumps.count if a else 0):
                L = a.lumps.item(k)
                v = L.volume * 1000
                lb = L.boundingBox
                lo = [lb.minPoint.x * 10, lb.minPoint.y * 10, lb.minPoint.z * 10]
                hi = [lb.maxPoint.x * 10, lb.maxPoint.y * 10, lb.maxPoint.z * 10]
                depth = min(h - l for h, l in zip(hi, lo))
                if v < 0.005 or depth < 0.02:
                    continue
                hits.append(dict(a=[ci, ni], b=[cj, nj], vol=round(v, 3), lo=[round(x, 2) for x in lo], hi=[round(x, 2) for x in hi],
                                 depth=round(depth, 2)))
        if i % 25 == 0:
            log("check: %d/%d bodies, %d pairs, %d hits, %.0f s" % (i, len(B), pairs, len(hits), time.time() - t0))
    json.dump(dict(time=time.strftime("%Y-%m-%d %H:%M:%S"), bodies=len(B), pairs=pairs, hits=hits, errors=errs,
                   step=load_info().get("step")), open(os.path.join(OUT, "interference_before_grind.json" if before else "interference.json"), "w"), indent=1)
    print("check: %d bodies, %d pairs, %d overlaps, %d errors" % (len(B), pairs, len(hits), len(errs)))

CLEAR_PAIRS = [("Camera module", "*", "Back cover", "*"), ("PCB", "J4", "Back cover", "*"), ("PCB", "J3", "Back cover", "*"),
               ("PCB", "U1", "Back cover", "*"), ("Magnet connector", "*", "Back cover", "*"), ("Magnet connector", "*", "Front shell", "*"),
               ("Magnet connector", "*", "PCB", "board"), ("LiPo battery", "LiPo pouch (Adafruit #1317)", "Back cover", "*"),
               ("LiPo battery", "LiPo pouch (Adafruit #1317)", "PCB", "board"), ("LiPo battery", "JST-PH plug (in J4)", "Back cover", "*"),
               ("LCD panel", "*", "Front shell", "*"), ("LCD panel", "*", "Keymat", "*"), ("Ribbons and wires", "Camera FPC", "Back cover", "*"),
               ("Ribbons and wires", "LCD FPC", "Front shell", "*"), ("PCB", "board", "Front shell", "*"), ("PCB", "board", "Back cover", "*"),
               ("PCB", "*", "Back cover", "*"), ("Keymat", "*", "PCB", "board"),
               ("LiPo battery", "LiPo pouch (Adafruit #1317)", "Front shell", "*"), ("LiPo battery", "*", "Front shell", "*"),
               ("Ribbons and wires", "*", "Front shell", "*"), ("PCB", "*", "Front shell", "*"),
               ("LiPo battery", "LiPo pouch (Adafruit #1317)", "Screws", "*"), ("LiPo battery", "LiPo pouch (Adafruit #1317)", "PCB", "J4"),
               ("Ribbons and wires", "LiPo lead red", "Back cover", "*"), ("Ribbons and wires", "LiPo lead red", "PCB", "board"),
               ("PCB", "U1", "Front shell", "*"), ("PCB", "U1", "Keymat", "*"), ("PCB", "U1", "LCD panel", "*"),
               ("PCB", "U1", "Screws", "*"), ("PCB", "J5", "Back cover", "*"), ("PCB", "J1", "Back cover", "*"),
               ("Ribbons and wires", "LCD FPC", "Back cover", "*"), ("Ribbons and wires", "LCD FPC", "PCB", "*"), ("PCB", "D2", "Back cover", "*"),
               ("Window mask", "*", "LCD panel", "*"), ("Magnet connector", "*", "Keymat", "*"),
               ("LCD panel", "*", "Window lens", "*"), ("LCD panel", "*", "Solar cell (dummy)", "*"), ("LCD panel", "*", "Keycaps", "*"),
               ("LCD panel", "*", "PCB", "board"), ("LCD panel", "*", "Back cover", "*"), ("Ribbons and wires", "LCD FPC", "LCD panel", "*"),
               ("Ribbons and wires", "LCD FPC", "Camera module", "*"), ("Ribbons and wires", "LCD FPC", "Window mask", "*"),
               ("Ribbons and wires", "LCD FPC", "PCB", "board"), ("Ribbons and wires", "LCD FPC", "PCB", "J1"),
               ("Ribbons and wires", "LCD FPC", "LiPo battery", "*"), ("Ribbons and wires", "LCD FPC", "Screws", "*")]

# ── Full clearance table (verification/07): every board part and every loose part vs every shell feature ──
SHELL_COMPS = ["Front shell", "Back cover", "Keymat", "Keycaps", "Window lens", "Window mask", "Battery lid", "Screws",
               "Solar cell (dummy)", "Rubber feet"]
FULL_INTENDED = [("LiPo battery", "LiPo pouch", "Back cover", "rests on the floor (0.1 mm tape under it, not modelled)"),
                 ("Keymat", "*", "PCB", "mat lies on the key side of the board"),
                 ("PCB", "board", "Keymat", "mat lies on the key side of the board (0.11 in the model)"),
                 ("Magnet connector", "Magnet face", "Front shell", "face glued in the U-notch (0.25 per side, glue fills it)")]
FULL_LIMIT = 0.2

def stage_fulltable(g):
    """Minimum gap of every board body (by designator) and every loose part against every shell body, closed.
    Writes clearance_table_full.json/.md. FAIL = gap < FULL_LIMIT or overlap, unless the contact is intended."""
    import adsk.core, adsk.fusion
    B = all_bodies()
    mm = app.measureManager
    tbm = adsk.fusion.TemporaryBRepManager.get()
    shell = [(c, n, b) for c, n, b in B if c in SHELL_COMPS]
    parts = [(c, n, b) for c, n, b in B if c not in SHELL_COMPS and c not in CASE_COMPS and c != "Magnetic cable plug (detached)"]
    near = 3.0
    rows = []
    t0 = time.time()
    for i, (cp, npart, bp) in enumerate(parts):
        bb = bp.boundingBox
        for cs, ns, bs in shell:
            sb = bs.boundingBox
            if (bb.minPoint.x - sb.maxPoint.x) * 10 > near or (sb.minPoint.x - bb.maxPoint.x) * 10 > near or \
               (bb.minPoint.y - sb.maxPoint.y) * 10 > near or (sb.minPoint.y - bb.maxPoint.y) * 10 > near or \
               (bb.minPoint.z - sb.maxPoint.z) * 10 > near or (sb.minPoint.z - bb.maxPoint.z) * 10 > near:
                continue
            try:
                r = mm.measureMinimumDistance(bp, bs)
                d = r.value * 10
                p1 = r.positionOne
                at = [round(p1.x * 10, 2), round(p1.y * 10, 2), round(p1.z * 10, 2)]
            except Exception as e:
                rows.append(dict(part=[cp, npart], shell=[cs, ns], gap=None, err=str(e)[:60])); continue
            overlap = 0.0
            if d < 0.05:                                   # touching or overlapping: measure the overlap volume
                try:
                    a = tbm.copy(bp); b2 = tbm.copy(bs)
                    for t, src in ((a, bp), (b2, bs)):
                        if src.assemblyContext and abs(t.boundingBox.minPoint.z - src.boundingBox.minPoint.z) > 1e-4:
                            tbm.transform(t, src.assemblyContext.transform2)
                    tbm.booleanOperation(a, b2, adsk.fusion.BooleanTypes.IntersectionBooleanType)
                    overlap = sum(a.lumps.item(k).volume for k in range(a.lumps.count)) * 1000 if a and a.lumps.count else 0.0
                except Exception:
                    overlap = -1.0
            rows.append(dict(part=[cp, npart], shell=[cs, ns], gap=round(d, 3), overlap_mm3=round(overlap, 3), at=at))
        if i % 40 == 0:
            log("fulltable: %d/%d parts, %d rows, %.0f s" % (i, len(parts), len(rows), time.time() - t0))
    def verdict(r):
        if r.get("gap") is None:
            return "ERROR"
        for c, n, s, why in FULL_INTENDED:
            if r["part"][0] == c and (n == "*" or r["part"][1].startswith(n)) and r["shell"][0] == s:
                return "PASS (intended contact: %s)" % why
        if r.get("overlap_mm3", 0) > 0.005:
            return "FAIL (overlap %.2f mm3)" % r["overlap_mm3"]
        return "PASS" if r["gap"] >= FULL_LIMIT else "FAIL (< %.1f mm)" % FULL_LIMIT
    for r in rows:
        r["verdict"] = verdict(r)
    rows.sort(key=lambda r: (r.get("gap") if r.get("gap") is not None else -9))
    info = load_info()
    json.dump(dict(time=time.strftime("%Y-%m-%d %H:%M:%S"), limit=FULL_LIMIT, step=info.get("step"), rows=rows),
              open(os.path.join(OUT, "clearance_table_full.json"), "w"), indent=1)
    md = ["# Full clearance table: every part vs every shell feature (closed case, grinds applied)", "",
          "Generated %s by `build_final_assembly_v15.py fulltable`. Board STEP %s. Minimum distance per body pair (Fusion "
          "measureMinimumDistance; pairs whose boxes are more than %.0f mm apart are not listed). FAIL = gap under %.1f mm "
          "or an overlap, unless the contact is intended. Positions are front-view X, Y (mm from the board centre, display up) "
          "and Z from the outside of the back cover; KiCad x = 150 - X, y = %.2f - Y." % (
              time.strftime("%Y-%m-%d %H:%M"), (info.get("step") or {}).get("mtime_s", "?"), near, FULL_LIMIT, CY), "",
          "| Part | Body | Shell part | Shell body | Gap (mm) | At (front X, Y, Z) | KiCad x, y | Verdict |", "| --- | --- | --- | --- | --- | --- | --- | --- |"]
    for r in rows:
        g_ = "n/a" if r.get("gap") is None else "%.2f" % r["gap"]
        at = r.get("at") or [0, 0, 0]
        md.append("| %s | %s | %s | %s | %s | %.1f, %.1f, %.1f | %.1f, %.1f | %s |" % (
            r["part"][0], r["part"][1], r["shell"][0], r["shell"][1], g_, at[0], at[1], at[2], 150 - at[0], CY - at[1], r["verdict"]))
    fails = [r for r in rows if r["verdict"].startswith("FAIL")]
    md += ["", "**%d pairs checked, %d FAIL, %d PASS.**" % (len(rows), len(fails), len(rows) - len(fails))]
    open(os.path.join(OUT, "clearance_table_full.md"), "w", encoding="utf8").write("\n".join(md) + "\n")
    print("fulltable: %d parts x %d shell bodies -> %d rows, %d FAIL" % (len(parts), len(shell), len(rows), len(fails)))
    for r in fails[:20]:
        print("  FAIL", r["part"], r["shell"], r.get("gap"), r.get("overlap_mm3"), r.get("at"))

def stage_clearances(g):
    """Smallest gap between named parts (positive = clearance). Writes clearances.json."""
    B = all_bodies()
    mm = app.measureManager
    out = []
    for ca, na, cb, nb in CLEAR_PAIRS:
        A = [b for c, n, b in B if c == ca and (na == "*" or n == na)]
        Bb = [b for c, n, b in B if c == cb and (nb == "*" or n == nb)]
        best = None
        for a in A:
            for b in Bb:
                if a == b:
                    continue
                try:
                    r = mm.measureMinimumDistance(a, b)
                    d = r.value * 10
                    if best is None or d < best[0]:
                        p1, p2 = r.positionOne, r.positionTwo
                        best = (d, [round(p1.x * 10, 2), round(p1.y * 10, 2), round(p1.z * 10, 2)], a.name, b.name)
                except Exception as e:
                    pass
        out.append(dict(a=[ca, na], b=[cb, nb], gap=None if best is None else round(best[0], 2),
                        at=None if best is None else best[1], bodies=None if best is None else best[2:]))
        log("clear %s/%s - %s/%s: %s" % (ca, na, cb, nb, out[-1]["gap"]))
    json.dump(out, open(os.path.join(OUT, "clearances.json"), "w"), indent=1)
    print("\n".join("%s/%s - %s/%s: %s at %s" % (o["a"][0], o["a"][1], o["b"][0], o["b"][1], o["gap"], o["at"]) for o in out))

def stage_lipo_alt(g):
    """What-if: the LiPo pouch moved by dz (Z) and dx (front-view X); checks it against every other body,
    reports the smallest gaps, then puts it back. ARGS: dz=-5.4 dx=0"""
    import adsk.core, adsk.fusion
    dz, dx = float(ARGS.get("dz", -5.4)), float(ARGS.get("dx", 0))
    keep = [k for k in ARGS.get("unground", "").split(",") if k]     # grind zones to leave un-ground
    if keep:
        set_grind(keep, True)
    try:
        _lipo_alt(dz, dx)
    finally:
        if keep:
            set_grind(keep, False)

def _lipo_alt(dz, dx):
    import adsk.core, adsk.fusion
    occ = occ_named("LiPo battery")
    pouch = [b for b in occ.bRepBodies if b.name.startswith("LiPo pouch")][0]
    tbm = adsk.fusion.TemporaryBRepManager.get()
    t = tbm.copy(pouch)
    m = adsk.core.Matrix3D.create(); m.translation = adsk.core.Vector3D.create(cm(dx), cm(dz), 0)
    m.translation = adsk.core.Vector3D.create(cm(dx), 0, cm(dz))
    tbm.transform(t, m)
    out = []
    mm = app.measureManager
    for c, n, b in all_bodies():
        if c == "LiPo battery" and n.startswith("LiPo pouch"):
            continue
        bb = b.boundingBox
        if bb.minPoint.x * 10 > 150 - LIPO_K[0] + LIPO[0] / 2 + abs(dx) + 6 or bb.maxPoint.x * 10 < 150 - LIPO_K[0] - LIPO[0] / 2 - abs(dx) - 6:
            continue
        if bb.minPoint.y * 10 > CY - LIPO_K[1] + LIPO[1] / 2 + 6 or bb.maxPoint.y * 10 < CY - LIPO_K[1] - LIPO[1] / 2 - 6:
            continue
        try:
            a = tbm.copy(t); bc = tbm.copy(b)
            if b.assemblyContext and abs(bc.boundingBox.minPoint.z - bb.minPoint.z) > 1e-4:
                tbm.transform(bc, b.assemblyContext.transform2)
            tbm.booleanOperation(a, bc, adsk.fusion.BooleanTypes.IntersectionBooleanType)
            v = sum(a.lumps.item(k).volume for k in range(a.lumps.count)) * 1000 if a and a.lumps.count else 0
            lo = None
            if v > 0.005:
                lb = a.boundingBox
                lo = [round(lb.minPoint.z * 10, 2), round(lb.maxPoint.z * 10, 2)]
            out.append(dict(part=[c, n], overlap_mm3=round(v, 3), z=lo))
        except Exception as e:
            out.append(dict(part=[c, n], error=str(e)[:60]))
    hits = [o for o in out if o.get("overlap_mm3", 0) > 0.005]
    json.dump(dict(dz=dz, dx=dx, checked=len(out), hits=hits), open(os.path.join(OUT, "lipo_alt.json"), "w"), indent=1)
    print("lipo what-if dz=%g dx=%g: %d bodies near, %d overlaps" % (dz, dx, len(out), len(hits)))
    for h in hits:
        print("  ", h)

def stage_export(g):
    import adsk.fusion
    em = design.exportManager
    root = design.rootComponent
    em.execute(em.createFusionArchiveExportOptions(os.path.join(OUT, "ai_calculator_v15_lcd_final_assembly.f3d"), root))
    em.execute(em.createSTEPExportOptions(os.path.join(OUT, "ai_calculator_v15_lcd_final_assembly.step"), root))
    try:
        app.activeDocument.save("v15-LCD final assembly regenerated")
    except Exception as e:
        log("cloud save skipped: %s" % str(e)[:100])
    print("exported f3d + step")

# ── Renders ──
def set_vis(names, vis):
    for occ in design.rootComponent.occurrences:
        if occ.component.name.replace(PREFIX, "").replace("Replica - ", "") in names:
            occ.isLightBulbOn = vis

CASE_SHOWN = set()                                    # case components to show in renders (none by default)

def show_only(hide):
    for occ in design.rootComponent.occurrences:
        nm = occ.component.name.replace(PREFIX, "").replace("Replica - ", "")
        occ.isLightBulbOn = nm not in hide and (nm not in CASE_COMPS or nm in CASE_SHOWN)

def shoot(fname, eye, target, up=(0, 0, 1), ext=None, fit=False, view=None, w=1600, h=1200):
    import adsk.core
    vp = app.activeViewport
    cam = vp.camera
    if view:
        cam.viewOrientation = view
        cam.isFitView = True
    else:
        cam.isFitView = False
        cam.target = P(*target); cam.eye = P(*eye)
        cam.upVector = adsk.core.Vector3D.create(*up)
        if ext:
            cam.viewExtents = cm(ext)
    cam.isSmoothTransition = False
    vp.camera = cam
    if fit:
        vp.fit()
    adsk.doEvents(); vp.refresh(); adsk.doEvents()
    vp.saveAsImageFile(os.path.join(OUT, "renders", fname + ".png"), w, h)
    return fname

def hide_cutters():
    for comp in design.allComponents:
        for skt in comp.sketches:
            skt.isLightBulbOn = False
        for b in comp.bRepBodies:
            if "cutter" in b.name:
                b.isLightBulbOn = False
    try:
        for i in range(design.rootComponent.constructionPlanes.count):
            design.rootComponent.constructionPlanes.item(i).isLightBulbOn = False
    except Exception:
        pass

EXPLODE = {"Rubber feet": -42, "Screws": -58, "Battery lid": -38, "Back cover": -30, "Camera module": 0, "LiPo battery": 0,
           PCB_NAME: 0, "Ribbons and wires": 0, "Magnet connector": 0, "LCD panel": 12, "Keymat": 22, "Keycaps": 34,
           "Front shell": 50, "Solar cell (dummy)": 50, "Window mask": 57, "Window lens": 64, "Magnetic cable plug (detached)": 0,
           "Slide case (in use)": -80, "Slide case (stored)": 85}

def explode(on):
    import adsk.core
    base = load_info().get("pcb_transform")
    for occ in design.rootComponent.occurrences:
        nm = occ.component.name.replace(PREFIX, "").replace("Replica - ", "")
        dz = EXPLODE.get(nm, 0) if on else 0
        m = adsk.core.Matrix3D.create()
        if nm == PCB_NAME:
            if not base:
                continue
            m.setWithArray(base)
        t = m.translation; t.z += cm(dz); m.translation = t
        try:
            occ.isGrounded = False                    # Fusion grounds the first component: it would not move
        except Exception:
            pass
        occ.transform2 = m
    # no design.snapshots.add() here: it snaps the first (ground-to-parent) occurrence back

def section(plane, offset, flip=False):
    import adsk.core
    sa = design.analyses.sectionAnalyses
    root = design.rootComponent
    pl = {"xz": root.xZConstructionPlane, "yz": root.yZConstructionPlane, "xy": root.xYConstructionPlane}[plane]
    inp = sa.createInput(pl, cm(offset))
    inp.flip = flip
    inp.isHatchShown = True
    return sa.add(inp)

def stage_renders(g):
    import adsk.core
    I = load_info()
    if "pcb_transform" not in I:
        save_info(pcb_transform=list(occ_named(PCB_NAME).transform2.asArray()))
    os.makedirs(os.path.join(OUT, "renders"), exist_ok=True)
    hide_cutters()
    only = [s for s in ARGS.get("only", "").split(",") if s]
    want = lambda k: not only or any(k.startswith(o) for o in only)
    V = adsk.core.ViewOrientations
    done = []
    try:
        explode(False)
        show_only([])
        if want("hero"):
            done.append(shoot("hero_front", (95, -150, 140), (0, 0, 6), ext=190))
            done.append(shoot("hero_back", (-110, -130, -120), (0, 0, 6), ext=190))
            done.append(shoot("front", None, None, view=V.TopViewOrientation))
            done.append(shoot("back", None, None, view=V.BottomViewOrientation))
        if want("back_cover_off"):
            show_only(["Back cover", "Screws", "Rubber feet", "Battery lid", "Magnetic cable plug (detached)"])
            done.append(shoot("back_cover_off", (-80, -90, -150), (0, 10, 6), ext=180))
            done.append(shoot("back_cover_off_top", (0, 0, -300), (0, 0, 6), up=(0, 1, 0), ext=235))
            done.append(shoot("back_cover_off_closeup_top", (-40, 25, -110), (5, 62, 5), ext=70))
            show_only([])
        if want("exploded"):
            explode(True)
            done.append(shoot("exploded_front", (150, -190, 150), (0, 0, 8), ext=235))
            done.append(shoot("exploded_back", (-170, -150, -120), (0, 0, 0), ext=230))
            explode(False)
        if want("section"):
            cam = F(*CAM_K)
            j4 = I.get("parts", {}).get("J4")
            Xm = I.get("magnet", {}).get("X", 150 - MAG_XK)
            hide = ["Magnetic cable plug (detached)"]
            show_only(hide)
            cuts = [("section_camera", "xz", cam[1], (cam[0], cam[1] + 200, 4), (cam[0], cam[1], 5), 30),
                    ("section_camera_wide", "xz", cam[1], (0, cam[1] + 200, 6), (0, cam[1], 6), 62),
                    ("section_j4_battery", "xz", (j4[2] + j4[3]) / 2 if j4 else 64, (-8, 300, 6), (-8, 64, 6), 60),
                    ("section_battery_lengthwise", "yz", F(*LIPO_K)[0] + 4.0, (300, 66, 6), (0, 68, 6), 40),
                    ("section_magnet_slot", "yz", Xm, (300, 72, 6), (Xm, 74, 6), 30),
                    ("section_screen", "xz", 45.95 + 6.0, (0, 300, 6), (0, 52, 6), 62),
                    ("section_screen_fpc_end", "xz", 45.95 + 6.0, (-25, 300, 6), (-25, 52, 6), 22)]
            for name, pl, off, eye, tgt, ext in cuts:
                if not want(name) and not want("section"):
                    continue
                s = section(pl, off, flip=False)
                try:
                    done.append(shoot(name, eye, tgt, ext=ext))
                finally:
                    s.deleteMe()
            show_only([])
        if want("antenna"):                              # rev F: the ESP32 antenna tab vs the faceplate's inner rib
            u1 = I.get("parts", {}).get("U1")
            Xt = u1[1] if u1 else 35.26                   # tip of the module (front X max)
            Yc = ((u1[2] + u1[3]) / 2) if u1 else CY - 97.0
            show_only(["Magnetic cable plug (detached)"])
            s = section("xz", Yc, flip=False)            # cut across the module: tip vs rib, closed case
            try:
                done.append(shoot("section_antenna", (Xt - 2, Yc + 200, 6), (Xt - 2, Yc, 6), ext=14))
                done.append(shoot("section_antenna_wide", (Xt - 10, Yc + 200, 6), (Xt - 10, Yc, 6), ext=36))
            finally:
                s.deleteMe()
            show_only(["Back cover", "Screws", "Rubber feet", "Battery lid", "Magnetic cable plug (detached)"])
            done.append(shoot("antenna_corner", (Xt + 25, Yc - 30, -45), (Xt - 3, Yc, 6), ext=34))   # back cover off
            show_only([])
        if want("grind"):
            done += grind_renders(g)
    finally:
        explode(False)
        show_only([])
        hide_cutters()
        set_grind(GRINDS, False)
    print("renders:", done)

def stage_tailcase(g):
    """Worst-case tail (LCD_FPC_LEN_WORST = 36.9, or tail=..): a temporary FPC body with the same route, measured against the
    back cover (rib B) and every other body near the bow; result in tail_worstcase.json, then deleted."""
    import adsk.core, adsk.fusion
    I = load_info(); lcd = I["lcd"]
    j5_zmid = lcd["j5_zmid"]; slot_X = lcd["slot_X"]; tip = lcd["j5_face_X"] + FPC_INSERT
    zt = lcd["z_tft_top"] + LCD_FPC_T / 2
    xg = 150 - LCD_GL_X[1]
    def bow(z_low):
        r_up = min(FPC_BOW_R, (j5_zmid - z_low) / 2.05)
        x_rise = tip - LCD_STIFF - r_up
        pts = [(xg + LCD_BOND, zt), (slot_X, zt), (slot_X, z_low), (x_rise, z_low), (x_rise, j5_zmid), (tip, j5_zmid)]
        return round_path_r(pts, [FPC_BEND_R, FPC_BOW_R, r_up, r_up], 12), r_up
    yt = CY - LCD_AA_K[1] - LCD_FPC_DY
    out = {}
    mm = app.measureManager
    tbm = adsk.fusion.TemporaryBRepManager.get()
    lens = [float(x) for x in ARGS.get("tails", "36.3,36.6,36.9,37.2").split(",")]
    for L in lens:
        zl, path, r_up = fit_bow(bow, j5_zmid, L)
        pc = clear_comp("TAILCASE (temporary)")
        try:
            prism(pc, "xz", [(x, 0.0, z) for x, z in band(path, LCD_FPC_T)], yt - LCD_FPC_W / 2, yt + LCD_FPC_W / 2, "LCD FPC %.1f" % L)
            fb = list(occ_named("TAILCASE (temporary)").bRepBodies)[0]
            rows = []
            for c, n, b in all_bodies():
                if c in ("TAILCASE (temporary)", "Ribbons and wires", "LCD panel") or (c == "PCB" and n == "J5"):
                    continue
                bb, fbb = b.boundingBox, fb.boundingBox
                if (bb.minPoint.x - fbb.maxPoint.x) > 0.4 or (fbb.minPoint.x - bb.maxPoint.x) > 0.4 or (bb.minPoint.y - fbb.maxPoint.y) > 0.4                         or (fbb.minPoint.y - bb.maxPoint.y) > 0.4 or (bb.minPoint.z - fbb.maxPoint.z) > 0.4 or (fbb.minPoint.z - bb.maxPoint.z) > 0.4:
                    continue
                try:
                    r = mm.measureMinimumDistance(fb, b)
                    p1 = r.positionOne
                    d = r.value * 10
                    ov = 0.0
                    if d < 0.02:
                        a = tbm.copy(fb); b2 = tbm.copy(b)
                        if b.assemblyContext and abs(b2.boundingBox.minPoint.z - bb.minPoint.z) > 1e-4:
                            tbm.transform(b2, b.assemblyContext.transform2)
                        tbm.booleanOperation(a, b2, adsk.fusion.BooleanTypes.IntersectionBooleanType)
                        ov = sum(a.lumps.item(k).volume for k in range(a.lumps.count)) * 1000 if a and a.lumps.count else 0.0
                    rows.append(dict(part=[c, n], gap=round(d, 3), overlap_mm3=round(ov, 3), at=[round(p1.x * 10, 2), round(p1.y * 10, 2), round(p1.z * 10, 2)]))
                except Exception as e:
                    rows.append(dict(part=[c, n], err=str(e)[:60]))
            rows.sort(key=lambda r: r.get("gap", 99))
            out["%.1f" % L] = dict(bow_Z=round(zl, 3), bow_Z_outer=round(zl - LCD_FPC_T / 2, 3), r_up=round(r_up, 2), nearest=rows[:8])
            log("tailcase %.1f: bow outer Z %.3f, nearest %s" % (L, zl - LCD_FPC_T / 2, rows[:2]))
        finally:
            occ_named("TAILCASE (temporary)").deleteMe()
    json.dump(dict(time=time.strftime("%Y-%m-%d %H:%M:%S"), fpc_dy=LCD_FPC_DY, bow_r=FPC_BOW_R, cases=out),
              open(os.path.join(OUT, "tail_worstcase.json"), "w"), indent=1)
    for L, o in out.items():
        print(L, o["bow_Z_outer"], o["nearest"][:3])

def stage_probe(g):
    """In-plane gaps (v15): columns of the LCD / FPC footprints from the board's key face up to just under the plate,
    measured against every shell body. A column hits anything that hangs down beside the panel (solar frame, ribs, pins),
    so the number is the sideways room, not the (much larger) vertical gap. Temporary component, deleted afterwards."""
    yc = CY - LCD_AA_K[1]
    cols = {
        "LCD backlight column": ([(150 - LCD_BL_X[1], yc - LCD_BL_H / 2), (150 - LCD_BL_X[0], yc - LCD_BL_H / 2),
                                  (150 - LCD_BL_X[0], yc + LCD_BL_H / 2), (150 - LCD_BL_X[1], yc + LCD_BL_H / 2)], Z_B, R.Z_PLATE_TOP - 0.05),
        "LCD FPC key-side column": ([(150 - (LCD_SLOT_XK + 0.5), yc - LCD_FPC_W / 2), (150 - LCD_GL_X[1], yc - LCD_FPC_W / 2),
                                     (150 - LCD_GL_X[1], yc + LCD_FPC_W / 2), (150 - (LCD_SLOT_XK + 0.5), yc + LCD_FPC_W / 2)], Z_B, R.Z_PLATE_TOP - 0.05),
        "LCD footprint, Z 8.0-9.4 (side room)": ([(150 - LCD_BL_X[1], yc - LCD_BL_H / 2), (150 - LCD_BL_X[0], yc - LCD_BL_H / 2),
                                  (150 - LCD_BL_X[0], yc + LCD_BL_H / 2), (150 - LCD_BL_X[1], yc + LCD_BL_H / 2)], 8.0, 9.4),
        "v14 e-paper footprint, Z 8.0-9.4 (calibration)": ([(F(*EPD_K)[0] - EPD[0] / 2, F(*EPD_K)[1] - EPD[1] / 2), (F(*EPD_K)[0] + EPD[0] / 2, F(*EPD_K)[1] - EPD[1] / 2),
                                  (F(*EPD_K)[0] + EPD[0] / 2, F(*EPD_K)[1] + EPD[1] / 2), (F(*EPD_K)[0] - EPD[0] / 2, F(*EPD_K)[1] + EPD[1] / 2)], 8.0, 9.4),
        "LCD stack (real height)": ([(150 - LCD_BL_X[1], yc - LCD_BL_H / 2), (150 - LCD_BL_X[0], yc - LCD_BL_H / 2),
                                     (150 - LCD_BL_X[0], yc + LCD_BL_H / 2), (150 - LCD_BL_X[1], yc + LCD_BL_H / 2)], Z_B + LCD_T["tape"], Z_B + sum(LCD_T.values())),
    }
    pc = clear_comp("PROBE (temporary)")
    out = []
    mm = app.measureManager
    try:
        for nm, (pts, z0, z1) in cols.items():
            xy_prism(pc, pts, z0, z1, nm)
        shell = [(c, n, b) for c, n, b in all_bodies() if c in SHELL_COMPS]
        for b in occ_named("PROBE (temporary)").bRepBodies:
            for c, n, sb in shell:
                try:
                    r = mm.measureMinimumDistance(b, sb)
                    p = r.positionTwo
                    out.append(dict(probe=b.name, shell=[c, n], gap=round(r.value * 10, 3), at=[round(p.x * 10, 2), round(p.y * 10, 2), round(p.z * 10, 2)]))
                except Exception as e:
                    out.append(dict(probe=b.name, shell=[c, n], err=str(e)[:60]))
    finally:
        occ_named("PROBE (temporary)").deleteMe()
    out.sort(key=lambda o: (o["probe"], o.get("gap", 99)))
    json.dump(out, open(os.path.join(OUT, "probe_inplane.json"), "w"), indent=1)
    for o in out:
        if o.get("gap", 0) < 15:
            print(o)

def stage_v15renders(g):
    """v15 pictures: front with the screen as a chroma key (compose_v15.py pastes the mock UI), exploded, sections through
    the LCD (lengthwise = FPC + slot + loop + J5; across = height stack vs the window and the plate)."""
    import adsk.core
    global SCREEN_LOOK
    os.makedirs(os.path.join(OUT, "renders"), exist_ok=True)
    hide_cutters()
    only = [s for s in ARGS.get("only", "").split(",") if s]
    want = lambda k: not only or any(k.startswith(o) for o in only)
    I = load_info()
    yc = CY - LCD_AA_K[1]
    xc = 150 - LCD_AA_K[0]
    wc = g["win_c"]
    done = []
    try:
        explode(False)
        if want("front"):
            show_only(["Magnetic cable plug (detached)"])
            for state, look in (("off", "Paint - Enamel Glossy (Dark Grey)"), ("key", CHROMA)):
                SCREEN_LOOK = look; stage_looks(g); hide_cutters(); adsk.doEvents()
                done.append(shoot("front_screen_%s" % state, (0, 1.6, 300), (0, 1.6, 6), up=(0, 1, 0), ext=172))
                done.append(shoot("display_screen_%s" % state, (wc[0], wc[1], 300), (wc[0], wc[1], 10), up=(0, 1, 0), ext=78))
                done.append(shoot("hero_screen_%s" % state, (95, -150, 140), (0, 0, 6), ext=190))
            SCREEN_LOOK = "Paint - Enamel Glossy (Dark Grey)"; stage_looks(g); hide_cutters()
        if want("exploded"):
            explode(True); show_only(["Magnetic cable plug (detached)"])
            done.append(shoot("exploded_front", (150, -190, 150), (0, 0, 8), ext=235))
            done.append(shoot("exploded_back", (-170, -150, -120), (0, 0, 0), ext=230))
            explode(False)
            show_only(["Magnetic cable plug (detached)", "Back cover", "Screws", "Rubber feet", "Battery lid", "Front shell", "Keymat", "Keycaps",
                       "Window lens", "Window mask", "Solar cell (dummy)", "LiPo battery", "Magnet connector", "Camera module"])
            done.append(shoot("lcd_on_board_iso", (-60, -40, 70), (xc - 8, yc, 8), ext=75))
            done.append(shoot("lcd_fpc_slot_closeup", (-80, 25, 30), (150 - 178.0, yc, 7.0), ext=30))
            show_only([])
        if want("section"):
            show_only(["Magnetic cable plug (detached)"])
            cuts = [("section_lcd_lengthwise", "xz", yc, (xc, yc + 200, 6), (xc - 4, yc, 6), 66),
                    ("section_lcd_fpc_end", "xz", yc, (150 - 170.0, yc + 200, 5.5), (150 - 170.0, yc, 5.5), 30),
                    ("section_lcd_across", "yz", xc, (300, yc, 7), (xc, yc, 7), 40),
                    ("section_lcd_across_top", "yz", xc, (300, yc + 12, 9), (xc, yc + 12, 9), 10),
                    ("section_lcd_across_solar", "yz", xc + 12, (300, yc + 14, 8), (xc + 12, yc + 16, 8), 16)]
            for name, pl, off, eye, tgt, ext in cuts:
                s = section(pl, off, flip=False)
                try:
                    done.append(shoot(name, eye, tgt, ext=ext))
                finally:
                    s.deleteMe()
            show_only([])
    finally:
        explode(False); show_only([]); hide_cutters()
    print("renders:", done)

def stage_case_renders(g):
    """Slide case, window mask and photo-comparison pictures (renders/case_*, renders/mask_*, renders/compare/*_cad.png)."""
    import adsk.core
    V = adsk.core.ViewOrientations
    hide_cutters()
    only = [s for s in ARGS.get("only", "").split(",") if s]
    want = lambda k: not only or any(k.startswith(o) for o in only)
    done = []
    IU, ST = "Slide case (in use)", "Slide case (stored)"
    cam = F(*CAM_K)
    Xm = load_info().get("magnet", {}).get("X", 150 - MAG_XK)
    try:
        explode(False)
        if want("case"):
            CASE_SHOWN.clear(); CASE_SHOWN.add(IU); show_only([])
            done.append(shoot("case_in_use_front", (95, -150, 140), (0, 0, 6), ext=195))
            done.append(shoot("case_in_use_back", (-110, -130, -120), (0, 0, 6), ext=195))
            done.append(shoot("case_in_use_side", (300, 0, 6), (0, 0, 6), ext=175))
            done.append(shoot("case_camera_hole_closeup", (-35, 10, -80), (0, cam[1], -1), ext=34))
            show_only(["Magnetic cable plug (detached)"])
            s = section("xz", cam[1])
            try:
                done.append(shoot("case_camera_hole_section", (cam[0], cam[1] + 200, 2), (cam[0], cam[1], 2), ext=34))
            finally:
                s.deleteMe()
            show_only([])
            done.append(shoot("case_magnet_closeup_in_use", (Xm + 45, 150, 35), (Xm, 80, 4), ext=55))
            CASE_SHOWN.clear(); CASE_SHOWN.add(ST); show_only([])
            done.append(shoot("case_stored_front", (95, -150, 140), (0, 0, 6), ext=195))
            done.append(shoot("case_stored_back", (-110, -130, -120), (0, 0, 6), ext=195))
            done.append(shoot("case_magnet_closeup_stored", (Xm + 45, 150, 35), (Xm, 80, 6), ext=55))
            # exploded: the 3 parts (silver front shell, navy back shell, navy case) + what sits between them
            CASE_SHOWN.clear(); CASE_SHOWN.add(IU)
            explode(True)
            show_only(["Magnetic cable plug (detached)", "Screws", "Rubber feet", "Battery lid"])
            done.append(shoot("exploded_3_parts_and_case", (190, -170, 40), (0, 0, -5), ext=260))
            show_only([n for n in [PCB_NAME, "LCD panel", "Camera module", "LiPo battery", "Ribbons and wires", "Magnet connector",
                                   "Keymat", "Keycaps", "Window lens", "Window mask", "Solar cell (dummy)", "Magnetic cable plug (detached)",
                                   "Screws", "Rubber feet", "Battery lid"]])
            done.append(shoot("exploded_shells_only", (190, -170, 40), (0, 0, -5), ext=260))
            explode(False)
            CASE_SHOWN.clear(); show_only([])
        if want("mask"):
            CASE_SHOWN.clear()
            wc = g["win_c"]
            for state, hide in (("before", ["Window mask"]), ("after", [])):
                show_only(hide + ["Magnetic cable plug (detached)"])
                done.append(shoot("mask_%s_display" % state, (wc[0], wc[1], 300), (wc[0], wc[1], 10), up=(0, 1, 0), ext=78))
                done.append(shoot("mask_%s_front" % state, (0, -3, 300), (0, -3, 6), up=(0, 1, 0), ext=170))
                done.append(shoot("mask_%s_iso" % state, (60, -60, 110), (wc[0], wc[1], 9), ext=90))
            show_only([])
        if want("compare"):                            # CAD at the angles of Nirav's key photos
            os.makedirs(os.path.join(OUT, "renders", "compare"), exist_ok=True)
            CASE_SHOWN.clear(); show_only(["Magnetic cable plug (detached)"])
            done.append(shoot("compare/front_cad", (0, 0, 300), (0, 1.6, 6), up=(0, 1, 0), ext=172))
            done.append(shoot("compare/back_cad", (0, 0, -300), (0, 1.6, 6), up=(0, 1, 0), ext=172))
            done.append(shoot("compare/side_cad", (300, 0, 6), (0, 1.6, 6), up=(0, 0, 1), ext=172))
            done.append(shoot("compare/front_iso_left_cad", (-90, -120, 150), (0, 0, 6), ext=190))
            rest = [n for n in EXPLODE if n != "Front shell"] + ["Keycaps", "Keymat"]
            show_only([n for n in rest if n != "Front shell"])
            done.append(shoot("compare/front_shell_inside_cad", (0, 0, -300), (0, 1.6, 6), up=(0, 1, 0), ext=172))
            show_only([n for n in rest if n != "Back cover"] + ["Front shell"])
            done.append(shoot("compare/back_cover_inside_cad", (0, 0, 300), (0, 1.6, 6), up=(0, 1, 0), ext=172))
            done.append(shoot("compare/back_cover_inside_persp_cad", (0, -170, 160), (0, 5, 2), up=(0, 0, 1), ext=190))
            show_only([])
    finally:
        CASE_SHOWN.clear(); explode(False); show_only([]); hide_cutters()
    print("renders:", done)

def stage_view(g):
    """Debug picture: view=name eye=x;y;z target=x;y;z ext=mm hide=a;b sec=xz@y | yz@x"""
    hide_cutters()
    explode(ARGS.get("explode", "0") == "1")
    hide = [h for h in ARGS.get("hide", "").split(";") if h]
    show_only(hide)
    sec = None
    if ARGS.get("sec"):
        pl, off = ARGS["sec"].split("@")
        sec = section(pl, float(off), ARGS.get("flip", "0") == "1")
    try:
        f = lambda k: tuple(float(v) for v in ARGS[k].split(";"))
        shoot(ARGS.get("view", "debug"), f("eye"), f("target"), up=f("up") if "up" in ARGS else (0, 0, 1), ext=float(ARGS.get("ext", 40)))
    finally:
        if sec:
            sec.deleteMe()
        show_only([])
        explode(False)
    print("view ok")

GRIND_ORDER = ["solar_box", "rib_b", "camera_window", "magnet_slot", "antenna_relief", "pins_hooks"]   # order of the grinding manual (LR44 cup: not ground, rev D)

GRIND_VIEWS = {
    # zone: (components hidden, eye, target, up, extent)
    "solar_box": (["Front shell", "Keycaps", "Keymat", "Window lens", "Window mask", "Solar cell (dummy)", PCB_NAME, "LCD panel", "Camera module",
                   "LiPo battery", "Ribbons and wires", "Magnet connector", "Magnetic cable plug (detached)", "Screws"],
                  (40, 20, 70), (12, 68, 2), (0, 0, 1), 55),
    "rib_b": (["Front shell", "Keycaps", "Keymat", "Window lens", "Window mask", "Solar cell (dummy)", PCB_NAME, "LCD panel", "Camera module",
               "LiPo battery", "Ribbons and wires", "Magnet connector", "Magnetic cable plug (detached)", "Screws"],
              (25, 10, 45), (0, 44, 1), (0, 0, 1), 34),
    "camera_window": (["Magnetic cable plug (detached)"], (-45, 0, -110), (0, 43.84, 0), (0, 0, 1), 75),
    "magnet_slot": (["Magnetic cable plug (detached)", "Magnet connector"], (60, 160, 30), (17, 81, 5), (0, 0, 1), 45),
    # zone 6: faceplate inside, looking at the screen-section wall on the ESP32 side (front +X), everything else hidden
    "antenna_relief": (["Back cover", "Keycaps", "Keymat", "Window lens", "Window mask", "Solar cell (dummy)", PCB_NAME, "LCD panel",
                        "Camera module", "LiPo battery", "Ribbons and wires", "Magnet connector", "Magnetic cable plug (detached)",
                        "Screws", "Battery lid", "Rubber feet"],
                       (5, 20, -40), (34, 42, 5), (0, 1, 0), 40),
}

def grind_renders(g):
    out = []
    keys = GRINDS
    for z, (hide, eye, tgt, up, ext) in GRIND_VIEWS.items():
        show_only(hide)
        if z == "magnet_slot":
            tgt = (150 - MAG_XK, tgt[1], tgt[2]); eye = (150 - MAG_XK + 40, eye[1], eye[2])
        k = GRIND_ORDER.index(z)
        for state, n_done in (("before", k), ("after", k + 1)):     # in grinding order: earlier steps done
            set_grind(GRIND_ORDER, True)
            set_grind(GRIND_ORDER[:n_done], False)
            hide_cutters()
            adsk_refresh()
            out.append(shoot("grind_%s_%s" % (z, state), eye, tgt, up=up, ext=ext))
        if z == "camera_window":                      # also from inside, board removed
            show_only(["Front shell", "Keycaps", "Keymat", "Window lens", "Window mask", "Solar cell (dummy)", PCB_NAME, "LCD panel", "Camera module",
                       "LiPo battery", "Ribbons and wires", "Magnet connector", "Magnetic cable plug (detached)", "Screws"])
            for state, n_done in (("before", k), ("after", k + 1)):
                set_grind(GRIND_ORDER, True)
                set_grind(GRIND_ORDER[:n_done], False)
                hide_cutters()
                adsk_refresh()
                out.append(shoot("grind_%s_inside_%s" % (z, state), (0, 43.84, 150), (0, 43.84, 0), up=(0, 1, 0), ext=40))
    set_grind(keys, False)
    show_only([])
    return out

def adsk_refresh():
    import adsk.core
    adsk.doEvents()
    app.activeViewport.refresh()

# ── Step-by-step pictures for the assembly guide (renders/steps/) ──
# Each shot: only the parts in `show` are visible (component names), bodies whose name contains a string
# in `hide_b` are hidden, `move` = {component: (dx, dy, dz[, flip])} (flip "y" = 180 deg about the Y axis
# through the part's X/Z centre, "x" = about the X axis), camera = target + direction * 300 mm.
# `anchors` = named model points whose pixel position is saved to renders/steps/anchors.json (for the
# annotation script). Back views look from the component side (calculator face down, display end up).
SHELL = ["Front shell", "Keymat", "Keycaps", "Window lens", "Window mask", "Solar cell (dummy)"]
BOARD_SET = [PCB_NAME, "LCD panel", "Camera module", "Magnet connector", "Ribbons and wires"]
D_BACK, D_FRONT = (0.28, -0.42, -1.0), (0.28, -0.42, 1.0)
D_SIDE = (1.0, -0.55, -0.8)                 # exploded shots: the gaps along Z show
UP = (0, 1, 0)
TP = {k: (150 - x, CY - y, Z_F) for k, (x, y) in {"TP1": (121.0, 110.4), "TP4": (120.6, 113.1), "TP7": (128.0, 84.6)}.items()}
H6 = (150 - 130.6, CY - 186.99, Z_F)
J3_PIN1 = (150 - 123.69, CY - 72.0, 6.0)
J3_PIN4 = (150 - 131.31, CY - 72.0, 6.0)
CAMC = (0.0, CY - 95.1, 1.6)

STEP_SHOTS = [
    dict(name="s01_open_shell", show=SHELL + ["Back cover", "Battery lid"], move={"Back cover": (-92, 0, 0, "y"), "Battery lid": (-92, 0, 0, "y")},
         target=(-46, 0, 4), d=D_BACK, ext=190, anchors=dict(mat=(0, -20, 7.5), shell=(30, -60, 10), cover=(-92, -40, -2), window=(0, 46, 15))),
    dict(name="s04_parts", show=[PCB_NAME, "LCD panel", "Camera module", "LiPo battery", "Magnet connector", "Ribbons and wires"],
         hide_b=["FPC"], move={"LCD panel": (-78, 0, 0, "x"), "Camera module": (-62, -44, 0), "LiPo battery": (-65, -95, 0),
                                "Ribbons and wires": (-65, -95, 0), "Magnet connector": (-88, -135, 0)},
         target=(-38, 2, 5), d=D_BACK, ext=185,
         anchors=dict(board=(0, 0, 7), panel=(-78, 45.84, 8), camera=(-62, 0, 1.6), battery=(-80.6, -26.3, 2.5), magnet=(-65, -60, 5))),
    # v15: the LCD tail. Section along the tail's centre line (KiCad y 93.1): panel -> slot (x 181.0) -> one bow -> J5 mouth (x 160.53)
    dict(name="s05a_lcd_tail", show=[PCB_NAME, "LCD panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo"],
         section=("xz", CY - 93.6, False), target=(-19, 45.34, 5.6), d=(-0.25, 1.0, 0.3), up=(0, 0, 1), ext=30,
         anchors=dict(panel=(-20, 45.34, 8.9), slot=(-31.0, 45.34, 7.4), bow=(-21, 45.34, 2.3), j5=(-9.1, 45.34, 6.5),
                      board=(-14, 45.34, 7.4), stiff=(-13.5, 45.34, 6.5), tape=(-24, 45.34, 7.85))),
    dict(name="s05b_lcd_j5", show=[PCB_NAME, "LCD panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo"],
         target=(-19, 45.84, 5.0), d=(0.55, -0.45, -1.0), ext=36,
         anchors=dict(j5=(-9.1, 45.84, 6.0), latch=(-7.6, 45.84, 6.0), slot=(-31.0, 37.5, 7.0), bow=(-21, 40.0, 2.3),
                      pin1=(-7.4, 38.59, 6.0), led=(-7.4, 49.1, 6.0))),
    dict(name="s05c_lcd_on_board", show=[PCB_NAME, "LCD panel", "Ribbons and wires"], hide_b=["Camera FPC", "LiPo"],
         target=(0, 3, 8), d=D_FRONT, ext=170, h=1800,
         anchors=dict(panel=(0, 45.84, 9.3), tail=(-28.5, 45.84, 9.1), keypads=(0, -30, 7.8), aa=(0, 45.84, 9.35))),
    dict(name="s06a_camera", show=[PCB_NAME, "Camera module", "Ribbons and wires"], hide_b=["LCD FPC", "LiPo"],
         target=(0, 12, 5), d=D_BACK, ext=95, h=1600, anchors=dict(camera=CAMC, j1=(0, -19.6, 5.5), ribbon=(0, 15, 5.5))),
    dict(name="s06b_camera_j1", show=[PCB_NAME, "Camera module", "Ribbons and wires"], hide_b=["LCD FPC", "LiPo"],
         target=(0, -17, 5.5), d=D_BACK, ext=36, anchors=dict(j1=(0, -19.6, 5.0), latch=(0, -22.3, 5.0), ribbon=(0, -8, 5.5))),
    dict(name="s07_magnet_j3", show=[PCB_NAME, "Magnet connector"], move={"Magnet connector": (0, 6.5, 0)},
         target=(22.5, 72, 5.5), d=D_BACK, ext=42, anchors=dict(pin1=J3_PIN1, pin4=J3_PIN4, j3=(22.5, 70.5, 5.5), leg1=(26.25, 81.5, 5.7), face=(22.5, 88, 5.7))),
    dict(name="s07b_magnet_in", show=[PCB_NAME, "Magnet connector"],
         target=(22.5, 72, 5.5), d=D_BACK, ext=42, anchors=dict(pin1=J3_PIN1, j3=(22.5, 70.5, 5.5), face=(22.5, 81.5, 5.7))),
    dict(name="s08a_keymat", show=SHELL, move={"Keymat": (0, 0, -28)},
         target=(0, 0, -4), d=D_SIDE, ext=195, h=1600, anchors=dict(mat=(0, -20, -20), notch=(0, -75, -20), posts=(30, -60, 9))),
    dict(name="s08b_board_in", show=SHELL + BOARD_SET, hide_b=["LiPo"],
         move={k: (0, 0, -30) for k in BOARD_SET}, target=(0, 0, -6), d=D_SIDE, ext=200, h=1600,
         anchors=dict(board=(0, 0, -23), h6=(H6[0], H6[1], -23), h6post=(H6[0], H6[1], 7.0), magnet=(22.5, 78, -24), notch=(22.5, 81, 9))),
    dict(name="s08c_magnet_flush", show=SHELL + BOARD_SET, hide_b=["LiPo"],
         target=(22.5, 80, 6), d=(0.35, 1.0, -0.3), ext=44, anchors=dict(face=(22.5, 81.7, 5.7), notch=(32, 81.7, 9.5), rim=(5, 81.5, 8.0))),
    dict(name="s09_battery", show=SHELL + BOARD_SET + ["LiPo battery"],
         target=(-4, 55, 5), d=D_BACK, ext=88,
         anchors=dict(battery=(-15.6, 68.7, 1.0), j4=(8.4, 67, 2.5), plug=(8.4, 60, 2.5), tp1=TP["TP1"], tp4=TP["TP4"], tp7=TP["TP7"], camera=CAMC)),
    dict(name="s10a_cover_lowering", show=SHELL + BOARD_SET + ["LiPo battery", "Back cover", "Battery lid", "Screws"],
         move={"Back cover": (0, 0, -32), "Battery lid": (0, 0, -32), "Screws": (0, 0, -52)}, target=(0, 0, -12), d=D_SIDE, ext=210, h=1600,
         anchors=dict(cover=(0, -40, -32), window=(0, CY - 95.1, -32), screws=(30, -60, -50))),
    dict(name="s10b_closed_back", show=SHELL + BOARD_SET + ["LiPo battery", "Back cover", "Battery lid", "Screws", "Rubber feet"],
         target=(0, 0, 5), d=D_BACK, ext=185, h=1800, anchors=dict(window=(0, CY - 95.1, 0), magnet=(22.5, 81.7, 5.7))),
    dict(name="s11_finished_front", show=SHELL + BOARD_SET + ["LiPo battery", "Back cover", "Battery lid", "Screws", "Rubber feet"],
         target=(0, 0, 10), d=D_FRONT, ext=185, h=1800, anchors=dict(screen=(0, 46, 17), magnet=(22.5, 81.7, 5.7), on=(28, 10, 17))),
]

def place(moves):
    """Occurrence transforms for a step shot (the PCB keeps its base transform)."""
    import adsk.core
    base = load_info().get("pcb_transform")
    for occ in design.rootComponent.occurrences:
        nm = occ.component.name.replace(PREFIX, "").replace("Replica - ", "")
        m = adsk.core.Matrix3D.create()
        if nm == PCB_NAME and base:
            m.setWithArray(base)
        mv = moves.get(nm)
        if mv:
            if len(mv) > 3 and mv[3]:
                if mv[3] == "y":                      # shell halves: one shared axis (X 0, Z 4)
                    cx, cy, cz = 0.0, 0.0, cm(4.0)
                else:
                    bb = occ.boundingBox
                    cx = (bb.minPoint.x + bb.maxPoint.x) / 2
                    cy = (bb.minPoint.y + bb.maxPoint.y) / 2
                    cz = (bb.minPoint.z + bb.maxPoint.z) / 2
                axis = adsk.core.Vector3D.create(0, 1, 0) if mv[3] == "y" else adsk.core.Vector3D.create(1, 0, 0)
                r = adsk.core.Matrix3D.create()
                r.setToRotation(math.pi, axis, adsk.core.Point3D.create(cx, cy, cz))
                m.transformBy(r)
            t = adsk.core.Matrix3D.create()
            t.translation = adsk.core.Vector3D.create(cm(mv[0]), cm(mv[1]), cm(mv[2]))
            m.transformBy(t)
        try:
            occ.isGrounded = False
        except Exception:
            pass
        occ.transform2 = m

def set_grid(on):
    try:
        cd = app.userInterface.commandDefinitions.itemById("ViewLayoutGridCommand").controlDefinition
        for i in range(cd.listItems.count):
            it = cd.listItems.item(i)
            if it.name == "Layout Grid":
                old = it.isSelected
                it.isSelected = on
                return old
    except Exception:
        return None

def stage_steps(g):
    """Assembly-guide pictures, one per step: python build_final_assembly.py steps [only=s05,s07]"""
    import adsk.core
    outdir = os.path.join(OUT, "renders", "steps")
    os.makedirs(outdir, exist_ok=True)
    I = load_info()
    if "pcb_transform" not in I:
        save_info(pcb_transform=list(occ_named(PCB_NAME).transform2.asArray()))
    only = [s for s in ARGS.get("only", "").split(",") if s]
    apath = os.path.join(outdir, "anchors.json")
    anchors = json.load(open(apath)) if os.path.exists(apath) else {}
    grid_was = set_grid(False)
    hide_cutters()
    vp = app.activeViewport
    W = 1600
    done = []
    try:
        for s in STEP_SHOTS:
            if only and not any(s["name"].startswith(o) for o in only):
                continue
            place({})
            for occ in design.rootComponent.occurrences:
                occ.isLightBulbOn = occ.component.name.replace(PREFIX, "").replace("Replica - ", "") in s["show"]
            hb = s.get("hide_b", [])
            for occ in design.rootComponent.occurrences:
                for b in occ.component.bRepBodies:
                    if "cutter" in b.name:
                        b.isLightBulbOn = False
                    else:
                        b.isLightBulbOn = not any(h in b.name for h in hb)
            place(s.get("move", {}))
            H = s.get("h", 1200)
            sec = section(*s["section"]) if s.get("section") else None
            try:
                t = s["target"]; d = s["d"]; n = math.sqrt(sum(c * c for c in d))
                eye = tuple(t[k] + d[k] / n * 300 for k in range(3))
                cam = vp.camera
                cam.isFitView = False
                cam.target = P(*t); cam.eye = P(*eye)
                cam.upVector = adsk.core.Vector3D.create(*s.get("up", UP))
                cam.viewExtents = cm(s["ext"])
                cam.isSmoothTransition = False
                vp.camera = cam
                adsk.doEvents(); vp.refresh(); adsk.doEvents()
                vp.saveAsImageFile(os.path.join(outdir, s["name"] + ".png"), W, H)
                an = {}
                for k, p in list(s.get("anchors", {}).items()) + [("_c", t)]:
                    q = vp.modelToViewSpace(P(*p))
                    an[k] = [round(q.x, 1), round(q.y, 1)]
                anchors[s["name"]] = dict(size=[W, H], viewport=[vp.width, vp.height], vp_points=an)
                done.append(s["name"])
            finally:
                if sec:
                    sec.deleteMe()
    finally:
        place({})
        for occ in design.rootComponent.occurrences:
            occ.isLightBulbOn = True
            for b in occ.component.bRepBodies:
                b.isLightBulbOn = "cutter" not in b.name
        hide_cutters()
        if grid_was is not None:
            set_grid(grid_was)
        json.dump(anchors, open(apath, "w"), indent=1)
    print("steps:", done)

def collision_table(fn="interference.json"):
    """Markdown table of interference.json (offline). Worst first; KiCad position for each overlap."""
    d = json.load(open(os.path.join(OUT, fn)))
    rows = ["| # | Part A | Part B | Front view X, Y | KiCad x, y | Z range | Overlap box (mm) | Depth | mm³ |",
            "| --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    hits = sorted(d["hits"], key=lambda h: -h["depth"])
    for i, h in enumerate(hits, 1):
        X, Y = (h["lo"][0] + h["hi"][0]) / 2, (h["lo"][1] + h["hi"][1]) / 2
        sz = [h["hi"][k] - h["lo"][k] for k in range(3)]
        rows.append("| %d | %s: %s | %s: %s | %.1f, %.1f | %.1f, %.1f | %.2f–%.2f | %.2f × %.2f × %.2f | **%.2f** | %.2f |" % (
            i, h["a"][0], h["a"][1], h["b"][0], h["b"][1], X, Y, 150 - X, CY - Y, h["lo"][2], h["hi"][2], sz[0], sz[1], sz[2], h["depth"], h["vol"]))
    return d, "\n".join(rows)

# ═════════════════════════════════════════════════════════════════════════════
# Battery upgrade study (2026-10-06, final_assembly/battery_upgrade.md). NOT part of "all": rev D stays as built.
#   python build_final_assembly.py lr44 mode=trim|remove|off      add / replace / delete the "GRIND lr44_holder" feature
#   python build_final_assembly.py cell name=.. L=.. W=.. T=.. X=.. Y=.. [rot=y]   candidate cell on the back floor + check
#   python build_final_assembly.py cell name=off                 remove the candidate, show the fitted LiPo again
# Envelope maths (offline): final_assembly/battery_envelope.py
LR44_ZONE = ((-29.0, -7.65), (62.54, 77.34))   # holder footprint (I-bars X -28.6..-8.0, Y 62.94..76.94) + 0.4; 0.5 short of the solar frame (X -7.15)
LR44_TRIM_Z = 7.0                              # trim: holder bottom Z 5.1 -> 7.0 (1.9 off; it stays 3.6 tall)
LR44_STUB = 0.2                                # remove: 0.2 stub left on the plate (plate itself 1.2 untouched)

def stage_lr44(g):
    mode = ARGS.get("mode", "trim")
    fr = comp_named("Front shell")
    fb = R.body_named(fr, "Front shell")
    tl = design.timeline
    for i in reversed(range(tl.count)):
        it = tl.item(i)
        try:
            nm = it.entity.name
        except Exception:
            nm = it.name or ""
        if nm.startswith("GRIND lr44_holder"):
            it.entity.deleteMe()
    if mode == "off":
        print("lr44 grind removed"); return
    z1 = LR44_TRIM_Z if mode == "trim" else R.Z_PLATE_TOP - LR44_STUB
    (x0, x1), (y0, y1) = LR44_ZONE
    xy_prism(fr, [(x0, y0), (x1, y0), (x1, y1), (x0, y1)], 3.0, z1, "GRIND lr44_holder (%s)" % mode, "cut", [fb])
    save_info(lr44=dict(mode=mode, zone=LR44_ZONE, z_to=z1))
    print("lr44 %s: zone X %s Y %s cut up to Z %.2f (rim Z %.1f -> %.2f below the rim)" % (mode, LR44_ZONE[0], LR44_ZONE[1], z1, R.SEAM_Z, z1 - R.SEAM_Z))

def stage_cell(g):
    import adsk.core, adsk.fusion
    name = ARGS.get("name", "cell")
    lp = occ_named("LiPo battery")
    rb = comp_named("Ribbons and wires")
    for o in list(design.rootComponent.occurrences):
        if o.component.name == PREFIX + "Battery candidate":
            o.deleteMe()
    old = [b for b in lp.bRepBodies if b.name.startswith("LiPo pouch")] + [b for b in rb.bRepBodies if b.name.startswith("LiPo lead")]
    if name == "off":
        for b in old:
            b.isLightBulbOn = True
        print("candidate removed"); return
    for b in old:
        b.isLightBulbOn = False
    L, W, T = float(ARGS["L"]), float(ARGS["W"]), float(ARGS["T"])
    if ARGS.get("rot") == "y":
        L, W = W, L
    X, Y = float(ARGS["X"]), float(ARGS["Y"])
    cc = clear_comp("Battery candidate")
    z0 = R.BACK_PLATE
    sk = new_sketch(cc, "cell")
    R.rrect(sk, (X, Y), L, W, 1.0)
    R.extrude(cc, R.profiles(sk), z0, T, name="Cell %s" % name)
    cell = cc.bRepBodies.item(cc.bRepBodies.count - 1)
    appearance(cell, "Aluminum - Anodized Glossy (Grey)")
    # leads: plug in J4 (unchanged) -> under the board -> into the cell's +X end
    plug = [b for b in lp.bRepBodies if b.name.startswith("JST-PH plug")][0]
    pb = plug.boundingBox
    xm, yface, zm = (pb.minPoint.x + pb.maxPoint.x) * 5, pb.minPoint.y * 10, (pb.minPoint.z + pb.maxPoint.z) * 5
    lend = X + L / 2
    bay_X = lend + LIPO_WIRE_D / 2 + 0.05
    lzm = z0 + T / 2
    tbs = []
    for nm, dxw, dz in (("red", 1.0, 0.7), ("black", -1.0, -0.7)):
        yl = min(Y + W / 2 - 1.5, yface - 3.0) + dxw * 0.8
        pts = [(xm + dxw, yface - 0.03, zm), (xm + dxw, yface - 2.0 - (dxw + 1), zm + dz),
               (bay_X, yface - 2.0 - (dxw + 1), zm + dz), (bay_X, yl, zm + dz), (bay_X, yl, lzm + 0.8 * dz),
               (lend + 0.01, yl, lzm + 0.8 * dz)]
        tbm_add(cc, tube(pts, LIPO_WIRE_D), "Lead %s" % nm)
    adsk_refresh()
    # check the cell (and its leads) against every other visible body: overlap + smallest gap
    tbm = adsk.fusion.TemporaryBRepManager.get()
    mm = app.measureManager
    rows, hits = [], []
    cell = [b for b in occ_named("Battery candidate").bRepBodies if b.name.startswith("Cell")][0]   # occurrence proxy (measure needs it)
    cb = cell.boundingBox
    for c, n, b in all_bodies():
        if c == "Battery candidate":
            continue
        bb = b.boundingBox
        if bb.minPoint.x * 10 > cb.maxPoint.x * 10 + 4 or bb.maxPoint.x * 10 < cb.minPoint.x * 10 - 4 or \
           bb.minPoint.y * 10 > cb.maxPoint.y * 10 + 4 or bb.maxPoint.y * 10 < cb.minPoint.y * 10 - 4:
            continue
        try:
            d = mm.measureMinimumDistance(cell, b).value * 10
        except Exception as e:
            d = None
        a = tbm.copy(cell); bc = tbm.copy(b)
        try:
            if b.assemblyContext and abs(bc.boundingBox.minPoint.z - bb.minPoint.z) > 1e-4:
                tbm.transform(bc, b.assemblyContext.transform2)
            tbm.booleanOperation(a, bc, adsk.fusion.BooleanTypes.IntersectionBooleanType)
            v = sum(a.lumps.item(k).volume for k in range(a.lumps.count)) * 1000 if a and a.lumps.count else 0
        except Exception:
            v = None
        rows.append(dict(part=[c, n], gap=None if d is None else round(d, 2), overlap_mm3=None if v is None else round(v, 3)))
        if v and v > 0.005:
            hits.append(rows[-1])
    rows.sort(key=lambda r: (r["gap"] if r["gap"] is not None else 99))
    fn = os.path.join(OUT, "battery_fit_fusion.json")
    allr = json.load(open(fn)) if os.path.exists(fn) else {}
    allr[name] = dict(L=L, W=W, T=T, centre=[X, Y], kicad=[round(150 - X, 2), round(CY - Y, 2)], Z=[z0, z0 + T],
                      hits=hits, nearest=[r for r in rows if r["gap"] is not None][:12], time=time.strftime("%Y-%m-%d %H:%M"))
    json.dump(allr, open(fn, "w"), indent=1)
    print("cell %s %.1f x %.1f x %.1f at (%.2f, %.2f): %d overlaps; nearest:" % (name, L, W, T, X, Y, len(hits)))
    for r in rows[:10]:
        print("   ", r)

# ═════════════════════════════════════════════════════════════════════════════
STAGES = ["new", "shell", "grind", "pcb", "parts", "mask", "check", "clearances", "fulltable", "export", "v15renders"]   # v15: no slide case
# + "steps" (guide pictures, run on its own); battery study: "lr44", "cell" (run on their own, never in "all")

def geometry_cached():
    p = os.path.join(OUT, "geometry.json")
    G = json.load(open(p))
    G["screws"] = [(n, tuple(c)) for n, c in G["screws"]]
    G["locs"] = [(n, tuple(c)) for n, c in G["locs"]]
    return G

def send(stage, extra):
    import urllib.request, urllib.error
    args = dict(extra, stage=stage)
    script = "ARGS = %r\n" % args + open(os.path.abspath(__file__), encoding="utf8").read()
    secret = open(os.path.join(os.path.expanduser("~"), ".fusion-mcp-secret")).read().strip()
    req = urllib.request.Request("http://127.0.0.1:7654/execute", data=json.dumps({"script": script}).encode(),
                                 headers={"Authorization": "Bearer " + secret, "Content-Type": "application/json"})
    logf = os.path.join(OUT, "build.log")
    start = os.path.getsize(logf) if os.path.exists(logf) else 0
    try:
        r = json.load(urllib.request.urlopen(req, timeout=60))
    except urllib.error.HTTPError as e:
        r = json.load(e)
    except Exception as e:
        r = {"error": "timeout: %s" % e}
    res = str(r.get("result", ""))
    if "timeout" in str(r.get("error", "")).lower():
        tail = lambda: open(logf, "rb").read()[start:].decode("utf8", "replace") if os.path.exists(logf) else ""
        t0 = time.time()
        while "stage %s done" % stage not in tail() and "FAILED" not in tail():
            if time.time() - t0 > 3000:
                r = {"error": "gave up waiting"}; break
            time.sleep(3)
        else:
            res = "(finished after the bridge timed out)\n" + tail()[-1500:]
            r = {}
            if "FAILED" in tail():
                print(tail()[-3000:]); sys.stdout.flush(); os._exit(1)
    print(res or r.get("error") or r)
    if res.startswith("Error") or r.get("error"):
        sys.stdout.flush(); os._exit(1)

if "ARGS" in globals():
    import adsk.core, adsk.fusion, traceback
    log("stage " + ARGS["stage"])
    try:
        G = geometry_cached()
        if ARGS["stage"] != "new":
            design = find_design()
        wire_replica()
        globals()["stage_" + ARGS["stage"]](G)
    except Exception:
        log("stage %s FAILED %s" % (ARGS["stage"], traceback.format_exc()))
        raise
    log("stage %s done" % ARGS["stage"])
elif __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    os.makedirs(OUT, exist_ok=True)
    g = R.geometry()                                   # fresh: picks up hardware/fitcheck/backcover_fx115es.json
    json.dump(R.to_json(g), open(os.path.join(OUT, "geometry.json"), "w"))
    print(json.dumps(offline_numbers(), indent=1))
    stages = [a for a in sys.argv[1:] if "=" not in a]
    extra = dict(a.split("=", 1) for a in sys.argv[1:] if "=" in a)
    if stages == ["all"]:
        stages = STAGES
    if stages == ["table"]:
        print(collision_table(extra.get("file", "interference.json"))[1]); stages = []
    for st in stages:
        t = time.time()
        print("==", st); sys.stdout.flush()
        send(st, extra)
        print("   %.0f s" % (time.time() - t)); sys.stdout.flush()
