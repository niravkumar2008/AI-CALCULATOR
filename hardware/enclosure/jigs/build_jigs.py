"""3D-printable grinding jigs for the donor fx-115ES shell (see README.md in this folder).

    python ../fusion_run.py build_jigs.py stage=build     builds the jigs as component "Jigs" in the open final-assembly
                                                          document (run build_final_assembly.py new shell grind ... first),
                                                          checks them against the shell, exports STLs + renders
    python ../fusion_run.py build_jigs.py stage=remove    deletes the "Jigs" component again

Frame = build_final_assembly.py front-view model frame (X right, Y up, Z = 0 at the outside of the back cover).
The jigs are modelled IN PLACE on the shell (so the check is real), then exported in that position; turn them over
in the slicer (each STL notes which face goes on the bed).

  A  back-cover plate      rests on the 4 upper screw bosses (pins in their 2.2 holes); windows = solar-box zone and a
                           16 mm hole round the camera centre (rib B + camera). Dremel router base rides on its top.
  A2 camera drill bush      drops into A's 16 mm hole: 2.1 mm pilot exactly on the camera centre.
  A3 depth gauge 7.2        sets the router bit: 7.2 below the base = 0.2 above the floor when the base is on plate A.
  B  magnet-notch saddle    clips over the faceplate's top wall (face down on the bench), registers on the rim, the
                           outer top wall and the right-hand corner; window = the 21.5 x 7.95 U-notch.
  C  LR44 trim sled         (battery upgrade only) rides on the faceplate rim; a sandpaper tongue takes the LR44 holder
                           down to Z 7.0 (5.5 below the rim) and stops itself.
"""
import importlib.util, json, math, os, sys, time

ENC = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\enclosure"
OUTJ = os.path.join(ENC, "jigs")

def _load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

B = _load(os.path.join(ENC, "build_final_assembly.py"), "bfa")
R = B.R

# ── parameters ──
CLR = 0.15                 # printed fit clearance against the shell
PLATE_T = 3.0              # A: plate thickness
PIN_D, PIN_L = 2.0, 3.0    # A: pins into the 2.2 boss holes
PLATE_Y_MIN = 12.0         # A: stays above the ramp of the navy side walls
CAM_HOLE_D = 16.0          # A: opening round the camera centre (covers the 14 mm of rib B too)
BUSH = dict(plug_d=15.7, flange_d=22.0, flange_t=2.0, hole_d=2.1, guide_h=8.0)
DEPTH_SET = 7.2            # A3: bit length below the router base (plate top Z 8.4 -> Z 1.2 = 0.2 above the floor)
SADDLE = dict(x_from=10.0, outer_t=3.0, inner_t=2.0, bridge_t=3.0, wrap_to_y=68.0, outer_to_z=11.0, inner_to_z=10.0)
SLED = dict(x=(-42.0, 42.0), y=(46.0, 81.6), t=3.0, sweep_x=2.5, sweep_y=1.5, paper=0.6, knob=(22.0, 16.0, 12.0))

def clip_half(poly, axis, val, keep_ge=True):
    """Sutherland-Hodgman clip of a polygon by the half plane coord[axis] >= val (or <=)."""
    inside = (lambda p: p[axis] >= val) if keep_ge else (lambda p: p[axis] <= val)
    out = []
    for i in range(len(poly)):
        a, b = poly[i - 1], poly[i]
        ia, ib = inside(a), inside(b)
        if ia != ib:
            t = (val - a[axis]) / (b[axis] - a[axis])
            out.append((a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1])))
        if ib:
            out.append(b)
    return out

def arc(V, test):
    """the contiguous run of outline points passing test (outline is closed)"""
    n = len(V)
    ok = [test(p) for p in V]
    if all(ok):
        return list(V)
    s = next(i for i in range(n) if not ok[i])          # start after a failing point
    run, best = [], []
    for k in range(1, n + 1):
        i = (s + k) % n
        if ok[i]:
            run.append(V[i])
        else:
            if len(run) > len(best):
                best = run
            run = []
    return best if len(best) >= len(run) else run

