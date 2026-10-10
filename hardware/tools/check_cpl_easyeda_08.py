# Second-opinion CPL check (verification/08). Run with KiCad python:
#   uvx --from easyeda2kicad easyeda2kicad --lcsc_id <every BOM LCSC id> --footprint --symbol --output ee/ee.kicad_sym --overwrite
#   "C:/Program Files/KiCad/10.0/bin/python.exe" check_cpl_easyeda_08.py   (expects ee/ee.pretty next to this script)
import pcbnew, csv, math, sys, json, os
B = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\kicad\ai_calc.kicad_pcb"
CPL = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\fab\ai_calc_CPL_JLCPCB.csv"
BOM = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\fab\ai_calc_BOM_JLCPCB.csv"
LIB = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ee", "ee.pretty")

tm = pcbnew.ToMM; V = pcbnew.VECTOR2I; mm = pcbnew.FromMM
cpl = {r['Designator']: r for r in csv.DictReader(open(CPL))}
lcsc = {}
for r in csv.DictReader(open(BOM)):
    for d in r['Designator'].split(','):
        lcsc[d.strip()] = r['LCSC Part #']
eefp = {os.path.splitext(f)[0]: f for f in os.listdir(LIB)}
# map LCSC -> EasyEDA footprint name from the fetched library (names printed by easyeda2kicad)
LC2FP = {
 "C45783":"C0805","C1525":"C0402","C19702":"C0603","C52923":"C0402","C19666":"C0603","C12530":"C0402",
 "C29823":"C1206","C15849":"C0603","C8598":"SOD-123_L2.7-W1.6-LS3.7-RD-1","C193402":"SOD-123FL_L2.7-W1.8-LS3.8-RD",
 "C1002":"L0603","C6364666":"FPC-SMD_24P-P0.50_FPC-0.5-24P-HYH2.0","C46061768":"CONN-SMD_4P-P2.54_HX-PM2.54-1X4PWT",
 "C295747":"CONN-SMD_P2.00_S2B-PH-SM4-TB-LF-SN","C135265":"IND-SMD_L4.0-W4.0_SMNR4020","C15127":"SOT-23_L2.9-W1.3-P1.90-LS2.4-BR",
 "C469327":"SOT-323_L2.0-W1.3-P1.30-LS2.1-BR","C25744":"R0402","C25765":"R0402","C25741":"R0402","C26083":"R0402",
 "C23157":"R0603","C25900":"R0402","C3013941":"BULETM-SMD_ESP32-S3-MINI-1-N8","C424093":"SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BL",
 "C841192":"TSOT-23-5_L2.9-W1.6-P0.95-LS2.8-BL","C53099":"SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BR","C53100":"SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BR",
 "C138713":"WQFN-24_L4.0-W4.0-P0.50-TL-EP2.5","C7519":"SOT-23-6_L2.9-W1.6-P0.95-LS2.8-BL"}

b = pcbnew.LoadBoard(B)
fps = {f.GetReference(): f for f in b.GetFootprints()}

def padinfo(p):
    pos = p.GetPosition()
    return dict(num=p.GetNumber(), x=round(tm(pos.x),3), y=round(tm(pos.y),3), net=p.GetNetname(),
                layer=pcbnew.LayerName(p.GetLayer()) if hasattr(pcbnew,'LayerName') else str(p.GetLayer()),
                sx=round(tm(p.GetSize().x),3), sy=round(tm(p.GetSize().y),3), shape=p.GetShape(),
                attr=p.GetAttribute(), drill=round(tm(p.GetDrillSize().x),3))

out = {}
def refkey(r):
    import re
    m = re.match(r"([A-Za-z]+)(\d+)", r); return (m.group(1), int(m.group(2)))
