"""KNOCKOFF picture library (copy of hardware/enclosure/guide_renders.py, main checkout, adapted 2026-10-15).
Render + camera + anchor code only: no model geometry is changed or saved.

    python ko_guide_renders.py ver=ko set=grind  [only=ko_z1,ko_z3]   grinding before/after  -> renders/grind/
    python ko_guide_renders.py ver=ko set=detail [only=ko_d01]        knockoff assembly close-ups -> renders/detail/

Fusion must be open with the FusionMCPBridge add-in and the knockoff assembly document loaded (root occurrences
"KO15 - ...", built by ../build_knockoff_assembly.py; File > Open cad/assembly/ai_calculator_v15_knockoff_assembly.f3d is
enough). The build script is loaded as a module (its stages do not run) and its helpers are reused: place(), section(),
hide_cutters(), set_grid(). Grind features ("GRIND <key>") are only suppressed / unsuppressed and are all restored to
active (all ground) at the end. The component "KO15 - Templates" (3D-printed marking caps) is hidden in every picture and
stays hidden afterwards. Nothing is saved or exported.

Each picture is <outdir>/<name>.png (white background, 1600 x 1200 or 1600 x 1000); model points named in a shot's
`anchors` / `polys` are written to <outdir>/anchors.json in image pixels, for ko_annotate_guides.py (Pillow, offline).

Knockoff frame (mm, front view, X right, Y up, Z = 0 at the outside of the back cover, top edge Y 82.1):
camera window centre (0, 42.0) between back ribs 2 (Y 47.1) and 3 (Y 36.9); battery cups (-14.5, 70.5) and (-25.0, 70.5)
Ø11.8 (front shell); battery-corner screw (-33.5, 66.2); raised battery plate X -37.3..-3.2, Y 59.7..79.2 (0.6 tall, guess),
ground flat under the LiPo; solar box X 4.3..23.9, Y 65.5..75.6 (5.5 tall).
"""
import json, math, os, sys, time, importlib.util

HERE = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\.claude\worktrees\knockoff-pcb-photography-guide-8082f1\hardware\knockoff\guides"
KO = os.path.dirname(HERE)
SRC = {"ko": os.path.join(KO, "build_knockoff_assembly.py")}
LOG = os.path.join(HERE, "ko_guide_renders.log")
CY = 138.94
F = lambda x, y: (150.0 - x, CY - y)          # KiCad -> model front frame
HIDE_ALWAYS = ("Templates",)                  # never in a picture

def log(msg):
    with open(LOG, "a", encoding="utf8") as f:
        print(time.strftime("%H:%M:%S"), msg, file=f)

# ═════════════════════════════════════════════════════════════════════════════
# Shot lists. Component names without the "KO15 - " prefix.
#   show, hide_b (body-name substrings), move {comp: (dx,dy,dz[,flip])}, target, d (view direction, eye = target + d*300)
#   or eye, up, ext (view extent, mm), w/h, ortho, section (plane, offset, flip), grind_off (GRIND name prefixes suppressed;
#   default: none = all ground), anchors {k: (x,y,z)}, polys {k: [(x,y,z), ...]}
# ═════════════════════════════════════════════════════════════════════════════
PCB = "PCB (ai_calc_board.step)"
BACK = ["Back cover", "Battery lid"]
FRONT = ["Front shell"]
UPY = (0, 1, 0)

def box_pts(x0, x1, y0, y1, z):
    return [(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)]

def box8(x0, x1, y0, y1, z0, z1):
    return box_pts(x0, x1, y0, y1, z0) + box_pts(x0, x1, y0, y1, z1)

def circle(c, r, z, n=28):
    return [(c[0] + r * math.cos(2 * math.pi * k / n), c[1] + r * math.sin(2 * math.pi * k / n), z) for k in range(n)]

# ── knockoff geometry (build_knockoff_shell.py / build_knockoff_assembly.py) ──
TOP = 82.1                                   # top outer edge (front Y)
RIM = 1.5                                    # parting line (SEAM_Z)
Z67 = RIM + 7.0                              # "7.0 mm below the rim"
XM, MAGY = 22.5, 81.667                      # magnet notch centre X, top-wall outer face Y
NOTCH = (XM - 10.75, XM + 10.75, RIM, RIM + 7.95)
CAMY = 42.0                                  # KNOCKOFF camera centre (KiCad y 96.94): 40.1 below the top edge
RIB2, RIB3 = 47.1, 36.9                      # back ribs 2 and 3 (centre lines, 1.0 wide, 1.0 tall)
SOLAR = (4.3, 23.9, 65.5, 75.6)              # solar box outline (frame 1.0 wide round it), Z 1.0 -> 6.5
SOLAR_C = ((SOLAR[0] + SOLAR[1]) / 2, (SOLAR[2] + SOLAR[3]) / 2)
PLATE = (-37.3, -3.2, 59.7, 79.2)            # raised battery plate, Z 1.0 -> 1.6 (0.6 guess)
BPG = (-29.61, -3.1, 59.6, 79.07)            # battery_plate grind area (LiPo + 0.5, clipped to the plate + 0.1)
BPG_C = ((BPG[0] + BPG[1]) / 2, (BPG[2] + BPG[3]) / 2)
CUPS = [(-14.5, 70.5), (-25.0, 70.5)]        # front-shell battery cups, Ø11.8 inside
CORNER_SCREW = (-33.5, 66.2)
DOOR_C = (-25.0, 70.5)                       # Ø10 battery hole in the back cover
J4_BOX = (4.43, 12.37, 62.67, 71.31)         # J4 courtyard (front frame), Z 1.39 -> 6.94
PLUG_C = (8.4, 60.6, 4.2)                    # JST-PH plug in J4
LIPO_C = (-15.6, 68.7)

