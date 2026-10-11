# 15. v15-LCD key pads vs the original Casio fx-115ES keyboard PCB

Date 2026-10-08. Report only: nothing in `kicad_v15_lcd/` or v14 was edited.

## Verdict

**ADJUST 2 PADS: SW1 (SHIFT) and SW50 (ON). SW2 (ALPHA) is optional. None of them blocks an order.**

The other 47 pads match the Casio board. Rows 3-9 (a/b ... =) are within **0.03-0.51 mm** (mean about 0.2 mm), and row 2 and the REPLAY pad are within 0.82 mm. The top row is the exception. On the Casio board the SHIFT/ALPHA/MODE/ON row is wider than ours (about 1 mm further out on each side), and the two outer keys sit about 1 mm higher. The largest offsets are **SW50 ON 1.54 mm** (dx -0.91, dy -1.23), **SW1 SHIFT 1.51 mm** (+1.15, -0.98) and **SW2 ALPHA 1.12 mm** (+1.10, +0.21). Two independent photos show this, the new board photo and the flat key-mat photo `6996fcbd`, so it is not fitting noise. Even at those offsets the carbon pill still sits 85-88 % on our comb, so today's pads would work. Moving them puts the contact back in the middle of each pad.

The count, the key order and the key map all match the Casio board and the firmware: 50 contacts, 4 REPLAY directions, and ON on its own pad.

**v14 = v15.** All 50 SW footprints (position, rotation 180, footprint, every finger pad and net) and all 10 post holes are identical in `kicad/ai_calc.kicad_pcb` and `kicad_v15_lcd/ai_calc_v15_lcd.kicad_pcb`. Any change below applies to both if v14 is ever re-spun.

## Changes to make (for the v15 board editor)

| Ref | Key | Now (x, y) mm | New (x, y) mm | Size | Notes |
|---|---|---|---|---|---|
| SW1 | SHIFT | 176.623, 120.795 | **177.75, 119.80** | keep 6.0 x 4.5 | Moving it up and away from H3 gives about 1.1 mm copper-to-hole (nearest pad corner 4.12 mm from the H3 centre at 174.0, 126.1). The `_H3notch` variant then isn't needed, but keeping it does no harm. Re-route the ROW0/COL0 stubs and re-run DRC (board edge, LCD-side parts). |
| SW50 | ON | 123.377, 120.795 | **122.45, 119.55** | keep 6.0 x 4.5 | Nearest pad corner is 4.76 mm from the H1 centre. Check the clearance to the board edge and any parts at the top-right (front view). Nets KEY_ON/GND. |
| SW2 | ALPHA | 166.026, 120.795 | 167.15, 120.80 (optional) | keep | x only. The pill already sits 100 % on the pad, so this only centres it. |
| SW3 | MODE | 133.974, 120.795 | no change | keep | Estimate -0.53, +0.25 with a spread of 0.48 between the three estimates, so it's inside the noise. |

For SW1, SW2 and SW50, move the courtyard/KEYPADS user-layer outline and the tracks to the TCA8418 with the pad. No pad size changes are needed anywhere.

## How it was measured

1. **Our pads.** A script read SW1-SW50 and the H holes from both `.kicad_pcb` files (regex on the footprint blocks only). It gave every finger pad (all `rect`, 180 deg), the pad bounding box, the centre and the ROW/COL nets.
2. **Casio contacts in the new photo** (`ec76c6e7`, 1932 x 2576, hand-held, key side up). I picked all **50** contacts by hand on 1.6-3x zoomed, gridded crops (about +-5 px, or +-0.33 mm). The automatic centroids were unreliable on the textured pads. The count is 5 in the top row (SHIFT, ALPHA, UP, MODE, ON), plus LEFT, RIGHT and DOWN of the ring, 4 in row 2, 3 x 6 in rows 3-5 and 4 x 5 number keys. The top 30 are round meander ("brain") interdigitated pads, about 6.2 mm across. The 20 number keys are radial cross/asterisk interdigitated pads, about 6.5 mm across. The ring has exactly 4 pads and no centre contact. ON is an ordinary round pad (top right, front view).
3. **Rectification.** The 10 Casio post holes visible on the board were matched to our holes: screw bosses H3/H1, H10/H8, H4, H9/H2, H6, H14/H13. Because the photo is the front view, KiCad x is mirrored. A homography fit through them has **RMS 0.40 mm**, leave-one-out 0.64 mm, and worst H3 0.86 mm. (Affine: RMS 0.48.) Scale is about 15.4 px/mm.
4. **Three estimates per pad.**
   - (a) Board photo through the hole homography. This is absolute, relative to the posts.
   - (b) Board photo fitted to our rows 3-9 only, with the top section extrapolated.
   - (c) Key-mat photo `6996fcbd` (flat on a table, contact side up). The 50 pill centroids were found automatically on the dark pills, then fitted to our rows 3-9.

   The table uses the **mean of (a)-(c)**. "Spread" is the largest distance from any one estimate to that mean.
