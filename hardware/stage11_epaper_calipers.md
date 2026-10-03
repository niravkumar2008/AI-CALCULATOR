# Stage 11: e-paper moved to the measured window, calipers applied, review fixes (ai_calc_pcb_11)

**Result:** `kicad-cli pcb drc --refill-zones --schematic-parity` gives **0 errors, 0 warnings, 0 unconnected, 0 schematic-parity issues**. ERC is 0/0. The report is in `kicad/ai_calc_drc_violations.json`.

**Still don't order.** Question 1 below (the wall thickness beside the screen) could mean the top part of the board is about 2 mm too wide on each side.

## What changed, and why
| # | Change | Why (plain English) |
|---|---|---|
| 1 | **E-paper block moved up 3.57 mm** (J2, slot, C19–C30, D3–D5, L1, Q3, R11, R12). This was started in the last session; this stage finished it. | Your C12/C13 readings put the window's top edge 24.0 mm below the case top, so the window runs from y 80.95 to 105.25 (centre 93.1). The panel's active area is now centred at y 92.99, 0.1 mm off the window centre. |
| 2 | **E-paper block completely re-routed.** J2 fans out on F.Cu in two tidy groups: pins 2–5 go over the top of J2, pins 18–24 go under it. The six MCU signals drop through vias beside the pins and run on B.Cu around the slot to the ESP32. | The half-finished routing had a short (GND to +3V3 at C19/L1), 5 solder-mask bridges and 3 missing wires. It also looped long tracks around J2 in 0.05 mm steps. The new routing is shorter and every track is straight. |
| 3 | R11 and R12 swapped places around Q3: R12 (3 Ω sense) now sits above Q3, R11 (10 k gate pull-down) below. | Now the gate wire (GDR) and the current-sense wire (RESE) each reach their resistor without crossing. |
| 4 | Panel outline, active area and window redrawn on the KEEPOUTS layer: panel −3.57 mm, window = measured **60.65 × 24.3**. | The drawings now match the copper and your calipers. |
| 5 | **R20 pull-up moved from +3V3 to VBUS_SENSE** (review S1, schematic + PCB). | With no cable in, R20 used to push about 20 µA from the battery backwards through D6 into the charger's STAT pin and the magnet contacts. Now R20 only gets power when a cable is in. The magnet contacts never see battery voltage again. Firmware reads it the same way: low = charging, high = done. With no cable it reads low, so check VBUS_SENSE first. |
| 6 | **U3 AP2112K-3.3 → RT9080-33GJ5, LCSC C841192** (review S2). Same footprint. | I checked the pinout against Richtek's datasheet DS9080-09: 1 VIN, 2 GND, 3 EN, 4 NC, 5 VOUT, the same as the AP2112K. EN may be tied to VIN. It needs ≥ 1 µF on the output (we have 10 + 22 + 22 µF). It draws 2 µA instead of 55 µA, so standby goes from about 6 weeks to about 4–6 months. The trade-off: dropout is 0.31 V typ / 0.53 V max at 600 mA. |
| 7 | **New C32 = 22 µF 0805 on +3V3** (C45783, the same basic part as C1), at (118.4, 81.0) next to U3's output (review S4). | It gives Wi-Fi transmit bursts a local reserve. |
| 8 | D7 (SMF5.0A TVS) checked (review S3). Pad 1 = VBUS = cathode, and the footprint (D_SMF) has the cathode bar on pad 1. The LCSC body (SOD-123FL) fits the D_SMF land (both about 2.8 × 1.8 mm). | Nothing changed. In JLCPCB's preview, check that the band sits on the pad toward J3/VBUS. Backwards, it would short the cable. |
| 9 | **New test points TP5 = +3V3, TP6 = BAT+, TP7 = EN** (1.5 mm pads, F side, schematic + PCB). | For first power-up you can now probe 3V3, battery, GND (TP4), EN, BOOT (TP1) and UART TX/RX (TP2/TP3) without touching chip pins. |
| 10 | **Locating-post holes H2, H4, H6, H8, H9, H10: 4.4 → 4.2 mm** (H13/H14 were already 4.2). The screw-post holes H1/H3 stay at 6.0 mm. | C7 says the locating posts are 2.9 mm, so a 4.2 hole leaves 0.65 mm of play all round. That covers the ±0.5 mm photo uncertainty. The screw posts are 3.85 mm, so 6.0 leaves 1.07 mm of play. That's needed because H1 is about 0.9 mm from where the mat photo puts its post. Which posts are screws was confirmed on the bare Casio board photo (`132b4559`): two big holes at the top, small holes elsewhere, and screw notches at the bottom. |
| 11 | New copper keep-out "antenna clearance edge strip" (x 113–115.2, y 89–105, both layers, no pour). | Espressif wants no copper beside the module's antenna. The GND pour used to run right up to the board edge there. |
| 12 | GND stitching: every GND pour island ≥ 1 mm² has 2 vias, or connects through pads and tracks where no via fits (a few small islands under J1/U4). New GND vias sit next to C19, D4, C27, R12, C32 and the C21–C30 column. | This is the same 2-vias-per-island rule as stages 6–10. |
| 13 | 63 "staircase" track chains straightened (1,780 tiny segments → 302), mostly on the camera bus. | These were left by the old grid router. Same connections and same clearances, just fewer bends. |
| 14 | New Windows-friendly tools: `tools/pilroute.py` (A* router using only KiCad's python), `tools/gnd_islands.py`, `tools/smooth.py`. | Freerouting (needs Java 17) and shapely aren't on this PC. |

## The e-paper ribbon (FPC) length budget
The panel, slot and J2 all moved by the same 3.57 mm, and only in y. So the path is unchanged:
- out of the panel's right short edge (x 179.5)
- folds 180° behind the panel
- runs back 6.6 mm to the 1.0 × 14 mm slot at x 172.9 (now y 85.83–99.83, centre 92.83)
- up through the 0.8 mm board
- into J2 (x 177.9, centre y 92.83, opening facing the slot)

It still needs the panel's 14.3 mm ribbon, and anything from 13.5 to 15 mm works (V9/E7). The panel centre (92.99) and the slot/J2 centre (92.83) are 0.16 mm apart, so the ribbon goes in straight.

## Caliper readings, how each was used
| Reading | Used? | Result |
|---|---|---|
| C1 159.9 / C2 79.3 / C3 161 × 79.3 | check only | The traced outline is 160.8 × 79.0. Close enough; the outline wasn't changed. |
| C4 wall 5.5 (4.7) at the screen section | **not applied, see Q1** | Our board is 72.3 wide there (x 113.84–186.16). With a 79.3 case and 4.7–5.5 walls, the inside would be only 68.3–69.9. |
| C5 wall 0.8 at the keypad section | check only | Board keypad section 64.13 wide. Plenty of room. |
| C6 Casio board 65.05 × 97.9 | check only | Our keypad section is 64.13 wide, 0.46 mm narrower on each side than Casio's, so it fits. I didn't widen it: no benefit, and some risk. |
| C7 posts 3.85 (screw) / 2.9 (locating) | **applied** | Hole sizes, change 10. |
| C8 "61.8 outside to outside" | not applied, Q2 | No post pair matches. |
| C9 "3.7 / 49.65" | not applied, Q3 | No pair matches 45.8 mm centre to centre. |
| C10 bottom posts 42.85 o-o, ~16.5 from walls | check | That gives 39.0 centre to centre. The board's bottom notches are 38.94 apart. ✓ |
| C11 top-left corner post 4.2 / 6.1 from the inside walls | check | The CAD replica puts it at about (117.3, 65.7) KiCad mm. The board's top-left cut-out leaves ≥ 1.1 mm round a 3.85 post there. ✓ |
| C12 window 60.65 × 24.3, C13 top edge 24.0 from the case top | **applied** | E-paper position, changes 1/4. The window's left-right centre is assumed to be the case centre (x = 150). |

## Things I checked and left alone (robustness review)
- **The camera socket J1** still uses the CamReversed footprint.
- **The stage-6 camera GPIO map** is unchanged (the camera nets weren't re-assigned, only straightened).
- **No LED.**
- **Magnet contacts:** R20 no longer back-feeds them (change 5).
- **Vias:** all 0.6/0.3 mm, a 0.15 mm ring (JLCPCB minimum 0.13). The narrowest track is 0.2 mm, and the smallest gap 0.15 mm (0.1 mm minimum).
- **USB D+/D− lengths:** 48.2 / 45.9 mm. A 2.2 mm difference doesn't matter at the ESP32's 12 Mbit/s full-speed USB (the limit is in the hundreds of mm).
- **Decoupling:** each ESP32 3V3 pin has C1/C2/C3 right at it. C19 and C23 sit at L1 and J2. C32 is new.
- **Silkscreen:** DRC has no silk-over-pad warnings, and there's no silk on B side key pads.
- **Thermal reliefs:** no starved-thermal warnings.

## Assumptions made in this stage
1. The window is centred left-right on the case (x = 150). Only its size and top edge were measured.
2. The panel's active area sits in the middle of the glass. This is the same as in earlier stages. If your panel's active area is off-centre, the picture shifts by that amount.
3. H1 stays at (128.36, 126.74). The 6.0 mm hole covers the 0.9 mm photo offset of a 3.85 mm screw post.
4. C4's thick wall is either the stubs or a local feature, not a solid wall along the whole screen section (Q1).
5. A few small GND islands under J1/U4 have fewer than 2 vias. They connect through pads and tracks, and no via fits there.

## Firmware to-do (from the review)
- **CHG_STAT (IO35):** with R20 now on VBUS_SENSE, read STAT only while VBUS_SENSE (IO37) is high. With no cable it reads low, which does not mean "charging".
- **S4:** cap Wi-Fi TX power (for example `esp_wifi_set_max_tx_power(44)`, about 11 dBm), and keep the AI lock-out below about 3.6 V on battery. The RT9080 dropout is a bit higher than the AP2112K's.
- **S6:** before deep sleep, drain the TCA8418 FIFO and clear INT_STAT. Otherwise INT stays low and R15 draws 330 µA.
- **S6:** set the camera GPIOs to input with no pulls before cutting CAM power. Never drive PWDN (IO48) high while the camera is off. Turn off USB-Serial-JTAG when there's no cable.

## Questions for Nirav
These are also in `QUESTIONS_AND_ISSUES.md` → "PCB (stage 11)".
1. **(blocks order) C4.** Is the 5.5 / 4.7 mm wall solid along the whole screen section? If yes, the board's top part (72.3 mm wide) won't fit and needs to shrink by about 2 mm per side, which means moving the ESP32. Please measure the **inside width of the front shell at the screen section, wall to wall, at the height where the board edge sits**, and note where any stubs are.
2. **C8 "61.8 outside to outside":** which two posts?
3. **C9 "3.7 / 49.65":** which two posts, and is 3.7 a diameter?
4. **E7:** the panel's ribbon length, panel edge to tip (13.5–15 mm works).
5. **Is the window centred left-right on the case?** If not, measure from the left edge of the window to the left edge of the case.
6. **The window (60.65) is 1.4 mm wider than the panel glass (59.2).** You'll see about 0.5–0.8 mm of whatever is behind the glass at each end. Is a black mask sticker OK?
