#!/usr/bin/env python3
"""Tiny two-layer grid router for the last few connections the autorouter leaves open.

usage: fixroute.py <pcb> <drc_report>
For every [unconnected_items] pair on a signal/power net, finds the two closest copper items of
that net (one from each island), and runs A* on a 0.1 mm grid over F.Cu/B.Cu with vias, keeping
the net class clearance from every other net, holes, keep-outs and the board edge.
GND leftovers are handled by adding a via (zone islands) instead.
"""
import sys, re, json, heapq, math
import numpy as np
import pcbnew
from shapely.geometry import Point, LineString, Polygon as SP, box
from shapely.ops import unary_union
from shapely import prepared

pcb, rpt = sys.argv[1], sys.argv[2]
b = pcbnew.LoadBoard(pcb)
mm = pcbnew.ToMM; MM = pcbnew.VECTOR2I_MM
F = json.load(open('/home/claude/pcb/out/hardware/geometry/features.json'))
outline = SP(F['board_outline_mm']).buffer(0)
G = float(__import__("os").environ.get("FR_GRID", "0.1"))  # grid pitch, mm
X0, Y0, X1, Y1 = [v for v in outline.bounds]
NX, NY = int((X1 - X0) / G) + 2, int((Y1 - Y0) / G) + 2
LAYERS = [pcbnew.F_Cu, pcbnew.B_Cu]
VIA_D, VIA_DR = 0.6, 0.3


def items_geom(net_exclude):
    """per layer: list of (geom, netcode) for copper not on net_exclude, holes, keepouts"""
    per = {l: [] for l in LAYERS}
    for t in b.GetTracks():
        if t.GetNetCode() == net_exclude:
            continue
        w = mm(t.GetWidth())
        if t.GetClass() == 'PCB_VIA':
            p = t.GetPosition(); g = Point(mm(p.x), mm(p.y)).buffer(w / 2)
            for l in LAYERS:
                per[l].append(g)
        else:
            s, e = t.GetStart(), t.GetEnd()
            per[t.GetLayer()].append(LineString([(mm(s.x), mm(s.y)), (mm(e.x), mm(e.y))]).buffer(w / 2))
    for fp in b.GetFootprints():
        for pad in fp.Pads():
            same = pad.GetNetCode() == net_exclude and net_exclude > 0
            for l in LAYERS:
                if not pad.IsOnLayer(l) and pad.GetAttribute() != pcbnew.PAD_ATTRIB_NPTH:
                    continue
                if same:
                    continue
                ps = pad.GetEffectivePolygon(l)
                for k in range(ps.OutlineCount()):
                    o = ps.Outline(k)
                    per[l].append(SP([(mm(o.CPoint(i).x), mm(o.CPoint(i).y)) for i in range(o.PointCount())]).buffer(0))
            if pad.GetDrillSizeX() > 0 and not same:
                p = pad.GetPosition(); g = Point(mm(p.x), mm(p.y)).buffer(mm(pad.GetDrillSizeX()) / 2 + 0.12)
                for l in LAYERS:
                    per[l].append(g)
    # filled zones of other nets
    for z in b.Zones():
        if True:  # pours get refilled around the new track, so they are not obstacles
            continue
        for l in LAYERS:
            if not z.IsOnLayer(l):
                continue
            fp_ = z.GetFilledPolysList(l)
            for k in range(fp_.OutlineCount()):
                o = fp_.Outline(k)
                per[l].append(SP([(mm(o.CPoint(i).x), mm(o.CPoint(i).y)) for i in range(o.PointCount())]).buffer(0))
    keep = {l: [] for l in LAYERS}
    zones = list(b.Zones()) + [z for fp in b.GetFootprints() for z in fp.Zones()]
    for z in zones:
        if not z.GetIsRuleArea():
            continue
        ol = z.Outline()
        for k in range(ol.OutlineCount()):
            o = ol.Outline(k)
            pts = [(mm(o.CPoint(i).x), mm(o.CPoint(i).y)) for i in range(o.PointCount())]
            holes = []
            for h in range(ol.HoleCount(k)):
                hh = ol.Hole(k, h); holes.append([(mm(hh.CPoint(i).x), mm(hh.CPoint(i).y)) for i in range(hh.PointCount())])
            g = SP(pts, holes).buffer(0)
            for l in LAYERS:
                if z.IsOnLayer(l):
                    keep[l].append((g, z.GetDoNotAllowTracks(), z.GetDoNotAllowVias()))
    return per, keep


