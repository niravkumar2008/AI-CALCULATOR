"""Battery envelope study (offline, no Fusion needed).

    python battery_envelope.py              -> prints the envelopes + fit table, writes battery_envelope.json
                                              and renders/battery_upgrade/envelope_*.png (needs Pillow)

Rasterises the space above the back-cover floor (front-view frame of build_final_assembly.py: X right, Y up,
Z = 0 at the outside of the back cover; KiCad x = 150 - X, y = 138.94 - Y) and finds, for every cell
height H, the largest axis-aligned box that lies on the floor (Z 1.0) and clears every obstacle by MARGIN
(0.3 mm sideways and on top; the bottom rests on the floor).
Obstacles come from geometry.json (shell rev D), placement.json (board v14 STEP) and the replica constants.

Configurations
  revD          top-left corner, donor fx-115ES shell as built (LR44 holder present, 4 grinds done)
  lr44_trim     same, LR44 holder cut down to Z 7.0 (1.9 mm off its 5.5 height; it stays 3.6 tall)
  lr44_ground   same, LR44 holder removed (0.2 stub left on the plate)
  custom        a drop-in printed / moulded shell with the same outside and stack-up (floor 1.0, board at 7.0),
                no ribs / rings / LR44 holder / solar box: anywhere under the board, screw bosses + posts kept,
                camera module + its ribbon corridor kept free.
"""
import json, math, os, sys, importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
ENC = os.path.dirname(HERE)
spec = importlib.util.spec_from_file_location("rep", os.path.join(ENC, "build_fx115es_replica.py"))
R = importlib.util.module_from_spec(spec); spec.loader.exec_module(R)
G = json.load(open(os.path.join(HERE, "geometry.json")))
PL = json.load(open(os.path.join(HERE, "placement.json")))

MARGIN = 0.3
FLOOR = R.BACK_PLATE                    # 1.0: inside of the back-cover floor
ZB0 = R.Z_BOARD_B                       # 7.0: board F (component) face
ZB1 = ZB0 + R.BOARD_T
ZP = R.Z_PLATE_TOP                      # 10.6: faceplate underside (screen section)
WIRE = 1.5                              # lead channel kept free on the -X side of J4 / the plug
GRIND_STUB = 0.2                        # LR44 holder ground to this much above the plate underside
TRIM_Z = 7.0                            # lr44_trim: holder bottom raised to this Z
REGIONS = {"corner": (-38.0, 6.0, 54.0, 81.0, 0.1), "whole": (-38.5, 38.5, -77.0, 81.0, 0.25)}

def inside(p, poly):
    x, y = p; c = False
    for i in range(len(poly)):
        x1, y1 = poly[i - 1]; x2, y2 = poly[i]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            c = not c
    return c

def seg_dist(p, a, b):
    ax, ay = a; bx, by = b; px, py = p
    dx, dy = bx - ax, by - ay
    t = max(0, min(1, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy or 1)))
    return math.hypot(px - ax - t * dx, py - ay - t * dy)

def poly_dist(p, poly):
    return min(seg_dist(p, poly[i - 1], poly[i]) for i in range(len(poly)))

def rect(x0, x1, y0, y1):
    return ("rect", (min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1)))

def board_outline():
    """board v14 outline (front view) from the edge-cut segments in placement.json: the closed loop through the
    most points (the slots are short separate loops)."""
    segs = [((150 - a[0], 138.94 - a[1]), (150 - b[0], 138.94 - b[1])) for a, b in PL["slots"]]
    key = lambda p: (round(p[0], 2), round(p[1], 2))
    adj = {}
    for a, b in segs:
        adj.setdefault(key(a), []).append(key(b)); adj.setdefault(key(b), []).append(key(a))
    seen, best = set(), []
    for s in adj:
        if s in seen:
            continue
        loop, prev, cur = [s], None, s
        seen.add(s)
        while True:
            nxt = [n for n in adj[cur] if n != prev and n not in seen]
            if not nxt:
                break
            prev, cur = cur, nxt[0]; seen.add(cur); loop.append(cur)
        if len(loop) > len(best):
            best = loop
    return best

