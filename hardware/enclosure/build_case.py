"""AI Calculator custom case, built in Fusion 360 from the KiCad board.

Run stage by stage through the FusionMCPBridge add-in:
    python fusion_run.py build_case.py stage=shells     (new document, outer shells)
    ... stage=front | back | keymat | legends | parts | check | export | shots
Run locally with plain `python build_case.py` to check the outline against the board.

Frame: millimetres, front view. X = 150 - x_kicad (the KiCad board is drawn from
the back, so X is mirrored), Y = 138.94 - y_kicad (up = towards the display),
Z = 0 at the outside of the back cover, front face at Z = T.
"""
import math, os, re

REPO = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR"
PCB_FILE = os.path.join(REPO, "hardware", "kicad", "ai_calc.kicad_pcb")
OUT = os.path.join(REPO, "hardware", "enclosure")
CX, CY = 150.0, 138.94

# ── Design parameters (mm) ────────────────────────────────────────────────────
CLR = 0.5         # board edge to inner wall
WALL = 2.2        # side wall
FLOOR = 2.0       # back cover thickness
PLATE = 2.0       # front plate thickness
Z_SPLIT = 7.0     # parting line = PCB back face
PCB_T = 0.8
Z_PCB_F = Z_SPLIT + PCB_T          # 7.8, PCB front face (keypads, e-paper)
T = 13.8                            # total thickness = fx-115ES (Casio spec 13.8; calipers D10 override)
Z_PLATE = T - PLATE                 # 11.6, underside of the front plate
LIP_H, LIP_T, LIP_GAP = 1.5, 1.1, 0.1
EDGE_R = 1.5      # outer edge fillet, front and back
KEY_CLR = 0.5     # key hole = pad + 2*KEY_CLR
CAP_CLR = 0.25    # cap = hole - 2*CAP_CLR
CAP_PROUD = 1.2   # cap top above the front face
SHEET_T = 0.8     # keymat sheet
TRAVEL = 0.6      # plunger to keypad gap
EPD_T = 1.0       # e-paper module thickness (check on the real panel)
LENS_T = 1.0      # laser-cut acrylic window
SCREW_D, SCREW_HEAD_D, SCREW_HEAD_H, PILOT_D = 2.4, 4.5, 1.5, 1.7   # M2 x 10
OUT_TOP_R, OUT_BOT_R, TAPER_R = 12.0, 7.0, 15.0

def F(x, y):
    """KiCad point -> front-view point."""
    return (CX - x, CY - y)

# ── KiCad board ───────────────────────────────────────────────────────────────
def _blocks(s, tag):
    for m in re.finditer(r"\(" + tag + r"\s", s):
        i = m.start()
        d, j = 0, i
        while True:
            if s[j] == '"':                      # skip strings: labels like "(" and "(-)"
                j += 1
                while s[j] != '"':
                    j += 2 if s[j] == "\\" else 1
            elif s[j] == "(":
                d += 1
            elif s[j] == ")":
                d -= 1
                if d == 0:
                    break
            j += 1
        yield s[i:j + 1]

def _nums(pat, b):
    return tuple(map(float, re.search(pat, b).group(1).split()))

def read_board():
    s = open(PCB_FILE, encoding="utf8").read()
    layer = lambda b: re.search(r'\(layer "([^"]+)"', b).group(1)
    # board outline: chain Edge.Cuts lines into one loop
    segs = []
    for b in _blocks(s, "gr_line"):
        if layer(b) == "Edge.Cuts":
            segs.append((_nums(r"\(start ([-\d. ]+)\)", b), _nums(r"\(end ([-\d. ]+)\)", b)))
    key = lambda p: (round(p[0], 3), round(p[1], 3))
    segs.sort(key=lambda sg: min(sg[0][1], sg[1][1]))     # start on the top edge, not the e-paper slot
    loop, rest = [segs[0][0], segs[0][1]], segs[1:]
    while rest:
        end = key(loop[-1])
        if len(loop) > 2 and end == key(loop[0]):        # outline closed; the rest is the e-paper slot
            break
        for i, (a, b) in enumerate(rest):
            if key(a) == end or key(b) == end:
                loop.append(b if key(a) == end else a)
                rest.pop(i)
                break
        else:
            raise ValueError("Edge.Cuts is not one closed loop near %s" % (end,))
    board = [F(*p) for p in loop[:-1]]

    texts, rects, circles = {}, [], []
    for b in _blocks(s, "gr_text"):
        texts.setdefault(layer(b), []).append((b.split('"')[1], _nums(r"\(at ([-\d.]+ [-\d.]+)", b)))
    for b in _blocks(s, "gr_rect"):
        (x1, y1), (x2, y2) = _nums(r"\(start ([-\d. ]+)\)", b), _nums(r"\(end ([-\d. ]+)\)", b)
        rects.append((layer(b), F((x1 + x2) / 2, (y1 + y2) / 2), round(abs(x2 - x1), 2), round(abs(y2 - y1), 2)))
    for b in _blocks(s, "gr_circle"):
        c, e = _nums(r"\(center ([-\d. ]+)\)", b), _nums(r"\(end ([-\d. ]+)\)", b)
        circles.append((layer(b), F(*c), round(math.dist(c, e), 2)))

    # keypads: User.2 rects, named by the label at the same centre
    keys = []
    for L, c, w, h in rects:
        if L != "User.2":
            continue
        name = min(texts["User.2"], key=lambda t: math.dist(F(*t[1]), c))[0]
        keys.append((name, c, w, h))

    holes = []
    for b in _blocks(s, "footprint"):
        m = re.search(r'Post_Hole_([\d.]+)mm|ScrewBoss_Hole_([\d.]+)mm', b)
        if m:
            ref = re.search(r'\(property "Reference" "([^"]+)"', b).group(1)
            holes.append((ref, F(*_nums(r"\(at ([-\d.]+ [-\d.]+)", b)), float(m.group(1) or m.group(2)),
                          "post" if m.group(1) else "screw"))

    ko = {(w, h): c for L, c, w, h in rects if L == "User.3"}
    pins = [c for L, c, r in circles if L == "User.3" and r == 2.5]      # back-cover pins
    cam = [c for L, c, r in circles if L == "User.3" and r == 3.0][0]
    notches = [(c, r) for L, c, r in circles if L == "User.1"]           # top screws (board notch / cut-out)
    return dict(board=board, keys=keys, holes=holes, pins=pins, cam=cam, notches=TOP_SCREWS_115,  # was the fx-300ES notches
                epd=ko[(59.2, 29.2)], active=ko[(48.55, 23.7)], cam_ko=ko[(12.0, 12.0)],
                batt=ko[(31.0, 11.5)], magnet=ko[(21.5, 7.5)])