rows = []
for ref in sorted(cpl, key=refkey):
    r = cpl[ref]; f = fps.get(ref)
    if f is None:
        rows.append(dict(ref=ref, result="FAIL", note="not on board")); continue
    l = lcsc.get(ref, "?"); fpn = LC2FP.get(l)
    x = float(r['Mid X'][:-2]); y = -float(r['Mid Y'][:-2]); rot = float(r['Rotation'])
    kpos = f.GetPosition(); krot = f.GetOrientationDegrees()
    cen = f.GetBoundingBox(False, False).GetCenter() if hasattr(f,'GetBoundingBox') else kpos
    dpos = math.hypot(tm(kpos.x)-x, tm(kpos.y)-y)
    kpads = [p for p in f.Pads() if p.GetNumber()]
    kp = {}
    for p in kpads: kp.setdefault(p.GetNumber(), []).append(p)
    e = pcbnew.FootprintLoad(LIB, fpn)
    e.SetPosition(V(mm(x), mm(y))); e.SetOrientationDegrees(rot)
    ep = [p for p in e.Pads() if p.GetNumber()]
    def dist(a, c): return math.hypot(tm(a.GetPosition().x-c.GetPosition().x), tm(a.GetPosition().y-c.GetPosition().y))
    per = {}
    for a in ep:
        if a.GetNumber() in kp:
            per[a.GetNumber()] = round(min(dist(a, c) for c in kp[a.GetNumber()]), 3)
        else:
            per[a.GetNumber()] = None
    # net landed by each EasyEDA pad (which KiCad pad is under it)
    landing = {}
    for a in ep:
        hit = [c for c in kpads if c.HitTest(a.GetPosition())]
        landing[a.GetNumber()] = sorted(set(c.GetNumber() for c in hit))
    worst = max([v for v in per.values() if v is not None] or [float('nan')])
    missing = [k for k,v in per.items() if v is None]
    allon = all(landing[k] for k in landing)
    numok = all(landing[k] == [k] for k in landing)
    # pad-1 on both
    rows.append(dict(ref=ref, lcsc=l, eefp=fpn, kfp=f.GetFPIDAsString(), kicad_rot=krot, cpl_rot=rot, cpl_minus_kicad=(rot-krot)%360,
                     pos_delta_mm=round(dpos,3), n_kicad_pads=len(kpads), n_ee_pads=len(ep), worst_same_num_mm=worst,
                     ee_pads_without_kicad_number=missing, every_ee_pad_on_a_kicad_pad=allon, every_ee_pad_on_same_number=numok,
                     landing={k:v for k,v in landing.items() if v != [k]},
                     ee_pad1_rel=[round(tm(p.GetPosition().x)-x,3), round(tm(p.GetPosition().y)-y,3)] if any(p.GetNumber()=="1" for p in ep) else None))
out['cpl'] = rows
# connectors + U1 pad/net dump
dump = {}
for ref in ["J1","J2","J3","J4","U1","U6","U2","U3","U4","U5","U7","Q1","Q2","Q3","D1","D2","D3","D4","D5","D6","D7","L1","FB1"]:
    f = fps[ref]
    dump[ref] = dict(pos=[round(tm(f.GetPosition().x),3), round(tm(f.GetPosition().y),3)], rot=f.GetOrientationDegrees(),
                     fp=f.GetFPIDAsString(), layer=f.GetLayerName(), pads=[padinfo(p) for p in f.Pads()])
out['parts'] = dump
# all non-CPL footprints (keys, TPs, holes, fiducials)
others = []
for ref, f in sorted(fps.items(), key=lambda kv: refkey(kv[0]) if kv[0] and kv[0][0].isalpha() and any(ch.isdigit() for ch in kv[0]) else (kv[0],0)):
    if ref in cpl: continue
    others.append(dict(ref=ref, fp=f.GetFPIDAsString(), layer=f.GetLayerName(), pos=[round(tm(f.GetPosition().x),3), round(tm(f.GetPosition().y),3)],
                       attr=f.GetAttributes(), pads=[padinfo(p) for p in f.Pads()][:4], npads=len(list(f.Pads()))))
out['others'] = others
json.dump(out, open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "board_dump.json"), "w"), indent=1)
# print CPL table
print("| Ref | LCSC | EasyEDA fp | KiCad rot | CPL rot | delta | pos d | ee pads | worst same-# (mm) | all on pad | same # | landing mismatches |")
for r in rows:
    if 'lcsc' not in r: print(r); continue
    print(f"| {r['ref']} | {r['lcsc']} | {r['eefp']} | {r['kicad_rot']:g} | {r['cpl_rot']:g} | {r['cpl_minus_kicad']:g} | {r['pos_delta_mm']} | {r['n_ee_pads']}/{r['n_kicad_pads']} | {r['worst_same_num_mm']} | {r['every_ee_pad_on_a_kicad_pad']} | {r['every_ee_pad_on_same_number']} | {r['landing']} |")
