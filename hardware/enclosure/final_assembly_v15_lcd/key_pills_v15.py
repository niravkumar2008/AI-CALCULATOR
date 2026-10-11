"""Key pads vs key-mat carbon pills (v15-LCD final board, verification 14/15).

Run with KiCad's python:  "C:/Program Files/KiCad/10.0/bin/python.exe" key_pills_v15.py   (numbers)
then with a python that has matplotlib:  python key_pills_v15.py   (picture only, from key_pills.json)
Reads (read only): hardware/kicad_v15_lcd/ai_calc_v15_lcd.kicad_pcb (pad copper), board_snapshot.json (where the replica
keymat/keycaps put each key = the v14 pad centres), hardware/verification/15_keypad_vs_casio.md (Casio contact positions,
mean of the three photo estimates, and the pill diameter per key).
Writes key_pills.json, key_pills.md and renders/key_pills_top_row.png.

Per key and per pill position: share of the pill disc on the pad's copper bounding box (review-15 metric), share on the real
finger copper, number of finger-to-finger gaps the pill bridges, and the worst share if the pill is off by 0.7 mm in any
direction (the top-row photo uncertainty is +-0.6-0.8 mm)."""
import os, re, json, math, sys
try:
    import pcbnew
except ImportError:
    pcbnew = None

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
PCB = os.path.join(REPO, "hardware", "kicad_v15_lcd", "ai_calc_v15_lcd.kicad_pcb")
REV15 = os.path.join(REPO, "hardware", "verification", "15_keypad_vs_casio.md")
CY = 138.94
OLD = {"SW1": (176.623, 120.795), "SW2": (166.026, 120.795), "SW50": (123.377, 120.795)}

