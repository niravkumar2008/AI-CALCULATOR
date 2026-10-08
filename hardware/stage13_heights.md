# Stage 13: the three height problems (J4 battery socket, J3 magnet, camera)

> **⚠ Partly superseded by stage 14 / final assembly: see `FINAL_STATUS.md`.** The battery now lies flat on the back-cover floor, so the LR44 cup (grind #3 and the LiPo rows below) is **not** ground. Final grinds: solar box, rib B 14 mm, 7 mm camera drill, magnet U-notch (5b back-cover lip only if present). J3 order is VBUS, D−, D+, GND (stage 14). The battery is now the **Adafruit #1317, 150 mAh, 26 × 19.75 × 3.8** (decided 2026-10-06, `enclosure/final_assembly/battery_upgrade.md`), not the #1570 in the table below; it is held with **0.1 mm double-sided tape, no foam pad** (the "2–3 mm foam" and "≤ 0.2 mm tape" below are stage-13 wording).

**Board result:** `kicad-cli pcb drc --refill-zones --schematic-parity` = **0 errors, 0 warnings, 0 unconnected, 0 parity issues**. ERC = 0. Version silk "AI CALC v13 2026-10-03". Zip: `Claude outputs/ai_calc_pcb_13.zip`. **Stage 13b (2026-10-04) included:** the magnet connector is now option A (right-angle header), and two extra 22 µF caps sit on the ESP32 3.3 V feed.

**Short answer:** with Nirav's corrected depth (6.0 mm from the board to the inside of the back cover), the battery socket and the camera both fit once the shell is ground in two places. The magnet connector didn't fit as designed. Nirav chose option A, and it's done (section 3): the magnet sits in front of the board edge with its legs straightened into a right-angle SMD header that JLCPCB places. **Nothing height-related blocks the order any more.** Before ordering, do the dry fit and check the JLCPCB preview (`ORDER_CHECKLIST.md`).

## 1. The new numbers

Nirav, 2026-10-03:
- **D4 = about 6.0 mm**, measured from the board to the **inside** face of the back cover.
- **D1 = 5.5 mm**, measured from the front shell's **rim** (where the back cover meets it) down to the board's component side.

Now the stack adds up: back cover 1.0 + **6.0 free** + board 0.8 + about 4.0 on the key side = 11.8 mm (D10).

```
 back cover, outside  ------------------------------------------  0
 back cover floor 1.0 (never grind into this; keep >= 0.8 mm)
 ribs 1 / rings 4 / solar box 5-5.5 hang down from the floor  <- grind where needed
 free space 6.0 mm  (design limit 5.7 = 6.0 - 0.3 clearance)
 board component side (F.Cu)  ----------------------------------  7.0
 board 0.8
 key side: key mat / e-paper, face plate                          ~ 11.8
```

**Rule used everywhere:** part height + rib height above it must be at most **5.7 mm**.

The stubs and clips beside the screen stop above the board (photo 871567b6), so they never touch it. I kept the stage-12 narrowing anyway: it does no harm, and the ESP32 antenna overhang is good for the radio.

## 2. Results per part

| Part | Height | What's above it | Fits? | Margin to the back cover | What to do |
|---|---|---|---|---|---|
| **J4** battery socket (JST S2B-PH-SM4-TB) | 5.5 | solar box (5–5.5) | **yes, after grinding** | **0.5** (0.2 under the 5.7 limit) | Grind the solar box flat. No board change. |
| **Camera** OV5640 module (not soldered) | 5.4 (+ ≤0.1 tape) | rib B (1 mm) crosses it | **yes, after grinding** | **0.5–0.6** | Grind rib B flat for 14 mm over the camera. Drill a **7 mm** window. No board change. |
| **J3** right-angle header (C46061768) + **magnet connector** | header 2.5; magnet body 4.8 above the board (beside it, not on it) | solar box (header only) | **yes** (stage 13b) | header 3.5 after grinding; magnet 1.2 | Cut the top-wall slot (section 6), straighten the legs, glue the magnet. |
| LiPo (in the battery bay, beside the board) | 3.8 | solar box over its left end; LR44 cup (front shell) under its middle | yes, after grinding both | bay depth ~9.4 | Grind the solar box, trim the LR44 cup flat. |
| U2, U3, D1, Q2, D7, C5, C6, R2, R4 | 0.3–1.6 | solar box | yes, after grinding | ≥ 4.4 | Same solar-box grind. |
| D2, C16 | 1.26 / 0.5 | big ring (4) | **yes**, no grind needed now | 0.74 | The stage-12 plan to grind the big ring is no longer needed. |
| Everything else | ≤ 2.41 | 1 mm ribs or nothing | yes | ≥ 2.5 | — |

Full table: `fitcheck/clearance_table.md`. It now includes rows for the camera, the battery and the magnet. It's conservative: any rib within 1.5 mm of a part counts as being on top of it, so a few small parts near a rib show less margin than the CAD replica finds.

**Could moving U3/D1/Q2 avoid the solar-box grind?** No. J4, the left end of the LiPo and the magnet all sit under the solar box too, so it has to be ground anyway. Grinding it is also what Nirav prefers, so I left all the power parts where they're electrically best.

**Could J4 be lower instead?** I checked every PH-compatible right-angle SMD socket on JLCPCB: JST C295747 5.5, XUNPU C3029440 5.6, DEALON C2905019 5.6, HCTL C2845442 5.5, JXT C52204942 5.5. None is lower, because the PH plug itself is about 4.5 mm thick. LCSC has no sink-mount PH socket either. Grinding the solar box is the easy fix, and the battery (Adafruit #1570) doesn't change.

## 3. The magnet connector (J3): option A, done in stage 13b

**The real part.** I re-read the Yiwei MG04254FRA1S1N drawing (`geometry/adafruit_5358_MG04254FRA1S1N.pdf`, rendered at high resolution):

| Feature | Size |
|---|---|
| Face plate | 21 × 7 oval, 1.0 thick; 4 Ø1.5 contacts at 2.5 mm pitch on the centre line, magnets 14 mm apart |
| Behind the face | 0.7 plate, a 3.5 mm-tall neck, then a 1.0 mm rear block 5.0 mm tall. Total depth 4.0. |
| Pins | Ø0.6. They leave the rear block **on the body's centre line**, run straight back to 5.5 mm behind the face, bend 90° and end 1.8 mm below the rear block (4.3 below the centre line). |
| Leg length | Straightened, each leg is about **5.3 mm** long behind the rear block. |

**What the board does now:**
- **No notch needed.** The board's top edge is already at y ≈ 62. With its face flush in the top wall (outside y ≈ 57.3), the magnet body ends at y 61.3, so it sits fully **in front of** the board edge, straddling the board's height.
- **J3 = HX PM2.54-1x4P WT, LCSC C46061768** (right-angle SMD female header, 2.54 mm):
  - About 7,900 in stock, $0.18, Extended, so the same loading fee as before.
  - Body 10.66 × 8.5 × 2.5. Socket centre 1.3 mm above the board.
  - Placed at **(127.5, 72.0)**, entry facing the top wall at y 61.9.
  - Pins 1–4 at x 123.69 / 126.23 / 128.77 / 131.31. **Stage 14:** the order is now **VBUS, D−, D+, GND** (power and ground on the outer pins), with the magnet piece's **N end at pin 1**; the old VBUS, GND, D−, D+ order matched neither way of fitting the piece (verification 03 F1). See `stage14_verification_fixes.md`.
  - New footprint `ai_calc:PinSocket_1x04_P2.54mm_SMD_RA_C46061768` (pads 1.02 × 3.0 per the hanxia drawing) with a box 3D model.
- **Magnet height:** its centre is level with the socket centre (1.3 mm above the component side). So it spans from **2.2 mm below** the component side to **4.8 mm above** it.
  - That's 1.2 mm under the back cover (0.9 under the 5.7 limit).
  - Towards the keys there's 2.7 mm to the front plate.
  - It clears the back-cover lip by 0.2 mm, the solar box (which starts at y 64), the solar-window frame in the front shell (starts at y 65) and the top-corner screw post (x 116, y 65).
- **Moved to make room:**
  - D7 (VBUS TVS) to (119.2, 73.4), right next to J3's VBUS pin.
  - U2 (charger) to (135.9, 67.5).
  - C6 to (139.6, 66.0).
  - J4, Q1/Q2, D1, U3 and every other part stayed.
- **Re-routed:** VBUS, SYS, VBAT_P, USB D+/D−, CHG_PROG and CHG_STAT_RAW in that corner. Power nets are 0.5 mm, signals 0.2 mm, the same as before.
- **Unchanged electrically:** the magnet contacts still only see VBUS and GND. Battery voltage can't reach them, because D1 (Schottky) and Q2 (load share) are unchanged and R20 still pulls up to VBUS_SENSE.
- **Schematic:** J3 footprint, LCSC, MPN and datasheet fields updated.

**What Nirav does:**
1. Cut the slot in the top wall (grind list, item 5).
2. Straighten the 4 legs of the magnet piece with flat pliers so they point straight back out of the body.
3. Push them into J3, about 4.5 mm deep. If they bottom out before the face is flush, trim 0.5–1 mm off the legs with side cutters.
4. With the case closed, check that the face is flush. Then glue the body into the slot (epoxy or hot glue). The legs mustn't take the cable's pull.
5. **Before gluing**, do the multimeter check in `ORDER_WALKTHROUGH.md` → "Magnet piece" on the real cable: the leg that reads +5 V goes into J3 pin 1 (silk "N" / "+"), the GND leg into pin 4 (silk "−"), and D−/D+ must beep through to USB-A pins 2/3. (Stage 14: the old advice to "turn the piece over if mirrored" is withdrawn: with the old wiring that put 5 V on D+.)

**Risk:** the body depth and leg position come from the drawing. Check them on the real part when it arrives (P5). If the legs come out of the body more than 1 mm off-centre, bend a small step into them.

## 4. Camera: window and mounting

- **Where:** module centred at **(150.0, 95.1)**, as before, inside its 12 × 12 keep-out. The lens points at the back cover.
- **Window:** drill **7 mm** (a 9/32" bit is 7.1 mm) through the back cover at that spot, after grinding rib B.
  - Why 7: the lens top is about 1.5 mm below the outside of the cover. A 70–75° lens and a ~3 mm lens opening need about 5.3 mm. Adding ±0.5 mm for where the module ends up gives about 6.3 mm, and 7 leaves some spare.
  - The lens does **not** need to poke into the hole any more.
- **How to hold it:**
  - Put a piece of Kapton tape on the board under the module (insulation over the tracks).
  - Stick the module down with **thin** double-sided tape (≤ 0.2 mm, e.g. 3M 9080 or a Kapton loop). **Not foam tape:** 1 mm foam would leave only −0.5 to 0 mm.
  - Centre it under the window before closing the case.
- **Heat sink:** Seeed sells this module "with heat sink". If the heat sink adds height on top (total above 5.5), leave it off.
- **Cable (corrected in stage 14, verification 03 L1):** the drawing's 70.5 mm runs from the module's far edge to the tip, so the ribbon beyond the module is about 62 mm and there is only about **1.4 mm** to spare to J1 at (150, 161), not 10 mm. Lay it flat and straight; no S loop. If it comes up short, let the module sit up to 1.7 mm towards J1 inside its keep-out and drill the window where the lens actually is (decision rule in `stage14_verification_fixes.md`). It passes under the mid rib and the small ring (4 mm high), leaving 2 mm.

## 5. Other fixes in this stage

- **Locating posts H2/H9/H6/H13/H14.** The CAD (from caliper C15) and the photos disagree by 0.3–1.2 mm on where these posts are. Each hole now fits the post in **both** positions:

  | Hole | Now | Room for the post (photo / CAD position) |
  |---|---|---|
  | H2 | slot 4.2 × 5.0, centre y 176.25 | 0.30 / 0.61 |
  | H9 | slot 4.4 × 4.8, y 176.2 | 0.50 / 0.46 |
  | H6 | round 4.2, moved to y 186.99 | 0.43 / 0.10 |
  | H13 | slot 4.2 × 4.45, y 198.325 | 0.45 / 0.46 |
  | H14 | slot 4.2 × 4.65, y 198.375 | 0.60 / 0.61 |

  (Before: H2 −0.54, H9 −0.29, H6 −0.11 against the CAD posts.)
  - Copper stays ≥ 0.3 mm from every hole, which is why H2/H6 can't get bigger: key pads are right next to them.
  - New footprints: `ai_calc:Post_Slot_*` (plain non-plated slots; check that the JLCPCB quote doesn't add a charge for them).
  - If H6 still pushes on the real post, file the hole a little toward the top of the board.
- **The wire hook at the top right no longer needs snipping:** the clips stop above the board.
- **Board notes:** camera note updated (7 mm window, grind rib B) and the version silk is now v13.
- **Two extra 22 µF caps on +3.3 V (stage 13b):** C33 at (121.7, 84.75) and C34 at (124.55, 85.35), 0805, LCSC **C45783** (Basic, so no extra fee).
  - Both sit on the 0.5 mm +3.3 V trunk 5 mm before it reaches the ESP32, each with its own GND via. That's 66 µF bulk next to the module in total (C1 + C33 + C34) for Wi-Fi bursts.
  - Added in the schematic (MCU sheet, beside C32) and the BOM.
  - This was the only free spot: the module's 3V3 pin (123.3, 104) is boxed in by C1/C2/C3, TP1/TP4 and the board edge.
- **Fit-check tools:** `fitcheck/backcover_fx115es.json` now uses 6.0 / 5.5 mm free height. Rib B is split so only its camera part is ground, and the front shell's LR44 cup was added. `tools/make_fitcheck.py` adds the camera, battery and magnet rows, draws the LR44 cup on the front-shell map and prints a numbered grind list on each map.

## 6. Grind list (print `fitcheck/grind_map_back_cover.svg` and `grind_map_front_shell.svg` at 100 %)

| # | Where | What | Depth |
|---|---|---|---|
| 1 | Back cover, solar box (KiCad x 121–154, y 64–76; top-left on the mirrored map) | grid and frame flat **down to the floor** | 5–5.5 mm (D6/D13) |
| 2 | Back cover, rib B over the camera (x 143–157, y ≈ 95) | flat down to the floor, 14 mm long | 1 mm |
| 3 | Front shell, LR44 coin-cell cup (centre 168.3, 69.0; outer Ø 14) | trim flat to the floor | 5–6 mm (D7) |
| 4 | Back cover, camera window at (150.0, 95.1) | drill through | Ø 7 mm |
| 5 | **Front shell, top end wall: magnet slot** (KiCad x 116.75–138.25). Seen from the front, display up, that's **6.4 to 27.9 mm in from the right-hand side edge**. | Cut a U-notch **21.5 mm wide** from the rim (parting line) down to **7.95 mm below the rim**, through the 1.2 mm wall. Fit the magnet face (21 × 7) so it sits 0.45–7.45 mm below the rim, flush with the outside. Glue it in. | through the wall |
| — | Everywhere | never grind *into* the floor: keep at least 0.8 mm of plastic | — |

Why a notch from the rim rather than a closed window: the window's bottom edge would be only 0.45 mm above the rim, too thin to survive. With the notch, the 0.7 mm gap under the magnet is filled with glue, and the back cover closes it.

## 7. Off-board parts for the Fusion assembly (KiCad coordinates)

KiCad x to the right, y down (as in the board file), seen from the component (F.Cu) side. Z = height above the board's **component side** (positive = towards the back cover; the board itself is 0 to −0.8). Fusion front-view frame (same as the replica): X = 150 − x, Y = 138.94 − y.

| Part | Size (mm) | Centre (x, y) | Extent x / y | Z range | Orientation | Notes |
|---|---|---|---|---|---|---|
| **LiPo** Adafruit #1570 | 31 × 11.5 × 3.8 | (163.4, 69.25) | 147.9–178.9 / 63.5–75.0 | **−3.4 to +0.4** (on the front-shell floor after the LR44 cup is trimmed) | long side along x, flat (thickness along Z) | Beside the board, in the bay (board edge y 78.24, x ≥ 147.5). A 2–3 mm foam pad between it and the back cover holds it. Its leads go to J4 at (142.6, 74.6). |
| **Camera** Seeed OV5640 AF | 8.5 × 8.5 × 5.4 | (150.0, 95.1) | 145.75–154.25 / 90.85–99.35 | **0 to +5.4** (+0.1 tape) | lens axis +Z (out through the back cover), cable leaving towards +y (to J1) | Back-cover window Ø 7 at (150.0, 95.1). |
| **E-paper** Waveshare 2.13" V4 panel | 59.0 × 29.2 × 1.0 | (149.97, 92.99) | 120.47–179.47 / 78.39–107.59 | **−0.8 to −1.8** (key side, display facing the front window) | long side along x; ribbon at the x = 179.47 end, folds behind and up through the slot at x 172.9 to J2 | Active area 48.55 × 23.70 at x 123.17–171.72, y 81.14–104.84. |
| **Magnet connector** Yiwei MG04254FRA1S1N (#5358 / #5412 piece) | face 21 × 7 oval (1.0 thick) + body to 4.0 deep | face centre (127.5, 57.8); body centre (127.5, 59.3) | 117.0–138.0 / 57.3–61.3 (straight legs to y ≈ 66.6, inside J3) | **−2.2 to +4.8** (centre +1.3 = J3 socket centre) | face outward (−y) in the top-wall slot, 21 mm along x, 7 mm along Z | Pin 1 (VBUS, the piece's N end) at the low-x end; stage 14 order VBUS, D−, D+, GND. Slot in the front shell's top wall: x 116.75–138.25, U-notch from the rim 7.95 deep. |
| J3 (on the board) | HX PM2.54-1x4P WT | pads (127.5, 72.0) | body 122.17–132.83 / 61.9–70.4 | 0 to +2.5 | entry faces −y | Box model in the STEP. |
| J4 (on the board, for reference) | S2B-PH-SM4-TB | (142.6, 74.6) | 138.5–146.7 / 69.8–79.2 | 0 to +5.5 | as in the STEP | Fits under the ground solar box. |

## 8. Files

- Board: `kicad/ai_calc.kicad_pcb`.
  - Backup before this stage: `kicad/.mcp-backups/stage13_pre/`.
  - New footprints: `kicad/ai_calc.pretty/Post_Slot_*.kicad_mod`.
  - Stage 13b backup: `kicad/.mcp-backups/stage13b_pre/`.
  - New footprint: `PinSocket_1x04_P2.54mm_SMD_RA_C46061768.kicad_mod`, with model `ai_calc.3dshapes/HX_PM2.54-1x4P_WT_box.step`.
- Fit check: `fitcheck/clearance_table.md`, `fitcheck/grind_map_back_cover.svg`, `fitcheck/grind_map_front_shell.svg`, `fitcheck/dummy_board.stl`, `fitcheck/backcover_fx115es.json`.
- Fab: `fab/`, including a regenerated `fab/ai_calc_board.step` for the Fusion assembly.
- Progress log: `stage13_progress.md`.
