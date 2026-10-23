import sys, numpy as np, trimesh
def load(p):
    s = trimesh.load(p); return list(s.geometry.values())[0] if hasattr(s, "geometry") else s
def holes_at(mesh, z):
    sec = mesh.section(plane_origin=(0, 0, z), plane_normal=(0, 0, 1))
    planar, _ = sec.to_2D()
    out = []
    for poly in planar.polygons_full:
        for ring in poly.interiors:
            pts = np.asarray(ring.coords)[:-1]
            c = pts.mean(0); d = 2 * np.linalg.norm(pts - c, axis=1).mean()
            out.append((c, d))
    return planar, out
for path in sys.argv[1:]:
    m = load(path); ctr = m.bounds[:, :2].mean(0)
    print(f"\n{path}: watertight={m.is_watertight} size={np.round(m.extents, 2)}")
    for z in sorted({0.5, m.extents[2] / 2, m.extents[2] - 0.5}):
        _, hs = holes_at(m, z)
        # to_2D recentres; report positions relative to the plate centre
        planar, _ = holes_at(m, z)
        T = None
        desc = sorted((round(d, 2), tuple(np.round(np.abs(c - planar.bounds.mean(0)), 2))) for c, d in hs)
        print(f"  z={z:5.2f} mm: {len(hs)} holes -> " + ", ".join(f"d{d} @±{p[0]},±{p[1]}" for d, p in desc))