5. **Pill size.** Taken from the mat photo through its homography: number rows (incl. 0 . x10^x Ans =) **4.9 mm**, function rows and top row **3.5-3.9 mm**, REPLAY **3.4-3.6 mm**. "Pill on copper" is the share of a pill of that diameter, centred at the Casio position, that falls inside our pad's copper bounding box.

**Uncertainty.** Rows 3-9 are inside the hole pattern and agree across all three estimates within 0.05-0.46 mm, so treat them as +-0.4 mm. The top row and the ring are 1-7 mm outside the hole pattern (extrapolated), so treat them as +-0.6-0.8 mm. The top-row result still holds because:
- both photos agree on its direction;
- it shows with any of the fits;
- you can see it directly in the rectified image: the Casio SHIFT and ON pads sit visibly outside our cyan outlines.

## Images
- `keypad_overlay.png`: the Casio photo rectified into our board frame (front view, KiCad x mirrored, mm ticks). Our pad copper is drawn in cyan, the Casio centres and pill circles in red, and our post holes in green (orange x = fitted Casio hole). Labels show ref, legend and offset in mm (magenta >= 0.75 mm).
- `keypad_ours_vs_casio.png`: our B.Cu pads (front view) with ROW/COL labels and the three Casio estimates (red + = board hole fit, yellow + = board grid fit, blue + = mat) plus the mean pill circle.

## Every pad

