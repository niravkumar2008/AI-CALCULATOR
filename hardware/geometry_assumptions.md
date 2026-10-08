# Board geometry from the scan: assumptions and confidence

> **⚠ History (fx-300ES scan era).** The geometry was re-derived for the fx-115ES in stages 10–13. Current status: `FINAL_STATUS.md`.

> **Shell switch (2026-10-02):** Nirav now uses a **Casio fx-115ES** shell, not the fx-300ES Plus. Same key grid, but it has paired screw posts (not 12 pegs), stubs on the side walls beside the screen, and rings and ribs in the back cover. The geometry below is still from the fx-300ES scan: the post holes, outline and key pads must be re-derived from the new readings and a flatbed scan (measurement sheet C7–C15, F6) before ordering.

Stage 1 of the AI Calculator main board (ai_calc_pcb_1). Every physical dimension
here comes from `hardware/scan/9C102_A.pdf`, from its embedded image
(`pdfimages -png`, 1632 × 2126 px, saved as `scan/9C102_A_extracted.png`). The script
`geometry/derive_geometry.py` reproduces all the numbers. One exception: the key
labels and the window size were checked against Nirav's photo of an fx-300ES PLUS
front (2026-10-01).

**Coordinates.** KiCad mm, Y pointing down. The front shell's outer top edge on the
centreline is at (150, 60). Scan pixel (x, y) maps to KiCad `(150 + (x−621)/7.55, 60 + (y−469)/7.55)`.

**Confidence tags.** **high** = measured directly on a visible edge or hole, and
cross-checked. **med** = measured, but one step is inferred. **low** = inferred
because the feature can't be seen. Low items also sit on KiCad layer **User.1 "VERIFY"**.

---

## 1. What the scan shows (orientation)

| Question | Answer | Why |
|---|---|---|
| Left image | Front shell seen **from inside**, with Casio's board still in it | The board's silkscreen reads normally ("PWB-GY450AX-1005", "JC-94V0"). The solar cell and LR44 sit at the top, and the LCD module's metal shield is seen from behind. |
| Right image | Back cover seen **from inside** ("9C102A" moulded) | It shows screw holes, ribs, and the solar-cell box. |
| Mirroring | Both views are mirrored left↔right compared with the calculator front. ON (top-right on the front) appears top-**left** in the scan. | Check: mirroring the back cover onto the front shell puts its D-shaped hole exactly over the LR44 (2.5 px error). All six back-cover screw holes also land on the six front-shell bosses (RMS 0.4 mm, max 0.7 mm). See `overlay_back.png`. |
| KiCad view | **KiCad top view = scan view, no mirror.** F.Cu is the component side, facing the back cover. Key pads go on **B.Cu**, facing the rubber keys. | The scan maps 1:1 onto the board as you see it in pcbnew. To see the keys as on the calculator front, use View → Flip Board. |

## 2. Scale

The printed ruler can't be trusted. Its inch marks give 9.88 px/mm, which would make
the shell 4.75" long. That ratio is almost exactly 4/3, so the ruler was most likely
printed at 75 % ("fit to page").

Three things make the scale tricky. The back cover **wraps around** the front shell. The
front shell is therefore smaller than the published 161 × 80 mm. Parallax also makes
raised rims look about 1.5 % bigger. The back cover's right edge, the one farthest from
the photo centre, sticks out 9 px further than its left edge.

| Estimate of the front-feature scale | px/mm | Notes |
|---|---|---|
| Back-cover length = 161 mm (after removing parallax) × front/back ratio 1.013 | **7.55** | Primary. The 1.013 ratio comes from screw bosses that must line up, and it is the same in x and y. |
| Back-cover width = 79.5 / 80 mm, same method | 7.56 / 7.51 | |
| Ruler, if printed at exactly 75 % | 7.51 | Paper-level scale × 1.013 |
| LR44 cell, Ø 11.6 mm (85.3 px) | 7.40 | Sits lower than the rims. 7.53 if the cell is 11.4 mm. |
| Front shell = 161 mm long (rejected) | 7.40 | Rejected because the back cover sticks out 2.5–3 mm past the front shell on every side, for the same screw pattern. |
| Prior rough pass (hint only) | 7.52 | |

**Used: 7.55 px/mm.** The estimates spread from 7.40 to 7.56 (−2.1 % / +0.1 %). Leaving out
the LR44 one, they agree within ±0.7 %. The highest px/mm gives the smallest board, so 7.55 is
the safer pick. At 7.55 the front shell measures **157.9 × 75.9 mm** outside. Holes 20 mm from
the centre carry about ±0.15 mm of scale error.

