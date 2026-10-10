import pcbnew, sys, json
b = pcbnew.LoadBoard(sys.argv[1])
bb = b.GetBoardEdgesBoundingBox()
out = {"edge":[pcbnew.ToMM(bb.GetLeft()),pcbnew.ToMM(bb.GetTop()),pcbnew.ToMM(bb.GetRight()),pcbnew.ToMM(bb.GetBottom())],"fp":{}}
for f in b.GetFootprints():
    r = f.GetReference()
    p = f.GetPosition()
    fb = f.GetBoundingBox(False)
    pads = {}
    for pd in f.Pads():
        q = pd.GetPosition()
        pads[pd.GetNumber()] = [round(pcbnew.ToMM(q.x),2), round(pcbnew.ToMM(q.y),2), pd.GetNetname()]
    out["fp"][r] = {"x":round(pcbnew.ToMM(p.x),2),"y":round(pcbnew.ToMM(p.y),2),"rot":f.GetOrientationDegrees(),"side":"B" if f.IsFlipped() else "F","val":f.GetValue(),
      "bb":[round(pcbnew.ToMM(v),2) for v in (fb.GetLeft(),fb.GetTop(),fb.GetRight(),fb.GetBottom())],"pads":pads}
json.dump(out, open(sys.argv[2],"w"), indent=0)
print(out["edge"], len(out["fp"]))
