# Stage 12: second caliper batch, new photos, CAD fit check (ai_calc_pcb_12)

> **⚠ History (stage 12).** "Still don't order" below is resolved: the J4/J3 height problems were solved in stage 13/13b, and the battery now lies on the back-cover floor (the LR44 cup is not ground). The camera window is **7 mm** (stage 13), not the 6 mm below; the battery is the #1317 150 mAh. Current status: `FINAL_STATUS.md`.

**Result:** `kicad-cli pcb drc --refill-zones --schematic-parity` = **0 errors, 0 warnings, 0 unconnected, 0 parity issues**. ERC = 0. Report: `kicad/ai_calc_drc_violations.json`.

**Still don't order.** Two height problems at the top of the board (J4 battery socket and the J3 magnet socket) can't be settled from the readings we have. See "Clearance problems" and QUESTIONS_AND_ISSUES.md → "PCB (stage 12)".

Inputs: `measurements_2026-10-03.md` (C1–C15, D1–D13, P3), four new photos (2026-10-03 19:22: `2a97120d` front shell with mat, straight on; `fd923128` Casio board; `e59eff71` front with calipers; `fb0c138c` Waveshare HAT), the 23 older photos, the CAD replica fit check (`enclosure/replica/fitcheck_report.md`), and the Waveshare 2.13" V4 spec (mechanical drawing, page 6).

## 1. What changed on the board, and why