# Bottom screws sit in the two r=4 notches of the board's bottom edge (fitted from Edge.Cuts).
BOTTOM_SCREWS = [(F(130.53, 210.20), 4.0), (F(169.47, 209.934), 4.0)]

# ── fx-115ES shell geometry (stage 10, from Nirav's photos; see hardware/stage10_fx115es.md) ─────
# Top-corner screw posts: the fx-115ES ones sit ~4 mm higher than the fx-300ES notches on the board.
TOP_SCREWS_115 = [(F(117.8, 65.6), 3.0), (F(180.55, 65.8), 3.0)]
# Key openings in the front plate (w, h), measured on the fx-115ES shell; keyed by keypad width class.
KEY_OPEN = {"oval": (8.3, 5.6), 6.0: (8.1, 6.0), 7.0: (8.9, 5.9), 9.0: (11.8, 8.2)}
OVAL_KEYS = ("SHIFT", "ALPHA", "MODE", "ON")
WINDOW_115 = (60.65, 24.3)           # display opening, calipers C12 (photos said ~60 x 22)
WINDOW_TOP_FROM_EDGE = 24.0          # calipers C13: case top edge -> window top edge
MEAS_W, MEAS_L = 79.3, 161.0         # calipers C2/C3 (outer width, back-cover length); the photo trace is scaled to these
PIN_D = 3.85                         # calipers C7: fx-115ES post diameter (all the same)
SOLAR_115 = (F(139.15, 71.6), 34.0, 11.0)   # solar-cell window -> shallow decorative recess (no cell)

def fx115_outline():
    """fx-115ES silhouette traced from photo 88e03c27 (enclosure/fx115es_outline.json), front-view frame, CCW."""
    import json
    d = json.load(open(os.path.join(OUT, "fx115es_outline.json")))
    xs = [p[0] for p in d["outline"]]; ys = [p[1] for p in d["outline"]]
    sx, sy = MEAS_W / (max(xs) - min(xs)), MEAS_L / (max(ys) - min(ys))   # calipers win over the photo
    yc = (max(ys) + min(ys)) / 2
    V = [F(150 + (x - 150) * sx, yc + (y - yc) * sy) for x, y in d["outline"]]
    V = [p for i, p in enumerate(V) if math.dist(p, V[i - 1]) > 0.3]       # drop near-duplicates
    changed = True
    while changed:                                                         # drop (near-)collinear points
        changed = False
        for i in range(len(V)):
            a, p, c = V[i - 1], V[i], V[(i + 1) % len(V)]
            cr = (p[0] - a[0]) * (c[1] - p[1]) - (p[1] - a[1]) * (c[0] - p[0])
            if abs(cr) < 0.02 * math.dist(a, p) * math.dist(p, c):        # < ~1.1 degrees of turn
                V.pop(i); changed = True; break
    area = sum(V[i - 1][0] * V[i][1] - V[i][0] * V[i - 1][1] for i in range(len(V)))
    return (V if area > 0 else V[::-1]), [0.0] * len(V)

def window_centre(b):
    """fx-115ES display opening: C13 below the case top edge, centred left-right."""
    V, _ = outline(b["board"])
    return (0.0, max(y for x, y in V) - WINDOW_TOP_FROM_EDGE - WINDOW_115[1] / 2)

def key_open(name, w, h):
    return KEY_OPEN["oval"] if name in OVAL_KEYS else KEY_OPEN.get(round(w), (w + 2 * KEY_CLR, h + 2 * KEY_CLR))

# ── 2D geometry: polygons with filleted corners ───────────────────────────────
def _unit(v):
    l = math.hypot(*v)
    return (v[0] / l, v[1] / l)

def offset_poly(V, R, d):
    """Offset a CCW filleted polygon inwards by d (exact for lines + arcs)."""
    n = len(V)
    lines = []
    for i in range(n):
        a, b = V[i], V[(i + 1) % n]
        u = _unit((b[0] - a[0], b[1] - a[1]))
        nrm = (-u[1], u[0])
        lines.append(((a[0] + nrm[0] * d, a[1] + nrm[1] * d), u))
    V2, R2 = [], []
    for i in range(n):
        (p, u), (q, w) = lines[i - 1], lines[i]
        den = u[0] * w[1] - u[1] * w[0]
        t = ((q[0] - p[0]) * w[1] - (q[1] - p[1]) * w[0]) / den
        V2.append((p[0] + u[0] * t, p[1] + u[1] * t))
        R2.append(R[i] - d if den > 0 else R[i] + d)
    return V2, R2

def fillet_segments(V, R):
    """[('L', p0, p1) | ('A', p0, pmid, p1)] for a closed polygon with corner radii."""
    n, corners = len(V), []
    for i in range(n):
        v, r = V[i], R[i]
        a = _unit((V[i - 1][0] - v[0], V[i - 1][1] - v[1]))
        b = _unit((V[(i + 1) % n][0] - v[0], V[(i + 1) % n][1] - v[1]))
        if r <= 0:
            corners.append((v, None, v))
            continue
        alpha = math.acos(max(-1, min(1, a[0] * b[0] + a[1] * b[1])))
        t = r / math.tan(alpha / 2)
        bis = _unit((a[0] + b[0], a[1] + b[1]))
        c = (v[0] + bis[0] * r / math.sin(alpha / 2), v[1] + bis[1] * r / math.sin(alpha / 2))
        corners.append(((v[0] + a[0] * t, v[1] + a[1] * t), (c[0] - bis[0] * r, c[1] - bis[1] * r),
                        (v[0] + b[0] * t, v[1] + b[1] * t)))
    segs = []
    for i in range(n):
        t1, mid, t2 = corners[i]
        if mid:
            segs.append(("A", t1, mid, t2))
        nxt = corners[(i + 1) % n][0]
        if math.dist(t2, nxt) > 1e-6:
            segs.append(("L", t2, nxt))
    return segs

def rrect(c, w, h, r):
    x, y = c
    V = [(x + w / 2, y - h / 2), (x + w / 2, y + h / 2), (x - w / 2, y + h / 2), (x - w / 2, y - h / 2)]
    return fillet_segments(V, [min(r, w / 2 - 0.01, h / 2 - 0.01)] * 4)

def outline(board):
    """fx-115ES outline when the traced file is there, else the board-derived Casio-like one."""
    if os.path.exists(os.path.join(OUT, "fx115es_outline.json")):
        return fx115_outline()
    return outline_from_board(board)