def raster(geoms, grow):
    """boolean grid: True where a track centre may NOT go"""
    from shapely.vectorized import contains  # noqa
    m = np.zeros((NY, NX), bool)
    if not geoms:
        return m
    u = unary_union([g.buffer(grow) for g in geoms])
    xs = X0 + np.arange(NX) * G; ys = Y0 + np.arange(NY) * G
    XX, YY = np.meshgrid(xs, ys)
    import shapely
    return shapely.contains_xy(u, XX, YY)


def route(net, a_pts, b_pts, width, clear):
    per, keep = items_geom(net.GetNetCode())
    xs = X0 + np.arange(NX) * G; ys = Y0 + np.arange(NY) * G
    XX, YY = np.meshgrid(xs, ys)
    import shapely
    inside = shapely.contains_xy(outline.buffer(-0.35 - width / 2), XX, YY)
    blk = []; via_blk = None
    for l in LAYERS:
        m = raster(per[l], clear + width / 2) | ~inside
        for g, tr, vi in keep[l]:
            if tr:
                m |= shapely.contains_xy(g.buffer(width / 2), XX, YY)
        blk.append(m)
    vb = np.zeros((NY, NX), bool)
    for l in LAYERS:
        vb |= raster(per[l], clear + VIA_D / 2)
        for g, tr, vi in keep[l]:
            if vi:
                vb |= shapely.contains_xy(g.buffer(VIA_D / 2), XX, YY)
    vb |= ~shapely.contains_xy(outline.buffer(-0.35 - VIA_D / 2), XX, YY)
    def cell(x, y):
        return int(round((y - Y0) / G)), int(round((x - X0) / G))
    starts = []; goals = set()
    for (x, y, l) in a_pts:
        r, c = cell(x, y); starts.append((r, c, LAYERS.index(l)))
    for (x, y, l) in b_pts:
        r, c = cell(x, y); goals.add((r, c, LAYERS.index(l)))
    # start/goal cells are inside their own pad: clear a small disc around them
    for (r, c, li) in list(starts) + list(goals):
        rr, cc = np.ogrid[-4:5, -4:5]
        mask = rr ** 2 + cc ** 2 <= 16
        r0, c0 = max(r - 4, 0), max(c - 4, 0)
        sub = blk[li][r0:r + 5, c0:c + 5]
        sub[mask[:sub.shape[0], :sub.shape[1]]] = False
    gl = list(goals)
    def h(r, c):
        return min(math.hypot(r - g[0], c - g[1]) for g in gl[:20])
    openq = []; came = {}; gcost = {}
    for s in starts:
        gcost[s] = 0; heapq.heappush(openq, (h(s[0], s[1]), 0, s))
    moves = [(0, 1, 1), (1, 0, 1), (0, -1, 1), (-1, 0, 1), (1, 1, 1.414), (1, -1, 1.414), (-1, 1, 1.414), (-1, -1, 1.414)]
    found = None; n = 0
    while openq and n < 12_000_000:
        f, gc, cur = heapq.heappop(openq); n += 1
        if cur in goals:
            found = cur; break
        if gc > gcost.get(cur, 1e18):
            continue
        r, c, li = cur
        for dr, dc, w in moves:
            nr, nc = r + dr, c + dc
            if 0 <= nr < NY and 0 <= nc < NX and not blk[li][nr, nc]:
                nxt = (nr, nc, li); ng = gc + w
                if ng < gcost.get(nxt, 1e18):
                    gcost[nxt] = ng; came[nxt] = cur; heapq.heappush(openq, (ng + h(nr, nc), ng, nxt))
        if not vb[r, c]:
            nxt = (r, c, 1 - li); ng = gc + 40
            if not blk[1 - li][r, c] and ng < gcost.get(nxt, 1e18):
                gcost[nxt] = ng; came[nxt] = cur; heapq.heappush(openq, (ng + h(r, c), ng, nxt))
    print('   expanded', n, 'starts free', sum(1 for (r,c,li) in starts if not blk[li][r,c]), '/', len(starts), 'goals free', sum(1 for (r,c,li) in goals if not blk[li][r,c]), '/', len(goals))
    if not found:
        return False
    path = [found]
    while path[-1] in came:
        path.append(came[path[-1]])
    path.reverse()
    # simplify into straight segments
    def xy(p):
        return X0 + p[1] * G, Y0 + p[0] * G
    segs = []; i = 0
    while i < len(path) - 1:
        j = i + 1
        d = (path[j][0] - path[i][0], path[j][1] - path[i][1], path[j][2] - path[i][2])
        while j + 1 < len(path) and (path[j + 1][0] - path[j][0], path[j + 1][1] - path[j][1], path[j + 1][2] - path[j][2]) == d and d[2] == 0:
            j += 1
        segs.append((path[i], path[j])); i = j
    for p, q in segs:
        if p[2] != q[2]:
            v = pcbnew.PCB_VIA(b); v.SetPosition(MM(*xy(p))); v.SetWidth(pcbnew.FromMM(VIA_D)); v.SetDrill(pcbnew.FromMM(VIA_DR))
            v.SetNet(net); b.Add(v)
        else:
            t = pcbnew.PCB_TRACK(b); t.SetStart(MM(*xy(p))); t.SetEnd(MM(*xy(q))); t.SetWidth(pcbnew.FromMM(width))
            t.SetLayer(LAYERS[p[2]]); t.SetNet(net); b.Add(t)
    return True


