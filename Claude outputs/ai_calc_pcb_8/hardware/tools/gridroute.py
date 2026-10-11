"""Small two-layer grid router (A*) used for pre-routing the critical nets and for leftovers.

Obstacles are rasterised from the board on a G-mm grid. Each call routes one connection between
two point sets, keeping `clear` from other nets everywhere except inside `relief` discs around
the end points, where `relief_clear` applies (so fine-pitch pads can be entered).
"""
import math, heapq, json
import numpy as np
import shapely
import pcbnew
from shapely.geometry import Point, LineString, Polygon as SP
from shapely.ops import unary_union

mm = pcbnew.ToMM; MM = pcbnew.VECTOR2I_MM
LAYERS = [pcbnew.F_Cu, pcbnew.B_Cu]
VIA_D, VIA_DR = 0.6, 0.3
F = json.load(open('/home/claude/pcb/out/hardware/geometry/features.json'))
OUTLINE = SP(F['board_outline_mm']).buffer(0)


class Router:
    def __init__(self, board, G=0.1):
        self.b = board; self.G = G
        x0, y0, x1, y1 = OUTLINE.bounds
        self.X0, self.Y0 = x0, y0
        self.NX, self.NY = int((x1 - x0) / G) + 2, int((y1 - y0) / G) + 2
        xs = x0 + np.arange(self.NX) * G; ys = y0 + np.arange(self.NY) * G
        self.XX, self.YY = np.meshgrid(xs, ys)

    # ---------------- geometry of everything not on `netcode`
    def obstacles(self, netcode, extra_forbid=None):
        per = {l: [] for l in LAYERS}; holes = []
        b = self.b
        for t in b.GetTracks():
            if t.GetNetCode() == netcode:
                continue
            w = mm(t.GetWidth())
            if t.GetClass() == 'PCB_VIA':
                p = t.GetPosition(); g = Point(mm(p.x), mm(p.y)).buffer(w / 2)
                per[LAYERS[0]].append(g); per[LAYERS[1]].append(g)
            else:
                s, e = t.GetStart(), t.GetEnd()
                per[t.GetLayer()].append(LineString([(mm(s.x), mm(s.y)), (mm(e.x), mm(e.y))]).buffer(w / 2))
        for fp in b.GetFootprints():
            for pad in fp.Pads():
                same = netcode > 0 and pad.GetNetCode() == netcode
                if pad.GetDrillSizeX() > 0 and not same:
                    p = pad.GetPosition()
                    holes.append(Point(mm(p.x), mm(p.y)).buffer(mm(pad.GetDrillSizeX()) / 2))
                if same:
                    continue
                for l in LAYERS:
                    if not pad.IsOnLayer(l):
                        continue
                    ps = pad.GetEffectivePolygon(l)
                    for k in range(ps.OutlineCount()):
                        o = ps.Outline(k)
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
                hs = []
                for h in range(ol.HoleCount(k)):
                    hh = ol.Hole(k, h); hs.append([(mm(hh.CPoint(i).x), mm(hh.CPoint(i).y)) for i in range(hh.PointCount())])
                g = SP(pts, hs).buffer(0)
                for l in LAYERS:
                    if z.IsOnLayer(l):
                        keep[l].append((g, z.GetDoNotAllowTracks(), z.GetDoNotAllowVias()))
        if extra_forbid:
            for l, g in extra_forbid:
                keep[l].append((g, True, True))
        return per, holes, keep

    def _mask(self, geoms, grow):
        if not geoms:
            return np.zeros((self.NY, self.NX), bool)
        u = unary_union([g.buffer(grow) for g in geoms])
        return shapely.contains_xy(u, self.XX, self.YY)

    def route(self, net, a_pts, b_pts, width, clear, relief=1.0, relief_clear=0.17,
              b_cost=1.0, via_cost=40.0, extra_forbid=None, max_nodes=6_000_000, prefer=None):
        G = self.G; mg = G * 0.75
        per, holes, keep = self.obstacles(net.GetNetCode(), extra_forbid)
        ends = [Point(x, y) for x, y, _ in list(a_pts) + list(b_pts)]
        reliefm = self._mask(ends, relief)
        inside_t = shapely.contains_xy(OUTLINE.buffer(-0.35 - width / 2), self.XX, self.YY)
        inside_v = shapely.contains_xy(OUTLINE.buffer(-0.35 - VIA_D / 2), self.XX, self.YY)
        blk = []
        for l in LAYERS:
            strict = self._mask(per[l], clear + width / 2 + mg)
            loose = self._mask(per[l], relief_clear + width / 2 + mg)
            m = np.where(reliefm, loose, strict) | ~inside_t | self._mask(holes, 0.25 + width / 2 + mg)
            for g, tr, vi in keep[l]:
                if tr:
                    m |= shapely.contains_xy(g.buffer(width / 2), self.XX, self.YY)
            blk.append(m)
        vb = ~inside_v | self._mask(holes, 0.25 + VIA_D / 2 + mg)
        for l in LAYERS:
            vb |= self._mask(per[l], max(clear, 0.2) + VIA_D / 2 + mg)
            for g, tr, vi in keep[l]:
                if vi:
                    vb |= shapely.contains_xy(g.buffer(VIA_D / 2), self.XX, self.YY)

        def cell(x, y):
            return int(round((y - self.Y0) / G)), int(round((x - self.X0) / G))
        starts = [cell(x, y) + (LAYERS.index(l),) for x, y, l in a_pts]
        goals = set(cell(x, y) + (LAYERS.index(l),) for x, y, l in b_pts)
        for (r, c, li) in starts + list(goals):
            blk[li][max(r - 1, 0):r + 2, max(c - 1, 0):c + 2] = False
        gl = list(goals)

        def h(r, c):
            return min(math.hypot(r - g[0], c - g[1]) for g in gl[:24])
        lc = [1.0, b_cost]
        openq = []; came = {}; gcost = {}
        for s in starts:
            gcost[s] = 0; heapq.heappush(openq, (h(s[0], s[1]), 0, s))
        moves = [(0, 1, 1), (1, 0, 1), (0, -1, 1), (-1, 0, 1), (1, 1, 1.414), (1, -1, 1.414), (-1, 1, 1.414), (-1, -1, 1.414)]
        found = None; n = 0
        while openq and n < max_nodes:
            f, gc, cur = heapq.heappop(openq); n += 1
            if cur in goals:
                found = cur; break
            if gc > gcost.get(cur, 1e18):
                continue
            r, c, li = cur
            for dr, dc, w in moves:
                nr, nc = r + dr, c + dc
                if 0 <= nr < self.NY and 0 <= nc < self.NX and not blk[li][nr, nc]:
                    # diagonal moves must not cut a blocked corner
                    if dr and dc and (blk[li][r + dr, c] or blk[li][r, c + dc]):
                        continue
                    nxt = (nr, nc, li); ng = gc + w * lc[li]
                    if ng < gcost.get(nxt, 1e18):
                        gcost[nxt] = ng; came[nxt] = cur; heapq.heappush(openq, (ng + h(nr, nc), ng, nxt))
            if not vb[r, c] and not blk[1 - li][r, c]:
                nxt = (r, c, 1 - li); ng = gc + via_cost
                if ng < gcost.get(nxt, 1e18):
                    gcost[nxt] = ng; came[nxt] = cur; heapq.heappush(openq, (ng + h(r, c), ng, nxt))
        if not found:
            return None
        path = [found]
        while path[-1] in came:
            path.append(came[path[-1]])
        path.reverse()
        return self._commit(net, path, width)

    def _commit(self, net, path, width):
        b = self.b; G = self.G
        def xy(p):
            return self.X0 + p[1] * G, self.Y0 + p[0] * G
        added = []; i = 0
        while i < len(path) - 1:
            j = i + 1
            d = (path[j][0] - path[i][0], path[j][1] - path[i][1], path[j][2] - path[i][2])
            if d[2] != 0:
                v = pcbnew.PCB_VIA(b); v.SetPosition(MM(*xy(path[i]))); v.SetWidth(pcbnew.FromMM(VIA_D))
                v.SetDrill(pcbnew.FromMM(VIA_DR)); v.SetNet(net); b.Add(v); added.append(v); i = j; continue
            while j + 1 < len(path) and (path[j + 1][0] - path[j][0], path[j + 1][1] - path[j][1], path[j + 1][2] - path[j][2]) == d:
                j += 1
            t = pcbnew.PCB_TRACK(b); t.SetStart(MM(*xy(path[i]))); t.SetEnd(MM(*xy(path[j])))
            t.SetWidth(pcbnew.FromMM(width)); t.SetLayer(LAYERS[path[i][2]]); t.SetNet(net); b.Add(t); added.append(t)
            i = j
        return added


def pad_pts(pad):
    p = pad.GetPosition()
    return [(mm(p.x), mm(p.y), l) for l in LAYERS if pad.IsOnLayer(l)]


def net_pts(board, netcode):
    pts = []
    for t in board.GetTracks():
        if t.GetNetCode() != netcode:
            continue
        if t.GetClass() == 'PCB_VIA':
            p = t.GetPosition(); pts += [(mm(p.x), mm(p.y), l) for l in LAYERS]
        else:
            s, e = t.GetStart(), t.GetEnd(); L = t.GetLength()
            n = max(1, int(mm(L) / 0.5))
            for k in range(n + 1):
                x = mm(s.x) + (mm(e.x) - mm(s.x)) * k / n; y = mm(s.y) + (mm(e.y) - mm(s.y)) * k / n
                pts.append((x, y, t.GetLayer()))
    return pts