def outline_from_board(board):
    """Symmetric Casio-like outline: wide top, short taper, narrow bottom."""
    o = CLR + WALL
    xs = [abs(x) for x, y in board]
    top_hw = max(xs) + o                                   # 38.86
    bot_hw = max(abs(x) for x, y in board if y < -5) + o   # 34.77
    ytop = max(y for x, y in board) + o
    # bottom: clear the board and the bottom screw columns
    ybot = min(min(y for x, y in board) - o, min(c[1] for c, r in BOTTOM_SCREWS) - r_col("bottom") - 1.8)
    # taper: offset the board's taper line (top-section corner -> bottom-section corner) by o
    p1 = max((p for p in board if p[0] > 0 and p[1] < 30), key=lambda p: (round(p[0], 1), p[1]))
    p2 = max((p for p in board if p[0] > 0 and -10 < p[1] < 0), key=lambda p: -p[1])
    d = _unit((p2[0] - p1[0], p2[1] - p1[1]))
    nrm = (-d[1], d[0]) if -d[1] > 0 else (d[1], -d[0])
    # the board isn't symmetric: push the line out past the bulgier side too
    bulge = max(0, max((abs(x) - p1[0]) * nrm[0] + (y - p1[1]) * nrm[1]
                       for x, y in board if p2[1] - 1 < y < p1[1] + 1))
    q = (p1[0] + nrm[0] * (o + bulge), p1[1] + nrm[1] * (o + bulge))
    y_at = lambda X: q[1] + (X - q[0]) / d[0] * d[1]
    y1, y2 = y_at(top_hw), y_at(bot_hw)
    V = [(bot_hw, ybot), (bot_hw, y2), (top_hw, y1), (top_hw, ytop),
         (-top_hw, ytop), (-top_hw, y1), (-bot_hw, y2), (-bot_hw, ybot)]
    R = [OUT_BOT_R, TAPER_R, TAPER_R, OUT_TOP_R, OUT_TOP_R, TAPER_R, TAPER_R, OUT_BOT_R]
    return V, R

def r_col(kind):
    return {"bottom": 3.5, "lr44": 2.6, "cutout": 2.9, "mid": 2.7}[kind]

def sample(segs, step=0.25):
    pts = []
    for s in segs:
        if s[0] == "L":
            n = max(1, int(math.dist(s[1], s[2]) / step))
            pts += [(s[1][0] + (s[2][0] - s[1][0]) * k / n, s[1][1] + (s[2][1] - s[1][1]) * k / n) for k in range(n)]
        else:                                   # arc through 3 points
            (ax, ay), (bx, by), (cx, cy) = s[1], s[2], s[3]
            d = 2 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by))
            ux = ((ax**2 + ay**2) * (by - cy) + (bx**2 + by**2) * (cy - ay) + (cx**2 + cy**2) * (ay - by)) / d
            uy = ((ax**2 + ay**2) * (cx - bx) + (bx**2 + by**2) * (ax - cx) + (cx**2 + cy**2) * (bx - ax)) / d
            r = math.dist((ux, uy), s[1])
            a0, am, a1 = (math.atan2(p[1] - uy, p[0] - ux) for p in s[1:])
            sweep = (a1 - a0) % (2 * math.pi)
            if (am - a0) % (2 * math.pi) > sweep:
                sweep -= 2 * math.pi
            n = max(2, int(abs(sweep) * r / step))
            pts += [(ux + r * math.cos(a0 + sweep * k / n), uy + r * math.sin(a0 + sweep * k / n)) for k in range(n)]
    return pts

def inside(p, poly):
    x, y, c = p[0], p[1], False
    for i in range(len(poly)):
        (x1, y1), (x2, y2) = poly[i - 1], poly[i]
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            c = not c
    return c

def check_outline(b):
    V, R = outline(b["board"])
    inner = sample(fillet_segments(*offset_poly(V, R, WALL)), 0.1)
    gap = min(math.dist(p, q) for p in b["board"] for q in inner)
    assert all(inside(p, inner) for p in b["board"]), "board pokes through the inner wall"
    assert gap >= CLR - 0.05, "board-to-wall gap %.2f < %.2f" % (gap, CLR)
    return V, R, gap

# ═════════════════════════════════════════════════════════════════════════════
# Fusion side
# ═════════════════════════════════════════════════════════════════════════════
def cm(v):
    return v / 10.0

def P(x, y, z=0.0):
    import adsk.core
    return adsk.core.Point3D.create(cm(x), cm(y), cm(z))

def comp_named(name, create=False):
    import adsk.core
    root = design.rootComponent
    for occ in root.occurrences:
        if occ.component.name == name:
            return occ.component
    if create:
        occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        occ.component.name = name
        return occ.component
    raise KeyError(name)

def body_named(comp, name):
    for b in comp.bRepBodies:
        if b.name == name:
            return b
    raise KeyError(name)

def new_sketch(comp, name):
    sk = comp.sketches.add(comp.xYConstructionPlane)
    sk.name = name
    sk.isComputeDeferred = True
    return sk

def draw(sk, segs):
    L, A = sk.sketchCurves.sketchLines, sk.sketchCurves.sketchArcs
    for s in segs:
        if s[0] == "L":
            L.addByTwoPoints(P(*s[1]), P(*s[2]))
        else:
            A.addByThreePoints(P(*s[1]), P(*s[2]), P(*s[3]))

def circle(sk, c, d):
    sk.sketchCurves.sketchCircles.addByCenterRadius(P(*c), cm(d / 2))

def profiles(sk, loops=None):
    import adsk.core
    sk.isComputeDeferred = False
    col = adsk.core.ObjectCollection.create()
    for p in sk.profiles:
        if loops is None or p.profileLoops.count == loops:
            col.add(p)
    if col.count == 0:
        raise RuntimeError("sketch %s formed no profiles" % sk.name)
    return col

def smallest_profiles(sk, n):
    """The n smallest-area profiles (the holes when circles overlap other sketch geometry)."""
    import adsk.core
    sk.isComputeDeferred = False
    ps = sorted(sk.profiles, key=lambda p: p.areaProperties().area)[:n]
    col = adsk.core.ObjectCollection.create()
    for p in ps:
        col.add(p)
    return col

def log(msg):
    """Progress log, readable while Fusion is busy (the bridge only answers when a stage ends)."""
    import time
    with open(os.path.join(OUT, "build.log"), "a", encoding="utf8") as f:
        print(time.strftime("%H:%M:%S"), msg, file=f)

