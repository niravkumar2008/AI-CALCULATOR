# v15-LCD final assembly: does everything fit? (final board + ER-TFT019-1)

**Verdict: YES, it fits.** The final v15-LCD board (review 14: R23 15 Ω, SW1/SW2/SW50 moved) with the BuyDisplay ER-TFT019-1 panel fits the ground fx-115ES shell with no new grind. The closed-case check finds only the intended contacts.

- **The one tight spot is unchanged:** the tail's bow sits 0.25 mm above the un-ground part of rib B with the nominal 36.6 mm tail. With the longest tail the drawing allows (36.9 mm) the gap is 0.10 mm. From about 37.1 mm the bow touches the rib (37.2 mm gives a 0.74 mm³ overlap). A light touch is harmless; if the closed cover squeezes the bow, grind zone 2 about 15 mm further.
- **The three moved key pads** now sit centred under the Casio key-mat pills: each pill is 100 % on its pad and bridges 5 finger gaps, against 85–100 % and 4 gaps before the move.
- **Nothing found blocks the order.**

Re-run on 2026-10-08 at 21:46–22:00 by `build_final_assembly_v15.py`:
- **Stages:** new, shell, grind, pcb, parts, mask, check, clearances, probe, fulltable, tailcase (new), `check grind=off`, export, v15renders, steps.
- **Then:** `compose_v15.py`, `annotate_guide_v15.py` and `key_pills_v15.py` (new).
- **Only read, never changed:** the v14 model, `hardware/kicad/`, `hardware/fab/`, `hardware/archive/`, `final_assembly/`, `bringup_guide.html` and `hardware/kicad_v15_lcd/`. No problem was found in `kicad_v15_lcd/`. Nothing was committed.

## What was modelled

- **Board:** `board/ai_calc_v15_lcd_board.step`, exported at 21:46 with `kicad-cli pcb export step --subst-models --no-dnp` from the final `.kicad_pcb` (saved 21:37).
  - It has the same 15,709 CARTESIAN_POINTs as the 21:38 fab STEP, so no 3D body moved.
  - Footprint positions come from the final board: SW1 (177.75, 119.80), SW2 (167.15, 120.80), SW50 (122.45, 119.55), J5 (158.6, 93.1).
- **Panel: ER-TFT019-1, no touch** (datasheet rev 2.0 p.6):
  - Backlight 49.72 × 25.80, glass 48.52 × 24.80. The CF glass is 45.92, which leaves a 2.6 mm ledge. Thickness 1.43 ± 0.1.
  - Visual area 43.72 × 23.695, active area 42.72 × 22.695, centred on (150.0, 93.1). The drawing puts it 0.1 mm further from the tail end, inside the ±0.15 tolerance.
  - Tail 36.6 ± 0.3 × 15.5 × 0.3, 4.5 mm stiffener, modelled **0.5 mm off-centre towards finger 1** (KiCad y 93.6; report 14 M4).
  - **Still assumed:** the layer split inside the panel (0.65 / 0.35 / 0.30 / 0.13) and the 0.1 mm tape.
- **Kept unchanged:**
  - Seeed OV5640 AF camera, #1317 LiPo (rev E), #5358 magnet piece, JST-PH plug and leads.
  - v15 mask, opening 43.72 × 23.70.
  - Replica rev G with grinds 1, 2, 3, 5/5b, 6 and 6b. Zone 6c is not modelled.
  - Key mat and caps.

## 1. Interference (closed case, every body pair, 3D boolean)

