"""Unbranded CAD replica of the Casio fx-115ES calculator shell, built in Fusion 360.

    python build_fx115es_replica.py                 offline: compute + check the 2D geometry only
    python build_fx115es_replica.py all             every Fusion stage in order (new document)
    python build_fx115es_replica.py front back      just those stages
    python build_fx115es_replica.py shots views=front,back

Fusion must be open with the FusionMCPBridge add-in running (same as fusion_run.py).
Stages: new front back keys parts looks check export shots explode_shots
Progress goes to replica/build.log.

Frame (same as build_case.py): millimetres, front view. X right, Y up (towards the display),
Z = 0 at the outside of the back cover (rubber feet stick out below), front face at Z = T_BODY.
KiCad points convert with X = 150 - x_kicad, Y = 138.94 - y_kicad.

Nothing here is Casio branded: no logo, no model name, no key legends.
Every number below is a named parameter; the README in replica/ lists where each came from.
"""
import json, math, os, sys, time

REPO = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR"
ENC = os.path.join(REPO, "hardware", "enclosure")
OUT = os.path.join(ENC, "replica")
CX, CY = 150.0, 138.94

# ── Overall size ──────────────────────────────────────────────────────────────
L_FRONT = 159.9      # C1  front shell outer length
L_BACK = 161.0       # C3  back cover length (overhangs the front shell by half the difference at each end)
W = 79.3             # C2  outer width (front and back)
T = 13.8             # Casio spec thickness (fx-115ES), taken as body + rubber feet
FOOT_PROUD = 0.5     # guess: feet stick out this much below the back cover
T_BODY = T - FOOT_PROUD          # 13.3, back face Z=0 -> front face

# ── Walls and plates ──────────────────────────────────────────────────────────
WALL_KEY = 1.2       # C5  side wall in the keypad section (0.8 skin measured; 1.2 used)
WALL_SCREEN = 4.7    # C4  side wall in the screen section
WALL_TOP = 2.0       # guess: top end wall
WALL_BOTTOM = 1.6    # guess: bottom end wall
Y_SCREEN_SEC = 24.0  # guess: Y where the side wall steps from thin (keypad) to thick (screen)
FACE_T = 1.5         # guess: front plate thickness
SEAM_Z = 2.6         # guess: height of the back cover rim = parting line
BACK_PLATE = 1.4     # guess: back cover floor
LIP_H, LIP_T, LIP_GAP = 1.5, 0.8, 0.1   # guess: locating lip on the back cover
EDGE_R_FRONT = 2.0   # guess (photos): round-over of the front face edge
EDGE_R_BACK = 1.2    # guess: round-over of the back edge
Z_PLATE = T_BODY - FACE_T        # 11.8, underside of the front plate

# ── Display, solar cell ───────────────────────────────────────────────────────
WIN = (60.65, 24.3)  # C12 display window
WIN_R = 1.0          # guess: window corner radius
WIN_TOP_FROM_EDGE = 24.0   # C13 case top edge -> window top edge
BEZEL_MARGIN = 2.5   # guess: lens overlaps the window by this all round
BEZEL_DEPTH = 1.0    # guess: lens recess depth
LENS_T = 0.8         # guess: clear window thickness (top sits 0.2 below the face)
SOLAR_C = (10.85, 67.34)   # photo (stage 10), solar-cell window centre
SOLAR = (34.0, 11.0)       # photo, solar-cell window size
SOLAR_DEPTH = 0.8          # guess

# ── Keys ──────────────────────────────────────────────────────────────────────
# Opening sizes measured on the fx-115ES shell (build_case.py KEY_OPEN), keyed by KiCad pad width class
KEY_OPEN = {"oval": (8.3, 5.6), 6.0: (8.1, 6.0), 7.0: (8.9, 5.9), 9.0: (11.8, 8.2)}
KEY_R = {"oval": None, 6.0: 1.3, 7.0: 1.3, 9.0: 1.6}   # guess: opening corner radii
OVAL_KEYS = ("SHIFT", "ALPHA", "MODE", "ON")
REPLAY_D = 15.4      # 4-way pad opening
CAP_CLR = 0.25       # guess: cap to opening clearance per side
CAP_FLANGE = 0.4     # guess: retaining flange under the plate, per side
FLANGE_T = 0.45      # guess
KEY_PROUD = 1.5      # guess (photos): key tops above the face
REPLAY_PROUD = 1.2   # guess
CROWN_TAPER = 20.0   # degrees, bevel on the part above the face
MAT_T = 0.8          # guess: rubber keymat sheet
TRAVEL = 0.5         # guess: plunger gap to the board
DIRS = ("UP", "DOWN", "LEFT", "RIGHT")

# ── Internal stack (all guesses; the Casio board is only a reference body) ────
BOARD = (65.05, 97.9)        # C6 Casio board
BOARD_T = 0.8                # guess
Z_FLANGE_TOP = Z_PLATE - 0.08
Z_CAP_BOT = Z_FLANGE_TOP - FLANGE_T          # caps rest on the mat
Z_MAT_TOP = Z_CAP_BOT - 0.02
Z_MAT_BOT = Z_MAT_TOP - MAT_T
Z_BOARD_F = Z_MAT_BOT - TRAVEL - 0.45        # plungers are 0.45 tall
Z_BOARD_B = Z_BOARD_F - BOARD_T
Z_RIB_TOP = Z_BOARD_B - 0.1                  # back-cover ribs and rings stop just under the board
Z_MEET = 5.0                                 # screw posts (front) meet bosses (back) here
Z_LOC_END = Z_BOARD_B - 0.3                  # locating posts go through the board

# ── Posts (front shell) and bosses (back cover) ───────────────────────────────
SCREW_POST_D = 3.85  # C7
LOC_POST_D = 2.9     # C7
PILOT_D = 1.7        # guess: self-tapping screw pilot
BOSS_D = 5.5         # guess: back-cover boss
SCREW_D, CBORE_D, CBORE_H = 2.2, 4.4, 1.2    # guess: screw clearance + counterbore from outside
CORNER_FROM_SIDE, CORNER_FROM_TOP = 4.2, 6.1  # C11 corner post centre from the inside walls
BOTTOM_PITCH = 39.0  # C10 bottom screw posts centre to centre
BOTTOM_Y = -71.1     # board notches (build_case BOTTOM_SCREWS); C10 says ~16.5 from each wall
MID_SCREWS = [(22.4, 12.74), (-24.0, 12.74)]   # row 1/2 screw posts, mat photo (stage 10)
LOC_POSTS = ["H2", "H4", "H6", "H8", "H9", "H10", "H13", "H14"]   # from the board snapshot

# ── Side-wall stubs (screen section), placeholders until C14 ──────────────────
STUB_D, STUB_Y, STUB_Z0 = 3.0, (36.0, 48.0, 60.0), 6.0

# ── Back cover ────────────────────────────────────────────────────────────────
RIB_T = 1.0          # guess
RING_T = 1.0         # guess
SOLAR_BOX = (-4.0, 29.0, 62.9, 74.9)   # stage 10 photo: ribbed box behind the solar cell (X0, X1, Y0, Y1)
SOLAR_BOX_H = 3.0    # guess: kept below Z_MEET so it never reaches the front posts
DOOR_C = (-18.3, 69.94)   # stage 10 photo: battery opening centre
DOOR = (15.0, 13.0)  # guess: battery door size
DOOR_R, DOOR_CLR = 2.0, 0.15
FEET = [(27.0, 52.0), (-27.0, 52.0), (27.0, -62.0), (-27.0, -62.0)]   # guess
FOOT_D, FOOT_POCKET = 7.0, 0.4

LCD, LCD_T = (64.0, 28.0), 2.2   # guess: dummy LCD under the window (between the wall stubs)