def extrude(comp, prof, z0, h, op="new", bodies=None, name=None, taper=0.0):
    import adsk.core, adsk.fusion
    log("extrude %s %s z0=%.2f h=%.2f" % (comp.name, op, z0, h))
    ops = {"new": adsk.fusion.FeatureOperations.NewBodyFeatureOperation,
           "cut": adsk.fusion.FeatureOperations.CutFeatureOperation,
           "join": adsk.fusion.FeatureOperations.JoinFeatureOperation}
    ex = comp.features.extrudeFeatures
    inp = ex.createInput(prof, ops[op])
    inp.startExtent = adsk.fusion.OffsetStartDefinition.create(adsk.core.ValueInput.createByReal(cm(z0)))
    if taper:
        inp.setOneSideExtent(adsk.fusion.DistanceExtentDefinition.create(adsk.core.ValueInput.createByReal(cm(h))),
                             adsk.fusion.ExtentDirections.PositiveExtentDirection,
                             adsk.core.ValueInput.createByString("%g deg" % taper))
    else:
        inp.setDistanceExtent(False, adsk.core.ValueInput.createByReal(cm(h)))
    if bodies:
        inp.participantBodies = list(bodies)
    f = ex.add(inp)
    if name and op == "new":
        f.bodies.item(0).name = name
    return f

def edge_loop_at(body, z, outer=True):
    """Edges of the outer loop of the planar face(s) at height z."""
    import adsk.core
    col = adsk.core.ObjectCollection.create()
    for f in body.faces:
        g = f.geometry
        if isinstance(g, adsk.core.Plane) and abs(abs(g.normal.z) - 1) < 1e-6 \
                and abs(f.pointOnFace.z - cm(z)) < 1e-5:
            for lp in f.loops:
                if lp.isOuter == outer:
                    for e in lp.edges:
                        col.add(e)
    return col

def fillet(comp, edges, r):
    import adsk.core
    log("fillet %s %d edges r=%.2f" % (comp.name, edges.count, r))
    fi = comp.features.filletFeatures.createInput()
    v = adsk.core.ValueInput.createByReal(cm(r))
    try:
        fi.edgeSetInputs.addConstantRadiusEdgeSet(edges, v, True)
    except AttributeError:
        fi.addConstantRadiusEdgeSet(edges, v, True)
    return comp.features.filletFeatures.add(fi)

def chamfer(comp, edges, d):
    import adsk.core
    ci = comp.features.chamferFeatures.createInput2()
    ci.chamferEdgeSets.addEqualDistanceChamferEdgeSet(edges, adsk.core.ValueInput.createByReal(cm(d)), True)
    return comp.features.chamferFeatures.add(ci)

def text(sk, s, c, h, w, flip=False):
    import adsk.core
    ti = sk.sketchTexts.createInput2(s, cm(h))
    ti.setAsMultiLine(P(c[0] - w / 2, c[1] - h), P(c[0] + w / 2, c[1] + h),
                      adsk.core.HorizontalAlignments.CenterHorizontalAlignment,
                      adsk.core.VerticalAlignments.MiddleVerticalAlignment, 0)
    ti.fontName = "Arial"
    if flip:
        ti.isHorizontalFlip = True
    return sk.sketchTexts.add(ti)

def appearance(body, *words):
    try:
        lib = app.materialLibraries.itemByName("Fusion Appearance Library")
        for i in range(lib.appearances.count):
            a = lib.appearances.item(i)
            if a.name == words[0] if len(words) == 1 else all(w.lower() in a.name.lower() for w in words):
                local = design.appearances.itemByName(a.name) or design.appearances.addByCopy(a, a.name)
                body.appearance = local
                return a.name
    except Exception:
        pass

# ── Stages ────────────────────────────────────────────────────────────────────
def stage_shells(b):
    import adsk.core
    global design
    for d in list(app.documents):                # close earlier builds (never the user's own designs)
        p = adsk.fusion.Design.cast(d.products.itemByProductType("DesignProductType"))
        if p and any(o.component.name == "Front shell" for o in p.rootComponent.occurrences):
            d.close(False)
    doc = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
    design = adsk.fusion.Design.cast(app.activeProduct)
    V, R = outline(b["board"])
    outer = fillet_segments(V, R)
    inner = fillet_segments(*offset_poly(V, R, WALL))

    front = comp_named("Front shell", True)
    sk = new_sketch(front, "outline")
    draw(sk, outer)
    extrude(front, profiles(sk), Z_SPLIT, T - Z_SPLIT, name="Front shell")
    fb = body_named(front, "Front shell")
    sk = new_sketch(front, "cavity")
    draw(sk, inner)
    extrude(front, profiles(sk), Z_SPLIT, Z_PLATE - Z_SPLIT, "cut", [fb])
    sk = new_sketch(front, "groove")              # receives the back cover's lip
    draw(sk, fillet_segments(*offset_poly(V, R, WALL - LIP_T - LIP_GAP)))
    draw(sk, inner)
    extrude(front, profiles(sk, 2), Z_SPLIT, LIP_H + LIP_GAP, "cut", [fb])
    fillet(front, edge_loop_at(fb, T), EDGE_R)

    back = comp_named("Back cover", True)
    sk = new_sketch(back, "outline")
    draw(sk, outer)
    extrude(back, profiles(sk), 0, Z_SPLIT, name="Back cover")
    bb = body_named(back, "Back cover")
    sk = new_sketch(back, "cavity")
    draw(sk, inner)
    extrude(back, profiles(sk), FLOOR, Z_SPLIT - FLOOR, "cut", [bb])
    sk = new_sketch(back, "lip")
    draw(sk, fillet_segments(*offset_poly(V, R, WALL - LIP_T)))
    draw(sk, inner)
    extrude(back, profiles(sk, 2), Z_SPLIT, LIP_H, "join", [bb])
    fillet(back, edge_loop_at(bb, 0), EDGE_R)
    print("shells ok; size %s x %.1f mm" % ("%.1f x %.1f" % (max(p[0] for p in V) - min(p[0] for p in V), max(p[1] for p in V) - min(p[1] for p in V)), T))

def screws(b):
    """(centre, front column radius, back boss radius, kind) for all six M2 screws."""
    out = []
    for ref, c, d, kind in b["holes"]:
        if kind == "screw":
            out.append((c, d / 2 - 0.3, d / 2 + 1.0, "mid"))           # through the board's 6 mm holes
    for c, r in b["notches"]:
        if c[0] > 0:
            out.append((c, r_col("lr44"), r_col("lr44") + 1.2, "lr44"))  # board notch around it
        else:
            out.append((c, r_col("cutout"), r_col("cutout"), "cutout"))  # in the battery cut-out
    for c, r in BOTTOM_SCREWS:
        out.append((c, r_col("bottom"), r_col("bottom") + 0.75, "bottom"))
    return out