def band(V, d0, d1, test):
    """polygon between the outline offsets d0 and d1 (R.offset_const: + = inwards) over the arc where test holds"""
    a0 = arc(R.offset_const(V, d0), test)
    a1 = arc(R.offset_const(V, d1), test)
    # same direction: join a0 forward with a1 backward
    if math.dist(a0[0], a1[0]) > math.dist(a0[0], a1[-1]):
        a1 = a1[::-1]
    return a0 + a1[::-1]

def build(g):
    import adsk.core, adsk.fusion
    design = B.design
    root = design.rootComponent
    for o in list(root.occurrences):
        if o.component.name == B.PREFIX + "Jigs":
            o.deleteMe()
    jc = B.comp_named("Jigs", True)
    out = {}

    # ---------- A: back-cover plate ----------
    zb = R.Z_MEET - 0.1                                  # boss tops (5.4)
    outline = clip_half(R.offset_const(g["lip_i"], 0.8), 1, PLATE_Y_MIN)
    B.xy_prism(jc, outline, zb, zb + PLATE_T, "A back-cover plate")
    a = jc.bRepBodies.item(jc.bRepBodies.count - 1)
    x0, x1, y0, y1 = B.SOLAR_BOX_CUT
    B.xy_prism(jc, [(x0, y0), (x1, y0), (x1, y1), (x0, y1)], zb - 1, zb + PLATE_T + 1, "A window solar box", "cut", [a])
    cam = B.F(*B.CAM_K)
    B.xy_circle(jc, cam, CAM_HOLE_D, zb - 1, zb + PLATE_T + 1, "A hole camera / rib B", "cut", [a])
    bosses = [c for n, c in g["screws"] if n in ("corner L", "corner R", "mid L", "mid R")]
    for c in bosses:
        B.xy_circle(jc, c, PIN_D, zb - PIN_L, zb + 0.5, "A pin", "join", [a])
    # label recess "A  0.2" is left to the slicer; a notch on the top edge marks the top end
    a.name = "A back-cover plate"
    out["A"] = a

    # ---------- A2: camera drill bush ----------
    zt = zb + PLATE_T
    B.xy_circle(jc, cam, BUSH["plug_d"], zb + 0.2, zt, "A2 camera drill bush")
    a2 = jc.bRepBodies.item(jc.bRepBodies.count - 1)
    B.xy_circle(jc, cam, BUSH["flange_d"], zt, zt + BUSH["flange_t"], None, "join", [a2])
    B.xy_circle(jc, cam, 7.0, zt + BUSH["flange_t"] - 0.01, zt + BUSH["guide_h"], None, "join", [a2])
    B.xy_circle(jc, cam, BUSH["hole_d"], zb - 1, zt + BUSH["guide_h"] + 1, "A2 pilot hole", "cut", [a2])
    a2.name = "A2 camera drill bush"
    out["A2"] = a2

    # ---------- A3: depth gauge (printed flat, sits beside the jig in the model) ----------
    B.xy_prism(jc, [(45, 60), (65, 60), (65, 75), (45, 75)], 0, DEPTH_SET, "A3 depth gauge 7.2")
    out["A3"] = jc.bRepBodies.item(jc.bRepBodies.count - 1)

    # ---------- B: magnet-notch saddle (faceplate top wall) ----------
    S = SADDLE
    Vf, C = g["Vf"], g["C"]
    sel = lambda p: p[0] >= S["x_from"] and p[1] >= S["wrap_to_y"]
    outer = band(Vf, -CLR, -(CLR + S["outer_t"]), sel)
    inner = band(C, CLR, CLR + S["inner_t"], lambda p: S["x_from"] <= p[0] <= 33.5 and p[1] > 75)
    bridge = band(Vf, -(CLR + S["outer_t"]), 1.2 + CLR + S["inner_t"], sel)
    zr = R.SEAM_Z
    B.xy_prism(jc, bridge, zr - S["bridge_t"], zr, "B magnet-notch saddle")
    bb_ = jc.bRepBodies.item(jc.bRepBodies.count - 1)
    B.xy_prism(jc, outer, zr - 0.01, S["outer_to_z"], None, "join", [bb_])
    B.xy_prism(jc, inner, zr - 0.01, S["inner_to_z"], None, "join", [bb_])
    # the faceplate itself is the cutting tool's negative: subtract it (with clearance it is already clear; this
    # removes any overlap where the outline offsets approximate the real wall)
    fr = B.occ_named("Front shell")
    fb = [b for b in fr.bRepBodies if b.name == "Front shell"][0]
    tbm = adsk.fusion.TemporaryBRepManager.get()
    xm, w2, depth = 150 - B.MAG_XK, B.MAG_SLOT[0] / 2, B.MAG_SLOT[1]
    B.xy_prism(jc, [(xm - w2, 70), (xm + w2, 70), (xm + w2, 95), (xm - w2, 95)], zr - S["bridge_t"] - 1, zr + depth,
               "B window (U-notch)", "cut", [bb_])
    bb_.name = "B magnet-notch saddle"
    out["B"] = bb_

    # ---------- C: LR44 trim sled (battery upgrade) ----------
    L = SLED
    (zx0, zx1), (zy0, zy1) = B.LR44_ZONE
    B.xy_prism(jc, [(L["x"][0], L["y"][0]), (L["x"][1], L["y"][0]), (L["x"][1], L["y"][1]), (L["x"][0], L["y"][1])],
               zr - L["t"], zr, "C LR44 trim sled")
    c_ = jc.bRepBodies.item(jc.bRepBodies.count - 1)
    tx0, tx1, ty0, ty1 = zx0 + L["sweep_x"], zx1 - L["sweep_x"], zy0 + L["sweep_y"], zy1 - L["sweep_y"]
    B.xy_prism(jc, [(tx0, ty0), (tx1, ty0), (tx1, ty1), (tx0, ty1)], zr - 0.01, B.LR44_TRIM_Z - L["paper"], None, "join", [c_])
    for e, q in g["snap"]["teeth"]:                     # relief for the comb snap teeth (they hang 0.4 below the rim)
        if e == "top":
            xs = [p[0] for p in q]; ys = [p[1] for p in q]
            B.xy_prism(jc, [(min(xs) - 0.5, min(ys) - 0.5), (max(xs) + 0.5, min(ys) - 0.5), (max(xs) + 0.5, max(ys) + 2), (min(xs) - 0.5, max(ys) + 2)],
                       zr - 1.0, zr + 0.5, None, "cut", [c_])
    kx, ky, kh = L["knob"]
    cx_, cy_ = (zx0 + zx1) / 2, (zy0 + zy1) / 2
    B.xy_prism(jc, [(cx_ - kx / 2, cy_ - ky / 2), (cx_ + kx / 2, cy_ - ky / 2), (cx_ + kx / 2, cy_ + ky / 2), (cx_ - kx / 2, cy_ + ky / 2)],
               zr - L["t"] - kh, zr - L["t"] + 0.01, None, "join", [c_])
    c_.name = "C LR44 trim sled"
    out["C"] = c_
    B.adsk_refresh()
    return out

