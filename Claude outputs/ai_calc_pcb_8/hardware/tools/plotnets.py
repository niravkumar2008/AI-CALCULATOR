#!/usr/bin/env python3
"""plot tracks (F red, B blue) and pads for quick looks. usage: plotnets.py pcb out.png [x0 y0 x1 y1] [netprefix,...]"""
import sys, pcbnew
import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
b = pcbnew.LoadBoard(sys.argv[1]); mm = pcbnew.ToMM
x0, y0, x1, y1 = (float(v) for v in sys.argv[3:7]) if len(sys.argv) > 6 else (108, 56, 192, 214)
pref = sys.argv[7].split(',') if len(sys.argv) > 7 else None
fig, ax = plt.subplots(figsize=(10, 10 * (y1 - y0) / (x1 - x0)))
for fp in b.GetFootprints():
    for p in fp.Pads():
        bb = p.GetBoundingBox()
        col = '#aaaaaa' if not (pref and any(p.GetNetname().startswith(q) for q in pref)) else '#222222'
        ax.add_patch(plt.Rectangle((mm(bb.GetLeft()), mm(bb.GetTop())), mm(bb.GetWidth()), mm(bb.GetHeight()), fc=col, ec='none', alpha=.6))
for t in b.GetTracks():
    hl = pref and any(t.GetNetname().startswith(q) for q in pref)
    if t.GetClass() == 'PCB_VIA':
        p = t.GetPosition(); ax.plot(mm(p.x), mm(p.y), 'o', ms=3, color='k' if hl else '#999999')
    else:
        s, e = t.GetStart(), t.GetEnd()
        c = ('#d62728' if t.GetLayer() == pcbnew.F_Cu else '#1f77b4')
        ax.plot([mm(s.x), mm(e.x)], [mm(s.y), mm(e.y)], color=c, lw=max(0.6, mm(t.GetWidth()) * 4) if hl else 0.5, alpha=1 if hl else .35)
ax.set_xlim(x0, x1); ax.set_ylim(y1, y0); ax.set_aspect('equal'); ax.grid(alpha=.2)
fig.savefig(sys.argv[2], dpi=110, bbox_inches='tight')