def stage_front(b):
    front = comp_named("Front shell")
    fb = body_named(front, "Front shell")
    epd, act = b["epd"], b["active"]

    # display: flush lens recess, window, and a frame that holds the e-paper on the board
    sk = new_sketch(front, "display frame")
    draw(sk, rrect(epd, 58.0, 28.0, 1.0))
    extrude(front, profiles(sk), Z_PCB_F + EPD_T + 0.3, Z_PLATE - (Z_PCB_F + EPD_T + 0.3), "join", [fb])
    sk = new_sketch(front, "window")
    draw(sk, rrect(window_centre(b), WINDOW_115[0], WINDOW_115[1], 1.0))
    extrude(front, profiles(sk), Z_PCB_F, T - Z_PCB_F, "cut", [fb])
    sk = new_sketch(front, "lens recess")
    draw(sk, rrect(window_centre(b), WINDOW_115[0] + 4.0, WINDOW_115[1] + 4.0, 2.0))
    extrude(front, profiles(sk), T - LENS_T, LENS_T + 0.1, "cut", [fb])

    # screw columns (pilot holes for M2 self-tappers from the back) and locating pins
    sk = new_sketch(front, "columns")
    for c, r, _, kind in screws(b):
        circle(sk, c, 2 * r if kind != "mid" else 2 * r + 1.0)        # mid: shoulder sits on the board
    extrude(front, profiles(sk), Z_PCB_F, Z_PLATE - Z_PCB_F, "join", [fb])
    sk = new_sketch(front, "column tips")
    for c, r, _, kind in screws(b):
        circle(sk, c, 2 * r)
    extrude(front, profiles(sk), Z_SPLIT, PCB_T, "join", [fb])
    sk = new_sketch(front, "pilots")
    for c, *_ in screws(b):
        circle(sk, c, PILOT_D)
    extrude(front, smallest_profiles(sk, len(screws(b))), Z_SPLIT, 5.0, "cut", [fb])
    sk = new_sketch(front, "pins")
    for ref, c, d, kind in b["holes"]:
        if kind == "post":
            circle(sk, c, PIN_D)                                     # the real fx-115ES post size
    extrude(front, profiles(sk), Z_SPLIT, Z_PLATE - Z_SPLIT, "join", [fb])

    # key holes
    sk = new_sketch(front, "key holes")
    for name, c, w, h in b["keys"]:
        if name not in ("UP", "DOWN", "LEFT", "RIGHT"):
            ow, oh = key_open(name, w, h)
            draw(sk, rrect(c, ow, oh, oh / 2 if name in OVAL_KEYS else (1.5 if w < 8 else 1.8)))
    circle(sk, replay_centre(b), REPLAY_D)
    extrude(front, profiles(sk), Z_PLATE - 0.1, PLATE + 0.2, "cut", [fb])

    # name plate between display and top edge
    sk = new_sketch(front, "brand")
    text(sk, "AI CALCULATOR", (-17.0, SOLAR_115[0][1] + 2.0), 3.2, 34)
    extrude(front, sk.sketchTexts.item(0), T - 0.3, 0.4, "cut", [fb])
    sk = new_sketch(front, "solar recess")             # fx-115ES solar window, kept as a shallow panel
    draw(sk, rrect(SOLAR_115[0], SOLAR_115[1], SOLAR_115[2], 0.8))
    extrude(front, profiles(sk), T - 0.5, 0.6, "cut", [fb])
    print("front ok")

REPLAY_D = 15.4         # fx-115ES round-pad opening
CROWN_TAPER = 20.0     # degrees, bevel on the part of each cap above the plate
def replay_centre(b):
    pts = [c for n, c, w, h in b["keys"] if n in ("UP", "DOWN", "LEFT", "RIGHT")]
    return (sum(p[0] for p in pts) / 4, sum(p[1] for p in pts) / 4)

def stage_back(b):
    back = comp_named("Back cover")
    bb = body_named(back, "Back cover")
    sc = screws(b)

    # bosses under the six screws, posts under the 10 locating pins and the 5 back-cover pins
    sk = new_sketch(back, "bosses")
    for c, _, rb, _ in sc:
        circle(sk, c, 2 * rb)
    extrude(back, profiles(sk), FLOOR, Z_SPLIT - FLOOR, "join", [bb])
    sk = new_sketch(back, "posts")
    for ref, c, d, kind in b["holes"]:
        if kind == "post":
            circle(sk, c, 6.0)
    for c in b["pins"]:
        circle(sk, c, 4.0)
    extrude(back, profiles(sk), FLOOR, Z_SPLIT - FLOOR, "join", [bb])

    # let the lip pass the screw columns that sit against the wall
    sk = new_sketch(back, "lip reliefs")
    for c, rf, _, _ in sc:
        circle(sk, c, 2 * rf + 0.6)
    extrude(back, profiles(sk), Z_SPLIT, LIP_H + 0.1, "cut", [bb])

    # screw clearance + counterbores from outside
    sk = new_sketch(back, "screw holes")
    for c, *_ in sc:
        circle(sk, c, SCREW_D)
    extrude(back, smallest_profiles(sk, len(sc)), 0, Z_SPLIT, "cut", [bb])
    sk = new_sketch(back, "counterbores")
    for c, *_ in sc:
        circle(sk, c, SCREW_HEAD_D)
    extrude(back, smallest_profiles(sk, len(sc)), 0, SCREW_HEAD_H, "cut", [bb])

    # camera window with a chamfered entry
    sk = new_sketch(back, "camera")
    circle(sk, b["cam"], 6.0)
    extrude(back, smallest_profiles(sk, 1), 0, FLOOR, "cut", [bb])
    sk = new_sketch(back, "camera bezel")
    circle(sk, b["cam"], 8.0)
    extrude(back, smallest_profiles(sk, 1), 0, 0.6, "cut", [bb])
    sk = new_sketch(back, "camera guide")                 # locates the 12 x 12 module
    ck = b["cam_ko"]
    draw(sk, rrect(ck, 12.4 + 2.0, 12.4 + 2.0, 1.5))
    draw(sk, rrect(ck, 12.4, 12.4, 0.5))
    extrude(back, profiles(sk, 2), FLOOR, 1.2, "join", [bb])

    # magnetic connector slot through the top wall (both covers)
    mg = b["magnet"]
    top = max(y for x, y in b["board"]) + CLR + WALL
    sk = new_sketch(back, "magnet slot")
    draw(sk, rrect((mg[0], top - 1.5), 22.0, 6.0, 0.0))
    prof = profiles(sk)
    extrude(back, prof, FLOOR + 0.5, Z_SPLIT + LIP_H - FLOOR - 0.5, "cut", [bb])
    front = comp_named("Front shell")
    sk = new_sketch(front, "magnet slot")
    draw(sk, rrect((mg[0], top - 1.5), 22.0, 6.0, 0.0))
    extrude(front, profiles(sk), Z_SPLIT, LIP_H + LIP_GAP + 0.4, "cut", [body_named(front, "Front shell")])

    # battery cradle (LiPo 31 x 11.5 x 3.8), open at one end for the leads
    bt = b["batt"]
    sk = new_sketch(back, "battery cradle")
    draw(sk, rrect(bt, 31.6 + 1.6, 12.1 + 1.6, 0.8))
    draw(sk, rrect(bt, 31.6, 12.1, 0.3))
    extrude(back, profiles(sk, 2), FLOOR, 1.5, "join", [bb])
    sk = new_sketch(back, "battery lead gap")
    draw(sk, rrect((bt[0] + 16.3, bt[1]), 3.0, 5.0, 0))
    extrude(back, profiles(sk), FLOOR, 1.6, "cut", [bb])

    # rubber-foot pockets (8 mm self-adhesive bumpers) and the label
    sk = new_sketch(back, "feet")
    for c in [(25, 55), (-25, 55), (24, -60), (-24, -60)]:
        circle(sk, c, 8.4)
    extrude(back, smallest_profiles(sk, 4), 0, 0.6, "cut", [bb])
    sk = new_sketch(back, "label")
    text(sk, "AI CALCULATOR", (0, -18), 4.0, 50, flip=True)
    text(sk, "rev A   6 x M2x10", (0, -26), 2.2, 40, flip=True)
    for i in range(sk.sketchTexts.count):
        extrude(back, sk.sketchTexts.item(i), 0, 0.4, "cut", [bb])
    print("back ok")