- **After grinding:** 194 bodies, 413 pairs, **68 lumps, all intended, 0 errors**:
  - the tail tip inside J5: 62 lumps, 2.6 mm³, all between front X −10.50 and −8.53 (inside the 2 mm insertion) and KiCad y 85.85–101.35 (within J5's body, 84.6–101.6);
  - the magnet legs in J3 (4);
  - the LiPo leads in their plug (2).
- **Before grinding:** 100 lumps. That is the v14 rev G list plus the J5 tip. The v14 list is J4, the magnet on the top wall and lip, J3, D1, D7, U3, Q2, the LiPo, its plug and leads, the camera lens on rib B, and the board and U1 on the side-wall pins.
- **Nothing of the LCD, tail or J5 touches the un-ground shell.** So no new grind is needed, and no grind can be skipped.

## 2. Clearances (mm)

Full table: 527 pairs, 1 FAIL (board ↔ post H6, 0.10, the known v14 item).

| What | Gap (mm) |
| --- | --- |
| Bow ↔ rib B, tail 36.6 | **0.25** (front X −29.8, Y 44.24, Z 2.25; KiCad 179.8, 94.7) |
| Bow ↔ rib B, tail 36.3 / **36.9** / 37.2 | 0.40 / **0.10** / touches (0.74 mm³) |
| Tail ↔ slot walls | 0.35 each side |
| Tail ↔ slot ends | 2.75 (pad-30 end) / 1.75 (finger-1 end) |
| Tail ↔ Q4 | 1.32 |
| LCD ↔ front plate | 1.27 |
| LCD ↔ mask / lens | 1.48 / 1.57 |
| LCD ↔ solar dummy | 3.33 |
| LCD ↔ board | 0.1 tape (the model shows 0.19 because its board is 0.71 thick) |
| Tail ↔ plate / mask | 1.42 / 1.61 |
| Tail ↔ camera | 4.28 |
| Tail ↔ LiPo | 5.73 |
| Tail ↔ foot 2 | 1.87 |
| J5 ↔ back cover | 3.92 |
| U1 ↔ panel | 0.97 |

Unchanged from v14: camera 0.71, J4 0.41, J1 0.95, D2 0.66, magnet 0.25 / 0.71, LiPo ↔ front shell 0.30, LiPo plug 0.80, camera ribbon 0.48.

## 3. Height stack (Z from the outside of the back cover): unchanged

| Layer | Z (mm) |
| --- | --- |
| Board key face | 7.80 |
| Tape | 7.80–7.90 |
| Backlight | 7.90–8.55 |
| TFT glass | 8.55–8.90 (tail on the ledge 8.90–9.20) |
| CF glass + polarizer (LCD top) | 8.90–9.33 |
| Free space | 1.27 (1.17 with a +0.1 panel) |
| Plate underside | 10.6 |
| Mask | 10.81–10.91 (lens lifted 0.095) |
| Bow, lowest face | 2.25 (4.75 below the component face) |
| Rib B top, un-ground part | 2.0 |

## 4. Tail route and the rib-B margin

- **Route:**
  1. Bonded 1.3 mm on the ledge.
  2. Across to the slot (x 181.0) at Z 9.05, then straight down the slot.
  3. One U-bow under the board.
  4. Up into J5's mouth (x 160.53) at Z 6.42, 2.0 mm inside.
- **Length:** the shortest route is 30.2 mm, so the 36.6 mm tail leaves 6.4 mm spare, all of it in the bow. Bow corners 1.2 mm radius, top bend 1.0 mm.
- **Tolerance:** each 0.3 mm of tail moves the bow down 0.15 mm.

| Tail (mm) | Bow lowest face Z | Gap to rib B |
| --- | --- | --- |
| 36.3 | 2.40 | 0.40 |
| 36.6 | 2.25 | 0.25 |
| 36.9 | 2.10 | 0.10 |
| 37.2 | 1.95 | touches |

- **Next nearest body:** the slot wall, at 0.35 mm.
- **If a tail measures over about 37.0 mm, or the step-10 marker dot shows the bow squeezed:** grind zone 2 about 15 mm further towards the slot side, at the same depth. Don't fold the tail. 2 mm corners are not an option (they need Z 1.73).

## 5. Moved key pads vs pills (`key_pills.md`, `renders/key_pills_top_row.png`)

| Key | New pad | Casio pill offset | On pad (before → now) | Gaps (before → now) | Pill 0.7 mm off |
| --- | --- | --- | --- | --- | --- |
| SW1 SHIFT, Ø 3.8 | 177.75, 119.80 | 0.03 | 88 → 100 % | 4 → 5 | 95 % |
| SW2 ALPHA, Ø 3.9 | 167.15, 120.80 | 0.21 | 100 → 100 % | 4 → 5 | 90 % |
| SW50 ON, Ø 3.5 | 122.45, 119.55 | 0.02 | 85 → 100 % | 4 → 5 | 97 % |

- **Coverage:** about 56–59 % of each pill lies on finger metal; one bridged gap is enough to close a key.
- **Other keys are unchanged:** REPLAY 91–94 %, bottom row 96–97 %, as verification 15 found.
- **Modelling note:** the replica's mat domes and caps sit on the old pad centres (`board_snapshot.json`, rev G read-only), so in the CAD they are 1.50 / 1.12 / 1.55 mm off the new pads. That only affects the pictures, not the fit.

## 6. Pin 1 and backlight (from report 14, no CAD change)

- **Pin 1:** finger 1 arrives at the bottom of J5, at pad 1 (157.4, 100.35) by the silk tick, fingers up. The tail's finger-1 edge is now at y 101.35.
- **Backlight:** R23 is 15 Ω, giving 26 mA typical (16–37 across panels, ≤ 51 worst case). Electrical only, no 3D effect.
- **Still owed on real parts:** the tail length, the pin-1 diode test and the backlight current.

## 7. What changes for Nirav

- **No new grind.** Leave the rest of rib B unless the step-10 check shows the bow squeezed.
- **Measure the tail in step 4.** Up to 37.0 mm is fine.
- **Panel placement:** use the jig, or centre the panel on the slot with the backlight end 4.34 mm from the slot's near edge. The tail then sits about 0.5 mm towards the finger-1 end; that is normal.
- **The ER-TFT019-1 arrives bare.** The guide's breakout part is now optional.
- **Mask unchanged:** 43.72 × 23.70, centred.

## 8. Files

- **Changed:**
  - `build_final_assembly_v15.py`: `LCD_AA` 22.695, `LCD_FPC_DY` 0.5, `LCD_FPC_LEN_WORST`, `MASK_OPEN`, `fit_bow()`, `tail=` arg, stage `tailcase`, s05a on y 93.6, v15 name in the full-table header.
  - `annotate_guide_v15.py`.
- **New:** `key_pills_v15.py`, `key_pills.json`, `key_pills.md`, `tail_worstcase.json`, `renders/key_pills_top_row.png`.
- **Regenerated:** the board STEP (21:46), `.f3d` / `.step`, every check JSON / MD, and all renders and step pictures under the same names.
- **`assembly_guide_v15_lcd.html` text updated:** parts list, step 3, rib-B callouts in steps 2 and 5, the step 4 tail limit, the step 5 slot note, the step 8 key note.
- **Unchanged:** `compose_v15.py`, the mask template, the position jig, `board_snapshot.json`.
