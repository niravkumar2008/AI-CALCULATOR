# Knockoff shell — photo review (2026-10-14)

Shell: Alibaba Casio-style knockoff (Runzon RZ-991ES PLUS type), bought for bulk.
Photos: `photos_2026-10-14/K01…K10` (phone, ruler in frame). Reference numbers: fx-115ES readings in
`hardware/measurements_2026-10-03.md` (main checkout) and the v15 board in `hardware/kicad_v15_lcd/`.

Photo-only estimates are about ±1 mm in plan view; heights from the side-on photos (K02, K03) are **not usable**
(ruler not touching the part, strong perspective).

## What the photos show

| # | Finding | Evidence | Confidence |
|---|---|---|---|
| 1 | **The v15 paper board fits the front shell, and the posts come up through its holes.** 10 holes (5 per side, left and right patterns differ like the fx-115ES pairs) + 2 bottom-edge notches line up with posts. | K10 | High for "within ~1 mm" |
| 2 | Shell size and shape match the fx-115ES (≈ 150–160 × 75–80 mm, length:width ≈ 2.0 vs 2.03). The ruler sits above the floor, so the photo reads ~5 % small. | K07, K10 | Medium |
| 3 | **Same 50-key layout** as fx-115ES/991ES: row 1 = 2 + D-pad + 2, row 2 = 2 + 2, three rows of 6, four rows of 5 → 46 + 4 D-pad = 50. Matches our 50 B.Cu key pads. **Nirav (2026-10-14): the knockoff key mat lines up exactly with the real fx-115ES key mat**, so the v15 key pads (already checked against the Casio contacts in review 14) need no change. | K01, K05 + Nirav | High |
| 4 | Key mat = one rubber sheet with black carbon pills, located by the same posts. | K01 | High |
| 5 | Original PCB covers the **key area only**, is screwed to the front-shell posts with small screws, has a COB chip (black blob), a coin cell holder on the board, and meander (interdigitated) round gold key contacts. | K08, K09 | High |
| 6 | Original LCD is a **separate glass module with a flex ribbon** to the PCB (not a zebra strip), backed by yellow foam. Our v15 IPS replaces it, so its flex does not matter. | K07, K08, K09 | High |
| 7 | Top of the front shell: **two round moulded pockets** (top right) and a **black rectangle** (top left; solar cell, possibly a dummy). The paper sits over or around them; their **height above the board plane is unknown**. J4 (5.5 mm tall) and the LiPo are near here. | K01, K06, K09, K10 | Must measure |
| 8 | Screen window looks similar in proportion to the fx-115ES window (60.65 × 24.3). Size not confirmed. | K06, K07 | Medium |

## Not covered yet

- **Back cover inside**: no photo yet. This is where the camera hole (7 mm, under the 8.5 × 8.5 camera), the ribs and the LiPo sit.
- All heights (see the measurement list).

## CAD model (started 2026-10-14, photos only)

`build_knockoff_shell.py` = the fx-115ES replica builder (`hardware/enclosure/build_fx115es_replica.py`, main checkout)
with the knockoff changes marked `KNOCKOFF`: two joined round battery pockets instead of the LR44 cup with clip bars,
and the solar window moved right to clear them. Everything else is still the fx-115ES geometry.
Outputs go to `cad/`, and Fusion components are named "Knockoff - …".

```
python hardware/knockoff/build_knockoff_shell.py                      # offline 2D check -> cad/check_2d.txt
python hardware/knockoff/build_knockoff_shell.py new front back keys parts looks check shots
```

**Build 1 (2026-10-14):** all stages ran, interference **0** (the first try had the left pocket 0.7 mm³ into the
top-left corner screw boss; the pockets were moved 0.8–1.6 mm right). Outputs: `cad/knockoff_shell.f3d`, `.step`,
`cad/stl/`, renders in `cad/renders/` (`inside_front_back.png` matches photo K01: solar cell, two pockets, window, posts).

When the readings arrive, change these numbers and rebuild:
K-M2 → `D1_RIM`, K-M3 → `COIN_CUP[2]` / `SOLAR_FRAME[1]`, K-M4 → `D4_IN`, K-M5 → `T_TOP` / `T_KEY`, K-M6 → `WIN`.
The back cover is still the fx-115ES one (no knockoff photo yet).

## Rev 2: photos K11–K17 and the detailed assembly (2026-10-14)

Nirav checked the shell against the fx-115ES and dropped the measurement sheet: "most of the measurements are the same".
He listed what differs: the top-right corner (seen from inside), the battery and screw holes there, fewer holes in the
original board (not a problem), small styling differences (ignore), and a new camera spot from the back cover's grooves.

How the numbers were read: K01 is straight down (9.52 px/mm on the 79.3 width). K12 (back cover) is tilted, so it was
mapped with a homography fitted on the four screw bosses the paper board confirmed (mid pair 46.5 c-c at Y 13.0, bottom
pair 39.0 c-c at Y -71.65). Check point: the top-right boss maps to (32.4, 74.3) against the fx-115ES (34.05, 74.05).
Expect ±1.5 mm. Cross-check: the back cover's battery hole (X -25.1) lies right under the outer cup from K01 (X -25.0).

