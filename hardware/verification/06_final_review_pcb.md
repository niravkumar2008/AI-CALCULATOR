# 06 — Final adversarial review of PCB v14 (before ordering)

Date 2026-10-06, 01:20–09:10 USEDT. Read-only: **nothing under `hardware/kicad/`, `hardware/fab/` or `hardware/bom/` was changed** (no v15, no fab regeneration, no polish — the coordinator cut those unless a FATAL turned up, and none did). All work ran on scratch copies. Everything below was re-derived from the raw files (`.kicad_pcb` parsed with my own s-expression parser, netlist exported with `kicad-cli`, the fab CSVs, the module drawings), not from the earlier docs.

## Verdict: **GO** (no FATAL, no SERIOUS found; nothing in the files needs to change before ordering)

Two things still have to be proven on the bench, exactly as the earlier docs already say: the paper dry fit of the post holes (plus one extra tab for the ESP32 antenna overhang, see M1), and the camera-ribbon / magnet-cable meter checks when the parts arrive. Neither can be settled from the files.

---

## Findings, ranked

### FATAL
None.

### SERIOUS
None.

### MINOR

**M1. The ESP32 antenna overhang is not on the paper dry-fit map.**
- Evidence: U1 at (127.55, 97.0) rot 90; its pads span x 120.55–134.55 but the module body runs to about x 114.7 (20.5 mm long module). The board edge there is at x 118.9 (Edge.Cuts 118.9, 82.9 → 118.9, 114.0). So about 4.2 mm of module sticks out past the board edge, 35.3 mm from the board's centre line (x 150).
- Caliper C4 gave the screen-section wall as 4.7–5.5 mm *including the wire-holder stubs* (inside width 69.9–68.3 mm), and photo 871567b6 says those stubs stop above the board plane. If that is right, the module clears; if the wall really is 4.7 mm at the module's height (the module is 2.4 mm tall on the F side), the module hits the wall and the board can't seat.
- Failure mode: board can't be closed; fix is filing the inside of the wall at that spot (plastic), not a board change.
- Fix now (no board change): when Nirav does the paper dry fit, add a 4.5 × 15.4 mm tab of paper on the left edge between y 90 and 104.5 (board coordinates), i.e. the module outline (exact outline: x 114.4–118.9, y 89.3–104.7, as drawn in `Claude outputs/paper_dry_fit_v14.svg`; note added by 11), and check it clears the wall.

**M2. About 1.25 mm of GND pour lies under the root of the antenna.**
- Evidence: the module's GND pad row 46–60 is at x 120.55 (pad edge 120.15); the GND zone on both layers fills out to x 119.2 for y 89.3–104.7 (the two "antenna clearance" keep-outs only cover y 85–89.3 and 104.7–109). Espressif asks for no copper under the antenna area (the part of the module beyond the GND pad row).
- Failure mode: slight detuning / a dB or two less range. Wi-Fi still works. Not worth a board change for this run; for v15 extend the two keep-outs to join (x 118.4–119.8, y 85–109).

**M3. Pin-1 silk marks on J1, J2, J4, L1 and Q3 are 0.06 mm lines** (JLCPCB prints ≥ 0.15). Cosmetic: the JLCPCB preview, `fab/render_top.png` and the assembly PDF are the references, not the silk. Same as 04 C1.

**M4. Camera decoupling is 9.5–11 mm from J1** (C13/AVDD 9.6, C12/DOVDD 9.5, C16/AF 10.9, C18/RESET 10.0 mm). The module has its own caps on the sensor; this works (XIAO Sense does the same), just not tight.

### NOTE

