"""Concept layout v2: battery in the old LR44 corner, magnet behind the solar cell.
Seen from the back with the back cover off (= KiCad top view); mirrored vs the front."""
import json
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle, Polygon as P, FancyBboxPatch
from shapely.geometry import Polygon, box

F = json.load(open("features.json"))
board = Polygon(F["board_outline_mm"])
board = board.difference(box(147.5, 40, 200, 78.3).buffer(0.8).buffer(-0.8))  # bigger battery cut-out
BAT = (147.9, 63.5, 31.0, 11.5)        # Adafruit #1570 LiPo: 31 x 11.5 x 3.8 mm, 100 mAh
MAG = (136.0, 63.75, 21.5, 7.5)         # 5358 zone centred behind the solar cell

fig, ax = plt.subplots(figsize=(8.6, 15.5), dpi=140)
bg = "#F6F5F0"; fig.patch.set_facecolor(bg); ax.set_facecolor(bg)
ax.add_patch(P(F["outer_shell_mm"], closed=True, fc="#2B3A55", ec="#1b2438", lw=1.2, alpha=0.18))
ax.add_patch(P(list(board.exterior.coords), closed=True, fc="#1E5E45", ec="#0d2a1f", lw=1.3))
for h in F["holes"]:
    ax.add_patch(Circle(h["at"], h["d"] / 2, fc=bg, ec="#0d2a1f", lw=0.7))
for n in F["notches"][:2]:
    ax.add_patch(Circle(n["at"], 2.2, fc="#7a8aa8", ec="none"))
for b in ([116.755, 69.007], F["coin_pocket_cut"]["top_right_boss"]):
    ax.add_patch(Circle(b, 2.25, fc="#7a8aa8", ec="#4b5872", lw=0.8))

def rect(x, y, w, h, **k):
    ax.add_patch(Rectangle((x, y), w, h, **k))
def crect(c, s, **k):
    rect(c[0] - s[0] / 2, c[1] - s[1] / 2, s[0], s[1], **k)
def label(x, y, t, size=7, color="w", **k):
    ax.text(x, y, t, fontsize=size, color=color, ha="center", va="center", **k)

# other side (B): window + e-paper, dashed
crect(F["window"]["center"], F["window"]["size"], fill=False, ec="#d36bff", ls="--", lw=1)
crect(F["epaper"]["center"], F["epaper"]["outline"], fill=False, ec="#ffb347", ls="--", lw=1)
label(150, 108.6, "e-paper + window on the other side", 6.5, "#ffd9a0")
# back-cover pins
for p in F["back_cover_pins"]:
    ax.add_patch(Circle(p["at"], 2.5, fill=False, ec="#e58bd8", ls=":", lw=0.9))
# camera
c = F["camera"]["center"]
crect(c, (8.5, 8.5), fc="#2D3336", ec="#9aa3a8", lw=1)
ax.add_patch(Circle(c, 3.0, fc="#0E1418", ec="#e05050", lw=1.4))
rect(c[0] - 6, c[1] + 4.6, 12, 2.2, fc="#c68a3c", ec="none")
label(c[0], c[1] + 9.2, "camera (OV5640) + 24-pin socket", 6.5)
# ESP32-S3-MINI-1 with antenna to the left edge
rect(116.5, 80.5, 20.5, 15.4, fc="#3B4147", ec="#9aa3a8", lw=1)
rect(113.9, 79.5, 5.6, 17.4, fill=False, ec="#f2c230", ls="--", lw=1)
label(127.5, 88.2, "ESP32-S3\nMINI-1", 6.5)
label(116.7, 98.4, "antenna\nkeep-out", 5.5, "#f2c230")
# e-paper socket + driver parts
rect(157.0, 87.0, 14, 3.2, fc="#c68a3c", ec="none"); label(164, 92.6, "e-paper socket + driver", 5.8)
# battery in old LR44 corner
x, y, w, h = BAT
ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0,rounding_size=1.2",
                            fc="#6E7FA6", ec="#2f3b5c", lw=1.2))
label(x + w / 2, y + h / 2 - 1.2, "Adafruit #1570 LiPo\n31 x 11.5 x 3.8 mm, 100 mAh", 6.5)
label(x + w / 2, y + h / 2 + 3.6, "sits in the board cut-out (full case depth)", 5.2, "#dfe6ff")
rect(150.0, 79.4, 7, 3.4, fc="#d9dcd8", ec="#6b747c"); label(163.8, 81.1, "battery socket JST-PH", 5.5)
ax.plot([152.5, 152.5], [75.2, 79.4], color="#d2432f", lw=1.2)
ax.plot([154.5, 154.5], [75.2, 79.4], color="#222", lw=1.2)
# magnet behind solar cell
mx, my, mw, mh = MAG
rect(mx - mw / 2, 60.0, mw, mh, fc="#B8BEC4", ec="#3e4a3e", lw=1.2)
ax.add_patch(FancyBboxPatch((mx - 10.5, 60.25), 21, 3.4, boxstyle="round,pad=0,rounding_size=1.7",
                            fc="#15181B", ec="none"))
for i in range(4):
    ax.add_patch(Circle((mx + (i - 1.5) * 2.5, 61.95), 0.6, fc="#c9a24a"))
    ax.add_patch(Circle((mx + (i - 1.5) * 2.5, 67.0), 0.45, fc="#c9a24a"))
label(mx, 70.2, "magnetic connector (5358) in top-wall slot\nwhere the solar cell was; legs -> 4-pin header", 5.6)
# power block
rect(124.0, 72.6, 20, 5.0, fc="#20252A", ec="#6b747c"); label(134, 75.1, "charger + 3.3 V + ESD", 5.6)
# keypad scanner
rect(144.5, 110.8, 11, 4.5, fc="#20252A", ec="#6b747c"); label(150, 113.05, "TCA8418", 5.8)
# key pads (B side) faint
for k in F["keys"]:
    crect((k["x_mm"], k["y_mm"]), (k["w"], k["h"]), fill=False, ec="#d8c48a", lw=0.7, alpha=0.75)
    label(k["x_mm"], k["y_mm"], k["label"], 4.8, "#e9ddb5")
# back-cover ribs behind number rows: keep flat
for yy in (172.2, 183.6, 195.0):
    rect(118.5, yy - 0.6, 63, 1.2, fc="#e58bd8", alpha=0.35, ec="none")
label(150, 216.2, "pink bands: back-cover ribs press here (keep this side flat)", 6.2, "#7b2f74")

ax.set_xlim(104, 196); ax.set_ylim(219, 52); ax.set_aspect("equal")
ax.set_xticks(range(110, 196, 10)); ax.set_yticks(range(60, 220, 10)); ax.tick_params(labelsize=6)
ax.grid(alpha=0.15)
ax.set_title("AI Calculator board, concept layout v2 (to scale, mm)\n"
             "Seen from the BACK with the back cover off, so left/right are mirrored vs the front\n"
             "Battery in the old coin-cell corner, magnet connector behind the solar cell",
             fontsize=9.5)
plt.savefig("layout_concept_v2.png", bbox_inches="tight", facecolor=bg)