BOARD = board_outline()

def obstacles(cfg):
    """(name, kind, data, z0, z1); kind rect=(x0,x1,y0,y1), circle=(cx,cy,r), poly=[pts], seg=(a,b,half-width)"""
    ob = [("faceplate plate",) + rect(-50, 50, -90, 90) + (ZP, 20),
          ("keypad plate / key caps",) + rect(-50, 50, -90, R.Y_RAMP[1]) + (R.Z_DOME_TOP, 20)]
    custom = cfg == "custom"
    zc = ZP - R.COIN_CUP[2]
    if cfg == "revD":
        ob.append(("LR44 holder ring", "circle", (R.DOOR_C[0], R.DOOR_C[1], R.COIN_CUP[0] / 2 + R.COIN_CUP[1]), zc, ZP))
        for q in G["coin"]:
            ob.append(("LR44 holder bar/lug", "poly", q, zc, ZP))
    elif cfg == "lr44_trim":
        ob.append(("LR44 holder ring (trimmed)", "circle", (R.DOOR_C[0], R.DOOR_C[1], R.COIN_CUP[0] / 2 + R.COIN_CUP[1]), TRIM_Z, ZP))
        for q in G["coin"]:
            ob.append(("LR44 holder bar/lug (trimmed)", "poly", q, TRIM_Z, ZP))
    elif cfg == "lr44_ground":
        xs = [p[0] for q in G["coin"] for p in q]; ys = [p[1] for q in G["coin"] for p in q]
        ob.append(("LR44 holder stub (ground)",) + rect(min(xs), max(xs), min(ys), max(ys)) + (ZP - GRIND_STUB, ZP))
    # solar window frame + cell (frame outer rounded rectangle, treated solid)
    sx, sy = R.SOLAR_C; w = R.SOLAR[0] + 0.4 + 2 * R.SOLAR_FRAME[0]; h = R.SOLAR[1] + 0.4 + 2 * R.SOLAR_FRAME[0]
    ob.append(("solar window frame",) + rect(sx - w / 2, sx + w / 2, sy - h / 2, sy + h / 2) + (ZP - R.SOLAR_FRAME[1], ZP))
    # screw posts (front) + bosses (back), locating posts (they end 0.3 under the board)
    for n, c in G["screws"]:
        ob.append(("screw post " + n, "circle", (c[0], c[1], R.SCREW_POST_D / 2), R.Z_MEET, ZP))
        ob.append(("screw boss " + n, "circle", (c[0], c[1], R.BOSS_D / 2), FLOOR - 0.01, R.Z_MEET))
    for n, c in G["locs"]:
        ob.append(("locating post " + n, "circle", (c[0], c[1], R.LOC_POST_D / 2), R.Z_LOC_END, ZP))
    for q in G["inner_ribs"] + G["hooks"]:
        ob.append(("inner rib / wire hook", "poly", q, R.STUB_Z0, ZP))
    for e, q in G["snap"]["teeth"]:
        ob.append(("snap tooth " + e, "poly", q, R.SNAP_TOP[1] if e == "top" else R.SNAP_BOT[2], ZP))
    if not custom:                                        # donor back cover: ribs, rings, wall ticks
        for q in G["ticks"]:
            ob.append(("wall tick", "poly", q, FLOOR - 0.01, R.SEAM_Z + R.LIP_H))
        for n, a, b, hgt in G["ribs"]:
            if n in ("solar box", "upper rib B (camera)"):
                continue                                  # ground flat (grinds 1 + 2)
            ob.append((n, "seg", (a, b, R.RIB_T / 2), FLOOR - 0.01, FLOOR + hgt))
        for rg in G["rings"]:
            n, c, r_ = rg[:3]                              # (name, centre, outer radius)
            ob.append((n, "circle", (c[0], c[1], r_), FLOOR - 0.01, FLOOR + R.RING_H))
    # board v14 (Z 7.0-7.8) and its F-side parts (they hang towards the floor), the e-paper panel (key side)
    ob.append(("board", "poly", BOARD, ZB0, ZB1))
    for ref, v in PL["parts"].items():
        x0, x1, y0, y1, z0, z1 = v[:6]
        ob.append(("board part " + ref,) + rect(x0, x1, y0, y1) + (z0, z1))
    j4 = PL["parts"]["J4"]
    pw, ph, pl = 6.0, 4.5, 4.0
    xm, zm = (j4[0] + j4[1]) / 2, (j4[4] + j4[5]) / 2
    ob.append(("JST-PH plug in J4",) + rect(xm - pw / 2, xm + pw / 2, j4[2] - pl, j4[2]) + (zm - ph / 2, zm + ph / 2))
    ex, ey = 150 - 149.969, 138.94 - 92.986
    ob.append(("e-paper panel",) + rect(ex - 29.5, ex + 29.5, ey - 14.6, ey + 14.6) + (ZB1 + 0.15, ZB1 + 1.2))
    # lead channel: WIRE mm free on the -X side of J4 and in front of the plug (the leads bend into it there)
    ob.append(("lead channel to J4",) + rect(j4[0] - WIRE, j4[1], j4[2] - pl - 3.0, j4[3]) + (FLOOR - 0.01, ZB0))
    # camera module (on the floor, lens in the 7 mm window) + its ribbon to J1 (reserved corridor)
    cx, cy = 150 - 150.0, 138.94 - 95.1
    ob.append(("camera module",) + rect(cx - 4.25, cx + 4.25, cy - 4.25, cy + 4.25) + (FLOOR - 0.01, ZB0))
    j1 = PL["parts"]["J1"]
    ob.append(("camera ribbon corridor",) + rect(-6.25, 6.25, j1[2], cy) + (FLOOR - 0.01, ZB0))
    # magnet connector + its legs (top right)
    ob.append(("magnet connector",) + rect(22.5 - 11, 22.5 + 11, 72.0, 82.0) + (FLOOR - 0.01, ZB0))
    return ob