| Ref | Key | ROW/COL | Footprint | Our centre (x, y) | Photo px (x, y) | Hole fit dx, dy | Board grid dx, dy | Mat dx, dy | **Mean dx, dy** | **Offset** | Spread | Pill Ø | Pill on copper |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| SW1 | SHIFT | COL0/ROW0 | 6.0x4.5_H3notch | 176.623, 120.795 | 592, 947 | +1.23, -0.54 | +0.99, -0.85 | +1.23, -1.53 | +1.15, -0.98 | **1.51** | 0.56 | 3.8 | 88 % |
| SW2 | ALPHA | COL1/ROW0 | 6.0x4.5 | 166.026, 120.795 | 760, 965 | +1.28, +0.48 | +1.05, +0.15 | +0.96, +0.00 | +1.10, +0.21 | **1.12** | 0.33 | 3.9 | 100 % |
| SW3 | MODE | COL4/ROW0 | 6.0x4.5 | 133.974, 120.795 | 1286, 972 | -0.22, +0.62 | -0.49, +0.17 | -0.89, -0.04 | -0.53, +0.25 | 0.59 | 0.48 | 3.6 | 100 % |
| SW4 | CALC | COL6/ROW1 | 6.0x4.5 | 176.623, 132.450 | 606, 1138 | +0.29, -0.20 | +0.08, -0.41 | +0.17, -0.28 | +0.18, -0.30 | 0.35 | 0.15 | 3.8 | 100 % |
| SW5 | ∫dx | COL7/ROW1 | 6.0x4.5 | 166.026, 132.450 | 775, 1142 | +0.24, -0.01 | +0.05, -0.24 | +0.04, -0.31 | +0.11, -0.19 | 0.22 | 0.22 | 3.8 | 100 % |
| SW6 | x⁻¹ | COL8/ROW1 | 6.0x4.5 | 133.974, 132.450 | 1274, 1148 | +0.39, +0.15 | +0.20, -0.17 | +0.10, -0.26 | +0.23, -0.09 | 0.25 | 0.29 | 3.5 | 100 % |
| SW7 | logₐb | COL9/ROW1 | 6.0x4.5 | 123.377, 132.450 | 1450, 1150 | -0.49, +0.24 | -0.71, -0.10 | -0.09, -0.23 | -0.43, -0.03 | 0.43 | 0.39 | 3.5 | 100 % |
| SW8 | a/b | COL0/ROW1 | 6.0x4.5 | 176.623, 141.325 | 604, 1288 | +0.33, +0.38 | +0.16, +0.24 | +0.11, +0.18 | +0.20, +0.27 | 0.33 | 0.17 | 3.7 | 100 % |
| SW9 | √ | COL1/ROW1 | 6.0x4.5 | 166.026, 141.325 | 772, 1291 | +0.32, +0.55 | +0.17, +0.39 | -0.03, +0.17 | +0.15, +0.37 | 0.40 | 0.27 | 3.8 | 100 % |
| SW10 | x² | COL2/ROW1 | 6.0x4.5 | 155.298, 141.325 | 945, 1289 | +0.07, +0.42 | -0.06, +0.23 | +0.11, +0.06 | +0.04, +0.24 | 0.24 | 0.19 | 3.7 | 100 % |
| SW11 | x^n | COL3/ROW1 | 6.0x4.5 | 144.702, 141.325 | 1109, 1291 | +0.14, +0.52 | +0.01, +0.31 | +0.02, +0.09 | +0.06, +0.31 | 0.31 | 0.23 | 3.6 | 100 % |
| SW12 | log | COL4/ROW1 | 6.0x4.5 | 133.974, 141.325 | 1272, 1288 | +0.33, +0.27 | +0.20, +0.04 | +0.10, +0.21 | +0.21, +0.17 | 0.27 | 0.16 | 3.5 | 100 % |
| SW13 | ln | COL5/ROW1 | 6.0x4.5 | 123.377, 141.325 | 1438, 1289 | +0.14, +0.34 | -0.01, +0.08 | -0.13, +0.20 | +0.00, +0.21 | 0.21 | 0.19 | 3.5 | 100 % |
| SW14 | (−) | COL0/ROW2 | 6.0x4.5 | 176.623, 150.728 | 608, 1433 | +0.08, +0.22 | -0.06, +0.13 | +0.03, +0.01 | +0.02, +0.12 | 0.12 | 0.12 | 3.7 | 100 % |
| SW15 | °'" | COL1/ROW2 | 6.0x4.5 | 166.026, 150.728 | 771, 1431 | +0.33, +0.11 | +0.22, -0.00 | -0.07, -0.04 | +0.16, +0.02 | 0.16 | 0.24 | 3.7 | 100 % |
| SW16 | hyp | COL2/ROW2 | 6.0x4.5 | 155.298, 150.728 | 941, 1433 | +0.21, +0.25 | +0.13, +0.12 | +0.09, +0.00 | +0.15, +0.12 | 0.19 | 0.14 | 3.6 | 100 % |
| SW17 | sin | COL3/ROW2 | 6.0x4.5 | 144.702, 150.728 | 1109, 1431 | +0.01, +0.13 | -0.07, -0.02 | -0.02, +0.01 | -0.03, +0.04 | 0.05 | 0.09 | 3.6 | 100 % |
| SW18 | cos | COL4/ROW2 | 6.0x4.5 | 133.974, 150.728 | 1273, 1431 | +0.10, +0.14 | +0.03, -0.03 | +0.01, +0.02 | +0.05, +0.04 | 0.06 | 0.11 | 3.5 | 100 % |
| SW19 | tan | COL5/ROW2 | 6.0x4.5 | 123.377, 150.728 | 1438, 1430 | -0.05, +0.08 | -0.13, -0.10 | -0.13, +0.03 | -0.10, +0.00 | 0.10 | 0.11 | 3.5 | 100 % |
| SW20 | RCL | COL0/ROW3 | 6.0x4.5 | 176.623, 160.265 | 606, 1574 | +0.15, -0.32 | +0.03, -0.37 | -0.01, -0.31 | +0.06, -0.33 | 0.34 | 0.09 | 3.7 | 100 % |
| SW21 | ENG | COL1/ROW3 | 6.0x4.5 | 166.026, 160.265 | 772, 1574 | +0.16, -0.27 | +0.08, -0.34 | -0.05, -0.34 | +0.06, -0.32 | 0.32 | 0.12 | 3.6 | 100 % |
| SW22 | ( | COL2/ROW3 | 6.0x4.5 | 155.298, 160.265 | 941, 1571 | +0.11, -0.43 | +0.07, -0.51 | +0.04, -0.31 | +0.07, -0.42 | 0.42 | 0.11 | 3.6 | 100 % |
| SW23 | ) | COL3/ROW3 | 6.0x4.5 | 144.702, 160.265 | 1111, 1571 | -0.26, -0.38 | -0.27, -0.48 | -0.04, -0.33 | -0.19, -0.40 | 0.44 | 0.16 | 3.5 | 100 % |
| SW24 | S⇔D | COL4/ROW3 | 6.0x4.5 | 133.974, 160.265 | 1273, 1571 | -0.06, -0.34 | -0.07, -0.45 | -0.02, -0.32 | -0.05, -0.37 | 0.37 | 0.08 | 3.5 | 100 % |
| SW25 | M+ | COL5/ROW3 | 6.0x4.5 | 123.377, 160.265 | 1438, 1571 | -0.24, -0.29 | -0.25, -0.42 | -0.13, -0.30 | -0.21, -0.34 | 0.39 | 0.09 | 3.5 | 100 % |
| SW26 | 7 | COL0/ROW4 | 9.0x7.0 | 175.695, 170.728 | 618, 1740 | +0.23, -0.10 | +0.15, -0.13 | +0.05, +0.07 | +0.14, -0.05 | 0.15 | 0.15 | 4.9 | 100 % |
| SW27 | 8 | COL1/ROW4 | 9.0x7.0 | 162.848, 170.728 | 831, 1741 | -0.48, +0.08 | -0.49, +0.04 | +0.03, +0.07 | -0.31, +0.06 | 0.32 | 0.34 | 4.9 | 100 % |
| SW28 | 9 | COL2/ROW4 | 9.0x7.0 | 150.000, 170.728 | 1024, 1740 | -0.08, +0.12 | -0.05, +0.07 | -0.00, +0.06 | -0.04, +0.08 | 0.09 | 0.05 | 4.9 | 100 % |
| SW29 | DEL | COL3/ROW4 | 9.0x7.0 | 137.152, 170.728 | 1222, 1738 | -0.10, +0.10 | -0.03, +0.03 | +0.01, +0.07 | -0.04, +0.07 | 0.08 | 0.07 | 4.9 | 100 % |
| SW30 | AC | COL4/ROW4 | 9.0x7.0 | 124.305, 170.728 | 1412, 1740 | +0.25, +0.34 | +0.33, +0.26 | -0.08, +0.05 | +0.16, +0.22 | 0.27 | 0.30 | 4.9 | 100 % |
| SW31 | 4 | COL0/ROW5 | 9.0x7.0 | 175.695, 181.854 | 618, 1913 | +0.16, -0.04 | +0.12, -0.07 | +0.02, +0.12 | +0.10, +0.01 | 0.10 | 0.14 | 4.9 | 100 % |
| SW32 | 5 | COL1/ROW5 | 9.0x7.0 | 162.848, 181.854 | 826, 1912 | -0.26, +0.06 | -0.23, +0.03 | -0.02, +0.11 | -0.17, +0.07 | 0.18 | 0.16 | 4.9 | 100 % |
| SW33 | 6 | COL2/ROW5 | 9.0x7.0 | 150.000, 181.854 | 1031, 1915 | -0.66, +0.43 | -0.56, +0.39 | +0.01, +0.10 | -0.40, +0.30 | 0.51 | 0.46 | 4.9 | 100 % |
| SW34 | × | COL3/ROW5 | 9.0x7.0 | 137.152, 181.854 | 1222, 1908 | -0.28, +0.09 | -0.15, +0.05 | +0.04, +0.09 | -0.13, +0.08 | 0.15 | 0.17 | 4.9 | 100 % |
| SW35 | ÷ | COL4/ROW5 | 9.0x7.0 | 124.305, 181.854 | 1414, 1908 | -0.07, +0.25 | +0.09, +0.21 | -0.02, +0.09 | -0.00, +0.18 | 0.18 | 0.10 | 4.9 | 100 % |
| SW36 | 1 | COL0/ROW6 | 9.0x7.0 | 175.695, 192.980 | 620, 2088 | -0.03, +0.19 | -0.04, +0.14 | -0.07, +0.05 | -0.05, +0.13 | 0.14 | 0.08 | 4.9 | 100 % |
| SW37 | 2 | COL1/ROW6 | 9.0x7.0 | 162.848, 192.980 | 821, 2083 | -0.08, +0.11 | +0.00, +0.07 | -0.05, +0.05 | -0.04, +0.08 | 0.09 | 0.05 | 4.9 | 100 % |
| SW38 | 3 | COL2/ROW6 | 9.0x7.0 | 150.000, 192.980 | 1020, 2080 | -0.11, +0.13 | +0.04, +0.09 | +0.02, +0.05 | -0.02, +0.09 | 0.09 | 0.11 | 4.9 | 100 % |
| SW39 | + | COL3/ROW6 | 9.0x7.0 | 137.152, 192.980 | 1216, 2077 | -0.05, +0.15 | +0.16, +0.11 | +0.09, +0.02 | +0.06, +0.09 | 0.11 | 0.13 | 4.9 | 100 % |
| SW40 | − | COL4/ROW6 | 9.0x7.0 | 124.305, 192.980 | 1412, 2074 | -0.20, +0.17 | +0.05, +0.13 | +0.06, +0.04 | -0.03, +0.11 | 0.12 | 0.18 | 4.9 | 100 % |
| SW41 | 0 | COL0/ROW7 | 6.0x4.5 | 175.695, 204.106 | 620, 2255 | -0.10, +0.03 | -0.07, -0.06 | -0.15, -0.01 | -0.11, -0.01 | 0.11 | 0.06 | 5.0 | 97 % |
| SW42 | . | COL1/ROW7 | 6.0x4.5 | 162.848, 204.106 | 818, 2250 | +0.01, -0.03 | +0.14, -0.11 | -0.09, -0.01 | +0.02, -0.05 | 0.05 | 0.13 | 4.9 | 97 % |
| SW43 | ×10^x | COL2/ROW7 | 6.0x4.5 | 150.000, 204.106 | 1018, 2248 | -0.13, +0.07 | +0.08, +0.01 | +0.01, -0.01 | -0.01, +0.02 | 0.03 | 0.13 | 4.9 | 97 % |
| SW44 | Ans | COL3/ROW7 | 6.0x4.5 | 137.152, 204.106 | 1216, 2242 | -0.24, +0.01 | +0.05, -0.05 | +0.10, -0.03 | -0.03, -0.02 | 0.04 | 0.21 | 4.9 | 97 % |
| SW45 | = | COL4/ROW7 | 6.0x4.5 | 124.305, 204.106 | 1408, 2239 | -0.09, +0.04 | +0.24, -0.01 | +0.17, -0.05 | +0.11, -0.00 | 0.11 | 0.20 | 5.0 | 97 % |
| SW46 | UP | COL6/ROW0 | 5.0x4.0 | 150.000, 119.000 | 1028, 926 | +0.33, -0.37 | +0.07, -0.79 | -0.07, -1.00 | +0.11, -0.72 | 0.72 | 0.41 | 3.5 | 92 % |
| SW47 | DOWN | COL7/ROW0 | 5.0x4.0 | 150.000, 131.900 | 1028, 1135 | +0.21, -0.00 | +0.03, -0.28 | -0.00, -0.56 | +0.08, -0.28 | 0.29 | 0.31 | 3.4 | 100 % |
| SW48 | LEFT | COL8/ROW0 | 5.0x4.0 | 156.623, 125.563 | 925, 1030 | +0.21, -0.29 | -0.00, -0.61 | -0.13, -0.90 | +0.02, -0.60 | 0.60 | 0.36 | 3.6 | 94 % |
| SW49 | RIGHT | COL9/ROW0 | 5.0x4.0 | 143.377, 125.563 | 1130, 1028 | +0.37, -0.56 | +0.14, -0.93 | +0.05, -0.92 | +0.19, -0.80 | **0.82** | 0.30 | 3.4 | 91 % |
| SW50 | ON | GND/KEY_ON | 6.0x4.5 | 123.377, 120.795 | 1456, 951 | -0.61, -0.87 | -0.93, -1.37 | -1.20, -1.46 | -0.91, -1.23 | **1.54** | 0.47 | 3.5 | 85 % |