# ── Slide cover (optional) ────────────────────────────────────────────────────
RAIL_DEPTH = 0.5     # guess: groove along both long sides for the hard cover
RAIL_Z = (SEAM_Z + 0.6, SEAM_Z + 1.8)
COVER_T, COVER_CLR = 1.2, 0.25
COVER_Y = 66.0       # guess: cover side walls run |Y| < this

SPLINE_STEP = 2.5    # outer surface = smooth spline through the trace every 2.5 mm

def F(x, y):
    """KiCad point -> front-view point."""
    return (CX - x, CY - y)

# ═════════════════════════════════════════════════════════════════════════════
# 2D geometry (pure Python, runs offline and inside Fusion)
# ═════════════════════════════════════════════════════════════════════════════
def _unit(v):
    l = math.hypot(*v)
    return (v[0] / l, v[1] / l)

def area(P):
    return sum(P[i - 1][0] * P[i][1] - P[i][0] * P[i - 1][1] for i in range(len(P))) / 2

def trace_outline(length):
    """Traced fx-115ES silhouette scaled to W x length, CCW, centred on the trace's mid-height."""
    d = json.load(open(os.path.join(ENC, "fx115es_outline.json")))
    # the file stores the loop as 3 runs out of order (top-left, sides+bottom, top-right):
    # split at the jumps and chain the runs end to end, reversing where needed
    o = [tuple(p) for p in d["outline"]]
    runs, cur = [], [o[0]]
    for a, b in zip(o, o[1:]):
        if math.dist(a, b) > 1.5:
            runs.append(cur); cur = []
        cur.append(b)
    runs.append(cur)
    loop = runs.pop(0)
    while runs:
        k, rev = min(((k, rev) for k in range(len(runs)) for rev in (False, True)),
                     key=lambda t: math.dist(loop[-1], (runs[t[0]][::-1] if t[1] else runs[t[0]])[0]))
        r = runs.pop(k)
        loop += r[::-1] if rev else r
    d["outline"] = loop
    xs = [p[0] for p in d["outline"]]; ys = [p[1] for p in d["outline"]]
    sx, sy = W / (max(xs) - min(xs)), length / (max(ys) - min(ys))
    yc = (max(ys) + min(ys)) / 2                       # same placement as build_case.fx115_outline
    V = [(-(x - CX) * sx, CY - yc - (y - yc) * sy) for x, y in d["outline"]]
    V = [p for i, p in enumerate(V) if math.dist(p, V[i - 1]) > 0.3]
    changed = True
    while changed:
        changed = False
        for i in range(len(V)):
            a, p, c = V[i - 1], V[i], V[(i + 1) % len(V)]
            cr = (p[0] - a[0]) * (c[1] - p[1]) - (p[1] - a[1]) * (c[0] - p[0])
            if abs(cr) < 0.02 * math.dist(a, p) * math.dist(p, c):
                V.pop(i); changed = True; break
    return V if area(V) > 0 else V[::-1]

def offset_var(V, dfun):
    """Offset a CCW polygon inwards; dfun(outward normal) gives the distance for each edge (miter joins)."""
    n = len(V)
    lines = []
    for i in range(n):
        a, b = V[i], V[(i + 1) % n]
        u = _unit((b[0] - a[0], b[1] - a[1]))
        d = dfun((u[1], -u[0]))
        lines.append(((a[0] - u[1] * d, a[1] + u[0] * d), u, d))
    out = []
    for i in range(n):
        (p, u, d1), (q, w, d2) = lines[i - 1], lines[i]
        den = u[0] * w[1] - u[1] * w[0]
        own = q                                       # this edge's offset start
        if abs(den) < 1e-6:
            out.append(own); continue
        t = ((q[0] - p[0]) * w[1] - (q[1] - p[1]) * w[0]) / den
        x = (p[0] + u[0] * t, p[1] + u[1] * t)
        out.append(x if math.dist(x, V[i]) < 3 * max(d1, d2, 0.1) + 0.5 else own)
    return untangle(out)

def untangle(Pl, span=8):
    """Remove the small loops an inward offset makes at tight spots: when edge i crosses edge j
    (j a few edges further on), drop the vertices between them and keep the crossing point."""
    Pl = list(Pl)
    changed = True
    while changed:
        changed = False
        n = len(Pl)
        for i in range(n):
            a, b = Pl[i], Pl[(i + 1) % n]
            for k in range(2, min(span, n - 2)):
                j = (i + k) % n
                c, d = Pl[j], Pl[(j + 1) % n]
                den = (b[0] - a[0]) * (d[1] - c[1]) - (b[1] - a[1]) * (d[0] - c[0])
                if abs(den) < 1e-12:
                    continue
                t = ((c[0] - a[0]) * (d[1] - c[1]) - (c[1] - a[1]) * (d[0] - c[0])) / den
                u = ((c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0])) / den
                if 0 < t < 1 and 0 < u < 1:
                    xp = (a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1]))
                    idx = [(i + m) % n for m in range(1, k + 1)]      # vertices i+1 .. j
                    keep = [Pl[m] for m in range(n) if m not in idx]
                    pos = keep.index(a) + 1
                    Pl = keep[:pos] + [xp] + keep[pos:]
                    changed = True
                    break
            if changed:
                break
    return Pl

def offset_const(V, d):
    return offset_var(V, lambda n: d)

def wall_fun(side, top, bottom):
    def f(n):
        w = min(1.0, max(0.0, (abs(n[1]) - 0.5) / 0.4))
        return side * (1 - w) + (top if n[1] > 0 else bottom) * w
    return f

def crossings(P, y0):
    out = []
    for i in range(len(P)):
        a, b = P[i], P[(i + 1) % len(P)]
        if (a[1] - y0) * (b[1] - y0) < 0:
            t = (y0 - a[1]) / (b[1] - a[1])
            out.append((i, (a[0] + t * (b[0] - a[0]), y0), b[1] > a[1]))
    assert len(out) == 2, "expected 2 crossings at y=%.2f" % y0
    up = [c for c in out if c[2]][0]; down = [c for c in out if not c[2]][0]
    return up, down                                   # right side goes up, left side goes down (CCW)

def chain(P, i0, i1):
    """Vertices i0+1 .. i1 (mod n)."""
    n, out, i = len(P), [], i0
    while i != i1:
        i = (i + 1) % n
        out.append(P[i])
    return out

def cavity(V):
    """Inside of the front shell: thin side walls below Y_SCREEN_SEC, thick ones above."""
    K = offset_var(V, wall_fun(WALL_KEY, WALL_TOP, WALL_BOTTOM))
    S = offset_var(V, wall_fun(WALL_SCREEN, WALL_TOP, WALL_BOTTOM))
    (kr, kr_p, _), (kl, kl_p, _) = crossings(K, Y_SCREEN_SEC)
    (sr, sr_p, _), (sl, sl_p, _) = crossings(S, Y_SCREEN_SEC)
    C = [kl_p] + chain(K, kl, kr) + [kr_p, sr_p] + chain(S, sr, sl) + [sl_p]
    C = [p for i, p in enumerate(C) if math.dist(p, C[i - 1]) > 0.05]
    assert area(C) > 0
    return C, K, S

def resample(P, step):
    """Points every ~step mm along a closed polygon (spline fit points for the outer surface)."""
    per = sum(math.dist(P[i - 1], P[i]) for i in range(len(P)))
    n = int(per / step)
    out, i, acc = [], 0, 0.0
    segs = [(P[i], P[(i + 1) % len(P)]) for i in range(len(P))]
    k, pos = 0, 0.0
    for j in range(n):
        target = j * per / n
        while pos + math.dist(*segs[k]) < target:
            pos += math.dist(*segs[k]); k += 1
        a, b = segs[k]
        t = (target - pos) / max(1e-9, math.dist(a, b))
        out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
    # light smoothing (the trace has ~0.1 mm pixel noise)
    m = len(out)
    return [((out[j - 1][0] + 2 * out[j][0] + out[(j + 1) % m][0]) / 4,
             (out[j - 1][1] + 2 * out[j][1] + out[(j + 1) % m][1]) / 4) for j in range(m)]