GRIND_SHOTS = []
def gpair(name, zone_key, **kw):
    """before (this zone suppressed, the others ground) + after (all ground), same camera."""
    GRIND_SHOTS.append(dict(kw, name=name + "_before", grind_off=[zone_key]))
    GRIND_SHOTS.append(dict(kw, name=name + "_after", grind_off=[]))

gpair("ko_z1_solar_box", "solar_box", show=BACK, target=(SOLAR_C[0], SOLAR_C[1] - 1, 3.0), d=(0.35, -0.75, 1.0), up=UPY, ext=40,
      anchors=dict(c=(SOLAR_C[0], SOLAR_C[1], 4), x0=(SOLAR[0], SOLAR[2], 1.0), x1=(SOLAR[1], SOLAR[2], 1.0), y1=(SOLAR[1], SOLAR[3], 1.0),
                   ztop=(SOLAR[1] + 0.8, SOLAR[2] - 0.8, 6.5), zbot=(SOLAR[1] + 0.8, SOLAR[2] - 0.8, 1.0), post=(34.05, 74.05, 4),
                   floor=(SOLAR_C[0], 61.0, 1.0)))
gpair("ko_z2_battery_plate", "battery_plate", show=BACK, target=(BPG_C[0] - 2, BPG_C[1] - 1, 1.3), d=(0.3, -0.7, 1.0), up=UPY, ext=46,
      anchors=dict(c=(BPG_C[0], BPG_C[1] - 4, 1.6), x0=(BPG[0], BPG[2], 1.6), x1=(BPG[1], BPG[2], 1.6), y1=(BPG[1], BPG[3], 1.6),
                   ztop=(BPG[1], BPG[2], 1.6), zbot=(BPG[1], BPG[2], 1.0), hole=(DOOR_C[0], DOOR_C[1], 1.0),
                   screw=(CORNER_SCREW[0], CORNER_SCREW[1], 1.6), rim=(PLATE[0] + 1.0, 62.0, 1.6)),
      polys=dict(zone=box_pts(BPG[0], BPG[1], BPG[2], BPG[3], 1.62)))
gpair("ko_z3_camera_window_inside", "camera_window", show=BACK, target=(0, CAMY, 1.0), d=(0.25, -0.6, 1.0), up=UPY, ext=30,
      anchors=dict(c=(0, CAMY, 1.0), e0=(-3.5, CAMY, 1.0), e1=(3.5, CAMY, 1.0), rib2=(-9, RIB2, 2.0), rib3=(-9, RIB3, 2.0)))
gpair("ko_z3_camera_window_outside", "camera_window", show=BACK, target=(0, CAMY, 0.0), d=(0.25, -0.6, -1.0), up=UPY, ext=30,
      anchors=dict(c=(0, CAMY, 0.0), e0=(-3.5, CAMY, 0.0), e1=(3.5, CAMY, 0.0)))
gpair("ko_z5_magnet_notch", "magnet_slot (front", show=FRONT, target=(XM, MAGY, 5.0), d=(0.3, 1.0, 0.45), up=(0, 0, 1), ext=42,
      anchors=dict(c=(XM, MAGY, 5.0), l=(NOTCH[0], MAGY, RIM), r=(NOTCH[1], MAGY, RIM), lt=(NOTCH[0], MAGY, NOTCH[3]),
                   rt=(NOTCH[1], MAGY, NOTCH[3]), rim=(XM - 18, MAGY, RIM)))
gpair("ko_z5b_lip", "magnet_slot (back", show=BACK, target=(XM, 79.5, 1.8), d=(-0.2, -0.65, 1.0), up=UPY, ext=40,
      anchors=dict(c=(XM, 79.5, 2.0), l=(NOTCH[0], 79.8, 2.3), r=(NOTCH[1], 79.8, 2.3)))