**N1. J1 camera socket (`_CamReversed`): my own re-derivation says it is correct; confidence ~90 %, same as before.**
- From the Mingjiaying MJY5OAF-F3M-V1 drawing (`OV5640_Camera_Module_Specification.pdf`, re-fetched tonight, page rendered at 6×): the pin table is 1 STROBE, 2 AGND, 3 SDA, 4 AVDD2.8, 5 SCL, 6 RST, 7 VSYNC, 8 PWDN, 9 HSYNC, 10 DVDD1.5, 11 DOVDD, 12 Y9, 13 MCLK, 14 Y8, 15 GND, 16 Y7, 17 PCLK, 18 Y6, 19 Y2, 20 Y5, 21 Y3, 22 Y4, 23 AF-GND, 24 AFVDD2.8. Fingers 0.3 wide at 0.5 pitch, 11.5 wide (= C6364666 land pattern: 0.3 × 1.2 pads at 0.5). FPC 70.5 ± 0.2 overall **including the 8.5 mm module**.
- The *Bottom view* (foam side, gold fingers visible, cable running to page-left) marks "1" at the page-bottom edge of the fingers and "24" at the top. Flipping to the *Top view* (lens up) about the cable axis puts pin 1 at the page-top edge. On the board the lens faces the back cover (F side) and the cable runs +y into J1, so the top view is turned 90° CCW: pin 1 ends up at the smaller x. J1 pad 1 is at x 144.25 and pad 24 at x 155.75: **finger N lands on pad N, every function matches the net** (J1 nets: 1 NC, 2 GND, 3 SIOD, 4 AVDD, 5 SIOC, 6 RESET, 7 VSYNC, 8 PWDN, 9 HREF, 10 DVDD, 11 2V8, 12 D7, 13 XCLK, 14 D6, 15 GND, 16 D5, 17 PCLK, 18 D4, 19 D0, 20 D3, 21 D1, 22 D2, 23 GND, 24 AF).
- If it is wrong anyway (the ribbon is folded once on its way, or Seeed ships a mirrored variant): finger 15 (GND) meets CAM_DVDD, AVDD/DOVDD meet data pins at ≤ 2.8 V, no over-voltage; the camera is just dead. Recovery is a 180° twist in the ribbon (J1 is dual-contact), no board change. Keep the 2 ↔ 15 beep test.

**N2. Camera ribbon has ~1.4 mm spare, not 10 mm** (03 L1 re-confirmed: 70.5 includes the module). Decision rule in `stage14_verification_fixes.md` §6 stands.

