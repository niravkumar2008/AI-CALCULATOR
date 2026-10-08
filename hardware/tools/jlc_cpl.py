#!/usr/bin/env python3
"""Turn KiCad's raw position file into JLCPCB's CPL, with rotation corrections.

JLCPCB places every part using the EasyEDA footprint of its LCSC number. Some of
those footprints are drawn at a different zero angle from KiCad's, so the KiCad
rotation has to be corrected or the part goes on with pin 1 in the wrong corner.

    JLCPCB rotation = (KiCad rotation + offset) mod 360

The offsets were derived (verification/04_manufacturing.md, re-checked in stage 14
with easyeda2kicad, see stage14_verification_fixes.md) by fetching each EasyEDA
footprint, rotating it by the CPL angle and checking that EasyEDA pin N lands on
KiCad pad N. Lookup order: designator override, then LCSC number, then footprint.
A part that is not in any table keeps offset 0, which is right for symmetric
two-pad passives and for every other part verified in stage 14.

usage: jlc_cpl.py <pos_raw.csv from kicad-cli> <BOM csv (Designator, LCSC Part #)> <out CPL csv>
"""
import csv
import re
import sys

# Offset by LCSC part number (degrees, added to the KiCad rotation).
ROT_BY_LCSC = {
    "C15127": 180,     # AO3401A, SOT-23: EasyEDA "-BR" footprint (Q1, Q2)
    "C424093": 270,    # MCP73831T-2ACI/OT, SOT-23-5 "-BL" (U2)
    "C841192": 270,    # RT9080-33GJ5, TSOT-23-5 "-BL" (U3)
    "C53099": 180,     # ME6211C28M5G-N, SOT-23-5 "-BR" (U4)
    "C53100": 180,     # ME6211C15M5G-N, SOT-23-5 "-BR" (U5)
    "C7519": 270,      # USBLC6-2SC6, SOT-23-6 "-BL" (U7)
    "C46061768": 180,  # HX PM2.54-1x4P WT right-angle header (J3): EasyEDA body is on +y of the pads
    # Verified offset 0 (listed so the table documents every polarised part):
    "C469327": 0,      # SI1308EDL SOT-323 (Q3)
    "C138713": 0,      # TCA8418 WQFN-24 (U6)
    "C3013941": 0,     # ESP32-S3-MINI-1-N4R2 (U1)
    "C6364666": 0,     # FPC 0.5-24P (J1 CamReversed: pin numbers intentionally mirrored, geometry matches; J2)
    "C295747": 0,      # JST S2B-PH-SM4-TB (J4)
    "C8598": 0,        # B5819W SOD-123 (D1-D6)
    "C193402": 0,      # SMF5.0A SOD-123FL (D7)
}

# Fallback by KiCad footprint name, used only when an LCSC number is missing.
ROT_BY_FOOTPRINT = {}

# Per-designator overrides (none needed today).
ROT_BY_REF = {}

SKIP_PREFIXES = ("SW", "TP", "H", "FID")


def expand(designators):
    """'C1,C32-C34' -> ['C1', 'C32', 'C33', 'C34'] (kicad-cli BOM range syntax)."""
    out = []
    for tok in designators.replace(" ", "").split(","):
        if not tok:
            continue
        m = re.fullmatch(r"([A-Za-z]+)(\d+)-([A-Za-z]+)?(\d+)", tok)
        if m:
            out += [f"{m.group(1)}{i}" for i in range(int(m.group(2)), int(m.group(4)) + 1)]
        else:
            out.append(tok)
    return out


def main(pos_path, bom_path, out_path):
    lcsc_of = {}
    with open(bom_path, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            for ref in expand(row["Designator"]):
                lcsc_of[ref] = row.get("LCSC Part #", "").strip()

    rows = list(csv.DictReader(open(pos_path, newline="", encoding="utf-8")))
    n = 0
    changed = []
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Designator", "Mid X", "Mid Y", "Layer", "Rotation"])
        for r in rows:
            ref = r["Ref"]
            if ref.startswith(SKIP_PREFIXES):
                continue
            lcsc = lcsc_of.get(ref, "")
            if not lcsc:
                sys.exit(f"jlc_cpl: {ref} has no LCSC number in the BOM; refusing to write a CPL")
            if ref in ROT_BY_REF:
                off = ROT_BY_REF[ref]
            elif lcsc in ROT_BY_LCSC:
                off = ROT_BY_LCSC[lcsc]
            else:
                off = ROT_BY_FOOTPRINT.get(r.get("Package", ""), 0)
            rot = (float(r["Rot"]) + off) % 360.0
            if off:
                changed.append(f"{ref} {float(r['Rot']):g}->{rot:g}")
            side = "Top" if r["Side"] == "top" else "Bottom"
            w.writerow([ref, r["PosX"] + "mm", r["PosY"] + "mm", side, f"{rot:.6f}"])
            n += 1
    missing = sorted(set(lcsc_of) - {r["Ref"] for r in rows})
    if missing:
        sys.exit(f"jlc_cpl: in BOM but not in the position file: {missing}")
    print(f"CPL rows {n}; rotation-corrected: {', '.join(changed)}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:])
