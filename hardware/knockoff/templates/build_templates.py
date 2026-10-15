"""Marking templates for the knockoff shell: where the camera hole and the magnet notch go (2026-10-15).

    python hardware/knockoff/templates/build_templates.py            run inside Fusion through the bridge (see below)

Needs Fusion with FusionMCPBridge and the knockoff assembly open (python hardware/knockoff/build_knockoff_assembly.py
new shell grind pcb parts). The templates are modelled IN PLACE on the closed calculator, checked for overlap against every
body of the assembly, exported as STL, and rendered.

  T1  top-end cap      slides over the top end of the CLOSED calculator. Back face: a 7 mm hole on the camera centre
                       (KiCad 150, 96.94 = centred, 40.1 mm below the top edge). Top wall: a window that is exactly the
                       magnet U-notch (21.5 wide, 6.4-27.9 mm in from the right edge seen from the front, from the parting
                       line up 7.95 mm). The back plate is a frame + centre strip, so it never sits on feet, the battery lid
                       or any other unknown bump on the back.
  T2  centre plug      pushes into T1's 7 mm hole from outside: 2.1 mm pilot exactly on the camera centre.

Frame = build_knockoff_assembly.py front-view model frame (X right, Y up, Z = 0 at the outside of the back cover).
"""
import importlib.util, json, math, os, sys, time

WT = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\.claude\worktrees\knockoff-pcb-photography-guide-8082f1"
OUTT = os.path.join(WT, "hardware", "knockoff", "templates")

def _load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

B = _load(os.path.join(WT, "hardware", "knockoff", "build_knockoff_assembly.py"), "kba")
R = B.R

# ── parameters ──
CLR = 0.35            # sleeve clearance all round (FDM: looser than the 0.15 of the grinding jigs; 0.25 touched the back cover by 0.05 at the side-wall ramp)
WALL = 1.8            # cap side / top wall
PLATE_T = 1.6         # back plate
Y_MIN = 24.0          # the cap reaches this far down the calculator (front-view Y; the top edge is Y 82.1)
BAND = 5.0            # back-plate frame width along the edges
STRIP = 16.0          # back-plate centre strip width (carries the camera hole)
LIP_IN, LIP_T = 3.0, 1.2   # front lip over the top edge of the face (holds the cap on), only over the top end
LIP_Y = 70.0          # the lip runs where the outline is above this Y (top end + corners)
CAM = (0.0, 42.0)     # camera centre (front view), = B.F(*B.CAM_K)
CAM_HOLE = 7.0
PLUG = dict(d=6.8, flange_d=12.0, flange_t=1.2, hole_d=2.1)

def clip_half(poly, axis, val, keep_ge=True):
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
    n = len(V)
    ok = [test(p) for p in V]
    if all(ok):
        return list(V)
    s = next(i for i in range(n) if not ok[i])
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
    """polygon between outline offsets d0 and d1 (R.offset_const: + = inwards) along the arc where test holds"""
    V = R.densify(V, 1.0)          # the traced outline has single long segments down the straight sides
    a0 = arc(R.offset_const(V, d0), test)
    a1 = arc(R.offset_const(V, d1), test)
    if math.dist(a0[0], a1[0]) > math.dist(a0[0], a1[-1]):
        a1 = a1[::-1]
    return a0 + a1[::-1]

def geometry2d(g):
    """Everything T1 needs in 2D (also used by the paper template, so the two can't disagree)."""
    Vb, Vf = g["Vb"], g["Vf"]
    top = max(p[1] for p in Vb)
    outer = clip_half(R.offset_const(Vb, -(CLR + WALL)), 1, Y_MIN)
    win_l = clip_half(clip_half(clip_half(R.offset_const(Vb, BAND), 1, Y_MIN + BAND), 0, -STRIP / 2, False), 1, top - BAND, False)
    win_r = clip_half(clip_half(clip_half(R.offset_const(Vb, BAND), 1, Y_MIN + BAND), 0, STRIP / 2, True), 1, top - BAND, False)
    walls = band(Vb, -CLR, -(CLR + WALL), lambda p: p[1] >= Y_MIN)
    lip = band(Vb, -(CLR + WALL), LIP_IN, lambda p: p[1] >= LIP_Y)
    xm, w2 = 150 - B.MAG_XK, B.MAG_SLOT[0] / 2
    xr = max(p[0] for p in Vf)
    return dict(top=top, outer=outer, win_l=win_l, win_r=win_r, walls=walls, lip=lip, notch_x=(xm - w2, xm + w2),
                notch_z=(R.SEAM_Z, R.SEAM_Z + B.MAG_SLOT[1]), right_edge=xr, cam=CAM, cam_d=CAM_HOLE,
                feet=[tuple(f) for f in R.FEET], outline=Vb)