def stage_keymat(b):
    km = comp_named("Keymat", True)
    V, R = outline(b["board"])
    z0 = Z_PLATE - SHEET_T                      # sheet underside
    top_y = b["epd"][1] - 14.0 - 1.6            # stop below the display frame

    sk = new_sketch(km, "sheet")
    draw(sk, fillet_segments(*offset_poly(V, R, WALL + 0.4)))
    extrude(km, profiles(sk), z0, SHEET_T, name="Keymat")
    kb = body_named(km, "Keymat")
    sk = new_sketch(km, "sheet trim")
    draw(sk, rrect((0, top_y + 50), 120, 100, 0))
    extrude(km, profiles(sk), z0 - 1, SHEET_T + 2, "cut", [kb])

    keys = [k for k in b["keys"] if k[0] not in ("UP", "DOWN", "LEFT", "RIGHT")]
    cap = lambda n, w, h: tuple(v - 2 * CAP_CLR for v in key_open(n, w, h))
    # thin hinge ring around every cap so the sheet flexes (0.4 left)
    sk = new_sketch(km, "hinges")
    for name, c, w, h in keys:
        cw, ch = cap(name, w, h)
        draw(sk, rrect(c, cw + 1.6, ch + 1.6, 2.0))
    circle(sk, replay_centre(b), REPLAY_D - 2 * CAP_CLR + 1.6)
    extrude(km, profiles(sk), z0 - 0.1, 0.5, "cut", [kb])
    # caps through the plate
    sk = new_sketch(km, "caps")
    for name, c, w, h in keys:
        cw, ch = cap(name, w, h)
        draw(sk, rrect(c, cw, ch, ch / 2 - 0.01 if name in OVAL_KEYS else (1.3 if w < 8 else 1.6)))
    circle(sk, replay_centre(b), REPLAY_D - 2 * CAP_CLR)
    extrude(km, profiles(sk), z0, T - z0, "join", [kb])
    # bevelled crowns above the plate (a taper is far cheaper than filleting 47 loops)
    extrude(km, profiles(sk), T, CAP_PROUD, "join", [kb], taper=-CROWN_TAPER)
    # plungers onto the carbon pads, TRAVEL above the board
    sk = new_sketch(km, "plungers")
    for name, c, w, h in b["keys"]:
        draw(sk, rrect(c, w - 1.0, h - 1.0, 0.8))
    extrude(km, profiles(sk), Z_PCB_F + TRAVEL, z0 - (Z_PCB_F + TRAVEL), "join", [kb])
    # REPLAY rocks on a centre pivot that touches the board
    sk = new_sketch(km, "replay pivot")
    circle(sk, replay_centre(b), 2.0)
    extrude(km, profiles(sk), Z_PCB_F + 0.05, z0 - Z_PCB_F - 0.05, "join", [kb])
    # clear the screw columns and locating pins (sheet, plungers and cap bases below the plate)
    sk = new_sketch(km, "pin clearance")
    for c, r, _, kind in screws(b):
        circle(sk, c, 2 * r + (1.0 if kind == "mid" else 0) + 0.6)
    for ref, c, d, kind in b["holes"]:
        if kind == "post":
            circle(sk, c, d)
    extrude(km, profiles(sk), Z_PCB_F, Z_PLATE - Z_PCB_F, "cut", [kb])
    print("keymat ok")

# Arial has ² ³ ¹ ¯ but no ⁻ ⁿ ˣ, so x⁻¹ is spelled with a macron
LEGENDS = {"x^3": "x³", "x^-1": "x¯¹", "log_a": "log■□", "sqrt": "√■", "x^2": "x²", "x^n": "x■",
           "o''": "°'\"", "S<>D": "S⇔D", "x": "×", "/": "÷", "-": "−", "x10^x": "×10x", "a/b": "■/□"}

def glyph_width(s):
    """Rough rendered width of s in Fusion's Arial, in text heights."""
    return sum(0.95 if ch.isupper() or ch in "⇔■□×÷" else 0.45 if ch in "()'.,¹²³¯°\"" else 0.72 for ch in s)

def stage_legends(b):
    km = comp_named("Keymat")
    kb = body_named(km, "Keymat")
    sk = new_sketch(km, "legends")
    n = 0
    for name, c, w, h in b["keys"]:
        if name in ("UP", "DOWN", "LEFT", "RIGHT"):
            continue
        s = LEGENDS.get(name, name)
        top_w = w + 2 * KEY_CLR - 2 * CAP_CLR - 2 * CAP_PROUD * math.tan(math.radians(CROWN_TAPER))
        th = min(3.4 if w >= 9 else 2.2, (top_w - 1.4) / glyph_width(s))
        try:
            text(sk, s, c, th, top_w + 4)
            n += 1
        except Exception as e:
            print("legend", name, e)
    for i in range(sk.sketchTexts.count):
        try:
            extrude(km, sk.sketchTexts.item(i), T + CAP_PROUD - 0.3, 0.4, "cut", [kb])
        except Exception as e:
            print("legend cut", i, e)
    # REPLAY: four arrow dimples
    rc = replay_centre(b)
    sk = new_sketch(km, "replay arrows")
    for dx, dy in ((0, 1), (0, -1), (1, 0), (-1, 0)):
        tip = (rc[0] + dx * 6.3, rc[1] + dy * 6.3)
        base = (rc[0] + dx * 4.3, rc[1] + dy * 4.3)
        px, py = -dy * 1.5, dx * 1.5
        pts = [tip, (base[0] + px, base[1] + py), (base[0] - px, base[1] - py)]
        draw(sk, [("L", pts[i], pts[(i + 1) % 3]) for i in range(3)])
    extrude(km, profiles(sk), T + CAP_PROUD - 0.4, 0.5, "cut", [kb])
    print("legends ok: %d" % n)

