#!/usr/bin/env python3
"""List any via that touches a bare key pad area."""
import sys, pcbnew
b = pcbnew.LoadBoard(sys.argv[1]); mm = pcbnew.ToMM
areas = []
for fp in b.GetFootprints():
    if fp.GetReference().startswith('SW'):
        xs, ys = [], []
        for p in fp.Pads():
            bb = p.GetBoundingBox(); xs += [mm(bb.GetLeft()), mm(bb.GetRight())]; ys += [mm(bb.GetTop()), mm(bb.GetBottom())]
        areas.append((fp.GetValue(), min(xs), min(ys), max(xs), max(ys)))
bad = []
for t in b.GetTracks():
    if t.GetClass() == 'PCB_VIA':
        p = t.GetPosition(); x, y = mm(p.x), mm(p.y); r = mm(t.GetWidth()) / 2
        for name, x0, y0, x1, y1 in areas:
            if x0 - r < x < x1 + r and y0 - r < y < y1 + r:
                bad.append((name, round(x, 2), round(y, 2), t.GetNetname()))
print('vias in key areas:', len(bad))
for b_ in bad: print(' ', b_)