gpair("ko_z6_antenna_relief", "antenna_relief", show=FRONT, target=(34.0, 50.0, 6.0), d=(-0.75, -0.35, -1.0), up=UPY, ext=46,
      anchors=dict(c=(34.0, 50.0, 6.0), y15=(34.95, TOP - 15, RIM), y50=(34.95, TOP - 50, RIM),
                   zr=(34.95, 40, RIM), z7=(34.95, 40, Z67), rib=(34.95, 60, 7.0)))
gpair("ko_z6b_pins_hooks", "pins_hooks", show=FRONT, target=(33.5, 45.0, 6.5), d=(-0.75, -0.35, -1.0), up=UPY, ext=50,
      anchors=dict(c=(33.5, 45.0, 6.5), p1=(34.6, 56.0, 7.5), p2=(34.6, 47.5, 7.5), p3=(34.6, 37.3, 7.5),
                   h1=(34.4, 64.5, 7.0), h2=(34.4, 59.0, 7.0), h3=(34.4, 24.2, 7.0)))
gpair("ko_z6b_pins_hooks_both_walls", "pins_hooks", show=FRONT, target=(0, 45.0, 6.0), d=(0.0, -0.45, -1.0), up=UPY, ext=88,
      anchors=dict(c=(0, 45, 6), lw=(-34.6, 47.5, 7.5), rw=(34.6, 47.5, 7.5)))
# overviews (orthographic, straight on): every zone, before -> highlight = difference to all-ground
KO_ZONES = ["solar_box", "battery_plate", "camera_window", "magnet_slot", "antenna_relief", "pins_hooks"]
for st, off in (("before", KO_ZONES), ("after", [])):
    GRIND_SHOTS.append(dict(name="ko_overview_back_cover_" + st, grind_off=off, show=BACK, target=(0, 2.0, 1.0), eye=(0, 2.0, 300),
                            up=(-1, 0, 0), ext=108, w=1600, h=1000, ortho=True,
                            anchors=dict(z1=(SOLAR_C[0], SOLAR_C[1], 2), z2=(BPG_C[0], BPG_C[1] - 5, 1.6), z3=(0, CAMY, 1),
                                         z5b=(XM, 80.5, 2), top=(0, TOP, 1))))
    GRIND_SHOTS.append(dict(name="ko_overview_faceplate_" + st, grind_off=off, show=FRONT, target=(0, 2.0, 6.0), eye=(0, 2.0, -300),
                            up=(1, 0, 0), ext=108, w=1600, h=1000, ortho=True,
                            anchors=dict(z5=(XM, 81.0, 4), z6=(34.9, 50, 4), z6b_l=(-34.6, 47.5, 7.5), z6b_r=(34.6, 56, 7.5),
                                         top=(0, TOP, 4))))
# camera hole drill position (outside, straight on) and the magnet U-notch, straight on, orthographic
GRIND_SHOTS.append(dict(name="ko_camera_hole_position", grind_off=[], show=BACK, target=(0, 50.0, 0.0), eye=(0, 50.0, -300), up=UPY,
                        ext=78, ortho=True, measure="cam",
                        anchors=dict(c=(0, CAMY, 0), e0=(-3.5, CAMY, 0), e1=(3.5, CAMY, 0), top=(0, TOP, 0))))
GRIND_SHOTS.append(dict(name="ko_magnet_notch_dims", grind_off=[], show=FRONT, target=(XM + 5, MAGY, 5.0), eye=(XM + 5, 300, 5.0),
                        up=(0, 0, -1), ext=40, ortho=True, measure="notch",
                        anchors=dict(l=(NOTCH[0], MAGY, RIM), r=(NOTCH[1], MAGY, RIM), lt=(NOTCH[0], MAGY, NOTCH[3]),
                                     rt=(NOTCH[1], MAGY, NOTCH[3]))))