def clip_below(P, y0):
    """Part of polygon P with y <= y0 (Sutherland-Hodgman, one edge)."""
    out = []
    for i in range(len(P)):
        a, b = P[i - 1], P[i]
        ina, inb = a[1] <= y0, b[1] <= y0
        if ina != inb:
            t = (y0 - a[1]) / (b[1] - a[1])
            out.append((a[0] + t * (b[0] - a[0]), y0))
        if inb:
            out.append(b)
    return out

def x_at(P, y, side):
    xs = []
    for i in range(len(P)):
        (x1, y1), (x2, y2) = P[i - 1], P[i]
        if (y1 - y) * (y2 - y) <= 0 and y1 != y2:
            xs.append(x1 + (y - y1) * (x2 - x1) / (y2 - y1))
    return max(xs) if side > 0 else min(xs)

def y_at(P, x, side):
    ys = []
    for i in range(len(P)):
        (x1, y1), (x2, y2) = P[i - 1], P[i]
        if (x1 - x) * (x2 - x) <= 0 and x1 != x2:
            ys.append(y1 + (x - x1) * (y2 - y1) / (x2 - x1))
    return max(ys) if side > 0 else min(ys)

def side_band(P, xmin, xout, right):
    """Polygon between the part of P beyond |x| > xmin on one side and a far line at |x| = xout."""
    s = 1 if right else -1
    # rotate so the run is contiguous (P is CCW, the run never wraps more than once)
    idx = [k for k, p in enumerate(P) if p[0] * s > xmin]
    runs, cur = [], [idx[0]]
    for a, b in zip(idx, idx[1:]):
        if b == a + 1:
            cur.append(b)
        else:
            runs.append(cur); cur = [b]
    runs.append(cur)
    if len(runs) > 1 and runs[0][0] == 0 and runs[-1][-1] == len(P) - 1:
        runs = [runs[-1] + runs[0]] + runs[1:-1]
    run = max(runs, key=len)
    pts = [P[k] for k in run]
    y_first, y_last = pts[0][1], pts[-1][1]
    return pts + [(s * xout, y_last), (s * xout, y_first)]

def rrect_pts(c, w, h, r, n=6):
    """Rounded rectangle as a polygon (for 2D checks)."""
    x, y = c
    r = min(r, w / 2, h / 2)
    pts = []
    for cx_, cy_, a0 in ((x + w / 2 - r, y + h / 2 - r, 0), (x - w / 2 + r, y + h / 2 - r, 90),
                         (x - w / 2 + r, y - h / 2 + r, 180), (x + w / 2 - r, y - h / 2 + r, 270)):
        for k in range(n + 1):
            a = math.radians(a0 + 90 * k / n)
            pts.append((cx_ + r * math.cos(a), cy_ + r * math.sin(a)))
    return pts

def seg_dist(p, a, b):
    ab = (b[0] - a[0], b[1] - a[1]); l2 = ab[0] ** 2 + ab[1] ** 2
    t = 0 if l2 == 0 else max(0, min(1, ((p[0] - a[0]) * ab[0] + (p[1] - a[1]) * ab[1]) / l2))
    return math.dist(p, (a[0] + ab[0] * t, a[1] + ab[1] * t))

def poly_dist(p, P):
    return min(seg_dist(p, P[i - 1], P[i]) for i in range(len(P)))

def key_open(name, w):
    k = "oval" if name in OVAL_KEYS else round(w)
    ow, oh = KEY_OPEN.get(k, KEY_OPEN[7.0])
    r = oh / 2 - 0.01 if k == "oval" else KEY_R.get(k, 1.3)
    return ow, oh, r

def geometry():
    """Everything the Fusion stages draw, in one dict (also written to replica/geometry.json)."""
    snap = json.load(open(os.path.join(OUT, "board_snapshot.json")))
    Vf, Vb = trace_outline(L_FRONT), trace_outline(L_BACK)
    C, K, S = cavity(Vf)
    top = max(y for x, y in Vf)
    win_c = (0.0, top - WIN_TOP_FROM_EDGE - WIN[1] / 2)

    # corner screw posts from C11 (mirrored), iterate because the inside top wall is curved
    cx_, cy_ = 29.0, 70.0
    for _ in range(4):
        cx_ = x_at(S, cy_, +1) - CORNER_FROM_SIDE
        cy_ = y_at(S, cx_, +1) - CORNER_FROM_TOP
    screws = [("corner R", (cx_, cy_)), ("corner L", (-cx_, cy_)),
              ("mid R", tuple(MID_SCREWS[0])), ("mid L", tuple(MID_SCREWS[1])),
              ("bottom R", (BOTTOM_PITCH / 2, BOTTOM_Y)), ("bottom L", (-BOTTOM_PITCH / 2, BOTTOM_Y))]
    holes = {h[0]: tuple(h[1]) for h in snap["holes"]}
    locs = [(n, holes[n]) for n in LOC_POSTS]

    keys = [(n, tuple(c), w, h) for n, c, w, h in snap["keys"]]
    replay = tuple(snap["replay"])
    openings = []
    for n, c, w, h in keys:
        if n in DIRS:
            continue
        ow, oh, r = key_open(n, w)
        openings.append(dict(name=n, c=c, w=ow, h=oh, r=r, oval=n in OVAL_KEYS, number=round(w) == 9))

    stubs = []
    for y in STUB_Y:
        for s in (+1, -1):
            stubs.append((x_at(S, y, s), y))

    rb = json.load(open(os.path.join(REPO, "hardware", "fitcheck", "backcover_fx115es.json")))
    ribs, rings = [], []
    for name, v in rb.items():
        if name.startswith("_"):
            continue
        if v[0] == "ring":
            rings.append((name, F(*v[1]), v[2]))
        else:
            (x1, y1), (x2, y2) = F(*v[1]), F(*v[2])
            # keep ribs 1.5 mm off the inside walls (and clear of the wall stubs)
            lim = lambda x, y: max(-(abs(x_at(C, y, -1)) - 1.5), min(x_at(C, y, +1) - 1.5, x))
            ylim = lambda y: min(y, y_at(C, (x1 + x2) / 2, +1) - 1.5)
            ribs.append((name, (lim(x1, y1), ylim(y1)), (lim(x2, y2), ylim(y2))))
    x0, x1, y0, y1 = SOLAR_BOX
    box = [((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)), ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0)),
           ((x0 + (x1 - x0) / 3, y0), (x0 + (x1 - x0) / 3, y1)), ((x0 + 2 * (x1 - x0) / 3, y0), (x0 + 2 * (x1 - x0) / 3, y1))]

    mat = clip_below(offset_const(C, 0.4), Y_SCREEN_SEC - 1.0)
    bottom_inner = y_at(C, 0.0, -1)
    board_c = (0.0, bottom_inner + 0.4 + BOARD[1] / 2)

    rail_in = offset_const(Vf, RAIL_DEPTH)
    lip_in = offset_const(Vf, RAIL_DEPTH - 0.15)
    cov_in = offset_const(Vf, -COVER_CLR)
    cov_out = offset_const(Vf, -(COVER_CLR + COVER_T))
    return dict(Vf=Vf, Vb=Vb, Vf_s=resample(Vf, SPLINE_STEP), Vb_s=resample(Vb, SPLINE_STEP), C=C, K=K, S=S, top=top, win_c=win_c, screws=screws, locs=locs, keys=keys,
                replay=replay, openings=openings, stubs=stubs, ribs=ribs, rings=rings, box=box, mat=mat,
                board_c=board_c, board_clip=offset_const(C, 0.3), lip_o=offset_const(C, LIP_GAP), lip_i=offset_const(C, LIP_GAP + LIP_T),
                rail=[side_band(rail_in, 25.0, 50.0, True), side_band(rail_in, 25.0, 50.0, False)],
                cover_lip=[side_band(lip_in, 25.0, 50.0, True), side_band(lip_in, 25.0, 50.0, False)],
                cov_in=cov_in, cov_out=cov_out, cov_out_s=resample(cov_out, SPLINE_STEP))

