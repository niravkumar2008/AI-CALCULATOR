import pcbnew, csv, math, os, json
B=r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\kicad\ai_calc.kicad_pcb"
CPL=r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\fab\ai_calc_CPL_JLCPCB.csv"
LIB=os.path.join(os.path.dirname(os.path.abspath(__file__)),"ee","ee.pretty")
tm=pcbnew.ToMM; V=pcbnew.VECTOR2I; mm=pcbnew.FromMM
cpl={r['Designator']:r for r in csv.DictReader(open(CPL))}
b=pcbnew.LoadBoard(B); fps={f.GetReference():f for f in b.GetFootprints()}
EE={"J1":"FPC-SMD_24P-P0.50_FPC-0.5-24P-HYH2.0","J2":"FPC-SMD_24P-P0.50_FPC-0.5-24P-HYH2.0","J3":"CONN-SMD_4P-P2.54_HX-PM2.54-1X4PWT","J4":"CONN-SMD_P2.00_S2B-PH-SM4-TB-LF-SN","U1":"BULETM-SMD_ESP32-S3-MINI-1-N8","U6":"WQFN-24_L4.0-W4.0-P0.50-TL-EP2.5","D7":"SOD-123FL_L2.7-W1.8-LS3.8-RD","D1":"SOD-123_L2.7-W1.6-LS3.7-RD-1","U2":"SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BL","Q1":"SOT-23_L2.9-W1.3-P1.90-LS2.4-BR"}
def gfx(f):
    out=[]
    for d in f.GraphicalItems():
        ln=d.GetLayerName()
        if ln in ("F.Fab","F.SilkS","F.CrtYd"):
            bb=d.GetBoundingBox()
            out.append((ln, d.GetClass(), round(tm(bb.GetLeft()),2),round(tm(bb.GetTop()),2),round(tm(bb.GetRight()),2),round(tm(bb.GetBottom()),2)))
    return out
def summary(f,label):
    pads=[p for p in f.Pads() if p.GetNumber()]
    xs=[tm(p.GetPosition().x) for p in pads]; ys=[tm(p.GetPosition().y) for p in pads]
    print(f"  {label}: pads bbox x[{min(xs):.2f},{max(xs):.2f}] y[{min(ys):.2f},{max(ys):.2f}]")
    for p in pads[:3]+pads[-2:]:
        print(f"    pad {p.GetNumber()} at ({tm(p.GetPosition().x):.2f},{tm(p.GetPosition().y):.2f}) size {tm(p.GetSize().x):.2f}x{tm(p.GetSize().y):.2f}")
    for ln in ("F.Fab","F.SilkS","F.CrtYd"):
        items=[g for g in gfx(f) if g[0]==ln]
        if items:
            l=min(i[2] for i in items); t=min(i[3] for i in items); r=max(i[4] for i in items); bt=max(i[5] for i in items)
            print(f"    {ln} bbox x[{l:.2f},{r:.2f}] y[{t:.2f},{bt:.2f}] ({len(items)} items)")
for ref,een in EE.items():
    f=fps[ref]; r=cpl[ref]
    x=float(r['Mid X'][:-2]); y=-float(r['Mid Y'][:-2]); rot=float(r['Rotation'])
    print(f"== {ref} KiCad fp {f.GetFPIDAsString()} at ({tm(f.GetPosition().x):.2f},{tm(f.GetPosition().y):.2f}) rot {f.GetOrientationDegrees():g}; CPL rot {rot:g}")
    summary(f,"KiCad")
    e=pcbnew.FootprintLoad(LIB,een); e.SetPosition(V(mm(x),mm(y))); e.SetOrientationDegrees(rot)
    summary(e,"EasyEDA@CPL")
    # also EasyEDA pin1 text/marker: list F.SilkS circles
    for d in e.GraphicalItems():
        if d.GetClass()=="PCB_SHAPE" and d.GetShapeStr()=="Circle" and d.GetLayerName()=="F.SilkS":
            print(f"    EE silk circle at ({tm(d.GetCenter().x):.2f},{tm(d.GetCenter().y):.2f})")
    for d in f.GraphicalItems():
        if d.GetClass()=="PCB_SHAPE" and d.GetShapeStr()=="Circle" and d.GetLayerName()=="F.SilkS":
            print(f"    KiCad silk circle at ({tm(d.GetCenter().x):.2f},{tm(d.GetCenter().y):.2f})")
