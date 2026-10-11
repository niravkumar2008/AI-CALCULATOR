"""Small two-layer A* grid router that needs only KiCad's own python (pcbnew + numpy + PIL).

Stage 11 replacement for gridroute.py/fixroute.py on Windows, where shapely and Freerouting
(Java 17) are not available. Obstacles come from KiCad's own shapes (TransformShapeToPolygon
with the right clearance), rasterised with PIL on a G-mm grid inside a window. The found grid
path is "string-pulled" into straight segments, so the result looks hand-routed, not staircased.

    import pilroute as pr
    b = pcbnew.LoadBoard(pcb)
    R = pr.Router(b, (x0, y0, x1, y1), G=0.05)
    R.route('EPD_BUSY', pr.pad_pts(b, 'U1', '28'), pr.pad_pts(b, 'J2', '9'))
    b.Save(pcb)

Clearance = max(own netclass, other item's) + width/2, holes 0.25 mm, board edge 0.3 mm,
rule areas (keep-outs, key areas = no vias) respected. Same-net copper is free to cross.
"""
import math, heapq
import numpy as np
import pcbnew
from PIL import Image, ImageDraw

mm = pcbnew.ToMM; MM = pcbnew.VECTOR2I_MM; FM = pcbnew.FromMM
LAYERS = [pcbnew.F_Cu, pcbnew.B_Cu]
VIA_D, VIA_DR = 0.6, 0.3
EDGE_CLR, HOLE_CLR = 0.3, 0.25


def poly_pts(o):
    return [(mm(o.CPoint(i).x), mm(o.CPoint(i).y)) for i in range(o.PointCount())]


