# AI calculator: final assembly, 1:1 (Fusion 360)

> **Stage 14 note:** the model uses the stage-13b board STEP; v14 only re-wired J3's pins and notched SW1, so the mechanics are unchanged. Final decision: the battery lies on the back-cover floor and the LR44 cup is **not** ground (the "LR44 cup trimmed flat" in the table below is the old plan). See `../../FINAL_STATUS.md`.

> **Rev C2 (2026-10-05): the body is two parts + a separate case.** Rebuilt from Nirav's photos: the silver
> faceplate and the navy back cover now split the way the real calculator does (the navy back cover forms the side
> walls round the keypad and the bottom; the silver part is deep only in the screen section). Added the black
> window mask and the navy slide case (hidden in the main configuration). Results: `assembly_report.md` section 7.
>
> **The correct files:** `ai_calculator_final_assembly.f3d` / `.step` in this folder (rebuilt 2026-10-05 14:54 on the
> v14 board STEP `hardware/fab/ai_calc_board.step` of 2026-10-04 14:50). The replica alone: `../replica/fx115es_replica.f3d` / `.step`. Older copies elsewhere are out of date.
>
> | Part | Colour | Fusion component | What it is |
> | --- | --- | --- | --- |
> | 1. Faceplate (the "front shell") | silver | `Replica - Front shell` | face with the keys, display window (lens recess), solar window; posts, ribs and collars underneath. Deep walls only in the screen section (stubs, clips, wire channel, top wall with the magnet U-notch); round the keypad and the bottom it is a faceplate with a short skirt that sits inside the back cover |
> | 2. Back cover | navy | `Replica - Back cover` | the screwed-on tray: floor, ribs and rings, bosses; low rim in the screen section, walls rising along a ramp to full height round the keypad and the bottom end; rail groove on both long sides (guess) |
> | 3. Slide case (accessory) | navy | `Final - Slide case (in use)` (on the back) and `Final - Slide case (stored)` (over the face) | separate hard case, **hidden** in the main configuration; not used for testing. `slide_case_mods.md` |
> | Window mask | black | `Final - Window mask` | 0.1 mm black sticker inside the lens, opening = e-paper active area. `window_mask.md`, `window_mask_template.svg/.dxf` |
>
> To show the case in Fusion: turn on the light bulb of one of the two `Slide case` components (never both).

This is the whole finished calculator as one 3D model: our ESP32-S3 board with every part on it,
inside the **ground (modified) fx-115ES shell** (unbranded replica), with the e-paper, camera, battery,
magnetic charging connector, key mat, key caps, window and screws. It is for checking the real product
before ordering the board.

- Fusion: **`ai_calculator_final_assembly.f3d`** (full timeline; File > Open in Fusion) and
  `ai_calculator_final_assembly.step` (any CAD program). The script tries to save the open document to your Fusion
  project as "AI Calculator - Final Assembly 1:1", but Fusion's cloud data wasn't reachable during this build
  ("InternalValidationError: id.size()"), so the build document is still open as "Untitled": save it under that name
  yourself, or open the .f3d.
- Collisions, in plain English: **`assembly_report.md`**.
- Pictures: `renders/` (list at the end).

**Board version used:** `hardware/fab/ai_calc_board.step` of **2026-10-04 04:11** (stage 13b: magnet header J3 =
C46061768 at KiCad (127.5, 72.0), no board notch, magnet face flush in a U-notch in the top wall).

