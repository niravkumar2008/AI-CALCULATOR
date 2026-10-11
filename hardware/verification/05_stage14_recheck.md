# 05: Independent re-check of stage 14 (v14) before the one-shot order

Date: 2026-10-04. Read-only. All work ran on copies in a scratch folder. No design file was modified and nothing was committed.

Tools: KiCad 10.0 `kicad-cli` and `pcbnew`; easyeda2kicad 1.0.1 (in a scratch venv); shapely for the geometry.

## Verdict: **GO** for ordering (PCB + full assembly)

**No FATAL items were found.**
- **Schematic changes vs. the stage-13b backup:** only the intended ones. J3 pins 2/3/4 were re-netted, SW1 got its new footprint, and the SW4/SW5 values changed.
- **Board changes:** also only intended. The J3 re-wire (4 tracks out, 6 in, 1 via out, 2 in), the SW1 notch, the v14 silk, "N / + / −", and the KEYPADS/KEEPOUTS user-layer labels.
- **Nothing else moved.** Setup/stackup, zones and Edge.Cuts are byte- or geometry-identical. All 147 footprints have the same position, rotation and layer.
- **J3 pin order:** re-derived from Adafruit's own #5412 drawing (downloaded fresh from cdn-shop.adafruit.com) and agrees: **1 VBUS, 2 D−, 3 D+, 4 GND, N end at pin 1.**
- **CPL rotations:** pass for all 22 polarised or multi-pin parts, checked against EasyEDA footprints that I fetched per LCSC number.
- **DRC and ERC:** DRC is 0 / 0 / 0 with parity (zones refilled). ERC is 0.

**Conditions** (not blockers; already in `ORDER_WALKTHROUGH.md`):
- The magnet meter check before gluing stays **mandatory**. See C1 and C2.
- Check the JLCPCB placement preview against the walkthrough's step-5 table.

---

## Findings