class Router:
    def __init__(self, board, win, G=0.05, margin=None):
        self.b = board; self.G = G
        self.x0, self.y0, self.x1, self.y1 = win
        self.W = int((self.x1 - self.x0) / G) + 1; self.H = int((self.y1 - self.y0) / G) + 1
        self.mg = margin if margin is not None else G * 0.75
        ol = pcbnew.SHAPE_POLY_SET(); board.GetBoardPolygonOutlines(ol, False); self.outline = ol
        self.ds = board.GetDesignSettings()

    # ------------------------------------------------------------- raster helpers
    def _px(self, x, y):
        return ((x - self.x0) / self.G + 0.5, (y - self.y0) / self.G + 0.5)

    def _new(self):
        im = Image.new('1', (self.W, self.H), 0); return im, ImageDraw.Draw(im)

    def _polyset(self, tgt, ps, fill=1, direct=False):
        d = tgt[1]
        for k in range(ps.OutlineCount()):
            pts = [self._px(*p) for p in poly_pts(ps.Outline(k))]
            if len(pts) < 3: continue
            if direct:
                d.polygon(pts, fill=1)
                for h in range(ps.HoleCount(k)):
                    d.polygon([self._px(*p) for p in poly_pts(ps.Hole(k, h))], fill=0)
                continue
            if ps.HoleCount(k) == 0 or fill == 0:
                d.polygon(pts, fill=fill); continue
            # outline with holes: draw on a scratch image, then paste only the set pixels
            # (drawing the holes straight onto d would erase obstacles already drawn there)
            im, dd = self._new(); dd.polygon(pts, fill=1)
            for h in range(ps.HoleCount(k)):
                hp = [self._px(*p) for p in poly_pts(ps.Hole(k, h))]
                if len(hp) > 2: dd.polygon(hp, fill=0)
            tgt[0].paste(1, None, im)

    def _disc(self, d, x, y, r, fill=1):
        (cx, cy) = self._px(x, y); rr = r / self.G
        d.ellipse([cx - rr, cy - rr, cx + rr, cy + rr], fill=fill)

    def _near(self, bb, grow):
        return not (mm(bb.GetRight()) + grow < self.x0 or mm(bb.GetLeft()) - grow > self.x1 or
                    mm(bb.GetBottom()) + grow < self.y0 or mm(bb.GetTop()) - grow > self.y1)

    def netclass_of(self, netname):
        # read the net classes from the .kicad_pro (the SWIG NETCLASS object isn't usable here)
        if not hasattr(self, '_nc'):
            import json, os, fnmatch
            pro = json.load(open(os.path.splitext(self.b.GetFileName())[0] + '.kicad_pro'))
            ns = pro['net_settings']
            self._cls = {c['name']: (c['track_width'], c['clearance']) for c in ns['classes']}
            self._nc = [(p['pattern'], p['netclass']) for p in ns.get('netclass_patterns', [])]
            self._fn = fnmatch.fnmatchcase
        for pat, cls in self._nc:
            if self._fn(netname, pat):
                return self._cls[cls]
        return self._cls['Default']

    # ------------------------------------------------------------- obstacle maps
    def masks(self, netcode, width, clear, extra=None, via_clear=None):
        """blk[li] = cell centre can't hold a track centre; vb = cell can't hold a via."""
        hw = width / 2; vr = VIA_D / 2; mg = self.mg
        vclear = clear if via_clear is None else via_clear
        tr = [self._new() for _ in LAYERS]; vi = self._new()
        b = self.b
        items = []
        for t in b.GetTracks():
            if t.GetNetCode() != netcode or netcode <= 0:
                items.append(t)
        for fp in b.GetFootprints():
            for p in fp.Pads():
                if p.GetNetCode() != netcode or netcode <= 0 or p.GetNetCode() == 0:
                    items.append(p)
                if p.HasHole():   # hole clearance applies to every hole, own net too (via-in-hole)
                    if self._near(p.GetBoundingBox(), 2):
                        for li in range(2):
                            ps = pcbnew.SHAPE_POLY_SET()
                            p.TransformHoleToPolygon(ps, FM(HOLE_CLR + hw + mg), FM(0.005), pcbnew.ERROR_OUTSIDE)
                            self._polyset(tr[li], ps)
                        ps = pcbnew.SHAPE_POLY_SET()
                        p.TransformHoleToPolygon(ps, FM(HOLE_CLR + vr + mg), FM(0.005), pcbnew.ERROR_OUTSIDE)
                        self._polyset(vi, ps)
        for it in items:
            if not self._near(it.GetBoundingBox(), 2):
                continue
            for li, L in enumerate(LAYERS):
                if not it.IsOnLayer(L):
                    continue
                oc = mm(it.GetOwnClearance(L))
                c = max(clear, oc)
                ps = pcbnew.SHAPE_POLY_SET()
                it.TransformShapeToPolygon(ps, L, FM(c + hw + mg), FM(0.005), pcbnew.ERROR_OUTSIDE)
                self._polyset(tr[li], ps)
                ps = pcbnew.SHAPE_POLY_SET()
                it.TransformShapeToPolygon(ps, L, FM(max(vclear, oc) + vr + mg), FM(0.005), pcbnew.ERROR_OUTSIDE)
                self._polyset(vi, ps)
        # board edge (outline + internal slots)
        ins_t = self._new(); ins_v = self._new()
        self._polyset(ins_t, self.outline, direct=True); self._polyset(ins_v, self.outline, direct=True)
        for k in range(self.outline.OutlineCount()):
            rings = [self.outline.Outline(k)] + [self.outline.Hole(k, h) for h in range(self.outline.HoleCount(k))]
            for r in rings:
                pts = [self._px(*p) for p in poly_pts(r)]; pts.append(pts[0])
                for dd, g in ((ins_t[1], EDGE_CLR + hw + mg), (ins_v[1], EDGE_CLR + vr + mg)):
                    w = int(round(2 * g / self.G)) + 1
                    dd.line(pts, fill=0, width=w)
                    for q in pts:
                        rr = g / self.G; dd.ellipse([q[0] - rr, q[1] - rr, q[0] + rr, q[1] + rr], fill=0)
        # rule areas
        zones = list(b.Zones()) + [z for fp in b.GetFootprints() for z in fp.Zones()]
        for z in zones:
            if not z.GetIsRuleArea() or not self._near(z.GetBoundingBox(), 2):
                continue
            for li, L in enumerate(LAYERS):
                if not z.IsOnLayer(L):
                    continue
                if z.GetDoNotAllowTracks():
                    ps = pcbnew.SHAPE_POLY_SET(z.Outline())
                    ps.Inflate(FM(hw + mg), pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS, FM(0.005))
                    self._polyset(tr[li], ps)
                if z.GetDoNotAllowVias():
                    ps = pcbnew.SHAPE_POLY_SET(z.Outline())
                    ps.Inflate(FM(vr + mg), pcbnew.CORNER_STRATEGY_ROUND_ALL_CORNERS, FM(0.005))
                    self._polyset(vi, ps)
        blk = [np.array(tr[li][0], dtype=bool) | ~np.array(ins_t[0], dtype=bool) for li in range(2)]
        vb = np.array(vi[0], dtype=bool) | ~np.array(ins_v[0], dtype=bool)
        if extra:   # extra: list of (li or None, polygon pts) forbidden for tracks (li) / vias (None)
            for li, pts in extra:
                im, d = self._new(); d.polygon([self._px(*p) for p in pts], fill=1)
                a = np.array(im, dtype=bool)
                if li is None: vb |= a
                else: blk[li] |= a
        return blk, vb

    # ------------------------------------------------------------- search
    def cell(self, x, y):
        return int(round((y - self.y0) / self.G)), int(round((x - self.x0) / self.G))

    def xy(self, r, c):
        return self.x0 + c * self.G, self.y0 + r * self.G

    def route(self, netname, A, B, width=None, clear=None, via_cost=1.2, b_cost=1.0, f_cost=1.0,
              layers=(0, 1), extra=None, max_expand=4_000_000, commit=True, via_clear=None):
        """A, B: lists of (x, y, li) anchor points (exact copper positions). Returns added items."""
        net = self.b.FindNet(netname)
        w0, c0 = self.netclass_of(netname)
        width = width or w0; clear = clear if clear is not None else c0
        blk, vb = self.masks(net.GetNetCode(), width, clear, extra, via_clear)
        for li in (0, 1):
            if li not in layers: blk[li][:] = True
        G = self.G
        starts = []; goals = {}
        for (x, y, li) in A:
            r, c = self.cell(x, y)
            if 0 <= r < self.H and 0 <= c < self.W and li in layers: starts.append((r, c, li, x, y))
        for (x, y, li) in B:
            r, c = self.cell(x, y)
            if 0 <= r < self.H and 0 <= c < self.W and li in layers: goals[(r, c, li)] = (x, y)
        for (r, c, li, *_ ) in starts: blk[li][r, c] = False
        for (r, c, li) in goals: blk[li][r, c] = False
        self._blk, self._vb = blk, vb
        gl = list(goals.keys())
        step = max(1, len(gl) // 40); gs = gl[::step]
        def h(r, c):
            return min(max(abs(r - g[0]), abs(c - g[1])) + 0.414 * min(abs(r - g[0]), abs(c - g[1])) for g in gs)
        lc = [f_cost, b_cost]; vc = via_cost / G
        gcost = np.full((2, self.H, self.W), np.inf); came = {}
        oq = []
        for (r, c, li, x, y) in starts:
            gcost[li, r, c] = 0; heapq.heappush(oq, (h(r, c), 0.0, (r, c, li)))
        moves = [(0, 1, 1), (1, 0, 1), (0, -1, 1), (-1, 0, 1), (1, 1, 1.4142), (1, -1, 1.4142), (-1, 1, 1.4142), (-1, -1, 1.4142)]
        found = None; n = 0
        H, W = self.H, self.W
        while oq and n < max_expand:
            f, gc, cur = heapq.heappop(oq); n += 1
            r, c, li = cur
            if gc > gcost[li, r, c]: continue
            if cur in goals: found = cur; break
            bl = blk[li]
            for dr, dc, w in moves:
                nr, nc = r + dr, c + dc
                if 0 <= nr < H and 0 <= nc < W and not bl[nr, nc]:
                    if dr and dc and (bl[r + dr, c] or bl[r, c + dc]): continue
                    ng = gc + w * lc[li]
                    if ng < gcost[li, nr, nc]:
                        gcost[li, nr, nc] = ng; came[(nr, nc, li)] = cur
                        heapq.heappush(oq, (ng + h(nr, nc) * min(lc), ng, (nr, nc, li)))
            ol = 1 - li
            if not vb[r, c] and not blk[ol][r, c]:
                ng = gc + vc
                if ng < gcost[ol, r, c]:
                    gcost[ol, r, c] = ng; came[(r, c, ol)] = cur
                    heapq.heappush(oq, (ng + h(r, c) * min(lc), ng, (r, c, ol)))
        self.expanded = n
        if not found: return None
        path = [found]
        while path[-1] in came: path.append(came[path[-1]])
        path.reverse()
        s0 = [s for s in starts if (s[0], s[1], s[2]) == path[0]][0]
        pts = [(s0[3], s0[4], path[0][2])] + [(*self.xy(r, c), li) for (r, c, li) in path] + [(*goals[found], found[2])]
        segs, vias = self._pull(pts, blk)
        if not commit: return segs, vias
        return self._commit(net, segs, vias, width)

    def free_seg(self, blk_l, a, b):
        L = math.hypot(b[0] - a[0], b[1] - a[1]); n = max(1, int(L / (self.G * 0.4)))
        for k in range(n + 1):
            x = a[0] + (b[0] - a[0]) * k / n; y = a[1] + (b[1] - a[1]) * k / n
            r, c = self.cell(x, y)
            if not (0 <= r < self.H and 0 <= c < self.W) or blk_l[r, c]:
                return False
        return True

    def _pull(self, pts, blk):
        # split into same-layer runs; a via sits where the layer changes
        runs = [[pts[0]]]; vias = []
        for p in pts[1:]:
            if p[2] != runs[-1][-1][2]:
                vias.append((runs[-1][-1][0], runs[-1][-1][1])); runs.append([(runs[-1][-1][0], runs[-1][-1][1], p[2])])
            runs[-1].append(p)
        segs = []
        for run in runs:
            li = run[0][2]; bl = blk[li]
            # the exact anchors may sit inside relief cells; allow them as endpoints
            i = 0
            while i < len(run) - 1:
                j = len(run) - 1
                while j > i + 1 and not self.free_seg(bl, run[i][:2], run[j][:2]):
                    j -= 1
                a, bb = run[i][:2], run[j][:2]
                if math.hypot(bb[0] - a[0], bb[1] - a[1]) > 1e-4:
                    segs.append((a, bb, li))
                i = j
        return segs, vias

    def _commit(self, net, segs, vias, width):
        b = self.b; added = []
        for (a, c, li) in segs:
            t = pcbnew.PCB_TRACK(b); t.SetStart(MM(*a)); t.SetEnd(MM(*c)); t.SetWidth(FM(width))
            t.SetLayer(LAYERS[li]); t.SetNet(net); b.Add(t); added.append(t)
        for (x, y) in vias:
            v = pcbnew.PCB_VIA(b); v.SetPosition(MM(x, y)); v.SetWidth(FM(VIA_D)); v.SetDrill(FM(VIA_DR))
            v.SetNet(net); b.Add(v); added.append(v)
        return added


def pad_pts(b, ref, num):
    fp = b.FindFootprintByReference(ref)
    out = []
    for p in fp.Pads():
        if p.GetNumber() == num:
            q = p.GetPosition()
            out += [(mm(q.x), mm(q.y), li) for li, L in enumerate(LAYERS) if p.IsOnLayer(L)]
    return out


def net_pts(b, netname, step=0.25, layers=(0, 1), only=None):
    """anchor points along every track/via/pad of a net (optionally only items for which only(item) is True)"""
    code = b.FindNet(netname).GetNetCode(); pts = []
    for t in b.GetTracks():
        if t.GetNetCode() != code or (only and not only(t)): continue
        if t.GetClass() == 'PCB_VIA':
            q = t.GetPosition(); pts += [(mm(q.x), mm(q.y), li) for li in layers]
        else:
            li = LAYERS.index(t.GetLayer())
            if li not in layers: continue
            s, e = t.GetStart(), t.GetEnd(); n = max(1, int(mm(t.GetLength()) / step))
            for k in range(n + 1):
                pts.append((mm(s.x) + (mm(e.x) - mm(s.x)) * k / n, mm(s.y) + (mm(e.y) - mm(s.y)) * k / n, li))
    for fp in b.GetFootprints():
        for p in fp.Pads():
            if p.GetNetCode() == code and not (only and not only(p)):
                q = p.GetPosition(); pts += [(mm(q.x), mm(q.y), li) for li, L in enumerate(LAYERS) if p.IsOnLayer(L) and li in layers]
    return pts


def islands(b, netname):
    """Copper islands of one net (tracks, vias, pads; zones ignored), by geometric contact.
    Returns a list of lists of items."""
    code = b.FindNet(netname).GetNetCode()
    items = [t for t in b.GetTracks() if t.GetNetCode() == code]
    items += [p for fp in b.GetFootprints() for p in fp.Pads() if p.GetNetCode() == code]
    par = list(range(len(items)))
    def find(i):
        while par[i] != i:
            par[i] = par[par[i]]; i = par[i]
        return i
    def pts(it):
        if it.GetClass() == 'PCB_TRACK' or it.GetClass() == 'PCB_ARC':
            return [it.GetStart(), it.GetEnd()]
        return [it.GetPosition()]
    for i, a in enumerate(items):
        for j in range(i + 1, len(items)):
            c = items[j]
            if not any(a.IsOnLayer(L) and c.IsOnLayer(L) for L in LAYERS): continue
            if not a.GetBoundingBox().Intersects(c.GetBoundingBox()): continue
            hit = False
            for L in LAYERS:
                if not (a.IsOnLayer(L) and c.IsOnLayer(L)): continue
                for p in pts(c):
                    if a.HitTest(p, 0): hit = True; break
                if not hit:
                    for p in pts(a):
                        if c.HitTest(p, 0): hit = True; break
                if hit: break
            if hit: par[find(i)] = find(j)
    groups = {}
    for i, it in enumerate(items): groups.setdefault(find(i), []).append(it)
    return list(groups.values())


def item_pts(items, step=0.25):
    out = []
    for t in items:
        if t.GetClass() == 'PCB_VIA':
            q = t.GetPosition(); out += [(mm(q.x), mm(q.y), 0), (mm(q.x), mm(q.y), 1)]
        elif t.GetClass() == 'PAD':
            q = t.GetPosition(); out += [(mm(q.x), mm(q.y), li) for li, L in enumerate(LAYERS) if t.IsOnLayer(L)]
        else:
            li = LAYERS.index(t.GetLayer()); s, e = t.GetStart(), t.GetEnd()
            n = max(1, int(mm(t.GetLength()) / step))
            for k in range(n + 1):
                out.append((mm(s.x) + (mm(e.x) - mm(s.x)) * k / n, mm(s.y) + (mm(e.y) - mm(s.y)) * k / n, li))
    return out


def island_at(b, netname, x, y, tol=0.05):
    for g in islands(b, netname):
        for p in item_pts(g, 0.1):
            if abs(p[0] - x) < tol + 0.1 and abs(p[1] - y) < tol + 0.1:
                return g
    return None


def polyline(b, netname, pts, layer=0, width=None, vias=()):
    """hand-route: straight segments through pts [(x,y),...] on LAYERS[layer]; vias at given points"""
    net = b.FindNet(netname); added = []
    if width is None:
        width = Router(b, (0, 0, 1, 1)).netclass_of(netname)[0]
    for a, c in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(b); t.SetStart(MM(*a)); t.SetEnd(MM(*c)); t.SetWidth(FM(width))
        t.SetLayer(LAYERS[layer]); t.SetNet(net); b.Add(t); added.append(t)
    for (x, y) in vias:
        v = pcbnew.PCB_VIA(b); v.SetPosition(MM(x, y)); v.SetWidth(FM(VIA_D)); v.SetDrill(FM(VIA_DR))
        v.SetNet(net); b.Add(v); added.append(v)
    return added


def connect_all(b, netname, win, G=0.05, log=print, **kw):
    """join every copper island of a net (smallest island first, to all the others)"""
    while True:
        g = islands(b, netname)
        if len(g) < 2:
            return True
        g.sort(key=len)
        src = g[0]; dst = [it for k in g[1:] for it in k]
        R = Router(b, win, G=G)
        r = R.route(netname, item_pts(src), item_pts(dst), **kw)
        if r is None:
            log(f'  {netname}: FAILED ({len(g)} islands, expanded {R.expanded})')
            return False
        log(f'  {netname}: joined island ({len(g)} -> {len(g) - 1}), {len(r)} items')