**Distortion.** The ruler edge is straight within 1.3 px RMS over 1900 px. The iOS scan
already corrected perspective to the page. Parallax was corrected as above, and the
remaining error is under 0.5 % for front-shell features. The ruler is tilted 0.3°, but
the shell is not (both post columns sit at x = 621 ± 1 px).

## 3. Geometry table

| # | Feature | Value (KiCad mm unless noted) | Conf. | How |
|---|---|---|---|---|
| G1 | Board size | 72.3 wide (top section), 64.1 wide (keypad section), 148.75 tall (y 61.99 → 210.74) | high (keypad) / med (top) | See G2/G3 |
| G2 | Keypad-section edge (y > 141) | Casio's own board edge (488 px = 64.6 mm), moved **0.25 mm inward** | high | Casio's board proves it fits. The extra 0.25 mm covers JLCPCB's ±0.2 mm outline tolerance. Casio's edge sits 0.9–1.6 mm inside the outer shell. |
| G3 | Top-section and waist edge (y < 141) | Outer silhouette minus **1.85 mm** (1.35 mm wall + 0.5 mm clearance), made symmetric about the centreline using the **narrower** side | med | The visible blue rim is 7–15 px wide. The left wall touches the ruler, so the right wall was mirrored. |
| G4 | Waist | Outer shell narrows from ≈ 76 mm (y ≤ 115 mm) to ≈ 66.5 mm (y ≥ 150 mm). The board follows it to y ≈ 141, then joins the keypad section with a smooth step. | med | Silhouette width profile |
| G5 | Convex corners | 5 mm radius. The bottom corners copy Casio's ≈ 4.8 mm. | high | |
| G5b | **Screws: exactly 6** (Nirav confirmed) | Top pair = G10 + G11, middle pair = G7, bottom pair = G6. The top screw beside the LR44 opening also holds the battery door. | high | All 6 back-cover holes line up with front-shell bosses (RMS 0.4 mm, max 0.7 mm). Everything else in this table is a plastic peg, not a screw. |
| G6 | Bottom screw-boss notches | Ø 8.0 half-circles at (130.53, 210.20) and (169.47, 209.93) | high | Blue boss tops in Casio's edge notches, which line up with the back-cover screw holes |
| G7 | Mid screw bosses (through the board) | Ø 6.0 NPTH at (128.36, 126.74) and (171.22, 126.34) | high | Blue boss with white ring, Ø 4.5 mm boss. Lines up with the back-cover screws. |
| G8 | Posts / pegs (10) | Ø 4.4 NPTH. Left x ≈ 128.6–130.7, right x ≈ 169.3–171.5, at y 137.0, 145.96 (R), 155.5 (L), 165.0, 176.3, 187.5 | high | Blue pegs Ø 2.9–3.3 mm, seen through Casio's holes. 0.55–0.75 mm radial clearance. |
| G9 | Post columns | 41.6 mm apart in the function section, 38.6 mm in the number section | high | The earlier "~41 mm" was one average for both sections. |
| G10 | Top-left screw boss | U-slot Ø 6.0 at (116.76, 69.01), open to the left edge | **med → VERIFY V1** (screw confirmed, position mirrored) | Hidden under the solar cell. Placed from the mirrored back-cover screw hole. A faint boss shows at that spot in the scan. |
| G11 | Top-right screw boss (battery-door screw) + **battery bay** (layout v2) | Corner cut-out: x ≥ 147.5, y ≤ 78.3 (boss at 181.9, 74.7). An Adafruit #1570 LiPo (31 × 11.5 × 3.8 mm) sits at x 147.9–178.9, y 63.5–75.0 and uses the full case depth. It clears the boss by 0.77 mm. Cut-out x ≥ 147.5. The battery opening in the back cover is ≈ 10.9 × 12.6 mm. | **low → VERIFY V3** | The old LR44 (5.4 mm thick) fit here, so a 3.8 mm pouch should too, once the holder ribs are Dremeled out. Depth not yet measured. |
| G12 | Solar-cell zone | (119.9, 61.9)–(152.1, 73.2) | **low → VERIFY V2** | A box on the back cover presses the solar cell here. The board keeps this area because the magnet header needs the top edge. See section 5. |
| G13 | Window | ≈ **57 × 21 mm**, centred (149.9, 96.6) | **low → VERIFY V8** | Not visible in the scan. Taken from the photo, as a ratio to the key grid (window width = 1.06 × function-column span). This replaces the old 64 × 25 mm estimate. It sits inside the LCD shield (67.4 × 32.7 mm, centre y 95.3). |
| G14 | E-paper | Outline 59.2 × 29.2, active 48.55 × 23.7, centred on the window, on the B side | med | Position only. The panel's off-centre active area gets handled in stage 4. |
| G15 | Camera | Back-cover hole Ø 6 at (150.0, 95.1). 12 × 12 mm module keepout on F side. | **low → VERIFY V6** | Not in the scan. The build-book concept puts it centred, 37 mm below the back-cover top. It lands on the "1" of the moulded "9C102A", 2.8 mm from the nearest back-cover pin. |
| G16 | Magnetic connector slot | **21.5 × 7.5 mm** slot to cut in the top wall, **centred at x = 136 behind the old solar cell** (layout v2). The zone runs from y 60 to 67.5, covering wall, body and legs. 4 leg holes at **2.50 mm** pitch, x 132.25–139.75, y ≈ 67. | **low → VERIFY V5** | Adafruit 5358 = Yiwei MG04254FRA1S1N: face 21 × 7 mm obround, about 5.5 mm deep with the right-angle legs, 2 A / 36 V. It mounts like a normal right-angle part at the board edge, so it needs about 6 mm of height above the board's F side (or a notch in the board edge; stage 4). The 0.6 mm legs go into a standard 2.54 mm female header (0.12 mm accumulated error). |
| G17 | Back-cover pins (5) | Ø 5 at (122.6, 85.0), (149.5, 85.6), (176.4, 85.6), (125.0, 112.5), (174.6, 112.5) | high | They pressed on the LCD shield. Now they press on our board, so no parts go there. |
| G18 | Back-cover ribs | Horizontal ribs sit behind number rows N1–N3 (front y 1303/1389/1475 px). There is a vertical rib on the centreline and rib dashes along both walls. | high | They support key presses. Keep F-side parts off those strips (stage 5). |