def bbox(o, m):
    k, d = o[1], o[2]
    if k == "rect":
        return d[0] - m, d[1] + m, d[2] - m, d[3] + m
    if k == "circle":
        return d[0] - d[2] - m, d[0] + d[2] + m, d[1] - d[2] - m, d[1] + d[2] + m
    if k == "poly":
        xs = [p[0] for p in d]; ys = [p[1] for p in d]
        return min(xs) - m, max(xs) + m, min(ys) - m, max(ys) + m
    a, b, hw = d
    return min(a[0], b[0]) - hw - m, max(a[0], b[0]) + hw + m, min(a[1], b[1]) - hw - m, max(a[1], b[1]) + hw + m

def near(o, x, y, m):
    k, d = o[1], o[2]
    if k == "rect":
        x0, x1, y0, y1 = d
        return math.hypot(max(x0 - x, 0, x - x1), max(y0 - y, 0, y - y1)) < m
    if k == "circle":
        return math.hypot(x - d[0], y - d[1]) < d[2] + m
    if k == "poly":
        return inside((x, y), d) or poly_dist((x, y), d) < m
    return seg_dist((x, y), d[0], d[1]) < d[2] + m

def height_map(cfg, region):
    """free height above the floor (lowest obstacle bottom - MARGIN - FLOOR) for every column, + what limits it"""
    X0, X1, Y0, Y1, st = REGIONS[region]
    nx, ny = int(round((X1 - X0) / st)), int(round((Y1 - Y0) / st))
    m = MARGIN + st * 0.71                                # cell centre -> cell corner
    Hm = [[99.0] * nx for _ in range(ny)]
    who = [[""] * nx for _ in range(ny)]
    lip = G["lip_i"]                                      # back cover's lip = innermost wall low down
    for j in range(ny):                                   # outside the lip: no room
        y = Y0 + (j + 0.5) * st
        xs = []
        for k in range(len(lip)):
            (x1, y1), (x2, y2) = lip[k - 1], lip[k]
            if (y1 > y) != (y2 > y):
                xs.append(x1 + (y - y1) * (x2 - x1) / (y2 - y1))
        xs.sort()
        for i in range(nx):
            x = X0 + (i + 0.5) * st
            ok = any(xs[k] + m <= x <= xs[k + 1] - m for k in range(0, len(xs) - 1, 2))
            if not ok:
                Hm[j][i] = 0.0; who[j][i] = "side / top wall (lip)"
    # top / bottom of the lip outline (curved ends): distance test near the ends
    for o in obstacles(cfg):
        z0, z1 = o[3], o[4]
        if z1 <= FLOOR + 0.02:
            continue
        t = (z0 - MARGIN if z0 > FLOOR + 0.02 else FLOOR) - FLOOR
        bx0, bx1, by0, by1 = bbox(o, m)
        i0, i1 = max(0, int((bx0 - X0) / st)), min(nx - 1, int((bx1 - X0) / st) + 1)
        j0, j1 = max(0, int((by0 - Y0) / st)), min(ny - 1, int((by1 - Y0) / st) + 1)
        for j in range(j0, j1 + 1):
            y = Y0 + (j + 0.5) * st
            row, wr = Hm[j], who[j]
            for i in range(i0, i1 + 1):
                if row[i] <= t:
                    continue
                if near(o, X0 + (i + 0.5) * st, y, m):
                    row[i] = max(0.0, t); wr[i] = o[0]
    return Hm, who