def build(g):
    import adsk.core, adsk.fusion
    design = B.design
    for o in list(design.rootComponent.occurrences):
        if o.component.name == B.PREFIX + "Templates":
            o.deleteMe()
    tc = B.comp_named("Templates", True)
    G = geometry2d(g)
    z_back = -0.05                                   # plate top face, just off the back cover's outside (Z 0)
    z_face = R.T_TOP + CLR                           # lip underside, just above the face at the top end
    # T1 back plate (frame + centre strip)
    B.xy_prism(tc, G["outer"], z_back - PLATE_T, z_back, "T1 top-end cap")
    t1 = tc.bRepBodies.item(tc.bRepBodies.count - 1)
    for w in (G["win_l"], G["win_r"]):
        B.xy_prism(tc, w, z_back - PLATE_T - 1, z_back + 1, None, "cut", [t1])
    # walls round the top end, up to the face, and the lip over the face's top edge
    B.xy_prism(tc, G["walls"], z_back - PLATE_T, z_face, None, "join", [t1])
    B.xy_prism(tc, G["lip"], z_face - 0.01, z_face + LIP_T, None, "join", [t1])
    # magnet-notch window in the top wall (from the parting line up 7.95)
    x0, x1 = G["notch_x"]; za, zb = G["notch_z"]
    B.xy_prism(tc, [(x0, 60), (x1, 60), (x1, 100), (x0, 100)], za, zb, "T1 window magnet notch", "cut", [t1])
    # camera hole
    B.xy_circle(tc, CAM, CAM_HOLE, z_back - PLATE_T - 1, z_back + 1, "T1 hole camera", "cut", [t1])
    t1.name = "T1 top-end cap"
    # T2 centre plug (from outside: flange below the plate)
    zp = z_back - PLATE_T
    B.xy_circle(tc, CAM, PLUG["d"], zp, z_back - 0.05, "T2 centre plug")
    t2 = tc.bRepBodies.item(tc.bRepBodies.count - 1)
    B.xy_circle(tc, CAM, PLUG["flange_d"], zp - PLUG["flange_t"], zp + 0.01, None, "join", [t2])
    B.xy_circle(tc, CAM, PLUG["hole_d"], zp - PLUG["flange_t"] - 1, z_back + 1, "T2 pilot", "cut", [t2])
    t2.name = "T2 centre plug"
    B.adsk_refresh()
    return dict(T1=t1, T2=t2), G

def check():
    """overlap (mm3) and closest gap of each template against every body of the closed calculator"""
    import adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    mm = B.app.measureManager
    to = B.occ_named("Templates")
    res = {}
    others = []
    for o in B.design.rootComponent.occurrences:
        n = o.component.name
        if n.endswith("Templates") or "plug (detached)" in n:
            continue
        others += [(n.replace(B.PREFIX, ""), b) for b in o.bRepBodies if b.isLightBulbOn and "cutter" not in b.name]
    for jb in to.bRepBodies:
        hits, gap = [], None
        for n, tb_ in others:
            a = tbm.copy(jb); b2 = tbm.copy(tb_)
            try:
                tbm.booleanOperation(a, b2, adsk.fusion.BooleanTypes.IntersectionBooleanType)
                v = sum(a.lumps.item(i).volume for i in range(a.lumps.count)) * 1000 if a and a.lumps.count else 0
            except Exception:
                v = 0
            if v > 1e-4:
                hits.append(dict(part=n, body=tb_.name, overlap_mm3=round(v, 4)))
            if n in ("Back cover", "Front shell"):
                try:
                    d = mm.measureMinimumDistance(jb, tb_).value * 10
                    gap = d if gap is None else min(gap, d)
                except Exception:
                    pass
        res[jb.name] = dict(hits=hits, closest_gap_to_shell=None if gap is None else round(gap, 3))
    return res

