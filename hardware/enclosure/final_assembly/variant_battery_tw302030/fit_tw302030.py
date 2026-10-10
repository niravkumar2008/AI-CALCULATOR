"""Fit check: Taiwoo TW302030 (140 mAh, L 30 (+2/-0.5) x W 20 +-0.5 x T 3.0 +-0.3 incl. PCB, JST-PH leads) in the rev E
battery spot, v14 and v15 final assemblies. Variant only: the baseline files (interference.json, clearances.json,
battery_fit_fusion.json, the .f3d/.step) are not touched, nothing is saved or exported.

    python fit_tw302030.py ver=v14 case=nominal     model + check, results -> <ver folder>/variant_battery_tw302030/results.json
    python fit_tw302030.py ver=v14 case=worst
    python fit_tw302030.py ver=v14 case=revE        same checks for today's #1317 (tape-L reference; nothing modelled)
    python fit_tw302030.py ver=v14 case=off         delete the candidate, show the #1317 again

Placement rule = rev E (battery_upgrade.md section 8): same corner, cell's LEFT edge at front X -28.61 (2.6 from the
corner post), TOP edge at Y 78.58 (0.8 from the top lip); long side left-right, lead end (+X) towards J4. The cell is
modelled ON the 0.1 mm tape (Z 1.1 to 1.1 + T). Leads: same route as the #1317 model (out of the lead end, down beside
it at bay X, over rib A, along under the plug, into the plug from below).
Tape L, same rules: strip 1 (6 x 18) under the lead end, outer edge 1 mm in from the lead edge, 1 mm above the bottom
edge; strip 2 (4 x 16) along the bottom edge, 1 mm in from the left edge, 1 mm above the bottom edge.
Frame: front view mm, X right, Y up, Z = 0 at the outside of the back cover. KiCad x = 150 - X, y = 138.94 - Y.
"""
import json, math, os, sys, time, importlib.util

REPO = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR"
ENC = os.path.join(REPO, "hardware", "enclosure")
SRC = {"v14": os.path.join(ENC, "build_final_assembly.py"),
       "v15": os.path.join(ENC, "final_assembly_v15_lcd", "build_final_assembly_v15.py")}
VDIR = {"v14": os.path.join(ENC, "final_assembly", "variant_battery_tw302030"),
        "v15": os.path.join(ENC, "final_assembly_v15_lcd", "variant_battery_tw302030")}
LOG = os.path.join(VDIR["v14"], "fit.log")
COMP = "TW302030 candidate"

LEFT, TOP = -28.61, 78.58          # rev E corner (the #1317's left and top edges)
Z0 = 1.1                           # floor 1.0 + 0.1 mm tape
CASES = {"nominal": (30.0, 20.0, 3.0), "worst": (32.0, 20.5, 3.3), "revE": (26.02, 19.75, 3.8)}
WIRE_D = 1.1
LEAD_SPEC = (50.0, 80.0)           # order spec: 50-80 mm leads
DCXYX_BAY = (-6.0, 6.0, 138.94 - 116.0, 138.94 - 108.0, 1.0, 7.0)   # v15 DCXYX S-fold bay (KiCad y 108-116, x ~144-156)
# back-cover feature boxes (front X, Y, Z): pieces cut out of the cover body to measure each landmark on its own
FEATURES = {"corner post (top-left screw boss)": (-38.0, -30.0, 69.0, 79.0, 1.05, 9.5),
            "rib A (cross rib below the cell)": (-34.0, 12.0, 56.6, 58.4, 1.05, 4.0),
            "top lip / comb snap (top wall inside)": (-34.0, 12.0, 79.0, 82.5, 1.05, 9.5)}

def log(msg):
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    with open(LOG, "a", encoding="utf8") as f:
        print(time.strftime("%H:%M:%S"), msg, file=f)

def strips(L, W):
    """Tape L for a cell with its left edge at LEFT, top edge at TOP (front X, Y): (x0, x1, y0, y1)."""
    x0, x1, y0, y1 = LEFT, LEFT + L, TOP - W, TOP
    return {"strip 1 (6 x 18, under the lead end)": (x1 - 7.0, x1 - 1.0, y0 + 1.0, y0 + 19.0),
            "strip 2 (4 x 16, along the bottom edge)": (x0 + 1.0, x0 + 17.0, y0 + 1.0, y0 + 5.0)}

# â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
def load_build(ver):
    spec = importlib.util.spec_from_file_location("build_" + ver, SRC[ver])
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    m.app = app
    m.ARGS = {}
    m.design = m.find_design()
    m.wire_replica()
    return m

def proxies(occ_name):
    return list(M.occ_named(occ_name).bRepBodies)

