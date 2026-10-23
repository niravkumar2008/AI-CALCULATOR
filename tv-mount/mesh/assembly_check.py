"""Virtual assembly check: TV | printed plate | Suptek plate, with real hardware sizes.
z=0 is the TV's back surface; +z goes toward the arm."""
from manifold3d import Manifold
P = dict(plate_t=14.0, pocket_d=13.0, pocket_depth=8.0, m6_clear=6.6, m4_pilot=3.6, m4_depth=14.0,
         sup_t=2.0, sup_hole=4.5, tv_hole_depth=10.0)
SEG = 64
def cyl(d, h, x, y, z0): return Manifold.cylinder(h, d/2, d/2, SEG).translate((x, y, z0))
def box(sx, sy, h, z0): return Manifold.cube((sx, sy, h), True).translate((0, 0, z0 + h/2))
C100 = [(sx*50, sy*50) for sx in (-1, 1) for sy in (-1, 1)]
C75 = [(sx*37.5, sy*37.5) for sx in (-1, 1) for sy in (-1, 1)]

def build(bolt, m4_len, washer):
    t = P["plate_t"]
    plate = box(125, 125, t, 0)
    for x, y in C100:
        plate -= cyl(P["m6_clear"], t + .02, x, y, -.01)
        plate -= cyl(P["pocket_d"], P["pocket_depth"] + .01, x, y, t - P["pocket_depth"])
    for x, y in C75:
        plate -= cyl(P["m4_pilot"], P["m4_depth"] + .01, x, y, t - P["m4_depth"])
    sup = box(120, 120, P["sup_t"], t)                     # flat side down on the printed plate
    for x, y in C100 + C75:
        sup -= cyl(P["sup_hole"], P["sup_t"] + .02, x, y, t - .01)
    floor = t - P["pocket_depth"]
    bolts = Manifold()
    head_base = floor + (1.6 if washer else 0)
    for x, y in C100:
        if washer: bolts += cyl(12.0, 1.6, x, y, floor)
        bolts += cyl(bolt["head_d"], bolt["head_h"], x, y, head_base)
        bolts += cyl(6.0, bolt["len"], x, y, head_base - bolt["len"])   # shank toward the TV
    m4 = Manifold()
    top = t + P["sup_t"]
    for x, y in C75:
        m4 += cyl(7.0, 2.8, x, y, top)                      # pan head on the arm side
        m4 += cyl(4.0, m4_len, x, y, top - m4_len)
    return plate, sup, bolts, m4, head_base, top

def vol(m): return m.volume() if hasattr(m, "volume") else m.get_volume()

bolts = {"M6x16 hex head":    dict(len=16, head_d=11.05, head_h=4.0),
         "M6x16 socket head": dict(len=16, head_d=10.0, head_h=6.0),
         "M6x16 button head": dict(len=16, head_d=10.5, head_h=3.3)}
print(f"TV hole depth assumed: {P['tv_hole_depth']} mm\n")
for name, b in bolts.items():
    for washer in (False, True):
        for m4_len in (15,):
            plate, sup, bo, m4, head_base, top = build(b, m4_len, washer)
            head_vs_sup = vol(bo ^ sup); bolt_vs_plate = vol(bo ^ plate)
            screw_vs_plate = vol(m4 ^ plate) ; engage_tv = b["len"] - (head_base - 0)
            m4_tip = top - m4_len; m4_floor = 0.5   # keep at least 0.5 mm off the TV back
            ok = (head_vs_sup < 1e-6 and bolt_vs_plate < 1e-6 and engage_tv >= 8
                  and engage_tv <= P["tv_hole_depth"] and m4_tip >= m4_floor)
            print(f"{name:18s} washer={'yes' if washer else 'no ':3s} M4x{m4_len:<2d} | "
                  f"head clears Suptek: {'yes' if head_vs_sup < 1e-6 else 'NO'} | "
                  f"M6 into TV {engage_tv:4.1f} mm | M4 in plastic {min(m4_len - P['sup_t'], P['m4_depth']):4.1f} mm, "
                  f"{f'tip {m4_tip:.1f} mm from TV'} | {'OK' if ok else 'PROBLEM'}")
