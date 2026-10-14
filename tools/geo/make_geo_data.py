#!/usr/bin/env python3
"""Generates core/geo_data.cpp (the Geography app's world map) from Natural Earth.

Inputs (public domain, download them yourself; never commit them):
  ne_110m_admin_0_countries.geojson         country outlines, names, ISO codes, continents
  ne_110m_populated_places.geojson          capitals (FEATURECLA "Admin-0 capital")
  ne_10m_populated_places_simple.geojson    fallback for capitals the 110m file lacks

Usage:
  python tools/geo/make_geo_data.py <dir with the three files> [core/geo_data.cpp]

What it does: keeps all 177 features of the countries file, simplifies every ring
with Douglas-Peucker (EPS degrees), quantizes to int16 degrees x100, picks one
capital per country (see CAPITAL_CHOICE below), and writes const tables that the
C++ side reads through core/geo.h. The output is deterministic for the same input.
See tools/geo/README.md.
"""
import json
import math
import os
import sys

EPS = 0.12          # Douglas-Peucker tolerance in degrees (110m data is ~0.1 deg accurate anyway)
MIN_RING_AREA = 0.05  # deg^2: drop islands smaller than this (a country always keeps its largest ring)
NEAR_GAP = 3.0      # deg: rings this close to the main part join the country's zoom box

CONTINENTS = ["Africa", "Asia", "Europe", "North America", "South America", "Oceania",
              "Antarctica", "Seven seas (open ocean)"]

# Countries where Natural Earth lists more than one capital, or where its 110m
# "Admin-0 capital" flag is not the de-jure capital. Value: the capital we keep.
CAPITAL_CHOICE = {
    "TZA": "Dodoma",          # de-jure capital (NE 110m flags Dar es Salaam, the old seat)
    "BEN": "Porto-Novo",      # constitutional capital (Cotonou is the seat of government)
    "CIV": "Yamoussoukro",    # official capital (Abidjan is the economic capital)
    "ZAF": "Pretoria",        # executive capital of three (Cape Town legislative, Bloemfontein judicial)
    "BOL": "Sucre",           # constitutional capital (La Paz is the seat of government)
    "LKA": "Sri Jayawardenepura Kotte",  # legislative/official capital (Colombo commercial)
    "NLD": "Amsterdam",       # constitutional capital (The Hague is the seat of government)
    "MYS": "Kuala Lumpur",    # official capital (Putrajaya is the administrative centre)
    "SWZ": "Mbabane",         # administrative capital (Lobamba royal/legislative)
    "CHL": "Santiago",
    "NGA": "Abuja",
    "ISR": "Jerusalem",       # Natural Earth's default; not editorialized
    "MMR": "Naypyidaw",
    "PHL": "Manila",
    "JPN": "Tokyo",
    "MAR": "Rabat",
}
# Capitals the 110m places file lacks, taken from the 10m "simple" file
# (matched by name among its "... capital" places).
FALLBACK_10M = {
    "FLK": "Stanley", "GRL": "Nuuk", "PRI": "San Juan", "PSX": "Ramallah",
    "NCL": "Nouméa", "SDS": "Juba",
}
# 110m "Admin-0 capital alt" entries that are the only capital NE gives the feature.
ALT_OK = {"SAH": "Bir Lehlou"}
# Capital names changed after picking: English where NE's NAME is the local form,
# and a rename NE's data predates. Positions stay Natural Earth's.
RENAME = {
    "DNK": "Copenhagen",  # NE NAME is "København"; its NAME_EN is "Copenhagen"
    "KAZ": "Astana",      # renamed back from Nur-Sultan in September 2022; NE still says Nur-Sultan
}
# No capital in Natural Earth for these features: Antarctica, the French Southern and
# Antarctic Lands (administered from Saint-Pierre, Reunion), and N. Cyprus.
NO_CAPITAL = {"ATA", "ATF", "CYN"}


def ring_area(r):
    a = 0.0
    for i in range(len(r)):
        x1, y1 = r[i]
        x2, y2 = r[(i + 1) % len(r)]
        a += x1 * y2 - x2 * y1
    return abs(a) / 2


def dp(pts, eps):
    """Douglas-Peucker on an open polyline; keeps both ends."""
    if len(pts) < 3:
        return pts[:]
    keep = [False] * len(pts)
    keep[0] = keep[-1] = True
    stack = [(0, len(pts) - 1)]
    while stack:
        a, b = stack.pop()
        ax, ay = pts[a]
        bx, by = pts[b]
        dx, dy = bx - ax, by - ay
        L = math.hypot(dx, dy)
        best, bi = -1.0, -1
        for i in range(a + 1, b):
            px, py = pts[i]
            d = abs(dy * (px - ax) - dx * (py - ay)) / L if L > 0 else math.hypot(px - ax, py - ay)
            if d > best:
                best, bi = d, i
        if best > eps:
            keep[bi] = True
            stack.append((a, bi))
            stack.append((bi, b))
    return [p for p, k in zip(pts, keep) if k]