def check(g):
    """2D sanity checks; returns a list of text lines."""
    out = []
    Vf, C = g["Vf"], g["C"]
    xs = [p[0] for p in Vf]; ys = [p[1] for p in Vf]
    out.append("front outline %.2f x %.2f mm, back %.2f long" % (max(xs) - min(xs), max(ys) - min(ys),
               max(p[1] for p in g["Vb"]) - min(p[1] for p in g["Vb"])))
    for n, c in g["screws"]:
        out.append("screw post %-9s (%6.2f, %6.2f)  side wall gap %.2f" % (n, c[0], c[1], poly_dist(c, C) - SCREW_POST_D / 2))
    cr = dict(g["screws"])["corner R"]
    out.append("C11 check: corner post %.2f from inside side wall, %.2f from inside top wall"
               % (x_at(g["S"], cr[1], +1) - cr[0], y_at(g["S"], cr[0], +1) - cr[1]))
    b = dict(g["screws"])["bottom R"]
    out.append("C10 check: bottom post %.2f from inside side wall" % (x_at(C, b[1], +1) - b[0]))
    out.append("window centre (%.2f, %.2f), top edge %.2f below case top" % (g["win_c"] + (g["top"] - g["win_c"][1] - WIN[1] / 2,)))
    posts = [(n, c, SCREW_POST_D) for n, c in g["screws"]] + [(n, c, LOC_POST_D) for n, c in g["locs"]]
    worst = []
    for o in g["openings"]:
        P = rrect_pts(o["c"], o["w"], o["h"], o["r"])
        for n, c, d in posts:
            gap = poly_dist(c, P) - d / 2
            from_inside = abs(c[0] - o["c"][0]) < o["w"] / 2 and abs(c[1] - o["c"][1]) < o["h"] / 2
            if from_inside:
                gap = -gap
            if gap < 0.6:
                worst.append("  post %-8s vs opening %-6s gap %.2f" % (n, o["name"], gap))
    for n, c, d in posts:
        gap = math.dist(c, g["replay"]) - REPLAY_D / 2 - d / 2
        if gap < 0.6:
            worst.append("  post %-8s vs REPLAY gap %.2f" % (n, gap))
    out.append("posts closer than 0.6 mm to a key opening: %d" % len(worst))
    out += worst
    door = rrect_pts(DOOR_C, DOOR[0], DOOR[1], DOOR_R)
    for n, c in g["screws"]:
        if poly_dist(c, door) < BOSS_D / 2 + 1.0 or (abs(c[0] - DOOR_C[0]) < DOOR[0] / 2 and abs(c[1] - DOOR_C[1]) < DOOR[1] / 2):
            out.append("  WARNING boss %s %.2f from the battery door" % (n, poly_dist(c, door) - BOSS_D / 2))
    for f in FEET:
        for n, c in g["screws"]:
            if math.dist(f, c) < FOOT_D / 2 + CBORE_D / 2 + 1:
                out.append("  WARNING foot %s near screw %s" % (f, n))
    return out

# ═════════════════════════════════════════════════════════════════════════════
# Fusion side (helpers copied from build_case.py)
# ═════════════════════════════════════════════════════════════════════════════
def cm(v):
    return v / 10.0

def P(x, y, z=0.0):
    import adsk.core
    return adsk.core.Point3D.create(cm(x), cm(y), cm(z))

def log(msg):
    with open(os.path.join(OUT, "build.log"), "a", encoding="utf8") as f:
        print(time.strftime("%H:%M:%S"), msg, file=f)

ROOT_TAG = "Replica - Front shell"

def find_design():
    """The replica document (activate it if another one is in front)."""
    import adsk.fusion
    for d in app.documents:
        p = adsk.fusion.Design.cast(d.products.itemByProductType("DesignProductType"))
        if p and any(o.component.name == ROOT_TAG for o in p.rootComponent.occurrences):
            if not d.isActive:
                d.activate()
            return p
    raise RuntimeError("replica document not open: run stage new first")

def comp_named(name, create=False):
    import adsk.core
    root = design.rootComponent
    for occ in root.occurrences:
        if occ.component.name == "Replica - " + name:
            return occ.component
    if create:
        occ = root.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        occ.component.name = "Replica - " + name
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

def poly(sk, pts):
    L = sk.sketchCurves.sketchLines
    n = len(pts)
    first = prev = None
    for i in range(n):
        a, b = pts[i], pts[(i + 1) % n]
        ln = L.addByTwoPoints(prev.endSketchPoint if prev else P(*a), P(*b) if i < n - 1 else first.startSketchPoint)
        first = first or ln
        prev = ln

def spline(sk, pts):
    """Closed outline as two fitted splines (left/right halves) joined tangentially.
    A single closed spline gives a periodic edge that Fusion refuses to fillet."""
    import adsk.core
    n = len(pts)
    iR = max(range(n), key=lambda i: pts[i][0] - abs(pts[i][1]) * 0.01)
    iL = min(range(n), key=lambda i: pts[i][0] + abs(pts[i][1]) * 0.01)
    def run_(a, b):
        out, i = [pts[a]], a
        while i != b:
            i = (i + 1) % n
            out.append(pts[i])
        return out
    def open_spline(ps, start=None, end=None):
        col = adsk.core.ObjectCollection.create()
        col.add(start or P(*ps[0]))
        for p in ps[1:-1]:
            col.add(P(*p))
        col.add(end or P(*ps[-1]))
        return sk.sketchCurves.sketchFittedSplines.add(col)
    s1 = open_spline(run_(iR, iL))
    s2 = open_spline(run_(iL, iR), s1.endSketchPoint, s1.startSketchPoint)
    sk.geometricConstraints.addTangent(s1, s2)
    return s1, s2

def rrect(sk, c, w, h, r):
    """Rounded rectangle from lines and fillet arcs."""
    x, y = c
    r = max(0.0, min(r, w / 2 - 0.005, h / 2 - 0.005))
    L, A = sk.sketchCurves.sketchLines, sk.sketchCurves.sketchArcs
    if r <= 0:
        poly(sk, [(x + w / 2, y - h / 2), (x + w / 2, y + h / 2), (x - w / 2, y + h / 2), (x - w / 2, y - h / 2)])
        return
    q = r * (1 - 1 / math.sqrt(2))
    pts = [(x + w / 2, y - h / 2 + r), (x + w / 2, y + h / 2 - r), (x + w / 2 - r, y + h / 2), (x - w / 2 + r, y + h / 2),
           (x - w / 2, y + h / 2 - r), (x - w / 2, y - h / 2 + r), (x - w / 2 + r, y - h / 2), (x + w / 2 - r, y - h / 2)]
    mids = [(x + w / 2 - q, y + h / 2 - q), (x - w / 2 + q, y + h / 2 - q), (x - w / 2 + q, y - h / 2 + q), (x + w / 2 - q, y - h / 2 + q)]
    for k in range(4):
        a, b = pts[2 * k], pts[2 * k + 1]                 # straight side
        if math.dist(a, b) > 1e-4:
            L.addByTwoPoints(P(*a), P(*b))
        A.addByThreePoints(P(*b), P(*mids[k]), P(*pts[(2 * k + 2) % 8]))

def circle(sk, c, d):
    sk.sketchCurves.sketchCircles.addByCenterRadius(P(*c), cm(d / 2))

def profiles(sk, loops=None, test=None):
    import adsk.core
    sk.isComputeDeferred = False
    col = adsk.core.ObjectCollection.create()
    for p in sk.profiles:
        if loops is not None and p.profileLoops.count != loops:
            continue
        if test:
            c = p.areaProperties().centroid
            if not test((c.x * 10, c.y * 10)):
                continue
        col.add(p)
    if col.count == 0:
        raise RuntimeError("sketch %s formed no profiles" % sk.name)
    return col

