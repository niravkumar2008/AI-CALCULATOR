# Knockoff shell: guide pictures and animations

Pictures and GIFs of the v15 board in the knockoff (RZ-991ES type) shell, for the build guide and the grinding guide.
Rendered from the Fusion 360 knockoff assembly (`../build_knockoff_assembly.py`, components "KO15 - ...",
`../cad/assembly/ai_calculator_v15_knockoff_assembly.f3d`). The marking-cap component "KO15 - Templates" is hidden in
every picture. Model frame: mm, front view, X right, Y up, Z = 0 at the outside of the back cover, top edge Y 82.1.

Every `*_ann.png` is the labelled copy of the `.png` with the same name (made offline by `ko_annotate_guides.py`).
In grinding pairs, red on "before" = material to remove (the pixel difference to "after"); green outline on "after" = where it was.

## Grinding (`renders/grind/`)

| File | What it shows |
|---|---|
| `ko_z1_solar_box_before/after` | Zone 1, back cover inside: the open solar box (X 4.3..23.9, Y 65.5..75.6, 5.5 tall) ground flat to the floor |
| `ko_z2_battery_plate_before/after` | Zone 2, back cover inside: the raised battery plate flattened under the LiPo (dashed box, ~26.5 x 19.5) |
| `ko_z3_camera_window_inside_before/after` | Zone 3, inside: 7 mm camera hole at (0, 42.0), midway between back ribs 2 and 3 |
| `ko_z3_camera_window_outside_before/after` | Zone 3, outside of the back cover: where the drill comes out |
| `ko_z5_magnet_notch_before/after` | Zone 5, front shell top wall: magnet U-notch 21.5 wide x 7.95 deep from the rim |
| `ko_z5b_lip_before/after` | Zone 5b, back cover: the lip under the magnet notch taken level with the rim |
| `ko_z6_antenna_relief_before/after` | Zone 6, front shell: inner side-wall rib notched to 7.0 mm below the rim (14 -> 49 mm from the top) |
| `ko_z6b_pins_hooks_before/after` | Zone 6b, front shell: 3 pins + 3 hooks snipped off the inner rib (one wall, close-up) |
| `ko_z6b_pins_hooks_both_walls_before/after` | Zone 6b on both side walls |
| `ko_overview_back_cover_before/after` | Straight-on view of the back cover inside, all knockoff zones numbered (1, 2, 3, 5b) |
| `ko_overview_faceplate_before/after` | Straight-on view of the front shell inside, zones 5, 6, 6b (battery cups stay) |
| `ko_camera_hole_position` | Outside view with dimensions: hole centre 40.1 mm below the top edge, on the width centre line, 7 mm |
| `ko_magnet_notch_dims` | Top-end view of the U-notch with its dimensions (21.5 / 7.95 / 6.4 / 27.9 from the right edge) |
| `anchors.json` | Model points projected to image pixels for the labels (written by the renderer) |

## Assembly close-ups that differ from v15 (`renders/detail/`)

| File | What it shows |
|---|---|
| `ko_d01_battery_corner_back` | Back cover inside: LiPo on the ground plate, leads, JST plug; J4 and the front-shell cups dashed |
| `ko_d02_battery_corner_board` | Same corner seen from the back with the cover off: board, LiPo, leads, J4 + plug |
| `ko_d03_battery_corner_section` | Section at Y 70.5 (cup centre line): cups / board / LiPo / J4 / back-cover floor + battery lid |
| `ko_d04_camera_between_ribs` | Camera module on the back cover between ribs 2 and 3 (board hidden) |
| `ko_d05_camera_rib_section` | Section at X 0: camera vs ribs 2 and 3 (0.35 mm each side), 7 mm window, board |
| `ko_d06_window_mask_off` | Screen lens in the front shell with the black window mask lifted off |
| `ko_d07_window_mask_on` | Mask stuck on the lens |
| `anchors.json` | Label anchor points |

## Animations (`animations/`, 960 x 720)

| File | What it shows |
|---|---|
| `assembly_ko.gif` / `assembly_ko_poster.jpg` | Build order of the guide (steps 5-11): LCD, camera, magnet, key mat, board, battery, cover; screen comes on |
| `turntable_ko.gif` / `turntable_ko_poster.jpg` | Closed calculator turning 360 deg, screen on (firmware screenshot) |
| `xray_ko.gif` / `xray_ko_poster.jpg` | Exploded <-> assembled loop with see-through shells |

Raw frames go to `_frames/ko/<anim>/` (git-ignored). The screen is a green chroma key in Fusion, replaced offline by
`hardware/renders_ui_v15/05_calc_result_functions.png` (main checkout, read only).

## Regenerate

Fusion 360 open with the FusionMCPBridge add-in and the knockoff assembly document loaded (root occurrences "KO15 - ...").
Run one Fusion job at a time. Nothing is saved; grind features are all left active and Templates hidden afterwards.

```
cd hardware/knockoff/guides
python ko_guide_renders.py ver=ko set=grind              # grinding pictures (only=ko_z1,ko_z3 for some)
python ko_guide_renders.py ver=ko set=detail             # assembly close-ups
python ko_annotate_guides.py                             # all *_ann.png (offline; "grind" / "detail" / name prefixes to limit)
python ko_animate.py ver=ko anim=all                     # assembly + turntable + xray (anim=xray for one; pick=0,12 test frames)
python ko_animate.py compose ver=ko                      # rebuild the GIFs from the raw frames only (no Fusion)
```

Sources: `ko_guide_renders.py`, `ko_annotate_guides.py`, `ko_animate.py` are copies of `hardware/enclosure/guide_renders.py`,
`annotate_guides.py` and `animate_assembly.py` (main checkout) with ver "ko". Change from the originals besides the
knockoff shot lists: the scale-bar `px_per_mm` is now `min(W, H) / ext` (the original's estimate was 5-10 % off).