Signs: dx/dy are Casio minus ours, in KiCad mm (KiCad top view, so +x is toward the SHIFT side and -y is up toward the LCD).

## REPLAY ring and ON
- **Ring:** 4 pads on both boards (UP SW46, DOWN SW47, LEFT SW48, RIGHT SW49), no centre contact. The Casio ring comes out 0.3-0.8 mm higher (-y) than ours. The absolute hole fit says 0.0-0.56 mm, and the extrapolated fits say 0.3-1.0 mm. The pill (3.4-3.6 mm) still sits 91-100 % on our 5.0 x 4.0 pads. **No change.** Stage 10 already moved UP/DOWN outward by 1.2/1.0 mm, and that holds: the ring's vertical span is now right to within about 0.4 mm.
- **ON (SW50):** a plain pad on the Casio board in the same place as ours, top right in the front view. It's on its own KEY_ON/GND pair here, not in the matrix, which is correct for the wake pin. Only its position is off (see Changes).

## Shape: our rectangular combs vs Casio's round/cross pads
- **How the contact works.** The conductive carbon pill closes the key when it lies across at least one gap between a finger of comb A and a finger of comb B. Our fingers are 0.45 mm wide with 0.30 mm gaps (0.75 mm pitch). A 3.5 mm pill therefore crosses **4-5 gaps**, each a chord of up to about 3 mm (roughly 10-15 mm of total bridged gap). A 4.9 mm pill crosses 6-7. That keeps the resistance far below what the TCA8418 needs (internal pull-ups of about 100 kohm, against tens to hundreds of ohms for the carbon). Gap count and pad size matter. The outline shape does not.
- **Why Casio uses round/cross shapes.** The pills are round, so a round pad of about 6.2-6.5 mm wastes no area and is orientation-free. Casio also routes its traces between pads on a single-sided board, where compact round pads help. The radial cross on the number keys gives gaps in every direction, so a pill that lands off-centre (or tilts on a wide key) still bridges.
- **Ours is at least as tolerant.** Our pads are bigger than Casio's in the direction that matters: 9 x 7 against about 6.5 mm on the number keys, 6 x 4.5 against about 6.2 mm on the function keys. The pill fits with 1.0-2.0 mm of slack in x. The weak direction is y on the 6.0 x 4.5 pads (only 0.3-0.5 mm of slack for a 3.8 mm pill). Partial overlap still works, because the fingers run vertically, so a pill hanging off the top or bottom still crosses every finger it overlaps.
- **Copying Casio's exact shapes would be riskier.** It means custom arc/polygon footprints, re-checking DRC around every finger, re-routing 50 pads, and getting the meander gaps right (Casio's look about 0.3-0.4 mm, so no better than ours). There's no electrical gain, and each change risks DRC/net mistakes.
- **Recommendation:** keep the rectangular interdigitated footprints and ENIG (already specified; HASL is too bumpy). Only fix positions (SW1/SW50, optionally SW2). If space ever allows, the bottom row (SW41-45, 6.0 x 4.5 under a 4.9 mm pill: 97 % on copper) could grow to about 6.5 x 5.5. It works as is. It was shrunk on purpose in stage 4/5 for post clearance.