**N3. Power tree re-derived from the netlist, all correct:** VBUS → D1 (A VBUS, K SYS) → SYS; VBUS → U2 VDD (pin 4), U2 VBAT (3) = VBAT_P, PROG (5) = R2 20 k → **50 mA** (0.33 C for the 150 mAh #1317, 0.5 C for a 100 mAh #1570; both OK); VBAT_P → Q1 S, Q1 D = BAT+ = J4.2, Q1 G = R3 100 k to GND (reversed cell blocked); Q2 G = VBUS, S = SYS, D = VBAT_P (off on USB, on from battery); SYS → U3 VIN/EN → +3V3; SYS → U4/U5 VIN, CE = CAM_PWR_EN (R10 100 k down). Nothing connects the battery to the magnet contacts (D1 blocks, Q2 body diode points VBAT_P→SYS, charger blocks). Backwards magnet piece = VBUS↔GND swap into D7 forward, host current-limits; 5 V never reaches a data line (05 C1, 08 §2.3). 3V3 rail peak ≈ 385 mA (Wi-Fi 355 + e-paper 30) vs RT9080 600 mA; the camera (≤ 150 mA) is on SYS, not on the 3V3 rail. Near an empty cell (3.3 V) a full-power TX burst can pull +3V3 to ~3.0 V: the firmware's 3.6 V AI lock-out, 11 dBm TX cap and "camera off before Wi-Fi" rules remain required (02 C1).

**N4. Mount holes vs calipers:** H1–H3 46.50 (C8 46.5 ✓), H8–H10 42.40 (C8 42.3 ✓), H2–H9 38.35 and H13–H14 38.60 (photo pairs 38.3/38.6 ✓), row spacing 10.7–11.3 mm. C15 rows (case top at KiCad y ≈ 56.9): H1 69.2 vs 69.95, H8 80.1 (row not in C15), H4 89.1 vs 89.6, H2 119.35 vs 120.45, H6 130.1 vs 130.2, H13 141.4 vs 142.4. Residuals ≤ 1.1 mm, all inside the 4.2/6.0 mm holes' 0.65–1.07 mm radial play and the slots on H2/H9/H13/H14. The paper dry fit is still the final word (H6 tightest).

**N5. Screen window vs panel:** KEEPOUTS rect 119.675–180.325 × 80.95–105.25 = 60.65 × 24.3 (C12 ✓), top edge at y 80.95 = 24.05 below the case top (C13 ✓), centred x 150. Active-area rect 123.17–171.72 × 81.136–104.836 = 48.55 × 23.70 (2.13" SSD1680 panel ✓), centre y 92.99 vs window 93.10. J2 pads at y 87.08–98.58 on the F side at x 179.48: inside the window's y span but on the component side, so not visible. 1.0 × 14.0 mm slot at x 172.4–173.4, y 85.83–99.83 ✓.

**N6. Heights:** nothing new. J4 5.5 and camera 5.4 under the 5.7 limit only after the solar-box / rib-B grinds (stage 13); J3 header 2.5.

**N7. CHG_STAT / VBUS_SENSE:** 10 k/20 k gives 3.33 V on IO37 at 5.0 V (≤ VDD + 0.3) and feeds R20; fine.

**N8. E-paper booster = SSD1680 reference:** L1 68 µH +3V3→SW, Q3 Si1308 G=GDR/S=RESE/D=SW, R11 10 k, R12 3 Ω, C20 4.7 µF/50 V SW→PUMP, D3 SW→PREVGH, D4 PUMP→GND, D5 PREVGL→PUMP, 1 µF/50 V on pins 4, 5, 18–24, 15/16 = +3V3, 8 (BS1) = GND, 6/7 NC. J2 pad N = panel pin N (rot −90, pad 1 at y 87.08).

**N9. Stock on JLCPCB could not be re-checked tonight** (the part pages show "Extended" for U1, U3 and J3 but no live quantity; the fetch quota then ran out). 04 saw 1,888 × U1 on 10-04. If JLCPCB flags U1 out of stock at upload, order the bare PCBs anyway and ask for a pre-order of C3013941; no substitute module (an N8R8 would lose IO33–37).

---

## Checks done and results

| # | Check | How | Result |
|---|---|---|---|
| 1 | ERC | `kicad-cli sch erc --severity-all` on a copy | **0** on all 7 sheets |
| 2 | DRC | `kicad-cli pcb drc --refill-zones --schematic-parity --severity-all` | **0 violations, 0 unconnected, 0 parity** |
| 3 | Board pads vs schematic nets | every numbered pad of 147 footprints vs the kicad-cli netlist | **0 mismatches** |
| 4 | U1 ESP32-S3-MINI-1-N4R2 pin table | netlist vs datasheet Table 3-1 and `pins_final.h` | all 65 pads match; IO26 NC; IO0/3/45/46 free; IO19/20 = USB only |
| 5 | U2 MCP73831 / U3 RT9080 / U4, U5 ME6211 / U6 TCA8418 / U7 USBLC6 / Q1–Q3 / D1–D7 / L1 / FB1 | netlist pin functions vs datasheets | all match (table in N3; D7 pad 1 = K = VBUS) |
| 6 | J1 camera pin order and CamReversed | Mingjiaying drawing re-derived (N1) | correct, ~90 % |
| 7 | J2 e-paper pin order vs Waveshare/Good Display 2.13 V4 | netlist | all 24 match |
| 8 | J3 magnet order vs #5412 cable | 05 §2 / 08 §2.3 re-read; nets 1 VBUS, 2 D−, 3 D+, 4 GND; silk N/+ at pin 1, − at pin 4 | correct; bench meter check stays mandatory; item (a) closed in 08 |
| 9 | J4 polarity | pad 1 (x 141.6) GND, pad 2 BAT+; stock JST S2B-PH geometry | matches Adafruit convention; meter-check the red wire |
| 10 | Strapping, EN RC, BOOT/TP, recovery | netlist | IO0 → TP1 only; EN = R1 10 k + C4 1 µF; TP1→TP4 + tap TP7 works with the battery in |
| 11 | USB D+/D− | tracks | 35.1 / 32.5 mm, 3 vias each, side by side, USBLC6 in path |
| 12 | Power tree, charge current, 3V3 budget, magnet reversal | netlist + datasheets | N3, all OK |
| 13 | Keypad matrix | 50 SW footprints, pad nets | 50 unique (ROW, COL) pairs, no duplicates, all on B.Cu only; ON = KEY_ON/GND |
| 14 | Key pad geometry | SW26 / SW1 / SW46 pads | 0.45 mm fingers, 0.75 mm alternating pitch (0.3 gap), 9×7 / 6×4.5 / 5×4 pads; carbon pills 4.5–6 mm bridge several finger pairs |
| 15 | SW1 vs H3 | hole-to-copper script | 0.25 mm min everywhere (zone rule); no copper closer than 0.25 mm to any NPTH; slots 0.30 |
| 16 | Antenna area | copper listing x < 121.5, y 84–112 | board edge at 118.9 = antenna overhangs; 1.25 mm GND sliver at the root (M2); keep-outs present top/bottom |
| 17 | Camera XCLK / data trace lengths | track sums | XCLK 105.7, PCLK 94.3, D0–D7 79–111, VSYNC 132.6, HREF 114.6 mm, all F.Cu, no vias; skew ≤ 0.1 ns, fine at 20 MHz |
| 18 | SPI to e-paper, I2C | track sums | EPD_CLK 68, DIN 77, CS 72 mm (4 MHz); I2C 59/63 mm with 4.7 k pull-ups to +3V3; SCCB 4.7 k to CAM_2V8 |
| 19 | Decoupling placement | pad-to-pin distances | C2 2.6 mm from U1 3V3, C3 3.8, C1 4.3; C31 3.2 from U6 VCC; LDO caps 2.0–2.5 mm; camera caps 9.5–11 mm (M4) |
| 20 | BOM vs CPL vs schematic | fab CSVs vs netlist | 77 = 77 = 77 designators, every LCSC and footprint equal; SW/TP/H/FID excluded |
| 21 | CPL positions and rotations | CPL vs board + `jlc_cpl.py` table | 0 mismatches; offsets +180 Q1/Q2/U4/U5/J3, +270 U2/U3/U7 (08 re-verified against EasyEDA) |
| 22 | Outline | Edge.Cuts | 139 segments, closed, 71.65 × 148.76 mm, slot 1.0 × 14.0 |
| 23 | Silk | widths | 0.12–0.25 mm except 0.06 mm pin-1 marks (M3); "AI CALC v14 2026-10-04" and JLC mark present |
| 24 | Window / active area / slot | KEEPOUTS layer vs C12/C13 | N5, OK |
| 25 | Post holes vs calipers | hole centres vs C8/C15/photo pairs | N4, OK within hole play; dry fit still required |
| 26 | Heights vs D1/D4 | stage 13 table | unchanged, grinds required |
| 27 | Live JLCPCB stock | web | not verifiable tonight (N9) |
| 28 | 08_second_opinion_pcb.md | read | no FAIL; item (a) closed; no disagreement with this review |

## What Nirav does before paying

1. Paper dry fit (FINAL_STATUS §1) **plus the antenna tab** (M1).
2. Upload `fab/` exactly as it is; rotate nothing in the preview.
3. When parts arrive: J4 red wire to "+", camera fingers 2↔15 beep, magnet +5 V leg into J3 pin 1 by meter before gluing.
