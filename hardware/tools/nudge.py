#!/usr/bin/env python3
"""Move a routed footprint a little and drag the track ends that sit on its pads.
usage: nudge.py <pcb> REF dx_mm dy_mm"""
import sys, pcbnew
pcb, ref, dx, dy = sys.argv[1], sys.argv[2], float(sys.argv[3]), float(sys.argv[4])
b = pcbnew.LoadBoard(pcb)
fp = b.FindFootprintByReference(ref)
d = pcbnew.VECTOR2I_MM(dx, dy)
pads = [(p.GetPosition(), p) for p in fp.Pads()]
moved = 0
for t in b.GetTracks():
    if t.GetClass() == 'PCB_VIA':
        continue
    for pos, p in pads:
        if t.GetNetCode() != p.GetNetCode():
            continue
        if p.HitTest(t.GetStart()):
            t.SetStart(t.GetStart() + d); moved += 1
        if p.HitTest(t.GetEnd()):
            t.SetEnd(t.GetEnd() + d); moved += 1
fp.Move(d)
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
b.Save(pcb)
print('moved', ref, 'track ends', moved)