def check(bodies):
    """overlap (mm3) and smallest gap of each jig with the shell part it sits on"""
    import adsk.core, adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    mm = B.app.measureManager
    jo = B.occ_named("Jigs")
    res = {}
    target = {"A": "Back cover", "A2": "Back cover", "B": "Front shell", "C": "Front shell"}
    for k, comp in target.items():
        jb = [b for b in jo.bRepBodies if b.name.startswith(k + " ")][0]
        tgt = [b for b in B.occ_named(comp).bRepBodies if b.isLightBulbOn and "cutter" not in b.name]
        r = []
        for tb_ in tgt:
            a = tbm.copy(jb); b2 = tbm.copy(tb_)
            tbm.booleanOperation(a, b2, adsk.fusion.BooleanTypes.IntersectionBooleanType)
            v = sum(a.lumps.item(i).volume for i in range(a.lumps.count)) * 1000 if a and a.lumps.count else 0
            try:
                d = mm.measureMinimumDistance(jb, tb_).value * 10
            except Exception:
                d = None
            r.append(dict(body=tb_.name, overlap_mm3=round(v, 3), gap=None if d is None else round(d, 2)))
        res[k] = r
    return res

def export(bodies):
    import adsk.fusion
    em = B.design.exportManager
    os.makedirs(OUTJ, exist_ok=True)
    files = []
    jo = B.occ_named("Jigs")
    for b in jo.bRepBodies:
        fn = os.path.join(OUTJ, "".join(ch if ch.isalnum() else "_" for ch in b.name).strip("_") + ".stl")
        o = em.createSTLExportOptions(b, fn)
        o.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementHigh
        em.execute(o)
        files.append(os.path.basename(fn))
    return files