## 4. Keypad (50 contacts = 49 matrix + ON)

Positions come from the scan. The 12 pegs sit in the gaps **between** keys, and the
back-cover ribs sit **behind** the number rows. Both give the same rows. Labels follow the
fx-300ES PLUS layout from Nirav's photo. The full list is in `geometry/keys.csv`.

| Section | Columns (front L→R, scan x px) | Pitch | Rows (scan y px) | Pad (mm) | Conf. |
|---|---|---|---|---|---|
| Numbers 7–=, 5 cols | 815, 718, 621, 524, 427 | 12.85 × 11.13 mm | N1 1305, N2 1389, N3 1473, N4 1557 | 9.0 × 7.0 | high (N4 med: one row past the last gap) |
| Function rows R3–R5, 6 cols | 822, 742, 661, 581, 500, 420 | 10.66 × 9.5 mm | 1083, 1154, 1226 | 7.0 × 5.5 | med |
| R1 SHIFT ALPHA · MODE ON | cols 1, 2, 5, 6 | | 928 | 6.0 × 4.5 | med (rows from photo ratios: R1→R2 11.7 vs 11.5 mm, R2→R3 8.9 vs 9.0 mm) |
| R2 **Abs x³ · x⁻¹ log□** | cols 1, 2, 5, 6 | | 1016 | 6.0 × 4.5 | med |
| REPLAY ▲▼◀▶ | centre (150.0, 125.6) mm; ▲▼ ±5.3 mm, ◀▶ ±6.6 mm | | | 5.0 × 4.0 | **low → VERIFY V7** |

Each function pad's corners clear the nearest Ø 4.4 hole by about 0.5 mm. The real pad
shape (split, interleaved copper) and the matrix get set in stages 2 and 4. A first
proposal: the TCA8418 uses 8 rows × 9 columns following the physical rows, with N4 sharing
a row line with R2. ON goes to its own RTC wake pin.

**Firmware mismatch found.** `core/device.h` lists row 2 as `Inv, NCr, Pol, Cube`. The real
keys are **Abs, x³, x⁻¹, log□**. On this model nCr and Pol are SHIFT functions of ÷ and +.
`DKey` needs `Abs` and `LogBase` in place of `NCr` and `Pol`. The board uses the real
layout. The firmware fix is logged for a later stage.

## 5. Choices made where two readings were possible