def simplify_ring(ring, eps):
    r = ring[:-1] if ring[0] == ring[-1] else ring[:]
    if len(r) <= 4:
        return r
    # split the closed ring at the point farthest from the first one
    x0, y0 = r[0]
    far = max(range(len(r)), key=lambda i: (r[i][0] - x0) ** 2 + (r[i][1] - y0) ** 2)
    a = dp(r[:far + 1], eps)
    b = dp(r[far:] + [r[0]], eps)
    out = a[:-1] + b[:-1]
    if len(out) < 3:  # degenerate after simplification: keep 3 spread points
        n = len(r)
        out = [r[0], r[n // 3], r[2 * n // 3]]
    return out


def quantize(ring):
    out = []
    for lon, lat in ring:
        q = (int(round(lon * 100)), int(round(lat * 100)))
        q = (max(-18000, min(18000, q[0])), max(-9000, min(9000, q[1])))
        if not out or out[-1] != q:
            out.append(q)
    if len(out) > 1 and out[0] == out[-1]:
        out.pop()
    return out


def fold(s):
    """ASCII sort key (accents dropped) so 'Côte d'Ivoire' sorts under C."""
    import unicodedata
    return "".join(ch for ch in unicodedata.normalize("NFKD", s) if not unicodedata.combining(ch)).lower()


def c_str(s):
    out = '"'
    for b in s.encode("utf-8"):
        if b == 0x22 or b == 0x5C:
            out += "\\" + chr(b)
        elif 0x20 <= b < 0x7F:
            out += chr(b)
        else:
            out += "\\%03o" % b  # octal escapes can't swallow the next character
    return out + '"'


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "."
    dst = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), "..", "..", "core", "geo_data.cpp")
    countries = json.load(open(os.path.join(src, "ne_110m_admin_0_countries.geojson"), encoding="utf-8"))
    places = json.load(open(os.path.join(src, "ne_110m_populated_places.geojson"), encoding="utf-8"))
    places10 = json.load(open(os.path.join(src, "ne_10m_populated_places_simple.geojson"), encoding="utf-8"))

    caps = {}
    for f in places["features"]:
        q = f["properties"]
        caps.setdefault(q["ADM0_A3"], []).append((q["NAME"], q["FEATURECLA"], q["ADM0CAP"], q["LONGITUDE"], q["LATITUDE"]))
    caps10 = {}
    for f in places10["features"]:
        q = f["properties"]
        if "capital" in q["featurecla"].lower():  # San Juan is an "Admin-1 capital" there
            caps10.setdefault(q["adm0_a3"], []).append((q["name"], q["longitude"], q["latitude"]))

    iso_of = {f["properties"]["ADM0_A3"]: f["properties"]["ISO_A3_EH"] for f in countries["features"]}

    def capital_for(a3):
        if a3 in NO_CAPITAL:
            return None
        lst = caps.get(a3, [])
        if a3 in CAPITAL_CHOICE:
            m = [c for c in lst if c[0] == CAPITAL_CHOICE[a3]]
            assert len(m) == 1, (a3, lst)
            return m[0][0], m[0][3], m[0][4]
        if a3 in ALT_OK:
            m = [c for c in lst if c[0] == ALT_OK[a3]]
            assert len(m) == 1, (a3, lst)
            return m[0][0], m[0][3], m[0][4]
        if a3 in FALLBACK_10M:
            # the 10m file codes some places by ISO (South Sudan is SSD there, SDS here)
            m = [c for k in sorted({a3, iso_of[a3]}) for c in caps10.get(k, []) if c[0] == FALLBACK_10M[a3]]
            assert len(m) == 1, (a3, caps10.get(a3))
            return m[0]
        m = [c for c in lst if c[1] == "Admin-0 capital" and c[2] == 1]
        assert len(m) == 1, (a3, lst)
        return m[0][0], m[0][3], m[0][4]

    out = []
    for f in countries["features"]:
        p = f["properties"]
        a3 = p["ADM0_A3"]
        iso, official = p["ISO_A3"], True
        if iso == "-99":
            iso = p["ISO_A3_EH"]
        if iso == "-99":  # no ISO code (N. Cyprus, Somaliland, Kosovo): NE's own ADM0_A3
            iso, official = a3, False
        cont = CONTINENTS.index(p["CONTINENT"])
        g = f["geometry"]
        polys = g["coordinates"] if g["type"] == "MultiPolygon" else [g["coordinates"]]
        rings = [r for poly in polys for r in poly]
        largest = max(rings, key=ring_area)
        kept = []
        for r in rings:
            if r is not largest and ring_area(r) < MIN_RING_AREA:
                continue
            q = quantize(simplify_ring(r, EPS))
            if len(q) >= 3:
                kept.append((r is largest, q))
        assert kept and kept[0] is not None
        kept.sort(key=lambda t: not t[0])  # largest (main) ring first
        qrings = [q for _, q in kept]
        # zoom box: the main ring plus rings within NEAR_GAP of it
        def bb(q):
            xs = [x for x, _ in q]
            ys = [y for _, y in q]
            return [min(xs), min(ys), max(xs), max(ys)]
        box = bb(qrings[0])
        changed = True
        used = {0}
        while changed:
            changed = False
            for i, q in enumerate(qrings):
                if i in used:
                    continue
                b = bb(q)
                gx = max(0, b[0] - box[2], box[0] - b[2]) / 100
                gy = max(0, b[1] - box[3], box[1] - b[3]) / 100
                if gx <= NEAR_GAP and gy <= NEAR_GAP:
                    box = [min(box[0], b[0]), min(box[1], b[1]), max(box[2], b[2]), max(box[3], b[3])]
                    used.add(i)
                    changed = True
        cap = capital_for(a3)
        if cap:  # NE writes "Washington,  D.C." with two spaces
            cap = (" ".join(RENAME.get(a3, cap[0]).split()), cap[1], cap[2])
        out.append({
            "name": p["NAME"], "iso": iso, "official": official, "cont": cont,
            "cap": cap, "label": (int(round(p["LABEL_X"] * 100)), int(round(p["LABEL_Y"] * 100))),
            "rings": qrings, "box": box,
        })

    out.sort(key=lambda c: fold(c["name"]))
    isos = [c["iso"] for c in out]
    assert len(set(isos)) == len(isos), "duplicate codes"

    points, rings = [], []
    lines = []
    lines.append("// GENERATED by tools/geo/make_geo_data.py from Natural Earth 1:110m (public domain).")
    lines.append("// Do not edit by hand: change the generator and re-run it (see tools/geo/README.md).")
    lines.append('#include "geo.h"')
    lines.append("")
    lines.append("namespace calc {")
    lines.append("namespace geo {")
    lines.append("")
    crow = []
    for c in out:
        first_ring = len(rings)
        for q in c["rings"]:
            rings.append((len(points), len(q)))
            points.extend(q)
        cap = c["cap"]
        if cap:
            cn, clon, clat = cap
            cs, cx, cy = c_str(cn), int(round(clon * 100)), int(round(clat * 100))
        else:
            cs, cx, cy = '""', 0, 0
        crow.append("    {%s, %s, %s, %d, %d, %d, %d, {%d, %d, %d, %d}, %d, %d, Continent(%d), %s}," % (
            c_str(c["name"]), c_str(c["iso"]), cs, cx, cy, c["label"][0], c["label"][1],
            c["box"][0], c["box"][1], c["box"][2], c["box"][3], first_ring, len(c["rings"]), c["cont"],
            "true" if c["official"] else "false"))
    assert len(points) < 65536 and len(rings) < 65536
    lines.append("extern const Point kGeoPoints[] = {")
    for i in range(0, len(points), 8):
        lines.append("    " + " ".join("{%d,%d}," % pt for pt in points[i:i + 8]))
    lines.append("};")
    lines.append("extern const int kGeoPointCount = %d;" % len(points))
    lines.append("")
    lines.append("extern const Ring kGeoRings[] = {")
    for i in range(0, len(rings), 8):
        lines.append("    " + " ".join("{%d,%d}," % r for r in rings[i:i + 8]))
    lines.append("};")
    lines.append("extern const int kGeoRingCount = %d;" % len(rings))
    lines.append("")
    lines.append("// name, code, capital, capital lon/lat x100, label lon/lat x100, zoom box x100,")
    lines.append("// first ring, ring count, continent, code is ISO 3166-1 alpha-3. Sorted by name.")
    lines.append("extern const Country kGeoCountries[] = {")
    lines.extend(crow)
    lines.append("};")
    lines.append("extern const int kGeoCountryCount = %d;" % len(out))
    lines.append("")
    lines.append("}  // namespace geo")
    lines.append("}  // namespace calc")
    with open(dst, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    strbytes = sum(len(c["name"].encode()) + 1 + len(c["iso"]) + 1 + (len(c["cap"][0].encode()) + 1 if c["cap"] else 1) for c in out)
    print("countries %d, rings %d, points %d" % (len(out), len(rings), len(points)))
    print("bytes: points %d, rings %d, countries(32-bit, 36 B each) %d, strings %d, total %d" % (
        4 * len(points), 4 * len(rings), 36 * len(out), strbytes,
        4 * len(points) + 4 * len(rings) + 36 * len(out) + strbytes))
    caps_seen = {}
    for c in out:
        if c["cap"]:
            caps_seen.setdefault(c["cap"][0], []).append(c["name"])
    dups = {k: v for k, v in caps_seen.items() if len(v) > 1}
    if dups:
        print("WARNING duplicate capital names:", dups)


if __name__ == "__main__":
    main()