def renders():
    hide_all = ["Keymat", "Keycaps", "Window lens", "Window mask", "Solar cell (dummy)", "Battery lid", "Rubber feet", "E-paper panel",
                "Camera module", "LiPo battery", "Magnet connector", "Magnetic cable plug (detached)", "Screws", "Ribbons and wires",
                B.PCB_NAME, "Battery candidate", "Slide case (in use)", "Slide case (stored)"]
    jo = B.occ_named("Jigs")
    def only(keys):
        for b in jo.bRepBodies:
            b.isLightBulbOn = any(b.name.startswith(k + " ") for k in keys)
    shots = []
    B.hide_cutters()
    os.makedirs(os.path.join(B.OUT, "renders"), exist_ok=True)
    # A + A2 on the back cover (inside view)
    B.show_only(hide_all + ["Front shell"]); only(["A", "A2"])
    shots.append(B.shoot("../../jigs/render_A_back_cover_plate", (-60, -40, 120), (0, 50, 5), up=(0, 0, 1), ext=95))
    only(["A"])
    shots.append(B.shoot("../../jigs/render_A_top", (0, 46, 150), (0, 46, 5), up=(0, 1, 0), ext=90))
    # B on the faceplate (face down): seen from above the rim = from -Z
    B.show_only(hide_all + ["Back cover"]); only(["B"])
    shots.append(B.shoot("../../jigs/render_B_magnet_saddle", (60, 140, -60), (22, 80, 5), up=(0, 0, -1), ext=45))
    # C on the faceplate
    only(["C"])
    shots.append(B.shoot("../../jigs/render_C_lr44_sled", (-80, 10, -70), (-18, 70, 4), up=(0, 0, -1), ext=75))
    for b in jo.bRepBodies:
        b.isLightBulbOn = True
    B.show_only([])
    return shots

def main():
    B.app = app
    B.design = B.find_design()
    B.ARGS = ARGS
    B.wire_replica()
    g = B.geometry_cached()
    st = ARGS.get("stage", "build")
    if st == "remove":
        for o in list(B.design.rootComponent.occurrences):
            if o.component.name == B.PREFIX + "Jigs":
                o.deleteMe()
        print("jigs removed"); return
    bodies = build(g)
    res = check(bodies)
    files = export(bodies)
    shots = renders() if ARGS.get("renders", "1") == "1" else []
    json.dump(dict(time=time.strftime("%Y-%m-%d %H:%M"), check=res, stl=files), open(os.path.join(OUTJ, "jigs_check.json"), "w"), indent=1)
    print(json.dumps(res, indent=1)); print(files); print(shots)

if "ARGS" in globals():
    main()