def smallest_profiles(sk, n):
    import adsk.core
    sk.isComputeDeferred = False
    ps = sorted(sk.profiles, key=lambda p: p.areaProperties().area)[:n]
    col = adsk.core.ObjectCollection.create()
    for p in ps:
        col.add(p)
    return col

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
        for i in range(f.bodies.count):
            f.bodies.item(i).name = name if f.bodies.count == 1 else "%s %d" % (name, i + 1)
    return f

def edge_loop_at(body, z, outer=True):
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

def appearance(body, name):
    try:
        lib = app.materialLibraries.itemByName("Fusion Appearance Library")
        a = lib.appearances.itemByName(name)
        local = design.appearances.itemByName(name) or design.appearances.addByCopy(a, name)
        body.appearance = local
        return name
    except Exception as e:
        return "failed %s" % e

def inside(p, Pl):
    x, y, c = p[0], p[1], False
    for i in range(len(Pl)):
        (x1, y1), (x2, y2) = Pl[i - 1], Pl[i]
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1):
            c = not c
    return c

def rib_rect(a, b, t):
    u = _unit((b[0] - a[0], b[1] - a[1])); n = (-u[1] * t / 2, u[0] * t / 2)
    return [(a[0] + n[0], a[1] + n[1]), (b[0] + n[0], b[1] + n[1]), (b[0] - n[0], b[1] - n[1]), (a[0] - n[0], a[1] - n[1])]

# ── Stages ────────────────────────────────────────────────────────────────────
def stage_new(g):
    import adsk.core, adsk.fusion
    global design
    for d in list(app.documents):                     # close earlier replica builds only
        p = adsk.fusion.Design.cast(d.products.itemByProductType("DesignProductType"))
        if p and any(o.component.name == ROOT_TAG for o in p.rootComponent.occurrences):
            d.close(False)
    app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
    design = adsk.fusion.Design.cast(app.activeProduct)
    design.designType = adsk.fusion.DesignTypes.ParametricDesignType
    comp_named("Front shell", True)
    print("new document")

def stage_front(g):
    fr = comp_named("Front shell")
    sk = new_sketch(fr, "outline")
    spline(sk, g["Vf_s"])
    extrude(fr, profiles(sk), SEAM_Z, T_BODY - SEAM_Z, name="Front shell")
    fb = body_named(fr, "Front shell")
    sk = new_sketch(fr, "cavity")
    poly(sk, g["C"])
    extrude(fr, profiles(sk), SEAM_Z - 0.1, Z_PLATE - SEAM_Z + 0.1, "cut", [fb])
    fillet(fr, edge_loop_at(fb, T_BODY), EDGE_R_FRONT)

    # posts first, so the key openings nick them where the real ones sit in the webs
    sk = new_sketch(fr, "screw posts")
    for n, c in g["screws"]:
        circle(sk, c, SCREW_POST_D)
    extrude(fr, profiles(sk), Z_MEET, Z_PLATE - Z_MEET + 0.01, "join", [fb])
    sk = new_sketch(fr, "pilot holes")
    for n, c in g["screws"]:
        circle(sk, c, PILOT_D)
    extrude(fr, smallest_profiles(sk, len(g["screws"])), Z_MEET, Z_PLATE - Z_MEET - 0.5, "cut", [fb])
    sk = new_sketch(fr, "locating posts")
    for n, c in g["locs"]:
        circle(sk, c, LOC_POST_D)
    extrude(fr, profiles(sk), Z_LOC_END, Z_PLATE - Z_LOC_END + 0.01, "join", [fb])
    sk = new_sketch(fr, "wall stubs")                  # placeholders (C14)
    for c in g["stubs"]:
        circle(sk, c, STUB_D)
    extrude(fr, profiles(sk), STUB_Z0, Z_PLATE - STUB_Z0 + 0.01, "join", [fb])

    # display window, lens recess, solar-cell recess
    sk = new_sketch(fr, "lens recess")
    rrect(sk, g["win_c"], WIN[0] + 2 * BEZEL_MARGIN, WIN[1] + 2 * BEZEL_MARGIN, WIN_R + BEZEL_MARGIN)
    extrude(fr, profiles(sk), T_BODY - BEZEL_DEPTH, BEZEL_DEPTH + 1, "cut", [fb])
    sk = new_sketch(fr, "window")
    rrect(sk, g["win_c"], WIN[0], WIN[1], WIN_R)
    extrude(fr, profiles(sk), Z_PLATE - 1, FACE_T + 2, "cut", [fb])
    sk = new_sketch(fr, "solar recess")
    rrect(sk, SOLAR_C, SOLAR[0], SOLAR[1], 0.8)
    extrude(fr, profiles(sk), T_BODY - SOLAR_DEPTH, SOLAR_DEPTH + 1, "cut", [fb])

    # key openings
    sk = new_sketch(fr, "key openings")
    for o in g["openings"]:
        rrect(sk, o["c"], o["w"], o["h"], o["r"])
    circle(sk, g["replay"], REPLAY_D)
    extrude(fr, profiles(sk), Z_PLATE - 1, FACE_T + 2, "cut", [fb])

    # grooves along both long sides for the slide-on hard cover
    if RAIL_DEPTH > 0:
        sk = new_sketch(fr, "cover rails")
        for band in g["rail"]:
            poly(sk, band)
        extrude(fr, profiles(sk), RAIL_Z[0], RAIL_Z[1] - RAIL_Z[0], "cut", [fb])
    print("front ok, faces", fb.faces.count)

def stage_back(g):
    bk = comp_named("Back cover", True)
    sk = new_sketch(bk, "outline")
    spline(sk, g["Vb_s"])
    extrude(bk, profiles(sk), 0, SEAM_Z, name="Back cover")
    bb = body_named(bk, "Back cover")
    fillet(bk, edge_loop_at(bb, 0), EDGE_R_BACK)
    sk = new_sketch(bk, "cavity")                     # inside of the lip, so the lip grows out of the rim
    poly(sk, g["lip_i"])
    extrude(bk, profiles(sk), BACK_PLATE, SEAM_Z, "cut", [bb])
    sk = new_sketch(bk, "lip")
    poly(sk, g["lip_o"]); poly(sk, g["lip_i"])
    extrude(bk, profiles(sk, 2), SEAM_Z - 0.01, LIP_H + 0.01, "join", [bb])

    sk = new_sketch(bk, "bosses")
    for n, c in g["screws"]:
        circle(sk, c, BOSS_D)
    extrude(bk, profiles(sk), BACK_PLATE - 0.01, Z_MEET - 0.1 - BACK_PLATE + 0.01, "join", [bb])

    # ribs and rings that back up the board / display (stage 10 photo, ~1 mm)
    sk = new_sketch(bk, "ribs")
    rects = [rib_rect(a, b, RIB_T) for n, a, b in g["ribs"]]
    for r in rects:
        poly(sk, r)
    extrude(bk, profiles(sk, test=lambda p: any(inside(p, r) for r in rects)), BACK_PLATE - 0.01,
            Z_RIB_TOP - BACK_PLATE + 0.01, "join", [bb])
    for n, c, r in g["rings"]:
        sk = new_sketch(bk, n)
        circle(sk, c, 2 * r); circle(sk, c, 2 * r - 2 * RING_T)
        extrude(bk, profiles(sk, 2), BACK_PLATE - 0.01, Z_RIB_TOP - BACK_PLATE + 0.01, "join", [bb])
    sk = new_sketch(bk, "solar box")
    rects = [rib_rect(a, b, RIB_T) for a, b in g["box"]]
    for r in rects:
        poly(sk, r)
    extrude(bk, profiles(sk, test=lambda p: any(inside(p, r) for r in rects)), BACK_PLATE - 0.01,
            SOLAR_BOX_H + 0.01, "join", [bb])

    # screw holes with counterbores from outside
    sk = new_sketch(bk, "screw holes")
    for n, c in g["screws"]:
        circle(sk, c, SCREW_D)
    extrude(bk, smallest_profiles(sk, 6), -1, Z_MEET + 1, "cut", [bb])
    sk = new_sketch(bk, "counterbores")
    for n, c in g["screws"]:
        circle(sk, c, CBORE_D)
    extrude(bk, smallest_profiles(sk, 6), -1, CBORE_H + 1, "cut", [bb])

    # battery door opening and rubber-foot pockets
    sk = new_sketch(bk, "battery opening")
    rrect(sk, DOOR_C, DOOR[0], DOOR[1], DOOR_R)
    extrude(bk, smallest_profiles(sk, 1), -1, BACK_PLATE + 1.01, "cut", [bb])
    sk = new_sketch(bk, "feet pockets")
    for c in FEET:
        circle(sk, c, FOOT_D + 0.2)
    extrude(bk, smallest_profiles(sk, 4), -1, FOOT_POCKET + 1, "cut", [bb])
    print("back ok, faces", bb.faces.count)