# SHIFT (yellow) / ALPHA (red) labels printed above the keys, from the firmware key table
# (sim/web/page.html TOP/FN/NUM), keyed by the KiCad pad names.
FACE = {"MODE": ("SETUP", ""), "x^-1": ("x!", ""), "x^3": ("³√", ""), "x^n": ("x√", ""),
        "log": ("10^x", ""), "ln": ("e^x", ""), "(-)": ("", "A"), "o''": ("", "B"),
        "hyp": ("", "C"), "sin": ("sin¯¹", "D"), "cos": ("cos¯¹", "E"), "tan": ("tan¯¹", "F"),
        "RCL": ("STO", ""), "ENG": ("←", ""), "(": ("%", ""), ")": (",", "X"), "S<>D": ("", "Y"),
        "M+": ("M−", "M"), "x": ("nPr", ""), "/": ("nCr", ""), "+": ("Pol", ""), "-": ("Rec", ""), "9": ("CLR", ""), "DEL": ("INS", ""), "AC": ("OFF", ""), "0": ("Rnd", ""),
        ".": ("Ran#", "RanInt"), "x10^x": ("π", "e")}
FACE_H, FACE_DEPTH = 1.3, 0.25

def stage_faceplate(b):
    """Deboss the SHIFT/ALPHA labels into the plate and fill them with coloured inlays."""
    front = comp_named("Front shell")
    fb = body_named(front, "Front shell")
    inlay = comp_named("Legend inlays", True)
    labels = []
    for name, c, w, h in b["keys"]:
        sh, al = FACE.get(name, ("", ""))
        y = c[1] + h / 2 + KEY_CLR + 0.35 + FACE_H / 2
        hw = w + 2 * KEY_CLR
        if sh and al:
            labels += [(sh, (c[0] - hw / 4, y), hw / 2, "shift"), (al, (c[0] + hw / 4, y), hw / 2, "alpha")]
        elif sh or al:
            labels.append((sh or al, (c[0], y), hw, "shift" if sh else "alpha"))
    for s_, c, w, kind in labels:
        sk = new_sketch(front, "face " + s_)
        th = min(FACE_H, (w + 1.5) / glyph_width(s_))
        text(sk, s_, c, th, w + 6)
        extrude(front, sk.sketchTexts.item(0), T - FACE_DEPTH, FACE_DEPTH + 0.1, "cut", [fb])
        sk2 = new_sketch(inlay, "inlay " + s_)
        text(sk2, s_, c, th, w + 6)
        f = extrude(inlay, sk2.sketchTexts.item(0), T - FACE_DEPTH, FACE_DEPTH - 0.02, name="%s %s" % (kind, s_))
        for body in f.bodies:
            appearance(body, "Paint - Enamel Glossy (Yellow)" if kind == "shift" else "Paint - Enamel Glossy (Red)")
    # fill the brand name too
    sk = new_sketch(inlay, "inlay brand")
    text(sk, "AI CALCULATOR", (0, b["active"][1] + 21.0), 3.2, 50)
    f = extrude(inlay, sk.sketchTexts.item(0), T - 0.3, 0.28, name="brand")
    for body in f.bodies:
        appearance(body, "Paint - Enamel Glossy (White)")
    print("faceplate ok: %d labels" % len(labels))

def stage_parts(b):
    # lens
    lens = comp_named("Lens", True)
    sk = new_sketch(lens, "lens")
    draw(sk, rrect(b["active"], 53.3, 27.3, 1.9))
    extrude(lens, profiles(sk), T - LENS_T, LENS_T, name="Lens (1 mm acrylic)")
    # reference parts (not printed)
    ref = comp_named("Reference parts", True)
    sk = new_sketch(ref, "pcb")
    pts = b["board"]
    draw(sk, [("L", pts[i - 1], pts[i]) for i in range(len(pts))])
    extrude(ref, profiles(sk), Z_SPLIT, PCB_T, name="PCB")
    pcb = body_named(ref, "PCB")
    sk = new_sketch(ref, "pcb holes")
    for r_, c, d, kind in b["holes"]:
        circle(sk, c, d)
    extrude(ref, smallest_profiles(sk, len(b["holes"])), Z_SPLIT - 0.1, PCB_T + 0.2, "cut", [pcb])
    sk = new_sketch(ref, "e-paper")
    draw(sk, rrect(b["epd"], 59.2, 29.2, 0.5))
    extrude(ref, profiles(sk), Z_PCB_F, EPD_T, name="E-paper 2.13in")
    sk = new_sketch(ref, "battery")
    draw(sk, rrect(b["batt"], 31.0, 11.5, 1.0))
    extrude(ref, profiles(sk), FLOOR + 0.2, 3.8, name="LiPo 31x11.5x3.8")
    sk = new_sketch(ref, "camera")
    draw(sk, rrect(b["cam_ko"], 8.5, 8.5, 0.5))
    extrude(ref, profiles(sk), 3.2, Z_SPLIT - 3.2, name="Camera module (guess)")
    sk = new_sketch(ref, "camera lens")
    circle(sk, b["cam"], 5.4)
    extrude(ref, profiles(sk), 1.0, 2.2, name="Camera lens (guess)")
    sk = new_sketch(ref, "magnet")
    mg = b["magnet"]
    draw(sk, rrect(mg, 21.5, 7.5, 0.5))
    extrude(ref, profiles(sk), FLOOR + 1.0, Z_SPLIT - FLOOR - 1.0, name="Magnetic connector (guess)")
    stage_looks(b)
    print("parts ok")

LOOKS = {("Front shell", "Front shell"): "Plastic - Matte (Black)",
         ("Back cover", "Back cover"): "Plastic - Matte (Black)",
         ("Keymat", "Keymat"): "Plastic - Matte (White)",
         ("Lens", "Lens (1 mm acrylic)"): "Glass (Grey)",
         ("Reference parts", "PCB"): "Paint - Enamel Glossy (Green)",
         ("Reference parts", "E-paper 2.13in"): "Paint - Enamel Glossy (White)",
         ("Reference parts", "LiPo 31x11.5x3.8"): "Paint - Enamel Glossy (Dark Grey)",
         ("Reference parts", "Camera module (guess)"): "Paint - Enamel Glossy (Black)",
         ("Reference parts", "Magnetic connector (guess)"): "Paint - Enamel Glossy (Yellow)"}

def stage_looks(b):
    for (c, n), look in LOOKS.items():
        try:
            print(n, "->", appearance(body_named(comp_named(c), n), look))
        except KeyError:
            pass

