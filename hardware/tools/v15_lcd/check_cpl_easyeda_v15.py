# Stage 15 (v15-LCD) copy of check_cpl_easyeda.py: v15 board; new parts J5 (C2919501), Q5 (C20917), R23 (C23345).
# Stage 14: place each JLCPCB/EasyEDA footprint (fetched with easyeda2kicad --footprint into LIB) at the
# CPL position + rotation and check EasyEDA pin 1 lands on KiCad pad 1 and every EasyEDA pad lies on a KiCad pad.
# usage (KiCad python): check_cpl_easyeda.py <fab/ai_calc_CPL_JLCPCB.csv> <easyeda .pretty dir>
import pcbnew, csv, math, sys
B=r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\kicad_v15_lcd\ai_calc_v15_lcd.kicad_pcb"
LIB=sys.argv[2]; cpl={r['Designator']:r for r in csv.DictReader(open(sys.argv[1]))}
FP={"C15127":"SOT-23_L2.9-W1.3-P1.90-LS2.4-BR","C424093":"SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BL","C841192":"TSOT-23-5_L2.9-W1.6-P0.95-LS2.8-BL",
"C53099":"SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BR","C53100":"SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BR","C7519":"SOT-23-6_L2.9-W1.6-P0.95-LS2.8-BL",
"C46061768":"CONN-SMD_4P-P2.54_HX-PM2.54-1X4PWT","C469327":"SOT-323_L2.0-W1.3-P1.30-LS2.1-BR","C138713":"WQFN-24_L4.0-W4.0-P0.50-TL-EP2.5",
"C3013941":"BULETM-SMD_ESP32-S3-MINI-1-N8","C6364666":"FPC-SMD_24P-P0.50_FPC-0.5-24P-HYH2.0","C295747":"CONN-SMD_P2.00_S2B-PH-SM4-TB-LF-SN",
"C8598":"SOD-123_L2.7-W1.6-LS3.7-RD-1","C193402":"SOD-123FL_L2.7-W1.8-LS3.8-RD","C2919501":"FPC-SMD_30P-P0.50_HDGC_0.5K-HX-30PWB","C20917":"SOT-23-3_L2.9-W1.3-P1.90-LS2.4-BR","C23345":"R0603","C22810":"R0603"}
b=pcbnew.LoadBoard(B); tm=pcbnew.ToMM; V=pcbnew.VECTOR2I; mm=pcbnew.FromMM
def lc(f):
    try: return f.GetFieldText("LCSC")
    except Exception: return ""
print("| Ref | LCSC | EasyEDA footprint | KiCad rot | CPL rot | EasyEDA pin 1 -> KiCad pad 1 (mm) | worst same-number pad offset (mm) | every EasyEDA pad on a KiCad pad | Result |")
print("|---|---|---|---|---|---|---|---|---|")
for f in sorted(b.GetFootprints(),key=lambda f:(f.GetReference()[0],int(''.join(c for c in f.GetReference() if c.isdigit()) or 0))):
    ref=f.GetReference(); l=lc(f)
    if l not in FP or ref not in cpl: continue
    e=pcbnew.FootprintLoad(LIB,FP[l]); r=cpl[ref]
    x=float(r['Mid X'][:-2]); y=-float(r['Mid Y'][:-2]); rot=float(r['Rotation'])
    e.SetPosition(V(mm(x),mm(y))); e.SetOrientationDegrees(rot)
    kp={}; kpads=[p for p in f.Pads() if p.GetNumber()]
    for p in kpads: kp.setdefault(p.GetNumber(),[]).append(p)
    ep=[p for p in e.Pads() if p.GetNumber()]
    def dist(a,c): return math.hypot(tm(a.GetPosition().x-c.GetPosition().x),tm(a.GetPosition().y-c.GetPosition().y))
    e1=[p for p in ep if p.GetNumber()=="1"]
    d1=min(dist(a,c) for a in e1 for c in kp.get("1",[])) if e1 and "1" in kp else float('nan')
    worst=0; 
    for a in ep:
        if a.GetNumber() in kp: worst=max(worst,min(dist(a,c) for c in kp[a.GetNumber()]))
    # geometry: every EasyEDA pad centre inside some KiCad pad (any number)
    allon=all(any(c.HitTest(a.GetPosition()) for c in kpads) for a in ep)
    ok = d1<0.3 and allon and worst<0.3
    note=""
    if ref=="J1": ok_geo=allon; note=" (pin numbers mirrored on purpose: geometry only)"; ok=allon
    if ref=="J5": ok_geo=allon; note=" (pin numbers mirrored on purpose, _LcdReversed, review 13: geometry only)"; ok=allon
    if ref=="J3" and not ok and allon: note=" (unpolarised 4 identical contacts: geometry only)"; ok=allon
    print(f"| {ref} | {l} | {FP[l]} | {f.GetOrientationDegrees():g} | {rot:g} | {d1:.2f} | {worst:.2f} | {'yes' if allon else 'NO'} | {'PASS' if ok else 'FAIL'}{note} |")