def stage_keys(g):
    km = comp_named("Keymat", True)
    sk = new_sketch(km, "sheet")
    poly(sk, g["mat"])
    extrude(km, profiles(sk), Z_MAT_BOT, MAT_T, name="Keymat (rubber)")
    kb = body_named(km, "Keymat (rubber)")
    sk = new_sketch(km, "plungers")
    for n, c, w, h in g["keys"]:
        rrect(sk, c, w - 1.0, h - 1.0, 0.8)
    extrude(km, profiles(sk), Z_BOARD_F + TRAVEL, Z_MAT_BOT - Z_BOARD_F - TRAVEL + 0.01, "join", [kb])
    sk = new_sketch(km, "post holes")
    for n, c in g["screws"]:
        circle(sk, c, SCREW_POST_D + 0.6)
    for n, c in g["locs"]:
        circle(sk, c, LOC_POST_D + 0.6)
    extrude(km, profiles(sk, test=lambda p: any(math.dist(p, c) < 2.5 for n, c in g["screws"] + g["locs"])),
            Z_BOARD_F, Z_MAT_TOP - Z_BOARD_F + 0.1, "cut", [kb])

    kc = comp_named("Keycaps", True)
    caps = [(o["name"], o["c"], o["w"] - 2 * CAP_CLR, o["h"] - 2 * CAP_CLR, o["r"]) for o in g["openings"]]
    rd = REPLAY_D - 2 * CAP_CLR
    sk = new_sketch(kc, "flanges")
    for n, c, w, h, r in caps:
        rrect(sk, c, w + 2 * CAP_FLANGE, h + 2 * CAP_FLANGE, r + CAP_FLANGE if r < h / 2 - 0.1 else (h + 2 * CAP_FLANGE) / 2)
    circle(sk, g["replay"], rd + 2 * CAP_FLANGE)
    f = extrude(kc, profiles(sk), Z_CAP_BOT, FLANGE_T, name="cap")
    bodies = [f.bodies.item(i) for i in range(f.bodies.count)]
    sk = new_sketch(kc, "shafts")
    for n, c, w, h, r in caps:
        rrect(sk, c, w, h, r if r < h / 2 - 0.1 else h / 2)
    circle(sk, g["replay"], rd)
    prof = profiles(sk)
    extrude(kc, prof, Z_CAP_BOT + FLANGE_T - 0.01, T_BODY - Z_CAP_BOT - FLANGE_T + 0.01, "join", bodies)
    # crowns: tapered tops (a taper is much cheaper than filleting 47 loops)
    sk2 = new_sketch(kc, "crowns")
    for n, c, w, h, r in caps:
        rrect(sk2, c, w, h, r if r < h / 2 - 0.1 else h / 2)
    extrude(kc, profiles(sk2), T_BODY, KEY_PROUD, "join", list(kc.bRepBodies), taper=-CROWN_TAPER)
    # REPLAY sits a little lower than the keys: trim its crown
    sk3 = new_sketch(kc, "replay trim")
    circle(sk3, g["replay"], rd + 1)
    extrude(kc, profiles(sk3), T_BODY + REPLAY_PROUD, 3, "cut", list(kc.bRepBodies))
    # flanges clear the posts that sit in the webs between keys
    sk4 = new_sketch(kc, "post clearance")
    for n, c in g["screws"]:
        circle(sk4, c, SCREW_POST_D + 0.6)
    for n, c in g["locs"]:
        circle(sk4, c, LOC_POST_D + 0.6)
    extrude(kc, profiles(sk4, test=lambda p: any(math.dist(p, c) < 2.5 for n, c in g["screws"] + g["locs"])),
            Z_CAP_BOT - 0.1, FLANGE_T + 0.2, "cut", list(kc.bRepBodies))
    # name each cap after its key (nearest centre)
    names = [(n, c) for n, c, *_ in caps] + [("REPLAY", g["replay"])]
    for b in kc.bRepBodies:
        bb = b.boundingBox
        mid = ((bb.minPoint.x + bb.maxPoint.x) * 5, (bb.minPoint.y + bb.maxPoint.y) * 5)
        b.name = "Key " + min(names, key=lambda t: math.dist(t[1], mid))[0]
    print("keys ok: mat + %d caps" % kc.bRepBodies.count)