def max_rect(mask):
    """largest-area all-True rectangle: (area, i0, i1, j0, j1) in cell indices (inclusive)"""
    ny, nx = len(mask), len(mask[0])
    hist = [0] * nx; best = (0, 0, 0, 0, 0)
    for j in range(ny):
        row = mask[j]
        for i in range(nx):
            hist[i] = hist[i] + 1 if row[i] else 0
        stck = []
        for i in range(nx + 1):
            h = hist[i] if i < nx else 0
            s = i
            while stck and stck[-1][1] >= h:
                s, hh = stck.pop()
                if hh * (i - s) > best[0]:
                    best = (hh * (i - s), s, i - 1, j - hh + 1, j)
            stck.append((s, h))
    return best

def envelopes(Hm, region, hs):
    X0, X1, Y0, Y1, st = REGIONS[region]
    out = []
    for H in hs:
        a, i0, i1, j0, j1 = max_rect([[c >= H - 1e-9 for c in row] for row in Hm])
        if a == 0:
            break
        out.append(dict(H=round(H, 2), L=round((i1 - i0 + 1) * st, 2), W=round((j1 - j0 + 1) * st, 2),
                        X=[round(X0 + i0 * st, 2), round(X0 + (i1 + 1) * st, 2)], Y=[round(Y0 + j0 * st, 2), round(Y0 + (j1 + 1) * st, 2)],
                        Z=[FLOOR, round(FLOOR + H, 2)], vol_cm3=round(a * st * st * H / 1000, 2)))
    return out