def old_bodies():
    lp = M.occ_named("LiPo battery"); rb = M.occ_named("Ribbons and wires")
    return [b for b in lp.bRepBodies if b.name.startswith("LiPo pouch")] + [b for b in rb.bRepBodies if b.name.startswith("LiPo lead")]

def drop_candidate():
    for o in list(M.design.rootComponent.occurrences):
        if o.component.name == M.PREFIX + COMP:
            o.deleteMe()

def solid(body, x, y, z):
    import adsk.core, adsk.fusion
    return body.pointContainment(adsk.core.Point3D.create(x / 10, y / 10, z / 10)) == adsk.fusion.PointContainment.PointInsidePointContainment

def tape_check(cover, L, W):
    """Each strip: every 0.1 mm point solid at Z 0.95 (floor under it) and free at Z 1.05 (flat floor, nothing on it);
    smallest distance to the lid opening (points of the opening region that are NOT solid at Z 0.95)."""
    hole = []
    ox0, ox1, oy0, oy1 = -26.0, -9.5, 62.0, 79.0
    nx, ny = int((ox1 - ox0) / 0.1) + 1, int((oy1 - oy0) / 0.1) + 1
    for i in range(nx):
        x = ox0 + i * 0.1
        for j in range(ny):
            y = oy0 + j * 0.1
            if not solid(cover, x, y, 0.95):
                hole.append((x, y))
    hb = [min(p[0] for p in hole), max(p[0] for p in hole), min(p[1] for p in hole), max(p[1] for p in hole)] if hole else None
    out = {}
    for nm, (x0, x1, y0, y1) in strips(L, W).items():
        bad_floor, bump = 0, 0
        n = 0
        x = x0
        while x <= x1 + 1e-6:
            y = y0
            while y <= y1 + 1e-6:
                n += 1
                if not solid(cover, x, y, 0.95):
                    bad_floor += 1
                if solid(cover, x, y, 1.05):
                    bump += 1
                y += 0.25
            x += 0.25
        dmin = min((math.hypot(max(x0 - px, 0, px - x1), max(y0 - py, 0, py - y1)) for px, py in hole), default=None)
        out[nm] = dict(rect_X=[round(x0, 2), round(x1, 2)], rect_Y=[round(y0, 2), round(y1, 2)], points=n,
                       points_not_on_solid_floor=bad_floor, points_with_something_on_the_floor=bump,
                       gap_to_lid_opening=None if dmin is None else round(dmin, 2))
    return out, ([round(v, 2) for v in hb] if hb else None)

def measure(a, b):
    try:
        return round(app.measureManager.measureMinimumDistance(a, b).value * 10, 2)
    except Exception:
        return None

def overlap(a, b):
    import adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    x = tbm.copy(a); y = tbm.copy(b)
    try:
        tbm.booleanOperation(x, y, adsk.fusion.BooleanTypes.IntersectionBooleanType)
        return round(sum(x.lumps.item(k).volume for k in range(x.lumps.count)) * 1000, 3) if x.lumps.count else 0.0
    except Exception:
        return None