## Key mapping vs the firmware
I checked the board's ROW/COL nets for every SW against `firmware-v15-lcd/src/keys.cpp` `kMatrix[8][10]`, with the expected legend coming from the pad's physical place on the fx-115ES layout. The result is **0 mismatches, 49 unique matrix crossings, plus ON on KEY_ON/GND**:
- ROW0: SHIFT C0, ALPHA C1, MODE C4, UP C6, DOWN C7, LEFT C8, RIGHT C9
- ROW1: a/b, sqrt, x^2, x^n, log, ln (C0-C5); CALC, integral, x^-1, log_a b (C6-C9)
- ROW2: (-), deg-min-sec, hyp, sin, cos, tan
- ROW3: RCL, ENG, (, ), S<>D, M+
- ROW4-7: 7 8 9 DEL AC / 4 5 6 x / / 1 2 3 + - / 0 . x10^x Ans =

The physical order (front-left = larger KiCad x) matches the Casio board in the photo: SHIFT is top-left, ON is top-right, and LEFT sits on the SHIFT side of the ring.

One small doc nit: the comment above `kMatrix` still cites `hardware/kicad/keypad.kicad_sch, SW1-SW49` (the v14 path). The nets are the same in v15.

## Other observations (no action requested)
- The Casio holes are smaller than ours. The drilled hole looks about 4.2 mm, inside an 8.6 mm mask-free ring. Fitted, the Casio H3 lands at about (173.2, 125.7), 0.86 mm from ours. That's inside our 6 mm hole's play, and the mat photos had put it at 174.0, so leave H3 as it is.
- **To confirm the top row before moving SW1/SW50**, either photo would settle it to about +-0.2 mm:
  - lay the Casio board **flat on a table, key side up, with a ruler beside it**, and photograph it straight down from about 40 cm, with no hand and the LCD flex flat;
  - or put it through the flatbed scanner.

  The K-F2 print-overlay test in `production/KNOCKOFF_SHELL_PLAN.md` will also show it.