| # | Change | Why (plain English) |
|---|---|---|
| 1 | **Left edge of the screen section pulled in to x = 118.9** (from 113.84–114.6) between y 82.9 and 114.0, then a slant back to the old edge at y 119.6. | The "4.7 mm wall" (C4) is an outer skin + a wire channel + a thin inner rib, with 3 pins (C14) and 2 wire hooks sticking in. From the straight photo (key-fitted, RMS 0.37 mm) the pins reach x ≈ 116.8–117.4 and the top hook x ≈ 118.2. The board now stops 0.6–2.1 mm clear of them. |
| 2 | **Right edge pulled in to x = 183.65** (from 185.4–186.16) between y 78.3 and 116, then a slant to the old edge at y 119.2. | Same wall on the right: pins at x ≈ 184.0–184.2 (C14 5.45 from the outside puts them at 184.2), hook at 182.5 / 184.75. The board now stops 0.35–0.55 mm short of the pins. The bottom-right hook (184.75, 114.3) is outside the board. The top-right hook (182.5, 84.5) is 1.2 mm inside the board edge: it sits next to the e-paper socket J2 area with no parts there, but it touches the board face. **Snip that one hook (and the top-left one if it touches) with side cutters**; they only held Casio's wires. |
| 3 | J2's lower fan-out tracks (EPD_PREVGH/VSH/VPP/VDD) squeezed from 0.5 mm to 0.4 mm pitch. | That freed 0.5 mm so the right edge could move in. Same connections, clearances still ≥ 0.2 mm. |
| 4 | **ESP32 (U1) antenna now overhangs a board cut-out.** The board ends at x = 118.9; the module's antenna part runs from 114.75 to 119.85, so ~4 mm of the antenna hangs past the board edge. | This is Espressif's preferred antenna placement (antenna off the board edge, no copper under it). The module is still held by all its pads (the first pad column is at x 120.55). The old "antenna edge strip" rule area was removed (no board there now); the top/bottom antenna keep-outs were trimmed to the new edge. The U1 silkscreen box was cut at the edge (library footprint updated to match). |
| 5 | **H1 (left screw post, row 1/2) moved 0.86 mm: (128.36, 126.74) → (127.5, 126.1).** | C8 says this pair is 46.5 apart centre to centre; the board had 45.64. Now H1–H3 = 46.50. Both photos put the post at x 127.5–127.6. The old hole left only ~0.2 mm of play. KEYPAD_INT, I2C_SCL and a GND track were re-routed around it, and the SCL via moved 0.2 mm; two GND stitching vias under the new hole were removed. |
| 6 | **H8 → (128.85, 137.0), H10 → (171.25, 136.8)** (moved 0.24 / 0.28 mm inward). | C8 says this pair is 42.3 apart; the board had 42.85. Now 42.40. They couldn't go further without cutting COL1/COL4 (hole-to-copper must stay ≥ 0.25 mm). With 4.2 mm holes and 2.9 mm posts that still leaves ≥ 0.6 mm of play. |
| 7 | The other locating holes (H2, H4, H6, H9, H13, H14) and H3 **not moved**. | The new straight photo agrees with the stage-10 photo positions to 0.2–0.5 mm, with mixed signs (scatter, not a shift). C15 (see §3) disagrees with the photos by up to ±1 mm, but with opposite signs on neighbouring rows, which points at reading error, not at the holes. I tried widening H9/H6 into slots, but key-pad copper is too close (0.3 mm edge rule). |
| 8 | **Top corners kept as they are.** | C9 (68.1 apart) and C11 put the corner screw posts at ≈ (115.9 / 184.0, 65), which is outside the board in the right corner (board's top edge there is at y 78.3, battery cut-out) and ≥ 1.5 mm clear of the top-left cut-out. "Nowhere near the board" is right. Filling the top-left cut-out back in wouldn't add useful space (the LR44 cup and solar box are there), so I left it. |
| 9 | **E-paper panel drawing 59.2 → 59.0 mm** (P3), and the active area redrawn from the Waveshare V4 drawing. | See §4. No copper change needed. |
| 10 | **3D models attached for U1 (ESP32-S3-MINI-1) and U6 (TCA8418)**, both stored in the project (`ai_calc.3dshapes/`). | The CAD fit check had to use guessed boxes. U1's model came from LCSC/EasyEDA and was offset so its body lines up with the pads (checked: body 114.75–135.25 × 89.3–104.7, 2.4 mm tall). |
| 11 | **3 fiducials** FID1 (121.5, 203.5), FID2 (178.5, 203.5), FID3 (177.0, 112.5): 1 mm copper, 2 mm mask opening, F side. | JLCPCB adds its own tooling, but fiducials help the pick-and-place camera with the 0.5 mm-pitch parts (ESP32, TCA8418, FPC sockets). Free. They are not in the BOM or CPL. |
| 12 | Silkscreen: "AI CALC v12 2026-10-03" (moved off H10), test-point names (BOOT, GND, 3V3, BAT, EN, TX, RX), and "+"/"−" beside the battery socket J4. | For first power-up with a meter. No silk on any key pad or exposed pad (DRC checks this). |
| 13 | `tools/make_fitcheck.py` rewritten: part heights now come from the 3D models; new `fitcheck/clearance_table.md`; grind maps mark the features to grind in red. `fitcheck/backcover_fx115es.json` gained the solar box, a heights table and the free-height cases. | Task 3. |

Things deliberately left alone: J1 CamReversed footprint, the stage-6 camera GPIO map, no LED, magnet contacts never on battery voltage, R20 → VBUS_SENSE, C32, TP5–TP7, 2-layer 0.8 mm ENIG.

## 2. The screen-section width (was blocking)

Side view across the screen section (left wall, looking from the top of the calculator; not to scale):

```
 outside                                          inside
 |<-1.2->|<--- ~2.5 wire channel --->|<1.0>|
 +-------+                           +-----+                 back cover (D5 ~1)
 |       |                           |     |    =========================== <- inside of back cover
 | outer |                           |inner|      free ~4.0-4.6 (see 3)
 | skin  |                           | rib |  ...........................
 |       |                           |     |  ######## board 0.8 ########  <- board F side faces up here
 |       |      pins (C14) stick in  |     o====> 5.45 from outside, top 5.0 above floor
 |       |      5.45 from the outside|     |  ~~~~~~~~ key mat ~~~~~~~~~~
 |       |                           |     |      board top face 5.5 above the front floor (D1)
 +-------+---------------------------+-----+==============================  front floor (window side)
 x=111.5         (KiCad x)              114.3-114.8     pins reach 116.8-117.4 (L) / 184.0-184.2 (R)
```

- The pins are 5.0 above the floor and the board occupies 4.7–5.5 above the floor, so the pins sit right at board level: the board edge must not reach them. The inner rib runs from the floor up to the parting line, so it also blocks the board.
- Free width at board height, screen section, by y (KiCad x): pins and hooks on the left at 116.8–118.3, on the right at 182.5–184.2. The board is now **x 118.9 → 183.65 (64.75 wide)** there. Clearance: left ≥ 0.6 mm to the hook, ≥ 1.5 mm to the pins; right 0.35–0.55 mm to the pins, and the one top-right hook needs snipping.
- Below y ≈ 116 (keypad section) the walls are 0.8 mm (C5) and the board is unchanged (C6: our keypad section is narrower than Casio's board).

## 3. Heights: what's behind the board (F side faces the back cover)

**Stack-up.** D10 says the closed calculator is 11.3 (keypad) / 11.75 (top part). The front face is about 1.5 thick at the window, the board's top face is 5.5 above the front floor (D1), the board is 0.8, the back cover about 1.0 (D5):

D1 is measured from the front floor to the board's **top face**, which is its F side (the board lies key side down). So the free height above the F side is about 11.75 − 1.5 (face) − 5.5 (D1) − 1.0 (back cover) = **≈ 3.75–4.25**. D4 cross-check: the LCD module (3.75, D8) sits on the floor, so its back is 1.75 below the board line, and D4's 5.5 would then leave ≈ 3.75 above the board. The CAD replica (which reads D1/D4 to the outside faces) gets 4.59. D4 read literally says 5.5. I used **4.5 as the design value, 4.0 pessimistic and 5.5 optimistic**, and checked all three.

```
 back cover outside  ----------------------------------------  Z 0
 back cover floor 1.0
 ribs 1 / rings 4 / solar box 5-5.5 (D13, D6) hang down from here
 free height above board F side: 4.0 .. 4.6 .. 5.5   <-- the open question
 board F side (parts point up, towards the back cover)
 board 0.8
 key mat + keys, 5.5 down to the front floor (D1)
 front face ~1.5
 front outside ----------------------------------------------  Z 11.3 / 11.75
```

The full table for all 82 F-side parts is `fitcheck/clearance_table.md`. Summary:

| Part (height from 3D model) | Over it | Margin at 4.5 | After grinding | What I did |
|---|---|---|---|---|
| **J4** battery socket S2B-PH-SM4-TB, **5.50** | solar box (5–5.5) | −6.5 | **−1.0** (−0.0 at 5.5) | **Not solved. Blocks order.** No JST-PH SMD socket on JLCPCB is lower than 5.45. Options in Q12-1. A 3.6 mm MX1.25 socket (C7430468, in stock) is ready in the library (`ai_calc:MX1.25_2P_SMD_RA_H3.6_C7430468`) but needs a battery with a 1.25 mm plug. |
| **J3** magnet socket, **5.00** + magnet legs | solar box | −6.0 | **−0.5** | **Not solved. Blocks order.** The magnet connector isn't in hand (P5); its legs plug into J3 from above, so the magnet body sits even higher. Needs P5 + D3/D11 measured on the real part. |
| U2, U3, D1, Q2, D7, C5, C6… (≤ 1.6) | solar box | −1.5 to −2.6 | **+2.9 or more** | Grind the solar box flat (D6: "I can grind it"). Marked red on the grind map. |
| **D2** (1.26), **C16** (0.55) | big ring wall (4) | −0.76 / +0.0 | **+1.2** | Grind the arc of the big ring next to D2/C16 down to 2 mm (marked). Moving D2 would need re-routing the camera bus; not worth it. |
| U1 ESP32 2.41 | upper rib B (1) | +1.09 | — | OK (+0.59 even at 4.0). |
| L1 2.0, J1 2.0, J2 2.0 | 1 mm ribs | +1.5 | — | OK. |
| C20 1.59, all other passives | ribs or nothing | ≥ +1.9 | — | OK. |
| **Camera module** 8.5 × 8.5 × 5.4 (not soldered) | rib B through the camera spot | 5.4 vs 4.5 | — | The lens has to poke into the 6 mm back-cover hole (cover 1.0 thick): body (without lens) must be ≤ ~4 mm and the barrel ≤ 6 mm across. Rib B must be ground where it crosses the camera. Check the split on the real module (F3). |
| LiPo 3.8 (in the bay, under board level) | LR44 cup (5–6) | — | — | The LR44 cup ribs in the front shell must be cut out (D7, grindable), as planned since stage 3. |

## 4. E-paper (P3: 59.0 × 29.2)

- Waveshare V4 drawing: outline 59.2 ± 0.2 × 29.2 ± 0.2 × 1.0, active area 48.55 × 23.70. Your 59.0 is within tolerance. I redrew the panel 59.0 long, keeping the FPC end (right end, x 179.47) fixed, so the ribbon/slot/J2 geometry is unchanged.
- **Active area is not centred on the glass**: 2.70 mm from the non-FPC end, ~7.95 mm from the FPC end, 2.75 mm top and bottom. Earlier stages assumed it was centred. On our board the picture is therefore centred at x ≈ 147.4, **2.6 mm left of the window centre** (in the view from the front it's toward the FPC side, i.e. the right-hand side of the window as you look at the calculator — mirror if unsure). It's fully inside the window (3.5 mm margin one side, 8.6 the other). It's cosmetic; firmware can shift the drawing area, or the e-paper block could move 2.6 mm in a later revision.
- Vertical: active area centre y 92.99 vs window 93.10 (0.1 mm). The new straight photos put the window centre at 92.75–93.36 and its left-right centre at 150.0 ± 0.3, so Q4 from stage 11 (window centred) is confirmed.
- FPC slot budget unchanged: ribbon 14.30 ± 0.3 by the drawing, path needs 13.5–15. **E7 still not measured** (Nirav can measure it on the panel in the HAT, photo `fb0c138c`).

## 5. C15 and residuals (hole positions)

C15 was read "from the outside of the calculator to the outside of the holes". I fitted the three possible conventions (to the hole centre, its far edge, its near edge) against the photo-derived hole rows, solving for the top reference each time:

| Convention | Top edge at KiCad y | Residuals corner / H1-H3 / H4 / H2-H9 / H6 / H13-H14 / bottom (mm) | RMS |
|---|---|---|---|
| centre | 55.97 | −0.13 / −0.18 / −0.39 / +0.82 / −1.03 / +0.32 / +0.47 | 0.61 |
| far edge | 57.58 | −0.45 / −0.50 / −0.23 / +0.98 / −0.87 / +0.48 / +0.15 | 0.62 |
| near edge | 54.36 | +0.19 / +0.14 / −0.55 / +0.66 / −1.19 / +0.16 / +0.78 | 0.68 |

The conventions can't be told apart (0.6 mm RMS each). The two big residuals (H2/H9 +0.9, H6 −1.0) have opposite signs only 11 mm apart, while three independent photo fits agree with each other to ≤ 0.5 mm, so I trust the photos there. The CAD replica used the far-edge reading and reports H2 rubbing by 0.54; with the photo positions every locating hole has ≥ 0.4 mm of play.

Final hole residuals vs the new straight photo `2a97120d` (board − photo, x / y, mm): H1 +0.01/+0.05, H3 +1.0 (post tip) / +1.4 (mat hole) in x, H8 +0.02/−0.40, H10 +0.05/−0.21, H4 −0.14/−0.64, H2 +0.30/−0.40, H9 +0.22/−0.29, H6 +0.19/−0.53, H13 +0.11/−0.27, H14 +0.10/−0.56. The y values share a −0.4 offset (the photo's fit is shifted, not the holes). H3 was confirmed at 174.0 by the stage-10 mat photo, C8 (46.5 from H1) and the shell photo; the new photo's top row fit is worse (0.7 mm residuals on the 4 top keys), so I kept H3. Its 6 mm hole gives 1.07 mm of play for the 3.85 post, which covers the new photo's tip reading but not its mat-hole reading: dry-fit check Q12-5.

## 6. CAD fit-check collisions (fitcheck_report.md, STEP of 18:10) and what was done

| # (report) | Collision | Done |
|---|---|---|
| 1, 10, 22–28 | J4 into back floor / solar box | Open, blocks order (Q12-1). Solar box marked GRIND. |
| 2 | LiPo vs solar box grid | Battery sits in the bay below board level; grind the solar box flat (it's over the bay too). |
| 3, 7, 29 | J3 vs solar box / floor | Open, blocks order (Q12-2). |
| 4 | Camera vs rib B and floor | Grind rib B over the camera; lens into the 6 mm hole; body/lens split to confirm (Q12-3). |
| 5, 6, 8, 9, 19 | D1, Q2, D7, U3, C6 vs solar box | Grind the solar box flat: then ≥ 2.9 mm margin. |
| 11 | Board vs post H13 | The CAD uses C15 rows; photos put H13 within 0.3 mm. Not moved (§5). |
| 12, 16, 15 | Board vs H6, H2, H9 | Same; ≥ 0.4 mm play against the photo posts. H2/H9/H6 rows flagged for a quick dry fit (Q12-5). |
| 13 | Board vs mid-R screw post (H1) | **Fixed**: H1 moved to 46.5 spacing. |
| 14, 17 | Board vs screen-section inner ribs | **Fixed**: edges at 118.9 / 183.65. |
| 18 | Board vs solar box (board face) | Grind the solar box flat. |
| 20, 21 | LiPo vs LR44 cup | Cut the LR44 cup out of the front shell (D7, already in the plan). |
| 2D | stubs / wire holders −0.23 to −0.73 | **Fixed** by the new edges, except the top-right hook at (182.5, 84.5): snip it. |
| — | U1/U6 had no 3D model | **Fixed**: models attached. New STEP: `fab/ai_calc_board.step` (2026-10-03 23:02). |

## 7. Parts and cost (checked live on JLCPCB's parts API, 2026-10-03)

- **Regulator:** RT9080-33GJ5 **C841192 is an Extended part, 39,625 in stock, $0.120**. AP2112K-3.3 **C51118 is also Extended, 38,405 in stock, $0.171**. Same $3 loading fee either way, the RT9080 is cheaper and gives 4–6 months standby instead of ~6 weeks. **Kept RT9080.**
- All 31 LCSC numbers are in stock. Lowest: ESP32-S3-MINI-1-N4R2 1,893, L1 5,250, TCA8418 28k; all far above the 2–5 needed.
- 14 Extended part types (ESP32, TCA8418, MCP73831, RT9080, 2 × ME6211, USBLC6, SI1308, SMF5.0A, L1, R12 3 Ω, FPC socket, J3, J4) ≈ $42 in loading fees. I searched for Basic substitutes with the same footprint: none for the ICs/FET/TVS/inductor. The only candidate is **R12 3 Ω → 2.2 Ω C22939 (Basic)**, saving $3, but R12 sets the e-paper booster's current limit and Waveshare's reference uses 3 Ω (sheet item E5 not measured), so I didn't change it.
- Not changed: fixed main ICs and connectors.

## 8. Robustness checks

- **Polarity / pin 1:** every D/U/Q/J part has a pin-1 or cathode mark in its footprint silk (checked by script). J4 now has "+"/"−". D7: cathode bar on the VBUS pad (stage 11 check still holds).
- **Fiducials:** 3 added (change 11).
- **Version silk:** "AI CALC v12 2026-10-03", F side, on bare board between H1 and H3 rows, no pads under it.
- **Test points:** TP1 BOOT, TP2 TX, TP3 RX, TP4 GND, TP5 3V3, TP6 BAT, TP7 EN, all F side, all labelled, all in the screen section (reach them with the back cover off; nothing tall within 2 mm of any of them except TP6 beside J4's corner).
- **Net widths** (2-layer, 1 oz, 10 °C rise, IPC-2221 outer): BAT+, VBAT_P, SYS, VBUS = 0.5 mm (≈ 1.2 A) for ≤ 0.5 A peaks (Wi-Fi TX + charging); +3V3 trunk 0.5 mm, branches 0.25 mm (≈ 0.75 A) feeding single ICs; camera rails 0.25 mm for ≤ 0.15 A; GND is two pours. OK.
- **USB pair:** D+ 48.1 / D− 45.9 mm, 0.2 mm tracks, through the USBLC6. A 2.2 mm mismatch is irrelevant at 12 Mbit/s.
- **Thermal/DFM:** no starved thermals, smallest track 0.2 / gap 0.15 / via 0.6/0.3 (JLCPCB minimums 0.1 / 0.1 / 0.45/0.2). The LDOs dissipate < 0.2 W. GND islands: every pour island ≥ 1 mm² has ≥ 2 vias (`tools/gnd_islands.py`, 0 added).