def stage_check(b):
    import adsk.core
    root = design.rootComponent
    col = adsk.core.ObjectCollection.create()
    out = []
    for occ in root.occurrences:
        if occ.component.name == "Legend inlays":       # paint fill, sits inside its own debosses
            continue
        for body in occ.bRepBodies:
            col.add(body)
            bb = body.boundingBox
            out.append("%-28s %8.2f cm3  %5.1f x %5.1f x %5.1f mm" % (
                body.name, body.physicalProperties.volume, (bb.maxPoint.x - bb.minPoint.x) * 10,
                (bb.maxPoint.y - bb.minPoint.y) * 10, (bb.maxPoint.z - bb.minPoint.z) * 10))
    res = design.analyzeInterference(design.createInterferenceInput(col))
    out.append("interferences: %d" % res.count)
    for i in range(res.count):
        r = res.item(i)
        out.append("  %s  <->  %s : %.4f cm3" % (r.entityOne.name, r.entityTwo.name, r.interferenceBody.volume))
    for line in out:
        print(line)
        log("check | " + line)

def stage_export(b):
    import adsk.fusion
    em = design.exportManager
    d = os.path.join(OUT, "export")
    os.makedirs(d, exist_ok=True)
    root = design.rootComponent
    em.execute(em.createSTEPExportOptions(os.path.join(d, "ai_calc_case.step"), root))
    em.execute(em.createFusionArchiveExportOptions(os.path.join(d, "ai_calc_case.f3d"), root))
    for cname, bname, fname in (("Front shell", "Front shell", "front_shell.stl"),
                                ("Back cover", "Back cover", "back_cover.stl"),
                                ("Keymat", "Keymat", "keymat_TPU.stl")):
        o = em.createSTLExportOptions(body_named(comp_named(cname), bname), os.path.join(d, fname))
        o.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementHigh
        em.execute(o)
    lens = comp_named("Lens")
    lens.sketches.itemByName("lens").saveAsDXF(os.path.join(d, "lens_1mm_acrylic.dxf"))
    print("exported to", d)

def stage_shots(b):
    import adsk.core
    d = os.path.join(OUT, "renders")
    os.makedirs(d, exist_ok=True)
    vp = app.activeViewport
    views = ARGS.get("views", "iso_front,front,back,iso_back").split(",")
    V = adsk.core.ViewOrientations
    named = {"front": V.TopViewOrientation, "back": V.BottomViewOrientation,
             "iso_front": V.IsoTopRightViewOrientation, "iso_back": V.IsoBottomLeftViewOrientation,
             "side": V.RightViewOrientation, "left": V.LeftViewOrientation, "top_end": V.FrontViewOrientation}
    named["iso_left"] = V.IsoTopLeftViewOrientation
    for v in views:
        cam = vp.camera
        if v.startswith("top@"):            # top@x;y;extent: look down at the face around (x, y) mm
            x, y, ext = map(float, v.split("@")[1].split(";"))
            cam.isFitView = False
            cam.target = P(x, y, T)
            cam.eye = P(x, y, T + 300)
            cam.upVector = adsk.core.Vector3D.create(0, 1, 0)
            cam.viewExtents = cm(ext)
        elif v.startswith("detail"):        # detail@y;z;extent: look along +X at (0, y, z) mm
            y, z, ext = map(float, v.split("@")[1].split(";"))
            cam.isFitView = False
            cam.target = P(0, y, z)
            cam.eye = P(-300, y, z)
            cam.upVector = adsk.core.Vector3D.create(0, 0, 1)
            cam.viewExtents = cm(ext)
        else:
            cam.viewOrientation = named[v]
            cam.isFitView = True
        vp.camera = cam
        adsk.doEvents()
        vp.refresh()
        fname = v.split("@")[0] + ("_%d" % views.index(v) if "@" in v else "")
        vp.saveAsImageFile(os.path.join(d, ARGS.get("prefix", "") + fname + ".png"), 1600, 1200)
    print("shots:", views)

def stage_section(b):
    """Cut every body at X = ARGS x (default 0.2 mm, through display, camera and REPLAY), take
    the side and iso shots with prefix section_, then delete the cuts again."""
    import adsk.core
    x = float(ARGS.get("x", 0.2))
    made = []
    for occ in design.rootComponent.occurrences:
        comp = occ.component
        for body in list(comp.bRepBodies):
            sk = new_sketch(comp, "section")
            draw(sk, rrect((x - 100, 0), 200, 400, 0))
            made += [extrude(comp, profiles(sk), -5, 30, "cut", [body]), sk]
    ARGS["prefix"] = "section_"
    ARGS["views"] = ARGS.get("views", "side,iso_front")
    stage_shots(b)
    for f in reversed(made):
        f.deleteMe()
    print("section at X=%.1f" % x)

def stage_explode(b):
    """Move parts apart along Z (ARGS gap=mm, 0 to reassemble)."""
    import adsk.core
    gap = float(ARGS.get("gap", 15))
    order = {"Back cover": 0, "Reference parts": 1, "Keymat": 2, "Front shell": 3, "Legend inlays": 3, "Lens": 4}
    for occ in design.rootComponent.occurrences:
        m = adsk.core.Matrix3D.create()
        m.translation = adsk.core.Vector3D.create(0, 0, cm(gap * order.get(occ.component.name, 0)))
        occ.transform = m
    print("explode", gap)

if __name__ == "__main__" and "ARGS" not in globals():
    b = read_board()
    V, R, gap = check_outline(b)
    print("outline %s mm, board-to-wall min gap %.2f mm" % ("%.1f x %.1f" % (max(p[0] for p in V) - min(p[0] for p in V), max(p[1] for p in V) - min(p[1] for p in V)), gap))
    print("keys", len(b["keys"]), "holes", len(b["holes"]), "pins", len(b["pins"]), "notches", b["notches"])
    for s in screws(b):
        print("screw", [round(v, 2) for v in s[0]], s[3])
    print("replay", replay_centre(b), "cam", b["cam"], "epd", b["epd"], "batt", b["batt"], "magnet", b["magnet"])
    w = window_centre(b)
    print("window centre %s (KiCad y %.2f) vs e-paper active centre %s (KiCad y %.2f): e-paper is %.2f mm %s"
          % (tuple(round(v, 2) for v in w), CY - w[1], tuple(round(v, 2) for v in b["active"]), CY - b["active"][1],
             abs(w[1] - b["active"][1]), "too low" if b["active"][1] < w[1] else "too high"))
elif "ARGS" in globals():
    import adsk.core, adsk.fusion
    import traceback
    log("stage " + ARGS["stage"])
    try:
        globals()["stage_" + ARGS["stage"]](read_board())
    except Exception:
        log("stage %s FAILED %s" % (ARGS["stage"], traceback.format_exc()))
        raise
    log("stage %s done" % ARGS["stage"])