def stage_parts(g):
    ln = comp_named("Window lens", True)
    sk = new_sketch(ln, "lens")
    rrect(sk, g["win_c"], WIN[0] + 2 * BEZEL_MARGIN - 0.3, WIN[1] + 2 * BEZEL_MARGIN - 0.3, WIN_R + BEZEL_MARGIN - 0.15)
    extrude(ln, profiles(sk), T_BODY - BEZEL_DEPTH + 0.02, LENS_T, name="Window lens (clear)")

    so = comp_named("Solar cell (dummy)", True)
    sk = new_sketch(so, "cell")
    rrect(sk, SOLAR_C, SOLAR[0] - 0.4, SOLAR[1] - 0.4, 0.6)
    extrude(so, profiles(sk), T_BODY - SOLAR_DEPTH + 0.02, 0.5, name="Solar cell (dummy)")

    dr = comp_named("Battery door", True)
    sk = new_sketch(dr, "door")
    rrect(sk, DOOR_C, DOOR[0] - 2 * DOOR_CLR, DOOR[1] - 2 * DOOR_CLR, DOOR_R - DOOR_CLR)
    extrude(dr, profiles(sk), 0.0, BACK_PLATE, name="Battery door")
    db = body_named(dr, "Battery door")
    sk = new_sketch(dr, "tab")                         # hooks under the cover at the top end
    rrect(sk, (DOOR_C[0], DOOR_C[1] + DOOR[1] / 2 - 0.5), 6.0, 3.0, 0.5)
    extrude(dr, profiles(sk), BACK_PLATE, 0.5, "join", [db])
    sk = new_sketch(dr, "screw")                       # small screw at the bottom end
    c = (DOOR_C[0], DOOR_C[1] - DOOR[1] / 2 + 2.5)
    circle(sk, c, 1.6); circle(sk, c, 3.0)
    extrude(dr, smallest_profiles(sk, 1), -0.1, BACK_PLATE + 0.2, "cut", [db])
    extrude(dr, profiles(sk, 2), -0.1, 0.7, "cut", [db])
    sk = new_sketch(dr, "grip")
    for k in range(-2, 3):
        rrect(sk, (DOOR_C[0] + k * 1.8, DOOR_C[1] + 1.5), 0.8, 5.0, 0.3)
    extrude(dr, profiles(sk), -0.1, 0.4, "cut", [db])

    ft = comp_named("Rubber feet", True)
    sk = new_sketch(ft, "feet")
    for c in FEET:
        circle(sk, c, FOOT_D)
    extrude(ft, profiles(sk), -FOOT_PROUD, FOOT_PROUD + FOOT_POCKET - 0.02, name="Foot")

    rf = comp_named("Reference - Casio board (C6)", True)
    sk = new_sketch(rf, "board")
    rrect(sk, g["board_c"], BOARD[0], BOARD[1], 0.5)
    extrude(rf, profiles(sk), Z_BOARD_B, BOARD_T, name="Casio board (reference)")
    pb = body_named(rf, "Casio board (reference)")
    sk = new_sketch(rf, "corner clip")                 # real board corners follow the rounded case ends
    poly(sk, g["board_clip"]); poly(sk, [(-60, -100), (60, -100), (60, 100), (-60, 100)])
    extrude(rf, profiles(sk, 2), Z_BOARD_B - 0.1, BOARD_T + 0.2, "cut", [pb])
    sk = new_sketch(rf, "holes")
    for n, c in g["screws"]:
        circle(sk, c, SCREW_POST_D + 0.4)
    for n, c in g["locs"]:
        circle(sk, c, LOC_POST_D + 0.4)
    extrude(rf, profiles(sk, test=lambda p: any(math.dist(p, c) < 2.4 for n, c in g["screws"] + g["locs"])),
            Z_BOARD_B - 0.1, BOARD_T + 0.2, "cut", [pb])

    lc = comp_named("Reference - LCD (dummy)", True)  # makes the window read right; size is a guess
    sk = new_sketch(lc, "lcd")
    rrect(sk, g["win_c"], LCD[0], LCD[1], 0.5)
    extrude(lc, profiles(sk), Z_PLATE - 0.05 - LCD_T, LCD_T, name="LCD (dummy)")

    cv = comp_named("Slide cover", True)
    ztop_in = T_BODY + KEY_PROUD + 0.3
    sk = new_sketch(cv, "plate")
    spline(sk, g["cov_out_s"])
    extrude(cv, profiles(sk), ztop_in, COVER_T, name="Slide cover")
    cb = body_named(cv, "Slide cover")
    sk = new_sketch(cv, "walls")
    poly(sk, g["cov_out"]); poly(sk, g["cov_in"])
    extrude(cv, profiles(sk, 2), RAIL_Z[0] + 0.15, ztop_in - RAIL_Z[0] - 0.15 + 0.01, "join", [cb])
    sk = new_sketch(cv, "lips")
    for band in g["cover_lip"]:
        poly(sk, band)
    extrude(cv, profiles(sk), RAIL_Z[0] + 0.15, RAIL_Z[1] - RAIL_Z[0] - 0.3, "join", [cb])
    sk = new_sketch(cv, "trim outside")               # below the plate only, so the plate keeps its spline edge
    poly(sk, g["cov_out"]); poly(sk, [(-60, -100), (60, -100), (60, 100), (-60, 100)])
    extrude(cv, profiles(sk, 2), RAIL_Z[0], ztop_in - RAIL_Z[0], "cut", [cb])
    sk = new_sketch(cv, "open ends")                   # walls and lips only along the straight sides
    poly(sk, [(-26, -COVER_Y), (26, -COVER_Y), (26, COVER_Y), (-26, COVER_Y)])
    poly(sk, [(-60, COVER_Y), (60, COVER_Y), (60, 100), (-60, 100)])
    poly(sk, [(-60, -100), (60, -100), (60, -COVER_Y), (-60, -COVER_Y)])
    extrude(cv, profiles(sk), RAIL_Z[0] - 0.1, ztop_in - RAIL_Z[0] + 0.09, "cut", [cb])
    fillet(cv, edge_loop_at(cb, ztop_in + COVER_T), 1.5)
    stage_looks(g)
    print("parts ok")

LOOKS = {"Front shell": "Paint - Metallic (Silver)", "Back cover": "Plastic - Matte (Black)",
         "Keymat": "Rubber - Soft", "Window lens": "Glass (Grey)", "Solar cell (dummy)": "Paint - Metallic (Dark Grey)",
         "Battery door": "Plastic - Matte (Black)", "Rubber feet": "Rubber - Hard",
         "Reference - Casio board (C6)": "Plastic - Matte (Green)",
         "Reference - LCD (dummy)": "Glass - Heavy Color", "Slide cover": "Plastic - Translucent Matte (Gray)"}

def stage_looks(g):
    for cname, look in LOOKS.items():
        try:
            for b in comp_named(cname).bRepBodies:
                appearance(b, look)
        except KeyError:
            pass
    try:
        for b in comp_named("Keycaps").bRepBodies:
            n = b.name[4:]
            appearance(b, "Plastic - Matte (Gray)" if n in "0123456789" and len(n) == 1 or n in (".", "x10^x", "Ans", "=", "+", "-", "x", "/")
                       else "Paint - Metallic (Silver)" if n in OVAL_KEYS + ("REPLAY",)
                       else "Plastic - Matte (Red)" if n in ("AC", "DEL") else "Plastic - Matte (Black)")
    except KeyError:
        pass

def stage_check(g):
    import adsk.core
    col = adsk.core.ObjectCollection.create()
    out = []
    for occ in design.rootComponent.occurrences:
        vol = 0
        for body in occ.bRepBodies:
            col.add(body)
            vol += body.physicalProperties.volume
        bb = occ.boundingBox
        out.append("%-36s %3d bodies %7.2f cm3  %5.1f x %5.1f x %5.1f mm" % (
            occ.component.name, occ.bRepBodies.count, vol, (bb.maxPoint.x - bb.minPoint.x) * 10,
            (bb.maxPoint.y - bb.minPoint.y) * 10, (bb.maxPoint.z - bb.minPoint.z) * 10))
    ii = design.createInterferenceInput(col)
    ii.areCoincidentFacesIncluded = False
    res = design.analyzeInterference(ii)
    out.append("bodies checked: %d" % col.count)
    out.append("interferences: %s" % (res.count if res is not None else "0 (Fusion returned no result set)"))
    for i in range(res.count if res is not None else 0):
        r = res.item(i)
        out.append("  %s  <->  %s : %.4f cm3" % (r.entityOne.name, r.entityTwo.name, r.interferenceBody.volume))
    # second opinion: pairwise boolean intersection of every pair whose boxes overlap
    tb = adsk.fusion.TemporaryBRepManager.get()
    bodies = [col.item(i) for i in range(col.count)]
    hits, undet, pairs = [], [], 0
    for i in range(len(bodies)):
        for j in range(i + 1, len(bodies)):
            a, b = bodies[i], bodies[j]
            if not a.boundingBox.intersects(b.boundingBox):
                continue
            pairs += 1
            v = None
            for dz in (0.0, 0.002, -0.002):           # faces that just touch confuse the kernel: nudge 0.02 mm
                try:
                    ta, tbb = tb.copy(a), tb.copy(b)
                    if dz:
                        m = adsk.core.Matrix3D.create(); m.translation = adsk.core.Vector3D.create(0, 0, dz)
                        tb.transform(tbb, m)
                    tb.booleanOperation(ta, tbb, adsk.fusion.BooleanTypes.IntersectionBooleanType)
                    vv = sum(ta.lumps.item(k).volume for k in range(ta.lumps.count)) if ta and ta.lumps.count else 0
                    v = vv if v is None else min(v, vv)
                    if v < 1e-6:
                        break
                except Exception:
                    pass
            if v is None:
                undet.append("  (undetermined: %s <-> %s)" % (a.name, b.name))
                continue
            if v > 1e-6:
                hits.append("  %s / %s  <->  %s / %s : %.4f cm3" % (a.parentComponent.name, a.name,
                            b.parentComponent.name, b.name, v))
    out.append("pairwise boolean check: %d pairs with overlapping boxes, %d real overlaps" % (pairs, len(hits)))
    out += hits + undet
    with open(os.path.join(OUT, "interference.txt"), "w", encoding="utf8") as f:
        f.write("\n".join(out) + "\n")
    for line in out:
        log("check | " + line)
    print("\n".join(out[-12:]))