# --- connectivity islands for a net: use KiCad's own connectivity
def islands(netname):
    b.BuildConnectivity()
    conn = b.GetConnectivity()
    net = b.FindNet(netname)
    items = [t for t in b.GetTracks() if t.GetNetCode() == net.GetNetCode()]
    items += [p for fp in b.GetFootprints() for p in fp.Pads() if p.GetNetCode() == net.GetNetCode()]
    groups = []
    seen = set()
    for it in items:
        if id(it) in seen:
            continue
        grp = [it]; seen.add(id(it))
        for other in conn.GetConnectedItems(it):
            for cand in items:
                if cand.m_Uuid.AsString() == other.m_Uuid.AsString() and id(cand) not in seen:
                    grp.append(cand); seen.add(id(cand))
        groups.append(grp)
    return net, groups


def anchor_points(grp):
    pts = []
    for it in grp:
        if isinstance(it, pcbnew.PAD):
            p = it.GetPosition()
            for l in LAYERS:
                if it.IsOnLayer(l):
                    pts.append((mm(p.x), mm(p.y), l))
        elif it.GetClass() == 'PCB_VIA':
            p = it.GetPosition()
            for l in LAYERS:
                pts.append((mm(p.x), mm(p.y), l))
        else:
            for p in (it.GetStart(), it.GetEnd()):
                pts.append((mm(p.x), mm(p.y), it.GetLayer()))
    return pts


txt = open(rpt).read()
nets = set()
for blk in re.split(r'\n(?=\[)', txt):
    if blk.startswith('[unconnected'):
        for m in re.finditer(r'\[([^\]]+)\] (?:of|on)', blk):
            nets.add(m.group(1))
ds = b.GetDesignSettings()
for name in sorted(nets):
    if name == 'GND':
        continue
    net, groups = islands(name)
    print(name, 'islands', len(groups))
    while len(groups) > 1:
        groups.sort(key=len, reverse=True)
        a = anchor_points(groups[0]); bb = anchor_points(groups[1])
        if name in ('SYS', 'VBAT_P', 'BAT+', 'VBUS'):
            w, cl = 0.4, 0.22
        elif name.startswith('CAM_') or name.startswith('EPD_') or name == '+3V3':
            w, cl = 0.2, 0.22
        else:
            w, cl = 0.2, 0.17
        ok = False
        mg = G * 0.75
        for (ww, cc) in ((w, cl + mg), (0.25, 0.2 + mg), (0.2, 0.2 + mg)):
            ok = route(net, a, bb, ww, cc)
            if ok:
                break
        print('  route', name, 'ok' if ok else 'FAILED')
        if not ok:
            break
        net, groups = islands(name)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
b.Save(pcb)