| Item | Chosen | Alternative | Why |
|---|---|---|---|
| Scale | 7.55 px/mm (smaller board) | 7.40 (LR44, front = 161 mm) | Safer, and the screw-boss match supports it |
| Keypad edge | Casio's edge − 0.25 mm | Outer − 1.85 mm (0.9 mm narrower) | Casio's board is proven to fit, so a thicker assumed wall would only cost space |
| LR44 corner | Battery bay: board cut out, ribs trimmed, LiPo sits in it (Nirav, layout v2) | Battery behind the screen area | The bay gets the full case depth |
| Magnet connector | Top-wall slot (Nirav, 2026-10-02) | Battery door: rejected because the 21 × 7 mm face won't pass the ≈ 11 × 12.6 mm opening without cutting the case or making a bump | Datasheet vs scan |
| Solar zone | **Kept** (not the "smaller board" choice) | Cut the whole top strip to y ≈ 80 mm | The hard constraint puts the magnet header at the board's top edge. The back-cover solar box will probably touch here, so trim it flush (invisible from outside), or tell me to cut the strip and move the magnet connector. |
| Window | 57 × 21 mm (photo ratio) | 64 × 25 mm (older estimate) | The photo is direct evidence. The older number came from scaling the whole case. |

## 6. Carry-forward findings

1. **The e-paper active area (23.7 mm tall) is taller than the window (≈ 21 mm).** About
   2.7 mm, roughly 14 pixel rows, will be hidden. Firmware should keep content inside the
   visible band, and the window measurement should be checked when the case is open.
2. The e-paper outline is only about 1.1 mm wider than the window on each side, so
   placement tolerance is tight.
3. The board's back side behind number rows N1–N3 and the five pin spots must stay flat
   (ribs and pins press there).
4. Magnetic connector pin order gets set in stage 2 (the datasheet numbers pins 1–4 only), as a one-line change.

## 7. VERIFY list (lowest confidence first)

| ID | Item | What to look for on `geometry/overlay_front.png` |
|---|---|---|
| V2 | Solar zone vs back-cover box | Will the box need trimming? |
| V6 | Camera position and depth | Is there room behind the board at the "9C102A" spot? |
| V5 | Magnet slot | Is x = 136 (behind the solar cell) OK? Is there about 6 mm of height above the board there, or should I use an edge notch? |
| V8 | Window 57 × 21 | Measure when the case is open |
| V1 | Top-left screw boss | Screw confirmed; position only. Orange cross at top left. |
| V3 | Battery bay depth | Is there 3.8 mm plus clearance once the LR44 ribs are trimmed? (clay test / calipers) |
| V7 | REPLAY contact spots | Inside the pad |
| V4 | Top-section wall 1.35 mm | Cyan line vs the blue wall |
| V9 | E-paper FPC route (stage 5) | The panel's 14.3 mm FPC folds 180° behind the panel, runs back 7 mm, goes up through a 1.0 × 14 mm board slot at x = 172.9 and plugs into J2 on the F side. Measure your panel's FPC length (panel edge → tip) with calipers. Anything from 13.5 to 15 mm works. |
| V10 | Camera FPC pin 1 (stage 4) | Before the first plug-in, beep fingers 2 and 15 together (both GND). If they don't beep, beep 2 against 10: if that pair beeps, the cable is the other way round from my drawing reading (see stage4_5_layout.md). |
| V11 | Back-cover centre rib vs J1 | J1 (camera socket, 2 mm tall) sits on the centre line at y 157.5–162.5. Check that the back cover's vertical centre rib stops above or below it. |
| V12 | Height above the F side | Tallest parts: JST battery socket 5.5 mm (top left of the battery), magnet socket 5 mm + connector legs, camera module 5.4 mm. Measure the gap from board to back cover at those three spots. |

## Files

- `geometry/derive_geometry.py`: reproduces everything below from the scan image
- `geometry/board_outline.dxf`: mm, layers EDGE_CUTS, HOLES, OUTER_SHELL, WINDOW, EPAPER, CAMERA, MAGNET, KEYPADS, BACK_PINS
- `geometry/overlay_front.png`, `overlay_back.png`, `overlay_full.png`: the outline, holes and keys drawn on the scan; the back cover with mapped bosses and the camera drill point
- `geometry/board_mm.png`: clean mm drawing; `kicad_render_user_layers.png`: pcbnew render
- `geometry/features.json`, `geometry/keys.csv`: every number, machine-readable
- `kicad/ai_calc.kicad_pro/.kicad_pcb/.kicad_sch`: KiCad 10. Edge.Cuts, 12 NPTH holes from the project library `ai_calc.pretty`, 0.8 mm ENIG stackup. User layers: VERIFY, KEYPADS, KEEPOUTS, SHELL_REF. DRC: 0 violations.