**Result in one line:** everything fits after the 5 planned grinds, except the LiPo where stage 13 places it (it hits
the solar-window frame and cell, and its wires can't get round the board edge). Laying it on the back-cover floor fixes
that; see `assembly_report.md`.

## Part list

| Part (Fusion component) | Source | Size used (mm) | Where it sits |
| --- | --- | --- | --- |
| Front shell | fx-115ES replica (`../replica/`, your calipers), **ground**: LR44 cup trimmed flat, magnet slot cut in the top wall | 159.9 × 79.3, 11.8 / 11.3 thick | |
| Back cover | replica, **ground**: solar box flat to the floor, rib B flat over the camera, 7 mm camera window drilled, lip notched at the magnet slot | 161 × 79.3, floor 1.0 | |
| PCB | `hardware/fab/ai_calc_board.step` (KiCad export, every part with its 3D model) | 0.71 in the STEP (KiCad says 0.8) | component (F.Cu) side 6.0 above the inside of the back floor (Z 7.0), turned over so the parts face the back cover |
| E-paper panel | Waveshare 2.13" V4 raw panel: outline 59.2 × 29.2 × 1.05, active area 48.55 × 23.71 (Waveshare manual); your caliper P3 59.0 | 59.0 × 29.2 × 1.05 (bottom glass 0.7 + film/cover 0.35, 5 mm bare ledge with the driver IC at the ribbon end: estimate) | key side, on 0.15 double-sided tape, active area at the KiCad keep-out (147.445, 92.986), display facing the window |
| E-paper ribbon | 24-pin, 0.5 pitch; 14.3 long (stage 5 / V9) | 13.0 wide × 0.12 | out of the panel's ribbon end, folded 180° under the panel (in the tape gap), back to the 1.0 × 14 board slot at x 172.9, through it, 90° into J2 |
| Camera module | Seeed OV5640 AF (for XIAO ESP32S3 Sense), 8.5 × 8.5 × 5.4 (stage 13) | sensor board 1.0 + AF motor + Ø6 lens barrel 1.0 (split is an estimate) | component side at KiCad (150.0, 95.1) on 0.1 tape, lens pointing at the 7 mm window |
| Camera ribbon | Seeed: 70.5 long (stage 2) | 8.0 wide × 0.12 | flat, lying on the tallest part in its path, into J1. Path needed 58.7 mm from the module's near edge (tip 2 mm inside J1); Seeed's 70.5 mm is read from the module's far edge, so about 62 mm is ribbon and the spare is only 1.4–3.3 mm (review 07): no loop, lay it straight |
| LiPo battery | Adafruit #1317, 3.7 V 150 mAh (rev E) | 26.02 × 19.75 × 3.8 | KiCad x 152.59–178.61, y 60.36–80.11, centre (165.6, 70.24), flat on the back-cover floor, Z 1.0–4.8 (z_rel +2.2 to +6.0; stage 13 had it against the front plate, which collides) |
| JST-PH plug + leads | JST PHR-2 housing 6.0 × 4.5; leads Ø1.1 | only the part outside J4 (4.0 long) | in J4, leads round the board edge into the bay |
| Magnet connector (device half) | Adafruit #5358 = Yiwei MG04254FRA1S1N (`hardware/geometry/adafruit_5358_MG04254FRA1S1N.pdf`): obround face 21 × 7, 4 pads Ø1.5 at 2.5 pitch, pins Ø0.6 | face flange 1.0 + housing 19 × 5 × 3.0 (housing size estimated from the side view); legs straightened to 7.3 | face flush in the U-notch of the top wall at KiCad x 127.5, Z 2.2–9.2 (stage 13b); body ends just in front of the board edge; legs (5.3 long) pushed into J3 |
| Magnetic cable plug (detached) | Adafruit #5412 (USB-A to 4-pin magnetic, 60 cm) | mating face 21 × 7, overmould 23 × 9 × 10, Ø3.5 cable (estimated from photos) | 12 mm in front of the slot, shown unplugged |
| Key mat, key caps (47), window lens, battery lid, rubber feet, solar cell | replica | see `../replica/README.md` | as in the replica |
| Screws (6) | **assumption:** Casio-style 2.0 mm self-tapping pan head, **2 × 8** | head Ø3.8 × 1.1, modelled core Ø1.6 (threads not modelled) | from the back, head in the Ø4.4 counterbore. 8 mm gives 3.7 mm of thread in the front post; 6 mm would only give 1.7. Measure one of yours |

Not modelled: solder, wires' exact route, tape pads (0.1 / 0.15 mm), key legends (unbranded).

## Assumptions (check these on the real parts)

1. **Stack-up:** back floor 1.0 + 6.0 free (D4) + board + key side; the board is placed by its component face, so the parts on that side are at their true distance from the back cover. The STEP board is 0.71 thick (KiCad 0.8): the key side shows 0.09 low.
2. **Shell:** everything not measured in the replica is as listed in `../replica/README.md` (guesses marked there).
3. **Grinding** is modelled as flat to the floor (1.0 mm of floor left everywhere; the plan's limit is 0.8).
4. **E-paper** on 0.15 mm double-sided tape. Without it the ribbon fold has no room under the panel.
5. **Battery** on the back-cover floor (stage-14 re-check), not against the front plate as in stage 13 section 7.
6. **Magnet connector** as stage 13b: face 21 × 7 × 1.0, then a 0.7 plate, a 3.5-tall neck and a 5.0-tall rear block (4.0 deep in all), legs straightened to 5.3. Widths behind the face and the cable plug's shape are estimates. The U-notch (21.5 wide, 7.95 up from the rim) also cuts the back cover's locating lip there (the lip is a replica guess).
7. **Screws** 2 × 8 self-tappers (above).

## Re-run after a board change

Fusion open with the FusionMCPBridge add-in, then in `hardware/enclosure`:

```bash
python build_final_assembly.py pcb parts check export renders
```

`pcb` re-imports `hardware/fab/ai_calc_board.step` and finds every part by its KiCad reference
(`hardware/kicad/ai_calc.kicad_pcb`, read only); `parts` re-places the ribbons, plug and leads on the
connectors it finds (J1, J2, J4). If the board outline or the magnet position changes, edit the
parameters at the top of `build_final_assembly.py` (`MAG_XK`, `MAG_ZREL`, `LIPO_K`, `CAM_K`, ...).
Then rewrite `assembly_report.md` from `interference.json`.

A complete rebuild (new document, about 3 minutes plus the check):

```bash
python build_final_assembly.py all
```

`all` = `new shell grind pcb parts mask case check case_check export renders case_renders` (rev C2). `mask` adds the
window mask and lifts the lens by its thickness; `case` builds both slide-case positions (hidden) with the camera
hole; `check` runs with the case hidden; `case_check` checks each case position against everything (`case_fit.json`);
`case_renders only=case,mask,compare` makes the new pictures. Then `python final_assembly/window_mask_template.py`.

The shell comes from `build_fx115es_replica.py` (loaded as a module, unchanged), so any replica
parameter change shows up here on the next `all`. `python build_final_assembly.py` with no stage only
prints the derived numbers. Stages: `new shell grind pcb parts check export renders`;
`renders only=hero,section,grind,exploded,back_cover_off` makes a subset.

Each grind zone is its own timeline feature named `GRIND <zone>`: suppress it in Fusion to see the shell before grinding.

## Files

| File | What |
| --- | --- |
| `../build_final_assembly.py` | the script (parameters at the top) |
| `ai_calculator_final_assembly.f3d` / `.step` | the model |
| `assembly_report.md` | interference check, plain English |
| `interference.json` | raw check output (every overlap with its box) |
| `placement.json` | where every STEP part landed, STEP timestamp |
| `geometry.json` | the replica's 2D numbers used for this build |
| `PROGRESS.md`, `build.log` | build notes |
| `renders/` | pictures |

## Renders (`renders/`)

| File | What |
| --- | --- |
| `hero_front.png`, `hero_back.png`, `front.png`, `back.png` | the finished calculator (cable plug shown detached) |
| `exploded_front.png`, `exploded_back.png` | exploded views |
| `back_cover_off.png`, `back_cover_off_top.png`, `back_cover_off_closeup_top.png` | back cover, screws and feet removed: board, camera + ribbon, LiPo, J4 plug + leads, magnet in its notch |
| `section_camera.png`, `section_camera_wide.png` | cut through the camera and its 7 mm window |
| `section_j4_battery.png`, `section_battery_lengthwise.png` | cut across J4 + plug + LiPo, and along the battery |
| `section_magnet_slot.png` | cut through the magnet, its legs in J3, the top wall |
| `section_screen.png`, `section_screen_fpc_end.png` | cut across the screen section, and close-up of the e-paper ribbon fold, slot and J2 |
| `case_in_use_front/back/side.png`, `case_stored_front/back.png` | the slide case on the back (in use) and over the face (stored) |
| `case_camera_hole_closeup.png`, `case_camera_hole_section.png` | the 8 mm case hole over the 7 mm camera window |
| `case_magnet_closeup_in_use.png`, `case_magnet_closeup_stored.png` | the magnet face with the case on (open end: not blocked) |
| `exploded_3_parts_and_case.png`, `exploded_shells_only.png` | silver faceplate, navy back cover and navy case, apart |
| `mask_before_*.png` / `mask_after_*.png` | display without / with the window mask (straight on, whole front, angled) |
| `compare/cmp_*.jpg` | Nirav's photos next to the CAD at the same angle (front, faceplate inside, back cover inside); `compare/*_cad.png` the CAD views |
| `grind_<zone>_before.png` / `_after.png` | one pair per grind, in grinding order (each "before" already has the earlier grinds done): `solar_box`, `rib_b`, `camera_window` (+ `_inside_`), `lr44_cup`, `magnet_slot` |

Other stages: `check grind=off` (the shell before grinding, `interference_before_grind.json`), `clearances`
(smallest gaps, `clearances.json`), `lipo_alt dz=-5.6 [dx=..] [unground=lr44_cup]` (what-if for the battery position),
`view` (any debug picture).

## Assembly-guide pictures (`renders/steps/`, `renders/photos/`)

`python build_final_assembly.py steps [only=s05,s07]` renders the 16 step shots (`STEP_SHOTS` in the script: parts shown, moves, camera,
labelled anchor points) to `renders/steps/sNN_*.png` plus `anchors.json`. Then `python final_assembly/annotate_guide.py` (Pillow)
draws the arrows and labels into `*_ann.png`, and resizes/labels Nirav's photos into `renders/photos/` (`*.jpg`, `*_ann.jpg`, private).
`assembly_guide.html` uses the `_ann` files.
