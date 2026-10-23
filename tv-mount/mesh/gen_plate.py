"""Printed VESA adapter plate: TV (100x100, M6) <-> Suptek MD6TB plate (75x75, M4).

Stack: TV | printed plate | Suptek steel plate | arm head.
- M6 x 16 bolts go through the plate into the TV; heads sit in pockets on the Suptek-side face.
- M4 screws go from the arm side through the Suptek plate's 75x75 holes into blind holes here.
Printed TV-face down: pockets and M4 holes open upward, no supports needed.
"""
import sys
import numpy as np
import trimesh
from manifold3d import Manifold, CrossSection

SIZE        = 125.0   # mm square
CORNER_R    = 6.0
THICK       = 14.0
TV_PITCH    = 100.0   # TV VESA
M6_CLEAR    = 6.6
POCKET_D    = 13.0    # fits M6 hex (11.5 mm across corners), socket heads (10 mm), M6 washers (12 mm)
POCKET_DEPTH = 8.0    # fits a socket head (6 mm) or hex head + washer; 6 mm floor -> M6x16 engages 10 mm
SUP_PITCH   = 75.0    # Suptek plate holes used for the M4 screws
M4_PILOT    = 3.6     # self-tapping pilot in PLA
M4_DEPTH    = 14.0    # through: an M4x15 tip stops 1 mm short of the TV
BED_CENTER  = (128.0, 128.0)   # X1C 256 mm bed
SEG         = 64

def rounded_square(s, r):
    h = s / 2 - r
    pts = []
    for cx, cy, a0 in ((h, h, 0), (-h, h, 90), (-h, -h, 180), (h, -h, 270)):
        for k in range(17):
            a = np.radians(a0 + 90 * k / 16)
            pts.append((cx + r * np.cos(a), cy + r * np.sin(a)))
    return CrossSection([pts])

def cyl(d, h, x, y, z0):
    return Manifold.cylinder(h, d / 2, d / 2, SEG).translate((x, y, z0))

plate = Manifold.extrude(rounded_square(SIZE, CORNER_R), THICK)
e = 0.01
for sx in (-1, 1):
    for sy in (-1, 1):
        x, y = sx * TV_PITCH / 2, sy * TV_PITCH / 2
        plate -= cyl(M6_CLEAR, THICK + 2 * e, x, y, -e)
        plate -= cyl(POCKET_D, POCKET_DEPTH + e, x, y, THICK - POCKET_DEPTH)
        x, y = sx * SUP_PITCH / 2, sy * SUP_PITCH / 2
        plate -= cyl(M4_PILOT, M4_DEPTH + 2 * e, x, y, THICK - M4_DEPTH - e)

def to_trimesh(man):
    mm = man.to_mesh()
    t = trimesh.Trimesh(vertices=np.asarray(mm.vert_properties)[:, :3], faces=np.asarray(mm.tri_verts), process=True)
    assert t.is_watertight and t.is_winding_consistent and t.volume > 0
    return t

# Fit template: same outline and hole positions, 2 mm thick, all holes through.
TEMPLATE_T = 2.0
template = Manifold.extrude(rounded_square(SIZE, CORNER_R), TEMPLATE_T)
for sx in (-1, 1):
    for sy in (-1, 1):
        template -= cyl(M6_CLEAR, TEMPLATE_T + 2 * e, sx * TV_PITCH / 2, sy * TV_PITCH / 2, -e)
        template -= cyl(4.5, TEMPLATE_T + 2 * e, sx * SUP_PITCH / 2, sy * SUP_PITCH / 2, -e)

m = plate.to_mesh()
mesh = trimesh.Trimesh(vertices=np.asarray(m.vert_properties)[:, :3], faces=np.asarray(m.tri_verts), process=True)
assert mesh.is_watertight and mesh.is_winding_consistent and mesh.volume > 0
mesh.apply_translation((BED_CENTER[0], BED_CENTER[1], 0))
out = sys.argv[1]
mesh.export(f"{out}/tv_vesa_adapter_plate.3mf")
mesh.export(f"{out}/tv_vesa_adapter_plate.stl")
tm = to_trimesh(template); tm.apply_translation((BED_CENTER[0], BED_CENTER[1], 0))
tm.export(f"{out}/fit_template_2mm.3mf")
print(f"volume {mesh.volume/1000:.1f} cm3, ~{mesh.volume/1000*1.24:.0f} g PLA solid, bbox {np.round(mesh.extents,2)}")