| # | Level | Finding |
|---|---|---|
| F-none | FATAL | None. |
| L-none | LIKELY PROBLEM | None. |
| C1 | CHECK | **A backwards magnet fit is still harmful, just not to data lines.** The piece glued in reversed puts host 5 V on board GND and host GND on board VBUS. D7 (SMF5.0A) then conducts forward, so board VBUS sits about 0.8–1.2 V *below* board GND. The MCP73831 (U2) VDD pin (abs. min about −0.3 V) and Q2's gate see that negative voltage until the source current-limits. A computer port limits quickly. **A phone charger may supply 2 A or more into D7 and U2's ESD diode**, so damage is possible. No pin order avoids this: the cable has power at one end and GND at the other. The walkthrough's function-based meter procedure (find the +5 V leg, put it in pin 1) prevents it. Don't skip it. |
| C2 | CHECK | **The data order rests on one Chinese factory drawing (5412_C17238_4P.pdf).** If the real cable has D+/D− the other way round, nothing is damaged, but USB won't enumerate and the board can't be re-wired. The meter procedure's step 3 detects this before gluing. Charging still works either way. |
| C3 | CHECK | **Q1/Q2 residual is 0.213 mm**, just over the 0.2 mm criterion. This is a land-pattern difference, not a rotation error: EasyEDA's SOT-23 "LS2.4" puts the pad centres about 0.2 mm further in along the lead axis than KiCad's SOT-23. Pin 1→1, 2→2 and 3→3 all match. A wrong rotation gives 2.82 mm. **PASS.** |
| C4 | CHECK | **J1 numbering is mirrored on purpose** (CamReversed; memory and docs say don't undo). Geometry is exact: all 26 pads incl. tabs within 0.00 mm at 180°, versus 3.16 mm at 0°, so the body orientation is right. The order notes already tell JLCPCB not to rotate it. |
| C5 | CHECK | **J3's 4 pads are symmetric.** Direction comes from the body outline: EasyEDA silk/courtyard transformed at CPL 180° spans y 60.8–72.9, the same side as KiCad's courtyard (61.5–73.8, over the top edge). At 0° it would land at y 71–83, over D1/D6/U7. The walkthrough tells the reviewer to check this. |
| C6 | CHECK | **The "MAGNET 5358 … 4 pins @2.50" note** is unchanged from 13b. The #5358 legs are at 2.50 mm and J3 is a 2.54 mm socket: 0.12 mm accumulated over 4 legs, fine for straightened legs. |
| OK1 | OK | Schematic netlist diff (kicad-cli, copies of both): 134 parts, 92 nets in both. The only pin-net changes are J3.2 GND→USB_DM, J3.3 USB_DM→USB_DP, J3.4 USB_DP→GND. Connectivity sets otherwise identical. |
| OK2 | OK | Board pad→net diff: only J3.2/3/4 (same as the schematic) and the SW1 pad geometry (all still COL0/ROW0). |
| OK3 | OK | **USB path.** J3.3 USB_DP → U7 pins 1 and 6 (USBLC6 I/O1 pair) → U1 pin 24 = IO20 = USB_D+. J3.2 USB_DM → U7 pins 3 and 4 (I/O2 pair) → U1 pin 23 = IO19 = USB_D−. U7.2 = GND, U7.5 = +3V3 (the VBUS rail pin of the clamp, the usual way to connect it). Checked against the ESP32-S3-MINI-1 datasheet table (IO0 = pin 4 … IO19 = 23, IO20 = 24) and the ST pinout. DRC parity 0 and 0 unconnected confirm the copper. |
| OK4 | OK | **Silk.** "N" (120.69–121.91, 68.91–70.49) and "+" (120.61–121.99, 70.51–72.09) sit left of J3 pin 1; "−" (132.91–134.29, 70.81–72.39) sits right of pin 4. No silk text overlaps any pad's mask opening (0.05 mm margin). "AI CALC v14 2026-10-04" is present, and the v13 text is gone. |
| OK5 | OK | **Copper to hole edge** (my own check: pads, tracks, vias, refilled zones, graphics, both layers, against every hole; NPTH vs any net, PTH/vias vs other nets): minimum **0.250 mm** (GND pour at H1/H3/H4/H6/H8/H10, set by rule). H2/H9/H13/H14 are at 0.300. Vias vs other nets are ≥ 0.302. **SW1 to H3: 0.055 mm (13b) → 0.282 mm (v14).** |
| OK6 | OK | **Copper to board edge** (incl. slots and notches): minimum 0.30 mm (GND pours, at the rule; 0.298 shows as polygon chord error). The closest track is 0.40 mm (EPD_VDD); the closest pad is 0.45 mm (C32). No copper is outside the outline. |
| OK7 | OK | **Keypad.** All 50 SW footprints (100 pads) keep their row/column nets in both the schematic and the board. U6 (TCA8418) pin nets are identical. Firmware `kMatrix` only renames Abs→Calc and Cube→Integral in the same slots. |
| OK8 | OK | **BOM.** 77 individual designators, no ranges, no duplicates. Every CPL designator (77) has a BOM line and vice versa. Every BOM LCSC number and footprint matches the schematic netlist and the board footprint's LCSC field. CPL X/Y equal the board positions (Y negated); all parts are Top. |
| OK9 | OK | **Outputs match the board.** The saved zone fill equals a fresh refill (no change). The fab PTH drill has 333 holes, equal to the board's 333 vias, and includes the new vias (127.3, −74.0) and (131.31, −69.0) but not the removed (128.4, −73.6). |
| OK10 | OK | **DRC ignores.** `.kicad_pro` is identical to 13b. The only DRC checks set to ignore are footprint_filters/type mismatch, missing_courtyard, npth_inside_courtyard, track_not_centered_on_via and tuning_profile, as before. `lib_footprint_mismatch` is still active and passes, so the new SW1 footprint equals its library copy. |

---

## 1. Regression diff, stage 13b backup vs. v14

### Schematic

Only `connectors.kicad_sch` and `keypad.kicad_sch` differ in the file text. `ai_calc`, `mcu`, `power`, `camera` and `epaper` are byte-identical.

| Item | 13b | v14 | Class |
|---|---|---|---|
| J3 value | "…5358): VBUS GND D- D+" | "…5358, N end at pin 1): VBUS D- D+ GND" | intended (J3) |
| J3.2 net | GND | USB_DM | intended (J3) |
| J3.3 net | USB_DM | USB_DP | intended (J3) |
| J3.4 net | USB_DP | GND | intended (J3) |
| SW1 footprint | KeyPad_6.0x4.5 | KeyPad_6.0x4.5_H3notch | intended (SW1 notch) |
| SW4 value | Abs | CALC | intended (key names) |
| SW5 value | x^3 | ∫dx | intended (key names) |

Every other component (value, footprint, LCSC) and every other pin→net is identical.

### Board

The setup/stackup header is byte-identical. All 147 footprints have the same x, y, rotation, layer and footprint ID, except SW1's footprint ID. Zones (57) and Edge.Cuts/graphics are unchanged.

| Item | Change | Class |
|---|---|---|
| J3 pads 2/3/4 | nets as in the schematic | intended |
| SW1 COL0 bottom bar | 6.0 × 0.6 bar → 4.423 × 0.6 (x 175.20–179.62) + 1.677 × 0.35 (x 173.62–175.30, top edge unchanged at y 122.445); fingers unchanged | intended (notch) |
| Track removed | F.Cu 131.31,72 → 129.8,73.1 USB_DP (old pad 4) | intended |
| Track removed | F.Cu 126.23,72 → 126.18,71.4 GND (old pad 2) | intended |
| Track removed | F.Cu 128.77,72 → 128.4,73.6 USB_DM (old pad 3) | intended |
| Track removed | B.Cu 128.4,73.6 → 127.5,74.8 USB_DM | intended |
| Via removed | (128.4, 73.6) USB_DM | intended |
| Tracks added | DP: 128.77,72 → 128.77,72.6 → 129.8,73.1 (existing via) | intended |
| Tracks added | DM: 126.23,72 → 126.23,73.3 → 127.3,74.0 + B.Cu 127.3,74.0 → 127.5,74.8 | intended |
| Tracks added | GND: 131.31,72 → 131.31,69.0, 0.3 mm | intended |
| Vias added | (127.3, 74.0) USB_DM | intended |
| Vias added | (131.31, 69.0) GND | intended |
| F.Silkscreen | "AI CALC v13 2026-10-03" → "AI CALC v14 2026-10-04"; added "N", "+", "−" | intended |
| KEYPADS user layer | "Abs" → "CALC", "x^3" → "∫dx" | intended |
| KEEPOUTS user layer | magnet note text updated | intended |

**Unintended changes: none.**

## 2. J3 magnet connector

**Cable (#5412).** I fetched `5412_C17238_4P.pdf` fresh from Adafruit's product page and rendered the face view at 8×.
- A leader line runs from each pogo pin to its own label.
- Reading from the **N** magnet to the **S** magnet: **−, D+, D−, +**.

**Board piece (#5358, MG04254FRA1S1N).**
- N and S magnets at the ends, 4 flat contacts at 2.50 mm between them.
- The drawing gives no pin functions. "1S1N" means one S and one N magnet.
- Contact order along the line is intrinsic: it doesn't depend on which side you view the face from.

**Mating.** The faces meet face to face, and opposite poles attract, so the cable's N end sits on the piece's S end. Each contact touches the one directly opposite it.
- From the piece's **S** end, the piece reads GND, D+, D−, VBUS.
- So from its **N** end it reads **VBUS, D−, D+, GND**.
- The straightened legs sit behind their contacts, so the leg order is the same.
- **This matches the board: N at pin 1, giving 1 VBUS, 2 USB_DM, 3 USB_DP, 4 GND.**
- The silk "N" and "+" at pin 1 and "−" at pin 4 are correct.

**Possible fits:**
- **Cable reversed on the piece:** the magnets repel, so it can't mate.
- **Piece fitted correctly (N at pin 1):** works.
- **Piece flipped (N at pin 4):**
  - Pin 1 gets GND, pin 4 gets VBUS, so the supply is reversed (see C1).
  - Pin 2 gets D+ and pin 3 gets D−, so the data lines are swapped: no enumeration, no damage.
  - **No 5 V reaches a data line.**
- **Offset by one position:** impossible, because there are 4 legs in a 4-way socket.
- **Under the old 13b wiring,** one of the two fits put 5 V on USB_DP. That is fixed.

The walkthrough's procedure finds the +5 V leg by measurement rather than by trusting the N/S marks. It is therefore robust even if the drawing's magnet labelling is wrong.

## 3. CPL rotations (EasyEDA footprints, fetched per LCSC number)

**Method:**
- Transform: KiCad Y = −(CPL Mid Y).
- Rotation is counter-clockwise on screen about the CPL point: x' = X + x·cosθ + y·sinθ, y' = Y − x·sinθ + y·cosθ.
- "pin1" is the distance from EasyEDA pad 1 to KiCad pad 1.
- "worst #" is the worst distance between same-numbered pads, over numeric pads present in both.
- "geom" is the worst distance from any pad to the nearest pad on the other footprint, in both directions.
- **Negative test:** with the KiCad rotation instead of the CPL rotation, Q1/Q2 fail at 2.82 mm, U2/U3/U7 at 2.11 mm and U4/U5 at 3.09 mm. The check therefore discriminates.

| Ref | LCSC | EasyEDA footprint | KiCad rot | CPL rot | pin1 (mm) | worst # (mm) | geom (mm) | Result |
|---|---|---|---|---|---|---|---|---|
| D1–D6 | C8598 | SOD-123_L2.7-W1.6-LS3.7-RD-1 | 0 | 0 | 0.05 | 0.05 | 0.05 | PASS |
| D7 | C193402 | SOD-123FL_L2.7-W1.8-LS3.8-RD | 180 | 180 | 0.18 | 0.18 | 0.18 | PASS (pad 1 = K = VBUS, band toward J3) |
| J1 | C6364666 | FPC-SMD_24P-P0.50_FPC-0.5-24P-HYH2.0 | 180 | 180 | 11.5 | (mirrored) | 0.00 (3.16 at +180) | PASS: numbering intentionally mirrored (C4) |
| J2 | C6364666 | same | 270 | 270 | 0.00 | 0.00 (pins 1–24) | 0.00 (3.16 at +180) | PASS |
| J3 | C46061768 | CONN-SMD_4P-P2.54_HX-PM2.54-1X4PWT | 0 | 180 | 7.62 | symmetric | 0.00 | PASS: body direction checked by outline (C5) |
| J4 | C295747 | CONN-SMD_P2.00_S2B-PH-SM4-TB-LF-SN | 0 | 0 | 0.00 | 0.00 (pins 1–2) | 0.00 (2.35 at +180) | PASS |
| L1 | C135265 | IND-SMD_L4.0-W4.0_SMNR4020 | 180 | 180 | 0.00 | 0.00 | 0.00 | PASS |
| Q1 | C15127 | SOT-23_L2.9-W1.3-P1.90-LS2.4-BR | 0 | 180 | 0.213 | 0.213 | 0.213 | PASS (C3; 2.82 if wrong) |
| Q2 | C15127 | same | 90 | 270 | 0.213 | 0.213 | 0.213 | PASS (C3) |
| Q3 | C469327 | SOT-323_L2.0-W1.3-P1.30-LS2.1-BR | 0 | 0 | 0.00 | 0.00 | 0.00 | PASS |
| U1 | C3013941 | BULETM-SMD_ESP32-S3-MINI-1-N8 | 90 | 90 | 0.02 | 0.03 (60 numbered pads) | 0.03 | PASS (geometry symmetric under rotation; numbering decides) |
| U2 | C424093 | SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BL | 0 | 270 | 0.012 | 0.013 | 0.013 | PASS |
| U3 | C841192 | TSOT-23-5_L2.9-W1.6-P0.95-LS2.8-BL | 0 | 270 | 0.013 | 0.013 | 0.013 | PASS |
| U4 | C53099 | SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BR | 0 | 180 | 0.162 | 0.163 | 0.163 | PASS |
| U5 | C53100 | SOT-23-5_L3.0-W1.7-P0.95-LS2.8-BR | 0 | 180 | 0.162 | 0.163 | 0.163 | PASS |
| U6 | C138713 | WQFN-24_L4.0-W4.0-P0.50-TL-EP2.5 | 0 | 0 | 0.083 | 0.083 | 0.083 (the extra KiCad items are EP paste sub-pads inside the EP) | PASS |
| U7 | C7519 | SOT-23-6_L2.9-W1.6-P0.95-LS2.8-BL | 0 | 270 | 0.013 | 0.013 | 0.013 | PASS |

**Rotation offsets in the CPL vs. KiCad:**

| Offset | Parts |
|---|---|
| +180 | J3, Q1, Q2, U4, U5 |
| +270 | U2, U3, U7 |
| 0 | everything else |

The 2-pad passives (R, C, FB1) are non-polar and not checked.

## 4. DRC / ERC (on copies)

- `kicad-cli pcb drc --refill-zones --schematic-parity --severity-all`: **0 violations, 0 unconnected, 0 parity.**
- `kicad-cli sch erc --severity-all`: **0.**

## 5 and 6. Board-wide geometry and keypad

See OK4–OK7 above.

## Out of scope (seen in passing, not changed in stage 14)

J4: pad 1 = GND, pad 2 = BAT+. That is the usual Adafruit JST-PH convention, and it was covered by the earlier verifications.

## Source files

- Scratch copies and scripts: `%TEMP%\claude\…\scratchpad\rc\` (`pcbdump.py`, `netdiff.py`, `geo.py`/`clr.py`, `cpl.py`/`cpl2.py`, `silk.py`).
- Drawings: `rc/5412.pdf` and `rc/5358.pdf` (Adafruit CDN).