def stage_export(g):
    import adsk.fusion
    em = design.exportManager
    d = OUT
    root = design.rootComponent
    em.execute(em.createFusionArchiveExportOptions(os.path.join(d, "fx115es_replica.f3d"), root))
    em.execute(em.createSTEPExportOptions(os.path.join(d, "fx115es_replica.step"), root))
    sd = os.path.join(d, "stl")
    os.makedirs(sd, exist_ok=True)
    n = 0
    for occ in root.occurrences:
        cname = occ.component.name.replace("Replica - ", "")
        if cname.startswith("Reference"):
            continue
        if cname == "Keycaps":                         # 47 caps: one file for all + one per cap
            o = em.createSTLExportOptions(occ, os.path.join(sd, "keycaps_all.stl"))
            em.execute(o); n += 1
        for k, b in enumerate(occ.bRepBodies):
            safe = "".join(ch if ch.isalnum() else "_" for ch in (b.name if cname == "Keycaps" else cname + "_" + b.name))
            if cname == "Keycaps":
                safe = "keycaps/%02d_%s" % (k, safe)
                os.makedirs(os.path.join(sd, "keycaps"), exist_ok=True)
            o = em.createSTLExportOptions(b, os.path.join(sd, safe.strip("_") + ".stl"))
            o.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementMedium
            em.execute(o); n += 1
    print("exported f3d, step and %d stl" % n)

def set_visible(names, vis):
    for occ in design.rootComponent.occurrences:
        if occ.component.name.replace("Replica - ", "") in names:
            occ.isLightBulbOn = vis

def stage_shots(g):
    import adsk.core
    d = os.path.join(OUT, "renders")
    os.makedirs(d, exist_ok=True)
    vp = app.activeViewport
    views = ARGS.get("views", "front,back,side,top_end,iso_front,iso_back").split(",")
    V = adsk.core.ViewOrientations
    named = {"front": V.TopViewOrientation, "back": V.BottomViewOrientation,
             "iso_front": V.IsoTopRightViewOrientation, "iso_back": V.IsoBottomLeftViewOrientation,
             "side": V.RightViewOrientation, "top_end": V.BackViewOrientation, "iso_left": V.IsoTopLeftViewOrientation}
    for comp in design.allComponents:
        for skt in comp.sketches:
            skt.isLightBulbOn = False
    hide = [s for s in ARGS.get("hide", "Slide cover;Reference - Casio board (C6)").split(";") if s]
    set_visible(hide, False)
    try:
        for v in views:
            cam = vp.camera
            if v.startswith("top@"):                 # top@x;y;extent
                x, y, ext = map(float, v.split("@")[1].split(";"))
                cam.isFitView = False
                cam.target = P(x, y, T_BODY); cam.eye = P(x, y, T_BODY + 300)
                cam.upVector = adsk.core.Vector3D.create(0, 1, 0)
                cam.viewExtents = cm(ext)
            elif v.startswith("eye@"):               # eye@ex;ey;ez;tx;ty;tz;extent
                ex_, ey, ez, tx, ty, tz, ext = map(float, v.split("@")[1].split(";"))
                cam.isFitView = False
                cam.target = P(tx, ty, tz); cam.eye = P(ex_, ey, ez)
                cam.upVector = adsk.core.Vector3D.create(0, 0, 1)
                cam.viewExtents = cm(ext)
            else:
                cam.viewOrientation = named[v]
                cam.isFitView = True
            vp.camera = cam
            adsk.doEvents(); vp.refresh()
            fname = ARGS.get("prefix", "") + (v.split("@")[0] + ("_%d" % views.index(v) if "@" in v else ""))
            vp.saveAsImageFile(os.path.join(d, fname + ".png"), 1600, 1200)
    finally:
        set_visible(hide, True)
    print("shots:", views)

def stage_explode(g):
    import adsk.core
    gap = float(ARGS.get("gap", 14))
    order = {"Rubber feet": -2, "Battery door": -1, "Back cover": 0, "Reference - Casio board (C6)": 1,
             "Reference - LCD (dummy)": 1,
             "Keymat": 2, "Keycaps": 3, "Front shell": 4, "Solar cell (dummy)": 5, "Window lens": 5, "Slide cover": 6}
    for occ in design.rootComponent.occurrences:
        m = adsk.core.Matrix3D.create()
        m.translation = adsk.core.Vector3D.create(0, 0, cm(gap * order.get(occ.component.name.replace("Replica - ", ""), 0)))
        occ.transform = m
    print("explode", gap)

def stage_section(g):
    """Cut every body at Y = ARGS y (or X = ARGS x), take side shots with prefix section_, then undo."""
    y = ARGS.get("y"); x = ARGS.get("x")
    made = []
    for occ in design.rootComponent.occurrences:
        comp = occ.component
        if comp.bRepBodies.count == 0:
            continue
        sk = new_sketch(comp, "section")
        if y is not None:
            poly(sk, [(-100, float(y)), (100, float(y)), (100, float(y) + 200), (-100, float(y) + 200)])
        else:
            poly(sk, [(float(x), -100), (float(x) + 200, -100), (float(x) + 200, 100), (float(x), 100)])
        made += [extrude(comp, profiles(sk), -5, 30, "cut", list(comp.bRepBodies)), sk]
    ARGS["prefix"] = ARGS.get("prefix", "section_")
    try:
        stage_shots(g)
    finally:
        for f in reversed(made):
            f.deleteMe()

def stage_explode_shots(g):
    ARGS["gap"] = ARGS.get("gap", "14"); stage_explode(g)
    ARGS["views"] = "iso_front,iso_back"; ARGS["prefix"] = "exploded_"
    ARGS["hide"] = "Reference - Casio board (C6)"
    stage_shots(g)
    ARGS["gap"] = "0"; stage_explode(g)

# ═════════════════════════════════════════════════════════════════════════════
STAGES = ["new", "front", "back", "keys", "parts", "check", "export", "shots", "explode_shots"]

def to_json(g):
    return json.loads(json.dumps(g))

def send(stage, extra):
    """Run one stage inside Fusion through the FusionMCPBridge add-in; wait on build.log if it runs long."""
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
    if "timeout" in str(r.get("error", "")).lower():
        tail = lambda: open(logf, "rb").read()[start:].decode("utf8", "replace") if os.path.exists(logf) else ""
        t0 = time.time()
        while "stage %s done" % stage not in tail() and "FAILED" not in tail():
            if time.time() - t0 > 900:
                r = {"error": "gave up waiting"}; break
            time.sleep(3)
        else:
            r = {"result": "(finished after the bridge timed out)\n" + tail()[-800:]}
            if "FAILED" in tail():
                r = {"result": "", "traceback": tail()[-2500:]}
    print(r.get("result") or r.get("error") or r)
    if r.get("traceback"):
        print(r["traceback"])
        sys.exit(1)

if "ARGS" in globals():
    import adsk.core, adsk.fusion, traceback
    log("stage " + ARGS["stage"])
    try:
        G = json.load(open(os.path.join(OUT, "geometry.json")))
        G["screws"] = [(n, tuple(c)) for n, c in G["screws"]]
        G["locs"] = [(n, tuple(c)) for n, c in G["locs"]]
        if ARGS["stage"] != "new":
            design = find_design()
        globals()["stage_" + ARGS["stage"]](G)
    except Exception:
        log("stage %s FAILED %s" % (ARGS["stage"], traceback.format_exc()))
        raise
    log("stage %s done" % ARGS["stage"])
elif __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    g = geometry()
    json.dump(to_json(g), open(os.path.join(OUT, "geometry.json"), "w"))
    lines = check(g)
    open(os.path.join(OUT, "check_2d.txt"), "w", encoding="utf8").write("\n".join(lines) + "\n")
    print("\n".join(lines))
    stages = [a for a in sys.argv[1:] if "=" not in a]
    extra = dict(a.split("=", 1) for a in sys.argv[1:] if "=" in a)
    if stages == ["all"]:
        stages = STAGES
    for st in stages:
        t = time.time()
        print("==", st)
        send(st, extra)
        print("   %.0f s" % (time.time() - t))
