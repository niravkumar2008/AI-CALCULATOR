"""Stage 11: straighten the 0.05-0.1 mm "staircase" jogs left by the grid routers.

usage: python smooth.py <pcb> [net_prefix,...]
A chain = segments of one net/layer/width joined end to end with no via, pad or branch in between.
Each chain with tiny segments is string-pulled: the longest straight shortcuts that still keep the
net-class clearance (same obstacle maps as pilroute.py) replace the jogs. Ends never move.
Run DRC afterwards (the stage-11 flow does).
"""
import sys, os, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pcbnew, pilroute as pr

mm = pcbnew.ToMM; MM = pcbnew.VECTOR2I_MM
P = sys.argv[1]; pref = sys.argv[2].split(',') if len(sys.argv) > 2 else None
b = pcbnew.LoadBoard(P)

def key(p): return (p.x, p.y)

tracks = [t for t in b.GetTracks() if t.GetClass() == 'PCB_TRACK']
if pref: tracks = [t for t in tracks if any(t.GetNetname().startswith(q) for q in pref)]
byg = {}
for t in tracks: byg.setdefault((t.GetNetCode(), t.GetLayer()), []).append(t)
vias = {}
for v in b.GetTracks():
    if v.GetClass() == 'PCB_VIA': vias.setdefault(v.GetNetCode(), []).append(key(v.GetPosition()))
pads = {}
for f in b.GetFootprints():
    for p in f.Pads(): pads.setdefault(p.GetNetCode(), []).append(p)

done = 0; removed = 0; added = 0
for (code, layer), ts in byg.items():
    adj = {}
    for t in ts:
        for e in (key(t.GetStart()), key(t.GetEnd())): adj.setdefault(e, []).append(t)
    vset = set(vias.get(code, []))
    def stop(n):
        if len(adj[n]) != 2 or n in vset: return True
        if adj[n][0].GetWidth() != adj[n][1].GetWidth(): return True
        pt = pcbnew.VECTOR2I(n[0], n[1])
        return any(p.IsOnLayer(layer) and p.HitTest(pt) for p in pads.get(code, []))
    seen = set()
    for t0 in ts:
        if id(t0) in seen: continue
        # walk to one end of the chain
        chain = [t0]; seen.add(id(t0))
        for direction in (0, 1):
            cur = t0; n = key(t0.GetEnd()) if direction == 0 else key(t0.GetStart())
            while not stop(n):
                nxt = [t for t in adj[n] if t is not cur][0]
                if id(nxt) in seen: break
                seen.add(id(nxt)); (chain.append if direction == 0 else lambda x: chain.insert(0, x))(nxt)
                cur = nxt; n = key(nxt.GetEnd()) if key(nxt.GetStart()) == n else key(nxt.GetStart())
        if len(chain) < 3 or not any(mm(t.GetLength()) < 0.2 for t in chain): continue
        # ordered point list
        a0 = chain[0]
        if len(chain) > 1 and key(a0.GetStart()) in (key(chain[1].GetStart()), key(chain[1].GetEnd())):
            pts = [key(a0.GetEnd()), key(a0.GetStart())]
        else:
            pts = [key(a0.GetStart()), key(a0.GetEnd())]
        for t in chain[1:]:
            pts.append(key(t.GetEnd()) if key(t.GetStart()) == pts[-1] else key(t.GetStart()))
        P_ = [(mm(x), mm(y)) for x, y in pts]
        xs = [p[0] for p in P_]; ys = [p[1] for p in P_]
        w = mm(a0.GetWidth()); net = a0.GetNetname()
        R = pr.Router(b, (min(xs) - 1.5, min(ys) - 1.5, max(xs) + 1.5, max(ys) + 1.5), G=0.025)
        _, clr = R.netclass_of(net)
        # leave the chain itself out of the obstacle map (it is the same net anyway)
        blk, _ = R.masks(code, w, clr)
        li = pr.LAYERS.index(layer); bl = blk[li]
        out = [P_[0]]; i = 0
        while i < len(P_) - 1:
            j = len(P_) - 1
            while j > i + 1 and not R.free_seg(bl, P_[i], P_[j]): j -= 1
            out.append(P_[j]); i = j
        if len(out) >= len(P_): continue
        for t in chain: b.Remove(t); removed += 1
        pr.polyline(b, net, out, li, w); added += len(out) - 1; done += 1
b.Save(P)
print(f'chains straightened {done}, segments {removed} -> {added}')