def fit(Hm, region, L, W, T):
    """best placement of an L x W x T cell (either way round): returns dict with the smallest side margin
    (beyond MARGIN), or None"""
    X0, X1, Y0, Y1, st = REGIONS[region]
    ny, nx = len(Hm), len(Hm[0])
    S = [[0] * (nx + 1) for _ in range(ny + 1)]
    for j in range(ny):
        acc = 0
        for i in range(nx):
            acc += Hm[j][i] >= T - 1e-9
            S[j + 1][i + 1] = S[j][i + 1] + acc
    full = lambda i0, i1, j0, j1: i0 >= 0 and j0 >= 0 and i1 < nx and j1 < ny and \
        S[j1 + 1][i1 + 1] - S[j0][i1 + 1] - S[j1 + 1][i0] + S[j0][i0] == (i1 - i0 + 1) * (j1 - j0 + 1)
    best = None
    for (a, b, rot) in ((L, W, "L along X"), (W, L, "L along Y")):
        ca, cb = int(math.ceil(a / st - 1e-6)), int(math.ceil(b / st - 1e-6))
        for j in range(ny - cb + 1):
            for i in range(nx - ca + 1):
                if not full(i, i + ca - 1, j, j + cb - 1):
                    continue
                # widest free box around it: grow X then Y
                gx0 = gx1 = gy0 = gy1 = 0
                while full(i - gx0 - 1, i + ca - 1, j, j + cb - 1): gx0 += 1
                while full(i - gx0, i + ca + gx1, j, j + cb - 1): gx1 += 1
                while full(i - gx0, i + ca - 1 + gx1, j - gy0 - 1, j + cb - 1): gy0 += 1
                while full(i - gx0, i + ca - 1 + gx1, j - gy0, j + cb + gy1): gy1 += 1
                sx, sy = (gx0 + gx1) * st + ca * st - a, (gy0 + gy1) * st + cb * st - b
                score = min(sx, sy)
                if best is None or score > best["slack_min"]:
                    xc = X0 + (i - gx0) * st + (ca + gx0 + gx1) * st / 2
                    yc = Y0 + (j - gy0) * st + (cb + gy0 + gy1) * st / 2
                    best = dict(slack_min=round(score, 2), slack_x=round(sx, 2), slack_y=round(sy, 2), rot=rot,
                                centre=[round(xc, 2), round(yc, 2)], kicad=[round(150 - xc, 2), round(138.94 - yc, 2)],
                                per_side_x=round(MARGIN + sx / 2, 2), per_side_y=round(MARGIN + sy / 2, 2),
                                top=round(MARGIN + min(Hm[jj][ii] for jj in range(j, j + cb) for ii in range(i, i + ca)) - T, 2))
    return best

# candidate cells: (name, source, mAh, L, W, T incl. PCM / tape, price, lead, note)
CELLS = json.load(open(os.path.join(HERE, "battery_cells.json"))) if os.path.exists(os.path.join(HERE, "battery_cells.json")) else []

def render(Hm, who, region, cfg, boxes, fn):
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        return
    X0, X1, Y0, Y1, st = REGIONS[region]
    ny, nx = len(Hm), len(Hm[0])
    sc = max(1, int(round(8 * st / 0.1))) if region == "corner" else 4
    im = Image.new("RGB", (nx * sc, ny * sc), "white")
    d = ImageDraw.Draw(im)
    def col(h):
        if h <= 0.05: return (60, 60, 60)
        if h < 3.8: return (200, 120, 110)
        if h < 5.7: return (240, 200, 120)
        if h < 6.8: return (170, 215, 150)
        return (120, 180, 230)
    for j in range(ny):
        for i in range(nx):
            d.rectangle([i * sc, (ny - 1 - j) * sc, (i + 1) * sc - 1, (ny - j) * sc - 1], fill=col(min(Hm[j][i], 9.9)))
    P = lambda x, y: ((x - X0) / st * sc, (Y1 - y) / st * sc)
    for (x0, x1, y0, y1, label, c) in boxes:
        d.rectangle([P(x0, y1), P(x1, y0)], outline=c, width=3)
        d.text((P(x0, y1)[0] + 4, P(x0, y1)[1] + 4), label, fill=c)
    d.text((6, 6), "%s  free height over the floor (0.3 mm margins): grey 0, red <3.8, amber 3.8-5.7, green 5.7-6.8, blue >6.8" % cfg, fill=(0, 0, 0))
    im.save(fn)