# ── knockoff assembly close-ups (only what differs from v15) ──
OFF_BACK = ["Back cover", "Battery lid", "Screws", "Rubber feet"]
DETAIL_KO = [
    # battery corner, inside of the back cover: LiPo on the ground plate, leads, plug in J4 (board hidden: J4 dashed),
    # the front-shell cups (above, on the far side of the board) dashed
    dict(name="ko_d01_battery_corner_back", show=BACK + ["LiPo battery", "Ribbons and wires", "Screws"], hide_b=["LCD FPC", "Camera FPC"],
         target=(-11, 68.5, 2.5), d=(0.3, -0.6, 1.0), up=UPY, ext=56,
         polys=dict(j4=box_pts(*J4_BOX, 1.4), cup1=circle(CUPS[0], 5.9, 4.85), cup2=circle(CUPS[1], 5.9, 4.85)),
         anchors=dict(battery=(LIPO_C[0], LIPO_C[1], 4.8), leads=(0.5, 66.0, 3.5), plug=PLUG_C, screw=(CORNER_SCREW[0], CORNER_SCREW[1], 9.2),
                      cup1=(CUPS[0][0], CUPS[0][1] + 5.9, 4.85), cup2=(CUPS[1][0], CUPS[1][1] + 5.9, 4.85), hole=(DOOR_C[0], DOOR_C[1], 1.0))),
    # same corner seen from the back with the cover off: board, LiPo, leads, J4 + plug
    dict(name="ko_d02_battery_corner_board", show_all=True, hide=OFF_BACK + ["Magnetic cable plug (detached)"], hide_b=["Camera FPC"],
         target=(-9, 68.5, 4.0), d=(-0.3, -0.6, -1.0), up=UPY, ext=56,
         anchors=dict(battery=(LIPO_C[0], LIPO_C[1], 1.0), plug=(PLUG_C[0], PLUG_C[1], 1.9), j4=(8.4, 67.0, 1.4), leads=(0.5, 66.0, 1.8))),
    # section across the corner at Y 70.5 (the cups' centre line): cups / board / LiPo / back cover stacked
    dict(name="ko_d03_battery_corner_section", show_all=True, hide=["Magnetic cable plug (detached)", "Key labels"],
         section=("xz", 70.5, False), target=(-12, 70.5, 6.0), eye=(-12, 370.5, 6.0), up=(0, 0, 1), ext=30, w=1600, h=1000, ortho=True,
         anchors=dict(cup1=(CUPS[0][0], 70.5, 8.5), cup2=(CUPS[1][0], 70.5, 8.5), board=(-5, 70.5, 7.4), battery=(LIPO_C[0], 70.5, 2.9),
                      floor=(-31, 70.5, 0.5), j4=(8.4, 70.5, 4.0), hole=(DOOR_C[0], 70.5, 0.3))),
    # camera between back ribs 2 and 3 (board hidden, camera module shown on the cover)
    dict(name="ko_d04_camera_between_ribs", show=BACK + ["Camera module"], target=(0, CAMY, 2.5), d=(0.35, -0.6, 1.0), up=UPY, ext=34,
         anchors=dict(camera=(0, CAMY, 6.9), rib2=(-10, RIB2, 2.0), rib3=(-10, RIB3, 2.0), window=(0, CAMY - 2.5, 1.0))),
    # section along the centre line through the camera: rib 3 / camera / rib 2
    dict(name="ko_d05_camera_rib_section", show=["Back cover", PCB, "Camera module", "Ribbons and wires"], hide_b=["LCD FPC", "LiPo"],
         section=("yz", 0.01, False), target=(0.0, CAMY, 4.0), eye=(300, CAMY, 4.0), up=(0, 0, 1), ext=13, w=1600, h=1000, ortho=True,
         anchors=dict(camera=(0, CAMY, 4.5), rib2=(0, RIB2, 1.5), rib3=(0, RIB3, 1.5), g2a=(0, 46.25, 2.2), g2b=(0, 46.6, 2.2),
                      g3a=(0, 37.4, 2.2), g3b=(0, 37.75, 2.2), window=(0, CAMY, 0.5), board=(0, 48.5, 7.4), lens=(0, CAMY, 1.6))),
    # screen lens + black window mask (same parts as v15, knockoff window)
    dict(name="ko_d06_window_mask_off", show=["Front shell", "Window lens", "Window mask"], move={"Window mask": (0, 0, -18)},
         target=(0, 46, 10), d=(0.25, -0.35, -1.0), up=UPY, ext=82, anchors=dict(mask=(0, 46, -7), lens=(0, 58, 11.3))),
    dict(name="ko_d07_window_mask_on", show=["Front shell", "Window lens", "Window mask"],
         target=(0, 46, 10), d=(0.25, -0.35, -1.0), up=UPY, ext=82, anchors=dict(mask=(0, 46, 10.8), edge=(-23, 46, 10.8))),
]

SETS = {"grind": ("ko", os.path.join(HERE, "renders", "grind"), GRIND_SHOTS),
        "detail": ("ko", os.path.join(HERE, "renders", "detail"), DETAIL_KO)}

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
                occ.isLightBulbOn = ((nm not in s.get("hide", []) and not nm.startswith("Slide case")) if s.get("show_all") else nm in show) and nm not in HIDE_ALWAYS
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
                # KNOCKOFF fix: the projected-unit-vector estimate of the original was 5-10 % off; orthographic views span
                # exactly `ext` mm over min(W, H) pixels
                rec["px_per_mm"] = round(min(W, H) / s["ext"], 3)
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
            occ.isLightBulbOn = not strip(occ.component.name).startswith("Slide case") and strip(occ.component.name) not in HIDE_ALWAYS
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
        ver = ARGS.get("ver", SETS[ARGS["set"]][0])
        assert ver == "ko", "this copy only knows ver=ko"
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