def export():
    import adsk.fusion
    em = B.design.exportManager
    files = []
    for b in B.occ_named("Templates").bRepBodies:
        fn = os.path.join(OUTT, "".join(ch if ch.isalnum() else "_" for ch in b.name).strip("_") + ".stl")
        o = em.createSTLExportOptions(b, fn)
        o.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementHigh
        em.execute(o)
        files.append(os.path.basename(fn))
    return files

def renders():
    import adsk.core
    vp = B.app.activeViewport
    P = lambda x, y, z: adsk.core.Point3D.create(x / 10, y / 10, z / 10)
    occ = {o.component.name.replace(B.PREFIX, ""): o for o in B.design.rootComponent.occurrences}
    keep = {"Front shell", "Back cover", "Keycaps", "Window lens", "Solar cell (dummy)", "Battery lid", "Rubber feet", "Templates", "Key labels"}
    old = vp.visualStyle
    shots = []
    try:
        vp.visualStyle = adsk.core.VisualStyles.ShadedWithVisibleEdgesOnlyVisualStyle
        for n, o in occ.items():
            o.isLightBulbOn = n in keep
        for name, eye, tgt, up, ext in [("render_T1_back", (-70, 10, -150), (0, 55, 0), (0, 0, -1), 110),
                                         ("render_T1_top_window", (60, 190, 40), (20, 80, 6), (0, 0, 1), 60),
                                         ("render_T1_front", (40, -60, 150), (0, 55, 8), (0, 0, 1), 110)]:
            cam = vp.camera; cam.isFitView = False; cam.cameraType = adsk.core.CameraTypes.PerspectiveCameraType
            cam.eye, cam.target = P(*eye), P(*tgt); cam.upVector = adsk.core.Vector3D.create(*up); cam.viewExtents = ext / 10
            vp.camera = cam
            for _ in range(2):
                adsk.doEvents(); vp.refresh()
            vp.saveAsImageFile(os.path.join(OUTT, name + ".png"), 1600, 1200)
            shots.append(name)
    finally:
        for o in occ.values():
            o.isLightBulbOn = True
        vp.visualStyle = old
    return shots

def main():
    B.app = app
    B.design = B.find_design()
    B.ARGS = ARGS
    B.wire_replica()
    g = B.geometry_cached()
    if ARGS.get("stage") == "remove":
        for o in list(B.design.rootComponent.occurrences):
            if o.component.name == B.PREFIX + "Templates":
                o.deleteMe()
        print("templates removed"); return
    bodies, G = build(g)
    res = check()
    files = export()
    shots = renders()
    json.dump(dict(time=time.strftime("%Y-%m-%d %H:%M"), check=res, stl=files, renders=shots,
                   params=dict(CLR=CLR, WALL=WALL, PLATE_T=PLATE_T, Y_MIN=Y_MIN, CAM=CAM, notch_x=G["notch_x"], notch_z=G["notch_z"])),
              open(os.path.join(OUTT, "templates_check.json"), "w"), indent=1)
    print(json.dumps(res, indent=1)); print(files); print(shots)

if "ARGS" in globals():
    main()
elif __name__ == "__main__":
    # offline: send this file to Fusion through the bridge (same as fusion_run.py)
    import urllib.request
    script = "ARGS = %r\n" % dict(a.split("=", 1) for a in sys.argv[1:]) + open(os.path.abspath(__file__), encoding="utf8").read()
    secret = open(os.path.join(os.path.expanduser("~"), ".fusion-mcp-secret")).read().strip()
    req = urllib.request.Request("http://127.0.0.1:7654/execute", data=json.dumps({"script": script}).encode(),
                                 headers={"Authorization": "Bearer " + secret, "Content-Type": "application/json"})
    try:
        r = json.load(urllib.request.urlopen(req, timeout=600))
    except Exception as e:
        r = {"error": str(e)}
    print(r.get("result") or r.get("error") or r)
    if r.get("traceback"):
        print(r["traceback"])