def compute():
    b = pcbnew.LoadBoard(PCB)
    pads = {}
    for fp in b.GetFootprints():
        ref = fp.GetReference()
        if not re.fullmatch(r"SW\d+", ref):
            continue
        rects = []
        for p in fp.Pads():
            bb = p.GetBoundingBox()
            rects.append((pcbnew.ToMM(bb.GetLeft()), pcbnew.ToMM(bb.GetRight()), pcbnew.ToMM(bb.GetTop()), pcbnew.ToMM(bb.GetBottom()), p.GetNetname()))
        pos = fp.GetPosition()
        pads[ref] = dict(c=(pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y)), rects=rects, fp=str(fp.GetFPID().GetLibItemName()))

    # review 15 table: ref, key, our old centre, mean dx dy, pill diameter
    casio = {}
    for line in open(REV15, encoding="utf8"):
        c = [x.strip() for x in line.split("|")]
        if len(c) > 14 and re.fullmatch(r"SW\d+", c[1]):
            ox, oy = [float(v) for v in c[5].split(",")]
            dx, dy = [float(v) for v in c[10].replace("*", "").split(",")]
            casio[c[1]] = dict(key=c[2], old=(ox, oy), pos=(ox + dx, oy + dy), d=float(c[13]))

    snap = json.load(open(os.path.join(HERE, "board_snapshot.json")))
    model = [(round(150 - k[1][0], 3), round(CY - k[1][1], 3)) for k in snap["keys"]]


    def cover(ref, c, d, step=0.02):
        r = d / 2
        rects = pads[ref]["rects"]
        x0 = min(q[0] for q in rects); x1 = max(q[1] for q in rects); y0 = min(q[2] for q in rects); y1 = max(q[3] for q in rects)
        n = inb = onc = 0
        k = int(r / step)
        for i in range(-k, k + 1):
            for j in range(-k, k + 1):
                px, py = c[0] + i * step, c[1] + j * step
                if (px - c[0]) ** 2 + (py - c[1]) ** 2 > r * r:
                    continue
                n += 1
                if x0 <= px <= x1 and y0 <= py <= y1:
                    inb += 1
                if any(q[0] <= px <= q[1] and q[2] <= py <= q[3] for q in rects):
                    onc += 1
        # gaps bridged: the narrow fingers the disc overlaps, sorted by x; neighbours on different nets = one bridged gap
        hit = sorted([q for q in rects if (q[1] - q[0]) < 1.0 and
                      math.hypot(max(q[0] - c[0], 0, c[0] - q[1]), max(q[2] - c[1], 0, c[1] - q[3])) < r], key=lambda q: q[0])
        gaps = sum(1 for a, bq in zip(hit, hit[1:]) if a[4] != bq[4])
        return round(100 * inb / n, 1), round(100 * onc / n, 1), gaps


    def worst(ref, c, d, off=0.7):
        return min(cover(ref, (c[0] + off * math.cos(a), c[1] + off * math.sin(a)), d, 0.05)[0] for a in [k * math.pi / 8 for k in range(16)])


    rows = []
    for ref in sorted(pads, key=lambda r: int(r[2:])):
        if ref not in casio:
            continue
        C = casio[ref]
        pc = pads[ref]["c"]
        mk = min(model, key=lambda m: math.dist(m, C["old"]))
        row = dict(ref=ref, key=C["key"], pad=[round(pc[0], 3), round(pc[1], 3)], moved=ref in OLD, footprint=pads[ref]["fp"], pill_d=C["d"])
        for tag, cpos in (("casio", C["pos"]), ("model", mk)):
            bbx, cu, gaps = cover(ref, cpos, C["d"])
            row[tag] = dict(pill=[round(cpos[0], 3), round(cpos[1], 3)], offset=round(math.dist(cpos, pc), 2),
                            on_bbox=bbx, on_copper=cu, gaps=gaps, worst_0p7=worst(ref, cpos, C["d"]))
        rows.append(row)

    json.dump(dict(pcb=PCB, pcb_mtime=os.path.getmtime(PCB), rows=rows, pad_rects={k: pads[k]["rects"] for k in OLD}), open(os.path.join(HERE, "key_pills.json"), "w"), indent=1)

    md = ["# Key pads vs key-mat pills (v15-LCD final board)", "",
          "Made by `key_pills_v15.py` from the final `kicad_v15_lcd/ai_calc_v15_lcd.kicad_pcb`. Pill = the carbon dot on the Casio key mat. "
          "**Casio** = pill centre measured in verification 15 (mean of three photo fits, +-0.4 mm rows 3-9, +-0.6-0.8 mm top row and ring). "
          "**Model** = where the CAD keymat dome / keycap sits (the replica places it on the v14 pad centre, board_snapshot.json). "
          "On bbox = share of the pill on the pad's copper outline (review-15 metric); on copper = share on the finger metal itself; "
          "gaps = finger-to-finger gaps the pill bridges (1 is enough to close the key); worst 0.7 = on-bbox share if the pill is 0.7 mm off "
          "in the worst direction.", "",
          "| Ref | Key | Pad centre (KiCad) | Pill Ø | Casio pill offset | on bbox | on copper | gaps | worst 0.7 | Model dome offset | on bbox | gaps |",
          "|---|---|---|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        c, m = r["casio"], r["model"]
        md.append("| %s%s | %s | %.3f, %.3f | %.1f | %.2f | %.0f %% | %.0f %% | %d | %.0f %% | %.2f | %.0f %% | %d |" % (
            r["ref"], " (moved)" if r["moved"] else "", r["key"], r["pad"][0], r["pad"][1], r["pill_d"], c["offset"], c["on_bbox"], c["on_copper"],
            c["gaps"], c["worst_0p7"], m["offset"], m["on_bbox"], m["gaps"]))
    open(os.path.join(HERE, "key_pills.md"), "w", encoding="utf8").write("\n".join(md) + "\n")



def picture(rows, rects):
    """Pillow only: three panels (SHIFT, ALPHA, ON), front view (KiCad x mirrored), 60 px/mm."""
    from PIL import Image, ImageDraw, ImageFont
    S, PW, PH, TOP = 60, 620, 560, 150
    font = lambda n: ImageFont.truetype(r"C:\Windows\Fonts\segoeui.ttf", n)
    img = Image.new("RGB", (3 * PW + 40, PH + TOP + 150), "white")
    d = ImageDraw.Draw(img)
    d.text((20, 15), "v15-LCD final board: moved top-row pads vs the key-mat carbon pills", fill="black", font=font(34))
    d.text((20, 62), "front view (key side, display up). Gold = pad copper, grey dashes = pad before the move, red = Casio pill (verification 15),"
           " red dots = pill 0.7 mm off, blue dashes = CAD keymat dome (replica, at the old pad)", fill=(60, 60, 60), font=font(17))
    for i, ref in enumerate(["SW1", "SW2", "SW50"]):
        r = next(x for x in rows if x["ref"] == ref)
        ox, oy = 20 + i * (PW + 0), TOP
        cx, cy = r["pad"]
        T = lambda x, y: (ox + PW / 2 - (x - cx) * S, oy + PH / 2 + (y - cy) * S)
        d.rectangle([ox, oy, ox + PW - 10, oy + PH], outline=(200, 200, 200))
        for q in rects[ref]:
            x0, y0 = T(q[1], q[2]); x1, y1 = T(q[0], q[3])
            d.rectangle([x0, y0, x1, y1], fill=(201, 162, 39))
        o = OLD[ref]
        x0, y0 = T(o[0] + 3, o[1] - 2.25); x1, y1 = T(o[0] - 3, o[1] + 2.25)
        for k in range(0, int(x1 - x0), 14):
            d.line([x0 + k, y0, min(x0 + k + 7, x1), y0], fill=(120, 120, 120), width=2); d.line([x0 + k, y1, min(x0 + k + 7, x1), y1], fill=(120, 120, 120), width=2)
        for k in range(0, int(y1 - y0), 14):
            d.line([x0, y0 + k, x0, min(y0 + k + 7, y1)], fill=(120, 120, 120), width=2); d.line([x1, y0 + k, x1, min(y0 + k + 7, y1)], fill=(120, 120, 120), width=2)
        def circ(c, rad, col, w, dash=0):
            X, Y = T(*c); R_ = rad * S
            if not dash:
                d.ellipse([X - R_, Y - R_, X + R_, Y + R_], outline=col, width=w); return
            import math as m
            n = int(2 * m.pi * R_ / dash)
            for k in range(0, n, 2):
                a0, a1 = 360 * k / n, 360 * (k + 1) / n
                d.arc([X - R_, Y - R_, X + R_, Y + R_], a0, a1, fill=col, width=w)
        circ(r["casio"]["pill"], r["pill_d"] / 2, (220, 20, 40), 5)
        circ(r["casio"]["pill"], r["pill_d"] / 2 + 0.7, (220, 20, 40), 2, 6)
        circ(r["model"]["pill"], r["pill_d"] / 2, (20, 60, 220), 3, 12)
        X, Y = T(cx, cy)
        d.line([X - 12, Y, X + 12, Y], fill="black", width=2); d.line([X, Y - 12, X, Y + 12], fill="black", width=2)
        c = r["casio"]
        d.text((ox + 10, oy + PH + 10), "%s %s  pad (%.2f, %.2f)" % (ref, r["key"], cx, cy), fill="black", font=font(24))
        d.text((ox + 10, oy + PH + 44), "Casio pill: %.0f %% on the pad, %d gaps bridged" % (c["on_bbox"], c["gaps"]), fill=(180, 10, 30), font=font(20))
        d.text((ox + 10, oy + PH + 70), "%.2f mm off centre; if 0.7 mm worse: %.0f %%" % (c["offset"], c["worst_0p7"]), fill=(180, 10, 30), font=font(20))
        d.text((ox + 10, oy + PH + 96), "CAD dome: %.2f mm off, %.0f %% on the pad" % (r["model"]["offset"], r["model"]["on_bbox"]), fill=(20, 60, 200), font=font(20))
    os.makedirs(os.path.join(HERE, "renders"), exist_ok=True)
    img.save(os.path.join(HERE, "renders", "key_pills_top_row.png"))


if __name__ == "__main__":
    if pcbnew:
        rows, pads = compute()
        rects = {k: pads[k]["rects"] for k in OLD}
    else:
        J = json.load(open(os.path.join(HERE, "key_pills.json")))
        rows, rects = J["rows"], J["pad_rects"]
    picture(rows, rects)
    for r in rows:
        if r["moved"] or r["casio"]["on_bbox"] < 99:
            print(r["ref"], r["key"], r["pad"], "casio", r["casio"], "model", r["model"])