if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    hs = [h / 10 for h in range(20, 96)]
    res, maps = {}, {}
    for cfg, region in (("revD", "corner"), ("lr44_trim", "corner"), ("lr44_ground", "corner"), ("custom", "whole")):
        Hm, who = height_map(cfg, region)
        maps[cfg] = (Hm, who, region)
        env = envelopes(Hm, region, hs)
        steps, last = [], None
        for e in env:                                    # keep only the heights where the box changes
            if last is None or (e["L"], e["W"]) != (last["L"], last["W"]):
                if last is not None:
                    steps.append(last)
            last = e
        if last:
            steps.append(last)
        res[cfg] = dict(region=region, envelopes=steps)
        print("==", cfg)
        for e in steps:
            print("  up to H %.1f: %.1f x %.1f  X %s Y %s  Z %.1f-%.1f  %.2f cm3" % (e["H"], e["L"], e["W"], e["X"], e["Y"], e["Z"][0], e["Z"][1], e["vol_cm3"]))
    fits = []
    for c in CELLS:
        row = dict(c)
        for cfg in maps:
            Hm, who, region = maps[cfg]
            f = fit(Hm, region, c["L"], c["W"], c["T"])
            row[cfg] = f
        fits.append(row)
        print("%-34s %5d mAh %5.1f x %4.1f x %3.1f | " % (c["name"][:34], c["mAh"], c["L"], c["W"], c["T"]) +
              " | ".join("%s %s" % (k, ("side %.2f/%.2f top %.2f" % (row[k]["per_side_x"], row[k]["per_side_y"], row[k]["top"])) if row[k] else "no") for k in maps))
    json.dump(dict(margin=MARGIN, floor_Z=FLOOR, board_Z=[ZB0, ZB1], plate_Z=ZP, configs=res, fits=fits),
              open(os.path.join(HERE, "battery_envelope.json"), "w"), indent=1)
    os.makedirs(os.path.join(HERE, "renders", "battery_upgrade"), exist_ok=True)
    for cfg, (Hm, who, region) in maps.items():
        boxes = []
        for e, c in zip(sorted(res[cfg]["envelopes"], key=lambda e: -e["vol_cm3"])[:2], [(0, 0, 200), (160, 0, 160)]):
            boxes.append((e["X"][0], e["X"][1], e["Y"][0], e["Y"][1], "%.1f x %.1f x %.1f" % (e["L"], e["W"], e["H"]), c))
        render(Hm, who, region, cfg, boxes, os.path.join(HERE, "renders", "battery_upgrade", "envelope_%s.png" % cfg))
    # what limits each envelope: obstacle names just outside its edges / above it
    for cfg, (Hm, who, region) in maps.items():
        X0, X1, Y0, Y1, st = REGIONS[region]
        ny, nx = len(Hm), len(Hm[0])
        for e in res[cfg]["envelopes"]:
            i0, i1 = int(round((e["X"][0] - X0) / st)), int(round((e["X"][1] - X0) / st)) - 1
            j0, j1 = int(round((e["Y"][0] - Y0) / st)), int(round((e["Y"][1] - Y0) / st)) - 1
            lim = {}
            for side, cells in (("-X", [(i0 - 1, j) for j in range(j0, j1 + 1)]), ("+X", [(i1 + 1, j) for j in range(j0, j1 + 1)]),
                                ("-Y", [(i, j0 - 1) for i in range(i0, i1 + 1)]), ("+Y", [(i, j1 + 1) for i in range(i0, i1 + 1)])):
                names = {}
                for i, j in cells:
                    if 0 <= i < nx and 0 <= j < ny and Hm[j][i] < e["H"]:
                        names[who[j][i] or "?"] = names.get(who[j][i] or "?", 0) + 1
                lim[side] = sorted(names, key=lambda n: -names[n])[:3]
            tops = {}
            for j in range(j0, j1 + 1):
                for i in range(i0, i1 + 1):
                    if Hm[j][i] < e["H"] + 0.25:
                        tops[who[j][i]] = 1
            lim["top"] = sorted(tops)[:4]
            e["limits"] = lim
            print(cfg, "H", e["H"], lim)
    json.dump(dict(margin=MARGIN, floor_Z=FLOOR, board_Z=[ZB0, ZB1], plate_Z=ZP, configs=res, fits=fits),
              open(os.path.join(HERE, "battery_envelope.json"), "w"), indent=1)
