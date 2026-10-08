# Stage 14: board-wide check that every copper item (pads, tracks, vias, zone fills, F and B) is
# >= LIMIT mm from every hole edge (NPTH: any net; plated holes / vias: other nets).
# usage (KiCad python): check_hole_clearance.py [board.kicad_pcb] [limit_mm=0.25]
# Board-wide check: every copper item (pads, tracks, vias, zone fills, F/B) to every hole edge (NPTH any net; PTH/via other nets).
import pcbnew, math, sys
P=sys.argv[1] if len(sys.argv)>1 else r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\kicad\ai_calc.kicad_pcb"
LIM=float(sys.argv[2]) if len(sys.argv)>2 else 0.25
b=pcbnew.LoadBoard(P); tm=pcbnew.ToMM
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
def sd(p,a,c):
    ax,ay=a; cx,cy=c; px,py=p; dx,dy=cx-ax,cy-ay; L=dx*dx+dy*dy
    t=0 if L==0 else max(0,min(1,((px-ax)*dx+(py-ay)*dy)/L)); return math.hypot(px-ax-t*dx,py-ay-t*dy)
def inter(a,b_,c,d):
    def o(p,q,r): return (q[0]-p[0])*(r[1]-p[1])-(q[1]-p[1])*(r[0]-p[0])
    return o(a,b_,c)*o(a,b_,d)<0 and o(c,d,a)*o(c,d,b_)<0
def ss(a,b_,c,d):
    if inter(a,b_,c,d): return 0.0
    return min(sd(a,c,d),sd(b_,c,d),sd(c,a,b_),sd(d,a,b_))
holes=[]
for f in b.GetFootprints():
    for p in f.Pads():
        if not p.HasHole(): continue
        dx,dy=tm(p.GetDrillSize().x),tm(p.GetDrillSize().y); c=p.GetPosition(); cx,cy=tm(c.x),tm(c.y)
        ang=math.radians(p.GetOrientationDegrees()); r=min(dx,dy)/2; h=(max(dx,dy)-min(dx,dy))/2
        ux,uy=(math.cos(ang),-math.sin(ang)) if dx>=dy else (math.sin(ang),math.cos(ang))
        holes.append(dict(name=f"{f.GetReference()}.{p.GetNumber()}",a=(cx-ux*h,cy-uy*h),c=(cx+ux*h,cy+uy*h),r=r,net=None if p.GetAttribute()==pcbnew.PAD_ATTRIB_NPTH else p.GetNetCode(),obj=p))
for t in b.GetTracks():
    if t.GetClass()=="PCB_VIA":
        c=t.GetPosition(); holes.append(dict(name=f"via@{tm(c.x):.2f},{tm(c.y):.2f}",a=(tm(c.x),tm(c.y)),c=(tm(c.x),tm(c.y)),r=tm(t.GetDrillValue())/2,net=t.GetNetCode(),obj=t))
# copper geometry: list of (layer,net,obj_desc, segs[(a,c)], halfwidth, polygon-or-None)
cu=[]
for L in (pcbnew.F_Cu,pcbnew.B_Cu):
    ln=b.GetLayerName(L)
    for f in b.GetFootprints():
        for p in f.Pads():
            if not p.IsOnLayer(L) or p.GetAttribute()==pcbnew.PAD_ATTRIB_NPTH: continue
            sp=pcbnew.SHAPE_POLY_SET(); p.TransformShapeToPolygon(sp,L,0,pcbnew.FromMM(0.002),pcbnew.ERROR_INSIDE)
            for i in range(sp.OutlineCount()):
                o=sp.Outline(i); pts=[(tm(o.CPoint(k).x),tm(o.CPoint(k).y)) for k in range(o.PointCount())]
                cu.append((ln,p.GetNetCode(),f"{f.GetReference()}.{p.GetNumber()}",pts,0,p))
    for t in b.GetTracks():
        if not t.IsOnLayer(L): continue
        if t.GetClass()=="PCB_VIA":
            c=t.GetPosition(); cu.append((ln,t.GetNetCode(),"via",[(tm(c.x),tm(c.y))],tm(t.GetWidth(L))/2,t))
        elif t.GetClass()=="PCB_TRACK":
            cu.append((ln,t.GetNetCode(),"track "+t.GetNetname(),[(tm(t.GetStart().x),tm(t.GetStart().y)),(tm(t.GetEnd().x),tm(t.GetEnd().y))],tm(t.GetWidth())/2,t))
    for z in b.Zones():
        if z.GetIsRuleArea() or not z.IsOnLayer(L): continue
        sp=z.GetFilledPolysList(L)
        for i in range(sp.OutlineCount()):
            rings=[sp.Outline(i)]+[sp.Hole(i,j) for j in range(sp.HoleCount(i))]
            for o in rings:
                pts=[(tm(o.CPoint(k).x),tm(o.CPoint(k).y)) for k in range(o.PointCount())]
                cu.append((ln,z.GetNetCode(),"zone "+z.GetNetname(),pts,0,z))
def bbox(pts,hw): xs=[p[0] for p in pts]; ys=[p[1] for p in pts]; return min(xs)-hw,min(ys)-hw,max(xs)+hw,max(ys)+hw
cub=[(c,bbox(c[3],c[4])) for c in cu]
def pip(p,pts):
    x,y=p; ins=False; n=len(pts)
    for i in range(n):
        (x1,y1),(x2,y2)=pts[i],pts[(i+1)%n]
        if (y1>y)!=(y2>y) and x < (x2-x1)*(y-y1)/(y2-y1)+x1: ins=not ins
    return ins
bad=[]; worst={}
for h in holes:
    R=h["r"]+LIM+1.0; hx0=min(h["a"][0],h["c"][0])-R; hx1=max(h["a"][0],h["c"][0])+R; hy0=min(h["a"][1],h["c"][1])-R; hy1=max(h["a"][1],h["c"][1])+R
    for (ln,net,desc,pts,hw,obj),(x0,y0,x1,y1) in cub:
        if x1<hx0 or x0>hx1 or y1<hy0 or y0>hy1: continue
        if obj is h["obj"]: continue
        if h["net"] is not None and net==h["net"]: continue
        if len(pts)==1: d=sd(pts[0],h["a"],h["c"])
        elif len(pts)==2 and hw>0: d=ss(pts[0],pts[1],h["a"],h["c"])
        else:
            n=len(pts); d=min(ss(pts[k],pts[(k+1)%n],h["a"],h["c"]) for k in range(n))
            if not desc.startswith("zone") and pip(h["a"],pts): d=-d
        clr=d-hw-h["r"]
        key=(h["name"],desc,ln)
        if clr<worst.get(key,(9,))[0]: worst[key]=(clr,)
for (hn,desc,ln),(clr,) in sorted(worst.items(),key=lambda kv:kv[1][0]):
    if clr<LIM: bad.append((hn,desc,ln,round(clr,3)))
nh=sum(1 for h in holes if h['net'] is None)
print(f"holes: {nh} NPTH, {len(holes)-nh} plated/vias; copper items {len(cu)}; limit {LIM}")
print("min per NPTH hole:")
for h in holes:
    if h["net"] is None:
        m=min([v[0] for k,v in worst.items() if k[0]==h["name"]] or [99]); print(f"  {h['name']}: {m:.3f}")
print("violations:",len(bad))
for x in bad: print("  ",x)