| Feature (front view, mm) | fx-115ES | Knockoff (rev 2) | Source |
|---|---|---|---|
| Battery-side corner screw | (-34.05, 74.05) | **(-33.5, 66.2)**: small boss in the front, plain hole in the back | K01 (-34.15, 66.7), K12 (-32.9, 65.6) |
| Pin in that corner | — | (-35.0, 73.2), Ø1.6 | K01, K16 |
| Coin-cell cups | one LR44 cup with clip bars at (-18.3, 69.94) | **two interlocking C cups**, Ø11.8, at (-14.5, 70.5) and (-25.0, 70.5) | K01, K15–K17 |
| Solar strip centre X | 10.85 | 13.3 | K01 |
| Back-cover ribs | grid + two rings + lower ribs | **3 full-width ribs** at Y 57.0 / 47.1 / 36.9 (25.1 / 35.0 / 45.2 below the top), 1.0 tall (guess) | K12 |
| Battery hole | Ø12 at (-18.3, 69.94), 16 × 15 lid recess | Ø10 at (-25.0, 70.5); lid recess cut to 11.8 (outside not photographed) | K12 |
| Raised plate round the hole | — | X -37.3..-3.2, Y 59.7..79.2, 0.6 tall (guess) + 2 through slots | K12 |
| Solar box (back cover) | X -4..29, Y 62.9..74.9, 4 × 2 grid | X 4.3..23.9, Y 65.5..75.6, no grid | K12 |
| Camera centre | Y 43.84 (KiCad y 95.1) | **Y 42.0 (KiCad y 96.94)**, midway between ribs 2 and 3, 0.35 mm each side | K12 ribs |

**Result (build 16:10–16:22, 2026-10-14):** `cad/assembly/ai_calculator_v15_knockoff_assembly.f3d` (15.6 MB) and `.step`
(22.7 MB), 409 bodies. Interference: **68 hits, identical to the fx-115ES v15 baseline** (62 ribbon-to-board touches,
4 magnet pins, 2 JST lead touches, all known and accepted), **0 new**. On the way there: (1) the LiPo lay on the raised
battery plate (0.6 mm) → new grind zone `battery_plate`, flat under the pouch + 0.5; (2) the plate had filled the
battery-corner screw hole and counterbore → re-cut after the plate; (3) the SETUP label sank into the face where it
starts to ramp → face labels follow the ramp. Grinds now: solar_box (removes the knockoff's top-right box; J4 and the
battery plug sit there, as on the fx-115ES), battery_plate, camera_window, magnet_slot, antenna_relief, pins_hooks.
Camera module Y 37.75–46.25 between rib 3 (36.4–37.4) and rib 2 (46.6–47.6): 0.35 mm each side.

Key labels (stage `legends`): 47 caps + 40 face labels, 0 failures. Text = our firmware key names (v15 board snapshot);
SHIFT functions yellow, ALPHA red, above the keys. Cap colours like the knockoff: lavender top rows + REPLAY, black
function rows, white number pad, orange DEL/AC. Printed as 0.04 mm painted layers in the component "Key labels".

Pictures in `cad/assembly/renders/`: `ko_front_labels`, `ko_keys_closeup`, `ko_hero`, `ko_battery_corner_inside`,
`ko_front_shell_inside`, `ko_back_cover_inside`, `ko_back_cover_top_inside`, `ko_back_cover_camera_lipo`, plus the v15
set (front, exploded, LCD sections). Dimension drawing: https://claude.ai/artifact/6eMncSDr8fyTLtRjTKgSeg
(`sheet/make_dimension_drawing.py`).

Detailed assembly: `build_knockoff_assembly.py` is a copy of the v15 fx-115ES assembly builder
(`hardware/enclosure/final_assembly_v15_lcd/build_final_assembly_v15.py`) that loads `build_knockoff_shell.py` instead of
the replica, moves the camera (`KO_CAM_DY`) and skips the rib B grind. Outputs: `cad/assembly/`.

## Measurements to confirm (caliper)

| ID | Measure | fx-115ES value | Knockoff |
|---|---|---|---|
| K-M1 | Original PCB thickness (ours is 0.8; a thicker original means keys may need a shim) | same as knockoff (Nirav) | 0.03 in ≈ 0.76 mm (unit confirmed by Nirav) → matches our 0.8 mm, no shim |
| K-M2 | Front shell: rim down to the top of a screw post (where the board sits) | D1 = 5.5 | |
| K-M3 | Front shell: rim down to the top of the two round pockets and of the black rectangle | — | |
| K-M4 | Back cover: rim down to the inside floor, at the middle and at the camera spot | D4 ≈ 6.0 (board to back-cover inside face) | |
| K-M5 | Closed thickness, top end / bottom end | 11.3 / 11.7–11.8 | |
| K-M6 | Screen window W × H | 60.65 × 24.3 | |
| K-M7 | Back cover: tallest ring/rib near the camera spot, height above the floor | 4.0 | |
| K-M8 | Front shell: rim to the top of the thin ribs beside the screen (L / R) | 4.5 / 4.5 | |
| K-M9 | Front shell: rim to the top of the solar-cell frame | 6.6 | |
| K-M10 | Screw post outside diameter | 3.85 | |
| K-M11 | Top-corner screw posts, centre to centre | 68.1 | |
| K-M12 | Window top edge to case top edge | 24.0 | |
| K-M13 | Key mat in the shell: rim to the top of its ridges (top / middle / bottom) → board seat, `MAT_T` stack | 6.3 | |
| K-M14 | Mat sheet thickness → `MAT_T` | 0.8 | |
| K-M15 | Mat thickness at a raised ridge | — | |
| K-M16 | Black pill depth below the ridges (small / big key) → `DOME_GAP` | 0.3 | |
| K-M17 | Mat top edge to the window's bottom edge (room for our screen) | ~11.7 | |

**Correction (2026-10-14):** K-M2 is measured to the post tops, which hold the board's *key* side, so its
fx-115ES value is 6.3 (5.5 to the board's back + 0.8 board), not 5.5. `D1_RIM` = K-M2 − 0.76.

Fill-in sheet with pictures: https://claude.ai/artifact/VHiWpfUghNv7vigVaftFta (source `sheet/template.html`;
`python hardware/knockoff/sheet/make_drawings.py` redraws its to-scale drawings from `cad/geometry.json`).