def piece(src, box, comp, name):
    """A box-shaped piece of `src` (world coords) added as a hidden body of the candidate component (for measuring)."""
    import adsk.core, adsk.fusion
    tbm = adsk.fusion.TemporaryBRepManager.get()
    x0, x1, y0, y1, z0, z1 = box
    a = tbm.copy(src)
    bx = tbm.createBox(adsk.core.OrientedBoundingBox3D.create(M.P((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2),
                       adsk.core.Vector3D.create(1, 0, 0), adsk.core.Vector3D.create(0, 1, 0), (x1 - x0) / 10, (y1 - y0) / 10, (z1 - z0) / 10))
    tbm.booleanOperation(a, bx, adsk.fusion.BooleanTypes.IntersectionBooleanType)
    if not a or a.lumps.count == 0:
        return None
    b = M.tbm_add(comp, [a], name)
    b.isLightBulbOn = False
    return b

def run_case(ver, case):
    L, W, T = CASES[case]
    X, Y = LEFT + L / 2, TOP - W / 2
    M.place({})
    cover = [b for b in proxies("Back cover") if b.name == "Back cover"][0]
    res = dict(case=case, ver=ver, L=L, W=W, T=T, Z=[Z0, round(Z0 + T, 2)], tape_under_cell_mm=0.1,
               cell_front_X=[round(LEFT, 2), round(LEFT + L, 2)], cell_front_Y=[round(TOP - W, 2), TOP],
               cell_kicad_x=[round(150 - LEFT - L, 2), round(150 - LEFT, 2)], cell_kicad_y=[round(138.94 - TOP, 2), round(138.94 - TOP + W, 2)],
               time=time.strftime("%Y-%m-%d %H:%M"))
    res["tape"], res["lid_opening_bbox_at_floor"] = tape_check(cover, L, W)
    if case == "revE":
        return res
    drop_candidate()
    for b in old_bodies():
        b.isLightBulbOn = False
    cc = M.clear_comp(COMP)
    sk = M.new_sketch(cc, "cell")
    M.R.rrect(sk, (X, Y), L, W, 1.0)
    M.R.extrude(cc, M.R.profiles(sk), Z0, T, name="Cell TW302030 %s %.1f x %.1f x %.1f" % (case, L, W, T))
    M.appearance(cc.bRepBodies.item(cc.bRepBodies.count - 1), "Aluminum - Anodized Glossy (Grey)")
    # leads: same construction as the #1317 model (stage parts), re-aimed at this cell's lead end
    plug = [b for b in proxies("LiPo battery") if b.name.startswith("JST-PH plug")][0]
    pb = plug.boundingBox
    xm, pmin, zm = (pb.minPoint.x + pb.maxPoint.x) * 5, pb.minPoint.y * 10, (pb.minPoint.z + pb.maxPoint.z) * 5
    lend = X + L / 2
    bay_X = lend + WIRE_D / 2 + 0.05
    lzm = Z0 + T / 2
    lens = {}
    for nm, dxw, dz in (("red", 1.0, 0.7), ("black", -1.0, -0.7)):
        yl = Y + dxw * 1.6
        pts = [(xm + dxw, pmin - 0.03, zm), (xm + dxw, pmin - 2.0 - (dxw + 1), zm + dz),
               (bay_X, pmin - 2.0 - (dxw + 1), zm + dz), (bay_X, yl, zm + dz), (bay_X, yl, lzm + 0.8 * dz),
               (lend + 0.01, yl, lzm + 0.8 * dz)]
        M.tbm_add(cc, M.tube(pts, WIRE_D), "Lead %s" % nm)
        lens[nm] = round(sum(math.dist(a, b) for a, b in zip(pts, pts[1:])) + 4.0, 1)   # + ~4 mm inside the plug housing
    M.adsk_refresh()
    occ = M.occ_named(COMP)
    cell = [b for b in occ.bRepBodies if b.name.startswith("Cell")][0]
    leads = [b for b in occ.bRepBodies if b.name.startswith("Lead")]
    # every other visible body near the cell: gap + overlap (cell and each lead)
    rows, hits = [], []
    cb = cell.boundingBox
    for c, n, b in M.all_bodies():
        if c in (COMP,) or c.startswith("Slide case") or c.startswith("Magnetic cable plug"):
            continue
        if c == "LiPo battery" and n.startswith("LiPo pouch"):
            continue
        if c == "Ribbons and wires" and n.startswith("LiPo lead"):
            continue
        bb = b.boundingBox
        if bb.minPoint.x * 10 > cb.maxPoint.x * 10 + 8 or bb.maxPoint.x * 10 < cb.minPoint.x * 10 - 8 or \
           bb.minPoint.y * 10 > cb.maxPoint.y * 10 + 8 or bb.maxPoint.y * 10 < cb.minPoint.y * 10 - 8:
            continue
        for wb in [cell] + leads:
            if wb in leads and c == "LiPo battery" and n.startswith("JST-PH plug"):
                continue                                     # the leads end in the plug
            g = measure(wb, b)
            v = overlap(wb, b)
            r = dict(what=wb.name.split(" ")[0] + (" " + wb.name.split(" ")[1] if wb in leads else ""), part=[c, n], gap=g, overlap_mm3=v)
            rows.append(r)
            if v and v > 0.005 and not (wb is cell and c == "Back cover"):
                hits.append(r)
    rows.sort(key=lambda r: (r["gap"] if r["gap"] is not None else 99))
    # named landmarks
    named = {}
    for nm, box in FEATURES.items():
        pc = piece(cover, box, cc, "probe cutter " + nm)
        if pc is not None:
            pp = [b for b in occ.bRepBodies if b.name == "probe cutter " + nm][0]
            named[nm] = dict(cell=measure(cell, pp), leads=min([g for g in (measure(l, pp) for l in leads) if g is not None], default=None))
    def best(cname, pref=None, refname=None):
        gs = [r for r in rows if r["part"][0] == cname and (pref is None or r["part"][1].startswith(pref)) and r["gap"] is not None]
        out = {}
        for who in ("Cell", "Lead red", "Lead black"):
            g = [r["gap"] for r in gs if r["what"] == who]
            if g:
                out["cell" if who == "Cell" else who.lower().replace(" ", "_")] = min(g)
        return out
    named["LR44 holder (front shell, above the cell)"] = best("Front shell")
    named["board (PCB laminate)"] = best("PCB", "board")
    named["J4 (battery socket on the board)"] = best("PCB", "J4")
    named["JST-PH plug in J4"] = best("LiPo battery", "JST-PH")
    for key, cname, pref in (("camera module", "Camera module", ""), ("camera FPC", "Ribbons and wires", "Camera FPC")):
        bs = [b for b in proxies(cname) if b.name.startswith(pref) and b.isLightBulbOn]
        named[key] = dict(cell=min([g for g in (measure(cell, b) for b in bs) if g is not None], default=None),
                          leads=min([g for g in (measure(l, b) for l in leads for b in bs) if g is not None], default=None))
    named["battery lid (sits in the opening, top 0.42 below the floor)"] = best("Battery lid")
    named["back-cover floor (tape layer)"] = dict(cell=round(Z0 - 1.0, 2))
    # lid opening vs the cell outline
    ob = res["lid_opening_bbox_at_floor"]
    if ob:
        named["lid opening (in the floor, the cell bridges it)"] = dict(
            opening_X=ob[:2], opening_Y=ob[2:], from_cell_left_edge=round(ob[0] - LEFT, 2), short_of_lead_edge=round(LEFT + L - ob[1], 2),
            above_cell_bottom=round(ob[2] - (TOP - W), 2), below_cell_top=round(TOP - ob[3], 2))
    if ver == "v15":
        bx = DCXYX_BAY
        dy = (TOP - W) - bx[3]
        named["DCXYX S-fold bay (KiCad y 108-116, variant_camera_dcxyx)"] = dict(cell=round(dy, 2), note="cell bottom edge to the bay's top end, along Y")
    res.update(hits=hits, nearest=[r for r in rows if r["gap"] is not None][:25], named=named,
               leads=dict(bay_X=round(bay_X, 2), plug_X=round(xm, 2), plug_mouth_Y=round(pmin, 2), used_mm=lens,
                          spec_mm=list(LEAD_SPEC), spare_mm=[round(LEAD_SPEC[0] - max(lens.values()), 1), round(LEAD_SPEC[1] - min(lens.values()), 1)]))
    return res

def main(ver, case):
    global LEFT
    shift = float(ARGS.get("shift", 0))
    LEFT = LEFT - shift                               # shift=1: whole cell 1 mm further left (towards the corner post)
    os.makedirs(VDIR[ver], exist_ok=True)
    if case == "off":
        drop_candidate()
        for b in old_bodies():
            b.isLightBulbOn = True
        M.adsk_refresh()
        return "candidate removed, #1317 shown again"
    res = run_case(ver, case)
    fn = os.path.join(VDIR[ver], "results.json")
    allr = json.load(open(fn)) if os.path.exists(fn) else {}
    res["shift_left_mm"] = shift
    allr[case + ("_shift%g" % shift if shift else "")] = res
    json.dump(allr, open(fn, "w"), indent=1)
    return "%s %s: %d overlaps" % (ver, case, len(res.get("hits", [])))

# â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
def send(args):
    import urllib.request, urllib.error
    script = "ARGS = %r\n" % args + open(os.path.abspath(__file__), encoding="utf8").read()
    secret = open(os.path.join(os.path.expanduser("~"), ".fusion-mcp-secret")).read().strip()
    req = urllib.request.Request("http://127.0.0.1:7654/execute", data=json.dumps({"script": script}).encode(),
                                 headers={"Authorization": "Bearer " + secret, "Content-Type": "application/json"})
    start = os.path.getsize(LOG) if os.path.exists(LOG) else 0
    tag = "run %s %s %s" % (args.get("ver"), args.get("case"), args.get("shift", ""))
    try:
        r = json.load(urllib.request.urlopen(req, timeout=60))
    except urllib.error.HTTPError as e:
        r = json.load(e)
    except Exception as e:
        r = {"error": "timeout: %s" % e}
    if "timeout" in str(r.get("error", "")).lower():
        tail = lambda: open(LOG, "rb").read()[start:].decode("utf8", "replace") if os.path.exists(LOG) else ""
        t0 = time.time()
        while tag + " done" not in tail() and "FAILED" not in tail() and time.time() - t0 < 3600:
            time.sleep(4)
        print(tail()[-3000:])
        return
    print(r.get("result") or r)

if "ARGS" in globals():
    import adsk.core, adsk.fusion, traceback
    tag = "run %s %s %s" % (ARGS.get("ver"), ARGS.get("case"), ARGS.get("shift", ""))
    log(tag)
    try:
        M = load_build(ARGS["ver"])
        msg = main(ARGS["ver"], ARGS["case"])
        log(msg)
        print(msg)
    except Exception:
        log(tag + " FAILED " + traceback.format_exc())
        raise
    log(tag + " done")
elif __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    send(dict(a.split("=", 1) for a in sys.argv[1:] if "=" in a))
